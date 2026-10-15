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
| `<species>.tbl`, one per character type (`#Character Info`) | `characterinfo_parse_table` (`0x4452f0`) | `char_infos` (`0x6d7e50`), at most 256 x 0x3d0 bytes, count `num_char_infos` (`0x714e54`); moves in `move_infos` (`0x7e12e8`), 3250 x 0x4c bytes | `char_info`, `move_info` |
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
  direction skills, AI ranges and turn rates, intelligence, icon height, drops. The moves are separate
  records (see [Character info and moves](#character-info-and-moves)).

## Character info and moves

**Loading (observed).** `characterinfo_load_party` (`0x4450c0`) runs once per session (`charinfo_party_loaded`,
`0x8b50b1`). It walks `species_table` (`0x580e60`, 223 entries of 12 bytes: name, species id, load byte). For
each entry whose load byte is 1 it parses `"<name>.tbl"` in lower case (`0x4451e6`..`0x445228`). In v1.40
that is 160 files. Their 63 `+Alternate character:` lines add copies, so 223 of the 256 records are used
(counted in the retail `tables.vpp`). A copy is the whole 0x3d0-byte record (`rep movsd`, 0xf4 dwords) with
its own name, resistances and texture entry. It shares the base record's move lists, and `+0x270` points
back at the base record.

Each file starts with `#Character Info`. All `$Character:` blocks of one file go into the same record:
the record pointer is taken once, before the loop. Every retail file has one block.

**Fields not listed elsewhere (observed).**

| Offset | Field |
|---|---|
| `+0x04c` | 16 move list heads, one per move type (`move_type_names` `0x596064`: attack, special attack, parry, dodge, flinch, non-combat flinch, death, cast, movement, misc, transition, ranged attack, emerge, banish, state; list 15 holds the run-time script anims) |
| `+0x094` | `$Skill:` values, 0x54 bytes by skill id (zeroed, then speed = 10) |
| `+0x0e8` | up, down, right, left skill bytes (`0x55` = none) |
| `+0x0f8` | seven resistance levels from `+Resistances:` (default 2, standard; `characterinfo_parse_resistances` `0x446930`) |
| `+0x19c` / `+0x21c` | count and pointer of the record's spell list: the spells of its "cast" moves (`spell_find_by_cast_id` of the variation), in `charinfo_cast_spells` (`0x88904c`); filled only for records that are neither heroes nor bosses (`0x44524d`..`0x4452c1`) |
| `+0x274` | `+Drop:` count; only the first 5 lines are kept (`cmp eax, esi` with `esi = 5`, `0x446420`) although the arrays hold 17 |
| `+0x300` | `+Multiplayer Drop:` count, at most 17 (`cmp eax, 0x11`, `0x4464c4`) |

**Moves (observed).** Each `+Move:` takes a 0x4c-byte record from a free list (`characterinfo_add_move`
`0x446740`, "Move_info array is full" when the 3250 run out) and pushes it on the head of its type's list.
So a list runs from the last move in the file to the first, and a lookup finds the last matching move.

| Offset | Field | Line |
|---|---|---|
| `+0x00` | action name (also the skeleton name) | `$Action:` |
| `+0x04` | anim file | `$Animation:` |
| `+0x08` | skeleton index, -1 while not loaded | |
| `+0x0c` | weapon class bits (`anim_class_names` `0x598fa0`: none, swords, large swords, maces, staves, pole arms, bows, crossbows, daggers); default 1, none | `+Animation class:` |
| `+0x10` | move type | `+Move:` |
| `+0x14` | variation (`variation_names` `0x595cf8`, 194 names) | `$Variation:` |
| `+0x18` | location (low, medium, high) | `$Location:` |
| `+0x1c` | three `{event, time}` triggers; time = frame / 30 | `$Trigger:` |
| `+0x34` | sound reference | `$Sound:` |
| `+0x40` | next move in the list | |
| `+0x44` | contact tag | `$Contact tag:` |
| `+0x48` | effect | `$Effect:` |

**Hurt moves are dropped (observed).** Right after each move is parsed, a record that is not a boss
(`species_is_boss` `0x4a14f0`) unlinks and frees the move if its variation is one of the ten names at
`0x596000`: stand_to_ready_hurt, ready_to_stand_hurt, stand hurt, ready hurt, walk hurt, run hurt, norm walk
hurt, norm run hurt, turn left, turn right (`0x44631a`..`0x446372`). The heroes are not bosses, so they lose
theirs too. In the retail tables this drops 354 of the 3302 `+Move:` blocks from 65 records (among them all four
heroes, Alligator Beast, Aoqi, the bacites and the Orenian soldiers), leaving 2948 records in use. Only the
boss records keep such moves (Tiger Rider, Phoenix Rider, Serpent Rider, Tentacle Beast, Evil Joseph).
*Inferred:* the hurt locomotion slots of every other character fall back to the normal anims (see
[Characters](characters.md#character-fields)).

**Other effects of the move list (observed).** A record with no "tripped lie down" state move (variation
`0x8f`) gets char_info flag `0x8`, cannot be tripped; one with no "pushed" misc move (`0x91`) gets `0x10`,
cannot be pushed (`0x44638a`..`0x4463cc`).

**Limits not checked (observed).** `$Default Weapon:` fills a two-entry array with no bound (`0x44583f`).
`$Trigger:` fills the three trigger slots with no bound (`0x4462e0`..`0x446318`). The retail tables never
exceed either limit. See [Bugs in the original game](game-bugs.md).

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

## Changes (2026-10-15)

- The character info row now names the files (`<species>.tbl`), the 256-record limit and the move records.
- New section [Character info and moves](#character-info-and-moves): how the tables load, the move record
  layout, the dropped hurt moves (354 of 3302 in the retail tables), the drop limits and the unchecked
  `$Default Weapon:` and `$Trigger:` counts. Checked against the disassembly and the retail `tables.vpp`.
