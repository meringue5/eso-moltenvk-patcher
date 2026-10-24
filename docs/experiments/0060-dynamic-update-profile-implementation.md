# Experiment 0060: native dynamic update profiles

- Date: 2026-10-24
- Outcome: **running; implementation/non-game and reinstall gates pass, gameplay acceptance pending**
- Rollback: **verified current-original backup available; candidate installed, settings/caches preserved**

## Question and hypothesis

Can one bridge and installer discover compatible relocated Vulkan and host
pacing sites, bind their profile to the current executable, and preserve the
vendor update and original-loader recovery generation?

Hypothesis: installer-time discovery and runtime re-discovery under identical
compiled rules permit independent relocations without recompiling the bridge.
Changed embedded code, routing, pacing semantics, or an unobserved original
generation must still stop installation.

## Target and controlled change

- Source starts at `6a7da24`; public 0.2.1 remains unchanged and unpublished
  development candidate 0.3.0-rc.1 is used for this work.
- ESO 12.1.6 / databuild 3309674, executable SHA-256
  `3bdcfda575da791d6a337ef34afe3390d0a373aacdcaad033b2b454ddce804e6`, UUID
  `b21e7036-3add-3db0-bd78-bd2ea5fb791a`.
- Fresh source audit against 12.1.5 returned `FAST_REBASE_SAFE`. Its candidate
  was reviewed and selected as `targets-eso-2026-10-24.json`; update check is
  now `CURRENT`. This is executable-profile selection, not original-loader
  provenance or permission to restore an old backup.
- Official MoltenVK 1.4.2 and the production compositor/pacing mode remain
  unchanged. No new renderer configuration or cache policy is selected.

## Implementation

`src/update_profile.c` is shared by the native auditor and x86_64 bridge. It
reads bounded file snapshots, verifies the compiled archive/object identity,
finds exactly one normalized embedded-object placement, validates patch bytes,
recovers unique GIPA/GDPA slots, and independently proves the pacing branch,
usleep stub, loop rejoin, and writable active-flag location. It rechecks inputs
after all discovery, including the client version/databuild stamp.

The source auditor proves the 17 Vulkan targets / 40 Vulkan references retain
their prior structure. The new native rules additionally establish the full
old-object reference boundary on this exact 12.1.6 baseline: 677 target/kind
pairs, 989 references, including non-Vulkan targets and rebased pointers. This
stronger native boundary was not independently compared against the missing
12.1.5 executable. A later non-Vulkan-boundary difference therefore stops the
native installer even if the narrower Python audit would accept it.

Rules contain hashes, small patch signatures, offsets and semantic counts;
they do not distribute an ESO object or reconstructed text. The archive is
read from the selected installation. Whole-archive hashing is intentionally
stricter than member-only equivalence.

Profiles are canonical bounded text. Neither installer nor runtime parses
caller-supplied addresses into patch targets: each derives an in-memory plan
and compares the supplied file byte-for-byte with its canonical serialization.
The marker and atomic recovery journal bind its digest. Runtime validation
also checks loaded UUID, runtime/renamed-original hashes and supported OS
range; only internally selected hook names/destinations can be used.

Reference scan coincidences are excluded only after instruction-boundary proof
from LC_FUNCTION_STARTS. The bundled HDE64 byte decoder is pinned to MinHook
v1.3.4, with portable unaligned loads and hidden symbols. A closed set of
length-preserving SIMD VEX forms is handled explicitly; other unsupported
encodings and multi-start chained-fixup pages fail closed. No LLVM, Python,
compiler, or decoder download is needed when running the packaged auditor.
License/provenance are preserved in source and binary-package notices.

The release transaction uses the shared bundle-idle gate and checks executable
and active-loader identity before replacement and marker publication, including
after auditor calls. A Bink-only vendor change must survive even if ESO's hash
does not change. It writes the profile before
publishing the marker, archives prior profiles, atomically records recovery
state/version, resumes only a matching interrupted generation, and preserves
modified/unowned profiles on removal. Re-running Install with a retained stale
bridge still requires launcher Repair. Source maintenance writes the same
profile; source Restore now refuses a retained bridge without a matching
current-executable attestation.

## Non-game validation

- Fresh source build: BinkOpen re-export, Rosetta 1-to-2 self-patch, inactive
  pacing, operational/log isolation, and all existing configuration/resource
  probes pass. Production-log identity is checked by the build script.
- Python 3.14.8 full suite: 154 tests pass.
- Seven native tests use synthetic Mach-O executables and AddressSanitizer /
  UndefinedBehaviorSanitizer. Independent host/object relocation, classic and
  single-start chained pointers, canonical executable binding, malformed
  metadata, changed queries/pacing, profile corruption/permissions/symlinks,
  archive changes, and LF/CRLF build stamps pass their acceptance/refusal gates.
- The release transaction fixture passes existing recovery/settings/privacy
  checks plus dynamic profile publication and repeated install, corruption
  preservation, compatible reinstall after a simulated vendor restore,
  interrupted publication, executable-only and Bink-only updater races before
  mutation/after payload staging,
  indeterminate process checks and unvalidated-OS refusal.

The synthetic transaction fixture uses an audit double; it does not prove
native semantic compatibility. The native/Python differential fixtures and
real-target native profile checks supply that separate evidence.

An intermediate real-target check found that the first native client-stamp
reader rejected the vendor's CRLF file while synthetic LF fixtures passed.
No installed file was changed. The reader now accepts both line endings, a
regression test covers the distinction, and build.sh must derive and verify a
profile on the real target before reporting success. This supersedes the
intermediate fixture-only confidence; only the final package/check results
below are delivery evidence.

## Preservation and remaining gate

Before installation, 34 identities (including profile absence) were inventoried
across the executable, loader/companions, recovery files, settings, caches,
production log and selected target. Compared with Experiment 0058, only the
source target pointer deliberately changed. All probes and fixture mutations
remain in temporary/build storage; the installed original is not restored.

The active Bink still re-exports the earlier renamed original. Neither that
file nor its raw recovery backup proves the vendor's current original. The
next external step is a bounded user-run launcher Repair, with ESO not started,
then closed applications and a fresh fingerprint/idle/restore assessment. Only
an observed supported original permits cache-preserving installation under the
standing authorization. Unknown loader hashes remain blocked.

The first real installer attempt completed executable/platform verification,
then stopped at the shared idle gate because the ZeniMax launcher was running.
It exited 1 before mutation. The prior bridge, pristine/renamed originals,
runtime and marker were separately copied to ignored preservation storage with
all five hashes verified before requesting user-run Repair. All 34 installation
inventory entries remained unchanged after this guarded attempt.

ARM64 and Rosetta x86_64 auditors derive byte-identical 1664-byte profiles on
the real client, digest
`ed4970761703564d57f677e01f3641b6275a1bc3f4756da6f80b14b76476686a`.
One measured invocation took 0.343 seconds natively and 0.796 seconds under
Rosetta; these are single diagnostic timings, not startup or FPS benchmarks.

## Delivery artifact and checks

The final local candidate is `ESO-MoltenVK-Patcher-0.3.0-rc.1.zip`,
3,474,852 bytes, SHA-256
`25339922a0467e59c1fb10c58b6cae5fc4529773da78c4848674d5c3a2f0eee6`.
Its bridge SHA-256 is
`4a204907c8236e8e87fcd790a9ae849e3a1d43f4704b927cfa79be8b7233247c`;
the universal native auditor is
`c6a37ef409ffd52b82736ec4443576a86968935390dc20528c1fd1f15fba3141`.
The last packaging run rebuilt from source, passed real-target native profile
derivation/verification, all build probes, the extended installer fixture, and
archive checksums/executability/LF/privacy/hygiene checks. No tag or public
release was created, and the local ZIP/proprietary preservation files remain
ignored by Git.

Python compilation, shell syntax and diff-whitespace checks pass; all 124 local
documentation links in the changed Markdown files resolve. Real installation
is still pending the user-controlled Repair/launcher-exit boundary. This record
does not classify the uninstalled candidate as runtime accepted.

Installation is not gameplay acceptance. If reinstall succeeds, a separately
bounded normal Steam-path user launch must verify new profile validation, all
17 redirects, pacing and 79/150/180 startup control, with focus/rendering/FPS
observations supplied by the user. No request for that launch precedes a
verified installed candidate.


## Post-Repair installation amendment (2026-10-24)

The user reported Repair complete and launcher closed. A fresh read-only check
confirmed the same 12.1.6 executable hash/UUID/databuild and `CURRENT` result.
The active original Bink was now directly observed as
`9a0872f484a970bfcaa0b7bf76906822143c1d30358ee17dcd30c4b4dc2e6de8`,
matching both the supported generation and verified raw recovery backup. This
satisfies the provenance gate that retained bridge/backup files alone could
not satisfy. Earlier pending observations above remain historical.

The verified packaged CLI completed `install --skip-settings --yes` with exit
0. Platform capability, current input identities, native derivation and shared
bundle-idle checks passed. Steam was idle and allowed. The installer recognized
the vendor-restored loader and preserved recovery state. No application was
started by the agent.

Post-install Status reports `READY`, release 0.3.0-rc.1, recovery verified and
dynamic profile verified (schema 1). The installed bridge, runtime and canonical
profile hashes match the delivery identities above. The 1664-byte profile has
digest `ed4970761703564d57f677e01f3641b6275a1bc3f4756da6f80b14b76476686a`;
both marker and installed journal bind it to the current executable. Journal
phase is `installed`. The renamed original has SHA-256
`7fac596c99afc36cbd5fc462aac74333d6ed76ecc9de591052201c5abee2eaa3`.

A fresh pre-install/post-install identity comparison confirms unchanged
executable, user settings, every inventoried active/backup pipeline cache,
production log (including inode/size/mtime), legacy pristine and raw recovery
backup. The seven changed inventory entries are active bridge, renamed
original, runtime, marker, new profile, journal and release version. Companion
file replacement with identical content is recorded as an inode change rather
than hidden as no change. Older recovery inventory entries remain intact.
Machine-local evidence is retained in the Experiment 0060 temporary directory:
`preservation-after-repair-before-install.json`, `preservation-after-install.json`,
`actual-install-after-repair.txt`, and `actual-status-after-install.txt`.

Rollback remains available through the candidate's verified Remove path using
the current raw original; the older legacy pristine must not be restored across
this update. No rollback was exercised on the real bundle because that would
unnecessarily replace the installed candidate; disposable transaction fixtures
cover removal. No public release or gameplay PASS is claimed.

One bounded user-controlled normal Steam-path run was requested after installed
verification: at most five minutes, no settings change or required app-switch
workaround, stop on crash/pink/focus failure/sustained low FPS. Runtime profile
validation, 17 redirects and existing 79/150/180 startup control must be checked
against the new run, alongside the user's rendering/focus/FPS observations.
The pre-install attestation-failure log remains historical evidence.
