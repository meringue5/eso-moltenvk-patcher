# ADR-001: installer-derived update profiles

- Status: implemented as development candidate; installation/gameplay acceptance pending
- Date: 2026-10-24
- Decision owner: project maintainer; direction authorized in the project chat
- Evidence: [Experiment 0058](experiments/0058-eso-12.1.6-update-profile-analysis.md)

## Implementation amendment (2026-10-24)

[Experiment 0060](experiments/0060-dynamic-update-profile-implementation.md)
owns the implementation and validation evidence. The shared native resolver,
canonical profile verification, runtime re-discovery and installer recovery
binding are implemented. The following design sections explain the contract;
the remaining gates are current-original observation, installation and gameplay
acceptance, followed separately by any public release decision.

Schema 1 uses derive-and-compare rather than parsing profile-supplied addresses.
It bounds the entire file at 4096 bytes and requires canonical fields/order,
owner-only access and a matching marker/journal digest. Profiles bind the
compiled rule-set ID/bridge ABI, executable hash/UUID/client identity,
archive/runtime/original hashes, recovered slots and patch plans. The bridge
payload hash is recorded in the package checksums and recovery journal rather
than inside the canonical profile. Runtime checks the pinned runtime and
renamed-original hashes before loading the replacement.

The native full-object reference boundary is bootstrapped from the audited
12.1.6 executable, and is stricter than the source auditor's Vulkan-only
reference comparison. Unsupported instruction encodings, multi-start chained
pages, ambiguous locations, and other unproved layouts stop for analysis.
Executable discovery remains separate from original Bink provenance.

## Context

An ESO update can leave the bridge installed while replacing the executable.
The runtime correctly refuses its obsolete executable attestation. Reinstall
should preserve the new game files and derive compatible patch locations,
rather than require a new bridge build for each relocation.

The source auditor already recovers the embedded object's placement, proc-query
slots, and inactive pacing branch. The packaged native auditor and runtime use
compiled locations. Their present contract cannot accept all the relocations
the source auditor can prove. The native auditor also does not independently
establish the production pacing branch's relocated structure.

Executable compatibility and current original-loader provenance are separate
requirements. A retained bridge cannot prove what original Bink the vendor
currently supplies. The source auditor's candidate copies the reference Bink
hashes; those fields are not observations of a new vendor original.

## Decision

Keep Install as an explicit user action. It will analyze the current client,
derive a bounded profile under package-owned compatibility rules, and install
only after both executable and loader gates pass. No background watcher,
automatic app launch, cache reset, settings change, or executable disk patch is
part of this design.

Use one native resolver/validator shared by the installer auditor and bridge.
The Python source auditor remains a differential reference during development;
the released path must not require Python, a compiler, or an Xcode installation.
The runtime profile is data, never executable code or shell-sourced input.

### Compatibility rules versus per-installation data

The package owns immutable compatibility rules: accepted embedded-archive
member hashes, normalized code fingerprints and narrowly defined relocation
rules, the fixed Vulkan hook IDs, external-reference semantics, proc-query
names/counts, pacing control-flow rules, runtime configuration, and supported
loader generations. An installed profile cannot weaken or replace these rules.

Install produces a versioned profile with these logical fields:

| Field | Contract |
|---|---|
| schema, rule-set ID, bridge ABI | Exact known versions; reject unknown values |
| executable identity | SHA-256, Mach-O UUID, architecture, version/databuild |
| embedded runtime identity | Both archive-member hashes and unique linked placement |
| Vulkan targets | Exactly 17 unique fixed hook IDs, image-relative locations, 12 original bytes |
| proc routes | Independently recovered GIPA/GDPA slots and verified query shape |
| pacing target | Independently recovered branch, original bytes, active-flag location and control-flow proof |
| loader identity | Current observed original hash, supported loader rule, renamed-copy hash |
| payload identity | Compiled bridge ABI/rule set, pinned runtime hash and supported production mode; bridge hash in package/journal |

The fixed hook-ID table chooses destinations inside the bridge. Profiles may
not supply arbitrary symbol names, paths, function pointers, absolute process
addresses, runtime toggles, or additional patch sites. Vulkan placement and
pacing locations must never share an assumed relocation delta.

Use deterministic serialization and bind the complete profile digest into the
enable marker and recovery journal. Reject unknown schemas, duplicate fields
or IDs, missing fields, malformed hashes, oversized files, integer overflow,
out-of-range offsets, overlap, wrong segment permissions, and symlink input.
Apply a documented file-size bound and owner-only permissions. A digest detects
inconsistency; it is not an authenticity boundary against an attacker with the
same user's write access. Package signing remains a separate release concern.

### Executable resolver

1. Read a stable snapshot of the executable, embedded archive, build stamp,
   runtime, and observed original Bink. Record identities before analysis.
2. Verify both archive members. Find exactly one embedded-code placement by
   short anchors followed by full normalized-text verification. Package only
   small signatures, hashes, and relocation metadata; never distribute object
   files or reconstructed proprietary text. Accept only enumerated linker
   transformations already supported by the reference audit.
3. Recover all 17 targets and validate the complete external-reference boundary
   by symbol/kind/count, including classic and chained fixups. Resolve ambiguous
   byte-scan coincidences using instruction boundaries; if that cannot be
   proved, reject rather than silently dropping a reference.
4. Recover unique GIPA/GDPA slots with exact route/name multiplicities and no
   unnamed sites. Locate the pacing branch separately; prove the active-byte
   test, usleep destination, loop rejoin, and valid data location.
5. Recheck every input identity after all analysis, including pacing analysis,
   and again immediately before the transaction's mutation boundary.

The native helper ports the discovery and validation semantics described above.
In particular, changing just the former native
auditor's address comparisons or only externalizing the patch table is
insufficient. The normalization format and instruction-boundary decoder are
covered by differential fixtures; unrepresented layouts remain fail-closed.

### Runtime enforcement

Read and validate a single immutable profile snapshot, its marker binding, and
the package-owned rule-set identity. Recheck the actual executable hash/UUID,
payloads, mapped segment bounds, and loaded original identity. Re-derive the
locations with the shared validator and compare them with the profile; matching
12-byte signatures alone does not authorize a caller-supplied location.

Resolve every Vulkan destination and prepare the pacing hook before changing
any page permission. Apply the existing all-sites validation and RX restoration
rules to both target domains. Reject an absent or invalid new profile before
redirection. Preserve exact-build legacy-marker handling as a separate,
explicit compatibility path; never let a malformed new profile fall back to it.
Measure validation cost before release; do not trade structural checks for
startup speed without a separately reviewed design.

### Original Bink policy

For the first implementation, retain an explicit package-owned allowlist of
verified original Bink generations. A changed hash requires loader analysis
even if all imported symbol names exist. Export presence does not prove ABI or
behavioral equivalence. A generic arbitrary-Bink adoption mechanism is deferred.

| Observed state | Action |
|---|---|
| Updated executable, active bridge | Require user-run launcher Repair; do not restore an old backup |
| Vendor-restored supported original, executable compatible | Preserve current original as a new recovery generation and install |
| Vendor-restored unsupported original | Leave it untouched; report loader analysis required |
| Exact same installed generation | Permit existing verified in-place patcher upgrade rules |
| Updated executable fails structural validation | Leave all files untouched; report manual analysis required |

Keep the prebuilt proxy's re-export contract explicit. It is linked against a
renamed Bink at build time; test every required import through the staged proxy
and supported original before broadening the allowlist. Verify architecture,
dependencies, install-name treatment, and any dynamic symbol lookups as well as
static imports. Do not promise loader-generation independence from a data-only
profile. Any replacement for install-name tooling must be bundled and tested
without creating a new end-user developer-tool requirement.

### Transaction and recovery

Stage the profile, renamed copy of the newly observed original, bridge, and
runtime only after the shared bundle-idle and restore gates pass. Archive prior
recovery generations without deleting them. Record the raw original hash before
retagging a copy; preserve the raw original as the restore source.

Journal the executable/original pair, profile digest, payload hashes, and phases.
Publish the enable marker last, after final input checks and installed-file
verification. On interruption, resume or restore only when current files match
the journaled same generation. If a vendor update intervened, stop instead of
overwriting it during rollback. Conflicting archives are an error, not a reason
to replace history.

Uninstall restores only a verified same-generation raw original. A vendor-
restored active original stays byte-identical. Remove companions only with
proven patch ownership. Preserve settings and all caches. Status reports
executable-audit, loader-provenance, recovery, and runtime-experience results
separately; static compatibility must not be displayed as gameplay acceptance.

## Options considered

| Option | Benefit | Cost / limitation |
|---|---|---|
| Rebuild a package for every executable | Smallest runtime change | Repeats manual maintenance for pure relocation |
| Accept new hashes with signature scanning only | Simple reinstall path | Cannot prove call boundary, pacing semantics, or correct target selection; rejected |
| Installer-derived profiles with shared validation | Reuses one package for proved-compatible relocations | Native analysis and parser work; selected |
| Audit and install automatically during launch | Fewer explicit steps | Cannot recover a vendor original, competes with updater, adds mutation to startup; deferred |

## Consequences and implementation gates

### Platform capability amendment (2026-10-24)

[Experiment 0059](experiments/0059-macos-27-rosetta-runtime-preflight.md) and the
[primary-source review](research/macos-27-rosetta-moltenvk-review.md) add an
independent platform gate. Before deriving a profile or mutating files, record
OS version/build and host/payload architectures; verify actual non-game x86_64
execution instead of inferring Rosetta from the OS version or a receipt. Source
maintenance also requires a fresh self-patch probe. Permission-denied
diagnostics are indeterminate, not missing-translation proof.

Run the standalone native auditor as arm64 on Apple Silicon. Retain x86_64 for
the bridge and runtime slice in the observed x86_64 ESO process. Keep platform
validation provenance in the journal/status alongside executable compatibility;
an OS upgrade triggers capability reassessment even if ESO's hash is equal.
macOS 28 and its limited game-support path remain unvalidated. An external
profile does not remove the game's Rosetta dependency.

If translation is unavailable, stop before mutation and explain Apple's manual
installation route where supported. Install/Status must not silently install
Rosetta, accept licenses, enable beta legacy-game mode, change architecture
preferences, or reboot. Preserve official MoltenVK 1.4.2 as this implementation's
runtime; a later release requires separate ABI/header, configuration,
cache-preservation, and functional validation.

### Implementation gates

This design reduces maintenance for compatible executable changes. It does not
establish compatibility for changed engine behavior, fix launcher activation,
or eliminate Repair when the current original is unavailable.

1. Implement the bounded profile codec and shared resolver in disposable
   fixtures first. Differentially compare against the source auditor, including
   the independent relocations observed in Experiment 0058.
2. Integrate profile-derived targets into a freshly built bridge. Test malformed
   profiles, wrong hash/UUID/ABI, false-positive sites, ambiguity, overflow,
   cross-segment offsets, missing routes, changed pacing, and all error paths.
3. Integrate journaled Install/Uninstall/Status. Cover original-generation
   preservation, retained-bridge refusal, compatible reinstall, repeated
   reinstall, interrupted phases, and vendor changes during audit/rollback.
4. Run full source checks, fresh isolated-log build, Bink re-export and Rosetta
   self-patch probes, native/Python differential tests, transaction fixtures,
   archive hygiene, and cache/settings/log preservation checks.
5. Only then acquire the current vendor original through user-run Repair if
   still necessary, repeat exact-input checks, and install under standing
   authorization when all gates pass. Request one bounded normal-path user
   launch with the question, stop condition, evidence, and acceptance criteria
   defined in its own experiment. No launch request is needed for this design.
