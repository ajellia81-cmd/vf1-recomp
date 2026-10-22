#include "model1.h"

// A pixel of the picture as the GPU puts the parts together: the ops in order, each texel picked as
// the GPU picks it (the nearest to the pixel's centre), KEY left out.
uint32_t model1::compose_pixel(const gpu_parts &p, int x, int y, int upto)
{
	uint32_t c = 0;
	for (const gpu_op &o : p.ops) {
		if (o.tex == upto && upto >= 0) break;
		if (x < o.clip_x0 || x >= o.clip_x1 || x < o.dx || x >= o.dx + o.dw || y < o.dy || y >= o.dy + o.dh) continue;
		if (o.tex < 0) { c = o.colour & 0xffffff; continue; }
		const gpu_image &im = p.img[o.tex];
		if (!im.px) continue;
		const int u = int(std::floor(o.sx + (x + 0.5f - o.dx) * (o.sw / o.dw)));
		const int v = int(std::floor(o.sy + (y + 0.5f - o.dy) * (o.sh / o.dh)));
		if (u < 0 || v < 0 || u >= im.w || v >= im.h) continue;
		const uint32_t t = im.px[size_t(v) * im.stride + u];
		if (t != KEY) c = t & 0xffffff;
	}
	return c;
}

void model1::compose_cpu(const gpu_parts &p, int w, int h, std::vector<uint32_t> &out)
{
	out.resize(size_t(w) * h);
	parallel_rows(h, [&](int a, int b) {
		for (int y = a; y < b; y++)
			for (int x = 0; x < w; x++) out[size_t(y) * w + x] = compose_pixel(p, x, y);
	});
}

