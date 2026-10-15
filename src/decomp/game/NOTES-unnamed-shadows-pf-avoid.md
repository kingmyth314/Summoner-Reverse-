# decomp57 notes: 0x4a3580-0x4aa77f (boss_ai.cpp's tail, handle slots, blob shadows, pathfinding geometry, avoidance)

Source written from the Sum.exe v1.40 disassembly (`node tools/asm.mjs`; floats, strings, the jump table
and the EH FuncInfo tables read from `Summoner/Sum.exe`). Summoner 2 was not used. Header base: the repo
include tree at 893c6a5 (the work started on 8ac0692 and was rebased twice while e718f3d and 893c6a5 were
merged; decomp56's declarations for the plane constructor 0x4a42e0, the polygon clippers, gr_offset_point_depth
and projector_record are used as they are). `CHANGED-HEADERS.txt` has the summary and the diff,
`include/DECISIONS.md` D3700-D3712.

Every file in `game/` passes `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings
-Wno-format-security -I include` with no errors and no warnings, and every other `.cpp` under
`src/decomp/game` and `src/decomp/vsdk` (at 893c6a5) compiles against this header copy with no errors.

**135 functions, all of 0x4a3580-0x4aa77f (every function start of sum.symbols and the function list):
88 exact / 45 likely / 2 unsure.** Nothing is left in the range.

## Units and their evidence

| File | Range | Evidence | Functions | e / l / u |
|---|---|---|---|---|
| `game/boss_ai.cpp` (repo file + 1 function) | 0x4a3580-0x4a363a | sum.symbols module boss_ai.cpp; directly after level_preload_effects, no data; the next unit opens with its initialiser (D3712) | 1 | 1 / 0 / 0 |
| `game/unnamed_handleslots_0x4a3640.cpp` | 0x4a3640-0x4a37c9 | initialiser entry 291 at its start; .bss 0x245c9e8-0x245cd7b after boss_ai.cpp's 0x245c9e0; .data 0x59cd88 | 5 | 2 / 3 / 0 |
| `game/unnamed_shadows_0x4a37d0.cpp` | 0x4a37d0-0x4a4ed7 | initialiser entries 292-297 inside it; .bss 0x245cd8c-0x245f39b, .data 0x59cd8c-0x59ce85; ends with the COMDAT vector2::to_vector3 that obstacles_debug_render calls | 28 | 16 / 10 / 2 |
| `game/unnamed_pfgeom_0x4a4ee0.cpp` | 0x4a4ee0-0x4a670f | .bss 0x245f3a0-0x2460cbb (D1203), no initialiser, no .data; begins with the COMDAT vector2::from_xz its segments_intersect_xz calls; the avoidance initialisers follow | 24 | 14 / 10 / 0 |
| `game/unnamed_avoidance_0x4a6710.cpp` | 0x4a6710-0x4aa77f | initialiser entries 298-301 at its start; .bss 0x2460cc0-0x2476bb7, .data 0x59ce88-0x59d34c; multi_door.cpp (strings.tbl id 246) starts 0x4aa780 | 77 | 55 / 22 / 0 |

No file names are known: none of the four new units has a path string or a strings.tbl block, so they
are placeholders (`unnamed_<what>_<address>.cpp`). The cut between the shadow and the geometry unit
(0x4a4ec0 / 0x4a4ee0) follows the .bss blocks (the shadow block ends with num_blob_shadows 0x245f398, the
geometry block starts at 0x245f3a0) and the COMDAT placement (each unit carries the inline vector2
member it calls first). The shadow unit is also the obstacle-debug unit; whether lobj_raycast_draw_lists
and the two vector2 COMDATs at its end belong to it rather than to the start of the geometry unit is
decided by the .bss only (they use none) - see the open questions.

Build notes: the shadow and the avoidance units call inline members out of line (vector3's constructor
0x43b580 and operator= 0x413190, matrix::operator= 0x4a4320, plane::set_normal_point 0x43b5a0,
vector2::operator= / operator+ 0x4aa3a0 / 0x4aa3c0), so they were built without inline expansion, like
the vsdk objects (D500). The geometry unit calls its small helpers too. The handle-slot unit expands its
slot constructor.

## Functions

### boss_ai.cpp (tail)

| Address | Function | Confidence |
|---|---|---|
| 0x4a3580 | level_page_in_characters | exact |

### unnamed_handleslots_0x4a3640.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4a3640 / 0x4a3650 | static_init / init_handle_slots_245c9e8 (8 x 0x38 slots, constructor expanded) | exact / likely |
| 0x4a36a0 | handle_slot_release | exact |
| 0x4a36f0 | handle_slots_reset_thunk (tail call) | likely |
| 0x4a3700 | handle_slots_reset | likely |

### unnamed_shadows_0x4a37d0.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4a37d0 / 0x4a37e0 | init_exp_const_245cdc8: `float = (float)exp(2.8f from .data)` | exact / likely |
| 0x4a3810 / 0x4a3820 | init_copy_const_245cdcc | exact / likely |
| 0x4a3830 / 0x4a3840, 0x4a3850 / 0x4a3860 | two empty initialisers (objects guessed: two vector3) | exact / unsure (x2) |
| 0x4a3870 | frame_sleep_when_inactive | exact |
| 0x4a3890 | obstacles_debug_render | likely |
| 0x4a3de0 / 0x4a3df0 | init_ccmd_obstacle_height_offset | exact |
| 0x4a3e10 | cmd_obstacle_height_offset | exact |
| 0x4a3e80 / 0x4a3e90 | init_shadow_records (blob_shadow's implicit constructor = shadow_record's at +0x74) | exact |
| 0x4a3eb0 | shadow_list_clear | exact |
| 0x4a3ef0 | shadow_record_add | likely |
| 0x4a42e0 | plane::plane(normal, point) COMDAT (decomp56's D3601) | exact |
| 0x4a4320 | matrix::operator= COMDAT | exact |
| 0x4a4360 | shadows_render | likely |
| 0x4a44b0 | shadow_render_on_face | likely |
| 0x4a46a0 | shadow_project_face | likely |
| 0x4a4880 | atexit_nop_4a4880 (the static vertex buffer's empty destructor) | likely |
| 0x4a4890 | shadow_transform_to_lobj | likely |
| 0x4a4b40 | shadow_collect_for_lobj | exact |
| 0x4a4bb0 | lobj_raycast_draw_lists | likely |
| 0x4a4e60 | vector2::nearly_equal | exact |
| 0x4a4ec0 | vector2::to_vector3 | exact |

### unnamed_pfgeom_0x4a4ee0.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4a4ee0 | vector2::from_xz | exact |
| 0x4a4f00 | float_lerp_range | exact |
| 0x4a4f30 | pf_select_graph | exact |
| 0x4a4f60 | seg2d_point_left_of | exact |
| 0x4a4fb0 | pf_segment_hits_walls (true = clear, D3707) | likely |
| 0x4a53d0 | pf_first_edge_crossing | likely |
| 0x4a54b0 | pf_trace_edge_crossings | likely |
| 0x4a5570 | segments_intersect_xz | exact |
| 0x4a55f0 | pf_straight_walk_clear | likely |
| 0x4a5750 | seg2d_segment_intersect | likely |
| 0x4a5bd0 | seg2d_line_intersect (D3708) | exact |
| 0x4a5c60 | pf_bsp_node_find | likely |
| 0x4a5d40 | pf_bsp_find_cell | likely |
| 0x4a5dc0 | pf_point_is_navigable | exact |
| 0x4a5e00 | grid_hash_bucket | exact |
| 0x4a5e20 | grid_linear_index | exact |
| 0x4a5e30 | pf_grid_hash_find | exact |
| 0x4a5eb0 | seg2d_intersect_params | exact |
| 0x4a5f70 | pf_grid_cells_in_box | likely |
| 0x4a6060 | pf_grid_cells_along_segment (DDA, jump table 0x4a649c) | likely |
| 0x4a64b0 | pf_cells_along_segment | exact |
| 0x4a64f0 | pf_cells_in_box | exact |
| 0x4a6530 | seg2d_point_near_middle | exact |
| 0x4a6600 | pf_layer_from_edges | likely |

### unnamed_avoidance_0x4a6710.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4a6710-0x4a67a0 | the three vector2-array initialisers (thunk + body each) | exact |
| 0x4a67e0 / 0x4a67f0 | init_ccmd_avoidance | exact |
| 0x4a6810 | cmd_avoidance | likely |
| 0x4a6b40 | avoid_clear_segment_flags | exact |
| 0x4a6b90 / 0x4a6bf0 | avoid_box_contains_point / avoid_circle_contains_point | exact / likely |
| 0x4a6ce0 / 0x4a6d00 / 0x4a6d20 | the exact variants, avoid_prim_test | exact |
| 0x4a6d60 / 0x4a6e00 | avoid_prim_from_segment / avoid_prim_overlap | likely |
| 0x4a6f50 | avoid_segment_hits_prim | exact |
| 0x4a6f90 | avoid_box_box_overlap | likely |
| 0x4a70e0 / 0x4a7120 / 0x4a7160 / 0x4a71b0 | points in box / circle, rect2d_overlap, seg_params_in_range | exact |
| 0x4a7210 | avoid_poly_nearest_edge_hit | likely |
| 0x4a7320 / 0x4a73d0 | avoid_poly_from_obstacle, avoid_prim_test_exact | exact |
| 0x4a73f0 | avoid_merge_obstacle_polys | likely |
| 0x4a7720-0x4a78b0 | link block / spline / spline point alloc, get, free (9) | exact |
| 0x4a7900 | avoid_spline_point_create | likely |
| 0x4a7990-0x4a7c70 | avoid_spline_create, the seglist / poly / obstacle / area allocators and frees, avoid_seglists_clear_refs, avoid_seglist_free, avoid_poly_release (13) | exact |
| 0x4a7cb0 | avoid_area_rebuild | likely |
| 0x4a7e30 / 0x4a8060 / 0x4a82a0 | corner miter offsets | likely |
| 0x4a8390 | avoid_do_poly_seg_list_links | likely |
| 0x4a8600 | rect2d_add_point | exact |
| 0x4a8680 | avoid_seglist_add_segment | likely |
| 0x4a89b0 / 0x4a89e0 / 0x4a8a60 / 0x4a8a80 | ptr_array_contains, avoid_collect_segment_refs, qsort_cmp_float_asc, avoid_ref_entry | exact |
| 0x4a8aa0 / 0x4a8b80 | avoid_sort_refs_along_segment / avoid_merge_shared_segments | likely |
| 0x4a8df0 | avoid_clip_segments | likely |
| 0x4a93c0 / 0x4a9400 | avoid_qsort_cmp_poly_word, avoid_age_segments | exact |
| 0x4a9440 | avoid_create_obstacle_polys | likely |
| 0x4a9b40 | avoid_area_add | likely |
| 0x4a9d60 / 0x4a9df0 / 0x4a9e60 | avoid_reset, avoid_area_remove, avoid_obstacles_overlap | exact |
| 0x4a9eb0 | avoid_add_primitive | likely |
| 0x4aa3a0 / 0x4aa3c0 | vector2::operator= / operator+ COMDATs | exact / likely |
| 0x4aa3e0 | avoid_poly_get | exact |
| 0x4aa400 | avoid_poly_add_search_nodes | likely |
| 0x4aa630 / 0x4aa6b0 / 0x4aa760 | avoid_point_blocked, avoid_area_contains_point, avoid_record_axis_at | exact |

## How the avoidance works (summary)

- avoid_area_add (spells: wall of fire etc.) makes up to two obstacles, size class 1 (the area jittered by
  up to 0.3 m, the axis by up to 0.1) and size class 2 (the description's axes plus the 0.5 margin), boxes
  or octagons. Kind: the description's +0x00 = 0 gives "avoid" obstacles (flag 4, cost only), nonzero
  "prohibit" ones (flag 2, they block walks). Disabled entirely by `avoidance enable`.
- Each obstacle joins the polygons it overlaps (same kind, size class and layer; at most 3 polygons and 3
  obstacles in all, else it is dropped): avoid_merge_obstacle_polys walks the union outline (at most 30
  points). The polygon is clipped against the nav walls into a segment list; pieces ending on a wall get
  a miter offset and, for prohibit lists, a closing segment along the wall. Corners become A* nodes with
  up to 20 links (neighbours, visible corners of other lists, visible graph nodes).
- pf_add_avoidance_nodes (pathfinding.cpp) blocks A* edges through prohibit polygons and adds
  avoid_poly_add_search_nodes' length-inside-the-obstacle as cost for avoid polygons.
- Removing an area rebuilds the polygons without its obstacles (avoid_area_rebuild), unless `avoidance
  no_remove`.

## Open questions

- The two empty static initialisers 0x4a3840 / 0x4a3860: which objects (written as two unreferenced
  vector3 at 0x245cdd0 / 0x245cddc, a guess), and what the .data floats 0x59cd98-0x59cda8 (2.8, 2.8, 320,
  240, 20) and the exp(2.8) constants 0x245cdc8 / 0x245cdcc were for (nothing reads them).
- shadow_project_face's static vertex buffer has a guard and an atexit callback that is a bare ret: a type
  with an empty destructor. Written as a wrapper struct (unsure); gr_vertex itself may have had it.
- The handle slots: what the 0x38-byte records and the buffers 0x245cba8 / 0x245cbe8 / 0x245cc28 /
  0x245cd30 hold (only handle_slots_reset touches the buffers; the slots are only released). They may
  belong to the effect streamer (handle_slots_reset starts with effect_stream_flush).
- Whether lobj_raycast_draw_lists and vector2::nearly_equal (no data) end the shadow unit or start the
  geometry unit; the cut follows source-map.md and the COMDAT rule.
- pf_debug_mode / pf_debug_path placement (D3711).
- The blob shadow's shadow_frame (0x64 bytes copied when shadow_record_add gets one; the only caller passes
  NULL) and the shadow record's +0xfc..+0x11f are not read in this range.
- The "pushed" corner point (corner +0x08) is used for soft lists only; for prohibit lists
  avoid_do_poly_seg_list_links pushes the query point instead.

## Doc and symbol disagreements (to fix in docs/ and symbols/)

- docs/sum-exe/source-map.md hand cuts: level_page_in_characters (0x4a3580) is boss_ai.cpp's (the cut
  before 0x4a3640 is right, but the table rows start at 0x4a37e0); the shadow unit runs to 0x4a4ed7 and its
  .data starts at 0x59cd8c; the geometry unit ends at 0x4a670f (row says 0x4a6600); the avoidance .data
  is 0x59ce88-0x59d34c and its .bss runs to 0x2476bb7 (row: 0x59cea4 and 0x246b92c) (D3709).
- docs/sum-exe/effects.md: "table 0x245ce8c" for the blob shadows is the shadow_record inside the
  records; the 16 records (0x1f8 bytes) start at 0x245ce18.
- docs/sum-exe/ai.md has no section on obstacle avoidance; the summary above could go there (the areas
  steer A* through extra corners and edge costs; prohibit areas block walks and clicks).
- sum.symbols: avoid_area_desc +0x2c (spells.cpp's `owner`) is the layer (D3702); avoid_poly_points_in_box
  / _in_circle take (shape, polygon) (D3705); pf_segment_hits_walls returns true when clear (D3707);
  seg2d_line_intersect's fourth argument is a float bias (D3708); avoid_seg_alloc / avoid_seg_free handle
  20-link blocks (0x54) for the corners' nodes, not segments; avoid_record_axis_at returns the corner's
  miter offset (corner +0x10); the shadow record table is 0x245ce18 (records) / 0x245ce8c (their
  shadow_record); avoidance_area_type is only printed ("type: prohibit rect/circle"); 0x59ce94 (set by
  "prohibit", cleared by "avoid") is never read; obstacles_debug_render's switches 0x59cd90 (size class),
  0x59cd94 (follow the leader's layer), 0x245cdf8 (all layers) and 0x245ce08 (offset lines) have no names.

## Game bugs noticed (from the code, not checked in game)

1. pf_first_edge_crossing (0x4a5447-0x4a5462) measures the distance to a layer-edge crossing as
   dx*dx + dx*dx: the z term is the x term again. When a move crosses two layer-transition edges, the one
   "nearest" in x is taken, which can put a character or a path on the wrong layer
   (pf_trace_edge_crossings, pf_straight_walk_clear).
2. seg2d_segment_intersect, shared end point a->b == b->a (0x4a57b0-0x4a57dd): returns 3 with out = b->b,
   the far end of the second segment, not the shared point.
3. avoid_create_obstacle_polys: when the merged polygon fails to clip, only the first merged polygon's
   segment list gets AVOID_SEGLIST_OFF cleared again (0x4a9b16); with two or three merged polygons the
   others stay switched off: their corners leave the A* graph while the polygons stay, until the next
   rebuild.
4. avoid_seglist_ensure_corner_offset reads segs[-1] for a corner on no segment, and
   avoid_merge_shared_segments takes segs[index - 2] as the "previous" segment when segs[index - 1] does
   not end at the corner (segs[-1] for index 1). Both read the list header as a segment (theoretical).
5. obstacles_debug_render, offset-line mode (0x245ce08): the second corner's offset is drawn from the first
   corner's position; the midpoint of the normal tick keeps a stale y (debug drawing only).
6. avoid_area_remove returns before avoid_area_free when one of the area's obstacles is no longer used
   (0x4a9e3f), so such an area keeps its slot (14 in all) until avoid_reset; the obstacles rebuilt before
   that one in the loop are already gone (theoretical: nothing in this range frees an area's obstacle
   without the area).
7. pf_bsp_node_find loops forever on a node type other than 1-3 (corrupt .pfg only).
