---
title: "Savegame format"
parent: Sum.exe
nav_order: 6.5
---

# Savegame format (`.sav`)

**Summary:** a `.sav` file is a raw dump of one fixed-size memory buffer, `savegame_buffer` (`0x2494b2c`,
0x4b000 bytes reserved). The buffer is owned by the savegame object `savegame_state` (`0x2493a38`). The
buffer is split into 14 sections at fixed offsets. The layout is computed once at start-up from the game
tables, so every save made by the same build of v1.40 is exactly **270,755 bytes**. The file starts with a
492-byte header (`SAVg`, version `0x27`, level name, description and a table of section sizes). A 201x151
RGB565 thumbnail follows at `0x200`, then the sections. Five sections start with a four-byte tag (`INVT`,
`ITEM`, `LENT`, `CMRA`, `CONT`). They are rebuilt from game objects when the game is saved, and applied to
new objects when it is loaded. The other sections have no tag. Those sections (story flags and variables,
level visit counts, conversation state, quest bits, level items, two-stage objects, automap labels and hot
buttons) *are* the live game state. The game reads and writes them in the buffer all the time, so saving
writes them to the file unchanged and loading reads them back. There is no compression, encryption or
checksum over the data.

`tools/savinfo.mjs` reads these fields from a save file (read-only).

## Evidence

- Disassembly (`node tools/asm.mjs fn`) of `savegame_write` (`0x4d1c40`), `savegame_read` (`0x4d25b0`),
  `savegame_write_header` (`0x4d1df0`), `savegame_read_header` (`0x4d29f0`), `savegame_report_sizes`
  (`0x4ceff0`), `savegame_report_read_sizes` (`0x4cf260`), every chunk writer and reader, the 14 layout
  functions `0x4d1800`..`0x4d1c10` (called in order from `level_items_load_names`, `0x4d4160`), and the
  character serializer `character_save_state` (`0x457150`, proposed name).
- Hex dumps of the eight saves in `Summoner/savegame/` (`summoner0.sav`..`summoner7.sav`; six in `masad`,
  two in `lenele1d`). They were read with `node`, and no file was written. All eight are 270,755 bytes, have
  identical size tables, and every tag is at the offset the layout code predicts. Joseph's character
  record decodes to level 3, 230 XP, HP 128/128 and AP 26/26, in line with the
  [Characters](characters.md) runtime dump.

## How the buffer is built (observed)

`savegame_alloc` (`0x4d3920`) calls `savegame_init_buffer` (`0x4cfbb0`) with buffer `0x2494b2c` and size
0x4b000. That zeroes the buffer. `level_items_load_names` then runs the layout functions. Each one stores a
section pointer and size in the savegame object and advances the running total at `+0x08`. The total
becomes the file size.

| Object field | Section | Layout function | Capacity (bytes) |
|---|---|---|---|
| `+0x00` / `+0x04` / `+0x08` | buffer, reserved size (0x4b000), used size (= file size) | `0x4cfbb0` | |
| `+0x0c` / `+0x10` | header | `0x4d1800` | 0x1f8 (504) |
| `+0x14` / `+0x18` | thumbnail, start aligned to 16 | `0x4d1820` | 0xed1e (60,702 = 201 x 151 x 2) |
| `+0x1c` | level visit counts (start of level data) | `0x4d1860` | 0xcc (51 dwords) |
| `+0x20 + 0x18*n` | per-level data block n: `{ptr, flags, automap bits, ints, floats, size}` | `0x4d1860` | from `level_script_table` |
| `+0x101c` / `+0x1020` | `INVT` party inventory | `0x4d1a50` | 0x20a0 (8,352) |
| `+0x4e8`, `+0x4ec + 8*(7n+i)` | conversation state, 7 conversations x 51 levels | `0x4d18e0` | from the conversation table `0x25388c0` |
| `+0x1014` / `+0x1018` | `ITEM` item-type flags | `0x4d19e0` | 8 + 4*N + (N+7)/8, N = item type count |
| `+0x1024` / `+0x1028` | `LENT` living entities | `0x4d1a80` | 0x34 + 175 x 0x180 + 0x198 = 67,660 |
| `+0x102c` / `+0x1030` | `CMRA` camera | `0x4d1ae0` | 0x28 (40) |
| `+0x1034` / `+0x1038` | `CONT` containers and gold | `0x4d1b10` | 0x18b4 (6,324) |
| `+0x103c` / `+0x1040` | quest bits | `0x4d1b40` | 0x19 (25 = 200 bits) |
| `+0x1044` / `+0x1048` | level items | `0x4d1b80` | 0x804 (2,052) |
| `+0x104c` / `+0x1050` | two-stage objects | `0x4d1bb0` | 0x673f (26,431) |
| `+0x1054` / `+0x1058` | automap labels | `0x4d1be0` | 0x1a04 (6,660) |
| `+0x105c` / `+0x1060` | hot buttons | `0x4d1c10` | 0xb4 (180) |
| `+0x1064` / `+0x10e4` | pending-load filename (0x80) and slot | `savegame_set_pending_load` | not saved |

The buffer order is the layout-call order. It differs from the order of the size table in the header
(`INVT` comes before the conversation block in the buffer, but after it in the table).

## File layout (observed in all eight saves)

| File offset | Size | Section |
|---|---|---|
| `0x00000` | 0x1ec used, 0x1f8 reserved, zero-padded to `0x200` | header |
| `0x00200` | 60,702 | thumbnail |
| `0x0ef1e` | 25,035 | level data (visit counts + per-level flags/bits/ints/floats) |
| `0x150e9` | 8,352 | `INVT` |
| `0x17189` | 64,624 | conversation state |
| `0x26df9` | 2,158 | `ITEM` |
| `0x27667` | 67,660 | `LENT` |
| `0x37eb3` | 40 | `CMRA` |
| `0x37edb` | 6,324 | `CONT` |
| `0x3978f` | 25 | quest bits |
| `0x397a8` | 2,052 | level items |
| `0x39fac` | 26,431 | two-stage objects |
| `0x406eb` | 6,660 | automap labels |
| `0x420ef` | 180 | hot buttons |
| `0x421a3` | end of file (270,755) | |

All integers are little-endian. Sections are packed with no padding, so most of them start at odd
offsets.

## Header (observed)

Written by `savegame_write_header`, checked by `savegame_read_header`.

| Offset | Type | Field | Example (`summoner7.sav`) |
|---|---|---|---|
| `0x000` | char[4] | tag `SAVg` (`0x67564153`) | `SAVg` |
| `0x004` | int32 | version; must be exactly `0x27` (39) | 39 |
| `0x008` | char[0x80] | `level_filename` (`0x60a2c8`) | `lenele1d` |
| `0x088` | char[0x20] | level script name (`0x60a2e8`, proposed `level_script_name`) | `lenele1d` |
| `0x0a8` | int32 | always 0 when written | 0 |
| `0x0ac` | float | `level_time` (`0x60a654`): unpaused seconds since the level was entered | 756.26 |
| `0x0b0` | char[0x100] | description typed by the player (`No description` by default) | `No description` |
| `0x1b0` | int32[13] | section sizes: thumb, level data, conversation, items, inv, le, camera, item_object, quest_flag, level_item, two_stage, automap_labels, hot_buttons | 60702, 25035, 64624, 2158, 8352, 67660, 40, 6324, 25, 2052, 26431, 6660, 180 |
| `0x1e4` | uint32 | `Script_filenames_tbl_checksum` (`0x25443ac`) | `0x985817c3` |
| `0x1e8` | int32 | header length, offset of this field + 4 | 0x1ec |

The size table is written by `savegame_report_sizes`, which also prints each value (`"thumb_size = %d"`
...). The level-data value is 0xcc plus the sum of the 51 per-level sizes, and the conversation value is the
sum of the 357 conversation sizes.

{: .note }
`savegame_read_header` copies field `0xa8` into the caller's third argument, which `savegame_read` passes as
`&level_time`. Field `0xac` (the saved time) goes into a local variable and is not used. A load therefore
sets `level_time` to 0. This has no effect, because `state_game_enter` zeroes `level_time` anyway
(*observed*; the swap looks like a harmless bug).

## Thumbnail (observed)

`saveload_capture_thumbnail` renders the scene into a 201x151 clip and stores it as RGB565 at `0x5e5d28`.
`savegame_write_thumbnail` copies the 60,702 bytes into the section. `savegame_alloc` creates the bitmap
(`bm_create(3, 201, 151, ...)`, handle in `0x59e794`) directly on the section memory, so the Save/Load
screen draws the buffer itself. `savegame_read_thumbnail_file` reads the first `0x1f8 + 0xed1e` bytes of a
file into the buffer for the preview. Because the thumbnail really starts at `0x200`, the last 8 bytes
(4 pixels) are not reloaded for the preview (*observed*, cosmetic).

## Level data: story flags, automap and variables (observed)

The section starts with 51 dwords, one per `level_script_table` record (`0x59f128`). Each dword counts how
many times that level has been entered: `savegame_inc_visit_count` (`0x4cfc80`, proposed) is called from
`state_game_enter`. In the saves, `masad` (index 3) has 1, and the two `lenele1d` saves also have 1 at
indices 4 and 7.

After the counts come 51 per-level blocks. Their sizes come from five `level_script_info` fields that
`symbols/types.h` does not map yet:

| `level_script_info` offset | Meaning | Used by |
|---|---|---|
| `+0x24` | number of named flags | `savegame_get_flag`, `savegame_set_flag` |
| `+0x28` | pointer to the flags' name hashes (`name_hash_nocase`) | same |
| `+0x2c` | automap layers; each layer is 64x64 = 4096 bits | `savegame_get_bit`, `savegame_set_bit` |
| `+0x30` / `+0x34` | number of named int variables / their hashes | `savegame_get_int`, `savegame_set_int` |
| `+0x38` / `+0x3c` | number of named float variables / their hashes | `savegame_get_float`, `savegame_set_float` (`0x4d0130` / `0x4d01c0`, proposed; used by `ls_worldmap1_*`) |

Block layout (`savegame_data_offset`, `0x4cfbd0`, returns a bit offset):

| Part | Size |
|---|---|
| flag bits, one per named flag, LSB first | `flags` bits |
| automap reveal bits (`automap_reveal_cell`) | `layers x 4096` bits |
| padding to a whole byte | |
| int32 variables | `4 x ints` bytes |
| float variables | `4 x floats` bytes |

Block size = `(flags + layers*4096 + 7) / 8 + 4 * (ints + floats)`. A flag is addressed by (level index,
name). The name is hashed and looked up in that level's hash list, so the saved data stores no names. The
flag order is the order of the names in the level's flag list in `Sum.exe`. Flags of other levels stay
readable after you leave a level. `savegame_find_flag_level` (`0x4d0240`) searches all 51 lists. Persistent
values such as `chain_config_set`'s `chainconfig%d%d` are stored as int variables.

## `INVT`: party inventory (observed)

| Offset | Type | Field |
|---|---|---|
| `+0x00` | char[4] | `INVT` |
| `+0x04` | int32 | `0x971868`, probably the party's gold (*inferred*: `buysell_recharge_item` subtracts from it; 52/61/41 in the saves) |
| `+0x08` | int32 | entry count n (clamped to 0..0x209) |
| `+0x0c` | n x 12 | `{uint32 item-type name hash, int32 count, int32 item field +0x08}` from the inventory list `0x970820` |
| then | int32 | chunk length (12 + 12n + 4) |

On load, `party_inventory_clear` runs, then `party_inventory_add_type(type, count, field, 1)` for each
entry. An unknown hash fails the load with `"Couldn't find item type!!!"`. The trailing length must match.
Two renamed items are mapped on load, in both `INVT` and `ITEM`: `Brown Dragon Scale` -> `Gray Dragon
Scale` and `Ring of Preservation` -> `Preservation Ring`, so older saves still load. The meaning of the
third field (0 or 1 in the saves) is *unknown*.

## Conversation state (inferred)

There are 7 conversation slots for each of the 51 levels, described by the table at `0x25388c0` (0x10 bytes
per slot: name pointer, speaker count *a*, topic count *b*). Slots whose name ends in `_m1` (multiplayer)
get no storage. A slot's block is `4 + 8a + 4b + align8(a) + align8(4b)` bytes. It holds a leading dword
that is nonzero once state has been saved (`savegame_has_conversation_data`), the speaker and topic name
hashes, and bit fields. `conversation_save_state` (`0x4d8390`) writes one bit per speaker and four bits
per topic (availability and three other topic flags, one of which marks the current topic) through
`savegame_set_speaker_bit` (`0x4d06e0`) and `savegame_set_topic_bit` (`0x4d09e0`), both proposed.
`conversation_restore_state` applies them when the level's conversation file is parsed. The exact bit
meanings are *unknown*.

## `ITEM`: item-type flags (observed)

| Offset | Type | Field |
|---|---|---|
| `+0x00` | char[4] | `ITEM` |
| `+0x04` | int32 | item type count N (521 in all saves); must equal the current `item_types` count (`0x8ec7c0`), or the load fails |
| `+0x08` | N x uint32 | name hash of each item type, in table order |
| then | (N+7)/8 bytes | bit i = bit 0 of `item_types[i]` flags (`+0x5c`) |

The meaning of flag bit 0 is *unknown*. Bit 2 of the same field is "cannot destroy" (`item_can_destroy`).

## `LENT`: living entities and per-character state (observed)

| Offset | Type | Field |
|---|---|---|
| `+0x00` | char[4] | `LENT` |
| `+0x04` | int32 | object handle counter `0x59a5a0` (see `obj_handle_base_next`) |
| `+0x08` | int32 | `0x59b7c8`, a second counter (*unknown*) |
| `+0x0c` | 0x24 bytes | party state (`savegame_write_party_state`, `0x4cf500`, proposed), below |
| `+0x30` | int32 | record count, at most 175 (0xaf); more fails the save |
| `+0x34` | records | one per entity: `uint32 name hash` + 0x17c bytes of `character_save_state`; for type-7 player characters also `float player+0x178` and `int32 skill_points` (player `+0x17c`) |
| then | groups | group table from `0x243e040` (10 x 0x54): count, then per group the leader's name hash, the member count - 1, the members' hashes, and the dword at `+0x4c` |
| then | int32 | used chunk length (30,032 in `summoner7.sav`); must match on load (`"total_size = %d, offset = %d"`) |

Records are written for the living-entity list `0x8ecd78` (next at `+0x318`), for entities in list
`0x8ed500` that have flag `0x100` in `+0x320` without `0x40000`, and for the "other players" list
`player_list_other`. On load, each record is matched *by name hash* to an entity that the level has already
created (`0x4cf790`, proposed `savegame_read_character`). Records without a match make the load fail, so a
save only loads into the level it was made in.

**Party state dword** (`+0x0c`). Example `0xfffffc01`: Joseph only, Joseph selected, no summon.

| Bits | Meaning |
|---|---|
| 0-3 | party members present, by species index (bit 0 = Joseph); missing members are created on load |
| 4 | combat mode (`0x5fccb4`) |
| 5 | `game_debug_flags & 0x1000` (toggled back with `0x439620` on load) |
| 6 | a summon was under control (`0x5fcc84` non-zero); the summon is then created as type 7, else type 5 |
| 7-9 | species index of the selected party member (signed; -1 = the summon is selected) |
| 10-31 | species index of the active summon (`0x5fcc8c`), -1 = none |

The 32 bytes after the dword hold the summon's name.

**Character record** (`character_save_state`, `0x457150`). The layout differs by entity type (4..9); the
size is always padded to 0x17c. The offsets below were decoded from Joseph's record (type 7) in
`summoner7.sav`:

| Record offset | Size | Source (`Character` field) |
|---|---|---|
| `0x000` | 4 | type `+0x00c` |
| `0x004` | 12 | position x, y, z `+0x010`..`+0x018` |
| `0x010` | 4 | heading, from the orientation matrix `+0x038` (`matrix3__extract_angles`) |
| `0x014` | 4 | `+0x68c` |
| `0x018` | 4 | flags `+0x084` |
| `0x01c` | 4 | flags `+0x320` |
| `0x020` | 4 x 19 | up to 4 active effect slots out of the 18 at `+0x0ac` (0x20 bytes each): state byte, slot index, byte `+0xbc`, two `GameTimer` remaining times (`+0xb0`, `+0xc0`), int `+0xc4`, float `+0xc8`; unused slots begin with `0xff` |
| `0x06c` | 9 x 4 | equipped items `+0x754`: item-type index, -1 = empty (Joseph: 277, 465, 343) |
| `0x090` | 36 x 4 | remaining time of the 36 player timers (`Player +0x028`) |
| `0x120` | 36 | the first 36 skill bytes `+0x614` |
| `0x144` | 2 | level `+0x600` (3) |
| `0x146` | 4 | experience `+0x604` (230) |
| `0x14a` | 4 | HP `+0x5f8` (128.0) |
| `0x14e` | 4 | max HP `+0x5f4` (128.0) |
| `0x152` | 2 | AP `+0x5fe` (26) |
| `0x154` | 2 | max AP `+0x5fc` (26) |
| `0x156` | 4 | `+0x720` |
| `0x15a` | var | AI state (`ai_state_save`, `0x4746d0`, proposed, on `+0x324`) |
| last | 1 | `+0x0a4` if the character is dead (`+0x320 & 0x80`), else 0 |

Non-player types (*observed in code, not decoded from a file*) skip the equipment, timer and skill blocks.
Some also save a timer, an int and a bool from the 0x550-byte record `0x24f6748 + 0x550 * (+0x6dc)`.

## `CMRA`: camera (observed)

`CMRA` followed by 9 dwords from `camera_get_state` (`0x478890`, proposed) on the camera `0x60a018`. On
load, `camera_set_state` (`0x4788e0`) is applied, the camera is re-targeted on `cur_character`, and the load
fails if `cur_character` is 0. In the saves the first dword is 35 and the others are floats (6.0, a yaw, 0.8,
2.0, 66.55, 3.3) and two ints (2, 2); their meanings are *unknown*.

## `CONT`: containers and gold piles (observed)

| Part | Layout |
|---|---|
| header | `CONT`, int32 container count |
| per container | the 40 records of `0x2302fe0` (0xe8 bytes each), in use, with items, and with first item `+0xa0 < 0`: float x, y, z, int32 item count, byte (`+0xe4` x constant, rounded), then per item `{int32 item-type index, int32 item +0x9c}` |
| gold piles | int32 count (at most 32), then `{float x, y, z, int32 amount +0x9c}` for loose gold objects (`obj_is_gold`) in the object list `0x970770` |

All eight saves have no containers (count 0).

## Quest bits (observed)

200 bits (`savegame_get_quest_bit` / `savegame_set_quest_bit`, `+0x103c`). Bit n is quest n of the journal
(`journal_init`, `journal_draw_quest_list`). The layout function calls `journal_notify_updated(n, 0)` for
n = 0..199.

## Level items (observed / inferred)

An int32 count plus up to 512 (0x200) name hashes. At start-up the list is filled from `level_items.tbl`
(`#Level Items`, `$Name:`); there are 177 entries in all eight saves. `savegame_read_level_items`
(`0x4d33f0`) deletes every placed item object (type 3) whose level-item name hash is no longer in the list.
So the list holds the unique level items that have not been picked up yet (*inferred*; removal is
`savegame_level_item_remove`).

## Two-stage objects (observed)

51 int32 counts, then 51 x 121 (0x79) name hashes, then 2 bits for each (level, entry) pair at byte
`0x6138`. An entry's hash is `name_hash_nocase(level_script_name + object name)`.
`savegame_two_stage_set` (`0x4d0e20`) and `savegame_two_stage_get` (`0x4d1110`), both proposed, are called
by `character_use_skill`, `object_use_result` and the door code. On load, `savegame_read_two_stage` moves each
two-stage mesh (`0xa5a304`, 0x8c bytes each) to its first- or second-stage position from bit 0. For the
other kind of entry it finds (table `0x248ce40`, `"checking lc"`) it sets a "disabled" flag from bit 0. A
hash that matches neither only produces `"WARNING: two-stage mesh chksum %d doesn't correspond..."`; the
load continues. The meaning of bit 1 is *unknown* (for doors it is probably "unlocked").

## Automap labels (observed)

An int32 count plus up to 128 (0x80) labels of 0x34 bytes: `{int32 level index, int32 layer, float x,
float y, int32, char[32] text}`. They are added by `savegame_automap_label_add` (`0x4d12b0`, proposed) from
the automap label editor. `savegame_read_automap_labels` rebuilds the current level's labels with
`automap_label_alloc`.

## Hot buttons (observed)

5 characters x 6 buttons x 6 bytes = 180: `{byte used, byte flag, int32 id}`. `savegame_hot_button_set`
(`0x4d16a0`) clears the same id from the character's other buttons. Joseph had no hot buttons in any of the
saves. No load-time reader exists: the HUD (`0x40f340`) reads the buffer directly.

## Saving and loading

**Save** (`savegame_write`, *observed*): `conversation_save_state` runs first (from the caller), then the
header, thumbnail, `ITEM`, `INVT`, `LENT`, `CMRA` and `CONT` writers rebuild their sections. Seven calls
to a stub (`0x4d25a0`, `return 1`; five more in `savegame_read`) mark where the untagged sections would be
written; those sections are already
current in the buffer. Then the file is opened with mode 2 and the whole used buffer (`+0x08` bytes) is
written in one `File__write`. Writers fail the save if a section would overflow its capacity (for example
more than 175 entities). Slots are named `summoner%d.sav`, plus `quicksave.sav`.

**Load** (*observed*): the header only is read first (`savegame_read_info`, used by the Save/Load list,
`quickload` and `cmd_loadgame`). `savegame_set_pending_load` stores the filename. `game_change_level` then
switches to the saved level, and after the level has loaded, `state_game_enter` calls
`savegame_load_pending`, which calls `savegame_read`. That function reads the whole file back over the
buffer and runs the checks below in order. The first failure prints a marker (`"5a"`..`"14a"`) and aborts:

1. `savegame_read_header`: tag `SAVg`; version == 0x27; the level name is in `level_script_table` and the
   script name is valid for it (else -1); every one of the 13 section sizes equals the current layout
   (else 0, with `"i = %d, <name>_size = %d"`); the header-length field == 0x1ec. The script-table checksum
   is read but **not** compared.
2. `ITEM`: tag and item-type count; every hash must resolve.
3. `INVT`: tag, every hash must resolve, trailing length.
4. `LENT`: tag, at most 175 records, every record matches an existing entity, trailing length.
5. `CMRA`: tag and a current character.
6. `CONT`: tag.
7. Level items, two-stage objects and automap labels (no tags; always succeed).

Because the sizes are checked against the running game's tables, a save only loads into a game whose
`items.tbl`, level script table and conversation tables give the same section sizes. A modded table that
adds an item type or a flag makes all old saves fail the header check (*inferred*).

The per-level flag and variable tables are fields `+0x24`-`+0x3c` of `level_script_info` in `symbols/types.h`.
