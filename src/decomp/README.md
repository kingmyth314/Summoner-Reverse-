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
| `vsdk/window.cpp` | `vsdk\ui\window.cpp` (`0x528430`-`0x52bf92`) | 40 | 22 / 15 / 3 |
| `vsdk/character_instance.cpp` | `vsdk\ca\character_instance.cpp` (`0x5194b0`-`0x51b6a1`) | 52 | 33 / 15 / 4 |
| `vsdk/ui_container.cpp` | `vsdk\ui\ui_container.cpp` (`0x5534f0`-`0x553b9b`) | 10 | 5 / 5 / 0 (vtable `0x581f40` slot order) |
| `vsdk/targautils.cpp` | `vsdk\bmpman\targautils.cpp` (`0x5549c0`-`0x5565d6`) | 21 | 8 / 13 / 0 |
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

**To fix when the headers are merged** (found by the agents, not yet resolved):
- `interface_set_screen(int id, ...)`: the second argument is `int` in two stub headers and `void*` in
  `stubs_menus_dialogue.h`; the call sites push both constants and pointers.
- `Color` is defined in more than one stub header.
- `mp_player` in the stubs lacks the name at `+0x34`.
- `File`: `stubs_sound_items_registry.h` declares only `find()`; `stubs_container_targa.h` defines a fuller `File`.
- `symbols/types.h` declares `ui_element` as a C struct, but the game uses it as the C++ base class of `ui_window`
  and `ui_gadget`, so a file that includes `types.h` cannot also declare those classes.

**Progress:** `node tools/decomp-progress.mjs` counts the written functions per original file (2026-10-15: 704 of
8,642, 407 exact, 261 likely, 12 unsure).

**Headers.** `include/` holds one stub header per batch (`stubs_*.h`) that declares the engine and UI functions
the files call, plus `ui_screen.h` and `dialogue_box.h`. The signatures are inferred from the call sites. They
overlap and will be merged into proper engine headers once there is a real build.

Syntax check (mingw is not the target compiler; it only catches syntax and type errors; VC6 accepts string
literals as `char*`):

```
i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -I src/decomp/include -I src/decomp/vsdk -I src/decomp/game -I symbols src/decomp/game/new_credits.cpp
```
