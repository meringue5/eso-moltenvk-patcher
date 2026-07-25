#!/usr/bin/env python3

from __future__ import annotations

import unittest

from analyze_lifecycle_log import summarize


class AnalyzeLifecycleLogTests(unittest.TestCase):
    def test_summarizes_recreated_swapchain_and_first_present(self) -> None:
        lines = [
            "SWAPCHAIN_CREATE: device=0x1 old=(nil) old_generation=0 "
            "extent=2048x1280 min_images=3 format=44 color_space=0 "
            "present_mode=2 result=0 swapchain=0x10 generation=1",
            "SWAPCHAIN_IMAGE: swapchain=0x10 generation=1 "
            "index=0 image=0x20",
            "SWAPCHAIN_IMAGE_VIEW_CREATE: device=0x1 generation=1 "
            "image=0x20 view=0x30 format=44 result=0",
            "SWAPCHAIN_FRAMEBUFFER_CREATE: device=0x1 generation=1 "
            "framebuffer=0x40 render_pass=0x50 attachments=1 tracked=1 "
            "mixed_generations=no extent=2048x1280 result=0",
            "SWAPCHAIN_PRESENT: queue=0x2 swapchain=0x10 generation=1 "
            "ordinal=1 image_index=0 result=0 item_result=0",
            "SWAPCHAIN_CREATE: device=0x1 old=0x10 old_generation=1 "
            "extent=1920x1200 min_images=3 format=44 color_space=0 "
            "present_mode=2 result=0 swapchain=0x11 generation=2",
            "SWAPCHAIN_PRESENT: queue=0x2 swapchain=0x11 generation=2 "
            "ordinal=1 image_index=0 result=0 item_result=0",
            "SWAPCHAIN_FRAMEBUFFER_DESTROY: device=0x1 generation=1 "
            "framebuffer=0x40",
            "SWAPCHAIN_IMAGE_VIEW_DESTROY: device=0x1 generation=1 view=0x30",
            "SWAPCHAIN_DESTROY: device=0x1 swapchain=0x10 generation=1 "
            "live_images=3 live_views=0 live_framebuffers=0",
        ]
        generations, anomalies = summarize(lines)
        self.assertEqual(anomalies, [])
        self.assertEqual(sorted(generations), [1, 2])
        self.assertEqual(generations[1].extent, "2048x1280")
        self.assertEqual(generations[2].old_generation, 1)
        self.assertEqual(generations[2].max_present_ordinal, 1)
        self.assertTrue(generations[1].destroyed)

    def test_flags_live_dependents_and_present_error(self) -> None:
        lines = [
            "SWAPCHAIN_CREATE: result=0 generation=1 extent=1x1 "
            "old_generation=0",
            "SWAPCHAIN_PRESENT: generation=1 ordinal=1 result=-4 "
            "item_result=-4",
            "SWAPCHAIN_DESTROY: generation=1 live_views=1 "
            "live_framebuffers=2",
        ]
        _, anomalies = summarize(lines)
        self.assertEqual(len(anomalies), 3)


if __name__ == "__main__":
    unittest.main()
