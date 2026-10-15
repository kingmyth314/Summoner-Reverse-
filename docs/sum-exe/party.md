---
title: Party
parent: Sum.exe
nav_order: 6.21
---

# Party
{: .no_toc }

The party is a list of at most five `Player` records: Joseph, Jekhar, Flece, Rosalind and one summon.
One member is the leader. You control the leader, and the camera follows it. Every other member is
run by the AI. It follows the leader in a wedge formation and joins your attacks. Solo mode (H) puts
every other member on hold, so they stay where they are and ignore group combat orders. Members join
and leave through level scripts, by name. Monster XP is split evenly between the living members
within 20 units of the kill. **Observed** in code, not tested in game unless stated.

1. TOC
{:toc}

## Records and lists

Each member has a `Player` record (`player.cpp`) and a `Character` object. The `Player` record is
described in [Characters](characters.md#player-record). There are exactly five records, 0x188 bytes
apart, at `0x5fc4d8`..`0x5fcc80`. `party_lists_init` (`0x433ca0`) links all five into
`player_free_list` (`0x5fc1c8`). **Observed.** Three circular lists hold them:

| List | Address | Holds |
|---|---|---|
| `party_list` | `0x5fc350` | current members, in species order (`party_sort_by_species`, `0x434270`) |
| `player_list_other` | `0x5fc030` | members who left the party but still exist |
| `player_free_list` | `0x5fc1c8` | unused records |

`player_alloc_slot` (`0x4334f0`) takes a free record and appends it to `party_list`. With no record
left, it loops on a `DebugError` ("No more player slots availeble"). **Observed.**

Fields of the `Player` record used by the party code (**observed**):

| Offset | Meaning |
|---|---|
| `+0x0c` | character object handle (`player_from_handle`, `0x433eb0`, searches `party_list`, then `player_list_other`) |
| `+0x10` | flags: `0x2` summon, `0x4` hold, `0x8` leader |
| `+0x14` | owning multiplayer player (compared with `mp_local_player`) |
| `+0x178` | max HP lent to the summon ([Summons](summons.md#cost-ap-and-hp)) |

`Character +0x320` carries the party flags on the character side: `0x8` hidden (the world map hides
the other members this way, see [World map](worldmap.md)), `0x80` dead, `0x100` summon, `0x800000`
left the party. **Observed** (bits read by the functions below). `0x8` is also the "dying" flag:
`character_on_death` sets it on every character that dies, party members included (`0x453870`), and
`character_resurrect` (`0x454070`) clears it together with `0x80`. `0x2` is set while the character is
asleep, paralysed or in an ice coffin (`status_effect_add` `0x454c3d`, cleared by `status_effect_cure`
`0x4555df`).

## The leader and `cur_character`

`party_leader` (`0x5fcc80`) points to the leader's `Player` record. `cur_character` (`0x5fcc88`) points
to its `Character`. `party_set_leader` (`0x433d60`) is the only place that changes both. **Observed.**
It:

1. does nothing if the new character's `ai_state` flag bit 0 (`+0x374`) is set (already player-controlled);
2. calls `player__drop_leader` on the old leader and `player__make_leader` on the new one;
3. sets `+0x374` bit 0 on the leader (AI off) and clears it on every other member (AI on), through
   `player__set_ai_control` (`0x433750`);
4. makes the leader the leader of its AI group (group `+0x48`), unless it is dead;
5. drops the new leader's goals of type 1 and, unless it is casting or disabled, makes it idle;
6. resets player input and the pick filter, and points the camera at it (`camera_set_target`).

`player__make_leader` (`0x433960`) sets flag `0x8` and clears hold (`0x4`). `player__drop_leader`
(`0x4339a0`) clears `0x8`. It sets `0x4` when solo mode is on and clears it otherwise. **Observed.**
`game_draw_leader_marker` (`0x4145e0`) draws the rotating ground ring under `cur_character`.

## Selecting a member

| Input | Function | Effect |
|---|---|---|
| F5-F9 | `hud_select_party_member` (`0x40e1f0`) | selects member 1-5 by its `party_list` position |
| Portrait click | `hud_portrait_action` (`0x40f1b0`) | same, or opens the status screen |
| Tab / Shift+Tab | `party_cycle` (`0x413360`) | next / previous member |
| Prev/next buttons | `ui_party_prev` / `ui_party_next` (`0x42a960` / `0x42a970`) | `party_cycle` on the skills and spells screens |

All paths end in `party_select_member` (`0x4396d0`). It refuses a member whose `Character +0x320` has
`0x80` (dead) or `0x8` (hidden) and plays sound `0x24`. Otherwise it calls `player__drop_leader` on
every member, then `party_set_leader`, and plays sound `0x23`. **Observed.** `party_cycle` also skips
members with flag `0x2` in `+0x320`, which the status code sets while a member is asleep, paralysed or
in an ice coffin (**observed** in `status_effect_add`; other setters were not searched). *Inferred:* Tab
passes over a disabled member, while F5-F9 can still select it. In multiplayer it only stops on members
owned by the local player. **Observed.** See [Controls](controls.md#default-keyboard-map-hud_handle_keys).

`party_count_active` (`0x434350`) counts members that exist and do not have `0x800000`. Outside the world
map (`cur_level_index` 4) it also skips hidden (`0x8`) members. The inventory trade buttons and the
skills and spells member buttons use it. **Observed.**

## How many members, and who can join

- **Five records, five members.** The four heroes plus one summon fill all five records, which is
  why F5-F9 has five keys. *Inferred.*
- **New members** come from `party_member_join` (`0x439ba0`, args: name, level, leader, place at start,
  char_info index). It does nothing (returns 0) while a savegame load is pending, because the save
  restores the party. In single player it first looks for the char_info in `party_list` and
  `player_list_other` and returns the existing character. If five or more records are already in
  use (both lists counted), it refuses. Multiplayer skips both tests. Otherwise it creates the
  character (`player_create`), adds XP up to the given level (`character_add_xp(xp_for_level(level))`,
  quiet, only above level 1), resets its AI, places it at the party start when asked, re-sorts the
  party and shows "%s has joined the party!". **Observed.**
- **Starting party:** `level_spawn_party` (`0x438a90`, from `level_load_files`) joins Joseph as leader
  when state 4 is entered from state 2 with no save pending, which in practice means a new game. With the
  `Full_party` cheat (`cmd_full_party`, `0x437800`) it would also join Rosalind, Jekhar and Flece at
  level 1. But when the level is `masad` it clears `Full_party` and returns after Joseph. `masad` is
  not a test level: it is Masad Village, level 3, where every new game starts (`mainmenu_start_new_game`
  passes `level_script_table[3]`). So on a new game `Full_party` only adds Joseph and is switched off;
  it can work only when the console starts another level through state 2 (`cutscene_change_level_for`).
  **Observed.** In multiplayer each player of `mp_player_list` joins with its `.plr` file instead.

Story joins (**observed** arguments; the level number is the level the member arrives at):

| Member | Level | Caller |
|---|---|---|
| Flece | 4 | `ls_lenele1c_frame` (`0x4fabfe`) |
| Rosalind | 6 | `lsf_iona_ext_found_rosalind` (`0x4f753a`) |
| Jekhar | 8 | `ls_lpalaceint_frame` (`0x4f2d0e`) |
| Rosalind, Jekhar | 15 | `ls_rand_grassland01_init` (`0x4ee97f`, `0x4eea20`) |
| Jekhar, Rosalind, Flece | 1 | `game_stage_set` (`0x4d44d1`, `0x4d4525`, `0x4d45b6`); *unknown* which stages |

The console commands `jekhar`, `flece` and `rosalind` (`cmd_jekhar` and so on) also call
`party_member_join`. **Observed.** A member joins at a fixed level, not at Joseph's. *Inferred* from
the constant arguments.

## Leaving and coming back

`party_remove_member` (`0x433f40`, by name) moves the record to `player_list_other`, removes the
character from its AI group, the active object list and the living list, and sets `0x800000`. It shows
"%s has left the party!". If the leader left, the first living member still in the party becomes
leader. **Observed.** The character is not deleted, so it keeps its level, items and skills.
*Inferred.*

`party_add_member` (`0x4340d0`, by name) reverses this. It searches `player_list_other`, relinks the
record, adds the character to the leader's AI group, clears `0x8`, `0x2` and `0x800000`, resurrects a
dead character, refills HP and AP, clears effects and re-sorts the party. **Observed.**

Level scripts use the pair to split the party for story scenes (**observed** names):

| Removed | Added back |
|---|---|
| Jekhar, Flece, Rosalind (`ls_templeint2_machival_death_cs_end`) | same three (`ls_templeint2_enter_forge`, `ls_templeint2_use`) |
| Jekhar, Flece, Rosalind (`ls_jadetemple_murod_death_cs_end`) | same three (`ls_jadetemple_finish`) |
| Joseph, Flece (`ls_lpalaceint02_init`, `ls_lenele3d_init`) | Joseph, Flece (`ls_lpalaceint02_leave`, `ls_lenele3a_leave`) |
| Joseph, Jekhar, Rosalind (`ls_eleh_init`) | Joseph (`ls_eleh_frame`) |
| Jekhar (`ls_wolong2_init`) | Jekhar (`ls_wolong2_frame`) |

`party_remove_player` (`0x4335a0`) is different: it deletes the character and frees the record. It is
used for summons, multiplayer disconnects and `party_remove_all`. **Observed.**

## Death

- A dead member stays in `party_list` with `0x80` set. It cannot be selected. **Observed.**
- `party_check_all_dead` (`0x439450`) runs every frame (in multiplayer only once the net game state is
  5 or more). When the leader is dead in single player, it fades the screen in and selects the first
  living member in `party_list` (species order, so Joseph first). Only when no member is alive does it
  set `game_flags` `0x40000`, unpause and show the death screen (`hud_mode_set_5`). In multiplayer a dead
  leader goes straight to `hud_party_dead`. **Observed.**
- Dead members are **not** revived on every level change. `party_level_reset` (`0x437e50`), called by
  `level_unload` when the party is kept, resurrects dead members and sets HP and AP to their maximum
  only when the new level is a town (`#Town` in its `_script.tbl`, read by
  `level_script_read_town_characters`) or the world map (`level_filename` equal to
  `level_script_table[4]`, `worldmap1`). It then also clears `Character +0x320` bits `0x2` and `0x400`
  and all effects. Into any other level the dead stay dead and the living keep their HP and AP. In
  multiplayer no level counts as a town, so only the world map would revive. **Observed.** A script that
  calls `party_add_member` also revives the member.
- On every level change `party_level_reset` deletes every object in the active list except the heroes
  (type 7 without the summon flag `0x100`): the summon gets its lent max HP back
  (`summon_return_lent_max_hp`) and is destroyed, so it is not carried over. The heroes' effects are
  cleared and their AI reset. **Observed.** A next state other than 4 (back to the menu) removes the
  whole party (`party_remove_all`) and clears the three "joined" bytes. **Observed.**
- A dead hero keeps type 7; only the summon's type changes (7 to 8). `character_on_death` skips the
  loot drop and the type change for a party member that is not a summon (`0x453f00`..`0x453f16`).
  **Observed.**
- A dying summon gives Joseph back the max HP lent to it (`summon_return_lent_max_hp`, `0x4536c0`) and is
  removed with `party_remove_player`. The summon skill's cooldown starts when the current summon dies. If
  Joseph dies, his summon turns hostile (team 3, type 5, "Summoned creature has turned against you!").
  **Observed.** See [Summons](summons.md#behaviour-dismissal-and-death).

## Solo mode and hold

**H** toggles solo mode through `hud_toggle_solo_mode` (`0x40e300`), optionally after an
"Enter solo mode?" box. The real work is `party_toggle_hold` (`0x439620`). **Observed:**

```
439620  mov al, [level_is_town]      ; in towns: clear game_flags 0x1000 and return
...
439646  xor ecx, 0x1000              ; toggle solo mode
439667  call player__is_leader       ; for every non-leader member:
439673  or al, 0x4                   ;   solo on: Player hold flag, ai_go_idle (unless AI mode 6)
4396a9  and al, -0x5                 ;   solo off: clear hold flag
```

What hold does (**observed** unless marked):

- The member's follow goal is suspended: `ai_goal_evaluate` (`0x475830`) returns 3 for a type-2 goal
  of a held party member, the same value it returns for a disabled AI. *Inferred:* 3 means "skip".
- `ai_member_skips_group_orders` (`0x493080`) is true for a held member, so the group combat think
  gives it no attack or spell orders.
- `party_order_attack` (`0x43a3a0`) skips held members.
- The summon's 50-unit leash is off in solo mode (below).

When you switch members in solo mode, `player__drop_leader` sets hold on the old leader and
`player__make_leader` clears it on the new one. So the member you leave stays put. **Observed.**

Solo mode is also turned on and off for you:

| When | Function | Effect |
|---|---|---|
| Sneaking starts (single player) | `ai_mode_sneaking_frame` (`0x4b4f1f`) | turns solo mode on if it was off, and remembers this in AI flag `0x4000000` |
| Sneaking ends | `ai_mode_sneaking_frame` (`0x4b50e7`) | toggles solo mode back unless it was already on |
| Leaving a level | `state_game_leave` (`0x438d55`) | turns solo mode off |
| Loading a save | `savegame_read_living` (`0x4d30e4`) | restores it from bit 5 of the party state dword ([Savegame](savegame.md#lent-living-entities-and-per-character-state-observed)) |

*Unknown:* whether a held member fights back when attacked directly.

## Following and formation

Every AI think, `ai_think_party_update` (`0x4b71c0`) gives each non-leader party member a follow goal
(type 2, 500 ms) on the leader and removes it from the leader. **Observed.** Followers think every
500 ms (`ai_think`, `ai_is_following_leader`). The follow logic is `ai_follow_character` (`0x4b5bd0`).
See [AI](ai.md#goals) for goals and modes.

**Formation** (`ai_formation_slot_position`, `0x4b52e0`). `ai_group_assign_formation` (`0x4b5160`)
gives each group member a slot, nearest member first. Slot offsets, in units, relative to the leader's
facing (**observed** values; "behind" is *inferred* from the matrix row used):

| Slot | Behind | Sideways |
|---|---|---|
| 1 | 2 | -2 |
| 2 | 2 | +2 |
| 3 | 4 | 0 |
| 4 | 4 | +4 |
| 5 | 4 | -4 |

**Follow rules** in `ai_follow_character` (**observed**):

- The target position is the leader's predicted position (`ai_predict_owner_position`).
- A follower that is not already walking, or whose leader is almost still (speed below 0.01), stops
  when it is within 5 units of the leader (squared distance 25; 3 units for non-party targets).
- It walks to its formation slot. If the path is shorter than 7 units, it only re-paths when the slot
  is more than 0.25 units away (leader moving) or more than 16 units away (leader standing).
- If the path is 7 units or longer, it re-paths next to the leader when the edge distance is over 3
  units, and gives up the goal when it is over 100 units.
- A path to `cur_character` longer than 20 units sets AI flag `0x8000000`. *Unknown* effect.

`ai_think_party_update` also makes a member step 2.5 units aside when a character that started
moving in the last 200 ms comes at it from within 3.2 units. *Inferred* from the vector tests.

## Combat behaviour

Clicking an enemy calls `party_order_attack`. The leader attacks it. Every other member that is not
held and has no target (`+0x384` = -1) gets the same attack goal. Combat mode turns on. **Observed.**

After that, the AI groups decide (`ai_groups_frame`, every 1.5-2.5 s; `ai_group_combat_think`,
`0x492db0`). Each member acts by its **combat style**, `ai_state +0x18` (`ai_get_combat_style`,
`0x473620`). **Observed:**

| Value | Name (`strings.tbl`, ai.cpp) | Group order |
|---|---|---|
| 0 | Melee | nearest enemy that is not helpless (`ai_group_think_style0`) |
| 1 | Support | enemy chosen by attacker count and HP (`ai_group_think_style1`) |
| 2 | Range | enemy with the fewest attackers (`ai_group_think_style2`) |
| 3 | Healer | heals, then attack spells (`ai_group_think_caster`) |
| 4 | Caster | like Range for targets; spell range 8.0 with enough AP (`character_get_attack_range`) |
| 5 | Healer/Caster | as 3, with the caster range of 4 |

The default comes from the species in `living_entity_init`: species 0 and 1 get Melee, 2 gets
Support, 3 gets Healer/Caster, and every other species Melee. **Observed** (`0x44eb2c`..`0x44eb7d`).
Species 0-3 are Joseph, Jekhar, Flece and Rosalind: `player_create` picks the progression table by
species through the jump table at `0x433950`, and each table's skills match that hero's `$Skill:` lines
(Jekhar axe weapons, Flece bow weapons, Rosalind heal and staff weapons). **Observed** (2026-10-15).
You change it on the status screen:
`status_ai_popup_show` (`0x42c450`) offers all six names, and `status_ai_popup_frame` (`0x42c6c0`)
stores the choice with `ai_set_18`. The style is saved with the character (`ai_state_save` writes
`+0x18` first). **Observed.** See [Interface](interface.md).

## Distance limits

| Limit | Rule | Function |
|---|---|---|
| Level exit | blocked with "Party members too far away" if any living, non-summon member is more than 10 units from `cur_character` (squared 100). Not checked in `rand-` encounter levels or on the world map. | `level_check_triggers` `0x4cad71`-`0x4cadeb`, `hud_confirm_level_exit` `0x40d6d6` |
| Summon leash | outside solo mode, a summon more than 50 units (edge distance) from a species-0 group member (Joseph) is dismissed | `ai_think_party_update` `0x4b7514`-`0x4b75d3` |
| XP share | a member must be within 20 units (squared 400) of the dying monster | `character_on_death` `0x453b84` |

The exit check also applies in solo mode. *Inferred:* held members must be brought back before you
can leave. On the world map only the leader is shown ([World map](worldmap.md)).

## Experience sharing

`character_on_death` (`0x4537f0`) pays XP when a monster with byte `+0x608` set dies. `character_add_hp`
writes that byte on every call: 1 when the source is a party member, 0 otherwise (`0x4508b7`). So the
last HP change before the death decides, not who did most of the damage. The party's summon is type 7,
so its hits count too. **Observed:**

1. It collects party members that are alive, not hidden, not summons and within 20 units.
2. Total XP = `char_info +0x2c` x `r`, where `r` = monster level / `char_info +0x30`; when `r` >= 1
   it uses `r^1.25` (`sqrt(sqrt(r)) x r`) instead. In multiplayer it is multiplied by the number of players.
3. The share is total / count, at most 2^31. "+N xp" (the total) floats over the monster. Each collected
   member gets the share as an integer (`character_add_xp`), except on a multiplayer client, where the host's
   award arrives over the network.

So a kill is split evenly, and members far away get nothing. Summons never take a share. Other
`character_add_xp` callers give XP to every member: `script_party_gain_xp` (`0x4ceb00`, "party gained %d
XP"). `party_set_level` (`0x4cec00`) is different: it resets each member at or below the given level (not
summons) to level 1 with no XP and no skill points, then adds `xp_for_level(level)` with `quiet` set, so
the skill points are spent at once ([Characters](characters.md#experience-and-levels)). **Observed.**

## Tips for players

- Fight near your whole party: a member more than 20 units away gets no XP from the kill. The XP of
  a kill is split, so fewer members nearby means more each.
- Solo mode is a hold command. Use it to park members out of danger, then switch with F5-F9; the one
  you leave also holds.
- Gather the party (within 10 units of the leader) before an exit, even in solo mode.
- Set each member's combat style on the status screen. It is saved.
- In solo mode a summon can wander far from Joseph without being dismissed.
- If the leader dies, control passes to the next living member; the game ends only when all are dead.
- Members who rejoin after a story split come back revived with full HP and AP.
- A member who dies stays dead until the party reaches a town or the world map, which also refills
  everyone's HP and AP. Other level exits do not heal.

## Evidence

These are function and instruction addresses read with `tools/asm.mjs`, not REA evidence IDs.
Float constants were read from `Sum.exe` (`0x580de8` = 50, `0x57e6b4` = 100, `0x58192c` = 400).

| Claim | Where |
|---|---|
| Five records of 0x188 bytes | `party_lists_init` `0x433cbc`-`0x433cd5` |
| Leader change | `party_set_leader` `0x433d60`, `player__make_leader` `0x433960`, `player__drop_leader` `0x4339a0` |
| Selection rules | `party_select_member` `0x4396e5`, `party_cycle` `0x413374`-`0x413408` |
| Join limit and level | `party_member_join` `0x439bf5`-`0x439cea` |
| Leave / rejoin | `party_remove_member` `0x433fab`-`0x4340ba`, `party_add_member` `0x43413b`-`0x434226` |
| Leader death | `party_check_all_dead` `0x4394b1`-`0x4394d6` |
| Solo mode | `party_toggle_hold` `0x439620`, `ai_goal_evaluate` `0x475953`-`0x475968`, `ai_member_skips_group_orders` `0x493080` |
| Sneak auto-solo | `ai_mode_sneaking_frame` `0x4b4f11`-`0x4b4f26`, `0x4b50d5`-`0x4b50e7` |
| Follow goal | `ai_think_party_update` `0x4b74c4`-`0x4b750f` |
| Formation | `ai_formation_slot_position` `0x4b5306`-`0x4b539d` |
| Follow distances | `ai_follow_character` `0x4b5bf5`, `0x4b5dbe`-`0x4b5ed1`, `0x4b5f92`-`0x4b5fd1` |
| Attack order | `party_order_attack` `0x43a3ec`-`0x43a412` |
| Combat styles | `init_strptrs_22e49a0` `0x472070`, `living_entity_init` `0x44eb2c`-`0x44eb7d`, `ai_state_save` `0x4746d0` |
| XP split | `character_on_death` `0x453a42`-`0x453d40` |
| Party hit byte `+0x608` | `character_add_hp` `0x450892`-`0x4508b7`; `character_on_death` `0x453a4a` |
| Share cap, client skip | `character_on_death` `0x453c42`-`0x453c60` (`0x4f000000` = 2^31), `0x453d40` |
| Dying flag `0x8`, dead `0x80` | `character_on_death` `0x453870`, `0x45393a`; `character_resurrect` `0x454070` |
| Flag `0x2` | `status_effect_add` `0x454c3d`; `status_effect_cure` `0x4555df` |
| Dead hero keeps type 7 | `character_on_death` `0x453f00`-`0x453fb2` |
| Species order | `player_create` jump table `0x433950` (targets `0x4337fb`, `0x43380c`, `0x43381e`, `0x433830`) |
| `party_set_level` | `0x4cec3a`-`0x4cecd3` (reset to level 1, `character_add_xp` with `quiet` = 1) |
| Join while a save is pending, char_info match | `party_member_join` `0x439bb1`-`0x439bc6`, `0x439c0a`, `0x439c25`, single-player test `0x439bf5` |
| Starting party, `masad` | `level_spawn_party` `0x438aaf`-`0x438afd` (`0x5930c0` = "masad"); new game level `mainmenu_start_new_game` `0x41dac0` (`0x59f1e8`) |
| Revival only in towns and on the world map | `party_level_reset` `0x437f5e`-`0x43801e` (`0x59f228` points to "worldmap1"); town bit `level_script_read_town_characters` `0x437af1`-`0x437b0f` |
| Objects deleted, summon not kept | `party_level_reset` `0x437eb2`-`0x437f58`; `living_entity__vfunc0` `0x44edf0` calls `party_remove_player` |
| Death check | `party_check_all_dead` `0x4394b1`-`0x4394f7`; multiplayer gate `state_game_frame` `0x438f94`-`0x438fa6` |

## Changes (2026-10-15, living_entity.cpp)

{: .important }
Checked against the disassembly of the `living_entity.cpp` functions. `0x2` in `Character +0x320` is no
longer *unknown*: it marks a member who is asleep, paralysed or in an ice coffin. `0x8` is also the dying
flag. The species order (Joseph, Jekhar, Flece, Rosalind) is now **observed**, and every non-hero species
defaults to Melee. Experience: `+0x608` is rewritten on every HP change, so the last hit decides whether a kill
pays; the share is capped at 2^31 and a multiplayer client does not award it. `party_set_level` resets the
members to level 1 and re-adds the XP quietly; it is not an XP gift. Death: a dead hero keeps type 7; a dying
summon returns the lent HP and starts the summon cooldown.

## Changes (2026-10-15, gameplay.cpp)

{: .important }
Checked against the disassembly of the `gameplay.cpp` functions (`src/decomp/game/gameplay.cpp`). The page said
dead members are revived on the next level change; `party_level_reset` revives and heals only when the new level is
a town or the world map, and other level changes leave the dead dead and the HP and AP as they are. `masad` is not
a test level but Masad Village, where every new game starts, so `Full_party` never adds the other heroes on a new
game. `party_member_join` matches existing members by char_info index, does nothing while a save is pending, and
applies the five-record limit only in single player. A level change destroys every non-hero object, the summon
included. The death check fades in before switching the leader and runs in multiplayer only from net state 5.
