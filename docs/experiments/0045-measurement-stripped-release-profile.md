# Experiment 0045: measurement-stripped compositor/pacing release profile

- Date: 2026-08-27
- Outcome: **planned; source and non-game preparation in progress**
- Rollback: **not applicable; installed Experiment 0044 state unchanged**

## Question

Can Experiment 0044's verified inactive-pacing bypass and fixed-window
compositor repair be packaged without its diagnostic graphics-pipeline timing
or any lifecycle-table bookkeeping after the bounded startup audit finishes?

## Evidence selecting this change

Experiment 0044 passed two consecutive ordinary starts with no pink and normal
FPS, including one `active=no` run that exercised the exact 100-ms sleep bypass.
That result validates the functional combination, but the experiment mode also
clocks and records the first 64 graphics-pipeline calls.

Source inspection additionally found that descriptor-set-layout and
pipeline-layout destruction, descriptor-set free, descriptor-pool reset and
destruction, shader-module destruction, and graphics-pipeline destruction did
not all honor the startup-finished gate. ESO retains intercepted Vulkan
function pointers for the process lifetime, so those paths could continue
locking and mutating lifecycle tables after ordinal 180 even though the repair
had already latched to forwarding at ordinal 150.

## Controlled change

Add isolated source-maintenance mode
`startup-compositor-neutralize-pacing-release`:

- retain the exact Experiment 0044 MoltenVK configuration;
- retain the exact inactive 100-ms sleep bypass;
- retain bounded pipeline/layout/descriptor/draw/swapchain/present identity
  needed by the fail-open compositor neutralizer;
- retain the exact generation-2 ordinal 71-149 suppression window and
  ordinal-150 forwarding latch;
- disable graphics-pipeline timing, including its clock and counter setup;
- keep pixel readback, compositor image sampling, and readiness canary off; and
- make all retained lifecycle wrappers direct-forward without table mutation or
  diagnostic logging after the ordinal-180 finished gate.

Experiment 0044 remains unchanged as historical evidence. This is a distinct
release profile rather than a reinterpretation of its mode.

## Non-game gates

Before installation require:

1. exact ESO update/profile and original-loader checks;
2. fresh source build and all existing bridge probes;
3. exact MoltenVK configuration equivalence with Experiment 0044;
4. lifecycle proof that compositor identity tracking remains active while no
   `STARTUP_PIPELINE_*` record is emitted;
5. analyzer rejection of any pipeline-timing record in release mode;
6. source or probe evidence that no tracked destroy/free/reset path mutates
   lifecycle tables after the finished gate;
7. Python, shell, release-transaction, and diff checks; and
8. official and embedded Metal-backed probes before touching the game bundle.

## User gate

After all non-game gates pass, perform one cache- and settings-preserving
installation under the standing verified maintenance authorization. The agent
must not launch ESO, Steam, or the launcher. One ordinary user-controlled
Steam-path start is sufficient for this binary-difference gate.

Pass requires normal FPS, no visible pink, exact pacing activation, 79
contiguous target suppressions at ordinals 71-149, the single ordinal-150
forwarding latch, ordinal-180 finish, no `STARTUP_PIPELINE_*` records, and no
lifecycle error or overflow. Any low-FPS result fails regardless of pink.

## Preparation result

The separate mode is implemented in source. Its configuration probe matches
Experiment 0044: live-resource checks off, Metal argument buffers off, safe
MTLHeap use, asynchronous queue submission, command pooling on, no command
prefill, and non-maximized concurrent compilation. Pipeline timing is not
enabled for the new mode, and timing-origin clock setup now occurs only when a
timing mode is explicitly enabled.

Every identified tracked destroy/free/reset path now checks the common
startup-finished gate before locking or mutating lifecycle tables. The first
fresh build passed the existing bridge, inactive-pacing, lifecycle, logging,
reset, render-audit, and MoltenVK configuration probes. The full Python suite,
release transaction fixture, Python compilation, shell syntax, and diff checks
also passed before this record was written.

The installed game bundle remains on Experiment 0044. Experiment 0045 is not
yet an installed or gameplay-validated release candidate.

## Rollback

No rollback was required because source preparation did not modify the game
bundle. The verified pristine loader and Experiment 0044 installation remain
unchanged.
