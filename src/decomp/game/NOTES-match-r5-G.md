# match17: VC6 12.00.8966 byte matching (ten units)

Compiler: cl 12.00.8966 (`scratchpad\vc6sp5\VC98`), per-unit flags from `src/decomp/vc6-flags.txt`. Sources copied
from `snapF\src\decomp`. "Before" = snapF sources and headers under 8966; "after" = this tree. Checked address by
address: no match lost in any file. Decisions D7600-D7604 (appended to `src/include/DECISIONS.md` here).

| file | before | after | gained |
|---|---|---|---|
| game/boss_ai.cpp | 45 / 58 | 49 / 58 | boss_cast_special_dispatch, boss_attack_stuns_dispatch, boss_attack_knocks_down_dispatch, boss_stage_attack_enabled_dispatch |
| game/characterinfo.cpp | 53 / 70 | 55 / 70 | move_skip_on_level, character_get_chain_skill |
| vsdk/effect_mem.cpp | 23 / 39 | 25 / 39 | effects_base_is_resident, effect_is_cutscene_base |
| vsdk/matrix.cpp | 35 / 46 | 38 / 46 | matrix::orthogonalized, matrix::transpose, matrix::orthogonalize_b |
| vsdk/skeleton.cpp | 17 / 28 | 19 / 28 (22 with /Ox /Ob0) | skeleton::get_root_velocity, get_root_rotation (+ vim_write_cache, vim_load, vim_reload_bitmaps with the flags below) |
| vsdk/unnamed_vector_0x523f30.cpp | 38 / 51 | 41 / 51 | vector3::rotated, project_onto_plane, vector3_rotate_towards_unit |
| vsdk/unnamed_vfx_0x536950.cpp | 15 / 28 | 20 / 28 | release_nodes, get_node_name, get_tag_name, bind_node, node_can_bind |
| vsdk/gr_glide.cpp | 44 / 61 | 44 / 61 | - |
| vsdk/gr_direct3d.cpp | 34 / 50 | 34 / 50 | - |
| vsdk/unnamed_ds_0x55bf90.cpp | 43 / 60 | 43 / 60 | - |
| **total** | **347 / 491** | **368 / 491** (371 with the flags line) | +21 (+24) |

## Flags (proposal, vc6-flags.txt not edited)

`vsdk/skeleton.cpp /Ox /Ob0 /GX /MT` (was /O2 /Ob0 /GX /MT): 19 -> 22 under 8966, nothing lost (vim_write_cache,
vim_load, vim_reload_bitmaps: the "" initialisers of the name buffers are separate unpooled literals, D6602).
Other flag sets for the other nine files gain nothing (characterinfo, boss_ai, effect_mem lose 1-2 with /Ox; the
/Ob0 units lose 12-28 with inlining).

## What worked (D7601)

- **Boss switch tables** (boss_ai): the binary's byte tables give index 0 to every other boss, so those bosses are
  explicit cases with their own `return false;` (or `return true;`); with `break` to the shared default 8966 drops
  them and compiles two compares.
- **Case order = code order** (vfx): MESH, LIGHT, EMITTER(_PART), VIF, DUMMY in all four tag switches.
- **One-bit bool results**: `bool r = bitfield; return r;` gives `and al, 1` (D6306 (7) is reproducible under
  8966). The dummy's bit 15 needed a real 16-bit bitfield (header change D7600).
- **Dot-product term order**: 8966 canonicalises a hand-written sum of products completely for a given operand
  shape; binding the matrix rows to references gives the inlined-dot order (vector3::rotated). Not reachable for
  unrotated or determinant.
- **Branch layout / store order**: matrix orthogonalize(d) test `flen <= 0` first; transpose stores u, r, f;
  bind_node stores bound, ci, tag (the bitfield word is loaded before the ci store).
- **Temporaries and returns**: the rotate origin is an unnamed temporary (`&(const vector3 &)vector3(0, 0, 0)`);
  project_onto_plane negates d in place; get_root_velocity returns a named local; get_root_rotation needs the
  frame count in a float local so 1/160 and 1/30 stay two multiplies.
- release_nodes: the vif list walk with `rec = next; next = next->next;`.
- move_skip_on_level: `if (m->id == 0x25 || m->id == 0x26) return 1;` (the last two tests merged) avoids 8966's
  `sete` for the final compare.
- character_get_chain_skill: `char_infos[p->char_info].field` directly (absolute addressing per access).

## Still differs, and why (D7602, D7603)

- matrix43 constructors (3) and get_rotation: 8966 loads the last two dwords before storing; no source form
  tried gives it (element stores, POD copies of 3 / 9 dwords, float locals, pointers, loops).
- matrix::determinant: term and factor order canonicalised; 36 uniform orders, 729 groupings, reference /
  pointer / array forms tried.
- matrix::get_axis_angle, vector3::get_major_axis: integer stores scheduled between fcomp and fnstsw.
- vector3::unrotated, random_unit_vector / random_direction(_in_cone), length_quick (8966 folds `t + t * 0.5f`),
  angle_and_axis, vector2 functions: FPU scheduling / slot reuse.
- boss_ai_unload: register choice explained by ECX being live (the unload callees are probably ai_state members,
  D7603 (1)); boss_dismount_dispatch: three folded callees (D7603 (2)); boss_move_in_range_dispatch: needs
  tiger_move_in_range / giant_salamanka_target_in_reach as bool (verified harmless in a scratch tree, not applied:
  other units' files) and still differs in the Luminar case; boss_threat_add / threat_find / rider_unlink_mount:
  scheduling and register choice.
- characterinfo_find_multi_slot: 64 / 64 instructions, only the two return blocks are in the other order (logic
  fix D7604); str_hash_nocase_rol6: ESI / EDI swapped; the long loaders: structure differences not attempted.
- effect_mem: vfx_load's flags expression reads base->flags as a signed 13-bit bitfield (shl/sar by 3) - a header
  question left open; others are scheduling.
- vfx: stop (one `test` register), setup_render_state (a load scheduled above a byte store), process / init_*.
- gr_glide: glide_load_dll is one instruction pair off (the handle copied to ESI after the first push; local
  handle forms get 462 / 462 instructions but not the order); gr_glide_tcache_free_entry gets its branch
  polarity with `if (num_sections != 0) { ... } else { ... }` but the inner loop still reloads `sections`.
- gr_direct3d: gr_d3d_tmapper gets closer with `float w = v->sw;` (the fst/fstp pair) but z is loaded before the
  depth store in the binary; draw_primitive's conditional lock flags are merged by 8966 in every form tried.
- unnamed_ds: ds_channel_release's do-while reload, ds_stream_read_chunk's branch polarity: forms tried
  (while / for / positive test) give no change.

## Logic errors / game bugs

- D7604: characterinfo_find_multi_slot - the binary loops on DebugError for ever when all four multiplayer slots
  are used (0x449ac5-0x449adc); the decomp returned one past the slot table. Fixed in the source; the hang is a
  latent game bug.

## Header changes

- include/vsdk/geom/vfx.h: vfx_dummy_desc +0x0c `ushort flags` becomes a union with `num_frames : 15` /
  `can_bind : 1` (D7600). All 259 .cpp pass mingw (0 errors); 16 other units including vfx.h compared before /
  after under 8966 with identical results.
