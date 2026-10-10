// Loads the regions tools/romset.py builds from the user's own dump.
#pragma once
#include "../machine/model1.h"
#include <string>

inline std::vector<uint8_t> load_file(const std::string &path)
{
	std::vector<uint8_t> d;
	FILE *f = fopen(path.c_str(), "rb");
	if (!f) { fprintf(stderr, "missing %s (run tools/romset.py on your vf.zip)\n", path.c_str()); exit(1); }
	fseek(f, 0, SEEK_END); d.resize(ftell(f)); fseek(f, 0, SEEK_SET);
	fread(d.data(), 1, d.size(), f);
	fclose(f);
	return d;
}

// Builds the machine, with the TGP's own program when the set has it.
inline model1::regions load_regions(const std::string &dir)
{
	model1::regions r;
	r.maincpu = load_file(dir + "/maincpu.bin");
	r.copro_tables = load_file(dir + "/copro_tables.bin");
	r.polygons = load_file(dir + "/polygons.bin");
	r.sndcpu = load_file(dir + "/sndcpu.bin");
	r.pcm1 = load_file(dir + "/pcm1.bin");
	r.pcm2 = load_file(dir + "/pcm2.bin");
	if (FILE *f = fopen((dir + "/tgp_program.bin").c_str(), "rb")) { fclose(f); r.tgp_program = load_file(dir + "/tgp_program.bin"); }
	if (FILE *f = fopen((dir + "/ioboard.bin").c_str(), "rb")) { fclose(f); r.ioboard = load_file(dir + "/ioboard.bin"); }
	return r;
}
