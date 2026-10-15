---
title: "vsdk: packfiles (.vpp)"
parent: Sum.exe
nav_order: 5.6
---

# vsdk: packfiles (`.vpp`)

**Summary:** Phase 1, module 2. `vsdk\vfile\packfile\file_packfile.cpp` loads the game's
`.vpp` archives and serves files from them. The code at `0x533db0`-`0x534a1b` follows the
[Summoner 2 decomp](../reference/engine-references.md)'s `file_packfile.cpp`, with a PC
storage layer (CD and hard drive instead of DVD, HDD and host). The unit has 24 functions (20 plus
the four compiler-generated string-pool initialiser and destructor helpers at `0x533db0`-`0x533de0`);
they and their tables are named in `symbols/sum.symbols`, and the PC layouts are in `symbols/types.h`.
The decompiled unit is [`src/decomp/vsdk/file_packfile.cpp`](../../src/decomp/vsdk/file_packfile.cpp).

{: .important }
Corrected 2026-10-15 from the decompiled unit: 24 functions, not 18; "FREAD BAIL" is printed for a NULL
handle, not a short read; `packfile_find_in_packfile` returns the size before the offset; `packfile_add`
tries the hard drive first; `level_load` adds the three level packfiles.

## Startup

`packfile_init` (`0x533df0`) adds, in order: `tables.vpp`, `summoner.vpp`, `chars.vpp`,
`items.vpp`, `effects.vpp`, `music.vpp`, `sounds.vpp`, `cutscene.vpp`. It then prints
"TOTAL PACKFILE LOAD TIME", "Packfile static alloc", "TOTAL PACKFILE FILES" and related
statistics (**observed** strings).

`packfile_add` (`0x534420`) skips a `.vpp` that is already loaded ("Ignoring packfile (%s)
which was already loaded!"). It then tries `install_path` + name on the hard drive first and
`cd_root_path` + name only when that `fopen` fails (`0x53445c`..`0x5344db`). It reads the
directory with `packfile_load_hdd` (`0x5342e0`) or `packfile_load_cd` (`0x5341a0`) and logs
"Adding HARD DRIVE PACKFILE : %s" or "Adding CD PACKFILE : %s". Limits: 16 packfiles
(`cmp edx, 0x10` at `0x5344ef`), names of at most 31 characters (`0x53450c`), and 13,500
directory entries in all (`0x34bc` x 0x1c bytes; "Packfile static alloc" prints 0x5c490 bytes).
A name that fails these tests is refused only after the file was found and opened. **Observed.**

## Format details (observed)

| Detail | Value | Where |
|---|---|---|
| Header magic | `0x51890ACE` | `packfile_check_header` (`0x533f60`) |
| Data alignment | 2048-byte sectors: each file starts at `(size + 0x7FF) >> 11` sectors after the previous | `packfile_assign_offsets` (`0x5340d0`) |
| File-name hash | `h = rol(h, 5) ^ tolower(c)` over the name, then the absolute value | `packfile_hash` (`0x534090`) |
| Lookup | Open addressing in a 0x50e9-slot (20,713) `{hash, entry}` table at `0x2d68780`; empty slots are -1 | `packfile_lookup_insert` (`0x533f10`), `packfile_find` (`0x5345f0`) |
| Loaded packfiles | 0x34-byte records at `0x2d68440`, count at `0x2d90ecc` | `packfile_find_packfile` (`0x5345a0`) |
| Name used for lookups | Only the file part of the path (`file_get_filename` at `0x534603`). Inserts take the next free slot and `packfile_find` returns the first match, so a file name present in two packfiles resolves to the one added first | `packfile_lookup_insert` (`0x533f10`), `packfile_find` (`0x5345f0`) |

## File API

| Address | Name | Notes |
|---|---|---|
| `0x5345f0` | `packfile_find(name)` | Hash lookup |
| `0x534690` | `packfile_fopen(entry)` | Opens the `.vpp` from the CD or hard-drive root |
| `0x5347e0` | `packfile_fclose(entry)` | |
| `0x534830` | `packfile_fseek(entry, offset, origin)` | `offset + sector * 0x800`, for every origin. Only `SEEK_SET` reaches it: its one caller, `File__seek`, converts to an absolute position and passes origin 0 (`0x5149ac`) |
| `0x5348a0` | `packfile_fread(...)` | "FREAD BAIL" when the handle is NULL or has no packfile (`0x5348a4`..`0x5348ad`); a short read returns `fread`'s smaller count without a message |
| `0x5348f0` | `packfile_fstatus(entry)` | Always 1 on PC |
| `0x5349a0` | `packfile_find_in_packfile(vpp, name, &size, &offset)` | Linear `_stricmp` search of one packfile's directory; size from entry `+0xc`, offset = sector `<< 11` (`0x5349ed`..`0x534a14`). Used by `TextFile__Open` (`0x515d9c`) for the in-memory "blast" buffer |
| `0x534900` | `packfile_read_whole_file(buf, size, vpp)` | Reads a whole `.vpp` (its header size, `+0x30`) into `buf`; opens it by its bare name, relative to the current directory, and returns 1 for a CD packfile without reading. Only caller `file_blast_into_buffer` (`0x517220`), which has no caller |

The C runtime file functions they use are named too: `fopen` `0x56d13c`, `fread`
`0x56fbc8`, `fseek` `0x56f979`, `fclose` `0x56d02c`, and `tolower` `0x56cebe`.

## On-disk layout and extractor

`packfile_load_hdd` (`0x5342e0`) shows the layout. It is the same as Red Faction's VPP
version 1:

1. **Sector 0 (header):** magic `0x51890ACE`, version (1), file count, total size of the `.vpp`
   (`packfile_check_header` stores it at `Packfile +0x30`, `0x533f72`; `packfile_read_whole_file` reads that
   many bytes; `symbols/types.h` calls the field the directory size, D2901).
2. **Directory:** 64-byte entries, 32 per 2048-byte sector. Each entry is a 60-byte
   NUL-padded name followed by a 4-byte size.
3. **Data:** starts at the sector after the directory. Each file starts on a 2048-byte
   boundary, in directory order.

[`tools/vpp.mjs`](../tools.md#toolsvppmjs) lists and extracts packfiles; extracting
`strings.tbl` gave the expected `#english` ... `#end` text. Retail contents (from `list`,
2026-10-13):

| Packfile | Contents |
|---|---|
| `tables.vpp` | 466 `.tbl` tables, 49 `.eax` |
| `summoner.vpp` | 1328 `.tga`, 29 `.vbm`, 7 `.vf` fonts, `.vaf`, `.arr` |
| `chars.vpp` | 1349 `.tga`, 380 `.mvf` skeletons/animations, 83 `.vim` meshes, 147 `.svf` |
| `items.vpp` | 560 `.tga`, 500 `.vim` |
| `effects.vpp` | 193 `.rfx` effects, 246 `.tga`, 54 `.vbm` |
| `cutscene.vpp` | 94 `.csc` cutscene scripts, 291 `.mvf`, 77 `.wav`, 119 `.tga` |
| `levele.vpp` | 50 `.lcf`, 60 `.pfg`, 37 `.vis` (level data) |
| `levelm.vpp` | 60 `.s3d` (level meshes) |
| `levelt.vpp` | 4743 `.tga`, 20 `.vbm` (level textures) |
| `music.vpp` / `sounds.vpp` | 36 / 1332 `.wav` |

`packfile_init` does not list `levele`, `levelm` or `levelt`. `level_load` adds them on every level load,
in the order `levelm.vpp`, `levele.vpp`, `levelt.vpp` (`0x4385f5`..`0x43860e`); after the first time,
`packfile_add` ignores them as already loaded. **Observed.**
