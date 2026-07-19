# Documentation guide

This directory separates current project state from accumulated research
history. Read documents according to the question being answered instead of
treating every old result as current.

## Where information belongs

| Question | Authoritative document |
|---|---|
| What is safe and active right now? | [Project status](STATUS.md) |
| What should happen next? | [Roadmap](ROADMAP.md) |
| What has been established across experiments? | [Findings](FINDINGS.md) |
| How does the bridge work? | [Architecture](ARCHITECTURE.md) |
| What happened in a specific run? | [Experiment index](experiments/README.md) |
| Which settings and operational workarounds are known? | [Settings](SETTINGS.md) and [Troubleshooting](TROUBLESHOOTING.md) |
| Which external material supports the research? | [Sources](SOURCES.md) |

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
- Promote a result to `FINDINGS.md` only when the evidence is durable enough to
  guide later work.
- Link to the owning document instead of copying exact hashes, crash details, or
  hypotheses into several files.
