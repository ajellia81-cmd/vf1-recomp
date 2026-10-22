# Preparation validation

Original source: ae7d6f60baa0b81a9c3cf9d604cd5cc6aaae107f.

Completed in the remote Linux workspace, without ROM data:

- Generated Musashi opcode source from its redistributable opcode definitions.
- G++ C++20 syntax/return-type checks across the iOS-preprocessed host/menu,
  V60/MB86233 cores, Model 1 machine/audio/render files, interpreter runtime,
  and the new display/control checks; VF_GPU3D_METAL=1, VF1_DISABLE_JIT=1.
- Built and ran the actual ZIP reader's missing-archive diagnostic check.
- Parsed Python build scripts, both YAML configurations and shell scripts.
- Compared decoded icon bytes with the user's supplied JPEG exactly.
- Checked whitespace and reviewed generated-file exclusions.

Not yet run: XcodeGen project verification on macOS, Apple SDK device/simulator
compilation, Metal/display/control runtime checks and document-import XCTest.
The workflow contains those checks; they have not passed merely because the
workflow was written. No IPA has been created for this port yet.

No physical-device/gameplay tests have been run. Game behavior, sound, controller
hotplug/remaps, 120 fps, suspend/relaunch and Quit (including LiveContainer)
need separate confirmation. The 2868x1320 presentation resolves the original
integer-scale picture; it does not add VF2's fractional native 3D rasterizer.

Pending remote prerequisite: a personal vf1-recomp fork must exist before
commits can be pushed and Actions can execute. The connected GitHub toolset
supports branch/tree/commit updates but does not expose fork creation.
