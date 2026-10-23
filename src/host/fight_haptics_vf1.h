#pragma once
#include "fight_haptics.h"
#include <vector>
// Read-only addresses documented by model1::fight_cheats and vfscript.
inline fight_haptics::Snapshot vf1_haptic_snapshot(const std::vector<uint8_t> &nv, const std::vector<uint8_t> &work, uint64_t frame) {
	fight_haptics::Snapshot s; s.frame = frame;
	if (nv.size() < 0x7600 || work.size() < 0x1310) return s;
	auto byte = [&](uint32_t a) -> uint32_t {
		if (a >= 0x400000 && a - 0x400000 < nv.size()) return nv[a - 0x400000];
		if (a >= 0x500000 && a - 0x500000 < work.size()) return work[a - 0x500000];
		return 0;
	};
	auto word = [&](uint32_t a) { return byte(a) | (byte(a + 1) << 8); };
	const uint32_t base = word(0x50130c) | (word(0x50130e) << 16);
	if (base < 0x400080 || base > 0x410000 - 0x1800) return s;
	s.epoch = base | (uint64_t(work[0x1281] | (work[0x1282] << 8)) << 32);
	s.maximum = word(0x501186);
	s.health[0] = int16_t(word(base - 0x2e)); s.health[1] = int16_t(word(base + 0x1800 - 0x2e));
	s.players = ((nv[0x7483] & 8) ? 1u : 0u) | ((nv[0x7503] & 8) ? 2u : 0u);
	s.live = s.players && (word(0x400182) & 0x8000) && (word(0x400202) & 0x8000);
	return s;
}
