#!/usr/bin/env python3
"""Classify the latest instrumented teso4m4 startup run.

This checker deliberately evaluates only bridge evidence. Reaching character
selection and the timing/classification of any crash report remain separate
user-observed startup behavior.
"""

from __future__ import annotations

import argparse
import re
from collections import OrderedDict
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path


LINE = re.compile(r"^\[run=([^]]+)] (.*)$")
RUN_ID = re.compile(
    r"^(?P<stamp>\d{8}T\d{6})\.(?P<nanos>\d{9})Z-pid(?P<pid>\d+)$"
)
CREATE_DEVICE = re.compile(
    r"^CREATE_DEVICE: call=(?P<call>\d+) .* extensions=(?P<count>\d+) "
    r"hdr_enabled=(?P<hdr>yes|no)$"
)
CREATE_DEVICE_EXT = re.compile(
    r"^CREATE_DEVICE_EXT: call=(?P<call>\d+) index=(?P<index>\d+) "
    r"name=(?P<name>.*)$"
)
CREATE_DEVICE_RESULT = re.compile(
    r"^CREATE_DEVICE_RESULT: call=(?P<call>\d+) result=(?P<result>-?\d+) "
    r"device=(?P<device>.*)$"
)
CREATE_DEVICE_FEATURE_PROFILE = re.compile(
    r"^CREATE_DEVICE_FEATURE_PROFILE: call=(?P<call>\d+) enabled=18 "
    r"prohibited_enabled=0 expected_prohibited=0$"
)
LEGACY_FEATURE_PROFILE = re.compile(
    r"^LEGACY_FEATURE_PROFILE: physical=.* raw_enabled=36 "
    r"visible_enabled=18 masked=18 expected_masked=18$"
)
REMOVED_ONE = re.compile(r"(?:^| )removed=1(?: |$)")
PERFORMANCE_DIRECT_NAMES = (
    "vkDeviceWaitIdle",
    "vkCreateSwapchainKHR",
    "vkDestroySwapchainKHR",
    "vkGetSwapchainImagesKHR",
    "vkCreateImageView",
    "vkDestroyImageView",
    "vkCreateRenderPass",
    "vkDestroyRenderPass",
    "vkCreateFramebuffer",
    "vkDestroyFramebuffer",
    "vkAcquireNextImageKHR",
    "vkQueuePresentKHR",
)


@dataclass(frozen=True)
class Verdict:
    run_id: str | None
    reasons: tuple[str, ...]

    @property
    def passed(self) -> bool:
        return self.run_id is not None and not self.reasons


def run_epoch(run_id: str) -> float | None:
    match = RUN_ID.fullmatch(run_id)
    if not match:
        return None
    try:
        stamp = datetime.strptime(match.group("stamp"), "%Y%m%dT%H%M%S").replace(
            tzinfo=timezone.utc
        )
    except ValueError:
        return None
    return stamp.timestamp() + int(match.group("nanos")) / 1_000_000_000


def parse_runs(text: str) -> OrderedDict[str, list[str]]:
    runs: OrderedDict[str, list[str]] = OrderedDict()
    for line in text.splitlines():
        match = LINE.match(line)
        if match:
            runs.setdefault(match.group(1), []).append(match.group(2))
    return runs


def evaluate_startup_log(
    text: str, *, after_epoch: float | None = None, expected_redirects: int = 17
) -> Verdict:
    runs = parse_runs(text)
    eligible: list[tuple[float, int, str, list[str]]] = []
    for order, (run_id, lines) in enumerate(runs.items()):
        epoch = run_epoch(run_id)
        if after_epoch is not None and (epoch is None or epoch < after_epoch):
            continue
        if any(line.startswith("ACTIVE: redirected ") for line in lines):
            eligible.append(
                (epoch if epoch is not None else float("-inf"), order, run_id, lines)
            )
    if not eligible:
        return Verdict(None, ("no active instrumented run matched the time gate",))

    _, _, run_id, lines = max(eligible)
    reasons: list[str] = []
    run_match = RUN_ID.fullmatch(run_id)
    if not run_match or run_epoch(run_id) is None:
        reasons.append("run id does not contain the required UTC timestamp and PID")
    elif not any(
        line == f"RUN_START: bridge starting pid={run_match.group('pid')}"
        for line in lines
    ):
        reasons.append("RUN_START PID does not match the run id")

    required_active = f"ACTIVE: redirected {expected_redirects} Vulkan entry points"
    if required_active not in lines:
        reasons.append(f"missing exact activation record: {required_active}")
    descriptor_mode = (
        "MODE: descriptor compatibility enabled live_resources=1 "
        "metal_argument_buffers=0"
    )
    legacy_allocation_mode = (
        "MODE: legacy allocation enabled live_resources=1 "
        "metal_argument_buffers=0 use_mtlheap=0"
    )
    reset_resource_trace_mode = (
        "MODE: reset resource trace enabled live_resources=1 "
        "metal_argument_buffers=0 use_mtlheap=1"
    )
    no_command_pooling_mode = (
        "MODE: command pooling disabled live_resources=1 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=0"
    )
    render_audit_mode = (
        "MODE: render audit enabled live_resources=1 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1"
    )
    reset_no_pipeline_cache_mode = (
        "MODE: reset pipeline cache bypass enabled live_resources=1 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1"
    )
    full_lifetime_audit_mode = (
        "MODE: full lifetime audit enabled live_resources=1 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1"
    )
    texture_cache_fix_mode = (
        "MODE: texture cache fix enabled live_resources=1 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1"
    )
    legacy_feature_profile_mode = (
        "MODE: legacy feature profile enabled live_resources=1 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1"
    )
    performance_safe_mode = (
        "MODE: performance safe enabled live_resources=1 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
        "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
        "lifecycle_trace=0"
    )
    performance_aggressive_mode = (
        "MODE: performance aggressive enabled live_resources=0 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
        "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
        "lifecycle_trace=0"
    )
    startup_color_audit_mode = (
        "MODE: startup color audit enabled live_resources=0 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
        "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
        "generation_limit=2 generation_2_present_limit=180"
    )
    startup_fx_neutralize_mode = (
        "MODE: startup FX neutralize enabled live_resources=0 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
        "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
        "generation_limit=2 generation_2_present_limit=180"
    )
    startup_present_pixel_audit_mode = (
        "MODE: startup present pixel audit enabled live_resources=0 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
        "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
        "generation_limit=2 generation_2_present_limit=180 pixel_samples=20"
    )
    startup_draw_audit_mode = (
        "MODE: startup draw audit enabled live_resources=0 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
        "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
        "generation_limit=2 generation_2_present_limit=180 "
        "pixel_samples=20 draw_provenance=enabled"
    )
    startup_input_audit_mode = (
        "MODE: startup input audit enabled live_resources=0 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
        "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
        "generation_limit=2 generation_2_present_limit=180 "
        "pixel_samples=20 draw_provenance=enabled input_provenance=enabled"
    )
    startup_compositor_audit_mode = (
        "MODE: startup compositor audit enabled live_resources=0 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
        "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
        "generation_limit=2 generation_2_present_limit=180 "
        "pixel_samples=20 draw_provenance=enabled input_provenance=enabled "
        "descriptor_classes=enabled"
    )
    startup_compositor_neutralize_mode = (
        "MODE: startup compositor neutralize enabled live_resources=0 "
        "metal_argument_buffers=0 use_mtlheap=1 command_pooling=1 "
        "synchronous_queue_submits=0 maximize_concurrent_compilation=1 "
        "generation_limit=2 generation_2_present_limit=180 "
        "draw_provenance=enabled input_provenance=enabled "
        "pixel_readback=disabled fallback=forward"
    )
    matched_modes = [
        mode
        for mode in (
            descriptor_mode,
            legacy_allocation_mode,
            reset_resource_trace_mode,
            no_command_pooling_mode,
            render_audit_mode,
            reset_no_pipeline_cache_mode,
            full_lifetime_audit_mode,
            texture_cache_fix_mode,
            legacy_feature_profile_mode,
            performance_safe_mode,
            performance_aggressive_mode,
            startup_color_audit_mode,
            startup_fx_neutralize_mode,
            startup_present_pixel_audit_mode,
            startup_draw_audit_mode,
            startup_input_audit_mode,
            startup_compositor_audit_mode,
            startup_compositor_neutralize_mode,
        )
        if mode in lines
    ]
    if len(matched_modes) != 1:
        reasons.append("exactly one supported compatibility mode was not enabled")
    expected_mtlheap = 0 if legacy_allocation_mode in matched_modes else 1
    expected_command_pooling = (
        0 if no_command_pooling_mode in matched_modes else 1
    )
    performance_mode = (
        performance_safe_mode in matched_modes
        or performance_aggressive_mode in matched_modes
        or startup_color_audit_mode in matched_modes
        or startup_fx_neutralize_mode in matched_modes
        or startup_present_pixel_audit_mode in matched_modes
        or startup_draw_audit_mode in matched_modes
        or startup_input_audit_mode in matched_modes
        or startup_compositor_audit_mode in matched_modes
        or startup_compositor_neutralize_mode in matched_modes
    )
    expected_live_resources = (
        0
        if (
            performance_aggressive_mode in matched_modes
            or startup_color_audit_mode in matched_modes
            or startup_fx_neutralize_mode in matched_modes
            or startup_present_pixel_audit_mode in matched_modes
            or startup_draw_audit_mode in matched_modes
            or startup_input_audit_mode in matched_modes
            or startup_compositor_audit_mode in matched_modes
            or startup_compositor_neutralize_mode in matched_modes
        )
        else 1
    )
    expected_synchronous_submits = (
        0 if performance_mode else 1
    )
    expected_concurrent_compilation = (
        1 if performance_mode else 0
    )
    expected_configuration = (
        f"MOLTENVK_CONFIG: live_resources={expected_live_resources} "
        "metal_argument_buffers=0 "
        f"use_mtlheap={expected_mtlheap} "
        f"synchronous_queue_submits={expected_synchronous_submits} "
        f"command_pooling={expected_command_pooling} prefill=0 "
        f"maximize_concurrent_compilation={expected_concurrent_compilation}"
    )
    if expected_configuration not in lines:
        reasons.append("the effective MoltenVK configuration was not verified")
    if (
        "HDR_COMPAT: filter=enabled extension=VK_EXT_hdr_metadata"
        not in lines
    ):
        reasons.append("HDR advertisement filter was not enabled")
    if (
        "HDR_SURFACE_COMPAT: filter=enabled format=64 colorSpace=1000104008"
        not in lines
    ):
        reasons.append("the exact ESO HDR surface-format filter was not enabled")
    if not any(
        line.startswith("MOLTENVK: loaded path=") for line in lines
    ):
        reasons.append("the replacement MoltenVK load was not recorded")
    if not any(
        line.startswith("GIPA: ")
        and "name=vkEnumerateDeviceExtensionProperties" in line
        and "shim=hdr-filter" in line
        for line in lines
    ):
        reasons.append("GIPA did not route device-extension enumeration through the filter")
    if not any(
        line.startswith("GIPA: ")
        and "name=vkCreateDevice" in line
        and "shim=device-trace" in line
        for line in lines
    ):
        reasons.append("GIPA did not route vkCreateDevice through the trace wrapper")
    if not any(
        line.startswith("GIPA: ")
        and "name=vkGetPhysicalDeviceSurfaceFormatsKHR" in line
        and "shim=surface-format-filter" in line
        for line in lines
    ):
        reasons.append("GIPA did not route surface-format enumeration through the filter")
    if legacy_feature_profile_mode in matched_modes:
        if not any(
            line.startswith("GIPA: ")
            and "name=vkGetPhysicalDeviceFeatures" in line
            and "shim=legacy-feature-profile" in line
            for line in lines
        ):
            reasons.append(
                "GIPA did not route physical-device features through the legacy profile"
            )
        feature_profile_lines = [
            line
            for line in lines
            if line.startswith("LEGACY_FEATURE_PROFILE: ")
        ]
        if not feature_profile_lines or any(
            not LEGACY_FEATURE_PROFILE.fullmatch(line)
            for line in feature_profile_lines
        ):
            reasons.append(
                "the legacy physical-device feature mask was not exact"
            )
    startup_audit_mode = (
        startup_color_audit_mode in matched_modes
        or startup_fx_neutralize_mode in matched_modes
        or startup_present_pixel_audit_mode in matched_modes
        or startup_draw_audit_mode in matched_modes
        or startup_input_audit_mode in matched_modes
        or startup_compositor_audit_mode in matched_modes
        or startup_compositor_neutralize_mode in matched_modes
    )
    if performance_mode and not startup_audit_mode:
        for name in PERFORMANCE_DIRECT_NAMES:
            records = [
                line
                for line in lines
                if line.startswith("GDPA: ") and f"name={name} " in line
            ]
            if not records or any("shim=none" not in line for line in records):
                reasons.append(
                    f"performance mode did not route {name} directly"
                )
        lifecycle_prefixes = (
            "DEVICE_WAIT_IDLE:",
            "SWAPCHAIN_CREATE:",
            "SWAPCHAIN_DESTROY:",
            "SWAPCHAIN_IMAGES:",
            "SWAPCHAIN_IMAGE:",
            "SWAPCHAIN_IMAGE_VIEW_",
            "SWAPCHAIN_RENDER_PASS_",
            "SWAPCHAIN_FRAMEBUFFER_",
            "SWAPCHAIN_ACQUIRE:",
            "SWAPCHAIN_PRESENT:",
        )
        if any(line.startswith(lifecycle_prefixes) for line in lines):
            reasons.append(
                "performance mode emitted lifecycle hot-path records"
            )
    if startup_audit_mode:
        if (
            "STARTUP_COLOR_AUDIT_BEGIN: generation_limit=2 "
            "generation_2_present_limit=180"
            not in lines
        ):
            reasons.append("startup color audit did not arm its bounded two-generation gate")
    if startup_fx_neutralize_mode in matched_modes:
        sentinel_begin = (
            "STARTUP_FX_SENTINEL_BEGIN: initializer_offset=0x35fcd42 "
            "window=generation-2-present-180 vectors=0x10,0x20,0x30 "
            "replacement=black-preserve-alpha"
        )
        if sentinel_begin not in lines:
            reasons.append("startup FX sentinel patch was not installed exactly")
    pixel_audit_mode = (
        startup_present_pixel_audit_mode in matched_modes
        or startup_draw_audit_mode in matched_modes
        or startup_input_audit_mode in matched_modes
        or startup_compositor_audit_mode in matched_modes
    )
    if pixel_audit_mode:
        pixel_begin = (
            "STARTUP_PRESENT_PIXEL_AUDIT_BEGIN: generation_1_samples=1 "
            "generation_2_samples=1,10,20,30,40,50,60,70,80,90,100,"
            "110,120,130,140,150,160,170,180"
        )
        pixel_ready = (
            "STARTUP_PRESENT_PIXEL_READY: synchronization=queue-wait-idle "
            "samples=20 points_per_sample=5"
        )
        if pixel_begin not in lines:
            reasons.append("startup present pixel schedule was not armed exactly")
        if pixel_ready not in lines:
            reasons.append("startup present pixel sampler was not ready exactly")
    draw_audit_mode = (
        startup_draw_audit_mode in matched_modes
        or startup_input_audit_mode in matched_modes
        or startup_compositor_audit_mode in matched_modes
        or startup_compositor_neutralize_mode in matched_modes
    )
    if draw_audit_mode and (
        "STARTUP_DRAW_AUDIT_BEGIN: generation_limit=2 "
        "generation_2_present_limit=180 max_distinct_pipelines_per_submit=8"
        not in lines
    ):
        reasons.append("startup draw provenance was not armed exactly")
    input_audit_mode = (
        startup_input_audit_mode in matched_modes
        or startup_compositor_audit_mode in matched_modes
        or startup_compositor_neutralize_mode in matched_modes
    )
    if input_audit_mode and (
        "STARTUP_INPUT_AUDIT_BEGIN: generation_limit=2 "
        "generation_2_present_limit=180 max_descriptor_set_layouts=2048 "
        "max_pipeline_layouts=2048 max_descriptor_sets=131072 max_bound_sets=16"
        not in lines
    ):
        reasons.append("startup input provenance was not armed exactly")
    if startup_compositor_audit_mode in matched_modes:
        if (
            "STARTUP_COMPOSITOR_AUDIT_BEGIN: image_bindings_per_set=2 "
            "sampled_subresources=base-mip-base-layer"
            not in lines
        ):
            reasons.append("startup compositor provenance was not armed exactly")
        if (
            "STARTUP_COMPOSITOR_IMAGE_READY: synchronization=queue-wait-idle "
            "points_per_image=5 formats=rgba8,bgra8,rgba16f"
            not in lines
        ):
            reasons.append("startup compositor image sampler was not ready exactly")
    if startup_compositor_neutralize_mode in matched_modes:
        if (
            "STARTUP_COMPOSITOR_NEUTRALIZE_BEGIN: generation=2 "
            "first_present=60 last_present=150 max_suppressed_draws=96 "
            "fallback=forward"
            not in lines
        ):
            reasons.append("startup compositor neutralizer was not armed exactly")
        if any(
            line.startswith("STARTUP_PRESENT_PIXEL_READY:")
            or line.startswith("STARTUP_COMPOSITOR_IMAGE_READY:")
            for line in lines
        ):
            reasons.append("startup compositor neutralizer enabled pixel readback")

    filter_lines = [line for line in lines if line.startswith("HDR_FILTER: ")]
    if not any(
        "query=count" in line and REMOVED_ONE.search(line)
        for line in filter_lines
    ):
        reasons.append("no count query proved that exactly one HDR extension was removed")
    if not any(
        "query=data" in line and REMOVED_ONE.search(line)
        for line in filter_lines
    ):
        reasons.append("no data query proved that exactly one HDR extension was removed")
    if any(not REMOVED_ONE.search(line) for line in filter_lines):
        reasons.append("at least one filtered enumeration did not remove exactly one HDR extension")

    surface_filter_lines = [
        line for line in lines if line.startswith("SURFACE_FORMAT_FILTER: ")
    ]
    if not any(
        "query=count" in line and REMOVED_ONE.search(line)
        for line in surface_filter_lines
    ):
        reasons.append("no surface-format count query removed the exact ESO HDR pair")
    if not any(
        "query=data" in line and REMOVED_ONE.search(line)
        for line in surface_filter_lines
    ):
        reasons.append("no surface-format data query removed the exact ESO HDR pair")
    if any(not REMOVED_ONE.search(line) for line in surface_filter_lines):
        reasons.append("at least one surface-format query removed an unexpected count")

    create_records: dict[str, tuple[int, str]] = {}
    create_extensions: dict[str, dict[int, str]] = {}
    create_results: dict[str, tuple[int, str]] = {}
    create_feature_profiles: dict[str, str] = {}
    malformed_create_lines: list[str] = []
    for line in lines:
        if line.startswith("CREATE_DEVICE: "):
            match = CREATE_DEVICE.fullmatch(line)
            if not match or match.group("call") in create_records:
                malformed_create_lines.append(line)
            else:
                create_records[match.group("call")] = (
                    int(match.group("count")),
                    match.group("hdr"),
                )
        elif line.startswith("CREATE_DEVICE_EXT: "):
            match = CREATE_DEVICE_EXT.fullmatch(line)
            if not match:
                malformed_create_lines.append(line)
            else:
                call_extensions = create_extensions.setdefault(
                    match.group("call"), {}
                )
                index = int(match.group("index"))
                if index in call_extensions:
                    malformed_create_lines.append(line)
                else:
                    call_extensions[index] = match.group("name")
        elif line.startswith("CREATE_DEVICE_RESULT: "):
            match = CREATE_DEVICE_RESULT.fullmatch(line)
            if not match or match.group("call") in create_results:
                malformed_create_lines.append(line)
            else:
                create_results[match.group("call")] = (
                    int(match.group("result")),
                    match.group("device"),
                )
        elif line.startswith("CREATE_DEVICE_FEATURE_PROFILE: "):
            match = CREATE_DEVICE_FEATURE_PROFILE.fullmatch(line)
            if not match or match.group("call") in create_feature_profiles:
                malformed_create_lines.append(line)
            else:
                create_feature_profiles[match.group("call")] = line

    if malformed_create_lines:
        reasons.append("malformed or duplicate vkCreateDevice diagnostic records")
    if not create_records:
        reasons.append("vkCreateDevice was not observed")
    elif any(hdr != "no" for _, hdr in create_records.values()):
        reasons.append("at least one VkDevice enabled VK_EXT_hdr_metadata")

    if any(
        name == "VK_EXT_hdr_metadata"
        for extensions in create_extensions.values()
        for name in extensions.values()
    ):
        reasons.append("the enabled device-extension list contains VK_EXT_hdr_metadata")

    if set(create_records) != set(create_results):
        reasons.append("vkCreateDevice request/result record counts differ")
    elif any(
        result != 0 or device in {"(nil)", "0x0", "0"}
        for result, device in create_results.values()
    ):
        reasons.append("at least one VkDevice creation failed or returned NULL")

    for call_id, (extension_count, _) in create_records.items():
        extensions = create_extensions.get(call_id, {})
        if set(extensions) != set(range(extension_count)):
            reasons.append(
                f"vkCreateDevice call {call_id} does not contain its exact indexed extension list"
            )
    if set(create_extensions) - set(create_records):
        reasons.append("device-extension records exist without a matching request")
    if legacy_feature_profile_mode in matched_modes:
        if set(create_feature_profiles) != set(create_records):
            reasons.append(
                "vkCreateDevice legacy feature-profile validation is incomplete"
            )
    elif create_feature_profiles:
        reasons.append(
            "legacy feature-profile validation appeared outside its selected mode"
        )

    if any(
        line.startswith("GDPA: ") and "name=vkSetHdrMetadataEXT" in line
        for line in lines
    ):
        reasons.append("ESO still queried vkSetHdrMetadataEXT after HDR filtering")
    if any("ERROR:" in line or "FATAL:" in line for line in lines):
        reasons.append("the selected run contains an error or fatal record")

    return Verdict(run_id, tuple(reasons))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument("--after-epoch", type=float)
    parser.add_argument("--expected-redirects", type=int, default=17)
    args = parser.parse_args()

    verdict = evaluate_startup_log(
        args.log.read_text(encoding="utf-8", errors="replace"),
        after_epoch=args.after_epoch,
        expected_redirects=args.expected_redirects,
    )
    print(f"startup run: {verdict.run_id or 'none'}")
    if verdict.passed:
        print("startup bridge verdict: PASS")
        return
    print("startup bridge verdict: FAIL")
    for reason in verdict.reasons:
        print(f"- {reason}")
    raise SystemExit(2)


if __name__ == "__main__":
    main()
