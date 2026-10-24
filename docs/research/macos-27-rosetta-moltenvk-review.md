# macOS 27, Rosetta, and MoltenVK maintenance review

- Review date: 2026-10-24
- Scope: Apple policy/release notes, official KhronosGroup releases and
  development changelog, and implications for installer-derived profiles
- Decision: retain official MoltenVK 1.4.2; add platform capability checks to
  [ADR-001](../UPDATE-COMPATIBILITY.md) before changing installed files
- Local evidence: [Experiment 0059](../experiments/0059-macos-27-rosetta-runtime-preflight.md)

## Apple policy and macOS 27 changes

[Apple Support](https://support.apple.com/en-us/102527), published 2026-09-21,
says general Rosetta availability includes macOS 27; macOS 28 retains limited
functionality for certain older games. macOS 27 does not itself end all
Intel-app support. The exception does not identify ESO or guarantee that its
launcher, modified loader, and runtime patch qualify.

The [macOS 27 release notes](https://developer.apple.com/documentation/macos-release-notes/macos-27-release-notes)
identify separate operational changes:

- Existing Rosetta installations are not automatically restored on upgrading
  to 27.0 (163213094).
- Universal apps previously forced through Rosetta open natively (168097174).
- Installer packages without a host-architecture declaration default to arm64
  (171187112).
- Intel loaders/plugins may lack a visible incompatibility notice (176042635).
- A beta-only legacy-game test mode changes system behavior and disables
  ordinary Rosetta (166398727). It is not a production recovery procedure.

The current ZIP/shell installer is not a PKG; future PKG packaging needs explicit
host-architecture review. Do not infer translation availability from OS version,
a receipt, or whether Settings lists the bridge.

[Apple's translation architecture documentation](https://developer.apple.com/documentation/apple-silicon/about-the-rosetta-translation-environment)
states that a process cannot mix arm64 and x86_64 modules. The observed ESO
client is x86_64, so its in-process bridge and MoltenVK slice must remain
x86_64. An arm64-only bridge cannot eliminate ESO's translation dependency.
Native arm64 is appropriate for the standalone auditor.

## Official MoltenVK state

The [official latest-release endpoint](https://github.com/KhronosGroup/MoltenVK/releases/latest)
and [release metadata](https://api.github.com/repos/KhronosGroup/MoltenVK/releases/latest)
still identify **v1.4.2**, non-draft/non-prerelease, published
`2026-07-24T14:00:46Z`. Current metadata for `MoltenVK-macos.tar` matches the
pinned archive: 59,563,008 bytes, SHA-256
`f95765a6229cb7b915990a2890ce12ebe36a730b021545d3d52ae69ce4c4024e`.
No new release archive was downloaded in this review.

The installed and retained vendor dylibs both match the pinned 1.4.2 hash
`aef00b13bcc808adf15b85bef9ae67393d92be7ed5dfe41cad16fa809e4a4c5f`.
Their universal architecture does not make x86_64 ESO native.

The reviewed development head was
`5189f8de86ec365860f63bf1da736815cfaea5cd`. Its
[versioned changelog](https://github.com/KhronosGroup/MoltenVK/blob/5189f8de86ec365860f63bf1da736815cfaea5cd/Docs/Whats_New.md)
labels **1.4.3 as unreleased**, with corrections for pipeline-cache shader
matching, image memory-type reporting, shader interfaces, and imported-resource
ownership, plus additional extensions. The cache correction merits a separate
applicability review. No local trace connects ESO to the imported-resource
conditions; an upstream leak correction does not prove a leak in ESO.

There is no demonstrated reason to combine an unreleased runtime with the
dynamic-location change. The bridge depends on the exact `MVKConfiguration`
interface and effective settings. A future upgrade needs matching headers,
official artifact/hash verification, export/configuration checks, isolated
non-game probes, cache preservation, and separate gameplay acceptance. It must
not silently replace the pinned runtime or enable Metal argument buffers.

## Effect on the approved design

Add a platform preflight before profile derivation and again before mutation:
record OS version/build, host/payload architectures, actual non-game x86_64
execution, and source-maintenance self-patch results. A receipt is supplementary
evidence. Distinguish unavailable, permission-denied, and unvalidated-platform
results; a blocked diagnostic does not prove missing Rosetta.

Separate native installer/auditor execution from the x86_64 game path. Offer
Apple's user-controlled Rosetta installation instructions when execution is
unavailable on a supported OS. Do not install Rosetta, accept its license,
enable legacy-game mode, change architecture preferences, or reboot as a side
effect of Install/Status. macOS 28 remains unvalidated for this project.

Current non-game success on one macOS 27.0.1 Mac supports continuing profile
implementation. It does not prove rendering, focus, FPS, or the complete normal
launch chain. The current logged skip is an executable-hash mismatch, not
evidence of a new MoltenVK or Rosetta failure.

## Search and evidence limits

Primary Apple docs and KhronosGroup release/API resources were checked live.
Apple documentation required its official Markdown representation for complete
release-note text; search snippets alone were not treated as authoritative.
Third-party game reports were not used to establish project support. Recent
official release entries contain no 1.4.3 release; development version labels
are not release eligibility.

No pre/post-27 performance comparison, macOS 28 test, alternate MoltenVK build,
or beta game-mode experiment was performed. No assertion about the cause of the
previously tracked activation issue follows from this review.
