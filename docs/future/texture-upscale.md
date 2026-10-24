---
title: Texture upscaling (research)
---

# Texture upscaling (research, 2026-10-23)

**Summary:** Higher-resolution textures for the 3D world, characters, items and effects need **no code at all**. The
game looks for a loose file on disk before it looks in the `.vpp` packfiles. A 4x copy of a character texture dropped
into the install folder was loaded by the original, unmodified Sum.exe and mapped correctly (**Observed**). There are
two catches:
- Interface images and fonts are drawn by pixel rectangle, so an upscaled one shows only part of itself (**Observed**
  on the main menu). They need a small renderer change: an experimental-build feature.
- An upscaled texture also needs upscaled mip files, or it is drawn without mipmapping (**Observed**).

The textures would be made by each player from their own copy of the game, with a script from this repository. The
upscaled images are derivative works of the game's art, so they are never committed or published here.

## What the game has

A census of the six packfiles that hold images (**Observed**, read from the packfile directories and image headers):

| | Count |
|---|---|
| Image files | 8,461 (563 MB), 10,741 frames counting animations |
| TGA, 24-bit | 7,106 |
| TGA, 32-bit (with alpha) | 1,246 |
| VBM (16-bit, mostly animations) | 104 |
| TGA 8-bit, PCX | 5, and a few PCX |
| Largest side <= 64 / <= 128 / <= 256 / <= 512 / > 512 | 4,793 / 2,157 / 1,204 / 196 / 111 |
| By packfile | `levelt.vpp` 4,768, `chars.vpp` 1,357, `summoner.vpp` 1,357, `items.vpp` 560, `effects.vpp` 300, `cutscene.vpp` 119 |

Most textures are tiny by modern standards: 83% are 128 pixels or smaller on their longest side.

## How the game finds a texture

`File::find` (`0x5141c0`, `src/decomp/vsdk/unnamed_vfile_0x5128b0.cpp`) is called with `PATH_ANY` for
bitmaps. For a `.tga` name it tries every data folder whose extension list contains `.tga`, on disk
(`File::find_at`: `_stat` first). Only when no folder has the file does it fall back to the packfiles
(**Observed** in the source, which is a byte match). The folders, from `setup_data_paths` (`game/summoner.cpp`):

| Folder | Extensions |
|---|---|
| `data\maps\levels`, `data\maps\characters`, `data\maps\items`, `data\maps\effects`, `data\maps\cutscenes` | `.vbm .tga .m2v` |
| `data\effects\characters`, `...\cutscenes`, `...\items`, `...\levels`, `...\spells` | `.tga .m2v .vfx .vbm` |
| `data\interface\pc` | `.vbm .tga .m2v .vaf` |
| `data\fonts` | `.vf .tga .vbm` |

So a texture pack is flat loose files in these folders; which of the `.tga` folders a file sits in does not matter for
lookup. A pack should **not** be a new `.vpp`: the game allows 16 packfiles and 13,500 directory entries in all
([packfiles](../sum-exe/vsdk-packfile.md)), and a packfile entry is found only when no loose file exists anyway.
`bm_prefer_vbm` (`0x502f70`) uses `name.vbm` in place of a requested `name.tga` when one exists, so the 104 VBMs need
a VBM writer; they are left for later.

## The test (2026-10-23)

In the test folder, with the original Sum.exe, SummonerFix, OpenGL. Each test image was scaled with plain pixel
repetition and given a magenta grid, so the screenshot shows whether the loose file was used. The files were removed
afterwards; the real game folder was not touched.

| Test | Result (**Observed**) |
|---|---|
| `Torso_Uwear_Josp.tga` (Joseph's tunic), 256x256 32-bit, made 1024x1024, in `data\maps\characters` | Loaded and mapped correctly on the model: the grid follows the tunic. |
| The same, mip files left as they are | The grid shows as broken dots: no mipmapping (below). |
| `MainMenu-bkgrnd1024.tga`, 1024x768, made 2048x1536, in `data\interface\pc` | Loaded, but the menu shows only the top-left quarter of the image, enlarged to the whole screen. |
| `Joseph01.tga`, 4x | No visible change: Joseph's model does not use it. `Joseph.vim` names `Joseph05.tga` and the `*_uwear_josp.tga` set. |

The game's memory in a level with the test running: about 110 MB private and 403 MB of address space, out of the
2 GB a 32-bit program without the large-address flag gets (**Observed**, Task Manager figures).

## Mipmaps

A TGA's smaller mip levels are separate files, `<name>-mip1.tga`, `-mip2.tga` and so on. `bm_read_header` counts them
only while each is exactly half the size of the one before and has the same depth (`bmpman.cpp`, around `0x502b4a`;
**Observed** in the source). An upscaled base next to the packfile's old mip files therefore ends the chain at level 0.
The game then draws that texture without mipmapping, which is what made the test grid break into dots (**Observed**).
Nothing crashes, but distant surfaces shimmer. A pack must write new `-mip<n>.tga` files at the new sizes, one more
level per doubling.

## Limits

- **OpenGL:** the texture size is capped at SummonerFix's `MaxTextureSize` (2048; the game allocates size x size x 4
  bytes for it). `gr_opengl_create_texture` clamps the power-of-two texture size to the cap but still converts the
  whole image (`gr_opengl.cpp`, about line 626). An image larger than the cap is not safe, so the cap is a hard limit
  (**Inferred** from the source; not tried).
- **Glide:** mipmapped bitmaps above 256x256 are refused (`gr_glide_round_texture_size` `0x547c00`,
  [dgVoodoo 2](dgvoodoo.md)). An upscaled pack is OpenGL only (and maybe Direct3D, whose limit is the device's).
- **Factor by size:** 111 textures are already over 512 pixels, so a single 4x factor would pass the 2048 cap.
  Proposed: 4x up to 256, 2x up to 512, unchanged above.
- **Memory (Inferred):** a 4x texture has 16 times the pixels. Levels load and free their own textures
  (`bm_set_mark` / `bm_release_from_mark`), so not all 563 MB are resident, but a heavy level could approach the
  2 GB limit. Measure a heavy level first; the fixed and experimental builds could also link with
  `/LARGEADDRESSAWARE` (4 GB on 64-bit Windows) once the game's own allocators are checked.
- **Disk and load time:** uncompressed TGAs at these factors come to several GB, and levels load more slowly.

## The interface (code change)

**Done 2026-10-23** in the experimental build, `[Experimental] HiResInterface=1` (`experimental/hires.cpp`). Tested
in game (**Observed**): with a 2x `MainMenu-bkgrnd1024.tga` the whole image is drawn at its proper size and the menu
buttons and clicks work; `experimental.log` reports "MainMenu-bkgrnd1024.tga 2048x1536 drawn as 1024x768". A 4x
character texture was drawn correctly in the same run.

Interface images and fonts are drawn with `gr_bitmap` / `gr_bitmap_ex`: a rectangle of the bitmap given in **pixels**
(`sx`, `sy`, `w`, `h`). Font glyphs too (`gr_string` -> `gr_bitmap_ex`, `gr_font.cpp`). The OpenGL renderer turns
those pixels into texture coordinates by dividing by the bitmap's width and height (`gr_opengl.cpp` about line 1258:
`u0 = sx * u_scale / bw`). With a 2x file, `bw` doubles and the same rectangle covers a quarter of the area: the
main-menu result above.

The design, for the experimental build behind its own ini switch:
1. Keep the **original** size of every bitmap in `bitmap_entry`. It can be read from the packfile copy
   (`packfile_find`) even when a loose file is used, so no side file is needed.
2. Upload the real (large) image to the GPU.
3. In the bitmap path, divide by the original size instead of the real one. The on-screen size is unchanged
   (callers pass it), the source rectangle maps to the right place, and the extra pixels give the sharpness.

3D drawing (world, characters, effects) uses texture coordinates from 0 to 1 and needs nothing.

As built:
- `bm_load_internal` notes, for a loose file whose packfile copy is smaller by the same factor in both directions,
  the copy's size (read from its header with `packfile_fopen` / `packfile_fread`).
- `bm_get_dimensions` returns that size while the OpenGL renderer is active; `bm_get_mipmap_info`, which the texture
  upload reads, keeps the real one. Direct3D and Glide cut textures into sections by the real size
  (`gr_poly_get_section`, the D3D texture cache), so there the switch changes nothing.
- The two places that read pixels themselves take the real size: the interface click masks
  (`ui_container::get_mask_pixel`) and the projected-light sample (`light_sample_projected_bitmap`).
- Entries are forgotten when `bm_alloc_entry` / `bm_alloc_entries` hand them out again.

**Fonts are separate.** A font's pixels are inside its `.vf` file and become a bitmap through `bm_create`
(`gr_font.cpp`), not a TGA lookup, so upscaled fonts need a `.vf` writer and a nominal size for that bitmap. Not done
yet.

## Making the textures

The pipeline is a script in `tools/`, run by the player on their own install:
1. Extract the images with `tools/vpp.mjs`.
2. Upscale them. Two quality issues need handling:
   - **Alpha:** 1,246 textures have it, and the Glide path uses pure green as a transparent key. Upscalers smear
     both, so colour and alpha / mask are scaled separately and the mask is re-thresholded.
   - **Seams:** tiling textures must be scaled with wrap-around.
3. Write `-mip<n>.tga` levels.
4. Place the files in the folders above.

**Upscaler choice is open.** ESRGAN-family models trained on game textures are the usual choice, plus a plain filter
(Lanczos or xBRZ) as a baseline to compare looks. No model or runtime was picked or downloaded; a download needs the
user's permission (file, source, size).

**Never in the repository:** extracted or upscaled images (`extracted/` is git-ignored). Only the script.

**As built (2026-10-23): `tools/texture-upscale.mjs`** with Real-ESRGAN's ncnn-vulkan build
(`realesrgan-ncnn-vulkan-20220424-windows.zip`, GitHub xinntao/Real-ESRGAN release v0.2.5.0, 45,474,481 bytes,
SHA-256 abc02804e17982a3be33675e4d471e91ea374e65b70167abc09e31acb412802d; BSD-3-Clause; kept outside the repository).
It runs on the GPU through Vulkan (tested on an Intel Arc 140V).
- **What it handles:** the size factors above; the wrap padding; alpha upscaled separately and re-thresholded when it
  was only 0 / 255; the green key filled before upscaling and restored afterwards; new mip chains.
- **Where the files go:** the game reads a bitmap's header from any data folder but its pixels only from the folder
  the load named (`bm_read_header` with `PATH_ANY`, `bm_lock` with the entry's `path_id`). So `levelt.vpp`'s textures
  go to `datamapslevels` (`s3d` loads them with `path_maps_levels`) and the rest to `datainterfacepc`.
- **Samples (Observed):** five textures (a tunic, a stone relief, iron bars, an arrow streak, a sword icon) with the
  general model (`realesrgan-x4plus`) and the anime model. The general one keeps stone grain; the anime one paints
  it smooth. Both keep the bars' and the streak's transparency clean.
- **Speed and size (Observed, 40 level textures):** about 2 s per texture and 2.3 MB of output each, with mips. The
  whole set is a few hours and several GB.

## The full run (2026-10-24)

The first full run (5,311 textures, about 3 hours, 5.1 GB with mips) showed three problems, all fixed in the script:

1. **A crash in the original game** (**Observed**): an access violation in the TGA reader (`Sum-orig.exe+0x555430`,
   from `bm_lock_tga`) right after the main menu. 50 texture names exist in two packfiles, 7 with different sizes
   (`chicken1.tga` is 3 KB in `chars.vpp`, 127 KB in `levelt.vpp`). The game uses only the first packfile's copy,
   but the script had upscaled both into different folders. The header then came from one file and the pixels from
   the other, and the larger one overflowed the buffer. Now the script takes each name from the packfile the game
   finds first, and every file is in both folders (hard links, no extra space). A check of the repaired pack: all
   10,585 files are in both folders, and every texture is exactly 2x or 4x the copy the game uses.
2. **Broken images from the GPU** (**Observed**): about a fifth of the textures came out black or as noise in the
   200-image batches on the Intel Arc 140V, although the same image alone came out right (the failure the upscaler's
   documentation describes for non-Nvidia GPUs). A fixed tile size (`-t 128`) did not help. The script now checks
   every model output against its input (shrunk back, mean difference; good results score under 10, broken ones
   40-200), runs a failed one again alone up to three times, and uses a plain bilinear enlargement if it still fails.
   On a 200-texture test: 57 run again, 56 fixed, 1 bilinear, none broken.
4. **Darker textures** (**Observed**): about 450 dark, speckled textures came out much darker, the same every time
   (a floor from mean brightness 46 to 19): the model smooths fine speckle it takes for noise. The script now puts
   the original's tone back (the difference between the input and the output shrunk to the input's size, enlarged
   smoothly and added), which keeps the model's detail. The broken-output test is now structural (the shrunk output
   must correlate with the input), since a correct result can differ in tone. On a 200-texture test after the
   change: no brightness shift, 198 within the difference threshold, 2 bilinear.
3. **The original game cannot use upscaled interface images** (**Observed**): its menus draw, but the click masks
   are read at the wrong place, so "Load Saved Game" started a new game. With the experimental build and
   `[Experimental] HiResInterface=1` the menus, the Load screen clicks and the game work; the 3D world and the HUD
   looked right. A pack for the original game would have to leave the interface images out.

## Decisions for the user

1. Which model (the user chose AI upscaling, 2026-10-23; Real-ESRGAN's ncnn-vulkan build is the first candidate,
   pending a download approval).
2. Whether to try the large-address flag in the fixed and experimental builds.
