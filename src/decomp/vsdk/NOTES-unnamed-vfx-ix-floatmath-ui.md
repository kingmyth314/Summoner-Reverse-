# decomp67: the vfx object, the ix unit, the float helpers and ui_element

Written from Sum.exe v1.40's disassembly (`tools/asm.mjs`), VC++ 6.0 target, not compiled with VC6. Dash
Faction (rf/math/ix.h) and Summoner 2 gave names only; no code was taken from either. Header copy based on
commit 8e27f5b (an earlier copy at 00017da was rebased during the work; see CHANGED-HEADERS.txt).
Decisions D4700-D4713 are appended to `include/DECISIONS.md`.

144 of the 145 functions in the three ranges are written (91 exact / 52 likely / 1 unsure). Left:
`0x543030` (sum.symbols ui_stub_ret_543030, a bare `ret` with no reference; it may close the ui_element
object or open skeleton.cpp, so it is not attributed).

## Units and evidence

| File | Range | Evidence |
|---|---|---|
| `vsdk/unnamed_vfx_0x536950.cpp` | `0x536950`-`0x538647` | Initialiser table `0x5889d0`-`0x5889d8` between the Direct3D state file (`0x5889cc`) and gr_direct3d.cpp (`0x5889dc`); .data `0x5a9cfc`-`0x5a9d23` (vfx_render_bitmap -1, "$vis", ">>> FAILED TO GET TAG %s\n"); .bss `0x2d90ee8`-`0x2dbfd1b`; closing COMDATs `0x538590`-`0x538647` (D1601, source-map "gr_direct3d.cpp is two objects"). No path string. |
| `vsdk/unnamed_ix_0x53ac20.cpp` | `0x53ac20`-`0x53debc` | After gr_direct3d.cpp's IsEqualGUID COMDAT `0x53ac00`, before cmdline.cpp's initialiser group `0x53dec0`; .data `0x5abe9c`-`0x5abf05` (axis pairs, four profile labels; cmdline.cpp's " " at `0x5abf08`), .bss `0x2dc6cc0`-`0x2dc6ea7` (statics and guards; cmdline_args `0x2dc6ea8` follows); closing COMDATs `0x53de00`-`0x53debc`. Only strings: the profile labels. |
| `vsdk/unnamed_floatmath_0x542500.cpp` | `0x542500`-`0x542cc2` | After the exception handler unit (strings to `0x5ac8bd`); .data `0x5ac8c8`-`0x5ac8d9`, .bss `0x2ee2518`-`0x2ee4523` (skeleton.cpp from `0x2ee4524`); the two unreferenced float_wrapper members close it (D4712). |
| `vsdk/unnamed_ui_element_0x542cd0.cpp` | `0x542cd0`-`0x54302c` | vtable `0x581ea0`; .data `0x5ac8dc` (the auto-id counter; skeleton.cpp's "vim" at `0x5ac8e0`); EH funclet `0x57d61e`; closed by ui_tooltip's scalar deleting destructor COMDAT `0x543010` (D4712). skeleton.cpp starts at `0x543040`. |

No file is named: none of the four has a path string or strings.tbl block, and the Summoner 2 splits have no
unit with these functions. `modules.txt` is therefore empty.

## Functions

### unnamed_vfx_0x536950.cpp (38: 16 / 22 / 0)

| Address | sum.symbols | Confidence |
|---|---|---|
| `0x536950` | init_gr_d3d_vertex_buf | likely |
| `0x536980` | init_vfx_camera_pos | exact |
| `0x5369a0` | init_vfx_camera_orient | exact |
| `0x5369c0` | vfx_instance__clear | likely |
| `0x5369f0` | vfx_instance_release_nodes | likely |
| `0x536a70` | vfx_instance_init_nodes | likely |
| `0x536bc0` | vfx_node_relocate | likely |
| `0x536e50` | vfx_base__load_bitmaps | exact |
| `0x536ed0` | vfx_base__init_vis_flags | likely |
| `0x536fb0` | vfx_instance_process | likely |
| `0x537230` | vfx_instance__stop | exact |
| `0x537270` | vfx_instance__is_stopped | exact |
| `0x537280` | vfx_instance__set_stopped | likely |
| `0x5372a0` | vfx_base__decode_tag | likely |
| `0x5373a0` | vfx_instance__get_node_name | likely |
| `0x537490` | vfx_instance__get_tag_name | likely |
| `0x537580` | vfx_instance__bind_node | likely |
| `0x537700` | vfx_instance__node_can_bind | likely |
| `0x537800` | vfx_instance__reset_flags | likely |
| `0x537840` | vfx_instance__get_node_transform | likely |
| `0x537a10` | vfx_instance__set_node_transform | likely |
| `0x537f10` | vfx_instance__find_tag | exact |
| `0x537f70` | vfx_instance__get_tag_transform | likely |
| `0x538260` | vfx_instance_render | likely |
| `0x538340` | vfx_base__get_num_tags | likely |
| `0x538390` | vfx_base__page_in_particle_bitmaps | exact |
| `0x5383e0` | vfx_get_camera | exact |
| `0x538430` | vfx_instance__setup_render_state | likely |
| `0x5384f0` | vfx_render_state_reset | exact |
| `0x538510` | vfx_instance__get_time_left | likely |
| `0x538570` | vfx_instance__ctor | likely |
| `0x538590` | vfx_instance__get_base | exact |
| `0x5385a0` | vfx_get_word_72 | exact |
| `0x5385b0` | vfx_instance__list18_item | exact |
| `0x5385d0` | vfx_instance__list1c_item | exact |
| `0x5385f0` | vfx_instance__list20_item | exact |
| `0x538610` | vfx_instance__list24_item | exact |
| `0x538630` | vfx_instance__list28_item | exact |

### unnamed_ix_0x53ac20.cpp (69: 49 / 19 / 1)

| Address | sum.symbols | Confidence |
|---|---|---|
| `0x53ac20` | ix_ray_polygon | likely |
| `0x53ad30` | atexit_nop_53ad30 | exact |
| `0x53ad40` | ix_linesegment_plane_t | likely |
| `0x53adf0` | ix_ray_plane_point | exact |
| `0x53ae60` | ix_linesegment_plane_front_t | likely |
| `0x53aec0` | ix_linesegment_triangle_front | exact |
| `0x53af40` | ix_ray_parallelogram | likely |
| `0x53b130` | ix_ray_quad | likely |
| `0x53b3e0` | atexit_nop_53b3e0 | exact |
| `0x53b3f0` | atexit_nop_53b3f0 | exact |
| `0x53b400` | ix_linesegment_sphere | likely |
| `0x53b6b0` | atexit_nop_53b6b0 | exact |
| `0x53b6c0` | atexit_nop_53b6c0 | exact |
| `0x53b6d0` | atexit_nop_53b6d0 | exact |
| `0x53b6e0` | atexit_nop_53b6e0 | exact |
| `0x53b6f0` | ix_point_in_polygon | likely |
| `0x53b8f0` | ix_sphere_sphere | exact |
| `0x53b930` | ix_sphere_polygon_plane | likely |
| `0x53bac0` | atexit_nop_53bac0 | exact |
| `0x53bad0` | ix_sphere_plane_front | exact |
| `0x53bb50` | ix_sphere_polygon_edge | unsure |
| `0x53bf40` | atexit_nop_53bf40 | exact |
| `0x53bf50` | atexit_nop_53bf50 | exact |
| `0x53bf60` | ix_sphere_polygon | likely |
| `0x53c1b0` | atexit_nop_53c1b0 | exact |
| `0x53c1c0` | atexit_nop_53c1c0 | exact |
| `0x53c1d0` | atexit_nop_53c1d0 | exact |
| `0x53c1e0` | atexit_nop_53c1e0 | exact |
| `0x53c1f0` | atexit_nop_53c1f0 | exact |
| `0x53c200` | ix_boundingbox_boundingbox | exact |
| `0x53c260` | ix_point_boundingbox | exact |
| `0x53c2c0` | ix_point_box_oriented | exact |
| `0x53c3e0` | atexit_nop_53c3e0 | exact |
| `0x53c3f0` | atexit_nop_53c3f0 | exact |
| `0x53c400` | atexit_nop_53c400 | exact |
| `0x53c410` | ix_sphere_boundingbox | exact |
| `0x53c4a0` | ix_sphere_box_oriented | exact |
| `0x53c5e0` | atexit_nop_53c5e0 | exact |
| `0x53c5f0` | atexit_nop_53c5f0 | exact |
| `0x53c600` | atexit_nop_53c600 | exact |
| `0x53c610` | ix_box_oriented_box_oriented | likely |
| `0x53cb60` | ix_line_line | likely |
| `0x53cd50` | atexit_nop_53cd50 | exact |
| `0x53cd60` | ix_ray_plane_point_b | exact |
| `0x53cde0` | ix_ray_plane_t | exact |
| `0x53ce30` | ix_ray_plane | exact |
| `0x53ced0` | ix_linesegment_box_oriented | exact |
| `0x53d0c0` | atexit_nop_53d0c0 | exact |
| `0x53d0d0` | atexit_nop_53d0d0 | exact |
| `0x53d0e0` | atexit_nop_53d0e0 | exact |
| `0x53d0f0` | atexit_nop_53d0f0 | exact |
| `0x53d100` | ix_ray_box_oriented | exact |
| `0x53d2a0` | atexit_nop_53d2a0 | exact |
| `0x53d2b0` | atexit_nop_53d2b0 | exact |
| `0x53d2c0` | atexit_nop_53d2c0 | exact |
| `0x53d2d0` | ix_ray_boundingbox | likely |
| `0x53d4d0` | ix_linesegment_boundingbox | likely |
| `0x53d720` | ix_slab_clip | exact |
| `0x53d7a0` | ix_ray_sphere | likely |
| `0x53d8f0` | seg2d_intersect | likely |
| `0x53d9d0` | ix_ray_closest_point_t | exact |
| `0x53da70` | atexit_nop_53da70 | exact |
| `0x53da80` | ix_point_linesegment | exact |
| `0x53dbb0` | ix_linesegment_sphere_chord_length | likely |
| `0x53dcd0` | ix_point_sphere_tangent | likely |
| `0x53de00` | vector3__plus_scalar | likely |
| `0x53de40` | vector3__minus_scalar | exact |
| `0x53de80` | vector3__add_scalar_assign | likely |
| `0x53dea0` | vector3__sub_scalar_assign | exact |

### unnamed_floatmath_0x542500.cpp (20: 11 / 9 / 0)

| Address | sum.symbols | Confidence |
|---|---|---|
| `0x542500` | fast_inv_sqrt | likely |
| `0x542540` | fast_sqrt_refined | likely |
| `0x542590` | fast_sqrt | likely |
| `0x5425d0` | float_to_int_round | exact |
| `0x5425f0` | fast_sqrt_tables_init | likely |
| `0x5426e0` | frand | exact |
| `0x542700` | frand_chance_in_n_tries | likely |
| `0x542770` | frand_range | exact |
| `0x542790` | atan2_fast | likely |
| `0x542800` | sqrt_abs | exact |
| `0x542830` | float_format_buf | likely |
| `0x542b50` | cross_2d | exact |
| `0x542b70` | float_ptr_near_value | exact |
| `0x542ba0` | float_near_ptr_value | exact |
| `0x542bd0` | float_ptrs_near_equal | exact |
| `0x542c10` | float_ptr_not_near_equal_val | exact |
| `0x542c40` | float_val_not_near_equal_ptr | exact |
| `0x542c70` | float_ptrs_not_near_equal | exact |
| `0x542cb0` | float_wrapper__set | likely |
| `0x542cc0` | float_wrapper__get | likely |

### unnamed_ui_element_0x542cd0.cpp (17: 15 / 2 / 0)

| Address | sum.symbols | Confidence |
|---|---|---|
| `0x542cd0` | ui_element__free_tooltips | likely |
| `0x542d20` | ui_element__ui_element | exact |
| `0x542d40` | ui_element__dtor | exact |
| `0x542da0` | ui_element__init | likely |
| `0x542e10` | ui_element__get_kind | exact |
| `0x542e20` | ui_element__set_position | exact |
| `0x542e40` | ui_element__get_position | exact |
| `0x542e60` | ui_element__set_size | exact |
| `0x542e80` | ui_element__get_size | exact |
| `0x542ea0` | ui_element__set_hidden | exact |
| `0x542ed0` | ui_element__set_visible | exact |
| `0x542f10` | ui_element__set_disabled | exact |
| `0x542f70` | ui_element__set_enabled | exact |
| `0x542fe0` | ui_element__is_hidden | exact |
| `0x542ff0` | ui_element__is_shown | exact |
| `0x543000` | ui_element__is_disabled | exact |
| `0x543010` | ui_tooltip__scalar_deleting_dtor | exact |

## What the code does (short)

- **vfx object.** vfx_base: relocation of a loaded effect (names against the header base, tables against the
  base, then every record kind; counts capped at 300 meshes, 15 lights, 90 emitters, 50 dummies, 100 forces),
  bitmap loading, the `$vis` default visibility (init_vis_flags, no caller), the flat tag index (decode_tag:
  meshes, emitter sub-tags, vif records, lights, dummies, emitters; get_num_tags). vfx_instance: the per-frame
  update (process: time = elapsed * 15, looping or stopping at the last frame, lights / dummies / effect camera
  while running, the first live emitter and the vif records once stopped), drawing one draw-list entry
  (render_node: mesh, or emitter update + draw), the tag functions (names, binding to a model tag, local and
  world placement with the children following, hiding with the children following, find_tag), the remaining
  time, the effect camera copy-out (vfx_get_camera), the character render state (setup_render_state / reset).
- **ix unit.** Segment / ray against planes, triangles, a parallelogram, a quad, spheres, boxes (axis-aligned:
  Woo's test and an outcode clip; oriented: through the box frame), a 15-axis OBB test, a plane-plane line,
  the swept-sphere polygon test (plane, edge cylinders, vertex spheres), 2D segment crossing, closest points.
  Most temporaries are function-local statics (D4705).
- **float helpers.** Table square roots (tables built once; nothing calls them), frand / frand_range /
  frand_chance_in_n_tries, atan2_fast, sqrt_abs, a float-to-text formatter, cross_2d, the 1e-4 comparisons,
  float_wrapper. Only frand, frand_range, atan2_fast and cross_2d have callers.
- **ui_element.** The base of every UI widget: tooltip list (its sentinel is a whole ui_tooltip, D4707),
  init with automatic ids counting down from -1, position / size, hidden / disabled with the focus and the
  disable / enable events (4, 9, 10).

## Open questions

- D4705: the atexit registrations of the ix statics need a vector3 destructor that vector.h does not have.
- D4703 / D1605: gr_vertex's default constructor (gr_vertex_pool's initialiser).
- D4704: vif_emitter_update's last parameter is the float elapsed time (declared int upstream).
- D4706: "" literals in .bss (here) against cmdline.cpp's assumption that VC6 puts them in .data.
- ix_sphere_polygon_edge's branch shape around the early entry times (the only "unsure").
- ix_box_oriented_box_oriented tests 3, 5, 8: the binary adds two Bf terms and multiplies once; whether the
  source was written that way or VC6 factored `b[2] * x + b[2] * y` is not known (likely-level elsewhere).
- 0x543030: owner unit.
- Commutative operand order (s + x vs x + s) in the scalar operators and a few sums: VC6's choice not verifiable
  without the compiler.

## Doc / symbol disagreements

- sum.symbols `vfx_instance__ctor` (0x538570) and `vfx_instance__clear` (0x5369c0) are vfx_base's constructor
  and clear (D4701).
- sum.symbols `vfx_instance__get_tag_transform` (0x537f70) does not compute a transform: it hides / shows a tag
  and, recursively, its children (header comment "result unused" was right: there is none).
- sum.symbols `vfx_instance__setup_render_state` takes (out byte, instance), not (instance) (decomp63 had it right).
- sum.symbols `vfx_get_word_72` (0x5385a0) is the vif record's live particle count (+0x72).
- sum.symbols `vfx_render_alpha` module `vsdk\ca`: the byte is the vfx object's .bss.
- sum.symbols `ix_ray_polygon` (0x53ac20): "ray against a level-object polygon ... ix_point_in_polygon" - the
  body is a segment against a point / normal plane with no polygon test (lobj_ray_test_faces does the rest).
- sum.symbols `ix_line_line` (0x53cb60) intersects two planes (inputs are {normal, d}).
- sum.symbols `ix_slab_clip` (0x53d720) returns the point's outcode (1 / 2 x, 4 / 8 y, 0x20 / 0x10 z).
- sum.symbols `ix_point_linesegment` (0x53da80): "its distance and the subtended angle (acos ...)" - it returns
  the clamped distance along the segment; the acos is in ix_point_sphere_tangent. ix.h's "the distance" too.
- sum.symbols `ix_sphere_boundingbox` (0x53c410) "(center, min, max, radius)": the order is (point, radius,
  min, max).
- sum.symbols data `seg2d_intersect_tmp` (0x2dc6dc8, guard 0x2dc6cc1) belongs to ix_ray_closest_point_t, not
  seg2d_intersect (which has no static).
- sum.symbols `fast_inv_sqrt_table` / `fast_sqrt_table` "filled by float_to_int_round": filled by
  fast_sqrt_tables_init.
- ix.h (decomp55) "the ray within length hits the sphere": ix_ray_sphere never reads the length.
- docs/sum-exe/source-map.md row "`0x53ac20`-`0x53dec0` ... then cmdline param inits": the inits at 0x53dec0 are
  cmdline.cpp's own (the second table row, ending at 0x53dea0, is right).

## Game bugs

- **vfx light tags (list1c_item, 0x5385d0).** The light list walker steps its index by 2 (`sub ecx, 2`), so tag
  k of the lights reaches light ceil(k / 2): lights 0 and 1 are found, light 2 and up resolve to the wrong light
  (or stop early). bind_node, get_node_transform, get_node_world_transform and the hide / show function use it, so
  in an effect with three or more lights, binding / placing / hiding light tags 2+ acts on another light.
- Not a bug: a stopped instance's process updates only the first emitter that still has live particles, but
  render_node updates every drawn emitter as well (vfx_emitter_instance__update skips a repeated time), so the
  others keep moving while they are drawn.
- **World placement of mesh children (get_node_world_transform 0x537e70).** A mesh whose parent is a light,
  dummy, emitter or vif record is placed at the parent's displacement (-delta = new - old position) rather than at
  its own position moved by it. Candidate (needs a check against an effect file with such a hierarchy).
- **vif records measure from the file position (0x537cb3).** The move applied to a vif record's children is
  computed from the descriptor's position, not the record's current one, so a second placement moves the children
  by the distance from the file position again. Candidate.
- **ix_box_oriented_box_oriented (0x53c610, dead code).** Four of the 15 separating-axis tests use the wrong box
  extent (b[2] for b[0], a[2] for a[0]); no caller, no effect in game.
- **ix_ray_sphere ignores the ray length (0x53d7a0).** The script editor's sphere picking (its only caller) hits
  spheres at any distance along the ray; editor only.
- vfx_render_state_reset does not reset vfx_render_blend (0x2d90f08), which nothing reads: no effect.
