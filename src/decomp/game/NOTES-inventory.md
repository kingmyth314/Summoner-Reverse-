# Phase 3: new_inventory.cpp

Written from build/sum.asm (`tools/asm.mjs fn`) and the raw .rdata/.data bytes of Sum.exe only (a small
scratch byte reader for the jump tables, the button table and the layout tables). No Summoner 2 or Dash
Faction code was read. `new_inventory.cpp` passes the requested `i686-w64-mingw32-g++ -fsyntax-only -Wall
-Wno-write-strings ...` with no warnings (checked against the repo at 73e07ab, where ItemType +0x10 is `mesh`).

Headers (now): the include tree, see "Header conversion" at the end. As written: `stubs_menus_dialogue.h`,
`ui_screen.h`, `stubs_gamecreate_journal.h`, `stubs_status.h` (existing),
plus the new `stubs_inventory.h` (guard STUBS_INVENTORY_H) and the unit header `new_inventory.h`.
`stubs_buysell_itemtrade.h` is not included: its `gr_light_desc` (with `f14`) clashes with the one in
`stubs_status.h`, and the inventory needs the stubs_status.h layout (+0x10 appearance, +0x1c model prefix).
`item_paging.h` and `timestamp.h` are not included either (Character / RealtimeTimer clashes).

## new_inventory.cpp (0x414c80-0x418572; .rdata 0x57f040-0x57fdbb; .data 0x58bfcc-0x58cbd7; .bss 0x5bd970-0x5c146b)

| Address | Function | Confidence |
|---|---|---|
| 0x414c80 / 0x414c90 | init_ccmd_show_ui thunk + initialiser (`console_command_M ccmd_show_ui(...)`) | exact |
| 0x414cb0 | cmd_show_ui | exact |
| 0x414d30 / 0x414d40 | init_empty_414d40 thunk + initialiser (read as inventory_cameras, Q1) | unsure |
| 0x414d50 / 0x414d60 | init_strptrs_5c1458 thunk + initialiser (inventory_action_strings) | likely |
| 0x414da0 / 0x414db0 | init_realtimetimer_5c0ffc thunk + initialiser (inventory_repeat_timer) | exact |
| 0x414dc0 | inv_cameras_parse_table | exact |
| 0x414de0 | inventory_load_bitmaps | likely (i == 0 test after the i == 27 branch) |
| 0x414fc0 | inventory_init | likely (dead found test, as status_init) |
| 0x4150c0 | inventory_page_in_equipped | likely (pointer-compare loop) |
| 0x415180 | inventory_update_species_tab | exact |
| 0x415220 | inventory_update_category_buttons | likely (switch on the loop index) |
| 0x4153d0 | inventory_category_first_index | likely (loop shape) |
| 0x415470 | inventory_update_scrollbar | likely (buysell Q2) |
| 0x415580 | inventory_update_trade_buttons | exact |
| 0x4155f0 | inventory_update_equip_slots | likely (statement order) |
| 0x415850 | inventory_count_rows | exact |
| 0x4158b0 | inventory_leave | exact |
| 0x4158f0 | inventory_frame | exact |
| 0x4159a0 | inventory_handle_click | likely (return tails, key table) |
| 0x416020 | inventory_get_list_item | exact |
| 0x416090 | inventory_equip_index_for_button | exact |
| 0x416110 | inventory_update_item_buttons | exact |
| 0x416200 | inventory_can_identify | exact |
| 0x416290 | inventory_hover_tooltips | exact |
| 0x416560 | inventory_show_item_info | exact |
| 0x4165c0 | inventory_page_up | exact |
| 0x4165f0 | inventory_page_down | exact |
| 0x416620 | inventory_button_action | exact |
| 0x4167e0 | inventory_goto_other_screen | exact |
| 0x416840 | inventory_goto_status | exact |
| 0x416860 | inventory_prev_member | exact |
| 0x416870 | inventory_next_member | exact |
| 0x416880 | inventory_scroll_up | exact |
| 0x4168a0 | inventory_scroll_down | exact |
| 0x4168d0 .. 0x4169d0 | inventory_goto_category0 .. 4 (5 functions) | exact |
| 0x416a10 | interface_close_screen | exact |
| 0x416a20 | inventory_update_buttons | exact |
| 0x416af0 | inventory_drag_scrollbar | likely |
| 0x416b80 | inventory_popup_result | likely (tails as breaks) |
| 0x416c10 | inventory_use_selected_item | likely (inline slot switch, `||` layout) |
| 0x416dc0 | inventory_use_list_item_begin | likely |
| 0x416eb0 | inventory_use_equipped_item_begin | likely |
| 0x416fb0 | inventory_identify_item | likely (`elapsed() == true`) |
| 0x417110 | inventory_confirm_destroy | exact |
| 0x417160 | inventory_confirm_destroy_cb | exact |
| 0x417170 | inventory_destroy_selected_item | likely (double item_can_destroy) |
| 0x417220 | inventory_drag_to_equip | likely (locals) |
| 0x4174a0 | inventory_equip_item | likely |
| 0x417630 | inventory_drag_from_equip | exact |
| 0x417740 | inventory_unequip_item | exact |
| 0x417790 | inventory_swap_rings | likely (jump-threaded final test) |
| 0x417860 | inventory_draw | exact |
| 0x417950 | inventory_draw_list_rows | likely (locals) |
| 0x417a60 | inventory_set_row_color | likely (switch vs ||) |
| 0x417b10 | inventory_draw_scrollbar | exact |
| 0x417bc0 | inventory_draw_item_model | likely |
| 0x417c50 | ui_render_item_mesh | likely (local order) |
| 0x417cd0 | inventory_item_view_offset | exact |
| 0x417da0 | inventory_model_view_begin | exact |
| 0x417df0 | inventory_model_view_end | exact |
| 0x417e10 | inventory_draw_character | likely (local order) |
| 0x417fe0 | inventory_render_character | likely (local order) |
| 0x418090 | inventory_draw_drag_item | exact |
| 0x418160 | inventory_draw_item_preview | exact |
| 0x4181d0 | inventory_preview_view_begin | likely (shared case tail) |
| 0x418430 | inventory_draw_equipped_hover | exact |
| 0x4184b0 | inventory_refresh_for_character | exact |
| 0x4184c0 | world_use_item_on_object | exact |
| 0x418500 | world_use_equipped_item_on_object | exact |
| 0x418540 | world_use_item_at_point | exact |

78 functions (8 of them compiler initialiser thunks / bodies): 49 exact, 27 likely, 2 unsure. Nothing left.

Facts:
- Button table (45 x 0x40 at 0x58c080, *new* name `inventory_button_defs`). Def flags: 1 paper-doll hot spot,
  2 act on release, 4 act on press with 500 / 100 ms auto-repeat (scroll arrows), 8 hover tooltip, 0x10
  usable in open mode 1. Action ids (HiLite + 1): 6 shop (when a shopkeeper is set) / journal (single
  player) / status, 8 status, 10 / 12 previous / next member, 14 / 16 scroll, 30 / 33 / 36 / 39 / 42
  category jumps, 44 close. Keys: Esc and I close (0x24), Q journal (single player only), R status, B shop
  (only with a shopkeeper), all 0x27. Mouse wheel pages.
- Paper-doll hot spots 18-27 -> equipment index (inventory_equip_index_for_button (formerly inventory_slot_for_item_slot)): 18 armour 7, 19 pants 8,
  20 boots 4, 21 / 22 rings 1 / 2, 23 neck 0, 24 weapon 5, 25 shield 6, 26 gauntlets 3, 27 heavy armour 7.
  ItemType.slot -> hot spot (drag_to_equip, preview_view_begin): 0 neck, 1 rings, 2 gauntlets, 3 boots,
  4 / 9 weapon (9 two-handed: no shield then), 5 shield, 6 armour, 7 pants, 8 heavy armour (covers armour
  and pants; hot spot 27 and picture 17 replace 18 / 19).
- Item action popup ids (inventory_action_strings, L692-696): 0 Info (identified), 1 Identify (enabled when
  inventory_can_identify), 2 Use (item_has_magic_info, not in open mode), 3 Destroy (list rows only, not in
  open mode, item_can_destroy), 4 Cancel. Identify uses the party member with the highest skill 0x14 whose
  Player.skill_cooldown[0x14] has run out (flags 0x88 excluded); failure shows L697 / L698.
- Use: spells with flags 2 (target character) or 4 (target location) start targeting
  (inventory_use_*_item_begin: hud_target_mode 0 / 1, pick radius = spell target_radius; spell 0x1d also
  pins the world pick on cur_character and sets the y radius to 1.0) unless the charge effect is
  self-only (+0x10); others are used at once on status_character.
- Weapons cannot be dragged off their hot spot (press on hot spot 24 is ignored); right click on an empty
  hot spot is impossible (update_equip_slots disables it), so the missing slot == -1 test there is harmless.
- Dropping a ring on the other ring hot spot swaps the rings (inventory_swap_rings (formerly inventory_unequip_both_hands)).
- The list draws ItemType names with the stack count right-aligned; red = weapon / armour / jewelry the
  character cannot equip (character_can_equip_item), blue = unidentified.
- Open mode (the inventory_init byte, `interface_set_screen(1, &byte)`): only unidentified items are rows,
  but inventory_category_first_index (formerly inventory_build_category) returns a party inventory index, so the category jumps scroll to the wrong
  row there (binary behaviour).
- Models: each equipped item is drawn in its slot (inventory_model_view_offset[res][equipment]) with a
  per-name correction for 35 items (0x57f148, all zero at 1024x768) and -15 px x for heavy armour at
  1024. The character's weapon / shield are drawn at char_info prop bones [0] and [15] (else [1]) through
  character_instance::get_bone_world_transform, skipped on level 0x18 in the servant disguise.
- .bss holes: 0x5c1019-0x5c101f (after inventory_open_mode) and 0x5c146c-0x5c146f (after the five action
  strings). ccmd_show_ui (0x5bd780, 0x10 bytes) sits between ui_choice_popup_flags and
  ui_choice_popup_mouse_mode in the interface unit's .bss, but its initialiser is this unit's first one.

## Wrong or incomplete shared declarations (not changed)

- `symbols/types.h`: `console_command` is a C struct (0xc bytes), but the objects are constructed through
  the thiscall ctor 0x50a180 and are 0x10 apart; stubs_player_effectmem.h / stubs_window_charinst.h declare a
  `class console_command` that cannot coexist with types.h. Shim `console_command_M` here.
- `stubs_status.h`: `ui_choice_popup_show(title, x, y, num, items, a, b, c)` - `c` is a `bool *` (the
  per-entry enabled array, sum.symbols ui_choice_popup_enabled); `a` is probably the int value array
  (ui_choice_popup_values). Passed as `(int)inventory_item_action_enabled` here.
- `stubs_status.h` and `stubs_buysell_itemtrade.h` both define `class gr_light_desc` (+0x14 `_unk14` vs
  `f14`); the status layout is the right superset. `party_inventory_entry` (buysell: `inv_item *`) is
  repeated here with `item_info *` (stubs_status.h); `item_charge_effect` repeated with `self_only` (+0x10)
  and `disposable` (+0x14). Merge needed.
- `src/decomp/game/new_buysell.cpp` buysell_goto_weapons .. goto_misc: `min(buysell_category_first_index(n),
  ...)` expands the macro and calls the function twice; the binary (0x405c50..) calls it once (a local).
  Fixed 2026-10-15 (the result is kept in a local `first`, as here).
- sum.symbols comments / names to fix:
  - `inventory_drag_start_mouse_y` (formerly `inventory_drag_start_thumb`) (0x5c0fbc) and `inventory_drag_start_thumb_y` (formerly `inventory_drag_start_y`) (0x5c1000) are swapped: handle_click
    stores game_mouse_y in 0x5c0fbc and the thumb y in 0x5c1000; drag_scrollbar subtracts 0x5c0fbc and adds
    0x5c1000 (used under the sum.symbols names here, with comments).
  - `inventory_equip_index_for_button` (formerly `inventory_slot_for_item_slot`) takes the hot-spot button index (18-27), not ItemType +0x1c.
  - `inventory_pressed_button` (formerly `inventory_selected_equip_slot`) is the pressed button index of any button (update_buttons fires
    button_action(it + 1)); `inventory_selection_is_list_row` (formerly `inventory_identify_pending`) means "opened from a list row" (1) vs a hot spot (0).
  - `inventory_update_category_buttons` (formerly `inventory_rebuild_lists`) only updates the category icons; `inventory_count_rows` (formerly `inventory_count_unidentified`) sets the row
    count; `inventory_update_trade_buttons` handles the member arrows 9 / 11; `inventory_swap_rings` (formerly `inventory_unequip_both_hands`)
    swaps the rings; `inventory_draw_list_rows` (formerly `inventory_draw_list_row`) draws all rows; `inventory_draw_drag_item` (formerly `inventory_draw_hover_item`) is the mode 4 drag
    preview; `inventory_category_first_index` (formerly `inventory_build_category`) returns the first index of the category.
  - `inventory_paused_game`: unpause is bit 0 (value 1), not "bit 1".
  - `ui_using_list_item_entry` (formerly `ui_using_list_item_index`) (0x5bd514) holds a party_inventory_entry pointer, not an int.
  - `character_equip_item` is (out_old1, out_old2, character, slot, item), not (..., character, item, ...).
  - `inventory_cameras` is 0x3628 bytes (same as status_cameras / buysell_cameras).
  - `ccmd_show_ui` is tagged module "-"; its initialiser (0x414c90) is in new_inventory.cpp.
- `equip_page_slots` (0x5819d4, *new*, .rdata near objects.cpp's vtable) is a const table shared with
  0x4afc4d: {10, 13, 14, 15, 12, -1, -1, 16, 11} item paging slots per equipment index.

## Open questions

- Q1: init_empty_414d40 (entry 50, a bare `ret`) is read as the initialiser of inventory_cameras through the
  inline empty `new_icam_table` constructor, as NOTES-status.md Q1 does for status_cameras.
- Q2: inventory_destroy_selected_item calls item_can_destroy twice on the same item; the source shape (two
  tests, or a macro / inline helper) is unknown.
- Q3: inventory_use_selected_item repeats the inventory_equip_index_for_button (formerly inventory_slot_for_item_slot) switch inline (jump table
  0x416d98) while other callers call the function; under /Ob1 that means the switch is written out again in
  the source (or the function is `inline` and VC6 declined some sites).
- Q4: inventory_identify_item compares `GameTimer::elapsed()` with `cmp al, 1`; written `== true`.
- Q5: world_use_*_on_object take any object with a handle at +0x80 (typed Character* here).

## Header conversion (2026-10-15)

The unit now compiles with `-I src/decomp/include` only. stubs_inventory.h is gone: the unit header is
`summoner/interface/new_inventory.h` (it no longer includes types.h); the declarations went to the headers
of their units (DECISIONS.md D43). Resolved from the list above:
- `console_command` is 0xc bytes (D40; the automap unit's .bss shows it); `console_command_M` is a typedef.
- One `gr_light_desc` (gr.h, D23): `light._unk14` became `light.f14` (D54). One `party_inventory_entry`
  (objects.h, D16) and one `item_charge_effect` (items.h; `self_only` stays a ubyte, D55).
- `character_equip_item` is (out_old1, out_old2, character, slot, item) in objects.h.
- `ui_using_list_item_entry` is a `party_inventory_entry*` (interface.h).
- The shims `CHARINST_M`, `AI_GOAL_M`, `INV_EQUIP` are identity macros over members (D45).
- `skill_is_available(int, Character *)` and `character_use_skill(int, void *target, Character *user)` are
  the signatures the disassembly gives (D48, new_skill.h).
- Code change: `*(vector3 *)cur_character->pos` became `cur_character->pos` (D42).
- Still open: `ui_choice_popup_show`'s last argument is the enabled `bool*` array; interface.h keeps `int`
  (types guessed), so the cast `(int)inventory_item_action_enabled` stays.
