---
title: "AI"
parent: Sum.exe
nav_order: 6.27
---

# AI

**Summary:** every living entity has an AI sub-object, `ai_state`, at `Character +0x324` (layout in
`symbols/types.h`). The AI is a mode/sub-mode state machine with 13 modes, whose names come from the game's own
table at `0x59ad60`. It is driven by **goals**: up to five per entity, each with a type and a priority, and the
most important goal picks the mode. `ai_do_frame` (`0x474950`) updates the entities in a time-limited pass.
Pathfinding provides travel paths, and combat uses five **attacker slots** around each target so that attackers
spread out. Function names and behaviour are from the disassembly (**observed** unless noted); about 120 AI
functions are named in `symbols/sum.symbols` with the prefix `ai_`.

## Modes

`ai_state +0x00` is the mode, `+0x04` the sub-mode. `ai_set_mode` (`0x473320`) does nothing if both are
unchanged. Otherwise it saves the previous pair at `+0x08`/`+0x0c` and frees the current path.
`ai_is_mode(mode, sub)` (`0x473200`, 70 callers) tests the pair; sub-mode `-1` matches any.

| Mode | Name | Notes |
|---|---|---|
| 0 | `AI_STATE_INACTIVE` | `ai_go_inactive` |
| 1 | `AI_STATE_IDLE` | `ai_go_idle` |
| 2 | `AI_STATE_TRAVELING` | sub 0 start (`ai_start_travel`), 1 to a point (`ai_travel_to_xyz`), 2 with a count (`ai_travel_to_point`), 8 timed move |
| 3 | `AI_STATE_COMBAT` | sub `0xf`/`0x1f`/`0x3f` attacking (`ai_is_attacking`), `0x10`, `0x11`, `0x16`/`0x17` other combat steps |
| 4 | `AI_STATE_CAST_SPELL` | sub `0x27` cast, `0x29` cast at a point |
| 5 | `AI_STATE_PATROL` | |
| 6 | `AI_STATE_SUMMONING` | a summon emerging (`ai_begin_emerge`) |
| 7 | `AI_STATE_BANISHING` | |
| 8 | `AI_STATE_DISABLED` | sub `0x2d` knocked down (`ai_knock_down`, shows "Knocked Down!"), `0x2f`, `0x31`, `0x2c` |
| 9 | `AI_STATE_HIDING` | sub 9 or `0xb` (`ai_enter_hiding`) |
| 10 | `AI_STATE_SNEAKING` | sub `0xc`, or `0xd` with owner flag `0x400` (`ai_enter_sneaking`) |
| 11 | `AI_STATE_MOUNT_TRAVEL` | riders on mounts |
| 12 | `AI_STATE_MOUNT_IDLE` | `ai_go_mount_idle` |

The knock-down and some disable functions have `_local` variants. The plain versions also send the change to the
other players in multiplayer.

## Per-frame update

- `ai_do_frame` (`0x474950`) walks the living-entity list (`0x8ecd78`, next `+0x318`). A `RealtimeTimer` and a time
  budget limit it, so a busy level spreads the AI work over several frames.
- `ai_frame` (`0x472a90`) is one entity's update:
  - suspension, and a periodic look-around (`ai_update_perception`, *inferred*) for mounts, sneaking and combat;
  - then the goals, and the handler for the current mode.

## Goals

`ai_state +0x8c` holds five goal slots of 0x34 bytes, and `+0x190` is the current goal. Goals are added with a type
and a priority; the AI sorts and validates them, and the most important one sets the mode.

| Type | Added by | Meaning |
|---|---|---|
| 1 | `ai_goal_add_attack` | attack an enemy (`character_is_enemy`) |
| 3 | `ai_goal_add_cast_spell` | cast a spell |
| 4 | `ai_goal_add_cast_at_target` | cast at a target |
| 5 | `ai_goal_add_cast_at_point` | cast at a point |
| 6 | `ai_set_combat_mode_goal` | stay in combat mode |
| 9 | `ai_goal_add_travel` | travel to a point (priority `0x6c`) |
| `0xc` | `ai_goal_add_move_to` | move to a point (walking sets flag `0x10`) |
| `0xd` | `ai_goal_add_move_to_timed` | move with a time limit (used by boss AI) |
| `0xf` | `ai_goal_add_sneak` | sneak (priority 999999) |

`ai_goal_add_hide` adds the hiding goal, or leaves hiding when the entity is already hiding. The goal handlers
are at `0x4b64c0`-`0x4b6790` (`AI_STATE_*` sub-modes as above).

## Attacker slots

Each target has five attacker slots at `ai_state +0x1b0`, 8 bytes each: an attacker handle and a time. The slot
directions are unit vectors 72 degrees apart around the target (`ai_attack_slot_direction`).
`ai_pick_attack_slot` picks the free slot that best matches the attacker's direction, and `ai_claim_attack_slot`
releases the attacker's old slot and takes the new one. `ai_count_attackers` counts the entities targeting
someone; the AI target is at `Character +0x384` (`ai_state +0x60`).

## Paths

The current path is at `ai_state +0x210` and the destination at `+0x54`..`+0x5c`.
- `ai_path_to` / `ai_path_find_new` call the pathfinder from the bottom of the mount or the entity.
- `ai_path_find_append` extends the path from its end, and `ai_path_begin` drops a path that is already almost
  reached.
- `ai_path_free` releases the nodes; it has 17 callers, including mode changes.

## Saving

`ai_state_save` (`0x4746d0`) and `ai_state_load` (`0x4747e0`) write and read the AI part of a character record in
the [savegame](savegame.md) (`LENT` chunk).

## Bosses

`ai_reset` loads the boss AI (`boss_ai_load`, from the `*_boss.tbl` tables) for type-9 entities and for
Luminar. Each boss has its own stage, frame and attack functions in `0x494000`-`0x4ca000`, named by boss (for
example `serpent_rider_set_flinch`, `tiger_load_anims`, `pyrul_boss_load`). See [Bosses](bosses.md) for the
dispatchers, the HP-based stage machines, each boss's mechanics and the cutscenes tied to them.
