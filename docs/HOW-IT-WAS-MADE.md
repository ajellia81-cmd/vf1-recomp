# How Virtua Fighter Recomp was made

This is a write-up of how the port was built and checked, for anyone who wants to know what "recomp" means here, how
much of it comes from MAME, and how AI coding tools were used. The commit history has the detail; this is the overview.

## The approach

Virtua Fighter (1993) runs on Sega's Model 1 board. Its game logic is a program for the main CPU, an NEC V60. Around
that CPU the board has a Fujitsu MB86233 geometry DSP (the TGP), a polygon rasterizer, the System 24 tile chip for the
2D layers, an I/O board, and a sound board (a 68000, two MultiPCMs and a YM3438).

**The V60 program is statically recompiled.** The port never ships any of the game's code. It ships an address map,
`assets/vf_map.txt`: where the program's functions start and where its indirect jumps and calls lead, addresses only.
When the game starts, it reads the V60 program from the player's own `vf.zip`, walks the code from those addresses and
turns it into ARM64 or x86-64 code with SLJIT (`src/rt/analyse.cpp`, `src/rt/native.cpp`). That takes about 30 ms.
The map was made from coverage runs (scripted fights, attract mode, the ending), and if anything goes wrong the game
falls back to the interpreter.

There are two forms of the recompiled code:

- **At start-up (the release build, `make native`).** Control flow is native code. The instructions the game spends
  most of its time on (byte moves, bit tests, branches, and the run loop's cycle and interrupt check, together about
  80% of what runs) are emitted as machine instructions. Every other instruction calls MAME's handler for it.
- **Ahead of time, to C++ (`make rec`, for development).** `tools/recomp/v60recomp` and `emit.py` write one C++
  function per V60 function. Each instruction's operands are decoded at build time and MAME's handler body is pasted in
  with them, so about 99.5% of the instructions (25,823 of about 25,960) become straight-line C++; the rest (bit-string
  instructions and a few addressing modes) still go through MAME's handler. This build needs the ROM, so it stays on the
  developer's machine.

Both keep the interpreter's exact cycle counting and interrupt timing, so the rest of the board sees the same CPU.

**The rest of the board is MAME's code.** The TGP runs its own program on MAME's MB86233 core. The rasterizer, tile
chip, sound chips (MultiPCM, the 68000 via Musashi, the YM3438 via ymfm) and the I/O board's Z80 and EEPROM follow
MAME. They were brought into a standalone runtime (`src/machine`) without the rest of MAME: one machine, no device
framework, no drivers for other games. At 1x and 4:3 the picture is MAME's, pixel for pixel.

**The host is new.** `src/host` is an SDL 3 program: the window, keyboard and pad controls for both players, an in-game
menu (RmlUi), saves, the first-launch file picker, and the parts that go beyond the arcade:

- **Render scale.** The 3D is drawn at 1x to 6x the arcade's resolution. Above 1x the polygon fill is MAME's filler
  rewritten to sample at pixel centres, so neighbouring polygons share their edges without cracks.
- **Smooth motion.** VF's logic runs at 30 Hz. Above 1x, each picture is drawn for a moment between the last two game
  frames: every object's matrix is blended with its match in the frame before, paired like a diff of the two display
  lists. Camera cuts and objects that can't be paired aren't blended. The game itself runs untouched; the picture is a
  game frame behind.
- **Widescreen.** In fights the 3D view opens out to the sides and the stage sky is drawn from its whole panorama. The
  title and player select get painted side columns, drawn for this port.
- **Antialiasing** (an edge filter, or 2x/3x supersampling), scanline and CRT filters, and hi-res stage skies built
  from the game's own panoramas.
- **Pacing and a load governor.** The game runs on its own clock, trimmed to keep the sound queued. If a machine can't
  keep up, the governor turns off antialiasing and then lowers the render scale.
- **Scene sync.** VF builds a new scene over several frames. The host holds the last complete picture until the new
  3D has settled, so scene changes don't show half-built frames.

**The GPU renderer.** Since v1.1.11 the 3D can also be drawn on the GPU (Display > 3D renderer): Metal on a Mac,
SDL_GPU compute on Windows (Direct3D 12 or Vulkan) and Linux / Steam Deck (Vulkan). The goal was the CPU renderer's
picture exactly, not an approximation of it:

- **Draw order as depth.** Model 1 draws its polygons in a sorted order, with no depth buffer to hand to a GPU. So the
  GPU isn't used as a normal triangle rasteriser: a compute shader fills the polygons' rows itself, and for each pixel
  the last polygon in the game's order wins (an atomic maximum of its place in the list), as on the CPU.
- **The same arithmetic.** Each edge's crossing is worked out as the CPU does it, each step rounded on its own.
  Matching that on every platform took care: GPU division isn't correctly rounded, so divisions are done in integer
  arithmetic; shader compilers fuse multiply-adds, so nothing is fused; and float-to-integer conversion copies x86's.
- **Checked, then trusted.** Over the attract and fights, at several scales and shapes, on Metal, Vulkan and
  Direct3D 12, the GPU's pictures are the CPU's: 100%, no pixel different. The game repeats a small version of this at
  every start: a test picture drawn both ways, then its first pictures of a fight. A difference, an error or a
  missing driver, and that run uses the CPU. Auto (the default) then uses the GPU unless it's clearly slower than the
  CPU on that machine.

## How it was checked

Nothing was taken as working until it was measured. These checks run on every change that touches the game:

- **Lockstep against the interpreter.** `tools/vflock.cpp` runs two machines side by side, one on MAME's V60
  interpreter and one on the recompiled code. It compares every CPU register after every scanline and the RAM after
  every frame. Standard runs are 18,000 frames (five minutes of scripted fights), for both the start-up and the C++
  forms. The first difference stops the run and says where.
- **Frame and RAM comparison with MAME.** Reference pictures are taken from MAME offscreen. The port's headless runner
  (`tools/vfrun.cpp`) renders the same frames: attract mode matches MAME pixel for pixel, with both the interpreter and
  the recompiled code. These checks need the ROMs, so the script that makes the MAME references isn't in this repo.
- **Scripted, headless runs.** `tools/vfscript.cpp` plays input scripts with no window: the operator menus, scripted
  fighters (random presses, or real moves and combos), a full ladder to the ending and name entry. It records clips
  with their sound, logs each frame's inputs, and dumps or pokes RAM to find the game's variables. Bugs were reproduced
  as scripts before they were fixed.
- **GPU against CPU.** From 2x the picture is put together on the GPU. A check draws the same pictures both ways and
  requires them to match pixel for pixel at every scale and shape. At start-up the game runs a small version of the
  same check on the player's own renderer (Metal, Direct3D, OpenGL or Vulkan).
- **Where MAME is wrong.** MAME is the reference, but not the arcade. When they disagree, cabinet footage and photos
  decide. In name entry, for example, MAME draws half the wheel's letters black, while cabinet captures show them blue
  and red, so the port departs from MAME there on purpose. The commit says why, and the MAME check is expected to differ
  on those frames.

A typical commit message ends with the check results, such as "vflock 18000 identical both modes; MAME 120/120 both;
GPU check 0 differ at every scale/shape".

## Working with AI

The recompiler, runtime, host and tools were written with Claude, through Claude Code. In practice the work went like
this:

- **A person directs, tests and decides.** The human side was QA and play-testing on real machines (Mac, Windows, a
  Steam Deck), deciding what the port should do (where to stay arcade-exact, where to add a setting), checking footage
  of the real cabinet, and reviewing what went out. Many fixes started as "this looks wrong at 4x" or "the bushes
  jump", and the AI's job was to find out why and prove it.
- **Agents in parallel, all headless.** Several agents worked at once on separate problems. Each had its own build
  and data folder and ran the game hidden (`VF_HIDDEN=1`, no window, no Dock icon) through the headless tools. They
  never opened a visible window on the machine the person was working on.
- **Measure, don't assume.** Claims had to come with a measurement: a lockstep run, a pixel diff, a frame trace
  (`VF_TRACE`), a timing. Several plausible fixes were reverted when the numbers didn't support them.
- **Memory in the repo.** AI sessions don't remember each other, so the knowledge lives in writing. Commit messages say
  what changed, why, and how it was checked. A private notes file holds findings, dead ends and open questions. A new
  session reads those first instead of rediscovering them.

The AI wrote nearly all the code. A person decided what was right.

## Problems worth knowing about

- **Fighters' hair on the floor** → MAME's old high-level simulation of the TGP gets some geometry wrong → the TGP now
  runs its real program, and the port requires the ROM that contains it.
- **Dotted lines along the ring's edge at 4x** → MAME's filler rounds vertices to whole pixels, leaving gaps between
  polygons → above 1x, edges are sampled at pixel centres from unrounded positions. 1x keeps MAME's filler.
- **Every button did the wrong move** → the game's bits are punch, kick, guard in a different order than assumed →
  checked against the game's own input test.
- **Wolf's sky had a hard line under the clouds at high scales** → the game streams a dithered row over the
  panorama's top that the hi-res sky didn't take → the hi-res sky takes that row too.
- **Name entry's letters black** → the colour blocks the game reads are never set, in MAME or in the port → coloured
  as on the cabinet, only for those letters.
- **Widescreen select showed 4:3 between bars for player 2 or a challenger** → the select has three layouts and only
  player 1's was recognised → each layout gets its painted sides.
- **Half-built scenes and flashes at scene changes** → VF takes down and builds scenes over several frames → scene sync
  holds the last whole picture until the new 3D's polygon count settles (at most 12 frames).
- **Smooth motion stuck at 60 pictures a second after a run of scene changes** → the governor counted deliberate holds
  as missed pictures → it now judges only steady play.
- **Frames raced after a slow moment** → the lost time ran as frames back to back → no more frames per pass than the
  screen's pace; the rest is dropped.
- **Sound gaps under Wine** → its device runs about 5% slow and the queue overflowed → over 300 ms queued, the game
  waits for the sound instead of dropping it.
- **Sarah's stage looked flat under the fighters** → the game pulses a light from below on her stage, setting its
  length as well as its direction, and MAME (and the port) normalised every light vector, so the undersides stayed one
  shade → a light pointing up (only that one) keeps its length, as the Model 1's geometry uses it, and the undersides
  pulse with the floor as on the cabinet. Every other light is unchanged.
- **A GPU picture that wasn't the CPU's on another platform** → a GPU's division, fused multiply-adds and
  float-to-integer conversion each differ from the CPU's → integer division, no fusing, x86's conversion, and a
  self-test at every start (see above).
- **Players' setups** → portable mode (a `portable.txt` beside the program keeps everything in its folder) and a ROM
  folder of the player's own (`--roms`, `VF_ROMS` or `roms_folder` in `settings.ini`), only read, so an update can't
  touch it.
- **A crash at exit on Linux** → a widescreen write ran past the end of each line in the tile chip → found with ASan
  and fixed. ASan now runs clean at several scales and shapes.

## Where to look

- `src/rt/native.cpp`, `src/rt/analyse.cpp`: the start-up recompiler and code discovery. `assets/vf_map.txt` is its
  map.
- `tools/recomp/` (`v60recomp`, `emit.py`): the ahead-of-time C++ recompiler (`make rec`).
- `tools/vflock.cpp`: the lockstep checker. Start here to see what "identical" means.
- `src/machine/`: the Model 1 board (`model1*.cpp`): memory map, TGP glue, video, sky, sound, the I/O board.
- `src/host/main.cpp`, `scene_sync.h`, `gpu_compose.h`: the host, pacing, governor, scene sync and GPU compositing.
- `src/gpu/`, `src/machine/model1_gpu.cpp`: the GPU renderer (Metal, SDL_GPU and their shaders).
- `tools/vfrun.cpp`, `tools/vfscript.cpp`: the headless runner and the scripted test harness. `tools/vfcov.cpp` makes
  the coverage runs the map comes from.
- `git log`: each commit message explains what changed, why, and how it was checked.
