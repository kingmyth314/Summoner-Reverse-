# Phase 3: the level completion, AI movement, AI think and dialogue box units (decomp68)

Written from Sum.exe's disassembly only (`node tools/asm.mjs fn`, raw .data / .rdata bytes read from
Summoner/Sum.exe). No Summoner 2 code was read. All four .cpp files pass
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include <file>` with no
diagnostics. The header tree is the repo's src/decomp/include as of a7d9e66 plus the changes in
CHANGED-HEADERS.txt; every .cpp of src/decomp/game and src/decomp/vsdk gives the same result with this tree as
with the repo's (all clean).

All 123 functions of the two ranges are accounted for (0x4b0960-0x4b7610 and 0x4d6660-0x4d7b10):
72 exact, 46 likely, 5 unsure. Nothing in the two ranges is left. Four unit files, all placeholders
(no path string, no strings.tbl header for any of them), so modules.txt is empty (it lists named files only).

| File | Unit | Functions | Exact / likely / unsure |
|---|---|---|---|
| game/unnamed_level_complete_0x4b0960.cpp | 0x4b0960-0x4b140f, .data 0x59d694-0x59d6e3, .bss 0x247e830-0x247e83b | 10 | 7 / 3 / 0 |
| game/unnamed_ai_move_0x4b1410.cpp | 0x4b1410-0x4b513f, .data 0x59d6e4-0x59d7c3, .bss 0x247e840-0x247e8c3 | 24 | 10 / 12 / 2 |
| game/unnamed_ai_think_0x4b5140.cpp | 0x4b5140-0x4b77ef, .rdata 0x581bb0, .bss 0x247e8c8-0x247e963 | 32 | 13 / 16 / 3 |
| game/unnamed_dialogue_box_0x4d6660.cpp | 0x4d6660-0x4d7b2f, .rdata vtables 0x581c4c-0x581c6f, .bss 0x25443b0-0x2546197 | 57 | 42 / 15 / 0 |

## Unit boundaries and evidence

- **Level completion / COM_SEQ (0x4b0960-0x4b140f).** Follows multispell.cpp (0x4afca0-0x4b0957, tagged by
  strings.tbl id 238, no .data). No initialiser. Its .data 0x59d694-0x59d6e3 is one block: the three boss names
  not pooled elsewhere ("Red Oni", "Galienne#$kingroom", "Barbarian Fighter#boss", reverse order of use) then
  "resetting %s to attack %s\n" (mp_handle_combat_sequence), so COM_SEQ is in the same object as the level
  code. Its .bss 0x247e830 continues multispell's 0x247e824-0x247e82f: adjacency only, not proof of
  multispell.cpp (D4808). Ends before the initialiser run 0x4b1410.
- **AI movement (0x4b1410-0x4b513f).** Opens with the initialiser run of entries 303 / 304 (the two ccmds);
  .data turn_scaler 0x59d6e4, the speed factor table 0x59d6e8, then its literals to 0x59d7c3 (the last is the
  "Destroying AA" message of summon_emerge_update); .bss 0x247e840 (ai_mode_frame_counts) to 0x247e8c3
  (Use_turn_anims). Ends before the next initialiser run.
- **AI think (0x4b5140-0x4b77ef).** Opens with initialiser entry 305 (an empty initialiser); no .data; the
  team priority table at .rdata 0x581bb0 comes after the movement unit's constants (-1.5 at 0x581ba8, 120 at
  0x581bac) and before its own (-4 0x581bf0, 0.0005, 7, 1/3000, 5/(2 pi) 0x581c00, 3.2 0x581c04); .bss
  0x247e8c8-0x247e963 (the formation debug arrays, the function-static timer 0x247e91c and its guard
  0x247e95c). pathfinding_visibility.cpp starts at 0x4b77f0.
- **Dialogue boxes (0x4d6660-0x4d7b2f).** Opens with the initialiser run of entries 359-368 after
  level_scripts.cpp's cmd_door_toggle (0x4d6600); conversation.cpp starts at 0x4d7b30 (entry 369). The
  strings it uses, ids 949 / 950, sit in an untitled strings.tbl block after new_mainmenu.cpp's id 948 (the
  `// new_mainmenu.cpp` header is followed by an empty `//` line), so no file name. .bss 0x25443b0-0x2546197.

## Open questions

- 0x25443bc-0x25443fb (0x40 bytes) inside the dialogue unit's .bss: no instruction of Sum.exe reads or writes
  it. Not defined (an unused global or array of the unit; its type is unknown).
- 0x4b5140 / 0x4b5150: the think unit's initialiser is a bare RET (a definition whose dynamic initialiser
  is empty, as ai.cpp's init_empty_472000); which object is not known, so none is written.
- ai_think's four calls to empty_function (0x4f13e0) are written as four empty ai_state members (names
  ours); they could be fewer functions.
- ai_avoid_characters loads the owner's group flag byte (`mov cl, [eax+0x4c]`, 0x4b1b73) and never uses it:
  probably a dead local in the source; not written.
- ai_formation_slot_position looks the group leader up and drops the result (0x4b52f0).
- dialogue_line::draw / dialogue_topic_entry::draw kinds 1 and 2: one tail-merged loop in the binary, two
  case bodies here.
- The empty-in-release profile_label scopes: their labels are not in the binary (function names used).

## Doc and symbol disagreements

- sum.symbols marks dialogue_text_box (0x2544408) and dialogue_topic_box (0x2545b68) module new_dialogue.cpp:
  both are defined by this unit's initialisers (0x4d6660 / 0x4d66e0); new_dialogue.cpp's .bss is 0x5bc418-.
- sum.symbols: 0x4d7140 "conversation_reset" is dialogue_box_base::reset (it clears a box's scroll and ring),
  0x4d7650 "conversation_start" is dialogue_box::reset (vtable slot 0), 0x4d7920 "dialogue_box_reset_thunk"
  is dialogue_topic_list::reset; the old header declared base::reset at 0x4d7920 (D4800).
- sum.symbols types dialogue_more_text / dialogue_end_text as char*; they are localize_strings slots
  (char **, passed to localize_string_or_invalid).
- conversation.h described dialogue_line +0x14 (_unk14) as "nonzero: conversation_select_topic keeps the
  scroll on this line": it is the line's text, NULL for the spacer line between two texts.
- source-map.md's per-reference table lists 0x4d6730 under new_mainmenu.cpp; 0x4d6730 is
  dialogue_topic_box__ctor (no string reference) and the ids 949 / 950 are loaded at 0x4d6890 / 0x4d68b0, in
  the untitled block after id 948.
- sum.symbols mp_mark_difficulty_complete: "ORs bits 26..26+n into +0x30" - it assigns the mask (see the
  game bugs).
- sum.symbols ai_clear_target_and_idle: it writes +0x38, ai_state::priority, not the target (+0x60).
- sum.symbols ai_follow_repath "thiscall ai (target)": two arguments, (self = the owner, target).
- ai.h: ai_target_query is 0x18 bytes, not 0x14 (D4804); ai_combat_frames_watched (0x247e84c) is
  ai_mode_frame_counts[3] (D4805); ai_follow_target (0x247e878) is written by the AI combat unit and never
  read anywhere.
- ai.h's goal handler block said void for all of them (D4803); ai_turn_towards' second argument was int and
  ai_follow_path's argument int (D4802).

## Game bugs (candidates)

- **mp_mark_difficulty_complete wipes the level progress** (0x4b0b1e, `mov [eax+0x30], edx`): when the final
  level (index 25) is completed, every player whose .plr flags lack the new difficulty bits gets the flags
  word replaced by those bits alone, clearing the 26 level-completion bits (and any other bit of the word).
  The intent was probably `|=`; it could also be a deliberate "start the next difficulty from the first
  level". Multiplayer only, saved to the .plr (mp_player::profile_flags).
- **Topic list "more" / "end" hover uses the wrong coordinate** (dialogue_topic_entry::draw, 0x4d6fe8 /
  0x4d707c): the bottom test compares the mouse x with box.y + text height (`cmp eax, ecx` with eax still
  the x), and the box's top left is used instead of the entry's own position: the highlight colour shows for
  the wrong mouse positions. Visual (the clicks are dialogue_frame's, not checked here).
- **Lines past the 64-line ring lose their speaker** (dialogue_box::add_text, 0x4d72da / 0x4d7301 push 0):
  once the conversation text ring is full, new lines are initialised with no talker, so
  conversation_update_talk_anims plays no talk move for them. Only in very long conversations.
- **ai_think_party_update dereferences a missing group member** (0x4b7574): the summon distance check calls
  living_from_handle on each group member and reads its char_info without a NULL test; a stale member
  handle would crash (summoned creature in a party group whose member died / left, single player or the
  summon's own machine).
- ai_formation_slot_position reads owner->ai.group->leader without a NULL test (0x4b52e6); its callers in
  this unit check the group first.
- The box draws' visibility test compares the visible height with the scroll offset instead of the line's
  top (0x4d779e / 0x4d7a48), so every line below the box is drawn (and clipped): wasted draw calls only.

## Functions

### unnamed_level_complete_0x4b0960.cpp (10 functions: 7 exact, 3 likely)

| Address | Function | Confidence |
|---|---|---|
| 0x4b0960 | mp_send_level_complete | exact |
| 0x4b09c0 | mp_handle_level_complete | exact |
| 0x4b09f0 | mp_mark_level_complete | exact |
| 0x4b0a50 | mp_send_difficulty_complete | exact |
| 0x4b0ab0 | mp_handle_difficulty_complete | exact |
| 0x4b0ae0 | mp_mark_difficulty_complete | exact |
| 0x4b0b50 | mp_level_find_boss | likely |
| 0x4b1150 | mp_level_complete_check | exact |
| 0x4b11a0 | mp_send_living_combat | likely |
| 0x4b1280 | mp_handle_combat_sequence | likely |

### unnamed_ai_move_0x4b1410.cpp (24 functions: 10 exact, 12 likely, 2 unsure)

| Address | Function | Confidence |
|---|---|---|
| 0x4b1410 | static_init_ccmd_Turn_anims | exact |
| 0x4b1420 | init_ccmd_Turn_anims | exact |
| 0x4b1440 | cmd_Turn_anims | exact |
| 0x4b1500 | static_init_ccmd_turn_scaler | exact |
| 0x4b1510 | init_ccmd_turn_scaler | exact |
| 0x4b1530 | cmd_turn_scaler | exact |
| 0x4b15a0 | ai_find_nearest_wall_hit | likely |
| 0x4b18e0 | ai_probe_move_for_walls | exact |
| 0x4b1a00 | closest_approach_time | likely |
| 0x4b1aa0 | ai_avoid_characters | unsure |
| 0x4b24f0 | vector3__dot (COMDAT of the inline vector3::dot, vector.h; no source here) | exact |
| 0x4b2510 | ai_steer_jitter | likely |
| 0x4b2670 | ai_path_closest_point | likely |
| 0x4b2890 | ai_wall_avoid_steer | likely |
| 0x4b2af0 | ai_turn_towards | likely |
| 0x4b3010 | ai_follow_path | unsure |
| 0x4b3c80 | ai_travel_steer_update | likely |
| 0x4b4100 | ai_mode_traveling_frame | likely |
| 0x4b4590 | ai_mode_cast_spell_frame | exact |
| 0x4b4660 | summon_emerge_update | likely |
| 0x4b4920 | ai_cast_spell_update | likely |
| 0x4b4ad0 | ai_mode_disabled_frame | exact |
| 0x4b4d80 | ai_mode_hiding_frame | likely |
| 0x4b4eb0 | ai_mode_sneaking_frame | likely |

### unnamed_ai_think_0x4b5140.cpp (32 functions: 13 exact, 16 likely, 3 unsure)

| Address | Function | Confidence |
|---|---|---|
| 0x4b5140 | static_init_empty_4b5150 | unsure |
| 0x4b5150 | init_empty_4b5150 | unsure |
| 0x4b5160 | ai_group_assign_formation | likely |
| 0x4b52e0 | ai_formation_slot_position | likely |
| 0x4b53c0 | ai_score_target | likely |
| 0x4b5790 | ai_retaliate_on_hit | likely |
| 0x4b5900 | ai_pick_attack_target | likely |
| 0x4b5a80 | ai_follow_repath | likely |
| 0x4b5bd0 | ai_follow_character | likely |
| 0x4b6000 | ai_goal_do_attack | likely |
| 0x4b6240 | ai_goal_do_follow | exact |
| 0x4b62a0 | ai_goal_do_travel | likely |
| 0x4b63e0 | ai_goal_do_nothing | exact |
| 0x4b63f0 | ai_goal_do_approach_party | exact |
| 0x4b6430 | ai_goal_do_combat_mode | likely |
| 0x4b64c0 | ai_goal_do_cast_spell | exact |
| 0x4b6500 | ai_goal_do_cast_at_target | exact |
| 0x4b6560 | ai_goal_do_cast_at_point | exact |
| 0x4b65c0 | ai_goal_do_move_to | exact |
| 0x4b6690 | ai_goal_do_move_to_timed | exact |
| 0x4b6710 | ai_goal_do_hide | exact |
| 0x4b6790 | ai_goal_do_sneak | exact |
| 0x4b6810 | ai_goal_do_special_attack | likely |
| 0x4b68c0 | ai_goal_do_pickup | exact |
| 0x4b6940 | ai_party_follow_think | likely |
| 0x4b6a30 | ai_clear_target_and_idle | exact |
| 0x4b6a70 | qsort_cmp_float_field4 | exact |
| 0x4b6a90 | ai_choose_attack_slot | likely |
| 0x4b6c10 | ai_party_spread_attackers | unsure |
| 0x4b71b0 | atexit_nop_4b71b0 | likely |
| 0x4b71c0 | ai_think_party_update | likely |
| 0x4b7610 | ai_think | likely |

### unnamed_dialogue_box_0x4d6660.cpp (57 functions: 42 exact, 15 likely)

| Address | Function | Confidence |
|---|---|---|
| 0x4d6660 | static_init_dialogue_box_2544408 | exact |
| 0x4d6670 | init_dialogue_box_2544408 | exact |
| 0x4d6680 | register_atexit_dialogue_box_2544408 | exact |
| 0x4d6690 | atexit_dtor_dialogue_box_2544408 | exact |
| 0x4d66a0 | dialogue_box__ctor | exact |
| 0x4d66e0 | static_init_dialogue_topic_box_2545b68 | exact |
| 0x4d66f0 | init_dialogue_topic_box_2545b68 | exact |
| 0x4d6700 | register_atexit_dialogue_topic_box_2545b68 | exact |
| 0x4d6710 | atexit_dtor_dialogue_topic_box_2545b68 | exact |
| 0x4d6720 | dialogue_box__dtor | exact |
| 0x4d6730 | dialogue_topic_box__ctor | exact |
| 0x4d6760 | static_init_color_2544400 | exact |
| 0x4d6770 | init_color_2544400 | exact |
| 0x4d6790 | static_init_color_2545b64 | exact |
| 0x4d67a0 | init_color_2545b64 | exact |
| 0x4d67c0 | static_init_color_25443b0 | exact |
| 0x4d67d0 | init_color_25443b0 | exact |
| 0x4d67f0 | static_init_color_2545b5c | exact |
| 0x4d6800 | init_color_2545b5c | exact |
| 0x4d6820 | static_init_color_25443b8 | exact |
| 0x4d6830 | init_color_25443b8 | exact |
| 0x4d6850 | static_init_color_2546194 | exact |
| 0x4d6860 | init_color_2546194 | exact |
| 0x4d6880 | static_init_dialogue_more_text | likely |
| 0x4d6890 | init_dialogue_more_text | likely |
| 0x4d68a0 | static_init_dialogue_end_text | likely |
| 0x4d68b0 | init_dialogue_end_text | likely |
| 0x4d68c0 | dialogue_split_quotes | likely |
| 0x4d69d0 | dialogue_box__layout_lines | exact |
| 0x4d6a20 | dialogue_box__alloc_label | exact |
| 0x4d6a50 | dialogue_box__free_label | exact |
| 0x4d6a90 | dialogue_topic_box__layout_entries | exact |
| 0x4d6ad0 | dialogue_line__init | exact |
| 0x4d6b80 | dialogue_line__draw | likely |
| 0x4d6de0 | dialogue_topic_entry__init | exact |
| 0x4d6e70 | dialogue_topic_entry__draw | likely |
| 0x4d70f0 | dialogue_box_base__ctor | likely |
| 0x4d7130 | dialogue_box_base__dtor | exact |
| 0x4d7140 | conversation_reset | exact |
| 0x4d7160 | dialogue_box__set_rect | exact |
| 0x4d7190 | dialogue_box__add_text | likely |
| 0x4d7410 | dialogue_box__first_visible_line | exact |
| 0x4d7480 | dialogue_box__last_visible_line | exact |
| 0x4d74f0 | dialogue_box__scroll_next_page | likely |
| 0x4d7600 | dialogue_box__line_fits | likely |
| 0x4d7650 | conversation_start | exact |
| 0x4d7680 | dialogue_text_box_draw | likely |
| 0x4d7800 | dialogue_topic_box_add | exact |
| 0x4d78b0 | dialogue_box__item_visible | likely |
| 0x4d7900 | dialogue_box__item_y | exact |
| 0x4d7920 | dialogue_box_reset_thunk | exact |
| 0x4d7930 | dialogue_topic_box_draw | likely |
| 0x4d7ab0 | dialogue_topic_box_reset | exact |
| 0x4d7ac0 | dialogue_topic_box_add_kind3 | exact |
| 0x4d7ae0 | dialogue_screen_open | likely |
| 0x4d7b00 | dialogue_screen_close | exact |
| 0x4d7b10 | dialogue_show_line | exact |

