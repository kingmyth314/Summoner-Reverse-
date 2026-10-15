# decomp45: vsdk\gr\glide\gr_glide.cpp

`vsdk/gr_glide.cpp`, Sum.exe v1.40 code `0x547390`-`0x54bb27`: the 3dfx Glide back end (renderer `0x67`).
Written from the disassembly only (`tools/asm.mjs`, raw reads of the jump tables, `.data` and the import
table). Summoner 2 / Dash Faction: names only. Syntax-checked with
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I decomp45/include`
(no errors; 2 `-Wsometimes-uninitialized` warnings that mirror the binary). All 86 `src/decomp` .cpp files
give the same compiler output against `decomp45/include` as against `src/decomp/include` (commit 09e45e3).

**All 63 sum.symbols functions of the unit are written: 40 exact, 23 likely, 0 unsure.** Plus 4 hidden
entries not in sum.symbols (all exact): the two initialiser bodies `0x5479a0` / `0x54a0f0`,
`gr_glide_tcache_frame` `0x547bf0` (empty, `gr_glide_flip` jumps to it) and `gr_glide_set_zbias` `0x54b900`
(no reference). Nothing left.

Not this unit: the 14 `gr_d3d_tcache_*` / `gr_d3d_*texture*` functions at `0x54bb60`-`0x54d430` that
sum.symbols also tags `gr_glide.cpp` (source-map.md: vsdk\gr\directx), and the `tcache_slot__*` members
`0x54d470`-`0x54d5a0` (declared in gr.h, called here).

## Functions

| Address | Name | Confidence | Notes |
|---|---|---|---|
| `0x547390` | `glide_unload_dll` | exact | |
| `0x5473b0` | `glide_load_dll` | exact | glide2x.dll, 112 GetProcAddress |
| `0x547990` | `init_glide_records_2f0eb70` (+ body `0x5479a0`) | exact | vector ctor iterator over `glide_tmus[2]` |
| `0x5479c0` | `gr_glide_tcache_init` | likely | the KB / MB float kept on the FPU stack |
| `0x547a90` | `gr_glide_tcache_free_entry` | likely | the empty-entry branch; four explicit per-TMU stores |
| `0x547b60` | `gr_glide_tcache_close` | exact | |
| `0x547ba0` | `gr_glide_tcache_flush` | exact | |
| `0x547bf0` | `gr_glide_tcache_frame` (hidden, name ours) | exact | empty |
| `0x547c00` | `gr_glide_round_texture_size` | exact | true when w, h are 4..256 powers of two |
| `0x547cc0` | `gr_glide_create_mipmapped_texture` | likely | if-else chain, two LOD switches and the aspect switches inline |
| `0x5484f0` | `gr_glide_create_texture_section` | likely | per-format 16.16 copy loops; the section fields re-read per block |
| `0x5492f0` | `gr_glide_create_texture` | likely | 256 x 256 tiles; `min(256, ...)` |
| `0x5494e0` | `gr_glide_get_section_count` | exact | |
| `0x549500` | `gr_glide_get_section_rect` | exact | |
| `0x549560` | `gr_glide_tcache_set` | likely | the (dead) reload switch |
| `0x5496e0` | `gr_glide_mark_texture_dirty` | exact | |
| `0x549730` | `gr_glide_tcache_free` | exact | |
| `0x549780` | `glide_record::glide_record` | exact | sets the tcache_slot fields itself |
| `0x5497a0` | `glide_record::tmu_init` (sum.symbols `glide_tmu_init`) | likely | the two grTexLodBiasValue calls share one call |
| `0x549840` | `glide_record::init_blocks` | exact | |
| `0x549880` | `glide_record::reset_blocks` | exact | memset + free list |
| `0x549900` | `glide_record::free_texture_thunk` | likely | wrapper in source order, not a COMDAT |
| `0x549910` | `glide_record::free_texture` | exact | |
| `0x549950` | `glide_record::evict_block` | exact | |
| `0x5499d0` | `glide_record::close_thunk` | likely | `jmp close` |
| `0x5499e0` | `glide_record::close` | exact | |
| `0x549a10` | `glide_record::reset_blocks_thunk` | likely | `jmp reset_blocks` |
| `0x549a20` | `glide_record::load_section` | likely | store order at the end |
| `0x549b50` | `glide_record::alloc` | likely | casts of the 2 MB bank compare (`shr` + signed `jle`) |
| `0x549c50` | `glide_record::release_block` | exact | |
| `0x549c80` | `glide_record::new_block` | exact | |
| `0x549cc0` | `gr_glide_close` | exact | |
| `0x549cf0` | `glide_clip_cursor` | exact | |
| `0x549d10` | `gr_glide_init` | likely | switch over the window mode; error lines 758, 762, 785, 813, 843 |
| `0x549f50` | `glide_msg_handler` | likely | WM_ACTIVATE / WM_ACTIVATEAPP |
| `0x549fc0` | `glide_set_active` | likely | closes / reopens the Glide window (Voodoo2 only) |
| `0x54a0b0` | `gr_glide_is_mode_supported` | exact | |
| `0x54a0d0` | `gr_glide_get_video_memory_mb` | exact | |
| `0x54a0e0` | `init_gr_glide_current_mode` (+ body `0x54a0f0`) | exact | |
| `0x54a100` | `gr_glide_set_mode` | likely | texture cases laid out 0, 2, 3, 1, 4, 6 |
| `0x54a540` | `gr_glide_apply_tint` | exact | |
| `0x54a5b0` | `gr_glide_set_viewport` | exact | |
| `0x54a600` | `gr_glide_clear` | exact | |
| `0x54a650` | `gr_glide_zbuffer_clear` | exact | |
| `0x54a6b0` | `gr_glide_deactivate` | exact | |
| `0x54a6d0` | `gr_glide_finish` | exact | |
| `0x54a6e0` | `gr_glide_flip` | likely | `n = count; count -= n` |
| `0x54a780` | `gr_glide_page_in` | exact | flags != 0 = TMU 1 |
| `0x54a7b0` | `gr_glide_stub_ret1` | exact | no reference |
| `0x54a7c0` | `gr_glide_set_bitmap` | exact | (mode, bm), no stage argument |
| `0x54a800` | `gr_glide_bitmap` | likely | vertex store order scheduled |
| `0x54af10` | `gr_glide_line` | likely | |
| `0x54b110` | `gr_glide_line_3d` | likely | |
| `0x54b240` | `gr_glide_tmapper` | likely | operand order of the uv products |
| `0x54b3a0` | `gr_glide_tmapper_multitexture` | likely | strength-reduced offsets; `vlist` written, never read |
| `0x54b900` | `gr_glide_set_zbias` (hidden, name ours) | exact | no reference |
| `0x54b930` | `init_ccmd_whee_thunk` | exact | |
| `0x54b940` | `init_ccmd_whee` | exact | `console_command ccmd_whee("whee", "", cmd_whee)` |
| `0x54b960` | `cmd_whee` | exact | sets `glide_whee`, no reader |
| `0x54b980` | `gr_glide_read_backbuffer` | exact | returns 3 |
| `0x54b9b0` | `gr_glide_set_fog` | likely | |
| `0x54ba90` | `gr_glide_set_antialias` | exact | empty |
| `0x54baa0` | `gr_glide_set_alpha_test` | exact | |
| `0x54bac0` | `gr_glide_set_gamma` | exact | |
| `0x54bad0` | `gr_glide_flush_pipeline` | exact | front buffer copied into the locked back buffer |

## How it works (short)

- Textures: one `glide_tcache_entry` per bitmap frame; a bitmap is cut into 256 x 256 sections (a
  mipmapped one must be a 4..256 power of two and is one section with all its levels). Conversions: 8-bit
  alpha through `gr_gamma_ramp`; paletted, 888 and 1555 to ARGB1555 (pure green 0,255,0 transparent; the
  bitmaps' 1555 alpha bit is inverted); 565 copied; 4444 copied; 8888 to 4444.
- TMU memory: `glide_record` keeps 2800 blocks per TMU as an address-ordered ring. New textures go after the
  last one, never across a 2 MB boundary; when memory or blocks run out it wraps to the bottom and evicts the
  blocks it overlaps (oldest first). Only TMU 0 is used for single-texture drawing; multitexture passes put the
  second pass on its `stage`.
- Focus: on a Voodoo2 (2 TMUs and >= 12 MB) the Glide window is closed on deactivation and reopened on
  activation (`glide_set_active`); otherwise `gr_glide_flip` passes the changes to `grSstControl` once a frame.

## Open questions

1. `.data` order: the literal "whee" (0x5ad550) lies with the globals 0x5ad500-0x5ad54f, before the
   literals of `gr_glide_init`, but `init_ccmd_whee`'s code is at 0x54b940, after the tmappers. In the source
   the console command is placed by its code; which one VC6 follows is open (also: where the .data globals
   go; written after `gr_glide_create_mipmapped_texture`).
2. `.bss` order vs definition order (the frametime.cpp question): written in address order, the three
   dynamically initialised objects where their initialisers lie.
3. 0x2f0f2a4-0x2f132a3 (16 KB, no reference) and the dwords 0x2f0f1f4, 0x2f0f20c, the byte 0x2f132cc:
   unknown; the 16 KB is written as `glide_unused_2f0f2a4`.
4. Branch-implied constants: several stores use a register known to be 0 or 2 from a compare
   (`mov [esp+0x20], esi` for format 2 at 0x547d5f; `mov [esp+0x10], eax` in the green-key case at
   0x548767). Written as the plain constant assignments; needs VC6 to confirm.
5. The `GLIDE_TEXTURE_SIZE` macro stands for the identical compare chains at 0x547c00 and 0x548539; the
   original may have written them out.

## Doc / symbol disagreements

- The brief and the old gr_glide.h said Glide 3; the binary loads **glide2x.dll** with Glide 2 decorated
  names (D2500).
- sum.symbols `glide_poly_vertex_ptrs` "void*[32]" is a `GrVertex[25]` vertex buffer (D2501);
  `glide_tmus` "uint8[0x90]" is `glide_record[2]` (D2503).
- sum.symbols `glide_deactivation_count` counts pending **activations** (msg handler `++` when active,
  0x549f95; `gr_glide_flip` turns it into `grSstControl(GR_CONTROL_ACTIVATE)`, 0x54a71f); the deactivations
  have their own counter 0x2f132b0. Name kept.
- sum.symbols `gr_glide_video_memory_mb` "texture memory in MB": it is SST 0's frame buffer RAM (`fbRam`,
  GrHwConfiguration +8). game-bugs.md (V60 note) says "probably MB": it is MB.
- sum.symbols / old gr_glide.h range "0x5479c0-0x54bb5f": the unit starts at 0x547390 (the DLL loader) and
  its code ends at 0x54bb27.
- `gr_glide_tcache_flush`'s symbol comment and the D3D notes call 0x547bf0 nothing: it is the empty
  per-frame hook (`gr_glide_tcache_frame`, ours).

## Game bugs noticed (none verified in game)

- **8 MB message, 4 MB test.** `gr_glide_init` quits with "At least 8 MB of available video memory required."
  (L944) when `fbRam < 4` (0x549e5d): a 4-6 MB Voodoo passes, the message names 8 MB. (Direct3D tests 5 MB.)
- **Alpha bitmaps larger than 256 texels** (latent): the 8-bit alpha path of
  `gr_glide_create_texture_section` ignores the section position (0x548666-0x5486ee) and copies the
  bitmap's top-left corner into every section.
- **Dead reload** (latent): no code sets a `dirty` byte, so `gr_glide_tcache_set`'s reload (0x5495ee) never
  runs; it would also copy unconverted texels (paletted / 888 / 8888 sources) and always from the start of the
  bitmap for every section.
- **No vertex bound** (latent, like C90): `gr_glide_tmapper` and `gr_glide_tmapper_multitexture` write `nv`
  vertices into the 25-entry `glide_poly_vertex_ptrs` without a check; the multitexture `vlist` is 32.
- **Animated bitmaps rebuilt twice** (minor): `gr_glide_mark_texture_dirty` passes the frame index as the
  bitmap handle to `gr_glide_create_texture` (0x549717), so the next `gr_glide_tcache_set` with the
  animation handle sees `entry->bitmap != bm` and builds the texture again.
- `gr_glide_init` calls `glide_clip_cursor(1)` when `glide_close_on_deactivate` is set (0x549d9a), before it
  sets that flag (0x549e9c): dead on the first init.
