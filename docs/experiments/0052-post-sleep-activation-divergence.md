# Experiment 0052: post-sleep activation-state divergence

- Date: 2026-08-30
- Outcome: **active reliability investigation; production regression not accepted**
- Rollback: **not applicable; the user installed public 0.2.0**

## Question

Why can a normal Steam-authenticated launch after the user's hibernation or
Deep Idle return leave ESO with detached mouse focus and approximately 40 FPS,
despite the public 0.2.0 production profile bypassing ESO's historic 100-ms
inactive pacing branch?

## Hypothesis

The recurring condition is a missed or misordered activation sequence between
macOS/Steam/ESO after a sleep boundary. ESO's internal AppKit-fed active byte
then remains false even though the window is visible. The 100-ms bypass removes
the older near-10-FPS result, but does not repair focus delivery or rule out a
separate approximately-40-FPS throttling/contended-start path.

The hypothesis is falsified if a failing run records `active=yes`, or if a
matched active/no-active pair shows the same focus and FPS result with the
same external lifecycle state.

## Target and change set

- ESO 12.0.8 / databuild `3288357`, exact SHA-256
  `a819aa2313e91676bdfa3987ae650d594a86faf2429ad56c736b5e6992680609`.
- Public ESO MoltenVK Patcher 0.2.0, bridge SHA-256
  `954e8ff6cd3aceb3bfd5f874140f655aac1672394f30557eaa3bff57896683ce`.
- Production mode: `startup-compositor-neutralize-pacing-release`, with Metal
  argument buffers disabled and no post-startup timing wrapper.
- User-installed settings profile is customized after install and is preserved.

## Preflight

- `scripts/check-update.sh` recognized the exact target before the 0.2.0
  install.
- Public Status reports a verified recovery record, exact compatibility, and
  the expected production runtime profile.
- No cache, settings, launcher, or bridge mutation is authorized for this
  investigation unless a separately scoped change passes the exact target,
  restore, source-build, and shared bundle-idle gates.

## Procedure

1. Preserve each natural recurrence before retrying: bridge run ID, production
   Status result, user focus/FPS observation, Steam/launcher process boundary,
   and nearby `pmset` sleep/wake records.
2. First perform read-only analysis of the existing AppKit callback, active-byte,
   WindowServer, Steam, and sleep/wake ordering. Do not add from-first-frame
   runtime instrumentation to the production profile.
3. If existing evidence cannot order those events, design a separately scoped,
   bounded diagnostic that has no control effect and is disabled by default.
   It must pass an initial-focus acceptance gate before any performance claim.
4. Treat a public Status `Last launch: PASS` as structural-runtime validation
   only until its classifier is improved; it does not measure mouse capture or
   frame rate.

Agents must not launch ESO, Steam, or the launcher. The user does not accept
application switching as a required recovery action. Do not force ESO's active
byte, synthesize AppKit activation, delete caches, or resume manual swapchain
A/B testing for this issue.

## Evidence

The user installed the public 0.2.0 package and reported that the hibernation
condition recurred with detached mouse focus and roughly 40 FPS, rather than
the historical near-10-FPS behavior.

Run `20260830T090357.701770000Z-pid36636` (18:03:57 KST) confirms the expected
public mode, MoltenVK 1.4.2 configuration, all 17 redirects, 79-draw startup
suppression, ordinal-150 forwarding, and ordinal-180 completion. Its first and
only recorded active-state observation is `active=no action=sleep-bypassed`.
The production profile intentionally omits post-window timing, so the bridge
log does not independently measure the user-observed approximately-40-FPS rate.

Public Status reports the client as exact and installed with verified recovery;
it reports the run as structurally passing while explicitly stating that it does
not prove visual focus or FPS. No thermal or CPU power warning was recorded.

`pmset` contains repeated Deep Idle and maintenance wake records on the same
day. The available current sample does not yet join the 18:03:57 KST launch to
one specific FullWake timestamp, so the exact sleep-to-launch interval remains
unproven for this run. The user's repeated hibernation correlation and the
earlier Deep Idle-associated `active=no` runs make that boundary the leading
condition to investigate, not a proven sole cause.

## Result

Public 0.2.0 does not eliminate the post-sleep focus/FPS pattern. The result
is a production reliability regression and an immediate investigation priority.
It is distinct from the former fixed 100-ms, approximately-10-FPS path: that
sleep is bypassed in the failing production run.

## Interpretation

Confirmed: a public, exact-target 0.2.0 run can begin `active=no` while all
bridge startup invariants pass, and the user concurrently observes detached
mouse focus and approximately 40 FPS.

Confirmed: the bridge's pacing hook is observe-and-return only; it does not
repair activation delivery. Therefore the `active=no` record is diagnostic
evidence, not a focus fix.

Inference: a sleep-related lifecycle race is the leading mechanism. Steam or
launcher lifecycle state, AppKit event ordering, another OS input/focus layer,
and a separate 40-FPS performance path remain competing explanations.

Unresolved: whether `active=no` is sufficient to cause this 40-FPS class, and
which event ordering change can prevent it without reintroducing the old 10-Hz
pace or compromising input correctness.

## Rollback

No rollback is performed. The user-selected public 0.2.0 installation remains
in place with its recovery record, settings customization, and cache state.

## Follow-up

Make this the current P0 reliability gate. First improve evidence capture and
status classification without changing runtime control. Consider a behavior
change only after the causal ordering is established and it has a natural
initial-focus acceptance result, safe failure mode, and verified restore path.
