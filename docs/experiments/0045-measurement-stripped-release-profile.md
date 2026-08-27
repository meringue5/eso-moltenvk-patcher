# Experiment 0045: measurement-stripped compositor/pacing release profile

- Date: 2026-08-27
- Outcome: **succeeded; exact packaged candidate passed the user-controlled launch**
- Rollback: **verified public-installer backup retained; Uninstall is available**

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

## User result

The exact packaged candidate passed on run
`20260827T071558.350339000Z-pid73173`. The user observed no pink and normal
performance, then raised graphics from mid-low to mid-high while on battery and
reported that it continued to run well for approximately one minute. The user
ended the test after macOS produced a low-battery warning. This short battery
observation is supporting confidence, not a sustained-performance or battery-
efficiency claim.

The bounded bridge log independently confirms:

```text
mode: startup-compositor-neutralize-pacing-release
pipeline_timing: disabled
post_window_bookkeeping: disabled
inactive_100ms_sleep: bypassed
inactive state observed: active=yes, action=forward
suppression: 79 exact draws, generation 2 ordinals 71-149
forward latch: one, generation 2 ordinal 150, present-deadline
finished: generation 2 ordinal 180
STARTUP_PIPELINE_* records: 0
errors or overflow records: 0
```

This exact run did not enter the inactive branch because the observed state was
active. Experiment 0044 already exercised the same compiled bypass with
`active=no` and `action=sleep-bypassed`; the release candidate retains that
exact patch identity while removing only diagnostic pipeline timing and post-
window lifecycle bookkeeping.

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

The complete non-game gate was repeated for the versioned 0.1.3 package:

```text
fresh bridge build and all bridge probes: PASS
Python tests: 138 PASS
release transaction fixture: PASS
same-target earlier-patcher Install/Uninstall upgrade: PASS
retained-bridge update refusal and generation rotation: PASS
Python compile, shell syntax, git diff check: PASS
official MoltenVK 1.4.2 Metal compatibility/surface probes: PASS on Apple M4
embedded MoltenVK 1.0.18 comparison probes: PASS on Apple M4
startup-surface Metal probe: PASS
runtime-readiness compiler canary: 10/10 PASS
internal package checksums and archive hygiene: PASS
candidate ZIP SHA-256: 26ca4273aae669231dcc3a04e998d59b74038361e97da0b5f746434c1d02a4d7
bridge SHA-256: 24735b44e83f1f6986cf2c36bca57616b8468fd026bc4ffa11062ed31a98f569
retagged original Bink SHA-256: f166982931adfef53a23165bc2f73be18016a9a25d1c396dbeb586109f1c9927
MoltenVK SHA-256: aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
```

After the shared bundle-idle gate passed, the Experiment 0044 bridge was
restored to the verified pristine loader with every pipeline-cache generation
preserved. The exact `Install.command` extracted from
`ESO-MoltenVK-Patcher-0.1.3.zip` then installed the candidate with settings
explicitly unchanged. Package Status reports release 0.1.3 on the exact target.
The installed bridge and MoltenVK hashes equal the packaged payloads, and the
following user-file hashes remained unchanged across the transaction:

```text
UserSettings.txt:                         104e894803e70dae30fdab887474a8f3116387375614484d36c3755c58745fb0
PipelineCache.esopc:                      34fb06cd57d009caf207a0ce7dcedd10c3c04e25932f1f9c9ac1311ff4a80ef8
PipelineCache.esopc.teso4m4-old-backup:  72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
```

The marker selects `startup-compositor-neutralize-pacing-release` and attests
the exact ESO SHA-256. The recovery state records release 0.1.3, the exact
original-loader generation, the bridge and MoltenVK hashes, and an installed
transaction phase. No game or launcher was started by the agent. The remaining
gate was the ordinary user-controlled launch recorded above, which passed.

## Rollback

No rollback has been required. The packaged Uninstall command retains the
verified original-loader backup and passed exact-generation and previous-
patcher removal tests in the disposable fixture.
