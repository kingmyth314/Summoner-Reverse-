# Phase 3: new_playmenu.cpp, new_mainmenu.cpp, new_dialogue.cpp (source half)

Written from build/sum.asm (`tools/asm.mjs fn`) and the raw .data/.rdata bytes of Sum.exe only. No Summoner 2
or Dash Faction code was read. Each .cpp passes
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wextra -I <this folder> -I symbols <file>` with no warnings.
The sizes in the headers were checked with static_asserts (button def 0x40, button state 0x18, text box 0x1750,
topic box 0x62c, conversation_db 0x310, Character.flags +0x320).

Files: `new_playmenu.cpp`, `new_mainmenu.cpp`, `new_dialogue.cpp`; their headers `new_*.h`; `ui_screen.h`
(the 0x40-byte button table entry and the 0x18-byte button state); `dialogue_box.h` (the text/topic boxes
and conversation objects, defined in another unit); `engine_stubs.h` (everything else that is called).

## new_playmenu.cpp (0x421590-0x421dd3; .data 0x58f4b8-0x58f6f8; .bss 0x5c3b78-0x5c3c5c)

| Address | Function | Confidence |
|---|---|---|
| 0x421590 | playmenu_load_bitmaps | exact |
| 0x4216f0 | playmenu_init | likely (where playmenu_mouse_mode = 0 is written) |
| 0x4217b0 | playmenu_leave | exact |
| 0x4217c0 | playmenu_frame | exact |
| 0x421820 | playmenu_handle_click | exact |
| 0x421900 | playmenu_update_buttons | exact |
| 0x4219c0 | playmenu_button_action | exact |
| 0x421aa0 | playmenu_confirm_new_game (untagged) | exact |
| 0x421ae0 | mainmenu_confirm_new_game_cb | exact |
| 0x421b00 | playmenu_open_save (untagged) | likely (position of the `buttons[0]` stores) |
| 0x421c80 | open_saveload_screen (untagged) | exact |
| 0x421ca0 | playmenu_confirm_quit (untagged) | exact |
| 0x421ce0 | mainmenu_confirm_quit_cb (untagged) | exact |
| 0x421cf0 | playmenu_draw | exact |
| 0x421d40 | playmenu_mouse_in_panel | exact |
| 0x421d70 | game_abandon_to_menu (untagged) | likely (placement; code exact) |

## new_mainmenu.cpp (0x41d740-0x41ddde; .data 0x58dd48-0x58e09e; .bss 0x5c2f98-0x5c30d8)

| Address | Function | Confidence |
|---|---|---|
| 0x41d740 | mainmenu_load_bitmaps | exact |
| 0x41d8b0 | mainmenu_init | exact |
| 0x41d910 | mainmenu_frame | exact |
| 0x41d960 | mainmenu_handle_click | likely (key switch cases grouped as the jump tables show) |
| 0x41dac0 | mainmenu_start_new_game | likely (second argument, see question 3) |
| 0x41dae0 | mainmenu_check_network | likely (if/else chain shape) |
| 0x41dbc0 | mainmenu_check_network_cb | exact |
| 0x41dbd0 | mainmenu_check_network_cb2 | exact |
| 0x41dbf0 | ui_open_options (untagged) | exact |
| 0x41dc00 | mainmenu_quit | exact |
| 0x41dc10 | mainmenu_update_buttons | likely (the merged `mode = 0` exit) |
| 0x41dca0 | mainmenu_button_action | exact |
| 0x41dd90 | mainmenu_draw (untagged) | exact |

## new_dialogue.cpp (0x408d80-0x4099ac; .rdata 0x57eb88-0x57ec20; .data 0x58a158-0x58a1dd; .bss 0x5bc418-0x5bc490)

| Address | Function | Confidence |
|---|---|---|
| 0x408d80 | dialogue_load_bitmaps | likely (operand order of the set_rect sums) |
| 0x408f90 | dialogue_init (untagged) | exact |
| 0x408fe0 | dialogue_leave (untagged) | exact |
| 0x409000 | dialogue_frame (untagged) | likely |
| 0x409650 | dialogue_draw (untagged) | likely |
| 0x409980 | dialogue_mouse_in_panel | exact |

The four untagged functions sit between the two tagged ones and use only this unit's .bss, so they belong
here. 0x4099b0 (init_realtimetimer_5bc6c8_thunk) starts new_gamecreate.cpp.

Totals: 35 functions (16 + 13 + 6): 25 exact, 10 likely, 0 unsure.

**No static initialisers.** None of the three units has a global with a constructor (no RealtimeTimer,
no Color): there is no initialiser thunk in their code ranges and their .bss is plain arrays and ints. The
Colors used (ui_color_highlight / ui_color_text) and the dialogue boxes are built in other units.

## Facts used

- Button records: table entry {char name[0x24]; int pos[3][2]; int flags} (0x40), indexed by
  resolution_index; state {flags, bitmap, x, y, w, h} (0x18). State flag 1 always drawn, 2 lit this frame
  (cleared at the start of every *_frame), 4 disabled. Draw test is `flags & 3`.
- Sizes: (int)(w * ui_get_scale() + 0.5f), all float (0.5f at 0x57e6b0), `_ftol`.
- Three load_bitmaps variants: play menu calls progress_update(0), always appends the suffix, tests the
  handle before bm_get_dimensions and keeps the position for empty names; main menu has no progress_update;
  dialogue has progress_update. Main menu and dialogue skip names with a '.' (strchr) for both the suffix and
  the scaling, and leave empty-name records fully cleared.
- Sounds: game_play_sound(0x23 click / 0x24 close, 1.0f, 0, 0, 0) in the menus; sound_play_2d(0x23 / 0x24 /
  0x25 hover, 1.0f) in the dialogue.
- Message boxes: ui_message_box_show(title, text, -1, -1, n, buttons, cb, flags); the last argument is 2
  only in the two one-button network boxes.
- Localized strings are `localize_strings[n]` (0x25cd4dc + 4n), plain char*; sprintf, not swprintf.
- Keys: 1 Esc, 0x10 Q, 0x1f S, 0x26 L, 0x31 N, 0x32 M, 0x3b F1, 0x3c F2 (DirectInput scan codes).
- `x % 64` on the dialogue rings is the signed form (`and 0x8000003f; jns; dec; or -0x40; inc`), so the
  operands are plain int.
- Dialogue scroll tests: `fild scroll_target; fcomp scroll` = `scroll_target != scroll` (int left operand).
- `(i == dialogue_selected_topic) ? 4 : 3` is `sete; add 3`.

## Open questions

1. **Elevator code after the main menu.** 0x41dde0-0x41e37f (elevator_popup_frame ... elevator_floor_cb_c)
   and ui_screen_stop_player (0x41e380) come after mainmenu_draw and before the chat unit's first static
   initialiser (0x41e3d0). Their strings ("$npc118", "$npc117", ...) start at 0x58e0a0, right after
   "vlogo.bik" (ends 0x58e09e). That is either the tail of new_mainmenu.cpp or a separate unit whose .data
   happens to start on the next 16-byte boundary. Not written; they are untagged and not clearly main menu.
2. **open_url (0x421de0)** follows game_abandon_to_menu. Its "open" (0x58f718) is the first of a run of
   pxochat strings ("*.pcx", "*.cfg", "ChatBanners", ...), so it is probably the start of new_pxochat.cpp.
   Not written. Between the play menu table and "open" there are 0x20 bytes at 0x58f6f8 that nothing
   references by address: {149, 3, 475, 75}, {524, 3, 475, 75} (two rectangles, banner-sized). Owner unknown.
3. **mainmenu_start_new_game's second argument** is the dword at 0x2538a10. sum.symbols puts that inside
   conversation_save_table (0x25388c0, entry 84), which looks wrong for a game_change_level argument. Written
   as `conversation_save_table[84]` so the load is the same; it needs its own name and type.
4. **game_abandon_to_menu placement.** Only caller is mainmenu_confirm_quit_cb (tail jump) and it sits
   before open_url, so it is placed in the play menu; it reads mp_local_player->+0x28->+0xc even in single
   player, exactly as the binary does.
5. **conversation_node flags.** conversation_db +0x204 / +0x208 point to records whose +4 word is read with
   `shr n; and 1` in dialogue_frame / dialogue_draw / conversation_end and with `test byte, 4` in the Esc test.
   Written as 1-bit bitfields (flag2, flag6); masks would also give the `test` form. The names cur_topic /
   cur_line / journal_updated are ours.
6. **String literal order.** In .data "eaxlogo.bik" (0x58e088) comes before "vlogo.bik" (0x58e094) although
   vlogo is used first. The compiler test will show
   whether VC6 emits a function's literals in reverse order or the source used them in another order.
7. **The repeated mode update in dialogue_frame.** The "3 if topics are waiting, else 1" update appears
   before the switch (value kept in EAX for the switch) and after it (store 3, test, maybe store 1). Written
   identically; one of them may be a different expression (a ternary, or `mode = 3; if (...) mode = 1`).
8. **Arrow position locals in dialogue_draw.** The hit test uses positions computed into registers, but
   gr_string recomputes them from memory, so the source probably had locals for the test and the full
   expression in the gr_string call (as written). Check with the compiler.
9. **sum.symbols fixes suggested** (not applied, read-only): dialogue_text_box / dialogue_topic_box are
   tagged new_dialogue.cpp but are constructed by 0x4d6660 / 0x4d66e0 and sit in the .bss of the dialogue-box
   unit (with dialogue_color_yellow, dialogue_end_text, dialogue_more_text); dialogue_topic_box is 0x62c bytes,
   not 0x38; level_format_display_name takes (out, level), not (level, out); the twelve untagged functions
   above (dialogue_init/leave/frame/draw, ui_open_options, mainmenu_draw, playmenu_confirm_new_game,
   playmenu_open_save, open_saveload_screen, playmenu_confirm_quit, mainmenu_confirm_quit_cb,
   game_abandon_to_menu) can take the module of their file.
10. **.rdata.** The dialogue position tables and the two one-glyph strings ("\xa9", "\xaa") are in .rdata, so
    they are `static const` arrays (names ours). The float constants 255.0f / 0.0f just before them
    (0x57eb80 / 0x57eb84) are compiler constants; whether this object emits 0.0f is for the build to show.
11. **Types guessed.** progress_update's argument, bm_load's third argument (-1), game_pause's two zeros,
    the bool/int return types of the bool-returning helpers, and conversation::end_cleanup(bool).
