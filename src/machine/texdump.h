// VF2_DUMP_TEXTURES=<dir>: every texture the game draws, saved as a PNG the first time it's drawn.
// The textures are only in texture RAM once the game has loaded them (each stage, each fighter),
// so they're taken from what's drawn: play, or let attract mode run, and the folder fills.
//   <dir>/color/<texels>_<colour>.png   as the game colours it (fully lit; RGBA, the clear
//                                        texels of a see-through texture transparent)
//   <dir>/raw/<texels>.png              the 16 grey levels the texture is made of
//   <dir>/index.csv                     each one's sheet, place and size, colour and luma table
#pragma once
#include "model2_video.h"
#include "../host/png.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <set>
#include <string>
#include <tuple>
#include <vector>

class texdump
{
public:
	explicit texdump(const char *dir) : m_dir(dir)
	{
		std::filesystem::create_directories(m_dir + "/color");
		std::filesystem::create_directories(m_dir + "/raw");
		m_index = fopen((m_dir + "/index.csv").c_str(), "w");
		if (m_index) fprintf(m_index, "file,raw,sheet,x,y,width,height,colour,lumabase,see_through,frame\n");
		fprintf(stderr, "textures: saving the ones drawn into %s\n", m_dir.c_str());
	}
	~texdump() { if (m_index) fclose(m_index); }

	// A frame's polygons: the textures new since the last frames saved
	void add(const std::vector<model2_video::drawn_poly> &drawn, const model2_video &v, u64 frame)
	{
		std::set<std::tuple<const u32 *, u32, u32, u32, u32, u32, u32, u8>> seen;
		for (const auto &p : drawn) {
			if (!(p.renderer & 2)) continue;   // not textured
			const m2_poly_extra_data &e = p.extra;
			const u32 pal = v.m_palram[e.colorbase + 0x1000] & 0x7fff;
			const u8 clear = p.renderer & 1;
			if (!seen.emplace(e.texsheet[0], e.texx, e.texy, e.texwidth, e.texheight, pal, e.lumabase, clear).second) continue;
			save(e, v, pal, clear, frame);
		}
	}

private:
	std::string m_dir;
	FILE *m_index = nullptr;
	std::set<u64> m_raw, m_color;
	std::vector<u8> m_tex;
	std::vector<u32> m_px;

	// The renderer's get_texel: the sheets are 2048x1024 texels, kept as 1024x2048, 4 bits each
	static u8 texel(const u32 *sheet, u32 x, u32 y)
	{
		const u32 right = x >= 1024 ? 1024u : 0u;
		const u32 x2 = x - right, y2 = y ^ right;
		const u32 offset = ((y2 >> 1) << 9) + (x2 >> 1);
		const u32 shift = ((offset & 1) << 4) + ((~y & 1) << 3) + ((~x & 1) << 2);
		return (sheet[offset >> 1] >> shift) & 0x0f;
	}

	static u64 fnv(const u8 *d, size_t n, u64 h = 1469598103934665603ull)
	{
		for (size_t i = 0; i < n; i++) h = (h ^ d[i]) * 1099511628211ull;
		return h;
	}

	void save(const m2_poly_extra_data &e, const model2_video &v, u32 pal, u8 clear, u64 frame)
	{
		const u32 w = e.texwidth, h = e.texheight, x0 = e.texx & 2047, y0 = e.texy & 1023;
		m_tex.resize(size_t(w) * h);
		for (u32 y = 0; y < h; y++)
			for (u32 x = 0; x < w; x++)
				m_tex[size_t(y) * w + x] = texel(e.texsheet[0], (x0 + x) & 2047, (y0 + y) & 1023);
		u64 key = fnv(m_tex.data(), m_tex.size());
		key = fnv(reinterpret_cast<const u8 *>(&w), 4, fnv(reinterpret_cast<const u8 *>(&h), 4, key));
		if (std::all_of(m_tex.begin(), m_tex.end(), [](u8 t) { return t == 0; })) return;   // not loaded yet

		char raw[32], color[64];
		snprintf(raw, sizeof raw, "%016llx", (unsigned long long)key);
		m_px.resize(m_tex.size());
		if (m_raw.insert(key).second) {
			for (size_t i = 0; i < m_tex.size(); i++) m_px[i] = 0xff000000u | (m_tex[i] * 0x111111u);
			write_png((m_dir + "/raw/" + raw + ".png").c_str(), m_px.data(), int(w), int(h));
		}

		// draw_scanline_tex at full light: the texel picks a luma from the polygon's table, the
		// luma and the polygon's colour an entry in the colour tables, then the gamma
		const u64 ckey = fnv(reinterpret_cast<const u8 *>(&pal), 4, fnv(reinterpret_cast<const u8 *>(&e.lumabase), 4, key ^ clear));
		if (!m_color.insert(ckey).second) return;
		const u16 *cr = &v.m_colorxlat[0x0000 / 2 + ((pal >> 0) & 0x1f) * 0x100];
		const u16 *cg = &v.m_colorxlat[0x4000 / 2 + ((pal >> 5) & 0x1f) * 0x100];
		const u16 *cb = &v.m_colorxlat[0x8000 / 2 + ((pal >> 10) & 0x1f) * 0x100];
		for (size_t i = 0; i < m_tex.size(); i++) {
			const u8 t = m_tex[i];
			if (clear && t == 0x0f) { m_px[i] = 0; continue; }
			const u8 luma = std::min<u8>(v.m_lumaram[(e.lumabase + t * 8) & 0x7fff], 0x3f);
			m_px[i] = 0xff000000u | u32(v.m_gamma_table[cr[luma] & 0xff]) << 16 |
				u32(v.m_gamma_table[cg[luma] & 0xff]) << 8 | v.m_gamma_table[cb[luma] & 0xff];
		}
		snprintf(color, sizeof color, "%s_%04x%04x", raw, pal, e.lumabase >> 7);
		write_png((m_dir + "/color/" + color + ".png").c_str(), m_px.data(), int(w), int(h), true);
		if (m_index) {
			fprintf(m_index, "color/%s.png,raw/%s.png,%d,%u,%u,%u,%u,%04x,%u,%d,%llu\n", color, raw,
				e.texsheet[0] == v.m_textureram0 ? 0 : 1, x0, y0, w, h, pal, e.lumabase >> 7, clear, (unsigned long long)frame);
			fflush(m_index);
		}
	}
};
