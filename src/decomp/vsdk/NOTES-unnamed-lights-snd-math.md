# decomp60: 0x51d250-0x525710 (effect_mem.cpp's end, lights, attached lights, sound, EAX, quaternion, vector)

Written from Sum.exe v1.40's disassembly (`node tools/asm.mjs`), names from symbols/sum.symbols. Headers were
rebased on the repo at 66fb364 (decomp61's File units and decomp59's bosses included). 275 functions
(every symbol in the range except 0x525720 / 0x525740 / 0x525760, which decomp61 wrote as part of
unnamed_vfile_rw_0x525720.cpp): **173 exact, 102 likely, 0 unsure**. Every .cpp passes the mingw syntax
check, and every src/decomp .cpp still compiles against this header copy (effect_mem.cpp replaced by the
copy here).

## Units and evidence

The static initialiser table (0x588004) runs entries 588 (effect_mem.cpp's last), 589-590 (0x51e070,
0x51e0a0), 591 (0x51fb10), 592-595 (0x5204c0-0x520550), 596-597 (0x5225b0, 0x5225e0), 598-599
(0x523f30, 0x523f60), 600-603 (0x525720-0x525780, the File unit). Every object in the range ends with the
out-of-line copies of the inline functions it calls (COMDATs after the regular functions), which gives the
boundaries the initialisers do not.

| File | Code | Data | Evidence |
|---|---|---|---|
| vsdk/effect_mem.cpp (named, replaces the repo copy) | 0x51bb70-0x51e06a | as before | sum.symbols tags 0x51d250-0x51e020 effect_mem.cpp; the record constructors, the Mempool members and three more COMDATs (vfx_get_length_seconds, fixed8::set, float_to_fixed8) close the object before the light unit's initialiser |
| vsdk/unnamed_lights_0x51e070.cpp | 0x51e070-0x51fb08 | .data 0x5a8900-0x5a890f, .bss 0x2c9f9a0-0x2cb45cb | initialisers 589 / 590; COMDATs light__light, float_min at the end; vsdk\gr |
| vsdk/unnamed_attached_lights_0x51fb10.cpp | 0x51fb10-0x520413 | .bss 0x2cb45d0-0x2cb540f | initialiser 591; real functions to attached_light::reset, then 13 COMDATs; vsdk\geom |
| vsdk/unnamed_ca_render_0x520420.cpp | 0x520420-0x5204b8 | none | after the attached-light COMDAT block, before the sound initialisers (D4007) |
| vsdk/unnamed_snd_0x5204c0.cpp | 0x5204c0-0x521e38 | .data 0x5a8910-0x5a895f, .bss 0x2cb5410-0x2cbf487 | initialisers 592-595; COMDATs Sound::is_used, clamp_uint; pooled strings (D4010) |
| vsdk/unnamed_eax_0x521e40.cpp | 0x521e40-0x5225a1 | .data 0x5a8960-0x5a8b4b, .bss 0x2cbf488-0x2cbf497 | after the sound COMDATs; no pooling (28 format literals); D4010 |
| vsdk/unnamed_quaternion_0x5225b0.cpp | 0x5225b0-0x523f29 | .bss 0x2cbf498-0x2cbf4af | initialisers 596 / 597; nine operator / constructor COMDATs at the end |
| vsdk/unnamed_vector_0x523f30.cpp | 0x523f30-0x525710 | .data 0x5a8b4c-0x5a8b63, .bss 0x2cbf4b0-0x2cbf4c3 | initialisers 598 / 599; five COMDATs at the end; the File unit follows (0x5a8b64 is File__write_line's) |

No file name is known for the placeholders: no path string (the only vsdk path literal in .data
0x5a8700-0x5a8b70 is effect_mem.cpp's), no strings.tbl block, and Summoner 2's units with the same vector /
quaternion functions are unnamed too (unit_00196B10, unit_00197430).

Build flags seen: every unit here calls vector3 / matrix / quaternion inline members out of line (no
inline expansion) but writes FSQRT / FCOS inline (intrinsics); the sound unit pools strings, the EAX unit
and effect_mem.cpp do not.

## Functions

effect_mem.cpp (22): 0x51d250 vfx_instance_create likely, 0x51d7b0 vfx_instance_free likely, 0x51db50
effects_hash_name exact, 0x51dbb0 effects_load_preload_rfx likely, 0x51dc20 effects_release_level_memory
exact, 0x51dc50 effects_release_range exact, 0x51dca0 effects_stub_ret1 exact, 0x51dcb0 vfx_all_resident
exact, 0x51dd00 vfx_load likely, 0x51de90 effects_base_is_resident exact, 0x51dec0 effect_is_cutscene_base
exact, 0x51def0 effects_release_cutscene_memory exact; COMDAT comment lines (exact): 0x51df10, 0x51df20,
0x51df60, 0x51df80, 0x51dfb0 (record constructors), 0x51dff0 / 0x51e010 (Mempool), 0x51e020
vfx_get_length_seconds, 0x51e040 fixed8::set, 0x51e060 float_to_fixed8. (Also split into one line each:
the earlier 0x51bd20-0x51bd70 initialiser comments.)

Lights (45; 30 exact / 15 likely): init_lights, init_light_list_head, light_reset_all,
light_set_all_enabled, light_transform_to_instance, light_set_ambient / _get_ambient, light_create_point,
light_get, light_alloc, light_create_spot, light_delete, light_set_pos / _get_pos, light_set_enabled /
_is_enabled, light_set_radius / _get_radius, light_set_color / _get_color, light_set_spot_dir / _get,
light_set_spot_angles / _get, lights_collect_enabled, lights_select_nearest, light_compare_dist,
lights_pop_selection, lights_transform_selection, lights_get_selection_count, lights_get_selected_info,
lights_get_selected, lights_shade_point, light_sample_projected_bitmap, lights_shade_vertices,
lights_set_max_per_object, lights_set_detail_flag, lights_set_dynamic_enabled,
lights_calc_point_specular_color, gr_env_map_uv, light_set_dynamic, light_is_dynamic, lights_debug_draw;
COMDATs light__light, float_min. Likely: the vector temporaries (transform_to_instance, shade_point,
specular, env map), the -1 / break shape of light_alloc, set_all_enabled / set_enabled (int argument).

Attached lights (25; 19 / 6): init_attached_lights, vfx_light_track::relocate, vfx_light_desc::create_light
/ free_light / update_light / sample, attached_lights_init_slots / _clear_slots, attached_light_create_b /
_create / _free, attached_light::reset; COMDATs 0x520280-0x520400 (constructor, channel and BitArray
relocate, get_position, vector channel sample, fixed4_to_float, the four samplers,
attached_light_create_from_vfx (likely), get_scale, fixed8_to_float).

character_instance::render (1, likely).

Sound (67; 50 / 17): the four initialisers with the -nosound destructor pair, Sound__params_equal, snd_init,
snd_init_sounds, snd_init_group_volumes, snd_close, snd_change_listener, snd_do_frame, snd_pause_all,
snd_unpause_all, snd_get_handle, snd_load, snd_unload, snd_free, snd_unload_all, snd_level_page_in,
snd_level_release, snd_play, snd_get_volume, snd_play_looping, snd_play_streamed(_looping),
snd_calc_volume_3d, sound_emitter_calc_volume, snd_calc_pan, sound_emitter_calc_pan,
sound_emitter_calc_doppler(_segment), snd_play_3d, snd_stop, snd_stop_all, snd_stop_fade, snd_is_playing,
snd_set_volume, snd_set_volume_in_group, snd_set_pan, snd_set_frequency, snd_get_frequency, snd_get_format,
snd_set_group, snd_set_group_volume, snd_get_group_volume, snd_get_num_playing_channels, snd_get_sound,
snd_file_exists, snd_get_sound_info, sounds_parse_table, sounds_parse_entry, snd_get_rolloff,
snd_is_ds3d_enabled, snd_is_eax_enabled, snd_restore_reverb, snd_mute_reverb, snd_is_reverb_muted,
snd_get_duration, snd_calc_duration, snd_estimate_file_duration, snd_calc_rolloff,
snd_set_eax_environment; COMDATs Sound::is_used, clamp_uint.

EAX (14; 6 / 8): eax_set_decay_time, eax_set_environment, eax_set_environment_by_name, eax_stub_ret_m1,
eax_close, eax_init, eax_buffer_init, eax_mute_reverb, eax_restore_reverb, eax_is_reverb_muted,
eax_save_props, eax_get_listener_props, eax_load_props, eax_set_listener_props.

Quaternion (43; 19 / 24): the two identity initialisers, both matrix constructors, multiply (float and
cquat), to_matrix, to_matrix_b, normalized / conjugate / log / exp / normalize / negate_xyz / log_in_place /
exp_in_place for both classes, make_identity, quaternion_dot, cquat_dot, slerp and squad for both,
log_of_delta for both, cquat::set; nine COMDATs (cquat copy / 4-value constructors, =, unary -, -, +;
quaternion unary -, +, -). Likely: most of the FPU-heavy bodies (register / slot reuse, operand order).

Vector (58; 31 / 27): vector3_zero / vector2_zero initialisers, vector2 len / len_sq / normalize / negate,
vector3 get_major_axis, length_quick, dot_xyz, is_zero, is_valid, normalized, normalized_or_x, negated_b,
rotated, unrotated, normalize, normalize_length, normalize_safe, normalize_safe_length, negate, rotate,
unrotate, zero, random_unit_vector, random_direction, random_direction_in_cone(_axis),
vector3_project_onto_normal, vector3_remove_normal_component, distance, distance_squared, distance_quick,
dot_of_difference, vector3_normalized_dir, vector3_near_equal, vector3_normal, vector3_perp,
vector3_angle_and_axis, direction_dot, project_onto_plane, vector3_rotate_about_axis,
vector3_rotate_towards(_unit), bounding_sphere_from_points, polygon_calc_centroid,
vector3_world_to_local / _local_to_world, vector2_distance(_squared), vector2_dist_sq_to_line,
vector2_closest_point_on_segment, vector2_project_onto_segment; COMDATs vector2(x, y), vector3 /=,
vector2 -, float * vector2, vector2 dot.

## Decisions (DECISIONS.md D4000-D4013)

D4000 fx_rec48 = vfx_instance, fx_rec30 = vfx_light_desc; D4001 vfx_light_track (two record types share the
vfx_light_desc__ prefix in sum.symbols); D4002 gr_light is the attached-light layout (0x98); D4003
attached_light_create's third argument is range_near (float); D4004 float_max / float_min / fixed helpers /
vector3::dot inline; D4005 effects_load_preload_rfx returns int; D4006 light argument types; D4007
character_instance::render as its own unit; D4008 vsdk/snd/ds.h; D4009 snd_play_3d (looping, a, unused) and
int returns; D4010 sound and EAX are two objects; D4011 to_matrix fills a matrix43; D4012 cquat integer
normalisation; D4013 quaternion.h.

## Open questions

1. The .bss order inside each unit follows the definitions in address order where the initialisers allow;
   VC6's order for uninitialised globals is not proven (same question as frametime.cpp).
2. vfx_load's copy of the base's two flag bits into instance bits 7 / 8 (a SHL 3 / SAR 3 sequence) is
   written as an explicit expression; the source probably used bitfields on both sides.
3. snd_help: the -nosound CmdLineParam's help text points into this unit's .bss (0x2cbf484, a zeroed
   4-byte object, written as `static char snd_nosound_help[4]`); what the source named there is unknown.
4. 0x55db30 (counts playing channels, no symbol) is called ds_get_num_playing_channels here.
5. quaternion_log_of_delta / cquat_log_of_delta assign to a local that is never constructed (no call to the
   empty constructor, which the other functions of the unit do call).
6. fixed.h, quaternion.h, ds.h and the unnamed_lights header are our file names; vector2's new inline
   constructor (x, y) is the COMDAT 0x525650.
7. character_instance::render's unit (D4007): a separate object is the reading that fits the COMDAT
   ordering; it could also be the attached-light file with an unusual emission order.

## Doc disagreements

- docs/sum-exe/source-map.md: the attached-light range ends at 0x520413, not 0x520420 (0x520420 is
  character_instance::render, a separate object); the EAX range "may be the tail of the sound file" is a
  separate object (D4010); the sound .bss starts at 0x2cb5410 (snd_listener_vel), not 0x2cb5420.
- docs/sum-exe/effects.md, Lights: type 4 is never created (spot lights, type 3, carry the projected bitmap
  at +0x68, and light_delete unlocks only types 1 and 4); +0x24 / +0x28 are radians (cos(angle / 2),
  default 2 pi, create_spot keeps b >= a + 0.5), not degrees; +0x90 is the light's brightest channel at
  the selection point and lights_select_nearest sorts brightest first, not by distance (light_compare_dist's
  name says dist).
- vsdk/gr/gr.h's gr_light comment ("400 records of 0x94 bytes at 0x2ca5de0") describes the attached-light
  layout (D4002).
- vsdk/snd/snd.h's old snd_play comment (unused, group): the fourth argument reaches ds_play, the fifth is
  unused; snd_play_3d's float arguments are (bool looping, int) (D4009).
- vector.h's vector2_project_onto_segment description ("t of the projection of p") : it is a segment /
  circle intersection (the first t where a + t (b - a) is at distance margin from p).
- vector3::normalize_safe_length returns 1, not the old length (0), for a zero vector.
- sum.symbols: Sound__params_equal is cdecl with the record pushed, not a member; random_direction_in_cone
  and random_direction_in_cone_axis are thiscall vector3 members in Sum.exe (Summoner 2's free functions).

## Game bugs (from the code; none run)

1. **quaternion(const matrix *) (0x522790), case 2 reads m[0][1] for m[0][0]**: sqrt(fz - (rvec.y + uy) + 1)
   (0x522841). Quaternions built from matrices with a negative trace whose largest diagonal is fvec.z (turns
   near 180 degrees about the forward axis) come out wrong, then normalised. Callers: s3d_read_navpoint,
   vfx_instance__get_node_world_transform. The same function picks case 2 whenever fz > uy, even when rx is
   larger (precision only).
2. **cquat::normalized / normalize divide 16383^2 by |q|^2 as integers** (0x523094, 0x5232f7): any cquat
   slightly longer than one unit normalises to zero, and a zero cquat divides by zero. cquat_log_of_delta
   uses normalized, and anim_track__calc_rot_tangents uses that for the rotation key tangents.
3. **eax_get_listener_props reads property 14 twice** (0x5223e4, 0x522405): dwFlags is filled with the air
   absorption value, so `eax_save_props` (console command eax_save_props) writes it into <level>.eax and
   eax_load_props sets those bits as the listener flags.
4. **snd_play_3d's 2D fallback applies the group volume twice and ignores its scale**: snd_calc_volume_3d
   already multiplies by the group volume, snd_play_3d divides by the sound's volume only and snd_play
   multiplies by group volume * volume again; the 1.75 scale is applied only when below 1 (never). Without
   DirectSound3D the effects volume acts squared on positioned sounds.
5. **sound_emitter_calc_pan does not normalise the direction** (0x5212e7): segment emitters (ambient lines)
   get |pan| > 1 beyond one unit, which snd_set_pan clamps: they sound hard left or right.
6. **snd_get_handle compares min_range before clamping <= 0 to 1.0**: such a sound never matches its own
   record and takes a new slot on every call; the name is strcpy'd into a 256-byte buffer before the 0x28
   length check.
7. **light_create_point / light_create_spot do not check light_alloc's -1**: with all 400 lights in use they
   write the record before the table (the end of lights_selection).
8. **Projected spot-light bitmaps**: light_sample_projected_bitmap bm_lock's a bitmap without data and
   nothing unlocks it (light_delete unlocks types 1 / 4 only); formats other than 1 and 3-7 return an
   uninitialised colour; the bitmap colour replaces the distance-attenuated one, also beyond the outer radius.
9. **eax_buffer_init** neither checks QueryInterface nor releases the property set it gets (a reference per
   played buffer while EAX is on).
10. **vfx_light_desc::create_light** passes the raw 8.8 range (256 times too large) to the new attached
    light; update_light later scales it.
11. lights_debug_draw draws the first light_count table records, not the used ones; vfx_all_resident (no
    caller) indexes effect_bases with a name index; lights_calc_point_specular_color dereferences a NULL
    normal inside the inner radius (no direct caller found); character_instance::render returns with its
    instance still set in ca_render_set_instance when light->_unk00 bit 0 is set.

## Not written in the range

Nothing: 0x525720-0x525760 (the File unit's initialisers) are in decomp61's unnamed_vfile_rw_0x525720.cpp.
Outside the range but used here and still unwritten: ds_* (0x55c000-0x55dfe0), vfx_instance_init_nodes /
release_nodes (0x536a70 / 0x5369f0), vfx_channel::eval (0x55bc00), BitArray::get (0x55be80), the gr 3D
state unit (gr_view_pos, gr_light_matrix, gr_instance_depth), sound emitters (0x546600-), ca_render_*
(0x50d6c0-0x50d6f0).
