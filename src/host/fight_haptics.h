#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace fight_haptics {
struct Snapshot {
	uint64_t frame = 0, epoch = 0;
	bool live = false;
	int health[2] = {-1, -1}, maximum = 0;
	unsigned players = 3;
};
struct Pulse {
	float low = 0, high = 0;
	unsigned ms = 0;
	bool knockout = false;
};
inline void merge(Pulse &a, const Pulse &b) {
	a.low = std::max(a.low, b.low); a.high = std::max(a.high, b.high);
	a.ms = std::max(a.ms, b.ms); a.knockout |= b.knockout;
}
// Read-only event extraction, once per ORIGINAL game frame, never per interpolated picture.
// Only health loss and zero-health finishers are enabled: no guessed guard/throw/counter events.
class Tracker {
	Snapshot previous{};
	bool ready = false;
public:
	void reset() { ready = false; previous = {}; }
	std::array<Pulse, 2> advance(const Snapshot &s, unsigned selected, int strength) {
		std::array<Pulse, 2> out{};
		bool valid = s.live && s.maximum > 0 && s.maximum <= 1000;
		for (int hp : s.health) valid &= hp >= 0 && hp <= s.maximum;
		if (!valid) { reset(); return out; }
		if (ready && s.frame == previous.frame) return out;
		const bool continuous = ready && s.frame == previous.frame + 1 && s.epoch == previous.epoch && s.maximum == previous.maximum;
		if (continuous && s.health[0] <= previous.health[0] && s.health[1] <= previous.health[1]) {
			const float gain = std::clamp(strength, 0, 100) / 100.0f;
			for (int victim = 0; victim < 2; ++victim) {
				const int damage = previous.health[victim] - s.health[victim];
				if (damage <= 0) continue;
				const float weight = std::sqrt(std::clamp(8.0f * damage / s.maximum, 0.0f, 1.0f));
				const bool ko = s.health[victim] == 0;
				Pulse received{(0.32f + 0.58f * weight) * gain, (0.15f + 0.50f * weight) * gain,
					unsigned(40 + 60 * weight) + (ko ? 75u : 0u), ko};
				Pulse landed{(0.12f + 0.40f * weight) * gain, (0.30f + 0.60f * weight) * gain,
					unsigned(25 + 40 * weight) + (ko ? 55u : 0u), ko};
				if (selected & s.players & (1u << victim)) merge(out[victim], received);
				if (selected & s.players & (1u << (victim ^ 1))) merge(out[victim ^ 1], landed);
			}
		}
		previous = s; ready = true;
		return out;
	}
};
} // namespace fight_haptics
