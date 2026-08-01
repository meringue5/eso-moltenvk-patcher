#!/usr/bin/env python3

import unittest

from analyze_startup_fx_neutralize import analyze


RUN = "20260801T120000.000000000Z-pid42"


def log(*, matched: bool = True, finished: bool = True) -> str:
    lines = [
        "MODE: startup FX neutralize enabled live_resources=0 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
        "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
        "generation_limit=2 generation_2_present_limit=180",
        "STARTUP_FX_SENTINEL_BEGIN: initializer_offset=0x35fcd42 "
        "window=generation-2-present-180 vectors=0x10,0x20,0x30 "
        "replacement=black-preserve-alpha",
    ]
    if matched:
        lines.append(
            "STARTUP_FX_SENTINEL: call=1 match=yes match_count=1 "
            "caller=fx-material return_offset=0x1ba0dc"
        )
    if finished:
        lines.append(
            "STARTUP_COLOR_AUDIT_FINISH: reason=generation-2-present-limit "
            "generation=2 ordinal=180"
        )
    return "\n".join(f"[run={RUN}] {line}" for line in lines)


class StartupFxNeutralizeTests(unittest.TestCase):
    def test_disappearance_confirms_sentinel(self) -> None:
        verdict, reasons = analyze(log(), False)
        self.assertEqual(verdict, "FX-SENTINEL-CAUSAL")
        self.assertEqual(reasons, [])

    def test_persistence_excludes_sentinel(self) -> None:
        verdict, reasons = analyze(log(), True)
        self.assertEqual(verdict, "FX-SENTINEL-EXCLUDED")
        self.assertEqual(reasons, [])

    def test_requires_exercised_and_finished_window(self) -> None:
        verdict, reasons = analyze(log(matched=False, finished=False), True)
        self.assertEqual(verdict, "INCONCLUSIVE")
        self.assertEqual(len(reasons), 2)


if __name__ == "__main__":
    unittest.main()
