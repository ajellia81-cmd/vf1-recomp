// Minimal PNG writer (RGB, or RGBA from ARGB; zlib from the system).
#pragma once
#include <cstdint>
#include <cstdio>
#include <vector>
#include <zlib.h>

inline bool write_png(const char *path, const uint32_t *xrgb, int w, int h, bool alpha = false)
{
	std::vector<uint8_t> raw;
	raw.reserve(size_t(h) * (w * (alpha ? 4 : 3) + 1));
	for (int y = 0; y < h; y++) {
		raw.push_back(0);
		for (int x = 0; x < w; x++) {
			uint32_t p = xrgb[size_t(y) * w + x];
			raw.push_back(p >> 16); raw.push_back(p >> 8); raw.push_back(p);
			if (alpha) raw.push_back(p >> 24);
		}
	}
	uLongf zlen = compressBound(raw.size());
	std::vector<uint8_t> z(zlen);
	compress2(z.data(), &zlen, raw.data(), raw.size(), 6);
	FILE *f = fopen(path, "wb");
	if (!f) return false;
	auto be32 = [&](uint32_t v) { uint8_t b[4] = {uint8_t(v >> 24), uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v)}; fwrite(b, 1, 4, f); };
	auto chunk = [&](const char *type, const uint8_t *data, uint32_t len) {
		be32(len);
		uLong crc = crc32(0, (const Bytef *)type, 4);
		fwrite(type, 1, 4, f);
		if (len) { fwrite(data, 1, len, f); crc = crc32(crc, data, len); }
		be32(uint32_t(crc));
	};
	static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
	fwrite(sig, 1, 8, f);
	uint8_t ihdr[13] = {uint8_t(w >> 24), uint8_t(w >> 16), uint8_t(w >> 8), uint8_t(w),
						uint8_t(h >> 24), uint8_t(h >> 16), uint8_t(h >> 8), uint8_t(h), 8, uint8_t(alpha ? 6 : 2), 0, 0, 0};
	chunk("IHDR", ihdr, 13);
	chunk("IDAT", z.data(), uint32_t(zlen));
	chunk("IEND", nullptr, 0);
	fclose(f);
	return true;
}
