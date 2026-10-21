---
title: Fixed and experimental builds
nav_order: 6.47
---

# Fixed and experimental builds (plan)

**Status:** plan, agreed 2026-10-21 with the advisor's review. **Decided by the user (2026-10-21): one tree with
three build configurations, and VC6 SP5 for now.** Nothing is built yet.

**Summary:** The decompiled source already links into a complete `Sum.exe` of its own, nothing taken from the
original ([Decompilation plan](decomp-plan.md), "The standalone link"), and every boss fight has run on it. That
makes two new builds possible. A **fixed build** is the original game with its bugs fixed in the source, plus what
a modern PC needs to run it, so that SummonerFix's patches are no longer necessary. An **experimental build** adds
new things on top: the [future projects](roadmap.md#future-projects) and controller support. The byte-matching
reference stays exactly as it is.

## Decisions (made 2026-10-21)

**1. One tree with three build configurations (recommended), or three git branches.**
`tools/link-build.mjs` already builds variants of the same source with `--define` (`SHIPPED_NOCD`, `REBUILT_OPENGL`,
recorded in `build/link/defines.txt`). The proposal is to extend that into three configurations:

| Configuration | Defines | Output | Purpose |
|---|---|---|---|
| `shipped` | none (the current default) | `Sum-rebuilt.exe` | the matching reference: the original game's code, byte for byte where proven |
| `fixed` | `FIXED_BUILD` (+ `SHIPPED_NOCD`, `REBUILT_OPENGL`) | `Sum-fixed.exe` | bugs fixed, runs on a modern PC without SummonerFix |
| `experimental` | `FIXED_BUILD`, `EXPERIMENTAL_BUILD` | `Sum-exp.exe` | the fixed build plus new features, each also behind its own switch |

A fix is written once, next to the original code, as `#ifdef FIXED_BUILD ... #else <original> #endif` (not `FIXED`: Windows' `wingdi.h` declares a type of that name). When a
decompilation fix (a D-number) lands on main, all three configurations get it automatically, and the strict byte-match
sweep keeps checking the `shipped` configuration. Git branches are still used, but only as short-lived feature
branches that merge back into main.

The alternative, long-lived `fixed` and `experimental` git branches, keeps the reference source free of `#ifdef`s.
Its cost: every later decompilation fix on main has to be merged into both branches by hand, and the byte-match
proof cannot check them.

**2. Compiler: stay on VC6 SP5 (recommended) for now.** The 5,445 proven functions are proven only under that
compiler; a modern compiler changes float precision, struct layout and every matching device at once. A port to a
current compiler (and later 64-bit or other platforms) is listed as an experimental item, not a prerequisite.

## The fixed build

In this order:

1. **What a modern PC needs to run the game at all**, ported from SummonerFix into the source: the 2x game speed
   fix (timing), OpenGL as the default renderer with its texture-size cap, the no-CD start (exists:
   `SHIPPED_NOCD`), the crash on Start New Game without an audio device (C1), intros skipping after about 25 days of
   uptime, the launcher-free start, game resolution, windowed and scaled output, the 3D aspect, the brightness fix,
   and key bindings.
2. **SummonerFix's gameplay fixes** (`bugfix.c`, about 45, each with a `[Fixes]` switch today): Robe of Restoration,
   the save-overwrite question, Phoenix Inferno, the Luminar beam crash, spell cooldowns, the loose gold pile, Leech,
   the detection cone, boss music and the rest of the list in [SummonerFix](summonerfix/index.md#fixes). Each is
   ported with its SummonerFix patch as the specification and its documented in-game reproduction as the test.
3. **Bugs no SummonerFix patch covers yet**, from [Bugs in the original game](sum-exe/game-bugs.md) in the order of
   [Bug fix priorities](sum-exe/bug-priorities.md). C317 first (the Mist Elemental crashes the game when a cutscene
   starts: live, a crash, a one-line NULL test in `summon_stash_for_cutscene`). C317 should also become a SummonerFix
   patch now, since players of the original game use SummonerFix, not this build.
4. **Undo the matching devices** ([Roadmap](roadmap.md), D12900, D15400, D11703, D15902, D16500, D14204): plain C++
   in place of code written to reproduce the original's bytes. They change no behaviour, so they are done when a
   function is touched anyway, not as a project of their own.
5. **Not included until seen in game:** candidates such as `pf_check_direct_walk`'s look-ahead (D15600). A fix needs
   a reproduction first ([verification rule](sum-exe/game-bugs.md)).

Multiplayer is left as the original until single player is done (the project's order: multiplayer last).

## The experimental build

In this order:

1. **Controller support, tier 1: a controller drives the cursor and the buttons.** The PC game is click-to-move;
   the arrow keys only turn and zoom the camera ([Controls](sum-exe/controls.md)). Tier 1 reads XInput and feeds the
   game's own input path: the left stick moves the cursor, the buttons click and send keys, the right stick turns and
   zooms the camera. The hooks exist already, built for agent mode: the cursor override in SummonerFix's `display.c`
   and `key_mark` / `mouse_mark_button` in `keys.c`. No game logic changes, so the same code can also go into
   SummonerFix for the original game. Estimated in days.
2. **Controller, tier 2: direct movement with the stick**, as in the PS2 version. This is new game code: a per-frame
   travel goal for the leader in the stick's direction, plus menus usable without a cursor. It comes after tier 1
   has proved the input path.
3. **The future projects:** autosave (the rules for when saving is allowed are known), the Tentacle Beast's grab
   and throw (`[Experimental] TentacleGrab` in SummonerFix today, ported), a save editor window, a console window and
   cheats page, wider display options (resolution list, vsync, frame rate, field of view, UI scale), dgVoodoo 2 for
   the Glide and Direct3D renderers.
4. **Later:** a modern-compiler port, and from there 64-bit or other platforms.

Every experimental feature also has its own switch in the ini, off by default, so a problem can be ruled out by
turning it off.

## Testing

The existing checks treat any difference from the original as a finding, which is wrong for a build that differs
on purpose. The rule:

- **Fixed build:** compare `Sum-fixed.exe` with `Sum-orig.exe` running **SummonerFix with its fixes on**
  (`[Fixes] Enabled=1`). Both then carry the same fixes, so the save, memory and screen checks
  (`tools/link-savecheck.mjs`, `link-memcheck.mjs`, `link-screencheck.mjs`) work unchanged. A fix that goes beyond
  SummonerFix declares its expected difference and gets its own scenario (its reproduction, now passing).
- **Experimental build:** with every experimental switch off it must pass the fixed build's checks; each feature is
  then tested on its own.
- **Invariant, every commit:** `tools/vc6-sweep.mjs --check src/decomp/vc6-matches.txt` (with `VC6_STRICT=1`) on the
  `shipped` configuration; the strict match count (5,445) must not drop. The fixed and experimental code must never
  change what the reference configuration compiles to.

## First steps

Done 2026-10-21 (built and checked; not run in game, see [Fixed build changes](fixed-build.md)):

1. `link-build.mjs --config shipped|fixed|experimental` (defines `FIXED_BUILD` / `EXPERIMENTAL_BUILD`: `FIXED` is
   taken by a type in Windows' `wingdi.h`), and `link-savecheck` / `link-screencheck --exe NAME --fixes` to test
   `Sum-fixed.exe` against `Sum-orig.exe` with SummonerFix's fixes on.
2. C317 as a SummonerFix patch (`[Fixes] SummonStashNoMove`) and as the first `FIXED_BUILD` change.
3. The other 42 `bugfix.c` fixes ported, and the handshake that keeps SummonerFix from patching code the source
   already fixes (`Sum-fixed.fixes`). See [Fixed build changes](fixed-build.md).

Next:

4. The modern-PC layers as `FIXED_BUILD` code. **Timing and start-up done 2026-10-21** (G1, G2, G3, C1 and the
   1024x768 default; SummonerFix stands down for each through `.fixes`; see [Fixed build changes](fixed-build.md#modern-pc-layers)).
   Still open: the display layer (scaling, mouse mapping, aspect, widescreen, brightness) and key bindings.
5. Controller tier 1 (written as a SummonerFix prototype, not yet loaded: see the SummonerFix docs).
6. Every in-game test above, once SummonerFix can load again.
