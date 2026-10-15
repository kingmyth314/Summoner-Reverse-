---
title: "AI"
parent: Sum.exe
nav_order: 6.27
---

# AI

**Summary:** every living entity has an AI sub-object, `ai_state`, at `Character +0x324` (layout in
`symbols/types.h`). The AI is a mode/sub-mode state machine with 13 modes, whose names come from the game's own
table at `0x59ad60`. It is driven by **goals**: up to five per entity, each with a type and a priority, and the
most important goal picks the mode. `ai_do_frame` (`0x474950`) calls `ai_frame` for every living entity every
frame. There is no time budget. Off-screen characters are throttled inside `ai_frame` instead: they run about
every 500-750 ms. Pathfinding provides travel paths, and combat uses five **attacker slots** around each target
so that attackers spread out. Function names and behaviour are from the disassembly (**observed** unless noted);
about 210 functions in `symbols/sum.symbols` have the prefix `ai_`. The decompiled unit is
[`src/decomp/game/ai.cpp`](../../src/decomp/game/ai.cpp).

{: .important }
**Corrected 2026-10-15.** `ai_do_frame` has no time budget. `ai_set_mode` does not free the path. Goal value
`0x6c` is the "no spell" argument, not a priority. Knock-down starts in sub-mode `0x2c`, not `0x2d`.
`ai_pick_attack_slot` does not look for a free slot. See "Changes (2026-10-15)" at the end.

## Modes

`ai_state +0x00` is the mode, `+0x04` the sub-mode. `ai_set_mode` (`0x473320`) returns 0 and does nothing if
both are unchanged. It also refuses `AI_STATE_DISABLED` while the entity is casting or in sub-mode `0xe`, `0xc`
or `0xd` (`0x47333e`..`0x47335e`). Otherwise it saves the previous pair at `+0x08`/`+0x0c`, releases the handle
slot at `+0x80` (`handle_slot_release`, `0x473386`), stores the new pair and restarts the mode timer `+0x1c`.
It does **not** free the path. At the end it only clears `+0x1f8` and bit 2 of `+0x230` (`0x4735bb`..`0x4735cd`).
`ai_is_mode(mode, sub)` (`0x473200`) tests the pair; sub-mode `-1` matches any.

`ai_set_mode` has three detours (**observed**):
- Leaving `AI_STATE_HIDING` other than through sub `0xb` turns the request into sub `0xb` (leave hiding). The
  requested pair waits at `+0x10`/`+0x14` (`0x4733c3`..`0x47342c`).
- A sneaking owner (Character flag `0x400`) that asks for any mode but traveling, sneaking or idle gets the sneak
  goal (`ai_goal_add_sneak`, `0x47344f`). The new mode is already stored by then.
- Leaving combat frees the attack point the entity held on its target (`0x47348a`..`0x4734c7`).

Sub-mode names come from the game's table `ai_substate_names` (`0x59ad94`). The table matches the code up to
`0x21`. From the cast sub-modes on, the code's values are five higher than the table index: the code casts in sub
`0x27`, which is table entry `0x22` `AIS_SPELL`. The names above `0x21` are therefore *inferred* by that shift.

| Mode | Name | Sub-modes used |
|---|---|---|
| 0 | `AI_STATE_INACTIVE` | `ai_go_inactive` |
| 1 | `AI_STATE_IDLE` | `ai_go_idle` clears the target and the priority |
| 2 | `AI_STATE_TRAVELING` | 0 `AIS_PATH_TRAVELING` (`ai_start_travel`), 1 `AIS_DIRECT_TRAVELING` (`ai_travel_to_xyz`), 2 `AIS_TURN_TO_TARGET` (`ai_travel_to_point`, from `character_walk_to_point`), 6 `AIS_GOTO_NAVPOINT` (clears the target), 8 `AIS_WANDER` (`ai_goal_do_move_to_timed`) |
| 3 | `AI_STATE_COMBAT` | `0xe` `AIS_TRANSITION`, `0xf` `AIS_CLOSE_COMBAT`, `0x10` `AIS_RANGED_COMBAT`, `0x11` `AIS_BACKSTAB`, `0x15` `AIS_CHOOSE_SPELL` (waits 0.5-1.5 s), `0x16`/`0x17` `AIS_GET_CLOSE`/`AIS_GET_FAR`, `0x1f` `AIS_RIDER_CHARGE` (`tiger_approach_target`), `0x3f` charge (`ai_charge_target`; no name in the table). `ai_is_attacking` is true for `0xf`, `0x1f` and `0x3f` |
| 4 | `AI_STATE_CAST_SPELL` | `0x27` cast, `0x29` cast at a point (*inferred* names `AIS_SPELL`, `AIS_LOCATION_SPELL`) |
| 5 | `AI_STATE_PATROL` | no handler in `ai_frame` |
| 6 | `AI_STATE_SUMMONING` | a summon emerging (`ai_begin_emerge`, then `summon_emerge_update`) |
| 7 | `AI_STATE_BANISHING` | no handler in `ai_frame` |
| 8 | `AI_STATE_DISABLED` | see [Disabled](#disabled) |
| 9 | `AI_STATE_HIDING` | 9 `AIS_ENTER_HIDING` or `0xb` `AIS_LEAVE_HIDING` (`ai_enter_hiding`) |
| 10 | `AI_STATE_SNEAKING` | `0xc` `AIS_ENTER_SNEAKING`, or `0xd` `AIS_LEAVE_SNEAKING` with owner flag `0x400` (`ai_enter_sneaking`) |
| 11 | `AI_STATE_MOUNT_TRAVEL` | riders on mounts; no handler in `ai_frame` |
| 12 | `AI_STATE_MOUNT_IDLE` | `ai_go_mount_idle` |

`ai_net_attack_target` uses sub `0x11` on the multiplayer host and `0xf` elsewhere.

## Disabled

`ai_mode_disabled_frame` (`0x4b4ad0`, jump table `0x4b4d5c`) runs the disabled sub-modes (**observed**; names
*inferred* by the five-step shift):

| Sub | Name | Set by | What happens |
|---|---|---|---|
| `0x2c` | `AIS_TRIPPED_FALL` | `ai_knock_down_local` | plays move (9, `0x8e`); on its trigger goes to `0x2d` |
| `0x2d` | `AIS_TRIPPED_DOWN` | the frame above | lies down until the mode timer passes `disable_ms` (`+0x1a4`), then `0x2e` |
| `0x2e` | `AIS_TRIPPED_UP` | the frame above | plays move (9, `0x90`); on its trigger goes idle |
| `0x2f` | `AIS_PUSHED` | `ai_disable_2f_local` | plays move (9, `0x91`); when it ends, returns to the previous mode |
| `0x30` | `AIS_PUSH_STUNNED` | not traced | plays move (9, `0x91`), then returns to the previous mode and adds status `0xa` |
| `0x31` | `AIS_STUNNED` | `ai_disable_31` | goes idle after 10 s (`cmp eax, 0x2710` at `0x4b4d43`) |

`ai_knock_down_local` (`0x4742b0`) first asks `ai_can_be_knocked_down` (`0x475030`). That refuses characters
with char_info flag `0x8`, in sub `0xe`/`0xc`/`0xd`, with status `0xa`, on a mount, or already in sub `0x2d` or
`0x2e`. Otherwise it stores the duration, sets sub `0x2c` and shows "Knocked Down!" for `cur_character`.
`ai_is_knocked_down` (`0x474fe0`) tests sub `0x2d`, the lying phase.

`ai_knock_down_local` also has a branch for a character that is already down (`0x4742c5`..`0x4742f8`). It can
never run, because `ai_can_be_knocked_down` has already refused that case (`0x47506c`). A knock-down while
casting passes both tests, but `ai_set_mode` refuses it. The message still shows (see
[Bugs](game-bugs.md)). The plain `ai_knock_down` and `ai_disable_2f` also send the change to the other players
in multiplayer (`mp_broadcast_ai_disable`). `mp_handle_ai_disable` calls the `_local` versions.

## Per-frame update

- `ai_do_frame` (`0x474950`) walks `obj_active_list` (`0x970770`). Types 4, 5, 7 and 9 get `ai_frame`, every
  frame. Type 4 runs even with the dead flag; the others only while not dead (jump table `0x474cc4`,
  `0x474bdd`..`0x474be9`). There is **no time budget**. The `RealtimeTimer` at `0x22e4908` only restarts the
  500 ms averages of the profiler counters (`0x47497d`..`0x474b24`). Before the loop it ages the five tracked cast
  effects and runs `ai_groups_frame` (not during a cutscene script; ten groups of up to six members that share
  an enemy list, see [Party](party.md#combat-behaviour)). After it, each boss gets
  `boss_ai_do_frame_hook`.
- `ai_frame` (`0x472a90`) is one entity's update. It is cdecl on the character. In order:
  - multiplayer travel flags, then `character_ai_suspended` (which reads `Ai_paused`);
  - the frame time is added to `Character +0x610` (cutscene time in a cutscene);
  - `ai_update_perception` (`0x472e80`) decides whether the AI runs this frame (below);
  - the sneak goal timer, hidden characters forced inactive, then `ai_think` (`0x4b7610`) where this machine
    controls the character;
  - the mode handler (jump table `0x472e40`), then `character_ai_move_frame`, the multiplayer movement updates
    and `character_on_death`.

`ai_update_perception` is a misleading name: it is an activity cull, not a look-around (**observed** code; the
meaning of the flags is *inferred*). It returns true to skip the rest of `ai_frame`.
- Always run: party members, characters in combat, in a cutscene, mounted, in sub `0xc`/`0xd`/`0xe`, dead, and
  bosses. Characters with object flag `0x2` also run every frame.
- Always skipped: characters that are neither active (object flag `0x1`) nor summons, and hidden characters.
- The rest run only when their timer `+0x4c0` expires, every 500-750 ms (`SetRandomTimeout(500, 750)` at
  `0x47301d`). With AI flag `0x40` set they then also test their bounding sphere, scaled by 5, against the view
  (`gr_cull_sphere`). Off-screen ones stay skipped.
- On the multiplayer host, idle monsters with AI flag `0x40` run on the same 500-750 ms timer.

The console variable `ai_frame_update_rate` ("rate ... in frames for offscreen creatures") is never read. The
throttle uses the millisecond timer above.

## Goals

`ai_state +0x8c` holds five goal slots of 0x34 bytes, and `+0x190` holds the **id** of the running goal
(`ai_is_following_leader` compares it with goal `+0x08`). Each slot has type `+0x00`, flags `+0x04`, id `+0x08`,
time `+0x0c`, priority `+0x10`, point `+0x14`..`+0x1c`, layer `+0x20`, spell `+0x24`, target `+0x28`, a float
argument `+0x2c` and a path `+0x30`.

All add functions go through `ai_goal_add` (`0x476190`), whose arguments are (type, priority, flags, target,
point, layer, spell, float). The priority is the caller's. `0x6c` is the spell argument: the "no spell" value that
every non-cast goal passes (`push 0x6c` at `0x473f55`). Adding an attack goal removes the other attack and
backstab goals; a travel goal removes the other travel goals; a cast goal removes the other cast goals. For a
player-controlled owner (AI flag `0x1`, `ai_state +0x50`), an attack or travel goal removes both kinds
(`0x4761ee`..`0x476207`). An existing goal with the same fields only gets the new priority. An existing follow goal
also takes the new target (`0x47622e`..`0x47625b`).

A new goal goes into an empty slot (priority -1). If there is none, it replaces the lowest-priority goal without flag
`0x1`, but only when its own priority is higher or it has flag `0x1` itself (`ai_goal_alloc` `0x476020`,
`0x476088`). Otherwise nothing is stored, but the slot is still returned and `ai_goal_add` writes the new fields
into it (game bug G230). A stored goal gets flag `0x2`, the next id (`ai_goal_next_id` `0x59b7c8`, saved with the
game) and `level_session_time`. When the owner is the party leader, the slot also gets flag `0x100`. Outside
conversations (`game_flags` `0x100`), the leader's hold flag (Player `+0x10` bit `0x4`) is cleared too
(`0x476124`..`0x476176`; with every slot kept this writes to `goals[-1]`, V230). The goal type names are in the
game's table `0x59ae98`.

| Type | Table name | Added by | Notes |
|---|---|---|---|
| 1 | `attack` | `ai_goal_add_attack` | refuses hidden targets and non-enemies (`character_is_enemy`) |
| 2 | `follow` | `ai_think_party_update` (priority 500, flag `0x1`), `script_action_follow` (priority 100, flags `0x21`) | the only callers that set flag `0x1` themselves |
| 3 | `cast_spell` | `ai_goal_add_cast_spell` | |
| 4 | `cast_targeted_spell` | `ai_goal_add_cast_at_target` | hidden targets only for spells 3 and 5 |
| 5 | `cast_location_spell` | `ai_goal_add_cast_at_point` | |
| 6 | `switch_combat_mode` | `ai_set_combat_mode_goal` | calls `ai_goal_alloc` directly, priority from its caller, flags 0 |
| 9 | `pathfind` | `ai_goal_add_travel` | running sets goal flag `0x200` |
| `0xc` | `goto_navpoint` | `ai_goal_add_move_to` | walking sets flag `0x10` |
| `0xd` | `wander` | `ai_goal_add_move_to_timed` | sets traveling sub 8; callers `script_action_wander` (priority 100) and `boss_release_mount` (priority 5000, float 30.0). The float is probably the wander radius (*inferred*) |
| `0xe` | `backstab` | `ai_goal_add_skill_from_behind` | only within 2 m behind the target |
| `0xf` | `sneak` | `ai_goal_add_sneak` | priority 999999 |
| `0x10` | `hide` | `ai_goal_add_hide` | priority 999999; leaves hiding instead when already hiding |

The other table names are `unused` (0), `switch_normal_mode` (7), `protect` (8), `turn_to` (`0xa`), `run_away`
(`0xb`) and `pickup item` (`0x11`). Nothing adds a type-7 goal; the dispatch would send it to
`ai_goal_do_combat_mode` like type 6. The goal handlers are at `0x4b6000`-`0x4b68c0`, one per type
(`ai_goal_do_*`). `ai_process_goals` (`0x4763a0`) does nothing in AI mode 6. Otherwise it first runs
`ai_goal_evaluate` (`0x475830`) on every used slot (`ai_goals_validate`): result 1 or 2 removes the goal, 3
suspends it (flag `0x8`), 0 clears the suspension. It then sorts with `ai_goal_compare`. Kept (`0x40`) and suspended
(`0x8`) goals go last; combat-mode (6) and sneak (`0xf`) goals go first; then the higher priority, then the newer
goal. The top goal becomes current (`+0x190` = its id) unless it is empty or suspended, or it is already current
and the owner is not idle. Its handler is called with the goal's fields. **Observed.**

## Attacker slots

Each target has five attacker slots at `ai_state +0x1b0`, 8 bytes each: an attacker handle and a timer. The slot
directions are unit vectors 72 degrees apart around the target (`ai_attack_slot_direction`, `0x4726d0`).
- `ai_pick_attack_slot` (`0x472780`) returns the slot whose direction best matches the attacker's bearing. It does
  not look at whether the slot is taken. Its one caller, `ai_approach_attack_slot`, first lists the free slots and
  picks the nearest itself (`0x4bd44a`..`0x4bd575`). It uses the picked slot only to route around the target.
- `ai_claim_attack_slot` (`0x472840`) clears the attacker's old slot, keeping the shorter remaining time. With
  slot `-1` it takes the best direction. A slot held by a living attacker is only taken over when that attacker has
  more time left (`0x472966`..`0x47297a`). Riders never claim a slot.
- `ai_count_attackers` (`0x473750`) counts the living entities whose AI target is the owner and that are in an
  attacking sub-mode. `ai_count_engaged_attackers` (`0x4736a0`) also counts the close, ranged and positioning
  sub-modes and, optionally, attack goals on the owner.

The AI target is at `Character +0x384` (`ai_state +0x60`).

## Paths

The current path is at `ai_state +0x210`, its start point at `+0x214`..`+0x21c`, and the travel point at `+0x54`..`+0x5c`. The priority of the
current order is `+0x38` (999999 for direct orders).
- `ai_path_to` (`0x473e50`) returns 0 or -1, not the path. It calls `ai_path_find_new` or `ai_path_find_append`.
  Its fifth argument is not read.
- `ai_path_find_new` (`0x473c40`) searches from the bottom of the mount chain, with that character's size and
  layer, and stores the path on that character's AI.
- `ai_path_find_append` (`0x473d80`) extends the path from its last node.
- `ai_path_begin` (`0x473ac0`) drops a one-node path that ends within 5 cm of its start (not in traveling sub 8).
- `ai_path_free` (`0x4a0480`) releases the nodes. It is called from 17 functions (20 call sites). Mode changes are
  not among them; a new path frees the old one (`ai_set_path`, `ai_path_find_new`), and `ai_state_shutdown`,
  `ai_reset` and `ai_reset_party_member` free it as well.
- The path search also steers around obstacle-avoidance areas (next section).

## Obstacle avoidance

Ground effects and summons mark areas that paths should go around (`avoid_area_add` `0x4a9b40`, callers
`spell_place_ground_effect`, `spell_effect_obj_set_timer`, `spell_spawn_hit_fx`, `summon_spawn_effect`). The code
is the unnamed unit `0x4a6710`-`0x4aa77f` (`src/decomp/game/unnamed_avoidance_0x4a6710.cpp`) (**observed**):

- An area (14 slots, `0x246b928`) holds up to two obstacles, a box or an octagon, one per size class. The area
  description's first word picks the kind: 0 makes an "avoid" obstacle (it only adds cost), non-zero a "prohibit"
  one (it blocks). `avoidance enable` / `disable` switches the whole system (`avoidance_enabled` `0x59ce88`, tested by
  `avoid_add_primitive` `0x4a9eb0`).
- Overlapping obstacles of the same kind, size class and layer are merged into one polygon (at most three), which
  is clipped against the navigation walls into a segment list. Its corners become extra search nodes with up to 20
  links (`avoid_do_poly_seg_list_links` `0x4a8390`).
- `pf_add_avoidance_nodes` (`0x49f810`, called by `pf_graph_search`) blocks search edges through prohibit polygons and
  adds the length inside an avoid polygon as cost (`avoid_poly_add_search_nodes` `0x4aa400`).
  `world_click_move` refuses a click inside a prohibit obstacle (`avoid_point_blocked` `0x4aa630`).
- `fx_obj_destroy` and `summon_emerge_update` remove an area (`avoid_area_remove` `0x4a9df0`); the polygons are then
  rebuilt without its obstacles, unless `avoidance no_remove` is set (`avoidance_no_remove` `0x59ce8c`).

## Saving

`ai_state_save` (`0x4746d0`) and `ai_state_load` (`0x4747e0`) write and read the AI part of a character record in
the [savegame](savegame.md) (`LENT` chunk). Only the combat style (`+0x18`) is saved for every entity. NPCs and
monsters (types 4 and 5) also save their five goals, 0x30 bytes each (the path is not saved). A goal's target is
stored as the name hash of the target, and `""` when there is none. On load it is resolved to the first living
entity with that name hash (`0x47486d`..`0x4748a9`). The character records themselves are matched the same way
(`savegame_read_character`), so this relies on unique character names.

## Console commands

| Command | Variable | Read by |
|---|---|---|
| `ai_state <name>` | none | never matches a name (compares pointers, `0x474d4f`); see [Bugs](game-bugs.md) |
| `pause_ai` | `Ai_paused` (`0x22e499c`) | `character_ai_suspended`, `camera_auto_view_update`, `ls_worldmap1_roll_encounter` |
| `ai_wander` | `Ai_path_wandering_enabled` (`0x59af58`) | `ai_follow_path` |
| `slowdown_scaler` | `slowdown_scaler` (`0x59af54`, 4.0) | `ai_frame` |
| `ai_frame_update_rate` | `0x59af5c` (8) | nothing |
| `attack_point` | `0x59af60` | nothing |
| `offense_point` | `0x59af64` | nothing |

## Bosses

`ai_reset` (`0x472450`) loads the boss AI (`boss_ai_load`, from the `*_boss.tbl` tables) for type-9 entities and
for Luminar (species `0x51`). Each boss has its own stage, frame and attack functions in `0x494000`-`0x4ca000`,
named by boss (for example `serpent_rider_set_flinch`, `tiger_load_anims`, `pyrul_boss_load`). See
[Bosses](bosses.md) for the dispatchers, the HP-based stage machines, each boss's mechanics and the cutscenes
tied to them.

## Evidence

Function and instruction addresses (no REA evidence ids; listings from `tools/asm.mjs`).

| Claim | Where |
|---|---|
| no time budget, types 4/5/7/9 every frame | `ai_do_frame` `0x474bc8`..`0x474c44`, jump table `0x474cc4` |
| timer only for the profiler | `0x474954`..`0x474b24` |
| set_mode releases the handle slot, not the path | `ai_set_mode` `0x47337f`..`0x47339b`, `0x4735bb`..`0x4735cd` |
| set_mode refuses DISABLED while casting | `0x47333e`..`0x47335e` |
| activity cull | `ai_update_perception` `0x472f43`..`0x47301d`, `0x47303b`..`0x473088` |
| `0x6c` is the spell argument | `ai_goal_add_travel` `0x473f53`..`0x473f67`; `ai_goal_add` `0x47620c`..`0x4762f3` |
| type `0xd` is wander | `ai_goal_add_move_to_timed` `0x4741b9`; `ai_goal_do_move_to_timed` `0x4b6697`..`0x4b66bc`; `script_action_wander` `0x4dcac2`..`0x4dcad5` |
| knock-down sub `0x2c` | `ai_knock_down_local` `0x474310`; `ai_mode_disabled_frame` `0x4b4b60`, `0x4b4b6e`..`0x4b4b88` |
| re-knock branch unreachable | `ai_can_be_knocked_down` `0x47506a`..`0x475073`; `ai_knock_down_local` `0x4742b3`..`0x4742cc` |
| pick ignores occupancy | `ai_pick_attack_slot` `0x4727ef`..`0x472831`; `ai_approach_attack_slot` `0x4bd44a`..`0x4bd59e` |
| claim keeps the occupant with less time | `ai_claim_attack_slot` `0x472956`..`0x472995` |
| goal targets by name | `ai_state_load` `0x47486d`..`0x4748e5`; `ai_state_save` (`living_from_handle`, `name_hash_nocase`) |
| dead console variables | `tools/asm.mjs grep` of `0x59af5c`, `0x59af60`, `0x59af64`: only the command functions |

## Changes (2026-10-15)

Checked against the disassembly while writing `src/decomp/game/ai.cpp` (`NOTES-ai.md`).
- Per-frame update: no time budget in `ai_do_frame`. The throttle for off-screen characters is in
  `ai_update_perception`, which is an activity cull, not a perception pass.
- `ai_set_mode` releases the handle slot `+0x80`. It does not free the path. Its detours are now listed.
- Goals: `0x6c` is the spell argument, not a priority. Type `0xd` is the wander goal, not a timed move. Types
  `0xe` and `0x10` added; table names added; handler range corrected to `0x4b6000`-`0x4b68c0`.
- Modes: sub-mode names from the game's table; traveling sub 2 is a turn, sub 8 is wander; `0x11` is backstab.
- Disabled: knock-down sets sub `0x2c` (fall); `0x2d` is the lying phase. New section with the six sub-modes.
- Attacker slots: `ai_pick_attack_slot` does not look for a free slot. `ai_claim_attack_slot` only replaces an
  occupant with more time left.
- Paths: `ai_path_to` returns 0 or -1. `ai_path_free` is not called on mode changes.
- Saving: only the combat style for all; goals for types 4 and 5; targets by name hash.
- New: console commands table. The function count is about 210, not 120.
