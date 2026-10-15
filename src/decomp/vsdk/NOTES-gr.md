# decomp40: vsdk\gr\gr.cpp

`vsdk/gr.cpp`, Sum.exe v1.40 code `0x505550`-`0x5081bf`: the renderer-independent graphics layer (modes,
gr_screen, console commands, init, clipping, fades, gamma, colour / texture state, 2D drawing, fog,
screenshots, dispatch to the Direct3D / Glide / OpenGL back ends). Written from the disassembly only
(tools/asm.mjs; import names from the PE import table). Built on the repo at commit `01a1737`
(base-commit.txt): the include copy was refreshed after gr_direct3d.cpp, characterinfo.cpp and ai.cpp were merged.

Syntax check: `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include
vsdk/gr.cpp` is clean (no warnings). All 70 `src/decomp` .cpp files compile against this include copy, with
the same 11 warnings as against the repo's headers.

**All 102 sum.symbols functions of the unit are written: 89 exact / 13 likely / 0 unsure.** Nothing left.
Each `init_gr_mode_*` entry is two generated functions (the table entry `jmp`s to the body 16 bytes on);
the body address is in the comment. The three COMDAT inlines at the end of the object are listed with
comments only: `0x508160` gr::Mode(6 fields) (header inline, D1300) and `0x5081a0` gr__Screen__Screen (the
implicit constructor) are counted; `0x5081c0` gr__Vertex__Vertex (module vsdk\gr) is not produced by this
source while gr_vertex has no constructor (D1605, open) and is not counted.

## Functions

| Address | Name | Confidence | Notes |
|---|---|---|---|
| `0x505550`-`0x505ac0` | `init_gr_mode_*` (30) | exact | `gr::Mode g(t, c, a, b, z, f)` each; bodies at +0x10 |
| `0x505af0` | `init_gr_screen` | exact | jmp to the implicit constructor |
| `0x505b10` / `0x505b20` | `init_ccmd_level_fog_thunk` / `init_ccmd_level_fog` | exact | |
| `0x505b40` | `cmd_level_fog` | exact | float command macro |
| `0x505bd0` / `0x505be0` | `init_ccmd_alpha_fog_thunk` / `_alpha_fog` | exact | |
| `0x505c00` | `cmd_alpha_fog` | exact | |
| `0x505c90` / `0x505ca0` | `init_ccmd_gr_show_stats_thunk` / `_gr_show_stats` | exact | |
| `0x505cc0` | `cmd_gr_show_stats` | exact | bool macro on an int |
| `0x505d80` / `0x505d90` | `init_ccmd_antialiasing_thunk` / `_antialiasing` | exact / likely | help text is a zeroed .bss object (0x25d9230) |
| `0x505db0` | `cmd_antialiasing` | exact | |
| `0x505e00` | `gr_init` | likely | returns 0; 7th argument is vsync (D2003) |
| `0x506110` | `gr_check_dual_monitors` | likely | store after the call |
| `0x506160` | `gr_set_vsync` | exact | |
| `0x506170` / `0x506180` | `gr_screen_width` / `gr_screen_height` | exact | |
| `0x506190` | `gr_deactivate` | exact | |
| `0x5061b0` | `gr_init_context` | exact | D3D / OpenGL only |
| `0x5061f0` | `gr_screenshot` | likely | TGA RLE or JPEG |
| `0x5065e0` / `0x506630` | `gr_fade_out` / `gr_fade_in` | exact | |
| `0x506680` | `gr_get_fade_level` | exact | |
| `0x5066b0` | `gr_draw_fade` | exact | |
| `0x5067c0` | `gr_close` | exact | |
| `0x5067f0` | `Color::Color` | exact | |
| `0x506810` | `gr::Mode::Mode(gr_mode)` | exact | out of line here (D2002) |
| `0x506820` | `Color::set` | exact | |
| `0x506840` | `gr::set_clip` | exact | right edge `>=`, bottom `>` |
| `0x506950` / `0x506960` | `gr_get_aspect` / `gr_set_aspect` | exact | |
| `0x506970` | `gr_get_clip` | exact | |
| `0x5069a0` | `gr::reset_clip` | exact | |
| `0x5069c0` | `gr_clear` | exact | |
| `0x5069e0` | `gr_page_in` | exact | |
| `0x506a20` | `gr_flip` | exact | |
| `0x506a50` | `gr_zbuffer_clear` | exact | |
| `0x506a90` | `gr_set_gamma` | likely | reciprocal constants |
| `0x506b90` | `gr_set_color` | exact | |
| `0x506c10` / `0x506c40` / `0x506c50` | `gr_set_color_ptr` / `gr_set_alpha` / `gr_set_texture` | exact | |
| `0x506c70` | `gr_mark_texture_dirty` | exact | |
| `0x506cb0` | `gr_bitmap_ex` | likely | do / while reclip loop; variable homes |
| `0x506ec0` | `gr_bitmap_stretched` | exact | |
| `0x506f90` | `gr_bitmap` | likely | |
| `0x507100` | `gr_bitmap_float` | likely | vertex constructor (D1605) |
| `0x507310` | `gr_line` | likely | float clip, two mirrored branches |
| `0x507690` | `gr_circle` | likely | no caller |
| `0x5077d0` | `gr_rect` | likely | vertex constructor (D1605) |
| `0x5079b0` | `gr_is_mode_supported` | exact | |
| `0x5079f0` | `gr_get_video_memory_mb` | exact | OpenGL 8 |
| `0x507a20` | `gr_tmapper` | exact | |
| `0x507a80` | `gr_tmapper_multipass` | likely | OpenGL path emulated per pass |
| `0x507d20` | `gr_noop_507d20` | exact | |
| `0x507d30` | `gr_read_backbuffer` | exact | |
| `0x507db0` | `gr_set_fog` | exact | |
| `0x507f70` | `gr_set_tint` | exact | |
| `0x507fa0` | `gr_stub_ret_507fa0` | exact | |
| `0x507fb0` | `gr_finish` | exact | glFinish via the thunk (D2004) |
| `0x507fd0` / `0x508020` | `gr_tcache_flush` / `gr_tcache_free` | exact | |
| `0x508060` / `0x508070` / `0x508080` | three stubs | exact | |
| `0x508090` | `rgb_to_lightened_grey` | likely | reciprocal constants |
| `0x5080c0` / `0x508110` / `0x508140` | `gr_set_antialias` / `gr_set_alpha_test` / `gr_flush_pipeline` | exact | |
| `0x508160` | `gr::Mode::Mode(6)` | exact | header inline, COMDAT here |
| `0x5081a0` | `gr__Screen::gr__Screen` | exact | implicit |

## Data

`.data` `0x5a6058`-`0x5a606b`: Gr_alpha_fog_ratio 0.8, Gr_level_fog_multiplier 1.0, gr_fade_level 1.0,
gr_gamma 1.0, gr_gamma_percent 180 (`0x5a6068`, written by gr_set_gamma only, name ours); then the literals
to `0x5a639c` (the four console commands' literals first). `.bss` `0x25d88a0`-`0x25d9237`: 30 gr::Mode,
gr_fullscreen_rect (`0x25d88b0`, name ours, write-only), gr_color_gamma_lut, gr_screen (0xa8), four
console_command objects (`0x25d8cf0`, `0x25d8de0`, `0x25d8e00` (name ours), `0x25d9210`), gr_fade_duration /
_time, gr_gamma_ramp, the flags `0x25d921c`-`0x25d922c`, ccmd_antialiasing_help (`0x25d9230`, ours). The
unreferenced dwords before each 16-byte-aligned object are alignment.

## Header changes (details in CHANGED-HEADERS.txt and DECISIONS.md D2000-D2005)

- gr__Screen gets its real 0xa8 layout; gr_color, gr_texture(_2), gr_fog_*, gr_tint_*, gr_vsync are field
  macros (D2000).
- The 30 mode globals are `gr::Mode`, the front end takes `gr::Mode`, `operator gr_mode()` added (ours)
  (D2001); Mode(gr_mode) out of line (D2002); gr_init returns int and takes vsync (D2003).
- gl_min.h: GL_MIN_NO_DLLIMPORT (D2004). New gr_glide.h; two D3D cache calls; jpeg_write_file (D2005).

## Open questions

- Division or multiplication by the reciprocal (1/255, 1/3, 0.001): written as multiplications.
- gr_vertex's constructor (D1605): gr_rect / gr_bitmap_float run `vector constructor iterator` with
  0x5081c0; this source gets no call until gr_vertex has a vector3 member.
- gr_opengl_set_antialias is declared `int` in gr_opengl.h; gr_set_antialias forwards its bool dword
  unchanged (`mov eax, [esp + 4]; push eax`), so the original took bool.
- gr_string (int mode) and ui_bitmap_scaled (gr_mode) probably take gr::Mode in the original (D2001).
- The duplicated gr_set_color in gr_draw_fade: two calls in the source, or a macro?
- gr_check_dual_monitors: the false store after GetEnvironmentVariableA means the source assigned after
  the call; the `>= 1` comparison (cmp eax, 1; jb) could be `> 0` on an unsigned.

## Where the docs disagree with the code

1. docs/sum-exe/vsdk-gr.md gives the unit as `0x505e00`-`0x5081d0`; it starts at `0x505550` (30 mode
   initialisers, gr_screen's, four console commands) and its own code ends at `0x50815f`, the COMDATs at
   `0x5081bf`.
2. sum.symbols / types.h: gr_color, gr_texture, gr_texture_2, gr_fog_*, gr_tint_* and 0x25d8dd0 are not
   separate globals but gr_screen fields; gr__Screen__Screen constructs two Colors at +0x54 / +0x61, not
   "vector3 members"; types.h's "+0x54.. not yet verified" is now verified (D2000).
3. sum.symbols module column: gr_mode_lightmap_multitexture (new_options.cpp), gr_mode_particle_additive
   (vsdk\ca), gr_mode_particle_alpha (vsdk\geom), gr_mode_ui_bitmap, gr_mode_automap_additive,
   gr_mode_flat_alpha, gr_mode_mesh_alpha, gr_mode_vfx_flat_additive, the gr_fog_* fields,
   Gr_alpha_fog_ratio, Gr_level_fog_multiplier, Gr_show_stats, cmd_antialiasing_on, gr_antialias_enabled
   ("-") all belong to gr.cpp (its initialisers / .data / .bss).
4. sum.symbols gr_is_mode_supported: "false for multitexture modes without multitexture support" is not in
   this function; OpenGL returns true for every mode, an unknown renderer false.
5. sum.symbols init_gr_mode_25d8cd0 "(gr_set_gamma)": gr_set_gamma only compares against 0x25d8cd0 as the
   end of gr_color_gamma_lut; the mode has no reader. 11 more modes have no reader either (0x25d8cdc,
   0x25d88a4, 0x25d88cc, 0x25d88a0, 0x25d8d00, 0x25d8cd8, 0x25d88c8, 0x25d8dd8 "lightmap blend",
   0x25d8d18, 0x25d8ce8, 0x25d8d14).
6. gr.h: gr_init's last argument was "dual"; it is vsync (D2003). gr.h's gr_rect "z" is each corner's sw.
7. vsdk-gr.md: gr_screenshot's other format is JPEG (jpeg_write_file); format 0 = TGA, anything else JPEG.

## Game bugs noticed

1. gr_init, windowed (200) and desktop (202): the aspect is the desktop's width / height, not the game
   resolution's (the bug SummonerFix's GetSystemMetrics hook fixes). The window is centred horizontally in
   the work area but placed at its top edge.
2. gr_set_gamma: gr_color_gamma_lut is filled with i clamped to 0..255, an identity table, so the OpenGL
   tmapper, which maps vertex colours through it, applies no gamma; on OpenGL (no ramp upload either) the
   gamma setting reaches only the 8-bit alpha textures through gr_gamma_ramp.
3. gr_screenshot: any pixel format other than 565 / 888 / 8888, including 0 from a failed
   gr_d3d_read_backbuffer, ends in the fatal "see your graphics guy" DebugError.
4. gr_set_fog with fog on and a negative near or far < near leaves gr_fog_enabled set with the previous
   call's distances, and the back end is still told to apply them.
5. gr_tmapper_multipass, OpenGL path: copies the vertex pointers into a 32-entry stack array with no
   bound check.
6. cmd_antialiasing toggles its own flag (starts false) instead of the state the options screen set, so the
   first "antialiasing" can turn it on when it already is.
7. gr_draw_fade calls gr_set_color twice with the same value (harmless).
8. gr::set_clip tests the right edge with >= and the bottom edge with > (same result), and an origin
   right of or below the screen gives a negative clip_left / clip_top.
