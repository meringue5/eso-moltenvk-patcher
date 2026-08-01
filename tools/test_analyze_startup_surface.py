#!/usr/bin/env python3

from __future__ import annotations

import unittest

from analyze_startup_surface import analyze_lifecycle, analyze_timing


RUN = "20260725T103547.901947000Z-pid25514"


def record(message: str, run_id: str = RUN) -> str:
    return f"[run={run_id}] {message}"


def create(generation: int, old_generation: int, height: int) -> str:
    old = "0x0" if old_generation == 0 else "0x123"
    return record(
        "SWAPCHAIN_CREATE: device=0x1 "
        f"old={old} old_generation={old_generation} extent=3420x{height} "
        "min_images=2 format=44 color_space=0 present_mode=2 result=0 "
        f"swapchain=0x{generation} generation={generation}"
    )


def good_lifecycle() -> str:
    return "\n".join(
        [
            record("unrelated event", "another-run"),
            create(1, 0, 2148),
            record(
                "SWAPCHAIN_ACQUIRE: device=0x1 swapchain=0x1 generation=1 "
                "ordinal=1 image_index=0 result=0"
            ),
            record(
                "SWAPCHAIN_PRESENT: queue=0x2 swapchain=0x1 generation=1 "
                "ordinal=1 image_index=0 result=0 item_result=0"
            ),
            create(2, 1, 2146),
        ]
    )


class LifecycleTests(unittest.TestCase):
    def test_accepts_single_present_then_two_pixel_height_correction(self) -> None:
        result = analyze_lifecycle(good_lifecycle(), RUN)
        self.assertTrue(result.matches_transient_surface, result.reasons)
        self.assertEqual((result.first_acquires, result.first_presents), (1, 1))

    def test_rejects_two_first_generation_presents(self) -> None:
        text = good_lifecycle().replace(
            create(2, 1, 2146),
            record(
                "SWAPCHAIN_PRESENT: queue=0x2 swapchain=0x1 generation=1 "
                "ordinal=2 image_index=1 result=0 item_result=0"
            )
            + "\n"
            + create(2, 1, 2146),
        )
        result = analyze_lifecycle(text, RUN)
        self.assertFalse(result.matches_transient_surface)
        self.assertIn(
            "the first generation did not have exactly one successful present",
            result.reasons,
        )

    def test_rejects_non_height_only_change(self) -> None:
        result = analyze_lifecycle(
            good_lifecycle().replace("extent=3420x2146", "extent=3418x2146"), RUN
        )
        self.assertFalse(result.matches_transient_surface)


class TimingTests(unittest.TestCase):
    CLIENT = "\n".join(
        [
            "2026-07-25T19:46:15.941+09:00 [zos] DeviceWaitIdle: 0.7 ms",
            "2026-07-25T19:46:15.946+09:00 [zos] OnDeviceReset: 5.1 ms",
            "2026-07-25T19:46:16.798+09:00 [zos] DeviceWaitIdle: 0.1 ms",
            "2026-07-25T19:46:16.800+09:00 [zos] OnDeviceReset: 0.2 ms",
        ]
    )
    INTERFACE = (
        "2026-07-25T19:46:18.387+09:00 [zos] "
        "ZO_PregameStateManager_SetState - from: nil, to: AccountLogin"
    )

    def test_measures_convergence_before_account_login(self) -> None:
        result = analyze_timing(self.CLIENT, self.INTERFACE)
        self.assertFalse(result.reasons, result.reasons)
        self.assertEqual(result.transient_ms, 852.0)
        self.assertEqual(result.convergence_ms, 854.0)

    def test_rejects_incomplete_reset_pair(self) -> None:
        result = analyze_timing(self.CLIENT.splitlines()[0], "")
        self.assertTrue(result.reasons)


if __name__ == "__main__":
    unittest.main()
