# Documentation guide

This directory separates current project state from accumulated research
history. Read documents according to the question being answered instead of
treating every old result as current.

The project was promoted to production on 2026-08-01. Earlier research and
experiment documents remain immutable historical evidence; current production
claims belong in `PRODUCTION.md`, `STATUS.md`, and the top-level README.

## Where information belongs

| Question | Authoritative document |
|---|---|
| What is the supported production release baseline? | [Production baseline](PRODUCTION.md) |
| What is safe and active right now? | [Project status](STATUS.md) |
| What should happen next? | [Roadmap](ROADMAP.md) |
| What has been established across experiments? | [Findings](FINDINGS.md) |
| How does the bridge work? | [Architecture](ARCHITECTURE.md) |
| How is a launcher update checked and rebased? | [Update runbook](UPDATES.md) |
| What does the production bridge log? | [Logging policy](LOGGING.md) |
| How is the public DMG built and released? | [Release packaging](RELEASE.md) |
| What happened in a specific run? | [Experiment index](experiments/README.md) |
| What do comparable external projects teach us? | [Research index](research/README.md) |
| Which unscheduled ideas are worth retaining? | [Miscellaneous notes](misc.md) |
| Which settings and operational workarounds are known? | [Settings](SETTINGS.md) and [Troubleshooting](TROUBLESHOOTING.md) |
| Which primary external sources support the research? | [Sources](SOURCES.md) |

`AGENTS.md` at the repository root owns standing workflow and safety rules.
`CHANGELOG.md` records repository releases. Neither file should accumulate test
results or a rolling narrative of the investigation.

## Update rules

- Update `STATUS.md` in place when the verified baseline, blocker, or next gate
  changes. Include the date and the command or experiment that supports it.
- Keep `ROADMAP.md` forward-looking. Move completed run details into an
  experiment record rather than retaining a diary in the roadmap.
- Treat experiment records as append-only evidence. Correct them with a dated
  amendment, and preserve the original observation.
- Store dated searches, precedent comparisons, and literature syntheses under
  `research/`. Keep their search limitations explicit and do not present an
  absence of public results as proof that no private or unindexed work exists.
- Promote a result to `FINDINGS.md` only when the evidence is durable enough to
  guide later work.
- Link to the owning document instead of copying exact hashes, crash details, or
  hypotheses into several files.
