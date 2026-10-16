# VC6 match pass 5 (ten vsdk units)

Base: `snapC/src/decomp` (the repo's src/decomp at 0e15720). Compiler: VC6 Professional cl 12.00.8168 (`vc6p`),
per-unit flags from `src/decomp/vc6-flags.txt` (unchanged; all ten are /O2 /Ob0 /GX /MT), headers from
`match5/src/include`. `before/<unit>.txt` and `after/<unit>.txt` are the `vc6-compare -v` outputs.

| Unit | Before | After | Won |
|---|---|---|---|
| vsdk/gr_direct3d.cpp | 31 / 50 | 34 / 50 | set_default_transforms, begin_scene, end_scene |
| vsdk/matrix.cpp | 28 / 46 | 31 / 46 | rows_match, extract_angles, transpose_in_place |
| vsdk/unnamed_vector_0x523f30.cpp | 34 / 51 | 35 / 51 | vector3_near_equal |
| vsdk/unnamed_ix_0x53ac20.cpp | 18 / 35 | 19 / 35 | ix_sphere_polygon_plane |
| vsdk/unnamed_gr_3d_0x50d9b0.cpp | 22 / 39 | 28 / 39 | get_view_fov, fov_to_vertical, set_far_clip, check_normal_facing, plane_faces_viewer, float_key_less |
| vsdk/effect_mem.cpp | 18 / 39 | 23 / 39 | align_up_64, vfx_preload_by_name, alloc_stream_slot, find_stream_slot, hash_name |
| vsdk/unnamed_ca_mesh_0x50af80.cpp | 8 / 12 | 8 / 12 | (float fabs compares now dword, as in the binary) |
| vsdk/unnamed_vfx_0x536950.cpp | 13 / 28 | 15 / 28 | find_tag, get_time_left |
| vsdk/unnamed_vfx_channel_0x55b440.cpp | 11 / 19 | 12 / 19 | vfx_channel::clear |
| vsdk/unnamed_eax_0x521e40.cpp | 10 / 14 | 11 / 14 | eax_is_reverb_muted |
| **Total** | **193 / 333** | **216 / 333** | +23 |

No function lost its match (checked per address). Every matching function's comment says `Confidence: exact` (72
comments raised). No header changed. mingw (`i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Iinclude`): 0
`error:` lines for all ten files. Decisions D6300-D6306 are appended to `src/include/DECISIONS.md`.

## What worked (D6300)

- **`(float)fabs(x) < 0.0001f`**: the plain fabs compares in double (`fcomp QWORD`), fabsf is an out-of-line call
  under /Ob0. 4 matches (rows_match, near_equal, sphere_polygon_plane, extract_angles: `(float)fabs(a) >
  (float)fabs(b)` also gives the binary's `fxch / fxch / fcompp`). Applied to ca_mesh's two clip tests too.
- **Swap order**: the temporary takes the value the binary stores last (`t = uvec.x; uvec.x = rvec.y; rvec.y = t`).
- **Float locals that split constant folding** (`a = atan(..) * 2.0f; *h = a * RAD_TO_DEG`) and for converted
  ints (`float w = (float)clip_width` gives `fild / fdivp`); a local for a stored value reused (`d`, set_far_clip).
- **bool return shape**: float compare with `mov eax, 1 / xor eax, eax` is `return a < b;`; `test / setne al` on
  an int global is the if-form.
- **Locals**: global loop bounds read once (stream slots), declaration order (hash_name), a call argument
  that is a call into a local first (find_tag), a float argument stored to a slot (get_time_left), a pointer
  local to an array element (vfx_preload_by_name).
- **Element stores one per line in the binary's order** (set_default_transforms, D6301), a range test for the
  scene depth (D6302), memset for the 12-byte channel clear (D6303).

## What still differs and why (D6306)

The large majority is build 8966 against 8168, not the source:

- **Return / copy-initialisation elision** (about 25 functions): `return vector3(...)`, `return matrix(...)`,
  `vector3 v = a - b;`. 8966 builds the value in place; 8168 makes a temporary and calls the copy constructor
  (vfx_node::get_pos / get_orient, vfx_vec_channel / quat_channel::eval, vector3::normalized / normalized_or_x /
  rotated / unrotated / negated_b, vector3_rotate_towards_unit, matrix::operator* / multiply_by_transpose /
  multiply_transposed / transpose, matrix43::inverse, ix_ray_sphere, ix_sphere_boundingbox,
  ix_point_sphere_tangent, gr_get_normal_facing_dot, ...). Direct initialisation does not help.
- **Scheduling**: loads moved above stores and integer ops placed between `fcomp` and `fnstsw` (get_axis_angle,
  linesegment_boundingbox's out->z store, effect_base_set_info / finish_stream / release, get_tag_name /
  get_node_name, gr_d3d_init, gr_d3d_clear / zbuffer_clear, vfx_instance::stop, render_node's mesh case,
  setup_render_state's vi->time load, eax_save_props / eax_load_props' per-call stack pops).
- **Struct copies**: 9 / 3 dword copies load the last pair before storing (matrix43 constructors, get_rotation);
  element stores, memcpy and POD copies all differ.
- **Slot reuse**: 8966 puts locals in dead argument slots (seg2d_intersect, gr_setup_3d).
- **Term sorting**: matrix::determinant (8168 canonicalises the sum of products whatever the source order).
- **Tail merging**: matrix::orthogonalized / orthogonalize_b share one normalize() call between two branches.
- **Byte narrowing**: `and al, 1` for a bool from a one-bit field (effects_base_is_resident, is_cutscene_base).
- **Register allocation / loop shape**: vfx_channel::eval (8966 keeps cursor / size in memory),
  gr_get_frustum_planes (second loop registers; now within two instructions), effects_init_pools,
  vfx_instance_create / free, d3d_compute_pixel_format_shifts, gr_d3d_make_line_verts, the big render functions
  (set_state, bitmap, tmapper, the three render_character_mesh, init_device, ddraw_error_string), ix functions
  with large frames.
- **Switch / branch lowering**: gr_d3d_draw_primitive (8168 hoists the speculative lock-flag load),
  d3d_enumerate_ddraw_devices (block order of the shared NULL return), eax_init (the failure block placement:
  every if / else / nested / early-return form gives the same layout), gr_project_vertex (the final return
  reloads the flags byte in 8966).
- **Header-bound**: gr_light_desc::gr_light_desc (the binary constructs a vector3 at +0xc; gr.h splits that field
  into three; not changed), gr_cull_bounding_box_transformed (the binary has one constructor call where the source
  has a vector3 and a plane).

Partial progress kept (closer, still differing): eax_init (outer if, now 136 / 136 bytes), gr_get_frustum_planes,
ix_linesegment_boundingbox (582 bytes, same instruction count, D6305), render_node (D6304), setup_render_state.

## Logic errors and game bugs found

- **Sign of the vfx node index (D6304)**: render_node read vfx_node_ref::index as signed; the binary zero-extends
  it (as unnamed_render_scene_0x481680.cpp already casts it). Fixed in the reads; only an index >= 0x8000 would
  behave differently.
- **ix_linesegment_boundingbox aliasing (D6305)**: the old source re-read out->x after storing out->y; the binary
  computes the differences once. Same result unless out aliases a or b; the new source follows the binary.
- **gr_d3d_set_default_transforms (D6301)**: the projection uses the constant 1.0 (zfar / (zfar - 1)), not
  view._44; same value. The binary stores proj._44 = 0 after the memset (redundant, harmless).
- No new game bug. Known ones confirmed while reading: matrix::extract_angles' pitch multiplies where it should
  divide (latent, already in the comment), matrix43::transpose_in_place is not a transpose (unreferenced).
- Note: 26 functions were already marked `Confidence: exact` and still differ, all for the compiler reasons above
  (return elision, struct copies, scheduling); they were left as they are.

## Tools

`../m5tools/show.mjs <unit> <addr>...` prints our listing next to the original; `../m5tools/tryv.mjs <unit> <addr>
<marker> <variants> [keep]` compiles several versions of one function and keeps the first that matches.
