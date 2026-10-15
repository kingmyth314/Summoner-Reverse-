# Phase 3: new_status.cpp (screen 2, the Character screen)

Written from build/sum.asm (`tools/asm.mjs fn`, plus `llvm-objdump --start-address` where a jump table had
misaligned the listing: 0x42b610 and 0x42bfc0) and the raw .rdata/.data bytes of Sum.exe only. No Summoner 2
or Dash Faction code was read. `new_status.cpp` passes the requested `i686-w64-mingw32-g++ -fsyntax-only -Wall
-Wno-write-strings ...` with one warning, `spent` set but not used (on purpose, see status_apply_skill_points).
A scratch `static_assert` file checked the 0x14-byte skill row, gr_light_desc 0x20, skill_info 0x34
(description +0x2c), new_icam_table 0x3628, and the types.h offsets used (Character +0x314/+0x324/+0x558/
+0x614/+0x754, Player +0x17c, gr__Screen +0x2c).

Headers: `stubs_menus_dialogue.h`, `ui_screen.h`, `stubs_gamecreate_journal.h` (as new_journal.cpp), plus
`stubs_status.h` for the rest. Character, Player, ai_state and gr__Screen come from symbols/types.h.

## new_status.cpp (0x42b1c0-0x42e62e; .rdata 0x5807a0-0x580d93; .data 0x5927e0-0x592a1f; .bss 0x5f5aa8-0x5f960b)

| Address | Function | Confidence |
|---|---|---|
| 0x42b1c0 / 0x42b1d0 | init_empty_42b1d0 thunk + initialiser (read as status_cameras, see Q1) | unsure |
| 0x42b1e0 / 0x42b1f0 | init_strptrs_5f95fc thunk + initialiser (status_modifier_key_names) | likely |
| 0x42b220 / 0x42b230 | init_realtimetimer_5f93b0 thunk + initialiser (status_button_timer) | exact |
| 0x42b240 | status_cameras_parse_table | exact |
| 0x42b260 | status_load_bitmaps | exact |
| 0x42b480 | status_init | likely |
| 0x42b570 | status_update_species_tab | exact |
| 0x42b610 | status_update_member_buttons | exact |
| 0x42b680 | status_build_skill_list | likely |
| 0x42b800 | status_update_scrollbar | likely |
| 0x42b910 | status_leave | exact |
| 0x42b970 | status_apply_skill_points | likely |
| 0x42b9f0 | status_cancel_skill_points | exact |
| 0x42ba50 | status_frame | exact |
| 0x42bb00 | status_handle_click | likely |
| 0x42bfc0 | status_get_skill_points | exact |
| 0x42c020 | status_confirm_skills | exact |
| 0x42c080 | status_goto_pending_screen | exact |
| 0x42c0c0 | status_confirm_skills_cb | exact |
| 0x42c0e0 | status_page_up | exact |
| 0x42c110 | status_page_down | exact |
| 0x42c140 | status_button_action | exact |
| 0x42c260 | status_confirm_inventory | exact |
| 0x42c270 | status_goto_shop_or_inventory | likely |
| 0x42c2d0 | status_confirm_skills_switch_member | exact |
| 0x42c380 | status_show_prev_member | exact |
| 0x42c3a0 | status_confirm_skills_b_cb | exact |
| 0x42c3c0 | status_show_next_member | exact |
| 0x42c3e0 | status_confirm_skills_b_cb2 | exact |
| 0x42c400 | status_scroll_up | exact |
| 0x42c420 | status_scroll_down | exact |
| 0x42c450 | status_ai_popup_show | likely |
| 0x42c4c0 | status_confirm_close | exact |
| 0x42c4d0 | status_show_skill_tooltip | exact |
| 0x42c540 | status_update_buttons | likely |
| 0x42c630 | status_drag_scrollbar | likely |
| 0x42c6c0 | status_ai_popup_frame | exact |
| 0x42c710 | status_handle_skill_buttons | likely |
| 0x42c820 | status_skill_increment | likely |
| 0x42c8d0 | status_skill_decrement | exact |
| 0x42c940 | status_chain_popup_frame | exact |
| 0x42c9c0 | status_draw | likely |
| 0x42ca50 | status_draw_model | likely |
| 0x42cce0 | status_model_view_begin | exact |
| 0x42cd60 | status_model_view_end | exact |
| 0x42cd80 | status_draw_level | likely |
| 0x42cff0 | status_draw_stats | likely |
| 0x42d640 | status_draw_skills | likely |
| 0x42da10 | status_draw_scrollbar | likely |
| 0x42dac0 | status_draw_skill_points | likely |
| 0x42dcd0 | status_draw_resistances | likely |
| 0x42e410 | status_draw_status_effects | likely |
| 0x42e5c0 | status_draw_combat_style | exact |
| 0x42e620 | status_refresh_for_character | exact |

57 functions (51 written + 3 compiler initialiser pairs): 33 exact, 22 likely, 2 unsure.

Data names marked *new* in the source: the button table (0x5807a0), the .rdata layout tables 0x580ce0-0x580d93,
the .data tables 0x592810, 0x592828, 0x59284c-0x5928e7 (minus the two named value_right tables), the three
bitmap name tables 0x592900-0x592917, `status_skill_button_pos` 0x592918 (sum.symbols names its members
status_skill_button_x / _y), `status_scrollbar_track` 0x592938 (sum.symbols names the y member
status_scrollbar_track_y 0x59293c), `status_skill_points_pos` 0x59295c, and in .bss
`status_skill_buttons_shown` 0x5f93ac and the hole `status_unused_5f93c4`.

## Facts

- Buttons (21): 1-4 species name tabs (bit 0 set for the shown character's species), 5/6 inventory,
  7/8 buy/sell if a shopkeeper is set, else journal (10) in single player, inventory (1) in multiplayer,
  9/10 and 11/12 previous / next member (disabled with at most one member, the summon not counted, or in
  multiplayer), 13-16 scroll arrows (flag 2, 500 / 100 ms repeat), 17/18 combat-style popup, 19/20 close.
  Keys: Esc and R close (sound 0x24), B buy/sell (only with a shopkeeper), I inventory, Q journal (single
  player only); wheel pages the list. Every exit goes through status_confirm_skills ("Accept changes to
  skills?" when points are pending).
- Mouse modes: 0 idle, 1 button held, 2 dragging the thumb, 3 combat-style popup (entered by
  status_update_buttons after the release when the popup is up), 4 +/- held, 5 chain key popup.
- Skill rows: the Player's progression entries whose level the character has reached (pick lock 0x15 and
  appraise 0x14 hidden in multiplayer, negative values left out), then the unlocked chain attacks (at most 35
  rows). Row +8 = skill byte * 0.1 (base), +0xc = get_skill * 10 (with equipment), +0x10 pending points. The
  value drawn is level + pending, blue / red when equipment raises / lowers it. A point can be added while
  (base + pending) * 10 < xp_level_skill_cap(level) * 10. Apply adds pending * 10 to Character.skill (and
  sends mp_send_character_skills in multiplayer); cancel returns the points to Player.skill_points.
- The 0x5f93ac byte is a latch: once a row has shown the +/- buttons (skill points > 0) it stays set until the
  screen opens again or the member changes, so the last point can be taken back.
- Right click on a chain row: popup CAPS / SHFT / CTRL / ALT (localize 734-737); the choice is
  character_set_chain_skill(player, slot, skill). Combat-style popup: the six ai_combat_style_names at
  (style x - 0x57ef64[res], style y - 100); the choice goes to ai_state +0x18 (ai_set_18).
- Magic resistance bonus colour = resistance - (Magic resist skill * 0.5 + level * 0.02) * 100, i.e. what
  character_get_resistance adds on top of those two. Speed = skill 2 * 1000 %, bonus = that - byte * 10.
- Status effects: the names of the active effects except ids 14, 15 and 17, joined with ", ", wrapped to
  at most 8 lines of status_effects_width.
- The model view: the character instance animates only in single player; when 0x25d921d (set by the
  Direct3D code) is true, the screen outside the view rectangle is filled with black rectangles first.
  status_model_view_begin shifts gr_screen offset_x/y by status_model_offset, plus status_model_offset_mp for
  Jekhar (species 1) in multiplayer.
- Bitmap tables: 0x592900 "Icon-SklMod" is the idle +/- pair (sum.symbols: status_skill_row_bm, also used for
  the hit size), 0x592908 "-UP" the + half pressed (status_skill_button_bm), 0x592910 "-DN" the - half pressed
  (status_skill_button2_bm). Only the scrollbar tile is scaled by ui_get_scale.

## Open questions

1. **init_empty_42b1d0** (entry 80, a bare `ret`) is read as the dynamic initialiser of `status_cameras`,
   with an inline empty constructor on the stub `new_icam_table` (VC6 keeps the initialiser even when the
   inlined constructor is empty). The real table type is unknown (0x3628 bytes, filled by new_icam_load).
2. **Initialiser order vs .bss order.** The initialisers run cameras (80), key names (81), timer (82); in .bss
   the timer (0x5f93b0) comes before the key names (0x5f95fc). If both orders were definition order they
   would agree, so either VC6 does not lay .bss out in definition order or the key-name table is defined
   before the timer but placed elsewhere. The source follows the .bss order.
3. **0x5f93c4** (between status_pending_screen and status_buttons) is never referenced; written as an unused
   int, as pickup's 0x5c3a44. 0x5f5aa4 (before status_skills_dirty) is also unreferenced and left to
   new_spell.cpp.
4. **status_apply_skill_points** sums the pending points in EDI and never reads the sum. Written as a dead
   local; VC6 /O2 might drop it, in which case the source had some other use (or none).
5. **status_set_bonus_color** is an inline helper with a float parameter: every colour choice is fild of an
   int and two fcom with 0.0f, then one shared gr_set_color_ptr call. A macro would give the same code.
6. **Locals:** status_handle_click calls gr_get_font_height once at the top with the result unused, then
   again before the row tests (written as an initialised local overwritten later). The sizes of
   status_load_bitmaps' filename (0x70 bytes in the frame) and the 16-byte buffers are read from the frames.
7. **gr_light_desc:** its constructor builds a vector3 at +0xc, yet status_draw_model stores the multiplayer
   appearance at +0x10, inside that vector3. Either the class is not a light description (the symbol name may
   be wrong) or the vector3 is shorter than it looks.
8. **"%d", "%d/%d", "" and the ScrollBarMid names** are pooled copies in other units' .data (0x589820,
   0x58ac40, 0x5bd0d0, 0x588b8c/0x588b74); the build needs string pooling across objects (/GF), as the
   journal and pickup notes say.
9. **Key case order** in status_handle_click follows the body layout (ESC/R, B, I, Q); the table indices are
   by key value.

## Shared header issues (not changed)

- symbols/types.h declares Character, ai_state and matrix3 as C structs, so the thiscall methods the screen
  calls (character_get_skill, character_get_max_stat, character_equip_bonus, character_get_resistance,
  character_resist_equip_bonus, character_has_status, character_hero_model_prefix, character_get_protection on
  +0x558, ai_set_18 / ai_get_combat_style on +0x324, matrix3__make_identity) are declared on layout-free shims
  in stubs_status.h (`CHAR_M(c)->get_skill(2)` etc.). In the merged headers they should be members.
- types.h Character.ai is `uint8_t[0x234]` and .combat `uint8_t[0x54]`, not the ai_state / living_combat types.
- ItemType.damage (+0x68) is `int16_t` in types.h but read zero-extended here (as in new_assess.cpp).
- sum.symbols: character_set_chain_skill is described as (char, slot, skill), but status_chain_popup_frame
  passes the Player record; ui_tooltip_set is described as (x, y, text) but takes (x, y, w, h, text).
- stubs_player_effectmem.h also declares `skill_info` (only +0x08 recovery) and `living_list`; this unit's
  `skill_info` adds name (+0x00) and description (+0x2c). player.h (Player, player_from_handle) cannot be
  included with types.h, so player_from_handle and summon_player are declared again in stubs_status.h.
- stubs_assess_pickup.h declares localize_string_or_invalid, ui_color_white and its own Character class; it
  cannot be included with stubs_menus_dialogue.h (both define Color and RealtimeTimer/Character), so those
  are declared again.
