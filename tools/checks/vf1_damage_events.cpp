#include "../../src/host/vf1_damage_events.h"
#include "../../src/host/fight_haptics_vf1.h"
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
 // Identical measured damage has identical amplitude/duration before and after KO.
 for(int damage : {1,4,12,30}) for(int gain : {0,25,75,100}) for(int victim=0;victim<2;++victim) {
  Tracker normal,post;Snapshot a{1,0,true,{176,176},176,3};a.damage_events=true;
  Snapshot b=a;b.health[victim]=0;normal.advance(a,3,gain);post.advance(b,3,gain);
  ++a.frame;++b.frame;a.health[victim]-=damage;
  a.damage_total[victim]=b.damage_total[victim]=damage;
  auto n=normal.advance(a,3,gain),p=post.advance(b,3,gain);
  for(int player=0;player<2;++player) {
   assert(n[player].low==p[player].low && n[player].high==p[player].high);
   assert(n[player].ms==p[player].ms && !p[player].knockout);
  }
 }
 // A health decrease plus a matching event is one hit, not two summed strengths.
 Tracker a,b;Snapshot s{1,0,true,{176,176},176,3};a.advance(s,3,75);
 s.damage_events=true;b.advance(s,3,75);++s.frame;s.health[1]-=4;s.damage_total[1]=4;
 auto event=b.advance(s,3,75);s.damage_events=false;auto hp=a.advance(s,3,75);
 assert(event[0].low==hp[0].low && event[1].high==hp[1].high && event[1].ms==hp[1].ms);
 // Damage from a different writer (e.g. throws) must not be discarded.
 // Run through the real VF1 snapshot adapter, with one human on either side.
 for(int human=0;human<2;++human) for(int victim=0;victim<2;++victim) for(int amount:{4,20,50}) {
  std::vector<uint8_t> nv(0x10000),work(0x10000);
  auto put=[](auto &v,unsigned a,unsigned x){v[a]=uint8_t(x);v[a+1]=uint8_t(x>>8);};
  put(work,0x130c,0x0700);put(work,0x130e,0x0040);put(work,0x1186,192);
  put(nv,0x182,0x8000);put(nv,0x202,0x8000);nv[human?0x7503:0x7483]=8;
  const unsigned hp[2]={0x700-0x2e,0x1f00-0x2e};
  put(nv,hp[0],176);put(nv,hp[1],176);uint64_t totals[2]{};
  Tracker tracker,reference;
  auto snap=vf1_haptic_snapshot(nv,work,1,totals);tracker.advance(snap,3,75);reference.advance(snap,3,75);
  put(nv,hp[victim],176-amount);snap=vf1_haptic_snapshot(nv,work,2,totals);
  auto thrown=tracker.advance(snap,3,75);
  totals[victim]=amount;auto ordinary=reference.advance(vf1_haptic_snapshot(nv,work,2,totals),3,75);
  assert(thrown[human].ms && !thrown[human^1].ms);
  assert(thrown[human].low==ordinary[human].low && thrown[human].high==ordinary[human].high && thrown[human].ms==ordinary[human].ms);
  assert(!tracker.advance(snap,3,75)[human].ms);
  snap.frame=3;assert(!tracker.advance(snap,3,75)[human].ms);
 }
 // Mixed known-strike plus uncounted throw loss uses max(event, HP loss), not sum.
 Tracker mix,plain;Snapshot m{1,0,true,{176,176},176,3};m.damage_events=true;
 mix.advance(m,3,75);plain.advance(m,3,75);
 m.frame=2;m.health[1]-=12;m.damage_total[1]=4;auto mixed=mix.advance(m,3,75);
 m.damage_total[1]=12;auto combined=plain.advance(m,3,75);
 assert(mixed[0].low==combined[0].low && mixed[1].ms==combined[1].ms);
 // A throw that ends the round still gets exactly one KO tail.
 Tracker finish;Snapshot k{1,0,true,{176,10},176,3};k.damage_events=true;finish.advance(k,3,75);
 k.frame=2;k.health[1]=0;auto lethal_throw=finish.advance(k,3,75);
 assert(lethal_throw[0].knockout && lethal_throw[1].knockout);
 k.frame=3;assert(!finish.advance(k,3,75)[0].ms);
 // After the replay/round flags drop, zero-HP contacts can still come from
 // the verified subtraction PC during the human match's victory interval.
 for(int human=0;human<2;++human) for(int victim=0;victim<2;++victim) {
  std::vector<uint8_t> nv(0x10000),work(0x10000);
  auto put=[](auto &v,unsigned a,unsigned x){v[a]=uint8_t(x);v[a+1]=uint8_t(x>>8);};
  put(work,0x130c,0x0700);put(work,0x130e,0x0040);put(work,0x1186,192);
  nv[human?0x7503:0x7483]=8;
  const unsigned hp[2]={0x700-0x2e,0x1f00-0x2e};
  put(nv,hp[0],100);put(nv,hp[1],100);put(nv,hp[victim],0);
  uint64_t totals[2]{};Tracker victory;
  auto v=vf1_haptic_snapshot(nv,work,1,totals);
  assert(v.live && v.event_only);victory.advance(v,3,75);
  totals[victim]+=vf1_damage_store(0xffbb96,0xffff,0,-20,true);
  auto hit=victory.advance(vf1_haptic_snapshot(nv,work,2,totals),3,75);
  assert(hit[human].ms && !hit[human^1].ms && !hit[human].knockout);
  // An unrelated health change in this interval cannot stand in for a contact.
  put(nv,hp[victim^1],50);v=vf1_haptic_snapshot(nv,work,3,totals);
  assert(!victory.advance(v,3,75)[human].ms);
  nv[0x7483]=nv[0x7503]=0;totals[victim]+=20;
  assert(!victory.advance(vf1_haptic_snapshot(nv,work,4,totals),3,75)[human].ms);
 }
 puts("PASS: verified zero-health post-round contacts, human routing, no HP-only replay/setup feedback");
 puts("PASS: actual health-loss fallback for throws dealt/received, adapter human routing, no double counting and throw KO");
 puts("PASS: pre-clamp KO/post-KO contacts, both sides, no duplicate/clamp/setup/replay/stale events");
}
