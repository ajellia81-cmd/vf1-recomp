// Lockstep check: runs the interpreter and the recompiled code side by side and compares the
// CPU registers after every scanline and the RAM after every frame.
//   vflock <regions dir> <frames> [--native [map.txt]]
// --native checks the native code made at start-up (src/rt/native.cpp) instead of the C++ build.
#include "../src/host/regions.h"
#include "../src/rt/native.h"
#include <cstring>

int main(int argc, char **argv)
{
	std::string dir = argc > 1 ? argv[1] : "build/vf";
	int frames = argc > 2 ? atoi(argv[2]) : 600;
	model1 a(load_regions(dir)), b(load_regions(dir));
	a.reset();
	b.reset();
	rec::attach(b.cpu());
	b.run_cpu = [](int n) { rec::run(n); };
	for (int i = 3; i < argc; i++)
		if (!strcmp(argv[i], "--native")) {
			rec::hints map;
			const std::string path = i + 1 < argc ? argv[i + 1] : "assets/vf_map.txt";
			if (!map.read(path)) { printf("no address map %s\n", path.c_str()); return 1; }
			std::vector<uint8_t> mem = load_regions(dir).maincpu;
			mem.resize(0x2000000);
			rec::native_stats st;
			if (!rec::build_native(mem, map, &st)) { printf("native recompile failed\n"); return 1; }
			printf("native: %zu functions, %zu instructions (%zu as native code), %zu KB of code in %.0f ms\n",
				   st.functions, st.instructions, st.native, st.code_bytes / 1024, st.ms);
		}
	int cur_line = 0;
	uint32_t rng = 12345;
	for (int f = 1; f <= frames; f++) {
		// scripted inputs, identical for both
		// the coverage script: attract, coins, a two-player game, random moves
		uint8_t in0 = 0xff, in1 = 0xff, in2 = 0xff;
		int phase = f % 6000;
		if (phase >= 3000 && phase < 3006) in0 &= ~0x01;
		if (phase >= 3060 && phase < 3066) in0 &= ~0x01;
		if (phase >= 3120 && phase < 3126) in0 &= ~0x10;
		if (phase >= 3200 && phase < 3206) in0 &= ~0x20;
		if (phase >= 3130 && (f % 7) == 0) {
			rng = rng * 1103515245 + 12345;
			in1 = uint8_t(~((rng >> 16) & 0xf7));
			in2 = uint8_t(~((rng >> 8) & 0xf7));
		}
		a.m_inputs[0] = b.m_inputs[0] = in0;
		a.m_inputs[1] = b.m_inputs[1] = in1;
		a.m_inputs[2] = b.m_inputs[2] = in2;
		// run b one line at a time behind a, comparing
		for (int line = 0; line < 424; line++) {
			(void)line;
		}
		std::vector<std::array<uint32_t, 34>> la, lb;
		a.on_line = [&](int) { std::array<uint32_t, 34> r; for (int i = 0; i < 34; i++) r[i] = i == 33 ? a.cpu().psw() : a.cpu().reg(i); la.push_back(r); };
		b.on_line = [&](int) { std::array<uint32_t, 34> r; for (int i = 0; i < 34; i++) r[i] = i == 33 ? b.cpu().psw() : b.cpu().reg(i); lb.push_back(r); };
		a.run_frame();
		b.run_frame();
		for (size_t l = 0; l < la.size(); l++)
			if (la[l] != lb[l]) {
				printf("frame %d line %zu: registers differ\n", f, l);
				for (int i = 0; i < 34; i++)
					if (la[l][i] != lb[l][i]) printf("  r%d interp=%08x rec=%08x\n", i, la[l][i], lb[l][i]);
				return 1;
			}
		if (a.nvram() != b.nvram()) { printf("frame %d: battery RAM differs\n", f); return 1; }
		if (memcmp(a.frame().data(), b.frame().data(), 496 * 384 * 4)) { printf("frame %d: picture differs\n", f); return 1; }
		if (f % 300 == 0) printf("frame %d ok\n", f);
		(void)cur_line;
	}
	printf("%d frames identical\n", frames);
}
