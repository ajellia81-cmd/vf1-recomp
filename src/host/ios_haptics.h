#pragma once
// Native phone fallback; physical controllers use the host's existing SDL player assignments.
void ios_haptics_play(float low, float high, unsigned milliseconds);
void ios_haptics_prepare();
void ios_haptics_suspend();
void ios_haptics_stop();
void ios_haptics_shutdown();
