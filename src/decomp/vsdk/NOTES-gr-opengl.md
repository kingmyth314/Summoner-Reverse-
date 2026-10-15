# decomp33: vsdk\gr\opengl\gr_opengl.cpp

`vsdk/gr_opengl.cpp`, Sum.exe v1.40 code `0x54d5c0`-`0x550658`: the OpenGL back end (renderer `0x68`).
Written from the disassembly only (tools/asm.mjs, import names from the PE import table). Syntax-checked with
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include`; every
`src/decomp` .cpp still compiles against this include copy (same warnings as against the repo's).

**All 54 sum.symbols functions of the unit are written**, plus the hidden `0x54ed50` and the initialiser's
second half `0x54efc0` (not in sum.symbols): 56 entries, **41 exact / 15 likely / 0 unsure**
(of the 54 symbol functions: 40 exact, 14 likely). Nothing left.

Not written (not source of this unit):
- `0x5504f0` `glFinish` (module `-`): the linker's import thunk `jmp [0x57e318]`. Only `gr_finish`
  (gr.cpp, `0x507fbd`) jumps to it; gr_opengl.cpp calls the import directly.
- `0x54d470` `opengl_record__ctor` and the `tcache_slot__*` helpers before `0x54d5c0`: they belong to the
  Direct3D / Glide texture caches (callers `gr_d3d_tcache_*`, `gr_glide_tcache_*`), whatever their folder
  column says.

## Functions

| Address | Name | Confidence | Notes |
|---|---|---|---|
| `0x54d5c0` | `gl_convert_a8_to_rgba8888` | exact | (tex_h, tex_w, bm_h, bm_w, src, dst) like all converters |
| `0x54d630` | `gl_convert_888_to_rgba8888_scaled` | likely | 16.16 loop with a whole-texel step; green key; no caller |
| `0x54d780` | `gl_convert_565_to_rgb888` | exact | |
| `0x54d860` | `gl_convert_888_to_rgb888` | exact | |
| `0x54d8d0` | `gl_convert_4444_to_rgba8888` | likely | 16.16 loop |
| `0x54da80` | `gl_convert_1555_to_rgba8888` | likely | 16.16 loop; top bit set = alpha 0 |
| `0x54dc20` | `gl_convert_1555_to_rgba8888_scaled` | likely | really 565 -> opaque RGBA (D1303); no caller |
| `0x54dda0` | `gl_convert_8888_to_rgba8888` | likely | 16.16 loop; BGRA -> RGBA |
| `0x54deb0` | `gl_convert_4444_to_rgba4444` | exact | |
| `0x54df20` | `gl_convert_1555_to_rgba5551` | exact | alpha bit inverted into bit 0 |
| `0x54df90` | `gl_convert_888_to_rgba5551` | exact | green key |
| `0x54e050` | `gl_convert_8888_to_rgba4444` | exact | |
| `0x54e0e0` | `gl_build_mipmaps` | likely | copies stored levels, box-filters 565 / 888 / 8888 down to 1x1 |
| `0x54e640` | `gr_opengl_tcache_close` | exact | |
| `0x54e690` | `gr_opengl_texture_init` | likely | operator new spelled out; registry "PackedPixels" |
| `0x54e740` | `gr_opengl_delete_texture` | exact | skips slots used this frame |
| `0x54e7a0` | `gr_opengl_tcache_flush` | exact | |
| `0x54e800` | `gr_opengl_tcache_frame` | exact | |
| `0x54e840` | `gr_opengl_create_texture` | likely | (bm, entry, update, unused); format switch (jump table `0x54ed38`) |
| `0x54ed50` | `gr_opengl_get_texture_scale` (ours) | exact | hidden, no reference |
| `0x54ed90` | `gr_opengl_tcache_set` | exact | |
| `0x54eed0` | `gr_opengl_mark_texture_dirty` | exact | |
| `0x54ef20` | `gr_opengl_tcache_free` | exact | |
| `0x54ef90` | `gr_opengl_set_flag_5ad7e8` | exact | write-only flag, no caller |
| `0x54efa0` | `gr_opengl_set_flag_2f13318` | exact | write-only flag, no caller |
| `0x54efb0` | `init_gr_opengl_current_mode` | likely | generated initialiser thunk (table `0x588a1c`) |
| `0x54efc0` | (generated) | likely | `gr::Mode()` on `gr_opengl_current_mode` (jmp `0x5368a0`) |
| `0x54efd0` | `gr_opengl_get_current_mode` | exact | |
| `0x54efe0` | `gr_opengl_set_mode` | exact | jump tables `0x54f2d0`/`2e4`/`2fc`/`30c` |
| `0x54f320` | `gr_opengl_page_in` | exact | |
| `0x54f350` | `gr_opengl_set_bitmap` | exact | |
| `0x54f370` | `gl_init_multitexture` | exact | |
| `0x54f3f0` | `gl_extension_supported` | exact | |
| `0x54f410` | `gr_opengl_init_context` | likely | ortho / viewport block as in set_viewport |
| `0x54f680` | `gr_opengl_set_pixel_format` | exact | |
| `0x54f6f0` | `gr_opengl_init` | exact | lines 0x291 / 0x29c |
| `0x54f790` | `gr_opengl_close` | exact | |
| `0x54f7c0` | `gr_opengl_flip` | exact | |
| `0x54f7e0` | `gr_opengl_clear` | exact | |
| `0x54f870` | `gr_opengl_zbuffer_clear` | exact | |
| `0x54f8c0` | `gr_opengl_set_viewport` | exact | |
| `0x54f9a0` | `gr_opengl_apply_tint` | likely | sum operand order |
| `0x54fa30` | `gr_opengl_bitmap` | likely | local layout, corner order |
| `0x54fe10` | `gr_opengl_set_color` | exact | empty |
| `0x54fe20` | `gr_opengl_set_scale_2f13360` | exact | v * 0.0625 * 0.001, write-only, no caller |
| `0x54fe40` | `gr_opengl_line` | exact | |
| `0x54ff00` | `gr_opengl_line_3d` | exact | |
| `0x54ffa0` | `gl_setup_multitexture_state` | exact | |
| `0x550080` | `gr_opengl_tmapper` | likely | texture-coordinate operand order |
| `0x5502c0` | `gr_opengl_read_backbuffer` | likely | byte order inside the row swap |
| `0x5503b0` | `gr_opengl_invalidate_mode` | exact | no reference |
| `0x5503c0` | `gr_opengl_set_fog` | exact | |
| `0x550500` | `gr_opengl_setup_view` | exact | no reference |
| `0x5505d0` | `gl_set_vertex_arrays` | exact | no reference; second argument unused |
| `0x550630` | `gr_opengl_set_antialias` | exact | empty |
| `0x550640` | `clamp_ubyte` | exact | |

## Data

`.data` `0x5ad7c8`-`0x5ad9df`: `gr_opengl_bound_texture[2]` (D1301), unreferenced `0x5ad7d0` (-1), the
minimum texture size `0x5ad7d4`/`0x5ad7d8` (1, names ours), unreferenced `0x5ad7dc` (4) / `0x5ad7e0` (1),
`gr_opengl_packed_pixels`, `gl_flag_5ad7e8`, `gr_opengl_next_texture_id`; the literals of texture_init,
tcache_flush and create_texture; then `0x5ad854` (1: blend 1 = GL_ONE, GL_ONE; name ours),
`gr_opengl_vertex_color` / `_alpha`, the u / v scales; then the literals of gl_init_multitexture and
gr_opengl_init (two separate copies of the path: no string pooling). The second group is placed after
create_texture in the source to keep that order. `.bss` `0x2f13318`-`0x2f13363` in address order; the
unreferenced dwords `0x2f13320` and `0x2f1334c` are defined as `gl_unused_*`. `gr_opengl_current_mode` is
defined between `gr_opengl_set_flag_2f13318` and `gr_opengl_get_current_mode`, where its initialiser sits.

`gl_tcache_entry` (0x1c bytes, `MAX_BITMAPS` slots, indexed by `bm_get_current_frame`): +0 GL name, +4 / +8
u / v scale, +0xc resident bitmap handle, +0x10 counted bytes, +0x14 uses this frame, +0x18 dirty.

## SummonerFix

SummonerFix patches LoadSettings, not this unit (`0x43da64` renderer 0x67 -> 0x68, `0x43db45` window mode
201 -> 200, because `gr_opengl_init` refuses 201). It hooks four calls made here: `glGetIntegerv`
(`GL_MAX_TEXTURE_SIZE` in `gr_opengl_texture_init`; the 1 GB scratch buffer), `glViewport`
(`gr_opengl_init_context`, `gr_opengl_set_viewport`; display scaling), `SwapBuffers` (`gr_opengl_flip`;
bars, the FAST badge) and `glReadPixels` (`gr_opengl_read_backbuffer`; thumbnails).

## Open questions

- The 16.16 converters: VC6 turned the padding loops into bare `rep stosd`; written as `for (; x < tex_w;
  x++) *d++ = 0` loops (a memset would add the `rep stosb` tail). Needs the compiler to confirm.
- `gl_build_mipmaps` / `gr_opengl_create_texture`: register allocation is close, but variable declarations
  and the stack slots VC6 reuses (arguments as locals) may differ.
- `gr_opengl_read_backbuffer` returns GL_RGBA bytes but reports format 7, whose byte order in this unit's
  converters is B, G, R, A (`gl_convert_8888_to_rgba8888` takes red from byte 2). If the consumers of
  `gr_read_backbuffer` (screenshots, savegame thumbnails) treat it as the game's 8888, red and blue are
  swapped under OpenGL. Not checked against the consumers.
- `gr_opengl_set_mode`'s colour / alpha flags: named `vertex_color` / `vertex_alpha` in sum.symbols, but set
  means "ignore the vertex value" (white / 255 is used), e.g. colour source TEXTURE.
- What `0x5ad7d0`, `0x5ad7dc`, `0x5ad7e0`, `gl_flag_5ad7e8`, `gl_flag_2f13318` and `gl_scale_2f13360` were for
  (written or initialised only).

## Game bugs noticed

1. `gl_build_mipmaps`, 565 case: the source pointer never skips the second row (`0x54e5a8` adds 4 bytes per
   output texel only), so each generated level averages rows k and k + 1 and stretches the top half of the
   previous level; the bottom half is lost. 888 / 8888 step two rows correctly.
2. `gr_opengl_apply_tint` stores `(ubyte)(sum * tint)` before `clamp_ubyte`, so the clamp does nothing and
   bright tinted colours wrap around; the "luminance" is r + g + b, not an average.
3. `gr_opengl_init` with window mode 202 (desktop size) never creates a context; `glGetString` then returns
   NULL and `strstr` faults. Its error text names `gr_direct3d_init` (copy-paste).
4. `gl_init_multitexture`: without the GL_ARB_multitexture extension the entry points stay NULL and are
   called at once by `gl_setup_multitexture_state` (the "does not support multitexturing" box only covers
   fewer than two units).
5. `gr_opengl_create_texture` counts 2 bytes per texel of the last level processed, so mipmapped textures are
   counted at 1x1 and 32-bit textures at half size (the VRAM figure is meaningless). With `update` it sends
   level 0 twice (glTexSubImage2D, then glTexImage2D). Its early return for a size below 1 leaves the
   memory pool set to NULL (unreachable: the minimum size is 1).
6. `gr_opengl_delete_texture` skips slots used in the current frame, so `gr_opengl_tcache_flush` can leave
   textures resident and then prints (into string_buffer only) the VRAM warning and zeroes the counter.
7. `gr_opengl_setup_view` (dead code): gluLookAt's target is the forward vector, not eye + forward, and
   gluPerspective gets a near plane of 0.
