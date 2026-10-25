# Experiment 0060: native dynamic update profiles

- Date: 2026-10-24 / 2026-10-25
- Outcome: **partial success; dynamic-profile runtime and reinstall pass, initial focus/FPS acceptance fails**
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


## User-launch result amendment (2026-10-25)

The user completed one normal Steam-path launch and exit, shorter than the
five-minute bound. On entering the world after character selection, initial
focus was lost and the displayed FPS was approximately 40. The user switched
away with Cmd-Tab and returned; focus then attached to the game and the FPS
returned to approximately 60. This was a recovery action observed in a failed
initial-focus test, not an acceptable required workflow. No matched-scene
performance benchmark or independently measured frame-time result is claimed.

The matching retained run is `20261025T120456.428363000Z-pid50997`
(2026-10-25 21:04:56 KST). Confirmed log evidence:

- `UPDATE_PROFILE_VALIDATED`: schema 1, installed digest
  `ed4970761703564d57f677e01f3641b6275a1bc3f4756da6f80b14b76476686a`.
- Pinned runtime effective configuration verified, all 17 Vulkan entries
  redirected, inactive 100-ms sleep bypass active, focus callbacks unmodified
  and active byte observed only.
- Compositor latch: generation 2, ordinal 150, 79 suppressed draws; startup
  finished at ordinal 180. No ERROR/FATAL/SKIP record appears in this run.
- Initial `active=no` at 1,686 ms. No prior key/active notification appears.
  Resign-active notification at 44,545 ms still has `active_byte=no`.
- Become-key at 45,887 ms; become-active at 45,899 ms; internal `active=yes`
  at 45,954 ms. This ordering is consistent with the reported app-switch
  recovery. It does not independently locate the character-selection/world
  transition or prove which upstream component failed activation.

The packaged Status reports `Last launch: PASS` for its seven structural
invariants and explicitly disclaims FPS/focus verification. That verdict does
not override this experiment's initial-focus/FPS failure. The historical
`tools/check_startup_log.py` checker was also tried; it rejected six legacy
instrumentation expectations absent from this production mode (including
RUN_START PID/config/proc-route trace fields). Its verdict is not used to
classify this candidate; current packaged checks and direct milestone review
supply the structural evidence.

The production log was copied without modification into ignored local evidence
storage `artifacts/0060-user-launch-20261025/`, alongside an extracted 649-line
run and identity manifest. Full log SHA-256 is
`3a528c1018f2b7238217e17393924c9599ad7eee29cbee3bea45714be570e29b`,
size 284,717 bytes. Raw logs remain uncommitted. Post-run executable, bridge,
renamed original, runtime, profile, marker, journal and raw recovery hashes all
match the verified installation. No installed files, settings or caches were
changed by this analysis, and no app was started by the agent.

Result: runtime dynamic discovery/revalidation and startup controls are proven
on this exact 12.1.6 run; initial focus and approximately-60-FPS acceptance
failed. This repeats the activation symptom on macOS 27 with argument buffers
off and the inactive sleep already bypassed. Neither dynamic relocation nor
the runtime upgrade is shown to repair activation. The known Steam/launcher to
AppKit activation boundary remains the next investigation; no further manual
launch repetition or one-shot activation fallback is requested. Candidate
0.3.0-rc.1 remains installed with verified same-generation removal available.
No rollback, public release or overall gameplay PASS is claimed.


## Second natural-launch amendment and release scope (2026-10-25)

The user subsequently reported that the second launch did not repeat the
initial focus defect, and asked whether to release the dynamic update feature
now rather than continue extending the scope. This preserves the first failure
above; it does not prove a first-run-only cause or a permanent activation fix.

Read-only update verification remains CURRENT. The matching second run is
`20261025T121527.882687000Z-pid52734` (21:15:27 KST). It independently logs
the same validated schema-1 digest, all 17 redirects, the 79-draw/ordinal-150
latch and ordinal-180 finish. Unlike the first run, become-key arrives at
1,332 ms, become-active at 1,341 ms, and the first observed internal state is
`active=yes` at 1,659 ms. Much later resignation/reactivation events do not
invalidate the initially healthy activation. No additional controlled launch
was requested. The second run and current full log are preserved in ignored
`artifacts/0060-user-launch-20261025-second/` with a SHA-256 identity manifest;
analysis does not modify the production log.

The release recommendation is a public 0.3.0 prerelease with the intermittent
initial-activation issue disclosed, rather than expanding this candidate into
an unattended installer. This is a recommendation, not publication authority
or a completed release. The first-run acceptance failure remains recorded and
public stable-release eligibility needs an explicit scope/known-issue decision.

Current capability: the installer derives a new compatible executable profile;
the loaded bridge independently re-discovers and verifies that installed plan
at runtime. It does not re-attest a changed executable automatically: the
marker hash check precedes discovery. If an official updater replaces the Bink
proxy, that proxy cannot execute to restore itself. Re-running Install remains
necessary for changed executable inputs; a retained proxy requires vendor
Repair first, and unsupported original generations still stop. Future support
for unattended recovery would require an external component and the same
provenance/idle/transaction gates. No such component was added here.


## Publication decision amendment (2026-10-25)

Following the second normal initial-activation run and explicit review of
unattended-reinstall limits, the user authorized immediate publication.
Experiment [0061](0061-public-dynamic-profile-prerelease.md) records public
0.3.0-rc.1 with byte-identical tested bridge/auditor/runtime payloads, updated
player documentation and the activation issue disclosed. This supersedes the
prior no-publication state without changing the first failure, second success,
or the lack of a proven first-run-only cause. Stable promotion is separate.
