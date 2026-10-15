---
title: "Interface screens"
parent: Sum.exe
nav_order: 6
---

# Interface screens

**Summary:** The game's menus are 23 numbered **screens**, switched by
`interface_set_screen(id, arg)` (`0x412620`). It calls the current screen's *leave* function
and then the new screen's *init*. The current id is `interface_cur_screen` (`0x5bd814`); 0 means
no menu, just the HUD. [Game states](main-loop.md#game-state-table-0x588a70) open screens
on entry, and each state's frame handler calls `interface_frame` (`0x4119a0`).

## Screens

The source files come from the [string-id file map](source-map.md#functions-placed-by-string-id)
and the alphabetical link order of the `new_*.cpp` files. **O** means the screen's own
strings confirm it; **I** means it is placed by position.

| Id | Init | Leave | Screen | File | Conf. |
|---|---|---|---|---|---|
| 0 | `0x40cef0` | – | HUD (no menu) | `new_hud.cpp` | I |
| 1 | `0x414fc0` | `0x4158b0` | Inventory: full-screen panel, "Inventory", "Party Items" | `new_inventory.cpp` | O |
| 2 | `0x42b480` | `0x42b910` | Character (status): full-screen panel, "Character", "Skills", "Resistances" | `new_status.cpp` | O |
| 3 | `0x402250` | – | Map: left side panel with the automap | `new_automap.cpp` | O |
| 4 | `0x4207d0` | `0x420960` | Pickup | `new_pickup.cpp` | I |
| 5 | `0x429b30` | – | Spells: left side panel | `new_spell.cpp` | O |
| 6 | `0x427520` | `0x427df0` | Skills: left side panel, "Combat" | `new_skill.cpp` | O |
| 7 | `0x404bd0` | `0x4051a0` | Buy/sell | `new_buysell.cpp` | I |
| 8 | `0x4256a0` | `0x425c80` | Save/load (arg: which) | `new_saveload.cpp` | I |
| 9 | `0x4216f0` | `0x4217b0` | Play menu | `new_playmenu.cpp` | I |
| 10 | `0x41ab10` | `0x41aef0` | Quest Journal: full-screen panel, "Main Quests", "Side Quests", "Description" | `new_journal.cpp` (*inferred* name) | I |
| 11 | `0x422fc0` | `0x4231a0` (`pxochat_leave_thunk`, jumps to `0x422370`) | PXO chat lobby | `new_pxochat.cpp` | I |
| 12 | `0x41f0f0` | `0x41fa60` | Summoner Options: full-screen panel (audio, gameplay, visual settings); opened by F2 and by the main menu's Options button (`0x41dbf4`) | `new_options.cpp` (*inferred* name) | O |
| 13 | `0x408f90` | `0x408fe0` | Dialogue (arg: character name): copies the name and starts the conversation (`0x4d7650`); `hud-dialoguebox` is loaded nearby (`0x408d80`) | `new_dialogue.cpp` (*inferred* name) | I |
| 14 | `0x41d8b0` | – | Main menu: "Title", plays `vlogo.bik` and `eaxlogo.bik` | `new_mainmenu.cpp` | O |
| 15 | `0x40a850` | – | Multiplayer game list | `new_gamelist.cpp` | I |
| 16 | `0x406590` | – | Multiplayer character select: "Multiplayer Char", `*.plr` | `new_charselect.cpp` | O |
| 17 | `0x409b10` | – | Multiplayer game creation | `new_gamecreate.cpp` | I |
| 18 | `0x41c080` | – | Multiplayer level selection: UI table "PXO_LevelSelect" (`0x41c450`); opened by state 14 | `new_levelselect.cpp` (*inferred* name) | O |
| 19 | `0x419380` | `0x419550` | Item trade | `new_itemtrade.cpp` | I |
| 20 | `0x401380` | `0x4017e0` | Assess (arg: target) | `new_assess.cpp` | I |
| 21 | `0x4085b0` | `0x4089c0` | Credits: `credits.tbl`, `Credits-Bkgrnd640/1024.tga` | `new_credits.cpp` | O |
| 22 | `0x40c7e0` | `0x40c850` | Help: `DemoHelp640.tga` | `new_help.cpp` | O |

A dash in the Leave column means an empty leave: `interface_set_screen` still calls `empty_function` (`0x4f13e0`)
for screens 0, 3, 5 and 14-18 (jump table `0x4129d8`), the folded copies of their empty `<screen>_leave`
functions. **Observed.** When the screen being left is not full-screen and the new one is 1, 2, 7 or 10,
`interface_set_screen` re-picks `status_character` (`0x5bd810`) if it is NULL: `cur_character` when it is the
main hero, else in multiplayer the local player's character unless hidden or a summon, else the first type-7
living entity of the level that is not a summon (hidden ones only on the world map). When none is found it
restarts itself with `(0, 0)` (`jmp 0x41262c` at `0x412770`, `0x4127b9`, `0x4127d3`). Screens 3, 5 and 6 clear
the overhead labels before their init, and `saveload_init`, `options_init` and `help_init` get "opened from
somewhere other than the main menu" (`prev != 14`) as their in-game flag. **Observed.**

Screens 1, 2, 3, 5, 6, 10 and 12 were confirmed from in-game frame captures (2026-10-13). The keys are I, R, M, C, S and F2 (Options); the journal was opened from the HUD. Correction: screen 12 is Options, not the journal; the journal is screen 10. The HUD is a bottom bar with: Menu/Map/Solo buttons, the portrait, four party slots, the compass, the Summoner emblem, action icons and quick slots. All 23 screens are now identified. The quest journal probably reads `qjournal.tbl` (*inferred* from the name).

**HUD action icons** (agent mode, 2026-10-13):

| Icon | Opens |
|---|---|
| Crossed swords | No screen (probably a combat-mode toggle) |
| Portrait | Character (2) |
| Scroll | Spells (5) |
| Book | Quest Journal (10) |
| Chest | Inventory (1) |
| Figure | Skills (6) |

## Per-screen functions

Each screen also has a per-frame input function and a draw function. `interface_frame` (`0x4119a0`) and `interface_draw` (`0x411cd0`) switch on `interface_cur_screen` to call them. They are named `<screen>_frame` (for example `inventory_frame`, `0x4158f0`) and `<screen>_draw` (`inventory_draw`, `0x417860`) in `symbols/sum.symbols`. A frame function takes the key code and returns it, or 0 when it consumed it. `interface_draw` draws the HUD (`hud_draw`, `0x40f6e0`) before the screen.

## The 3D view is clipped per screen

{: .important }
Corrected 2026-10-15 from the disassembly: `0x410510` is `hud_get_panel_height`, the height of the HUD bar,
not a panel width. The side panels use their own width functions, and the cutscene and full-screen cases
were missing.

`interface_set_view_clip` (`0x412220`) runs before the 3D view is drawn. It resets the clip and sets a new
one with `gr__set_clip` (`0x506840`). **Observed** (jump table `0x41238c`, case bytes `0x4123a4`):

- During a cutscene (`game_flags & 0x20`) the view is letterboxed to 2:1: the full width, a height of
  width / 2, centred vertically.
- With the HUD hidden (`ui_input_locked`, `0x5bd962`) and on the full-screen screens (1, 2, 7, 8, 10-12,
  14-18, 21, 22) the view is the whole screen.
- On the HUD (0), pickup (4), play menu (9), dialogue (13) and assess (20) the HUD bar's height
  (`hud_get_panel_height`, `0x410510`) is cut from the bottom.
- On the map (3), spells (5), skills (6) and item trade (19) the HUD bar is cut from the bottom and the side
  panel's width is cut from the left (`automap_get_panel_width` `0x404460`, `spells_get_panel_width`
  `0x42ada0`, `skills_get_panel_width` `0x429060`, `itemtrade_get_panel_width` `0x41a8b0`).

Outside cutscenes it then recomputes the vertical field of view for the new clip (`gr_fov_to_vertical`,
`0x50e5e0`, stored at `0x60a1a4`). The world is rendered beside the panels, not under them.
`interface_screen_is_fullscreen` (`0x412ab0`, case bytes `0x412ad8`) is false for exactly the screens that
get a cut (0, 3, 4, 5, 6, 9, 13, 19, 20) and true for all others. **Observed.**

This is one of the places that decides layout from the screen width, so widescreen support
would need to change it.
