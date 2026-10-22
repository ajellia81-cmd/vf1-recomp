#pragma once
#include <SDL3/SDL.h>
#include "../machine/parallel.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

class screen_filter {
public:
	explicit screen_filter(SDL_Renderer *renderer) : m_renderer(renderer) {}
	~screen_filter() { clear(); }
	void clear() { if (m_texture) SDL_DestroyTexture(m_texture); m_texture = nullptr; m_w = m_h = m_kind = 0; }
	bool draw(int kind) {
		SDL_FRect r;
		if (!kind || !SDL_GetRenderLogicalPresentationRect(m_renderer, &r)) return true;
		const int w = int(std::lround(r.w)), h = int(std::lround(r.h));
		if (w < 1 || h < 1) return true;
		if (!m_texture || w != m_w || h != m_h || kind != m_kind) {
			if (m_texture) SDL_DestroyTexture(m_texture);
			m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
			if (!m_texture) return false;
			m_w = w; m_h = h; m_kind = kind;
			const bool crt = kind == 2;
			// the lines want a couple of pixels each to show: they fade in from 1.5 to 3 per line
			const float per_line = h / 384.0f;
			const float depth = (crt ? 0.45f : 0.6f) * std::clamp((per_line - 1.5f) / 1.5f, 0.0f, 1.0f);
			// The lines are a whole number of screen pixels apart: the picture's lines to the nearest,
			// so every line looks the same. Sampled at a fraction of a line apart (the Steam Deck's
			// 800-pixel screen: 2.08 pixels a line), the pattern beat into uneven bands every dozen
			// lines. At a whole number of pixels a line this is the picture's own lines, as before;
			// two pixels a line: a bright row and a dark one.
			const int period = std::max(1, int(std::lround(per_line)));
			std::vector<float> row(h);
			for (int y = 0; y < h; y++) {
				const float f = period == 2 ? float(y % 2) * 0.5f : (float(y % period) + 0.5f) / float(period);
				row[y] = 1 - depth * (1 - std::sin(f * 3.14159265f));   // 1 mid-line, darker at its edges
			}
			auto m_texture_at = [&](int x, int y, float rgb[3]) {
				rgb[0] = rgb[1] = rgb[2] = row[y];
				if (!crt) return;
				for (int c = 0; c < 3; c++) if (c != x % 3) rgb[c] *= 0.78f;   // the grille: one colour per pixel column
				const float dx = (x + 0.5f) / w * 2 - 1, dy = (y + 0.5f) / h * 2 - 1;
				const float d = std::min(1.0f, (dx * dx + dy * dy) / 2);
				for (int c = 0; c < 3; c++) rgb[c] *= 1 - 0.22f * d * d;      // the corners
			};
			// Multiplying only darkens: drawn as twice the product instead (dst * m_texture * 2), the m_texture
			// also brightens the middle of each line, so the picture keeps about its brightness.
			const SDL_BlendMode times2 = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_DST_COLOR, SDL_BLENDFACTOR_SRC_COLOR,
				SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
			const bool doubled = SDL_SetTextureBlendMode(m_texture, times2);
			if (!doubled) SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_MOD);   // a renderer without it: darker
			float gain = 1;
			if (doubled) {
				double sum = 0; int n = 0;
				for (int y = 0; y < h; y++)
					for (int x = 0; x < w; x += 7) { float v[3]; m_texture_at(x, y, v); sum += v[0] + v[1] + v[2]; n += 3; }
				gain = std::min(1.4f, float(n / sum));
			}
			const float k = 255 * gain / (doubled ? 2 : 1);
			std::vector<uint32_t> px(size_t(w) * h);
			parallel_rows(h, [&](int y0, int y1) {   // on every core: a 4K window's is ~20 ms on one
				for (int y = y0; y < y1; y++)
					for (int x = 0; x < w; x++) {
						float v[3];
						m_texture_at(x, y, v);
						uint32_t c[3];
						for (int i = 0; i < 3; i++) c[i] = uint32_t(std::min(255.0f, v[i] * k + 0.5f));
						px[size_t(y) * w + x] = 0xff000000u | c[0] << 16 | c[1] << 8 | c[2];
					}
			}, 4);
			SDL_UpdateTexture(m_texture, nullptr, px.data(), w * 4);
			SDL_SetTextureScaleMode(m_texture, SDL_SCALEMODE_NEAREST);
		}
		return SDL_RenderTexture(m_renderer, m_texture, nullptr, nullptr);   // over the picture (the logical area), under the menu
	}
private:
	SDL_Renderer *m_renderer;
	SDL_Texture *m_texture = nullptr;
	int m_w = 0, m_h = 0, m_kind = 0;
};
