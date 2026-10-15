# Phase 3: new_hud.cpp (screen 0, the in-game HUD)

I wrote this from the Sum.exe disassembly only: `tools/asm.mjs`, plus `llvm-objdump --start-address` run on each
function so that the jump tables do not misalign the listing, and the raw .rdata/.data bytes. I did not read any
Summoner 2 or Dash Faction code.

`new_hud.cpp` passes the requested `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings ...` with no
errors. It gives 10 warnings, all `-Wformat-security`: they come from `sprintf(text, XSTR(...))` with a
localized format and no arguments, which is what the code does.

A scratch `static_assert` file checked these sizes:
- hud_portrait_slot 0x10 (species +0xc)
- hud_drag_state 4
- lobj 0x70 (+0x14, +0x44, +0x50, +0x5c)
- lc_entry 0x10c
- file_entry 0x38
- Color_ctor 4
- the button table 0xf80

It also checked the types.h offsets the code uses:
- Character +0x1c, +0x84, +0x314, +0x320, +0x5f8, +0x5fe, +0x71c
- Player +0x10, +0x14, +0x17c
- game_camera +4, +8
- char_info +0x10

Headers (now): the include tree, see "Header conversion" at the end. As written: `stubs_menus_dialogue.h`,
`ui_screen.h`, `stubs_gamecreate_journal.h` and `stubs_status.h`, as
new_status.cpp uses them, plus the new `stubs_hud.h` for everything else.

## new_hud.cpp (0x40cb60-0x41075a; .rdata 0x57ee00-0x57ef0f; .data 0x58ac88-0x58bcbb; .bss 0x5bca58-0x5bd0cf)

70 functions: 10 compiler-generated, 60 written. Confidence: 45 exact, 25 likely, 0 unsure.

| Address | Function | Confidence |
|---|---|---|
| 0x40cb60 / 0x40cb70 | init_gametimer_5bd0cc thunk + initialiser (hud_camera_mode_timer) | exact |
| 0x40cb80 / 0x40cb90 | init_gametimer_5bd094 (hud_camera_forced_timer) | exact |
| 0x40cba0 / 0x40cbb0 | init_gametimer_array_5bd040 (implicit ctor of hud_portrait_slot x5) | exact |
| 0x40cbd0 / 0x40cbe0 | init_gametimer_5bd088 (hud_hot_button_timer) | exact |
| 0x40cbf0 / 0x40cc00 | init_gametimer_5bd090 (hud_death_timer) | exact |
| 0x40cc10 | hud_load_bitmaps | likely |
| 0x40cef0 | hud_init | exact |
| 0x40cf40 | hud_frame | likely |
| 0x40d160 | hud_update_portraits | likely |
| 0x40d4d0 | hud_portrait_set_state | likely |
| 0x40d540 | hud_update_anim_time | likely |
| 0x40d5d0 | hud_update_combat_buttons | exact |
| 0x40d640 | hud_confirm_level_exit | likely |
| 0x40d840 | hud_level_exit_step_back_thunk (an `(int button)` callback that tail-jumps) | exact |
| 0x40d850 | hud_level_exit_step_back | exact |
| 0x40d910 | hud_confirm_level_exit_cb | exact |
| 0x40d930 | hud_handle_keys | likely |
| 0x40e0d0 | hud_key_save | likely |
| 0x40e1f0 | hud_select_party_member | exact |
| 0x40e250 | hud_key_playmenu | exact |
| 0x40e260 | hud_key_help | exact |
| 0x40e270 | hud_key_pause | likely |
| 0x40e300 | hud_toggle_solo_mode | exact |
| 0x40e380 | hud_key_h_cb | exact |
| 0x40e390 | hud_key_automap | exact |
| 0x40e3c0 | hud_toggle_combat_mode | exact |
| 0x40e3e0 | hud_key_status | exact |
| 0x40e3f0 | hud_key_spells | exact |
| 0x40e420 | hud_key_journal | exact |
| 0x40e430 | hud_key_inventory | exact |
| 0x40e450 | hud_key_skills | exact |
| 0x40e480 | hud_interact_nearest | likely |
| 0x40e740 | quickload | exact |
| 0x40e7c0 | quicksave | likely |
| 0x40e980 | hud_key_camera | exact |
| 0x40e9d0 | hud_use_hot_button | likely |
| 0x40eb30 | hud_draw_tooltips | likely |
| 0x40ee20 | hud_update_buttons | exact |
| 0x40eeb0 | hud_button_action | likely |
| 0x40f120 | hud_portraits_handle_click | exact |
| 0x40f1b0 | hud_portrait_action | likely |
| 0x40f230 | hud_hot_buttons_handle_click | exact |
| 0x40f340 | hud_hot_buttons_drag | likely |
| 0x40f570 | hud_death_timer_frame | exact |
| 0x40f5d0 | hud_confirm_quit | exact |
| 0x40f650 | hud_confirm_quit_cb | exact |
| 0x40f690 | death_popup | exact |
| 0x40f6e0 | hud_draw | likely |
| 0x40f880 | hud_portrait_get_color | likely |
| 0x40fa90 | hud_draw_camera_mode | likely |
| 0x40fc20 | hud_draw_camera_forced_icon | likely |
| 0x40fd40 | hud_draw_health_bars | likely |
| 0x40fed0 | hud_draw_animated_icons | likely |
| 0x410020 | hud_draw_hot_buttons | likely |
| 0x4101e0 | draw_game_over | exact |
| 0x4102a0 | mp_draw_loading_status | likely |
| 0x410510 | hud_get_panel_height | exact |
| 0x410520 | hud_mouse_in_panel | exact |
| 0x410570 | hud_drag_icon_begin | likely |
| 0x4105d0 | hud_drag_icon_end | exact |
| 0x4105e0 | hud_drag_icon_draw | exact |
| 0x4106d0 | hud_mode_set_5 | exact |
| 0x4106e0 | hud_party_dead | exact |
| 0x410730 | hud_dead_clear_popups | exact |
| 0x410750 | hud_mode_reset | exact |

The unit is complete; nothing is left. 0x410760 (new_icam_load) starts the next unit: its string "Unable to
parse item ..." follows this unit's strings at 0x58bcbc.

### Data and new names

**.rdata 0x57ee00.** All names are new. Each table has one entry per resolution_index:
- hud_hot_button_size [3][2]
- hud_camera_mode_pos [3][2]
- hud_camera_icon_width [3]
- `const char hud_portrait_prefix[] = "hud-charportrait"` (0x57ee3c)
- hud_hp_bar_pos [3][2]
- hud_bar_size [3][2]
- `const char hud_pause_name[] = "pause.vbm"` (0x57ee80)
- hud_pause_icon_pos, hud_flash_paused_pos
- hud_hot_button_origin, hud_hot_button_spacing
- mp_loading_panel_width, mp_loading_name_width
- hud_death_popup_y (0x57ef04)

The two strings are in .rdata, so they must be const arrays, not literals. 0x57ef10-0x57ef34 are float `__real`
literals.

**.data 0x58ac88:**
- `hud_button_defs[62]` (new name; 62 x 0x40 bytes, ui_button_def, not const because it is in .data)
- `hud_portrait_species[16]` (0x58bc08, new name)
- `hud_flash_paused_names`
- then the string literals.

The literals "\xb9" (0x58bcb4) and "\xba" (0x58bcb8) are one-glyph strings in the HUD font: the camera-mode
icon and the camera-lock icon.

**.bss 0x5bca58-0x5bd0cf.** The variables are defined in address order. Several sum.symbols names are really
fields of hud_buttons, and the code uses them that way:
- `hud_hot_buttons_x/_y/_w/_h` (0x5bce20) are `hud_buttons[40]` x/y/w/h ("Main-HUD-HotBtnBkg").
- 0x5bcd30/34 are `hud_buttons[30].x/y` (the pause button).
- 0x5bca84 is `hud_buttons[1].h` (hud_get_panel_height).
- 0x5bcdd0 is `hud_buttons[37].flags` (SoloActive).

**Layouts:**
- `hud_portrait_slots` is 5 records of `{Character*, state, GameTimer, species}`. The sum.symbols comment gives
  `{character, ?, GameTimer, state}`; the state is at +4 and the species (0..3, or 4 for the rest) at +0xc.
  The species selects the hot-button set.
- `hud_drag_flags` is used as bit fields: active:1, slot:3 (6 = none), is_skill:1, id:27.

## Header workarounds (stubs_hud.h)

- **GameTimer** is redeclared under `#ifndef TIMESTAMP_H`. `vsdk/timestamp.h` cannot be included together with
  `stubs_gamecreate_journal.h`, because both define RealtimeTimer.
- **Color_ctor** is a subclass that declares `Color::Color(r, g, b, a)` (0x5067f0). Color in
  `stubs_menus_dialogue.h` has no constructor.
- **New thiscall shims:** `Character_HM`, `ai_state_HM`, `character_instance_HM`, `savegame_HM`, `camera_HM`,
  and the `conversation_storage` class. `Character_M` and `ai_state_M` already exist in stubs_status.h and
  cannot be extended.
- **`game_camera_obj`** stands for the object game_camera (0x60a018). types.h's typedef name `game_camera`
  makes the symbol's own name impossible while types.h is included.
- **`lobj`** is declared with +8 flags, +0x14 pos, +0x44 centre and +0x50/+0x5c bbox. It differs from the partial
  lobj in `stubs_sound_items_registry.h`, which has faces at +0xc; the two headers are never included together.
- **`MP_PLAYER_STATE` (+0x14) and `MP_PLAYER_ID` (+0x1c)** are macros, like `MP_PLAYER_NAME`, because mp_player
  in stubs_menus_dialogue.h stops at +0x2c.
- **`world_pick_portrait_clicked`** (0x60ab68) and **`ui_choice_popup_margin_y`** (0x57ef70, `{3, 3, 3}`) are
  new names; sum.symbols has none for these addresses.

## Wrong or incomplete shared declarations (not changed)

1. **sum.symbols savegame_hot_button_get** says (member, slot, out kind, out id). The callee and all six call
   sites give `(int *kind, int *id, int member, int slot)`. **savegame_hot_button_set** is
   `(kind, id, member, slot)`, not "(member, button, flag, id)".
2. **types.h `Player.mp_owner`** is `int32_t`, but it is an `mp_player*`: hud_frame reads +0x14 through it, and
   mp_draw_loading_status reads +0x34. **`Character._link04`** is the next object of obj_active_list, so it is
   a pointer to the same object type.
3. **stubs_menus_dialogue.h** declares `game_change_level(const char *, void *)`. quickload passes the 64-byte
   level name from the savegame header as the second argument, so it is a `const char *`.
4. **`KEY_CTRLED 0x4000`** in vsdk/window.cpp: docs/sum-exe/controls.md said 0x4000 is Alt, and here Alt+S /
   Alt+L are quicksave / quickload. I used `KEY_ALTED`. (Resolved: 0x4000 is Ctrl, checked in key_mark; the
   code now says `KEY_CTRLED`, DECISIONS.md D44. The keys are Ctrl+S / Ctrl+L.)
5. **`hud_show_message`** takes `char *` in stubs_assess_pickup.h and `const char *` in
   stubs_menus_dialogue.h.
6. **sum.symbols module column:**
   - `hud_target_mode` (0x58bdb8) is marked new_hud.cpp but lies after version_check_result (0x58bd3c,
     new_int_multi_update.cpp) and ui_bitmap_suffix_tga. It belongs to the interface unit's .data, so it is
     extern here.
   - `lc_entries` / `lc_entry_count` (0x248ce40 / 0x2491a68) are marked new_hud.cpp but lie far from this
     unit's .bss and are written by lc_entry_add (level_scripts_common.cpp), so they are extern.
   - `hud_message_text` / `hud_chain_message_text` (0x5bd32c / 0x5bd304) lie after this unit's .bss, with
     their functions at 0x414a20 / 0x414a80.
7. **sum.symbols naming:**
   - `hud_draw_camera_mode` (formerly `hud_draw_detail_level`) / `hud_shown_camera_mode` (formerly `hud_shown_detail_level`) / `hud_camera_mode_timer` (formerly `hud_detail_level_timer`) draw the camera mode
     (game_camera +0, 0 low / 1 high / 2 auto / 3 forced high; 5 means use +4), not the detail level.
   - `hud_draw_camera_forced_icon` (formerly `hud_draw_status_message`) / `hud_camera_forced_timer` (formerly `hud_status_message_timer`) draw the camera-lock glyph.
   - `world_hover_portrait_character` (formerly `world_pick_target_mode`) receives the hovered portrait's `Character*` from hud_frame.
8. **docs/sum-exe/controls.md "not mapped yet":**
   - Q: journal (10)
   - Enter: combat mode
   - F2: options (12)
   - F3: hud_key_save (screen 8 in save mode, or a warning)
   - F4: open_saveload_screen
   - Ctrl+S: quicksave (0x4000 is Ctrl, D44)
   - Ctrl+L: quickload

   All of these except F2 need single player. Also: X returns the key after it acts. Tab, S, H, C and the number
   keys do nothing on level 4 (the world map).

## Open questions

1. **hud_frame tests `hot_button_timer.valid() || elapsed()`.** The highlight that hud_handle_keys sets with a
   100 ms timeout is therefore cleared on the next frame. I reproduced it as is; it is probably a bug for
   `!valid() || elapsed()`.
2. **The .bss order differs from the initialiser order.** The static initialisers (entries 20-24) run for
   0x5bd0cc, 0x5bd094, 0x5bd040, 0x5bd088, 0x5bd090. If initialisers follow definition order, the source did
   not define these variables in address order. new_status.cpp shows the same mismatch. I kept address order.
3. **Stack slot reuse.** In hud_handle_keys (number keys) and hud_use_hot_button, VC6 keeps the hot-button id
   in the dead stack slot of the parameter. I wrote a local `id`; check whether VC6 does this unprompted.
4. **quicksave frame.** The buttons are at 0x0, the file list at 0x4, the filename at 0x38 and the message at
   0x54 (72 bytes). Nothing is placed at 0xc-0x37, so the sizes (filename[28]) are a guess. The free loop
   stores each node in `files.prev` before deleting it; it is probably an inline list method in the original.
5. **hud_interact_nearest skips item objects for which `item_obj_is_in_view` returns true.** Either the name has
   the opposite sense (true = culled) or the original skips visible items. The FPU sums the squares as
   z, y, x; I wrote the usual x, y, z expression.
6. **hud_portrait_get_color case 3 computes red from the green channel** (r = g). This looks like a source typo,
   and VC6 CSEs it. The float g lives in the stack slot of the parameter n.
7. **Switch body order.** In hud_draw_animated_icons the case bodies come out as 2, 1, default; in
   hud_draw_camera_mode (formerly hud_draw_detail_level) as 2, 0, 1/3. I wrote the cases in that order. If VC6 emits bodies in source order,
   this is right; otherwise the source order was 0, 1, 2.
8. **hud_drag_icon_begin merges the four bit-field stores into one** (`((kind != 0) | id << 1) << 4 |
   active | 0xc`). If VC6 does not merge bit-field stores like this, the original built the flags word by hand.
9. **mp_draw_loading_status.** The panel width is re-read from its table at each use, and the panel height
   shares a stack slot with the string height later. I read both as block-scoped temporaries.

## Header conversion (2026-10-15)

The unit now compiles with `-I src/decomp/include` only. stubs_hud.h is gone: the unit header is
`summoner/interface/new_hud.h` (it now also declares hud_key_help, which interface.h had); the declarations
went to the headers of their units (DECISIONS.md D43). The workarounds above are resolved:
- GameTimer comes from `vsdk/os/timestamp.h`; `Color` has its constructor (D41).
- `Character_HM`, `ai_state_HM`, `character_instance_HM` are members with identity macros (D45);
  `savegame_HM` is `&savegame_state` with the hot-button members (D46); `conversation_storage` is
  levelscripts/conversation.h's class.
- `game_camera_obj` became `game_camera` in the code, `CAMERA_HM` is `&game_camera` (D49).
- One `lobj` with all fields (D51); `MP_PLAYER_STATE` / `MP_PLAYER_ID` expand to mp_player fields (D50).
- Code changes: Ctrl, not Alt (D44); the object walk in hud_interact_nearest uses obj::next and
  item_obj::is_in_view (D45); `world_hover_portrait_character` is a `Character*`, the `(int)` casts are gone
  (D47); `pos[i]` became `.x/.y/.z` and the `*(vector3 *)` casts were dropped (D42); quickload's unused
  level-time local is a float (D52).
- Renamed after sum.symbols: hud_draw_camera_mode, hud_draw_camera_forced_icon, hud_shown_camera_mode,
  hud_camera_mode_timer, hud_camera_forced_timer, world_hover_portrait_character, level_table_find_id.
