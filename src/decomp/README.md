# Decompiled source (Phase 3 pilot)

C++ source rewritten from Sum.exe's disassembly, one file per original translation unit, for a future
byte-matching build with Visual C++ 6.0 (see [docs/decomp-plan.md](../../docs/decomp-plan.md), "Phase 3" and
"Toolchain"). **None of it has been compiled with VC6 yet**, so "exact" in the notes means "the control flow,
constants and globals match the disassembly", not "the bytes match".

Rules:
- Everything is written from Sum.exe's own code (`tools/asm.mjs`). The Summoner 2 decompilation has no licence and
  is used for names only, never for code.
- Function and global names come from `symbols/sum.symbols`; each function carries its address and a confidence
  comment (exact / likely / unsure).
- The `NOTES*.md` files list the functions with addresses and confidence, and the open questions.

| File | Original unit | Functions | Exact / likely / unsure |
|---|---|---|---|
| `vsdk/frametime.cpp` | `frametime.cpp` (`0x501470`-`0x5017e6`) | 14 | see `vsdk/NOTES.md` (29 / 8 / 0 with timestamp) |
| `vsdk/timestamp.cpp` | `timestamp.cpp` (`0x5017f0`-`0x501b16`) | 23 | |
| `vsdk/registry.cpp` | `vsdk\os\registry.cpp` | 14 | 10 / 4 / 0 |
| `vsdk/cmdline.cpp` | `vsdk\os\cmdline.cpp` | 20 | 14 / 6 / 0 |
| `vsdk/gr_font.cpp` | `vsdk\gr\gr_font.cpp` | 17 | 9 / 7 / 1 |
| `vsdk/effect_mem.cpp` | `vsdk\geom\effect_mem.cpp` (`0x51bb70`-`0x51e018`) | 38 | 20 / 18 / 0 (13 vfx_instance functions not written yet) |
| `game/player.cpp` | `player.cpp` (`0x433340`-..) | 27 | 12 / 15 / 0 |
| `game/summoner.cpp` | `summoner.cpp` (with WinMain `0x43e5e0`) | 61 | 53 / 8 / 0 |
| `vsdk/window.cpp` | `vsdk\ui\window.cpp` (`0x528430`-`0x52bf92`) | 40 | 22 / 15 / 3 |
| `vsdk/character_instance.cpp` | `vsdk\ca\character_instance.cpp` (`0x5194b0`-`0x51b6a1`) | 52 | 33 / 15 / 4 |
| `vsdk/ui_container.cpp` | `vsdk\ui\ui_container.cpp` (`0x5534f0`-`0x553b9b`) | 10 | 5 / 5 / 0 (vtable `0x581f40` slot order) |
| `vsdk/targautils.cpp` | `vsdk\bmpman\targautils.cpp` (`0x5549c0`-`0x5565d6`) | 21 | 8 / 13 / 0 |
| `vsdk/bmpman.cpp` | `vsdk\bmpman\bmpman.cpp` | 64 | 42 / 21 / 1 |
| `vsdk/gr_opengl.cpp` | `vsdk\gr\opengl\gr_opengl.cpp` (the renderer SummonerFix enables) | 56 | 41 / 15 / 0 |
| `vsdk/gr_direct3d.cpp` | `vsdk\gr\directx\gr_direct3d.cpp` (`0x534a20`-`0x536944` is a second file of `vsdk\gr\directx`, see source-map.md) | 65 of 66 | 37 / 25 / 3 |
| `vsdk/memory.cpp` | `vsdk\os\memory.cpp` | 19 | 18 / 1 / 0 |
| `vsdk/stringpool.cpp` | `vsdk\os\stringpool.cpp` | 16 | 14 / 2 / 0 |
| `vsdk/plainstring.cpp` | `plainstring.cpp` (folder not known; Summoner 2 name) | 20 | 12 / 8 / 0 |
| `vsdk/parse.cpp` | `vsdk\parse\parse.cpp` (TextFile) | 51 | 32 / 18 / 1 |
| `vsdk/matrix.cpp` | `vsdk\math\matrix.cpp` | 48 | 27 / 20 / 1 |
| `vsdk/skeleton.cpp` | `vsdk\ca\skeleton.cpp` | 27 | 15 / 12 / 0 |
| `vsdk/character.cpp` | `vsdk\ca\character.cpp` | 46 + 3 | 25 / 22 / 2 |
| `game/new_credits.cpp` | `new_credits.cpp` (`0x408590`-`0x408d72`) | 11 | 7 / 4 / 0 |
| `game/new_help.cpp` | `new_help.cpp` (`0x40bf30`-`0x40cb55`) | 16 + 5 initialiser pairs | 5 / 11 / 0 |
| `game/gamesound.cpp` | `gamesound.cpp` (`0x468c80`-`0x469720`..) | 20 | 14 / 5 / 1 |
| `game/item_paging.cpp` | `item_paging.cpp` | 12 | 5 / 3 / 4 |
| `game/new_playmenu.cpp` | `new_playmenu.cpp` | 16 | 13 / 3 / 0 |
| `game/new_mainmenu.cpp` | `new_mainmenu.cpp` | 13 | 9 / 4 / 0 |
| `game/new_dialogue.cpp` | `new_dialogue.cpp` | 6 | 3 / 3 / 0 |
| `game/new_assess.cpp` | `new_assess.cpp` | 20 | 12 / 8 / 0 |
| `game/new_pickup.cpp` | `new_pickup.cpp` | 22 | 12 / 10 / 0 |
| `game/new_gamecreate.cpp` | `new_gamecreate.cpp` | 22 | 16 / 5 / 1 |
| `game/new_journal.cpp` | `new_journal.cpp` | 29 | 17 / 12 / 0 |
| `game/new_buysell.cpp` | `new_buysell.cpp` | 44 | 32 / 11 / 1 |
| `game/new_itemtrade.cpp` | `new_itemtrade.cpp` | 40 | 29 / 11 / 0 |
| `game/new_options.cpp` | `new_options.cpp` | 37 | 19 / 18 / 0 |
| `game/new_saveload.cpp` | `new_saveload.cpp` | 43 | 25 / 18 / 0 |
| `game/new_status.cpp` | `new_status.cpp` (`0x42b1c0`-`0x42e62e`) | 57 | 33 / 22 / 2 |
| `game/new_charselect.cpp` | `new_charselect.cpp` | 36 | 20 / 16 / 0 |
| `game/new_levelselect.cpp` | `new_levelselect.cpp` | 29 | 15 / 13 / 1 |
| `game/new_automap.cpp` | `new_automap.cpp` (`0x401e80`-`0x404871`) | 46 | 34 / 10 / 2 |
| `game/new_inventory.cpp` | `new_inventory.cpp` (`0x414c80`-`0x418572`) | 78 | 49 / 27 / 2 |
| `game/new_hud.cpp` | `new_hud.cpp` (`0x40cb60`-`0x41075a`) | 70 | 45 / 25 / 0 |
| `game/new_skill.cpp` | `new_skill.cpp` | 39 | 28 / 11 / 0 |
| `game/new_spell.cpp` | `new_spell.cpp` | 38 | 31 / 7 / 0 |
| `game/new_tutorial.cpp` | `new_tutorial.cpp` | 15 + 7 initialiser pairs | 12 / 10 / 0 |
| `game/new_gamelist.cpp` | `new_gamelist.cpp` | 29 + 2 initialiser pairs | 14 / 15 / 0 |
| `game/new_pxochat.cpp` | `new_pxochat.cpp` | 57 + 14 initialiser pairs | 20 / 36 / 1 |
| `game/multisave.cpp` | `multisave.cpp` (`0x46cc60`-`0x46da80`) | 10 | see `NOTES-multi.md` (38 / 23 / 1 for the four files) |
| `game/multitrade.cpp` | `multitrade.cpp` | 26 | |
| `game/multichat.cpp` | `multichat.cpp` | 13 | |
| `game/multicontainer.cpp` | `multicontainer.cpp` | 13 | |
| `game/gametarget.cpp` | `gametarget.cpp` | 26 | 12 / 14 / 0 |
| `game/multi_chattracker.cpp` | `multi_chattracker.cpp` | 20 | 11 / 9 / 0 |
| `game/multispell.cpp` | `multispell.cpp` | 22 | 16 / 6 / 0 |
| `game/objects.cpp` | `objects.cpp` (`0x4697e0`-`0x46b45b`, unoptimised) | 48 | 37 / 10 / 1 |
| `game/items.cpp` | `items.cpp` (`0x449d90`-`0x44d9ee`) | 41 | 31 / 10 / 0 |
| `game/characterinfo.cpp` | `characterinfo.cpp` | 122 | 81 / 26 / 15 (the 15 are empty Mempool initialisers) |
| `game/s3d.cpp` | `s3d.cpp` (`0x46dd30`-`0x470560`) | 52 | 37 / 15 / 0 |
| `game/levelrender.cpp` | `levelrender.cpp` (`0x48db30`-`0x492af0`) | 30 of 39 (the three draw-list back ends left) | 13 / 5 / 12 |
| `game/spells.cpp` | `spells.cpp` | 44 of 53 (cast pipeline left) | 30 / 14 / 0 |
| `game/combat.cpp` | `combat.cpp` (`0x464010`-`0x469010`) | 59 of 63 | 38 / 20 / 1 |
| `game/boss_ai.cpp` | `boss_ai.cpp` (`0x4a04b0`-`0x4a3575`) | 59 | 42 / 17 / 0 |
| `game/ai.cpp` | `ai.cpp` (+ the 3 cast_effect functions after it, D1905) | 110 | 76 / 30 / 4 |
| `game/pathfinding.cpp` | `pathfinding.cpp` (`0x49c840`-`0x4a04af`) | 40 | 20 / 20 / 0 |
| `game/conversation.cpp` | `levelscripts\conversation.cpp` (`0x4d7b30`-..) | 32 | 22 / 10 / 0 |
| `game/conversationctl.cpp` | `conversationctl.cpp` (`0x4d87d0`-`0x4daf50`) | 40 | 23 / 16 / 1 |
| `game/level_scripts.cpp` | `levelscripts\level_scripts.cpp` (`0x4ceec0`-`0x4d6651`, savegame code included) | 124 + 34 initialiser helpers | 96 / 27 / 1 |
| `game/level_script_khosani_lab.cpp` | `level_script_khosani_lab.cpp` | 48 | 33 / 15 / 0 |
| `game/level_script_worldmap.cpp` | `level_script_worldmap.cpp` | 31 | 18 / 13 / 0 |

**Progress:** `node tools/decomp-progress.mjs` counts the written functions per original file.

**Headers.** `include/` is one consistent header set laid out like the original source tree
(`vsdk/os/memory.h`, `vsdk/vfile/file.h`, `engine/objects/living_entity.h`, `summoner/player/player.h`, ...):
see [include/README.md](include/README.md) for the scheme and what each header covers, and
[include/DECISIONS.md](include/DECISIONS.md) for how the conflicts between the old per-batch stub headers
were resolved (with the disassembly evidence). The unit headers (`timestamp.h`, `window.h`, `new_*.h`, ...)
live in the same tree; the `.cpp` files stay in `vsdk/` and `game/` and include headers by path from the
include root. `symbols/types.h` stays the C reference; the headers mirror its layouts (same offsets, size
checks with `LAYOUT_CHECK`) and add the thiscall methods.

Syntax check (mingw is not the target compiler; it only catches syntax and type errors; VC6 accepts string
literals as `char*`):

```
i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I src/decomp/include src/decomp/game/new_credits.cpp
```
