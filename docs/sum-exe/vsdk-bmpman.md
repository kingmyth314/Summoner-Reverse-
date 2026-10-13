---
title: "vsdk: bitmap manager"
parent: Sum.exe
nav_order: 5.7
---

# vsdk: bitmap manager (`bmpman.cpp`)

**Summary:** Phase 1, module 3. `vsdk\bmpman\bmpman.cpp` (`0x501c20`-`0x503cf0`) loads and
caches every texture and image. 45 of its functions are named in `symbols/sum.symbols`, and
the entry layout is `bitmap_entry` in `symbols/types.h`. Summoner 2's decomp has no bitmap
code, so names come from the module's own error strings and from
[Dash Faction](../reference/engine-references.md)'s `rf::bm` API, which descends from this code.
Names are confidence **O**, or **I** where only behaviour supports them.

## Design

- **Table:** `bm_bitmaps` (`0x2591c78`) has 2,800 (0xaf0) entries of 0x48 bytes. A
  handle is simply the entry index.
- **Lists:** entries move between a free list (`0x25c5c18`) and a used list (`0x25c3000`).
  Both sentinels are entry-shaped, with `next` at +0x2C and `prev` at +0x30.
- **Hash:** names are hashed into `bm_hash_table` (`0x257f0b0`, 0xaf1 slots, open
  addressing). `bm_hash_name` (`0x502650`) is `rol(h, 6) ^ tolower(c)`, then the absolute
  value. That is one bit of rotation more than the [packfile hash](vsdk-packfile.md).
- **Names:** stored once in `bm_name_pool` (`0x2581c74`), a 64 KB `StringPool`.
- **Animations:** take a contiguous run of entries. The header has `anim_type` 1 and the
  frame count; each frame has `anim_type` 2 and its frame number. `bm_get_current_frame`
  (`0x502560`) picks the frame for the current time.
- **Mark/release:** `bm_set_mark` (`0x501eb0`) snapshots the hash table and the name-pool
  length. `bm_release_from_mark` (`0x501f20`) frees everything loaded since (for example,
  per level). The asserts name both functions (**observed**).

### Bitmap types and formats

| `type` | Extension | Loader |
|---|---|---|
| 1 | `.pcx` | `bm_lock_pcx` (`0x503aa0`) |
| 2 | `.tga` | `bm_lock_tga` (`0x503710`); mip levels in `name_mip%d.tga` files |
| 3 | (user) | `bm_create` (`0x503cf0`), named "USERBMAP" |
| 4 | `.vbm` | `bm_lock_vbm` (`0x5038f0`); header read by `bm_read_vbm_header` (`0x501d20`) |
| 5 | `.m2v` | `bm_lock_m2v` (`0x503b20`) |

This numbering differs from Red Faction's, where 4 is VAF and 5 VBM. `bm_prefer_vbm`
(`0x502f70`) substitutes `name.vbm` for a requested `name.tga` when it exists.

The pixel `format` uses the same numbers as Dash Faction's `bm::Format`.
`bm_bytes_per_pixel` (`0x502450`) gives 1 for 8-bit, 2 for 565/4444/1555/88, 3 for 888 and 4
for 8888. `bm_has_alpha` (`0x5035e0`) is true for 4444, 1555 and 8888.

### Missing files

`bm_load_internal` (`0x502720`) does not fail on a missing file. `bm_create_missing`
(`0x5030a0`) registers a 32x32 user bitmap under the missing name. Its pixels
(`0x25c5c8c`, built by `bm_init`) are a pink-grey fill with a grey line every 8 pixels:
Summoner's "missing texture" pattern.

## Main entry points

| Address | Name | Notes |
|---|---|---|
| `0x502320` / `0x502090` | `bm_init` / `bm_close` | |
| `0x5026e0` | `bm_load(name, path_id)` | Optional preload (`bm_preload_on_load`, `0x25c6894`) |
| `0x5025d0` | `bm_find(name)` | |
| `0x502a00` | `bm_read_header` | Named by its own error message |
| `0x503650` / `0x503b50` | `bm_lock` / `bm_unlock` | Same as Dash Faction's `bm::lock`/`unlock` |
| `0x503500`, `0x503550`, `0x5035c0`, `0x503b60` | `bm_get_dimensions`, `bm_get_mipmap_info`, `bm_get_format`, `bm_get_filename` | Signatures match Dash Faction's |
| `0x503c80` | `bm_release` | Also frees texture-cache slots (`0x508020`, renderer side) |
| `0x5021e0` / `0x502240` | `bm_set_reduce_textures` / `bm_set_reduced` | Half-size textures (*inferred*: a low-detail option) |

## Related helpers named here

- **`File`** (`vsdk\vfile`): the same class as Red Faction's `rf::File` in Dash Faction.
  - `File::File` `0x5140d0`, `~File` `0x5140f0`
  - `find` `0x5141c0`, `open(name, mode, path_id)` `0x514630`, `close` `0x514740`
  - `read(buf, len, min_ver, unused)` `0x525fe0`, `read_int` `0x525990`
  - `close` releases either a C `FILE*` or a [packfile](vsdk-packfile.md) entry.
- **`StringPool`**: `Reset` (`0x544bd0`) and `Add` (`0x544c40`, "Length of string pool
  exceeded") match Summoner 2. `GetLength` (`0x544c10`) and `SetLength` (`0x544e30`) are PC
  helpers used by the mark/release mechanism.
- **C runtime:** `atexit` (`0x56d1cd`).

## Image readers and writers (`targautils.cpp`, `0x554b60`-`0x5565e0`)

| Format | Read | Write |
|---|---|---|
| TGA | `tga_read_header` `0x555110`, `tga_read_bitmap` `0x555450` (RLE: `tga_rle_decode` `0x555330`) | `tga_write` `0x555b50` (type 2), `tga_write_rle` `0x555970` (type 10) |
| PCX | `pcx_read_header` `0x555cf0`, `pcx_read_bitmap` `0x555f60` | `pcx_write` `0x5562a0` |
| M2V | `m2v_read_header` `0x554b60`, `m2v_read_bitmap` `0x554d00` (whole file loaded raw) | |
| JPEG | | `jpeg_write_file` `0x5565e0` |

**Screenshots can be JPEG:** [`gr_screenshot`](vsdk-gr.md#drawing) writes either an RLE
`.tga` or a `.jpg`. The JPEG writer drives the Independent JPEG Group's libjpeg, which is
statically linked: its messages, such as "Bogus marker", are in `.rdata`. The libjpeg
functions after `0x5565e0` are library code and do not need decompiling; they can be
matched against the IJG source later.

File helpers named alongside: `File::size` (`0x514860`), `File::error` (`0x514a40`),
`File::write` (`0x526410`) with `write_uint8`/`write_uint16`, `file_change_ext`
(`0x5130a0`), and the C runtime's `fwrite` (`0x56fcdf`).
