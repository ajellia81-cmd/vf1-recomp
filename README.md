# Virtua Fighter Recomp

Virtua Fighter (SEGA, arcade, Model 1, 1993) as a native PC game: the arcade's own NEC V60 code is
recompiled to native code, and the rest of the Model 1 board (the TGP geometry coprocessor, the
polygon rasterizer, the tile layers, the sound board and the I/O board) is reimplemented around it.
Render scales up to 6x, widescreen and ultrawide, smooth motion at your screen's refresh rate, the
arcade's frames pixel for pixel at 1x. Mac, Windows and Steam Deck / Linux.

**Downloads and more:** https://vf1-recomp.pages.dev/

An unofficial fan project, not made, approved or supported by SEGA, and not affiliated with it.

## Bring your own MAME `vf` set

Nothing from the game is in this repository: no ROMs and nothing made from them. You need your own
dump of the game as MAME's `vf` set (`vf.zip`). The game asks for it the first time it starts and
recompiles its code to native code on your machine (`src/rt/native.cpp`, with `assets/vf_map.txt`,
a list of where the program's functions start: addresses only).

## Build

Needs a C++20 compiler, zlib, SDL 3 and FreeType (Mac: `brew install sdl3 freetype`).

```
make native            # build/bin/vf-native: the game, recompiled from your vf.zip when it starts
build/bin/vf-native
```

The release packages: `tools/mac/package.sh` (Mac, universal), `tools/win/build.sh` (Windows,
cross-compiled with MinGW-w64), `tools/deck/build.sh` (Steam Deck / x86-64 Linux, in Valve's Steam
Runtime "sniper" SDK container). They also take `assets/backgrounds/` (painted widescreen sides for the
title and player select screens), which isn't in this repository: the game runs without it, and those
two screens are then shown 4:3.

For development, the V60 code can also be recompiled ahead of time to C++ (kept in `build/`, never
committed): `python3 tools/romset.py roms/vf.zip build/vf roms/model1io.zip`, then `make` builds
`build/bin/vf`. `make rec` builds `vflock`, which runs MAME's V60 interpreter and the recompiled code
side by side and compares every register at every scanline.

## Layout

| Path | What |
| --- | --- |
| `src/core` | the NEC V60 interpreter and the Fujitsu MB86233 (TGP) core, from MAME |
| `src/machine` | Model 1: memory map, interrupts, TGP, rasterizer, tile chip, sound and I/O boards, smooth motion |
| `src/rt` | the recompiler's runtime, and the recompile done on the player's machine (sljit) |
| `src/host` | the SDL 3 host: window, input, pacing, the RmlUi menu (`assets/ui`) |
| `tools/recomp` | the ahead-of-time recompiler (V60 to C++) |
| `tools` | region builder, disassembler, headless runner, lockstep checker, the release scripts, the site |

## Credits

Built on MAME's work on the Model 1: the V60 core and disassembler (Farfetch'd, R. Belmont), the TGP,
the rasterizer, the System 24 tile chip and the machine (Olivier Galibert), the MultiPCM (Miguel Angel
Horna) and the sound board (R. Belmont), all BSD-3-Clause. Also Musashi (Karl Stenerud, MIT), ymfm
(Aaron Giles, BSD-3-Clause), sljit (Zoltan Herczeg, BSD-2-Clause), the Z80 core (Nicolas Allemand,
MIT), RmlUi (MIT), GLM (MIT), stb_image (Sean Barrett, public domain / MIT), SDL 3 (zlib), FreeType
(FreeType License) and the Barlow Condensed font (SIL Open Font License). `tools/release/licenses.sh`
gathers their notices for the downloads.

Virtua Fighter is SEGA's. This project is not affiliated with or endorsed by SEGA.

## Licence

MIT for this project's own code: see LICENSE. Code from MAME keeps its BSD-3-Clause licence and
notices; the rest of `third_party/` keeps its own.

## Support

If you enjoy it, you can buy me a coffee: https://buymeacoffee.com/mrfinster
