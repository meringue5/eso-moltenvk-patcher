#!/usr/bin/env python3

from __future__ import annotations

import unittest

from check_startup_log import evaluate_startup_log, run_epoch


RUN = "20260719T130000.123456789Z-pid70000"


def record(message: str, run_id: str = RUN) -> str:
    return f"[run={run_id}] {message}"


def good_log() -> str:
    return "\n".join(
        [
            record("RUN_START: bridge starting pid=70000"),
            record("MOLTENVK: loaded path=/sanitized/libMoltenVK.teso4m4.dylib"),
            record("HDR_COMPAT: filter=enabled extension=VK_EXT_hdr_metadata"),
            record(
                "HDR_SURFACE_COMPAT: filter=enabled format=64 colorSpace=1000104008"
            ),
            record("ACTIVE: redirected 17 Vulkan entry points"),
            record(
                "GIPA: instance=0x1 name=vkEnumerateDeviceExtensionProperties raw=0x2 returned=0x3 shim=hdr-filter"
            ),
            record(
                "GIPA: instance=0x1 name=vkCreateDevice raw=0x4 returned=0x5 shim=device-trace"
            ),
            record(
                "GIPA: instance=0x1 name=vkGetPhysicalDeviceSurfaceFormatsKHR raw=0x6 returned=0x7 shim=surface-format-filter"
            ),
            record(
                "HDR_FILTER: physical=0x1 raw=131 visible=130 removed=1 query=count result=0"
            ),
            record(
                "HDR_FILTER: physical=0x1 raw=131 visible=130 removed=1 query=data capacity=130 written=130 result=0"
            ),
            record(
                "SURFACE_FORMAT_FILTER: physical=0x1 surface=0x3 raw=60 visible=59 removed=1 query=count result=0"
            ),
            record(
                "SURFACE_FORMAT_FILTER: physical=0x1 surface=0x3 raw=60 visible=59 removed=1 query=data capacity=59 written=59 result=0"
            ),
            record("CREATE_DEVICE: call=1 physical=0x1 extensions=2 hdr_enabled=no"),
            record("CREATE_DEVICE_EXT: call=1 index=0 name=VK_KHR_swapchain"),
            record("CREATE_DEVICE_EXT: call=1 index=1 name=VK_KHR_maintenance1"),
            record("CREATE_DEVICE_RESULT: call=1 result=0 device=0x2"),
        ]
    )


class StartupLogTests(unittest.TestCase):
    def test_accepts_complete_filtered_run(self) -> None:
        verdict = evaluate_startup_log(good_log())
        self.assertTrue(verdict.passed, verdict.reasons)

    def test_rejects_hdr_proc_query(self) -> None:
        text = good_log() + "\n" + record(
            "GDPA: device=0x2 name=vkSetHdrMetadataEXT result=(nil) [NULL]"
        )
        verdict = evaluate_startup_log(text)
        self.assertFalse(verdict.passed)
        self.assertIn(
            "ESO still queried vkSetHdrMetadataEXT after HDR filtering",
            verdict.reasons,
        )

    def test_rejects_hdr_enabled_device(self) -> None:
        text = good_log().replace("hdr_enabled=no", "hdr_enabled=yes")
        verdict = evaluate_startup_log(text)
        self.assertFalse(verdict.passed)
        self.assertIn(
            "at least one VkDevice enabled VK_EXT_hdr_metadata", verdict.reasons
        )

    def test_time_gate_rejects_stale_run(self) -> None:
        cutoff = run_epoch(RUN)
        assert cutoff is not None
        verdict = evaluate_startup_log(good_log(), after_epoch=cutoff + 1)
        self.assertFalse(verdict.passed)
        self.assertIsNone(verdict.run_id)

    def test_rejects_missing_filter_evidence(self) -> None:
        text = "\n".join(
            line for line in good_log().splitlines() if "query=data" not in line
        )
        verdict = evaluate_startup_log(text)
        self.assertFalse(verdict.passed)
        self.assertIn(
            "no data query proved that exactly one HDR extension was removed",
            verdict.reasons,
        )

    def test_rejects_missing_surface_filter_evidence(self) -> None:
        text = "\n".join(
            line
            for line in good_log().splitlines()
            if not line.startswith(record("SURFACE_FORMAT_FILTER:"))
        )
        verdict = evaluate_startup_log(text)
        self.assertFalse(verdict.passed)
        self.assertIn(
            "no surface-format data query removed the exact ESO HDR pair",
            verdict.reasons,
        )

    def test_rejects_non_exact_removed_counts(self) -> None:
        text = good_log().replace("removed=1", "removed=10")
        verdict = evaluate_startup_log(text)
        self.assertFalse(verdict.passed)
        self.assertIn(
            "no surface-format data query removed the exact ESO HDR pair",
            verdict.reasons,
        )

    def test_rejects_incomplete_device_extension_list(self) -> None:
        text = "\n".join(
            line for line in good_log().splitlines() if "index=1" not in line
        )
        verdict = evaluate_startup_log(text)
        self.assertFalse(verdict.passed)
        self.assertIn(
            "vkCreateDevice call 1 does not contain its exact indexed extension list",
            verdict.reasons,
        )

    def test_selects_newest_run_by_timestamp(self) -> None:
        newer = "20260719T140000.123456789Z-pid70001"
        newer_log = good_log().replace(RUN, newer).replace("pid=70000", "pid=70001")
        verdict = evaluate_startup_log(newer_log + "\n" + good_log())
        self.assertTrue(verdict.passed, verdict.reasons)
        self.assertEqual(verdict.run_id, newer)

    def test_rejects_invalid_run_timestamp_without_crashing(self) -> None:
        invalid = "20261340T250000.123456789Z-pid70000"
        verdict = evaluate_startup_log(good_log().replace(RUN, invalid))
        self.assertFalse(verdict.passed)
        self.assertIn(
            "run id does not contain the required UTC timestamp and PID",
            verdict.reasons,
        )


if __name__ == "__main__":
    unittest.main()
