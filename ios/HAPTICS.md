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
