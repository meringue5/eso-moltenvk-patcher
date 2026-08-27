# Production logging policy

The production bridge writes its log to:

```text
~/Library/Logs/ESO MoltenVK Patcher/bridge.log
```

If that directory cannot be created, it falls back to the legacy temporary
log path. The normal installer and status tool should surface the production
log path rather than asking players to inspect `/tmp`.

## Levels

| Level | Intended use | Included by default |
|---|---|---|
| `error` | A bridge safety check, dependency, or patch transaction failed | Yes |
| `warn` | The bridge intentionally declined to activate | Yes |
| `info` | Startup, selected mode, runtime compatibility, activation, and the bounded compositor-neutralizer outcome | Yes |
| `debug` | Diagnostic state from bounded maintenance instrumentation | No |
| `trace` | Vulkan proc lookups, pointer values, per-frame, and per-draw records | No |

The default is `info`. In 0.2.0 it deliberately excludes raw pointer values,
complete GIPA/GDPA lookup traces, and every individual suppressed startup draw.
The begin record, ordinal-150 latch with total suppression count, ordinal-180
finish, mode, pacing, configuration, and activation records remain. These records are
valuable only when a developer is diagnosing a bounded issue and add no value
to routine player support.

For a source-maintenance run, set `TESO4M4_LOG_LEVEL` to `error`, `warn`,
`info`, `debug`, or `trace` before starting the launcher. The planned
`ESO MoltenVK Patcher.app` will expose a support-diagnostics switch instead of
requiring players to set an environment variable.

## Retention and support

Version 0.2.0 rotates `bridge.log` at 1 MiB, retains one `bridge.log.1`
generation, and opens both with owner-only permissions. Rotation or logging
failure never weakens the runtime's fail-closed patch validation.

The public `Diagnostics.command` is the user-controlled **Export Support
Report** action. It exports checksums, selected client and installer-state
summaries, and only allowlisted events from the latest bridge run. It excludes
ESO credentials, full user settings, caches, proprietary game files, home
paths, raw pointer-bearing traces, and unrelated system logs.
