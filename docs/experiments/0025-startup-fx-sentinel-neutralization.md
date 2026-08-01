# Experiment 0025: bounded startup FX-sentinel neutralization

- Date: 2026-08-01
- Outcome: **inconclusive intervention; exact hook installed but initializer had zero calls during the bounded window**
- Rollback: **complete; normal `performance-aggressive` restored with all caches and settings preserved**

## Question

Does the exact-magenta default initialized for ESO's `FXMaterial` parameter
block supply the transient full-screen startup color observed after generation
2 becomes drawable?

## Hypothesis

If that default reaches the startup presentation, replacing only its RGB
components with black during the already validated two-generation audit window
will remove or change the pink frame. If the initializer is exercised and the
pink frame persists unchanged, this exact sentinel is excluded as its source.

An unexercised initializer, a non-matching output block, an unprofiled caller,
or failure to reach the ordinal-180 finish is inconclusive rather than a
negative result.

## Static connection and exact target

For the fingerprinted ESO 12.0.7 executable, the initializer at image offset
`0x35fcd42` writes these three consecutive vectors:

```text
offset 0x10: (1, 0, 1, 0)
offset 0x20: (1, 0, 1, 0)
offset 0x30: (1, 0, 1, 1)
```

Its only two direct call sites return at image offsets `0x1ba0dc` and
`0x1bb46d`. Their enclosing paths immediately select
`technique_FXMaterial` and `technique_FXMaterialTransparent`, respectively.
The nearby embedded shader metadata names the associated `cbFXMaterial`,
`FXMaterialPS`, `FXMaterialTransparentPS`, `FXMaterialVS`, and
`ZoFXMaterial.fx` objects. This is a materially stronger connection than the
earlier isolated `#FF00FF` constant, but it still does not statically prove
that this material writes the startup swapchain.

The exact 17-byte initializer prefix, its first 16-byte constant, both five-byte
direct calls, executable SHA-256, and Mach-O UUID are recorded in the selected
target manifest. Generation for another ESO executable fails closed, and the
fast update-rebase path refuses any manifest containing experimental patch
targets until they receive manual analysis.

## Candidate change set

The new `startup-fx-neutralize` mode retains the exact effective MoltenVK 1.4.2
`performance-aggressive` configuration and both existing pipeline caches and
settings. It also retains Experiment 0024's bounded lifecycle window:

- generation 1 plus generation 2 through present ordinal 180;
- no mutation after that atomic finish gate;
- first eight initializer events logged, then detail logging capped;
- normal mode never installs the FX patch.

The patch validates the original 17 bytes before writing. An executable
trampoline reproduces the displaced prologue and RIP-relative constant load,
then resumes at byte 17. The wrapper calls that complete original initializer
first. Only if all three vectors still exactly match the profiled magenta
pattern does it write `(0,0,0,0)`, `(0,0,0,0)`, and `(0,0,0,1)`. Alpha is
preserved, non-matching materials are unchanged, and the code page is returned
to RX protection. Failure to restore RX is fatal rather than continuing with a
writable code page.

This is a causal intervention, not another broad shader, texture, overlay,
resolution, cache, or configuration experiment.

## Non-game validation

No Steam, launcher, or ESO process was started or controlled, and no installed
file, cache, or setting was changed. A `/tmp` app view pointed the build at the
current executable, pristine Bink restore source, and bundled legacy MoltenVK.

The synthetic x86 probe installs the same 17-byte absolute jump and executable
trampoline into a private VM page, executes an initializer with the profiled
output, verifies exact black substitution, closes the bounded window, and
verifies that the next execution retains magenta. It also proves that a
one-byte mismatch prevents any substitution.

```text
FX sentinel smoke: PASS exact_match=1 bounded_window=1 trampoline=1
104 Python tests: PASS
full source build: PASS
all existing bridge smoke probes: PASS
startup-fx-neutralize MoltenVK configuration probe: PASS
python compileall, shell syntax, git diff check: PASS
```

## One-run decision table

After an explicitly approved installation, one user-controlled normal
Steam-path startup is sufficient. The user need only report whether the pink
frame appeared; the bridge log supplies the remaining classification.

| Visual result | Exact matched initializer event | Bounded finish | Verdict |
|---|---:|---:|---|
| Pink absent or visibly changed to black | yes | yes | FX sentinel is causal for the startup color |
| Same pink frame persists | yes | yes | This exact FX sentinel is excluded |
| Either result | no, malformed, or unprofiled | either | Inconclusive; do not interpret the visual result |
| Either result | yes | no | Inconclusive; bounded execution not proven |

`tools/analyze_startup_fx_neutralize.py` implements this table. The
`FX-SENTINEL-CAUSAL` and `FX-SENTINEL-EXCLUDED` verdicts both require an exact
installed-patch record, contiguous matched calls from a profiled caller, and
the generation-2 ordinal-180 finish.

## Installation checkpoint

The user explicitly approved `startup-fx-neutralize` installation. The update
gate recognized the exact ESO 12.0.7 executable and databuild. The existing
bridge was restored to the pristine Bink loader with cache preservation, then
the new source build was installed with the same preservation gate. Both
process checks completed without finding ESO, the launcher, or Steam.

Post-install verification establishes:

```text
marker: startup-fx-neutralize
installed proxy/build SHA-256: fc95d3c84d16609d5bd7e300a2753babc7ea262b72dca3681d19cf6c293e1aaf
installed MoltenVK/build SHA-256: aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f
active 1.4.2 cache SHA-256: 498afb3db97c57c6fe6b0baef5307bf0c6a9330a73478519caa6cf659474a55b
old-backup cache SHA-256: 72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c
settings SHA-256: 297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c
```

The cache and settings hashes match the prepared pre-install boundary exactly.
The ignored evidence boundary is
`artifacts/experiment-0025-20260801T090413Z`. No agent launched Steam, the
launcher, or ESO.

## User-run gate

One normal Steam-path startup is now justified. Observe only through account
login plus approximately 30 seconds, then quit normally. Stop earlier for a
crash, hang, or unexpected rendering. Report whether pink remained unchanged
or instead disappeared/changed to black. After evidence collection, restore
the normal `performance-aggressive` marker and verify all preserved hashes.

## User-run result

The user launched ESO through the normal Steam path and observed the same pink
frame. The exact bridge run is `20260801T091202.657581000Z-pid75132`. Automatic
startup verification passed, no crash report was produced, and all 48 settings
remained structurally identical.

The experimental patch installed exactly, and the two-generation audit
completed. It again recorded one generation-1 and 180 generation-2
full-surface clears, all opaque black `(0,0,0,1)`. But there were **zero**
`STARTUP_FX_SENTINEL` call records before the exact generation-2 ordinal-180
finish. The analyzer therefore reports:

```text
startup-fx-events: 0
startup-fx-verdict: INCONCLUSIVE
startup-fx-reason: the FX initializer was not observed inside the startup window
```

The unchanged pink frame cannot exclude the sentinel by the experiment's
decision table because no value was actually substituted. It does establish
that this initializer is not called after the bridge installs its hook and
before the bounded startup window ends. A material object created before the
bridge constructor remains a logical alternative, so the result is not
promoted to causal exclusion. Do not repeat this intervention; any successor
must target a later value copy/use or a presented draw rather than this
initializer call.

## Rollback result

Evidence was collected before rollback. The active 1.4.2 cache retained its
7,977,079-byte size and changed normally during the run to SHA-256
`aed8bce13b26a8d2760b69d34440d27b1bdf244b4cdee6490bd847f759b904ba`.
The old-backup cache remained
`72ac0b0dcb4a7bb3bb5b12b150fe923f5814cf38284eb0afe9b12ed6dea07e1c`,
and settings remained
`297f855804d9af13544331152976c468bc5a2f269daaeefaa9357353ecfacf2c`.

The pristine-loader restore and cache-preserving reinstall completed. The
current marker is `performance-aggressive`; installed proxy and official
MoltenVK match the validated build. No agent launched Steam, the launcher, or
ESO.
