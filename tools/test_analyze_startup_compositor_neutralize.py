#!/usr/bin/env python3

import unittest

from analyze_startup_compositor_neutralize import analyze


RUN = "20260802T030000.000000000Z-pid42"


def log(
    *, fallback: bool = False, finished: bool = True,
    early_transition: bool = False
) -> str:
    lines = [
        "MODE: startup compositor neutralize enabled live_resources=0 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
        "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
        "generation_limit=2 generation_2_present_limit=180 "
        "draw_provenance=enabled input_provenance=enabled "
        "pixel_readback=disabled fallback=forward",
        "STARTUP_COMPOSITOR_NEUTRALIZE_BEGIN: generation=2 first_present=60 "
        "last_present=150 max_suppressed_draws=96 fallback=forward",
    ]
    first_ordinal, last_ordinal = (
        (71, 71) if early_transition else (80, 149)
    )
    for draw, ordinal in enumerate(
        range(first_ordinal, last_ordinal + 1), start=1
    ):
        lines.append(
            "STARTUP_COMPOSITOR_NEUTRALIZE_SUPPRESS: generation=2 "
            f"ordinal={ordinal} pipeline=c43e4410d3b33fe7 "
            "descriptor_update_signature=01922f8394b93e32 "
            f"draw={draw}"
        )
    latch_ordinal = 72 if early_transition else 150
    lines.append(
        "STARTUP_COMPOSITOR_NEUTRALIZE_LATCH: action=forward "
        f"reason={'present-deadline' if fallback else 'descriptor-transition'} "
        f"generation=2 ordinal={latch_ordinal} pipeline=c43e4410d3b33fe7 "
        "descriptor_update_signature=a7d448d22e640458 "
        f"suppressed_draws={last_ordinal - first_ordinal + 1}"
    )
    if finished:
        lines.append(
            "STARTUP_COLOR_AUDIT_FINISH: reason=generation-2-present-limit "
            "generation=2 ordinal=180"
        )
    return "\n".join(f"[run={RUN}] {line}" for line in lines)


class StartupCompositorNeutralizeTests(unittest.TestCase):
    def test_disappearance_confirms_placeholder_boundary(self) -> None:
        verdict, reasons = analyze(log(), False)
        self.assertEqual(verdict, "COMPOSITOR-PLACEHOLDER-NEUTRALIZED")
        self.assertEqual(reasons, [])

    def test_persistence_excludes_placeholder_boundary(self) -> None:
        verdict, reasons = analyze(log(), True)
        self.assertEqual(verdict, "COMPOSITOR-PLACEHOLDER-EXCLUDED")
        self.assertEqual(reasons, [])

    def test_rejects_fallback_or_incomplete_window(self) -> None:
        verdict, reasons = analyze(log(fallback=True, finished=False), False)
        self.assertEqual(verdict, "INCONCLUSIVE")
        self.assertIn("the bounded two-generation window did not finish", reasons)
        self.assertIn("the neutralizer ended through a fallback path", reasons)

    def test_rejects_transition_before_proven_magenta_window(self) -> None:
        verdict, reasons = analyze(log(early_transition=True), True)
        self.assertEqual(verdict, "INCONCLUSIVE")
        self.assertIn(
            "suppression ended before the proven magenta interval", reasons
        )
        self.assertIn(
            "the forwarding latch preceded the proven scene boundary", reasons
        )


if __name__ == "__main__":
    unittest.main()
