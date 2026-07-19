# Contributing

Bug reports should include the Mac model, macOS version, ESO executable SHA-256,
MoltenVK version, exact test duration, and a sanitized log. Never upload account
names, email addresses, access tokens, complete game binaries, Bink libraries,
pipeline caches, or unredacted crash reports.

Runtime changes must retain three properties:

1. Refuse unknown ESO executable builds.
2. Preserve a byte-for-byte pristine loader backup.
3. Provide a tested restore path before installation is allowed.

Performance claims need an A/B test in the same location and camera angle, with
FPS, GPU frame time, app memory, Metal memory, and thermal state recorded.

Before a controlled runtime test, assign an experiment ID and copy
[`docs/experiments/TEMPLATE.md`](docs/experiments/TEMPLATE.md). Follow the
[`docs/README.md`](docs/README.md) ownership rules so current state, durable
findings, and per-run evidence remain distinct.
