# Experiment 0048: post-window wrapper cost

- Date: 2026-08-27
- Outcome: **succeeded; measured cost is too small to justify runtime trampoline risk**
- Rollback: **not applicable; no game bundle or runtime profile changed**

## Question

Does the cached lifecycle-wrapper layer that remains after the ordinal-180
startup gate consume enough CPU time to justify a self-retiring dispatch or
trampoline in the successor to 0.1.3?

## Hypothesis

ESO retains the function pointers returned by `vkGetInstanceProcAddr` and
`vkGetDeviceProcAddr`, so changing later proc queries cannot retire already
cached wrappers. A mutable direct-forward trampoline could remove the remaining
branch and atomic loads, but it is justified only if the measured tax is
material relative to a 16.67 ms frame budget.

## Target and change set

- Production reference: ESO 12.0.8/databuild `3288357`, official MoltenVK
  1.4.2, and the 0.1.3 release profile.
- Code change: extend the existing x86_64 lifecycle probe with matched direct
  and post-finished-gate calls for acquire/present, indexed draw, and descriptor
  update paths.
- Runtime behavior: unchanged. No marker, installed bridge, settings, cache,
  launcher, or game process was modified.

## Evidence

Five Rosetta runs reported stable integer nanoseconds per operation:

| Path | Direct | Post-window wrapper | Residual tax |
|---|---:|---:|---:|
| Acquire + present pair | 3-4 ns | 12-13 ns | 8-9 ns |
| Indexed draw | 1 ns | 6 ns | 5 ns |
| Descriptor update | 1 ns | 4 ns | 3 ns |

The benchmark proves that the test used cached wrapper pointers and the
finished startup gate. The existing functional lifecycle, startup color,
compositor, input, and forwarding checks continued to pass.

## Result

The wrapper tax is real but not material enough to be the first performance
candidate. Even several thousand hot calls per frame imply only tens of
microseconds, a small fraction of the 16.67 ms budget at 60 FPS.

## Interpretation

Confirmed: later proc-query behavior cannot replace pointers ESO already
cached, and the current direct-forward C wrappers add single-digit nanoseconds
per hot call in this probe.

Inference: a mutable assembly trampoline would reduce some of that tax, but
its implementation and cross-thread correctness burden are disproportionate to
the measured upper-bound opportunity.

## Follow-up

Retain the benchmark as a regression guard. Defer self-retiring dispatch until
whole-frame CPU evidence shows wrapper dispatch is material on another target.
Proceed to an actual MoltenVK descriptor-path candidate in Experiment 0049.
