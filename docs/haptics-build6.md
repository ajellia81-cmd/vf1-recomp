# Personal build 6: post-KO damage events

Source: d93bd96561be7c38b7f72958eb51138ab770a8b0
Successful workflow: https://github.com/ajellia81-cmd/vf1-recomp/actions/runs/39956006117
IPA: VirtuaFighter-build6-unsigned.ipa (7,453,327 bytes)
SHA-256: cf78c54bea06575556e26fe1a884a2ecf9296129923d22d67f3d06501124b77a

The user-supplied scalar trace shows damage subtraction at guest PC 0xFFBB96
before the separate zero-health clamp at 0xFFBB9C. The iOS bus accumulates bounded,
complete health-field damage stores from the observed subtraction PC during live
play. This retains post-KO 0 -> negative -> 0 contacts that frame-end health deltas
miss. No game-memory writes or audio triggers are introduced.

The tracker consumes monotonic totals once per original frame, retaining player
routing and damage-responsive strength, avoiding health/event double counting,
and applying the longer KO tail only on positive-to-zero transitions. The phone
and controller cold-start preparation from build 5 remains in place.

Apple CI passed interpreter-only project verification, Model 1 Metal/display/
controls/autosave and missing-archive checks, nonblocking controller concurrency
and scalar diagnostic bounds/export checks, tracker/adapter and new pre-clamp
event tests, touch Menu checks, iPhone Release and simulator compilation, both
iOS Simulator import XCTest cases, packaging and artifact upload. Linux synthetic
tests also passed. All automated tests use no game data.

The downloaded artifact digest matches GitHub and the IPA digest matches the
runner. Both ZIP containers passed integrity checks. Bundle version, source,
interpreter-only/no-ROM metadata and absence of game archives were verified.
The executable contains both native haptic backends and the diagnostic menu.

Physical iPhone/controller feedback, broader move coverage and gameplay frame
pacing remain untested here. The supplied trace provides scalar event evidence,
not a physical-device test of this updated build. No ROM bytes or disassembly
were used or distributed.
