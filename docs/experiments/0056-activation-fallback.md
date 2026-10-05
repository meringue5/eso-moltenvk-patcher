# Experiment 0056: one-shot activation fallback

- Date: 2026-09-30
- Outcome: **failed; the first retained one-shot request did not activate ESO**
- Rollback: **not performed; rc.4 remains installed with verified alternatives
  available (rc.3 observation-only, rc.2, or same-generation restore)**

## Question

Experiment 0055 showed that failed launches never receive AppKit activation.
Can a single activation request from ESO's own process, made when AppKit has
not activated ESO about 4 s after launch, prevent the detached-mouse,
low-FPS state without the user switching apps? And which application holds
activation when a launch fails?

## Context established before the change

- The ZeniMax launcher log records `Launching Process ...
  eso.app/Contents/MacOS/eso`. The launcher spawns the executable as a child
  process, not through LaunchServices, so macOS does not hand activation to
  the new app. Launch chain: Steam, `zosSteamStarterMac2`, the ZeniMax
  launcher (CEF), then `eso`.
- The launcher binary references `activateIgnoringOtherApps:` and
  `activateWithOptions:`. ESO references `activateIgnoringOtherApps:`,
  `activate`, and `makeKeyAndOrderFront:`. The call order was not traced.
- Non-game x86_64 probe on macOS 26.6.2. After the probe had yielded
  activation to Finder:
  - refused: `-[NSApplication activate]`, and window ordering alone
    (`orderFrontRegardless`, `makeKeyWindow`);
  - reactivated the probe: `-activateIgnoringOtherApps:YES`,
    `NSRunningApplication activateWithOptions:` with ignoring-other-apps, and
    `SetFrontProcessWithOptions`.

  This probe condition (self-yielded, then reclaiming) differs from a child
  process that never held activation. Whether the call is granted in the
  failing launch path is the open question.

## Change set

`0.2.1-rc.4` = rc.3 plus the following (release `0c4289a`, main `5889d73`).
ZIP SHA-256 `0051ea8c2789cff950b0c928b081292b740bf291b2e6144b3f1782191f12e244`;
bridge SHA-256
`645c396416eb12861a7c6b260c0bebd3f20de947c2a3017c484abeaf10c6989b`.

- Each `INACTIVE_PACING_EVENT` gains `front=`, a coarse category of the
  frontmost application: `self`, `zos-launcher`, `steam`, `other`, `none`,
  or `unknown`. No other application's identity is logged.
- `INACTIVE_PACING_ACTIVATION` records:
  - `phase=launch action=arm` at `NSApplicationDidFinishLaunching`;
  - after 4.0 s, `phase=check action=not-needed` if `NSApp` is active;
  - otherwise one `-activateIgnoringOtherApps:YES` (`phase=request`),
    followed 1.5 s later by `phase=result`.

  Each record includes `app_active`, `front`, `active_byte`, and `t_ms`.
- No input synthesis, no active-byte write, no private API, no retry loop.

## Preflight

- A harness running the exact shim block produced `not-needed` when active.
  After it yielded to Finder, `request` was followed by
  `result app_active=yes front=self`. With the cooperative `-activate` in the
  same position, the result was `app_active=no front=other`, which is why
  the legacy call is used.
- Build, Bink re-export, Rosetta self-patch, inactive pacing smoke, installer
  fixture, archive, and 143 Python tests pass. Main also builds.
- Installed in place with `--skip-settings` after the exact-target and idle
  gates. `UserSettings.txt` stayed `788c34ec...`, cache identities pass, and
  Status is `READY`.

## User request

Play normally. After each launch, report whether the mouse was detached or
FPS was low at start, and whether an app switch was still needed.

Stop after three launches in which the fallback fires
(`phase=request`), or after eight launches. Stop immediately if a crash,
pink screen, bridge error, or an unwanted focus jump occurs, such as ESO
stealing focus from an app the user deliberately switched to during
loading.

## Pass / fail criteria

- Pass: every launch with `phase=request` shows `phase=result
  app_active=yes`, followed by ESO `active=yes`, and the user reports no
  manual switch.
- Fail: `phase=request` with `result app_active=no`. In-process activation
  is then insufficient, and the remaining fix is on the launcher's hand-off.
- Launches with `not-needed` are controls. Their `front=` values at the
  early events show which app yields activation in successful launches.

## Earlier 2026-10-05 follow-up

The user recalled that later ordinary play may not have shown another major
problem, but explicitly said the memory was uncertain. This is retained as a
low-confidence follow-up only: it has no per-launch focus/FPS report and cannot
be counted toward the pass criteria or stop condition.

The latest retained rc.4 run,
`20260930T082554.639631000Z-pid70913`, was a control. AppKit made ESO key and
active, ESO's byte followed at 1,690 ms, and the one-shot check recorded
`action=not-needed` at 4,160 ms. It therefore supplies no evidence about whether
an actual `phase=request` repairs a failed launch. At the time of that note,
Experiment 0056 remained running.

## Result: one-shot request failed

Later on 2026-10-05, user-controlled run
`20261005T122449.163336000Z-pid8767` supplied the first retained case in which
the fallback fired. The bridge loaded normally: 17 redirects, inactive pacing
bypass, 79 suppressed draws, ordinal-150 forwarding, ordinal-180 finish, and
no bridge error.

The activation sequence was decisive:

| Time from pacing preparation | Observation |
|---:|---|
| 132 ms | Finish-launch event; `front=zos-launcher`, AppKit inactive, ESO byte false |
| 2,428 ms | First pacing sample `active=no` |
| 4,137 ms | One-shot `activateIgnoringOtherApps:` request; `front=self`, AppKit inactive, ESO byte false |
| 5,641 ms | Request result still `app_active=no front=self active_byte=no` |
| 36,361--36,368 ms | After the user's Cmd-Tab cycle, window-key and app-active events arrived |
| 36,453 ms | ESO byte changed to `active=yes` |

The user reported low FPS after login until Cmd-Tab out and back. Unlike the
earlier failures, mouse focus was not visibly detached. The log cannot measure
either property, but it independently aligns the manual switch with AppKit
activation and ESO's state transition.

This meets the experiment's explicit fail criterion. A one-shot activation
request from the child game process is insufficient even when ESO is already
the frontmost category. More repetitions of rc.4 are not required. The
remaining repair boundary is the ZeniMax/Steam-to-ESO activation hand-off;
designing or changing that path is separate work and was not performed here.
