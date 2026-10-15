# Phase 3: new_gamecreate.cpp and new_journal.cpp (source half)

Written from build/sum.asm (`tools/asm.mjs fn`) and the raw .rdata/.data bytes of Sum.exe only. No Summoner 2
or Dash Faction code was read. Both .cpp files pass the requested
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings ...` with no warnings. A scratch check confirmed
sizeof(mp_char_profile) 0x154, ui_button_def 0x40 and quest_info 0x6c.

Headers: `stubs_menus_dialogue.h` and `ui_screen.h` (existing), plus `stubs_gamecreate_journal.h` for what they
lack (RealtimeTimer, gr_split_str and mouse_get_delta, all guarded by `#ifndef STUBS_CREDITS_HELP_H`; gr clip
calls, key_to_ascii, localize_quest_name, gr_string_truncate_ellipsis, the character profile record,
mp_host_game, the shop/statscreen helpers, the savegame class and quest_table). `stubs_credits_help.h` cannot
be included as well because it also defines Color and declares interface_set_screen with a different signature.

## new_gamecreate.cpp (0x4099b0-0x40a65f; .rdata 0x57ec20-0x57ecc7; .data 0x58a1e0-0x58a79f; .bss 0x5bc490-0x5bc708)

| Address | Function | Confidence |
|---|---|---|
| 0x4099b0 | init_realtimetimer_5bc6c8_thunk (compiler, no source) | exact |
| 0x4099c0 | init_realtimetimer_5bc6c8 (compiler: `RealtimeTimer gamecreate_cursor_timer`) | exact |
| 0x4099d0 | gamecreate_load_bitmaps | exact |
| 0x409b10 | gamecreate_init | likely (word clear of field_active read as an inlined memset) |
| 0x409c60 | gamecreate_set_difficulty | likely (where the store of the difficulty sits) |
| 0x409cd0 | gamecreate_frame | exact |
| 0x409db0 | gamecreate_update_create_button | exact |
| 0x409de0 | gamecreate_handle_click | unsure (shape of the text input part, see Q1) |
| 0x40a020 | gamecreate_cancel | exact |
| 0x40a030 | gamecreate_update_buttons | exact |
| 0x40a0c0 | gamecreate_button_action | exact |
| 0x40a230 | gamecreate_players_dec | exact |
| 0x40a250 | gamecreate_players_inc | exact |
| 0x40a270 | gamecreate_levels_above_dec | exact |
| 0x40a280 | gamecreate_levels_above_inc | exact |
| 0x40a2a0 | gamecreate_levels_below_dec | exact |
| 0x40a2b0 | gamecreate_levels_below_inc | exact |
| 0x40a2d0 | gamecreate_create_game | exact |
| 0x40a310 | gamecreate_draw | exact |
| 0x40a380 | gamecreate_draw_text_field | likely (min(0, w - text w) spelling) |
| 0x40a4f0 | gamecreate_draw_player_count | likely |
| 0x40a570 | gamecreate_draw_limits | likely |

22 functions: 16 exact, 5 likely, 1 unsure.

Facts:
- Button table (23 x 0x40): flag 1 = HiLite acting through the Press entry after it on release, flag 2 = acts
  itself (the three difficulty check boxes 6/8/10). Ticks 5/7/9 show the difficulty (bit 0, set_difficulty).
  init disables 10 unless profile +0x4c has 0x8000000 and 8 unless it has 0x4000000 (difficulty unlocks).
  Begin (21) is disabled while the game name is empty.
- init defaults: name = player name (mp_local_player +0x34) + localize_strings[819] ("'s Game"), cut to 31;
  password empty; 4 players; difficulty 0; levels above and below both 4.
- mp_host_game(name, password, max_players, difficulty, levels_below (limit_b), levels_above (limit_a)), then
  state 14. Cancel / Esc = state 12. Text keys: Esc clears the field and drops focus, Enter drops focus,
  Backspace, max 31 / 15 characters. Sounds 0x25 arrows, 0x23 check boxes and Begin, 0x24 Cancel.
- levels_above_dec and levels_below_dec stop at 0 (`test eax, eax / jle`); the sum.symbols comment of
  gamecreate_levels_below_dec says "min 1", which is wrong. players_dec does stop at 1.
- Symbol `gamecreate_button_defs` is at 0x58a21c, the flags field (+0x3c) of entry 0; the table starts at
  0x58a1e0. Here the table is named gamecreate_button_defs.

## new_journal.cpp (0x41a940-0x41be94; .rdata 0x57ff24-0x57fffb; .data 0x58d4b8-0x58d97f; .bss 0x5c1888-0x5c1acb)

| Address | Function | Confidence |
|---|---|---|
| 0x41a940 | init_realtimetimer_5c18bc_thunk (compiler) | exact |
| 0x41a950 | init_realtimetimer_5c18bc (compiler: `RealtimeTimer journal_repeat_timer`) | exact |
| 0x41a960 | journal_load_bitmaps | exact |
| 0x41ab10 | journal_init | likely (the zero stores as chained assignments) |
| 0x41abf0 | journal_update_list_scrollbar | likely (FPU locals; dead count > 0 guard) |
| 0x41ad40 | journal_select_quest | exact |
| 0x41ade0 | journal_update_desc_scrollbar | likely |
| 0x41aef0 | journal_leave | exact |
| 0x41af30 | journal_frame | exact |
| 0x41af80 | journal_handle_click | likely (key case order from the body layout) |
| 0x41b470 | journal_get_quest | likely |
| 0x41b4d0 | journal_list_page_up | likely (two stores of the element) |
| 0x41b520 | journal_list_page_down | likely |
| 0x41b570 | journal_desc_page_up | exact |
| 0x41b5a0 | journal_desc_page_down | exact |
| 0x41b5d0 | journal_button_action | exact |
| 0x41b6f0 | journal_goto_shop_or_status | exact |
| 0x41b720 | journal_goto_inventory | exact |
| 0x41b750 | journal_list_scroll_up | exact |
| 0x41b780 | journal_list_scroll_down | exact |
| 0x41b7c0 | journal_desc_scroll_up | exact |
| 0x41b7e0 | journal_desc_scroll_down | exact |
| 0x41b810 | journal_update_buttons | exact |
| 0x41b8e0 | journal_drag_scrollbar | likely (three stores in the list branch, one in the description branch) |
| 0x41b9f0 | journal_draw | exact |
| 0x41ba60 | journal_draw_quest_list | likely (struct copy of the row origins, early exit) |
| 0x41bbf0 | journal_draw_description | likely |
| 0x41bd40 | journal_draw_list_scrollbar | likely (operand order of the last tile y) |
| 0x41be00 | journal_draw_desc_scrollbar | likely |

29 functions: 17 exact, 12 likely, 0 unsure.

Facts:
- Lists: 0 main quests, 1 side quests (quest_info.side_quest), only quests whose savegame quest bit is set.
  Description: line 0 = localize_quest_name, line 1 blank, lines 2.. = gr_split_str (16 lines max) of the
  current step's text; journal_desc_line_count = split lines + 2.
- Buttons (19): 1/2 left arrow = buy/sell (7) if a shopkeeper is set else status (2); 3/4 right arrow =
  inventory (1, byte argument 0); 5..16 scroll arrows (flag 2: act on press, 500 ms delay, 100 ms repeat);
  17/18 close (interface_close_screen). Keys: Esc/Q screen 0, B buy/sell (only with a shopkeeper),
  I inventory, R status. The mouse wheel pages whichever pane is under the cursor.
- journal_leave_flags (formerly journal_paused_game) (0x5c1a9c) is really a leave-flags word: 1 unpause, 2 statscreen_refresh_anim(0),
  4 shop_set_shopkeeper_from_char(0); init sets 6 (+1 when it paused), the screen-switch paths clear 6.
- 0x5c1a96 is a third visibility byte (description scrollbar), written by journal_update_desc_scrollbar and
  never read; declared here as journal_desc_scrollbar_visible.
- The 0.5 rounding is used for the button sizes, not for the scrollbar tile size.
- quest_table / quest_count (0x253ef40 / 0x25443a4) are tagged new_journal.cpp in sum.symbols but lie far from
  this unit's .bss (events_parse_table fills them); extern here.
- journal_selected_quest / journal_hover_quest are typed int in sum.symbols but hold quest_info pointers.

## Open questions

1. gamecreate_handle_click: after the key handling of a focused field the binary tests field_active[i] again
   and returns 0 when it is clear; every path that clears it already returned. The surrounding shape (search
   loop then `if (i < 2)`, or an inline function, or a goto) is a guess, and the default case of the field
   switch (key = 0) jumps straight past the `if (key)` test.
2. gamecreate_draw_player_count / draw_limits centre the number with the string height on both axes (the
   width output is never read). Either the source had this slip or gr_get_string_size's parameter order is
   (h, w); credits_get_line_size suggests (w, h).
3. Order of the key cases in journal_handle_click (Esc/Q, B, I, R) and whether the zero byte passed to
   interface_set_screen(1, ...) is a char or bool local.
4. The "%d", "|" and ScrollBarMid strings are pooled copies in other units' .data (0x589820, 0x588c10,
   0x588b74/0x588b8c); a VC6 build would place literals of these units in their own .data unless string
   pooling (/Gf) merges them across objects at link time.

## Shared header issues (not changed)

- stubs_menus_dialogue.h `mp_player` ends at +0x2c; the player name at +0x34 is needed here (MP_PLAYER_NAME
  macro in stubs_gamecreate_journal.h). sum.symbols types mp_local_player as void*.
- stubs_credits_help.h and stubs_menus_dialogue.h disagree on interface_set_screen's second parameter
  (int vs void*): the journal passes a pointer to a zero byte, so void* is right.
- stubs_credits_help.h and stubs_menus_dialogue.h both define Color, so they cannot be included together.
