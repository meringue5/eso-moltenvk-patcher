#!/usr/bin/env python3

from __future__ import annotations

import unittest

from analyze_reset_resource_log import EXPECTED_COUNTERS, analyze


RUN = "20260725T120000.123456789Z-pid40000"


def record(message: str) -> str:
    return f"[run={RUN}] {message}"


def good_log() -> str:
    lines = [
        record(
            "RESET_RESOURCE_TRACE_BEGIN: wait=4 established_swapchains=2 "
            "detail_limit=48 present_limit=8"
        ),
        record(
            "RESET_RESOURCE_TRACE_SWAPCHAIN: ordinal=3 swapchain=0x3 "
            "extent=3420x2146"
        ),
        record(
            "RESET_RESOURCE_TRACE_SUMMARY: reason=present-limit "
            "reset_presents=8 failures=0 details=4"
        ),
    ]
    lines.extend(
        record(f"RESET_RESOURCE_COUNT: name={name} value=0")
        for name in sorted(EXPECTED_COUNTERS)
    )
    return "\n".join(lines)


class ResetResourceLogTests(unittest.TestCase):
    def test_accepts_complete_trace(self) -> None:
        result = analyze(good_log())
        self.assertTrue(result.complete)
        self.assertFalse(result.anomalies)
        self.assertEqual(result.reset_presents, 8)

    def test_rejects_missing_counter(self) -> None:
        text = "\n".join(
            line
            for line in good_log().splitlines()
            if "name=create_image " not in line
        )
        result = analyze(text)
        self.assertTrue(
            any("missing counters: create_image" in item for item in result.anomalies)
        )

    def test_rejects_failure_count_mismatch(self) -> None:
        text = good_log().replace("failures=0", "failures=1")
        result = analyze(text)
        self.assertTrue(
            any("failure summary mismatch" in item for item in result.anomalies)
        )

    def test_time_gate_rejects_stale_trace(self) -> None:
        result = analyze(good_log(), after_epoch=2_000_000_000)
        self.assertIsNone(result.run_id)
        self.assertFalse(result.complete)


if __name__ == "__main__":
    unittest.main()
