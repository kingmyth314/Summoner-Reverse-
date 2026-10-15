# decomp63: the render-scene unit, the shadow-table unit and the equip packets (0x481680-0x487b0f)

Written from the Sum.exe v1.40 disassembly only (`node tools/asm.mjs fn|grep|callers|strings`, a range dump
of build/sum.asm, raw .data bytes and the EH FuncInfo tables read from Summoner/Sum.exe). Summoner 2 was not
used. Headers: a copy of `src/decomp/include` at 00017da (re-copied twice: decomp60, decomp62 and decomp64
landed while this batch was written; my changes were re-applied onto vfx.h under decomp60 / decomp64's
names, and fx.h / effect_mem.h's own decomp64 declarations replaced mine), plus the changes listed in
CHANGED-HEADERS.txt and DECISIONS.md D4300-D4309. All three files pass
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include` without
warnings; all 173 `src/decomp/{game,vsdk}/*.cpp` compile against the copy with the same warning counts as
against the repo headers.

109 of the 110 functions of 0x481680-0x487b10 are written: **69 exact / 40 likely / 0 unsure**.
Left: `mp_handle_unequip_item` (0x487b10-0x487b83), the last function of the equip-packet file; the batch
range ends at its start address. It belongs at the end of game/unnamed_equip_packets_0x4878a0.cpp.

All three files are optimised builds (/O2 /Ob1: no frame pointer, 0x90 padding, shared __real@ floats).

## Units and evidence

| File (placeholder) | Code | .data | .bss | Functions | exact / likely / unsure |
|---|---|---|---|---|---|
| `game/unnamed_render_scene_0x481680.cpp` | 0x481680-0x48743f | 0x59bd70-0x59c11f | 0x22ea498-0x22fb71b | 99 | 64 / 35 / 0 |
| `game/unnamed_shadow_table_0x487440.cpp` | 0x487440-0x48789f | 0x59c120-0x59c17f | 0x22fb720-0x22fedd3 | 7 | 3 / 4 / 0 |
| `game/unnamed_equip_packets_0x4878a0.cpp` | 0x4878a0-0x487b8f | none | none | 3 of 4 | 2 / 1 / 0 |

No file can be named: no assert path, no `// file.cpp` block of strings.tbl, no localize id in the range,
sum.symbols module "-" for every function. levelrender.cpp (path literal 0x59c524) is a different object
that starts at 0x48db30 (after multichat.cpp 0x487b90, multicontainer.cpp and the fx unit), so the
render-scene code is not "levelrender.cpp" even though sum.symbols names its functions levelrender_*.
modules.txt is therefore empty.

- **Start 0x481680**: static initialiser entry 239 (multiendgame.cpp's entry 238 is at 0x4811a0, its last
  RET 0x481673). Entries 239-249 are this unit's: three empty initialisers, then the eight console
  commands in the order lod_dist, main_lod_dist, render_level, render_characters, render_effects,
  set_fog, Cull_ground, Cull_ground_too (the next entry, 250, is the containers unit at 0x489730).
- **.data of the render-scene unit**: variables 0x59bd70-0x59bdab (raw bytes: -1, -1, four bool 1s, 50.0,
  35.0, 50.0, 35.0, 1, -1, -1, 1, 9.0, 27.0, 0xff, 1), then the literals in function order ("lod_dist"
  0x59bdac .. "$vis_set" 0x59c114). The previous unit's last literal is multiendgame.cpp's "Unknown"
  (0x59bd68).
- **Cut at 0x487440 (D4304)**: the variable shadows_enabled (0x59c120 = 1) and five more variables
  (move_target_bm = -1, 2.0, 0.15, 1.0, 5.0) follow the render-scene unit's last literal; the shadows.tbl
  literals follow them, and multichat.cpp's command table starts at 0x59c180. shadow_get_character_anchor
  (0x487440) is the first function to read that .data (0x59c128); detail_apply_level_settings
  (0x487370) reads only the first unit's (0x59bd70, 0x22fb010). .bss: the render-scene unit's last byte
  is 0x22fb718 (levelrender_layer_fog_applied), the shadow table starts at 0x22fb720 and ends with its
  count 0x22fedd0; multichat.cpp's ring starts at 0x22fedd8.
- **The equip packets 0x4878a0-0x487b83**: no .data, no .bss (the send counters 0x595758 / 0x5958e0 are
  mp_packet_table[0x33 / 0x41].sent). Nothing ties them to the shadow unit or to multichat.cpp; kept in a
  placeholder file of their own (D4304). The source map's `0x487b90 multichat.cpp` row stays as it is.
- **.bss of the render-scene unit 0x22ea498-0x22fb71b**: after multiendgame.cpp's mp_net_handle_map
  (0x22e94f4 + 4000 = 0x22ea494). VC6 does not keep the definition order in .bss here (the hover tint
  components are spread over the block), so the .bss order says nothing about the source. Four dwords
  in the block are referenced by no instruction (0x22ec4c4, 0x22f8ce4, 0x22f8cfc, 0x22f9554): kept as
  named dwords.
- **.rdata**: the unit's own floats are 0x581a80-0x581aa8 (2500, -256, 256, 24, 900, 50625, 1600, 5625,
  1200, 32, 127); the rest are shared constants.

## How the frame is drawn (levelrender_render_scene 0x482200)

The current layer (0..19, set by level_set_current_layer) selects a visibility record: a list of
"$vis" sets of the .vis file, the "HIGH" variants with Summoner_options.fog_quality != 0 (a cutscene's
$VisSet list replaces it while level_vis_forced). The entry pool (0xc800 bytes) and the three lists are
reset; the level fade steps 256 alpha units a second (0 = ground culled, Cull_ground); the eye and the
level lights are set up; then:

1. levelrender_fade_occluders: every object of the sets starts the frame opaque; the camera occluders
   and the members of their "$seethru" set fade to alpha 0x80 in 270 ms and back after 1 s without
   occluding.
2. levelrender_add_lobjs: each object once (lobj flag 0x2000): culled by sphere (doors grow by their
   travel) and box; partly faded objects go to the sorted list, objects with alpha faces (flag 0x10) to
   the opaque list, the rest to the attach list. Lamp objects (flag 8) fade their glow out.
3. levelrender_build_draw_lists (single player, or a network game in state >= 5): items ("Gold" only,
   20 m), containers (20 m), living entities (fog distance, a drawn floor object, 75 m when small, 225 m in
   cutscenes) into the opaque list (opaque model) or the sorted list, and their props within 30 m.
4. levelrender_add_effects: effect nodes, +0x2c particle records (updated, unless paused) and emitters;
   the $vis tags of every effect are reapplied when the vis sets change.
5. The sorted list is Shell-sorted back to front; blob shadows for the living-entity entries within 20 m
   of both lists; the sky effects are drawn around the eye before a z-buffer clear; then attach, opaque,
   shadows, sorted; particles; snow (per layer); speaker icons, speech lines, name labels, the hovered
   entity's health bar, the chain marker, floating texts, lamp glows; finally letterbox bars in a
   Direct3D cutscene and a 3-pixel bar on S3 cards.

## Functions

| Address | Function | Confidence |
|---|---|---|
| 0x481680 | init_empty_481690_thunk | exact |
| 0x481690 | init_empty_481690 | exact |
| 0x4816a0 | init_empty_4816b0_thunk | exact |
| 0x4816b0 | init_empty_4816b0 | exact |
| 0x4816c0 | init_empty_4816d0_thunk | exact |
| 0x4816d0 | init_empty_4816d0 | exact |
| 0x4816e0 | init_ccmd_lod_dist_thunk | exact |
| 0x4816f0 | init_ccmd_lod_dist | exact |
| 0x481710 | cmd_lod_dist | exact |
| 0x4817a0 | init_ccmd_main_lod_dist_thunk | exact |
| 0x4817b0 | init_ccmd_main_lod_dist | exact |
| 0x4817d0 | cmd_main_lod_dist | exact |
| 0x481860 | init_ccmd_render_level_thunk | exact |
| 0x481870 | init_ccmd_render_level | exact |
| 0x481890 | cmd_render_level | exact |
| 0x481950 | init_ccmd_render_characters_thunk | exact |
| 0x481960 | init_ccmd_render_characters | exact |
| 0x481980 | cmd_render_characters | exact |
| 0x481a40 | init_ccmd_render_effects_thunk | exact |
| 0x481a50 | init_ccmd_render_effects | exact |
| 0x481a70 | cmd_render_effects | exact |
| 0x481b30 | init_ccmd_set_fog_thunk | exact |
| 0x481b40 | init_ccmd_set_fog | exact |
| 0x481b60 | cmd_set_fog | likely |
| 0x481c40 | character_draw_overhead_bitmap | likely |
| 0x481e00 | char_fog_set | exact |
| 0x481e40 | char_fog_disable | exact |
| 0x481e50 | char_fog_get_far | exact |
| 0x481e70 | init_ccmd_Cull_ground_thunk | exact |
| 0x481e80 | init_ccmd_Cull_ground | exact |
| 0x481ea0 | cmd_Cull_ground | exact |
| 0x481f60 | init_ccmd_Cull_ground_too_thunk | exact |
| 0x481f70 | init_ccmd_Cull_ground_too | exact |
| 0x481f90 | cmd_Cull_ground_too | exact |
| 0x482050 | levelrender_fade_out_start | exact |
| 0x482070 | levelrender_fade_reset | exact |
| 0x482090 | hover_tint_active | exact |
| 0x4820b0 | hover_tint_set_object | exact |
| 0x4820e0 | hover_tint_set_character | exact |
| 0x482110 | level_set_current_layer | likely |
| 0x482200 | levelrender_render_scene | likely |
| 0x482850 | levelrender_fog_enable | likely |
| 0x4829e0 | levelrender_fade_occluders | likely |
| 0x482d40 | levelrender_fog_disable | exact |
| 0x482d70 | levelrender_setup_level_lights | likely |
| 0x482e20 | levelrender_build_draw_lists | likely |
| 0x483260 | levelrender_add_character | likely |
| 0x4833b0 | levelrender_add_bone_attachment | likely |
| 0x483430 | levelrender_push_attachment | likely |
| 0x483570 | levelrender_add_items | likely |
| 0x4836f0 | levelrender_add_containers | likely |
| 0x483850 | levelrender_add_lobjs | likely |
| 0x4838f0 | levelrender_add_lobj | likely |
| 0x483ba0 | levelrender_add_effects | likely |
| 0x483f90 | levelrender_effect_apply_vis | likely |
| 0x4841e0 | levelrender_add_effect_entry | likely |
| 0x484380 | lamp_glows_draw | likely |
| 0x4843e0 | lamp_glow_update_lobjs | likely |
| 0x484600 | lamp_glow_update_effects | likely |
| 0x484830 | levelrender_sort_back_to_front | exact |
| 0x4848c0 | levelrender_draw_health_bars | likely |
| 0x484aa0 | overhead_label_slots_expire | exact |
| 0x484ae0 | levelrender_draw_speech_text | likely |
| 0x484e30 | overhead_label_place | likely |
| 0x4850a0 | levelrender_draw_name_labels | likely |
| 0x485380 | levelrender_draw_float_text | likely |
| 0x485810 | levelrender_draw_speaker_icons | likely |
| 0x4858f0 | levelrender_draw_list | likely |
| 0x4863b0 | levelrender_rank_light | likely |
| 0x486520 | levelrender_lobj_collect_projections | exact |
| 0x4865a0 | levelrender_level_load | likely |
| 0x4868f0 | detail_levels_parse_table | likely |
| 0x486a40 | levelrender_apply_lobj_detail_flag | exact |
| 0x486ab0 | levelrender_level_unload | exact |
| 0x486b00 | levelrender_clear_draw_lists | exact |
| 0x486b10 | fog_set_distances | exact |
| 0x486b90 | fog_set_level_default | exact |
| 0x486bc0 | fog_restore_level_default | exact |
| 0x486be0 | levelrender_get_flag_22fb6f7 | exact |
| 0x486bf0 | fog_get_near_current | exact |
| 0x486c20 | fog_get_far_current | exact |
| 0x486c60 | level_vis_load | likely |
| 0x486d40 | level_vis_parse | likely |
| 0x487030 | level_vis_reset | exact |
| 0x487080 | fog_set_high_camera_default | exact |
| 0x4870b0 | fog_set_low_camera_default | exact |
| 0x4870e0 | fog_set_color | exact |
| 0x487100 | fog_set_layer | exact |
| 0x487150 | level_vis_force_begin | exact |
| 0x487170 | level_vis_invalidate | exact |
| 0x487190 | level_vis_force_reset | exact |
| 0x4871b0 | level_vis_force_add | exact |
| 0x4871e0 | level_vis_force_end | exact |
| 0x4871f0 | fog_layer_set_snow | exact |
| 0x487210 | fog_layer_snow_allowed | exact |
| 0x487230 | fx_draw_flagged_particles | likely |
| 0x4872b0 | overhead_label_slots_clear | exact |
| 0x4872d0 | options_apply_detail | exact |
| 0x487370 | detail_apply_level_settings | likely |
| 0x487440 | shadow_get_character_anchor | likely |
| 0x4874c0 | shadows_parse_table | likely |
| 0x487630 | shadow_draw_character | exact |
| 0x4876b0 | shadows_draw_characters | exact |
| 0x487750 | move_target_set_bitmap | exact |
| 0x487760 | move_target_draw_decal | likely |
| 0x487800 | place_ground_decal | likely |
| 0x4878a0 | mp_send_equip_item | exact |
| 0x487950 | mp_handle_equip_item | likely |
| 0x487a70 | mp_send_unequip_item | exact |

Not written: `0x487b10 mp_handle_unequip_item` (outside the range).

The "likely" ones are mostly statement-order or expression-form guesses (FPU operand order of the depth
dot products, which local a stack slot is, the max / min macro shapes, the scope objects of the EH
frames). Notes on individual functions are in their comments.

## Header changes (CHANGED-HEADERS.txt has the list)

- levelrender.h: levelrender_entry completed, levelrender_attach_entry / levelrender_effect_entry, LRE_*;
  lobj_render_state (seethru, flags masks LRS_*, GameTimer); hover_tint_active and levelrender_level_load
  return int (D4300); the three back ends; levelrender.cpp's levelrender_lights / the two flag bytes /
  levelrender_wave_vertices; includes the two new unit headers at its end (D4305).
- New: engine/rendering/unnamed_render_scene_0x481680.h, unnamed_shadow_table_0x487440.h.
- vfx.h: the fields levelrender reads in vfx_emitter_desc / vfx_node_desc / vfx_desc21c / vfx_base /
  vfx_instance, vfx_base_tag, vfx_inst21c, vfx_node_ref, the render-state and vif_* functions (D4302,
  D4309), attached_light::world_pos comment (D4303). fx.h (fx_char_link, fx_sky_list, level_light_get)
  and effect_mem.h (fx_rec98::glow) already had what the unit needs (decomp64).
- gr.h: gr_light_desc::reset, Gr_3d_zmult, gr_start_instance / gr_stop_instance / gr_set_far_clip /
  gr_line_quad_3d. multi.h: MP_INV_EQP / MP_INV_UQP. No existing parameter list changed; no definition
  or call had to change.

## Open questions

- Which of the three empty initialisers (entries 239-241) belongs to which of the three vector objects
  (overhead_label_positions, levelrender_eye_pos, levelrender_eye_orient).
- render_scope (D4306): the original is probably a profiling macro whose class is empty in release; its
  name and header are not known.
- detail_apply_level_settings stores "+UnlockMedium" (fog quality 1) / "+UnlockHigh" (2) into
  game_camera +0x08, which the camera unit names high_view_suppressed and treats as "defer mode-3
  requests" (camera_set_mode 0x479930). Either the camera field's name is inverted or the table flags
  lock the high camera despite their names. Not run.
- The records at vfx_base +0x28 (decomp60: emitters) are drawn by vfx_instance::render_node as kind 4
  and have a bounding sphere; the 0x21c records at +0x2c (decomp64: vfx_desc21c) are what the vif_*
  particle functions take. The names follow decomp60 / decomp64 (D4302); which of the two is the
  particle emitter of the effect editor is not settled.
- path_internal (0x59bd74) is in this unit's .data, but only setup_data_paths (summoner.cpp) writes it.
- level_vis_reset points both lists of every layer at level_vis_set_lists and leaves
  level_vis_set_records[0] unset (with fog quality 1-2 the layer still sees $vis_base through the first
  list, so no visible effect).

## Doc disagreements

- docs/sum-exe/vsdk-gr.md, Fog (lines 145-148): "These two globals are named the wrong way round in
  sum.symbols: 0x59bd7c (fog_near) ...". sum.symbols has since renamed them (0x59bd7c fog_far_distance,
  0x59bd80 fog_near_distance); the paragraph should say so instead.
- sum.symbols `character_is_culled` (0x44f020): the result is gr_cull_sphere's, true when the sphere is
  outside the view (levelrender_build_draw_lists treats true as culled, 0x482ecc; living_entity.cpp
  writes it so). The name reads the other way.
- sum.symbols argument lists that differ from the code: levelrender_push_attachment is (item, character,
  bone pos, bone orient, alpha (unused), light level), not "(list, character, matrix, pos, alpha, model)";
  levelrender_add_bone_attachment is (character, item type, prop tag), not "(list, character, slot)";
  shadow_draw_character's radius and third argument (vector3_zero) are not read; overhead_label_place is
  (x, y, label height, in/out side, in/out row). levelrender_add_items / levelrender_add_containers take
  one object each.
- sum.symbols `fog_fading` (0x22fb6f8): "advanced in levelrender_fog_enable" — it is read there but no
  instruction sets it (the fade code is dead), and the same holds for 0x22fb714 (named here
  levelrender_no_lighting) and levelrender.cpp's 0x243e038.
- docs/sum-exe/camera.md (lines 86-94) gives the camera fog defaults 20 / 30 and 35 / 50: confirmed
  (fog_set_low_camera_default / fog_set_high_camera_default store near / far in that order).

## Game bugs (candidates for docs/sum-exe/game-bugs.md; not numbered)

- **Ghost Rider props in its cutscenes.** levelrender_add_character skips the attachment list of
  species 0x5a in a cutscene only when the cutscene script is "Ghost-Escape_cscript.tbl" *and*
  "Ghost-Dismount_cscript.tbl" (two `_stricmp` == 0 tests chained with and, 0x48333d-0x483369), which is
  never true: the attachments are always drawn. *Inferred* intent: or. Visual, latent unless the
  attachments look wrong in those two cutscenes (not run).
- **Lamp glows of effects skip one unrelated occluder.** lamp_glow_update_effects skips the attach-list
  entry whose index equals the current sorted-list index (`cmp edi, [esp + 0x20]`, 0x484785), a
  copy of lamp_glow_update_lobjs' "skip yourself" test that compares indexes of two different lists. One
  level object per glow node is never tested, so a glow can shine through it. Visual.
- **Three console toggles do nothing.** render_level, render_characters and render_effects toggle
  Render_level / Render_characters / Render_effects (0x59bd79-0x59bd7b), which no other instruction
  reads.
- **shadows.tbl is loaded and never used.** shadows_parse_table reads up to 175 records (name, radius,
  scale, +Prop) into 0x22fb720; no instruction reads them. Every character shadow uses the same constants
  (0x59c12c-0x59c134) and shadow_bm; the radius 20.0 shadows_draw_characters passes is ignored.
- **Dead code behind never-set flags.** fog_fading (0x22fb6f8: the frame-rate fog scaling in
  levelrender_fog_enable, fog_get_near_current / fog_get_far_current), 0x22fb714 (no level lights,
  distance culling of level objects, no lamp glows, LOD 2) and levelrender.cpp's 0x243e038 (re-run of
  level_lights_setup) are read but never set.
- **Level fog values swapped (candidate; needs the level tables).** level_script_parse_table calls
  fog_set_level_default(fog_far, fog_near) (level_scripts_common.cpp); its first argument becomes the
  near default (0x59bd88), which fog_restore_level_default / fog_set_distances copy into
  fog_near_distance, gr_set_fog's start distance. A "+Fog Far:" value therefore starts the fog and
  "+Fog Near:" ends it; per-layer "+Distance:" values are swapped the same way by
  level_set_current_layer (layer far into fog_near_distance, 0x48218a-0x4821b6). With near > far,
  gr_set_fog then limits near to half of far (K92's code). Whether the retail tables were written
  around this is not checked (tables.vpp's level tables are not extracted).
- **Overhead icons and labels use the clip origin for x only.** character_draw_overhead_bitmap,
  levelrender_draw_speech_text, levelrender_draw_float_text and levelrender_draw_health_bars subtract
  the clip x offset from the projected x but never the y offset (0x481d9c, 0x484c18, 0x4854e8,
  0x4849ce). Latent: they are drawn only outside cutscenes (game flag 0x20), where the clip origin is 0.
