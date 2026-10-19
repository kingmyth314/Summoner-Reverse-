---
title: Never-run function review
nav_order: 6.45
---

# Never-run function review (2026-10-18)
{: .no_toc }

Five agents read the 429 non-matching decompiled functions that no hybrid-game scenario had run, each against the
original Sum.exe code. They found three decompilation errors, all fixed and none visible in the retail game
(D9800-D9802); the in-game checks that followed found a fourth (D9803). For each function they worked out
whether and how it can be reached. This page keeps their reports as the plan for the in-game checks, and the
results of those checks. Background: [Decompilation plan, the hybrid game](decomp-plan.md).

1. TOC
{:toc}

## Outcome

| Group | Functions | Errors found (all **Observed** in the disassembly) | Fixed |
|---|---|---|---|
| A: multiplayer, network | 86 | `irc_channel::users` is unsigned (`irc_get_channel_list` `0x4ae120`) | D9802; the function now byte-matches |
| B: graphics | 96 | none; six last-bit x87 notes | - |
| C: UI, HUD, maps, script editor | 90 | `script_editor_create_ui` `0x4e0690` filled four size classes, not three; `options_drag_sliders` `0x41fff0` folded `0.1f * 1.5f` into `0.15f` | D9800, D9801 |
| D: game logic | 113 | none; three x87 notes | - |
| E: engine | 44 | none; one early-exit return value in a function with no caller | - |

Reachability is **Inferred** unless the report says it was seen: the agents read the code and did not run the
game. The checks against the original in the game follow this list (coverage, then dual calls or save
comparison; [tools](tools.md)).

How the reports were made: every file compiled strict with `tools/vc6-compare.mjs --keep`, each function's VC6
listing set beside `tools/asm.mjs fn`, scripts comparing calls, string literals, constants, compare masks, jump
kinds, struct offsets and relocation targets, jump tables decoded from Sum.exe, and every remaining difference read
by hand. The scratch scripts were not kept. The group reports below are the agents' own, lightly edited (they say
"I"); the agent command sequences use SummonerFix's agent mode ([SummonerFix](summonerfix/index.md)).

{: .note }
Since the review: the quicksave chain counted as never run only because `tools/hybrid-test.mjs` wrote its coverage
file before the quicksave; it now writes it afterwards.

## In-game checks

The proposed scenarios played on the hybrid game, one session each (coverage on, dual calls on the never-run
functions expected to run; [tools](tools.md)). All **Observed**. A dual result `same n/n` means the decompiled
function gave the original's memory, return value and `rand` state on each checked call.

| Scenario | Never-run functions that ran | Dual calls | Notes |
|---|---|---|---|
| Script editor (`[Debug] ScriptEditorKey`, `,` `.`) | about 60 (editor UI, vsdk list boxes and buttons, `gr_clip_line`, `gr_line_3d`) | `pick_hover`, `update_buttons`, `ray_hit_object` same 10/10 | D9800 confirmed: size class 3's entries (`0x255e9cc`, `0x255a7d0`) stay 0. `draw_navpoints` differs only in the clipper's scratch vertex (`gr_clip_temp_verts` +0x1a..+0x30): the editor's marker vertices never set the attribute byte at +0x1a in either build, so the clipper copies stack bytes; lines do not read them |
| Phoenix (`rand-hills01_v2`, `phoenix_combat 1`, `phoenix_stage 3`) | 9 | `phoenix_choose_attack` 10/10, `phoenix_reposition_update` 1/1, `vif_emitter_spawn_point` 10/10, `spell_probe_ground_point` 2/2; `avoid_poly_add_search_nodes` 8/10, then 10/10 after D9803 | `fire_weapon_missile` and `spell_place_ground_effect` differ on their first call only, in `bm_bitmaps`: the original's call loads the missile or effect bitmap from file, which the second call cannot repeat (a loader in the checked call). Calls 2-10 the same |
| Tiger (`TempleInt_v2`, `tiger_combat 1`, `tiger_stage 3`) | 8 | `tiger_pick_target`, `tiger_choose_attack`, `tiger_wander_update`, `tiger_stage_move_update`, `tiger_pick_wander_point` same | `tiger_combat_update`, `tiger_path_to_target` not reached |
| Evil Urath (`TempleInt2`, `urath_combat 1`) | 11 | the four leap functions and `evil_urath_ai_select_action` same; `pf_find_cell_crossing` 10/10 | one select call differed only in the keyboard state of Escape (`key_down_state`), pressed by the agent during the check. Joseph and Machival did not fight |
| Pyrul (`KhosaniLab2`, flag `pyrulstart`, `$npc002`) | 3 | `pyrul_choose_attack` 10/10 | `pyrul_cast_special`, `pyrul_fire_attack` not reached |
| Titus (`IkaemosBottomInt2`, `$npc001`, `titus_combat 1`) | 3 | `titus_ai_select_action` 10/10 | |
| Murod Oni (`jadetemple`, `npckill Murod`) | 2 | `murod_oni_update_offense_points` 10/10, `murod_oni_collect_area_hits` 1/1 | |
| Ghost Rider, Serpent, Luminar stage 2 | 6 | `ghost_find_floor_below` 1/1, `approach_target_point` 10/10, `boss_mount_pick_flee_point` 1/1, `offset_point_sideways` 10/10 | the Serpent illusion stays unreached (dead in v1.40, K213); the Giant Salamanka did not appear |
| UI screens (`interface_set_screen` for each, page-up functions by direct call) | 21 | 17 functions same: the status, spells, journal and item-trade click handlers 10/10, `automap_center_on_player` 10/10, `party_find_next_member`, `hud_draw_tooltips`, the seven page-up functions | a direct call of `pickup_page_up` with no pickup screen open crashes in the original too (null container): a misuse of the test, not an error |
| Firewall, avoidance console, path visibility | 9 | `avoid_box_box_overlap`, `avoid_poly_nearest_edge_hit`, `avoid_merge_obstacle_polys`, `cmd_avoidance`, `quaternion(const matrix *)` (the reviewer's unverified rounding note: no difference), `get_node_world_transform`, `quaternion::to_matrix_b`, `spell_probe_ground_point` same | `spell_probe_ground_point` differed in a Firewall node's orientation only while its callees were dual-checked in the same run; alone it matched 10/10 in the same sequence. Rule: do not check a function and its callees in one run |
| Level scripts, Gongji's conversation | 7 | `ls_ionaext_frame`, `ls_catacombs_use` 3/3, `conversation_execute_action`, `conversation::farewell` 2/2, `ls_jadetemple_action` | the KhosaniLab puzzle did not run from a direct call |
| Walking, character screen, map, hot buttons | 13 | `world_pick_under_mouse`, `travel_steer_update`, `ai_travel_offset`, `level_format_display_name`, `gr_bitmap_float`, `automap_handle_click` 10/10, `savegame::hot_button_set` | `int_to_string_with_commas` call 3 differed only in the keyboard state of R, pressed during the check |
| Console graphics (`render_3d_obstacles`, `map_grid`, `sysinfo`, Print Screen as JPEG and TGA, `slew`) | 8 | `gr_clip_line`, `gr_line`, `tga_rle_encode_row` 10/10 | `movement_read_controls` (slew) differs only in clock values stored in the key bookkeeping (`key_down_timestamps`, `key_down_time_checked`); the camera position and orientation it writes are the same, and `key_down_time` alone matches 10/10. Not explained (**Unknown**) |
| Summon and quicksave (save comparison) | `summon_create`, the summon branch of the save | - | two runs flag different records, all time-dependent (NPC positions, timers 70-120 ms behind): timing, not an error |
| Sewer boss, console `tentacle_test` | - | - | the original crashes too (`navpoint_find("$npc034")` returns NULL in this script; `0x4c7ee2`) |
| Multiplayer character screen (`interface_set_screen` 16 from the main menu; create a character, Select) | 19 | `mp_charselect_init`, `update_create_button`, `handle_click` 10/10, `wrap_text_input` 8/8, `mp_player_alloc` same | Select opened the PXO lobby (Options are set to Internet), so the version check and the IRC list functions ran; no traffic: WinSock is never started while the connection speed reads "None" (SummonerFix answers that registry value from its `[Registry]` section), so the HTTP socket cannot open. The test's `.plr` file was removed |
| Script editor's "< Time" button | - | - | the editor's buttons and hotkeys ignore the agent's clicks, keys (`key`, `postkey`, held `keydown`/`keyup`) and posted mouse messages; `level_script_restart`, `level_ambient_find_at`, `select_prev`, `focus_selection` not reached |
| Inventory (Info on a ring, drag to the paper doll, hover) | 7 | `inventory_popup_result`, `inventory_drag_to_equip`, `inventory_preview_view_begin`, `inventory_hover_tooltips` 10/10, `ring_format_info` 1/1 | `inventory_equip_item` ran once, unchecked; later drops did not equip |
| Options sliders (gamma, sound; left with Cancel) | 1 | `options_drag_sliders` 10/10 | D9801 confirmed in the game |
| World map (`worldmap1`) | 6 | `worldmap_frame`, `worldmap_marker_clamp_to_edge`, `vector3_angle_and_axis`, `ls_worldmap1_frame` 10/10; both encounter pickers 1/1 (direct calls; the `rand` state is compared too) | |
| Texture detail (`change_texture_res` twice), `key_clear` by direct call, a console `.vcs` script | 5 | `bm_set_reduced` 10/10, `key_clear` 2/2 | the script (mixed CRLF / LF line ends, comments, a 200-character line) ran through `LineFile::detect_eol` and `read_line` without error; file reading cannot be dual-checked (the file position is not rewound); the test file was removed |
| Eleh: the locked Tomb portcullis and an examinable crate (direct calls with the level objects) | 2 | `object_try_unlock_or_open` 2/2, `container_examine` 1/1 | |
| Items and skills by direct call: a Health Tonic, Appraise (Flece, a created Haste Ring), Assess on Gongji, Backstab's behind test | 5 | `character_use_inventory_item` (call 2; call 1 differs only in `bm_bitmaps`, the heal effect's bitmap the original's call loads), `character_use_skill` 2/2, `character_use_skill_on` 2/2, `add_skill_from_behind` 1/1, `character_learn_chain_skill` 1/1 | Appraise left the AP unchanged and Assess did not open its screen, so both may have stopped at a skill check |
| LPalaceInt guards | 2 | `script_guard_sees_player` 10/10, `script_build_avoid_spline` 1/1 | the spline built by a direct call of `script_alloc_avoid_spline` |
| Jade Temple: Gongji's reply paged, the automap dragged | 3 | `dialogue_box::scroll_next_page` 1/1, `automap_drag_scroll` 10/10 | the label input and the journal's quest-step lookup were not reached |
| Giant Salamanka (`KhosaniLab`: walk into `$salstart`, its intro) and the KhosaniLab beam puzzle (flippers 28, 27, 26) | 8 | `giant_salamanka_ai_frame`, `giant_salamanka_ai_select_action` 10/10; `trace_beam` 1/1 and its five corner and turn helpers | setting `salstart` first skips the intro, which is what shows the boss and switches its combat on |
| Evil Joseph and Machival (`TempleInt2`: `npckill` Evil Urath, then Evil Joseph) | 6 | `evil_joseph_ai_frame`, `evil_joseph_ai_select_action`, `machival_ai_select_action` 10/10; `evil_joseph_pick_helper_summon` 1/1; `spell_spawn_creature` (Machival's Meteor Storm targets) 1/1 | |
| Frames and helpers that had run unchecked | 9 | `tiger_ai_frame` 10/10, `vector3_rotate_about_axis` 10/10, `script_editor_handle_input`, `ix_ray_boundingbox`, `gr_line_3d`, `ui_gadget::send_event` 10/10, `ui_message_box_hit_test` 10/10, `level_trigger_point_left_of_line` 1/1, `cmd_sysinfo` 1/1 | |
| Not reached | - | - | the Tiger's charge (`tiger_path_to_target`, `tiger_combat_update`: a 30 % stage-2 pick), Pyrul's spell moves, Life Drain (no Wraith appeared from a direct `summon_create`), the level item pickup, the hover tint, particle compaction |


**Broad round (2026-10-19).** 97 non-matching functions that had run in some scenario but were never dual-checked
(loaders, constructors, allocators, sound, texture and UI code left out by name) were checked together, ten calls
each, in Jade Temple (walking, Gongji, Firewall, screens) and in the Urath fight. 90 gave the original's results on
every call. The rest:

| Function | Calls that differ | Cause |
|---|---|---|
| `projector_record::to_lobj_space` `0x4c0740` | 10/10, 10/10 | x87 rounding of the local planes' d: **fixed (D9804)**, 30/30 the same afterwards |
| `conversation_controller::camera_compute_shot` `0x4d8f50` | 10/10 | the original never sets `side.y` (a game bug the decomp keeps), so normalising it reads stack bytes, which differ between the builds |
| `conversation_controller::camera_blend`, `decal_build_clip_planes`, `decals_draw_for_lobj` | 3/10, 2/10 and 1/10, 4/10 | the last bits of floats: precision, being worked on |
| `levelrender_draw_list`, `fx_missile_hit` | 4/10, 1/10 | one more lock on a decal's bitmap within the frame (0 again between frames on both builds): follows from the decal precision |
| `ai_state::follow_repath` | 4/10, 8/8 | path-node pointers from `path_find_new`'s allocation (the known checker artifact) |

Across all runs, 628 of the 890 non-matching functions have now run in a checked scenario (466 before the review).
Multiplayer is left out at the user's request (2026-10-19): the LAN screens would need networking switched on.

One more decompilation error came out of these runs and is fixed: **D9803**, `avoid_poly_add_search_nodes`
`0x4aa400` computed the obstacle crossing per component at 80 bits where the original rounds the y difference and
product through `vector2` temporaries (one bit in the 7th digit of the returned length).

## Group A: multiplayer and network (86 functions)

**Counts:**
- **Suspected errors:** 1, minor.
- **Reachable in plain single player:** 4 functions.
  - By console command: monster_layout_load, cmd_disp_rate.
  - At startup with a setting: nw_ras_init.
  - Through the LAN menus or a direct call: mp_reset_net_stats.
- **Reachable by a direct agent call:** 5 more.
- **Reachable through the LAN menus on one machine:** 17, with no peer and no server needed.
- **Likely reachable in a LAN game hosted alone:** about 15. This comes from reading the code; I did not observe it.
- **Not reachable:** the rest (about 45). They need a peer, a server or the old PXO hosts.

### How I checked
1. **Compiled every file:** all 25 source files with `vc6-compare.mjs --keep` (strict mode).
2. **Compared call sequences:** a scratch script. For each function it lines up, ours against the original:
   - calls
   - string literals, read from Sum.exe
   - push and cmp constants
   - test/and/or masks
   - conditional-jump kinds (equal / signed / unsigned)
3. **Compared normalised instructions:** a scratch script. Registers, stack slots and symbol names are blanked out, but struct offsets and mnemonics are kept.
4. **Read leftover differences by hand.** Every function that still differed was read against `asm.mjs fn`.
5. **Decoded the 10 jump tables from Sum.exe** and checked each case against the source's switch.
6. **Checked the data tables:** all 50 `const` tables these files use match Sum.exe byte for byte.
7. **Checked for globals and function-statics without an address comment (the D9204 kind):** none.

### (a) Suspected errors
Nothing of the D9200–D9203 kind turned up: no inverted test, no wrong mask, offset or constant, no missing or extra call, no wrong argument order.

- **0x4ae120 `irc_get_channel_list`: the channel's user count is probably unsigned in the original.** Medium confidence that our type is wrong; effect on behaviour negligible.
  - **Original:** reads it at `0x4ae25e` with `xor eax,eax … mov ax, word ptr [ecx+0x22]; push eax`. That zero-extends the value, which VC6 does only for an unsigned 16-bit field.
  - **Ours:** source line 853 `sprintf(num, "%d ", irc_channel_cur->users)` with `short users` (multi_chattracker.h line 36; irc_add_channel's parameter is also `short`). VC6 emits `movsx`.
  - **Effect:** a different printed count only for an IRC channel with 32,768 users or more.

Checked and harmless (not errors):
- **`door_pick_by_mesh` 0x4aaa10:** the original tests `lock <= skill+0.05` (`fcompp; test ah,0x41`, 0x4aaacb). Ours tests `skill+0.05 >= lock`. Same result except for NaN.
- **`nw_WorkReliable`:**
  - A dead `cmp eax,0x258` at 0x51839b has no branch after it.
  - The wrap test `cmp ax,0xff6a; jae` is equivalent to ours `cmp 65385; ja`.
- **`http_get::send_get`:** the original's `0x2f54718` is a zero byte in .bss, the same empty string as ours `""`.
- **Clamps:** in several functions `setle/dec/and` against our jg/jl. Same result.
- **Bit tests:** `sar` against `shr` before `& 1`. Same bit.
- **x87 operand order** in pxochat_update_scrollbar: it differs, but both compute the same value.
- **Jump tables, all matched:** 0x4aff80, 0x4b0100, 0x4110a0, 0x4b10e4 (14 boss names), 0x46d098 (species order 0,2,3,1), 0x4078a0, 0x48d9e0, 0x41d72c, 0x4253c0.

### (b) Reachability

What the route names mean:

- **LAN menus** (no second machine; the project's C200 note says the same):
  1. In Options, choose LAN and connection speed "fast" (registry `ConnectionSpeed=fast`, which LoadSettings reads), then restart.
  2. On the main menu, `key M` (`mainmenu_check_network` → state 10).
- **LAN host alone:** create a character, Select, Create Game, Create (`mp_host_game`, state 14), then start a level as the only player. 0x43ec40 starts the level once every player has status 3. Inferred, not observed.
- **PXO:** these would contact the old PXO tracker, the IRC chat server and www.pxo.net (hard-coded `http://www.pxo.net/files/summoner/northamerica/version.nfo` and the banner URL). Those hosts are no longer PXO's, so I propose no tests there. Pointing them at a local stand-in would need a hosts-file change, which is your call.

| Address | Function | Single player? | Route / agent sequence |
|---|---|---|---|
| 0x43fe00 | monster_layout_load | Yes, console | Bind `[ConsoleKeys]` to `save_mlayout test` and `load_mlayout test`, `postkey` both in a loaded level |
| 0x442380 | cmd_disp_rate | Yes, console | `disp_rate GM_QURY` only prints the single-player refusal. Full path: poke `0x5fbfd8` with bit 2 set, `postkey`, poke back |
| 0x5178c0 | nw_ras_init | Yes, at startup | Options speed Slow or 56k, restart; `peek 2710e9c`. Needs the hybrid hooks in place before LoadSettings (0x43d919) |
| 0x4419d0 | mp_reset_net_stats | LAN menus, or direct | `call0 4419d0` |
| 0x4ac240 | chat_process_irc_message | Direct call | `callstr 4ac240 :bob!u@h~JOIN~#c 0`, `… :bob!u@h~PRIVMSG~#c~:hello 0`, `… :srv~322~me~#c~5~:topic 0`; compare eax with the original |
| 0x4adb40 | irc_add_user | Direct (via the parser) | JOIN line, or `:srv~353~me~=~#c~:@al~+bo~cy` |
| 0x4110b0 | version_read_nfo_file | Direct call | `callstr 4110b0 <path to a test .nfo>`. `1 40 0` should return 1, `2 0 0` should return 0 |
| 0x43fb60 | state6_enter | Direct only (no `gameseq_set_state(6)` anywhere) | `calli 401020 6; wait 1000; capture s6; calli 401020 1` |
| 0x4b0b50 | mp_level_find_boss | Direct; likely as host alone (`mp_level_start`) | On a boss level, `call0 4b0b50` then `peek 247e830 12`. Avoid levels without a boss: there it calls `mp_mark_level_complete` |
| 0x406590, 0x406bb0, 0x406bf0, 0x4072e0, 0x4076d0 | the character screen (init, Create button state, click handler, description input, `mp_char_create`) | Yes, LAN menus | Type a name, pick a class, Create, type a description |
| 0x407410, 0x407e10 | charselect page_down, drag_scrollbar | Yes, LAN menus | Only once a list is longer than its page (for example a long description) |
| 0x46cc80, 0x46d450 | mp_player_file_create, mp_player_file_read_summary | Yes, LAN menus | Create writes a `.plr`; the next visit scans it |
| 0x441490, 0x441610 | mp_session_reset, mp_player_alloc | Yes, LAN menus | Entering the character screen; then Select |
| 0x40acd0 | gamelist_handle_click | Yes, LAN menus | After Select |
| 0x40b120 | gamelist_page_up | Practically no | Needs more LAN games than one page |
| 0x409cd0, 0x409de0, 0x43edc0 | gamecreate frame and click, mp_host_game | Yes, LAN menus | Create Game, type in the fields, Create |
| 0x41c450, 0x41ce50, 0x41d5e0 | level select click, drag, chat line | Yes, LAN menus (drag only with more than 6 levels, e.g. `all_multi_levels`; the chat line likely) | Click a level; type a chat line |
| 0x517e00, 0x518ce0 | nw_WorkReliable, nw_accept_reliable | Likely, every frame for a LAN host | `multi_do_frame` → `mp_server_frame` → `mp_server_accept_connections`. Wait in the level select |
| 0x441820, 0x517820 | multi_process_packet, nw_buffer_pop | Possibly | Only when a datagram arrives (the game list's own broadcast may echo back; unverified) |
| 0x4b02a0, 0x4afca0, 0x4afd60 | mp_spell_cast_sync and its two writers | Likely, host alone | The callers check `game_flags & 2`; cast any spell |
| 0x480530, 0x47fdf0 | mp_send_obj_update, mp_send_new_lep | Likely, host alone | Walk around; `load_mlayout` as host |
| 0x41e5d0, 0x41e6b0 | chat_input_handle_key, chat_input_add_char_wrap | Likely, host alone (in-game chat is multiplayer state 4 only) | `key ENTER`, type more than 63 characters (also try `/msg bob …`), `key ENTER` |
| 0x46d610, 0x46d0b0 | mp_player_file_load, mp_player_file_write | Likely, host alone | `level_spawn_party`; autosave and leaving the game |
| 0x441660 | mp_player_disconnect | Possibly | Leaving the game |
| 0x442060, 0x441e90 | multi_send_packet, multi_send_packet_buffered | Possibly | The host mostly broadcasts to an empty player list |
| 0x441150 | multi_process_client_status | No | Client paths; the host's own socket is -1 |
| 0x4aaa10, 0x489120, 0x4affb0, 0x4b04a0, 0x4b06a0, 0x4b08f0, 0x4719e0, 0x470860, 0x4b1280, 0x4801f0, 0x4806a0, 0x480d70, 0x480f60, 0x441da0, 0x518d70, 0x518f30, 0x5191c0 | packet handlers and peer / join paths | No | Need a peer |
| 0x4ac100, 0x4adcd0, 0x4ae120 | IRC socket and lobby paths | No (irc_process_input only trivially) | Need the PXO IRC server |
| 0x4223d0, 0x422980, 0x422b40, 0x422fc0, 0x423050, 0x423370, 0x4233d0, 0x423420, 0x423a20, 0x424170, 0x4244f0, 0x425270 | PXO lobby, screen 11 | No | PXO only |
| 0x410f40, 0x51b780, 0x55a9a0, 0x55abd0, 0x55ae20, 0x55b0d0, 0x55b310, 0x48d870 | version check, downloads, tracker login | No | Contact www.pxo.net or the PXO tracker |
| 0x559ed0 | ftp_get::ftp_get | No, dead in practice | Both URLs the game uses are http |

### (c) No behavioural difference found
All other 85 functions match the original in behaviour:
- **multi_chattracker:** irc_read_line, irc_add_user, irc_process_input, and chat_process_irc_message (identical call, string and word-index sequence).
- **multi_door / multicontainer:** door_pick_by_mesh, mp_handle_item_drop.
- **multisave:** all 4 functions.
- **multispell:** all 7 functions, including the bitfield packing in mp_handle_release_spell.
- **multitrade:** mp_handle_it_okay.
- **UI screens:** new_charselect (7), new_gamecreate (2), new_gamelist (2), new_levelselect (3), new_multichat (2), new_pxochat (14).
- **new_int_multi_update:** version_check_run; version_read_nfo_file (sscanf order and the unsigned `<= 14000` checked).
- **Level and game state:** mp_handle_actv_gm, mp_level_find_boss, mp_handle_combat_sequence, state6_enter (constants checked), monster_layout_load, mp_host_game.
- **Object packets:** mp_send_new_lep, mp_handle_new_path, mp_send_obj_update, mp_handle_obj_update, mp_handle_status_add, mp_handle_ai_disable.
- **Packet sending:** multi_process_packet, mp_reset_net_stats, mp_send_tracked, multi_send_packet_buffered and multi_send_packet (buffer stride 0x204, limit 0x200 checked), cmd_disp_rate.
- **Session:** multi_process_client_status, mp_session_reset, mp_player_alloc, mp_player_disconnect, pxo_login_request.
- **Downloads:** ftp_get::ftp_get, all five http_get functions, InetGetFile::InetGetFile (url / local_file argument order checked).
- **vsdk net:** nw_buffer_pop, nw_ras_init, nw_WorkReliable, nw_accept_reliable, nw_NewSendReliable, nw_ConnectToServer, nw_NewCloseSocket (reliable-socket offsets and the 0xd2d record size checked).

## Group B: graphics (96 functions)

**Suspected errors:** none with high or medium confidence. Every comparison and jump direction, bit mask, struct offset, argument order, call, loop bound and float constant (checked bit for bit) I traced matches the original. What remains are low-ranked rounding differences: a value kept at 80 bits in one version and rounded to float in the other, or a sum added in a different order.
- **0x53d8f0 seg2d_intersect** (reachable): the original divides s by the unrounded 80-bit `den` (`fdiv st, st(1)` at 0x53d965); ours divides by the stored float. VC6 also dropped the original's second `den == 0` test (0x53d987..0x53d996). I checked that dropping it changes nothing: when that test would fire, ours divides by zero and the result still fails the range tests, so it returns false too. Low.
- **0x5519f0 polygon_clip_2d** (no callers): `float len = (float)(...)` rounds the length; the original divides by the unrounded value (0x551a79..0x551a9f). Low.
- **0x53b400 ix_linesegment_sphere** (no callers): `d` is stored as a float before `r*r - d*d`; the original uses distance()'s unrounded return (0x53b600..0x53b61c). Low.
- **0x53c610 ix_box_oriented_box_oriented** (no callers): all 15 tests use the same products as the original, including the wrong-axis bugs and the temporary shared by tests 7 and 9. Seven tests (2, 4–8, 10) add the bound's terms in a different order (e.g. test 1 at 0x53c6b4). Low.
- **0x5106a0 matrix::determinant** (only caller chain has no callers): same terms and signs, different order. Low.
- **0x522790 quaternion::quaternion(const matrix *)** (reachable through 0x537a10): logic checked case by case. The original keeps `s` at 80 bits and reuses a stored float sum in case 0 (0x5228e4); ours may round differently. I did not verify this one. Low.

**Two non-errors worth knowing:**
- `bbox_silhouette_table` (vsdk/unnamed_clip_0x5590b0.cpp:27) has no address comment. All 216 values match .rdata 0x581f98 and it is read-only, so it is harmless.
- 0x537a10 is called `vfx_instance__set_node_transform` in sum.symbols and `get_node_world_transform` in our source. The code places the node, so the sum.symbols name is the better one.

**Globals without an address (the D9204 kind):** I ran `hybrid-build.mjs -v --force` on the 19 files that hold the 37 reachable functions,. No variable keeps a separate copy in the DLL (no `hybrid-globals.txt`, no "keep a copy" lines). The only problem is that 4 matrix/matrix43 constructors defined in matrix.cpp don't resolve when that file is left out of the build; that is not a decomp error. The Direct3D and Glide files were not included.

**Method.** Each file was compiled strict with `--keep`. A script compared each function's calls, float constants, immediates and compare masks with the original, and I read every mismatch by hand. I traced every reachable function instruction by instruction, and most of the dead ones. The Direct3D and Glide back ends got the automatic comparison plus spot checks only.

### Reachability (single player, OpenGL): 37 reachable, 59 not

"No callers" means no call site and no pointer to the function anywhere in Sum.exe (I searched every dword of the file).

**Reachable with a known route (20):**

| Address | Function | Route / proposed sequence |
|---|---|---|
| 0x48c630 | fx_create_spell_area | Area spells (kind 11). Cast an area spell near enemies, or `callt 4740c0 <AI = Character+0x324> 0 <spell>` (ai_goal_add_cast_spell). |
| 0x48c8a0 | fx_create_spell_effect_facing | Kind-13 hit effects. Select the target, `peek 5fcc88`, then `spellhit <target> <spell> 100`. |
| 0x48d2b0 | fx_find_spawn_point | Effects placed beside the caster (spell_cast_execute, fx_create_spell_effect_near). Console `summon` or a summon spell. |
| 0x53d8f0 | seg2d_intersect | Only from fx_find_spawn_point. Same as above, standing beside a wall (`navgoto` into a corridor first). |
| 0x484e30 | overhead_label_place | Overhead speech labels; its logic only runs when two or more labels compete for a slot. `npcgoto` to a talking group, `wait`, `capture`. |
| 0x4a3890 | obstacles_debug_render | Console `render_3d_obstacles` via `postkey`. |
| 0x53e520 | gr_line_3d | Same console command, or the script editor. |
| 0x559350 | gr_clip_line | `render_3d_obstacles` with the camera turned so lines leave the screen. |
| 0x53f720 | gr_draw_sphere | Script editor (`[Debug] ScriptEditorKey=F12`, `key F12`) in a level with navpoints. |
| 0x53d2d0 | ix_ray_boundingbox | Click an object while the script editor is open. |
| 0x507310 | gr_line | Console `map_grid`, then open the automap; also the script editor's UI borders. |
| 0x502240 | bm_set_reduced | Console `change_texture_res`, or the detail option in Options. |
| 0x5061f0 | gr_screenshot | Print Screen (JPEG by default). |
| 0x554ea0 | tga_rle_encode_row | Console `targa`, then Print Screen. |
| 0x5502c0 | gr_opengl_read_backbuffer | Print Screen, or a save from the save screen (thumbnail; a quicksave may not take one). |
| 0x537a10 | get_node_world_transform | `firewall 1`. |
| 0x522790 | quaternion ctor | Only through 0x537a10's emitter and dummy tags: `firewall 1`. |
| 0x5249e0 | vector3_angle_and_axis | Console `summon`, or open the world map. |
| 0x524b50 | vector3_rotate_about_axis | ai_turn_towards: any fight. That tests never ran it is odd — check with `hybridcov` first. |
| 0x532cb0 | vif_emitter_spawn_point | Vif emitters with a spawn path. |

**Reachable only with the right content or setting (17):**

| Address | Function | Condition |
|---|---|---|
| 0x558f40 | spline::eval | Vif spawn path with 3 or more keys. |
| 0x532a20 | vif_compact_particles | Particle buffer compaction under heavy particle load (try `firewall 8`). |
| 0x51f960 | gr_env_map_uv | Environment-mapped vfx meshes. |
| 0x48de60 | levelrender_wave_lobj | Level objects with flag 0x20. Unknown which level; sweep levels with `level` + `capture`. |
| 0x470300 | s3d_read_color_record | Only if a level's .s3d has colour records; probably none do. |
| 0x503aa0 | bm_lock_pcx | A .pcx bitmap being loaded; may not exist in the shipped data. |
| 0x555f60 | pcx_read_bitmap | Only from bm_lock_pcx. |
| 0x507100 | gr_bitmap_float | Automap camera icon (if its bitmap exists), or world-map locations with flag 2. |
| 0x54f9a0 | gr_opengl_apply_tint | While the hover tint is on: `move` the cursor onto an NPC or object. |
| 0x54d5c0, 0x54deb0, 0x54df20, 0x54df90, 0x54e050 | the five gl_convert_* | A texture of the matching format on the packed-pixel path. |
| 0x55f110, 0x55f580, 0x55f900 | gr_poly_split_* | Multitexture polygons with a bitmap larger than 256; not seen with OpenGL. |

**Not reachable (59):**
- **Direct3D only (17):** 0x48fdb0, 0x50c130, 0x561900, 0x534a40, 0x535550, 0x5355e0, 0x5356d0, 0x535eb0, 0x536360, 0x538760, 0x538850, 0x538b70, 0x5394e0, 0x5395c0, 0x53a1d0, 0x53a370, 0x53a7f0.
- **Glide only (15):** 0x48e6c0, 0x50b4c0, 0x5473b0, 0x547cc0, 0x5484f0, 0x5492f0, 0x549500, 0x549560, 0x549950, 0x549a20, 0x549b50, 0x549d10, 0x549f50, 0x54a800, 0x54b3a0.
- **Called only from those back ends (1):** 0x55fb00 gr_poly_get_section.
- **No callers (26):** 0x541020, 0x54d630, 0x54dc20, 0x511920, 0x511bb0, 0x5549c0, 0x5594f0, 0x559890, 0x50ed60, 0x50ee70, 0x53e8d0, 0x53f060, 0x53af40, 0x53b130, 0x53b400, 0x53c610, 0x51f560, 0x5519f0, 0x522ea0, 0x523370, 0x523430, 0x5253a0, 0x536ed0. Three more are reached only from functions that have no callers: 0x5106a0 (via matrix3::inverse), 0x522930 (via quaternion_log_of_delta) and 0x5246f0 (via random_direction_in_cone_axis).

### Reviewed with no behavioural difference found

**Traced against the original in detail:**
- **Level, effects and HUD:** levelrender_wave_lobj, s3d_read_color_record (keeps the green-into-blue bug), the three fx functions, overhead_label_place, obstacles_debug_render.
- **Bitmaps and screenshots:** bm_set_reduced, bm_lock_pcx, pcx_read_bitmap, gr_screenshot, tga_rle_encode_row, gr_bitmap_float, gr_line (clip field offsets and all eight clip cases).
- **OpenGL:** all seven gl_convert_*, gr_opengl_apply_tint, gr_opengl_read_backbuffer.
- **3D drawing and clipping:** spline::eval, gr_clip_line, gr_line_3d, gr_draw_sphere, ix_ray_boundingbox, gr_env_map_uv, the three gr_poly_split_*, gr_classify_sphere_view, bbox_silhouette_corners, gr_get_frustum_planes, gr_cull_bounding_box_transformed, gr_poly_project_to_screen, gr_bitmap_3d_oriented_quad_facing.
- **Maths:** quaternion ctor logic, vector3_angle_and_axis, vector3_rotate_about_axis, random_direction_in_cone, vector2_dist_sq_to_line, ix_ray_parallelogram, ix_ray_quad, quaternion::multiply/exp, cquat log/exp_in_place, matrix43_interpolate_quat, matrix43::transpose_in_place.
- **Effects data and the rest:** get_node_world_transform (jump table, recursion arguments), vif_compact_particles, vif_emitter_spawn_point, init_vis_flags (case labels checked), report_memory, m2v_skip_extension_and_user_data (vtable slots), lights_calc_point_specular_color (flag bits and type test only).

**Direct3D and Glide, automatic comparison plus spot checks.** Three mismatches looked like problems and are not:
- The grDepthBiasLevel call missing from gr_glide_tmapper_multitexture belongs to a separate function at 0x54b900 that the disassembler ran into.
- gr_d3d_render_character_mesh's two original texture-cache calls are one call duplicated by the compiler.
- The float constants that seemed missing in levelrender_draw_list_d3d/glide and gr_d3d_init_device are named globals in our code, not literals.

## Group C: UI, HUD, maps, script editor, vsdk UI (90 functions)

### How I checked
I compiled every file with `VC6_STRICT=1 node tools/vc6-compare.mjs <file> --keep`. The output went to temp folders only. I then compared each function with the original in `build/sum.asm` using scratch scripts:
- **Fingerprint diff:** call targets, immediates, struct offsets, globals and float constants, all resolved to Sum.exe addresses.
- **Branch-site diff:** each conditional jump paired with the compare before it, including the `fcom` operand and `test ah` mask. An inverted test like D9200 or D9201 shows up as an unpaired site.
- **Globals and statics left in the DLL:** I checked `-v` unchecked targets, `build/hybrid-all/hybrid-globals.txt`, and `static` inside functions. None of these files has one; the unchecked items are const tables with equal contents, `__real` constants and strings.

I read every remaining difference by hand. Almost all were compiler layout: tail merges, loop rotation, switch layout, or the original's listing running past the function into a jump table or the next function. I compared constant tables byte for byte where behaviour depends on them: `inventory_model_view_offset`, `ui_inputbox_charset`, and the switch index tables of `script_create`, `params_ok_cb`, `action_type_allowed`, `ui_inputbox::process`, `hud_draw_tooltips`, `inventory_popup_result` and `inventory_preview_view_begin`.

### (a) Suspected errors

#### 1. `0x4e0690` `script_editor_create_ui`: the size-table loop bound (high confidence)
Original code at `0x4e06c0`..`0x4e0710`:
- `mov eax, 0x255e9ac` starts at `&script_editor_char_sizes[0][1]`.
- Each pass does `add eax, 0xc`, then `cmp eax, 0x255e9d0` (char_sizes + 40) and `jl`.
- So the loop runs for i = 0..2 only.
- `char_sizes[3]` (0x255e9cc..d7) and `script_editor_char_size_radii[3]` (0x255a7d0) are never written and stay 0.

Our source has `for (i = 0; i < 4; i++)` at line 971. It compiles to `cmp eax, char_sizes+56` and runs 4 times. That gives size class 3 ("huge": radius and height 99.9) a 199.8 x 99.9 x 199.8 box and a radius of about 299.6.

Only `script_editor_pick_hover` (`0x4e2c74`, `0x4e2ca0`) and `script_editor_focus_selection` (`0x4e769c`) read these entries, and only for a character of size 3. No retail char_info uses "huge": all `$Size:` lines in `extracted/tables` are small, medium or large. So the retail game shows nothing, but a modded huge creature would be pickable from everywhere in the hybrid's editor.
- **Fix:** `i < 3`.
- **Test:** open the editor, then `peek 255e9cc 12` and `peek 255a7d0 4`. The original gives zeros; the hybrid doesn't.

#### 2. `0x41fff0` `options_drag_sliders`: gamma constant folded (medium confidence, x87 precision like D9203)
- **Original** (`0x420244`..`0x420271`): `fild n; fmul [0x580214] (0.1f); fmul [0x580210] (1.5f); fadd [0x58020c] (0.7f)`.
- **Ours** (line 853): compiles to `fmul 0.15f; fadd 0.7f`. VC6 folded the two constants; the other three sliders compile like the original.
- **Effect:** in 53/64-bit FPU precision, steps 1 and 8 differ in the last bit (0.85 vs 0.84999996, 1.9000001 vs 1.8999999). In 24-bit mode they are equal.
- **Where it goes:** `options_apply_gamma` and `options_edit.gamma_correction`. A player can't see it.
- **Fix:** keep the 0.1f product in its own statement.

#### 3. `0x429410` `character_use_skill` Appraise case (low confidence)
- **Original** (`0x4295ac`..`0x4295bb`): `fild; fsqrt; fst [esp+0x14]; fmul [0x580758]`, using the unrounded root.
- **Ours** (line 1246): `fstp root`, then reloads the rounded float.
- The `skill > root*k` test could only flip on an exact tie, so the effect is negligible.

#### Note: `0x40e7c0` `quicksave` has actually run already
`tools/hybrid-test.mjs` (`runOnce`) runs `hybridcov cov_<tag>; call0 40e7c0`. The coverage dump happens before the call, so the list shows `quicksave` as never run. In fact every save test ran the decompiled `quicksave` and its saves matched. The static review found no difference either.

### (b) Reachability

**Common set-up** (agent mode, from a loaded test save):
- **Screens:** `calli 412620 <screen> <arg>` opens a screen (`interface_set_screen`; ids in `docs/sum-exe/interface.md`).
- **Script editor:** set `[Debug] ScriptEditorKey=F12` and `postkey F12`, or `peek 5fbfd8 1` then `poke 5fbfd8 <byte|80>`.
  - Never press Save (Ctrl+S).
  - Hotkeys from `script-editor.md`: `,` / `.` Prev/Next, Insert New, Ctrl+A add action, Ctrl+E Edit.
- **Mouse wheel:** the `*_page_up` functions run when the wheel delta (dz) is above 0.
  - dz = `mouse_wheel` (0x257ef88) minus the last reading. `poke 257ef88 78` gives one notch up; a later `poke 257ef88 0` gives one down.
  - If the game reads the mouse through DirectInput, use `postmsg 20a 780000 0` instead, and check with `peek 257f02c 4`.
  - The bodies only act when the scroll position is above 0. A direct call with `[Hybrid] Dual` is simplest: `poke <pos> 3; calli <fn> 0; peek <pos> 4`.

| Address | Function | SP? | How / why not | Agent sequence |
|---|---|---|---|---|
| 0x4fbd80 | ls_worldmap1_frame | yes | worldmap1 frame hook (table 0x59f230) | `level worldmap1 worldmap1; wait 3000` |
| 0x4fcaf0 | ls_worldmap1_pick_encounter_stage9 | yes | from the frame at stage 9..14; void function | `level worldmap1 worldmap1; call0 4fcaf0 40` (uses rand: dual call) |
| 0x4fce40 | ls_worldmap1_pick_encounter_stage15 | yes | stage 15+ | `call0 4fce40 40`, varying flags with `flagl` |
| 0x4013f0 | assess_format_info | yes | assess_init (screen 20, arg = target) | `peek 5fcc88 4`; `calli 412620 0x14 0x<char>` |
| 0x401a20 | assess_page_up | yes | assess + wheel | `poke 5b28b8 3; calli 401a20 0` |
| 0x4023b0 | automap_center_on_player | yes | automap_init / frame | `key M` (or `calli 412620 3 0`) |
| 0x402760 | automap_handle_click | yes | map clicks | `key M; click left <map>` |
| 0x403000 | automap_drag_scroll | yes | right-drag on the map | `down right; move …; up right` |
| 0x403350 | automap_label_text_input | yes | editing a custom label | add a label, click the map, `key A; key ENTER` |
| 0x405790 | buysell_page_up | yes | buy/sell (7) + wheel | trader scenario; `poke 5b2db8 3; calli 405790 0` |
| 0x408620 | credits_parse_table | yes | credits_init (21) | `calli 412620 0x15 0` |
| 0x40e7c0 | quicksave | yes (already run) | Ctrl+S | `call0 40e7c0` |
| 0x40eb30 | hud_draw_tooltips | yes | mouse resting on a HUD button, portrait or hot button | `move <portrait>; wait 1500; capture` |
| 0x40f340 | hud_hot_buttons_drag | yes | spell/skill icon dropped on a hot button | `calli 412620 5 0`; `down left` on an icon; `wait 400; move <slot>; up left` |
| 0x410570 | hud_drag_icon_begin | yes | start of the same drag | same |
| 0x416290 | inventory_hover_tooltips | yes | hovering an item | `key I; move <row>; wait 1000` |
| 0x4165c0 | inventory_page_up | yes | inventory + wheel | `poke 5bd970 3; calli 4165c0 0` |
| 0x416b80 | inventory_popup_result | yes | item popup open | `click right <item>; click left <entry>` |
| 0x416fb0 | inventory_identify_item | yes | popup Identify on an unidentified item | add one with savedit first |
| 0x417220 | inventory_drag_to_equip | yes | drag to the paper doll | `down left; wait 400; move <slot>; up left` |
| 0x4174a0 | inventory_equip_item | yes | on that drop | same |
| 0x4181d0 | inventory_preview_view_begin | yes | preview while dragging | same; capture before `up left` |
| 0x419530 | itemtrade_count_party_rows | yes | itemtrade_init (19) | `poke 609fe0 1; level jadetemple jadetemple; calli 412620 0x13 0` |
| 0x419710 | itemtrade_handle_click | yes | item trade frame | screen 19, click |
| 0x419bb0 | itemtrade_party_page_up | yes | item trade + wheel | `poke 5c1580 1; calli 419bb0 0` |
| 0x41af80 | journal_handle_click | yes | every journal frame | `calli 412620 0xa 0` |
| 0x41b570 | journal_desc_page_up | yes | journal + wheel | `poke 5c1ac8 3; calli 41b570 0` |
| 0x41f0f0 | options_init | yes | F2 | `key F2` (then Cancel) |
| 0x41fff0 | options_drag_sliders | yes | slider drag | F2, drag the gamma slider, Cancel; dual call for error 2 |
| 0x420d20 | pickup_page_up | yes | pickup (4) + wheel | `bag 30`, poke its address into 5c3b6c, `calli 412620 4 0x5c3b6c`; `poke 5c3b64 3; calli 420d20 0` |
| 0x421b00 | playmenu_open_save | yes | play menu Save button | `calli 412620 9 0; click left <Save>` (don't confirm) |
| 0x426260 | saveload_page_up | yes | load screen + wheel | main menu Load (858, 368); `poke 5c5b9c 3; calli 426260 0` |
| 0x429410 | character_use_skill | yes | skills panel / hot button | `calli 412620 6 0; click <skill>` |
| 0x42a030 | spells_set_page | yes | spells page buttons | `calli 412620 5 0; click <page>` |
| 0x42a270 | spells_handle_click | yes | spells frame | screen 5, click |
| 0x42b050 | character_use_skill_on | yes | targeted skill, then click the target | Appraise / Pick Lock, then click the target |
| 0x42bb00 | status_handle_click | yes | status frame | `calli 412620 2 0; click` |
| 0x42c0e0 | status_page_up | yes | status + wheel | `poke 5f93a8 3; calli 42c0e0 0` |
| 0x494400 | camera_blend_sequence::advance | **no** | dead: nothing in Sum.exe builds a state-1 sequence (D3401/D3806) | — |
| 0x430c80 | editor_dialog_show | no | only through the editor's Save (script_save, script_write_objects) | only on a scratch copy of the game |
| 0x4315c0 | editor_dialog_handle_event | no | callback of editor_dialog_show | as above |
| 0x413200 | party_find_next_member | yes | next-member arrow | full party; `calli 413320 1` or click the arrow |
| 0x413290 | party_find_prev_member | yes | previous-member arrow | same |
| 0x414110 | ui_message_box_hit_test | yes | any message box | `level rand-forest01 Rand-Forest01_v2; call0 40e7c0` (save refused), then hover / click OK |
| 0x418ba0 | item_info_popup_page_up | yes | item info popup + wheel | right-click item, Info; `poke 5c1568 2; calli 418ba0 0` |
| 0x4e0690 | script_editor_create_ui | yes (debug) | first editor frame | `postkey F12`; `peek 255e9cc 12` |
| 0x4e2bb0 | script_editor_pick_hover | yes (debug) | every editor frame | move over a character |
| 0x4e2f30 | script_editor_ray_hit_object | yes (debug) | from pick_hover / add_navpoint | same |
| 0x4e30d0 | script_editor_draw_navpoints | yes (debug) | every frame | capture |
| 0x4e3a20 | script_editor_handle_input | yes (debug) | every frame | clicks, Q/W/E drags |
| 0x4e4910 | script_action_format | yes (debug) | selecting a scripted character | `.` until one is selected |
| 0x4e53a0 | script_editor_list_door_sounds | yes (debug) | Door mode, click a door | — |
| 0x4e58a0 | script_editor_place_creature | yes (debug) | Creature mode, New, click a navpoint | — |
| 0x4e5b10 | script_editor_add_navpoint | yes (debug) | Navpoint mode, Insert, click the floor | — |
| 0x4e6040 | script_editor_update_buttons | yes (debug) | every frame | — |
| 0x4e62e0 | script_editor_fill_action_lists | yes (debug) | Change on a character | — |
| 0x4e7460 | script_editor_select_prev | yes (debug) | `,` | — |
| 0x4e75c0 | script_editor_focus_selection | yes (debug) | Prev / Next | — |
| 0x4e7a40 | script_editor_action_type_allowed | yes (debug) | Ctrl+A | — |
| 0x4e8020 | script_editor_edit_button_cb | yes (debug) | Ctrl+E | — |
| 0x4e84b0 | script_editor_apply_action_params | yes (debug) | action-type window OK | — |
| 0x4e88a0 | script_create | yes (debug) | first action on an unscripted character | — |
| 0x4e8cf0 | script_editor_default_action_navpoint | yes (debug) | script_create with "Walk to" | avoid Cutscene Script mode (C220 crash) |
| 0x4e9310 | script_write_level_script | no | Save only | — |
| 0x4e9ce0 | script_write_objects | no | Save only | — |
| 0x4ea1e0 | script_write_level_table | no | Save only; replaces `<level>.tbl` | — |
| 0x4ea500 | level_items_write_table | no | Save only; replaces `level_items.tbl` | — |
| 0x4eb5c0 | script_editor_params_ok_cb | yes (debug) | parameter window OK | — |
| 0x4eb9e0 | script_editor_door_sound_radio_cb | yes (debug) | door window radios | — |
| 0x4ebb20 | script_editor_door_sound_inputbox_cb | yes (debug) | typing in the door sound box, Tab / Enter | — |
| 0x4ec730 | script_editor_creature_ok | yes (debug) | creature window OK | — |
| 0x4ecbe0 | script_editor_level_item_ok | yes (debug) | level item window OK | — |
| 0x42f500 | worldmap_frame | yes | world map | `level worldmap1 worldmap1` |
| 0x42f870 | worldmap_marker_clamp_to_edge | yes | a location off screen | world map, `key LEFT 2000` |
| 0x430580 | automap_labels_save | yes, but writes data | console `save_map_labels` rewrites `automap_labels.tbl` | only on a scratch copy, via `[ConsoleKeys]` |
| 0x552310 | ui_inputbox::process | yes (debug) | vtable 0x581f30; in single player only the editor uses vsdk boxes | type in an editor box |
| 0x552870 | ui_inputbox::set_valid_chars | yes (debug) | called 15 times by create_ui | opening the editor runs it |
| 0x552c50 | ui_inputbox::set_invalid_chars | no in SP | only from ui_parse_inputbox (multiplayer UI tables) | force with `callt 552c50 <box> <str>` |
| 0x52ca60 | ui_listbox_entry::wrap_text | yes (debug) | editor list layout | — |
| 0x52d080 | ui_listbox::process | yes (debug) | vtable 0x581e48 | click in an editor list |
| 0x52d650 | ui_listbox::create | yes (debug) | create_ui | — |
| 0x52da40 | ui_listbox::add_entry | yes (debug) | create_ui and the list fills | — |
| 0x52e380 | ui_listbox::scroll_page_up | yes (debug) | PgUp in a focused editor list | — |
| 0x52e590 | ui_listbox::scroll_page_down | yes (debug) | PgDn | — |
| 0x52e920 | ui_listbox::get_vscroll_fraction | **no** | dead: no call site or data reference anywhere | — |
| 0x52ebe0 | ui_listbox::snap_scroll_bottom | yes (debug) | set_vscroll | scroll an editor list |
| 0x52ed70 | ui_listbox::set_vscroll_fraction | **no** | dead: no call site or data reference | — |
| 0x526a50 | ui_button::process | yes (debug) | vtable 0x581d9c | click any editor button except Save |
| 0x55e420 | ui_slider_base::get_marker_rect | yes (debug) | list scroll bars | — |
| 0x55ed30 | ui_slider::create_scrollbar | yes (debug) | ui_listbox::create | — |

**Totals:** 79 of 90 can run in single player in the user's install, 40 of them only inside the debug script editor. The 11 that can't:
- 3 dead: `camera_blend_sequence::advance`, `ui_listbox::get_vscroll_fraction`, `set_vscroll_fraction`.
- 1 multiplayer only: `ui_inputbox::set_invalid_chars`.
- 6 reachable only through the editor's Save: the 4 `.tbl` writers plus `editor_dialog_show` and `editor_dialog_handle_event`.
- `automap_labels_save`, which works but rewrites a data table, so only on a scratch copy.

### (c) No behavioural difference found
The other 87 functions. These were checked by hand beyond the automatic diffs:
- All nine `*_page_up` functions: `max(pos - page, 0)` and the guard, which is `>= 0` for buy/sell and save/load as in the original.
- HUD and inventory:
  - `hud_drag_icon_begin`: the merged bit-field store matches.
  - `hud_draw_tooltips`: the case table matches.
  - `inventory_popup_result`, `inventory_identify_item`, `inventory_drag_to_equip`, `inventory_equip_item`.
  - `inventory_preview_view_begin`: all 10 cases and the offset table match.
- Other screens:
  - `options_init`, `ui_message_box_hit_test`, `journal_handle_click`, `itemtrade_handle_click`, `status_handle_click`.
  - `party_find_next_member` / `party_find_prev_member`: the 0x82 / 0x8a masks match.
  - `automap_center_on_player`, `automap_drag_scroll`, `automap_label_text_input`, `automap_labels_save`, `assess_format_info`.
- Script editor:
  - `place_creature`, `add_navpoint`, `script_create`, `select_prev`, `focus_selection`, `action_type_allowed`, `apply_action_params`.
  - `default_action_navpoint`: the original's NULL read at `[0x28]` is kept (C220).
  - `params_ok_cb`, `door_sound_radio_cb`, `door_sound_inputbox_cb`, `level_item_ok`.
  - `creature_ok`: the always-true skill clamp is the original's.
  - `handle_input`, `write_objects`, `level_items_write_table`, `script_action_format`.
  - `editor_dialog_handle_event`: callback 3 calls callback 2, as in the original.
- vsdk UI: `ui_button::process` (all 10 states), `ui_inputbox::process` (key table), `set_valid_chars` (charset), `snap_scroll_bottom`, `ui_listbox::process` (`and bh,-17` is the same mask as `0xffffefff`).

The rest had clean automatic diffs or differences explained by layout only, and were not read line by line:
- Script editor: the rest of `create_ui`, `editor_dialog_show`, `pick_hover`, `ray_hit_object`, `draw_navpoints`, `update_buttons`, `fill_action_lists`, `edit_button_cb`, `write_level_script`, `write_level_table`, `list_door_sounds`.
- vsdk UI: the `ui_listbox` create, add_entry, wrap_text, scroll, get_vscroll_fraction and set_vscroll_fraction functions, and the two `ui_slider` functions.
- Other: `credits_parse_table`, `spells_*`, `hud_hot_buttons_drag`, the `worldmap_*` pair, `camera_blend_sequence::advance`, `ls_worldmap1_*`, `character_use_skill_on`, `itemtrade_count_party_rows`, `automap_handle_click`.

## Group D: game logic (113 functions)

I reviewed all 113 functions. None has a semantic error at medium or high confidence. Only three low-confidence x87 rounding notes remain.

### Method
- **Compiled the code.** All 42 source files were compiled with `vc6-compare --keep` (`VC6_STRICT=1`). Each function's VC6 listing was set against `asm.mjs fn`.
- **Compared features automatically.** A script compared struct offsets, immediates, call targets, globals, `fcomp`/`test ah` masks and float constants. The original's float values were read from Sum.exe.
- **Compared relocation targets.** A copy of vc6-compare dumped every relocation, resolved to the address the tree gives it, and matched it against the original's operand addresses. This catches wrong globals, missing address comments (the D9204 class) and wrong callees.
- **Read every function by hand against the original.** I checked each branch sense (the `test ah,0x41/0x01/0x40` jumps, `jl`/`jge`), the jump tables (decoded from Sum.exe, entry by entry for the spell, boss and AI tables), the bitfield masks, the argument order of calls and the loop bounds.
- **Checked for function statics.** None of the 113 functions declares a `static` local.

### (a) Suspected errors
All low confidence, all x87 rounding:
1. **`0x4f7fe0` / `0x4f8070` / `0x4f8100`, `ls_khosanilab_puzzle_corner_nn` / `_np` / `_pp`.** The original stores the products `s*d2->y` and `s*d1->x` to float temporaries before using them (`0x4f802c fstp [esp+4]`, `0x4f804b fstp [esp]`, and the same in `_np`/`_pp`). Ours keeps them on the FPU stack. Because `d` is 0 or ±1 and `s` is 0.3 or 0.7, the products are exact, so I expect no visible effect.
2. **`0x475430`, `matrix3_unpack_orientation` (multiplayer only).** The original rounds `r1*r1` and `u1*u1` to float (`0x475487`, `0x4754b0`) but keeps `a = 1 - r0*r0` and `u0*u0` in 80 bits. Ours has `a` as a float local, so the last bit may differ.
3. **`0x4ddc60`, `script_build_avoid_spline`.** Only the x87 operation order differs; the semantics were checked to match.

These look suspicious in a diff but are not errors:
- **Merged tails.** `ls_catacombs_use` calls `lsf_ball1_used` / `ball2_used` a different number of times, and `object_try_unlock_or_open` / `container_examine` share message and sound tails. VC6 merged identical code; every case still reaches the same handler.
- **Reversed `fcom` operands.** `avoid_box_box_overlap`, `avoid_sort_refs_along_segment` and `phoenix_choose_attack` use `test ah,0x41` where the original has `test ah,0x01`. Each was checked to be equivalent, equality included; they differ only for NaN.
- **Urath leaps.** The "missing 0.0333 constant" is the global `reported_frametime` (`0x5a5740`), not a literal.
- **`spell_probe_ground_point`.** Ours calls `get_node_world_transform`, which is the same address `0x537a10` under an old name.
- **`loadgame_debug_hook`.** It has no address (the DLL's own copy); the original calls a bare `ret` at `0x40acc0`.
- **`ring_format_info`.** Two `.bss` "string" references are the empty `""` initialiser; both read a zero byte.
- **`container_examine`.** Calls `stricmp` instead of `_stricmp`; same semantics.
- **`write_item_objects`.** Compares pointers unsigned (`jb`) where the original uses `jl`; same result for these addresses.
- **`scroll_next_page`.** The `max()` macro calls `gr_get_font_height` twice; it has no side effects.

**Seven of the "never-run" functions are already tested by the quicksave comparisons.** `hybrid-test.mjs runOnce` sends `hybridcov` before the quicksave (`call0 40e7c0`), so the coverage files never count the quicksave chain, yet every hybrid quicksave ran it and matched the original:
- `savegame_location_allowed`
- `savegame::write_header`, `write_living`, `write_party_state`, `write_character`, `write_item_objects`
- `Character::save_state`

Only their rare branches remain untested (a summon out, type 9, dead characters).

Coverage was also only collected for the test-save load and Iona. So `boss_mount_pick_flee_point` (riders released when jadetemple or Liangshan loads) and `follow_repath` (party followers re-pathing) probably already ran, unrecorded, in the other level tests.

### (b) Proposed scenarios
`poke 609fe0 1` before `level` brings the full party. Console commands need a `[ConsoleKeys]` binding (`Key=command`) and then `postkey Key`. The SCRIPT name variants still need checking against `level_script_table`. `CUR` = `peek 5fcc88 4`.

| # | Scenario | Functions reached |
|---|---|---|
| S1 | Hold-to-walk (`down left; move 900 250; wait 3000; up left`, full party); `key R` twice; `key M` twice; Esc then Save, and Ctrl+S; the journal key (one of Q / Enter / F3 / F4); `callt 4d16a0 0x2493a38 0 5 0 2` | `world_pick_under_mouse`, `Character::ai_travel_offset`, `travel_steer_update`, `follow_repath`, `statscreen_load_anim`, `level_format_display_name`, `savegame::find_flag_level`, `savegame::hot_button_set` |
| S2 | Click a locked door (with and without its key) and an examinable container | `object_try_unlock_or_open`, `container_examine` |
| S3 | `poke 60acd4 1` (`config_combat_flag`), `poke 8b50b2 1` (`Quick_combo`), then melee as cur_character | `character_learn_chain_skill` |
| S4 | `firewall 3`, then click-walk across the walls; console `avoidance enable` / `avoidance rect` / `avoidance reset`; `poke 245c9d8 1` (`Precalc_vis_method`) and click-walk again; click an off-mesh point | `spell_place_ground_effect`, `spell_probe_ground_point`, the 8 avoidance functions, `cmd_avoidance`, the 7 pathvis functions, `pf_find_cell_crossing` |
| S5 | Console `summon golem`, then quicksave with the golem out | `summon_create`; the summon branch of `write_party_state` |
| S6 | `bag 1 Fire~Arrow~Rune`, open it, use the rune on a monster; open the item info popup (or `calli 44b7a0 <item*>` with the pointer from `peek 970824 4`); pick up a level item | `character_use_inventory_item`, `ring_format_info`, `savegame::level_item_remove` |
| S7 | `level jadetemple jadetemple`, talk to Gongji (CheckScales / LoseScales need scales added with savedit), page down a long reply, choose "Farewell." | `conversation_execute_action`, `ls_jadetemple_action`, `scroll_next_page`, `conversation::farewell` |
| S8 | `level IonaExt IonaExt`; `level lenele1b lenele1b`, then `_v2` and `_v3`; Catacombs: `callstr 4fe5e0 cry_kinge` / `cry_kingf` / `Puzzle-Ball01`; KhosaniLab: `callstr 4f95a0 puzzle-flippers31` | `ls_ionaext_frame`, `ls_lenele1b_init`, `ls_catacombs_use`, the 6 khosanilab puzzle functions |
| S9 | `level LPalaceInt LPalaceInt` (or `_v2`), walk into a guard's view, then wear the servant disguise | `script_guard_sees_player`, `script_build_avoid_spline` |
| S10 | `poke 5fbfd8 (V|0x80)` to open the script editor, wait, `key Backspace` (< Time), restore V | `level_ambient_find_at`, `level_script_restart` |
| S11 | `level TempleInt2 TempleInt2`; `urath_combat 1`; drop Urath's HP below 80 / 60 / 40 / 20 % with `callt 450850 URATH 0xC3960000 -1 0 0` (−300 HP each); then `darkjo_combat 1`, `machival_combat 1` | the 5 Evil Joseph functions, the 5 Evil Urath functions, `machival_ai_select_action` |
| S12 | `level KhosaniLab2 KhosaniLab2`; `pyrul_combat 1`; `pyrul_stage 2`, then `pyrul_stage 3` | the 3 Pyrul functions, `spell_spawn_creature` (meteor storm), `spell_place_ground_effect` |
| S13 | KhosaniLab: `gsal_combat 1`, `gs_stage 2/3`, party melee | both `giant_salamanka_*`; likely `approach_target_point` |
| S14 | `level IkaemosBottomInt2 IkaemosBottomInt2`; `titus_combat 1`; `titus_stage 2`, then `3` | `titus_ai_select_action` |
| S15 | `level rand-hills01 rand-hills01`; `phoenix_combat 1`; stay at range; `phoenix_stage 3` | `phoenix_choose_attack`, `phoenix_reposition_update`, `fire_weapon_missile` |
| S16 | `level TempleInt TempleInt`; `tiger_combat 1`; fight past the four spear moves; `tiger_stage 3` | the 8 tiger functions |
| S17 | jadetemple, `navgoto $murodtrigger`, `npckill Murod`, fight Murod Oni (or `level Liangshan Liangshan`) | both `murod_oni_*`, `boss_mount_pick_flee_point` |
| S18 | `level rand-forestnite1 …; ghost_combat 1`; `level rand-forest01 Rand-Forest01_v2; serpent_combat 1; serpent_stage 2`; `poke 24523a8 38` for the illusion | `ghost_find_floor_below`, `serpent_part_is_busy_or_asleep`, both `serpent_illusion_*` |
| S19 | `level Sewerboss sewerboss`; console `tentacle_test`, then `tentacles` | both `cmd_tentacle*` |
| S20 | `level IonaEXT02 IonaEXT02_v2`; console `stage 2`; wait for a grab | `offset_point_sideways` |
| S21 | Flece's Backstab from behind (or `callt 4741d0 FLECE+0x324 TARGET 5`); an AI caster that knows Life Drain (spell 0xd) | `add_skill_from_behind`, `consider_life_drain` |
| S22 | Walk across a two-point line trigger | `level_trigger_point_left_of_line` |
| S23 | Add an automap label | `savegame::automap_label_add` |

### (c) Not reachable in single player
- **Multiplayer only:**
  - `living_combat::send_net` (every call is behind `game_flags & 2`).
  - `spell_prepare_cast` and `spell_cast_launch_effects`: only in the multiplayer host branch of `spell_cast_execute` (`0x47c5f0`) and of combat.
  - `Character::spell_apply_hit_effects`: callers are the launch function and three `fx_frame` sites, all inside `game_flags & 2` blocks.
  - The three `mp_handle_*` packet handlers.
  - `matrix3_pack_orientation` / `matrix3_unpack_orientation` (NEW_LEP packets).
- **Dead or debug:**
  - `state_loadgame_enter` and `loadgame_level_list_cb`: game state 8 is never entered in v1.40.
  - `cd_prompt_disc2`: only when `eleh.s3d` is not installed, never on a full install.
  - `serpent_illusion_start` / `_update`: dead in v1.40 (game bug K213); only with the S18 poke.
  - `level_script_restart` / `level_ambient_find_at`: script editor only (S10).
  - `cmd_avoidance`, `cmd_tentacles`, `cmd_tentacle_test`: console only, through `[ConsoleKeys]`.
  - The pathvis family: only with `Precalc_vis_method` set (default 0); S4.

### (d) No behavioural difference found
All 113 functions, apart from the x87 notes in (a). Two were read lightly and are worth knowing about:
- **`ring_format_info`:** the per-ring text blocks were sampled. Constants, strings, the 60000 thresholds and every branch were checked.
- **`Character::save_state`:** sampled, but covered by the save tests.

## Group E: engine (44 functions)

No behavioural decompilation error at medium or high confidence.

### (a) Suspected errors

- **`float_format_buf` `0x542830`** (low, no effect): returns 0 on two early exits where the original leaves eax:
  when `buf == NULL` only `al` is loaded (`0x54283c`, return at `0x542854`); for value 0 with size < 4, eax holds the
  `fnstsw` status (`0x5428d1`, return `0x5428e0`). Nothing calls the function.
- Notes: `fast_sqrt_tables_init` `0x5425f0` has no caller (the tables are zero in both builds);
  `LineFile::detect_eol` returns 1 where the original returns `fill_buffer`'s eax, always 0 there (path never
  taken). No function-static or global without an address comment.

### (b) Reachability (22 in single player)

- **Retail paths:** `int_to_string_with_commas` `0x5152a0` (character screen, key R); `movement_read_controls`
  `0x531f50` (console `slew`, keys A / numpad); `PrintSystemInfo` `0x53171f` and `cmd_sysinfo` `0x531aa8` (console
  `sysinfo`); `LineFile::detect_eol` `0x558430` and `read_line` `0x558600` (console `script <name>` with a `.vcs`
  file in the install folder: CRLF, LF and mixed line ends, long lines, comments); `RecordExceptionInfo` `0x5419a0`
  and `ShowModuleInfo` `0x541fb0` (a crash, which writes `errorlog.txt`); `cmdline_take_argument` `0x53e250`
  (launch with `-port` / `-rz`).
- **Script editor** (`poke 5fbfd8 |= 0x80`, or `[Debug] ScriptEditorKey`): `ui_checkbox::create` `0x553d00`,
  `ui_radio_button::create` `0x5548a0`, `ui_gadget::send_event` `0x52c330` / `process` `0x52c390` / `set_parent`
  `0x52c570`, `ui_window::handle_arrow_key` `0x52abf0` / `send_event` `0x52aff0` / `init` `0x52b100`.
- **Direct3D renderer only:** `0x54bca0`, `0x54c0a0`, `0x54d000`, `0x54d120`.
- **Launch with `-eax`:** `eax_init` `0x521f40`.

### (c) Direct call only (13) and no practical path (9)

- **Direct call:** `file_blast_into_buffer` `0x517220`, `fast_inv_sqrt` `0x542500`, `fast_sqrt_refined`
  `0x542540`, `fast_sqrt` `0x542590`, `float_format_buf` `0x542830`, `key_clear` `0x509b50`, `LineFile::write`
  `0x5587c0`, `sound_emitter_calc_volume` `0x5210b0`, `sound_emitter_calc_pan` `0x521270`,
  `snd_set_volume_in_group` `0x5216e0`, `file_list_files` `0x513590` (allocates), `ui_window::parse_radio_button`
  `0x529d70` / `parse_slider` `0x52a180`.
- **No practical path:** `mem_report_line` `0x512640`, `TextFile::Error` `0x515ee0`, `TextFile::ParseUInt`
  `0x516380`, `file_write_text_block` `0x517180`, `ds_create_buffer` `0x55ca10`, `ds_fill_buffer` `0x55cae0` and
  `snd_stream_fill_pcm` `0x566820` (PCM only; every shipped WAV is MS-ADPCM), `File::set_cipher_key` `0x5143e0`,
  `File::calc_crc32` `0x514d60`.
