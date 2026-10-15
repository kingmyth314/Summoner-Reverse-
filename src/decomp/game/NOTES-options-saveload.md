# Phase 3: new_options.cpp and new_saveload.cpp (source half)

Written from build/sum.asm (`tools/asm.mjs fn`) and the raw .rdata/.data bytes of Sum.exe only. No Summoner 2
or Dash Faction code was read. Both .cpp files pass the requested
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings ...` with no warnings.

Headers: `stubs_menus_dialogue.h`, `ui_screen.h` and `vsdk/timestamp.h` (RealtimeTimer) as they are, plus
`stubs_options_saveload.h` for the rest: the summoner.cfg block (`SummonerOptions` with its inline constructor),
the sound / gr / popup calls, File, the file_find_files list, a fuller `savegame` class, conversation_storage,
level_display_names. `stubs_gamecreate_journal.h` is not included (see "Shared declarations").

## new_options.cpp (0x41ee90-0x4205ae; .rdata 0x580134-0x5801ff; .data 0x58e0d0-0x58f38f; .bss 0x5c32d8-0x5c3a2f)

| Address | Function | Confidence |
|---|---|---|
| 0x41ee90 | init_options_config_defaults_thunk (compiler, no source) | exact |
| 0x41eea0 | init_options_config_defaults (compiler: inline `SummonerOptions()` for `options_edit`) | exact |
| 0x41ef30 | options_load_bitmaps | likely (always-drawn entries 0/1/2/19/70 as a switch) |
| 0x41f0f0 | options_init | likely (place of the mouse-mode store) |
| 0x41f250 | options_set_sfx_volume | exact |
| 0x41f2b0 | options_set_music_volume | exact |
| 0x41f300 | options_set_camera_speed | likely (clamp comparisons) |
| 0x41f380 | options_apply_gamma | likely (`/ 1.5f` for the binary's `* 2/3`, Q1) |
| 0x41f410 | options_set_eax | likely (store order in the no-EAX branch) |
| 0x41f490 | options_set_solo_confirm | exact |
| 0x41f4d0 | options_set_invert_camera | exact |
| 0x41f510 | options_set_dialogue_camera | likely (statement order in the cases) |
| 0x41f5c0 | options_set_cfg_3c | exact |
| 0x41f600 | options_set_combat_flag | exact |
| 0x41f640 | options_set_party_model_detail | likely (as dialogue_camera) |
| 0x41f6f0 | options_set_model_detail | likely |
| 0x41f7a0 | options_set_texture_quality | exact |
| 0x41f7f0 | options_set_lightmaps_off | exact |
| 0x41f840 | options_set_cfg_21 | exact |
| 0x41f870 | options_set_detail_24 | likely |
| 0x41f920 | options_set_fog_quality | likely |
| 0x41f9d0 | options_set_antialias | likely (`c ? on : false` for neg/sbb/and) |
| 0x41fa20 | options_set_cfg_34 | exact |
| 0x41fa60 | options_leave | exact |
| 0x41fa90 | options_apply_and_save | exact |
| 0x41fae0 | options_restore_volumes | exact |
| 0x41fb00 | options_frame | exact |
| 0x41fb50 | options_handle_click | exact |
| 0x41fc50 | options_close | exact |
| 0x41fc80 | options_update_buttons | exact |
| 0x41fd20 | options_button_action | likely (sound per case or after the switch, Q3) |
| 0x41fff0 | options_drag_sliders | likely (float temporaries of the clamp) |
| 0x420320 | options_draw | likely (order of the two antialias tests) |
| 0x420430 | options_draw_sfx_slider | likely (operand order of the fill width) |
| 0x420490 | options_draw_music_slider | likely |
| 0x4204f0 | options_draw_camera_speed_slider | likely |
| 0x420550 | options_draw_gamma_slider | likely |

37 functions: 19 exact, 18 likely, 0 unsure.

Facts:
- `options_edit` (0x5c32e0) is a whole 0x40-byte `SummonerOptions`: init copies `Summoner_options` (0x60acc8)
  in with one rep movsd, accept copies it back, `summoner_cfg_save`, `options_apply_detail`, then pxo_cfg_flags
  bit 0 (cfg +0x3c set) or bit 1 (clear). Cancel only restores the two volumes (the sliders act live).
  init_options_config_defaults is byte-for-byte the same code as init_game_config_defaults (0x43d190) on the
  other object, so the constructor is inline in a shared header. Fields +0x0a/+0x0b are bytes set to 0 there.
- The variable name `Summoner_options` and the field `gamma_correction` come from cmd_gamma's help string.
- Buttons (75 x 0x18 state records, standard `ui_button_state`): flag 4 = slider knob (entries 1 sfx, 2 music,
  19 camera speed 1.0..2.0, 70 gamma 0.7..2.2), hit area widened by 8/10/12 px; flag 2 = check box hot spot;
  71/73 accept / cancel (flag 1, act through 72/74). Radio pairs: EAX 3/5, solo confirm 7/9, combat 11/13,
  invert 15/17, dialogue camera 20/22/24, cfg_3c 26 (off) / 28 (on), party detail 30/32/34, model detail
  36/38/40, texture 42 (0 and 1) / 44 (2), lightmaps check 46, cfg_21 check 48, detail_24 50/52/54, fog
  56/58/60, antialias 62 (on) / 64 (off), cfg_34 66 (on) / 68 (off).
- Knob x = slider x + (int)(slider w * value + 0.5) - knob w / 2; the green fill ends at the knob centre.
  Dragging rounds to steps of 0.1 (`(int)((x / w + 0.05) * 10) * 0.1`, then `+ 1.0` for the camera speed and
  `* 1.5 + 0.7` for gamma) and calls the setter only when the value changed.
- Without EAX both EAX boxes are unticked and disabled (bit 4); nothing re-enables them. Without lightmap
  multitexturing (`gr_is_mode_supported(&gr_mode_lightmap_multitexture)` false) lightmaps_off is forced on.
  Without card antialiasing the value is forced off and boxes 62/63 are not drawn.
- options_flags: 1 paused by init (only when the game was running and game_flags bit 0), 2 opened from the
  main menu (close goes back to screen 14, else 0), 4 accepted.
- options_draw calls ui_choice_popup_draw when options_mouse_mode == 3, a mode nothing in this unit sets, then
  tail-jumps to options_draw_version (0x4146c0, interface code).

## new_saveload.cpp (0x425410-0x42723b; .rdata 0x580604-0x58070b; .data 0x590068-0x59066f; .bss 0x5c5968-0x5f4a77)

| Address | Function | Confidence |
|---|---|---|
| 0x425410 / 0x425420 | init_strptrs_5f4a48 thunk + initialiser (`saveload_month_names`, 12 `char *&`) | likely |
| 0x4254a0 / 0x4254b0 | init_realtimetimer_5e3614 thunk + initialiser (`saveload_cursor_timer`) | exact |
| 0x4254c0 / 0x4254d0 | init_realtimetimer_5e35f8 thunk + initialiser (`saveload_button_timer`) | exact |
| 0x4254e0 | saveload_load_bitmaps | likely |
| 0x4256a0 | saveload_init | likely (order of the resets) |
| 0x425730 | saveload_update_save_button | exact |
| 0x425790 | saveload_refresh | exact |
| 0x4257f0 | saveload_update_mode_buttons | exact |
| 0x4258c0 | saveload_scan_saves | likely (hand-written list free, block-scoped locals) |
| 0x425b00 | saveload_update_scrollbar | likely (dead count > 0 guard, as the journal) |
| 0x425c10 | saveload_load_thumbnail | exact |
| 0x425c80 | saveload_leave | exact |
| 0x425c90 | saveload_frame | exact |
| 0x425d60 | saveload_update_select_button | exact |
| 0x425d80 | saveload_update_delete_button | likely (two disable stores) |
| 0x425de0 | saveload_handle_input | likely (text input shape, blink expression) |
| 0x426260 | saveload_page_up | likely |
| 0x426290 | saveload_page_down | exact |
| 0x4262c0 | saveload_close | exact |
| 0x4262f0 | saveload_save_or_load | exact |
| 0x426320 | saveload_try_save | likely (1..3 range test) |
| 0x426370 | saveload_save_to_slot | likely (local order) |
| 0x426480 | saveload_load_selected | exact |
| 0x426500 | saveload_confirm_overwrite | exact |
| 0x426540 | saveload_confirm_overwrite_cb | exact |
| 0x426570 | saveload_button_action | exact |
| 0x426670 | saveload_scroll_up | exact |
| 0x426690 | saveload_scroll_down | exact |
| 0x4266c0 | saveload_confirm_delete | exact |
| 0x426700 | saveload_delete_save | exact |
| 0x4267e0 | saveload_update_buttons | exact |
| 0x4268b0 | saveload_hover_thumbnail | exact |
| 0x426930 | saveload_drag_scrollbar | likely |
| 0x4269c0 | saveload_draw | exact |
| 0x426a20 | saveload_draw_list | likely (`/ 3600.0f`, Q1) |
| 0x426be0 | saveload_format_level_name | likely (branch layout) |
| 0x426d10 | saveload_draw_info | likely (local layout) |
| 0x426fd0 | saveload_draw_thumbnail | likely |
| 0x427050 | saveload_draw_scrollbar | likely (operand order, as the journal) |
| 0x427100 | saveload_capture_thumbnail | likely (565 conversion expression) |

43 functions: 25 exact, 18 likely, 0 unsure.

Facts:
- **`saveload_load_mode` (0x5f4a46) is true in save mode** (it shows SaveLoad-SG/-SF/-Save, adds the
  `<New Savegame>` row, makes the action button save). From the main menu (flags bit 1) saveload_refresh refuses
  the save list, and saveload_init(save, from menu) does nothing at all.
- interface_set_screen(8, &byte) calls saveload_init(byte, prev != 14). in_game: capture the thumbnail first,
  pause if not paused (no game_flags test, unlike options).
- List entry (0x130): filename[0x20], level (-1 = new row), script slot, level_time (header +0xac), file time
  (+0x2c, the file_find node's time; time() for the new row), description[0x100]. At most 32. Saves whose level
  name or script name is no longer valid are skipped. The new row is inserted at the top with memmove and
  L237 "No description".
- Save: the new row takes the first `summoner%d.sav` that File::find does not see; other rows are overwritten
  after a Warning / "Overwrite existing savegame?" Yes/No box. Load: savegame_set_pending_load(filename, 0) and
  `game_change_level(level_script_table[level].name, conversation_save_table[(level * 7 + script) * 4])`, i.e.
  the name pointer of the level's conversation/script slot (0x10-byte records at 0x25388c0).
- Blocked saves: savegame_location_allowed() 1..3 returns without a message (the HUD key shows one; this screen
  greys arrows 5/7 instead).
- The row index, not scroll + row, is used in three places (bugs kept): the overwrite test in saveload_try_save,
  "row > 0" in saveload_load_thumbnail, and "selected == 0" in saveload_draw_thumbnail.
- Info panel: localtime of the file time as "h:mm am/pm" (0 -> 12), "Mon. d, yyyy" with localize_strings
  702..713, description in up to 9 lines of the box width; while editing, "|" is appended and cut off on the
  off phase of the 500 ms blink. Up to 255 characters; Esc clears and stops, Enter stops.
- Thumbnail capture: clip 201x151, gr_setup_3d(camera_view_orient, camera_view_pos, game_camera.fov, 1, 1),
  gr_clear, render with game_flags 0x20000, gr_read_backbuffer: format 3 copied (0xed1e bytes), format 7
  (32-bit) converted to 565 from bytes 0/1/2 as r/g/b, anything else black; then gr_mark_texture_dirty.
- Buttons 9/11 (flag 4) act on press and repeat every 100 ms after 500 ms; flag-2 entries act through the
  next entry on release when the timer is invalid. Double click on a row = save/load at once.

## Open questions

1. **Division by constants.** The binary multiplies by the floats 2/3 (0x580204, gamma knob) and 1/3600
   (0x58070c, shared with time_format_hms). Written as `/ 1.5f` and `/ 3600.0f`, assuming VC6 /O2 replaces a
   float division by a constant with the reciprocal. If it does not, the source had the multiplications.
2. **.bss order.** The dynamic initialisers run month table (entry 77), cursor timer (78), button timer (79),
   which is the reverse of their addresses (0x5f4a48, 0x5e3614, 0x5e35f8); pxochat's timers are not monotonic
   either. So VC6's .bss order is not definition order. The three objects are defined first, in initialiser
   order; the rest in address order.
3. **options_button_action**: every box case's tail is one shared copy (push sound / call / add esp, 0x18,
   and cases 67/69 even share the setter call). Written with a sound call per case (as gamecreate_button_action);
   it may have been one call after the switch. saveload_button_action keeps separate tails, also for the two
   identical scroll cases.
4. **Unreferenced data**: .rdata 0x5806f4 `{50,450},{63,563},{80,720}` (kept as `extern const`, since an
   unreferenced internal const would be dropped), and 4-byte holes at 0x5c3324 and 0x5e361c (left out).
5. Literal order in .data: "summoner%d%s" 0x590634, "%02d:%02d" 0x590644, then draw_info's in reverse order of
   use ("%s %d, %d" 0x590650 before "%d:%02d %s" 0x59065c, "pm" 0x590668 before "am" 0x59066c).
6. saveload_scan_saves frees the file list by hand (`prev = next; next = next->next; delete prev`, then the
   head points at itself); probably an inline helper or macro of vfile.
7. saveload_handle_input: `cursor_visible = changed || !cursor_visible` is a guess for "store changed, then set
   1 if the old value was 0".
8. VC6 `time_t` is 32-bit; the mingw check uses its own size, so only VC6 gives the 0x130 / 0x38 layouts.

## Shared declarations that are wrong or missing (not changed)

- sum.symbols: `saveload_load_mode` means save mode (name inverted). `saveload_thumbnail_buf` is typed
  uint16[201*151] but is 201*151*4 bytes (0x5c5ba0-0x5e35db; format 7 reads 4 bytes per pixel).
  `options_buttons` comment says "bitmap +8, x/y +0x10/+0x14": the records are the usual ui_button_state
  (bitmap +4, x/y +8/+0xc, w/h +0x10/+0x14). `saveload_update_select_button` comment says "disabled (bit 2)",
  it is bit 4 (value 4). `level_display_names` is typed char*[104] but holds 52 {name, char **text} pairs.
  `conversation_db` is tagged new_saveload.cpp but lies far outside this unit's .bss. The address 0x5f4a46
  comment and saveload_save_or_load's comment disagree on save vs load mode.
- stubs_gamecreate_journal.h: `class savegame` declares only get_quest_bit, so read_info / read_thumbnail_file /
  write / set_pending_load cannot be added; stubs_options_saveload.h defines a fuller class (the two headers
  cannot be included together).
- `File` is now defined in three stub headers (stubs_sound_items_registry.h, stubs_container_targa.h, here).
  `File__delete` is spelled `Delete` (delete is a keyword).
- RealtimeTimer is defined in stubs_credits_help.h, stubs_assess_pickup.h, stubs_gamecreate_journal.h and
  vsdk/timestamp.h; these files use timestamp.h.
- symbols/types.h `typedef struct game_camera {...} game_camera;` makes it impossible to declare the object
  game_camera (0x60a018) under its own name; the fov field is reached as `camera_fov` (0x60a1a4).
- stubs_menus_dialogue.h `conversation_save_table` (void*[357*4]): the records are 0x10 bytes {name, speakers,
  topics, ?}; index (level*7 + slot)*4 is the slot name. This also answers NOTES-menus-dialogue open question
  on `conversation_save_table[84]` in new_mainmenu.cpp: 84 = (3*7 + 0)*4, the name of level 3's slot 0, the
  same kind of argument saveload_load_selected passes to game_change_level.
