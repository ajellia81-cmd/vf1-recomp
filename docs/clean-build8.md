# Clean personal build 8: throw damage feedback

The user reports VF1 has no vibration when dealing or receiving throw damage;
VF2 throw feedback is confirmed working and is not modified for this issue.

Cause found in source: once damage_events is enabled, Tracker replaces observed
health loss with totals from the single verified strike PC 0xFFBB96. Other
health-reducing paths therefore yield zero event damage and are discarded.

Fix: use max(actual frame health loss, verified pre-clamp event total delta).
This restores feedback for damage through other paths (including throws) while
retaining pre-clamp post-KO strikes and avoiding summing the same hit twice.
A grab that causes no damage does not create feedback. Existing human-side,
live/replay, reset, refill, duplicate-frame and gain gates remain unchanged.
The fix does not claim a newly mapped throw writer or recover throw damage
hidden entirely by a zero-health clamp.

Regression checks exercise the actual VF1 snapshot adapter with both human sides,
both victims and damage 4/20/50, unchanged strike counters, no-event frames and
ordinary-hit parity. Further cases cover mixed event/HP loss and lethal throws.
All earlier pre-clamp/KO/post-KO checks remain. Clean-build7 diagnostic removal
and package inspections are preserved. Version 8 replaces build7 as the final
VF1 candidate. No physical device gameplay test was performed here.

## Post-replay victory-pose contacts

The user additionally reports missing feedback when striking during the victory
pose after the replay. Previously both the store observer and frame adapter
required the two active-round flags for every contact, excluding this interval.
The revised observer also accepts the already verified full-word subtraction
PC at zero victim health while a human remains in the match. The adapter admits
zero-health human-match snapshots with event_only: only verified event totals
can emit feedback outside the active-round gate; arbitrary HP changes cannot.
This preserves dynamic pre-clamp damage strength and does not repeat the KO tail.

No independent victory-versus-replay flag is invented. If a human-match replay
itself executes the same verified zero-health subtraction, it may also vibrate;
HP-only replay/setup transitions remain silent. Physical confirmation of the
reported victory contacts is still required. Tests cover both human/victim sides,
zero-health contacts with round flags off, no-human suppression and HP-only silence.
