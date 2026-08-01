#!/usr/bin/env python3

from __future__ import annotations

import unittest

from analyze_startup_color_audit import analyze_audit


RUN = "20260801T120000.000000000Z-pid123"


def record(message: str) -> str:
    return f"[run={RUN}] {message}"


def audit_log(
    clear: str | None,
    generation_2_clear: str | None = "0,0,0,1",
) -> str:
    lines = [
        record(
            "STARTUP_COLOR_AUDIT_BEGIN: generation_limit=2 "
            "generation_2_present_limit=180"
        ),
        record(
            "STARTUP_COLOR_BEGIN: generation=1 command_buffer=0x1 "
            "framebuffer=0x2 render_pass=0x3 framebuffer_extent=64x66 "
            "render_area=0,0,64x66 clear_value_count=0 contents=0"
        ),
    ]
    if clear is not None:
        lines.append(
            record(
                "STARTUP_COLOR_CLEAR: generation=1 command_buffer=0x1 "
                "framebuffer=0x2 attachment=0 aspect=0x1 color_attachment=0 "
                f"rgba={clear} rgba_hex=ignored rect_count=1"
            )
        )
    lines.extend(
        [
            record(
                "STARTUP_COLOR_SUBMIT: generation=1 queue=0x4 submit=0 "
                "command_index=0 command_buffer=0x1 framebuffer=0x2 "
                "signal_count=1 result=0"
            ),
            record(
                "SWAPCHAIN_PRESENT: queue=0x4 swapchain=0x5 generation=1 "
                "ordinal=1 image_index=0 result=0 item_result=0"
            ),
            record(
                "STARTUP_COLOR_BEGIN: generation=2 command_buffer=0x6 "
                "framebuffer=0x7 render_pass=0x3 framebuffer_extent=64x64 "
                "render_area=0,0,64x64 clear_value_count=0 contents=0"
            ),
        ]
    )
    if generation_2_clear is not None:
        lines.append(
            record(
                "STARTUP_COLOR_CLEAR: generation=2 command_buffer=0x6 "
                "framebuffer=0x7 attachment=0 aspect=0x1 color_attachment=0 "
                f"rgba={generation_2_clear} rgba_hex=ignored rect_count=1"
            )
        )
    lines.extend(
        [
            record(
                "STARTUP_COLOR_SUBMIT: generation=2 queue=0x4 submit=0 "
                "command_index=0 command_buffer=0x6 framebuffer=0x7 "
                "signal_count=1 result=0"
            ),
            record(
                "SWAPCHAIN_PRESENT: queue=0x4 swapchain=0x8 generation=2 "
                "ordinal=1 image_index=0 result=0 item_result=0"
            ),
            record(
                "STARTUP_COLOR_AUDIT_FINISH: "
                "reason=generation-2-present-limit generation=2 ordinal=180"
            ),
        ]
    )
    return "\n".join(lines)


class AuditTests(unittest.TestCase):
    def test_classifies_neon_clear(self) -> None:
        result = analyze_audit(audit_log("1,0,1,1"), RUN)
        self.assertTrue(result.passed, result.reasons)
        self.assertEqual(result.classification, "explicit-clear-attachments")
        self.assertEqual(
            result.rgba_values,
            ((1.0, 0.0, 1.0, 1.0), (0.0, 0.0, 0.0, 1.0)),
        )
        self.assertEqual(result.generation_rgba_values[0][0], 1)
        self.assertEqual(result.generation_rgba_values[1][0], 2)

    def test_classifies_black_clear(self) -> None:
        result = analyze_audit(audit_log("0,0,0,1"), RUN)
        self.assertTrue(result.passed, result.reasons)
        self.assertEqual(
            result.rgba_values,
            ((0.0, 0.0, 0.0, 1.0), (0.0, 0.0, 0.0, 1.0)),
        )

    def test_classifies_no_clear(self) -> None:
        result = analyze_audit(audit_log(None, None), RUN)
        self.assertTrue(result.passed, result.reasons)
        self.assertEqual(result.classification, "no-submitted-color-clear")
        self.assertEqual(result.generation_rgba_values, ())

    def test_rejects_submit_after_present(self) -> None:
        text = audit_log("1,0,1,1")
        lines = text.splitlines()
        submit_index = next(
            index
            for index, line in enumerate(lines)
            if "STARTUP_COLOR_SUBMIT: generation=1" in line
        )
        present_index = next(
            index
            for index, line in enumerate(lines)
            if "SWAPCHAIN_PRESENT:" in line and "generation=1" in line
        )
        lines[submit_index], lines[present_index] = (
            lines[present_index],
            lines[submit_index],
        )
        result = analyze_audit("\n".join(lines), RUN)
        self.assertFalse(result.passed)

    def test_accepts_suboptimal_present_as_presented(self) -> None:
        text = audit_log("1,0,1,1").replace(
            "generation=2 ordinal=1 image_index=0 result=0 item_result=0",
            "generation=2 ordinal=1 image_index=0 "
            "result=1000001003 item_result=1000001003",
        )
        result = analyze_audit(text, RUN)
        self.assertTrue(result.passed, result.reasons)

    def test_ignores_non_audit_bridge_records_after_finish(self) -> None:
        text = audit_log("1,0,1,1") + "\n" + record(
            "GDPA: device=0x1 name=vkCmdDraw result=0x2 returned=0x2 shim=none"
        )
        result = analyze_audit(text, RUN)
        self.assertTrue(result.passed, result.reasons)


if __name__ == "__main__":
    unittest.main()
