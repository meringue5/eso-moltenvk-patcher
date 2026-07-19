# Project status

Last updated: 2026-07-19

## Safety state

The experimental MoltenVK bridge is not validated for gameplay. Experiment
0001 activated MoltenVK 1.4.1 and then crashed during early graphics startup;
the failure is documented in the [experiment record](experiments/0001-moltenvk-1.4.1-full-redirect.md).

The non-destructive `scripts/status.sh` check on 2026-07-19 reported:

- the analyzed ESO executable still matches the fingerprint in the
  [target manifest](../config/targets-eso-2026-07-11.json);
- the active Bink loader is original and the bridge is inactive;
- the enable marker is absent.

This is a point-in-time observation, not a persistent guarantee. Run the status
check again before any work involving the game bundle. Inactive companion files
may exist beside the game executable; the status result above does not inventory
or remove them.

## Active blocker

The startup `EXC_BAD_ACCESS` with `RIP=0` must be explained before performance
testing. The leading hypotheses are an incomplete function/callback table and
mixing handles between the old static runtime and the new runtime. See the
[Experiment 0001 crash analysis](experiments/0001-crash-analysis.md) for the
observed evidence and the limits of that evidence.

The current bridge source traces every `vkGetInstanceProcAddr` request, returned
address, and null result. The default experimental marker mode is `live-check`,
which enables `MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=1`; it is unproven and is not
a fix.

## Next gate

Before another controlled startup run:

1. Make Vulkan wrapper cross-reference analysis exhaustive beyond direct
   `E8`/`E9` calls, including address-taken functions, tables, and relocations.
2. Compare every function queried through `vkGetInstanceProcAddr` between the
   old 1.0.18 behavior and MoltenVK 1.4.1.
3. Establish that every path receiving new MoltenVK handles remains within the
   new runtime.
4. Prepare bridge-log, crash-report, and unified-log collection before seeking
   approval to install anything.

Do not repeat Experiment 0001 unchanged. Any next run is startup evidence
collection, not a gameplay or performance test, and still requires explicit
approval before the game bundle is modified.
