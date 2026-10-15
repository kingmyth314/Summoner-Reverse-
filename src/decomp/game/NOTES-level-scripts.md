# Phase 3: level_scripts.cpp (decomp27)

`game/level_scripts.cpp` is levelscripts\level_scripts.cpp of Sum.exe v1.40, code 0x4ceec0-0x4d6651
(static initialiser table entries 345-358), written from the disassembly only (`node tools/asm.mjs fn`,
raw .data bytes read from Sum.exe). No Summoner 2 or Dash Faction code was read. The file passes
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include
game/level_scripts.cpp` with no diagnostics, against the header tree in `include/` (the repo's tree at
bb5e521 plus CHANGED-HEADERS.txt); every existing src/decomp .cpp gives the same diagnostics with this tree
as with the repo's.

**Everything is written.** 160 sum.symbols functions in the range: 124 functions (96 exact, 27 likely,
1 unsure) and 36 initialiser / atexit helpers, of which 34 are covered by their definitions (exact) and
2 (0x4d5020 / 0x4d5030, an empty dynamic initialiser) are not reproduced. Nothing is left in the range.

Data written: .data savegame_thumbnail_bm (0x59e794), cur_level_index (0x59f120), level_script_table
(0x59f128, 51 records from the image), level_table (0x59fde8, 26 records, dynamic initialiser 0x4d5140);
.bss 0x2493a18-0x25443af (savegame_state, the six console commands, savegame_buffer, events_int_pool,
level_items, navpoints, the three StringPools and their buffers, script_objects, conversation_save_table,
quest_table and the counters).

## Order of the unit

Savegame first in the file is also the binary's order: the savegame object (0x4ceec0-0x4d3920), events.tbl
/ automap tables / layout (0x4d39c0-0x4d4160), game stages (0x4d42e0-0x4d4c4e), the save / load commands,
the initialisers of script_objects and the StringPools, the level table, then the level-script helpers
(0x4d54e0-0x4d6651). The objects with dynamic initialisers are defined where their initialiser code is
(definition order = initialiser order); their .bss addresses are in another order (ccmd_loadgame
0x2493a18 < ccmd_Preparse_scripts 0x2493a28 < savegame_state 0x2493a38 < ccmd_savegame 0x2494b20 <
ccmd_gamestage 0x24dfb30 < ccmd_door_toggle 0x24ed5f0 < script_filenames_string_pool < events_string_pool <
script_objects < ccmd_door_open < level_script_stringpool < ccmd_door_close), the same open question as in
the other pilots. The plain .bss variables are defined at the top in address order.

## Functions

| Address | Function | Confidence |
|---|---|---|
| 0x4ceec0 | static_init_savegame_state | exact |
| 0x4ceed0 | init_savegame_state | exact |
| 0x4ceee0 | register_atexit_savegame_state | exact |
| 0x4ceef0 | atexit_dtor_savegame_state | exact |
| 0x4cef00 | static_init_ccmd_Preparse_scripts | exact |
| 0x4cef10 | init_ccmd_Preparse_scripts | exact |
| 0x4cef30 | cmd_Preparse_scripts | exact |
| 0x4ceff0 | savegame_report_sizes | exact |
| 0x4cf260 | savegame_report_read_sizes | exact |
| 0x4cf500 | savegame_write_party_state | likely |
| 0x4cf680 | savegame_read_party_state | exact |
| 0x4cf6c0 | savegame_write_character | exact |
| 0x4cf790 | savegame_read_character | exact |
| 0x4cf8d0 | savegame_write_groups | likely |
| 0x4cfa00 | savegame_read_groups | exact |
| 0x4cfb80 | savegame_state__clear | exact |
| 0x4cfbb0 | savegame_init_buffer | exact |
| 0x4cfbd0 | savegame_data_offset | exact |
| 0x4cfc40 | savegame_get_visit_count | exact |
| 0x4cfc60 | savegame_set_visit_count | exact |
| 0x4cfc80 | savegame_inc_visit_count | exact |
| 0x4cfca0 | savegame_get_flag | exact |
| 0x4cfd30 | savegame_set_flag | likely |
| 0x4cfe00 | savegame_refresh_object_flags | exact |
| 0x4cfea0 | savegame_refresh_character_flags | exact |
| 0x4cff20 | savegame_get_bit | exact |
| 0x4cff80 | savegame_set_bit | exact |
| 0x4d0030 | savegame_get_int | exact |
| 0x4d00b0 | savegame_set_int | exact |
| 0x4d0130 | savegame_get_float | exact |
| 0x4d01c0 | savegame_set_float | exact |
| 0x4d0240 | savegame_find_flag_level | likely |
| 0x4d02d0 | savegame_init_conversation_slot | likely |
| 0x4d0590 | savegame_has_conversation_data | exact |
| 0x4d05f0 | savegame_get_speaker_bit | exact |
| 0x4d06e0 | savegame_set_speaker_bit | exact |
| 0x4d0870 | savegame_get_topic_bit | exact |
| 0x4d09e0 | savegame_set_topic_bit | exact |
| 0x4d0c00 | savegame_get_quest_bit | exact |
| 0x4d0c40 | savegame_set_quest_bit | exact |
| 0x4d0c90 | quest_evaluate | exact |
| 0x4d0d00 | savegame_level_item_add | exact |
| 0x4d0d60 | savegame_level_item_remove | exact |
| 0x4d0dd0 | savegame_level_item_has | exact |
| 0x4d0e20 | savegame_two_stage_set | unsure |
| 0x4d1110 | savegame_two_stage_get | likely |
| 0x4d12b0 | savegame_automap_label_add | exact |
| 0x4d1390 | savegame_automap_label_remove | likely |
| 0x4d14f0 | savegame_automap_labels_reset_level | likely |
| 0x4d1620 | savegame_automap_label_count | exact |
| 0x4d1630 | savegame_hot_buttons_clear_member | exact |
| 0x4d1660 | savegame_hot_button_clear | exact |
| 0x4d16a0 | savegame_hot_button_set | likely |
| 0x4d1770 | savegame_hot_button_get | exact |
| 0x4d17e0 | savegame_clear_buffer | exact |
| 0x4d1800 | savegame_layout_header | likely |
| 0x4d1820 | savegame_layout_thumbnail | likely |
| 0x4d1860 | savegame_layout_level_data | exact |
| 0x4d18e0 | savegame_layout_conversation | likely |
| 0x4d19e0 | savegame_layout_items | likely |
| 0x4d1a50 | savegame_layout_inventory | exact |
| 0x4d1a80 | savegame_layout_living | likely |
| 0x4d1ae0 | savegame_layout_camera | exact |
| 0x4d1b10 | savegame_layout_item_objects | exact |
| 0x4d1b40 | savegame_layout_quest_flags | exact |
| 0x4d1b80 | savegame_layout_level_items | exact |
| 0x4d1bb0 | savegame_layout_two_stage | exact |
| 0x4d1be0 | savegame_layout_automap_labels | exact |
| 0x4d1c10 | savegame_layout_hot_buttons | exact |
| 0x4d1c40 | savegame_write | exact |
| 0x4d1df0 | savegame_write_header | likely |
| 0x4d1f80 | savegame_write_thumbnail | exact |
| 0x4d1fb0 | savegame_write_items | exact |
| 0x4d2090 | savegame_write_inventory | exact |
| 0x4d2150 | savegame_write_living | exact |
| 0x4d2320 | savegame_write_camera | exact |
| 0x4d2370 | savegame_write_item_objects | exact |
| 0x4d25a0 | savegame_chunk_stub | exact |
| 0x4d25b0 | savegame_read | likely |
| 0x4d2800 | savegame_read_info | exact |
| 0x4d2910 | savegame_read_thumbnail_file | exact |
| 0x4d29f0 | savegame_read_header | exact |
| 0x4d2b70 | savegame_read_items | exact |
| 0x4d2c90 | savegame_read_inventory | exact |
| 0x4d2dc0 | savegame_read_living | likely |
| 0x4d3100 | savegame_read_camera | likely |
| 0x4d31e0 | savegame_read_item_objects | likely |
| 0x4d33f0 | savegame_read_level_items | likely |
| 0x4d34e0 | savegame_read_two_stage | likely |
| 0x4d3740 | savegame_read_automap_labels | exact |
| 0x4d3870 | savegame_set_pending_load | exact |
| 0x4d38b0 | savegame_pending_load_length | exact |
| 0x4d38d0 | savegame_load_pending | exact |
| 0x4d3920 | savegame_alloc | exact |
| 0x4d39c0 | events_parse_table | likely |
| 0x4d4020 | level_parse_automap_table | exact |
| 0x4d4150 | level_script_event_flag_clear | exact |
| 0x4d4160 | savegame_build_layout | exact |
| 0x4d42e0 | game_stage_set | likely |
| 0x4d47b0 | game_stage_get | exact |
| 0x4d4a50 | savegame_location_allowed | exact |
| 0x4d4bc0 | static_init_ccmd_gamestage | exact |
| 0x4d4bd0 | init_ccmd_gamestage | exact |
| 0x4d4bf0 | cmd_gamestage | exact |
| 0x4d4c50 | static_init_ccmd_savegame | exact |
| 0x4d4c60 | init_ccmd_savegame | exact |
| 0x4d4c80 | cmd_savegame | exact |
| 0x4d4e70 | static_init_ccmd_loadgame | exact |
| 0x4d4e80 | init_ccmd_loadgame | exact |
| 0x4d4ea0 | cmd_loadgame | exact |
| 0x4d4ff0 | static_init_script_object_timers | exact |
| 0x4d5000 | init_script_object_timers | exact |
| 0x4d5020 | static_init_empty_4d5030 | not reproduced |
| 0x4d5030 | init_empty_4d5030 | not reproduced |
| 0x4d5040 | static_init_stringpool_2539f18 | exact |
| 0x4d5050 | init_stringpool_2539f18 | exact |
| 0x4d5070 | register_atexit_stringpool_2539f18 | exact |
| 0x4d5080 | atexit_dtor_stringpool_2539f18 | exact |
| 0x4d5090 | static_init_stringpool_24f6218 | exact |
| 0x4d50a0 | init_stringpool_24f6218 | exact |
| 0x4d50c0 | register_atexit_stringpool_24f6218 | exact |
| 0x4d50d0 | atexit_dtor_stringpool_24f6218 | exact |
| 0x4d50e0 | static_init_stringpool_24ed600 | exact |
| 0x4d50f0 | init_stringpool_24ed600 | exact |
| 0x4d5110 | register_atexit_4d5110 | exact |
| 0x4d5120 | atexit_dtor_stringpool_24ed600 | exact |
| 0x4d5130 | static_init_level_table | likely |
| 0x4d5140 | init_level_table | likely |
| 0x4d54e0 | level_create_world_effects | exact |
| 0x4d5570 | level_script_select | likely |
| 0x4d56d0 | level_script_load_steps | exact |
| 0x4d5720 | level_add_navpoint | likely |
| 0x4d58e0 | navpoints_reset | exact |
| 0x4d58f0 | script_object_suspend | exact |
| 0x4d5960 | script_object_resume_index | exact |
| 0x4d5990 | conversation_execute_action | likely |
| 0x4d5e20 | level_script_use | exact |
| 0x4d5e40 | static_init_ccmd_door_open | exact |
| 0x4d5e50 | init_ccmd_door_open | exact |
| 0x4d5e70 | cmd_door_open | exact |
| 0x4d5ed0 | level_script_flag_hash_known | exact |
| 0x4d5f10 | level_script_int_hash_known | exact |
| 0x4d5f50 | name_hash_nocase | exact |
| 0x4d5ff0 | quest_find_by_name | exact |
| 0x4d6040 | quest_journal_set_entry | exact |
| 0x4d6080 | quest_get_current_step | exact |
| 0x4d6100 | script_filenames_parse_table | exact |
| 0x4d6300 | level_script_find | exact |
| 0x4d6340 | level_script_find_index | exact |
| 0x4d6390 | level_table_find_id | exact |
| 0x4d63b0 | level_format_display_name | likely |
| 0x4d64c0 | level_table_display_name | exact |
| 0x4d64f0 | level_ambient_pause_all | exact |
| 0x4d6520 | level_ambient_unpause_all | exact |
| 0x4d6550 | static_init_ccmd_door_close | exact |
| 0x4d6560 | init_ccmd_door_close | exact |
| 0x4d6580 | cmd_door_close | exact |
| 0x4d65d0 | static_init_ccmd_door_toggle | exact |
| 0x4d65e0 | init_ccmd_door_toggle | exact |
| 0x4d6600 | cmd_door_toggle | exact |

The likely / unsure reasons are in the comment above each function. The main ones: statement order the
compiler scheduled (layout functions, read_camera), merged bit-field stores (write_party_state), tails the
compiler shared or duplicated (set_flag's two refresh calls, read_living's set-leader tail,
conversation_execute_action kinds 7 / 11), loop shapes after strength reduction (automap_labels_reset_level,
read_two_stage, init_conversation_slot), and the helpers written as inline functions or macros of ours
(savegame_find_hash, conversation_slot_is_mp, savegame_remap_item_hash, SAVEGAME_PUT_SIZE /
SAVEGAME_CHECK_SIZE): the binary has the code expanded each time, the original spelling is not visible.

## Where docs/sum-exe/savegame.md disagrees with the code

1. **LENT, inactive list.** The page says records are written for entities of list 0x8ed500 "that have
   flag 0x100 in +0x320 without 0x40000". The code (0x4d21dc-0x4d21ef counting, 0x4d2274-0x4d2286 writing)
   writes those that do **not** have 0x100 (the summon flag), or that have 0x40000.
2. **Header check.** "The script-table checksum is read but not compared": savegame_report_read_sizes only
   skips it (`add edx, 4` at 0x4d24e3); it is not read at all.
3. **Load step 7.** "Level items, two-stage objects and automap labels (no tags; always succeed)":
   savegame_read_two_stage fails ("12a") when a door's name plus level_script_name is 64 characters or
   longer (0x4d3588 / 0x4d35ac).
4. **CONT.** The container byte is `+0xe4 * 100` truncated by _ftol (0x4d242e), not rounded. The gold piles'
   amount is written but not read back: savegame_read_item_objects skips it (`add esi, 8` at 0x4d33a7) and
   spawns every pile with a count of 1 (`push 1` at 0x4d33c2). The page does not say so.
5. **Level items.** The load also deletes the container holding a picked-up level item (0x4d3465-0x4d34a0).
6. **savegame_find_flag_level** (0x4d0240) returns the flag's value in the first level whose list has the
   hash, not a level; quest_get_current_step uses it as a bool.
7. **Conversation bits** (refinement of "exact bit meanings are unknown"): the speaker bit is
   CFLAG_IMPORTANT (bit 0x8 of the speaker flags); topic kind 0 is topic flag bit 0, 1 `initial`, 2
   `hidden`, 3 "this topic is the speaker's greeting (hail topic)". The bits are read only once the level's
   visit count is > 0. init_conversation_slot copies the persistent speakers' (CFLAG_PERSISTENT) bits from
   the first other slot of the level that has data.
8. **Two-stage bit 1** is the door's locked state; a door without an entry reports door_info +0x54 bit 3.
9. **Automap labels** cannot be removed or reset while the list holds 128 (both functions return at
   count >= 0x80, 0x4d13ca / 0x4d1526).

## Open questions

1. **two_stage_set, existing container entry** (0x4d0fdf-0x4d1070): after "stuffing flag" the stored bit
   comes from the `locked` argument slot right after a zero was written to it, so both bits end up 0 and
   `open` is ignored. Written as `locked = false; ... (locked ? 1 : 0)` (unsure). container_examine only
   marks unused containers, so the path looks unreachable in practice.
2. **init_empty_4d5030** (entry 351): an empty dynamic initialiser between script_objects and the
   StringPools: an object with an inline empty constructor; which one is not known (as in spells.cpp).
3. **level_script_select**: without a level script (or in multiplayer) "<level>.vfx" is copied into
   level_effects[level_effect_count] but the count is not increased here, so level_create_world_effects
   does not create it unless another function counts the record (not checked).
4. The bool outputs of two_stage_get are stored from `& 1` without setne (0x4d11dc-0x4d11e7); the original
   may have used a byte type there.
5. The .bss order versus the definition order (above); 0x2493a14-0x2493a17 (level_script_restarting,
   Dont_load_fx, No_cutscenes) are left to level_scripts_common.cpp.
6. **level_table**: entry 0's id (3) is static in the image, every later field is stored by init_level_table:
   VC6 initialises the constant prefix statically, as D602 found for level_display_names.

## Game bugs noticed

1. **Dropped gold loses its amount on load** (savegame_read_item_objects 0x4d33a7 / 0x4d33c2): every
   loose gold pile is recreated with count 1.
2. **game_stage_set stage 19** sets trapped_luminar on level 14 (`push 0xe` at 0x4d437a) instead of 22,
   where it is read (0x4d4343); the flag name is not in level 14's list, so the call does nothing (harmless:
   that branch only runs when the flag is already set).
3. **Automap labels**: with 128 saved labels, deleting or editing a label is not saved (remove returns
   early) and the label comes back after a reload.
4. **Automap bit bound** (get_bit / set_bit 0x4cff41 / 0x4cffa7): `index < num_bits * 8`, eight times the
   block's bits; a bad index would read or write into the level's ints, floats or the next level.
5. **door_open / door_close / door_toggle console commands** ignore door_find_by_name's failure and pass an
   uninitialised lobj pointer for an unknown door name.
6. **events.tbl parser** (latent, data-driven): +Float: names are checked against the int lists
   (level_script_int_hash_known at 0x4d3d95); a $Level: line that is not +Flag: / +Int: / +Float: never
   advances the file (endless loop); more than 200 quests write through a NULL record; more than 12
   stages overflow the record; +Invalidate: ends the quest's stage list.
7. **quest_journal_set_entry** shows "Quest Journal Updated" also when it clears a quest, and the layout
   calls it 200 times at start-up. **quest_find_by_name(NULL)** returns 0 (quest 0), not -1.
8. **name_hash_nocase** copies the name into a 128-byte local without a length check.
