# Phase 3: level_scripts_common.cpp (decomp41)

`game/level_scripts_common.cpp` is levelscripts\Scripts\level_scripts_common.cpp of Sum.exe v1.40, code
0x4ca700-0x4ceebf (static initialiser table entries 338-344, 0x58854c-0x588564), written from the
disassembly only (`node tools/asm.mjs fn`, raw .data bytes read from Sum.exe). No Summoner 2 or Dash Faction
code was read. The file passes `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings
-Wno-format-security -I include game/level_scripts_common.cpp` with no diagnostics against the header tree in
`include/` (the repo's tree at f7229e0 plus CHANGED-HEADERS.txt); all 72 existing src/decomp .cpp files give
the same diagnostics with this tree as with the repo's.

**Everything is written.** 92 sum.symbols functions in the range (module column level_scripts_common.cpp):
71 exact, 17 likely, 4 unsure. The 4 unsure are the two empty static initialisers (0x4ca700 / 0x4ca710 and
0x4ca740 / 0x4ca750), whose objects are guessed. Nothing is left in the range. The unit ends at 0x4ceebf;
0x4ceec0 starts level_scripts.cpp (savegame_state's initialiser), as level_scripts.cpp's NOTES say.

Data written: .data 0x59e0e8-0x59e0ff (level_trigger_type_names, level_trigger_id_names, Do_frame_callback
= 1; the rest up to 0x59e793 are this unit's literals); .bss 0x248ce40-0x2493a17 (every global of the
block, including lc_entries / lc_entry_count, which sum.symbols tags new_hud.cpp; level_scripts.cpp's NOTES
had left 0x2493a14-0x2493a17 to this unit: level_script_restarting, Dont_load_fx, No_cutscenes).

## Functions

| Address | Function | Confidence |
|---|---|---|
| 0x4ca700 | static_init_empty_4ca710 | unsure |
| 0x4ca710 | init_empty_4ca710 | unsure |
| 0x4ca720 | static_init_level_script_timer | exact |
| 0x4ca730 | init_level_script_timer | exact |
| 0x4ca740 | static_init_empty_4ca750 | unsure |
| 0x4ca750 | init_empty_4ca750 | unsure |
| 0x4ca760 | static_init_ccmd_dont_load_fx | exact |
| 0x4ca770 | init_ccmd_dont_load_fx | exact |
| 0x4ca790 | cmd_dont_load_fx | exact |
| 0x4ca850 | static_init_ccmd_do_frame_callback | exact |
| 0x4ca860 | init_ccmd_do_frame_callback | exact |
| 0x4ca880 | cmd_do_frame_callback | exact |
| 0x4ca940 | static_init_ccmd_eax_save_props | exact |
| 0x4ca950 | init_ccmd_eax_save_props | exact |
| 0x4ca970 | cmd_eax_save_props | likely |
| 0x4caa00 | level_check_triggers | likely |
| 0x4caed0 | level_trigger_point_left_of_line | exact |
| 0x4caf40 | level_find_player_starts | exact |
| 0x4cb010 | script_place_party_at_starts | exact |
| 0x4cb060 | level_script_restart | likely |
| 0x4cb350 | level_script_reload | exact |
| 0x4cb3a0 | level_script_reparse | exact |
| 0x4cb430 | level_items_parse_table | likely |
| 0x4cb7b0 | level_parse_table | likely |
| 0x4cc010 | level_parse_ambient | likely |
| 0x4cc190 | level_parse_trigger | likely |
| 0x4cc4d0 | level_parse_shopkeeper | likely |
| 0x4cc7e0 | level_script_init | likely |
| 0x4ccab0 | level_script_frame | exact |
| 0x4ccb40 | level_ambient_sounds_frame | exact |
| 0x4ccba0 | level_script_entries_frame | likely |
| 0x4ccc50 | script_character_has_pending_goal | exact |
| 0x4ccc70 | level_script_leave | exact |
| 0x4ccd10 | level_apply_pending_change | exact |
| 0x4ccdf0 | script_cutscene_done | exact |
| 0x4cce10 | static_init_ccmd_No_cutscenes | exact |
| 0x4cce20 | init_ccmd_No_cutscenes | exact |
| 0x4cce40 | cmd_No_cutscenes | exact |
| 0x4ccf00 | script_play_cutscene | exact |
| 0x4ccf90 | level_trigger_set_exit | exact |
| 0x4ccff0 | level_trigger_set_script_use | exact |
| 0x4cd060 | level_trigger_set_exit_by_refname | exact |
| 0x4cd0c0 | level_trigger_set_script_use_by_refname | exact |
| 0x4cd120 | level_trigger_trip | likely |
| 0x4cd250 | level_ambient_add | exact |
| 0x4cd2c0 | level_ambient_remove | exact |
| 0x4cd330 | level_ambient_find_at | exact |
| 0x4cd390 | lc_entry_add | likely |
| 0x4cd470 | lc_entry_clear | exact |
| 0x4cd4b0 | lc_entry_exists | exact |
| 0x4cd4e0 | lc_entry_mark_used | exact |
| 0x4cd520 | lc_find_by_name | exact |
| 0x4cd570 | lc_find_by_hash | likely |
| 0x4cd5f0 | level_script_parse_table | likely |
| 0x4cddc0 | script_object_resume | exact |
| 0x4cddf0 | script_object_find | exact |
| 0x4cde70 | script_object_find_character | exact |
| 0x4cdef0 | script_object_say | exact |
| 0x4cdf30 | script_party_member_say | exact |
| 0x4cdfa0 | script_party_remove_item | exact |
| 0x4cdff0 | script_eleh_strip_party_gear | exact |
| 0x4ce140 | script_object_set_disabled | exact |
| 0x4ce1c0 | script_npc_set_disabled | exact |
| 0x4ce1e0 | script_object_is_type5 | exact |
| 0x4ce220 | script_object_receive_go | exact |
| 0x4ce320 | script_npc_send_go | exact |
| 0x4ce340 | script_object_goto_label | exact |
| 0x4ce3a0 | script_npc_goto_label | exact |
| 0x4ce3c0 | script_check_navpoint | exact |
| 0x4ce460 | script_object_near_player | exact |
| 0x4ce4d0 | script_player_near_navpoint | exact |
| 0x4ce540 | script_npc_near_player | exact |
| 0x4ce570 | script_object_is_dead | exact |
| 0x4ce5d0 | script_object_drop_loot | exact |
| 0x4ce600 | script_npc_is_dead | exact |
| 0x4ce620 | script_npc_drop_loot | exact |
| 0x4ce640 | script_object_talk | likely |
| 0x4ce6d0 | script_npc_talk | exact |
| 0x4ce6f0 | script_guard_sees_player | likely |
| 0x4ce950 | level_shopkeeper_find | exact |
| 0x4ce9a0 | party_count_item_by_name | exact |
| 0x4cea70 | script_player_has_equipped | exact |
| 0x4ceab0 | script_party_acquire_item | exact |
| 0x4ceb00 | script_party_gain_xp | exact |
| 0x4ceb80 | script_topic_toggle_by_item | exact |
| 0x4cebc0 | script_topic_toggle_by_item_inv | exact |
| 0x4cec00 | party_set_level | likely |
| 0x4ced90 | script_avoid_spline_find_free | exact |
| 0x4cedb0 | script_avoid_splines_reset | exact |
| 0x4cedd0 | script_avoid_spline_in_use | exact |
| 0x4cedf0 | script_alloc_avoid_spline | exact |
| 0x4cee90 | script_free_avoid_spline | exact |

## Order and data

The initialiser order (entries 338-344) is: empty, level_music_stop_timer (0x24902a8), empty,
ccmd_dont_load_fx (0x2490c80), ccmd_do_frame_callback (0x2490c68), ccmd_eax_save_props (0x24902b8),
ccmd_No_cutscenes (0x2490c90, initialiser at 0x4cce10 between script_cutscene_done and cmd_No_cutscenes).
The objects are defined at their initialiser's place; the plain .bss variables are defined at the top in
address order, as in the other pilots. The .bss addresses of the command objects are not in definition
order (the same open question as in level_scripts.cpp).

The two empty initialisers are bare `ret`s. An object with an inline empty constructor gives one (D1801:
Mempool; vector3's constructor is inline and empty). The only vector3 globals of the block are
level_exit_return_pos (0x2493a08) and the positions inside level_ambients (0x2490458), so entry 338 is
written as level_ambients and 340 as level_exit_return_pos. **Open:** whether VC6 makes an empty initialiser
for an array of a struct whose only class member is a vector3, and which object is which.

Gaps in the .bss that nothing reads or writes: 0x2490c64, 0x2490c7c, 0x2490c9c, 0x2491a6c (each right after
a 0xc-byte console command, so alignment). 0x2490c78 is only zeroed by level_script_reparse
(level_unk_2490c78, name ours).

## Open questions

- level_trigger +0x44 is never read or written by this unit (between ref_name and id).
- script_object_talk calls conversation_active.is_idle() twice before anything else (0x4ce646, 0x4ce654): a
  macro or an inline that expanded twice? Written as two tests.
- level_trigger_trip has no result: the "tripped" bool lives in AL across the loop and the only caller
  (ls_catacombs_frame) ignores EAX; the tripped path returns with AL = the trigger's flags byte.
- The fog distance names: fog_set_level_default stores its first argument at 0x59bd88, which sum.symbols
  (since f7229e0) calls fog_level_near_distance, but level_parse_table passes the "+Fog Far:" value first
  (0x4cbf02-0x4cbf09). Either the table keys or one of the names are swapped; the header uses neutral
  parameter names. fog_set_layer likewise gets the first "+Distance:" float into level_layer_fog_near.
- The door sound parameters: the third float defaults to 0.1 and a parsed 0.1 becomes 1.0
  (`fcomp [0x580214]` 0x4cd83e / 0x4cd8da); params[0] is the second float times 255, params[1] the first,
  params[2] the third. What the three mean is not known.
- Character flag 0x400 (cur_character in script_guard_sees_player: the view range is scaled by light_level
  and the cone narrowed to 0.71) is not named in living_entity.h; probably sneaking / hiding.
- game_flags 0x100, set by script_object_talk before conversation_begin, has no name in summoner.h.
- Character::set_armour_alt_texture is declared (int, int) by living_entity.cpp; the body reads bytes
  (D2107).

## Doc disagreements

- docs/sum-exe/worldmap.md ("Leaving an encounter") says level_check_triggers compares `rand-` with
  `_strnicmp`, length 5. The call pushes 4 (`push ebx` with ebx = 4, 0x4cad56), so any level name starting
  with "rand" skips the checks. The skip also applies in towns (level_is_town) and on the world map
  (cur_level_index 4), which the page does not say.
- symbols/sum.symbols, lc_entry_add / lc_entry_clear / lc_entry_exists: the argument is a level object
  pointer, not a hash (D2106).
- sum.symbols level_exit_trigger_index (0x2490c60): it holds the trigger's +Index: value and becomes
  level_entry_point; level_apply_pending_change does not test a bit of it (D2104).
- sum.symbols script_party_member_say: "current character only unless debug flag 0x1000". It is the other
  way round: with game_flags 0x1000 (GF_SOLO_MODE) only cur_character speaks; without it any party member of
  that name does.
- sum.symbols party_set_level: "skills +0x93". The copy loop starts at char_info +0x94 (`inc eax` before the
  read of `[eax + ecx + 0x93]`, 0x4cecb0) into Character +0x614, 84 bytes: char_info::skill.
- sum.symbols level_trigger_trip: "sprintf ... under game_flags". The sprintf is unconditional (only into
  string_buffer, never shown).
- docs/sum-exe/level-scripts.md, frame slot: called "while Do_frame_callback is set". Also only while
  game_flags 0x20 (cutscene) is clear and no cutscene script is loaded (cutscene_script_table empty), and
  after level_check_triggers.
- level_scripts.h's comment says set_exit "clears trigger +0x54 bit 2": it clears the value 2
  (LEVEL_TRIGGER_SCRIPT_USE, bit 1).

## Game bugs noticed

- **level_find_player_starts (0x4cafcc):** after the "$playerNN" fallback loop it tests
  `level_player_starts[4]` (the loop counter after the loop), i.e. level_entry_point, which is >= 1, so
  "Unable to locate player starts ... Find Allender." can never fire; a level without player starts goes on
  with NULL starts.
- **level_script_leave (0x4cccbb):** `for (i = 0; i < level_ambient_count; i++) level_ambient_remove(i)`
  while level_ambient_remove compacts the table and decrements the count: only every other emitter is
  deleted (sound_emitter_delete); the others stay alive with their table entries gone.
- **party_set_level (0x4cecf0):** the skill point round-robin loops until the points are spent; when every
  progression entry is at 100 or above the member's level, it never ends (a hang in the game stage code
  that sets the party level).
- **script_alloc_avoid_spline (0x4cee23):** the "already has a spline" test is made twice with the
  character's handle (the second was presumably meant for the target), and the target check tests the
  handle (`test esi, esi` after living_from_handle, 0x4cee5a) instead of the looked-up character, so a stale
  target handle passes a NULL character to script_build_avoid_spline.
- **level_script_restart (0x4cb2cc):** the level items are respawned at `level_items[i].navpoint` for every
  record, also those of another script variant whose navpoint was never looked up (0 or stale), and whether
  or not they were picked up; navpoint_get_position then gets a NULL name. (Debug path: the script
  editor's restart.)
- level_script_restart and level_script_leave free the avoid splines without resetting their slots (the
  reparse resets them).
- party_count_item_by_name counts equipment slots 0-8 only (slot 9 is never counted).
- Several messages are only formatted into buffers and never shown: "\"%s\" not found." and "No level item
  entry ..." (level_items_parse_table), "Couldn't find level trigger", "Unable to locate character ... for
  shopkeeper support", "Ambient sound navpoint not found", "Tripped trigger", the string pool usage.
