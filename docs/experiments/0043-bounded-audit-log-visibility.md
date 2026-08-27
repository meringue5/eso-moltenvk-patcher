# Experiment 0043: bounded compositor audit log visibility

- Date: 2026-08-27
- Outcome: **source and non-game gates passed; installation pending**
- Rollback: **not started; Experiment 0042 remains installed and active**

## Question

Can the already-running forward-only compositor audit preserve its bounded
pixel, draw, descriptor, and two-input image evidence under the production
default `info` log level, without enabling unbounded proc or lifecycle trace?

## Evidence selecting this gate

Experiment 0042 exact run `20260827T053611.563842000Z-pid26073` selected the
correct mode and produced normal FPS with visible pink, but the analyzer found
none of its required audit records. The log classifier assigns generic
`STARTUP_PRESENT_*`, `STARTUP_DRAW_*`, `STARTUP_INPUT_*`, and compositor-image
messages to `debug`, and assigns `STARTUP_COLOR_*` to `trace`. The default
production level is `info`, so the evidence was deterministically filtered.

## Controlled change

Keep the complete Experiment 0042 runtime, inactive pacing bypass, MoltenVK
configuration, cache state, settings, draw forwarding, sampling schedule, and
analyzer unchanged. Promote only these already-bounded audit families to
`info`:

- color-audit begin and finish;
- `STARTUP_PRESENT_*`;
- draw-audit begin;
- input-audit begin;
- compositor-audit begin; and
- `STARTUP_COMPOSITOR_IMAGE_*`.

Generic `GIPA`, `GDPA`, detailed color-clear records, and other trace/debug
families remain filtered. Error classification remains unchanged.

## Gates

Before installation, require a fresh complete bridge build, lifecycle and mode
probes, analyzer regression, all Python tests, release transaction regression,
static checks, and the official/embedded Metal-backed non-game probes. Install
only after ESO and the launcher close and the shared bundle-idle gate passes;
preserve all settings and caches.

## Planned user gate

One ordinary Steam-path launch with a report of pink visibility and FPS state.
Require the exact mode, all bounded begin/ready/finish markers, twenty aligned
samples, no audit error or overflow, and a decisive compositor-input verdict.

## Result

The logging policy is now a separately compiled module with a dedicated probe.
The probe directly verifies that every analyzer-required bounded family is
`info`, detailed color/GIPA records remain `trace`, generic lifecycle detail
remains `debug`, and error/warning precedence remains intact.

```text
fresh complete bridge build: PASS
bounded log-policy probe: PASS
  bounded audit=info, detail=trace, generic=debug
Bink re-export and Rosetta self-patch: PASS
inactive pacing and lifecycle/image probes: PASS
combined MoltenVK mode configuration: PASS
Python tests: 135 PASS
release installer transaction regression: PASS
Python compile, shell syntax, git diff check: PASS
official MoltenVK 1.4.2 Metal compatibility/surface probes: PASS on Apple M4
embedded MoltenVK 1.0.18 comparison probes: PASS on Apple M4
bridge SHA-256: 13cfbe01e6427b26f5a3a1dbf36a85627e35014324d1770315406824191f5d34
MoltenVK SHA-256: aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
```

No running game or bundle file was modified during preparation. Installation
and the replacement user launch remain pending.

## Rollback

Not started. The pristine loader remains available.
