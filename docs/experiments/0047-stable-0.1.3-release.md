# Experiment 0047: stable 0.1.3 public release

- Date: 2026-08-27
- Outcome: **succeeded; public release and server asset digest verified**
- Rollback: **v0.1.2 remains immutable and available as the prior release**

## Release question

Can the exact user-validated Experiment 0045 runtime and Experiment 0046
generation-aware recovery state machine be published as a new immutable 0.1.3
release with a stronger but evidence-bounded startup-stability claim?

## Selected artifact

```text
tag:             v0.1.3
release commit:  bc3a5b846276b039d1f4635ad3816ba676f3c33d
asset:           ESO-MoltenVK-Patcher-0.1.3.zip
size:            3414468 bytes
SHA-256:         26ca4273aae669231dcc3a04e998d59b74038361e97da0b5f746434c1d02a4d7
bridge SHA-256:  24735b44e83f1f6986cf2c36bca57616b8468fd026bc4ffa11062ed31a98f569
mode:            startup-compositor-neutralize-pacing-release
```

The installed bridge is byte-identical to the package payload. The exact
package candidate passed its user-controlled Steam-path launch with no pink and
normal FPS. The bounded log recorded 79 suppressions at generation-2 ordinals
71-149, one ordinal-150 forward latch, ordinal-180 completion, zero pipeline-
timing records, and zero error or overflow records.

## Release notes policy

The public notes call 0.1.3 the strongest startup-stability release validated
so far for the exact M4/ESO 12.0.8 target. That confidence is grounded in the
inactive 100-ms pacing bypass, bounded compositor repair, measurement-stripped
release run, and safer update/uninstall recovery. The notes explicitly avoid a
universal FPS or battery-life promise. The user's approximately one-minute
battery-mode mid-high observation is reported only as supporting confidence.

## Publication and verification

Commit `bc3a5b8` was pushed to `main` and annotated tag `v0.1.3` was published.
GitHub created the non-draft, non-prerelease release at:

`https://github.com/meringue5/eso-moltenvk-patcher/releases/tag/v0.1.3`

The latest-release endpoint selected `v0.1.3` with the single named ZIP asset.
GitHub's server-reported asset digest exactly matched the locally verified
SHA-256 above. The point-in-time asset `download_count` was 0 immediately after
publication.

No public download was performed for verification because doing so would
artificially increment that counter and make recipient accounting ambiguous.
The server digest, asset size, tag target, release body, latest endpoint, local
ZIP checksums, internal manifest, and archive hygiene were all verified instead.

## Rollback

No rollback was required. The installed 0.1.3 candidate retains its verified
Uninstall backup. The earlier `v0.1.2` tag and asset were not modified.
