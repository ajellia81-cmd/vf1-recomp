# Virtua Fighter — personal iOS port

Upstream: https://github.com/mr-finster/vf1-recomp, source ae7d6f60baa0b81a9c3cf9d604cd5cc6aaae107f.
Personal fork: ajellia81-cmd/vf1-recomp, branch ios-port. Never push upstream.

The iOS target is interpreter-only (V60), without VF_NATIVE, SLJIT, dynamic
native-code allocation, or ROM-generated source. No game data is included.
Import a personally owned complete MAME vf.zip with the TGP program on-device.
Import is coordinated/copied and validated off the UIKit thread, then installed
atomically. Files and saves remain inside this app's Application Support folder.

Display name: Virtua Fighter. Bundle ID: com.ajellia81.vf1recomp. The supplied
opaque square artwork is decoded and resized by macOS sips for AppIcon.

The app uses Model 1's own Metal renderer, configurable SDL gamepads/keyboards,
and native touch directions/G/P/K/Coin/Start/Menu when no controller is connected.
Pad selection/remapping remains in the original Controls menu. Swap state is saved.
Display offers 1x-6x and 2868x1320 output. The latter resolves the original 4x
geometry into a 2868x1320 target; it is not VF2's fractional native 3D rasterizer.
Antialiasing is separately selectable. Fullscreen follows the landscape device
aspect. UI coordinates and FPS safe-area placement are isolated from game targets.
Brightness is 50-200%; Screen and Background filters retain the original menus.
Smooth motion can be toggled, with maximum-refresh iPhone opt-in. Actual 120 fps
is not guaranteed or measured without a device and game.

Quit completes saves, waits for workers and tears down resources, then exits the
iOS process. In LiveContainer this may also close the container execution screen;
automatic return to its app list is not guaranteed. Backgrounding saves state and
opens the menu. The game state itself is not serialized; NVRAM/rankings are saved.

## Remote build

The GitHub Actions workflow compiles static pinned SDL/FreeType dependencies,
generates Musashi opcode tables from its redistributable source, prepares the
icon and XcodeGen project, runs synthetic Metal/display/controls/import checks,
compiles iPhone/simulator targets and packages an unsigned IPA for personal signing.
No Xcode archive/export, Apple credentials, or user computer is required.
Project source of truth: project.yml. Generated projects, libraries and build
outputs are ignored. Build output: VirtuaFighter-build1-unsigned.ipa.

Local Linux C++ syntax and missing-archive checks do not establish iOS compilation,
valid import, game graphics/audio/cheats/input, achieved frame rate or device Quit.
Apple SDK and device validation must be recorded separately after they occur.
