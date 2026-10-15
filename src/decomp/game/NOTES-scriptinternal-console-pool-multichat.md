# decomp44: script_internal.cpp, console.cpp, norlsallocpool.cpp, new_multichat.cpp

Written from Sum.exe v1.40's disassembly (`tools/asm.mjs`) for VC++ 6.0 (cl 12.00.8966). None of it has been
compiled with VC6; "exact" means the control flow, constants, calls and globals match the disassembly.
Summoner 2 and Dash Faction were used for names only (through sum.symbols).

Header tree: a copy of src/decomp/include at commit 277c102 (BASE-COMMIT.txt) with the changes in
CHANGED-HEADERS.txt and decisions D2300-D2310 in include/DECISIONS.md. Every src/decomp .cpp compiles against
it (mingw -fsyntax-only, same warnings as the repo tree), with the three one-line definition changes listed
in CHANGED-HEADERS.txt; the only new warning is console.cpp's deliberate `%f` with an int (the original's).

| File (decomp44/) | Original unit | Functions | Exact / likely / unsure |
|---|---|---|---|
| `game/script_internal.cpp` | `levelscripts\script_internal.cpp` (0x4db340-0x4de0a3) | 26 of 26 | 10 / 15 / 1 |
| `vsdk/console.cpp` | `console.cpp` (0x50a000-0x50af59) | 20 of 20 + 2 compiler helpers | 15 / 7 / 0 |
| `vsdk/norlsallocpool.cpp` | `norlsallocpool.cpp` (0x532360-0x532653; folder not known) | 14 of 14 | 9 / 5 / 0 |
| `game/new_multichat.cpp` | `new_multichat.cpp` (0x41e3d0-0x41ee8f) | 16 of 16 (4 initialiser helpers) | 9 / 7 / 0 |

`vsdk/norlsallocpool.cpp`: the file name is Summoner 2's; the folder is not known (source-map.md), so it
goes to src/decomp/vsdk/ with the header left at vsdk/os/norlsallocpool.h.

## Functions

### script_internal.cpp

| Address | Function | Confidence | Note |
|---|---|---|---|
| 0x4db340 | script_object_spawn_at_navpoint | likely | moves an entity that already started there instead of creating one |
| 0x4db420 | script_parse_navpoint | likely | cutscene #Navpoints; DebugError line 899 on a ground miss |
| 0x4db700 | script_parse_navpoints | likely | level #Navpoints; updates the last same-named navpoint or appends |
| 0x4dba30 | script_parse_objects | likely | #Objects record and its +Action: lines (jump table 0x4dc448) |
| 0x4dc4d0 | script_object_copy_from_template | likely | scripted hero copies the party copy's flag 0x2000000 and items 0-8 |
| 0x4dc570 | navpoint_find | exact | |
| 0x4dc600 | navpoint_get_position | exact | orientation from the heading only |
| 0x4dc6e0 | script_action_walk_to | likely | actions 0-2; move through appends path pieces |
| 0x4dc860 | script_action_wait_at | likely | target = navpoint fvec + own position |
| 0x4dc900 | script_action_do_state | exact | |
| 0x4dc930 | script_action_play_action | exact | |
| 0x4dc990 | script_action_die | exact | |
| 0x4dca10 | script_action_goto | exact | returns bool (D2304) |
| 0x4dca90 | script_action_wander | exact | |
| 0x4dcb10 | script_action_follow | exact | |
| 0x4dcbc0 | script_action_combat_mode | likely | |
| 0x4dccb0 | script_action_replace_weapon | likely | |
| 0x4dce60 | script_action_replace_armor | likely | |
| 0x4dd0b0 | script_entry_start | likely | jump table 0x4dd698 (type + 1) |
| 0x4dd720 | script_entry_advance | exact | |
| 0x4dd780 | script_entry_check_done | likely | byte table 0x4dda08, jump table 0x4dd9d4 |
| 0x4dda30 | script_object_near_player_dist | likely | |
| 0x4dda90 | script_navpoint_near_player_dist | likely | |
| 0x4ddaf0 | script_check_visit_count | exact | |
| 0x4ddb10 | script_entry_jump_to_sequence | likely | |
| 0x4ddc60 | script_build_avoid_spline | unsure | see open questions |

Data: script_action_names (0x5a045c, 33), script_navpoint_type_names (0x5a04e0), cutscene_saved_equipment
(0x2546760, 16 x {Character *, item_info *, slot}) and its count (0x2546820).

### console.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x50a000 | init_ccmd_print_to_file_thunk (compiler helper, not in sum.symbols) | exact |
| 0x50a010 | init_ccmd_print_to_file | exact |
| 0x50a030 | cmd_print_to_file | exact |
| 0x50a180 | console_command::console_command | likely |
| 0x50a210 | execute_command_line | likely |
| 0x50a560 | set_command_line | exact |
| 0x50a570 | get_next_char | exact |
| 0x50a5a0 | print_to_console (empty) | exact |
| 0x50a5b0 | process_command | likely |
| 0x50a9a0 | get_token | likely |
| 0x50aa30 | skip_whitespace | exact |
| 0x50aa50 | make_lowercase_token | likely |
| 0x50aab0 | read_word | exact |
| 0x50ab00 | read_quoted_string | exact |
| 0x50ab60 | init_char_types | likely |
| 0x50ac30 | init_ccmd_script_thunk | exact |
| 0x50ac40 | init_ccmd_script | exact |
| 0x50ac60 | cmd_script | exact |
| 0x50acd0 | load_script | likely |
| 0x50aef0 | console_printf | exact |
| 0x50af50 | console_set_font | exact |
| 0x50af60 | LineFile scalar deleting destructor (COMDAT for `delete lf`) | exact |

### norlsallocpool.cpp

0x532360 constructor (likely), 0x532380 destructor (exact), 0x532390 Release, 0x5323d0 AddBlock, 0x532420
InitBlocks, 0x532460 Init, 0x532480 Reset, 0x532490 Alloc, 0x5324e0 AllocZeroed (exact), 0x532510 Align,
0x532540 GetUsed, 0x532570 Grow, 0x532620 GetUsed_thunk (likely), 0x532630 SetOffset (exact).

### new_multichat.cpp

0x41e3d0 / 0x41e3e0 and 0x41e400 / 0x41e410 the two initialiser pairs (chat_lines' six RealtimeTimers,
chat_cursor_timer; exact), 0x41e420 chat_reset (exact), 0x41e430 chat_input_frame (likely), 0x41e4a0
chat_update_lines (likely), 0x41e590 chat_input_key_enter (exact), 0x41e5d0 chat_input_handle_key (likely),
0x41e6b0 chat_input_add_char_wrap (likely), 0x41e8e0 chat_draw (exact), 0x41e900 chat_draw_lines (likely),
0x41eb80 chat_get_origin (exact), 0x41ebd0 chat_draw_input (likely), 0x41ed50 chat_input_cancel (exact),
0x41ed60 chat_add_line (likely). Data: .rdata 0x5800f8-0x580133 (chat_width {310, 388, 496},
chat_origin {5,370} {6,463} {8,592}, chat_origin_fullscreen {5,460} {6,575} {8,736}; names ours).

## Open questions

- console.cpp: the literals of ccmd_script's constructor (0x5a6f74, 0x5a6f98) precede cmd_print_to_file's
  ("console_commands.txt" 0x5a6fa0), but init_ccmd_script's code sits after init_char_types. The object is
  written where the code is; the literal order suggests an earlier declaration or a different placement.
- console.cpp: 0x25dadf4 (between checking_status_for_command and ccmd_print_to_file) has no reference.
- console.cpp: the "" literals at 0x25db924 / 0x25db928 are in .bss (as in plainstring.cpp / skeleton.cpp).
- script_internal.cpp: str_copy_append_32 (0x4db250) and filename_has_cscript_suffix (0x4db2c0) sit between
  conversationctl.cpp's last function (cmd_dcamera) and this unit, untagged; their literal "_cscript.tbl" is
  pooled (0x5939b4), so it does not place them. They may be this file's first two functions.
- script_internal.cpp: static_init_script_editor_defaults / init_script_editor_defaults (0x4de0b0 / 0x4de0c0)
  follow the unit; their data (0x2556ab0) is in the script editor's .bss, so they belong to the next unit.
- script_build_avoid_spline: it computes the point 0.5 ahead of c into two variables, traces from c to the
  first, and traces again from the first to the second when they differ (never: they are copies). The
  source shape of that part is a guess (unsure).
- Local buffer sizes are guesses where the frame leaves room (script_parse_navpoints' name, chat_draw_input's
  68-byte buffer, load_script's 1024-byte path).
- print_to_console's second parameter: always 0; int or bool is not visible (the body is a bare ret).

## Doc disagreements

- docs/sum-exe/console.md says the PC constructor's missing "type" argument means "the command type is
  decided differently on PC (*inferred*; not yet analysed)". It is: every callback calls
  process_command(mask) with the ARG_* bits it accepts (ARG_NONE 1, STRING 2, INT 8, FLOAT 0x10, TRUE 0x40,
  FALSE 0x80, plus QUOTED 4, HEX 0x20, + 0x100, - 0x200, , 0x400); a mismatch prints "Error: '%s' invalid
  type" and longjmps out of the command. execute_command_line sets the three mode flags: a plain call sets
  calling_a_command_function and checking_status_for_command, "? cmd" only checking_status, "help cmd" /
  "man cmd" or any "?" in the line only doing_help; "debug cmd" traces the parse.
- console.md does not say that the console has no entry point in v1.40: print_to_console is empty,
  execute_command_line's only caller is load_script, whose only caller is cmd_script, itself a command.
  So the commands run only through a patch that calls execute_command_line (Timeslip's k### bindings) and
  .vcs scripts started from such a call.
- The print_to_file description says "console.txt"; the command writes console_commands.txt (the name of
  the file Timeslip's patch ships, docs/patches/timeslip.md).
- sum.symbols: char_types (0x25dae14) is int[256], not uint8[256] (D2309); the thunk 0x50a000 (jmp
  init_ccmd_print_to_file) is missing; init_char_types does not read the argument LoadSettings pushes.
- level_scripts.h: SCRIPT_ACTION_LOOP (0x0d) is the "die" action (it sets end mode 1, which is what
  level_script_restart sees); navpoint +0x20 is -1 / pick_layer's result for script navpoints, not 0.

## Game bugs noticed

- S1 (script_parse_objects): a "+No create" object with actions copies its trigger name from an
  uninitialised 256-byte stack buffer (only the normal path fills it) into the 32-byte trigger field with
  strcpy; stack garbage without an early NUL overruns into the next script object.
- S2 (script_parse_navpoint / _navpoints): "+Plane:" never takes effect. Level and cutscene navpoints get
  the floor's layer from the ground ray, camera and ambient navpoints -1, so the parsed value is always
  overwritten.
- S3 (script_action_replace_armor): with armour worn and the action's item "(none)" (-1), the early test
  reads item_types[-1].slot, 0x100 bytes before the table.
- S4 (script_action_replace_weapon / _armor): a removed item whose ItemType slot is not one of the expected
  ones (4/5/9, 6/7/8) is recorded with a stale slot; the saved-item count is never bounded (16 records).
- S5 (script_parse_objects): no limit checks on the 54 actions per object or the 200 / 32 objects.
- S6 (script_entry_start): a "die" action that cannot start (no move) still sets end mode -1, so the
  5-second retry never runs; "Unable to start script entry ..." and "Unable to find prop ..." are formatted
  and never shown.
- S7 (script_entry_start): in the game intro the hold test dereferences charinfo_find_move_any's result
  without a NULL check.
- C1 (load_script): the ".vcs" path is cut at the first '.' of the whole path (strchr, not strrchr), so a
  '.' in the install directory breaks every script; a "/* */" comment on one line opens a block comment, and
  the line numbers in "Error processing script line %d" skip comment lines.
- C2 (process_command): the debug trace prints the integer argument with "%f".
- C3 (console_command): a second command with an already registered name (or the 301st) stays unlinked with
  NULL fields, silently.
- M1 (chat_add_line): a message that wraps into more than six pieces makes the scroll size negative and
  writes past chat_lines; players' lines are limited to 63 characters by the input, so only a long system
  or remote text could reach it.
- M2 (chat_draw_lines): a whisper line without a space is drawn all blue, not in the highlight colour
  (cosmetic; whispers always have "<name> ").
