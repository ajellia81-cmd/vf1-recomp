#include "../../src/host/vf1_damage_events.h"
#include "../../src/host/fight_haptics.h"
#include <cassert>
#include <cstdio>
int main() {
 using namespace fight_haptics;
 assert(vf1_damage_store(0xffbb96,0xffff,22,-8,true)==30);
 assert(vf1_damage_store(0xffbb96,0xffff,0,-30,true)==30);
 assert(!vf1_damage_store(0xffbb9c,0xffff,-30,0,true)); // clamp is not a second hit
 assert(!vf1_damage_store(0xfeac05,0xffff,200,40,true)); // setup is not damage
 assert(!vf1_damage_store(0xffbb96,0xffff,30,0,false)); // attract/replay gate
 assert(!vf1_damage_store(0xffbb96,0xff,30,0,true)); // partial stores are not accepted
 assert(!vf1_damage_store(0xffbb96,0xffff,0,0,true));
 for(int victim=0;victim<2;++victim) {
  Tracker t; Snapshot s{1,10,true,{176,176},176,3};s.damage_events=true;s.health[victim]=22;
  assert(!t.advance(s,3,75)[0].ms);
  ++s.frame;s.damage_total[victim]+=vf1_damage_store(0xffbb96,0xffff,22,-8,true);s.health[victim]=0;
  auto ko=t.advance(s,3,75);assert(ko[0].knockout && ko[1].knockout);
  // Capture the subtraction before the game clamps 0 -> -30 -> 0 within this frame.
  ++s.frame;s.damage_total[victim]+=vf1_damage_store(0xffbb96,0xffff,0,-30,true);
  s.damage_total[victim]+=vf1_damage_store(0xffbb9c,0xffff,-30,0,true);
  auto post=t.advance(s,3,75);assert(post[0].ms && post[1].ms && !post[0].knockout && !post[1].knockout);
  assert(post[0].ms<ko[0].ms && post[1].ms<ko[1].ms);
  assert(!t.advance(s,3,75)[0].ms); // repeated display frame
  ++s.frame;assert(!t.advance(s,3,75)[0].ms); // no subtraction, no phantom contact
  ++s.frame;s.damage_total[victim]+=30;auto p1=t.advance(s,1,75);assert(p1[0].ms && !p1[1].ms);
  ++s.frame;s.live=false;s.damage_total[victim]+=30;assert(!t.advance(s,3,75)[0].ms);
  ++s.frame;s.live=true;assert(!t.advance(s,3,75)[0].ms); // return baseline
  ++s.frame;s.damage_total[victim]=0;assert(!t.advance(s,3,75)[0].ms); // reset, no unsigned underflow
  ++s.frame;s.damage_total[victim]+=30;s.epoch++;assert(!t.advance(s,3,75)[0].ms);
 }
 // A health decrease plus a matching event is one hit, not two summed strengths.
 Tracker a,b;Snapshot s{1,0,true,{176,176},176,3};a.advance(s,3,75);
 s.damage_events=true;b.advance(s,3,75);++s.frame;s.health[1]-=4;s.damage_total[1]=4;
 auto event=b.advance(s,3,75);s.damage_events=false;auto hp=a.advance(s,3,75);
 assert(event[0].low==hp[0].low && event[1].high==hp[1].high && event[1].ms==hp[1].ms);
 puts("PASS: pre-clamp KO/post-KO contacts, both sides, no duplicate/clamp/setup/replay/stale events");
}
