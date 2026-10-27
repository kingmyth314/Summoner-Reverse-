---
title: Vulkan renderer (L4)
parent: Projects
nav_order: 1.5
---

# The Vulkan renderer (rung L4, started 2026-10-26)

> **State on 2026-10-27:** in `main` (merged from the branch `l4-renderer` on 2026-10-27), the game plays under Vulkan (`[Display] Renderer=Vulkan`):
> menus, levels, characters, effects, saves with thumbnails, brightness and the FAST badge, with no validation errors.
> The main menu, the load screen and the brightness pass match OpenGL exactly in agent captures (half size: every second
> pixel each way), at 1024x768 and at 1366x768; levels match within the run-to-run noise of OpenGL itself. M0-M5 done:
> all 48 levels of the soak behave as under OpenGL (the original's three crashes), with 0 validation errors and frame
> rates at least as good; saves are unaffected; minimise, focus loss, window resizing and the intro movie work. Merged
> into `main` on 2026-10-27 (the earlier `main` is tagged `main-before-l4`).

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
| M3 | 3D: polygons, depth, fog, blend modes, two texture stages, level geometry, characters | levels match OpenGL per level | **done 2026-10-26** for 3 levels: within OpenGL's own run-to-run noise; every level 2026-10-27 (M5): within the noise on 38 of 47, the rest explained or marginal |
| M4 | Movies, read-back (thumbnails, screenshots, captures), brightness, widescreen, hi-res, the FAST badge | everything the 64-bit build does under OpenGL | **done 2026-10-26** (widescreen: 1366x768 with centred menus and the hi-res interface switch; no texture pack tried) |
| M5 | Parity: the 48-level soak and the save check under Vulkan, frame times, a clean validation log | merge into `main` | **done 2026-10-27**: 45 of 48 levels (the original's three crashes), 0 validation errors, every level's picture within OpenGL's run-to-run noise, full-size menus identical, saves unaffected, minimise / focus / resize / movies |

After M5: present-mode options for variable refresh, an off-screen image for upscaling (FSR 1 first), full-colour
textures as an option.

## Settings

| Key | Values | Meaning |
|---|---|---|
| `[Display] Renderer` | `OpenGL` (default), `Vulkan` | the back end; Vulkan falls back to OpenGL when it cannot start |
| `[Display] VulkanPresent` | `Auto` (default), `Fifo`, `Mailbox`, `Immediate` | the swapchain's present mode. `Auto` follows `[Experimental] VSync` as the OpenGL build does: Fifo with vsync, else Mailbox, or Immediate where Mailbox is not offered (this PC, 2026-10-26) |
| `[Debug] VulkanValidation` | `0` (default), `1` | the Khronos validation layer (Vulkan SDK), its messages in `summonerfix.log` |
| `[Debug] VulkanTiming` | `0` (default), `1` | each frame slower than 25 ms in `summonerfix.log`: the textures made in it (count, size, time) and the waits for the GPU (count, time); M5, 2026-10-26. Since 2026-10-27 also the waits for the frame slot's fence, the swapchain image and the present, and every 240 frames a `present:` line: the time between presents (mean, sd, min, max, frames over 1.5 times the mean) |

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
  with the validation layer on (OpenGL 82). Timing without the layer is M5's. *(Corrected in M5, below: the 43 fps
  was the soak's own capture, a read-back of 1.5 s, not the level; fixed.)*

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

### M5, 2026-10-26: every level, and the slow read-back

**The soak under Vulkan** (`link-soak`, 48 levels in one session each, the 64-bit experimental build at 1366 wide,
2560x1440 window). Observed:

| | validation layer on | validation layer off (after the fix below) |
|---|---|---|
| Levels played through | 45 | 45 |
| Failed | IonaExt, LPalaceInt, tancredhouse | the same three |
| Validation errors / warnings | 0 / 0 in every level | (layer off) |
| fps after the summon, walls of fire and capture | 35.4-42.7 | 108.7-120 |

- The three failures are the soak's known crash, which the original has in the same three levels
  ([modern port](modern-port.md)): all three `errorlog.txt` entries are the same access violation at
  `character_instance::get_anim_length` + 0xc (a read of address 0x64).
- Under OpenGL the same point read 82-113 fps (the M0 soak of the same exe through the redirect, 3 levels; an earlier
  OpenGL soak of the 64-bit exe, 8 levels: 85-110). Not the same session, so a rough comparison.

**The slow read-back.** Every level read 42.7 fps after the capture. The game's `fps` averages 16 frames and caps a
frame at 0.25 s (`max_reported_frametime`, 0x5a5750, in `vsdk/frametime.cpp`), so 15/120 s + 0.25 s over 16 frames is 42.7 fps: one frame of at least 0.25 s. `[Debug] VulkanTiming=1`
(new, the table above) showed the capture's frame at **1,496 ms**, of which 5.6 ms was the wait for the GPU. The rest
was the CPU reading the read-back buffer, which was in host-visible, coherent and **uncached** memory: reading
uncached memory is slow. `vk_create_buffer` now picks a cached type (`HOST_CACHED`) for a buffer that is a copy's
destination, where the GPU offers one. After the fix: the capture's frame **28.5 ms** (half size); a full-size
capture does not reach the 25 ms log threshold. The save thumbnail goes through the same read-back (`vk_read_window`)
and gains the same way (Inferred).

Also seen with `VulkanTiming`: at a level start, frames of 31, 80 and 210 ms, the first two making 36 textures
(3 MB) in about 11 ms in all, the 210 ms one with no texture and no wait: the game's own work after a load (Inferred:
the same frames under OpenGL are not timed yet).

**`capturefull NAME`** (agent mode, the in-source host): every pixel of the window, where `capture` keeps every second
one. Observed: 2560x1440 against 1280x720.

**The save check under Vulkan, 2026-10-27** (`link-savecheck --exe Sum-fixed-clang-x64.exe --fixes --runs 2
--rebuilt-runs 2`, Wolong, masad, sewer and Catacombs; once with `Renderer=Vulkan`, once with `Renderer=OpenGL`; the
original runs under SummonerFix, which reads anything but Direct3D and Glide as OpenGL). Observed, dwords beyond the
original runs' spread:

| | Wolong | masad | sewer | Catacombs |
|---|---|---|---|---|
| 64-bit exe under Vulkan | 19 | 97 | 155 | 36 |
| 64-bit exe under OpenGL | 162 | 110 | 17 | 178 |

Neither renderer is ahead: these are the known NPC-goal differences of the clang builds ([modern
port](modern-port.md)), which move from run to run. Taken directly, each Vulkan save against all six non-Vulkan saves
of its level (the original's four, OpenGL's two): 2 and 0, 37 and 0, 0 and 2, 0 and 1 dwords beyond their spread; an
OpenGL save against the other five, the same way: 0, 3, 108, 0. The renderer leaves no trace in the saves.

**Robustness, 2026-10-27**, with the validation layer on, Observed:
- **Minimise and restore** (`ShowWindow` from outside, in play): nothing is drawn while minimised; after the restore
  the game draws at 120 fps. 0 validation errors.
- **Focus lost and back** (`WM_ACTIVATEAPP` and `WM_ACTIVATE` posted, as Alt+Tab sends them): the game sees itself in
  the background and back in front, and draws on. 0 errors.
- **`Mode=Window` resized** (1600x1200, then client areas 1184x1421, 2184x1421 and maximised 2560x1369): the
  swapchain is recreated at each size and the menu stays centred and letterboxed. 0 errors.
- **The same in a level** (Lenele, a golem summoned; client areas 1284x1421, 2184x1421, maximised 2560x1369, then
  minimised and restored): the swapchain and depth buffer are remade each time, the picture is letterboxed and the
  golem hides what is behind it; 108-115 fps, 0 errors. A first attempt also cast walls of fire, and the game crashed
  4 s after them at `character_instance::get_anim_length` + 0xc, the soak's known crash of the original (a
  townsperson dying in the fire), not the resize; without the fire, no crash.
- **The intro movie played to the end** (`SkipIntroMovies=0`; the main menu plays `vlogo.bik`, and `eaxlogo.bik` only
  with EAX sound, off here): `vlogo.bik`, 416 of 416 frames, 13.85 s of wall time for
  13.83 s of video, then the main menu; 0 errors. Under OpenGL the same exe: 13.84 s, and the same "latest frame
  496 ms late" in the movie log (Vulkan 492 ms), so that figure is the movie player's, not the renderer's.

**Full-size captures, 2026-10-27** (`capturefull`, 2560x1440, 1366 wide with centred menus): the main menu and the
load screen with a save's thumbnail are **identical to OpenGL at every pixel** (`imgdiff` maximum difference 0); the
menu against the load screen, as a control, differs in 56% of pixels. Observed.

**Every level's picture against OpenGL, 2026-10-27.** The same exe soaked twice under OpenGL (validation off), and
each level's soak capture (after the golem and the walls of fire) compared with `imgdiff`, % of pixels beyond a 1-px
shift. Observed:
- **The noise is large:** OpenGL run 1 against run 2 differs by 0.9% (masad, paused by its tutorial) to 67%
  (IkaemosBottomInt2), median about 22%: the golem, the fire, weather and the camera differ from run to run.
- **Vulkan sits inside it:** on 38 of 47 levels the nearer of the two OpenGL runs is closer to Vulkan than the
  OpenGL runs are to each other. Were Vulkan just a third OpenGL run, about a third of the levels would come out above
  (by symmetry); 9 do, 7 of them by 0.5-6 points (Catacombs, IkaemosExt, KhosaniLab, Liangshan, Rand-IcelandNite01,
  WolongCaverns, lenele2aa).
- **lenele3a** (64% between the OpenGL runs, 82-85% for Vulkan): the Vulkan capture had the camera inside a wall,
  in combat. Run twice more under Vulkan: one capture is within 8.8% of OpenGL run 2, the other within 20.8% of
  OpenGL run 1, and the two Vulkan runs 64% apart. The camera's place, not the renderer.
- **masad** (paused by its tutorial, so the scene is still): each renderer repeats itself (Vulkan against Vulkan
  0.93%, OpenGL against OpenGL 0.86%) but Vulkan against OpenGL is 1.8-4.3% in all four pairs. The differing pixels
  are an animated effect (a line of glints on the ground), the selection ring and polygon edges; zoomed side by side,
  the still textures look the same and the glints sit at other places along their path. Inferred: the pause comes
  at a slightly different moment of the effects' animation under each renderer (the frame times of the level start
  differ), and each renderer is repeatable. Unknown whether part of it is rasterization (the edges); a frozen-frame
  comparison would separate the two.

**On other GPUs** (Inferred, not run; only this PC's Intel Arc 140V has been tried):
- `GL_CLAMP` clamps to the border colour by the OpenGL specification, and the Vulkan sampler does the same
  (transparent black). Some OpenGL drivers, NVIDIA's among them by reputation, clamp to the edge instead, so on such a
  card an OpenGL capture can differ from a Vulkan one along clamped texture edges, and Vulkan is the one that follows
  the specification.
- The depth buffer is `X8_D24_UNORM_PACK32` where the GPU offers it, else `D32_SFLOAT` (AMD offers no 24-bit
  depth-only format, as far as known). With 32-bit float depth, depth ties between coplanar polygons can resolve
  differently from OpenGL's 24-bit buffer.

### Variable refresh, 2026-10-27: frame pacing

Variable refresh needs nothing new from the renderer: `[Display] VulkanPresent` already offers Fifo, Mailbox and
Immediate, and `[Experimental] FrameRateCap` caps the frame rate (the usual recipe is vsync on with a cap a few
frames below the display's maximum, or vsync off). What decides whether the display actually follows the game is the
driver and Windows (independent flip for a borderless window, the Windows "variable refresh rate" setting, the
monitor's own setting), and nothing in Vulkan reports it. This PC's driver offers neither `VK_KHR_present_wait` nor a
present-timing extension, only `VK_EXT_full_screen_exclusive` (vulkaninfo, Observed), so the measurement is the CPU's:
the time from one present to the next (`[Debug] VulkanTiming=1`, the `present:` lines), on the main menu and then
20-50 s in play from a test save, 2560x1440 borderless. Observed:

| Present mode | FrameRateCap | Present-to-present | sd | Frames over 1.5x the mean |
|---|---|---|---|---|
| Immediate | 0 (the game's 120) | 8.33 ms (120.0 fps) | 0.1-0.4 ms | 0-1 per 240, except the level start |
| Immediate | 55 | 18.18 ms (55.0 fps) | 0.2-0.7 ms | 0 |
| Immediate | 144 | 6.94 ms (144.0 fps) | 0.1-0.6 ms | 0-1 |
| Fifo | 0 | 31.7 ms (31.5 fps) | 0.4-3 ms | 0-2 |
| Fifo | 55 | 31.9 ms (31.4 fps) | 1-3 ms | 0-4 |

- **The cap is kept to a fraction of a millisecond** without vsync: the fixed build's frame limiter (G1, a
  high-resolution wait) paces the frames, and the Vulkan back end adds no jitter. This is what variable refresh needs
  from the game: an even frame time below the display's maximum.
- **Fifo ran at half the display's rate:** Windows reported 59.95 Hz (DWM's composition timing), yet each frame
  waited about 31 ms in `vkAcquireNextImageKHR` (the fence and the present took under 0.1 ms). The OpenGL renderer
  with `[Experimental] VSync=1` ran at 28.8 fps in the same session, and `Mode=Window` changed nothing, so it is not
  the Vulkan code. On 2026-10-26 Fifo had run at 60 fps on this PC. Unknown why; Inferred, a guess only: the
  monitor was asleep while nobody was at the PC (the tests ran unattended), and Windows paced vsync without it.
- The monitor (a Dell S2721DGF) ran at 59 Hz, while Windows lists modes up to 143 Hz (WMI, Observed). At 59 Hz a
  variable refresh window would be about 48-59 Hz, below the game's 120, so whether the display follows the game
  stays **Unknown** until the monitor runs at its full rate and something outside the game reads the presents
  (PresentMon, or the monitor's own refresh readout).
- Level starts show single long frames (330-830 ms) in every mode: the level's loading, not the presents.
