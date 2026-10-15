# decomp55: the script editor unit 0x4de0b0-0x4ecf90

Base: repo HEAD 893c6a5. Written only from Sum.exe disassembly (`node tools/asm.mjs`).

## Output

- `game/unnamed_script_editor_0x4de0b0.cpp` (the unit; part1-6.cpp are its pieces, concatenated in order).
  File name is a placeholder: no assert path or strings.tbl name for this unit.
- `include/summoner/unnamed_script_editor_0x4de0b0.h` (new unit header), other header edits in
  `CHANGED-HEADERS.txt`, D3500-D3506 appended to `include/DECISIONS.md`, README row added.

## Unit cuts

- Code 0x4de0b0-0x4ecf90: static initialisers entries 374-473 (472 ccmd_remove_nav, 473
  ccmd_render_s3d_navpoints; entry 474 at 0x4ed050 belongs to the next unit, cutscene_script_rewind_actors
  0x4ed550 is outside).
- .data 0x5a08a8-0x5a1573 (script_editor_mode_names and the editor's string/constant data).
- .bss 0x2546828-0x256b74f, all defined in the unit's .cpp (checked with tools/bsscheck.js).

## Coverage

All 508 functions of the range have a `// 0xADDR` line: 460 exact, 46 likely, 8 unsure. Nothing is left
unwritten.

- unsure (6): the empty initialisers 0x4de450, 0x4de470, 0x4de490, 0x4de4b0, 0x4de4d0, 0x4dff00 (bare `ret`;
  which empty-constructor object each belongs to is a guess from the order).
- likely: init_script_editor_defaults 0x4de0c0 (written as one aggregate array), create_ui, pick_hover,
  ray_hit_object, draw_navpoints, draw_marker, handle_input, script_action_format, list_ambient_sounds,
  list_door_sounds, place_creature, add_navpoint, update_buttons, fill_action_lists, on_delete,
  select_prev / focus_selection / select_first / select_next, refresh_action_editor, action_up_cb /
  action_down_cb, edit_button_cb, apply_action_params, script_create, script_editor_default_action_navpoint,
  script_write_level_script, script_write_objects, script_write_level_table, level_items_write_table,
  script_save, script_name_cb, ambient_name_cb, params_window_cb, params_ok_cb, choice_ok_cb,
  choice_listbox_cb, door_sound_radio_cb, door_sound_inputbox_cb, door_key_cb, door_item_cb, door_ok,
  creature_ok, level_item_cb, level_item_ok (control flow certain; register-level order of some float
  expressions, loop shapes or temporaries not proven).

## Decisions (DECISIONS.md D3500-D3506)

D3500 hover/selection as `script_editor_pick` records (0x2546bd0 / 0x255a008), mouse ray an ix_ray.
D3501 script_action_defaults fields. D3502 0x254a8b0 is the saved orientation matrix (sum.symbols calls it
script_editor_saved_pos), the saved position is 0x2563a58. D3503 script_editor_name_room and
script_editor_ambient_file_list are gadget fields. D3504 ui_inputbox +0x5ac char filter. D3505 four
misnamed sum.symbols functions. D3506 ui_listbox::select_by_data returns bool.
Also unnamed in sum.symbols, named here: 0x2563630 / 0x2551a60 (action up/down buttons), 0x255cab0 (an unused
button), 0x256b73d (camera key held), 0x256b748 (script_editor_insert_action), Color arrays 0x254f1f0 /
0x254f6f8, marker sizes 0x255e9a8 [4][3] and radii 0x255a7c4 [4].

## Disagreements with docs/sum-exe/script-editor.md

- Action panel button roles differ (up/down buttons exist; the "delete navpoint" path adds a $camera navpoint).
- Save also writes the cutscene's `$Level:` line.
- The "Give item" action gives gold.
- `%d.int` files (doc: unknown) are temporary tables written then renamed over the real .tbl (File::make_backup
  0x514ae0 is a rename).
- sum.symbols names wrong as in D3502, D3503, D3505.

## Game bugs (for docs/game-bugs)

.tbl / script corruption:
- script_write_objects writes `+Action` without its `$Name` when the start navpoint type does not match: the
  written table is unreadable for that action; `+Trigger` gets an extra blank line.
- script_write_level_table loops forever when the table lacks an exact "#Ambient Sounds" line, and drops
  ambient sounds attached to level markers.
- level_items_write_table duplicates records of other scripts through stale navpoint pointers, matches
  `#Level` only as a whole line and loops forever without `#End`.
- script_editor_on_delete copies one element past the navpoint array; level item navpoint pointers and
  cutscene actions are not updated, so saved .tbl files name the wrong navpoints.
- level_item_ok removal leaves stale indices (possible endless name loop).
- replace_navpoint_references updates only the first character.
Crashes:
- script_editor_default_action_navpoint dereferences NULL (+0x28) / uses &navpoints[-1] with no navpoints.
- apply_action_params kind 9 dereferences NULL; show_door_lock with no record; show_script_dialog strstr(NULL).
- delete_navpoint (adds a camera navpoint) has no 750-navpoint limit check.
Wrong values:
- Door window prints bytes with %.1f (garbage) and OK writes them back.
- Creature OK with level 0 zeroes HP/AP scale and sets every skill except #2 to 100.
- show_action_params (deletes an action) removing a cutscene script decrements the level script count.
- The hover arrow of a level object is drawn when the selection is an object.
- Enter in the door sound inputbox clears the key box focus; ambient OK leaks sound handles; the duplicate-name
  list counts inactive serpents differently.

## Left

Nothing unwritten. Open: the 6 unsure initialiser assignments; the 46 likely functions above.
