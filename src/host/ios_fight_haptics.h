#pragma once
#include "fight_haptics.h"
#include "ios_haptics.h"
#include <SDL3/SDL.h>

class ios_fight_haptics {
	fight_haptics::Tracker tracker;
	SDL_JoystickID identities[2]{};
	bool was_blocked = true;
public:
	void stop(SDL_Gamepad *const pads[2]) {
		tracker.reset(); ios_haptics_stop();
		for (int p = 0; p < 2; ++p) if (pads[p]) SDL_RumbleGamepad(pads[p], 0, 0, 0);
	}
	void sync(SDL_Gamepad *const pads[2], bool blocked) {
		bool changed = false;
		for (int p = 0; p < 2; ++p) {
			const auto id = pads[p] ? SDL_GetGamepadID(pads[p]) : 0;
			changed |= id != identities[p]; identities[p] = id;
		}
		if (changed || blocked != was_blocked) stop(pads);
		if ((changed || blocked != was_blocked) && !blocked) ios_haptics_prepare();
		was_blocked = blocked;
	}
	void play(SDL_Gamepad *const pads[2], int player, const fight_haptics::Pulse &pulse, unsigned selected) {
		if (!pulse.ms || (pulse.low <= 0 && pulse.high <= 0)) return;
		bool supported = pads[player] && SDL_GetBooleanProperty(SDL_GetGamepadProperties(pads[player]), SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
		if (supported && SDL_RumbleGamepad(pads[player], Uint16(pulse.low * 65535), Uint16(pulse.high * 65535), pulse.ms)) return;
		// A shared phone follows P2 only when P2 alone is selected; otherwise P1.
		if (player == (selected == 2 ? 1 : 0)) ios_haptics_play(pulse.low, pulse.high, pulse.ms);
	}
	void frame(const fight_haptics::Snapshot &s, SDL_Gamepad *const pads[2], bool enabled, bool blocked, unsigned selected, int strength) {
		sync(pads, blocked || !enabled);
		if (blocked || !enabled) return;
		const auto pulses = tracker.advance(s, selected, strength);
		for (int p = 0; p < 2; ++p) play(pads, p, pulses[p], selected);
		if (!s.live) stop(pads);
	}
	void test(SDL_Gamepad *const pads[2], unsigned selected, int strength) {
		stop(pads);
		const float gain = std::clamp(strength, 0, 100) / 100.0f;
		for (int p = 0; p < 2; ++p) if (selected & (1u << p)) play(pads, p, {0.6f * gain, 0.8f * gain, 100, false}, selected);
	}
};
