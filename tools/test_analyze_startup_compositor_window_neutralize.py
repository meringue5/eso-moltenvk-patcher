#!/usr/bin/env python3

import unittest

from analyze_startup_compositor_window_neutralize import (
    PACING_ACTIVE,
    PACING_BYPASS_MODE,
    PACING_READY,
    analyze,
)


RUN = "20260802T040000.000000000Z-pid43"


def log(
    *, first: int = 71, last: int = 149, deadline: int = 150,
    mode: str | None = None,
) -> str:
    lines = [
        mode or (
            "MODE: startup compositor neutralize enabled live_resources=0 "
            "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
            "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
            "generation_limit=2 generation_2_present_limit=180 "
            "draw_provenance=enabled input_provenance=enabled "
            "pixel_readback=disabled fallback=forward"
        ),
        "STARTUP_COMPOSITOR_NEUTRALIZE_BEGIN: generation=2 first_present=71 "
        "last_present=150 max_suppressed_draws=96 strategy=ordinal-window "
        "fallback=forward",
    ]
    if mode == PACING_BYPASS_MODE:
        lines.extend([PACING_READY, PACING_ACTIVE])
    for draw, ordinal in enumerate(range(first, last + 1), start=1):
        descriptor = "01922f8394b93e32" if ordinal < 100 else "a7d448d22e640458"
        lines.append(
            "STARTUP_COMPOSITOR_NEUTRALIZE_SUPPRESS: generation=2 "
            f"ordinal={ordinal} pipeline=c43e4410d3b33fe7 "
            f"descriptor_update_signature={descriptor} draw={draw}"
        )
    lines.extend([
        "STARTUP_COMPOSITOR_NEUTRALIZE_LATCH: action=forward "
        "reason=present-deadline generation=2 "
        f"ordinal={deadline} pipeline=c43e4410d3b33fe7 "
        "descriptor_update_signature=0000000000000000 "
        f"suppressed_draws={last - first + 1}",
        "STARTUP_COLOR_AUDIT_FINISH: reason=generation-2-present-limit "
        "generation=2 ordinal=180",
    ])
    return "\n".join(f"[run={RUN}] {line}" for line in lines)


class StartupCompositorWindowNeutralizeTests(unittest.TestCase):
    def test_complete_window_and_no_pink_passes(self) -> None:
        verdict, reasons = analyze(log(), False)
        self.assertEqual(verdict, "WINDOW-NEUTRALIZED")
        self.assertEqual(reasons, [])

    def test_pacing_bypass_mode_is_eligible(self) -> None:
        verdict, reasons = analyze(log(mode=PACING_BYPASS_MODE), False)
        self.assertEqual(verdict, "WINDOW-NEUTRALIZED")
        self.assertEqual(reasons, [])

    def test_complete_window_but_pink_reports_failed_repair(self) -> None:
        verdict, reasons = analyze(log(), True)
        self.assertEqual(verdict, "WINDOW-NEUTRALIZATION-FAILED")
        self.assertEqual(reasons, [])

    def test_short_or_early_deadline_window_is_inconclusive(self) -> None:
        verdict, reasons = analyze(log(last=79, deadline=80), True)
        self.assertEqual(verdict, "INCONCLUSIVE")
        self.assertIn(
            "suppression ended before the proven magenta interval", reasons
        )
        self.assertIn(
            "the neutralizer did not forward at the exact deadline", reasons
        )


if __name__ == "__main__":
    unittest.main()
