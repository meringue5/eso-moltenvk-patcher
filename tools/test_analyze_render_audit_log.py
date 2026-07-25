#!/usr/bin/env python3

from __future__ import annotations

import unittest

from analyze_render_audit_log import EXPECTED_COUNTERS, analyze, analyze_all


RUN = "20260725T130000.123456789Z-pid50000"


def record(message: str) -> str:
    return f"[run={RUN}] {message}"


def good_log(overrides: dict[str, int] | None = None) -> str:
    values = dict.fromkeys(EXPECTED_COUNTERS, 0)
    values.update(overrides or {})
    lines = [
        record(
            "RENDER_AUDIT_BEGIN: mirror=enabled sample_limit=64 "
            "anomaly_limit=64 slot_capacity=131072"
        ),
        record(
            "RENDER_AUDIT_SUMMARY: reason=present-limit complete=yes "
            "samples=64 anomalies=0"
        ),
    ]
    lines.extend(
        record(f"RENDER_AUDIT_COUNT: name={name} value={value}")
        for name, value in sorted(values.items())
    )
    return "\n".join(lines)


class RenderAuditLogTests(unittest.TestCase):
    def test_accepts_complete_audit(self) -> None:
        result = analyze(good_log())
        self.assertTrue(result.complete)
        self.assertFalse(result.anomalies)
        self.assertIn(
            "descriptor-lifetime: no stale/dead-reference signal "
            "in the bounded window",
            result.findings,
        )

    def test_reports_descriptor_signal(self) -> None:
        result = analyze(
            good_log({"descriptor_stale_slots_bound": 3})
        )
        self.assertIn(
            "descriptor-lifetime: SIGNAL (3 stale/dead-reference events)",
            result.findings,
        )

    def test_rejects_overflow(self) -> None:
        result = analyze(good_log({"state_overflows": 1}))
        self.assertTrue(any("overflowed" in item for item in result.anomalies))

    def test_rejects_stale_time_gate(self) -> None:
        result = analyze(good_log(), after_epoch=2_000_000_000)
        self.assertIsNone(result.run_id)
        self.assertFalse(result.complete)

    def test_all_runs_keeps_chronological_order(self) -> None:
        second = good_log().replace(
            RUN, "20260725T130100.123456789Z-pid50001"
        )
        results = analyze_all(good_log() + "\n" + second)
        self.assertEqual(len(results), 2)
        self.assertEqual(results[0].run_id, RUN)
        self.assertTrue(results[1].run_id.endswith("pid50001"))

    def test_validates_reset_pipeline_cache_bypass_mode(self) -> None:
        text = "\n".join(
            (
                record(
                    "MODE: reset pipeline cache bypass enabled "
                    "live_resources=1 metal_argument_buffers=0 "
                    "use_mtlheap=1 command_pooling=1"
                ),
                good_log(
                    {
                        "graphics_pipelines_created": 3,
                        "graphics_pipelines_created_during_audit": 3,
                    }
                ),
                record(
                    "RESET_PIPELINE_CACHE_BYPASS: requested_cache=0x1 "
                    "forwarded_cache=0x0 pipelines=3 result=0"
                ),
            )
        )
        result = analyze(text)
        self.assertFalse(result.anomalies)
        self.assertIn(
            "pipeline-cache-bypass: calls=1 pipelines=3",
            result.findings,
        )

    def test_rejects_incomplete_pipeline_cache_bypass(self) -> None:
        text = "\n".join(
            (
                record(
                    "MODE: reset pipeline cache bypass enabled "
                    "live_resources=1 metal_argument_buffers=0 "
                    "use_mtlheap=1 command_pooling=1"
                ),
                good_log({"graphics_pipelines_created": 3}),
                record(
                    "RESET_PIPELINE_CACHE_BYPASS: requested_cache=0x1 "
                    "forwarded_cache=0x0 pipelines=2 result=0"
                ),
            )
        )
        result = analyze(text)
        self.assertTrue(
            any("count mismatch" in item for item in result.anomalies)
        )


if __name__ == "__main__":
    unittest.main()
