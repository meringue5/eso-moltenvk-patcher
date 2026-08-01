#!/usr/bin/env python3

import unittest

from analyze_startup_present_pixels import BEGIN, FINISH, MODE, READY, analyze


RUN = "20260801T130000.000000000Z-pid42"
SCHEDULE = ((1, 1), (2, 1), *((2, ordinal) for ordinal in range(10, 181, 10)))


def log(*, magenta_at: tuple[int, int] | None = None, skip: bool = False) -> str:
    lines = [MODE, BEGIN, READY]
    for generation, ordinal in SCHEDULE:
        exact = 5 if (generation, ordinal) == magenta_at else 0
        black = 0 if exact else 5
        lines.append(
            "STARTUP_PRESENT_PIXEL_SUMMARY: "
            f"generation={generation} ordinal={ordinal} image_index=0 "
            "requested_extent=3420x2146 texture_extent=3420x2146 format=44 "
            f"samples=5 exact_magenta={exact} near_magenta={exact} black={black}"
        )
    if skip:
        lines.append(
            "STARTUP_PRESENT_PIXEL_SKIP: generation=2 ordinal=90 "
            "image_index=0 image=tracked synchronization=unconfirmed sampler=ready"
        )
    lines.append(FINISH)
    return "\n".join(f"[run={RUN}] {line}" for line in lines)


class StartupPresentPixelTests(unittest.TestCase):
    def test_correlated_magenta_is_inside_swapchain(self) -> None:
        verdict, reasons = analyze(log(magenta_at=(2, 60)), True)
        self.assertEqual(verdict, "SWAPCHAIN-MAGENTA-CONFIRMED")
        self.assertEqual(reasons, [])

    def test_correlated_pink_without_magenta_is_after_swapchain(self) -> None:
        verdict, reasons = analyze(log(), True)
        self.assertEqual(verdict, "POST-SWAPCHAIN-MAGENTA")
        self.assertEqual(reasons, [])

    def test_skipped_sample_is_inconclusive(self) -> None:
        verdict, reasons = analyze(log(skip=True), True)
        self.assertEqual(verdict, "INCONCLUSIVE")
        self.assertEqual(
            reasons,
            ["at least one scheduled frame was not safely sampled"],
        )


if __name__ == "__main__":
    unittest.main()
