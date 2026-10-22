# Personal iOS port

- Work on ajellia81-cmd/vf1-recomp, ios-port. Upstream mr-finster/vf1-recomp is read-only.
- Small reviewable commits; never force-push or push upstream.
- Preserve VF1_IOS and VF1_DISABLE_JIT. Never define VF_NATIVE for iOS, include
  SLJIT/native.cpp, obtain ROMs, or commit game/ROM-generated data.
- The user authorized a personal unsigned IPA using the attached icon. No public
  release, signing secrets or ROM distribution. No dependence on the user's PC.
- ios/project.yml is authoritative. Do not commit generated projects, dependencies,
  opcode tables or build outputs.
- Use Korean progress/final messages and distinguish source/Linux, Apple SDK,
  simulator and physical-device checks. Never claim gameplay or 120 fps from CI.
