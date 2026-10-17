# match24 notes (cl 12.00.8966 = VC6 SP5, game units /O2 per vc6-flags.txt)

Tree: `match24/src` (copied from `snapO/src/decomp`). Compiler: `scratchpad/vc6sp5/VC98`. Decisions: D8300-D8305 in
`src/include/DECISIONS.md`. Changed files: `CHANGED.txt`.

**Total over the 40 files: 575 -> 601 byte matches (of 706 compared functions); no match lost (checked address by
address, `sum.mjs before.json after.json`).** mingw `-fsyntax-only -std=c++98`: 259 files, 0 errors (a header changed).

## Per file (before -> after under 8966)

| file | before | after | won |
|---|---|---|---|
| multiplayerdata.cpp | 5/10 | 10/10 | 0x4af340 broadcast_new_info, 0x4af400 pack_player_info, 0x4af760 handle_net_info, 0x4af7d0 send_player_data, 0x4af990 handle_player_data |
| new_buysell.cpp | 37/42 | 38 | 0x405bc0 goto_inventory |
| unnamed_states_0x431870.cpp | 9/14 | 11 | 0x431a40 game_change_level, 0x4322e0 loadgame_load_cb |
| multicontainer.cpp | 9/13 | 11 | 0x488c50 handle_item_get, 0x488eb0 handle_container_close |
| gameplay.cpp | 35/39 | 36 | 0x437e50 party_level_reset |
| unnamed_party_inventory_0x46b460.cpp | 26/30 | 28 | 0x46b460, 0x46b4b0 (find_next_in_category x2) |
| new_playmenu.cpp | 12/16 | 14 | 0x421590 load_bitmaps, 0x421820 handle_click |
| unnamed_gamelist_0x4705b0.cpp | 12/14 | 13 | 0x470b60 handle_deny_jn |
| unnamed_level_complete_0x4b0960.cpp | 6/10 | 8 | 0x4b09c0, 0x4b0ab0 |
| item_paging.cpp | 5/8 | 6 | 0x46cb50 find_player_slot |
| unnamed_lobby_0x43ef30.cpp | 24/25 | 25/25 | 0x43f450 parse_channel_list |
| unnamed_input_0x4343a0.cpp | 7/9 | 9/9 | 0x4343d0 cmd_slew, 0x434660 camera_process_input |
| multiendgame.cpp | 7/9 | 9/9 | 0x4814e0 player_disconnected, 0x4815c0 handle_leave_game |
| level_script_slums.cpp | 5/7 | 6 | 0x4fb160 ls_lenele1b_action |
| multi_door.cpp | 4/6 | 5 | 0x4aa780 handle_door_toggle |
| all other files of the list | unchanged | | frame 18/21, evil_joseph 14/19, evil_urath 13/18, ai_groups 13/18, multisave 4/8, lobj_raycast 3/7, worldmap_automap 31/35, items 27/31, levelselect 21/25, murod_oni 10/14, multichat 13/13, new_multichat 8/12, experience 5/8, summoner_cd 3/6, dialogue 4/6, spell 35/38, options 32/35, new_gamelist 26/29, conversation 24/27, level_script_worldmap 20/23, mp_join 14/17, effectstream 11/14, cutscene_camera 9/12, mlayout 7/9, credits 7/9 |

Closer but not matching (source kept in the closer form): buysell_handle_click and mp_levelselect_handle_click
(element pointers), worldmap_frame (`vector3 d; d = a - b;`: up to the fsqrt result's pops), chat_input_handle_key
(early `return key` plus the char path returning itself: up to the Esc block's position).

Markers: every function that matches now says exact ("exact (VC6 byte match, match24; was likely)": 69 raised,
most of them matched before this pass under 8966 without the marker update); markers that said exact but differ
under 8966 were lowered to likely: buysell_page_up, item_create, character_meets_item_requirements, playmenu_frame,
experience_parse_level, cd_set_root (no-CD byte edit, not chased), spells_set_page, conversation::clear_speaker,
ls_worldmap1_frame, effect_stream_start, effect_stream_update, ls_lenele1b_init.

## What worked (D8300-D8304)

1. **memcpy packet reads (D8302).** The long-open `mov eax, [eax]` of small packet handlers (D5812 / D7203) is
   `memcpy(&local, data, 4)`: deny_jn, level_complete, difficulty_complete, item_get, container_close, leave_game,
   door_toggle, player_data. The packet writers already use memcpy (MP_ADD_DATA); FreeSpace's multi code (the
   engine's ancestor) reads with `GET_DATA`, a memcpy macro. Worth applying to the other `*(int *)data` handlers
   (mp_handle_respawn and the D7203 list are in other agents' files).
2. **MP_ADD_FLOAT without parentheses (D8301, header).** `float t = (c->hp);` is fld/fstp under 8966, `float t =
   c->hp;` integer moves. A general hint: parenthesised macro arguments can change float copies.
3. **Shared trailing statement written once (D8304)**: 8966 tail-duplicates it with unmerged pops
   (player_disconnected, handle_player_data, party_level_reset).
4. **If / else of two calls for a bool argument** (`push 1 / jmp / push 0`): ls_lenele1b_action (also for an `&&`
   chain), cmd_slew. A pointer argument stays a ternary (`save ? save->text : NULL`).
5. **Loop on the parameter** `for (start++; start < count; start++)` (`mov esi, [start]; inc esi`).
6. Store orders (playmenu_handle_click, broadcast_new_info, lobby_parse_channel_list), `int type = p->type;` read
   first (pack_player_info), branch layouts (game_change_level `level != -1` first, loadgame_load_cb `event ==
   PRESSED` wrapper, find_player_slot `if (i < 4) {..; return;} for (;;) DebugError`), `switch (key) { case
   KEY_ESC: }`, element pointers, one reassigned local (party_level_reset), `char page[4]` (D8300).
7. **Bool bit-field of a copied word (D8303, device)**: dword load + `shr al, n` comes only from a bool / char
   bit-field read from a dword copy (probes in `probe/p1-p3.cpp`). camera_process_input exact with it.

## Still differs, and why (D8305)

- `max(x, 0)` AND register: buysell_count_party_items, buysell_page_up, gamelist_page_up (D7104).
- game_is_paused (`mov eax, [game_flags]; shr al, 2`) and game_render_frame (`not al; shr al, 5`): 8966 narrows
  every copy of the global int to a byte load (16 forms + struct-copy devices tried); game_flags is probably a
  struct of bool bit-fields in the original (header question, not changed).
- playmenu_frame (key stored back to its slot), playmenu_open_save and door_pick_by_mesh (frame slot sharing:
  save / buttons at [esp], d / open / locked in one dword), mp_handle_actv_gm / join_request (dead `data` slot).
- Shrink-wrapped pushes and early-return placement: automap_labels_parse_table (-1 kept in EBX),
  mp_levelselect_draw_levels, state_game_enter (the default body placed inside the level_load failure code).
- Register choice: buysell_handle_click (`mov bl, 4`, -1 not in EBP), mp_levelselect_handle_click's list loop
  (declaration orders tried), experience_parse_level (two induction pointers), character_use_inventory_item (item
  load before frand), evil_joseph_ai_frame (submode loaded into EAX; the rest matches), state_title_enter,
  mp_build_game_info, item_page_in (the binary uses one more register), options_init (reload of options_edit after
  the struct copy: memcpy and pointer forms give the same).
- mp_handle_item_drop: the matrix is built in a temporary and copied as nine dwords; matrix has a user copy
  constructor and an operator= through vector3 (the assign form calls the vector3 copy constructor).
- mp_handle_combat_sequence: the byte fields are stored, reloaded as dwords and masked (`and ebx, 0xff`), the
  special byte into the dead `data` slot; memcpy reads did not give it.
- effect_stream_update / _start (case layout, store / return order), cutscene sample clamps (`fld bound; fld st(1);
  fcompp`: inline min / max helpers tried), chat_input_handle_key (Esc block placed last), ls_lenele1b_init (stack
  pops not merged, D7305), xp_for_level (loop entry test `cmp edx, 0x14`), the boss select_action functions
  (shared return blocks, D7305).
- summoner_cd: the analysed Sum.exe carries the no-CD byte edit; not chased.

## Logic errors / game bugs

No decomp logic error found; behaviour unchanged by every edit (the rewrites are equivalent: the merged
Enter-empty / Esc cancel, the reassigned `c` in party_level_reset reads the same handle, the clamps keep
their semantics). No new game bug. Note for others: D7506's "float copied through fld / fstp"
(write_character) may be a parenthesised macro argument (D8301).

## Header edits

- `include/summoner/multi/multi.h`: MP_ADD_FLOAT's `(v)` -> `v` (comment updated). Users compared old vs new under
  8966: multiplayerdata (won mp_send_player_data), unnamed_mp_join_0x4ab0f0 (14/17 both), unnamed_swi_0x488230
  (12/14 both).

## Tools (match24/)

`dd.mjs <rel> [addr|all] [ctx|full]` (vc6-compare -v plus side.mjs per differing function), `try.mjs` (root set to
this tree), `apply.mjs <rel> <variants.json> [names]`, `sum.mjs before.json after.json` (per-file, won / lost),
`markers.mjs after.json [--apply]`. Git Bash heredocs drop one level of backslashes: variant JSON with `\\n` or
`'\\0'` must be written with the Write tool.
