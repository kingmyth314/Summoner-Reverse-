---
title: "vsdk: graphics (gr, gr_font)"
parent: Sum.exe
nav_order: 5.9
---

# vsdk: graphics (`gr.cpp`, `gr_font.cpp`) and keyboard

**Summary:** `vsdk\gr\gr.cpp` (`0x505550`-`0x5081cf`) is a thin dispatcher over three renderers. Most
functions check `gr_screen.mode` (`0x25d8d3c`) and call the Direct3D (`0x66`), Glide (`0x67`) or OpenGL
(`0x68`) back end. The unit also holds the 30 render modes, `gr_screen`, four console commands, the window
setup, clipping, fades, gamma, fog and screenshots. `gr_font.cpp` (`0x5081d0`-`0x50939f`) draws bitmap fonts.
The keyboard module follows at `0x5093a0`. All 102 functions of `gr.cpp` are named and written in
`src/decomp/vsdk/gr.cpp` (89 exact, 13 likely). Most names and signatures match
[Dash Faction](../reference/engine-references.md)'s `rf::gr` API. That code probably descends from this one
(*inferred*).

The gamma option does almost nothing with the OpenGL renderer that [SummonerFix](../summonerfix/index.md)
selects: see [Gamma](#gamma) and game bug K90.

{: .important }
Corrected 2026-10-15 against the decompiled unit: the unit starts at `0x505550`, not `0x505e00`; the
screenshot's second format is JPEG; the gamma, fog, colour and texture "globals" are fields of `gr_screen`;
`gr_is_mode_supported` and `gr_get_video_memory_mb` are no longer open; the mouse range starts at
`0x500a30`. Details under [Changes](#changes-2026-10-15).

## Layout of the unit

All **observed** in the disassembly.

| Range | Content |
|---|---|
| `0x505550`-`0x505aef` | 30 render-mode initialisers (static-initialiser table `0x5887ec`-`0x588860`). Each is a table entry that jumps 16 bytes on to its body |
| `0x505af0` | `init_gr_screen` (table `0x588864`) |
| `0x505b10`-`0x505df8` | Console commands `level_fog`, `alpha_fog`, `gr_show_stats`, `antialiasing` and their initialisers (table `0x588868`-`0x588874`) |
| `0x505e00`-`0x50815f` | The functions below |
| `0x508160`-`0x5081cf` | Out-of-line header inlines: `gr::Mode::Mode` (6 fields), the implicit `gr__Screen` constructor, `gr__Vertex__Vertex` |

`.data` is `0x5a6058`-`0x5a6397`: `Gr_alpha_fog_ratio` (0.8), `Gr_level_fog_multiplier` (1.0),
`gr_fade_level` (1.0), `gr_gamma` (1.0), `gr_gamma_percent` (180, written by `gr_set_gamma` only, name ours),
then the string literals. The path string appears twice (`0x5a62d8`, `0x5a6364`), so string pooling was off.
`.bss` is `0x25d88a0`-`0x25d9237`. `gr_fonts` (`gr_font.cpp`) follows at `0x25d9238`.

The mode objects in `.bss` are not in source order. Their addresses follow the initialiser order, which
explains why the source-map's `.bss` rule once cut this file in two.

## `gr_screen`

`gr_screen` (`0x25d8d30`) is one 0xa8-byte struct. The memset in `gr_init` covers `0x2a` dwords
(`mov ecx, 0x2a` at `0x505e03`). Its implicit constructor (`0x5081a0`) constructs two `Color` members, not
vector3s. Fields that `sum.symbols` lists as separate globals are members (**observed**):

| Address | Offset | Field |
|---|---|---|
| `0x25d8d34` / `0x25d8d38` | +0x4 / +0x8 | `max_w` / `max_h` |
| `0x25d8d3c` / `0x25d8d40` | +0xc / +0x10 | renderer (`0x66`-`0x68`) / window mode (200-202) |
| `0x25d8d48` | +0x18 | aspect |
| `0x25d8d58` | +0x28 | `far_zvalue`, written by `gr_init` only, never read |
| `0x25d8d5c`-`0x25d8d80` | +0x2c-+0x50 | clip origin, size and edges |
| `0x25d8d84` | +0x54 | current colour (`gr_color`) |
| `0x25d8d88` / `0x25d8d8c` | +0x58 / +0x5c | current bitmaps (`gr_texture`, `gr_texture_2`) |
| `0x25d8d90`-`0x25d8dbc` | +0x60-+0x8c | fog: flag, colour, distances, scaled ranges |
| `0x25d8dc0`-`0x25d8dcc` | +0x90-+0x9c | tint flag and factors |
| `0x25d8dd0` | +0xa0 | vsync (`gr_set_vsync`, read by `gr_d3d_flip` `0x53a718`, `gr_glide_flip` `0x54a762`) |

## Frame and state

| Address | Name | OpenGL back end | Notes |
|---|---|---|---|
| `0x505e00` | `gr_init(w, h, renderer, window_mode, hwnd, far_z, vsync)` | `gr_opengl_init` `0x54f6f0` | The 7th argument is stored to `gr_screen.vsync` (`0x505eaa`). Sets up the window, then the back end, fog off, gamma, clear, flip |
| `0x5067c0` | `gr_close` | `gr_opengl_close` `0x54f790` | Then `gr_close_fonts` |
| `0x506a20` | `gr_flip` | `gr_opengl_flip` `0x54f7c0` (`SwapBuffers`) | First flushes the sorted alpha polygons (`gr_poly_buffer_flush` `0x545d70`), resets the clip and draws the fade |
| `0x5069c0` | `gr_clear` | `gr_opengl_clear` `0x54f7e0` | Black `glRecti` over the clip rectangle, blending off |
| `0x506a50` | `gr_zbuffer_clear(enable)` | `gr_opengl_zbuffer_clear` `0x54f870` | Does nothing when `enable` is false |
| `0x506840` / `0x506970` / `0x5069a0` | `gr::set_clip` / `gr_get_clip` / `gr::reset_clip` | `gr_opengl_set_viewport` `0x54f8c0` | |
| `0x506170` / `0x506180` / `0x506950` | `gr_screen_width` / `gr_screen_height` / `gr_get_aspect` | | `gr_set_aspect` (`0x506960`) has no caller |
| `0x506b90`, `0x506c40`, `0x506c50` | `gr_set_color(r,g,b,a)`, `gr_set_alpha`, `gr_set_texture(bm, bm2)` | `gr_opengl_set_color` `0x54fe10` is empty | `gr_set_color` calls the back end only when the colour changes, and never for Glide. `gr_set_alpha` does not tell the back end |
| `0x506a90` | `gr_set_gamma(gamma)` | none | See [Gamma](#gamma) |
| `0x507db0` / `0x507f70` | `gr_set_fog(enable, r, g, b, near, far)` / `gr_set_tint` | `gr_opengl_set_fog` | Fog uses the `level_fog` / `alpha_fog` console variables. See [Fog](#fog) |
| `0x5079b0` | `gr_is_mode_supported(mode)` | (none) | OpenGL returns true for every mode, an unknown renderer false. The multitexture test is in the back ends (`0x53aad0`, `0x54a0b0`). Only caller: the lightmap mode test in `GameInit` and `options_set_lightmaps_off` |
| `0x5079f0` | `gr_get_video_memory_mb` | (none) | OpenGL returns the constant 8 (`0x507a03`, game bug K64) |
| `0x5080c0` | `gr_set_antialias(on)` | `0x550630` (empty) | Does nothing unless `gr_antialias_enabled` (`0x25d921c`) is set. Only `gr_d3d_init_device` sets it (`0x539dd5`) |
| `0x507fb0` | `gr_finish` | `glFinish` | Direct3D does nothing |
| `0x508160` | `gr::Mode::Mode` | | Packs six 5-bit fields, first argument lowest (`0x50816a`-`0x508191`), as Dash Faction's `gr::Mode` |

## Render modes

`gr::Mode(texture source, colour source, alpha source, alpha blend, zbuffer, fog)`. The 30 objects are built
by the initialisers at `0x505550`-`0x505ac0` (**observed**). 12 of them have no reader in the binary:
`0x25d8cdc`, `0x25d88a4`, `0x25d88cc`, `0x25d88a0`, `0x25d8d00`, `0x25d8cd8`, `0x25d88c8`, `0x25d8dd8`,
`0x25d8d18`, `0x25d8cd0`, `0x25d8ce8`, `0x25d8d14`. `gr_set_gamma` compares a pointer with `0x25d8cd0`, but only as
the end of the table before it (`cmp ecx, 0x25d8cd0` at `0x506b59`).

| Object | Mode | Used by (examples) |
|---|---|---|
| `gr_string_mode` `0x25d8df4` | (3, 2, 3, 3, 0, 3) | `gr_string` callers |
| `gr_mode_ui_bitmap` `0x25d8d08` | (1, 1, 0, 3, 0, 3) | `automap_draw_map`, `draw_loading_icon` |
| `gr_bitmap_wrap_mode` `0x25d88c4` | (1, 2, 3, 3, 0, 3) | `ui_bitmap_scaled`, `gr_bitmap` callers |
| `gr_line_mode` `0x25d8ce4` / `gr_rect_mode` `0x25d88ac` | (0, 0, 0, 3, 0, 3) | `gr_line`, `gr_rect`, `gr_draw_fade` |
| `gr_line_3d_mode` `0x25d88c0` / `gr_mode_flat_alpha` `0x25d8d28` | (0, 0, 0, 3, 4, 0) | 3D lines, `status_draw_model` |
| `gr_mode_mesh_alpha` `0x25d8df8` | (1, 2, 3, 3, 4, 0) | character meshes, level render |
| `gr_mode_lightmap_multitexture` `0x25d8d1c` | (4, 1, 3, 3, 4, 0) | tested with `gr_is_mode_supported` |
| `gr_mode_particle_alpha` `0x25d8df0` / `gr_mode_particle_additive` `0x25d8d24` | (2, 2, 3, 3, 1, 0) / (2, 2, 3, 2, 1, 3) | particles, snow, shadows |

The other named modes are in `symbols/sum.symbols`.

## Gamma

`gr_set_gamma` (`0x506a90`) builds two tables (**observed**):

- `gr_gamma_ramp[256]` (`0x25d8e0c`, ints): `255 * (i / 255) ^ (1 / gamma)`, clamped to 0..255
  (`_CIpow` at `0x506ad8`). A gamma above 1 brightens.
- `gr_color_gamma_lut[256]` (`0x25d88d0`, **floats**): `i` clamped to 0..255 (`0x506b1f`-`0x506b63`). This is an
  identity table. No gamma reaches it.

Then it calls the back end, for Direct3D and Glide only (`0x506b65`-`0x506b7a`). The options slider clamps the
value to 0.7..2.2 (`options_apply_gamma` `0x41f380`) and applies it at once. Accepting the options screen
(`options_apply_and_save` `0x41faa8`), `GameInit` (`0x43e141`) and each level load (`0x48661f`) apply
`config_gamma` (`0x60acf8`, default 1.0) again through `options_apply_detail` (`0x487341`). The only gamma
string in Sum.exe is `grGammaCorrectionValue`: no Windows gamma-ramp API is imported. The console command `gamma` only stores `config_gamma` (`0x43d538`). It takes effect at the next
`options_apply_detail`.

What the setting changes under each renderer (code **observed**, screen effect *inferred*):

| Renderer | Whole screen | Vertex colours | Textures |
|---|---|---|---|
| Direct3D | `gr_d3d_set_gamma` (`0x53ab20`) uploads the ramp with `IDirectDrawGammaControl::SetGammaRamp`. The interface exists only when the DirectDraw caps have bit `0x20000` (DDCAPS2_PRIMARYGAMMA, `0x539d8a`) | `gr_d3d_tmapper` and `gr_d3d_tmapper_multipass` pass per-vertex colours through `gr_gamma_ramp` (`0x536247`, `0x5366b6`). With a hardware ramp, lit polygons get gamma twice (K91) | 8-bit alpha bitmaps (format 2) get their alpha through the ramp at upload (`0x54c233`) |
| Glide | `gr_glide_set_gamma` (`0x54bac0`) calls `grGammaCorrectionValue(gamma)`, also at init and re-activation (`0x549ebb`, `0x54a050`) | `gr_glide_tmapper` uses the identity table, so no double gamma | format 2 alpha through the ramp (`0x5486bd`, `0x547d8c`) |
| OpenGL | **nothing**. `gr_set_gamma` has no OpenGL case | `gr_opengl_tmapper` converts colours through the identity table (`0x550150`-`0x550182`) | format 2 alpha through the ramp (`gl_convert_a8_to_rgba8888`, `0x54d5f3`) |

So under OpenGL the slider changes only the alpha of 8-bit-alpha textures. It changes them only for textures
uploaded after the change: `options_apply_gamma` does not flush the texture cache. The scene's brightness stays
the same (game bug K90). Font glyphs also go through the ramp (`gr_font_create` `0x5088c1`). The fonts are built
in `GameInit` (`0x43df76`-`0x43df9e`) before the configured gamma is applied (`0x43e110`), so they always use
gamma 1.0 on every renderer.

## Fog

`gr_set_fog` (`0x507db0`) stores the colour and the flag first. With fog on it limits `near` to half of `far`
(a quarter on Glide), then skips the rest if `near < 0` or `far < near` (`0x507e0d`-`0x507e2f`). Otherwise it
computes the scaled ranges: the far distance is multiplied by `Gr_level_fog_multiplier`, and the alpha fog
starts at `Gr_alpha_fog_ratio` of the way from near to far. The back end is always called with 1
(**observed**). On a skipped update the old distances stay active (game bug K92, latent).

Its callers pass the start and end distance from `0x59bd80` and `0x59bd7c` (`levelrender_fog_enable`
`0x482850`). These two globals are named the wrong way round in `sum.symbols`: `0x59bd7c` (`fog_near`) is the
end distance. It is passed as `far` and to `gr_set_far_clip` (`0x4829a3`), `set_fog` prints it as "far"
(`0x481beb`), and `fog_set_distances` keeps it above `0x59bd80` (`0x486b5f`-`0x486b7a`).

## Drawing

| Address | Name | Notes |
|---|---|---|
| `0x506f90` | `gr_bitmap(bm, x, y, mode)` | Clipped |
| `0x506cb0` | `gr_bitmap_ex(bm, x, y, w, h, sx, sy, mode)` | Clipped, re-clips after a negative source offset; only caller `gr_string` |
| `0x506ec0` | `gr_bitmap_stretched(bm, x, y, w, h, sx, sy, sw, sh, mode)` | Not clipped. Back end: `gr_opengl_bitmap` `0x54fa30` |
| `0x507100` | `gr_bitmap_float(bm, x, y, scale, angle, mode)` | A rotated quad through `gr_tmapper`; callers `automap_draw_map`, `worldmap_draw_location` |
| `0x507310` / `0x5077d0` | `gr_line` / `gr_rect(x, y, w, h, mode, z)` | `gr_line` clips in floats. `gr_rect` draws one quad through `gr_tmapper` |
| `0x507690` | `gr_circle(x, y, radius, mode)` | No caller |
| `0x507a20` / `0x507a80` | `gr_tmapper` / `gr_tmapper_multipass` | Direct3D and Glide draw all passes themselves. For OpenGL each pass is one `gr_tmapper` call |
| `0x507d30` | `gr_read_backbuffer(x, y, w, h, buf)` | Returns the pixel format: 3 (565) for Direct3D and Glide, 7 (8888) for OpenGL, 0 for an unknown renderer or a failed Direct3D lock (`0x53a82b`) |
| `0x5061f0` | `gr_screenshot(name, format, path_id)` | Reads the clip rectangle. Format 0 writes an RLE `.tga`, anything else a `.jpg` through `jpeg_write_file` (`0x5565e0`). A format other than 565, 888 or 8888 is fatal (`DebugError` at `0x506279`, game bug C91) |
| `0x508090` | `rgb_to_lightened_grey(r, g, b)` | `(avg + (255 - avg) * 0.6) / 255`; only caller `combat_spawn_blood` |

### Screen fades

`gr_fade_in(ms)` (`0x506630`) and `gr_fade_out(ms)` (`0x5065e0`) set a black overlay. `gr_draw_fade`
(`0x5066b0`) draws it on every `gr_flip`. The overlay advances by `reported_frametime` (**observed**), so fade
speed follows the [frame timing](timing-bugs.md). `ms < 1` jumps to the end state at once.

### Texture cache

- `gr_page_in(bm)` (`0x5069e0`) uploads a bitmap. `bm_load` and the level, item, character and vfx page-in
  code call it.
- `gr_mark_texture_dirty(bm)` (`0x506c70`) is called by the [bitmap manager](vsdk-bmpman.md)
  (`bm_set_reduced`, `bm_replace_file`) and by the save-game thumbnail code. `gr_tcache_free(bm)` (`0x508020`) is
  called by `bm_release`.
- `gr_tcache_flush` (`0x507fd0`) empties the cache and resets both current bitmaps to -1. The OpenGL version
  prints "WARNING: VRAM is at %d".

### Console commands

`level_fog` and `alpha_fog` set the two fog floats. `gr_show_stats` toggles `Gr_show_stats` (`0x25d9220`).
`antialiasing` (`0x505db0`) toggles its own flag `cmd_antialiasing_on` (`0x25d9224`), not the options setting
(game bug K93). It prints "antialiasing not supported on this card." unless the Direct3D device set
`gr_antialias_enabled`. Its help text is an empty `.bss` string (`0x25d9230`).

## Fonts (`gr_font.cpp`)

The `Font` record is 0x6C bytes and matches Dash Faction's `gr::Font` field for field (see `symbols/types.h`).
There are at most **6** fonts (Red Faction allows 12): `gr_fonts` at `0x25d9238`, with the count at `0x25d94c0`.

| Address | Name | Notes |
|---|---|---|
| `0x5081d0` | `gr_close_fonts` | Frees every font's tables; called by `gr_close` |
| `0x508510` | `gr_load_font(name, a2)` | The first font loaded becomes the default |
| `0x508560` | `gr_font_create` | "Too many fonts", "Font too big". Packs the glyphs into a 64, 128 or 256-pixel 4444 texture (`0x508662`-`0x50867b`). Glyph alpha goes through `gr_gamma_ramp` (`0x5088c1`) |
| `0x508970` | `gr_font_read_file` | "Trying to read a newer font version" |
| `0x508c60` | `gr_string(x, y, s, font, mode)` | `x = 0x8000` centres the text, as in Dash Faction |
| `0x508360` | `gr_get_string_size(&w, &h, s, len, font)` | |
| `0x508df0` / `0x509030` | `gr_fit_string_String` / `gr_font_fit_string` | How much of a string fits a width; the `String` version has no direct caller |
| `0x509130` | `gr_split_str` | Word wrap. French keeps the space before `:`, `?` and `!` (`gr_font_french_punct`, `0x5082d0`) |
| `0x508300` / `0x508330` | `gr_get_font_height` / `gr_get_font_height2` | |

## Keyboard (`0x5093a0`-`0x509f3f`)

The keyboard follows the usual Volition design. A window-message hook (`key_msg_handler` `0x509880`),
registered with `os_add_msg_handler` (`0x530c20`), calls `key_on_keydown` / `key_on_keyup`. These call
`key_mark(code, state, latency)` (`0x5093a0`), which:
- updates `key_down_state` and `key_press_counts` under `key_lock`;
- appends the key to a 16-entry queue that `key_inkey` (`0x509ce0`) reads.

Other entry points: `key_init` (`0x509770`), `key_flush` (`0x509ab0`), `key_down_time` (`0x509bb0`),
`key_to_ascii` (`0x509ca0`), `key_get_shift_status` (`0x509d90`) and the readers `key_is_down` (`0x509f00`),
`key_get_press_count` (`0x509e20`) and `key_queue_not_empty` (`0x509ec0`). The names follow that family's usual
API (*inferred*). Data follows the last function; `console.cpp` starts at `0x50a010`.

Two details matter for the planned
[custom key bindings](../roadmap.md#custom-key-bindings-for-player-actions):

- **Layout remapping:** `key_remap_layout` (`0x509560`) swaps scan codes according to `key_layout`
  (`0x25daa50`). Layout 2 is French AZERTY (A/Q, Z/W, M); layout 3 is German QWERTZ (Y/Z). Bindings therefore
  see the same physical keys on every layout.
- **NumLock:** on Windows 9x only, `key_numlock_fix` (`0x509a00`) turns NumLock off at start-up.

## Mouse (`0x500a30`-`0x501468`)

{: .important }
Corrected 2026-10-15: the module ends at `0x501468` (the end of `clamp_int`, `0x501450`), not `0x50144f`.
v1.40 never uses DirectInput, and the mouse-look re-centring is dead code (see below).

The mouse module sits just before `frametime.cpp`. It starts with two static initialisers for the button
records (`0x500a30`, `0x500a60`). The `vsdk` block therefore starts a little earlier than the
[source map](source-map.md) estimated (`0x501000`).

| Address | Name | Notes |
|---|---|---|
| `0x500de0` | `mouse_init(w, h)` | Centres the pointer and registers a message handler with `os_add_msg_handler` |
| `0x500e90` | `mouse_di_init` | Returns -1 at once while `mouse_use_cursor` is 1, which is always (see below). The unused rest: `DirectInputCreateA` version 0x700, falling back to 0x300, then the system mouse, non-exclusive and foreground |
| `0x500a90` | `mouse_update` | Called every frame: latches the buttons, then reads movement |
| `0x500cd0` | `mouse_di_read` | DirectInput `GetDeviceState`; re-acquires after focus loss (`Sleep(500)`) |
| `0x500c80` | `mouse_set_pos(x, y)` | `ClientToScreen` + `SetCursorPos`, only while the window is in the foreground |
| `0x501090` | `mouse_flush` | |
| `0x501100`-`0x501440` | button state and queries | `mouse_mark_button`, `mouse_get_pos`, `mouse_is_down`, `mouse_was_pressed`, `mouse_get_delta`, `mouse_keep_centered_enable`, ... |

How `mouse_update` reads movement:
- **Cursor mode, always:** `mouse_use_cursor` (`0x5a5734`) is 1 in `.data`. The only store is
  `mouse_init` setting it to 1 again when `mouse_di_init` fails (`0x500e59`), so `mouse_di_init` returns -1
  before `DirectInputCreateA` (`0x500e90`..`0x500e9b`) and the DirectInput path (`mouse_di_read`,
  `0x500cd0`) never runs. **Observed.** The position comes from `GetCursorPos` + `ScreenToClient`, is clamped
  to the game resolution with `clamp_int`, and when the clamp changed it the OS cursor is moved back inside
  with `mouse_set_pos` (only while the window is in the foreground, `0x500b56`..`0x500ba6`).
- **Mouse look (dead):** the re-centring at `0x500c04` needs both `keep_mouse_centered` (`0x257f031`) and
  `mouse_hidden` (`0x257f030`). Their setters (`mouse_set_hidden` `0x501400`, `mouse_set_visible`
  `0x501410`, `mouse_keep_centered_enable` `0x501420`) have no callers and no pointer references, so the
  pointer is never warped back. **Observed.**
- **Double clicks** are the game's own, 400 ms per button (`mouse_double_click_time`, `0x5a5738`); the
  `WM_*BUTTONDBLCLK` messages are ignored (handler jump table `0x500fec`).

[SummonerFix](../summonerfix/index.md) relates to this module in two ways:
- It **is** the `dinput.dll` that Windows loads for Sum.exe's static import of `DINPUT.dll!DirectInputCreateA`
  (IAT `0x57e02c`, thunk `0x57b4b0`). Its `DirectInputCreateA` forwards to the system DLL, but v1.40 never
  calls it (`mouse_di_init` returns first).
- Its mouse mapping hooks the `ScreenToClient` call in `mouse_update` and the `ClientToScreen` call in
  `mouse_set_pos`.

`os_get_main_wnd` (`0x530c00`) and `os_foreground` (`0x530c10`) return the Dash Faction globals `main_wnd`
(`0x2cbff10`) and `is_main_wnd_active` (`0x2cbff1c`).

## Still to do

- The Direct3D and Glide back ends beyond the dispatch targets and the parts above.
- Why `LoadSettings` passes the vsync option as `gr_init`'s sixth argument (as 0.0 or 1.0) and a constant 0
  as the vsync argument (`0x43db08`-`0x43db4d`, game bug V90). `options_apply_detail` sets the real value
  later, so nothing shows.

## Evidence

| Claim | Addresses |
|---|---|
| Unit start, initialisers | `0x505550`-`0x505ac0`; table `0x5887ec`-`0x588874`; `init_gr_screen` `0x505af0` |
| `gr_screen` size and fields | `gr_init` `0x505e03`-`0x505eb0`; `gr__Screen__Screen` `0x5081a0` |
| vsync argument | `gr_init` `0x505e52`, `0x505eaa`; `gr_set_vsync` `0x506164`; `options_apply_detail` `0x48734d`-`0x48735a` |
| Identity colour table | `gr_set_gamma` `0x506b12`-`0x506b63` |
| Gamma per renderer | `0x506b65`-`0x506b7a`; `gr_d3d_set_gamma` `0x53ab27`-`0x53ab6d`; `gr_d3d_init_device` `0x539d8a`-`0x539dc4`; `gr_glide_set_gamma` `0x54bac0`; `gr_opengl_tmapper` `0x550150`-`0x550182` |
| Gamma on textures and fonts | `0x54c1a2`, `0x54c233`; `0x5486bd`; `0x54d5f3`; `gr_font_create` `0x5088c1`; `GameInit` `0x43df76`, `0x43e110` |
| Slider range | `options_apply_gamma` `0x41f380`-`0x41f3c1` |
| Screenshot format | `gr_screenshot` `0x50625d`-`0x506279`; `screenshot_save` `0x432610`, `0x43263c` |
| `gr_is_mode_supported` | `0x5079b0`-`0x5079c5`; `0x53aad0`; `0x54a0b0` |
| Fog skip | `gr_set_fog` `0x507dce`-`0x507e2f` |
| Fog end is `0x59bd7c` | `levelrender_fog_enable` `0x482997`-`0x4829c6`; `cmd_set_fog` `0x481bdf`-`0x481bf7`; `fog_set_distances` `0x486b5f`-`0x486b7a` |
| Anti-aliasing flag | `gr_d3d_init_device` `0x539dd5`; `gr_set_antialias` `0x5080c0`; `cmd_antialiasing` `0x505db0`-`0x505dda` |

## Changes (2026-10-15)

Checked against `src/decomp/vsdk/gr.cpp` and the disassembly. The unit range was `0x505e00`-`0x5081d0`; it
starts at `0x505550` with the mode initialisers and the console commands. `gr_screenshot`'s other writer is
`jpeg_write_file`. `gr_set_gamma` is not "D3D and Glide only" in effect: it fills two tables used by all three
renderers, and the OpenGL renderer gets no gamma (new [Gamma](#gamma) section). `gr_clear`, `gr_flip`,
`gr_set_color` and `gr_set_alpha` notes were completed. New sections: layout, `gr_screen`, render modes, fog,
console commands. `0x5079b0`, `0x5079f0` and `0x508df0` are no longer open. The keyboard ends at `0x509f3f`,
not `0x509f00`. The mouse module starts at `0x500a30`; the links to `#mouse-0x500a90-0x501090` in
`controls.md` and `summonerfix/index.md` need the new anchor `#mouse-0x500a30-0x50144f` (since corrected again to
`#mouse-0x500a30-0x501468`: the module ends with `clamp_int`).
