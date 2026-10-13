---
title: "vsdk: packfiles (.vpp)"
parent: Sum.exe
nav_order: 5.6
---

# vsdk: packfiles (`.vpp`)

**Summary:** Phase 1, module 2. `vsdk\vfile\packfile\file_packfile.cpp` loads the game's
`.vpp` archives and serves files from them. The code at `0x533df0`-`0x5349a0` follows the
[Summoner 2 decomp](../reference/engine-references.md)'s `file_packfile.cpp`, with a PC
storage layer (CD and hard drive instead of DVD, HDD and host). 18 functions plus their
tables are named in `symbols/sum.symbols`, and the PC layouts are in `symbols/types.h`.

## Startup

`packfile_init` (`0x533df0`) adds, in order: `tables.vpp`, `summoner.vpp`, `chars.vpp`,
`items.vpp`, `effects.vpp`, `music.vpp`, `sounds.vpp`, `cutscene.vpp`. It then prints
"TOTAL PACKFILE LOAD TIME", "Packfile static alloc", "TOTAL PACKFILE FILES" and related
statistics (**observed** strings).

`packfile_add` (`0x534420`) skips a `.vpp` that is already loaded ("Ignoring packfile (%s)
which was already loaded!"). It then reads it from the CD (`packfile_load_cd`, `0x5341a0`)
or the hard drive (`packfile_load_hdd`, `0x5342e0`), and logs "Adding CD PACKFILE : %s" or
"Adding HARD DRIVE PACKFILE : %s".

## Format details (observed)

| Detail | Value | Where |
|---|---|---|
| Header magic | `0x51890ACE` | `packfile_check_header` (`0x533f60`) |
| Data alignment | 2048-byte sectors: each file starts at `(size + 0x7FF) >> 11` sectors after the previous | `packfile_assign_offsets` (`0x5340d0`) |
| File-name hash | `h = rol(h, 5) ^ tolower(c)` over the name, then the absolute value | `packfile_hash` (`0x534090`) |
| Lookup | Open addressing in a 0x50e9-slot (20,713) `{hash, entry}` table at `0x2d68780`; empty slots are -1 | `packfile_lookup_insert` (`0x533f10`), `packfile_find` (`0x5345f0`) |
| Loaded packfiles | 0x34-byte records at `0x2d68440`, count at `0x2d90ecc` | `packfile_find_packfile` (`0x5345a0`) |

## File API

| Address | Name | Notes |
|---|---|---|
| `0x5345f0` | `packfile_find(name)` | Hash lookup |
| `0x534690` | `packfile_fopen(entry)` | Opens the `.vpp` from the CD or hard-drive root |
| `0x5347e0` | `packfile_fclose(entry)` | |
| `0x534830` | `packfile_fseek(entry, offset, origin)` | `offset + sector * 0x800` |
| `0x5348a0` | `packfile_fread(...)` | "FREAD BAIL" on a short read |
| `0x5348f0` | `packfile_fstatus(entry)` | Always 1 on PC |
| `0x5349a0` | `packfile_find_in_packfile(vpp, name, &offset, &size)` | *Inferred* role |

The C runtime file functions they use are named too: `fopen` `0x56d13c`, `fread`
`0x56fbc8`, `fseek` `0x56f979`, `fclose` `0x56d02c`, and `tolower` `0x56cebe`.

## Why this matters

With the header magic, directory layout, sector alignment and hash known, a small tool can
list and extract `.vpp` contents. That opens up the game's tables, textures and models for
later phases.
