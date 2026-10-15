# vsdk\ca\character.cpp (decomp30)

Written from Sum.exe v1.40 only (`tools/asm.mjs`, raw .data bytes for the literals and float constants).
No Summoner 2 or Dash Faction code was read. `vsdk/character.cpp` passes
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include <file>`
with no warnings, and all 54 `src/decomp` .cpp files of the repo at a38189f compile against `include/`
(no errors, the same warning counts as with the repo headers). Header changes: CHANGED-HEADERS.txt;
decisions D1000-D1004 at the end of `include/DECISIONS.md`.

## Bounds

Code 0x540810-0x541952, then the compiler-generated 0x541960 / 0x541970 / 0x541980. 0x540790
(gr::Vertex::assign) belongs to the gr unit before it; 0x5419a0 RecordExceptionInfo starts the next unit.
Every function of the unit (46 in sum.symbols' module column) is written; nothing is left.

| Address | Function | Confidence |
|---|---|---|
| 0x540810-0x540850 | ca_characters initialiser: thunk, EH vector constructor (200 x 0x1f8), atexit, EH vector destructor | likely x4 (generated, from the definition) |
| 0x540870-0x5408a0 | the 0x2ee22f8 object (ca_character_holder): thunk, constructor call, atexit, destructor call | likely x4 (generated) |
| 0x5408b0-0x5408e0 | ca_string_pool: thunk, StringPool(), atexit, ~StringPool() | likely x4 (generated) |
| 0x5408f0 | ca_vertex_weights::ca_vertex_weights (sum.symbols `__clear`; returns this, D1002) | likely |
| 0x540900 | ~ca_vertex_weights (bare ret; no symbol) | unsure |
| 0x540910 | ca_vertex_weights::add | exact |
| 0x540940 | ca_vertex_weights::clear_b(int, int) | unsure |
| 0x540950 / 0x540960 | ca_character::ca_character / ~ca_character (empty) | exact x2 |
| 0x540970 | add_skeleton (character_add_skeleton) | likely |
| 0x540b80 | free_skeletons | exact |
| 0x540bd0 | free_skeleton | likely |
| 0x540c10 / 0x540c30 | lock_skeleton_count / unlock_skeleton_count | exact x2 |
| 0x540c80 / 0x540cf0 / 0x540d70 | add_skeleton_trigger / ensure_skeleton_trigger / set_skeleton_trigger_time | exact x3 |
| 0x540df0 | load_shadow (character_load_shadow) | likely |
| 0x540ed0 / 0x540f20 | find_state_skeleton / find_action_skeleton | exact x2 |
| 0x540f70 | find_mesh_tag | exact |
| 0x541020 | report_memory | likely (order of the sums) |
| 0x541160 | reload_mesh | likely |
| 0x541240 / 0x541270 | replace_texture / get_lod_index | exact x2 |
| 0x541290 | ca_init | exact |
| 0x541300 | character_clear | likely (zero registers) |
| 0x541330 / 0x541360 / 0x541400 | ca_close / character_unload / ca_find_character | exact x3 |
| 0x541450 | character_load | exact |
| 0x541500 | character_create_base | likely |
| 0x541700 / 0x541740 / 0x541760 | ca_preload_alloc_slot / character_release / character_release_all | exact x3 |
| 0x541790 | ca_load_preload_index | likely (one stack buffer for the names and the error text) |
| 0x5418f0 | ca_preload_find | likely (the entries addressed through the data block's header) |
| 0x541960 | ~ca_character_holder (jmp ~ca_character; no symbol) | exact (generated) |
| 0x541970 | ca_character_holder() (sum.symbols ca_character_holder__ctor) | exact (generated) |
| 0x541980 | skeleton's scalar deleting destructor (module `vsdk\ca` in sum.symbols) | exact (generated) |

The unit's 46 symbols: 23 exact, 22 likely, 1 unsure. Besides them: 0x540900 (unsure), 0x541960 and
0x541980 (exact, generated).

## Data

.data 0x5ac0e0-0x5ac367: literals only, one copy per use (path four times, "Error opening : %s\n" twice,
each mem_report_line "%d" its own). 0x5ac350 "p" has no reference. Float constants used: 0x581d60 (1/30),
0x581adc (30), 0x581d5c (160), shared with other units.

.bss: ca_string_pool 0x2dc7070, ca_characters 0x2dc7088, ca_preload_slots (the ca_mesh table, 205 x 0x30)
0x2ddfa48, ca_preload_loaded / count / size 0x2de20b8-0x2de20c0, ca_preload_data (1 MB) 0x2de20c4,
ca_preload_offsets / ca_preload_sizes (70 each, names ours) 0x2ee20c4 / 0x2ee21dc, 0x2ee22f4 (no
reference), ca_character_2ee22f8 (the holder, 0x1f8), ca_num_characters and four counters ca_init sets
like skeleton.cpp's (-1 / 0 / 0 / -1; names ours) 0x2ee24f0-0x2ee2500, and the "" literals 0x2ee2504
(report_memory) / 0x2ee2508 (create_base buffer). Error lines: 470, 641, 692, 1278.

## Open questions

- Source order: the error lines put character_create_base (470) before character_add_skeleton (641 /
  692), but its code is after character_load (1278). VC6 did not emit this unit in line order (or
  create_base sits in an included file with the same __FILE__?). The .cpp is in address order.
- .bss order: the initialisers run ca_characters, holder, ca_string_pool, but ca_string_pool has the
  lowest address. The definitions follow the initialiser order.
- 0x2ee45ac (ca_current_character, name ours): written by add_skeleton only, but placed after
  skeleton.cpp's .bss. Defined here; it may be a communal / other-unit variable.
- What the holder at 0x2ee22f8 is (a ca_character wrapped in a class with generated constructor and
  destructor, never referenced).
- ca_vertex_weights 0x540900 (bare ret) as the destructor, 0x540940's two arguments.
- ca_character +0x50 and +0x1f0 are only cleared; +0x1f0 could be a third LOD slot (D1001).
- report_memory's per-key sizes (64 per position key, 36 per rotation key) are not the key classes'
  sizes (0x28, 0x14): older layouts or rough counts.

## Game bugs noticed (for docs/game-bugs)

- ca_load_preload_index does not bound the entry count (70 offset / size slots) or the data size (1 MB
  buffer): a larger preload index overruns .bss. An open failure is formatted into a buffer and
  dropped (no message, no error).
- character_create_base does not check ca_preload_alloc_slot's NULL (205 mesh slots for 200 characters
  with up to 2 meshes each) and stores lods[i] for every name without a bound; the stripped name (up to
  40 characters) is strcpy'd into the 0x20-byte mesh file name, overrunning into the mesh pointer for
  names of 32 characters or more.
- character_unload frees each LOD's mesh but not its shadow mesh (outside a mempool the shadow leaks).
- reload_mesh would index lods[-1] if the current mesh were not one of the LODs (not reachable:
  create_base always makes the mesh lods[0]).
