---
title: Reproduce and play
nav_order: 1.5
---

# Reproduce and play
{: .no_toc }

How to get from a fresh clone of this repository to each result it claims: the byte-matching proof, the 32-bit
fixed and experimental builds, the 64-bit player build, and the test harness. Each recipe ends with what you
should see. Checked from a fresh clone on 2026-10-26 (**Observed**): the sweep gave 5,613 matches and the 64-bit
player build linked with nothing unresolved. Where things stand now: [Current state](status.md).

1. TOC
{:toc}

## What you supply

Nothing below is in the repository: no game files, compilers or third-party DLLs.

| Item | Version used | Where it goes | Needed for |
|---|---|---|---|
| **Summoner** PC, v1.40 (sold in digital stores) | `Sum.exe` 1,785,856 bytes; this project's copy has SHA-256 `f22c84a17ae4023a9113a68c76647feee41044ecde56ffb358744db47f8bf9d3`, which carries a no-CD edit in `cd_find_drive` / `cd_set_root` ([decompilation plan](decomp-plan.md)) | the install copied (or hard-linked) to `Summoner\` inside the repository (git-ignored) | everything |
| **Node.js** | 24 | on `PATH` | every tool in `tools/` |
| **Visual C++ 6.0 + Service Pack 5** | `c2.dll` 12.00.8966 | anywhere outside the repository; `VC6_DIR` = its `VC98` folder | the byte match, the 32-bit VC6 builds; also the VC6 headers and libraries the 32-bit clang build links against ([Setup](setup.md), [Sources](reference/sources.md)) |
| **llvm-mingw** (UCRT, x86_64 host) | 20260908 (clang 23.1.1); LLVM 16 or later needed (`-ffp-eval-method`) | `CLANG_DIR` = its `bin` folder | the clang builds, 32 and 64-bit; SummonerFix |
| **FFmpeg**, LGPL shared build | n9.0 (BtbN) | `FFMPEG_DIR` = the extracted folder (headers in `include\`); its five DLLs beside the 64-bit exe | movies in the 64-bit build (without it they are skipped) |
| **Vulkan SDK** (LunarG, optional) | 1.4.363.0 | its installer sets `VULKAN_SDK` | the Vulkan renderer in the 64-bit build (without it, `Renderer=Vulkan` falls back to OpenGL) |
| libjpeg 6b source (optional) | `jpegsrc.v6b.tar.gz` | `third_party\jpeg6b\jpeg-6b` | JPEG screenshots in the rebuilt exes (without it a stub is linked and says so) |

The downloads, with URLs and checksums, are in [Sources](reference/sources.md). The analysis tools (REA, Ghidra,
x64dbg) are only needed to repeat the reverse engineering, not to build: [Tooling setup](setup.md).

## Step 1: verify the byte matching

```powershell
$env:VC6_DIR = 'C:\path\to\vc6sp5\VC98'
$env:VC6_STRICT = '1'
node tools/vc6-sweep.mjs --check src/decomp/vc6-matches.txt
```

Compiles every unit with VC6 SP5 and compares each function with `Summoner\Sum.exe`. **Expect:**
`259 units: 5613 match, 2 wrong target, 310 differ, 1 not found` and `5613 matched there, 5613 now, 0 lost`.
A different `Sum.exe` than the one above may lose a few matches (the no-CD bytes are already accounted for).

## Step 2: build the 32-bit exes (VC6)

```powershell
node tools/link-build.mjs --config shipped        # build\link\Sum-rebuilt.exe: the decompiled game as it shipped
node tools/link-build.mjs --config fixed          # build\link-fixed\Sum-fixed.exe: 53 fixes in the source (46 game bugs)
node tools/link-build.mjs --config experimental   # build\link-exp\Sum-exp.exe: plus the new features
```

**Expect** `259 units (... 0 failed); unresolved 0; multiply defined 0`. These run like the original: in the game
folder with SummonerFix's `dinput.dll` ([SummonerFix](summonerfix/index.md), built with
`tools/build-summonerfix.ps1`). What each configuration changes: [builds](builds.md),
[fixed build](fixed-build.md), [experimental build](experimental-build.md).

## Step 3: build and install the 64-bit player build

```powershell
$env:CLANG_DIR = 'C:\path\to\llvm-mingw\bin'
$env:FFMPEG_DIR = 'C:\path\to\ffmpeg-n9.0-latest-win64-lgpl-shared-9.0'
node tools/link-build.mjs --config experimental --compiler clang --arch x64
```

Output: `build\link-exp-clang-x64\Sum-exp-clang-x64.exe`. It needs no SummonerFix: its display, key bindings,
brightness, settings and widescreen are in its own source ([the player build](future/modern-port.md#the-player-build-the-64-bit-experimental-exe-2026-10-26)).
**Install** into a Summoner folder, beside `Sum.exe`:

| File | From |
|---|---|
| `Sum-exp-clang-x64.exe` | the build folder |
| `avformat-63.dll`, `avcodec-63.dll`, `avutil-61.dll`, `swscale-10.dll`, `swresample-7.dll` | FFmpeg's `bin` folder |
| `summonerfix.ini` | the one SummonerFix uses (`src/summonerfix/summonerfix.ini` is the template); the 64-bit exe reads the same keys |
| `SummonerFixConfig.exe` (optional) | `tools/build-summonerfix.ps1`; edits the ini, `F10` opens it in game |
| `Sum-exp-clang-x64.addr` (only for tests) | the build folder: agent-mode commands with Sum.exe addresses need it; the game starts and plays without it (checked 2026-10-26) |

Start `Sum-exp-clang-x64.exe`. Settings come from `summonerfix.ini` (`[Display] GameWidth`, `[KeyBindings]` ...),
not from the registry. `summonerfix.log` beside it says what the build found (`modern host`, `registry: game WxH`,
`movies: ...`). Saves go to `savegame\` as in the original, and are interchangeable with the 32-bit builds' saves.

**Not yet tested in 64-bit:** the brightness slider in the options screen, the monitor-ramp fallback, mouse-button
bindings, the config key, a real controller.

## Step 4: run the tests

The tests drive the game through **agent mode** ([Tools](tools.md#toolsagentmjs-agent-mode)): `tools/agent.mjs` writes commands
to `agent\cmd.txt` and the game answers in `agent\result.txt`. Use a **test folder**, never your own saves:

1. A folder outside the repository (the tools default to `..\..\SummonerToolchain\rebuilt-test`, or `--game DIR`
   / `SUMMONER_GAME`) holding hard links to the `.vpp` archives, copies of `data\`, `summoner.cfg`, `binkw32.dll`,
   `eax.dll`, an ini, and test saves in `savetest\` (`[Debug] TestSaves=1` makes every exe use that folder).
2. `Sum.exe` copied there as `Sum-orig.exe`, with SummonerFix's `dinput.dll`, for the original's runs.
3. The exe under test with its `.addr` file. The `.addr` maps Sum.exe addresses to the rebuilt exe's; link-build
   writes it only after a full hybrid build (`node tools/hybrid-build.mjs src/decomp/game/*.cpp src/decomp/vsdk/*.cpp --out build/hybrid-all`),
   so run that once.

| Test | Command | Expect |
|---|---|---|
| Save state per level | `node tools/link-savecheck.mjs OUT Wolong:Wolong ... --exe Sum-fixed-clang-x64.exe --fixes` | the 64-bit exe: about 5-35 dwords beyond the original runs' spread per level, all monster and NPC positions and goals (float rounding), plus a 144-dword block of party timers on some runs (test timing) ([details](future/modern-port.md#float-expressions-in-double--ffp-eval-methoddouble-2026-10-26)) |
| Every level | `node tools/link-soak.mjs OUT --exe Sum-exp-clang-x64.exe` | 45 of 48 levels; the same 3 crash in the original (a character killed by the test's fire walls whose death move has no animation) |
| Screens | `node tools/link-screencheck.mjs OUT --exe ...` | identical menus and prologue; small differences in animated views |

**Practical rules** (each cost a failed run once): stop one exe (`node tools/agent.mjs stop`) before starting
another, since the game is single-instance; set ini keys inside the existing section, as a second `[Debug]` section
is ignored; Esc on the main menu quits the game.
