#!/usr/bin/env python3

from __future__ import annotations

import unittest

from analyze_reset_log import analyze


class AnalyzeResetLogTests(unittest.TestCase):
    def test_extracts_terminal_live_reset(self) -> None:
        text = "\n".join(
            [
                "2026-07-25T19:10:09.346+09:00 "
                "[zos][ZoRenderDeviceVk] DeviceWaitIdle: 13.612667 ms",
                "2026-07-25T19:10:09.348+09:00 "
                "[zos][ZoRenderDeviceVk] DeviceWaitIdle: 0.052709 ms",
                "2026-07-25T19:10:09.348+09:00 "
                "[zos][ZoRenderDeviceVk] fpCreateSwapchainKHR: 0.108167 ms",
                "2026-07-25T19:10:09.348+09:00 "
                "[zos][ZoRenderDeviceVk] OnDeviceReset: 0.365750 ms",
            ]
        )
        events, errors = analyze(text)
        self.assertEqual(errors, 0)
        self.assertEqual(len(events), 1)
        self.assertEqual(events[0].wait_count, 2)
        self.assertEqual(events[0].longest_wait_ms, 13.612667)
        self.assertEqual(events[0].swapchain_ms, 0.108167)
        self.assertTrue(events[0].terminal)

    def test_counts_error_markers(self) -> None:
        _, errors = analyze("warning: device lost\n")
        self.assertEqual(errors, 1)


if __name__ == "__main__":
    unittest.main()
