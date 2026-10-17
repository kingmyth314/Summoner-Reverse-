# match20: VC6 SP5 (cl 12.00.8966) byte matches in eight units

Tree: match20/src (copied from snapI). Flags: vc6-flags.txt is the coordinator's vc6-flags.vsdk-ow.txt (D7502, vsdk
units /Ox /Ow, game lines unchanged), with one change: `vsdk/unnamed_quaternion_0x5225b0.cpp /Ox /Ob0 /GX /MT /Ow`
(D7900). Game "before" = snapI source with the snapI flags; vsdk "before" = snapI source under the new D7502 flags.

| file | before | after | functions |
|---|---|---|---|
| game/ai.cpp | 74 | 78 | 88 |
| game/spells.cpp | 22 | 26 | 39 |
| game/unnamed_interface_0x411270.cpp | 52 | 55 | 69 |
| game/unnamed_ai_move_0x4b1410.cpp | 6 | 8 | 19 |
| game/level_scripts_common.cpp | 65 | 65 | 78 |
| vsdk/unnamed_lights_0x51e070.cpp (/Ox /Ob0 /Ow) | 30 | 35 | 41 |
| vsdk/unnamed_snd_0x5204c0.cpp (/Ox /Ob0 /Ow) | 46 | 48 | 58 |
| vsdk/unnamed_quaternion_0x5225b0.cpp (/Ox /Ow -> /Ox /Ob0 /Ow) | 11 | 21 | 32 |
| total | 306 | 336 | 424 |

(Under the old /O2 flags the starting point was 297; the D7502 flags alone account for 9 of the vsdk gains.) No
address lost (checked address by address, old and new flags). Every new match has its marker raised to exact.

## What worked (D7901 has the list)

- quaternion: /Ob0 (the unit calls its inline members out of line) plus the identity constants moved to the end so
  that vc6-compare pairs 0x522610 with the right constructor (D7900): normalized, conjugate, log, cquat::conjugate,
  squad, the matrix constructor. Then cquat::normalized (reverse declarations), both slerps (`<= EPS` branch first),
  both log_of_delta (r from the conjugate, a named normalized local).
- lights: header D7904 (signed `short enabled : 1`, bool setters) for set_all_enabled / set_enabled /
  collect_enabled, `? true : false` keeps light_is_enabled; light_reset_all (count store before the memset);
  light_set_dynamic (pointer in a local).
- snd: snd_init_sounds (pointer + count-down loop), snd_level_release (index and pointer together).
- ai: init and reset (loop instead of memset), can_be_knocked_down (one `||`), can_backstab_target (`d.dot(back)`).
- spells: spell_spawn_hit_fx (store order), cmd_summon (`pos = c->pos; pos += fvec * 4`), status_effect_duration
  and _modifier (every status a case, bodies in the binary's order; `(level * 4.0f) * 0.01f`).
- interface: interface_set_view_clip (all screens listed), interface_set_screen and interface_mouse_over_panel
  (folded callees named apart, D7903; `? true : false` result).
- ai_move: summon_emerge_update (case order), mode_hiding_frame (`t = 1` before the flag clear).

Also found: /Ow over all 92 vsdk units with /O2 gave +92 / -2 and over the game -332 (D7902; superseded by D7502).

## Still differs, and why (D7905)

- snd: the four volume clamps (block layout), calc_volume_3d / emitter volume and pan (constant kept in a frame
  slot), snd_play_3d, sounds_parse_table (registers).
- lights: create_spot (intensity store scheduled later), select_nearest, shade_point, shade_vertices,
  calc_point_specular_color (frame / registers), gr_env_map_uv (dead argument slot).
- quaternion: the pointer-matrix constructor, both multiply (quaternion::multiply now to +0xe8 with `w1 = w; w2 =
  q.w;` and a named cross product, kept), to_matrix(_b) (8966 canonicalises the product order), exp, the four
  cquat log / exp (FPU spills), cquat_squad (scheduling).
- ai: call_for_help, pick_attack_slot (sdot form kept: 4 bytes left, the second get_center's x sum order),
  claim_attack_slot (frame), offset_point_sideways, ai_update_perception, add_skill_from_behind, save, load,
  ai_do_frame (the loop form for ai_mode_frame_counts did not change it), cast_effect_track_add (register swap).
- spells: spell_target_value (body order and silence fall-through kept; the returns are tail-merged the other way),
  spell_effect_obj_set_timer (FPU copies), spell_fire_missile, spell_effect_alloc (register swap), the big ones.
- level_scripts_common: level_parse_table (scoped buffers kept: the frame size is now right, slots differ),
  level_trigger_point_left_of_line (frame 0x28), level_ambient_find_at (repeated loop test), trigger_trip,
  lc_entry_add, the parsers.
- interface: party_find_next/prev_member (EAX return), ui popups / message box (registers, layout).
- ai_move: steer_jitter (limit and decay before the profile label give the size; two temporaries' slots swapped),
  the big steering functions.

Markers that say exact but differ (pre-existing, left as they were): 0x47e4d0, 0x47e830, 0x4caed0, 0x4cd330,
0x4ce220, 0x413200, 0x413290, 0x4134b0, 0x413c70, 0x414110, 0x521680, 0x5216e0, 0x521760, 0x5218e0, 0x521a60.

## Logic errors / game bugs

None found in this pass (all changes are behaviour-identical). D7903 note: the 21 folded functions now named in
the interface unit have no definitions yet (a link would need empty / return-true bodies in their units).

## Header edits

- include/vsdk/gr/unnamed_lights_0x51e070.h: `light::enabled` `ushort : 1` -> `short : 1`; light_set_all_enabled
  and light_set_enabled take bool (D7904). Users rechecked: unnamed_vfx_emitter_0x55ff00.cpp identical with old /
  new header, interface unit unchanged; mingw over all 259 files: 0 errors.

## Tools (scratchpad/match20)

cmp.sh (vc6-compare with this tree), try.mjs (variant tester; MANG=<mangled prefix> prints byte-differing lines),
side2.mjs / sd2.sh (byte-level side by side), tri.mjs (triage), owsweep.mjs (flag sweep), ap.mjs, mark.sh.
