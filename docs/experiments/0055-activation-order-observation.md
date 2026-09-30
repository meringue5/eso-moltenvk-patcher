# Experiment 0055: activation-order observation

- Date: 2026-09-30
- Outcome: **succeeded; hypothesis falsified (fault precedes ESO)**
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

## Result (2026-09-30)

The user reported four consecutive natural launches: success, failure,
failure, success. That meets the stop condition. Every run contained the
observer, 17 redirects, the ordinal-150 latch and ordinal-180 finish, and no
bridge error. `t_ms` is measured from pacing preparation.

| Run | User | AppKit key/active | ESO first state |
|---|---|---|---|
| `20260930T052813.531663000Z-pid54498` | success | key 1307, active 1310 | `active=yes` 1587 |
| `20260930T062356.750483000Z-pid54847` | failure | none recorded | `active=no` 1692 |
| `20260930T062446.071684000Z-pid54864` | failure | none recorded | `active=no` 1625 |
| `20260930T062512.704249000Z-pid54868` | success | key 1939, active 1943 | `active=yes` 2205 |

In all four runs, `NSApplicationDidFinishLaunching` arrived at 106--183 ms,
and the window posted occlusion changes about 1.3--1.9 s later. The window
was therefore on screen in every run.

In both successful runs, `NSWindowDidBecomeKey` and
`NSApplicationDidBecomeActive` arrived together. ESO's byte followed
262--277 ms later, with no user action.

In both failed runs, AppKit posted neither event. ESO's byte stayed false
until each run's last record. Both failed runs were short (about 50 s and
26 s); the log shows no later activation, which is consistent with the user
quitting and relaunching rather than switching apps.

The unified log retained no activation records for this interval.

## Interpretation

Confirmed: ESO's active byte tracks AppKit activation. It became true within
about 0.3 s whenever AppKit made the application active and its window key.
The hypothesis that ESO misses an activation that AppKit delivered is
falsified. In the failing launches, macOS never made ESO the active
application, although its window was visible. This matches the user's
detached mouse. The bridge's inactive pacing bypass is not the cause: it only
removes the 100-ms sleep and cannot deliver activation.

Inference: the fault lies in launch-time application activation between the
launching app (the ZeniMax launcher and/or Steam) and ESO. A likely mechanism,
not yet tested, is macOS 14+ cooperative activation. There, a newly launched
app becomes active only if the active app yields, and legacy
`activateIgnoringOtherApps:` requests may be ignored. ESO 12.1.5 is built for
a 10.13 minimum with SDK 13.1. It is unknown which activation call ESO makes,
and which app was frontmost in the failed runs.

## Next gate

1. Agent-only: find ESO's activation calls statically (for example
   `activateIgnoringOtherApps:` or `NSRunningApplication activate` selector
   references), and record which launcher/Steam processes exist at launch.
2. Observation extension: log the frontmost application's bundle identifier
   at each event through `NSWorkspace` notifications, still observe-only.
3. A behavior change stays outside the current authorization and needs
   explicit user approval. One example is a bounded, public-API request for
   ESO's own activation when AppKit has not activated it after launch. The
   ROADMAP guardrails (no forced active byte, no synthesized input, no
   private activation API) remain in force.

## 2026-09-30 static amendment

The 12.1.5 executable's selector strings include `activateIgnoringOtherApps:`,
`makeKeyAndOrderFront:`, and a bare `activate`. `LSMinimumSystemVersion` is
10.13. Starting with macOS 14, `activateIgnoringOtherApps:` no longer forces
activation and takes part in cooperative activation. This is consistent with
the inference above: ESO requests activation the legacy way, and whether it
wins depends on the launching app yielding. Call sites and ordering were not
traced. This remains an inference, not a demonstrated mechanism.
