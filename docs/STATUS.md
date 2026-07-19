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

Experiment 0002 was explicitly approved and installed at 2026-07-19 15:08 KST
from source commit `7a235dc` in `live-check` mode. The active Bink loader is now
the bridge and the enable marker is present. Before replacement, the absent
pristine backup was created from the active original loader and byte-verified;
the old pipeline cache backup and proxy re-export target were also verified.
Evidence collection is prepared, but ESO has not yet been launched. See the
[running experiment](experiments/0002-live-check-proc-trace-startup.md).

## Active blocker

The startup `EXC_BAD_ACCESS` with `RIP=0` must be explained before performance
testing. Exhaustive public-wrapper analysis and non-game proc probing reduce
the likelihood of a missing public Vulkan function or obvious old/new wrapper
mix, but do not rule out a callback, ESO abstraction vtable, private ABI,
lifetime, or surface/swapchain incompatibility. See the [Experiment 0001 crash
analysis](experiments/0001-crash-analysis.md) for the observed crash evidence.

The current bridge source traces every observed `vkGetInstanceProcAddr` and
`vkGetDeviceProcAddr` request, returned address, and null result. The default
experimental marker mode is `live-check`, which enables
`MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=1`; it is unproven and is not a fix.

## Next gate

The next gate is the user-controlled Steam launch. It must stop at a crash,
stable character selection, or 60 seconds, whichever comes first, and must not
enter the game world. Evidence must then be collected and the pristine loader
and old pipeline cache restored immediately.

Do not repeat Experiment 0001 unchanged. Experiment 0002 is evidence collection,
not a gameplay or performance test.
