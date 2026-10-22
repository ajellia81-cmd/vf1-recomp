# Model 1 iOS options audit

This is a source/application/persistence audit, not a ROM gameplay test. No game
archive was used. Runtime validation results belong in VALIDATION.md.

| Category / item | Reviewed behavior | Validation boundary |
| --- | --- | --- |
| Main: Resume / Back | Menu close/pop, audio and game clock resume | Source checked; touch Menu uses the same SDL event path |
| Main: Quit | Waits for worker, completes saves, destroys graphics/host locals, then iOS process exit | Device/LiveContainer exit needs confirmation |
| Display: Resolution | Original 1x-6x or 2868x1320 output resolved from 4x geometry; saved selector and hotkeys agree | Output target pixel check; not fractional native 3D rasterization |
| Display: Antialiasing | Off/Low/2x/3x, bounded to original maximum internal scale | Original renderer path; game images require device |
| Display: 3D renderer | CPU/GPU/Auto, Metal on iOS; original failure/comparison fallback | Synthetic Metal polygon check; actual game comparisons happen on-device |
| Display: Picture shape | Fit window, 4:3, 16:10, 16:9, 21:9; saved aspect retained | Source checked; final view resets viewport/clip/scale every pass |
| Display: Background filter | Original Model 1 sky filter and redraw; smoothing matters above 1x | Native-output mode uses 4x; actual sky appearance requires game |
| Display: Screen filter | Off/Scanlines/CRT, below menu; normalize across all row phases | Pixel checks require changed rows/columns and restored Off |
| Display: Brightness | 50-200%, only final picture; saved setting | 100/150/200/100% pixel checks |
| Display: Smooth motion | Toggle original interpolation, no automatic quality governor; iPhone maximum-refresh opt-in | Actual 120 fps depends on device, system and workload; unmeasured |
| Display: Show FPS | Presented-picture count, safe-area upper left, clears when disabled | Device placement remains confirmation |
| Sound: Volume | SDL stream gain; 0-100%, saved | Listening test needed |
| Sound: Music / Effects and voices | Original Model 1 mixer gains; 0-100%, saved | Listening test needed |
| Game: Play style | PC free play / Arcade with preserved coin settings; restart | Source checked; actual credits/start need game |
| Game: Reset game | Original machine reset, settings/NVRAM retained | Source checked; device behavior untested |
| Arcade: Language | Japanese/English assignment, one-time English default does not override user | Original NVRAM and restart path |
| Arcade: Difficulty / Ring size | Difficulty and default or selected width applied | Original NVRAM/settings and restart path |
| Arcade: Rounds CPU / versus | Original assignment bounds and restart | Source checked |
| Arcade: Continue / Demo sound | Assignment toggles; NVRAM saved | Source checked; actual behavior needs game |
| Arcade: Service menu | Test pending after boot; Down/Start operate cabinet inputs | Source checked; real-board menu needs game |
| Controls: Player 1 / Player 2 pad | Auto/specific/None retained, only shared SDL physical-input path | Both players' choice persistence checked; physical hardware untested |
| Controls: Keyboard / Pad buttons | Every action, capture/clear/duplicate removal and saved mapping | Full binding round-trip check; iOS external keyboard/pad needed |
| Controls: Reset to defaults / Swap pads | Original reset/conflict handling; same-GUID swap flag persisted | Source checked; two-controller hotplug/swap needs hardware |
| Cheats: Player 1 / Player 2 fighter | Normal/Random/Seed/Secret, per-player character/seed saved | Original restart/application path; seed entry uses SDL text input; device test needed |
| Cheats: Random opponents | Saved toggle, applied at restart | Requires game confirmation |
| Cheats: Infinite energy / One-hit KO | Saved, applied through original per-frame fight cheats | Requires game confirmation |
| Cheats: No time limit / Ring | Saved, original code patch/ring-size and restart paths; CPU stays interpreter | Requires game confirmation |
| Cheats: Kage's mask | Saved, original appearance modification | Requires game confirmation |
| Touch / suspension | Directions/G/P/K/Coin/Start/Menu only without connected controller; main-thread lifecycle saves | Drain-and-resume writer test; actual gestures/background/restore need device |

Fullscreen is always enabled on iOS and its desktop toggle is hidden. Windows
Graphics driver is absent on iOS. Keyboard shortcuts for render scale update
the mobile selector; changing antialiasing keeps the selected resolution mode.

## Build 2: original choices plus iPhone output

Resolution keeps the original 1x, 2x, 3x, 4x, 5x, 6x order and appends
2868 x 1320 (iPhone). Every resolution, including iPhone output, honors all
original Picture shape choices (Fit the window, 4:3, 16:10, 16:9, 21:9).
The iPhone output resolves the original 4x raster to 2868 x 1320; it does
not override or overwrite the selected aspect ratio.

## Build 3: touch Menu while paused

Menu taps survive repeated menu-state updates. Entering the menu releases only
gameplay buttons; it does not erase a pending Menu tap. The host samples the tap
before updating menu visibility and toggles the menu once per tap. The ROM-free
touch_menu check exercises the actual bridge transition helper.
