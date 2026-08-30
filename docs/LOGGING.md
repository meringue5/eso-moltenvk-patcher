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

The default is `info`. It deliberately excludes raw pointer values,
complete GIPA/GDPA lookup traces, and every individual suppressed startup draw.
The begin record, ordinal-150 latch with total suppression count, ordinal-180
finish, mode, pacing, configuration, and activation records remain. These records are
valuable only when a developer is diagnosing a bounded issue and add no value
to routine player support.

## Operational control and incident capture

The next public package adds `Logging.command`. It changes a small, per-ESO
installation preference in Application Support rather than reinstalling,
removing, or changing the bridge. The selected level applies on the **next**
normal Steam/ZeniMax-path ESO launch; it cannot change a process that is
already running.

`Logging.command` verifies the exact installed bridge and recovery record,
then offers these player-facing choices:

| Choice | Runtime level | Operating rule |
|---|---|---|
| Operational logging | `info` | Normal default. Leave this selected after an incident. |
| Diagnostic logging | `debug` | Temporarily select before one affected natural launch; return to `info` after collecting that launch. |

The command accepts the same choices for support automation as
`Logging.command --level info` or `Logging.command --level debug`. It records
only `level=…` in the installation state directory. It does not touch the ESO
bundle, Bink backup, pipeline cache, `UserSettings.txt`, credentials, or the
current process; it therefore does not require an install/remove cycle.

The bridge reads this preference only when no `TESO4M4_LOG_LEVEL` environment
override is set. An invalid, symlinked, non-owner, group-writable, or malformed
preference is ignored and safely falls back to `info`. Source maintenance may
still set `TESO4M4_LOG_LEVEL` to `error`, `warn`, `info`, `debug`, or `trace`;
the environment override takes precedence. `trace` intentionally remains
source-only because it can include pointer-bearing proc lookup and per-frame or
per-draw detail that is inappropriate for routine support and can distort the
timing being investigated.

## Maintenance plan

1. Run normal production operation at `info`; Status identifies the configured
   level without claiming it measures focus or FPS.
2. When a specific issue recurs, preserve the current run first. Set `debug`,
   reproduce only through the user's ordinary launch path, and retain the run
   ID plus the user-observed focus/FPS result.
3. Export Diagnostics, which remains filtered and never exports raw trace
   content. Return the preference to `info` after the bounded capture.
4. Any new debug field must have an owner, a named question, a bounded event or
   count limit, a documented privacy classification, and a policy-probe test.
   Promote it to `info` only when it is both operationally necessary and shown
   not to add hot-path work; otherwise remove it after the incident.

## Retention and support

The production log rotates at 1 MiB, retains one `bridge.log.1`
generation, and opens both with owner-only permissions. Rotation or logging
failure never weakens the runtime's fail-closed patch validation.

The public `Diagnostics.command` is the user-controlled **Export Support
Report** action. It exports checksums, selected client and installer-state
summaries, and only allowlisted events from the latest bridge run. It excludes
ESO credentials, full user settings, caches, proprietary game files, home
paths, raw pointer-bearing traces, and unrelated system logs.
