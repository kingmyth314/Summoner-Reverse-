# Phase 3: new_charselect.cpp and new_levelselect.cpp (source half)

Written from build/sum.asm (`tools/asm.mjs fn`, plus `llvm-objdump --start-address` where a jump table
made the listing misaligned) and the raw .rdata/.data/.text bytes of Sum.exe only. No Summoner 2 or Dash
Faction code was read. Both .cpp files pass the requested `i686-w64-mingw32-g++ -fsyntax-only -Wall
-Wno-write-strings ...` with no warnings. A scratch check confirmed sizeof(mp_char_record) 0x154 (level at
+0x150), file_find_entry 0x38, level_table_entry 0x10, char_info (types.h) 0x3d0.

Headers: `stubs_menus_dialogue.h`, `ui_screen.h`, `stubs_gamecreate_journal.h` and `vsdk/registry.h`
(os_registry_read_string_nodefault / os_registry_write_string), plus `stubs_charselect_levelselect.h` for
the rest (XSTR, mouse_was_double_clicked, three ui colours, File with find / Delete, file_find_files and its
entry, path_players, char_infos, charinfo_find_by_species, the full character record, the .plr helpers,
mp_session_reset, pxo_cfg_flags, the mp_player next / type macros, mp_player_list, the netgame globals,
the level helpers, chat_send_line, trim_whitespace, level_table).

## new_charselect.cpp (0x4063b0-0x408582; .rdata 0x57ea20-0x57eb33; .data 0x589828-0x58a0e7; .bss 0x5b6808-0x5bc3f7)

| Address | Function | Confidence |
|---|---|---|
| 0x4063b0 | init_realtimetimer_5b6c70_thunk (compiler) | exact |
| 0x4063c0 | init_realtimetimer_5b6c70 (compiler: `RealtimeTimer mp_charselect_repeat_timer`) | exact |
| 0x4063d0 | init_realtimetimer_5bc378_thunk (compiler) | exact |
| 0x4063e0 | init_realtimetimer_5bc378 (compiler: `RealtimeTimer mp_charselect_cursor_timer`) | exact |
| 0x4063f0 | mp_charcreate_load_bitmaps | exact |
| 0x406590 | mp_charselect_init | likely (list 0 reset written twice, as the binary stores it twice) |
| 0x406710 | mp_char_scan_files | likely (list free loop through the head's prev field) |
| 0x406820 | mp_charselect_update_scrollbar | likely |
| 0x406970 | mp_charselect_select_species | likely (place of the species store) |
| 0x4069e0 | mp_charselect_frame | exact |
| 0x406b40 | mp_charselect_update_button_states | exact |
| 0x406bb0 | mp_charselect_update_create_button | exact |
| 0x406bf0 | mp_charselect_handle_click | likely (see Q1) |
| 0x4072e0 | mp_charselect_wrap_text_input | likely |
| 0x4073c0 | mp_charselect_page_up | likely |
| 0x407410 | mp_charselect_page_down | likely |
| 0x407460 | mp_charselect_save_choice | exact |
| 0x4074b0 | mp_charselect_back | exact |
| 0x4074d0 | mp_charselect_clear_text_input | exact |
| 0x4074e0 | mp_charselect_button_action | exact |
| 0x407650 | mp_charselect_scroll_up | exact |
| 0x407680 | mp_charselect_scroll_down | exact |
| 0x4076d0 | mp_char_create | likely (name strlen tested twice) |
| 0x4078b0 | mp_char_confirm_delete (sum.symbols module "-", inside this unit) | exact |
| 0x4078f0 | mp_char_confirm_delete_cb (module "-", inside this unit) | exact |
| 0x407900 | mp_char_delete_file | exact |
| 0x4079c0 | mp_char_format_info | likely |
| 0x407d30 | mp_charselect_update_buttons | exact |
| 0x407e10 | mp_charselect_drag_scrollbar | likely |
| 0x407ed0 | mp_charselect_draw | exact |
| 0x407fd0 | mp_charselect_draw_char_list | likely |
| 0x4080c0 | mp_charselect_draw_scrollbar | likely |
| 0x408180 | mp_charselect_draw_name_input | likely |
| 0x4082d0 | mp_charselect_draw_text_input | likely |
| 0x408490 | mp_charselect_draw_char_info | exact |
| 0x408540 | mp_charselect_get_selected_profile | likely (bool parameter, see below) |

36 entries (32 functions + 4 compiler initialisers): 20 exact, 16 likely, 0 unsure.

Facts:
- Three lists share the per-list arrays: 0 characters, 1 the description typed for a new character,
  2 the info panel. The sum.symbols names of the focus flags are misleading: `mp_charselect_name_editing`
  (0x5b6830) = name box focus, `mp_charselect_name_input_active` (0x5bc3e4) = description box focus,
  `mp_charselect_text_input_active` (0x5b6838) = info panel open (set by mp_char_format_info, cleared by
  mp_charselect_clear_text_input = the panel's close button 20/21). Suggested renames: name_focus,
  desc_focus, info_open, clear_text_input -> close_info.
- Button table flag bits (0x589828, 34 x 0x40): 1 HiLite acting on release via the next entry, 2 arrow
  acting on press with repeat via the next entry, 4 check box acting itself on release, 8 info-panel entry
  (hit-tested only while the panel is open, drawn on top). Entries 0 and 19 (panel background) get the
  always-drawn bit. Delete/Profile/Select (9/11/13) need a selection; Create (15) a name and a class.
- Names: letters, digits, '_' and '-', first character a letter or '_', max 31. Description: max 255, no
  leading space. Double click on a character = Select (save the choice, state 12, or 11 with
  pxo_cfg_flags bit 0). ESC closes the panel, else goes back to state 1.
- Character record (0x154): filename +0x00 (0x28), then what mp_player_file_read_summary(out, filename)
  fills: name +0x28, char_infos index +0x48, flags +0x4c, description +0x50 (0x100), level +0x150 (short).
  Max 64 records. mp_charselect_species holds the char_infos index (charinfo_find_by_species), and the tick
  shown is entry 1 + 2 * index, so it assumes char_infos[0..3] are species 0..3.
- mp_charselect_selected (0x5b6824) is an int[3] (init resets all three); only [0] is used.
- Button 31 (info panel up arrow) calls mp_charselect_scroll_down(2), like 33: the byte table 0x407630 maps
  both to the same case. Original bug: the info panel cannot be scrolled up with the arrow.
- mp_charselect_draw_text_input sets no colour.
- 0x5bc37c is the pressed button (unnamed in sum.symbols; called mp_charselect_pressed_button here).

## new_levelselect.cpp (0x41bea0-0x41d72b; .rdata 0x57fffc-0x5800f7; .data 0x58d980-0x58dd47; .bss 0x5c1ad0-0x5c2f97)

| Address | Function | Confidence |
|---|---|---|
| 0x41bea0 | init_realtimetimer_5c1c6c_thunk (compiler) | exact |
| 0x41beb0 | init_realtimetimer_5c1c6c (compiler: `RealtimeTimer mp_levelselect_blink_timer`) | exact |
| 0x41bec0 | init_realtimetimer_5c1d5c_thunk (compiler) | exact |
| 0x41bed0 | init_realtimetimer_5c1d5c (compiler: `RealtimeTimer mp_levelselect_button_timer`) | exact |
| 0x41bee0 | mp_levelselect_load_bitmaps | exact |
| 0x41c080 | mp_levelselect_init | exact |
| 0x41c110 | mp_levelselect_update_scrollbar | likely |
| 0x41c250 | mp_levelselect_build_list | likely |
| 0x41c2a0 | mp_levelselect_frame | exact |
| 0x41c330 | mp_levelselect_sync_selection | exact |
| 0x41c380 | mp_levelselect_update_kick_button | likely (bool local) |
| 0x41c450 | mp_levelselect_handle_click | likely (see Q2) |
| 0x41cad0 | mp_levelselect_back | likely (if/else of the two calls) |
| 0x41cb20 | mp_levelselect_page_up | likely |
| 0x41cb70 | mp_levelselect_page_down | likely |
| 0x41cbc0 | mp_levelselect_start_thunk | exact |
| 0x41cbd0 | mp_levelselect_button_action | exact |
| 0x41cc90 | mp_levelselect_scroll_up | exact |
| 0x41ccc0 | mp_levelselect_scroll_down | exact |
| 0x41cd10 | mp_levelselect_kick_selected_player | exact |
| 0x41cd80 | mp_levelselect_update_buttons | exact |
| 0x41ce50 | mp_levelselect_drag_scrollbar | likely |
| 0x41cf00 | mp_levelselect_draw | exact |
| 0x41cf80 | mp_levelselect_draw_chat | unsure (colour switch of the other line types, Q3) |
| 0x41d1d0 | mp_levelselect_draw_levels | likely |
| 0x41d2d0 | mp_levelselect_draw_scrollbar | likely |
| 0x41d390 | mp_levelselect_draw_chat_input | likely |
| 0x41d4e0 | mp_levelselect_draw_players | likely |
| 0x41d5e0 | mp_levelselect_add_chat_line | likely (memmove size spelling) |

29 entries (25 functions + 4 compiler initialisers): 15 exact, 13 likely, 1 unsure.

Facts:
- Lists: 0 chat (64 lines of 0x44 bytes: type, text[64]), 1 levels (level_table indices with mp_flags
  bit 0, or all 26 with All_multi_levels). The player box (4 rows) has no scrollbar.
- sum.symbols' mp_levelselect_num_levels (0x5c1adc) is mp_levelselect_list_count[1] (update_scrollbar(1)
  reads it there); mp_levelselect_selected_row (0x5c1c68) is element [1] of an int[2] at 0x5c1c64 (init
  resets both). Declared as arrays here.
- Buttons (15): 1/2 back, 3..10 scroll arrows (flag 2), 11/12 kick, 13/14 start. Start is enabled for the
  host in init only; Kick by mp_levelselect_update_kick_button every frame (host, a type-3 player selected,
  not the host itself). Back: mp_player_disconnected(2, 0) for the host, (0, 0) otherwise, then state 11/12.
- Chat line types from mp_levelselect_add_chat_line(text, type): 0/1 -> first line 0, the rest 1; 2/3 ->
  2 then 3; 4 -> 4; type > 4 drops the message. Drawn: 0 name (up to and including the first space) in
  blue, 2 the name in the highlight colour, 4 magenta, 1/3 text colour. A wrapped line over 63 characters
  drops the whole message. Enter sends trim_whitespace(input) with chat_send_line when not empty.
- Only the host can pick a level (click = mp_levelselect_set_level, double click also starts and plays
  0x23); levels not available to every player are greyed and cannot be clicked.

## Open questions

1. mp_charselect_handle_click: the "if (!focus) return 0" after each key block (as in
   gamecreate_handle_click); the `if (i == 2) continue` in the list loop (the binary compares the row
   count pointer with &row_counts[2] inside the loop); the char check written with a bool local.
2. mp_levelselect_handle_click: the ESC switch is only reached while the chat box has no focus (the focused
   path jumps straight to the wheel code), so it is in the else branch. The player hit rows advance for
   every list entry but draw_players only for type-3 players: hit and drawn rows differ once a non-type-3
   player precedes one (original behaviour).
3. mp_levelselect_draw_chat: types 1 and 3 are tested explicitly though they get the same colour as the
   default; written as `case 1: case 3:` plus default. If VC6 drops those tests, the source had a
   different shape (an if chain, or more colours in the switch).
4. Buffers sized from the frame: name box copy 36 bytes, description / chat copies 260, info text 516
   (0x5b6c74-0x5b6e77) and mp_charselect_text_input 260 (0x5b683c-0x5b693f, max 255 used). The source sizes
   may be smaller (alignment) with unreferenced globals in the gaps.
5. mp_char_scan_files frees its list by storing each entry in the head's prev field before deleting it;
   probably a list macro or an inline from a vfile header.
6. .bss order is assumed to follow the definition order (as in the other pilots).

## Shared declarations that are wrong or short (not changed)

- stubs_gamecreate_journal.h: `mp_charselect_get_selected_profile(int validate)`: the binary tests only the
  low byte (`mov al, [esp+8]`), so the parameter is a bool/char; kept as int here to match.
- stubs_gamecreate_journal.h: `mp_char_profile` has only the flags; the record is mp_char_record (filename
  +0x00, name +0x28, char_info +0x48, flags +0x4c, description +0x50, level +0x150). The two should merge.
- stubs_menus_dialogue.h `mp_player`: no next (+0x00), type (+0x14) or name (+0x34); macros
  MP_PLAYER_NEXT / MP_PLAYER_TYPE added next to MP_PLAYER_NAME. stubs_player_effectmem.h has next but not the
  others, and also declares mp_player_list and char_infos / char_info.
- stubs_menus_dialogue.h `mp_player_disconnected(int id, int reason)`: the level select passes (2, 0) /
  (0, 0); the first argument looks like a reason or mode, not a player id.
- sum.symbols: mp_player_file_read_summary is (out, filename), not "(path, out)"; mp_charselect_selected,
  mp_levelselect_list_count and mp_levelselect_selected_row are arrays (see above); mp_charselect_name_box_x
  / _y are one interleaved {x, y}[3] table (mp_charselect_name_box_pos here); mp_charselect_button_defs is
  0x40 bytes before its symbol (0x589828, as gamecreate_button_defs).
- XSTR is defined only in stubs_credits_help.h, File only in headers that clash with stubs_menus_dialogue.h
  (Color / TextFile / Character); both repeated in stubs_charselect_levelselect.h under guards / as a
  minimal class.
