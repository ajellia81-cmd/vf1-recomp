#pragma once
#include <SDL3/SDL.h>
#include <algorithm>

// Modulate only the completed game picture. 100% preserves its original RGB;
// values above 100% multiply RGB before SDR clipping and keep black at black.
inline bool draw_game_picture(SDL_Renderer *renderer, SDL_Texture *picture, int brightness)
{
	if (!picture) return false;
	const float gain = std::clamp(brightness, 50, 200) / 100.0f;
	if (!SDL_SetTextureColorModFloat(picture, gain, gain, gain)) return false;
	const bool ok = SDL_RenderTexture(renderer, picture, nullptr, nullptr);
	SDL_SetTextureColorModFloat(picture, 1, 1, 1);
	return ok;
}

// Render targets and UI overlays have independent pixel/point coordinate spaces.
// Re-establish all window state instead of inheriting a target's scale or scissor.
inline bool prepare_game_view(SDL_Renderer *renderer, int width, int height)
{
	return SDL_SetRenderTarget(renderer, nullptr)
		&& SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX)
		&& SDL_SetRenderScale(renderer, 1, 1)
		&& SDL_SetRenderViewport(renderer, nullptr)
		&& SDL_SetRenderClipRect(renderer, nullptr);
}
inline bool prepare_overlay_view(SDL_Renderer *renderer)
{
	return SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED)
		&& SDL_SetRenderScale(renderer, 1, 1)
		&& SDL_SetRenderViewport(renderer, nullptr)
		&& SDL_SetRenderClipRect(renderer, nullptr);
}
