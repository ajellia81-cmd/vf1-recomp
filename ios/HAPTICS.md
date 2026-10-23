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

## Pre-clamp damage contacts (build 6)

The user's build 5 scalar trace identifies a real damage store at guest PC 0xFFBB96.
At frame 2010 it writes 22 -> -8, then 0xFFBB9C clamps -8 -> 0. After KO, frame
2108 writes 0 -> -30 at the same damage PC and clamps -30 -> 0 at the same clamp PC.
Both round/replay gates remain active. Sampling only at frame end loses this hit.

The iOS bus observes full-word health stores at the verified damage PC while a human
side and both existing live gates are active. It accumulates positive damage before
clamping, without changing game memory or CPU execution. Snapshots consume these
monotonic totals once per original frame; health loss is not counted a second time.
Only the transition from positive final health to zero gets a KO tail. Later contacts
at zero health receive ordinary impacts, retaining damage-dependent strength and
player routing. The clamp, setup writes, partial stores and inactive scenes are ignored.

Synthetic checks reproduce the observed subtract/clamp sequences for both sides and
check duplicate frames, routing, stale/reset totals, replay gating and no double-counting.
The observation establishes the missing event in the supplied VF1 session; broader
physical-device and move coverage still needs confirmation. No VF2 addresses are inferred.
No ROM bytes, disassembly or generated game code are used. Scalar diagnostics remain
available under Controls > Save hit diagnostics for further validation.


## Post-KO intensity audit

The verified pre-clamp writer retains actual damage after KO. The response curve
uses that damage exactly as in a normal hit; there is no post-KO gain reduction.
Only the original positive-to-zero transition adds a long KO tail. Synthetic
comparisons for damage 1/4/12/30, both victims and strengths 0/25/75/100 confirm
identical normal/post-KO amplitude and ordinary duration. No runtime change was
needed for VF1. Physical feel remains device-dependent.
