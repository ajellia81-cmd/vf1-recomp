# Personal haptics build 4 validation

- Source: d54b83c409876d51cf457b12abcf1085bfa0990e
- Workflow: https://github.com/ajellia81-cmd/vf1-recomp/actions/runs/39939550832
- Output: VirtuaFighter-build4-unsigned.ipa
- Size: 7,444,095 bytes
- SHA-256: 2b610e090dbcd02ed2ed82e2e737167557a34d7a65a9795dd7698b87bbfb09d6

Apple CI passed interpreter-only project verification, iPhone Release and simulator
Debug compilation, synthetic Model 1 Metal/display/controls/autosave checks, combat
haptics tracker/adapter checks, touch Menu checks and missing-archive checks.
Two actual iOS Simulator import XCTest cases passed with zero failures. Packaging
checks passed without signing, archive/export or ROM data.

Downloaded artifact SHA-256 and IPA SHA-256 match GitHub's artifact digest and
runner output respectively. ZIP integrity, bundle/build/source metadata, interpreter
policy and absence of game archives were checked. The executable contains the
Core Haptics backend and the vibration controls.

Linux synthetic tracker/adapter tests passed with C++20, -Wall -Wextra -Werror.
An additional temporary SDL mock harness checked assigned-controller routing, phone
fallback after failed rumble, Both/zero strength, menu cancellation, resume baseline,
controller identity changes and disable. This verifies logic, not physical hardware.

See HAPTICS.md for behavior and event limitations. No physical iPhone/controller
haptic test or gameplay test was performed for this new build. Existing user reports
about earlier builds do not establish this build's tactile feel.
