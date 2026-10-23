#include "../../src/host/fight_haptics.h"
#include <cassert>
#include <cstdio>
int main() {
	using namespace fight_haptics;
	Tracker tracker; Snapshot s{1, 10, true, {192, 192}, 192, 3};
	assert(!tracker.advance(s, 3, 75)[0].ms);
	s.frame++; s.health[1] -= 4; auto small = tracker.advance(s, 3, 75);
	assert(small[0].ms && small[1].ms && small[0].high > small[0].low && small[1].low > small[1].high);
	assert(!tracker.advance(s, 3, 75)[0].ms); // 120 Hz display duplicates cannot emit a second impact.
	s.frame++; s.health[1] -= 30; auto large = tracker.advance(s, 3, 75);
	assert(large[0].low > small[0].low && large[1].ms > small[1].ms);
	s.frame++; s.health[1] = 0; auto ko = tracker.advance(s, 3, 75);
	assert(ko[0].knockout && ko[1].knockout && ko[1].ms > large[1].ms);
	s.frame++; assert(!tracker.advance(s, 3, 75)[0].ms);
	s.frame++; s.health[0] = s.health[1] = 192; assert(!tracker.advance(s, 3, 75)[0].ms);
	s.frame++; s.health[0] -= 8; auto one = tracker.advance(s, 2, 100); assert(!one[0].ms && one[1].ms);
	s.frame++; s.health[1] -= 8; auto muted = tracker.advance(s, 3, 0); assert(muted[0].low == 0 && muted[1].high == 0);
	s.frame++; s.live = false; assert(!tracker.advance(s, 3, 100)[0].ms);
	s.frame++; s.live = true; s.health[0] -= 10; assert(!tracker.advance(s, 3, 100)[0].ms);
	s.frame += 3; s.health[0] -= 10; assert(!tracker.advance(s, 3, 100)[0].ms);
	s.frame++; s.epoch++; s.health[0] -= 10; assert(!tracker.advance(s, 3, 100)[0].ms);
	s.frame++; s.health[1] = -1; assert(!tracker.advance(s, 3, 100)[0].ms);
	puts("PASS: continuous damage response, KO tail, player routing, duplicate-frame/replay/reset/invalid-data suppression");
}
