---
title: "vsdk: graphics (gr, gr_font)"
parent: Sum.exe
nav_order: 5.9
---

# vsdk: graphics (`gr.cpp`, `gr_font.cpp`) and keyboard

**Summary:** Phase 1, module 5. `vsdk\gr\gr.cpp` (`0x505e00`-`0x5081d0`) is a thin
dispatcher over three renderers. Each function checks `gr_screen.mode` and calls the
Direct3D (0x66), Glide (0x67) or OpenGL (0x68) back end. `gr_font.cpp`
(`0x5081d0`-`0x5093a0`) draws bitmap fonts. The keyboard module follows at
`0x5093a0`-`0x509f00`. About 125 functions and globals are named. Most names and signatures
match [Dash Faction](../reference/engine-references.md)'s `rf::gr` API, which descends from
this code (confidence **O**).

The dispatchers also named their back-end targets, including the OpenGL functions that
[SummonerFix](../summonerfix/index.md) hooks.

## Frame and state

| Address | Name | OpenGL back end | Notes |
|---|---|---|---|
| `0x505e00` | `gr_init` | `gr_opengl_init` | Already named |
| `0x5067c0` | `gr_close` | `gr_opengl_close` `0x54f790` | Then `gr_close_fonts` |
| `0x506a20` | `gr_flip` | `gr_opengl_flip` `0x54f7c0` (`SwapBuffers`) | Resets the clip and draws the fade first |
| `0x5069c0` | `gr_clear` | `gr_opengl_clear` `0x54f7e0` | Black `glRecti` over the clip rectangle |
| `0x506a50` | `gr_zbuffer_clear(enable)` | `gr_opengl_zbuffer_clear` `0x54f870` | |
| `0x506840` / `0x506970` / `0x5069a0` | `gr::set_clip` / `gr_get_clip` / `gr::reset_clip` | `gr_opengl_set_viewport` | |
| `0x506170` / `0x506180` / `0x506950` | `gr_screen_width` / `gr_screen_height` / `gr_get_aspect` | | |
| `0x506b90`, `0x506c40`, `0x506c50` | `gr_set_color(r,g,b,a)`, `gr_set_alpha`, `gr_set_texture(bm, bm2)` | `gr_opengl_set_color` is empty | |
| `0x506a90` | `gr_set_gamma` | (D3D and Glide only) | Builds `gr_gamma_ramp[256]` (`0x25d8e0c`) |
| `0x507db0` / `0x507f70` | `gr_set_fog` / `gr_set_tint` | | Fog uses the `level_fog`/`alpha_fog` console variables |
| `0x508160` | `gr::Mode::Mode` | | Packs six 5-bit render-state fields, as Dash Faction's `gr::Mode` |

## Drawing

| Address | Name | Notes |
|---|---|---|
| `0x506f90` | `gr_bitmap(bm, x, y, mode)` | Clipped |
| `0x506cb0` | `gr_bitmap_ex(bm, x, y, w, h, sx, sy, mode)` | Clipped; used by `gr_string` |
| `0x506ec0` | `gr_bitmap_stretched(bm, x, y, w, h, sx, sy, sw, sh, mode)` | Back end: `gr_opengl_bitmap` `0x54fa30` |
| `0x507310` / `0x5077d0` | `gr_line` / `gr_rect` | |
| `0x507a20` / `0x507a80` | `gr_tmapper` / `gr_tmapper_multipass` | Polygons |
| `0x507d30` | `gr_read_backbuffer(x, y, w, h, buf)` | Returns the pixel format |
| `0x5061f0` | `gr_screenshot(name, format, path_id)` | Reads the back buffer and writes a TGA (or another format through `0x5565e0`) |

### Screen fades

`gr_fade_in(ms)` (`0x506630`) and `gr_fade_out(ms)` (`0x5065e0`) set a black overlay
that `gr_draw_fade` (`0x5066b0`) draws on every `gr_flip`. The overlay advances by
`reported_frametime` (*inferred* from the code), so fade speed follows the
[frame timing](timing-bugs.md).

### Texture cache

- `gr_page_in(bm)` (`0x5069e0`) uploads a bitmap. `bm_load` calls it when preloading.
- `gr_mark_texture_dirty(bm)` (`0x506c70`) and `gr_tcache_free(bm)` (`0x508020`) are called
  by the [bitmap manager](vsdk-bmpman.md).
- `gr_tcache_flush` (`0x507fd0`) empties the cache. The OpenGL version prints
  "WARNING: VRAM is at %d".

## Fonts (`gr_font.cpp`)

The `Font` record is 0x6C bytes and matches Dash Faction's `gr::Font` field for field (see
`symbols/types.h`). There are at most **6** fonts (Red Faction allows 12): `gr_fonts` at
`0x25d9238`, with the count at `0x25d94c0`.

| Address | Name | Notes |
|---|---|---|
| `0x508510` | `gr_load_font(name, a2)` | The first font loaded becomes the default |
| `0x508560` | `gr_font_create` | "Too many fonts", "Font too big"; packs the glyphs into a 64/128/256-pixel 4444 texture |
| `0x508970` | `gr_font_read_file` | "Trying to read a newer font version" |
| `0x508c60` | `gr_string(x, y, s, font, mode)` | `x = 0x8000` centres the text, as in Dash Faction |
| `0x508360` | `gr_get_string_size(&w, &h, s, len, font)` | |
| `0x509130` | `gr_split_str` | Word wrap. French keeps the space before `:`, `?` and `!` (`gr_font_french_punct`, `0x5082d0`) |
| `0x508300` / `0x508330` | `gr_get_font_height` / `gr_get_font_height2` | |

## Keyboard (`0x5093a0`-`0x509f00`)

The keyboard follows the usual Volition design. A window-message hook, registered with
`os_add_msg_handler` (`0x530c20`), calls `key_on_keydown` / `key_on_keyup`. These call
`key_mark(code, state, latency)` (`0x5093a0`), which:
- updates `key_down_state` and `key_press_counts` under `key_lock`;
- appends the key to a queue that `key_inkey` (`0x509ce0`) reads.

Other entry points: `key_init` (`0x509770`), `key_flush` (`0x509ab0`), `key_down_time`
(`0x509bb0`) and `key_to_ascii` (`0x509ca0`). The names follow that family's usual API
(confidence **I**). The three readers named earlier (`key_is_down`, `key_get_press_count`,
`key_queue_not_empty`) belong to the same module.

Two details matter for the planned
[custom key bindings](../roadmap.md#custom-key-bindings-for-player-actions):

- **Layout remapping:** `key_remap_layout` (`0x509560`) swaps scan codes according to
  `key_layout` (`0x25daa50`). Layout 2 is French AZERTY (A/Q, Z/W, M); layout 3 is German
  QWERTZ (Y/Z). Bindings therefore see the same physical keys on every layout.
- **NumLock:** on Windows 9x only, `key_numlock_fix` (`0x509a00`) turns NumLock off at
  start-up.

## Mouse (`0x500a90`-`0x501090`)

The mouse module sits just before `frametime.cpp`. The `vsdk` block therefore starts a little
earlier than the [source map](source-map.md) estimated (`0x501000`).

| Address | Name | Notes |
|---|---|---|
| `0x500de0` | `mouse_init(w, h)` | Centres the pointer and registers a message handler with `os_add_msg_handler` |
| `0x500e90` | `mouse_di_init` | `DirectInputCreateA` version 0x700, falling back to 0x300. Opens the system mouse, non-exclusive and foreground |
| `0x500a90` | `mouse_update` | Called every frame: latches the buttons, then reads movement |
| `0x500cd0` | `mouse_di_read` | DirectInput `GetDeviceState`; re-acquires after focus loss (`Sleep(500)`) |
| `0x500c80` | `mouse_set_pos(x, y)` | `ClientToScreen` + `SetCursorPos`, only while the window is in the foreground |
| `0x501090` | `mouse_flush` | |

How `mouse_update` reads movement:
- **DirectInput mode:** the mouse moves by DirectInput deltas.
- **Cursor mode:** used if DirectInput fails (`mouse_use_cursor`, `0x5a5734`). The position
  comes from `GetCursorPos` + `ScreenToClient`.
- **Mouse look:** optionally warps the pointer back to the centre and reports
  `mouse_dx`/`mouse_dy`.

[SummonerFix](../summonerfix/index.md) relates to this module in two ways:
- It **is** the `dinput.dll` that `mouse_di_init` loads. Its `DirectInputCreateA` forwards to
  the system DLL.
- Its mouse mapping hooks the `ScreenToClient` call in `mouse_update` and the
  `ClientToScreen` call in `mouse_set_pos`.

`os_get_main_wnd` (`0x530c00`) and `os_foreground` (`0x530c10`) return the Dash Faction
globals `main_wnd` (`0x2cbff10`) and `is_main_wnd_active` (`0x2cbff1c`).

## Still to do

- The Direct3D and Glide back ends beyond the dispatch targets (named **I** only).
- `0x5079b0`/`0x5079f0` (back-end capability queries) and `0x508df0` (a string
  helper built on another class).
