# AGENTS.md - working in this repository

For AI agents and anyone who wants the facts without the history. Humans: start at [README.md](README.md) and
[docs/quickstart.md](docs/quickstart.md). State on 2026-11-10: [docs/status.md](docs/status.md).

## What this is

A reverse engineering of **Summoner** (Volition, 2000), PC v1.40 `Sum.exe` (32-bit, Visual C++ 6.0 SP5):
- `src/decomp/`: the whole game rewritten in C++, one file per original translation unit. Every function carries
  its Sum.exe address and a confidence comment. 5,618 of 5,926 compiled functions match the original bytes.
- `src/summonerfix/`: `dinput.dll`, a patch DLL that makes the original exe run on Windows 10/11.
- `docs/`: everything found, as a GitHub Pages site (Jekyll, Just the Docs).
- `tools/`: Node.js 24 scripts (`.mjs`) and PowerShell scripts that build, compare and play-test.

No game files, compilers or third-party DLLs are in the repository; the game goes in `Summoner/` (git-ignored). The
only third-party source is AMD's FidelityFX Super Resolution 1 shader headers (MIT), in `src/decomp/modern/shaders/fsr/`.

## Layout

| Path | Contents |
|---|---|
| `src/decomp/game/`, `src/decomp/vsdk/` | the game and the Volition engine (VSDK), C++ |
| `src/decomp/include/` | headers; `DECISIONS.md` = numbered decisions (`D1234`) referenced from comments |
| `src/decomp/experimental/` | the features of the fixed and experimental builds (`FIXED_BUILD`; autosave and the tentacle grab `EXPERIMENTAL_BUILD`) |
| `src/decomp/modern/` | the in-source host for clang builds (`MODERN_BUILD`): display, keys, brightness, registry, agent mode, Bink via FFmpeg, the Vulkan renderer (`vk_*.cpp`, `shaders/`) |
| `src/decomp/vc6-matches.txt` | the functions that match byte for byte; the sweep checks against it |
| `src/link/` | link support for the standalone exes (stubs, no original code) |
| `src/summonerfix/`, `src/summonerfix-config/` | SummonerFix and its config app (C, llvm-mingw) |
| `testsaves/` | purpose-made test saves (README): fights from the boss list, and `playthrough/`, the main story in order |
| `symbols/sum.symbols` | every Sum.exe name: the source of truth for names (`node tools/symbols.mjs`) |
| `docs/sum-exe/` | one page per game system; `game-bugs.md` = the 357 bugs with ids (C/G/K + number) |
| `docs/future/modern-port.md` | the clang / 64-bit port (rungs L1-L4), with a summary at the top |
| `docs/future/vulkan-renderer.md` | L4's first module, the Vulkan renderer (in `main` since 2026-10-27): design, milestones M0-M5 |

## Build configurations

`node tools/link-build.mjs [--config C] [--compiler clang] [--arch x64]`

| Define | Set by | Meaning |
|---|---|---|
| (none) | `--config shipped` | the original code; the byte-match reference |
| `FIXED_BUILD` | `fixed`, `experimental` | bug fixes, each marked `// FIXED_BUILD: <bug id> <[Fixes] key>`, and since 2026-11-01 the features that do not change how the game plays (widescreen, game options, controller, frame cap, vsync, hi-res interface, level reverb, dgVoodoo), and the opt-in `[Cheats]` (2026-11-03) |
| `EXPERIMENTAL_BUILD` | `experimental` | features that change the game: autosave and the Tentacle Beast grab, marked `// EXPERIMENTAL_BUILD: <feature> <Key>` |
| `MODERN_BUILD` | `--compiler clang` | portable forms of VC6-only code, and the host in `src/decomp/modern/` |
| `_WIN64` | `--arch x64` | 64-bit; `PTR32` pointer fields in relocated file formats, `POOL64` pools, `LAYOUT_CHECK_32` |

The 64-bit **player build** is `--config experimental --compiler clang --arch x64`.

## Environment

| Variable | Value | Used by |
|---|---|---|
| `VC6_DIR` | the `VC98` folder of VC6 with SP5's compiler (`c2.dll` 8966) | every VC6 build, the sweep, the 32-bit clang build (headers, libs) |
| `CLANG_DIR` | llvm-mingw's `bin` (LLVM 16+; 20260908 used) | clang builds, SummonerFix |
| `FFMPEG_DIR` | FFmpeg n9.0 LGPL shared, extracted | 64-bit movies |
| `VULKAN_SDK` | the LunarG Vulkan SDK (1.4.363.0 used; its installer sets it) | the Vulkan renderer of the 64-bit build (headers, `glslc`, validation layer) |
| `SUMMONER_GAME`, `SUMMONER_EXE` | a test folder and the exe in it | `tools/agent.mjs` and the check tools |
| `VC6_STRICT=1` | | the sweep's exact mode (use it) |

## Invariants: check before every commit

`node tools/check.mjs` runs 1 (when game, engine or header source changed), 3 and 4's path check, plus a
cross-check of the setting names; GitHub Actions (`.github/workflows/ci.yml`) runs it without the sweep on every push
and builds SummonerFix, the save editor and the 64-bit exes. 2 stays manual.

1. `VC6_STRICT=1 node tools/vc6-sweep.mjs --check src/decomp/vc6-matches.txt` must end with `0 lost` (5,618).
   Code for the other builds goes under its define, so the shipped configuration's bytes never change.
2. The configurations you touched build with `unresolved 0; multiply defined 0`.
3. `node tools/docs-check.mjs` reports no broken link or missing front matter.
4. Never commit game files, binaries, compilers, DLLs, a player's saves or the texture pack; never personal paths or names.
   The one kind of save allowed (owner's decision, 2026-11-03): the purpose-made test saves in `testsaves/`.

## Writing documentation

Rules: [docs/contributing.md](docs/contributing.md). Key ones:
- Every claim is tagged **Observed** (read or run), **Inferred** (concluded, not run) or **Unknown**.
- Dates are absolute (2026-10-26).
- Corrections stay visible.
- New tools go into `docs/tools.md`.
- A bug goes into `docs/sum-exe/game-bugs.md` only once confirmed in the code and in the game.
The docs are also the project's log: long pages are chronological under a summary.

## Testing the game (agent mode)

`node tools/agent.mjs start|run "cmds"|stop` drives a running exe through `agent\cmd.txt` / `agent\result.txt`.
Command reference: `docs/tools.md`, section `tools/agent.mjs`. Some commands:
- `info` (state, screen, fps), `capture NAME`, `click X Y`, `key NAME`, `postkey NAME`;
- `peek` / `poke ADDR` (Sum.exe addresses); `party` (each member's position, layer, HP, flags, in both hosts: use it,
  not `peek` at structure offsets, which differ in the 64-bit exe); `doors [R]` (nearby doors: index, position,
  layer, open or closed, distance) and `dooropen I 1|0` (both hosts);
- `level NAME SCRIPT`, `npcgoto NAME`, `summon N`, `firewall N`, `shop NAME`, `savenow`, `speedkey 1|0`.

Coordinates are capture pixels (half the window).

Gotchas, each found the hard way:
- The game is single-instance: `agent.mjs stop` before starting another exe, or the new one exits with code 1.
- `summonerfix.ini`: a second `[Debug]` section is ignored. Set keys inside the first.
- Esc on the main menu quits the game. `agent.mjs start` already waits out the intro movies.
- `level` during a cutscene crashes even the original. Wait for state 4 with `game_flags` (0x5fbfd8) bit 0x20 clear.
- Quicksave refuses in combat and in some areas. Use `savenow`.
- Clicks toward a point behind the camera barely move the party (the HUD hides the bottom of the screen). Turn the
  camera first: `postkey Left 3000` is about half a circle (Left/Right arrows).
- A click on the ground selects whoever stands under the cursor, and the save keeps the selected member. Click
  Joseph's portrait before `savenow` (or use `testsaves.mjs place`).
- `talkto` skips hidden characters since 2026-11-09. A harness command that reaches what a player cannot see or click
  proves nothing about the game (G312 was withdrawn for that).
- `talkto` while a conversation is still open (screen 13) crashes even the original in `dialogue_frame`. Page with
  `dlg` until mode 0 or state 0 first.
- Use test saves only (`[Debug] TestSaves=1`, folder `savetest\`), never the player's `savegame\`.
- link-build recompiles when sources, the headers they include (since 2026-11-03), defines or clang flags change.
  Shader changes need `--recompile`.
- Git Bash's `sed -i` and `node -e` mangle backslashes and `\b` in replacement text. Edit files that contain them
  with an editor or a script file.
- The "gamma" console command only stores the value; `gr_gamma` is 0x5a6064.
- The sweep reads `Summoner/Sum.exe` of the checkout it runs in. In a git worktree there is none, and every unit
  reports "lost" (an ENOENT error per unit): copy `Sum.exe` into the worktree's `Summoner/` first (git-ignored).

## Evidence and history

- `src/decomp/NOTES-*.md`: the per-round notes of the byte matching.
- `docs/decomp-plan.md`: the whole decompilation story.
- `docs/hybrid-review.md`: the decompilation errors found by running the code.
- `src/decomp/include/DECISIONS.md`: why each matching device exists.
