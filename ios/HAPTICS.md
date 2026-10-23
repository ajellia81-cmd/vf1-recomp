# Personal iOS combat haptics

Controls > Vibration enables damage-responsive feedback (default on, strength 75%, Player 1).
Vibration strength, Vibration players and Test vibration are saved in settings.ini.
A controller with working SDL rumble receives its assigned player's feedback.
Otherwise the phone uses Core Haptics. With Both selected the shared phone follows P1;
select P2 to follow P2. Hardware without haptics remains silent.

Health loss is observed after original emulation frames, never interpolated display frames.
Intensity varies continuously with damage, with a sharper landed-hit response, a heavier
received-hit response and a longer zero-health finish. Concurrent impacts merge by maximum,
not addition; phone impacts replace old tails. Menu, pause, fast forward, background,
reset and controller changes cancel feedback and reset the baseline. The first frame,
health refill, skipped frames and invalid values cannot produce an impact.

This is an original implementation inspired by event-based fighting-game feedback,
not a reproduction of Tekken's unpublished algorithms. Guard, counter, throw, ring-out
and cinematic events are not inferred. Damage cheats may suppress or change feedback.

## Evidence and limits

Read-only addresses come from model1::fight_cheats (fighter pointer, energy and full
energy), tools/vfscript.cpp (live/replay flags), and the existing player selection flags.
Only selected human player sides receive feedback.

Synthetic tests check event extraction, routing, strength, KO, duplicate frames, resets,
invalid values and adapter gates without game data. Apple CI checks compilation; physical
iPhone/controller feel and exact game-transition timing still require device testing.

## Cold-start preparation

Phone engine startup and player creation run on a dedicated serial queue, prepared
when feedback is enabled/foregrounded. The engine stays warm during foreground
play; background and exit dispose it. Stops replace queued hits; effects older
than 100 ms are dropped after cold initialization. Reset recovery is silent.
The pinned SDL Apple driver uses the same off-thread motor preparation and
coalesced state updates validated by the personal Daytona build 8. Opening a
controller no longer creates native engines under SDL's joystick lock.

This removes synchronous cold engine work from the rendering thread. Physical
frame pacing and haptic timing must still be checked on the user's device.

## Post-KO diagnostic limitation

Health loss alone cannot identify a hit that leaves health unchanged. It must not
be synthesized from an attack button or arbitrary animation. The app keeps a
bounded ring of scalar stores to the documented health fields (including zero-to-zero),
with frame, instruction address, side and scene flags. Controls > Save hit diagnostics
exports haptics-hit-diagnostics.csv after the emulation worker has stopped.
No file I/O occurs on the emulation/rendering hot path. No ROM bytes or memory dump
is captured. Stores themselves are not assumed to be hits. Post-KO feedback remains
unresolved until a contact signal and replay exclusion are verified.
