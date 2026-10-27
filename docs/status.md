---
title: Current state
nav_order: 1.6
---

# Current state (2026-10-26)

What exists, what passes and what is open, one row per part. The pages linked hold the evidence and the history;
how to reproduce each line: [Reproduce and play](quickstart.md).

## Builds

| Build | Command | Output | Passes | Open |
|---|---|---|---|---|
| Byte-match reference | `node tools/vc6-sweep.mjs --check src/decomp/vc6-matches.txt` (VC6 SP5) | a report | 5,613 of 5,926 compiled functions give Sum.exe's bytes; 7,725 functions decompiled in all | the 310 that differ are listed per unit in the sweep's report ([plan](decomp-plan.md)) |
| Shipped (VC6) | `link-build --config shipped` | `Sum-rebuilt.exe` | plays the game; save and memory comparison over 28 levels | none known |
| Fixed (VC6) | `link-build --config fixed` | `Sum-fixed.exe` | 53 fixes in the source: 46 game bugs, 4 timing and start-up bugs, 2 launcher settings, movies under OpenGL | per-fix reproductions ([fixed build](fixed-build.md)) |
| Experimental (VC6) | `link-build --config experimental` | `Sum-exp.exe` | controller, autosave, frame cap, Tentacle grab, hi-res interface, widescreen in source | vsync and the autosave skip rules untested ([experimental build](experimental-build.md)) |
| 32-bit clang (L1, L2) | `--compiler clang` | `*-clang.exe` | plays; warning-clean except 6 known; NPC goal parity fixed | test timing only ([modern port](future/modern-port.md)) |
| **64-bit player build** (L3) | `--config experimental --compiler clang --arch x64` | `Sum-exp-clang-x64.exe` | runs without SummonerFix; movies (FFmpeg); 45 of 48 levels as the original; saves match within float rounding and load across exes; the shop, a six-minute fight, conversations, world map | slider, ramp, mouse bindings, controller untested in 64-bit |
| Vulkan renderer (L4, in `main` since 2026-10-27) | the player build with `VULKAN_SDK` set; `[Display] Renderer=Vulkan` | the same exe | M0-M5: the game plays under Vulkan; menus and brightness identical to OpenGL in captures; levels within OpenGL's run-to-run noise; all 48 soak levels as under OpenGL, 0 validation errors; saves unaffected; minimise, focus, resize and movies checked | on the branch `l4-present` (2026-10-27): RenderScale (FSR 1 below 1, supersampling above) and frame-pacing measurements; variable refresh itself needs the owner's monitor at its full rate and an elevated PresentMon ([Vulkan renderer](future/vulkan-renderer.md)) |

## Other parts

| Part | State | Page |
|---|---|---|
| SummonerFix (`dinput.dll` for the original game) | Windows 10/11, OpenGL, widescreen, key rebinding, controller, 40+ bug fixes with switches | [SummonerFix](summonerfix/index.md) |
| Game documentation | 35 topic pages on Sum.exe; 355 bugs checked in the code | [Sum.exe](sum-exe/index.md), [bugs](sum-exe/game-bugs.md) |
| Save editor | `SummonerSaveEditor.exe`, writes new saves only | [Save editor](save-editor.md) |
| Texture pack | made from your own install with Real-ESRGAN, never distributed | [Texture upscaling](future/texture-upscale.md) |
| Multiplayer | documented (66 packet types); not ported to 64-bit | [Multiplayer](sum-exe/multiplayer.md) |

## Next

1. A player's test of the 64-bit build.
2. L4, the idiomatic rewrite: the Vulkan renderer, M0-M5 done, merged into `main` on 2026-10-27; since then RenderScale with FSR 1 and frame-pacing measurements on the branch `l4-present`; variable refresh waits for the owner's monitor setting ([Vulkan renderer](future/vulkan-renderer.md)).
