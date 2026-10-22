#pragma once
#include "display_color.h"
#include <cmath>

// Resolve a supersampled integer-scale picture into a fixed presentation target.
// Geometry remains on Model 1's original integer raster grid; this is not a
// fractional/native 3D rasterizer. The window/UI state is rebuilt afterwards.
class output_picture {
 SDL_Renderer *ren;
 SDL_Texture *target = nullptr;
public:
 explicit output_picture(SDL_Renderer *r):ren(r){}
 ~output_picture(){clear();}
 void clear(){if(target)SDL_DestroyTexture(target);target=nullptr;}
 SDL_Texture *resolve(SDL_Texture *source, bool native, float aspect){
  if(!native)return source;
  if(!target){
   target=SDL_CreateTexture(ren,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,2868,1320);
   if(!target)return source;
   SDL_SetTextureBlendMode(target,SDL_BLENDMODE_NONE);
   SDL_SetTextureScaleMode(target,SDL_SCALEMODE_LINEAR);
  }
  const bool ok=SDL_SetRenderTarget(ren,target)
   && SDL_SetRenderLogicalPresentation(ren,0,0,SDL_LOGICAL_PRESENTATION_DISABLED)
   && SDL_SetRenderScale(ren,1,1) && SDL_SetRenderViewport(ren,nullptr)
   && SDL_SetRenderClipRect(ren,nullptr)
   && SDL_SetRenderDrawColor(ren,0,0,0,255) && SDL_RenderClear(ren)
   && SDL_RenderTexture(ren,source,nullptr,nullptr);
  prepare_game_view(ren,int(std::lround(384*aspect)),384);
  return ok?target:source;
 }
};
