// The player's own MAME `vf` set, read straight from the zip (what tools/romset.py does for our
// own builds): every ROM checked by CRC and laid out in the regions MAME's driver builds.
#pragma once
#include "../machine/model1.h"
#include <string>
#include <vector>

namespace romset {

struct result
{
	bool ok = false;
	std::string error;              // for the player: what's missing or wrong
	model1::regions regions;
	bool has_tgp = false, has_ioboard = false;
};

// zips: vf.zip, and optionally model1io.zip (the I/O board's program) or a newer vf.zip with the
// TGP's program. Every file in them is looked at; the ROMs are found by name or by CRC.
result load(const std::vector<std::string> &zips);

// Whether a zip holds the main game (enough to tell vf.zip from model1io.zip when picked).
bool is_game_set(const std::string &zip);

} // namespace romset
