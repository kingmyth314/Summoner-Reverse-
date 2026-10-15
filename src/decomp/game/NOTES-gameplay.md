# gameplay.cpp (decomp42)

`game/gameplay.cpp`, Sum.exe v1.40 code `0x437660`-`0x43a32f`. Written from the disassembly
(`tools/asm.mjs fn`, jump tables and .data / .rdata read from `Summoner/Sum.exe`). Not compiled with VC6.
Syntax check: `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I
decomp42/include decomp42/game/gameplay.cpp` passes with no warning; all 72 existing `src/decomp` units pass
against the header copy (base: repo 3930f76).

**71 of 71 functions written: 56 exact, 13 likely, 2 unsure.** Nothing is left.

Unit evidence: strings.tbl id 622 is read by party_member_join (`0x439d11`); the unoptimised cutscene file
ends at `0x437650`, gametarget.cpp starts at `0x43a330`. Static initialiser entries 117-129.
Data: .rdata `0x580df4`-`0x580e07`; .data `0x593c34`-`0x5943ef`; .bss `0x609f40`-`0x60ab5f`.

## Functions

| Address | Name | Confidence |
|---|---|---|
| `0x437660` | init_screen_rects_609fc0_thunk | exact |
| `0x437670` | init_screen_rects_609fc0 | likely |
| `0x4377d0` | init_ccmd_full_party_thunk | exact |
| `0x4377e0` | init_ccmd_full_party | exact |
| `0x437800` | cmd_full_party | exact |
| `0x4378c0` | init_ccmd_dump_sound_usage_thunk | exact |
| `0x4378d0` | init_ccmd_dump_sound_usage | exact |
| `0x4378f0` | cmd_dump_sound_usage | exact |
| `0x4379b0` | init_ccmd_add_xp_thunk | exact |
| `0x4379c0` | init_ccmd_add_xp | exact |
| `0x4379e0` | cmd_add_xp | exact |
| `0x437a40` | level_script_read_town_characters | likely |
| `0x437c20` | loading_screen_load_bitmap | exact |
| `0x437c80` | LoadingScreenDraw | exact |
| `0x437d70` | level_load_report | exact |
| `0x437e00` | level_unload | exact |
| `0x437e50` | party_level_reset | likely |
| `0x438040` | init_ccmd_Give_all_items_thunk | exact |
| `0x438050` | init_ccmd_Give_all_items | exact |
| `0x438070` | cmd_Give_all_items | exact |
| `0x438130` | state_game_enter | likely |
| `0x438570` | level_load | likely |
| `0x4386c0` | level_load_files | exact |
| `0x438880` | level_load_layers | likely |
| `0x4389f0` | level_restore_doors | likely |
| `0x438a90` | level_spawn_party | likely |
| `0x438c80` | state_game_leave | likely |
| `0x438d60` | sinit_game_camera | exact |
| `0x438d70` | init_game_camera | exact |
| `0x438d80` | register_dtor_game_camera | exact |
| `0x438d90` | dtor_game_camera | exact |
| `0x438da0` | init_empty_438db0_thunk | unsure |
| `0x438db0` | init_empty_438db0 | unsure |
| `0x438dc0` | game_render_frame | likely |
| `0x438f20` | game_render_frame_static_dtor | exact |
| `0x438f30` | state_game_frame | likely |
| `0x439370` | gameplay_update_cursor | exact |
| `0x439450` | party_check_all_dead | exact |
| `0x439500` | state_game_frame_static_dtor | exact |
| `0x439510` | camera_reset_to_player | exact |
| `0x439580` | combat_mode_set | exact |
| `0x439620` | party_toggle_hold | exact |
| `0x4396d0` | party_select_member | exact |
| `0x439750` | party_teleport_to | exact |
| `0x439800` | items_reset_for_new_game | exact |
| `0x439810` | init_ccmd_render_3d_obstacles_thunk | exact |
| `0x439820` | init_ccmd_render_3d_obstacles | exact |
| `0x439840` | cmd_render_3d_obstacles | exact |
| `0x439900` | init_ccmd_character_thunk | exact |
| `0x439910` | init_ccmd_character | exact |
| `0x439930` | cmd_character | likely |
| `0x439ba0` | party_member_join | exact |
| `0x439d50` | init_ccmd_flece_thunk | exact |
| `0x439d60` | init_ccmd_flece | exact |
| `0x439d80` | cmd_flece | exact |
| `0x439da0` | init_ccmd_jekhar_thunk | exact |
| `0x439db0` | init_ccmd_jekhar | exact |
| `0x439dd0` | cmd_jekhar | exact |
| `0x439df0` | init_ccmd_rosalind_thunk | exact |
| `0x439e00` | init_ccmd_rosalind | exact |
| `0x439e20` | cmd_rosalind | exact |
| `0x439e40` | sinit_ui_window_60a670 | exact |
| `0x439e50` | init_ui_window_60a670 | exact |
| `0x439e60` | register_atexit_439e60 | exact |
| `0x439e70` | dtor_ui_window_60a670 | exact |
| `0x439e80` | help_controls_window_draw | likely |
| `0x43a0a0` | help_draw_page | exact |
| `0x43a170` | state_help_key_handler | exact |
| `0x43a2b0` | state_help_enter | exact |
| `0x43a2e0` | state_help_frame | exact |
| `0x43a300` | state_help_leave | exact |

The two unsure: `init_empty_438db0_thunk` / `init_empty_438db0` (which object the empty initialiser
constructs; assigned to `world_last_click_pos`, D2208). The likely ones are marked in the source with their
reason (mostly the local layout, a duplicated tail that VC6 merged, or the profile_label scopes).

## What the unit does (observed)

- **State 4 enter (`state_game_enter`)**: from state 2 or 14 (new game) it resets the AI groups, the party
  inventory and `level_time`, and the camera snaps to the default view; from 5 (help) or 6 it does nothing.
  Then the level load (`level_load` / `level_load_files`), the pending savegame or the automap labels,
  `level_script_init(!loaded)`, the party placement, the starting Health Tonics (and 9,999,900 gold with
  the `0x5fbf19` cheat), the US open demo items, the conversation, `GF_LEADER_SOUND` set and
  `GF_PARTY_DEAD` cleared, the level visit count, `interface_load_all`, the loading report.
- **State 4 frame**: time, dead-party check, player input, cutscene or automap reveal, listener, view
  shake (`vfx_get_camera`), 3D setup, then three branches (paused: effects only; cutscene: scripts, cutscene
  events, objects, effects; normal: script entries, script, objects, effects, conversation, network), the
  cursor and world mouse, the leader marker, the render, the pending level change.
- **State 4 leave**: going to 5 or 6 keeps the level; otherwise everything is unloaded, and a next state
  other than 4 also drops the party.
- **State 5 is the help screen**, not a loading screen: "Summoner Help", one page of hard-coded English
  mouse / keyboard / camera controls, Esc returns to the previous state (the game stays paused).
- **party_level_reset** on a level change revives and heals the party only when the new level is a town
  (`#Town` in its `_script.tbl`) or worldmap1.

## Doc and symbol disagreements

1. docs/sum-exe/main-loop.md, state table: state 5 is "Loading (*inferred*)". It is the in-game help
   screen (help_draw_page, "Summoner Help" / "ESC to exit this screen"); sum.symbols' `state_loading_*`
   and `help_page` names follow the same guess.
2. sum.symbols `state_game_load_failed` (0x609f58): "set once the level is loaded". It is set only on the
   failure paths of state_game_enter (0x43821b); state_game_leave saves the conversation and re-pauses
   only when it is 0 (D2207).
3. docs/sum-exe/party.md: "Dead members are revived with full HP and AP on the next level change
   (party_level_reset)". Only when the next level is a town or the world map (`is_town` from
   level_script_read_town_characters, or `_stricmp(level_filename, level_script_table[4].name)`).
4. docs/sum-exe/party.md: "The test level masad clears Full_party". masad is Masad Village, the first story
   level (level_display_names); a new game that starts there with Full_party gets only Joseph and the flag
   is cleared.
5. sum.symbols `level_restore_doors`: "door_set_open from savegame_two_stage_get per door". Single player
   only finds each door's floor point. In multiplayer every door is opened at once (`door_set_open(l,
   -1.0, 1, 0)`) and stored as open + locked (`two_stage_set(1, 1, l)`); the two_stage_get results are not
   used.
6. sum.symbols `level_is_town` / `level_script_read_town_characters`: "forced 0 with debug flag 2". The
   bit is game_flags 2, GF_MULTIPLAYER: towns never count as towns in a network game.
7. Fog names: `fog_get_near_current` (0x486bf0) returns `fog_near_distance` (0x59bd80, after today's data
   rename), and game_render_frame prints it as "Fog_near" (first %f). The two getter names look swapped
   relative to the renamed data.
8. sum.symbols `help_controls_window_draw`: "draw callback in the record 0x594120 (+0xc)". 0x594120 is the
   camera controls table; 0x59412c is a one-entry page function table (help_page_draw_funcs). The debug
   key table 0x5940ec (12 strings, count 0x59411c) is never drawn.
9. sum.symbols `state_help_key_handler`: "arrow keys move 0x60a66c (clamped to 0)". Left clamps at 0,
   right wraps to 0 at the page count (1).
10. sum.symbols `init_screen_rects_609fc0`: the eight ints are the loading bar's four corners, and nothing
    reads them.
11. docs/sum-exe/source-map.md hand cuts: gameplay .bss "0x609f40-0x60a670". It runs to 0x60ab5f (the help
    ui_window 0x60a670 and state_help_active 0x60ab5c); the old summary row "0x4340d0-0x439ba0 |
    gameplay.cpp" (line 166) and summoner.h's old comment are stale.
12. sum.symbols comment on `GF_LEADER_SOUND` (summoner.h, "meaning unsure"): 0x200 is set at the end of
    state_game_enter and cleared by state_game_leave, i.e. "level entered".

## Game bugs and oddities noticed

1. **The fog debug text can never show** (game_render_frame): its static GameTimer is only started
   (`SetTimeout(0)`) inside the branch that needs it to be valid already.
2. **`character` console command without an argument** writes `name[strlen(name) - 1] = 0` with an empty
   name: one byte before the buffer on the stack.
3. **Revival only in towns**: dead members stay dead across level changes into non-town levels (see 3 above).
4. Console toggles with no effect: `Dump_sound_usage` and `Give_all_items` have no reader.
5. `gameplay_no_render_60a630` is read (state_game_frame, PollInputEvents) but never written: the
   `frame_sleep_when_inactive` branch is dead.
6. level_load_report formats three lines into string_buffer and never prints them.
7. level_load_layers reads `get_layer_n(1)` and ignores it; only layer 0 is compared with the set index.
8. party_member_join takes a Player slot before its (redundant) index test and does not return it when
   player_create fails.
9. In multiplayer every door is opened on level load (level_restore_doors).

## Open questions

- The profile_label texts and scopes in state_game_frame (EH states 0-7) are ours.
- Which object `init_empty_438db0` builds (D2208).
- `.data 0x593c68`-`0x593c77` (four ints of 1) and `.bss 0x60a42c`-`0x60a62f` (0x204 bytes) have no
  reader; the first is defined as `gameplay_data_593c68`, the second is not defined.
- `.rdata 0x580e08` (25.0) has no reader in the unit.
- loading_screen_load_bitmap's two arguments are unused (the model count and 0).
