# Phase 3: new_automap.cpp

Written from build/sum.asm (`tools/asm.mjs fn`) and the raw .rdata/.data bytes of Sum.exe only (a small
scratch byte reader for the jump tables, the button table and the layout tables). No Summoner 2 or Dash
Faction code was read. `new_automap.cpp` passes the requested `i686-w64-mingw32-g++ -fsyntax-only -Wall
-Wno-write-strings ...` with no warnings.

Headers (now): the include tree, see "Header conversion" at the end. As written: `stubs_menus_dialogue.h`,
`ui_screen.h`, `stubs_gamecreate_journal.h` (RealtimeTimer, gr clip,
gr_split_str, key_to_ascii, class savegame, KEY_R/I/BACKSPACE/ENTER), `stubs_buysell_itemtrade.h`
(gr_screen_width/height, min/max), `stubs_charselect_levelselect.h` (level_table_display_name, XSTR),
`symbols/types.h` (Character, Player, vector3, level_script_info, console_command), plus the new
`stubs_automap.h` (guard STUBS_AUTOMAP_H) and the unit header `new_automap.h`.

## new_automap.cpp (0x401e80-0x404871; .rdata 0x57e6b8-0x57e99b; .data 0x588bc0-0x588e03; .bss 0x5b29d8-0x5b2db7)

| Address | Function | Confidence |
|---|---|---|
| 0x401e80 / 0x401e90 | init_realtimetimer_5b2b38 thunk + initialiser (compiler: `automap_label_blink_timer`) | exact |
| 0x401ea0 / 0x401eb0 | init_colors_5b29e0 thunk + initialiser (compiler: `automap_label_type_colors[2]`) | exact |
| 0x401ef0 | automap_load_bitmaps | likely (order of the max() sums / mask-scale quotients) |
| 0x402250 | automap_init | exact |
| 0x402280 | automap_select_label | likely (expression order of the box position) |
| 0x4023b0 | automap_center_on_player | exact |
| 0x402490 | automap_labels_update_positions | exact |
| 0x402560 | automap_show_player_layer | likely (chained stores?) |
| 0x4025a0 | automap_set_layer | exact (tail jump into update_layer_buttons) |
| 0x4025f0 | automap_update_layer_buttons | exact |
| 0x402680 | automap_frame | exact |
| 0x402760 | automap_handle_click | likely (shared key tails, the doubled type test on Delete) |
| 0x402d80 | automap_map_point_revealed | likely (clamp shape) |
| 0x402e90 | automap_update_buttons | exact |
| 0x402f10 | automap_button_action | exact |
| 0x402fb0 | automap_layer_down | exact |
| 0x402fd0 | automap_layer_up | exact |
| 0x403000 | automap_drag_scroll | likely (min / max form) |
| 0x403100 | automap_add_label_at_mouse | exact |
| 0x403240 | automap_mouse_to_map_cell | exact |
| 0x403300 | automap_map_to_world | likely (z step computed first) |
| 0x403350 | automap_label_text_input | likely (blink toggle as if / else) |
| 0x403510 | automap_label_box_hover | exact |
| 0x4035c0 | automap_labels_hover | exact |
| 0x403690 | automap_draw | exact |
| 0x403750 | automap_draw_map | likely (cross-jumped map draw, operand order of sums) |
| 0x403ff0 | automap_draw_label | likely |
| 0x404310 | automap_draw_title | exact |
| 0x404460 | automap_get_panel_width | exact |
| 0x404470 | automap_mouse_in_panel | exact |
| 0x4044a0 / 0x4044b0 / 0x4044d0 | init_ccmd_map_mask thunk, initialiser, cmd_map_mask | exact |
| 0x404590 / 0x4045a0 / 0x4045c0 | init_ccmd_map_grid thunk, initialiser, cmd_map_grid | exact |
| 0x404680 / 0x404690 / 0x4046b0 | init_ccmd_show_coordinates thunk, initialiser, cmd_show_coordinates | exact |
| 0x404770 / 0x404780 / 0x4047a0 | init_ccmd_reset_map_labels thunk, initialiser, cmd_reset_map_labels | exact |
| 0x404860 / 0x404870 | init_empty_404870 thunk + empty initialiser | unsure (not reproduced, Q3) |

46 functions: 34 exact, 10 likely, 2 unsure.

Data (*new* = name not in sum.symbols):
- .rdata 0x57e6b8 `automap_button_defs[9]` (*new*; const, unlike the other screens), 0x57e8f8
  `automap_view_pos[3][2]`, 0x57e910 `automap_view_size[3][2]`, 0x57e928 / 0x57e93c the camera and
  You-R-Here file names as `const char[]` (pushed as .rdata addresses), 0x57e94c `automap_label_wrap_w[3]`,
  0x57e958 `automap_label_margin` {1, 0, 1, 1} (Q4), 0x57e968 `automap_label_box_gap[3]`, 0x57e974
  `automap_title_y[3]`, 0x57e980 `automap_layer_text_y[3]` (all *new*).
- .data 0x588bc0 `automap_mask_bm_name` (*new*), Map_mask = true, 0x588bc8 `automap_label_bm_names[2]`
  (*new*), Show_coordinates = true, then the literals (variables' literals in reverse order; "|" at 0x588c10
  is reused by three later units).
- .bss: as listed in the file. Unnamed addresses: 0x5b29e4 (second colour), 0x5b2b54 / 0x5b2b58
  `automap_mask_tile_w/h`, 0x5b2b6c `automap_mask_scale`, 0x5b2da0 `automap_camera_bm_w` (written only).
  0x5b29fc and 0x5b2cbc are 4-byte holes nothing references (each sits before an 8-aligned console_command
  or the button array).
- extern from automap.cpp: automap_labels, automap_num_labels (tagged new_automap.cpp in sum.symbols, but
  they are at 0x5f9838 / 0x5f9e64 next to the map corners in that unit's .bss), corners, reveal grid size.

## Open questions

1. **.bss order versus definition order.** The four console_command objects sit between the screen's
   variables (0x5b2a00, 0x5b2b60, 0x5b2ca8, 0x5b2da8), while their initialisers are the last functions of the
   unit, right before each command function, which suggests the objects are defined at the end of the source
   (as a macro: object + function). The two can only both hold if VC6 does not lay .bss out in definition
   order. The file defines everything in address order (like player.cpp's ccmd_max_skills).
2. **Initialiser order.** static_initializers entry 1 is the timer (0x5b2b38), entry 2 the colours (0x5b29e0):
   the timer is defined first in the original, the opposite of their .bss order. Same question as 1.
3. **init_empty_404870** (entry 7, after reset_map_labels): an empty dynamic initialiser, so a static object
   with an empty inline constructor defined after the last command. No .bss byte of this unit is left for it
   unless it is a 1-byte object in one of the two holes. Not written.
4. **automap_label_margin.** Four ints read individually (not folded), so a const struct (or an array indexed
   by constants). The code uses +0 for both the x and (in the unedited box) the y text offset, and +0xc instead
   of +8 for the cursor y without a newline; member names are a guess and the "wrong" uses are copied as is.
5. **automap_get_zoom** is my inline function: the same resolution switch with 1.0 / 1.25 / 1.6 is expanded in
   nine functions, stored as an immediate when the value must survive a call, kept on the FPU stack otherwise.
   A macro would give the same code.
6. **Masked map mode** is the immediate 0x4028821 (GR_MODE_AUTOMAP_MASKED); probably a gr__Mode built from
   constants in the source.
7. `automap_label_text` is 0x120 bytes (0x5b2a14-0x5b2b33), the same as draw_label's stack buffer; an odd size.
8. `automap_label_alloc` (automap.cpp) returns 0, not -1, when 32 labels exist, so add_label_at_mouse would
   overwrite label 0; the savegame cap (0x80) is only checked for custom labels.
9. KEY_I passes the address of a `false` bool (stored in the dead key argument slot) to
   interface_set_screen(1, ...): supports the `void *` second argument of stubs_menus_dialogue.h.

## Wrong or missing shared declarations (not changed)

- `stubs_player_effectmem.h`: `console_command` is declared as 0x10 bytes; this unit's .bss shows 0xc
  (ccmd_map_grid 0x5b2a00, automap_centered 0x5b2a0c; ccmd_show_coordinates 0x5b2b60, next variable 0x5b2b6c),
  as `symbols/types.h` says. The 0x10 spacing seen in player.cpp is probably 8-byte alignment.
- `symbols/types.h` `console_command` and the `Color` of stubs_menus_dialogue.h (and the other stubs) have no
  constructors; this unit needs both (0x50a180, 0x5067f0). Declared through layout-identical wrappers
  `console_command_t` / `Color_ctor` in stubs_automap.h.
- `class savegame` (stubs_gamecreate_journal.h) has only get_quest_bit; the automap label members are on the
  wrapper `savegame_automap` (SAVEGAME_AUTOMAP).
- sum.symbols: `automap_labels` comment "type +0, level +8, position +0xc" -> type +0, world x +4, world z +8,
  layer +0xc, text +0x10 (32 bytes). `savegame_automap_label_add/remove` "(level, layer, ...)": the second
  argument is a level_script_find_index slot. `level_table_find_id` (formerly `level_is_endgame`) returns the level_table index of a level
  (or -1), not an endgame test. `gr_rect` takes six arguments (a float last). `automap_label_type_colors` (formerly `ui_color_red_full`) is element 0
  of this unit's two-colour label array. `automap_label_alloc` "max 32": returns 0 when full.

## Header conversion (2026-10-15)

The unit now compiles with `-I src/decomp/include` only. stubs_automap.h is gone: the unit header is
`summoner/interface/new_automap.h`; the label array, corners, reveal grid and `automap_label` (0x30) are in
the new `summoner/automap.h` (automap.cpp); the rest went to the headers of their units (DECISIONS.md D43).
Resolved from the list above:
- `console_command` is 0xc bytes (D40); `console_command_t` is a typedef.
- `Color` has the constructor 0x5067f0 (D41); `Color_ctor` is a typedef.
- The savegame label members are on `class savegame`; `SAVEGAME_AUTOMAP` is `savegame_state` (D46).
- `game_camera_heading` is an alias of game_camera.yaw (+0x180, D49).
- Code changes: `cur_character->pos[0]` / `[2]` became `.x` / `.z` (D42); the colour array is named
  `automap_label_type_colors` after sum.symbols (formerly ui_color_red_full, element 0 only).
