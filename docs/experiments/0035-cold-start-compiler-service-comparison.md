# Experiment 0035: cold-start compiler-service comparison

- Date: 2026-08-16
- Outcome: **succeeded as a diagnostic comparison; root cause not yet proven**
- Rollback: **not required; production bridge remains installed**

## Question

What distinguishes a patched start that shows the pink startup surface and
approximately 10 FPS from an immediate patched restart that runs smoothly?

## Hypothesis

If the bad start is caused by total bridge nonactivation, its production log
should lack the 17-entry-point active state or the cache should retain the
embedded-runtime identity. If both starts activate the same bridge, a startup
pipeline or Metal compiler transition may instead distinguish them.

## Target and change set

- ESO macOS client 12.0.8, databuild `3288357`.
- Executable SHA-256:
  `a819aa2313e91676bdfa3987ae650d594a86faf2429ad56c736b5e6992680609`.
- Replacement runtime: official MoltenVK 1.4.2 from release 0.1.1.
- Production configuration and bounded startup compositor neutralizer were
  unchanged. No source, settings, or cache mutation was introduced for this
  comparison.

## Preflight

This was a retrospective comparison of two ordinary user-controlled starts,
not a prearranged launch experiment. After the runs, `scripts/check-update.sh`
reported `CURRENT`, and `scripts/status.sh` verified the exact target, current
bridge marker, official replacement runtime, and active 1.4.2 cache identity.
The verified original-loader backup remained available.

## Procedure

1. The user started ESO through the normal authenticated launcher path.
2. The first process showed the pink startup surface and low FPS. The user
   exited it after approximately 42 seconds.
3. Four seconds later, without restarting the launcher, the user started ESO
   again. The second process ran smoothly and continued into ordinary play.
4. The agent did not launch or stop ESO, Steam, or the launcher. Afterward it
   compared sanitized bridge events, process timing, unified-log compiler
   events, cache metadata, and crash-report presence.

## Evidence

The bad run was `20260816T112904.568575000Z-pid37123`; the smooth run was
`20260816T112950.671452000Z-pid37131`. The bad process exited at approximately
`20:29:46` KST, and the smooth process began at approximately `20:29:50` KST.
The launcher was not restarted between them.

Both production bridge records contain the same high-level state:

```text
runtime:            official MoltenVK 1.4.2
redirects:          17 Vulkan entry points active
ESO SHA-256:        a819aa2313e91676bdfa3987ae650d594a86faf2429ad56c736b5e6992680609
suppressed draws:   79
forward latch:      generation 2, ordinal 150, present-deadline
```

The first 46 seconds of the exact processes differ in the macOS unified log:

| Observation | Bad start | Smooth restart |
|---|---:|---:|
| ESO-side `MTLCompilerService` connection records | 0 | 10 |
| Metal compilation begin records | 0 | 6 |
| Metal compilation success records | 0 | 6 |
| Metal compilation failure records | 0 | 0 |
| Library builds | 0 | 2 |
| Function/pipeline builds | 0 | 4 |

The six successful jobs on the smooth start accumulated approximately 230 ms,
with a maximum individual interval of approximately 98 ms. They began about
27 seconds after process start. The bad process lived approximately 42 seconds,
so it covered the analogous elapsed-time window without recording a compiler
service connection. No new crash report followed either process.

`ShaderCache.cooked` remained unchanged. The active MoltenVK pipeline cache was
preserved after the earlier patched low-FPS checkpoint as SHA-256
`4a8c0e9372cb8fc91b10c823a230f4b35352423770a009fe8259c7ecfda3ac82` and after
the later smooth session as SHA-256
`7ad3264d75369be8ef6902390164103809fcf1ed9ad9733cb91d09dcc47c2542`.
Both were 8,344,388 bytes with the 1.4.2 identity. The files differ across
5,453,600 byte positions, but no intermediate snapshot was taken between the
two back-to-back starts; the delta includes the later long smooth session.

The user observations of pink output, approximate FPS, and smoothness were not
captured as FPS/GPU-time telemetry.

## Result

The immediate ESO-only restart recovered normal performance without a launcher
restart. Both processes demonstrably loaded MoltenVK 1.4.2, activated all 17
redirects, and reached the same compositor-neutralizer latch. Total bridge
nonactivation is therefore excluded for this bad start.

The strongest observed discriminator is that the bad process never recorded a
normal ESO-side connection to `MTLCompilerService`, while the smooth process
connected and completed six compilation jobs without failure.

## Interpretation

**Confirmed:** the pink/low-FPS state can occur while the replacement runtime
and all production redirects are active. Identical neutralizer counters also
show that those counters do not guarantee the user never sees pink outside the
bounded replacement interval or through a presentation path they do not cover.

**Confirmed:** restarting ESO alone was sufficient in this pair. A launcher
restart is not required for every recovery.

**Inference:** the evidence argues against the simple explanation that normal
shader-compilation load itself causes the low FPS. Compilation-service work was
present on the smooth start and absent on the bad start.

**Leading hypothesis:** the bad process sometimes fails to enter or complete
the normal Metal shader/pipeline initialization path, leaving a fallback,
placeholder, or otherwise incomplete presentation state. Restarting the ESO
process reinitializes that path.

The unified log cannot prove that no in-process compilation occurred, and the
cache delta cannot establish cache causality. The exact missing trigger and
whether compiler-service absence is cause or consequence remain unresolved.

## Rollback

No mutation was made and no rollback was required. The exact 0.1.1 bridge
remains installed. Settings and every captured cache generation remain
preserved.

## Follow-up

Add low-overhead timing around the first graphics-pipeline creation wave and
correlate it with `MTLCompilerService` connection timing. A future diagnostic
should classify a start before gameplay as bridge inactive, bridge active but
compiler path absent, or bridge active with normal compiler engagement. Do not
delete caches, distribute a warmed cache, or add speculative precompilation
until that relationship is proven.
