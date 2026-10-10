VIRTUA FIGHTER RECOMP (preview)
===============================

An unofficial fan project, not made, approved or supported by SEGA. Run it at your own risk.

Virtua Fighter (Sega Model 1, 1993), statically recompiled for the PC: the arcade game's own
code is translated to native code on your machine when it starts, and the rest of the Model 1
board is reimplemented around it. No game files are included: you need your own MAME "vf" set.


WHAT YOU NEED
- A Mac with macOS 11 (Big Sur) or later, Apple silicon or Intel.
- Your own copy of the game as MAME's "vf" set (vf.zip). That's all it needs.


FIRST LAUNCH
1. Unzip and move "Virtua Fighter Recomp" to Applications (or anywhere).
2. The app isn't signed, so the first time macOS won't open it. Double-click it once, close the
   message, then go to System Settings > Privacy & Security, scroll down and click Open Anyway.
   (On macOS 14 and earlier you can instead right-click the app, choose Open, then Open again.)
3. It asks for your vf.zip. Choose it. It's copied into
   ~/Library/Application Support/Virtua Fighter Recomp/roms, so you only do this once.


CONTROLS (change them in the menu: Esc, or View/Select on a pad)
Keyboard, player 1:  arrows move   Z guard   X punch   C kick   1 start
Keyboard, player 2:  I J K L move  B guard   N punch   M kick   2 start
Pads (Xbox layout):  d-pad/stick move   X guard   A punch   B kick   Y guard+punch (throw)
                     RB punch+kick   LB guard+kick   RT all three   Start start
Esc menu   P pause   F3 reset   F5 render scale   F6 picture shape   F9 frame timings
F11 fullscreen

8BitDo pads on a Mac: use Switch (S) mode.


OPTIONS
Display: render scale (up to 6x), antialiasing, picture shape (fits the window: widescreen and
ultrawide), background filter, fullscreen. Sound: volume, music, effects and voices.
Game: play style (PC: free play, no boot screen / arcade: coins and credits), reset.
Controls: each player's pad, keys and buttons.


YOUR FILES
Settings, saves (rankings, the operator settings) and your ROMs are in
~/Library/Application Support/Virtua Fighter Recomp. Put replacement stage skies in its
"backgrounds" folder (stage_beach_clouds.png and so on) to have them used instead of the originals.


CREDITS
Built on MAME's work on the Model 1 (the V60 CPU, TGP, sound and video), Musashi, ymfm, sljit,
SDL, RmlUi and FreeType: see LICENSES.txt. The recompile was done with Claude Opus 5.5.
