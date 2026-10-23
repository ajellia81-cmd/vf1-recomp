#pragma once
#include <cstdint>
// Read-only health-store metadata observed in the user's build 5 scalar trace.
// ffbb96 subtracts damage; ffbb9c clamps a negative result back to zero.
inline unsigned vf1_damage_store(uint32_t pc, uint16_t mask, int before, int after, bool live) {
 if (!live || pc != 0x00ffbb96 || mask != 0xffff) return 0;
 const int damage=before-after;
 return damage>0 && damage<=1000 ? unsigned(damage) : 0;
}
