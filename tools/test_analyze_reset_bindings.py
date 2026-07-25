#!/usr/bin/env python3

from __future__ import annotations

import unittest

from analyze_reset_bindings import analyze


RUN = "20260725T120000.123456789Z-pid40000"


def record(message: str) -> str:
    return f"[run={RUN}] {message}"


def trace(*details: str) -> str:
    return "\n".join(
        [
            record(
                "RESET_RESOURCE_TRACE_BEGIN: wait=4 "
                "established_swapchains=2 detail_limit=48 present_limit=8"
            ),
            *(record(f"RESET_RESOURCE_DETAIL: {detail}") for detail in details),
        ]
    )


class ResetBindingTests(unittest.TestCase):
    def test_accepts_aligned_nonoverlapping_bindings(self) -> None:
        result = analyze(
            trace(
                "operation=create_image flags=0x0 result=0 image=0x1",
                "operation=get_image_memory_requirements "
                "size=128 alignment=128 type_bits=0x1",
                "operation=bind_image_memory image=0x1 memory=0xa "
                "offset=0 result=0",
                "operation=create_image flags=0x0 result=0 image=0x2",
                "operation=get_image_memory_requirements "
                "size=256 alignment=128 type_bits=0x1",
                "operation=bind_image_memory image=0x2 memory=0xa "
                "offset=128 result=0",
            )
        )
        self.assertEqual(len(result.bindings), 2)
        self.assertFalse(result.anomalies)

    def test_rejects_misalignment_and_overlap(self) -> None:
        result = analyze(
            trace(
                "operation=create_image flags=0x0 result=0 image=0x1",
                "operation=get_image_memory_requirements "
                "size=256 alignment=128 type_bits=0x1",
                "operation=bind_image_memory image=0x1 memory=0xa "
                "offset=0 result=0",
                "operation=create_image flags=0x0 result=0 image=0x2",
                "operation=get_image_memory_requirements "
                "size=128 alignment=128 type_bits=0x1",
                "operation=bind_image_memory image=0x2 memory=0xa "
                "offset=64 result=0",
            )
        )
        self.assertTrue(any("not aligned" in item for item in result.anomalies))
        self.assertTrue(any("overlap" in item for item in result.anomalies))

    def test_reports_truncated_create_record(self) -> None:
        result = analyze(
            trace("operation=create_image flags=0x0 result=0 image=0x1")
        )
        self.assertEqual(result.incomplete_records, 1)
        self.assertFalse(result.anomalies)


if __name__ == "__main__":
    unittest.main()
