---
title: Vulkan renderer (L4)
parent: Projects
nav_order: 1.5
---

# The Vulkan renderer (rung L4, started 2026-10-26)

> **State on 2026-10-26:** on the branch `l4-renderer`, the game plays under Vulkan (`[Display] Renderer=Vulkan`):
> menus, levels, characters, effects, saves with thumbnails, brightness and the FAST badge, with no validation errors.
> The main menu, the load screen and the brightness pass match OpenGL exactly in agent captures (half size: every second
> pixel each way), at 1024x768 and at 1366x768; levels match within the run-to-run noise of OpenGL itself. M0-M4 done; M5 (the parity
> run over every level, frame times) next.

The first module of L4, the idiomatic rewrite ([modern port](modern-port.md)). The 64-bit player build gets a
Vulkan renderer beside the game's own OpenGL one. OpenGL stays the reference: every milestone is compared with it.

## Why Vulkan

The user chose Vulkan on 2026-10-26, over Direct3D 11 (my first suggestion) and OpenGL 3.3, so that the renderer is
current and ready for upscaling and variable refresh rate. What each of those needs (Inferred, from the vendors' SDK
documentation, not tried):
- **Variable refresh rate:** any current API with a flip-model or Vulkan swapchain and vsync off. Not a reason
  for one API over another, but Vulkan has it.
- **Upscaling:** FSR 1 runs on anything; DLSS supports Direct3D 11, 12 and Vulkan; FSR 2/3 and XeSS mainly Direct3D
  12 and Vulkan. Vulkan keeps all of them open. The temporal ones also need motion vectors and depth from the
  renderer, which a 2000-era engine does not make; that work is the same whatever the API.
- **Platforms:** Vulkan also runs natively on Linux and the Steam Deck.

## Design

### A fourth back end in OpenGL's slot

The engine already has three back ends (`vsdk/gr_direct3d.cpp`, `gr_glide.cpp`, `gr_opengl.cpp`). The game chooses
one by `gr_screen.mode` in about 45 places, most of them in `vsdk/gr.cpp`, whose bytes are part of the VC6 match.
Vulkan does not get a mode of its own (Observed in the code, 2026-10-26):
- With `[Display] Renderer=Vulkan`, the registry answer still selects OpenGL's mode (0x68), so **no dispatch site
  changes**, and everything that tests for OpenGL applies to Vulkan too: widescreen, hi-res, the movie path, vsync,
  the start-up choice.
- In the 64-bit build only (`MODERN_BUILD` and `_WIN64`), the OpenGL back end's entry points are renamed in their
  definitions (`<name>_gl`, `include/modern/gl_redirect.h`), and `src/decomp/modern/vk_dispatch.cpp` defines the
  original names: each calls the Vulkan version while the Vulkan back end is active, else the OpenGL one.
- The choice is made once, at the back end's first call (`gr_opengl_init_context`): Vulkan if it is asked for and a
  Vulkan 1.3 GPU starts, else OpenGL as before.

The search the advisor suggested decided this: OpenGL calls outside the OpenGL back end (2026-10-26, Observed). Few
places draw with OpenGL themselves, so redirecting is smaller than a fourth mode:

| Where | What | In Vulkan |
|---|---|---|
| `vsdk/gr_opengl.cpp` | the back end: 25 entry points (state, texture cache, bitmaps, lines, polygons, read-back, fog) and the two init calls | redirected |
| `game/levelrender.cpp` | `levelrender_draw_list_opengl`: **the level geometry**, drawn with OpenGL arrays | redirected (renamed around its definition) |
| `vsdk/unnamed_ca_mesh_0x50af80.cpp` | `gr_opengl_render_character_mesh`: the characters | redirected (the same) |
| `modern/display.cpp`, `modern/gamma.cpp` | our 64-bit host: the window mapping, the bars, captures, thumbnails, brightness, the FAST badge | Vulkan versions (M4) |
| `experimental/display.cpp` | vsync (`wglSwapIntervalEXT`) | the present mode |
| `vsdk/gr.cpp` | one `glFinish` (`gr_finish`) | a wait on the GPU |

**The interface to reproduce** (from `gr_opengl.cpp`, Observed):
- No shaders, no lighting in OpenGL: the game lights on the CPU and sends screen-space vertices (`glOrtho` over the
  clip rectangle). Positions are game pixels; texture coordinates are projective (`s*w, t*w, 0, w`) for perspective.
- Depth: `glOrtho(..., -1, 1)` makes window depth `(1 - z) / 2`; nearer vertices have larger z. Depth test
  `LEQUAL`, write per mode.
- Fog: linear, on the eye distance `|z|`, between `gr_vertex_depth` of the fog's near and far (the far end is the
  smaller value, so near objects stay clear).
- Alpha test always on (`GREATER 0`); blending one of six `glBlendFunc` pairs per `gr::Mode`.
- Two texture stages: stage 0 modulated, or `GL_DECAL` under stage 1 (modulated) for lightmapped surfaces.
- Textures clamp with `GL_CLAMP` (the border colour shows at the edges under linear filtering), and the packed 16-bit
  formats (RGBA 4444, 5551) for 8888, 4444, 1555 and 888 bitmaps: the Vulkan cache quantises the same way, so
  the pictures can be compared.
- `GL_POLYGON` fans of up to 30 vertices; lines as `GL_LINE_STRIP`.
- The read-back flips rows (OpenGL's origin is the bottom left); Vulkan's is the top left, so no flip.

### The frame

The game's picture is drawn straight into the swapchain image, mapped into the window as the 64-bit host maps the
OpenGL viewport (`modern_display_map`: centred, aspect-correct, the 1024-wide menus centred). An off-screen image
and a final scaling pass come with upscaling, not before (the advisor: not needed for parity, and it complicates
the read-back and the brightness).

Vulkan 1.3: dynamic rendering, synchronization2 and extended dynamic state (depth test, depth write, compare and
topology are dynamic, so pipelines differ only by blend mode and primitive class). Textures through one descriptor
array (descriptor indexing); the draw's texture indices go in push constants. Texture uploads and read-backs inside
a frame suspend the rendering pass, copy, and resume, so they are ordered with the draws as under OpenGL.

## Milestones

| | Milestone | Done when | State |
|---|---|---|---|
| M0 | Build wiring: the SDK through `VULKAN_SDK`, shaders compiled by `glslc` and embedded, `Renderer=Vulkan`, the redirect | it builds; the VC6 sweep keeps 5,613; OpenGL plays as before | **done 2026-10-26** |
| M1 | Device, swapchain, frame, resize; validation in test runs | a window clears with no validation errors | **done 2026-10-26** |
| M2 | 2D: bitmaps, lines, colours, tint, texture cache | the menus and the interface match OpenGL (image comparison with a tolerance) | **done 2026-10-26**: identical in half-size captures |
| M3 | 3D: polygons, depth, fog, blend modes, two texture stages, level geometry, characters | levels match OpenGL per level | **done 2026-10-26** for 3 levels: within OpenGL's own run-to-run noise; every level in M5 |
| M4 | Movies, read-back (thumbnails, screenshots, captures), brightness, widescreen, hi-res, the FAST badge | everything the 64-bit build does under OpenGL | **done 2026-10-26** (widescreen: 1366x768 with centred menus and the hi-res interface switch; no texture pack tried) |
| M5 | Parity: the 48-level soak and the save check under Vulkan, frame times, a clean validation log | merge into `main` | |

After M5: present-mode options for variable refresh, an off-screen image for upscaling (FSR 1 first), full-colour
textures as an option.

## Settings

| Key | Values | Meaning |
|---|---|---|
| `[Display] Renderer` | `OpenGL` (default), `Vulkan` | the back end; Vulkan falls back to OpenGL when it cannot start |
| `[Display] VulkanPresent` | `Auto` (default), `Fifo`, `Mailbox`, `Immediate` | the swapchain's present mode. `Auto` follows `[Experimental] VSync` as the OpenGL build does: Fifo with vsync, else Mailbox, or Immediate where Mailbox is not offered (this PC, 2026-10-26) |
| `[Debug] VulkanValidation` | `0` (default), `1` | the Khronos validation layer (Vulkan SDK), its messages in `summonerfix.log` |

## Build

`node tools/link-build.mjs --config experimental --compiler clang --arch x64` with `VULKAN_SDK` set to the LunarG
Vulkan SDK (1.4.363.0 used; [sources](../reference/sources.md#vulkan-sdk-the-vulkan-renderer)):
- its `Include\` for the Vulkan headers (`modern/vk_*.cpp`, defined `MODERN_VULKAN`);
- its `glslc` compiles `src/decomp/modern/shaders/*.vert|frag` to SPIR-V numbers in `<obj>/gen/*.inc`, embedded through
  `<obj>/gen/vk_shaders.h`;
- the SDK path is part of the build signature, so setting or changing it recompiles every unit once.

`vulkan-1.dll` is the graphics driver's and is loaded at run time: the exe starts on a PC without Vulkan. Without
`VULKAN_SDK` the back end is not built and `Renderer=Vulkan` logs that and uses OpenGL. The 32-bit builds never see
any of this (they compile with VC6's headers).

## Log

### M0, 2026-10-26: the build wiring

- `include/modern/gl_redirect.h` (the entry table and the renames), `modern/vk_dispatch.cpp` (the original names,
  the choice), `include/modern/vk.h`, `modern/vk_backend.cpp` (M0: loads the loader, creates an instance, logs the
  GPUs and shaders, returns false), `modern/shaders/gr.vert`, `gr.frag`, the `VULKAN_SDK` step in link-build.
- In the game source, under `VK_REDIRECT` only (64-bit `MODERN_BUILD`): the rename at the top of
  `vsdk/gr_opengl.cpp`, around `levelrender_draw_list_opengl` (`game/levelrender.cpp`) and around
  `gr_opengl_render_character_mesh` (`vsdk/unnamed_ca_mesh_0x50af80.cpp`); `gr_opengl.h` includes the table header.

Observed:
- VC6 sweep (`VC6_STRICT=1`): 5,613 matched, 0 lost.
- The 64-bit experimental build: 259 units, unresolved 0, multiply defined 0; the new units without warnings.
- With `Renderer=Vulkan` and `VulkanValidation=1` in the test folder: `vulkan: GPU 0: Intel(R) Arc(TM) 140V GPU (8GB),
  Vulkan 1.4.328, validation on`, the two shaders (1,316 and 988 bytes), then `renderer: Vulkan did not start; using
  OpenGL`, and the main menu at 120 fps.
- With `Renderer=OpenGL`, through the redirect: `link-soak --only Wolong,sewer,masad` 3 of 3 levels in play (level,
  characters, golem and firewall drawn), 82-120 fps.

### M1-M4, 2026-10-26: the game under Vulkan

The units (`src/decomp/modern/`, listed in `vk_internal.h`):
- `vk_device.cpp`: the loader, instance (validation with `[Debug] VulkanValidation=1`), device (Vulkan 1.3), swapchain,
  depth buffer, two frames in flight, resize, read-back, transfers outside the rendering pass.
- `vk_draw.cpp`: `gr_opengl_set_mode`'s state replicated switch by switch (a value a switch does not list keeps the
  previous state, as under OpenGL), 12 pipelines (6 blend pairs by triangles / lines), the clip rectangle's projection
  taken at `set_viewport` as OpenGL takes it, bitmaps, lines, polygons, clears.
- `vk_tcache.cpp`: the texture cache with OpenGL's bookkeeping, including what OpenGL keeps per texture object (the
  binding of each stage across frames, each texture's wrap mode); images in 32 MB blocks; replaced textures freed
  once no frame in flight uses them.
- `vk_scene_level.cpp`, `vk_scene_mesh.cpp`: `levelrender_draw_list_opengl` and `gr_opengl_render_character_mesh`
  followed statement by statement, writing the same arrays and drawing them through Vulkan (written by two agents,
  reviewed and merged).
- `vk_post.cpp`: brightness (the frame copied and drawn back through `pow(c, 1/gamma)`) and the FAST badge.
- `tools/imgdiff.mjs`: the image comparison ([Tools](../tools.md)).

Observed (this PC: Intel Arc 140V, driver 101.8331, 2560x1440 window, game 1024x768):
- **Textures as OpenGL stores them:** `gl_packed_pixels_supported` reads 0 in the running OpenGL build (the version
  string has no "1.2"), so OpenGL uploads 8 bits per channel; the Vulkan cache does the same.
- **Identical in the captures:** main menu, load screen with a save's thumbnail, and the menu at gamma 2.0: `imgdiff`
  maximum difference 0 against OpenGL. Each pair was checked to differ from the other screens (55-74%). Agent
  captures are half size by point sampling (every second pixel each way), so this covers a quarter of the window's
  pixels, the same quarter for both renderers; full-size captures come with M5.
- **Levels:** Wolong, sewer and masad in play under Vulkan (`link-soak`), level, characters, golem, firewall and
  effects drawn, 0 validation errors. Against the OpenGL captures: masad (paused by its tutorial) 0.8% beyond a
  one-pixel shift, all of it moving parts (the selection ring, the PAUSED sign, a sparkle trail); sewer 11% and
  Wolong 20%, where fire and fog move between captures. The still geometry differs only at edges.
- **Read-back:** a save made under Vulkan shows the right thumbnail; agent captures come from the Vulkan frame.
- **The badge** ("FAST x3") and **the movies** (the intro's frames through `gr_bitmap`) draw.
- **Frame rate:** 120 fps (the game's cap), with `VulkanPresent=Auto` choosing Immediate; the soak's minimum was 43 fps
  with the validation layer on (OpenGL 82). Timing without the layer is M5's.

Bugs found and fixed on the way:
- The pipelines were made before the swapchain's format was known; the validation layer reported every draw.
- A combined depth-stencil format needs both aspects in barriers; a depth-only format is used now.
- The texture cache was not set up: OpenGL's `init_context` does it the first time, and the Vulkan one did not.
- A texture freed between frames went with the next frame slot and was destroyed while the frame just submitted
  could still read it (the validation layer caught it when the intro movie was skipped).
- With Fifo (vsync) the game ran at 60 fps where the OpenGL build runs at 120, so the menus reacted more slowly.

Found while porting (Inferred from the code, not run; to check before they are called bugs):
- `gr_opengl_render_character_mesh` writes 3 indices per visible triangle into `ca_mesh_indices` (2,000 entries), so
  a sub-mesh with more than 666 visible triangles runs past it. In Sum.exe the next variable is unused; in the 64-bit
  build the layout differs, so the OpenGL path could overwrite live data. The Vulkan port keeps its own larger list.
- `levelrender_draw_list_opengl`'s plain path lets its vertex count grow over all groups of an object, and checks only
  `n < 3` after clipping (a 9-sided result would read past `levelrender_fan_tables`).

### Widescreen and the noise floor, 2026-10-26

At the owner's settings (`GameWidth=1366`, `[Display] CenterMenus` 1, `[Experimental] HiResInterface=1`; window
2560x1440), Observed:
- Main menu (the 1024-wide layout centred) and load screen: identical to OpenGL in the captures.
- masad and Wolong in play under both renderers. A first comparison of masad differed by 3.2% beyond a one-pixel
  shift, with the whole 3D view speckled. To tell renderer from run, each renderer was run twice:

| masad, % beyond a 1-px shift | |
|---|---|
| OpenGL run 1 against OpenGL run 2 | 1.28 |
| Vulkan run 1 against Vulkan run 2 (validation on) | 0.48 |
| OpenGL against Vulkan, validation on | 2.2-3.2 |
| OpenGL runs 1 and 2 against Vulkan, validation off | 0.61 and 1.19 |

  With the validation layer the game runs slower while the level starts, and the camera settles elsewhere (the
  game's camera follows the frame time). Without it Vulkan is within OpenGL's own run-to-run noise. Comparisons of
  levels are made with validation off; validation runs count errors.

Checked, from the link map (`Sum-exp-clang-x64.map`): `ca_mesh_indices` (4,000 bytes) is followed by
`ca_bss_26172c8`, 16,000 bytes that nothing references, so the OpenGL path's overrun past 666 visible triangles
reaches live data (`ca_gl_mesh_texcoords`) only past 3,333 visible triangles in one sub-mesh. Not a live risk in
the installed build; it depends on the link order, so it stays on the list.

Also on 2026-10-26: VC6 sweep 5,613 matched, 0 lost; the 32-bit clang experimental build links (6 known warnings).
