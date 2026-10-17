# VC6 match pass match26 (second 8966 pass, eight units)

Base: `snapQ/src/decomp` (merged state of this round). Compiler: cl 12.00.8966 (`scratchpad/vc6sp5/VC98`),
flags from `match26/src/vc6-flags.txt`, headers from `match26/src/include`. `before/<unit>.txt` and
`after/<unit>.txt` are the `vc6-compare -v` outputs. Helpers: `cmp.mjs` (vc6-compare on this tree), `show.mjs`
(status + side-by-side of an address), `try.mjs` + `parsev.mjs` (variant tester, text variant files in `v/`,
reports gained / lost addresses), `side2.mjs` (side.mjs that also marks register differences),
`markers.mjs` (sets the Confidence markers from the compare output).

| Unit | Flags | Before | After | Won |
|---|---|---|---|---|
| game/level_scripts.cpp | /O2 | 85 / 124 | 85 / 124 | (two functions closer) |
| game/unnamed_avoidance_0x4a6710.cpp | /O2 | 41 / 67 | 44 / 67 | avoid_link_block_alloc, avoid_point_blocked, avoid_area_contains_point |
| vsdk/unnamed_vfile_0x5128b0.cpp | /Ox /Ow | 64 / 82 | 64 / 82 | - |
| vsdk/gr_opengl.cpp | /Ox /Ob0 /Ow | 34 / 54 | 35 / 54 | gl_convert_888_to_rgb888 |
| vsdk/unnamed_ds_0x55bf90.cpp | /Ox /Ob0 /Ow | 43 / 60 | 51 / 60 | duplicate_buffer, init, channel_release, sound_release, load_buffer_data, play_3d, find_free_channel, estimate_duration |
| vsdk/gr_glide.cpp | /O2 /Ob0 -> /Ox /Ob0 /Ow | 44 / 61 | 48 / 61 | tcache_init, tcache_free_entry, reset_blocks, gr_glide_tmapper |
| vsdk/gr_direct3d.cpp | /Ox /Ob0 /Ow | 36 / 50 | 36 / 50 | - |
| vsdk/bmpman.cpp | /Ox /Ow | 43 / 56 | 46 / 56 | bm_hash_name, bm_load_animation_path, bm_load_animation |
| **Total** | | **390 / 554** | **409 / 554** | +19 |

No function lost its match (checked address by address, `before/` against `after/`). The first column of the
vsdk units already used the D7502 /Ox /Ow lines, so the earlier "still differs" lists were re-checked under them
first: the only flag change is gr_glide (D8500). No header changed. mingw (`i686-w64-mingw32-g++ -fsyntax-only
-std=c++98 -w -Iinclude`): 0 `error:` lines in all eight files. Decisions D8500-D8502 are appended to
`src/include/DECISIONS.md`.

Markers: every matching function says `Confidence: exact` (18 raised, among them the D7502 "likely (a byte match
under /Ox /Ow)" ones that now match under the tree's flags); every differing function that said exact now says
`likely (differs under cl 8966 in match26...)` (46 lowered: level_scripts 21, vfile 8, gr_opengl 7, gr_glide 3,
avoidance 2, gr_direct3d 2, bmpman 2, ds 1).

## Flags (D8500)

gr_glide under /Ox /Ob0 /Ow: +2 (gr_glide_tcache_init, glide_record::reset_blocks), -1 (glide_record::new_block).
new_block matches again with the tail link written through the array (`blocks[used_tail].next`), so the unit
moves to the vsdk flags like the rest of the library. /Ox alone: +0. unnamed_net was not part of this pass.

## What worked (D8501)

- **`while (1)` retry loops** (`if (hr >= 0) break; ...; if (tries++ >= 10) break;`): ds_channel_release,
  ds_sound_release. The do-while is peeled by 8966.
- **Result variable + break** for search loops whose binary keeps the not-found value in a register:
  avoid_link_block_alloc (EAX = 0), ds_find_free_channel (EBP = -1).
- **Inverted conditions** so the binary's fall-through is the then-block: ds_init's DirectSoundCreate /
  EAXDirectSoundCreate choice (`if (!dsinit)`), gr_glide_tcache_free_entry (`if (num_sections != 0) {..} else`).
- **Switch order + shared failure**: ds_load_buffer_data (ADPCM case first, its failure `goto fail` into the
  default's `r = -1`).
- **Index vs pointer**: index forms for ds_duplicate_buffer and glide_record::new_block; a `ds_buffer *b` local
  for ds_estimate_duration and ds_load_buffer_data; a walking `GrVertex *gv` in gr_glide_tmapper's for header.
- **Product order** `gv->oow * (scale_v * v)` (gr_glide_tmapper; all 12 orders tried).
- **`(double)` on a float argument of ceil** schedules the argument store first (bm_load_animation(_path)).
- **No `(int)` cast on the unsigned divisor** (ds_estimate_duration: the binary's int64 conversion).
- **A local copy of a pointer parameter** (`ubyte *s = src;`, gl_convert_888_to_rgb888).
- **FreeSpace store order** of the WAVEFORMATEX in ds_init; a play-flags local set by an if (ds_play_3d).
- **`for (p = name; *p; p++) hash = _rotl(hash, 6) ^ (char)tolower(*p);`** (bm_hash_name).
- **`vector2 p(pos->x, pos->z)`** (avoid_area_contains_point) and **a `(ubyte)` cast on a ushort flag test**
  (avoid_point_blocked).
- Closer, not matched: float fields of the save buffers read as integer moves (`*(int *)&x = *(int *)(data +
  off)`) in read_automap_labels (now differs only in one frame slot) and read_item_objects (gold_pos merged into
  pos; still one frame slot of 12 bytes too many and the lock byte's scheduling).

## What still differs and why (D8502)

- **unnamed_ds (9)**: ds_is_3d_buffer (`shr al / and al` byte narrowing: ten forms tried), ds_load_buffer
  (register / slot choice; it also passes `streamed` raw, so ds_load_buffer and snd_load would take a bool:
  not changed, the callers are all constant pushes but the header is shared), ds_create_buffer, ds_fill_buffer
  (the locals in dead argument slots; the inverted Lock test does not help), ds_load_buffer_acm (zero register),
  ds_play (8966 keeps ch in ESI; the index form gets closer), ds_play_streamed (the float priority is pushed
  raw), ds_stream_open (descriptor store scheduling), ds_stream_read_chunk (the `return -1` after acm_convert
  is moved to the end; the acm_stream_size call through a local gets the first half).
- **bmpman (10)**: release_from_mark (the second loop's start pointer kept in EBX across the first loop),
  free_data, set_reduced (last argument evaluated first), load_internal (one frame slot more, `reduced` in
  memory), read_header, prefer_vbm (push shrink-wrapping), replace_file, lock_tga / lock_vbm / lock_pcx
  (register choice; lock_pcx's `[esi*8 + base]` addressing).
- **gr_glide (13)**: glide_load_dll, create_mipmapped_texture, create_texture_section, create_texture,
  get_section_rect, tcache_set, evict_block, load_section, alloc (HEAD kept in EBX across the function),
  gr_glide_init (the first two stores swapped), glide_msg_handler, gr_glide_bitmap, tmapper_multitexture (the
  binary is 215 bytes longer: probably an unrolled or duplicated loop the source lacks; not investigated).
- **gr_opengl (19)**: the gl_convert loops (register choice; a8 is one SIB order away with `*d++` per branch),
  build_mipmaps (1044 against 1363 bytes: source shape), create_texture, set_mode, init_context,
  set_viewport (frame `push ecx` vs `sub esp, 8`), apply_tint (`fst`), bitmap, tmapper (the u / v products and
  stores), read_backbuffer.
- **unnamed_vfile (18)**: the FileReader readers (D7506's push placement), file_add_path, file_find_files,
  file_list_files, File::find (size), set_cipher_key / get_async_status (register choice and address
  arithmetic; index forms tried), seek (operand order: both orders of the sums give the same code),
  calc_crc32.
- **gr_direct3d (14)**: set_state, clear / zbuffer_clear (scheduling), bitmap, make_line_verts,
  tmapper_multipass, draw_primitive (preloaded discard flags), enumerate_ddraw_devices (`return NULL` placement),
  set_window (lea operand order), init (store order), init_device, enum_texture_format_callback,
  compute_pixel_format_shifts, read_backbuffer.
- **level_scripts (39) and unnamed_avoidance (23)**: register allocation and frame-slot order in the savegame
  members and the avoidance builders (already worked under 8966 in match16 / D7500); new attempts this pass:
  the spline getters (8966 always puts the shared `return NULL` last), avoid_spline_point_create (six store
  orders), cmd_avoidance (an if / else of the two print calls changes the size), write_character (the float
  copied through a stack temporary, D7506).

## Logic errors and game bugs found

- **Decomp correction (ds_estimate_duration, 0x55dd10):** the divisor was cast to int; the binary divides by
  the unsigned nSamplesPerSec (converted through a 64-bit integer). Only rates of 2^31 and up would differ.
- No new game bug. The known ones in these units stand (ds_init's retry through a NULL ds_direct_sound,
  D7503's three-entry index array in avoid_area_rebuild, the fade flag left on a channel).
