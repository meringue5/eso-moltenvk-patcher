import unittest

from analyze_startup_input_audit import analyze


SCHEDULE = [(1, 1)] + [(2, value) for value in (1, *range(10, 181, 10))]


def build_log(*, scene_updates="1111111111111111", missing_input=False):
    lines = [
        "[run=20260801T150000.000000000Z-pid42] MODE: startup input audit enabled live_resources=0 metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 synchronous_queue_submits=0 maximize_concurrent_compilation=1 generation_limit=2 generation_2_present_limit=180 pixel_samples=20 draw_provenance=enabled input_provenance=enabled",
        "[run=20260801T150000.000000000Z-pid42] STARTUP_PRESENT_PIXEL_AUDIT_BEGIN: generation_1_samples=1 generation_2_samples=1,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180",
        "[run=20260801T150000.000000000Z-pid42] STARTUP_DRAW_AUDIT_BEGIN: generation_limit=2 generation_2_present_limit=180 max_distinct_pipelines_per_submit=8",
        "[run=20260801T150000.000000000Z-pid42] STARTUP_INPUT_AUDIT_BEGIN: generation_limit=2 generation_2_present_limit=180 max_descriptor_set_layouts=2048 max_pipeline_layouts=2048 max_descriptor_sets=131072 max_bound_sets=16",
    ]
    for generation, ordinal in SCHEDULE:
        magenta = generation == 2 and 80 <= ordinal <= 140
        scene = generation == 2 and ordinal >= 150
        black = not magenta and not scene
        exact = 5 if magenta else 0
        black_count = 5 if black else 0
        lines.append(
            f"[run=20260801T150000.000000000Z-pid42] STARTUP_PRESENT_PIXEL_SUMMARY: generation={generation} ordinal={ordinal} image_index=0 requested_extent=3420x2146 texture_extent=3420x2146 format=44 samples=5 exact_magenta={exact} near_magenta={exact} black={black_count}"
        )
        draws = 0 if black else 1
        pipelines = 0 if black else 1
        pipeline = "0000000000000000" if black else "aaaaaaaaaaaaaaaa"
        lines.append(
            f"[run=20260801T150000.000000000Z-pid42] STARTUP_PRESENT_DRAW_SUMMARY: generation={generation} ordinal={ordinal} image_index=0 wait_count=1 matched_signals=1 tracked_commands=1 draw_count={draws} indexed_draw_count={draws} distinct_pipelines={pipelines} pipeline_overflow=no draw_signature=bbbbbbbbbbbbbbbb first_pipeline={pipeline} last_pipeline={pipeline} command_buffer=0x1 framebuffer=0x2"
        )
        if not black:
            lines.append(
                f"[run=20260801T150000.000000000Z-pid42] STARTUP_PRESENT_DRAW_PIPELINE: generation={generation} ordinal={ordinal} pipeline_index=0 signature=aaaaaaaaaaaaaaaa vertex_hash=cccccccccccccccc fragment_hash=dddddddddddddddd shader_hash_complete=yes pipeline_state=tracked"
            )
            lines.append(
                f"[run=20260801T150000.000000000Z-pid42] STARTUP_PRESENT_INPUT_PIPELINE: generation={generation} ordinal={ordinal} pipeline_index=0 pipeline_signature=aaaaaaaaaaaaaaaa layout_signature=eeeeeeeeeeeeeeee set_layouts=1 descriptors=1 images=1 buffers=0 samplers=0 input_attachments=0 push_bytes=0 layout_complete=yes"
            )
        update = scene_updates if scene else "1111111111111111"
        complete = "no" if missing_input and magenta else ("yes" if not black else "no")
        lines.append(
            f"[run=20260801T150000.000000000Z-pid42] STARTUP_PRESENT_DRAW_INPUT: generation={generation} ordinal={ordinal} bound_sets={0 if black else 1} descriptor_layout_signature=eeeeeeeeeeeeeeee descriptor_handle_signature=ffffffffffffffff descriptor_update_signature={update} push_signature=0000000000000000 push_bytes=0 input_complete={complete}"
        )
    lines.append(
        "[run=20260801T150000.000000000Z-pid42] STARTUP_COLOR_AUDIT_FINISH: reason=generation-2-present-limit generation=2 ordinal=180"
    )
    return "\n".join(lines) + "\n"


class StartupInputAuditTests(unittest.TestCase):
    def test_stable_bound_image_is_content_candidate(self):
        verdict, reasons = analyze(build_log(), True)
        self.assertEqual(verdict, "BOUND-RESOURCE-CONTENT-CANDIDATE")
        self.assertEqual(reasons, [])

    def test_descriptor_update_change_isolated(self):
        verdict, reasons = analyze(
            build_log(scene_updates="2222222222222222"), True
        )
        self.assertEqual(verdict, "DESCRIPTOR-STATE-CHANGE-CANDIDATE")
        self.assertEqual(reasons, [])

    def test_missing_magenta_input_fails_closed(self):
        verdict, reasons = analyze(build_log(missing_input=True), True)
        self.assertEqual(verdict, "INCONCLUSIVE")
        self.assertTrue(any("bound-input" in reason for reason in reasons))


if __name__ == "__main__":
    unittest.main()
