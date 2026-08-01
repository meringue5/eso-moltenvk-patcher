import re
import unittest

from analyze_startup_compositor_audit import analyze
from test_analyze_startup_input_audit import build_log as build_input_log


def build_log(
    *,
    scene_image="2222222222222222",
    scene_set0_buffer="3333333333333333",
    scene_set1_buffer="4444444444444444",
    magenta_input="scene",
    gui_scene_signature="6666666666666666",
    omit_class=False,
):
    text = build_input_log()
    text = text.replace(
        "MODE: startup input audit enabled live_resources=0 metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 synchronous_queue_submits=0 maximize_concurrent_compilation=1 generation_limit=2 generation_2_present_limit=180 pixel_samples=20 draw_provenance=enabled input_provenance=enabled",
        "MODE: startup compositor audit enabled live_resources=0 metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 synchronous_queue_submits=0 maximize_concurrent_compilation=1 generation_limit=2 generation_2_present_limit=180 pixel_samples=20 draw_provenance=enabled input_provenance=enabled descriptor_classes=enabled",
    )
    text = text.replace(
        "[run=20260801T150000.000000000Z-pid42] STARTUP_INPUT_AUDIT_BEGIN: generation_limit=2 generation_2_present_limit=180 max_descriptor_set_layouts=2048 max_pipeline_layouts=2048 max_descriptor_sets=131072 max_bound_sets=16",
        "[run=20260801T150000.000000000Z-pid42] STARTUP_INPUT_AUDIT_BEGIN: generation_limit=2 generation_2_present_limit=180 max_descriptor_set_layouts=2048 max_pipeline_layouts=2048 max_descriptor_sets=131072 max_bound_sets=16\n"
        "[run=20260801T150000.000000000Z-pid42] STARTUP_COMPOSITOR_AUDIT_BEGIN: image_bindings_per_set=2 sampled_subresources=base-mip-base-layer\n"
        "[run=20260801T150000.000000000Z-pid42] STARTUP_COMPOSITOR_IMAGE_READY: synchronization=queue-wait-idle points_per_image=5 formats=rgba8,bgra8,rgba16f",
    )
    text = text.replace("signature=aaaaaaaaaaaaaaaa", "signature=c43e4410d3b33fe7")
    text = text.replace("pipeline_signature=aaaaaaaaaaaaaaaa", "pipeline_signature=c43e4410d3b33fe7")
    text = text.replace("vertex_hash=cccccccccccccccc", "vertex_hash=c8307556011c995e")
    text = text.replace("fragment_hash=dddddddddddddddd", "fragment_hash=6907bd3576e3a930")
    text = text.replace(
        "layout_signature=eeeeeeeeeeeeeeee set_layouts=1 descriptors=1 images=1 buffers=0 samplers=0 input_attachments=0 push_bytes=0",
        "layout_signature=d175d2c1daed112d set_layouts=2 descriptors=8 images=2 buffers=6 samplers=0 input_attachments=0 push_bytes=0",
    )
    text = text.replace("bound_sets=1 ", "bound_sets=2 ")
    if omit_class:
        return text

    output = []
    input_line = re.compile(
        r"^\[run=(?P<run>[^]]+)] STARTUP_PRESENT_DRAW_INPUT: "
        r"generation=(?P<generation>[12]) ordinal=(?P<ordinal>\d+) "
        r"bound_sets=(?P<sets>\d+)"
    )
    for line in text.splitlines():
        output.append(line)
        match = input_line.match(line)
        if not match or match.group("sets") != "2":
            continue
        generation = int(match.group("generation"))
        ordinal = int(match.group("ordinal"))
        scene = generation == 2 and ordinal >= 150
        image = scene_image if scene else "1111111111111111"
        set0_buffer = scene_set0_buffer if scene else "3333333333333333"
        set1_buffer = scene_set1_buffer if scene else "4444444444444444"
        prefix = f"[run={match.group('run')}]"
        output.append(
            f"{prefix} STARTUP_PRESENT_DESCRIPTOR_CLASS: generation={generation} ordinal={ordinal} slot=0 layout_signature=e3c2499a89df1706 expected_images=0 expected_buffers=3 image_update_signature=0000000000000000 image_update_writes=0 image_update_call=0 buffer_update_signature={set0_buffer} buffer_update_writes=3 buffer_update_call=10 class_complete=yes"
        )
        output.append(
            f"{prefix} STARTUP_PRESENT_DESCRIPTOR_CLASS: generation={generation} ordinal={ordinal} slot=1 layout_signature=d0edad262f8c4230 expected_images=2 expected_buffers=3 image_update_signature={image} image_update_writes=2 image_update_call=11 buffer_update_signature={set1_buffer} buffer_update_writes=3 buffer_update_call=11 class_complete=yes"
        )
        scene_signature = scene_image if scene else "1111111111111111"
        gui_signature = gui_scene_signature if scene else "5555555555555555"
        scene_near = 5 if not scene and magenta_input == "scene" else 0
        gui_near = 5 if not scene and magenta_input == "gui" else 0
        output.append(
            f"{prefix} STARTUP_PRESENT_COMPOSITOR_IMAGE: generation={generation} ordinal={ordinal} set_slot=1 image_ordinal=0 binding=3 array_element=0 signature={scene_signature} update_call=11 view=0x1 image=0x2 format=97 view_type=1 mip=0 layer=0 layout=5"
        )
        output.append(
            f"{prefix} STARTUP_COMPOSITOR_IMAGE_SUMMARY: generation={generation} ordinal={ordinal} set_slot=1 binding=3 array_element=0 image_ordinal=0 vk_format=97 metal_format=115 mip=0 layer=0 extent=3420x2146 samples=5 exact_magenta={scene_near} near_magenta={scene_near} black=0"
        )
        output.append(
            f"{prefix} STARTUP_PRESENT_COMPOSITOR_IMAGE: generation={generation} ordinal={ordinal} set_slot=1 image_ordinal=1 binding=4 array_element=0 signature={gui_signature} update_call=11 view=0x3 image=0x4 format=44 view_type=1 mip=0 layer=0 layout=5"
        )
        output.append(
            f"{prefix} STARTUP_COMPOSITOR_IMAGE_SUMMARY: generation={generation} ordinal={ordinal} set_slot=1 binding=4 array_element=0 image_ordinal=1 vk_format=44 metal_format=80 mip=0 layer=0 extent=3420x2146 samples=5 exact_magenta={gui_near} near_magenta={gui_near} black=0"
        )
    return "\n".join(output) + "\n"


class StartupCompositorAuditTests(unittest.TestCase):
    def test_scene_descriptor_change_isolated(self):
        verdict, reasons = analyze(build_log(), True)
        self.assertEqual(
            verdict, "COMPOSITOR-SCENE-MAGENTA-DESCRIPTOR-CHANGE"
        )
        self.assertEqual(reasons, [])

    def test_gui_in_place_content_change_isolated(self):
        verdict, reasons = analyze(
            build_log(
                scene_image="1111111111111111",
                magenta_input="gui",
                gui_scene_signature="5555555555555555",
            ),
            True,
        )
        self.assertEqual(
            verdict, "COMPOSITOR-GUI-MAGENTA-IN-PLACE-CONTENT-CHANGE"
        )
        self.assertEqual(reasons, [])

    def test_no_direct_magenta_is_combined_candidate(self):
        verdict, reasons = analyze(
            build_log(magenta_input=None), True
        )
        self.assertEqual(verdict, "COMPOSITOR-COMBINED-INPUT-CANDIDATE")
        self.assertEqual(reasons, [])

    def test_missing_class_samples_fail_closed(self):
        verdict, reasons = analyze(build_log(omit_class=True), True)
        self.assertEqual(verdict, "INCONCLUSIVE")
        self.assertTrue(any("descriptor-class" in reason for reason in reasons))


if __name__ == "__main__":
    unittest.main()
