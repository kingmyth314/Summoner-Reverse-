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
symbol file (138 functions). The record layout is `level_script_info` in `symbols/types.h`. The shared code
that calls them, parses the level tables and offers the script helpers is `level_scripts_common.cpp`
(`0x4ca700`..`0x4ceebf`, see [Common code](#common-code-level_scripts_commoncpp)).

## Record layout

| Offset | Slot | Called from |
|---|---|---|
| `+0x00` | level file name (`masad`, `sewer`, `Rand-Desert`, ...) | `level_script_find` |
| `+0x04` | `init` | `level_script_init` (`0x4cc7e0`), from `state_game_enter`; the script editor's restart (`level_script_restart`, `0x4cb060`) calls the slot directly |
| `+0x08` | `frame` | `level_script_frame` (`0x4ccab0`), from `state_game_frame`. It runs only while `Do_frame_callback` is set, `game_flags` bit `0x20` (cutscene) is clear and no cutscene script is loaded, right after `level_check_triggers` |
| `+0x0c` | `leave` | `level_script_leave` (`0x4ccc70`), from `state_game_leave` and the script editor's reload; the restart calls the slot directly |
| `+0x10` | `use (name)` | an object or exit was used. The handler compares the name with `_stricmp`, then opens a gate or has a character speak a refusal ("The lift doesn't work."). It returns 0 |
| `+0x14` | unused | 0 in every record |
| `+0x18` | `action` | script and dialogue actions such as `CheckItems` or `AddRosalind`; also used by `conversation_execute_action` |
| `+0x1c` | loading steps | the loading-screen progress total, read by `LoadingScreenDraw` (default 10000) |
| `+0x20` | music name | looked up with `0x4696d0` (debug mode only) |

`0x4f13e0`, a bare `ret`, serves as the empty handler in many records.

## How it was found

- The [string report](../tools.md#toolsasmmjs) showed about 100 unnamed functions full of quest flags
  (`sewers_flag_crate_comment`, `wolong2_fight1_won`, ...).
- A scan of `.data` for {string pointer, code pointer} pairs found the table.
- The globals' readers gave each slot's role.

## Helpers

Below the table handlers, each level has helper functions that test one story flag and then act. A helper
might speak a line, start a cutscene, open a gate or spawn the next wave of enemies. 72 of them are named
`lsf_<flag>` after the first flag they test (for example `lsf_liangshan_flag_wave2`). These names are
*inferred*: the flag says when the helper runs, not everything it does.

## Common code (`level_scripts_common.cpp`)

Everything in this section is **observed** in the disassembly (`tools/asm.mjs`) unless it is marked
otherwise. It was written while decompiling `src/decomp/game/level_scripts_common.cpp` (92 functions) and
checked again for this page.

### Loading a level

`level_script_reparse` (`0x4cb3a0`, called by `level_script_select`) clears the per-level state (ambient
sounds, effects, script objects, triggers, shops and the clickable-object list `lc_entries`). It arms the
900 ms music timer and parses three tables:

| Parser | File | Sections |
|---|---|---|
| `level_script_parse_table` (`0x4cd5f0`) | `<script>_script.tbl` | `#Town`, `#Navpoints`, `#Objects`, `#Doors` (sounds, `+Locked:`, `+Key:`, saved open state), `#Clicks` (clickable objects with `+Give:` gold and `+Messagebox:` text) |
| `level_items_parse_table` (`0x4cb430`) | `level_items.tbl` | the `#Level:` block of the level. Only records whose `$Script:` is the current script get their navpoint looked up and their item spawned (subject to the save's level-item list) |
| `level_parse_table` (`0x4cb7b0`) | `<level>.tbl` | `#Effects`, `#Ambient Sounds`, `#Triggers`, `#Shopkeepers`, `#Automap`, `#Fog` |

Then `level_find_player_starts` (`0x4caf40`) looks up `$player<entry>-01`..`04`, where the entry is
`level_entry_point` (`0x60a33c`; -1 becomes 1). If the first one is missing, it tries `$player01`..`04`. Its
"Unable to locate player starts ... Find Allender." error can never fire: the test after the loop reads
`level_player_starts[4]`, which is `level_entry_point` itself.

```
4cafc4: cmp esi, 0x60a33c                       ; loop end: esi = &level_player_starts[4]
4cafca: jl 0x4cafa1
4cafcc: mov eax, [4*edi + 0x60a32c]             ; edi = 4: reads level_entry_point (>= 1)
4cafd3: test eax, eax
4cafd5: jne 0x4cb004                            ; always taken
```

`level_script_init` (`0x4cc7e0`, from `state_game_enter`) runs the level's `init` slot and restores the "used"
bit of each `lc_entries` record from the save. When its argument is set, it also scales the monsters' levels:

- single player: one roll of 0.8..1.2 times `cur_character`'s level, rounded, at least 1 and at most the
  script's `+Levelcap:` (`script_filenames.tbl`, default 100). Party characters (type 7) and characters above
  the leader's level keep theirs;
- multiplayer: the host level times the session scale, clamped to 1..cap, 25..cap+25 or 50..cap+50 by the
  difficulty. Four multiplayer levels also switch off traps or enable a boss fight.

### Every frame

`level_script_frame` (`0x4ccab0`):

1. updates the ambient sound emitters (`level_ambient_sounds_frame`);
2. when the 900 ms timer has run out and no cutscene plays, starts the level music, or stops the music
   when the level has none;
3. advances the script objects (`level_script_entries_frame`);
4. only when `game_flags` bit `0x20` is clear and the cutscene script name is empty: runs
   `level_check_triggers` (`0x4caa00`), then the level's `frame` slot if `Do_frame_callback` (console
   command `do_frame_callback`, default 1) is set.

```
4ccafe: test byte ptr [0x5fbfd8], 0x20          ; game_flags: cutscene
4ccb05: jne 0x4ccb36
4ccb08: call 0x4ed760                           ; cutscene_script_table, strlen == 0
4ccb1c: call 0x4caa00                           ; level_check_triggers
4ccb21: mov eax, dword ptr [0x24902ac]          ; level_script_frame_fn
4ccb2a: mov cl, byte ptr [0x59e0fc]             ; Do_frame_callback
4ccb34: jmp eax
```

`level_check_triggers` walks the `$Trigger:` records (32 slots of 0x64 bytes at `0x2490dd0`). A trigger with
bit `0x2` of `+0x54` set calls the level's `use` slot with its name. Any other trigger is a level exit. See
[World map, Leaving an encounter](worldmap.md#leaving-an-encounter) for the checks made before an exit.

### Leaving a level

`level_script_leave` (`0x4ccc70`) stops the level effects that are not marked `once`, removes the ambient
sound emitters, frees the avoid splines and runs the `leave` slot. The ambient loop removes by index while
`level_ambient_remove` compacts the table, so only every other emitter is deleted (`0x4cccbb`). The other
emitters keep their slots in the 128-emitter pool until the game is closed. See [Bugs](game-bugs.md).

### Script helpers

The `script_*` functions are what the `ls_*` handlers call. Some rules differ from what the names suggest:

| Helper | Rule |
|---|---|
| `script_party_member_say` (`0x4cdf30`) | with `game_flags` bit `0x1000` (solo mode) only `cur_character` can speak; without it, any party member with that name speaks (`test ah, 0x10` at `0x4cdf66`) |
| `script_play_cutscene` (`0x4ccf00`) | does nothing when no party member is alive. With `No_cutscenes` or in multiplayer it calls the continuation at once. Otherwise it parks the level's `frame` slot behind `script_cutscene_done` until the cutscene ends |
| `party_count_item_by_name` (`0x4ce9a0`) | identified items only: the party inventory stack plus equipment slots 0..8 of every party member |
| `party_set_level` (`0x4cec00`) | called by `game_stage_set`. Every party member not above the level restarts at level 1 from its `char_info` stats and skills (`+0x94`, 84 bytes, into `Character +0x614`), then gains the experience of that level. `character_add_xp` spends the skill points itself (+10 per point over the progression table), so the function's own spend loop never has points left |
| `script_guard_sees_player` (`0x4ce6f0`) | view range, view cone, blindness, invisibility and a line-of-sight ray; used by the Lenele palace sneak section |
| `level_trigger_set_exit` / `level_trigger_set_script_use` (`0x4ccf90` / `0x4ccff0`) | clear / set bit `0x2` of every trigger with that name (and script). `level_trigger_set_exit_by_refname` / `level_trigger_set_script_use_by_refname` (`0x4cd060` / `0x4cd0c0`) do the same for the first trigger with that `+RefName:` only, so "disable" makes the trigger a live exit |

## Evidence

Function and instruction addresses (no REA evidence ids; listings from `tools/asm.mjs`).

| Claim | Where |
|---|---|
| frame slot conditions | `level_script_frame` `0x4ccab0`..`0x4ccb36` |
| init slot, level scaling | `level_script_init` `0x4cc7e1`..`0x4cc7ea`, `0x4cc863`..`0x4cc914`, `0x4cc951`..`0x4cca2c`; `script_filenames_parse_table` `0x4d626b`..`0x4d628e` (default 100) |
| restart calls the slots directly | `level_script_restart` `0x4cb060`..`0x4cb083` |
| reparse and the three parsers | `level_script_reparse` `0x4cb3a0`..`0x4cb427` |
| level items only for the current script | `level_items_parse_table` `0x4cb5d6`..`0x4cb5f8`, `0x4cb680`..`0x4cb6a3` |
| player-start error never fires | `level_find_player_starts` `0x4cafc4`..`0x4cafd5` |
| every other ambient emitter freed | `level_script_leave` `0x4cccaf`..`0x4ccccc`; `level_ambient_remove` `0x4cd2c0`..`0x4cd320` |
| solo-mode speech | `script_party_member_say` `0x4cdf55`..`0x4cdf69` |
| cutscene parking | `script_play_cutscene` `0x4ccf42`..`0x4ccf79`; `script_cutscene_done` `0x4ccdf0`..`0x4cce0c` |
| `party_set_level` skills and spend | `party_set_level` `0x4ceca8`..`0x4cecc2`, `0x4cecd8`..`0x4ced47`; `character_add_xp` `0x44fecb`..`0x44ffa6` |
| refname helpers, first match only | `level_trigger_set_exit_by_refname` `0x4cd077`..`0x4cd0ac` |

## Changes (2026-10-15, level_scripts_common.cpp)

{: .important }
The `frame` row said only "while `Do_frame_callback` is set". The slot also needs no cutscene (flag or
script) and runs after `level_check_triggers`. The `init` and `leave` rows now say that the script editor's
restart calls the slots directly.

- Added [Common code](#common-code-level_scripts_commoncpp): level loading, the frame order, leaving a
  level, and the helpers whose rules differ from what their names suggest.
