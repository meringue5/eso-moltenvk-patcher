# AGENTS.md

This file contains standing instructions for coding agents working on
`teso4m4`. Keep changing project state and experiment history in `docs/` rather
than accumulating it here.

## Mission

`teso4m4` investigates Steam ESO behavior and performance on Apple Silicon,
including ways to replace or bridge its statically linked MoltenVK runtime
without breaking Steam launch and authentication.

This is a research project, not a working gameplay mod. Accuracy, reversibility,
and evidence preservation take priority over obtaining a quick FPS result.

## Start every task here

1. Run `git status --short --branch` and do not overwrite unrelated work.
2. Read these files in order:
   - `README.md`
   - `docs/README.md`
   - `docs/STATUS.md`
   - `docs/ROADMAP.md`
   - `docs/ARCHITECTURE.md`
3. Use `docs/experiments/README.md` to find only the experiment records relevant
   to the task. Do not infer current state from an old experiment.
4. Confirm that the active game loader is original before assuming a clean
   baseline. `scripts/status.sh` is the non-destructive status check.
5. Rebuild from source after bridge changes. Never rely on a stale `build/`
   directory.

## Documentation ownership

- `AGENTS.md`: stable operating and safety rules only.
- `docs/STATUS.md`: current verified baseline, active blocker, and next gate.
- `docs/ROADMAP.md`: ordered future work, not completed experiment history.
- `docs/FINDINGS.md`: durable observations promoted from completed work.
- `docs/experiments/`: immutable per-run intent, procedure, evidence, result,
  interpretation, and rollback state.
- `docs/research/`: dated external searches, precedent reviews, and literature
  syntheses; these are context for decisions, not local experiment evidence.
- `CHANGELOG.md`: repository release history, not research history.

When information changes, update its owner and link to it instead of copying it
into multiple files. Preserve superseded experiment conclusions as dated
amendments; do not rewrite the original observation.

## Safety rules

- Never launch ESO, Steam, or the launcher on the user's behalf. The user owns
  login and interactive game control.
- Never install the bridge or modify the game bundle without explicit approval.
- Never bypass the Steam authentication path or launch `eso.app` directly as a
  substitute for a Steam test.
- Never delete the pristine Bink backup, old pipeline cache, crash evidence, or
  user settings. Preserve and rename before replacement.
- Never use destructive Git commands or revert user changes.
- Unknown ESO hashes and UUIDs must fail closed.
- Validate every original patch-site byte before writing any patch.
- Resolve every destination before making any code page writable.
- Restore RX page permissions after patching, including error paths.
- A restore path must be available and checked before every install test.
- Treat a Steam update as an unknown build until fingerprints and offsets are
  re-established.

## Repository hygiene

Do not commit:

- ESO executables or app bundles
- Bink, Steam, ESO SDK, or extracted proprietary object files
- MoltenVK release binaries or archives
- pipeline/shader caches
- full `UserSettings.txt`
- raw `.ips` reports
- account names, email addresses, machine-specific absolute paths, or tokens

Small build fingerprints, offsets, byte signatures, sanitized settings excerpts,
and sanitized crash facts are allowed when needed for reproducibility.

`build/`, `vendor/`, logs, caches, dylibs, archives, and object files must remain
ignored. Fetch MoltenVK from the official pinned release through
`scripts/fetch-moltenvk.sh`.

## Build and validation

Normal local sequence:

```sh
./scripts/fetch-moltenvk.sh
./scripts/build.sh
python3 -m compileall -q tools
zsh -n scripts/*.sh
git diff --check
```

Expected non-game smoke checks from `scripts/build.sh`:

- Bink symbol re-export lookup succeeds.
- Rosetta self-patch probe changes its test result from 1 to 2.

The Vulkan compatibility probe may require execution outside a restricted
sandbox to access Metal. Running that probe is not permission to launch ESO.

## Installation gate

The installer is intentionally blocked unless the caller sets:

```sh
TESO4M4_EXPERIMENTAL=I_ACCEPT_CRASH_RISK
```

That environment variable is not standing user approval. An agent must still
receive explicit approval for each modification of the game bundle.

Consult `docs/STATUS.md` for the current experimental mode and known failure
state. Do not describe an unproven mode as a fix.

## Evidence standards

- Separate confirmed observation, inference, and hypothesis in documentation.
- Record exact build hashes, runtime versions, timestamps, and test modes.
- For performance A/B tests, hold zone, camera, resolution, settings, player
  density, and test duration as constant as possible.
- Capture FPS, GPU time, frame interval, app memory, Metal memory, and thermal
  state. FPS alone is insufficient.
- Do not claim a resource leak merely from increasing memory. The current
  evidence supports accumulation or retained state, not a proven leak source.
- The Metal HUD render-pass warning describes engine/render-graph behavior. Do
  not promise that a MoltenVK swap can automatically merge ESO render passes.

## Continuity protocol

At the end of substantial work:

1. Update the relevant experiment document with result, evidence, and rollback
   state, including failed experiments.
2. Promote only repeatable or independently supported observations to
   `docs/FINDINGS.md`.
3. Update `docs/STATUS.md` and `docs/ROADMAP.md` when the active blocker or next
   gate changes.
4. Update this file only when standing workflow or safety rules change.
5. Run static checks and confirm `git status` is understood.
6. Commit a coherent unit of work with a message that names the experiment or
   analysis performed.

Conversation history is supporting context. The repository documents and Git
history are the authoritative source of project state.
