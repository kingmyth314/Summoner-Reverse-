# Phase 3: the three units of 0x430bc0-0x433330 (decomp51)

**All three file names are placeholders.** None of the units has an assert path, a strings.tbl section or a
module in sum.symbols (docs/sum-exe/source-map.md, "Hand cuts inside the unnamed units", second table):

| File | Unit | Functions | Exact / likely / unsure |
|---|---|---|---|
| `game/unnamed_editor_dialogs_0x430bc0.cpp` | editor dialog boxes 0x430bc0-0x4317e0 | 9 | 6 / 3 / 0 |
| `game/unnamed_states_0x431870.cpp` | boot / title / load-game states 0x431870-0x4323d0 | 37 | 31 / 6 / 0 |
| `game/unnamed_frame_0x4323e0.cpp` | frame end, pause, input, screenshots, cursors, ccmds 0x4323e0-0x433280 | 33 | 29 / 4 / 0 |

79 functions with an address (66 exact, 13 likely), plus two empty debug hooks with no address of their own
(unsure, D3102). Headers: summoner/summoner.h and six vsdk / engine headers (CHANGED-HEADERS.txt); decisions
D3100-D3107 in include/DECISIONS.md.

Written from Sum.exe only (`tools/asm.mjs fn / grep / callers`; the .data bytes and the jump table 0x4315a0
read from Summoner/Sum.exe with a small dump script). No Summoner 2 or Dash Faction code was read. All three
files pass `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I
decomp51/include` with no output; the editor file switches off -Wdelete-non-abstract-non-virtual-dtor under
`#ifdef __GNUC__` (its literal deletes through non-virtual destructors, as objects.cpp D61). The 115 repo .cpp
files (state bf21e19) give the same diagnostics against decomp51/include as against src/decomp/include. The
header copy was rebased twice (c1fcd83 and bf21e19 landed during the work).

## The cuts (checked)

The table's three cuts hold. New evidence from .data (D3107): VC6 writes a unit's initialised data and then
its literal pool, so an initialised object right after a literal pool starts a unit.

| Boundary | Evidence |
|---|---|
| 0x430bc0 (editor start) | static_initializers entry 93 (init_empty) at the head; .bss 0x5fa278-0x5fa2c7 (count, six colours, the list head as a whole 0x30-byte dialog). The unit has **no initialised data**, so .data cannot confirm the start; it rests on the initialiser and .bss. Literals 0x59309c-0x5930b7 follow the previous unit's "...'Debug_help_str' variable up". |
| 0x431870 (states start) | build_stamp_pending (0x5930b8, = 1) follows the editor literals; entry 94 at the head; .bss 0x5fa2c8-0x5fbf1a. The ui_static / ui_button destructor COMDATs (0x431780 / 0x4317e0) sit after editor_dialog_destroy, their first use. |
| 0x4323e0 (frame start) | Gamewide_show_net_stats (0x5931b0, = 1) follows the states' last literal "Load" (0x5931a8); entry 100 at the head. |
| 0x433340 (player.cpp start) | hero_model_names (0x5934d0) follows the frame pool, which ends with the four ccmds' strings (0x5933c0-0x5934c9); cmd_net_stats reads 0x5931b0. So 0x432f80-0x433280 are the frame unit's, as the 2026-10-15 re-tag says. |

Literal order inside each pool: functions in address order, each function's literals in reverse order of use
(screenshot_save, FrameEnd, gameplay_load_cursors, editor_dialog_show).

Corrections for the source-map row text: the editor's ".data 0x59309c-0x5930a4" is its literal pool
0x59309c-0x5930b7 (no data), its .bss runs to 0x5fa2c7 (not 0x5fa29c); the states' .data is 0x5930b8-0x5931af,
the frame's 0x5931b0-0x5934cb.

## Functions

### unnamed_editor_dialogs_0x430bc0.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x430bc0 / 0x430bd0 | init_empty thunk / body: the Color array editor_dialog_colors[6] (empty inline constructor) | exact / likely (which object) |
| 0x430be0 | editor_dialog_init | exact |
| 0x430c80 | editor_dialog_show | likely (button blocks as a macro) |
| 0x4315c0 | editor_dialog_handle_event | likely (exit shape) |
| 0x431640 | editor_dialogs_free_closed | exact |
| 0x4316a0 | editor_dialog_destroy | exact |
| 0x431780 / 0x4317e0 | implicit ~ui_static / ~ui_button (COMDAT, comment only; D3100) | exact |

### unnamed_states_0x431870.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x431870-0x4318a0 | loadgame_window (ui_window): sinit / init / register_dtor / dtor | exact |
| 0x4318b0-0x4318e0 | loadgame_window_unused (second ui_window, no other use) | exact |
| 0x4318f0-0x431930 | the two list boxes, one declaration (one initialiser) | exact, 0x431900 likely |
| 0x431950 | implicit ~ui_listbox (COMDAT, comment only) | exact |
| 0x4319a0-0x4319d0 | loadgame_title_static | exact |
| 0x4319e0-0x431a20 | the two buttons, one declaration | exact, 0x4319f0 likely |
| 0x431a40 | game_change_level | likely (the three strcpy branches) |
| 0x431b70 / 0x431b80 / 0x431ba0 | ccmd_us_open_demo, cmd_us_open_demo | exact |
| 0x431c60 / 0x431cb0 / 0x431df0 / 0x431e00 | state_boot_enter, build_stamp_show, state_boot_leave, state_boot_frame | exact |
| 0x431e20 / 0x431e70 | state_title_enter (likely: the ?: form), state_title_frame (exact) | |
| 0x431e80 | state_loadgame_enter | likely (store order) |
| 0x432200 | loadgame_level_list_cb | likely |
| 0x4322c0 / 0x4322e0 / 0x432370 / 0x4323d0 | loadgame_list_dblclick_cb, loadgame_load_cb, state_loadgame_frame, state_loadgame_leave | exact |
| - | loadgame_fill_save_list (static inline, written out three times in the binary; name ours) | likely |
| - | loadgame_debug_hook (the 0x40acc0 calls with an argument; D3102) | unsure |

### unnamed_frame_0x4323e0.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4323e0 / 0x4323f0 / 0x432410 | ccmd_show_framerate, cmd_show_framerate | exact |
| 0x4324d0 / 0x4324e0 | game_status_message_timer (RealtimeTimer) | exact |
| 0x4324f0 | screenshot_save | likely (local array initialisers) |
| 0x432660 / 0x4326c0 | game_pause, game_unpause | exact |
| 0x432710 | PollInputEvents | likely (the debug-key block) |
| 0x432860 / 0x4328a0 | ui_poll_and_process, input_flush | exact |
| 0x4328b0 | FrameEnd | likely (two store orders, block-local sizes) |
| 0x432d60 / 0x432d80 / 0x432d90 | draw_loading_icon, game_pause_count_dec / _inc | exact |
| 0x432da0 | game_is_paused | likely (shift form) |
| 0x432db0 / 0x432e50 / 0x432e60 | gameplay_load_cursors, cursor_set_type, cursor_draw | exact |
| 0x432f50 / 0x432f70 | script_disable_player_input, script_enable_player_input | exact |
| 0x432f80-0x433280 | show_hud_info, targa, net_stats, gr_info: thunk / init / cmd each | exact |
| - | screenshot_debug_hook (the 0x4f13e0 call with an argument; D3102) | unsure |

Globals defined: editor .bss (editor_dialog_count, editor_dialog_colors, editor_dialog_list); states .data
(build_stamp_pending, states_flag_5930bc, play_intro_movies) and .bss (the load-game gadgets,
ccmd_us_open_demo, game_change_level_early 0x5fbf18, cheat_gold_5fbf19, US_open_demo); frame .data
(Gamewide_show_net_stats, cursor_type, frame_last_mouse_x / y) and .bss (game_flags, game_mouse_x / y, the
cursor bitmaps, game_status_message and its timer, the byte flags 0x5fc014-0x5fc022, input_debug_key_used /
_seen 0x5fc028 / 0x5fc029, game_pause_count, five console commands). None was defined elsewhere in src/decomp.

## Open questions

- The editor unit's start: no .data evidence (above). 0x5fa27c (4 bytes in its .bss) has no reference.
- loadgame_window_unused (0x5fb600): constructed and destroyed only.
- 0x24268f0 / 0x24268f4 (two gr_info counters) may be the first vector of a levelrender array (D3104).
- game_change_level's second parameter type (D3105).
- gr_set_alpha_test(true) after regaining the foreground (0x432747): the sum.symbols name of 0x508110 may not
  fit this use (a state restore after Alt+Tab).
- Whether the empty debug hooks were defined in these units (D3102).
- ui_process's second argument (1 at both callers).

## Doc disagreements

- **main-loop.md**, "One frame", step 4: bit 1 of 0x5fbfd8 is GF_MULTIPLAYER (game_flags), not a debug flag.
  Step 6: FUN_00431640 is editor_dialogs_free_closed. FrameEnd draws the FPS counter only when no screen is
  open (interface_get_screen() == 0); the render statistics never appear (nothing sets
  gr_info_total_time); FrameEnd also draws the cursor, the cutscene loading icon (state 4), saves the
  screenshot requested by Print Screen, skips gr_flip while game_flags 0x8000 is set and ends with
  snd_do_frame. The status message it can draw (game_status_message) is never written.
- **main-loop.md**, level changes: game_change_level also clears level_prev_filename, sets
  level_entry_point = 1, sets level_entered_by_load when a save is pending or the level is masad (index 3)
  with script "masad", and sets the write-only byte 0x5fbf18 when called before the first state.
- **controls.md**, "How a key press travels" step 3: Shift+Esc quits only in single player; in multiplayer it
  calls mp_player_disconnected (reason 2 for the host, 0 otherwise) and does nothing while paused.
  PollInputEvents first waits (100 ms sleeps, pause counter raised) while the window is in the background,
  tracks the debug key ` (0x29), and has a debug Backspace (0x800e) key flush that cannot trigger (D3103).
- **script-editor.md**: agrees (script_save reports through editor_dialog_show). Could add: the dialog is a
  modal ui_window of the given width, with up to 10 text lines and the button sets OK / OK-Cancel /
  Retry-Cancel / Yes-No / Yes-No-Cancel; the script editor always passes OK, no callbacks, flags 0xb.
- **sum.symbols**: ui_static__dtor / ui_button__dtor / ui_listbox__dtor are compiler-generated implicit
  destructors (D3100); build_stamp_get_name takes (out, size); multi_set_netgame_state's argument is a
  send-now flag (it broadcasts the session state); empty_frame_hook_40acc0 is also called with arguments;
  the "(module - -> player.cpp ...)" comments on 0x432f80-0x433280 are stale.

## Game bugs (none recorded in game-bugs.md yet)

1. **screenshot_save leaks the file list** (0x432552-0x43263f): the nodes from file_find_files are never freed,
   one list per Print Screen. The next number comes from the last name the scan returns (`list.prev`), not
   the highest, so on a file system that does not return sorted names a screenshot can overwrite an older
   one. *Observed* code, reachable.
2. **editor_dialog_handle_event calls the wrong callback** for the third button: it tests callbacks[2] and
   calls callbacks[1] (`call [esi + 0x28]`, 0x431628). Latent: the script editor passes no callbacks.
3. **editor_dialog_show** leaves d->buttons unset (operator new memory) for a buttons value above 4, which
   editor_dialog_destroy then deletes; with an empty text (0 lines) the text buffer is used uninitialised.
   Latent (the callers pass 0 and a message).
4. **loadgame_level_list_cb** checks the selected entry for NULL before the debug hook but dereferences it
   anyway for the refill (0x43222c). Latent: state 8 is never entered.
5. **The debug Backspace flush cannot trigger** (D3103): PollInputEvents tests modifier 0x8000, key_mark sets
   0x800. Dead code.
6. **Dead displays**: `gr_info` prints nothing (gr_info_total_time is never set), `show_hud_info` and
   `net_stats` toggle variables nothing reads, and the "Screenshot written to" message is never shown
   (screenshot_save hands it to an empty function; game_status_message has no writer).
