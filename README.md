<p align="center"><img src="assets/icon/icon.png" alt="Virtua Fighter Recomp" width="160"></p>

<h1 align="center">Virtua Fighter Recomp</h1>

<p align="center"><b>Virtua Fighter (SEGA, arcade, Model 1, 1993) as a native PC game: the arcade's own V60 code,
recompiled to native code, with render scales up to 6x, widescreen and smooth motion at any refresh rate.</b></p>

<p align="center">
  <a href="https://github.com/mr-finster/vf1-recomp/releases/latest"><img alt="Latest release" src="https://img.shields.io/github/v/release/mr-finster/vf1-recomp?label=release"></a>
  <a href="LICENSE"><img alt="Licence: MIT" src="https://img.shields.io/badge/licence-MIT-blue"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/platforms-macOS%20%7C%20Windows%20%7C%20Steam%20Deck%20%2F%20Linux-informational">
</p>

<p align="center">
  <a href="https://vf1-recomp.pages.dev/">Play</a> ·
  <a href="https://github.com/mr-finster/vf1-recomp/releases/latest">Download</a> ·
  <a href="https://www.youtube.com/watch?v=mHrTyikhkyg">Trailer</a> ·
  <a href="#status">Status</a> ·
  <a href="#controls">Controls</a> ·
  <a href="https://github.com/mr-finster/vf1-recomp/issues">Report a bug</a>
</p>

> [!IMPORTANT]
> No game files are included: no ROMs and nothing made from them. You need your own dump of the game as MAME's
> `vf` set (`vf.zip`). It is read on your computer, and the game's code is recompiled there when it starts.
> An unofficial fan project, not made, approved or supported by SEGA, and not affiliated with it.

## About

Virtua Fighter Recomp runs the arcade game's own NEC V60 program, recompiled to native code on your machine from
your `vf.zip`, and reimplements the rest of the Model 1 board around it: the TGP geometry coprocessor, the polygon
rasterizer, the tile layers, the sound board and the I/O board, from MAME's work on the Model 1. At 1x you get the
arcade's frames pixel for pixel; above it, render scales up to 6x, widescreen and ultrawide, and smooth motion at
your screen's refresh rate.

## Status

| Area | State |
| --- | --- |
| The arcade's V60 program, recompiled to native code | Working |
| TGP, rasterizer, tile layers, sound and I/O boards | Working: reimplemented from MAME's Model 1 code |
| Render scale up to 6x, widescreen and ultrawide, smooth motion | Working |
| Menu, keyboard and gamepads, two players | Working |
| macOS, Windows, Steam Deck / x86-64 Linux | Working |
| Painted widescreen sides for the title and player select | Working in the downloads; not in this repository (4:3 without them) |
| Online play | Not yet |

## Play

From the [latest release](https://github.com/mr-finster/vf1-recomp/releases/latest), or the
[site](https://vf1-recomp.pages.dev/):

| System | Download |
| --- | --- |
| macOS 11+ (Apple silicon and Intel) | `VirtuaFighterRecomp-mac.zip` |
| Windows 10 or 11 (64-bit) | `VirtuaFighterRecomp-windows.zip` |
| Steam Deck / x86-64 Linux | `VirtuaFighterRecomp-deck.tar.gz` |

**Mac and Windows**

1. Unzip it (Mac: move **Virtua Fighter Recomp** to Applications; Windows: the VirtuaFighter folder anywhere, not inside Program Files).
2. Open it (Windows: `VirtuaFighter.exe`).
3. Choose your `vf.zip` when it asks. A copy is kept, so you only do this once.

**Steam Deck**

1. Copy the download to the Deck (e.g. Downloads) and unpack it there: in Desktop Mode, right-click it in the file
   manager > Extract > Extract archive here. (Unpacking on Windows, or copying the folder through a USB stick, can
   lose the files' permissions, and then nothing launches.)
2. Put your `vf.zip` in the `VirtuaFighter/roms` folder.
3. Desktop Mode: Steam > Games > Add a Non-Steam Game to My Library > Browse, set the file type filter to "All
   files", pick the `vf` file in the VirtuaFighter folder, Add Selected Programs. In its Properties, set Start In to
   the VirtuaFighter folder (e.g. `/home/deck/Downloads/VirtuaFighter/`), and under Compatibility leave "Force the
   use of a specific Steam Play compatibility tool" off: it's a native Linux game, and Proton stops it launching.
4. Back in Game Mode, launch it from your Library with the default Gamepad layout.

> [!NOTE]
> The Mac and Windows builds aren't signed. On macOS, open it once, close the message, then System Settings >
> Privacy & Security > Open Anyway. On Windows, if SmartScreen warns, choose More info > Run anyway.

## Controls

Change them in the menu (<kbd>Esc</kbd>, or View / Select on a pad).

| Key | Does |
| --- | --- |
| <kbd>↑</kbd> <kbd>↓</kbd> <kbd>←</kbd> <kbd>→</kbd> | Player 1: move |
| <kbd>Z</kbd> <kbd>X</kbd> <kbd>C</kbd> | Player 1: guard, punch, kick |
| <kbd>1</kbd> | Player 1: start |
| <kbd>I</kbd> <kbd>J</kbd> <kbd>K</kbd> <kbd>L</kbd> | Player 2: move |
| <kbd>B</kbd> <kbd>N</kbd> <kbd>M</kbd> | Player 2: guard, punch, kick |
| <kbd>2</kbd> | Player 2: start |
| <kbd>Esc</kbd> | Menu: display, sound, game, controls, quit |
| <kbd>P</kbd> | Pause |
| <kbd>F3</kbd> | Reset |
| <kbd>F5</kbd> / <kbd>F6</kbd> | Render scale / picture shape |
| <kbd>F9</kbd> | Frame timings |
| <kbd>F11</kbd> | Fullscreen |
| <kbd>F12</kbd> | Screenshot |

Gamepad (Xbox layout): d-pad or stick to move, X guard, A punch, B kick, Y guard + punch (throws), RB punch + kick,
LB guard + kick, RT all three, Start to start. View (tap) opens the menu; View + LB / RB change the render scale /
picture shape.

## How it works

When it starts, the game's V60 program from your `vf.zip` is recompiled to native code with SLJIT
(`src/rt/native.cpp`), guided by `assets/vf_map.txt`: a list of where the program's functions start, addresses only,
no bytes from the ROM. Around it runs a reimplementation of the Model 1 board: memory map and interrupts, the
Fujitsu MB86233 TGP, the rasterizer, the System 24 tile chip, the sound board (68000, MultiPCM, YM3438) and the I/O
board, all from MAME's code. An SDL 3 host draws the picture at any scale and shape and fills in the frames between
the arcade's for smooth motion.

## Repository layout

| Path | Contents |
| --- | --- |
| `src/core` | the NEC V60 interpreter and the Fujitsu MB86233 (TGP) core, from MAME |
| `src/machine` | Model 1: memory map, interrupts, TGP, rasterizer, tile chip, sound and I/O boards, smooth motion |
| `src/rt` | the recompiler's runtime, and the recompile done on the player's machine (SLJIT) |
| `src/host` | the SDL 3 host: window, input, pacing, the RmlUi menu |
| `assets/` | `vf_map.txt` (function addresses), the app icon, the menu (`ui/`) and its font |
| `third_party/` | MAME code, Musashi, ymfm, SLJIT, the Z80 core, RmlUi, GLM, stb_image |
| `tools/recomp` | the ahead-of-time recompiler (V60 to C++), for development |
| `tools/port` | the script that ported MAME's TGP and rasterizer |
| `tools/*.cpp`, `tools/romset.py` | region builder, disassembler, headless runner, lockstep checker, coverage |
| `tools/mac`, `tools/win`, `tools/deck`, `tools/release` | the release packages and their licence notices |
| `tools/site` | the download site |

## Run it yourself

Needs a C++20 compiler, zlib, SDL 3 and FreeType (Mac: `brew install sdl3 freetype`).

```
make native            # build/bin/vf-native: the game, recompiled from your vf.zip when it starts
build/bin/vf-native
```

The release packages: `tools/mac/package.sh` (Mac, universal), `tools/win/build.sh` (Windows, cross-compiled with
MinGW-w64), `tools/deck/build.sh` (Steam Deck / x86-64 Linux, in Valve's Steam Runtime "sniper" SDK container).
They also take `assets/backgrounds/` (painted widescreen sides for the title and player select screens), which
isn't in this repository: the game runs without it, and those two screens are then shown 4:3.

For development, the V60 code can also be recompiled ahead of time to C++ (kept in `build/`, never committed):
`python3 tools/romset.py roms/vf.zip build/vf roms/model1io.zip`, then `make` builds `build/bin/vf`. `make rec`
builds `vflock`, which runs MAME's V60 interpreter and the recompiled code side by side and compares every register
at every scanline.

## How it's made

Made with the help of AI coding tools. Every build is checked against the arcade: the recompiled code runs side by
side with an interpreter of the original code, and both are compared frame by frame with MAME's pictures, RAM and
sound.

## Credits

Built on MAME's work on the Model 1: the V60 core and disassembler (Farfetch'd, R. Belmont), the TGP, the
rasterizer, the System 24 tile chip and the machine (Olivier Galibert), the MultiPCM (Miguel Angel Horna) and the
sound board (R. Belmont), all BSD-3-Clause. Also Musashi (Karl Stenerud, MIT), ymfm (Aaron Giles, BSD-3-Clause),
SLJIT (Zoltan Herczeg, BSD-2-Clause), the Z80 core (Nicolas Allemand, MIT), RmlUi (MIT), GLM (MIT), stb_image
(Sean Barrett, public domain / MIT), SDL 3 (zlib), FreeType (FreeType License) and the Barlow Condensed font (SIL
Open Font License). `tools/release/licenses.sh` gathers their notices for the downloads.

Virtua Fighter is SEGA's. This project is not affiliated with or endorsed by SEGA.

## Licence

MIT for this project's own code: see [LICENSE](LICENSE). Code from MAME keeps its BSD-3-Clause licence and notices;
the rest of `third_party/` keeps its own.

## Support

If you enjoy it, you can [buy me a coffee](https://buymeacoffee.com/mrfinster).
