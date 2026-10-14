---
title: "Level scripts"
parent: Sum.exe
nav_order: 6.3
---

# Level scripts

**Summary:** each level's hard-coded story logic is a set of C functions. They are found through
`level_script_table` (`0x59f128`), which has 51 records of 0x40 bytes, one per level file. When a level loads,
`level_script_select` (`0x4d5570`) finds the record for `level_filename` (`0x60a2c8`). It then copies the
record's handlers into globals, which the game states call. The handlers are named `ls_<level>_<slot>` in the
symbol file (138 functions). The record layout is `level_script_info` in `symbols/types.h`.

## Record layout

| Offset | Slot | Called from |
|---|---|---|
| `+0x00` | level file name (`masad`, `sewer`, `Rand-Desert`, ...) | `level_script_find` |
| `+0x04` | `init` | `level_script_init` (`0x4cc7e0`), from `state_game_enter`; also on restart |
| `+0x08` | `frame` | `level_script_frame` (`0x4ccab0`), from `state_game_frame`, while `Do_frame_callback` is set |
| `+0x0c` | `leave` | `level_script_leave` (`0x4ccc70`), from `state_game_leave`; also on restart |
| `+0x10` | `use (name)` | an object or exit was used. The handler compares the name with `_stricmp`, then opens a gate or has a character speak a refusal ("The lift doesn't work."). It returns 0 |
| `+0x14` | unused | 0 in every record |
| `+0x18` | `action` | script and dialogue actions such as `CheckItems` or `AddRosalind`; also used by `journal_format_event` |
| `+0x1c` | loading steps | the loading-screen progress total, read by `LoadingScreenDraw` (default 10000) |
| `+0x20` | music name | looked up with `0x4696d0` (debug mode only) |

`0x4f13e0`, a bare `ret`, serves as the empty handler in many records.

## How it was found

- The [string report](../tools.md#toolsasmmjs) showed about 100 unnamed functions full of quest flags
  (`sewers_flag_crate_comment`, `wolong2_fight1_won`, ...).
- A scan of `.data` for {string pointer, code pointer} pairs found the table.
- The globals' readers gave each slot's role.
