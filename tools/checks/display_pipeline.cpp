// Actual SDL/Metal readback with a synthetic test picture, no ROM data.
#include "../../src/host/gpu_compose.h"
#include "../../src/host/output_picture.h"
#include "../../src/host/screen_filter.h"
#include <cassert>
#include <cstdio>
static uint32_t pixel(SDL_Renderer*r,int x,int y){
 auto *a=SDL_RenderReadPixels(r,nullptr);assert(a);
 auto *b=SDL_ConvertSurface(a,SDL_PIXELFORMAT_ARGB8888);assert(b);
 uint32_t v=*reinterpret_cast<uint32_t*>(static_cast<unsigned char*>(b->pixels)+y*b->pitch+4*x);
 SDL_DestroySurface(b);SDL_DestroySurface(a);return v&0xffffff;
}
int main(){
 assert(SDL_Init(SDL_INIT_VIDEO));
 auto*w=SDL_CreateWindow("Synthetic display check",96,1320,SDL_WINDOW_HIDDEN);assert(w);
 auto*r=SDL_CreateRenderer(w,"metal");assert(r);
 gpu_compose compose;assert(compose.init(r));std::string why;assert(compose.self_check(why));
 auto*t=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STATIC,96,1320);assert(t);
 std::vector<uint32_t> data(96*1320,0xff404040);assert(SDL_UpdateTexture(t,nullptr,data.data(),96*4));
 output_picture output(r);screen_filter filter(r);
 for(int gain:{100,150,200,100}){
  assert(prepare_game_view(r,96,1320));assert(draw_game_picture(r,t,gain));
  const unsigned c=unsigned(64*gain/100);assert(pixel(r,40,40)==c*0x010101);
 }

 // A saved fixed shape letterboxes; only the iPhone choice fills any drawable.
 for (bool native : {false, true, false, true}) {
  SDL_Renderer *view = r;
  assert(prepare_game_view(view, 512, 288, native));
  int width=0,height=0; assert(SDL_GetRenderOutputSize(view,&width,&height));
  SDL_FRect rect{}; assert(SDL_GetRenderLogicalPresentationRect(view,&rect));
  if(native) { assert(rect.x==0 && rect.y==0 && rect.w==width && rect.h==height); }
  else { assert(rect.w < width || rect.h < height); }
 }
 SDL_SetRenderScale(r,4,4);SDL_Rect clip{0,0,8,8};SDL_SetRenderViewport(r,&clip);SDL_SetRenderClipRect(r,&clip);
 auto*resolved=output.resolve(t,true,2868.0f/1320);assert(resolved!=t);
 assert(SDL_SetRenderTarget(r,resolved));assert(pixel(r,2867,1319)==0x404040);
 assert(prepare_game_view(r,96,1320));assert(draw_game_picture(r,t,100));
 filter.draw(1);assert(pixel(r,40,0)!=pixel(r,40,1));
 assert(draw_game_picture(r,t,100));filter.draw(2);assert(pixel(r,40,40)!=pixel(r,41,40));
 assert(draw_game_picture(r,t,100));filter.draw(0);assert(pixel(r,40,40)==0x404040);
 assert(prepare_overlay_view(r));float x,y;SDL_GetRenderScale(r,&x,&y);assert(x==1&&y==1&&!SDL_RenderClipEnabled(r));
 filter.clear();output.clear();compose.release();SDL_DestroyTexture(t);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();
 std::puts("PASS: composition, brightness, native output target, stale coordinates and screen filter pixels");
}
