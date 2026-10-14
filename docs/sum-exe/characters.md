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
| `+0x080` | handle | character template; resolved by `0x46b1a0` | the template has a skill list (`+0x20` count, `+0x24` 12-byte entries) and skill points at `+0x17c` |
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
`character_skill_allowed` says the template does not allow the skill.

`character_get_protection` (`0x464de0`) works in one of two ways:

- with no armour worn, it returns `get_skill(1) + level x 0.01`;
- otherwise it adds up `armour_class_table[type +0x08] x type +0xf0` for each worn armour piece.

The item type layout is `ItemType` in `symbols/types.h`. Its fields so far: name `+0x00`, model `+0x0c`,
damage `+0x68` and protection `+0xf0`.

## Experience and levels

`character_add_xp` (`thiscall`, ECX = character) is called by the console command `add_xp`. In order, it:

1. stops at the level cap, `xp_max_level` (50, or 75 with debug flag 2);
2. adds the experience to `+0x604`;
3. gives a share of the experience to the rings in equipment slots 1 and 2 (`ring_add_xp`, `0x44ffc0`), which counts it per ring (`dark_ring_xp`, `fire_ring_xp`, ...);
4. checks `xp_level_reached(level, xp)`, which means `xp >= xp_for_level(level + 1)`;
5. on a level-up, calls `0x489ac0` with the template's class. That call updates the level, base max
   HP, base max AP and the skill points;
6. sets current HP and AP to the new maximum;
7. for party members, copies the template's per-level skill bytes and shows "+Level!".

`xp_level_table` has 0x18-byte records indexed by level. The first field is the experience needed:
level 2 = 100, 3 = 200, 4 = 450 (confirmed in game). Beyond level 20, each level adds 50,000, then
60,000, and so on. Debug flag 2 multiplies the thresholds by 5. The other fields of a record are not
mapped yet.

## Open questions

- The meaning of the two six-value groups at `+0x5ac` (attributes? base and current?).
- The rest of each `xp_level_table` record: four shorts per level that differ between levels.
- Where the resistances and skill points (4) come from. The skill points are probably at template `+0x17c`.
- The rest of the stat panel after protection (`0x42d58f` onwards).
