# Project naming and compatibility

## Canonical name

The public product name is **ESO MoltenVK Patcher**. The GitHub repository slug
is `eso-moltenvk-patcher`; the local workspace directory will use the same slug
after active sessions have moved away from the legacy path.
Release archives use `ESO-MoltenVK-Patcher-<version>.zip`, and the packaged
command-line implementation uses `eso-moltenvk-patcher`.

Use the product name in current README copy, release notes, screenshots,
support instructions, and new user-facing messages. Use kebab-case for URLs,
repository names, executables, and archive names. Use snake_case only where a
language or packaging system requires it.

## Legacy `teso4m4` identifiers

The repository was originally named `teso4m4`. That name is now a legacy
compatibility identifier rather than the public product name. It remains in:

- installed marker, backup, MoltenVK, cache, and log filenames;
- source-maintenance environment variables such as `TESO4M4_LOG_LEVEL`;
- C symbols, types, macros, and include guards;
- historical experiment records and existing asset filenames; and
- Git history and older release references.

These identifiers must not be mechanically replaced. Existing installations
must remain detectable, removable, repairable, and recoverable after the
rename. Historical evidence must continue to describe the name and paths that
actually existed at the time.

## Migration policy

New public surfaces use **ESO MoltenVK Patcher** immediately. Internal names
may migrate later only when the change provides read compatibility for the old
state, writes the new state deliberately, tests install/remove/reinstall across
the boundary, and documents the retirement window. Environment variables need
a deprecated legacy alias before any new prefix becomes exclusive.

Renaming the GitHub repository should preserve GitHub's old-URL redirect, but
the repository's own links and local `origin` URL must still be changed to the
canonical slug. Renaming the local workspace directory is the final step and
must wait until no other Codex or terminal session depends on the old absolute
path.
