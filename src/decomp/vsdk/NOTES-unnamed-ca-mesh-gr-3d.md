# decomp65: the character mesh renderer and the 3D view state (0x50af80-0x50f9cf)

Two placeholder units, every function of the range written (111 of 111 sum.symbols entries; 86 exact, 21
likely, 4 unsure). Written from Sum.exe's disassembly (tools/asm.mjs) only; the Summoner 2 reference has no
unit with these functions, so it gave no names. Decisions D4500-D4507 (include/DECISIONS.md).

| File | Unit | Functions | Exact / likely / unsure |
|---|---|---|---|
| `vsdk/unnamed_ca_mesh_0x50af80.cpp` | character mesh renderer `0x50af80`-`0x50d9ab` (vsdk\ca) | 22 + 8 COMDATs | 25 / 2 / 3 |
| `vsdk/unnamed_gr_3d_0x50d9b0.cpp` | 3D view state `0x50d9b0`-`0x50f9cf` (vsdk\gr) | 61 + 20 COMDATs | 61 / 19 / 1 |

No file is named: neither unit has a path string, a strings.tbl block or a Summoner 2 counterpart, so
modules.txt is empty.

## Units and evidence

**Character mesh renderer, 0x50af80-0x50d9ab** (`unnamed_ca_mesh_0x50af80.cpp`, header
`vsdk/ca/unnamed_ca_mesh_0x50af80.h`, included by `vsdk/ca/character.h`).
- Starts with nine static initialisers, static_initializers table 0x588880-0x5888a0, directly after console.cpp's
  last entry; console.cpp's code ends with the LineFile deleting destructor COMDAT 0x50af60.
- Regular functions to ca_render_stub_ret 0x50d7d0, then the COMDAT block 0x50d7e0-0x50d9ab (vector3::cross,
  the five fog / depth helpers, float_max, clamp_float), all called from this object (D4500).
- .data 0x5a7280-0x5a7293 (gr_reflection_map_bm -1, "refmap01.tga"), after console.cpp's literals (ending
  with "line %d: '%s'" at 0x5a7270).
- .bss 0x25db930-0x2682997: nine vector3 buffers (2000 or 4000 entries), the Glide vertex array (4000 GrVertex),
  the OpenGL colour / texture coordinate arrays, the shared index list, the per-vertex done flags and colours,
  the current mesh / character / instance and the unread flags dword. Two spans have no reference (0x25f8e68,
  0x9c40 bytes; 0x26172c8, 0x3e80 bytes) and are defined as byte arrays sized from the gaps.
- gr_load_reflection_map (vsdk\gr in sum.symbols) is here because its data is.

**3D view state, 0x50d9b0-0x50f9cf** (`unnamed_gr_3d_0x50d9b0.cpp`, header `vsdk/gr/unnamed_gr_3d_0x50d9b0.h`,
included at the end of `vsdk/gr/gr.h`).
- Initialisers 0x50d9b0-0x50deb5 (table 0x5888a8-0x5888ec): the view / light / eye matrices and positions, the
  matrix scale, four console commands (far_min, far_max, zmult, zmax), the clip plane arrays, the instance stack,
  two unused 5-element arrays and the saved state object.
- Regular functions gr_setup_3d 0x50dec0 to gr_light_record::read 0x50f490, then the COMDATs 0x50f590-0x50f9cf.
- matrix.cpp begins at 0x50f9d0 with identity_matrix's initialiser (table 0x5888f0).
- .data 0x5a7294-0x5a747f: Gr_far_min 31.25, Gr_far_max 250, gr_depth_scale 0.0001, Gr_3d_zmult 0.2, Gr_3d_zmax
  0.998, gr_view_zfar 1000; then the literals. The four command objects' strings come first (0x5a72ac-0x5a730c),
  then the four functions' (0x5a7310-0x5a747f); "far_min" and the usage text appear once per use (no string
  pooling).
- .bss 0x2682998-0x2692e9f, matrix.cpp's identity_matrix at 0x2692ea0. It holds buffers other objects use:
  vfx_mesh_sort_buf / vfx_mesh_sort_count (vfx_mesh_render) and the clipper's vertex lists 0x26926f4 / 0x2692758
  (gr_clip_and_project_poly, gr_poly_project_to_screen).
- sort_by_float_key, float_key_less, matrix43_compose, gr_light_desc's two members and the light record's two
  members lie between gr_restore_3d_state and the COMDAT block, so they are this object's.

## Functions

### unnamed_ca_mesh_0x50af80.cpp

| Address | sum.symbols name | Confidence |
|---|---|---|
| `0x50af80` | init_ca_vertex_buf_266f108 | exact |
| `0x50afb0` | init_ca_vertex_buf_25e74b0 | exact |
| `0x50afe0` | init_ca_vertex_buf_2622e48 | exact |
| `0x50b010` | init_ca_vertex_buf_2605988 | exact |
| `0x50b040` | init_ca_vertex_buf_260b748 | exact |
| `0x50b070` | init_ca_vertex_buf_25db930 | exact |
| `0x50b0a0` | init_ca_vertex_buf_25ed2e8 | exact |
| `0x50b0d0` | init_ca_vertex_buf_2628c08 | exact |
| `0x50b100` | init_ca_vertex_buf_2675698 | exact |
| `0x50b130` | gr_load_reflection_map | exact |
| `0x50b150` | ca_render_morph_vertices | likely |
| `0x50b380` | init_ca_offset_table_25ed270 | exact |
| `0x50b450` | ca_ramp_curve_eval | exact |
| `0x50b4c0` | gr_glide_render_character_mesh | unsure |
| `0x50c130` | gr_d3d_render_character_mesh | unsure |
| `0x50cd00` | gr_opengl_render_character_mesh | unsure |
| `0x50d6c0` | ca_render_set_instance | exact |
| `0x50d6e0` | ca_render_set_mesh | exact |
| `0x50d6f0` | ca_render_mesh | exact |
| `0x50d7b0` | ca_render_has_mesh | exact |
| `0x50d7c0` | ca_render_set_flags | exact |
| `0x50d7d0` | ca_render_stub_ret | exact |
| `0x50d7e0` | vector3__cross | exact |
| `0x50d830` | gr_get_fog_alpha | exact |
| `0x50d880` | gr_calc_depth | likely |
| `0x50d8b0` | gr_vertex_depth | exact |
| `0x50d8d0` | gr_get_point_fog_value | exact |
| `0x50d910` | gr_get_fog_value | exact |
| `0x50d960` | float_max | exact |
| `0x50d980` | clamp_float | exact |

### unnamed_gr_3d_0x50d9b0.cpp

| Address | sum.symbols name | Confidence |
|---|---|---|
| `0x50d9b0` | init_gr_view_matrix | exact |
| `0x50d9d0` | init_gr_view_pos | exact |
| `0x50d9f0` | init_gr_light_matrix | exact |
| `0x50da10` | init_gr_light_base | exact |
| `0x50da30` | init_gr_eye_matrix | exact |
| `0x50da50` | init_gr_eye_pos | exact |
| `0x50da70` | init_gr_matrix_scale | exact |
| `0x50da90` | init_ccmd_far_min_thunk | exact |
| `0x50daa0` | init_ccmd_far_min | exact |
| `0x50dac0` | cmd_far_min | exact |
| `0x50db50` | init_ccmd_far_max_thunk | exact |
| `0x50db60` | init_ccmd_far_max | exact |
| `0x50db80` | cmd_far_max | exact |
| `0x50dc10` | init_gr_user_clip_planes | exact |
| `0x50dc40` | init_ccmd_zmult_thunk | exact |
| `0x50dc50` | init_ccmd_zmult | exact |
| `0x50dc70` | cmd_zmult | exact |
| `0x50dd00` | init_ccmd_zmax_thunk | exact |
| `0x50dd10` | init_ccmd_zmax | exact |
| `0x50dd30` | cmd_zmax | exact |
| `0x50ddc0` | init_gr_instance_stack | exact |
| `0x50ddf0` | init_gr_vertex_buf_2692e28 | likely |
| `0x50de20` | init_gr_vertex_buf_2692830 | likely |
| `0x50de50` | init_gr_frustum_planes | exact |
| `0x50de80` | init_gr_clip_plane_26924b8 | exact |
| `0x50dea0` | init_gr_3d_state_saved | exact |
| `0x50dec0` | gr_setup_3d | likely |
| `0x50e150` | gr_setup_frustum | likely |
| `0x50e5a0` | gr_get_view_fov | exact |
| `0x50e5e0` | gr_fov_to_vertical | likely |
| `0x50e640` | gr_start_instance | exact |
| `0x50e750` | gr_stop_instance | exact |
| `0x50e7c0` | gr_set_far_clip | exact |
| `0x50e810` | gr_code_point | exact |
| `0x50e890` | gr_code_vertex | exact |
| `0x50e910` | gr_rotate_vertex | exact |
| `0x50e960` | gr_rotate_delta_vec | exact |
| `0x50e990` | gr_rotate_vertices | exact |
| `0x50ea00` | gr_offset_point_depth | likely |
| `0x50ea50` | gr_project_vertex | exact |
| `0x50eb30` | gr_check_normal_facing | exact |
| `0x50eb70` | gr_plane_faces_viewer | exact |
| `0x50eba0` | gr_get_normal_facing_dot | exact |
| `0x50ebe0` | gr_screen_to_world_ray | likely |
| `0x50ec90` | gr_project_size | exact |
| `0x50ecb0` | gr_scale_by_far_clip | exact |
| `0x50ecc0` | gr_vector_scaled_z | exact |
| `0x50ecd0` | gr_cull_sphere | likely |
| `0x50ed60` | gr_get_frustum_planes | exact |
| `0x50edc0` | gr_cull_bounding_box | likely |
| `0x50ee70` | gr_cull_bounding_box_transformed | likely |
| `0x50ef30` | gr_get_eye | exact |
| `0x50ef60` | gr_save_3d_state | likely |
| `0x50f120` | gr_restore_3d_state | likely |
| `0x50f2e0` | sort_by_float_key | likely |
| `0x50f380` | float_key_less | exact |
| `0x50f3a0` | matrix43_compose | exact |
| `0x50f3e0` | gr_light_desc__gr_light_desc | likely |
| `0x50f400` | gr_light_desc__reset | exact |
| `0x50f420` | gr_light__reset | exact |
| `0x50f490` | gr_light__read | exact |
| `0x50f590` | gr_clip_plane__gr_clip_plane | exact |
| `0x50f5a0` | gr_instance_entry__gr_instance_entry | exact |
| `0x50f5d0` | gr_3d_state__gr_3d_state | exact |
| `0x50f660` | matrix__matrix_default | exact |
| `0x50f680` | gr_setup_3d_free_clip_verts | likely |
| `0x50f690` | gr_clip_free_temp_verts | exact |
| `0x50f6a0` | vector3__scale | exact |
| `0x50f6c0` | gr_clip_plane__set_from_points | likely |
| `0x50f710` | gr_clip_plane__set_from_normal | likely |
| `0x50f760` | Plane__assign | unsure |
| `0x50f790` | vector3__scaled | exact |
| `0x50f7d0` | Plane__Plane_points | exact |
| `0x50f800` | Plane__set_from_points | exact |
| `0x50f840` | vector3__sub_assign | exact |
| `0x50f860` | Plane__distance_to_point | exact |
| `0x50f880` | Plane__transform | likely |
| `0x50f900` | gr_clip_plane__assign | likely |
| `0x50f930` | gr_instance_entry__assign | exact |
| `0x50f990` | quaternion__quaternion | exact |
| `0x50f9b0` | quaternion__assign | exact |

## Source-shape notes and open questions

- **The three renderers (unsure).** One function per back end, each about 3 KB of optimised FPU code: per
  sub-mesh the texture, the optional morph, a vertex loop (seam copies, four-bone skinning, model-space copy, view
  rotation and projection written out per component, a bounds test, lighting through lights_shade_vertices, the
  tint), then the triangle pass(es). The arithmetic, tests and stores follow the disassembly; how the source was
  split into statements is a guess (the compiler reorders the component expressions differently in each back
  end). The Glide back end re-reads `mesh->submeshes[i]` after every call (no sub-mesh pointer), the Direct3D and
  OpenGL ones keep one (the OpenGL one still passes `&mesh->submeshes[i]` to the morph). The Glide triangle pass
  appears three times (normal, additive tint pass, environment pass); written out three times.
- **COMDAT order.** Neither block is in the order of first use or of header order (D4500).
- **Two plane assignments** (0x50f760 here, 0x4c0a20 in the projector unit) and the constructor-less plane local of
  gr_cull_bounding_box_transformed: D4502.
- **do-while culling loops.** gr_cull_sphere and gr_cull_bounding_box test plane 0 before comparing the index with
  the count; written as do-while (likely). gr_cull_bounding_box_transformed has a normal for loop.
- **gr_light_desc constructor** calls vector3() on +0x0c, which the header has no member for (D4506); the call is
  not reproduced (likely).
- **gr_setup_3d's fov argument.** Below 2.0 it is used as the tangent of the half angle (the slot is reused for
  the tangent, hence a self-copy in the binary); otherwise degrees. gr_fov_to_vertical treats it the same way.
- **Unused data.** ca_vertex_buf_2605988 / _25db930 / _25ed2e8 / _2628c08 / _2675698, the two gap spans of the mesh
  unit, gr_plane_buf_2692e28 / _2692830 (16-byte objects with the folded "vector3 at +0" constructor; type guessed
  as plane), gr_user_clip_planes (only saved and restored), gr_projection_xmul_copy (written, saved, never read),
  ca_render_flags (written only).
- **The unread arguments**: the renderers' fifth, seventh and eighth (1, -1, -1 from ca_render_mesh),
  gr_setup_3d's fifth, lights_shade_vertices' eighth (the float 255.0 pushed as a dword).

## Doc disagreements (to check)

- docs/sum-exe/game-bugs.md K51: "(`ca_render_mesh`, which clamps first at `0x50d3a1`..`0x50d3c3`)": that clamp is in
  gr_opengl_render_character_mesh (0x50cd00); ca_render_mesh (0x50d6f0) only applies the point fog to the light
  record's alpha and dispatches. The character renderers also scale the sum by 0.75 before the clamp (below).
- sum.symbols gr_check_normal_facing / gr_get_normal_facing_dot "(pos, normal)": the arguments are (normal, point)
  (D4504); both callers' conventions agree with that.
- sum.symbols gr_frustum_near_plane: not a near plane. It is set with the far clip code (1) at
  far / Gr_level_fog_multiplier along the eye direction, and gr_cull_sphere uses it in place of the far plane.
- sum.symbols gr_scale_by_far_clip "f * 0x2692e80 (far clip scale)": 0x2692e80 is gr_matrix_scale.z.
- sum.symbols gr_light__reset / gr_light__read: the record is not gr_light (D4503).
- sum.symbols init_gr_vertex_buf_2692e28 / _2692830 "5 gr__Vertex": 16-byte elements; the constructor is only the
  folded copy sum.symbols names gr__Vertex__Vertex (a gr_vertex is 0x30 bytes).
- sum.symbols ca_glide_mesh_verts "float[]" is GrVertex[4000]; ca_gl_mesh_colors "uint32[]" is four bytes per
  vertex written one by one; ca_skin_normals' "accumulated with the positions" includes the bone translation.
- include/vsdk/ca/character.h ca_vertex_weights: bones at +0, weights at +4; the renderers read the weights at
  +0..+3 and the bone indices at +4..+7 (D4506).
- include/vsdk/gr/gr.h gr_light_desc +0x1c "character_hero_model_prefix": the renderers use it as an int
  (`<< 6`, a mask against the triangle group bits).
- docs/sum-exe/vsdk-gr.md and vsdk-ca.md do not describe either unit yet (the 3D pipeline, the instance stack,
  frustum culling, the mesh renderers).

## Game bugs (to check)

- **Skinned normals include the bone translation.** All three renderers transform the normal with the full skin
  matrix (rotation and translation) and normalise the weighted sum, so a skinned character's lighting normals lean
  towards each bone's model-space position instead of being rotated only (Glide 0x50b7e4-0x50b859, Direct3D
  0x50c514-0x50c58d, OpenGL 0x50d059-0x50d0e6). Characters drawn without an instance (no skinning) are lit with
  the mesh normals. Likely visible as lighting that depends on where a vertex sits rather than how it faces.
- **Direct3D seam vertices fog by the normal.** For a vertex that copies an earlier one (a uv seam),
  gr_d3d_render_character_mesh takes the fog alpha from ca_skin_normals[j - d].z (0x50c37f-0x50c39d) where the
  copied vertex used its view-space depth; that value is small (a normal, plus the bone translation of the first bug), so seam vertices mostly keep alpha 255 while their neighbours fade.
  Glide copies the whole vertex and OpenGL draws without per-vertex fog alpha, so only Direct3D (the retail
  default) is affected.
- **Tint grey 0.75 x the sum.** With gr_tint_enabled all three renderers replace the lit colour by
  tint * clamp((r + g + b) * 0.75, 0, 255) (0x580e04); any vertex brighter than a third of full white saturates.
  If an average was meant, the factor would be 1/3.
- **gr_cull_sphere's fog plane.** Every caller passes ignore_far 0, and with ignore_far 0 gr_cull_sphere tests
  gr_frustum_near_plane even when far clipping is off; that plane is only updated by gr_setup_frustum while far
  clipping is on, so in a frame without far clipping it holds the plane of the last fogged view (or the zero plane
  before the first one, which culls nothing). Whether a level without fog follows a fogged one with this effect
  needs a run.
- **Glide/D3D/OpenGL bounds differ.** A vertex is dropped (its triangles skipped) at |sx| >= 1224 or |sy| >= 1000 on
  Glide and at 2048 on Direct3D; OpenGL tests only the depth. Not a bug as such, but large or close-up characters
  lose triangles on Glide first.

## Headers (see CHANGED-HEADERS.txt)

New: vsdk/ca/unnamed_ca_mesh_0x50af80.h, vsdk/gr/unnamed_gr_3d_0x50d9b0.h, vsdk/math/bezier.h. Changed: gr.h,
vector.h, ix.h, character.h, vfx.h, d3d_min.h, gl_min.h, README.md, DECISIONS.md. The copy was taken from the
working tree at 00017da (with the uncommitted renames in DECISIONS.md, unnamed_lights_0x51e070.h, vector.h,
ui_gadgets.h that were present at the time); every src/decomp .cpp compiles against it.

## Left in the range

Nothing: all 111 functions of 0x50af80-0x50f9b0 are written. Not written and only declared: the bezier members
(0x558c00, 0x558c80, 0x558d90), bbox_get_corner (0x559960), bbox_corner_indices_from_normal (0x559a80),
gr_poly_reset_clip_verts (0x545120) and the vfx render state globals.
