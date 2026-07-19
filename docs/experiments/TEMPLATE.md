# Experiment NNNN: short subject

- Date: YYYY-MM-DD
- Outcome: **planned**
- Rollback: **not started**

## Question

State the single question this run is intended to answer.

## Hypothesis

State the expected result and what observation would falsify it.

## Target and change set

Record sanitized executable fingerprints, runtime versions, patch-set identity,
configuration flags, and source commit. Do not include local paths or raw
proprietary artifacts.

## Preflight

- Confirm `scripts/status.sh` output and restore readiness.
- Confirm game, Steam, and launcher processes are stopped before installation.
- Rebuild from source and record validation results.
- Prepare evidence collection before modifying the game bundle.
- Record the user's explicit installation approval for this run outside the
  repository if it contains personal information.

## Procedure

List the controlled steps, including the Steam-authenticated launch path and the
planned stop condition. Agents must not launch ESO, Steam, or the launcher.

## Evidence

Separate timestamps, hashes, log excerpts, measurements, and crash facts from
interpretation. Sanitize before committing.

## Result

Record what happened, including absence of expected evidence.

## Interpretation

Label confirmed observations, inferences, and remaining hypotheses separately.

## Rollback

Record what was restored and how the restored state was verified.

## Follow-up

State the next gate. Link durable conclusions promoted to `docs/FINDINGS.md` and
update `docs/STATUS.md` without copying the full run narrative.
