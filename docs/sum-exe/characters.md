---
title: "Characters, experience and levels"
parent: Sum.exe
nav_order: 6.2
---

# Characters, experience and levels

**Summary:** the selected party member is `cur_character` (`0x5fcc88`), a pointer to a character
object. That object holds the level, experience, HP and AP at `+0x5f4`..`+0x607`, and ten equipped-item
pointers at `+0x754`. Experience thresholds come from `xp_level_table` (`0x2305420`). Maximum HP and AP are
computed: base value plus equipment bonuses. The layout is `Character` in `symbols/types.h`.

## Evidence

- A runtime dump made in agent mode (2026-10-13), with Joseph at level 3. The Character screen showed
  HP 128/128, AP 26/26, 200 experience and 450 to the next level.
  `cur_character` was `0x9086c8`; the values were found at the offsets below.
- Disassembly of `character_add_xp` (`0x44fab0`), `character_get_max_stat` (`0x462840`),
  `character_equip_bonus` (`0x4627c0`), `xp_for_level` (`0x489a60`) and `party_level_reset`
  (`0x437e50`).

## Character fields

| Offset | Type | Field | How it is known |
|---|---|---|---|
| `+0x00c` | int | type: 7 for player party members | `character_add_xp` only shows the level-up message for type 7 |
| `+0x080` | handle | the object's handle | written by `obj_create`; `player_from_handle(handle)` returns the player record |
| `+0x2f4` | char[] | name | argument of `"%s gained level %d!"` |
| `+0x320` | flags | `0x80` = dead | `party_level_reset` revives characters with this flag set |
| `+0x5ac` | int[6] x2 | unknown: two identical groups (31, 30, 29, 26, 30, 29) | dump only |
| `+0x5f4` | float | base max HP (without equipment) | written from the level table; read by `character_get_max_stat(0)` |
| `+0x5f8` | float | current HP | 119 code references; set to max on level-up and restore |
| `+0x5fc` | short | base max AP | `character_get_max_stat(1)` |
| `+0x5fe` | short | current AP | set to max on level-up and restore |
| `+0x600` | short | level | dump (3); `character_add_xp` |
| `+0x604` | int | experience | dump (200); `character_add_xp` adds to it |
| `+0x558` | sub-object | protection helper; its first field points back at the character | `character_get_protection` |
| `+0x614` | byte[] | skill and attribute values x100, by skill id | `character_get_skill`: id 2 = speed (110), id 1 = base protection |
| `+0x754` | item*[10] | equipment slots; slots 1 and 2 are rings (they gain experience through `ring_add_xp`), slot 5 (`+0x768`) is the weapon, slots 3, 4, 6, 7 and 8 are armour | `character_equip_bonus`; the dump had a Short Sword in slot 5 and a Tunic in slot 7 |

Maximum HP is `base_max_hp + character_equip_bonus(0)`, with a minimum of 1. Maximum AP is
`base_max_ap + character_equip_bonus(1)`, with a minimum of 0. `character_equip_bonus(stat)` adds up
the modifiers of every equipped item. Each item type has two `{stat, value}` modifiers at `+0x40`.

## Character screen stats

`status_draw` (`0x42c9c0`) draws the screen through helpers: `status_draw_level` (level, experience, bar),
`status_draw_stats` (below), `status_draw_skills` (skill list and chain keys), `status_draw_skill_points`,
`status_draw_resistances` and `status_draw_ai`.

`status_draw_stats` (`0x42cff0`) draws the stat panel for `status_character` (`0x5bd810`). Each value
is coloured by its equipment bonus: one colour for a positive bonus, another for a negative one, and
the normal colour for none.

| Shown | Computed as | Joseph at level 3 |
|---|---|---|
| HP, AP | current / `character_get_max_stat` | 128/128, 26/26 |
| Speed | `character_get_skill(2)` x 100 | skill byte 110 → 1.10 → 110% |
| Damage | weapon item type `+0x68` (slot 5) | Short Sword: 35 |
| Protection | `character_get_protection` x 100 | Tunic 0.07 + level 3 x 0.01 = 0.10 → 10 |

`character_get_skill(id)` (`0x462440`) returns `skill[id] x 0.01`, adjusted by equipment
(`character_skill_equip_mod`, four `{skill, value}` entries at item type `+0x20`). It returns -999999 when
`character_skill_allowed` says the skill is locked (`player_skill_unlocked` on the player record, which checks story flags).

`character_get_protection` (`0x464de0`) works in one of two ways:

- with no armour worn, it returns `get_skill(1) + level x 0.01`;
- otherwise it adds up `materials[type +0x08].protection_mult x type +0xf0` for each worn armour piece (`materials` is the `materials.tbl` table at `0x8b9d4c`).

**Resistances** (`status_draw_resistances`) come from `character_get_resistance(type)` (`0x4626b0`). The types
are 0 fire, 1 ice, 2 energy, 3 magic, then 4-6 slashing, piercing and blunt in the screen's order. Equipment adds
its `+Resist Mod:` entries (`character_resist_equip_bonus`, item type `+0x30`). Magic resistance also
grows with skill `0x23` x 0.5 and level x 0.02, which is Joseph's 6% at level 3.

The item type layout is `ItemType` in `symbols/types.h`. It is mapped from the `items.tbl` parser (`items_parse_table`), where each
`$Field:` is stored at a fixed offset: for example kind `+0x04`, material `+0x08`, value `+0x18`, slot `+0x1c`, damage `+0x68`, weapon skill `+0x6c` and protection `+0xf0`.

## Status effects

`Character +0xac` holds 18 status slots of 0x20 bytes, indexed by status id. Each slot has a state at `+0` and
`GameTimer`s at `+4` and `+0x14`. `status_effect_add` (`0x454700`) checks immunities, sets the slot and shows
floating text. `status_effect_cure` (`0x4553a0`) clears it ("-Silence" ...).

| Id | Effect | Id | Effect |
|---|---|---|---|
| 0 | Blind | 6 | Power |
| 1 | Silence | 9 | Sleep |
| 2 | Faith | 10 | Paralyze |
| 3 | Curse | 11 | Poison |
| 4 | Bless | 12 | Cripple |
| 5 | Regen | 13 | Slow |

Ids 7, 8 and 14-16 exist (the maximum is 0x10) but show no text.

## Player record

Each party member also has a `Player` record (`player.cpp`), kept in the circular list `party_list` (`0x5fc350`).
`player_from_handle` (`0x433eb0`) finds it from the character's object handle. The record was confirmed in a runtime
dump of Joseph's record at `0x5fcaf8` (2026-10-14).

| Offset | Field | Joseph |
|---|---|---|
| `+0x000` / `+0x004` | next / prev in `party_list` | |
| `+0x00c` | character object handle | `0x2658d` (= `Character +0x80`) |
| `+0x020` / `+0x024` | count and pointer of the skill progression: `{skill, level, value}` entries | 17 entries at `0x593508` |
| `+0x17c` | **unspent skill points** | 4 (matches the Character screen) |
| `+0x180` | set on level-up from the level table | 3 |
| `+0x190` | pointer to the `Character` | `0x9086c8` |

On a level-up, `character_add_xp` adds the level table's skill points to `+0x17c`. For each progression
entry whose level equals the new level, it also sets `Character.skill[skill] = value`.

## Objects and handles

`obj_create` (`0x469c40`) takes a 12-byte slot from `obj_free_list` and links it into `obj_used_list`. The
slots are in `obj_slots` (`0x9438b0`). It stores the handle (slot index + a running base) at object `+0x80`,
and a network handle at `+0x8c`. `obj_from_handle` (`0x46b130`) reverses this. `living_from_handle`
(`0x46b1a0`) also checks that the object is a living entity.

## Experience and levels

`character_add_xp` (`thiscall`, ECX = character) is called by the console command `add_xp`. In order, it:

1. stops at the level cap, `xp_max_level` (50, or 75 with debug flag 2);
2. adds the experience to `+0x604`;
3. gives a share of the experience to the rings in equipment slots 1 and 2 (`ring_add_xp`, `0x44ffc0`), which counts it per ring (`dark_ring_xp`, `fire_ring_xp`, ...);
4. checks `xp_level_reached(level, xp)`, which means `xp >= xp_for_level(level + 1)`;
5. on a level-up, calls `0x489ac0` with the class. That call updates the level, base max
   HP, base max AP and the skill points;
6. sets current HP and AP to the new maximum;
7. for party members, applies the player record's skill progression and shows "+Level!".

`xp_level_table` has 0x18-byte records indexed by level. The first field is the experience needed:
level 2 = 100, 3 = 200, 4 = 450 (confirmed in game). Beyond level 20, each level adds 50,000, then
60,000, and so on. Debug flag 2 multiplies the thresholds by 5.

The rest of each record (`xp_level` in `symbols/types.h`, read by `experience_parse_level`) is:
- the skill points gained at that level (2);
- the level number;
- four 4-byte groups, one per species (`char_info +0x10`), with the HP and AP gained.

For Joseph's group, level 1 gives 64 HP and 5 AP, and levels 2 and 3 each add 32 HP and 3 AP. That makes
128 HP at level 3, and 2 + 2 = 4 skill points, both as the Character screen shows.

## Open questions

- The meaning of the two six-value groups at `+0x5ac` (attributes? base and current?).
- The rest of the stat panel after protection (`0x42d58f` onwards).
