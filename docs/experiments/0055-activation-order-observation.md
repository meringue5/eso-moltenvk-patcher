# Experiment 0055: activation-order observation

- Date: 2026-09-30
- Outcome: **running; observation build installed, awaiting natural launches**
- Rollback: **not required; reinstall 0.2.1-rc.2 or restore via the verified
  same-generation backup**

## Question

When ESO starts with its internal active byte false, the user sees low FPS and
a detached mouse until they switch apps and back. In what order do AppKit
activation, key-window, and occlusion events arrive relative to ESO's active
byte, in launches with the symptom and in launches without it? Experiment 0052
established that this happens without a sleep boundary.

## Hypothesis

In affected launches, AppKit reports that the application became active and
its window became key, but ESO's active byte stays false. ESO therefore
missed or mis-ordered an activation it depends on. The hypothesis is falsified
if AppKit itself does not report the application active (or the window key)
until the user's app switch, which would place the fault before ESO: in
launcher or Steam hand-off, or in window-server ordering.

## Target and change set

- ESO 12.1.5 / databuild 3303624, target `targets-eso-2026-09-30.json`.
- `0.2.1-rc.3` = 0.2.1-rc.2 runtime plus observation only
  (release `b1d8351`, main `f4b033a`). ZIP SHA-256
  `78a4c0bc8e0c13a25e145c746f9bc853f9f3020bc70452846062410933526bf9`;
  bridge SHA-256
  `c025c5bef5db70fad9b9ff3d6fe88b04c082962a1942b62d23789b1cb3fbc1f3`.
- `INACTIVE_PACING_STATE` gains `t_ms`, measured from pacing preparation on
  the uptime clock.
- `INACTIVE_PACING_OBSERVER` and at most 48 `INACTIVE_PACING_EVENT` records
  cover the following notifications, received through CoreFoundation's local
  center, each with ESO's active byte and `t_ms`:
  - `NSApplicationDidFinishLaunching`
  - `NSApplicationDidBecomeActive`
  - `NSApplicationDidResignActive`
  - `NSApplicationDidUnhide`
  - `NSWindowDidBecomeKey`
  - `NSWindowDidResignKey`
  - `NSWindowDidChangeOcclusionState`
- Callbacks read and log only. There is no focus, activation, AppKit, or ESO
  state mutation, and the runtime control profile is unchanged.

## Preflight

- A non-game probe verified that CoreFoundation's local center receives
  AppKit notifications posted through `NSNotificationCenter` under Rosetta
  x86_64.
- Inactive pacing smoke: `timed_states=2 observed_events=48 event_limit=1`,
  forwarding unchanged by observation. Build, Bink re-export, Rosetta
  self-patch, installer fixture, archive, and Python tests all pass.
- Installed in place over rc.2 with `--skip-settings` after the exact-target
  and bundle-idle gates. `UserSettings.txt` stayed `788c34ec...`; all
  pipeline-cache identities pass; Status `READY`.
- Known limitation: public Diagnostics exports `INACTIVE_PACING_STATE` but not
  `INACTIVE_PACING_EVENT`. Analysis reads the local production log directly.

## User request

- Why agent-only evidence is insufficient: activation order exists only in a
  real launch through the user's Steam or launcher path. The unified log did
  not retain ESO activation events for the 14:19 run.
- Action: launch ESO normally, as ordinary play requires. Record whether the
  mouse was detached or FPS was low at start, and whether an app switch was
  needed. Do not change settings or force a symptom.
- Duration: normal play; the evidence is complete about two minutes after
  world entry.
- Stop condition: two launches with the symptom and one without, or five
  launches, whichever comes first. Stop immediately on a crash, pink screen,
  or bridge error.
- Evidence: the run IDs with their `INACTIVE_PACING_*` records, plus the
  user's per-launch symptom report.

## Pass / fail criteria for the evidence

The evidence is usable if each launch contains `INACTIVE_PACING_OBSERVER`, at
least one `INACTIVE_PACING_EVENT`, timestamped states, and no bridge error.
Evidence supports the hypothesis if affected launches show
`NSApplicationDidBecomeActive` or `NSWindowDidBecomeKey` while
`active_byte=no`, with no ESO transition until the manual switch. Evidence
falsifies it if affected launches lack those events until the manual switch.
