#!/usr/bin/env python3

from __future__ import annotations

import unittest

from analyze_swapchain_experiment import analyze


def log_line(mode: str, *, promoted: int, forwarded: int, two: int, three: int) -> str:
    return (
        "[run=20260828T120000.000000000Z-pid123] "
        f"SWAPCHAIN_EXPERIMENT_SUMMARY: mode={mode} creates=2 "
        f"promoted={promoted} forwarded={forwarded} capability_misses=0 "
        f"returned_two={two} returned_three={three} "
        "returned_count_mismatches=0 acquire_samples=1000 acquire_p50_us=1 "
        "acquire_p95_us=2 acquire_p99_us=3 acquire_p999_us=4 acquire_max_us=5 "
        "present_samples=1000 present_p50_us=1 present_p95_us=2 "
        "present_p99_us=3 present_p999_us=4 interval_samples=1000 "
        "interval_p50_us=16666 interval_p95_us=17000 interval_p99_us=18000 "
        "interval_p999_us=22000 acquire_errors=0 present_errors=0"
    )


class AnalyzeSwapchainExperimentTests(unittest.TestCase):
    def test_accepts_exact_control(self) -> None:
        result = analyze(
            log_line("control", promoted=0, forwarded=2, two=2, three=0),
            expected_mode="control",
        )
        self.assertTrue(result.passed, result.reasons)

    def test_accepts_exact_triple_candidate(self) -> None:
        result = analyze(
            log_line("triple", promoted=2, forwarded=0, two=0, three=2),
            expected_mode="triple",
        )
        self.assertTrue(result.passed, result.reasons)

    def test_accepts_latest_periodic_checkpoint_without_destructor(self) -> None:
        text = log_line(
            "control", promoted=0, forwarded=2, two=2, three=0
        ).replace(
            "SWAPCHAIN_EXPERIMENT_SUMMARY:",
            "SWAPCHAIN_EXPERIMENT_CHECKPOINT:",
        )
        result = analyze(text, expected_mode="control")
        self.assertTrue(result.passed, result.reasons)
        self.assertEqual(result.values["source"], "checkpoint")

    def test_rejects_partial_promotion_and_short_run(self) -> None:
        text = log_line("triple", promoted=1, forwarded=1, two=1, three=1).replace(
            "interval_samples=1000", "interval_samples=20"
        )
        result = analyze(text, expected_mode="triple")
        self.assertFalse(result.passed)
        self.assertIn("candidate did not promote every swapchain", result.reasons)
        self.assertIn("fewer than 600 post-warmup samples", result.reasons)


if __name__ == "__main__":
    unittest.main()
