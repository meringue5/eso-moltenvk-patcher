# Experiment 0005: exact HDR surface-format filter startup

- Date planned: 2026-07-19
- Outcome: **planned; non-game source preflight complete**
- Installation: **not approved or performed**
- Baseline: **original loader active; Experiment 0004 evidence preserved**

## Question

Does removing only the surface-format pair that enables ESO's HDR flag prevent
the confirmed NULL `vkSetHdrMetadataEXT` call and allow Steam-authenticated
startup to remain stable at character selection for 60 seconds?

## Hypothesis

Experiment 0004 proved that hiding `VK_EXT_hdr_metadata` is insufficient.
Static disassembly and Rosetta crash state instead identify this exact pair as
the branch input that leads to the unchecked setter call:

```text
VK_FORMAT_A2B10G10R10_UNORM_PACK32 / VK_COLOR_SPACE_HDR10_ST2084_EXT
```

Embedded MoltenVK 1.0.18 does not expose that pair on the test M4. MoltenVK
1.4.1 does. If this is the only remaining trigger for the repeated crash, a
wrapper that removes exactly that pair will produce all of the following:

- raw 1.4.1 surface formats include the pair and ESO's visible list does not;
- ESO selects its non-HDR surface path;
- ESO does not query `vkSetHdrMetadataEXT` through GDPA;
- character selection remains stable for 60 seconds.

A repeated setter query, the same NULL call, another startup fault, or a
surface enumeration that removes anything other than exactly one matching pair
falsifies or weakens the hypothesis. A setter no-op is not part of this run.

## Target and change set

- ESO SHA-256:
  `dcca9fa9012edf7674e048ec3d5123d5e2b4ed6fa2c4e23f04c7ca33f56b4bd3`
- ESO Mach-O UUID: `867e93bc-a6e7-3109-bf8e-542ff59ccdff`
- Runtime: official pinned MoltenVK 1.4.1 release
- Redirect set: the same 17 byte-validated wrappers as Experiments 0001-0004
- Marker mode: `live-check`
- Retained compatibility behavior: hide only `VK_EXT_hdr_metadata`; trace the
  exact device-extension list and GIPA/GDPA results
- New behavior: route GIPA's `vkGetPhysicalDeviceSurfaceFormatsKHR` result
  through a wrapper that removes only format `64` plus color space
  `1000104008`, preserving every other entry and its order
- Source identity: the clean commit containing this plan and implementation;
  its exact hash is recorded in the prepared ignored evidence

This is a new experiment because the runtime-visible surface-format list is a
material behavioral change from Experiment 0004.

## Evidence supporting the plan

Confirmed post-mortem facts:

- Experiments 0002 and 0004 have the same `RIP=0`, outer ESO frames, and
  ASLR-adjusted Rosetta `tmp1` return address immediately after the indirect
  HDR setter call at ESO image offset `0x364c5c0`.
- ESO checks its internal byte at object offset `0x1e` before the setter path.
  During surface selection, it sets the source byte at offset `0x1d` for
  format `64` plus color space `1000104008`, then copies that state into
  offset `0x1e`.
- A non-game AppKit/Metal probe on the same M4 reports:

  | Runtime | Surface formats | Exact ESO HDR pair |
  |---|---:|---:|
  | Embedded MoltenVK 1.0.18 | 3 | no |
  | MoltenVK 1.4.1 raw | 60 | yes |
  | MoltenVK 1.4.1 filtered | 59 | no |

- The real 1.4.1 filter probe removed exactly one pair in both count and data
  calls and reported `surface filter validation: PASS`.
- The shared fake-runtime smoke test covers count-only, exact capacity, short
  capacity with `VK_INCOMPLETE`, exact-pair matching, and preservation of an
  HDR color-space entry with a different pixel format.
- Ten startup-log checker tests pass and now require surface-filter routing,
  count/data removal evidence, device-extension evidence, and absence of an HDR
  setter query.

These are non-game source and runtime checks. They do not establish ESO startup
success.

## Completed pre-install source validation

The failed Experiment 0004 checkpoint was preserved until its active proxy
became a hard prerequisite for rebuilding. After the user stopped Steam and the
launcher, all 14 evidence-file checksums were reverified and
`scripts/restore.sh` restored the original loader and old pipeline cache. The
Experiment 0004 evidence and displaced marker remain preserved. Post-restore
status showed the exact target fingerprint, original/inactive loader, absent
marker, and byte-identical active/pristine Bink files.

The official clean build fetched and verified MoltenVK 1.4.1, passed the Bink
symbol re-export lookup, changed the Rosetta self-patch probe from `1` to `2`,
and passed the fake extension/surface-filter and device-tracing smoke tests.

Fresh real AppKit/Metal probes against the rebuilt artifacts reported:

| Probe | Raw count | Visible count | Exact pair raw/visible | Result |
|---|---:|---:|---:|---|
| Embedded MoltenVK 1.0.18 | 3 | 3 | no / no | baseline confirmed |
| MoltenVK 1.4.1 raw | 60 | 60 | yes / yes | pair at index 53 |
| MoltenVK 1.4.1 filtered | 60 | 59 | yes / no | `PASS`, removed 1 |

The rebuilt 1.4.1 device probe also passed with raw HDR extension present,
visible HDR extension absent, device creation successful without HDR, non-null
GIPA, and NULL GDPA for the setter. The rebuilt legacy probe confirmed that the
old runtime advertises neither the HDR extension nor the setter.

Fresh static analysis of the restored embedded object and fingerprinted ESO
recovered 162 old Vulkan text symbols, 40 external references, exactly 17
redirected entry points, and no missing new-runtime export. Proc analysis found
19 GIPA sites with 17 unique names and 80 GDPA sites with 65 unique names;
unknown sites and route regressions were both zero. The surface-format query and
HDR setter sites were recovered at ESO offsets `0x364c103` and `0x364c59d`.

Ten startup-checker tests, Python byte-compilation, shell syntax checks, and
`git diff --check` all pass. The clean source commit and a fresh ignored
Experiment 0005 evidence directory form the exact pre-install checkpoint. None
of these steps authorizes installation; explicit approval for this exact bundle
modification remains mandatory.

## Exact user action after a separately approved install

The user owns all interactive control:

1. Launch ESO through the normal Steam and launcher path.
2. If character selection appears, remain there for 60 seconds.
3. Exit through the normal UI and report whether character selection was
   reached and remained stable.

Do not enter the world, change settings, capture Metal HUD data, or perform a
performance test. Stop immediately at a crash, hang, corruption, or completion
of the 60-second wait. Agents must not launch ESO, Steam, or the launcher.

## Automatic verdict

Evidence collection will pass only if the selected run records all of these:

- MoltenVK 1.4.1 loaded and all 17 redirects activated;
- raw/visible device-extension enumeration removed exactly one HDR extension;
- surface count and data calls each removed exactly one matching HDR pair;
- device creation succeeded without enabling `VK_EXT_hdr_metadata`;
- no `vkSetHdrMetadataEXT` GDPA query and no bridge error/fatal record;
- character selection remained stable for the full 60 seconds.

Any startup crash or HDR setter query fails the run. Missing or corrupt
run-scoped evidence is inconclusive.

## Rollback policy

Do not assume rollback merely to maintain a normal operating state. Preserve
the installed result as a checkpoint unless restoration is specifically needed
for evidence, comparison, rebuilding, or the next controlled change. Never
delete the pristine backup, either pipeline cache, crash evidence, or logs.

## Follow-up

If this startup run passes, design a separate bounded stability experiment.
If it fails, preserve the new checkpoint and return to the exact last query,
crash return address, and surface/device state before considering a guarded
setter implementation. No result here authorizes gameplay or performance
claims.
