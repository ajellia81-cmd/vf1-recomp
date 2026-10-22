// Synthetic flat polygon, no game/ROM data.
#include "../../src/gpu/gpu3d.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
int main(){
 const int w=2868,h=1320;
 const size_t bytes=(size_t(w)*h*4+16383)&~size_t(16383);
 auto px=static_cast<uint32_t*>(std::aligned_alloc(16384,bytes)); assert(px);
 std::fill_n(px,size_t(w)*h,0xff000000);
 auto gpu=gpu3d_create_metal(); assert(gpu);
 gpu3d_poly p{};
 p.x[0]=101;p.x[1]=1501;p.x[2]=1501;p.x[3]=101;
 p.y[0]=71;p.y[1]=71;p.y[2]=901;p.y[3]=901;
 p.cx0=0;p.cx1=w-1;p.cy0=0;p.cy1=h-1;p.y0=71;p.rows=830;
 p.flags=G3_ROWS;p.colour=0x004080c0;
 assert(gpu->draw(&p,1,nullptr,0,2*p.rows,px,bytes,w,h,0,h-1));
 for(int y=0;y<h;y++)for(int x=0;x<w;x++)
  assert(px[size_t(y)*w+x]==((x>=101&&x<1501&&y>=71&&y<901)?p.colour:0xff000000));
 gpu.reset();std::free(px);
 std::puts("PASS: Model 1 Metal synthetic polygon edges and color at 2868x1320");
}
