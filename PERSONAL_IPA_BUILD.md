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

## Build 2: original resolution order and selectable aspect

- Source: 6289a6d6d3f506bd849afca5f25d1c77d87f0bbd
- Successful Apple build: https://github.com/ajellia81-cmd/vf1-recomp/actions/runs/39788420982
- IPA: VirtuaFighter-build2-unsigned.ipa (7435427 bytes)
- SHA-256: f9f0dd3339e2299ca222ea5eac3c43b129522a7ab317146593791ee1747b438c

Resolution choices are 1x, 2x, 3x, 4x, 5x, 6x, then 2868 x 1320 (iPhone).
Every choice, including iPhone output, follows Picture shape: Fit the window,
4:3, 16:10, 16:9 or 21:9. VF2's former hidden Picture shape is restored and
no longer reset at launch. All saved resolution keys retain their old meaning.
Apple compilation, Metal composition/filter/brightness checks, controls and two
simulator import tests passed. Downloaded ZIP, build/source metadata and SHA-256
match the runner. Physical-device gameplay has not been tested by CI.
Interpreter-only execution, no ROM inclusion, personal unsigned use only.

## Build 3: touch Menu transition fix

- Source: 9e4b61173a18c96b20ae467919154c7eefc9cfd2
- Successful Apple build: https://github.com/ajellia81-cmd/vf1-recomp/actions/runs/39810615150
- IPA: VirtuaFighter-build3-unsigned.ipa (7435408 bytes)
- SHA-256: 9d373e8c6093963afa18dc434e16d6143c9a4667924528ee67654ef3ca280870

The UIKit bridge releases gameplay buttons when entering the menu without
erasing a pending Menu tap, including repeated updates while the menu stays open.
The actual transition-helper check, Metal/composition/filter/brightness checks,
controls persistence, Apple builds and two simulator import cases passed.
Downloaded ZIP integrity, build number, source identity and SHA-256 were verified.
The IPA contains no ROM data and remains interpreter-only and unsigned for
personal use. CI does not establish physical-device gameplay or performance.
