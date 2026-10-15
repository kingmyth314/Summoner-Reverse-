# Pilot notes: pathfinding.cpp (decomp32)

Source written from the Sum.exe v1.40 disassembly (`tools/asm.mjs fn`; floats, strings and the EH
FuncInfo tables read from `Summoner/Sum.exe`). Summoner 2 and Dash Faction were not used for code.
`game/pathfinding.cpp` passes `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings
-Wno-format-security -I include` with no diagnostics. Header changes: CHANGED-HEADERS.txt, DECISIONS.md
D1200-D1206.

## pathfinding.cpp (0x49c840-0x4a04af), optimised: 40 functions, 20 exact / 20 likely / 0 unsure

The whole unit is written (all 40 functions sum.symbols tags pathfinding.cpp, including the two
initialiser thunk pairs and the atexit callback). Five functions (pf_find_nearest_navigable_point,
pf_find_cell_crossing, pf_compute_endpoint_visibility, ai_path_smooth, ai_path_find) carry a
`profile_label` local: their EH frames' only unwind action is the empty destructor 0x434650 (FuncInfo
0x584050 / 0x584070 / 0x5840c8 / 0x5840f0 and their neighbours); the label texts are guesses.

| Address | Function | Confidence |
|---|---|---|
| 0x49c840 / 0x49c850 | init_ccmd_precalc_vis_method_thunk / init (console_command 0x2452d40) | exact |
| 0x49c870 | cmd_precalc_vis_method (toggle) | exact |
| 0x49c930 | init_empty_49c940_thunk | exact |
| 0x49c940 | init_empty_49c940: the empty initialiser of ai_path_nodes[1000] | likely |
| 0x49c950 | pathfinding_atexit_free | exact |
| 0x49c970 | pf_read_edges | likely |
| 0x49ca20 | pf_read_typed_refs (BSP nodes) | exact |
| 0x49caf0 | pf_compute_graph_bounds | exact |
| 0x49cbd0 | pf_read_edge_list | exact |
| 0x49cc50 | pf_read_region (wall grid) | exact |
| 0x49cd90 | pf_skip_node_floats | exact |
| 0x49cdd0 | pathfinding_load_graph | likely |
| 0x49d930 | ai_path_pool_init | exact |
| 0x49d970 | pf_next_search_id | exact |
| 0x49d980 | pathfinding_init | exact |
| 0x49d9f0 | pathfinding_free_graph | likely |
| 0x49db20 | seg2d_closest_point | likely |
| 0x49dbe0 | pf_door_index_is_open | exact |
| 0x49dc20 | pf_sweep_test_wall | likely |
| 0x49e050 | pf_wall_normal | likely |
| 0x49e0c0 | pf_sweep_record_hit | exact |
| 0x49e110 | pf_sweep_segment | likely |
| 0x49e470 | pf_slide_move | likely |
| 0x49e620 | pf_check_direct_walk | likely |
| 0x49e8c0 | pf_find_nearest_navigable_point | likely |
| 0x49ecb0 | pf_find_cell_crossing | likely |
| 0x49ee80 | pf_mark_nodes_visible_from_point | likely |
| 0x49f340 | pf_compute_endpoint_visibility | likely |
| 0x49f650 | ai_path_smooth | likely |
| 0x49f810 | pf_add_avoidance_nodes | likely |
| 0x49fa70 | pf_graph_search (A*) | likely |
| 0x4a01d0 | ai_path_find | likely |
| 0x4a02d0 | ai_path_alloc_node | exact |
| 0x4a0330 | ai_path_node_create_b | likely |
| 0x4a03c0 | ai_path_node_create | likely |
| 0x4a0440 | ai_path_free_node | exact |
| 0x4a0460 | ai_path_pop_node | exact |
| 0x4a0480 | ai_path_free | exact |
| 0x4a04a0 | pf_node_index | exact |

Globals defined here (.bss 0x2452cb8-0x245c9df, address order): pf_layer_links, pf_search_start_nodes,
ai_path_free_list, ai_path_peak_used, pf_search_iterations, ccmd_precalc_vis_method, ai_path_used,
pf_mem_size (0x2452d50), ai_path_nodes[1000] (0x2452d58), pf_bounds (rect2d, D1200),
pf_search_insertions (0x245c9a8), pf_search_start_count, the ten pool pointers 0x245c9b0-0x245c9d4,
Precalc_vis_method, pf_search_id (0x245c9dc). Names marked ours in the source where sum.symbols has none.
0x2452d3c and 0x2452d54 are not referenced by this unit's code.

## How the pathfinder works (summary)

- The .pfg (version 109 only) is loaded into level_mempool: points, edges, layer-transition edges
  (pf_walls, building pf_layer_links), per layer int lists, then per size class (3) and layer (20) a
  graph: convex cells with bounds, visibility nodes with links (to nodes of any layer of the size class),
  a second linked node list, a dropped block, a BSP; then a wall hash grid per graph.
- ai_path_find = pf_graph_search (+ ai_path_smooth unless Precalc_vis_method), then each node gets its
  normalised x / z step. pf_graph_search: BSP cells of both ends (an off-mesh goal is pulled back to the
  nearest cell-edge crossing), direct straight-walk test (flags 2 or 3, then 4), else endpoint
  visibility and an A* over the nodes visible from the start / goal plus avoidance corners, edge cost =
  length + avoidance cost; open list sorted by f (g + straight distance).
- pf_slide_move sweeps a move against the nav walls of the grid cells it crosses, the obstacle
  outlines and the spline walls, and slides the remainder along the wall (0.015 push-off).

## Open questions

- profile_label texts (five functions) are not in the binary; function names used.
- Which object 0x49c940 initialises: ai_path_nodes is assumed (the only array of a type with an empty
  inline constructor here).
- 0x2452d3c, 0x2452d54, 0x245c9c0 (pool pointer cleared, never allocated) and 0x2460c90 (count read,
  unused): purpose unknown.
- The .pfg "second node list" (graph +0x04 / +0x14) and the dropped block per graph: what the tools
  stored there.
- Owners of 0x245cd8c (path freed at exit, never set by code) and of the .data int 0x59cd8c (== 7 turns
  on the empty per-step search hook).
- avoid_prim's size (0x78 assumed) and the avoidance record names: from pathfinding.cpp's reads only.
- sum.symbols fixes suggested: pf_bounds_* swap x and z (D1200); pf_straight_walk_clear,
  pf_cells_along_segment and vector2_closest_point_on_segment argument orders (D1201, D1206);
  "500 path nodes" is 1000 (D1204).

## Game bugs noticed (from the code, not checked in game)

1. pf_graph_search leaks the partial path when the node pool runs out: the loop does
   `path = ai_path_node_create(..., path); if (!path) { ai_path_free(path); ... }`, i.e. it frees NULL
   (`push 0; call ai_path_free` at 0x4a01b5). The nodes already chained stay allocated until the next
   level load (ai_path_pool_init), so each failure makes the next exhaustion likelier.
2. pf_find_cell_crossing's y overlap test is reversed (0x49ed66-0x49ed84): a cell is examined only when
   the segment's z range contains the cell's whole z range (min below the cell's min, max above its
   max), while the x test is a normal overlap. Off-mesh goals therefore often find no crossing and the
   search returns no path.
3. ai_path_node_create (the A* intermediate nodes and mp_handle_new_path) never writes pos.y; it keeps the
   stale value of the pooled node.
4. ai_path_peak_used starts at 500 rather than 0, so the peak statistic never reports less.
5. pathfinding_load_graph's obsolete-version message says "Continuing without pathfinding data" but
   stops in the DebugError loop (VSDK_ERROR) after closing the file.
6. pf_find_nearest_navigable_point restarts its edge scan whenever the point is within 0.025 of an edge,
   excluding only the last two edges used; three mutually close edges could keep it looping (not
   bounded in the code; theoretical).
