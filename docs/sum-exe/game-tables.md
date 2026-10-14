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
