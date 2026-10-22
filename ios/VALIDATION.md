# Personal iOS build 1 validation

- Upstream: mr-finster/vf1-recomp, ae7d6f60baa0b81a9c3cf9d604cd5cc6aaae107f.
- Personal fork/branch: ajellia81-cmd/vf1-recomp, ios-port.
- Built source: 3fc81af755c27ad1900988c6852cbc289c58bd0b.
- Successful workflow: https://github.com/ajellia81-cmd/vf1-recomp/actions/runs/39782714684
- Output: VirtuaFighter-build1-unsigned.ipa, 7,435,535 bytes.
- SHA-256: 520adaa3133d4c37fec4e7e06113296ab33d4f78a01ae8ed28238097bbcf3be9.

Completed in the remote Linux workspace without ROM data:

- Generated Musashi opcode tables from its redistributable opcode definitions.
- G++ C++20 syntax/return-type checks across the iOS-preprocessed host/menu,
  V60/MB86233 cores, Model 1 machine/audio/render files, interpreter runtime
  and display/control check sources with VF1_DISABLE_JIT=1.
- Built and ran the actual ZIP reader's missing-archive diagnostic check.
- Passed the autosave drain/resume check (future writes still accepted).
- Parsed Python build scripts/YAML/shell scripts; checked whitespace.
- Verified decoded supplied icon bytes exactly against the uploaded JPEG.

Completed on macOS GitHub Actions:

- Built pinned static arm64 iPhoneOS/simulator SDL and FreeType packages.
- Actual Model 1 Metal synthetic polygon edges/colors at 2868x1320.
- Actual Model 1 compositor self-check and SDL/Metal pixel readback for brightness
  100/150/200/100%, fixed output target, stale scale/viewport/scissor reset,
  visible Scanlines/CRT differences and restored Off pixels.
- Both players' controller selection and every keyboard/pad action round-trips.
- Autosave drain/resume and missing-archive checks.
- XcodeGen source/resource/configuration verification; native.cpp and SLJIT excluded.
- Unsigned Release iPhone and Debug simulator app compilation.
- Two actual iOS Simulator import XCTest cases, zero failures: asynchronous
  invalid-document completion on the main thread and cancellation exactly once.
- arm64/iPhoneOS/app-icon/high-refresh metadata, no SLJIT symbols and ROM-free
  packaging checks. No signing credentials, archive/export or public release.

Synthetic display checks use host Homebrew SDL. The iOS app uses the pinned
static iOS SDL libraries. Neither is a game ROM test. Downloaded IPA ZIP integrity,
source/version/bundle metadata and SHA-256 match the successful runner output.

Not verified on a physical device: valid ROM/provider import, actual game
rendering/audio/rules/cheats, controller hotplug/remapping, touch gestures,
achieved 120 fps, suspension/relaunch and Quit including LiveContainer. The
2868x1320 presentation resolves the original 4x picture; it does not add VF2's
fractional native 3D rasterizer. See OPTIONS_AUDIT.md for source-level coverage
of every category. No game archive or extracted data was obtained or included.
