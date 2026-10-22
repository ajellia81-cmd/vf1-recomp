# Virtua Fighter personal unsigned IPA — build 1

- Personal fork: ajellia81-cmd/vf1-recomp, branch ios-port; upstream unchanged.
- Source: 3fc81af755c27ad1900988c6852cbc289c58bd0b.
- Successful remote build: https://github.com/ajellia81-cmd/vf1-recomp/actions/runs/39782714684
- Output: VirtuaFighter-build1-unsigned.ipa (7,435,535 bytes).
- SHA-256: 520adaa3133d4c37fec4e7e06113296ab33d4f78a01ae8ed28238097bbcf3be9.
- Bundle ID: com.ajellia81.vf1recomp; display name Virtua Fighter; build number 1.
- arm64 iPhoneOS Release, minimum iOS 15, unsigned for personal installation/signing.

The user supplied square white-background artwork, preserved in the icon build
pipeline, is compiled as AppIcon. Controllers use configurable SDL mappings;
native touch buttons appear only without a connected controller. Display has
integer scales 1x-6x or a 2868x1320 output resolved from 4x geometry, brightness
50-200%, screen/background filters, smooth-motion toggle and safe-area FPS.
The 2868x1320 option is not a fractional native 3D rasterizer. Main menu Quit
saves and cleans up before process exit; LiveContainer may close its execution
screen too. Automatic return to its app list is not guaranteed.

Source and pixel/Metal/control/autosave checks, Apple device/simulator compilation,
two simulator import tests and packaging checks passed. Downloaded IPA metadata,
ZIP integrity, source identity and hash were checked. Gameplay and actual 120 fps
are not established by CI. See ios/VALIDATION.md and ios/OPTIONS_AUDIT.md.

V60 execution is interpreter-only. No ROM data, extracted game data or generated
game program was downloaded, used or included. The player imports their own
complete MAME vf.zip including its TGP program, on-device. No release was published,
no Apple signing secrets were used, and no user computer was involved.
