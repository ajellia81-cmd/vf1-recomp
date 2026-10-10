// Coverage: runs VF on the interpreter with scripted inputs and records every executed
// instruction address, plus the call targets and indirect jumps seen. Output stays in build/.
//   vfcov <regions dir> <frames> <out.txt>
#include "../src/host/regions.h"
#include <map>
#include <set>

int main(int argc, char **argv)
{
	std::string dir = argc > 1 ? argv[1] : "build/vf";
	int frames = argc > 2 ? atoi(argv[2]) : 20000;
	const char *out = argc > 3 ? argv[3] : "build/cov.txt";
	model1 m(load_regions(dir));
	m.reset();
	std::map<uint32_t, uint64_t> pcs;
	std::map<uint8_t, uint64_t> ops;
	std::set<std::pair<uint32_t, uint32_t>> jumps;   // (instruction, target) when it set PC
	std::set<std::pair<uint32_t, uint32_t>> irqs;    // (vector, handler)
	uint32_t prev = 0xffffffff;
	bool irq_between = false;
	m.cpu().on_irq = [&](int v, uint32_t, uint32_t to) { irqs.insert({uint32_t(v), to}); irq_between = true; };
	m.on_instruction = [&](uint32_t pc) {
		if (prev != 0xffffffff && !irq_between) {
			// sequential if pc follows prev; the interpreter told us nothing about length, so
			// record every change of flow and let the recompiler drop the sequential ones
			jumps.insert({prev, pc});
		}
		irq_between = false;
		prev = pc;
		pcs[pc]++;
		ops[m.read8(pc)]++;
	};
	// Script: attract for a while, then coin + start, then mash random inputs through fights.
	uint32_t rng = getenv("VF_SEED") ? uint32_t(atoi(getenv("VF_SEED"))) : 12345;
	for (int f = 1; f <= frames; f++) {
		uint8_t in0 = 0xff, in1 = 0xff, in2 = 0xff;
		int phase = f % 6000;
		if (phase >= 3000 && phase < 3006) in0 &= ~0x01;            // coin
		if (phase >= 3060 && phase < 3066) in0 &= ~0x01;            // coin
		if (phase >= 3120 && phase < 3126) in0 &= ~0x10;            // start 1
		if (phase >= 3200 && phase < 3206) in0 &= ~0x20;            // start 2 (versus)
		if (phase >= 3130 && (f % 7) == 0) {
			rng = rng * 1103515245 + 12345;
			in1 = uint8_t(~((rng >> 16) & 0xf7));
			in2 = uint8_t(~((rng >> 8) & 0xf7));
		}
		m.m_inputs[0] = in0; m.m_inputs[1] = in1; m.m_inputs[2] = in2;
		m.run_frame();
	}
	printf("TGP calls:");
	for (auto &[fn, n] : m.m_tgp_calls) printf(" %02x:%llu", fn, (unsigned long long)n);
	printf("\nTGP missing:");
	for (auto &[fn, n] : m.m_tgp_missing) printf(" %02x:%llu", fn, (unsigned long long)n);
	printf("\nTGP result underflows:");
	for (auto &[pc, n] : m.m_tgp_underflow) printf(" %06x:%llu", pc, (unsigned long long)n);
	printf("\n");
	FILE *f = fopen(out, "w");
	for (auto &[pc, n] : pcs) fprintf(f, "pc %06x %llu\n", pc, (unsigned long long)n);
	for (auto &[a, b] : jumps) fprintf(f, "edge %06x %06x\n", a, b);
	for (auto &[v, h] : irqs) fprintf(f, "irq %x %06x\n", v, h);
	fclose(f);
	printf("%zu unique instruction addresses\nopcodes used: %zu\n", pcs.size(), ops.size());
	std::multimap<uint64_t, uint8_t> byn;
	for (auto &[o, n] : ops) byn.insert({n, o});
	int k = 0;
	for (auto it = byn.rbegin(); it != byn.rend() && k < 300; ++it, ++k)
		printf("%02x:%llu%s", it->second, (unsigned long long)it->first, (k % 8) == 7 ? "\n" : "  ");
	printf("\n");
}
