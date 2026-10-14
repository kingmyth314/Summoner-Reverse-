---
title: "Game data tables in memory"
parent: Sum.exe
nav_order: 6.4
---

# Game data tables in memory

**Summary:** the `.tbl` parsers copy each `$Field:` of a record to a fixed offset in a fixed-size record. Reading
which offset each field is stored at gives the in-memory layouts below. The layouts are in `symbols/types.h`.
Each was checked against at least one record in a runtime dump where noted.

| Table file | Parser | Records in memory | Type |
|---|---|---|---|
| `items.tbl` | `items_parse_table` (`0x44a090`) | allocated per item type; an item's `+0x4` points at it | `ItemType` (checked: Short Sword, Tunic) |
| `materials.tbl` | `materials_parse_table` (`0x449f60`) | `materials` (`0x8b9d4c`), 17 x 20 bytes | `material` |
| `spells.tbl` | `spells_parse_table` (`0x47a190`) | `spells` (`0x22e4f28`), 108 x 0x80 bytes | `spell_info` |
| character info (`$Character:`) | `characterinfo_parse_table` (`0x4452f0`) | `char_infos` (`0x6d7e50`), 0x3d0 bytes each, count `num_char_infos` | `char_info` |
| `experience.tbl` | `experience_parse_table` (`0x489930`) | `xp_level_table` (`0x2305420`), 0x18 bytes per level | (first field checked in game) |

## How the offsets are read

The VC6 optimiser often moves the store of a parsed value past the next field's name. A typical sequence is:
`push "$Speed:"`, `call TextFile__ParseFloat`, `push "$Weight:"`, `fstp [ebp + 0xe8]`. So each field is paired
with the first store of a register (not an immediate default) that follows its parse call. Some parsers walk
the table with a pointer into the middle of each record: `spells_parse_table` points at `+0x78`. Their
displacements are then negative and need that base added.

## Highlights

- **Items:** kind `+0x04` (0 weapon, 1 armour, 2 misc), material `+0x08`, value `+0x18`, slot `+0x1c`.
  - Weapon fields: damage `+0x68`, skill `+0x6c` (3 = sword weapons), speed `+0x70`, attack radius `+0x74`.
  - Armour fields: speed `+0xe8`, weight `+0xec`, protection `+0xf0`.
- **Materials:** damage, weight, speed and protection multipliers. Armour protection is
  `materials[material].protection_mult x protection` (see [Characters](characters.md#character-screen-stats)).
- **Spells:** class (heal, dark, energy, holy, fire, ice, summon, misc), cost, skill, hit chance, durations and
  timings, distance, damage type, target radius.
- **Character info:** HP/AP, experience awarded for a kill (`+0x2c`), level, damage, team, size, the four
  direction skills, AI ranges and turn rates, intelligence, icon height, drops.

## Materials (`materials.tbl`)

The 17 materials, in index order (`ItemType +0x08`; names from the parser's list at `0x598f5c`, values from
`materials.tbl`):

| Index | Material | Damage modifier | Weight | Speed | Protection |
|---|---|---|---|---|---|
| 0-4 | default, flesh, leather, stone, wood | 0 | 1.0 | 1.0 | 1.0 |
| 5-9 | bone, fire, water, gas, ice | 0 | 1.0 | 1.0 | 0.5 |
| 10 | copper | -2 | 0.6 | 0.7 | 0.8 |
| 11 | gold | -3 | 1.2 | 1.0 | 0.7 |
| 12 | bronze | -1 | 0.8 | 0.9 | 0.9 |
| 13 | iron | 0 | 1.0 | 1.0 | 1.0 |
| 14 | silver | 0 | 1.1 | 1.1 | 1.0 |
| 15 | admantium | +1 | 0.9 | 0.9 | 1.2 |
| 16 | volitionite | +2 | 1.0 | 1.0 | 1.4 |

The damage modifier is added to `weapon damage x 0.1` before the level factor (see [Combat](combat.md)). "Volitionite" is
presumably a nod to the developer, Volition.

## Spells

The 108 spells in index order (`spells.tbl`; the index is what `spells` (`0x22e4f28`) and `living_entity_get_hit_by_spell`
use). Skill is the school skill needed, Die the die size for damage or healing rolls, Hit the hit chance in percent.
Empty cells are not set in the table.

| # | Name | Class | Skill | Cost | Die | Hit | Damage type |
|---|---|---|---|---|---|---|---|
| 0 | heal | heal | 10 | 4 | 12 |  |  |
| 1 | cure | heal | 20 | 2 |  |  |  |
| 2 | regenerate | heal | 40 | 4 |  |  |  |
| 3 | resurrect | heal | 60 | 10 |  |  |  |
| 4 | vitalize | heal | 80 | 10 | 8 |  |  |
| 5 | revive | heal | 100 | 20 |  |  |  |
| 6 | Poison | heal | 1 | 5 | 4 | 80 |  |
| 7 | blind | dark | 10 | 2 |  | 60 |  |
| 8 | silence | dark | 20 | 3 |  | 60 |  |
| 9 | black cloud | dark | 40 | 5 |  |  |  |
| 10 | recall | dark | 60 | 10 |  |  |  |
| 11 | invisibility | dark | 80 | 20 |  |  |  |
| 12 | death | dark | 100 | 20 |  | 30 |  |
| 13 | Life Drain | dark | 1 | 10 | 4 | 80 |  |
| 14 | light | energy | 10 | 1 |  |  |  |
| 15 | lightning | energy | 20 | 3 | 6 |  | energy |
| 16 | empower | energy | 70 | 8 |  |  |  |
| 17 | power leech | energy | 100 | 5 |  |  |  |
| 18 | destroy undead | holy | 15 | 4 |  |  |  |
| 19 | bless | holy | 10 | 4 |  |  |  |
| 20 | wrath of god | holy | 50 | 10 |  |  |  |
| 21 | protect | holy | 30 | 8 |  |  |  |
| 22 | faith | holy | 70 | 10 |  |  |  |
| 23 | curse | holy | 100 | 20 |  | 75 |  |
| 24 | Jade Beam | holy | 1 | 20 | 20 | 80 |  |
| 25 | fire arrow | fire | 10 | 4 | 8 |  | fire |
| 26 | fireball | fire | 20 | 6 | 13 |  | fire |
| 27 | burning hands | fire | 30 | 2 |  |  | fire |
| 28 | meteor storm | fire | 40 | 6 | 10 |  | fire |
| 29 | wall of fire | fire | 70 | 10 |  |  | fire |
| 30 | inferno | fire | 100 | 20 | 13 |  | fire |
| 31 | dark wall | fire | 1 | 8 |  |  | fire |
| 32 | icicle | ice | 10 | 5 | 8 |  | ice |
| 33 | icy Sleep | ice | 30 | 4 | 4 | 65 | ice |
| 34 | freeze | ice | 50 | 6 |  | 55 |  |
| 35 | blizzard | ice | 70 | 15 |  |  | ice |
| 36 | ice coffin | ice | 100 | 20 |  | 50 |  |
| 37 | Blue Imp Sleep | ice | 1 | 1 | 4 | 70 |  |
| 38 | summon golem | summon | 0 | 0 |  |  |  |
| 39 | summon red minotaur | summon | 0 | 0 |  |  |  |
| 40 | summon blur | summon | 0 | 0 |  |  |  |
| 41 | summon blue imp | summon | 0 | 0 |  |  |  |
| 42 | summon blackfire elemental | summon | 0 | 0 |  |  |  |
| 43 | summon blood elemental | summon | 0 | 0 |  |  |  |
| 44 | summon poison elemental | summon | 0 | 0 |  |  |  |
| 45 | summon mist elemental | summon | 0 | 0 |  |  |  |
| 46 | summon wraith | summon | 0 | 0 |  |  |  |
| 47 | summon jade golem | summon | 0 | 0 |  |  |  |
| 48 | summon laharah | summon | 0 | 0 |  |  |  |
| 49 | summon samurai | summon | 0 | 0 |  |  |  |
| 50 | summon jade dragon | summon | 0 | 0 |  |  |  |
| 51 | summon water dragon | summon | 0 | 0 |  |  |  |
| 52 | summon wind dragon | summon | 0 | 0 |  |  |  |
| 53 | summon forest dragon | summon | 0 | 0 |  |  |  |
| 54 | banish summon | summon | 0 | 0 |  |  |  |
| 55 | generic arrow | misc | 0 | 0 |  |  | piercing |
| 56 | gsal fireball | misc | 0 | 0 | 8 |  | fire |
| 57 | gsal bite | misc | 0 | 0 | 9 |  | piercing |
| 58 | pushback | misc | 0 | 0 |  | 200 |  |
| 59 | trip | misc | 0 | 0 |  | 200 |  |
| 60 | poisoning | misc | 0 | 0 |  | 100 |  |
| 61 | paralyze | misc | 0 | 0 |  | 100 |  |
| 62 | leech | misc | 0 | 0 |  |  |  |
| 63 | conversion | misc | 0 | 0 |  |  |  |
| 64 | iceshield | misc | 0 | 0 |  |  | ice |
| 65 | fireshield | misc | 0 | 0 |  |  | fire |
| 66 | energyshield | misc | 0 | 0 |  |  | energy |
| 67 | sleep | misc | 0 | 0 |  | 100 |  |
| 68 | apdrain | misc | 0 | 0 |  |  |  |
| 69 | backstab | misc | 0 | 0 |  |  | piercing |
| 70 | Assess | misc | 0 | 0 |  |  |  |
| 71 | Aimed Attack | misc | 0 | 0 |  |  |  |
| 72 | Nightmare | misc | 0 | 0 |  |  |  |
| 73 | Rider Stun | misc | 0 | 0 |  |  |  |
| 74 | Phoenix burning hands | misc | 0 | 0 |  |  |  |
| 75 | Phoenix Inferno | misc | 0 | 0 |  |  |  |
| 76 | Titus stone gaze | misc | 0 | 0 |  |  |  |
| 77 | Pyrul fire arrow | misc | 0 | 0 | 10 |  | fire |
| 78 | Pyrul fireball | misc | 15 | 6 | 10 |  | fire |
| 79 | Pyrul meteor storm | misc | 50 | 6 | 10 |  | fire |
| 80 | Ghost Paralyzing Fear | misc | 50 | 6 |  | 45 |  |
| 81 | Ghost Summon Bone Servant | misc | 50 | 6 |  |  |  |
| 82 | Ghost Summon Bone Mage | misc | 50 | 6 |  |  |  |
| 83 | Ghost Summon Undead | misc | 50 | 6 |  |  |  |
| 84 | Ghost death | misc | 20 | 3 |  | 5 |  |
| 85 | Urath blind | misc | 0 | 0 |  | 75 |  |
| 86 | Urath silence | misc | 0 | 0 |  | 70 |  |
| 87 | Urath death | misc | 0 | 0 |  | 70 |  |
| 88 | Urath curse | misc | 0 | 0 |  | 200 |  |
| 89 | Urath poison | misc | 0 | 0 |  | 95 |  |
| 90 | Urath paralyze | misc | 0 | 0 |  | 75 |  |
| 91 | Urath dark spear | misc | 0 | 0 | 14 |  |  |
| 92 | Dark Joseph Summon Blood | misc | 50 | 6 |  |  |  |
| 93 | Dark Joseph Summon Mist | misc | 50 | 6 |  |  |  |
| 94 | Dark Joseph Summon Poison | misc | 50 | 6 |  |  |  |
| 95 | Dark Joseph Summon Blackfire | misc | 50 | 6 |  |  |  |
| 96 | Dark Joseph Summon Blade | misc | 50 | 6 |  |  |  |
| 97 | Dark Joseph Summon Lich | misc | 50 | 6 |  |  |  |
| 98 | Dark Joseph Summon Brownstone Golem | misc | 50 | 6 |  |  |  |
| 99 | Dark Joseph Summon Jade Golem | misc | 50 | 6 |  |  |  |
| 100 | Mach meteor storm | misc | 50 | 6 | 10 |  | fire |
| 101 | Dark void | misc | 50 | 6 |  |  |  |
| 102 | Laharah's scream | misc | 50 | 6 | 12 |  |  |
| 103 | Failed Dark void | misc | 50 | 6 |  |  |  |
| 104 | Jade Dragon | misc | 50 | 6 |  |  | fire |
| 105 | Water Dragon | misc | 50 | 6 |  |  | ice |
| 106 | Four Winds Dragon | misc | 50 | 6 |  |  | energy |
| 107 | Forest Dragon | misc | 50 | 6 |  |  |  |
