---
title: dgVoodoo 2
parent: Projects
nav_order: 3
---

# dgVoodoo 2 for the Glide and Direct3D renderers

**Summary:** Summoner needs exactly two dgVoodoo files: `Glide2x.dll` for the Glide renderer (loaded with
`LoadLibrary("glide2x.dll")` only when Glide is chosen) and `DDraw.dll` for the Direct3D 7 renderer (a **static**
import of `Sum.exe`, so it is loaded at every start, also under OpenGL), plus whatever `DDraw.dll` itself pulls in for
Direct3D 7 (dgVoodoo ships a `D3DImm.dll` for that; not verified here). SummonerFix already has the switch
(`[Display] Renderer=OpenGL|Direct3D|Glide`); it does not force OpenGL. But almost all of SummonerFix's display
layer (scaled output, widescreen, the 3D aspect fix, agent-mode cursor and captures, the controller, the brightness
fix, the save-thumbnail fix) lives in OpenGL and `SwapBuffers` hooks and is switched off under the other two
renderers. What the original renderers give back: the Bink logo and credits movies (the OpenGL renderer skips them),
the game's own hardware gamma, and the original look. Four hazards are visible in the code before any test: the
Direct3D back end refuses any adapter whose description contains "Voodoo" (which a name such as "dgVoodoo ..." would
match), Glide only opens 640x480, 800x600 or 1024x768, Glide refuses the windowed mode that the fixed and
experimental builds always pass, and the Direct3D windowed path assumes a 16-bit back buffer.

**Status (2026-10-21, evening): both renderers run through dgVoodoo 2.87.5.** Glide works as it is; Direct3D needed
one change, now in SummonerFix (the original exe) and the experimental build (see "Test results" below). The design
sections that follow were written first, from the decompiled source and SummonerFix's source; where dgVoodoo's own
behaviour is described there, it is what its documentation is generally known to say and is marked **not verified
here** unless the test results say otherwise.

## Test results (2026-10-21)

dgVoodoo 2 v2.87.5 from the author's GitHub releases ([Sources](../reference/sources.md)), the x86 `Glide2x.dll`,
`DDraw.dll`, `D3DImm.dll` and `dgVoodoo.conf` next to the exe, in a test folder (`SummonerToolchain/dgv-test`,
the game data hard-linked, copies of two test saves). dgVoodoo settings: `FullScreenMode = false` (dgVoodoo shows
the game's fullscreen mode in a window), both watermarks off, forced vsync off; Glide `VideoCard = voodoo_2`,
`OnboardRAM = 12`, 2 TMUs of 4 MB; Direct3D `VideoCard = geforce_ti_4800`. Input by real mouse clicks (agent
mode's cursor and `capture` work only under OpenGL, section 3).

| Run | Result |
|---|---|
| T0: original exe, OpenGL, dgVoodoo's `DDraw.dll` present (the static import) | **unchanged**: main menu at 120 fps |
| T2: original exe, Glide | **works**: main menu, a save loads (Khosani, full party), the 3D view, lightmaps, characters and HUD draw correctly; 60 fps |
| T1: original exe, Direct3D | **refused**: "Error -- dgVoodoo DX API Layer -- This Voodoo card cannot be run in Direct3D mode" (section 1's hazard). `AdapterIDType = nvidia` and every emulated card name leave the description as it is |
| T1 with the fix (below) | **works**: main menu, a save loads and plays, 60 fps; SummonerFix's clock check: the game clock equals QPC time (1:1, no 2x speed) |
| `Sum-exp.exe` (experimental build), Glide and Direct3D | **works**: a save loads and plays on both |
| Quit from the main menu under Direct3D | the game exits normally (`ExitProcess(0)`); only agent mode's `stop` command does not end it there (the test harness then kills it) |

**The fix.** `gr_d3d_init_device` refuses an adapter whose `szDescription` contains "Voodoo" (meant for 3dfx's
own Direct3D drivers). dgVoodoo's adapter is always "dgVoodoo DX API Layer". The test now lets a description that
starts with "dgVoodoo" through; a real Voodoo card is still refused:
- SummonerFix, original exe, `Renderer=Direct3D`: the `stristr` call at `0x539e9d` goes to `DgvDescStristr`
  (`src/summonerfix/main.c`).
- Experimental build: the same test in `vsdk/gr_direct3d.cpp` under `EXPERIMENTAL_BUILD`. The experimental build
  also opens Direct3D and Glide fullscreen (mode 201, which Glide requires) instead of the rebuilt exe's fixed
  window mode 200, unless `[Display] Windowed=1` (`game/summoner.cpp`).

**How to use it** (original `Sum.exe` with SummonerFix, or `Sum-exp.exe`): download dgVoodoo 2 from its author's
GitHub releases; copy `3DfxGlide2x.dll` (for Glide) or `MSDDraw.dll` and `MSD3DImm.dll` (for Direct3D)
next to `Sum.exe`, with `dgVoodoo.conf` (set it with `dgVoodooCpl.exe`: watermark off; for Direct3D any card);
set `[Display] Renderer=Glide` or `Direct3D` in `summonerfix.ini`. Removing the DLLs and setting
`Renderer=OpenGL` goes back. Under Glide and Direct3D, SummonerFix's OpenGL-only parts are off (widescreen, scaled
output, the controller, agent mode's cursor and captures; section 3).

**Not yet tested** (section 5): the logo and credits movies (T7), gamma (T4), Alt-Tab (T6), other resolutions (T8),
a long run (T9), screen-by-screen comparisons with OpenGL (T3), and dgVoodoo's own fullscreen output.

Evidence levels as in [Documentation standard](../contributing.md): **Observed** = read from the source or the
disassembly of `Sum.exe` v1.40; **Inferred** = concluded from the code, not run; **Unknown** = open.

## 1. Which DLLs each renderer loads

`gr_init` (`0x505e00`, `FUN_00505e00`) calls one back end per `gr_screen.mode`: `gr_d3d_init` (`0x5394e0`) for
`0x66`, `gr_glide_init` (`0x549d10`) for `0x67`, `gr_opengl_init` (`0x54f6f0`) for `0x68` (calls at `0x5060b4`,
`0x5060ac`, `0x5060a4`). **Observed.**

### Static imports (loaded by Windows at process start, before any game code)

`llvm-objdump -p Sum.exe`, graphics-related DLLs only. **Observed.**

| DLL | Imported names | Used by |
|---|---|---|
| `OPENGL32.dll`, `GLU32.dll` | the GL 1.1 entry points | OpenGL renderer; SummonerFix hooks them |
| `DDRAW.dll` | `DirectDrawEnumerateA` only (thunk `0x57b564`) | `d3d_enumerate_ddraw_devices` (`0x538850`) |
| `DINPUT.dll` | `DirectInputCreateA` | mouse init; this is the slot SummonerFix's `dinput.dll` proxy fills |
| `binkw32.dll` | `BinkOpen`, `BinkCopyToBuffer`, ... | `movie_play` (`0x5172f0`) |

No static import of `d3dim.dll`, `d3dim700.dll`, `glide*.dll` or any Direct3D 8/9 DLL. **Observed.**

Consequence (**Inferred**): a dgVoodoo `DDraw.dll` placed next to `Sum.exe` is loaded at every start, whatever the
renderer, because the loader resolves `DDRAW.dll!DirectDrawEnumerateA` from the application folder first
(`ddraw.dll` is not a KnownDLL; dgVoodoo's own instructions rely on this, not verified here). Under OpenGL it is
loaded but never called (only `d3d_enumerate_ddraw_devices` calls `DirectDrawEnumerateA`). So "OpenGL with dgVoodoo
files in the folder" is a configuration to test too (step T0 below).

### Direct3D (`0x66`): `vsdk\gr\directx\gr_direct3d.cpp`

`gr_d3d_init` (`0x5394e0`) -> `gr_d3d_init_device` (`0x5395c0`), in this order. **Observed** (source
`vsdk/gr_direct3d.cpp`, the `LoadLibraryA` call at `0x5395d3`):

1. `LoadLibraryA("ddraw.dll")`; on failure the function **returns silently** (no message; the game then runs with no
   device, *Inferred* crash later).
2. `os_registry_read_dword("D3D Optimizations")` -> `d3d_use_vertex_buffer` (SummonerFix answers "not set" -> 0).
3. `GetProcAddress(ddraw, "DirectDrawCreateEx")` -> `d3d_ddraw_enumerate_ex` (`0x2dc6c34`); missing -> silent return.
4. `d3d_enumerate_ddraw_devices` (`0x538850`): the **static** `DirectDrawEnumerateA`; its callback
   `d3d_ddraw_enum_callback` (`0x5389f0`) returns `DDENUMRET_CANCEL` (`0x538a58`), so only the primary display driver is
   listed (game-bugs V61). For it: `DirectDrawCreateEx(..., IID_IDirectDraw7)`, `QueryInterface(IID_IDirect3D7)`,
   `IDirect3D7::EnumDevices`; devices with `D3DDEVCAPS_HWTRANSFORMANDLIGHT` are skipped, the **last** remaining one is
   used (normally the HAL).
5. `DirectDrawCreateEx` again for that device, `QueryInterface(IID_IDirect3D7)`, `GetDeviceIdentifier`.
6. Window mode 200/202: `DDSCL_NORMAL`, a primary surface, an offscreen `DDSCAPS_3DDEVICE` render target **in the
   desktop's pixel format** (no `DDSD_PIXELFORMAT`), a clipper. Mode 201: `DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN`,
   `SetDisplayMode(max_w, max_h, 16)`, a flipping chain with one back buffer.
7. `EnumZBufferFormats`, z-buffer surface, `IDirect3D7::CreateDevice`, `EnumTextureFormats`, `GetCaps`, the blend-cap
   checks, the gamma control (`IID_IDirectDrawGammaControl` on the primary, only if `DDCAPS2_PRIMARYGAMMA`), fog mode,
   the **Voodoo refusal**, the vendor tweaks, `GetAvailableVidMem` (>= 5 MB free texture memory).
8. `gr_d3d_tcache_init`, `BeginScene`, `IDirect3D7::CreateVertexBuffer` (1000 `D3DFVF_TLVERTEX2`).

Every failed DirectX call jumps to one shared exit: `gr_d3d_close(); exit(0)` at `0x539b41`. This is the silent exit
seen on 2026-10-12 with the system's own DirectDraw ([SummonerFix](../summonerfix/index.md), "Direct3D: silent exit");
which call failed is still **Unknown**.

### Glide (`0x67`): `vsdk\gr\glide\gr_glide.cpp`

`gr_glide_init` (`0x549d10`). **Observed**:

1. Window mode 200 or 202: `VSDK_ERROR` "Glide can only be used in fullscreen mode for now." (line 758). Only 201
   continues.
2. `SetEnvironmentVariable("FX_GLIDE_NO_SPLASH", "1")`.
3. `glide_load_dll` (`0x5473b0`): `LoadLibrary("glide2x.dll")` (call at `0x5473b5`); failure ->
   "Glide DLL not found!" (string 945, line 785). Then **112** `GetProcAddress` calls with the Glide **2** stdcall
   names (`_grDrawLine@8`, `_grSstWinOpen@28`, `_guTexMemQueryAvail@4`, ...). None of the results is checked
   (**Observed**): a missing export becomes a NULL pointer that crashes only when that entry point is first called.
4. `Sleep(2000)`, `grGlideGetVersion`, `grGlideInit`, `grSstQueryHardware` (fail -> "No Glide hardware found!", 946),
   `grSstSelect(0)`.
5. `grSstWinOpen(hwnd, res, GR_REFRESH_60Hz, GR_COLORFORMAT_ABGR, GR_ORIGIN_UPPER_LEFT, 2, 1)` with
   `res = max_w == 800 ? 800x600 : max_w == 1024 ? 1024x768 : 640x480` (fail -> "Glide SstrWinOpen failed!", 947).
6. `fbRam` of SST 0 < 4 MB -> message 944 ("needs at least 8 mb", the 4/8 mismatch is a known game bug) and `exit(1)`.
   `nTexelfx < 2` -> `glide_single_tmu` (no multitexture modes, so no single-pass lightmaps). `nTexelfx > 1` and
   `fbRam >= 12` -> `glide_close_on_deactivate` (the Voodoo2 path: Alt-Tab closes and reopens the Glide window).
7. `gr_screen.max_bitmap_w/h = 256`, `gr_glide_tcache_init`, `grGammaCorrectionValue(gr_gamma)`, the message handler,
   `grHints(GR_HINT_STWHINT, ...)`, alpha test `GR_CMP_GREATER` / 0.

The entry points the game actually calls (all `glide_gr*` / `glide_gu*` call sites in `vsdk/gr_glide.cpp`,
`game/levelrender.cpp`, `vsdk/unnamed_ca_mesh_0x50af80.cpp`, `vsdk/unnamed_movie_0x5172f0.cpp`; **Observed**):
`grGlideInit`, `grGlideShutdown`, `grGlideGetVersion`, `grSstQueryHardware`, `grSstSelect`, `grSstWinOpen`,
`grSstWinClose`, `grSstControl`, `grSstIdle`, `grSstIsBusy`, `grBufferSwap`, `grBufferClear`, `grClipWindow`,
`grColorMask`, `grDepthMask`, `grDepthBufferMode` (W-buffer), `grDepthBufferFunction`, `grDepthBiasLevel`,
`grAlphaBlendFunction`, `grAlphaCombine`, `grColorCombine`, `grAlphaControlsITRGBLighting`, `grAlphaTestFunction`,
`grAlphaTestReferenceValue`, `grConstantColorValue`, `grFogMode` (table, `ADD2`, `MULT2`), `grFogTable`,
`grFogColorValue`, `guFogGenerateLinear`, `grGammaCorrectionValue`, `grHints`, `grTexCombineFunction`,
`grTexFilterMode`, `grTexClampMode`, `grTexMipMapMode`, `grTexLodBiasValue`, `grTexMinAddress`, `grTexMaxAddress`,
`grTexTextureMemRequired`, `grTexDownloadMipMap`, `grTexSource`, `guTexMemQueryAvail`, `grDrawLine`,
`grDrawPlanarPolygon`, `grDrawPlanarPolygonVertexList`, `grDrawPolygonVertexList`, `grLfbLock`, `grLfbUnlock`,
`grLfbReadRegion`. About 49 of the 112; dgVoodoo's Glide 2 must export these with the decorated names.

Order at start-up (**Observed** / **Inferred**): `DDRAW.dll` (static, process start) -> `dinput.dll`
(SummonerFix, static) -> ... -> `LoadSettings` -> `gr_init` -> then either `ddraw.dll` again by `LoadLibraryA`
(Direct3D; same module, reference count only) or `glide2x.dll` (Glide) or nothing new (OpenGL).

### OpenGL (`0x68`) for comparison

Static `OPENGL32.dll`; `gr_opengl_init` (`0x54f6f0`) works only in window mode 200 (SummonerFix turns that window into
its scaled output). Not touched by dgVoodoo, which does not wrap OpenGL (not verified here).

## 2. How the renderer is chosen, and SummonerFix's switch

`LoadSettings` (`0x43d7f0`, `game/summoner.cpp`). **Observed:**

```cpp
renderer = GR_GLIDE;                                   // 0x43da64: mov edi, 0x67   (REBUILT_OPENGL: GR_OPENGL)
os_registry_read_dword(NULL, "Selected Video Card", &card, -1);
if (card != -1) {
    sprintf(card_key, "Video Card %d", card);
    os_registry_read_string(NULL, card_key, card_name, "None");
    if (_stricmp(card_name, "None")) {
        if (strstr(card_name, "3DFX"))           renderer = GR_GLIDE;     // case-sensitive
        else if (strstr(card_name, "Direct 3D")) renderer = GR_DIRECT3D;
    }
}
if (cmdline_param_direct3d.found()) renderer = GR_DIRECT3D;            // -direct3d; -glide is never read
gr_init(w, h, renderer, 201 /* 0x43db45: push 0xc9; REBUILT_OPENGL: 200 */, hwnd, ...);
```

**SummonerFix does not force OpenGL; it has the switch already.** `[Display] Renderer` (default `OpenGL`) in
`summonerfix.ini`, read in `main.c LoadConfig`:

| Renderer= | `reghook.c` answers `Video Card 0` | `main.c ApplyDisplayPatches` | Display layer |
|---|---|---|---|
| `OpenGL` (default) | `"OpenGL (SummonerFix)"` (no match, default stays) | `0x43da64` `BF 67` -> `BF 68`; `0x43db45` 201 -> 200 | `DisplayInstall`, `WidescreenInstall` |
| `Direct3D` | `"Direct 3D (SummonerFix)"` -> `0x66` | none; 200 only with `[Display] Windowed=1` | none |
| `Glide` | `"3DFX Glide (SummonerFix)"` -> `0x67` | none; 200 with `Windowed=1` (Glide then fails, see below) | none |

`Selected Video Card` is answered with 0. **Observed** (`reghook.c QueryConfigValue`, `main.c`).

So a player with SummonerFix selects dgVoodoo's renderers with `Renderer=Glide` or `Renderer=Direct3D` (also in
SummonerFixConfig.exe). Nothing new is needed in SummonerFix to *leave the renderer alone*; what a "leave alone"
mode would add is only a fourth value (for example `Renderer=Registry`) that makes `QueryConfigValue` return -1 for
`Selected Video Card` / `Video Card 0` so the real registry (a launcher's choice) decides. Not needed for dgVoodoo.

Two traps (**Observed** in the code, **Inferred** effect):

- **Glide with `Windowed=1`** passes 200 and ends in "Glide can only be used in fullscreen mode for now." Glide needs
  `Windowed=0`; a windowed Glide picture must come from dgVoodoo's own windowed option (not verified here).
- **The fixed and experimental builds** (`REBUILT_OPENGL`, `tools/link-build.mjs` configurations `fixed` and
  `experimental`) compile the default to OpenGL **and the window mode to 200 unconditionally**. SummonerFix's registry
  hook is still installed for them (`main.c`, `RebuiltInit` branch), so `Renderer=Glide` makes `Sum-fixed.exe` /
  `Sum-exp.exe` choose Glide and then stop at the fullscreen-only error. `Renderer=Direct3D` gives the windowed Direct3D
  path (a captioned window at the game resolution). Section 6 fixes this for the experimental build.

## 3. What SummonerFix loses under Glide or Direct3D

`main.c DllMain` installs `DisplayInstall` and `WidescreenInstall` only for `RENDERER_OPENGL`; `ControllerInstall`
turns itself off for any other renderer (`controller.c` line 281). Everything that runs from `display.c`'s
`Hook_SwapBuffers` therefore stops. **Observed** (call graph of the source), effects **Inferred**:

| SummonerFix feature | Where | Under Glide / Direct3D |
|---|---|---|
| Scaled output (game resolution scaled to the monitor, borderless window, black bars) | `display.c` `glViewport`, `SetWindowLongA`, `SetWindowPos` hooks | **Lost.** The game's own fullscreen: a topmost popup the size of the desktop (`gr_init` 201) and a real mode change (Direct3D `SetDisplayMode(w, h, 16)`, Glide `grSstWinOpen` 640/800/1024). Scaling to the monitor becomes dgVoodoo's job (its scaling options, not verified here) |
| `[Display] Mode=Window`, `WindowWidth/Height` | `display.c` | **Lost.** Only `Windowed=1` (the game's own captioned window, Direct3D only) |
| 3D aspect in windowed mode (the `GetSystemMetrics` hook for `gr_init`) | `display.c Hook_GetSystemMetrics` | **Lost** for Direct3D `Windowed=1`: `gr_init` 200 takes the aspect from the desktop (`0x505e00` windowed branch; game-bugs, gr.cpp note 1), so a 16:9 desktop stretches the 3D view of a 4:3 game. Fullscreen 201 uses `max_w / max_h` and is correct |
| Widescreen (`GameWidth` > 1024, centred menus and HUD) | `widescreen.c` (needs `display.c GetMap`) | **Lost.** Glide cannot open such a size at all (section 4); Direct3D would ask for `SetDisplayMode(1366, 768, 16)` and draw the 1024-wide UI at the left. Use 640x480, 800x600 or 1024x768 |
| Agent mode cursor (`move`, `click X Y`, `drag`) | `display.c` `ScreenToClient` / `ClientToScreen` hooks | **Lost**: positions are ignored; clicks land where the real cursor is |
| Agent mode `capture` | `display.c Hook_SwapBuffers` + `SaveFrame` | **Lost**: "capture ...: timed out" |
| Agent mode `pad` | polled from `Hook_SwapBuffers` | **Lost** ("not released after 1 s") |
| Agent mode `key`, `keydown`, `info`, `peek`, `poke`, level commands, staying "in the foreground" | `keys.c` message handler, `agent.c` thread | **Kept** (no renderer dependence) |
| Controller (XInput) | `ControllerFrame` from `Hook_SwapBuffers`, cursor through `DisplayMoveCursor` | **Off** (by design, logged "needs the OpenGL renderer") |
| Brightness (K90: shader or monitor ramp) | `gamma.c GammaApply` from `Hook_SwapBuffers` | **Not needed**: the game's own gamma works on these renderers: Direct3D `IDirectDrawGammaControl` on the primary (only if the driver reports `DDCAPS2_PRIMARYGAMMA`), Glide `grGammaCorrectionValue`. Whether dgVoodoo implements both is **Unknown** (test T4) |
| Save thumbnails fix (`glReadPixels` mapped to the scaled frame) | `display.c Hook_glReadPixels` | **Not needed** (no scaling), but see the 565 assumption below |
| Speed key badge "FAST x3" | drawn in `Hook_SwapBuffers` | **Badge lost**; the speed itself (`keys.c`) is **kept** |
| `[Debug] CaptureEvery` | `Hook_SwapBuffers` | **Lost** |
| Bar clearing (K280, flickering strip right of centred menus) | `Hook_SwapBuffers` | Not applicable (no bars) |
| `MaxTextureSize` cap | `glfix.c` | Not applicable (Glide caps at 256, Direct3D uses the device caps; neither allocates `size^2 x 4`) |
| `OpenGLTextures` (K64) | `bugfix.c` patch at `0x507a03` | Not applicable. Direct3D reports free texture memory **in bytes** (game bug), so `summoner_cfg_load`'s `<= 8` test never lowers textures; Glide reports `fbRam` in MB, so a dgVoodoo Glide card with 8 MB or less gives **reduced textures on a fresh install** (no `summoner.cfg`) |
| Timing fix (G1, 2x speed), cutscene timer, key bindings, all `[Fixes]`, tentacle grab, hybrid | `timing.c`, `keys.c`, `bugfix.c`, ... | **Kept**: they patch game code, not the renderer |

What the original renderers **give back** (**Observed** in `movie_blit_frame`, `0x517430`): the Bink movies are
copied as 640x480 RGB565 into a locked back buffer, `grLfbLock` for Glide or `IDirectDrawSurface7::Lock` for
Direct3D; for OpenGL the function returns 1 at once, so **under OpenGL the Volition / EAX logo movies and the credits
movie (`geeks.bik`) end immediately** (*Inferred*; the 2026-10-14 SummonerFix note that `SkipIntroMovies=1` "shows the
main menu at once instead of the logo movies" suggests something was visible under OpenGL, so this is to be checked,
open question 5). The Glide-only mesh flag for `Stalag-Small` (`s3d.cpp`, `0x800000`) and the Glide depth scale
(65536, far plane 8000, `unnamed_gr_3d_0x50d9b0.cpp`) also come back: the game was tuned on these renderers.

## 4. The Glide renderer: formats, limits, known issues

From `vsdk/gr_glide.cpp` and its notes (`vsdk/NOTES-gr-glide.md`). **Observed** unless marked.

**Screen.** Fullscreen only (window mode 201). Three resolutions: `GR_RESOLUTION_640x480`, `800x600`, `1024x768`,
chosen from `gr_screen.max_w`; **any other width opens 640x480** while the game keeps drawing in its own size
(*Inferred*: a clipped or garbled picture, for example with SummonerFix's custom `GameWidth`, which `main.c` writes
into the 1024x768 slot of the resolution table). 60 Hz, ABGR colour, origin upper left, 2 colour buffers, 1 aux
(depth) buffer, **W-buffer** depth (`GR_DEPTHBUFFER_WBUFFER`, cleared with `GR_WDEPTHVALUE_FARTHEST`), fog through a
64-entry table (`guFogGenerateLinear`). The same resolution choice is repeated in `glide_set_active` (`0x549fc0`).

**Textures** (`gr_glide_create_mipmapped_texture` `0x547cc0`, `gr_glide_create_texture_section` `0x5484f0`):

| Bitmap format | Glide format | Note |
|---|---|---|
| 8-bit alpha | `GR_TEXFMT_ALPHA_8` | values through `gr_gamma_ramp` |
| 8-bit paletted | `GR_TEXFMT_ARGB_1555` | pure green (0, 255, 0) transparent |
| 888 | `GR_TEXFMT_ARGB_1555` | same colour key |
| 1555 | `GR_TEXFMT_ARGB_1555` | the bitmap's alpha bit is inverted |
| 565 | `GR_TEXFMT_RGB_565` | copied |
| 4444 | `GR_TEXFMT_ARGB_4444` | copied |
| 8888 | `GR_TEXFMT_ARGB_4444` | **alpha and colour cut to 4 bits** |

No NCC / `YIQ` / `P_8` formats, no `grTexDownloadTable` use (the palette is expanded by the game). Sizes: textures are
cut into **256 x 256** sections (`gr_screen.max_bitmap_w/h = 256`); a mipmapped bitmap must be a power of two from
**4 to 256** in each direction (`gr_glide_round_texture_size` `0x547c00`), else it is not drawn ("Glide: Not drawing
%dx%d bitmap"); aspect ratios up to 8:1. Mipmapping `GR_MIPMAP_NEAREST` on TMU 0, LOD bias 0.5. Texture memory is
managed by the game itself (`glide_record`, 2800 blocks per TMU, addresses from `grTexMinAddress` / `grTexMaxAddress`,
never across a **2 MB boundary**), so dgVoodoo must honour Glide 2's address-based `grTexDownloadMipMap` /
`grTexSource` model exactly. Multitexture (lightmaps, modes 4 and 6) needs **2 TMUs**; with one TMU those modes are
reported unsupported and drawn in several passes.

**Linear frame buffer.** `grLfbLock(GR_LFB_WRITE_ONLY, back buffer, GR_LFBWRITEMODE_565)` for the movies,
`grLfbReadRegion(back buffer, ..., w * 2)` for screenshots and save thumbnails (format returned: 3 = 565), and
`gr_glide_flush_pipeline` (`0x54bad0`) copies the **front** buffer into the locked back buffer. A wrapper that keeps
the frame on the GPU has to implement all three.

**Focus.** On a "Voodoo2" (2 TMUs and `fbRam >= 12`) every deactivation closes the Glide window
(`grSstWinClose`, `ShowWindow(SW_MINIMIZE)`) and activation reopens it (`grSstWinOpen`, `SW_MAXIMIZE`, texture cache
flushed); otherwise `grSstControl(GR_CONTROL_DEACTIVATE / ACTIVATE)` once a frame. The cursor is clipped to the game
rectangle only in the Voodoo2 case. Both are Alt-Tab test points.

**Window.** `gr_init` gives the Glide window (only Glide) `WS_EX_TRANSPARENT` (`0x505f18`-`0x505f1d`), topmost
popup the size of the desktop. On a real Voodoo the picture came out of the pass-through cable and the window was a
transparent placeholder. Mouse buttons reach the game as window messages (`mouse` handler `0x500f60`). Whether
clicks still reach a `WS_EX_TRANSPARENT` popup that dgVoodoo draws into is **Unknown** (without `WS_EX_LAYERED` they
normally do; test T2).

**Known game bugs on this path** (from the source; none verified in game): the 8 MB message for a 4 MB test; 8-bit
alpha bitmaps larger than 256 texels repeat their top-left section; no vertex bound in `gr_glide_tmapper`;
animated bitmaps rebuilt twice; `glide_clip_cursor(1)` dead on the first init.

**What dgVoodoo is generally documented to provide** (not verified here): Glide 1, 2 (2.11 to 2.60) and 3 in one
package (`Glide.dll`, `Glide2x.dll`, `Glide3x.dll`), output through Direct3D 11 or 12, an emulated card type (Voodoo
Graphics, Voodoo 2, Banshee, Voodoo 3), configurable on-board memory, texture memory per TMU and number of TMUs, a
resolution override and scaling to the desktop, windowed Glide, a vsync option, a gamma setting, and a watermark that
is on by default.

## 5. Test plan (when the user supplies the dgVoodoo files)

Files: from the dgVoodoo release, the **x86** builds only (`Sum.exe` is 32-bit), next to `Sum.exe`:
`Glide2x.dll` (Glide), `DDraw.dll` and `D3DImm.dll` (Direct3D 7), and `dgVoodoo.conf` (written by
`dgVoodooCpl.exe`). `Glide.dll`, `Glide3x.dll`, `D3D8.dll`, `D3D9.dll` are not used by Summoner and should stay out, so
that a problem cannot come from them. Record source, version, size and SHA-256 in `docs/reference/sources.md`; the DLLs
never go into the repository. Keep a copy of `summonerfix.ini` and the test saves; nothing in the test writes the
registry (SummonerFix answers the game's key).

dgVoodoo settings that matter (names as generally documented, not verified here):

| Setting | Glide | Direct3D | Why |
|---|---|---|---|
| Output API | D3D11 first | D3D11 first | |
| Full screen / windowed, scaling mode "keep aspect ratio" | both | both | replaces SummonerFix's scaling |
| Watermark | off | off | otherwise every capture shows it |
| Glide card type | Voodoo 2 first, then Banshee / Voodoo 3 | - | decides the TMU count and the `glide_close_on_deactivate` path |
| On-board memory | **12 MB or more** | - | `fbRam < 4` exits; `<= 8` lowers textures on a fresh install; `>= 12` with 2 TMUs is the Voodoo2 focus path |
| Texture memory per TMU, TMUs | 2 TMUs, >= 4 MB | - | lightmaps in one pass; the 2 MB-bank allocator |
| Glide resolution override | "unforced" first | - | the game only knows 640/800/1024 |
| Videocard | - | **not a name containing "Voodoo"**: a GeForce-class card first | the refusal at `0x539e9d` |
| VRAM | - | >= 32 MB | the 5 MB check, texture budget |
| Force vsync / frame rate | off first | off first | G1 (below) |
| Gamma / brightness | default | default | the game sets its own (test T4) |

`summonerfix.ini`: `[Display] Renderer=Glide` or `Direct3D`, `GameWidth=1024`, `GameHeight=768`, `Windowed=0`,
`[Debug] Agent=1` for the key-only parts of agent mode. Turn on `[Debug] Diagnostics=1` for the first runs: it logs the
`exit` caller (as for the 2026-10-12 Direct3D exit).

Steps (the lead runs them; I did not run the game):

- **T0. OpenGL with the dgVoodoo files present.** `Renderer=OpenGL`, `DDraw.dll` + `D3DImm.dll` in the folder. Pass:
  the game starts and plays exactly as without them (`summonerfix.log` unchanged apart from times). This checks the
  static `DDRAW.dll` import.
- **T1. Direct3D start.** `Renderer=Direct3D`. Pass: the logo movies play, then the main menu; no message box.
  Possible failures and their meaning: silent exit -> `0x539b41` (Diagnostics gives the caller; compare with
  2026-10-12); "Error -- %s -- This Voodoo card cannot be run in Direct3D mode" -> pick another dgVoodoo videocard
  name; "Missing Features" -> blend caps or 16-bit texture formats; "These drivers are not compliant with DirectX 7"
  -> warning only.
- **T2. Glide start.** `Renderer=Glide`, `Windowed=0`. Pass: logo movies, main menu, mouse clicks work (the
  `WS_EX_TRANSPARENT` question). Failures: "Glide DLL not found!" (wrong DLL name or 64-bit DLL), "No Glide hardware
  found!", "SstrWinOpen failed!", "needs at least 8 mb" (on-board memory), or a crash right after start (a missing
  Glide 2 export: the list in section 1).
- **T3. Screens.** On both renderers and on OpenGL, the same scenes from the test saves: main menu, a town, a dark
  level with fog, a fight with spell effects, the inventory and character screens, the map, a conversation, a cutscene.
  Compare: textures (Glide's 8888 -> 4444 alpha should show banding on soft alpha), lightmaps, fog distance and colour,
  transparency (green colour key), font edges, depth artefacts (Direct3D runs with a reversed depth test on every
  vendor but S3; Glide with the W-buffer). Screenshots with the game's own Print Screen (`gr_screenshot` through
  `gr_read_backbuffer`, 565 on both back ends; agent `capture` does not work here), and the save list thumbnails.
  Under Direct3D `Windowed=1` expect garbled screenshots and thumbnails (section 6, item 3).
- **T4. Gamma.** Options screen brightness slider, low / default / high, on both renderers. Pass: the picture
  changes; nothing outside the game window changes; after quitting the desktop gamma is back. On OpenGL the same slider
  goes through SummonerFix `Brightness` (K90).
- **T5. Speed and G1.** G1 (the frame limiter counting its own sleep, `FrameTimer` `0x501510`) is renderer-independent
  in the code, but a frame longer than the 1/120 s minimum (vsync at 60 Hz) never sleeps and hides it. With dgVoodoo's
  forced vsync off: `info` in agent mode shows `fps`; the in-game clock against a stopwatch over 60 s (as on
  2026-10-12) with SummonerFix's timing fix on (expected 1:1) and, for the record, off. Also compare the frame rate in
  the same busy scene between the three renderers.
- **T6. Alt-Tab and focus.** Alt-Tab out and back twice in the menu and in game, on both renderers; with the Glide
  card at 12 MB+ / 2 TMUs (window closed and reopened) and at 8 MB (`grSstControl`). Pass: picture and textures come
  back, no crash, the cursor is not stuck.
- **T7. Movies.** The logo movies at start (`SkipIntroMovies=0`) and the credits. Pass: picture and sound on both
  renderers; compare with OpenGL (open question 5).
- **T8. Resolutions.** 640x480, 800x600, 1024x768 on both; then (expected to fail on Glide) a custom `GameWidth`.
- **T9. Long run.** 20 minutes of play through a level change on each renderer; watch for texture corruption (the
  2 MB-bank allocator and eviction under Glide; `D3D Optimizations` vertex buffers off by default under Direct3D).

Record each run in the [SummonerFix test log](../summonerfix/index.md#test-log) style: date, renderer, dgVoodoo version
and card settings, result.

## 6. What the experimental build could add

The experimental build (`--config experimental`) is compiled with `REBUILT_OPENGL`, so today it can only run
Direct3D windowed and cannot run Glide (section 2). A switch, off by default, using the **same key SummonerFix reads**
so the two always agree (SummonerFix installs its OpenGL display hooks only for `Renderer=OpenGL`; if the exe chose
Glide while SummonerFix believed OpenGL, its `GetSystemMetrics` and `SetWindowPos` hooks would resize `gr_init`'s
fullscreen popup to the game size):

`[Display] Renderer = OpenGL | Direct3D | Glide` (default OpenGL = the fixed build's behaviour, "off"), and
`[Display] Windowed = 0 | 1` for Direct3D. `exp_ini_string` reads `summonerfix.ini`; it can be called before the
first frame.

Exact code places (line numbers of `W\src`, the dgvoodoo worker's copy):

1. **`game/summoner.cpp`, `LoadSettings` (`0x43d7f0`), lines 396-424.** Under `#ifdef EXPERIMENTAL_BUILD`, after the
   registry / `-direct3d` choice (line 417) and before `gr_init`: read `[Display] Renderer`; `Glide` -> `GR_GLIDE`,
   window mode 201; `Direct3D` -> `GR_DIRECT3D`, window mode `Windowed ? 200 : 201`; anything else -> the existing
   `REBUILT_OPENGL` values (`GR_OPENGL`, 200). The two `gr_init` calls (lines 419-425) get the window mode from a
   variable. For Glide, also force `resolution_index` to the table entry matching 640/800/1024 (or log and fall back
   to 1024x768) instead of a custom size, because of the `gr_glide_init` resolution choice. `exp_log` the result.
   The marker: `// EXPERIMENTAL_BUILD: dgvoodoo Renderer`, so `Sum-exp.fixes` tells SummonerFix that the exe chooses
   the renderer itself (SummonerFix's `reghook.c` answer then does not matter; its `main.c` byte patches do not apply
   to a rebuilt exe anyway).
2. **`vsdk/gr_direct3d.cpp`, `gr_d3d_init_device` (`0x5395c0`), the Voodoo refusal (`0x539e80`-`0x539eba`, block at
   `0x53a125`), source around line 1608.** Under the switch, skip the test when the identifier names dgVoodoo
   (`stristr(id.szDescription, "dgVoodoo")` or `stristr(id.szDriver, "dgVoodoo")`), keep it for everything else.
   Only needed if T1 shows the refusal with dgVoodoo's default card; otherwise not.
3. **`vsdk/gr_direct3d.cpp`, windowed path, source lines 1413-1440.** The render target takes the desktop's format
   (32-bit today) while `gr_d3d_read_backbuffer` (`0x53a7f0`) copies 2 bytes per pixel and returns 565, and
   `movie_blit_frame` (`0x517430`) writes RGB565 into it. *Inferred:* windowed Direct3D gives garbled screenshots,
   save thumbnails and movies. Fix under the switch: either create the offscreen surface with an explicit 565
   `DDPIXELFORMAT` (`DDSD_PIXELFORMAT`) or make `gr_d3d_read_backbuffer` and the movie blit convert from the surface's
   real format. Fullscreen (16-bit `SetDisplayMode`) is not affected.
4. **`vsdk/gr.cpp`, `gr_init` (`0x505e00`), windowed branch line 293.** The aspect from the desktop (game bug) can be
   fixed for the experimental build in the source (`aspect = max_w / max_h`), which SummonerFix does only for OpenGL.
   Line 297 (`WS_EX_TRANSPARENT` for Glide): only if T2 shows lost clicks.
5. **Per-frame work that does not depend on OpenGL.** `exp_frame_begin` / `exp_frame_end` (`GameTick`, `0x401070`) are
   renderer-independent, unlike SummonerFix's `SwapBuffers` hook. The experimental controller (`experimental/
   controller.cpp`) should therefore drive the cursor through the game's own mouse (`mouse_move_to`, the position
   `GameTick` reads) and not through `display.c`, so it works on all three renderers. An agent-mode capture for
   Glide/Direct3D could use the game's own `gr_read_backbuffer` (`0x507d30`; 565 on both) from `exp_frame_end`; this
   would be a new SummonerFix request, not part of this design.
6. **Not to change:** `gr_glide.cpp`'s 640/800/1024 table and 256 texture limit (dgVoodoo's own resolution override
   and the 256 limit are what the game was made for); the `D3DDEVCAPS_HWTRANSFORMANDLIGHT` skip (dgVoodoo exposes a
   plain HAL as well, not verified here).

Header / ini text the lead would add (not done here; research only):

```ini
[Display]
; OpenGL (recommended for modern drivers), Direct3D, or Glide (needs a Glide wrapper such as dgVoodoo 2:
; Glide2x.dll next to Sum.exe; Direct3D: DDraw.dll and D3DImm.dll). Glide runs fullscreen only.
Renderer=OpenGL
```

(the key exists; only the comment changes).

## Open questions

1. What dgVoodoo's virtual Direct3D adapter reports in `DDDEVICEIDENTIFIER2` (`szDriver`, `szDescription`,
   `dwVendorId`): does it contain "Voodoo" (refusal) or "3dfx", and which vendor path (ATI fog mode 3, S3 normal depth,
   Matrox table fog) does each selectable card take?
2. Which call failed in the 2026-10-12 native Direct3D exit (`0x539b41`): likely `SetDisplayMode(1920, 1080, 16)` or
   the HAL device; dgVoodoo would answer both, so this may never need an answer.
3. Does dgVoodoo's Glide 2 export all 49 used entry points with the decorated names, and the 63 unused ones (a NULL
   is harmless until called)?
4. Does a `WS_EX_TRANSPARENT` topmost popup receive mouse clicks once dgVoodoo draws into it (T2)?
5. Do the Bink logo movies show under OpenGL at all? The code says they end at the first frame; the 2026-10-14 note
   about `SkipIntroMovies` reads as if they were seen.
6. Which option is `Summoner_options.cfg_34`, copied to `gr_vsync` every frame (`unnamed_render_scene_0x481680.cpp`
   line 2700)? It decides whether Glide / Direct3D swap with vsync and so whether G1 can show on these renderers.
7. Is a custom `GameWidth` (widescreen) possible under Direct3D with dgVoodoo (arbitrary 16-bit modes), and is it
   worth porting the widescreen layout away from `display.c` for it?

## Evidence

No REA evidence IDs: this page was written from the decompiled source and SummonerFix's source, plus
`llvm-objdump` of `Sum.exe` v1.40 for the import table and the addresses below.

| Claim | Where |
|---|---|
| Static `DDRAW.dll!DirectDrawEnumerateA`, no Glide / d3dim import | `llvm-objdump -p Sum.exe`; thunk `0x57b564` |
| Direct3D loads `ddraw.dll`, `DirectDrawCreateEx` | `gr_d3d_init_device` `0x5395c0`, `LoadLibraryA` call `0x5395d3` |
| Voodoo / Banshee / 3dfx refusal | `stristr` calls `0x539e80`, `0x539e9d`, `0x539eba`; block `0x53a125` |
| Shared silent exit | `0x539b41` |
| Glide loads `glide2x.dll`, 112 `GetProcAddress`, unchecked | `glide_load_dll` `0x5473b0`, `LoadLibrary` call `0x5473b5` |
| Glide fullscreen only, 640/800/1024, `fbRam` tests | `gr_glide_init` `0x549d10` |
| Glide texture formats and 4..256 sizes | `0x547cc0`, `0x5484f0`, `0x547c00` |
| Renderer default and window mode | `LoadSettings` `0x43d7f0`: `mov edi, 0x67` `0x43da64`, `push 0xc9` `0x43db45` |
| `WS_EX_TRANSPARENT` for Glide | `gr_init` `0x505f18` (`cmp edi, 0x67`), `0x505f1d` (`push 0x20`) |
| Back-end dispatch | `gr_init` `0x5060a4`, `0x5060ac`, `0x5060b4` |
| Movies only on Glide / Direct3D | `movie_blit_frame` `0x517430` |
| SummonerFix renderer switch | `src/summonerfix/main.c` `LoadConfig`, `ApplyDisplayPatches`, `DllMain`; `reghook.c` `QueryConfigValue` |
| OpenGL-only SummonerFix parts | `display.c` `DisplayInstall`, `Hook_SwapBuffers`; `controller.c` line 281; `gamma.c` header |
