---
title: Decompilation plan
nav_order: 6.4
---

# Decompilation plan
{: .no_toc }

1. TOC
{:toc}

**Decision (2026-10-12):** first **map the whole binary**: every function, global and structure
named, typed and documented. Structure that work so it can later grow into a **byte-matching
decompilation** (C++ that the original compiler turns back into an identical Sum.exe, as the
Summoner 2 project does). Start with the **engine core** (`vsdk`), where reference projects
already supply names.

## Starting facts

| Fact | Value | Source |
|---|---|---|
| Compiler | Microsoft Visual C++ 6.0: C++ objects from cl 12.00 build 8966 (348), 8168 (1), 8047 (8); C objects build 8047 (161); masm 6.14/6.15 (38); linker 6.00 | Rich header (**observed**) |
| Size | 6,612 functions, 5,728 strings, `.text` 1.49 MB | Ghidra via REA |
| Debug info | None (no PDB path) | **Observed** |
| C runtime | Statically linked (MSVC 6 CRT, roughly `0x56a000`+ ; *inferred* from `__ftol`, `_strstr`, `__alldiv` locations) | Ghidra |
| Original source tree | `D:\projects\Summoner\pccode\` with `vsdk\` (shared engine), `engine\`, `summoner\`, `levelscripts\` | Assert/error strings |

Source files named in Sum.exe's own strings (35 of them):

| Folder | Files |
|---|---|
| `vsdk\os` | `cmdline.cpp`, `memory.cpp`, `registry.cpp`, `stringpool.cpp` |
| `vsdk\gr` | `gr.cpp`, `gr_font.cpp`; `directx\gr_direct3d.cpp`, `glide\gr_glide.cpp`, `opengl\gr_opengl.cpp` |
| `vsdk\ca` | `character.cpp`, `character_instance.cpp`, `skeleton.cpp` |
| `vsdk\bmpman` | `bmpman.cpp`, `targautils.cpp` |
| `vsdk\ui` | `ui_container.cpp`, `window.cpp` |
| `vsdk\math`, `vsdk\parse`, `vsdk\geom`, `vsdk\vfile\packfile` | `matrix.cpp`, `parse.cpp`, `effect_mem.cpp`, `file_packfile.cpp` |
| `engine\objects` | `item_paging.cpp`, `items.cpp`, `living_entity.cpp`, `objects.cpp` |
| `engine\ai` | `boss_ai.cpp`, `pathfinding.cpp`, `pathfinding_visibility.cpp` |
| `engine\...` | `characterinfo\characterinfo.cpp`, `gamesound\gamesound.cpp`, `s3d\s3d.cpp`, `rendering\levelrender.cpp` |
| `summoner\player` | `player.cpp` |
| `levelscripts` | `conversation.cpp`, `script_internal.cpp`, `scripts\level_scripts_common.cpp` |

## Phase 0: infrastructure

Do this first, because everything else depends on it.

1. **Symbol file as the source of truth.** REA's Ghidra sessions are ephemeral: names set in
   one session are gone in the next. So names live in the repository, in
   `symbols/sum.symbols`, one line per address:

   ```
   0x501510 FrameDone            func  vsdk\os\frametime.cpp  ; S2 name, verified
   0x5a5740 reported_frametime   float vsdk\os\frametime.cpp
   0x25d8d30 gr_screen           gr::Screen vsdk\gr\gr.cpp   ; Dash Faction layout
   ```

   Types (structures such as `gr::Screen`) go in `symbols/types.h`.
2. **Apply symbols everywhere automatically.** One script pushes the file into a REA/Ghidra
   session (`set_addresses_names`) and into x32dbg as labels, so names appear in both the
   static decompiler and the live debugger.
3. **Progress report.** A script that counts named and unnamed functions per address range
   and per source folder (the equivalent of the Summoner 2 project's `ninja progress`).
4. **Translation-unit map.** VC6 links each `.cpp` file's functions contiguously. Using
   assert strings (each names its `.cpp`), map **address ranges to source files**. This
   shows where every module sits, and it is exactly what a later matching decompilation
   needs to split the binary into objects.
5. **Mark the C runtime** range as library code so it is excluded from progress counts.

## Phase 1: engine core (`vsdk`)

Order chosen by how much the reference projects already tell us
([Engine references](reference/engine-references.md)):

| Order | Module | Why this order | Reference |
|---|---|---|---|
| 1 | `os`: frametime, timer, registry, cmdline, memory, stringpool, console | Partly mapped already; Summoner 2 has the source | S2 `src/*.cpp`, DF `rf/os/` |
| 2 | `parse` (`.tbl` tables) and `vfile\packfile` (`.vpp`) | Unlocks reading game data files | S2 `parse.cpp`, `file_packfile.cpp`; DF `vpackfile.cpp` |
| 3 | `gr` (+ OpenGL, Direct3D, Glide back-ends), `bmpman`, `gr_font` | Needed for widescreen and UI work; `gr::Screen` already identified | DF `rf/gr/`, `rf/bmpman.h` |
| 4 | input (keyboard, mouse, DirectInput) | Needed for custom key bindings | DF `rf/input.h`, `input/key.cpp` |
| 5 | `math`, `geom`, `ca` (character animation), `ui` | Large and shared by gameplay code | DF headers, S2 symbols |

Per module, the routine is:

1. Find the module's code: assert strings, imports, and calls from functions already named.
2. Compare it with the reference source or headers. Name the functions, globals and
   structures, and record the evidence level (**Observed** when the code matches the
   reference, **Inferred** otherwise).
3. Check uncertain cases live in x32dbg: breakpoints, memory reads, logging breakpoints.
4. Write a module page under `docs/sum-exe/` and add the names to `symbols/`.
5. Note anything that matters for SummonerFix or the [Roadmap](roadmap.md).

## Phase 2: game layers

`engine\objects`, `engine\ai`, `summoner\player`, `engine\characterinfo`, and
`levelscripts` (Summoner's own code, with no reference project), working outward from the
engine calls named in Phase 1 and from the [game-state table](sum-exe/main-loop.md#game-state-table-0x588a70).

## Phase 3: pilot for a byte-matching decompilation

When a small translation unit is fully mapped (for example `frametime.cpp`):

- Obtain the **VC6 compiler** (cl 12.00.8966 and the matching linker). Visual C++ 6.0 is
  proprietary, so this needs a legitimately licensed copy; we can't simply download it.
- Rewrite one translation unit in C++ and compare the compiled object with the original
  bytes function by function (objdiff, as the Summoner 2 project does).
- If it matches, set up the project layout and progress tracking for the rest.

## Status

| Phase 0 step | Status (2026-10-13) |
|---|---|
| 1. Symbol file | Done: `symbols/sum.symbols` and `symbols/types.h`; `node tools/symbols.mjs check` |
| 2. Apply symbols | x32dbg: `apply-x32dbg` (names show in the live disassembly). Ghidra: REA's Windows provider is **read-only** ("does not admit database mutation"), so `tools/decomp.mjs` applies names to Ghidra's output instead, including struct fields (`gr_screen.max_w`) |
| 3. Progress report | Done: `symbols/ghidra-functions.tsv` (6,329 functions) and `node tools/symbols.mjs progress` |
| 4. Translation-unit map | Done: [Source-file map](sum-exe/source-map.md) |
| 5. Mark C runtime | Done: it starts at `0x56c740` (*inferred*: first non-16-byte-aligned code; Ghidra's library names begin at `__ftol` `0x56c7e4`) |

**Baseline (2026-10-13):** 6,329 functions. 5,531 of them are game/engine code below
`0x56c740`; the rest are C runtime and library code (Ghidra recognised 441 by signature).
**43 / 5,531 named (0.8%).**

The 2026-10-12 REA blocker ("Open Windows path component failed [C:]") was cleared by a
Windows restart.

## Where we start

Phase 0 is complete. Next: Phase 1, module 1 (`os`: timer, registry, memory, cmdline,
stringpool, console), each compared with the Summoner 2 decomp source.
