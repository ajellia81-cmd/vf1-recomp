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
- **The cheats against the last release.** Before the random fighter shipped, one small driver was built against both
  the last release and the new code and played the same 16,000 frames: attract, a 1P arcade game on random inputs,
  and a challenger. It hashed every frame's picture, RAM, sound and program ROM. With the cheats at their defaults,
  all four matched on every frame. A second test turned each random mode on, played a select and a fight, then
  turned it off the way the menu's restart does. It required the ROM to match the original byte for byte. Four
  independent code reviews ran alongside, and their findings were fixed or written up before the release.
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

## A case study: the random fighter's heads

Random fighter (menu > Cheats) builds a fighter from parts of the cast: one character's head on another's body,
moves from three move lists. It took several rounds to make those fighters move like the real cast, and the rounds
show how the work went, including the wrong turns.

- **The complaint:** heads looked up, or twitched left and right, at the ends of moves. Screenshots alone couldn't say
  why, so the first step was always a measurement: a test-only log (`VF_JOINTLOG`) prints each fighter's head and
  limbs, as the game's own display lists have them, for real characters and random ones.
- **A fix that made it worse.** Smooth motion eases a random fighter's pose between moves. A first attempt eased every
  part down the skeleton, head included. The hands stayed on the wrists, but heads turned further and a face lost a
  piece. It was reverted the same day. The version that stayed eases only the limbs down the skeleton, and the
  head-to-torso angle was checked to be unchanged on every frame of three fights.
- **The wrong baseline.** Flick counts first came from one real fight (Akira against Jacky), which made random
  fighters look twice as bad as they were. A real baseline needs the same script played by each real character:
  `VF_COPYREC` copies any character's record into both players' places, so six real characters fought the same fight.
- **A correction that caused nodding.** The head-lean correction followed whose move was playing, so it changed at
  every change of move and the head nodded with it. It is now one lean per fighter. The lean is applied to the head's
  vertices, not its matrix, so the matrix logs never showed it: the log was extended to print what is drawn.
- **Finding the real cause in the game's code.** Comparing real Jacky's punch with a random fighter playing the same
  punch, frame by frame, showed identical heads in the middle of the move and a split only in the last 16 frames.
  The RAM was then dumped on every frame (`dump` in the test scripts) and diffed, real against random, until the
  values that split only there were found. A write watch (`VF_WATCHW`, which logs the program counter of every write)
  led to the code, and the disassembler (`vfdis`) showed it. The game blends the end of every move toward a stance it
  looks up by the fighter's move list in a list of the ten characters. A random fighter's move list isn't in it, so
  the lookup fell through to the first entry, Dural, whose stance has the other foot forward. Every move ended by
  twisting toward Dural's stance, then snapped into the fighter's own. A second fault was found the same way: the
  stance table is built at boot from player 1's record, which with random opponents was itself random.
- **The fix**, in the game's own terms: the lookup now reads a per-record key into a small table of the port's own
  (the same technique as the special moves), and the boot builds the stance table from the original records. Checked:
  the stance table is byte for byte the real game's, a punch's end into the stance moves the head 4-7 degrees in a
  frame (the real cast's range, down from up to 49), head flicks over whole fights are within the real cast's rate,
  and the real game is unchanged frame for frame. With the game's own blend working, the port's extra easing for
  random fighters was no longer needed (it made jump attacks ramp up out of the old pose), so it was switched off:
  random fighters now move with exactly the smoothing the real cast gets.
- **A fix's own side effect.** The owner then saw the stance snap once as the intro began. The first check said it
  didn't reproduce, but that check skipped the first 300 frames, which is the whole intro. Measured from the first
  frame, every random fighter's pose jumped 27-54 degrees at frame 241, and no real character's did. The boot fix
  held the original move lists for a fixed 240 frames, but a write watch showed the table is built on frames
  203-204 and the fighters are drawn from 211, so the lists came back while the fighters were on screen. They now
  come back at frame 206. Checked: no jump in the intro for three random casts, and the stance table is still byte
  for byte the real game's. The lesson: when a measurement says "can't reproduce", check what it left out.
- **Shared models.** The port styles a random fighter's head (its colours, its turn, the neck fit) by the models it
  draws, and two fighters can wear the very same models: a random fighter with Jeffry's head against the computer's
  Jeffry. Only the random fighter's style was listed, so the computer's Jeffry was drawn in it too: the random hue
  in his hair and a hole in his jaw where the neck fit moved part of his face. Now every fighter's head is listed
  (the computer's plainly, as the game has it), and a model both wear is told apart by which torso it is nearer.
- **Names as seeds.** A random fighter's name only says which three characters it's blended from, so a name can't be
  decoded into a fighter. Instead the port makes the fighter of each of 32,768 seeds once (on a copy of the game's
  data, put back after), keeps the first seed for each name (339 names, all found by seed 11,775), and a name typed
  in the menu means that seed. So the same name is the same fighter everywhere, and a name no fighter has is
  refused.
- **Two players, three choices each.** Each player can be Normal, Random or a character seed. Random fighters live in
  Akira's two records (his two costumes), so the question was which record each player ends up in. A write watch
  showed the pick stores a character number in the player's block and the match load turns it into the record. A
  first test said two players picking Akira share one record, but the test's own shortcut (writing the cursor
  straight into memory) caused that: with real joystick presses, player 1's side always gets one record and player
  2's the other. So each player's setting makes their side's record, and nothing can collide. The select shows only
  when a Normal player is at it, each side's portrait the head that side will get, and a random player's controls
  are locked there. Checked with eight set-ups, from one player on either side to a challenger joining mid-game.
  The lesson: a test shortcut can create the very bug it's looking for; check the finding with real inputs.
- **Random mode must not touch the real game.** A check found that switching Random fighter off left the last random
  fighter's head colour and neck fit on the real character who owns that head. Everything random mode sets is now
  cleared when it goes off, and the game with it off was compared frame by frame against the build before.

## Problems worth knowing about

- **The random fighter's round-1 build didn't happen at all** → its end check subtracted a start frame still in the
  future from an unsigned frame counter, which wrapped to a huge number and cleared the build on its first frame →
  the subtraction is signed. Found from a contact sheet of the intro, not from the code.
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
- **Random fighters' moves ended twisting round, the head looking up, then snapped into their stance** → the game
  looks a fighter's stance up by its move list in a list of ten; a random fighter's list isn't there, so it got
  Dural's, and the stance table was built at boot from a random record → the lookup reads a per-record key into the
  port's own table, and the boot builds the table from the original records.
- **Random fighters' heads nodded at every change of move** → the head-lean correction followed whose move was
  playing → one lean per fighter.
- **A random fighter's hand floated off its wrist while smooth motion eased between moves** → each part was eased on
  its own about the hips → arms and legs ease down the skeleton, each part on the one it hangs from.
- **Switching Random fighter off left a random head's colour on a real character** → the head styles were only
  refreshed while random mode ran → cleared as it goes off.
- **A seeded random fighter wasn't the same as the computer's** (another voice, some moves of the character whose
  place it took) → it was made on whatever record it went in, and the fields the mixer doesn't touch (the character
  number the game's move code and voices key on, size, weight) stayed that record's → every one is made on Akira's
  record and moved in, and the voice is looked up by the record's key in the port's own table (a five-byte read
  replaced by a call into the ROM's empty space), so it can be one of the three it's made of.
- **Before launch, a sweep of every character cheat** → three read-only reviews in parallel (the game's state, the
  rendering, the menus of both games), each finding checked in the code before anything changed. Real ones fixed (a
  skin match that turned themed clothes into skin, a secret character whose hands a Random opponents remake took,
  seed letters read by key position on non-US keyboards); the rest written down with why they were left.

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
- `src/machine/model1.cpp` (`random_fighter_*`), `model1_interp.cpp` (`pose_*`), `model1_video.cpp` (`neck_*`): Random
  fighter, its pose easing and its heads. Test-only switches print what the game does (`VF_JOINTLOG`, `VF_HEADLOG`,
  `VF_POSELOG`, `VF_CASTLOG`), compare with VF's own behaviour (`VF_STANCE_VF`, `VF_LEAN_BY_MOVE`, `VF_NOPOSE`) or
  watch the game's writes (`VF_WATCHW`); `tools/vfdis.cpp` disassembles the V60 code they lead to.
- `git log`: each commit message explains what changed, why, and how it was checked.
