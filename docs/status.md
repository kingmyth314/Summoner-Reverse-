---
title: Current state
nav_order: 1.6
---

# Current state (2026-11-10)

What exists, what passes and what is open, one row per part. The pages linked hold the evidence and the history;
how to reproduce each line: [Reproduce and play](quickstart.md).

## Builds

| Build | Command | Output | Passes | Open |
|---|---|---|---|---|
| Byte-match reference | `node tools/vc6-sweep.mjs --check src/decomp/vc6-matches.txt` (VC6 SP5) | a report | 5,618 of 5,926 compiled functions give Sum.exe's bytes; 7,725 functions decompiled in all | the 305 that differ are listed per unit in the sweep's report ([plan](decomp-plan.md)) |
| Shipped (VC6) | `link-build --config shipped` | `Sum-rebuilt.exe` | plays the game; save and memory comparison over 28 levels | none known |
| Fixed (VC6) | `link-build --config fixed` | `Sum-fixed.exe` | 53 fixes in the source: 46 game bugs, 4 timing and start-up bugs, 2 launcher settings, movies under OpenGL; since 2026-11-01 also the experimental features that do not change how the game plays (widescreen, game options, controller, frame cap, vsync, hi-res interface, level reverb, dgVoodoo) | per-fix reproductions; a real controller (the controller code is driven by agent mode's `pad` since 2026-11-03) ([fixed build](fixed-build.md)) |
| Experimental (VC6) | `link-build --config experimental` | `Sum-exp.exe` | the fixed build plus autosave and the Tentacle grab | vsync and the autosave rules checked 2026-11-03 ([fixed build](fixed-build.md#checked-2026-11-03)) |
| 32-bit clang (L1, L2) | `--compiler clang` | `*-clang.exe` | plays; warning-clean except 6 known; NPC goal parity fixed | test timing only ([modern port](future/modern-port.md)) |
| **64-bit player build** (L3) | `--config experimental --compiler clang --arch x64` | `Sum-exp-clang-x64.exe` | runs without SummonerFix; movies (FFmpeg); 45 of 48 levels as the original; saves match within float rounding and load across exes; the shop, a six-minute fight, conversations, world map | a real controller (slider, monitor ramp, mouse bindings and the controller code through `pad` checked 2026-11-03, [fixed build](fixed-build.md#checked-2026-11-03)) |
| Vulkan renderer (L4, in `main` since 2026-10-27) | the player build with `VULKAN_SDK` set; `[Display] Renderer=Vulkan` | the same exe | M0-M5: the game plays under Vulkan; menus and brightness identical to OpenGL in captures; levels within OpenGL's run-to-run noise; all 48 soak levels as under OpenGL, 0 validation errors; saves unaffected; minimise, focus, resize and movies checked | RenderScale (FSR 1 below 1, supersampling above; the 48-level soak at 0.75 and 1.5 as at 1, 2026-11-01), frame-pacing measurements, 24-bit movies (2026-11-01); variable refresh did not engage over the owner's HDMI connection (Intel GPUs use DisplayPort Adaptive-Sync or HDMI 2.1 VRR), untested over DisplayPort ([Vulkan renderer](future/vulkan-renderer.md)); the save thumbnail at game widths above 1024, wrong under both renderers, fixed in the widescreen code (on a side branch from 2026-10-27, in `main` since 2026-11-03: [experimental build](experimental-build.md#the-save-thumbnail-at-a-wide-game-width-fixed-2026-10-27)) |

## Other parts

| Part | State | Page |
|---|---|---|
| SummonerFix (`dinput.dll` for the original game) | Windows 10/11, OpenGL, widescreen, key rebinding, controller, 40+ bug fixes with switches; since 2026-11-01 the config app sets the 64-bit build's Vulkan, RenderScale and FSR keys and LevelReverb, and window sizes larger than the screen are shrunk to fit | [SummonerFix](summonerfix/index.md) |
| Game documentation | 35 topic pages on Sum.exe; 356 bugs checked in the code (2026-11-09: G312, Torvel's double, withdrawn the day it was added, the game hides him and only the test command reached him; G313, the King of Talas does not appear in Saanavarh until the late act 4 visit, found when the test command stopped reaching hidden characters, fixed: `[Fixes] TriggeredShow`) | [Sum.exe](sum-exe/index.md), [bugs](sum-exe/game-bugs.md) |
| Save editor | `SummonerSaveEditor.exe`, writes new saves only | [Save editor](save-editor.md) |
| Cheats | `[Cheats]` (2026-11-03), off by default, in SummonerFix and every rebuilt exe: an experience multiplier, a gold key, a skill-point key; set in the config app | [SummonerFix](summonerfix/index.md#cheats-cheats-2026-11-03) |
| Side quests | all 56 of `events.tbl` looked at, every one with dialogue played in the original exe (2026-11-09): 51 close with their reward (the escort too, followed from behind; The King of Talas only at the late Saanavarh visit, bug G313, fixed by `[Fixes] TriggeredShow`; The Lord Chamberlain's Men through the lanterns on 2026-11-10), 2 never close, 3 cannot appear; findings: quest items must be identified, Torva's 125,000 gold; corrected the same evening: the test command had reached hidden characters (G312 withdrawn, six quests replayed) | [Side quests](sum-exe/side-quests.md); `tools/testsaves/sidequests.mjs` |
| Test saves | `testsaves/` in the repository: 19 saves before the boss-list fights (2026-11-03; 22 since 2026-11-08 with Korel, Zane and Azha, and 12 repaired: enemies beside the party, companions below their level; 15 and 18 moved on 2026-11-10 so that a short walk starts each fight, 15 at the riders' bridge, with `testsaves.mjs place`) and 43 saves of the main story in order, Masad to the last fight (2026-11-04; 44 with pt-42b, 2026-11-08, the one save that reaches both endings; the potions the tools added had 0 charges and could not be used until 2026-11-08), made in agent mode from a new game; every one loads in the original exe | `testsaves/README.md`; [Tools](tools.md#toolstestsavestestsavesmjs-the-test-saves) |
| Texture pack | made from your own install with Real-ESRGAN, never distributed | [Texture upscaling](future/texture-upscale.md) |
| Multiplayer | documented (66 packet types); not ported to 64-bit | [Multiplayer](sum-exe/multiplayer.md) |

## Next

The whole list of what is left, from the review of 2026-11-10: [Roadmap](roadmap.md#what-is-left-review-of-2026-11-10).

1. A player's test of the 64-bit build; before it, a run of every rebuilt exe against the test saves (load, play a
   short stretch, compare with the original).
2. L4, the idiomatic rewrite: the Vulkan renderer, M0-M5 done, merged into `main` on 2026-10-27; since then RenderScale with FSR 1 (soaked at 0.75 and 1.5), frame-pacing measurements and 24-bit movies; variable refresh waits for a DisplayPort connection ([Vulkan renderer](future/vulkan-renderer.md)). Sound stays on DirectSound (the owner's choice, 2026-11-01); the experimental build reads each level's reverb file (`LevelReverb`, heard only with EAX) ([modern port](future/modern-port.md)).
