# gr_direct3d.cpp (decomp36)

`vsdk\gr\directx\gr_direct3d.cpp`, the Direct3D 7 back end (renderer 0x66, the retail default), written
from Sum.exe's disassembly only (`tools/asm.mjs fn`, raw reads of the jump tables, IIDs and .data).
Dash Faction / Summoner 2: names only. Syntax-checked with `i686-w64-mingw32-g++ -fsyntax-only -Wall
-Wno-write-strings -Wno-format-security -I decomp36/include` (clean); all 67 src/decomp .cpp files give
the same compiler output against decomp36/include as against src/decomp/include.

**65 of the 66 functions tagged gr_direct3d.cpp: 37 exact, 25 likely, 3 unsure.** Left:
init_gr_d3d_vertex_buf (0x536950) with gr_vertex_pool (0x2d90f10), which belong to the vfx object (D1601).

## Two objects (D1601)

The tagged code is two blocks with the vfx object (0x536950-0x538630, vsdk\geom) between them: the
initialiser table, the .bss and the .data all run first block / vfx / second block, and the first block
ends with its out-of-line header inlines (gr::Mode, d3d_f2dw). So 0x534a20-0x536944 (state and drawing)
is a separate file of vsdk\gr\directx whose name is not known, and 0x538650-0x53ac18 is gr_direct3d.cpp
(its `__FILE__` is at 0x53956c). Both are in `vsdk/gr_direct3d.cpp`, split at the "PART 2" line; a
byte-matching build needs them as two files.

Data. Part 1: .data 0x5a9cf8-0x5a9cf9 (gr_d3d_vertex_color / _alpha = 1), .bss 0x2d90ed4-0x2d90ee7,
initialiser 0x5889cc (gr_d3d_current_mode). Part 2: .data 0x5a9d24-0x5a9d3f (lock flags, two unreferenced
ints = 1, d3d_windowed = 1, the last fog start / end = -1.0), the literals 0x5a9d40-0x5abdf3 in function
order, .bss 0x2dbfd20-0x2dc6cbb (gaps at 0x2dc05c4, 0x2dc0784, 0x2dc2964, 0x2dc2978, 0x2dc298c have no
reference), initialisers 0x5889dc-0x5889e4 (-fb2, -vertexfog, -rz).

## Functions

| Address | Function | Confidence |
|---|---|---|
| 0x534a20 | init_gr_d3d_current_mode (compiler; thunk + body 0x534a30) | exact |
| 0x534a40 | gr_d3d_set_state | likely |
| 0x535420 | gr_d3d_invert_depth_test | exact |
| 0x535450 | gr_d3d_restore_depth_test | exact |
| 0x535480 | gr_d3d_apply_tint | exact |
| 0x535510 | gr_d3d_set_zbias | exact |
| 0x535520 | gr_d3d_page_in | exact |
| 0x535550 | gr_d3d_clear | exact |
| 0x5355e0 | gr_d3d_zbuffer_clear | exact |
| 0x535670 | gr_d3d_set_viewport | exact |
| 0x5356d0 | gr_d3d_bitmap | likely |
| 0x535ce0 | gr_d3d_set_bitmap | exact |
| 0x535d10 | gr_d3d_set_color | exact |
| 0x535d20 | gr_d3d_fog_alpha | likely |
| 0x535da0 | gr_d3d_line | likely |
| 0x535eb0 | gr_d3d_make_line_verts | likely |
| 0x535fd0 | gr_d3d_line_3d | likely |
| 0x5361a0 | gr_d3d_tmapper | likely |
| 0x536360 | gr_d3d_tmapper_multipass | unsure |
| 0x536790 | gr_d3d_has_bump_format | exact |
| 0x5367a0 | gr_d3d_set_bump_env | exact |
| 0x536940 | d3d_f2dw (header inline, gr_direct3d.h) | exact |
| 0x538650-0x538730 | the three CmdLineParam initialisers, atexit registrations and destructors (12, compiler) | exact |
| 0x538740 | gr_d3d_set_render_state | exact |
| 0x538760 | gr_d3d_draw_primitive | likely |
| 0x538850 | d3d_enumerate_ddraw_devices | likely |
| 0x538920 | d3d_enum_devices_callback | likely |
| 0x5389f0 | d3d_ddraw_enum_callback | likely |
| 0x538a70 | gr_d3d_set_default_transforms | likely |
| 0x538b70 | gr_d3d_set_window | likely |
| 0x538c70 | ddraw_error_string (117 cases, generated from a walk of the compare tree) | likely |
| 0x5394e0 | gr_d3d_init | likely |
| 0x539590 | gr_d3d_begin_scene | likely |
| 0x5395c0 | gr_d3d_init_device | unsure |
| 0x53a170 | d3d_pixel_format_masks_valid | likely |
| 0x53a1d0 | d3d_enum_texture_format_callback | unsure |
| 0x53a360 | gr_d3d_noop_hook | exact |
| 0x53a370 | d3d_compute_pixel_format_shifts | likely |
| 0x53a4c0 | d3d_enum_zbuffer_callback | exact |
| 0x53a4f0 | gr_d3d_close | likely |
| 0x53a5f0 | gr_d3d_flip_surface | exact |
| 0x53a610 | gr_d3d_end_scene | likely |
| 0x53a640 | gr_d3d_close_thunk | likely |
| 0x53a650 | gr_d3d_flip_thunk | likely |
| 0x53a660 | gr_d3d_flip | likely |
| 0x53a7f0 | gr_d3d_read_backbuffer | likely |
| 0x53a8c0 | gr_d3d_set_fog | likely |
| 0x53aad0 | gr_d3d_is_mode_supported | exact |
| 0x53aaf0 | gr_d3d_get_video_memory_mb | exact |
| 0x53ab00 | gr_d3d_set_antialias | exact |
| 0x53ab20 | gr_d3d_set_gamma | exact |
| 0x53ab80 | gr_d3d_set_alpha_test | exact |
| 0x53abb0 | gr_d3d_flush_pipeline | exact |
| 0x53ac00 | guid_equal = IsEqualGUID (objbase.h inline, out of line) | exact |

## Open questions

- The name of the first object's file (state / drawing, 0x534a20-0x536944). Its .bss is only the
  texture scales and the current mode; everything else it uses is gr_direct3d.cpp's.
- sum.symbols names swapped twice: gr_d3d_texture_scale_u / _v (D1602) and d3d_primary_surface /
  d3d_back_surface (D1603). Kept, with comments; fixing the symbols is open.
- gr_vertex's constructor (D1605): the original type constructs a vector3 at +0 (0x5081c0); gr.h's
  gr_vertex is POD, so the vfx unit's pool initialiser cannot be written as in the binary yet.
- gr_d3d_init_device: the Voodoo refusal block sits after the epilogue (0x53a125); VC6 does not usually move
  blocks, so the source may use a goto (or exit is noreturn there). The vendor checks are written as two ifs
  (ATI, then S3 / everyone else); a switch with a merged tail fits too.
- The switch in gr_d3d_begin_scene / gr_d3d_end_scene has a second, empty case (the compare tree tests 1
  separately); the original may have had an assert there.
- ddraw_error_string's case order follows the bodies; the source used the DDERR_ / D3DERR_ names.
- gr_d3d_tmapper_multipass: the pass and vertex records are only read here and in gr_poly_get_section;
  field names are ours.
- 0x2dc6ca4 / 0x2dc6ca8 (read in gr_d3d_flip as `n = x; if (n) x -= n`, so volatile) are never written:
  a request flag set from elsewhere in the original, or dead.

## Game bugs and oddities

- **Only the primary display driver is ever used.** d3d_ddraw_enum_callback returns DDENUMRET_CANCEL
  (0x538a58), so DirectDrawEnumerate stops after the first driver; a second 3D card with its own
  DirectDraw driver is never listed. Of that driver's devices, those with hardware T&L are skipped
  (d3d_enum_devices_callback) and the last remaining one is used (normally the HAL).
- **Direct3D reports video memory in bytes.** gr_d3d_video_memory_mb is set to GetAvailableVidMem's free
  texture memory in bytes (0x53a04d) and gr_get_video_memory_mb returns it unchanged; summoner_cfg_load
  lowers config_texture_quality when it is <= 8 (0x43e36c-0x43e376), which with Direct3D never happens.
- The "At least 8 MB of available video memory required." check tests 5 MB (0x500000) of free texture
  memory (0x53a01f).
- gr_d3d_zbuffer_clear clears the clip rectangle without the + 1 that gr_d3d_clear adds (0x5355e0 vs
  0x535550): the last column and row of the clip area keep their old depth (D3DRECT x2 / y2 are exclusive).
- Every card except S3 draws with the depth test reversed (clear to 0.0, D3DCMP_GREATEREQUAL); S3 cards get
  the normal test, fog mode 5 and Gr_far_min / Gr_far_max cut to a tenth. `-rz` toggles the compare.
- On a card with neither table nor vertex fog, gr_d3d_init_device sets up linear table fog with start =
  end = 1.0 and leaves d3d_fog_mode 0, so gr_d3d_set_fog never enables fog.
- 3dfx drivers and any device named Voodoo or Banshee are refused in Direct3D mode (message 943, exit);
  ATI Rage 128 (0x5246) sets gr_byte_25d921d, ATI 0x5144 sets a flag nothing reads.
- gr_d3d_read_backbuffer copies 2 bytes per pixel and returns format 3 (565) whatever the render target is.
- gr_d3d_close never releases d3d_vertex_buffer.
- d3d_bitmap_u_offset / _v_offset / d3d_line_pixel_offset are never written (always 0.0); the viewports
  have dvMaxZ = 0 (pre-transformed vertices, so harmless).
- d3d_pixel_format_masks_valid and d3d_compute_pixel_format_shifts would loop forever on a zero mask
  (only reachable with a palettised render target, which gr_d3d_init_device never creates).
- gr_d3d_fog_alpha (0x535d20) has no caller.
