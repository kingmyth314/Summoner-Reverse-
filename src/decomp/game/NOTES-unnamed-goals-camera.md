# decomp58: the goal / door unit and the camera unit (0x475180-0x479b5f)

Two translation units, both written under placeholder names (no path string, no strings.tbl block,
sum.symbols module "-"; Summoner 2 gave no file names for them):

| File | Code | .data | .bss | Functions | Exact / likely / unsure |
|---|---|---|---|---|---|
| `game/unnamed_goals_doors_0x475180.cpp` | 0x475180-0x4778ab | 0x59b7c8-0x59b7fb | none | 67 (66 symbols + the unnamed 0x476890) | 53 / 14 / 0 |
| `game/unnamed_camera_0x4778b0.cpp` | 0x4778b0-0x479b59 | 0x59b7fc-0x59b986 | 0x22e49b8-0x22e4b0c | 55 | 34 / 19 / 2 |

**The whole range 0x475180-0x479b60 is written (122 functions). Nothing is left in it.** Headers: see
CHANGED-HEADERS.txt (include/ is src/decomp/include at ffc5921 plus the changes; decisions D3800-D3808).
Both files pass `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include`
without errors (one -Wdelete-non-abstract-non-virtual-dtor warning in mp_handle_new_mon, as elsewhere for
Character), and every src/decomp game/vsdk .cpp gives the same output against include/ as against the repo's.
modules.txt is empty: neither file name is known.

## Unit evidence (D3800)

- **Start of the goal unit.** ai.cpp ends at 0x475178 (cast_effect_track_count, D1905). Its .data ends with
  its literals ("Ai_path_wandering_enabled" to 0x59b7c6). The next .data item is an initialised variable,
  ai_goal_next_id (0x59b7c8 = 1; ai_goal_alloc, savegame 0x4d218d / 0x4d2e16), then this unit's only
  literal "WARNING: unknown service request type -- %d\n" (0x59b7cc, mp_handle_srv_req). A variable after
  another object's literals starts a new object (D3309). Its .rdata floats (0x581a0c-0x581a24) follow
  ai.cpp's (to 0x581a08). No .bss: the handle map 0x22e94f4 lies after spells.cpp's .bss.
- **Inside the goal unit** nothing separates the mp helpers (0x475180-0x475823), the goals and service
  request (0x475830-0x47689e) and the doors / level objects (0x476890-0x4778ab): no static initialiser, no
  other .data or .bss. It may be two or three objects; kept as one placeholder.
- **The camera unit** starts with the static initialiser thunk of ccmd_zoom_limit (entry 229; geom_avoid
  230, mlook_sensitivity 231) and its .data with its own variables (mlook_sensitivity 0x59b7fc,
  camera_zoom_max 0x59b800, 1.5f 0x59b804) right after the goal unit's literal; its .bss sits between ai.cpp's
  (to 0x22e49b7) and spells.cpp's (0x22e4b18). spells.cpp begins at 0x479b60 (entry 232). game_camera
  (0x60a018) is gameplay.cpp's.
- Literal and float-constant order: within a function VC6 emits them in reverse order of use (zoom_limit:
  usage text, "off", "on"; mp_quantize_position: 0x581a14 used first, placed last), functions in address
  order. Useful for later unit cuts.

## Functions

### game/unnamed_goals_doors_0x475180.cpp

| Address | Name | Conf. |
|---|---|---|
| `0x475180` | mp_player_find_by_addr | exact |
| `0x4751d0` | mp_player_find_by_id | exact |
| `0x475200` | mp_player_find_by_name | exact |
| `0x475240` | mp_obj_from_net_handle | exact |
| `0x475280` | mp_local_handle_from_net | exact |
| `0x4752c0` | mp_handle_to_net | exact |
| `0x475300` | mp_map_net_handle | exact |
| `0x475350` | mp_all_players_have_status | exact |
| `0x475380` | matrix3_pack_orientation | likely |
| `0x475430` | matrix3_unpack_orientation | likely |
| `0x4755d0` | mp_broadcast_packet_3e | exact |
| `0x475610` | mp_handle_new_mon | exact |
| `0x475660` | mp_quantize_position | likely |
| `0x4756e0` | mp_dequantize_position | likely |
| `0x475780` | character_is_locally_controlled | exact |
| `0x475830` | ai_goal_evaluate | likely |
| `0x475d20` | ai_goal_compare | exact |
| `0x475db0` | ai_goals_sort | exact |
| `0x475dd0` | ai_state::goals_remove_type9 | exact |
| `0x475e00` | ai_state::goals_remove_type1 | exact |
| `0x475e30` | ai_state::goals_remove_types_3_5 | exact |
| `0x475eb0` | ai_state::goal_find | likely |
| `0x475fb0` | ai_goal_type_fields | exact |
| `0x476020` | ai_state::goal_alloc | likely |
| `0x476190` | ai_state::goal_add | exact |
| `0x476300` | ai_state::goal_remove | exact |
| `0x476380` | ai_state::goals_clear | exact |
| `0x4763a0` | ai_state::process_goals | exact |
| `0x476610` | ai_goals_validate | exact |
| `0x476670` | ai_state::has_goal_without_flag20 | exact |
| `0x4766a0` | ai_state::goals_mark_keep | exact |
| `0x4766c0` | ai_state::goals_remove_unmarked | exact |
| `0x4766f0` | ai_service_request | exact |
| `0x4767a0` | mp_handle_srv_req | exact |
| `0x476890` | lobj_index_compare | exact |
| `0x4768a0` | name_ptr_stricmp_compare | exact |
| `0x4768c0` | door_from_lobj | exact |
| `0x476940` | door_is_open | likely |
| `0x4769f0` | lobj_door_is_open | exact |
| `0x476a30` | door_in_use_range | likely |
| `0x476ab0` | door_find_floor_point | likely |
| `0x476e00` | door_set_open | exact |
| `0x476e80` | door_set_closed | exact |
| `0x476ee0` | door_toggle_state | exact |
| `0x476f30` | door_start_move | likely |
| `0x477080` | door_update | likely |
| `0x477280` | door_find_by_name | exact |
| `0x477310` | lobj_is_door | exact |
| `0x477340` | door_apply_saved_state | exact |
| `0x4773d0` | door_is_open_by_name | exact |
| `0x477450` | lobj_find_textured_record | exact |
| `0x4774a0` | level_marker_find | exact |
| `0x477540` | level_vis_record_find | exact |
| `0x477580` | level_trigger_record_find | exact |
| `0x4775c0` | lobj_get_layer | exact |
| `0x4775e0` | lobj::lobj | likely |
| `0x4775f0` | lobj::get_layer_n | exact |
| `0x477600` | lobj::pick_layer | exact |
| `0x4776a0` | lobj::set_layer_n | exact |
| `0x4776b0` | lobj_index_out_of_range | exact |
| `0x4776c0` | lobj_get_bbox | exact |
| `0x477720` | lobj_get_bounding_sphere | likely |
| `0x477770` | lobj_get_count | exact |
| `0x477780` | lobj_from_index | exact |
| `0x4777a0` | lobj_find_by_name | exact |
| `0x4777f0` | lobjs_hide_by_prefix | exact |
| `0x477850` | lobjs_show_by_prefix | exact |

### game/unnamed_camera_0x4778b0.cpp

| Address | Name | Conf. |
|---|---|---|
| `0x4778e0` | cmd_zoom_limit | likely |
| `0x477990` | cmd_geom_avoid | exact |
| `0x477a80` | cmd_mlook_sensitivity | exact |
| `0x477af0` | camera::apply_input_deltas | likely |
| `0x477ca0` | camera::auto_rotate_update | likely |
| `0x477ea0` | camera::compute_eye_pos | likely |
| `0x477f10` | camera::get_target_offset | likely |
| `0x477f60` | camera::update_orientation | likely |
| `0x478130` | camera::update_eye_position | likely |
| `0x478450` | camera_approach_value | exact |
| `0x4784e0` | camera::add_occluder | likely |
| `0x478520` | camera::find_occluders | likely |
| `0x478700` | camera::smooth_height | likely |
| `0x4787b0` | camera::auto_view_update | exact |
| `0x478890` | camera::get_state | exact |
| `0x4788e0` | camera::set_state | exact |
| `0x478930` | camera::view_high | exact |
| `0x478960` | camera::view_low | exact |
| `0x478990` | camera::frame | likely |
| `0x478d20` | camera_level_reset | exact |
| `0x478d40` | camera_leave_level | exact |
| `0x478d70` | camera::camera | exact |
| `0x478df0` | camera::~camera | exact |
| `0x478e00` | camera::update_target_pos | likely |
| `0x478e70` | camera::snap_to_target | exact |
| `0x478f60` | camera::reset_default | exact |
| `0x478fd0` | camera::set_height | exact |
| `0x478ff0` | camera::set_target | exact |
| `0x4790d0` | camera::copy_follow_view | exact |
| `0x479170` | camera::set_input | exact |
| `0x4791a0` | camera::free_look_update | likely |
| `0x4792c0` | camera::start_sequence | likely |
| `0x479430` | camera::cut_to_view_stub | exact |
| `0x479440` | camera::free_sequence | likely |
| `0x479480` | camera::update_sequence | likely |
| `0x4796a0` | camera::enemy_near_leader | exact |
| `0x479740` | camera::set_follow_flags | likely |
| `0x479790` | camera::resume_follow | exact |
| `0x4797b0` | camera::push_state | exact |
| `0x4797d0` | camera::pop_state | exact |
| `0x4797f0` | camera::end_sequence | exact |
| `0x479830` | camera::set_view_params | exact |
| `0x479870` | camera::set_default_view_params | exact |
| `0x4798b0` | camera::sync_yaw_to_target | exact |
| `0x479930` | camera::set_mode | likely |
| `0x479a60` | camera::cycle_mode | likely |
| `0x479ad0` | camera::set_sequence_transform | exact |

Compiler-generated, not in the table: 0x4778b0 / 0x4778c0, 0x477960 / 0x477970, 0x477a50 / 0x477a60 (the
console_command initialisers, exact), 0x478d00 / 0x478d10 (atexit destructors of camera_frame's statics,
unsure, D3805).

## Open questions

- Whether the goal unit is one object or several (see above); its real file name(s).
- camera_frame's static and local vector type (D3805): which class has an inline empty destructor; and the
  0x84 unreferenced .bss bytes 0x22e4a3c-0x22e4abf (camera_unk_22e4a3c).
- camera_occluders_unk (0x22e4acc) is zeroed and read, never set: a leftover of a removed occluder list?
- The definition order of the camera .bss variables (the ccmd objects lie mlook, zoom, geom; the
  initialisers run zoom, geom, mlook).
- mp_net_handle_map (0x22e94f4): defined by the object unit 0x47f9a0 (it resets it) or by spells.cpp's
  neighbour; not this unit's.
- The door +0x08 duration and the close_delay "ms + 1" encoding (`1 - (int)(delay * -1000)`): source form.
- ai_goal_type_fields gives type 5 (cast at a point) pos and spell but not target; type 3 only spell. The
  float argument (0x20) is used only by the wander goal (13).
- AI_GOAL_SWITCH_NORMAL (7) is dispatched to ai_goal_do_combat_mode like 6 and is never evaluated.

## docs/sum-exe/camera.md against the code

- "camera_find_occluders casts a ray from the eye to the target": two rays, target point -> eye, then eye ->
  target, each repeated through the draw lists; both ends are the *last* eye and target positions
  (statics refreshed only when they moved more than 0.08 units), the target point raised by
  min(look height, 0.8 x target radius); skipped during cutscenes (game_flags 0x20).
- "camera_cycle_mode ... In mode 3 it does nothing": with +0x008 set (forced high suppressed) mode 3 counts
  as 1 and F12 goes to mode 2.
- camera_smooth_height: the overshoot clamp uses -difference (game bug below), not the difference.
- camera_free_sequence deletes only state-1 sequences (blends, D3401); cutscene shots are only dropped.
- camera_enemy_near_leader excludes party members (type 7) from the 8-unit test; it takes no argument
  (sum.symbols "(handle)").
- camera_set_target: a target change does nothing more unless flag 0x1 is set (the doc implies always).
- camera_apply_input_deltas: the zoom floor is applied in two identical branches (low / high view).
- Correct as written: the presets, mode rules, remembered zooms, set_follow_flags(follow, free_look), the
  state stack, the fov 1/pi, the free look, zoom_limit not a toggle, geom_avoid unused.

## docs/sum-exe/ai.md and sum.symbols against the code

- ai.md "Goals": also, a player-controlled owner (ai flag 1) adding a travel or attack goal drops both
  kinds; an equal follow goal takes the new target too; sorting puts kept (0x40) and suspended (8) goals
  last, combat-mode (6) and sneak (15) goals first, then priority, then the newer goal.
- ai.md: ai_goal_alloc replaces the lowest-priority slot without flag 1 only for a higher priority (or flag
  1), and the party leader's goals get flag 0x100 and clear the leader's hold mode (outside conversations).
- sum.symbols ai_service_request "local handling or multi_send_packet": only a multiplayer client sends;
  there is no local branch (hud_key_pause's request does nothing in single player). ai.h said the same.
- sum.symbols ai_goal_do_cast_at_target "(target, spell)": the call passes (spell, target).
- sum.symbols pf_cells_in_box "(size class, layer, ...)": the layer comes first (graph index
  (layer + size * 20) * 0x60); the repo header already has (layer, size, ...).
- sum.symbols camera_add_occluder "(obj index)": thiscall with four arguments, the index second (ret 0x10).
- sum.symbols camera_set_follow_flags "camera_end_sequence(!a)": it is !b (the second argument).
- sum.symbols door_start_move "(door, ...)": (lobj, auto-close delay seconds or -1, force past the lock,
  play the sound).
- sum.symbols lobj_find_textured_record "comparator 0x476890": that function has no symbol (proposed
  lobj_index_compare).
- docs/sum-exe/source-map.md: the spells.cpp tag on 0x479ad0 is a stray (camera_set_sequence_transform).

## Game bugs

- **NEW_LEP orientations are corrupted (multiplayer).** matrix3_pack_orientation stores rvec.x / rvec.y
  twice (0x4753f3-0x475425) instead of uvec.x / uvec.y, so every entity created over the network gets an
  up row whose first two terms are the right row's; the receiver rebuilds the rest from them.
- **A low-priority goal overwrites another goal's parameters.** ai_goal_alloc returns the lowest-priority
  slot even when the new priority is not higher and nothing was stored (0x47608b jumps past the stores but
  keeps ESI); ai_goal_add then writes the new goal's target / point / spell / float into that slot, which
  keeps its old type and priority. A monster can end up attacking or walking to the wrong place.
- **goals[-1] write.** With all five goal slots kept (flag 1) the party leader's branch of ai_goal_alloc
  sets flag 0x100 at goals[-1].flags (ai_state +0x5c, travel_dest.z).
- **Crash during cutscene scripts.** ai_goal_evaluate, cast-at-target goal not running: a missing target
  only sets the result to 1 and the code then reads its type (`cmp [esi + 0xc], 7`) when a cutscene script
  is active (0x4759c4-0x4759e4).
- **Camera height step away from the floor.** camera_smooth_height replaces an overshooting step by
  -difference (`fchs` at 0x478773), so at frame times above 0.25 s the height moves away from the wanted
  height (then corrects over the next frames).
- **door_find_floor_point stack overflow.** It collects the door's navigation edges into a 4-entry stack
  array with no bound (0x476b47-0x476b61); a door with more than four door edges overwrites the cell count
  and the loop variables.
- camera_frame reads target->pos and bound_radius with no NULL test (0x478a75, 0x478b5e), and
  camera_set_target(NULL) with a current target reads NULL->pos: a crash if the camera ever runs without a
  target (not observed).
- Dead code / data: Camera_geometry_avoidance (geom_avoid) and camera_occluders_unk are never used;
  camera_set_default_view_params' values are never read; door_start_move's close_wait test is always
  false (cleared just before); the SRV_REQ unknown-type warning is only formatted into string_buffer;
  zoom_limit's texts lack a newline.
- ai_goal_compare is inconsistent for two combat-mode / sneak goals (both orders return -1); qsort copes.
