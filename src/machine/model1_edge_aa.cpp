// Antialiasing's Low setting: the 3D's edges smoothed once it's drawn, at the render scale (FXAA's
// idea without its search along an edge): where a pixel stands out from its four neighbours, it is
// blended toward the neighbour across the edge, by how much it stands out. Only pixels of the 3D
// and those next to it change, so the 2D stays as drawn; beside the 3D the picture beneath it is
// what it blends with: drawn whole, the picture before the 3D went on; in the GPU's parts, the 2D
// beneath as the GPU will put it together (parts_below), and a pixel that had nothing in 3D
// takes the blend as its own. Drawn whole, the pixels under the 3D are marked (UNDER_3D) before it
// goes on, so the 3D's own pixels are known as they are in parts: the same picture either way.
#include "model1.h"
#include "parallel.h"
#include <algorithm>
#include <cmath>
#include <cstring>

void model1::edge_smooth(bitmap_rgb32 &bitmap, bool parts)
{
	const int w = bitmap.width(), h = bitmap.height();
	int y0 = 0, y1 = h - 1;
	if (parts) {
		if (m_d3_y1 < m_d3_y0) return;
		y0 = std::max(0, m_d3_y0 - 1);
		y1 = std::min(h - 1, m_d3_y1 + 1);
	}
	// the picture is read as the 3D left it; the blends are kept aside (each band's own list) and
	// written once all is read
	auto orig = [&](int x, int y) { return bitmap.pix(y, x); };
	constexpr int BANDS = 64;
	if (m_aa_changes.size() != BANDS) m_aa_changes.resize(BANDS);
	for (auto &c : m_aa_changes) c.clear();
	const int rows = y1 - y0 + 1, per_band = (rows + BANDS - 1) / BANDS;
	// a pixel the 3D drew: in parts, anything but KEY; drawn whole, one that lost UNDER_3D
	auto drawn = [&](int x, int y) {
		const uint32_t c = orig(x, y);
		return parts ? c != KEY : !(c & UNDER_3D);
	};
	// a pixel's colour in the picture (beside the 3D: what's beneath)
	auto colour = [&](int x, int y) -> uint32_t {
		const uint32_t c = orig(x, y);
		if (parts && c == KEY) return parts_below(x, y) & 0xffffff;
		return c & 0xffffff;
	};
	// (drawn whole: the marks come off every pixel the 3D didn't draw, once read)
	auto unmark = [&] {
		if (parts) return;
		parallel_rows(h, [&](int a, int b) {
			for (int y = a; y < b; y++) { uint32_t *p = &bitmap.pix(y); for (int x = 0; x < w; x++) p[x] &= 0xffffff; }
		});
	};
	auto luma = [](uint32_t c) { return int((c >> 16) & 0xff) * 77 + int((c >> 8) & 0xff) * 150 + int(c & 0xff) * 29; };   // (x 256)
	constexpr int THRESHOLD = 32 * 256;          // less contrast than this isn't an edge
	parallel_rows(BANDS, [&](int ba, int bb) {
	for (int band = ba; band < bb; band++) {
		auto &changes = m_aa_changes[band];
		for (int y = y0 + band * per_band; y < std::min(y0 + (band + 1) * per_band, y1 + 1); y++) {
			if (y == 0 || y == h - 1) continue;
			const uint32_t *rm = &bitmap.pix(y), *rn = rm - w, *rs = rm + w;
			// most pixels are the same as their four neighbours (inside a polygon or the 2D): those
			// found first, a whole row at once (the compiler's vector instructions), 8 skipped at a time
			thread_local std::vector<uint8_t> differs;
			differs.assign(size_t(w) + 8, 0);
			uint8_t *dfr = differs.data();
			for (int x = 1; x < w - 1; x++) {
				const uint32_t m0 = rm[x];
				dfr[x] = ((rn[x] ^ m0) | (rs[x] ^ m0) | (rm[x - 1] ^ m0) | (rm[x + 1] ^ m0)) != 0;
			}
			for (int x = 1; x < w - 1; x++) {
				if (!dfr[x]) {
					uint64_t eight;
					while (x + 8 < w - 1 && (memcpy(&eight, dfr + x, 8), eight == 0)) x += 8;
					if (!dfr[x]) continue;
				}
				if (!drawn(x, y) && !drawn(x - 1, y) && !drawn(x + 1, y) && !drawn(x, y - 1) && !drawn(x, y + 1)) continue;
				const uint32_t cm = colour(x, y), cn = colour(x, y - 1), cs = colour(x, y + 1), cw = colour(x - 1, y), ce = colour(x + 1, y);
				const int lm = luma(cm), ln = luma(cn), ls = luma(cs), lw = luma(cw), le = luma(ce);
				const int lo = std::min({ lm, ln, ls, lw, le }), hi = std::max({ lm, ln, ls, lw, le });
				if (hi - lo < THRESHOLD) continue;
				// the edge's direction, and the side across it that differs most
				const bool across_v = std::abs(ln + ls - 2 * lm) >= std::abs(lw + le - 2 * lm);
				const uint32_t c1 = across_v ? cn : cw, c2 = across_v ? cs : ce;
				const int l1 = across_v ? ln : lw, l2 = across_v ? ls : le;
				const uint32_t other = std::abs(l1 - lm) >= std::abs(l2 - lm) ? c1 : c2;
				// how much the pixel stands out from its neighbours' average (FXAA's subpixel amount)
				const double sub = std::min(1.0, std::abs((ln + ls + lw + le) / 4.0 - lm) / double(hi - lo));
				const double s = sub * sub * (3 - 2 * sub);
				const int k = int(std::lround(s * s * 0.75 * 0.5 * 256));   // up to 3/8 of the way across
				if (k == 0) continue;
				auto mix = [&](int sh) { const int p = (cm >> sh) & 0xff, q = (other >> sh) & 0xff; return uint32_t(p + (((q - p) * k + 128) >> 8)) << sh; };
				changes.push_back({ uint32_t(y) * uint32_t(w) + uint32_t(x), mix(16) | mix(8) | mix(0) });
			}
		}
	}
	});
	uint32_t *px = &bitmap.pix(0);
	for (const auto &c : m_aa_changes)
		for (const auto &k : c) px[k.first] = k.second;
	unmark();
}
