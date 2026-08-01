#!/usr/bin/env python3

import unittest

from analyze_startup_draw_audit import (
    DRAW_BEGIN,
    FINISH,
    MODE,
    PIXEL_BEGIN,
    PIXEL_READY,
    analyze,
)


RUN = "20260801T140000.000000000Z-pid42"
SCHEDULE = ((1, 1), (2, 1), *((2, ordinal) for ordinal in range(10, 181, 10)))


def log(*, overflow: bool = False, missing_pipeline: bool = False) -> str:
    lines = [MODE, PIXEL_BEGIN, PIXEL_READY, DRAW_BEGIN]
    for generation, ordinal in SCHEDULE:
        magenta = generation == 2 and 80 <= ordinal <= 140
        black = generation == 1 or (generation == 2 and ordinal <= 70)
        exact = 5 if magenta else 0
        black_count = 5 if black else 0
        lines.append(
            "STARTUP_PRESENT_DRAW_SUMMARY: "
            f"generation={generation} ordinal={ordinal} image_index=0 "
            "wait_count=1 matched_signals=1 tracked_commands=1 "
            f"draw_count={1 if magenta or not black else 0} "
            f"indexed_draw_count={1 if magenta or not black else 0} "
            f"distinct_pipelines={1 if magenta or not black else 0} "
            f"pipeline_overflow={'yes' if overflow and magenta else 'no'} "
            "draw_signature=1111111111111111 "
            "first_pipeline=aaaaaaaaaaaaaaaa last_pipeline=aaaaaaaaaaaaaaaa "
            "command_buffer=0x1 framebuffer=0x2"
        )
        if magenta or not black:
            signature = "aaaaaaaaaaaaaaaa" if magenta else "bbbbbbbbbbbbbbbb"
            lines.append(
                "STARTUP_PRESENT_DRAW_PIPELINE: "
                f"generation={generation} ordinal={ordinal} pipeline_index=0 "
                f"signature={signature} vertex_hash=1111111111111111 "
                "fragment_hash=2222222222222222 "
                f"shader_hash_complete={'no' if missing_pipeline and magenta else 'yes'} "
                "pipeline_state=tracked"
            )
        lines.append(
            "STARTUP_PRESENT_PIXEL_SUMMARY: "
            f"generation={generation} ordinal={ordinal} image_index=0 "
            "requested_extent=3420x2146 texture_extent=3420x2146 format=44 "
            f"samples=5 exact_magenta={exact} near_magenta={exact} "
            f"black={black_count}"
        )
    lines.append(FINISH)
    return "\n".join(f"[run={RUN}] {line}" for line in lines)


class StartupDrawAuditTests(unittest.TestCase):
    def test_single_magenta_only_pipeline_is_isolated(self) -> None:
        verdict, reasons = analyze(log(), True)
        self.assertEqual(verdict, "DRAW-PIPELINE-CANDIDATE-ISOLATED")
        self.assertEqual(reasons, [])

    def test_pipeline_overflow_is_inconclusive(self) -> None:
        verdict, reasons = analyze(log(overflow=True), True)
        self.assertEqual(verdict, "INCONCLUSIVE")
        self.assertIn(
            "a sampled submit exceeded the pipeline identity cap", reasons
        )

    def test_incomplete_shader_identity_is_inconclusive(self) -> None:
        verdict, reasons = analyze(log(missing_pipeline=True), True)
        self.assertEqual(verdict, "INCONCLUSIVE")
        self.assertIn(
            "a magenta frame lacks complete draw/pipeline identity", reasons
        )


if __name__ == "__main__":
    unittest.main()
