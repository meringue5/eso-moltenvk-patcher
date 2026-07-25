#!/usr/bin/env python3

from __future__ import annotations

import unittest

from analyze_focus_log import summarize


class AnalyzeFocusLogTests(unittest.TestCase):
    def test_reports_foreground_and_keyboard_delivery(self) -> None:
        text = "\n".join(
            [
                "2026-07-25 19:35:48 WindowServer "
                "Process now frontmost: (<private>:25514)",
                "2026-07-25 19:37:18 WindowServer Deferring events from "
                "frontmost process (eso) -> <pid: 25514>",
                "2026-07-25 19:38:35 WindowServer destinations for Keyboard "
                "event: (<keyboardFocus; eso.25514>)",
                "2026-07-25 19:38:52 WindowServer Process death: eso pid: 25514",
            ]
        )
        result = summarize(text, 25514)
        self.assertEqual(result["frontmost_confirmations"], 2)
        self.assertEqual(result["keyboard_focus_records"], 1)
        self.assertEqual(result["process_deaths"], 1)
        self.assertEqual(result["first_focus_record"], "2026-07-25 19:35:48")
        self.assertEqual(result["last_focus_record"], "2026-07-25 19:38:35")

    def test_ignores_other_process(self) -> None:
        result = summarize(
            "2026-07-25 19:35:48 Process now frontmost: pid: 999", 25514
        )
        self.assertEqual(result["frontmost_confirmations"], 0)
        self.assertEqual(result["keyboard_focus_records"], 0)


if __name__ == "__main__":
    unittest.main()
