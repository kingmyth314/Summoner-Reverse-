---
title: "vsdk: characters (ca)"
parent: Sum.exe
nav_order: 5.97
---

# vsdk: characters and models (`ca`)

**Summary:** Phase 1, module 7 (started). `vsdk\ca\character.cpp` and `skeleton.cpp`
(`0x540790`-`0x544b80`) load characters, skeletons (`.mvf` animation) and meshes (`.vim`).
`character_instance.cpp` (`0x51a230`-`0x51b4a0`) animates them. Neither Summoner 2 nor Dash
Faction has this code, so names come from the module's own messages (**O**) or behaviour
(**I**). About 25 functions are named so far; the animation maths is not yet mapped.

## Loading

| Address | Name | Message / behaviour |
|---|---|---|
| `0x541450` | `character_load(info, ...)` | "Couldn't load character %s in charanim.tbl file." Reloads the textures if they were freed |
| `0x541500` | `character_create_base` | "No more base character room. Please up the MAX_BASE_CHARACTERS constant." |
| `0x540970` | `character_add_skeleton` | "No more room for skeletons in character %s -- see allender" |
| `0x540df0` | `character_load_shadow` | "ATTEMPTING TO LOAD", "SHADOW LOADED" / "SHADOW FAILED TO LOAD" |
| `0x543d50` / `0x544090` | `skeleton_add` / `skeleton_read` | "Too many skeletons"; "FILE ISN'T MVFREDUCED AND HAS MORPHING!!!", "Skeletons using too much memory" |
| `0x543920` / `0x543430` | `vim_load` / `vim_read` | "VIF LOADING: %s", "Unable to load vim: %s"; textures are loaded through [`bm_load`](vsdk-bmpman.md) |

### Preload index

Each level has a character preload index. `level_load_preload` (`0x447b10`) loads it with
`ca_load_preload_index` (`0x541790`). The file holds:
- a count;
- 0x48-byte name entries with offsets and sizes;
- one data blob, read into `ca_preload_data` (`0x2de20c4`).

`character_add_skeleton` first tries `ca_preload_find` (`0x5418f0`) and reads the skeleton
from memory, and opens the file only if that fails. Both paths use the same reader class.

### `FileReader` (`0x513a20`-`0x513e00`)

`FileReader` is a small class that reads from either a [`File`](vsdk-bmpman.md#related-helpers-named-here)
or a memory buffer. Its layout is `{mode, File*, buffer, pos, size}`. It provides:
- `read`, `read_int`, `read_float`, `read_int16`/`uint16`, `read_int8`/`uint8`;
- `read_vector` and `read_quaternion`;
- `read_string`, `seek`, `tell` and `eof`.

Each method forwards to the matching `File` method when the reader wraps a file. Those
methods are named too: `File::seek` `0x514910`, `tell` `0x5149f0`, and the typed reads at
`0x525850`-`0x525ee0`.

## Start-up: party, fonts and resolution

`GameInit` (`0x43df40`) prints "SYSTEM INIT TIME". It loads the four party characters
(Joseph, Jekhar, Flece, Rosalind) with `character_load` and reads `ComputerSpeed` from the
registry. It also chooses the main UI font from `resolution_index` (`0x60ad68`), the row of
`resolution_table` that `LoadSettings` passes to `gr_init` and `mouse_init`:

| `resolution_index` | Resolution | Font |
|---|---|---|
| 0 | 640x480 | `sumbones640.vf` |
| 1 | 800x600 | `sumbones800.vf` |
| 2 | 1024x768 (SummonerFix's custom resolution slot) | `sumbones1024.vf` |

[SummonerFix](../summonerfix/index.md) puts its resolution in slot 2, so the game always
uses the 1024 font and scales it to fit. Native-resolution UI work (see the
[roadmap](../roadmap.md)) would need larger fonts or font scaling here.

## Effects

`rfx_preload` (`0x51bfd0`, `geom\effect_mem.cpp`) loads `Preload.rfx`. It refuses to run if
the file is too big ("I refuse to run!"), and prints checkpoints "AS 0" to "AS 9" plus "ADD
STREAMING EFFECT". `vfx_preload` (`0x51c9c0`) loads named `.vfx` effects; for example, the
level loader preloads the `Mach_*` effects (MeteorStorm, DarkVoid, teleport in/out, quake)
for the `templeint2` level. These are probably the Machival boss's effects (there is a
`machival_boss.tbl`; *inferred*).

## Still to do

- `character_instance` animation (`0x519510`-`0x51b4a0`): blending, bone matrices.
- The `.vim` format, and the `.mvf` format past what `skeleton_read` shows: a `VMVF` magic and a version
  (every retail `.mvf`, 380 in `chars.vpp` and 291 in `cutscene.vpp`, is version 5 or 8). From version 2 on a
  rotation key is four int16 plus eight int16 that are skipped, and `anim_track__init_tangents` runs only for
  versions below 2 (`0x544643`), so only cutscene camera tracks build squad tangents ([game bugs](game-bugs.md), C262).
- `geom` (`0x51c000`-`0x524000`) and `ui` (`0x52b916`, `0x5536ef`).
