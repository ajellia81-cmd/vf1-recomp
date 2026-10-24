# Clean personal build 7

User requested final VF1/VF2 packages without diagnostic features. Build7 removes
Test vibration and Save hit diagnostics from the VF1 Controls menu, the recorder
instance and per-store recording, and its worker-wait export callback. Production
pre-clamp damage observation and damage-dependent post-KO feedback remain intact.
Vibration enable/strength/players settings, warmup/lifecycle, rendering and input
are preserved. No CSV exports or record buffers are present in the application.
Packaging checks the executable for diagnostic menu/export strings and recorder
symbols, bundle version 7, interpreter-only/no-ROM policy and ZIP integrity.
This is a personal unsigned IPA for re-signing, not a public Release publication.
Automated CI does not establish physical gameplay or vibration performance.
