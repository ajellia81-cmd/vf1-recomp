#include "../../src/host/fight_haptics_vf1.h"
#include <cassert>
int main() {
 std::vector<uint8_t> nv(0x10000), w(0x10000);
 auto put=[](auto &v,unsigned a,unsigned x) { v[a]=x; v[a+1]=x>>8; };
 put(w,0x130c,0x0700); put(w,0x130e,0x0040); put(w,0x1186,192);
 put(nv,0x700-0x2e,180); put(nv,0x1f00-0x2e,170);
 put(nv,0x182,0x8000); put(nv,0x202,0x8000); nv[0x7483]=8;
 auto s=vf1_haptic_snapshot(nv,w,12);
 assert(s.live && s.frame==12 && s.health[0]==180 && s.health[1]==170 && s.maximum==192 && s.players==1);
 nv[0x7503]=8; assert(vf1_haptic_snapshot(nv,w,13).players==3);
 nv[0x183]=0; assert(!vf1_haptic_snapshot(nv,w,14).live); // replay gate
 nv[0x183]=0x80; nv[0x7483]=nv[0x7503]=0; assert(!vf1_haptic_snapshot(nv,w,15).live); // attract
 put(w,0x130c,0xffff); put(w,0x130e,0xffff); assert(!vf1_haptic_snapshot(nv,w,16).live);
 assert(!vf1_haptic_snapshot({},w,17).live);
}
