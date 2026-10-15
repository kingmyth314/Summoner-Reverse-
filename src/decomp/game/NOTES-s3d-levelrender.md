# decomp28: s3d.cpp and levelrender.cpp

Written from the Sum.exe v1.40 disassembly (`node tools/asm.mjs fn`, raw .data bytes from
`Summoner/Sum.exe`). Summoner 2 and Dash Faction were not used. Headers: a copy of `src/decomp/include` at
6b657d4 (re-copied twice while working, after d790a53 and a38189f landed) plus the changes in
CHANGED-HEADERS.txt and DECISIONS.md D800-D806. Both files pass
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include`
without warnings; all 54 `src/decomp/{game,vsdk}/*.cpp` still compile against the copy, with the same
warning counts as against the repo headers.

Both units are optimised builds (/O2: no frame pointer, 0x90 padding). D500's inline vector members
apply: the code expands some uses and calls the COMDATs for others inside the same function
(levelrender_wave_lobj), which looks like VC6's per-function inline budget.

## s3d.cpp (`Engine\s3d\s3d.cpp`, 0x46dd30-0x4705a3): 52 functions, 37 exact / 15 likely / 0 unsure

Plus two unnamed bodies (0x46e0d0 `lmap_reset_all`, 0x46e150 `lmap_off_all`, names ours), both exact.

| Address | Function | Confidence |
|---|---|---|
| 0x46dd30-0x46dd80 | sinit / init / register / dtor of `s3d_level level_s3d`, and the compiler's s3d_level ctor 0x46dd80 / dtor 0x46dd70 (`level_names__ctor` / `__dtor`) | exact (6) |
| 0x46dd90 / 0x46dda0 | init_mempool_level_thunk / init_mempool_level (`Mempool level_mempool(buffer, 0x186a000, "level")`) | exact (2) |
| 0x46ddc0 / 0x46ddd0 / 0x46ddf0 | ccmd_test_meshes_on_load thunk / init / cmd_test_meshes_on_load | exact (3) |
| 0x46deb0 / 0x46dec0 / 0x46dee0 | ccmd_load_lightmaps thunk / init / cmd_load_lightmaps | exact (3) |
| 0x46dfa0 / 0x46dfb0 / 0x46dfd0 | ccmd_p3d_files thunk / init / cmd_p3d_files | exact (3) |
| 0x46e090 / 0x46e0a0 / 0x46e0c0 | ccmd_lmap_reset thunk / init / handler (tail jump to 0x46e0d0) | exact (3) |
| 0x46e110 / 0x46e120 / 0x46e140 | ccmd_lmap_off thunk / init / handler (tail jump to 0x46e150) | exact (3) |
| 0x46e190 | s3d_geometry__dtor (compiler's) | exact |
| 0x46e1a0 | s3d_level_clear | exact |
| 0x46e1b0 | level_load_s3d | likely |
| 0x46e910 | s3d_read_geometry | likely |
| 0x46ebe0 | s3d_read_meshes | likely |
| 0x46f100 | s3d_load_lightmap | likely |
| 0x46f210 | s3d_read_vertex_set | exact |
| 0x46f2c0 | s3d_read_navpoint | likely |
| 0x46f3a0 | s3d_read_named_path | exact |
| 0x46f460 | s3d_read_light | likely |
| 0x46f560 | s3d_read_vis_set | likely |
| 0x46f660 | s3d_read_texture_names | exact |
| 0x46f6d0 | s3d_read_header | exact |
| 0x46f870 | s3d_mark_alpha_meshes | likely |
| 0x46f930 | s3d_load_textures | exact |
| 0x46f990 | s3d_alloc_level_arrays | likely |
| 0x46fdb0 | s3d_geometry__ctor (compiler's) | exact |
| 0x46fdc0 | level_free_s3d | likely |
| 0x46fef0 | level_page_in_textures | exact |
| 0x46ff50 / 0x46ffa0 | s3d_read_face / s3d_read_faces | exact / exact |
| 0x46ffe0 / 0x470150 | s3d_read_mesh_ref / s3d_read_mesh_refs | likely / exact |
| 0x470190 / 0x4701e0 | s3d_read_box / s3d_read_boxes | likely / exact |
| 0x470220 | s3d_read_door | likely |
| 0x470300 | s3d_read_color_record | likely |
| 0x470410 / 0x470560 | s3d_read_textured_record / s3d_read_textured_records | likely / exact |

Data: `.data` S3d_load_lightmaps = true (0x59aa58; literals 0x59aa5c-0x59ad5f). `.bss` 0xa5a2a0-0x22e4425:
level_s3d, the 25.6 MB mempool buffer (0xa5a380), the pool cursors (names ours: vertex 0x22c4380, normal
0x22c4394, short 0x22e43b8, byte 0x22e43bc, uv 0x22e43dc, mesh face 0x22e43e0, face 0x22e4418, int
0x22e441c; 0x22c4384 / 0x22e43b4 / 0x22e4414 are only reset), the 128 KB lightmap buffer (0x22c4398),
level_mempool, the five console commands, the $vis_base globals and the flags. Objects with dynamic
initialisers are defined in initialiser order (entries 207-213), the others in address order.

How the loader works: s3d_read_header reads the version and every chunk count; s3d_alloc_level_arrays
resets level_mempool and allocates every array in it (so level_free_s3d frees nothing one by one); the
readers then fill the records in file order, taking slices of the per-level pools (vertices, normals,
uvs, int16s, per-vertex RGB bytes, faces, face references, ints) through the cursors. Every reader is a
sync function (write mode exists but nothing in Sum.exe opens a level for writing).

## levelrender.cpp (`Engine\rendering\levelrender.cpp`, 0x48db30-0x492af0): 30 of 39 functions written, 13 exact / 5 likely / 12 unsure

| Address | Function | Confidence |
|---|---|---|
| 0x48db30 / 0x48db40 | init_array_23df188 thunk / init (`attached_light levelrender_lights[16]`, D805) | exact (2) |
| 0x48db60-0x48dc10 | six init_empty thunk / body pairs (objects with empty inline constructors) | unsure (12: which globals they are) |
| 0x48dc20 / 0x48dc30 / 0x48dc50 | ccmd_time_mult thunk / init / cmd_time_mult | exact (3) |
| 0x48dce0 / 0x48dcf0 / 0x48dd10 | ccmd_space_mult thunk / init / cmd_space_mult | exact (3) |
| 0x48dda0 / 0x48ddb0 / 0x48ddd0 | ccmd_small_mesh_threshold thunk / init / cmd_small_mesh_threshold | exact (3) |
| 0x48de60 | levelrender_wave_lobj | likely |
| 0x48e460 | atexit_nop_48e460 (compiler's, the static GameTimer of wave_lobj) | exact |
| 0x48e470 | levelrender_create_vertex_buffer | likely |
| 0x48e4f0 | levelrender_free_d3d_vertex_buffer | exact |
| 0x48e520 | level_lights_setup | likely |
| 0x48f8c0 | levelrender_face_vertex_colors | likely |
| 0x48fbb0 | levelrender_sum_point_lights | likely |

(The twelve init_empty functions are a bare `jmp` / `ret` each; they are unsure only in which objects they
belong to.)

**Left (9 functions):** 0x48e6c0 levelrender_draw_list_glide (0x1200 bytes), 0x48fdb0
levelrender_draw_list_d3d (0x1730 bytes), 0x491500 levelrender_draw_list_opengl (0x15e0 bytes) and the
six compiler-generated atexit handlers of their function-local statics (0x48fd90, 0x48fda0, 0x4914e0,
0x4914f0, 0x492ae0, 0x492af0; they come with the functions). Their .bss is not defined yet:
levelrender_glide_poly_verts 0x2319070, levelrender_d3d_vert_colors 0x235b1f0, levelrender_gl_texcoords1
0x2366d70, levelrender_gl_texcoords0 0x2378770, the byte array 0x2395cf0 (OpenGL), 0x2400ec8 (view-space
vertices passed to levelrender_face_vertex_colors), levelrender_gl_vert_point_light 0x23c5b3c,
levelrender_glide_proj_verts 0x23c7a80, levelrender_glide_vert_colors 0x23d3608,
levelrender_d3d_proj_verts 0x23dfb08, levelrender_d3d_vert_point_light 0x23eb690, 0x23f81e4,
levelrender_gl_color_array 0x23fc844, levelrender_glide_vert_point_light 0x240ca48, 0x240d9e8,
levelrender_gl_proj_verts 0x240d9f0, levelrender_gl_vertex_array 0x2419570, 0x24268f4,
levelrender_gl_vert_colors 0x24268f8, levelrender_d3d_vert_lit 0x2432484, 0x2433424,
levelrender_glide_vert_lit 0x2437a74, and the guards 0x23eb68c / 0x23d3600 / 0x23c5b38. The per-vertex
arrays are 4000 entries (0xbb80 bytes of vector3 between neighbours). The fan triangulation tables of
.data (0x59c35c-0x59c47b, read by the D3D and OpenGL lists) are defined already.

Data written: `.data` level_light_scale 1.0 (0x59c358), the fan tables, Small_mesh_threshold 3.0,
Time_mult 5.0, Space_mult 20.0 (0x59c47c-0x59c484). `.bss`: levelrender_wave_vertex_face_count
(0x238a170, 12000 floats: the bound of wave_lobj's last loop is 0x2395cf0), levelrender_wave_vertex_normals
(0x2396ca8), levelrender_face_verts (0x23c5aa8, three 0x30-byte gr_vertex, name ours),
levelrender_lights (0x23df188), levelrender_face_vert_list (0x23ec630), levelrender_wave_vertices
(0x23ec658, 4000), the three console commands, levelrender_d3d_vertex_buffer (0x243e03c).

## Open questions

- Which six objects the empty initialisers 0x48db70-0x48dc10 construct. They run after
  levelrender_lights and before the console commands; every vector3 array qualifies (D500's empty
  inline constructor). levelrender.cpp defines the two wave arrays there; the other four must be among
  the draw lists' arrays, some of which are function-local statics (two per draw list).
- `new vector2[n]` in s3d_alloc_level_arrays leaves the same loop-counter vestige as vector3 /
  quaternion / matrix arrays (0x46fcd4-0x46fcf1), so vector2 probably had an empty inline constructor
  too; vector.h declares none (adding one would turn it into a non-aggregate, so it is not done here).
- s3d_version_flag (sum.symbols name) is set only by `_stricmp(filename, "ionaext.s3d") == 0` and only
  gates the "Ocean" flag 0x1000000: the name could be "s3d_is_ionaext".
- lobj_face's high byte (+0x07, synced as an int16), most s3d_light fields (+0x30..+0x44), the
  geometry record's +0x04 / +0x24 / +0x30 / +0x50 ints, the vertex-set fields and the colour records'
  meaning. s3d_geom_block's layout comes from sum.symbols' comment of 0x4bcdc0 only.
- s3d_mark_alpha_meshes sets the face flag with a byte `or` while s3d_read_face syncs the field as
  uint16: the field may be two bytes in the original.
- s3d_lightmaps_loaded (0x22e4420, name ours) is written by s3d_load_lightmap and never read by address.
- level_lights_setup's `case 1: case 4:` (spot) is written before `case 0:` (point) because its code
  comes first; VC6's case order for a compare chain is not checked.

## Game bugs noticed (none visible in play)

- s3d_read_color_record (0x470300): the third clamp reads green but stores into blue (bound 0.998
  instead of 1.0): green is never clamped and blue is replaced by the clamped green. Harmless: nothing
  reads the colour records (level +0x58 / +0x5c).
- level_free_s3d (0x46fdc0) clears the navpoint pointer (+0x3c) but not the navpoint count (+0x38), and
  the colour-record pointer but not its count. level_marker_find / script_editor_draw_navpoints loop
  level_num_navpoints over level_navpoints, so a call between level_unload and the next load would read
  through NULL; latent (nothing calls them there).
- s3d_read_door returns -1 when the file has no error and 0 on error (neg / sbb / neg / dec, the
  opposite of the other readers); level_load_s3d ignores the result.
- levelrender_wave_lobj accumulates corner normals and counts into two global arrays that are never
  cleared and never read: dead work every frame for every waving object (the counts only grow).
- levelrender_face_vertex_colors with the tint on uses the sum of the three RGB bytes clamped to 255
  (not their average), so tinted level faces reach the full tint colour once the average channel passes 85 (a third of 255).
- s3d_read_meshes compares "s1-sewercanal" with length 7, so every mesh named "s1-sewe..." gets flag
  0x100000 (probably meant as a prefix; checked non-bug unless such a mesh exists).
