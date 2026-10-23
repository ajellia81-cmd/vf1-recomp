# Personal build 5: cold haptics preparation

Source: 2a8ca81831865fa014b9d5a81a96d5166f2367da
Successful workflow: https://github.com/ajellia81-cmd/vf1-recomp/actions/runs/39948620024
IPA: VirtuaFighter-build5-unsigned.ipa (7,453,134 bytes)
SHA-256: 31c52f38f6f88ba9b5993a28c161973653e8cfbbe6ed5372e64c1ab527b870c8

The phone engine/player prepare on a serial queue before impacts. The pinned SDL
Apple controller driver prewarms native motors off the render thread and outside
the joystick lock, using the implementation tested in personal Daytona build 8.
Stops, disconnects and resets supersede queued work. Phone impacts stale after cold
initialization are dropped. Background and disabled feedback suspend the phone
engine; foreground play keeps it warm.

The first build attempt exposed a menu callback referencing a worker declared
later. The corrected source installs its join callback after worker construction,
before any menu event can run. The successful build includes that correction.

Apple CI passed interpreter-only project verification, iPhone Release and simulator
Debug compilation, synthetic Model 1 Metal/display/controls/autosave checks, actual
macOS execution of the controller concurrency harness (blocked initialization,
1000 coalesced requests, stop, disconnect during initialization/update, retry and
recovery), scalar diagnostic ring bounds/export, health tracker/adapter, touch Menu
and missing-archive checks, and both iOS Simulator import XCTest cases.

Downloaded artifact and IPA digests match GitHub/runner output. ZIP integrity,
bundle/build/source metadata, interpreter-only policy and no game archives were
verified. The executable contains VFCombatIOSRumbleContext, VFFightPhoneHaptics and
Save hit diagnostics, confirming that the patched SDL was linked.

No physical iPhone/controller haptics, gameplay or frame pacing test was performed.

## Unresolved request: post-KO contacts without health changes

Automatic detection still needs a verified contact event independent of health
loss. No speculative animation/button/sound trigger was enabled. Read-only scalar
health-store observations are retained in a fixed ring. Controls > Save hit
diagnostics exports haptics-hit-diagnostics.csv to Library/Application Support/
Virtua Fighter Recomp inside the app data container after joining the game worker.
Reproduce missing post-KO impacts, then export promptly. No ROM/program bytes or
full memory dump are captured. Stores are observations, not confirmed contacts.
