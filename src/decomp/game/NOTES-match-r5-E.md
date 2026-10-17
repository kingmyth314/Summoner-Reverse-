# match15 (29 files) - VC6 byte matching, measured with cl 12.00.8966 (VC6 SP5)

Started under 8168; on the coordinator's note the before lists were re-taken with the SP5 compiler
(vc6sp5/VC98) and every change re-checked there. before = repo tree (snapF) with the repo's vc6-flags.txt;
after = match15/src with match15/src/vc6-flags.txt (three /Ob0 lines added, D7300).

Total: 220 -> 280 of 347 (60 gained, 0 lost; address by address: before8966.list / after8966.list).
mingw -fsyntax-only: 0 error lines in all 29 files. No header changed.

| file | before | after |
|---|---|---|
| game/gamesound.cpp | 11/16 | 14/16 |
| game/item_paging.cpp | 5/8 | 5/8 |
| game/level_script_lenele1aa.cpp | 5/6 | 5/6 |
| game/level_script_rand_forestnite1.cpp | 4/5 | 5/5 |
| game/level_script_temple_inside.cpp | 8/10 | 8/10 |
| game/new_assess.cpp | 9/18 | 16/18 |
| game/new_gamelist.cpp | 19/29 | 26/29 |
| game/new_mainmenu.cpp | 8/13 | 13/13 |
| game/new_multichat.cpp | 8/12 | 8/12 |
| game/unnamed_cutscene_script_0x4ed050.cpp | 5/6 | 5/6 |
| game/unnamed_dialogue_table_0x4feb50.cpp | 18/21 | 20/21 |
| game/unnamed_editor_dialogs_0x430bc0.cpp | 3/5 | 3/5 |
| game/unnamed_evil_urath_0x4c4170.cpp | 11/18 | 13/18 |
| game/unnamed_level_complete_0x4b0960.cpp | 6/10 | 6/10 |
| game/unnamed_murod_0x4c5140.cpp | 7/12 | 8/12 |
| game/unnamed_projectors_0x4c0250.cpp | 5/11 | 9/11 |
| game/unnamed_shadow_table_0x487440.cpp | 6/7 | 7/7 |
| game/unnamed_shadows_0x4a37d0.cpp | 5/13 | 7/13 |
| vsdk/memory.cpp | 16/19 | 17/19 |
| vsdk/timer.cpp | 3/5 | 3/5 |
| vsdk/unnamed_anim_track_0x550660.cpp | 1/13 | 12/13 |
| vsdk/unnamed_d3d_tcache_0x54bb30.cpp | 3/14 | 9/14 |
| vsdk/unnamed_ds3d_0x55ddb0.cpp | 6/6 | 6/6 |
| vsdk/unnamed_exceptionhandler_0x5419a0.cpp | 6/8 | 6/8 |
| vsdk/unnamed_poly_clip_0x5519c0.cpp | 1/8 | 6/8 |
| vsdk/unnamed_time_format_0x532660.cpp | 0/1 | 0/1 |
| vsdk/unnamed_ui_button_0x526940.cpp | 19/24 | 20/24 |
| vsdk/unnamed_ui_slider_0x55e040.cpp | 20/26 | 21/26 |
| vsdk/unnamed_vfx_dummy_0x55fc70.cpp | 2/3 | 2/3 |

## What worked (D7301; full list in DECISIONS.md D7300-D7305)
- /Ob0 for anim_track, d3d_tcache, poly_clip (D7300); RVO now works under 8966, so anim_track's get_position /
  get_rotation / get_scale match unchanged.
- Screen templates (D6403/D6502/D6702) all still hold under 8966: assess, gamelist, mainmenu (13/13).
- Bit fields used as bool: (bool)bitfield gives shr / test cl,1 (shadow_collect_for_lobj, refresh_topic_flags).
- A call as call argument goes through a local (music_play(music_find_by_name(..)), evil_urath_cast_spell).
- Store order (cast_spell, conversation_speaker::init); inline dot for a plane distance (test_bounds);
  natural-order sqrt (evil_urath_filter_target); yaw = -yaw (place_ground_decal); profile_label (lobj_project_records).
- Bool / branch shapes: is_mempool_active, is_pressed, nearly_equal, is_texture_size, surface_play_sound,
  polygon_touches_planes; loop-invariant limit inside the loop (sphere_*_plane_volume).

## Still differs and why (D7305)
- page_up (assess, gamelist): max(x,0) AND register, every form tried.
- Shared return / epilogue blocks and case -1 slots: gamelist_handle_click, mp_server_list_draw, murod_ai_frame,
  murod / evil_urath select_action (urath's weights now as in the binary, closer).
- Scheduling of loads before stores: get_marker_rect (now 147 bytes = binary size, registers differ),
  allocate_in_mempool, d3d free_entry / get_section_rect, set_tooltip (owner store inside strcpy; the unlink and
  tail now as the binary).
- Frame slot sharing / temporaries: evil_urath leap_rise / travel / land / check_path, shadows_render,
  shadow_project_face, shadow_transform_to_lobj, to_lobj_space (plane::transform inline tried: larger), polygon_clip_*.
- calc_rot_tangents: only the store of tan_b.x is scheduled after the next loads (+0x499 of 0x62f).
- Not looked at in depth (time): multichat, exceptionhandler, item_paging, timer, editor_dialogs, temple_inside,
  vfx_dummy, time_format, cutscene_script, level_complete (its two 38-byte handlers differ only in ecx/eax),
  lenele1aa_init (stack pops not merged across 16 calls).

## Logic errors / game bugs
- evil_urath_ai_select_action: the 6/7 weight constant was one ulp off the binary (0x3f5b6db7 vs 0x3f5b6db8);
  now the binary's value (D7302).
- sound_play_3d_at passed float zeros to a bool / int parameter (same value; D7303).
- No new game bugs found.

## Confidence
52 markers raised to exact (every matched function that said likely / unsure). Markers that say exact but differ
were left as they were (D6207 practice).
