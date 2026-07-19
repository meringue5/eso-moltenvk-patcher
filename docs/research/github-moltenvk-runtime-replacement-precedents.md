# GitHub precedents for replacing a bundled MoltenVK runtime

- Review date: 2026-07-19
- Scope: public GitHub repositories, issues, pull requests, commits, and code
- Question: Has a newer MoltenVK previously been substituted into an old,
  proprietary game package in a way comparable to `teso4m4`?

## Conclusion

No public example was found for ESO itself, or for the exact `teso4m4`
architecture: patching Vulkan entry points in a closed-source native macOS
executable after MoltenVK has been statically absorbed into that executable.
This is a bounded search result, not proof that no private, unindexed, or
non-GitHub attempt exists.

Several adjacent projects replace a dynamically loaded MoltenVK in CrossOver,
Wine, or a developer-controlled application. They establish that replacement
can work, but also show that an upstream release is not automatically a drop-in
upgrade for an old game. Game-specific patches, legacy extension behavior,
resource-lifetime tolerance, and distributor-specific MoltenVK changes can be
decisive.

## Search coverage

The review searched public GitHub material for combinations of `Elder Scrolls
Online`, `ESO`, `Zenimax`, `eso.app`, `MoltenVK`, `override`, `replace`,
`libMoltenVK.dylib`, `DYLD_INSERT_LIBRARIES`, `fishhook`, and
`mach_vm_protect`. Searches covered the Khronos MoltenVK project and relevant
CrossOver, Wine, DXVK, Whisky, and XIV-on-Mac projects.

No relevant ESO or ZeniMax match appeared in repository, issue, pull-request,
commit, or code searches. No MoltenVK-specific public example was found that
used a proxy dylib plus runtime text patching to supersede a statically linked
MoltenVK implementation.

GitHub search cannot cover private repositories, deleted material, chat-only
work, or code that uses unrelated terminology. The negative result must
therefore be described as "not found in the reviewed public material."

## Closest precedents

| Case | Replacement mechanism | Reported result | Relevance to `teso4m4` |
|---|---|---|---|
| [MoltenVK-Detroit](https://github.com/DiAvisoo/MoltenVK-Detroit) | Replaces CrossOver's dynamic `libMoltenVK.dylib` with a Detroit: Become Human-specific fork; preserves the original and combines runtime settings, a game binary patch, and a shader cache | A game-targeted fork was required to make useful progress where the bundled runtime failed | Closest match in objective, but not in linkage or interception method |
| [CXPatcher](https://github.com/italomandara/CXPatcher) | Replaces CrossOver's dynamic MoltenVK and related translation components, with backup and restore support | Makes runtime substitution repeatable while acknowledging version-specific risk | Useful precedent for reversible deployment, not for static-link interception |
| [MoltenVK issue #2530](https://github.com/KhronosGroup/MoltenVK/issues/2530) | Manually substitutes upstream MoltenVK releases for CrossOver's bundled dylib and tests several games | Newer releases could regress performance; a CodeWeavers-patched build could outperform the nominally equivalent upstream release | Warns that provenance and downstream patches matter as much as version number |
| [MoltenVK issue #846](https://github.com/KhronosGroup/MoltenVK/issues/846) | Replaces an old MoltenVK shipped with the Steam game FPS Infinite | The new runtime crashed in surface-support setup; investigation exposed a mismatch around legacy `VK_MVK_macos_surface`, newer `VK_EXT_metal_surface`, SDL, and loader behavior | Strong precedent for a startup crash caused by legacy extension and proc-lookup assumptions |
| [MoltenVK issue #573](https://github.com/KhronosGroup/MoltenVK/issues/573) | Replaces Dolphin's 1.0.33 dylib with 1.0.34 | Immediate startup failure was fixed upstream, after which a separate surface-destruction crash appeared | Shows that one compatibility fix can expose the next lifetime or teardown incompatibility |
| [MoltenVK issue #2282](https://github.com/KhronosGroup/MoltenVK/issues/2282) | Upgrades the MoltenVK used by vkQuake | A descriptor-pool error and null handle that the application ignored became a crash in `mvkUpdateDescriptorSets` | Strong precedent for a newer runtime exposing an old application's tolerated invalid behavior |
| [MoltenVK issue #1817](https://github.com/KhronosGroup/MoltenVK/issues/1817) | Migrates a source-controlled application from static MoltenVK linkage to the Vulkan Loader | Requires a build/link integration change | Confirms that normal loader-based selection is a source-level migration, not a post-link remedy for ESO |

`MoltenVK-Detroit` and CXPatcher operate on a replaceable dynamic library. Their
success does not validate runtime patching of ESO's statically linked wrapper
entries. Conversely, their game-specific modifications suggest that a small
ESO compatibility layer or MoltenVK fork may be a more realistic end state than
an unmodified upstream dylib.

## Relevant upstream behavior

The [MoltenVK runtime guide](https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Runtime_UserGuide.md)
documents both static and dynamic integration. Replacing a file or selecting a
driver through the
[Vulkan Loader](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderDriverInterface.md)
only helps when the application actually uses that dynamic integration path.
ESO does not.

The [MoltenVK change history](https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/Whats_New.md)
also documents less-forgiving descriptor resource-lifetime behavior and the
`MVK_CONFIG_LIVE_CHECK_ALL_RESOURCES=1` compatibility option in the 1.4 series.
That makes the option a justified diagnostic control for ESO, but it does not
establish that descriptor lifetime caused Experiment 0001.

## Application to `teso4m4`

### Confirmed local observations

- ESO contains MoltenVK 1.0.18 as static code and has no dynamic Vulkan or
  MoltenVK dependency.
- Experiment 0001 successfully loaded MoltenVK 1.4.1 and patched the 17 selected
  public entry points.
- The process then crashed during early graphics initialization with `RIP=0`.
- The selected redirect set was based on direct calls and did not prove complete
  coverage of address-taken functions, tables, relocations, or callbacks.

### Inferences supported by the precedents

- Repeating Experiment 0001 unchanged is expected to fail again.
- Reaching startup with an unmodified upstream 1.4.1 dylib is plausible, but
  only after complete dispatch ownership and legacy behavior are established.
- A successful startup would not imply gameplay stability or a performance
  improvement. Downstream MoltenVK patches and per-game behavior have produced
  both regressions and improvements elsewhere.
- The likely deliverable, if a generic redirect remains incompatible, is an ESO
  compatibility shim or narrowly maintained MoltenVK patch set rather than a
  version-independent drop-in override.

### Risk judgment

The project should not yet be judged infeasible. The current failure occurred
after the replacement runtime was loaded and the selected wrappers were
redirected, so the basic proxy and Rosetta patch mechanism crossed its first
technical boundary. A null proc/callback, missed wrapper, or legacy extension
difference would be difficult but tractable.

The risk becomes substantially worse if evidence shows that ESO depends on
private MoltenVK 1.0.18 object layouts or that old and new runtime ownership
cannot be separated without patching a broad, moving set of internal calls. That
would require a redesign and could make the maintenance cost disproportionate.

The practical assessment is therefore:

| Goal | Current assessment |
|---|---|
| Repeat the existing full redirect unchanged | Expected to fail |
| Explain the startup crash | Reasonably achievable |
| Reach stable startup after exhaustive routing and compatibility work | Plausible, with high implementation risk |
| Sustain normal gameplay | Unknown until startup and teardown paths are covered |
| Improve the observed long-session performance degradation | Entirely unproven; a successful override may be neutral or slower |

## Decision gates

Continue the current architecture if the next evidence identifies a missing or
null public proc, an incomplete redirect, or a documented legacy behavior that
can be shimmed without weakening safety checks.

Reconsider the architecture if exhaustive analysis finds pervasive private ABI
dependencies, unavoidable mixed ownership of Vulkan objects, or a redirect set
too broad and version-sensitive to validate safely after each Steam update.

Even if a stable override is achieved, retain the old runtime as the performance
baseline. The replacement should be considered successful only if controlled
measurements show a worthwhile benefit without new correctness or stability
regressions.
