# Miscellaneous notes

This document retains plausible future directions that are not yet approved
experiments or roadmap commitments. Each item must remain clearly separated
from confirmed findings and completed experiment evidence.

## One shared MoltenVK runtime, with per-game adapters

### Idea

Keep a current MoltenVK runtime available on the Mac and make individual games
use it through the mechanism their packaging permits:

```text
current MoltenVK runtime
        ├── dynamically linked game: dylib replacement or loader selection
        └── statically linked game: compatibility shim and entry-point routing
```

Euro Truck Simulator 2 is a useful comparison because its macOS package
contains a dynamic `libMoltenVK.dylib` and its Vulkan renderer can be selected
instead of the older OpenGL path. ESO is materially different: its MoltenVK
1.0.18 implementation is statically linked into the executable, so placing a
new dylib on the system cannot make ESO use it without the bridge or a rebuilt
application.

### Why keep the idea

If compatibility and stability are established, a newer MoltenVK may improve
performance for some workloads by improving Vulkan-to-Metal translation or
resource management. The ETS2 observation makes this a reasonable hypothesis,
not a prediction for ESO. Newer MoltenVK releases can also regress performance,
and downstream builds may contain game-specific patches or settings.

### Proposed long-term shape

- Maintain one or more pinned MoltenVK builds, rather than assuming “latest”
  is universally best.
- Select the runtime per application and per compatibility profile.
- Keep dynamic-library replacement separate from ESO's static-runtime bridge.
- Record runtime version, configuration, game build, and rollback state for each
  comparison.
- Treat a stable launch as a correctness milestone; measure performance only
  afterward with matched scenes and settings.

### Not yet established

- macOS does not provide a universal system Vulkan driver path that overrides
  every application's private or statically linked MoltenVK.
- A stable ESO bridge does not imply that its long-session performance improves.
- A shared runtime is not currently a reason to modify the ESO installer or
  change the active game bundle.

### Decision gate

Revisit this idea after the ESO startup crash has been explained and the bridge
has passed controlled startup and teardown checks. Until then, it is a design
direction only, not an installation plan or a performance claim.
