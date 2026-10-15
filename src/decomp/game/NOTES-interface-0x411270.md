# Phase 3: the interface manager (0x411270-0x414cb0), game/unnamed_interface_0x411270.cpp

**The file name is a placeholder.** The original `.cpp` name of this unit is not known: it has no assert
path, no strings.tbl section, and sum.symbols tags it `-` (docs/sum-exe/source-map.md, "Hand cuts inside
the unnamed units", first table). Rename the file when the name turns up. The header is
`summoner/interface/interface.h` (extended).

I wrote this from the Sum.exe disassembly only: `tools/asm.mjs fn`, `llvm-objdump --start-address` where a
jump table misaligned the listing (0x412af0, 0x412db0), and the raw .rdata / .data bytes read from Sum.exe
(jump tables 0x411b7c, 0x411e40, 0x41238c/0x4123a4, 0x4129d8, 0x412a34/0x412a3c, 0x412a48, 0x412ad0/0x412ad8,
0x412d54, 0x413ffc/0x414004). I did not read any Summoner 2 or Dash Faction code.

Both files pass `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I
decomp48/include` with no output. All 104 other `src/decomp/game/*.cpp` and `src/decomp/vsdk/*.cpp` files
of the repo (state of commit 917a09d / 5bcb42f) compile against `decomp48/include` too. The header copy was
rebased three times while I worked (09e45e3 + a2d447c, 083753c, 917a09d landed): `decomp48/include` is the
repo's current tree plus the changes in CHANGED-HEADERS.txt. 917a09d brought new_multi.h, so
elevator_popup_frame is taken from there (not redeclared).

## Functions (122: 98 exact, 23 likely, 1 unsure)

The unit is, in address order: 24 static initialiser pairs (thunk + body, entries 25-48), the code
0x4116c0-0x414bb0, the show_ui initialiser pair (entry 49) and cmd_show_ui. The file keeps that order.

| Address | Function | Confidence |
|---|---|---|
| 0x411270-0x411550 | 16 init_color_* thunk + body pairs (entries 25-40) | exact |
| 0x411570 / 0x411580 | init_empty_411580 thunk + empty body, read as vector3 ui_pick_world_override_pos | exact / unsure |
| 0x411590 / 0x4115a0 / 0x4115b0 / 0x4115c0 | ui_character_instance: construct + atexit, ctor call, atexit registration, dtor | exact |
| 0x4115d0-0x411640 | 3 init_color_* pairs (entries 43-45) | exact |
| 0x411660 / 0x411670 | interface_input_delay_timer (RealtimeTimer) | exact |
| 0x411680-0x4116b0 | hud_message_timer, hud_chain_message_timer (GameTimer) | exact |
| 0x4116c0 | interface_init_global | likely |
| 0x4118c0 | interface_reset_state | exact |
| 0x4118e0 | interface_load_all | exact |
| 0x4119a0 | interface_frame | exact |
| 0x411be0 | ui_tooltip_hide_on_mouse_activity | likely |
| 0x411cd0 | interface_draw | exact |
| 0x411ea0 | ui_tooltip_draw | exact |
| 0x411f80 | hud_draw_message | likely |
| 0x4120d0 | hud_draw_chain_message | likely |
| 0x4121d0 | interface_pulse_update | exact |
| 0x412220 | interface_set_view_clip | likely |
| 0x4123c0 | ui_load_model_lights | exact |
| 0x4123f0 | ui_model_lights_reset | exact |
| 0x412400 | ui_3d_begin | likely |
| 0x4125a0 | ui_3d_end | exact |
| 0x412610 | interface_get_screen | exact |
| 0x412620 | interface_set_screen | likely |
| 0x412ab0 | interface_screen_is_fullscreen | likely |
| 0x412af0 | interface_mouse_over_panel | exact |
| 0x412db0 | interface_refresh_cur_screen | exact |
| 0x412dd0 | statscreen_set_reduced_textures | likely |
| 0x412e50 | statscreen_refresh_anim | exact |
| 0x412f20 | statscreen_clone_instance | likely |
| 0x413190 | vector3::operator= (COMDAT of the vector.h inline, comment only) | exact |
| 0x4131c0 | statscreen_restore_instance | exact |
| 0x413200 / 0x413290 | party_find_next_member / party_find_prev_member | exact |
| 0x413320 / 0x413340 | statscreen_show_next_member / statscreen_show_prev_member | exact |
| 0x413360 | party_cycle | likely |
| 0x413460 | point_in_rect | exact |
| 0x4134a0 | ui_get_scale | exact |
| 0x4134b0 | ui_draw_panel_frame | exact |
| 0x4135a0 | ui_draw_text_box | likely |
| 0x4136a0 | ui_choice_popup_show | likely |
| 0x4138a0 | ClearFrameFlag5bd77c | exact |
| 0x4138b0 | ui_choice_popup_frame | exact |
| 0x413900 | ui_choice_popup_handle_click | exact |
| 0x413970 | ui_choice_popup_hit_test | likely |
| 0x413a90 | ui_choice_popup_update_buttons | likely |
| 0x413c50 | ui_choice_popup_wait_release | exact |
| 0x413c70 | ui_choice_popup_draw | exact |
| 0x413e20 | ui_choice_popup_is_active | exact |
| 0x413e40 | ui_message_box_show | likely |
| 0x414020 | ClearFrameFlag5bd504 | exact |
| 0x414050 | ui_message_box_frame | exact |
| 0x4140a0 | ui_message_box_handle_click | exact |
| 0x414110 | ui_message_box_hit_test | exact |
| 0x414200 | ui_message_box_update_buttons | exact |
| 0x414270 | ui_message_box_draw | likely |
| 0x4144d0 | ui_message_box_is_active | exact |
| 0x4144f0 | ui_bitmap_scaled | exact |
| 0x414560 | ui_clear_pick_overrides | exact |
| 0x414580 / 0x4145b0 | ui_get_pick_screen_override / ui_get_pick_world_override | exact |
| 0x4145e0 | game_draw_leader_marker | exact |
| 0x4146a0 / 0x4146b0 | ui_input_lock / ui_input_unlock | exact |
| 0x4146c0 | options_draw_version | likely |
| 0x414750 | ui_tooltip_set | likely |
| 0x4148c0 / 0x4148e0 | interface_start_input_delay / interface_input_ready | exact |
| 0x414900 / 0x4149d0 / 0x414a10 | cutscene_draw_loading / cutscene_begin_loading / cutscene_loading_finish | exact |
| 0x414a20 / 0x414a80 | hud_show_message / hud_show_chain_message | likely |
| 0x414ae0 / 0x414bb0 | color_pulse_scale / ui_apply_pulse_rgb | likely |
| 0x414c80 / 0x414c90 | init_ccmd_show_ui thunk + body (entry 49) | exact |
| 0x414cb0 | cmd_show_ui | exact |

## Moved: the show_ui command

`ccmd_show_ui` (0x5bd780) and `cmd_show_ui` (0x414cb0), with their comments, moved from
`src/decomp/game/new_inventory.cpp` to the end of `game/unnamed_interface_0x411270.cpp` (the copy
`decomp48/game/new_inventory.cpp` no longer defines them; its head comment, its .data comment and the
static-initialiser comment say where they went and now start at entry 50). The prototype moved from
`new_inventory.h` to `interface.h`. Evidence: DECISIONS.md D2803. `NOTES-inventory.md` rows 0x414c80 /
0x414c90 / 0x414cb0 and its "ccmd_show_ui ... initialiser in new_inventory.cpp" remark need the same move
when this is merged (README.md's new_inventory.cpp range 0x414c80- becomes 0x414d30-).

## What the code shows (for the docs)

- **Unit ranges.** Code 0x411270-0x414d20. .rdata 0x57ef40-0x57f03f: twelve int layout tables, then the
  float literals 0x57f00c-0x57f038 (new_inventory.cpp's .rdata starts at 0x57f040). .data 0x58bda4-0x58c06f:
  ui_bitmap_suffix_tga (0x58bda4) and ui_bitmap_suffix (0x58bdac) are this unit's (their strings lie in its
  literal pool, D2806), then the data 0x58bdb4-0x58bea7 and the literals 0x58bea8-0x58c06f. .bss
  0x5bd2e0-0x5bd96f.
- **Static initialiser order** (= definition order): 16 colours, the empty initialiser (a vector3), the
  character instance, 3 panel colours, the delay timer, the two HUD message timers, and ccmd_show_ui (entry
  49) after the last function.
- **The screens' one-time set-up in file order.** interface_init_global and interface_load_all call one
  function per new_*.cpp in alphabetical order of the file names (assess, automap, buysell, charselect,
  credits, dialogue, gamecreate, gamelist, hud, inventory, itemtrade, journal, levelselect, mainmenu,
  (multichat in load_all), options, pickup, playmenu, pxochat, saveload, skill, spell, status, tutorial);
  the empty ones are folded into empty_function (0x4f13e0). The help screen's reset comes after status in
  interface_load_all. The leave calls of screens 0, 3, 5 and 14-18 are empty too.
- **Folded panel tests.** The screens without a panel of their own (1, 2, 7, 8, 10-12, 14-18, 21, 22) call
  ui_return_true (0x4063a0, new_buysell.cpp) from interface_mouse_over_panel: the folded copy of their
  `<screen>_mouse_in_panel` that returns true.
- **interface_set_screen** re-picks status_character only when the screen being left is not full-screen
  and the new one is 1, 2, 7 or 10: cur_character when it is the main hero, else (multiplayer) the local
  player's character unless hidden or a summon, else the first type-7 living entity on the level (hidden ones
  only on the world map) that is not the summon. When none is found it calls itself with (0, 0) (compiled as
  a jump back to the top). Leaving a full-screen screen for the HUD flushes the mouse. Screens 3, 5 and 6
  clear the overhead labels before their init. saveload_init / options_init / help_init get
  `prev != 14` (opened from somewhere other than the main menu) as their in_game flag.
- **interface_set_view_clip** (docs/sum-exe/interface.md "The 3D view is clipped per screen" is partly
  wrong): during a cutscene the view is letterboxed to 2:1 (h = w / 2, centred); with the HUD hidden
  (ui_input_locked) or on a full-screen screen it is the whole screen; on screens 0, 4, 9, 13 and 20 the
  HUD bar's height (hud_get_panel_height, 0x410510 - a height, not a panel width) is cut from the bottom;
  on 3, 5, 6 and 19 the side panel's width (automap / spells / skills / itemtrade_get_panel_width) is cut
  from the left as well. Outside cutscenes it then recomputes camera_fov with gr_fov_to_vertical.
- **interface_screen_is_fullscreen**: false for 0, 3, 4, 5, 6, 9, 13, 19, 20; true for the rest.
- **Message box**: flag 1 pauses a single-player game (and ClearFrameFlag5bd504 unpauses), flag 2 blocks
  Esc. Showing a box closes the side panels 3, 5, 6, 19, stops the player in multiplayer, and plays sound
  0x27 once the level runs (game_flags 0x200). Esc and a button both play 0x23. The callback gets the button
  index. Buttons are spread at (i + 1) / (n + 1) of the box width on the last line of the text area.
- **Choice popup**: flags bit 0 centred title, 1 centred entries, 2 an int value per entry (stored only
  then), 3 no cancel, 30 pressed, 31 shown; dragging a pressed entry out of its row with flag 4 starts a HUD
  drag icon (value bit 1 = skill, bits 2- = id). Esc / a click outside play 0x24, a choice 0x23.
- **Tooltip**: centred above the mouse (y offset -5 / -6 / -8 per resolution), clamped to the screen;
  while shown it stays until the wheel, a button, a key or the mouse leaving the owner's rectangle; while
  hidden any mouse movement restarts the 600 ms delay. interface_input_ready is false while the tooltip is
  shown.
- **HUD message lines**: 63 / 15 characters, 4000 ms, the last second fading (alpha = remaining / 4000,
  clamped to 1). The main line is not drawn on the dialogue screen; the chain line is not drawn at all
  while a tutorial page is active (sum.symbols says its position moves: it does not).
- **Cutscene escape**: Esc during a cutscene calls cutscene_cleanup unless the cutscene has +NoAbort.
- **Leader marker**: cursorspot-focus.tga for size-1 characters, cursorspot-focus-large.tga otherwise,
  1.5 x 1.5, turning at 90 degrees per second.
- **Model views**: ui_3d_begin sets the ambient to 0.5, at most 10 lights per object, turns all world lights
  off and adds the "inv-lights1.vfx" lights at the model's position (colour bytes / 255, range / 256,
  intensity 0.5); ui_3d_end restores all of it.
- **party_cycle** returns whether the leader changed; in multiplayer it ignores the direction.

## Doc / symbol disagreements

1. source-map.md: the unit's .data starts at 0x58bda4 (ui_bitmap_suffix_tga), not 0x58bdb4; its .rdata
   0x57ef40-0x57f03f is not listed.
2. interface.md "The 3D view is clipped per screen": 0x410510 is the HUD bar height; the side-panel screens
   use their own *_get_panel_width; cutscene letterbox and the full-screen cases are missing (above).
3. interface.md screen table: screen 11's leave is 0x4231a0 (pxochat_leave_thunk), not 0x422370 (its
   target); screens 5 and 14-18 have an (empty, folded) leave call, 0 has one too.
4. sum.symbols hud_draw_chain_message: "position moves when tutorial_is_active" - it returns without
   drawing.
5. sum.symbols ui_choice_popup_title char[512]: 0x201 bytes (strncpy 0x200 + terminator at 0x5bd718);
   ui_message_box_text char[259]: 0x100 bytes (terminator at 0x5bd490; the rest is padding).
6. sum.symbols ui_choice_popup_margin_y (0x57ef70) module new_hud.cpp: it is in the interface manager's
   .rdata block (0x57ef40-0x57f03f), used by its popup functions.
7. sum.symbols interface_input_delay_timer data:int: a RealtimeTimer (0x5bd950); statscreen_saved_instance
   void*: a character_instance*.
8. sum.symbols ui_tooltip_set "(x, y, text)": (x, y, w, h, text), the owner's rectangle first (the header
   was right).
9. interface.h said status_character is new_status.cpp's: it is the manager's (D2807). The message box's
   1024 background is "MessagePopupBckgrnd21024.tga" against "MessagePopupBkgrnd2640.tga"; both names exist
   in summoner.vpp, so it is not a bug.
10. 0x58be28 (screen width / 640) is written by interface_init_global and read nowhere (named
    ui_scale_640, ours).

## Game bugs (candidates for docs/game-bugs)

1. **Choice popups with a title: each entry's click area is 5 pixels above its text.** The draw adds
   ui_choice_popup_title_gap (5 at every resolution) below the title (0x413d45), ui_choice_popup_show adds
   it to the height, but ui_choice_popup_hit_test does not (0x4139b3-0x4139d1). With 12-20 pixel rows the
   lowest 5 pixels of an entry select the next one. The drag rectangle of update_buttons ignores the title
   height entirely (0x413b07-0x413b24).
2. **A pausing message box unpauses a game that was already paused.** Flag 1 calls game_pause (a no-op when
   paused, 0x432667) and ClearFrameFlag5bd504 calls game_unpause unconditionally (0x414034), so a box shown
   while the player had paused the game resumes it when closed.
3. **Shift+Tab cycles forward in multiplayer** (party_cycle ignores its argument when game_flags & 2).
4. **interface_set_screen can call the old screen's leave function twice.** The (0, 0) restart jumps back to
   the leave switch while interface_cur_screen is still the old screen (0x412770 -> 0x41262c). The re-pick
   only runs when the old screen is not full-screen, so e.g. opening the inventory from the skills panel
   with no usable party member runs skills_leave twice (from the HUD the leave is empty). Rare.
5. Latent: ui_message_box_draw copies each wrapped line into a 64-byte stack buffer with strncpy(len[i])
   and no clamp (0x414388); a 280 / 350 / 448 pixel line of narrow glyphs longer than 63 characters would
   overflow it. interface_frame dereferences cutscene_data without a NULL test when GF_CUTSCENE is set
   (0x4119dd). The multiplayer loop of party_cycle never ends if every party record's character is gone.

## Open questions

1. init_empty_411580 (entry 41): read as the vector3 ui_pick_world_override_pos (the unit's only object
   with an empty inline constructor besides Color, and every Color is built with arguments); unsure.
2. statscreen_clone_instance: the implicit character_instance::operator= is expanded member by member, but
   its vector3 members come out three ways (plain dword copies for pos / root_velocity, dwords plus the
   inline operator='s return copy through vector3__vector3_copy for motion_cur / motion_prev, the
   out-of-line COMDAT 0x413190 for root_offset). Whether VC6's inliner gives exactly this from the implicit
   operator= needs the compiler.
3. hud_show_message / hud_show_chain_message / ui_tooltip_set test the length twice; the source form
   (a macro?) is unknown.
4. The .bss order of the definitions cannot be reproduced from the addresses (as in the other units).
5. The two resolution switches of interface_init_global (whether case 0 / 1 are explicit labels).
