# Summoner Reverse

A reverse-engineering and decompilation project for **Summoner** (Volition / THQ, 2000), the PC version 1.40.
It has three parts:

- **Documentation** of how the game works, its file formats and 355 bugs found in its code ([`docs/`](docs/index.md),
  set up as a GitHub Pages site).
- **SummonerFix**, a drop-in `dinput.dll` that makes the original game run well on Windows 10 and 11 (no launcher,
  correct speed, OpenGL, widescreen, key rebinding) and fixes over 40 of those bugs, each with its own switch
  ([SummonerFix](docs/summonerfix/index.md)).
- **A matching decompilation** of `Sum.exe` in C++ ([`src/decomp`](src/decomp/NOTICE.md)): every function of the
  game is rewritten, 5,445 of the 5,926 compiled functions give the original machine code byte for byte with the
  original compiler, and the source links into a complete executable that plays the game, every boss fight
  included ([Decompilation plan](docs/decomp-plan.md)).

**You need your own copy of Summoner** (it is still sold in digital stores). This repository contains no game files.

The analysis used [Ghidra](https://github.com/NationalSecurityAgency/ghidra) through
[REA](https://github.com/morluto/rea), LLVM's `llvm-objdump`, and the game's own compiler for byte matching. The
documentation and the decompilation notes are also a working log, so they mention work rounds, numbered decisions
(`src/decomp/include/DECISIONS.md`) and "the user" (the project's owner, who played the tests).

## Building

- **SummonerFix:** LLVM-MinGW (`i686-w64-mingw32-gcc`), `tools/build-summonerfix.ps1`. Put the resulting
  `dinput.dll` next to `Sum.exe`. See [Setup](docs/setup.md). With Windows 11's **Smart App Control** on, Windows
  silently refuses to load the unsigned DLL and the game starts without it, and it may refuse to start freshly
  built or downloaded programs such as the save editor ([SummonerFix](docs/summonerfix/index.md)).
- **The save editor:** `tools/build-saveeditor.ps1` builds `SummonerSaveEditor.exe`, a window that changes a save's
  gold, party, inventory and story flags and always writes a new save ([Save editor](docs/save-editor.md)).
- **The fixed build:** `node tools/link-build.mjs --config fixed` builds `Sum-fixed.exe`, the decompiled game with
  43 bugs fixed in the source ([Fixed build changes](docs/fixed-build.md); not yet tested in game).
- **The decompilation:** Microsoft Visual C++ 6.0 with Service Pack 5 (compiler build 8966, the one Sum.exe was built
  with), which you have to supply yourself; set `VC6_DIR` to its `VC98` folder. `tools/vc6-sweep.mjs` compares every
  function with your `Sum.exe`; `tools/link-build.mjs` links the standalone executable (it also needs libjpeg 6b
  from the IJG source in `third_party/`). See [Setup](docs/setup.md) and [Tools](docs/tools.md).
- The scripts in `tools/` run with Node.js 24 from the repository folder.

## Findings so far

| Topic | Summary | Page |
|---|---|---|
| SummonerFix | Our own `dinput.dll` patch: runs on Windows 11 without the launcher (hidden OpenGL renderer), fixes 2x game speed, the >4 GHz timer and the 24.9-day cutscene skip, black save thumbnails; widescreen; key and mouse rebinding with a config app; fast-mode key with an on-screen badge; console-command keys; optional zoom, interaction-range and intro-skip settings; fixes the crash on Start New Game without an audio device, the low textures on a fresh OpenGL install, the brightness slider under OpenGL and world sounds at the sfx volume squared; 42 more fixes for bugs in the original game (crashes, AI slips, UI errors), almost all reproduced before they were fixed (in game, by a direct function call or with a faked failure), each with its own switch in the config app; a key for the hidden script editor | [SummonerFix](docs/summonerfix/index.md) |
| Main loop | Variable timestep, capped at 120 FPS, clamped at 0.25 s per frame, driven by a 15-entry game-state table | [Main loop and frame timer](docs/sum-exe/main-loop.md) |
| Game bugs | 355 defects in v1.40 checked in the code: crashes and hangs (most latent), gameplay slips such as a save overwrite without a prompt, cosmetic UI errors; which ones SummonerFix fixes | [Bugs in the original game](docs/sum-exe/game-bugs.md) |
| Timing bugs | QPC frequency truncated to 32 bits (>4 GHz); frame limiter double-counts its sleep (2x speed); cutscene timer breaks after ~24.9 days of uptime (reproduced and fixed) | [Timing bugs](docs/sum-exe/timing-bugs.md) |
| Characters | Character object, player record, 84 skills, status effects, experience table, resistances | [Characters](docs/sum-exe/characters.md) |
| AI | 13 AI states, goals, attacker slots, paths | [AI](docs/sum-exe/ai.md) |
| Bosses | 14 boss AIs: HP-based stages, dispatchers, mechanics, the cutscenes that gate them, console stage commands | [Bosses](docs/sum-exe/bosses.md) |
| World map | Exits per game stage; a 50% encounter roll every 10 s while moving; the full encounter table | [World map and encounters](docs/sum-exe/worldmap.md) |
| Script editor | Volition's level script editor is still in the retail game; it opens when one debug flag bit is set (tested) | [Script editor](docs/sum-exe/script-editor.md) |
| Sound | DirectSound/EAX layers, channels and streams, music selection; why a missing audio device crashes a new game | [Sound and music](docs/sum-exe/sound.md) |
| Cutscenes | All story scenes run in the engine (Bink only plays the logos and credits): `cutscene.tbl`, `.csc` camera shots, what is saved and restored, Esc skip and `+NoAbort` | [Cutscenes](docs/sum-exe/cutscenes.md) |
| Effects and lights | Effect objects, `.vfx` files and key tracks, vif particles, decals, lamp glows, the 400-entry light table | [Effects and lights](docs/sum-exe/effects.md) |
| Summons | Rings unlock the summons and gain power from experience; the creature's level comes from ring power; one summon at a time, max HP lent; dragons cost HP | [Summons](docs/sum-exe/summons.md) |
| Shops and loot | Fixed prices (sell 70%, 10% in multiplayer), free identification, gold and item drop rolls, boss key items | [Shops, loot and money](docs/sum-exe/shops-loot.md) |
| Party | Five Player records, leader and selection, solo mode is a hold order, formation and follow rules, XP split between members within 20 units | [Party](docs/sum-exe/party.md) |
| Camera | Orbit camera with low, high and automatic presets; arrows and wheel zoom (no tilt, tested); occluders fade instead of collision | [Camera](docs/sum-exe/camera.md) |
| Savegame | File format, read-only inspector and an editor that writes new files | [Savegame format](docs/sum-exe/savegame.md) |
| Multiplayer | All 66 packet types and their handlers | [Multiplayer packets](docs/sum-exe/multiplayer.md) |
| Combat | Melee and spell damage formulas | [Combat](docs/sum-exe/combat.md) |
| Game data | items, materials, spells (all 108), character info tables in memory | [Game data tables](docs/sum-exe/game-tables.md) |
| Level scripts | Per-level story logic through a 51-entry handler table | [Level scripts](docs/sum-exe/level-scripts.md) |
| Interface | All 23 interface screens and their functions; UI widget classes | [Interface](docs/sum-exe/interface.md), [UI](docs/sum-exe/vsdk-ui.md) |
| Decompilation | About 11,500 symbols in `symbols/sum.symbols`: every function (8,642, including about 2,300 that Ghidra misses and the whole C runtime) and 2,840 data globals; under 2% are still marked inferred | [Decompilation plan](docs/decomp-plan.md) |
| Matching decompilation | `src/decomp`: 7,725 functions rewritten in C++ (all game and engine code); Sum.exe was built with Visual C++ 6.0 Service Pack 5 (`c2.dll` build 8966, identified 2026-10-16), and with that compiler 5,445 of the 5,926 compiled functions give the original bytes exactly, with every checkable call and data reference pointing at the same place | [Decompilation plan](docs/decomp-plan.md) |
| Hybrid game | The whole decompiled source (5,922 functions) compiled with VC6 into a DLL that SummonerFix loads in place of the original functions: the game starts, loads a save and plays the same, so each decompiled function can be tested in the real game on its own. Comparing quicksaves with the original (18 levels, inventory, a trader) and running decompiled functions against the original on the game's own calls in the boss fights found five decompilation errors that byte matching could not see; reading the functions no test had run and then running them in the game found eight more (thirteen in all, all fixed; 628 of the 890 functions that do not match byte for byte have now run in a checked scenario) | [Decompilation plan](docs/decomp-plan.md) |
| Standalone rebuild | The decompiled source links into its own executable with Visual C++ 6.0 SP5 and its libraries, nothing from the original Sum.exe (`tools/link-build.mjs`); with libjpeg 6b built from the IJG source, a new game reaches Masad and plays; comparing its data with the original's found three more errors (D9811, D9813, D9814), and a play-test through the start of Act 1 one more (D9815, the prologue cinematic), and a review of the functions it first ran three more (D9816, D9817); later play-tests and a save and memory comparison over 28 levels found no further difference (2026-10-19) | [Decompilation plan](docs/decomp-plan.md) |
| Timeslip's patch (v3) | Hooks QPC/QPF through the import table, adds a virtual-clock speed control and removes the CD check | [Timeslip's patch](docs/patches/timeslip.md) |

## Repository layout

```
docs/            GitHub Pages site (Markdown, Jekyll "Just the Docs" theme)
src/summonerfix/ SummonerFix source (C, built with LLVM-MinGW)
src/summonerfix-config/ SummonerFixConfig.exe (key bindings and options)
symbols/         Symbol file and structure layouts for Sum.exe
tools/           Build, test and analysis helper scripts
Summoner/        Local game install - NOT committed (see .gitignore)
```

## Credits

- [Summoner 2 decompilation](https://github.com/mikegoforth/summoner-2-decomp) (PlayStation 2): a source of names
  for the engine Summoner shares with its sequel. Names only; no code was taken.
- [Dash Faction](https://github.com/rafalh/dash-faction) (Red Faction, the same engine family): a source of names
  for engine structures. Names only; no code was taken.
- The Independent JPEG Group's libjpeg 6b, which the standalone executable links (this software is based in part
  on the work of the Independent JPEG Group).

## Legal

This repository contains documentation, our own helper scripts, the SummonerFix source and a C++
reconstruction of `Sum.exe` (`src/decomp`, written from the disassembly of a retail copy). It does not contain
game files, game executables, extracted game data, compilers or third-party patch binaries.

- The MIT licence in [`LICENSE`](LICENSE) covers our own work: the tools, SummonerFix, the documentation and the
  symbol files. It does **not** cover `src/decomp`; see [`src/decomp/NOTICE.md`](src/decomp/NOTICE.md).
- This project is not affiliated with, endorsed by or connected to THQ Nordic, Volition or any other rights holder.
  "Summoner" is a trademark of its owner. If you hold rights in Summoner and want something changed or removed,
  please open an issue.
