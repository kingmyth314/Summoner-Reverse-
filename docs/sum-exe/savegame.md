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
writes them to the file unchanged and loading reads them back. There is no compression and no encryption.
The header holds a script-table checksum, but the loader skips it, so nothing checks the data.

`tools/savinfo.mjs` reads these fields from a save file (read-only). `tools/savedit.mjs` writes an edited *copy* (description,
gold, a party member's level, experience, HP/AP and skill points); a copy edited this way loaded in game with the new
gold and skill points (2026-10-14).

{: .important }
Corrected 2026-10-15 against the disassembly and `src/decomp/game/level_scripts.cpp`. The `LENT` rule for the
second entity list was reversed, the checksum is skipped rather than read, a load can fail in the two-stage
section, the `CONT` lock byte is truncated, and loose gold piles lose their amount on load. The automap label
record, the per-level flag source (`events.tbl`) and the load sequence were also wrong or incomplete. The list is at
the [end of the page](#changes-2026-10-15).

## Evidence

- Disassembly (`node tools/asm.mjs fn`) of `savegame_write` (`0x4d1c40`), `savegame_read` (`0x4d25b0`),
  `savegame_write_header` (`0x4d1df0`), `savegame_read_header` (`0x4d29f0`), `savegame_report_sizes`
  (`0x4ceff0`), `savegame_report_read_sizes` (`0x4cf260`), every chunk writer and reader, the 14 layout
  functions `0x4d1800`..`0x4d1c10` (called in order from `savegame_build_layout`, `0x4d4160`), and the
  character serializer `character_save_state` (`0x457150`, proposed name).
- The decompiled unit [`level_scripts.cpp`](../../src/decomp/game/level_scripts.cpp) (`0x4ceec0`..`0x4d6651`,
  written from the disassembly) and its notes.
- Hex dumps of the eight saves in `Summoner/savegame/` (`summoner0.sav`..`summoner7.sav`; six in `masad`,
  two in `lenele1d`). They were read with `node`, and no file was written. All eight are 270,755 bytes, have
  identical size tables, and every tag is at the offset the layout code predicts. Joseph's character
  record decodes to level 3, 230 XP, HP 128/128 and AP 26/26, in line with the
  [Characters](characters.md) runtime dump. Walking the `LENT` records with the rules below lands exactly on
  the trailing length in all eight (22,368 in the `masad` saves, 30,032 in the `lenele1d` saves).

## How the buffer is built (observed)

`savegame_alloc` (`0x4d3920`) calls `savegame_init_buffer` (`0x4cfbb0`) with buffer `0x2494b2c` and size
0x4b000. That zeroes the buffer. `savegame_build_layout` then runs the layout functions. Each one stores a
section pointer and size in the savegame object and advances the running total at `+0x08`. The total
becomes the file size.

| Object field | Section | Layout function | Capacity (bytes) |
|---|---|---|---|
| `+0x00` / `+0x04` / `+0x08` | buffer, reserved size (0x4b000), used size (= file size) | `0x4cfbb0` | |
| `+0x0c` / `+0x10` | header | `0x4d1800` | 0x1f8 (504) |
| `+0x14` / `+0x18` | thumbnail, start aligned to 16 | `0x4d1820` | 0xed1e (60,702 = 201 x 151 x 2) |
| `+0x1c` | level visit counts (start of level data) | `0x4d1860` | 0xcc (51 dwords) |
| `+0x20 + 0x18*n` | per-level data block n: `{ptr, flag count, automap bits, int count, float count, size}` | `0x4d1860` | from `level_script_table` |
| `+0x101c` / `+0x1020` | `INVT` party inventory | `0x4d1a50` | 0x20a0 (8,352) |
| `+0x4e8`, `+0x4ec + 8*(7n+i)` | conversation state, 7 script slots x 51 levels | `0x4d18e0` | from `conversation_save_table` (`0x25388c0`) |
| `+0x1014` / `+0x1018` | `ITEM` item-type flags | `0x4d19e0` | 8 + 4*N + (N+7)/8, N = item type count |
| `+0x1024` / `+0x1028` | `LENT` living entities | `0x4d1a80` | 0x34 + 175 x 0x180 + 0x198 = 67,660 |
| `+0x102c` / `+0x1030` | `CMRA` camera | `0x4d1ae0` | 0x28 (40) |
| `+0x1034` / `+0x1038` | `CONT` containers and gold | `0x4d1b10` | 0x18b4 (6,324 = 8 + 40 x 145 + 4 + 32 x 16) |
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
offsets. A tool can compute every offset from the size table: thumbnail at `0x200`, level data after it,
then `INVT`, the conversation block, `ITEM` and so on in the order above (*inferred* from the layout code).

## Header (observed)

Written by `savegame_write_header`, checked by `savegame_read_header`.

| Offset | Type | Field | Example (`summoner7.sav`) |
|---|---|---|---|
| `0x000` | char[4] | tag `SAVg` (`0x67564153`) | `SAVg` |
| `0x004` | int32 | version; must be exactly `0x27` (39) | 39 |
| `0x008` | char[0x80] | `level_filename` (`0x60a2c8`) | `lenele1d` |
| `0x088` | char[0x20] | `level_script_name` (`0x60a2e8`) | `lenele1d` |
| `0x0a8` | int32 | always 0 when written | 0 |
| `0x0ac` | float | `level_time` (`0x60a654`): unpaused seconds since the level was entered | 756.26 |
| `0x0b0` | char[0x100] | description typed by the player (`No description` by default); 256 characters or more fail the save (`cmp ecx, 0xff` at `0x4d1eee`) | `No description` |
| `0x1b0` | int32[13] | section sizes: thumb, level data, conversation, items, inv, le, camera, item_object, quest_flag, level_item, two_stage, automap_labels, hot_buttons | 60702, 25035, 64624, 2158, 8352, 67660, 40, 6324, 25, 2052, 26431, 6660, 180 |
| `0x1e4` | uint32 | `Script_filenames_tbl_checksum` (`0x25443ac`); written, never read | `0x985817c3` |
| `0x1e8` | int32 | header length, offset of this field + 4 | 0x1ec |

The size table is written by `savegame_report_sizes`, which also prints each value (`"thumb_size = %d"`
...). The level-data value is 0xcc plus the sum of the 51 per-level sizes, and the conversation value is the
sum of the 357 conversation sizes.

{: .note }
`savegame_read` passes `&level_time` as the **fourth** argument of `savegame_read_header` (`push 0x60a654` at
`0x4d2627`). The header reader copies field `0xa8` into the third argument (a local) and field `0xac`, the saved
time, into `&level_time` (`0x4d2ad7`, `0x4d2aea`). A load therefore restores the level time (*observed*). An
earlier version of this note said the two fields were swapped; that was a misreading of the argument order.

## Thumbnail (observed)

`saveload_capture_thumbnail` renders the scene into a 201x151 clip and stores it as RGB565 at `0x5e5d28`.
`savegame_write_thumbnail` copies the 60,702 bytes into the section. `savegame_alloc` creates the bitmap
(`bm_create(3, 201, 151, ...)`, handle `savegame_thumbnail_bm`, `0x59e794`) directly on the section memory, so
the Save/Load screen draws the buffer itself. `savegame_read_thumbnail_file` (`0x4d2910`) reads the first
`0x1f8 + 0xed1e` bytes of a file into the buffer for the preview and marks the texture dirty. Because the
thumbnail really starts at `0x200`, the last 8 bytes (4 pixels) are not reloaded for the preview (*observed*,
cosmetic).

## Level data: story flags, automap and variables (observed)

The section starts with 51 dwords, one per `level_script_table` record (`0x59f128`). Each dword counts how
many times that level has been entered: `state_game_enter` calls `savegame_inc_visit_count` (`0x4cfc80`) only
when the level was **not** entered by loading a save (`test bl, bl; jne` at `0x4384f0`). In the saves, `masad`
(index 3) has 1, and the two `lenele1d` saves also have 1 at indices 4 and 7. The conversation bits are read and
written only for a level whose count is above 0 (`savegame_get_speaker_bit` and the three other bit functions).

After the counts come 51 per-level blocks. Their sizes come from five fields of `level_script_info`
(`symbols/types.h`). The counts and hash lists are not in the image: `events_parse_table` (`0x4d39c0`) fills them
from the `$Level:` sections of `events.tbl` (`+Flag:`, `+Int:`, `+Float:`; the language section is chosen first).
The automap layer count comes from `level_parse_automap_table` (`0x4d4020`).

| `level_script_info` offset | Meaning | Used by |
|---|---|---|
| `+0x24` | number of named flags | `savegame_get_flag`, `savegame_set_flag` |
| `+0x28` | pointer to the flags' name hashes (`name_hash_nocase`: CRC-32 of the lower-cased name) | same |
| `+0x2c` | automap layers; each layer is 64x64 = 4096 bits | `savegame_get_bit`, `savegame_set_bit` |
| `+0x30` / `+0x34` | number of named int variables / their hashes | `savegame_get_int`, `savegame_set_int` |
| `+0x38` / `+0x3c` | number of named float variables / their hashes | `savegame_get_float`, `savegame_set_float` (`0x4d0130` / `0x4d01c0`; used by `ls_worldmap1_*`) |

Block layout (`savegame_data_offset`, `0x4cfbd0`, returns a bit offset):

| Part | Size |
|---|---|
| flag bits, one per named flag, LSB first | `flags` bits |
| automap reveal bits (`automap_reveal_cell`), bit `(layer*64 + y)*64 + x` | `layers x 4096` bits |
| padding to a whole byte | |
| int32 variables | `4 x ints` bytes |
| float variables | `4 x floats` bytes |

Block size = `(flags + layers*4096 + 7) / 8 + 4 * (ints + floats)`. A flag is addressed by (level index,
name). The name is hashed and looked up in that level's hash list, so the saved data stores no names. The
flag order is the order of the `+Flag:` lines of that level in `events.tbl`. A name that is not in the list is
ignored by `savegame_set_flag` (`0x4cfd90`). Setting a flag runs `quest_evaluate`; setting or clearing one
refreshes the object and character flags. Flags of other levels stay readable after you leave a level. `savegame_find_flag_level`
(`0x4d0240`) takes a name **hash**, searches the 51 lists in order, and returns the flag's value in the first
level whose list has it (0 if none); despite the name it does not return a level. Its only caller is
`quest_get_current_step`, which uses the result as a bool. Persistent values such as `chain_config_set`'s
`chainconfig%d%d` are stored as int variables.

`automap_reveal_cell` (`0x430110`) also sets the same bit in a twin level when both have the same layer count:
lenele1d (7) with lenele2d (49), lenele2aa (30) with lenele1aa (35), and lenele2ab (31) with lenele1ab (36)
(switch at `0x43016b`). `savegame_get_bit` and `savegame_set_bit` check the bit index against eight times the
block's automap bit count (`shl edi, 3` at `0x4cff44`), not the count itself. The callers check the layer but
not x and y.

## `INVT`: party inventory (observed)

| Offset | Type | Field |
|---|---|---|
| `+0x00` | char[4] | `INVT` |
| `+0x04` | int32 | `party_gold` (`0x971868`) (*observed* 2026-10-14: setting it to 5000 with `tools/savedit.mjs` showed 5000 gold in the inventory after loading) |
| `+0x08` | int32 | entry count n (clamped to 0..0x209 when written; not clamped on load) |
| `+0x0c` | n x 12 | `{uint32 item-type name hash, int32 count, int32 item field +0x08}` from the inventory list `0x970820` |
| then | int32 | chunk length (12 + 12n + 4) |

On load, `party_inventory_clear` runs, then `party_inventory_add_type(type, count, field, 1)` for each
entry. An unknown hash fails the load with `"Couldn't find item type!!!"`. The trailing length must match.
Two renamed items are mapped on load, in both `INVT` and `ITEM`: `Brown Dragon Scale` -> `Gray Dragon
Scale` and `Ring of Preservation` -> `Preservation Ring`, so older saves still load. The meaning of the
third field (0 or 1 in the saves) is *unknown*.

## Conversation state (observed)

There are 7 script slots for each of the 51 levels, described by `conversation_save_table` (`0x25388c0`,
0x10 bytes per slot, 0x70 per level: name pointer, speaker count *a*, topic count *b*). The slot name is a
level script name: `level_script_find_index` (`0x4d6340`) looks the header's script name up in the same table.
The layout stops at the first slot without a name. Slots whose name ends in `_m1` (multiplayer) get no
storage.

A slot's block is `4 + 8a + 4b + align8(a) + align8(4b)` bytes:

| Part | Size |
|---|---|
| "has data" dword, set by the first `savegame_init_conversation_slot` (`0x4d02d0`) | 4 |
| speaker name hashes | 4a |
| topic count of each speaker | 4a |
| topic name hashes, speaker by speaker | 4b |
| speaker bits, bit = speaker index (one kind) | align8(a) bytes reserved |
| topic bits, bit = 4 x topic index + kind | align8(4b) bytes reserved |

The bit areas reserve a byte for every bit they need, so most of their space stays zero.
`conversation_save_state` (`0x4d8390`) passes to `conversation_save_topics` (`0x4d83a0`), which writes, through
`savegame_set_speaker_bit` (`0x4d06e0`) and `savegame_set_topic_bit` (`0x4d09e0`):

| Bit | Meaning | Restored by `conversation_restore_state` (`0x4d84d0`) |
|---|---|---|
| speaker, kind 0 | speaker flag `0x08` (`CFLAG_IMPORTANT`) | `or byte [ebx+4], 8` / `and 0xfff7` |
| topic, kind 0 | topic flag bit 0 | bit 0 of the topic flags |
| topic, kind 1 | topic flag `0x08` (`initial`) | bit 3 |
| topic, kind 2 | topic flag `0x20` (`hidden`) | bit 5 |
| topic, kind 3 | this topic is the speaker's hail topic (speaker `+0x18`) | sets speaker `+0x18` |

For a speaker with `CFLAG_PERSISTENT` (`0x10`) the writer copies each bit to the level's other slots that
have data and are not `_m1`. `savegame_init_conversation_slot` copies the persistent speakers' bits from the
first other slot of the level that has data. The meaning of topic bit 0 is *unknown*.

## `ITEM`: item-type flags (observed)

| Offset | Type | Field |
|---|---|---|
| `+0x00` | char[4] | `ITEM` |
| `+0x04` | int32 | item type count N (521 in all saves); must equal the current `item_types` count (`0x8ec7c0`), or the load fails |
| `+0x08` | N x uint32 | name hash of each item type, in table order |
| then | (N+7)/8 bytes | bit i = bit 0 of the flags (`+0x5c`) of the type with hash i |

Flag bit 0 is the "identified" state of the item type (see [Shops and loot](shops-loot.md)), so the section
saves which item types the party has identified. On load each hash is looked up (`item_type_find_by_hash`), so
the bit goes to the right type even if the table order changed. Bit 2 of the same field is "cannot destroy"
(`item_can_destroy`).

## `LENT`: living entities and per-character state (observed)

| Offset | Type | Field |
|---|---|---|
| `+0x00` | char[4] | `LENT` |
| `+0x04` | int32 | object handle counter `obj_handle_next` (`0x59a5a0`, see `obj_handle_base_next`) |
| `+0x08` | int32 | `ai_goal_next_id` (`0x59b7c8`), a second counter |
| `+0x0c` | 0x24 bytes | party state (`savegame_write_party_state`, `0x4cf500`), below |
| `+0x30` | int32 | record count, at most 175 (0xaf); more fails the save |
| `+0x34` | records | one per entity: `uint32 name hash` + 0x17c bytes of `character_save_state`; for entities of type 7 also `float player+0x178` and `int32 skill_points` (player `+0x17c`) |
| then | groups | AI group table `ai_groups` (`0x243e040`, 10 x 0x54): count, then per group with members the leader's name hash, the member count - 1, the other members' hashes, and the dword at `+0x4c` |
| then | int32 | used chunk length, offset of this field + 4 (30,032 in `summoner7.sav`); must match on load (`"total_size = %d, offset = %d"`) |

Records are written in this order (`savegame_write_living`, `0x4d2150`):

1. every entity of `living_list` (`0x8eca60`, first `0x8ecd78`, next at `+0x318`);
2. every entity of `living_list_inactive` (`0x8ed1e8`, first `0x8ed500`) that does **not** have flag `0x100`
   in `+0x320`, or that has `0x40000` (count loop `0x4d21dc`..`0x4d21ef`, write loop `0x4d2274`..`0x4d2286`).
   `character_on_death` moves a dead character to this list (`0x45401b`..`0x454054`). Flag `0x100` marks the
   summon (`savegame_read_character` sets `summon_player` from it), so a dead summon is skipped unless it has
   `0x40000`;
3. every entry of `player_list_other` (`0x5fc030`), through `living_from_handle`.

Only type 7 adds the two extra dwords (`cmp [ebx+0xc], 7` at `0x4cf733` on save, `0x4cf84f` on load). A dead
party member has type 8 and gets no extra dwords.

On load, each record is matched *by name hash* to an entity in `living_list` that the level has already
created (`savegame_read_character`, `0x4cf790`). Only that list is searched (`0x4cf7ac`..`0x4cf7e2`). Records
without a match make the load fail, so a save only loads into the level it was made in. For a type-7 entity the
reader allocates a `Player` (`player_alloc_slot`, `player_init_from_character`), restores the two dwords and sets
player `+0x180` from the saved level (`xp_level_skill_cap`, `0x4cf890`..`0x4cf8a5`). After the records come
`ai_groups_reset` and the groups; an unknown leader or member name fails the load. Then party members with flag
`0x800000` are removed from the party, the length is checked, the party is sorted and the leader is set.

**Party state dword** (`+0x0c`). Example `0xfffffc01`: Joseph only, Joseph selected, no summon.

| Bits | Meaning |
|---|---|
| 0-3 | party members present, by species index (bit 0 = Joseph); on load each set bit creates a type-7 entity (`living_entity_create`, `0x4d2e48`..`0x4d2eb5`) |
| 4 | `combat_mode` (`0x5fccb4`) |
| 5 | `game_flags & 0x1000` (party hold; `party_toggle_hold`, `0x439620`, is called on load to set it again) |
| 6 | a summon was under control (`summon_player`, `0x5fcc84`, non-zero); the summon is then created as type 7, else type 5 |
| 7-9 | species index of the selected party member (signed; -1 = the summon is selected); the load fails if no party member matches |
| 10-31 | species index of the active summon (`cur_summon`, `0x5fcc8c`), -1 = none |

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

The type switch (`jmp [4*edx + 0x4587d7]` at `0x45739c`; `character_load_state` uses the table at `0x45a103`)
has a case for types 4, 5, 7 and 9. Types 6 and 8, the dead forms of 5 and 7, write only the common part
(type to effect slots) and the dead byte. Other non-player types (*observed in code, not decoded from a file*)
skip the equipment, timer and skill blocks. Some also save a timer, an int and a bool from the 0x550-byte
record `0x24f6748 + 0x550 * (+0x6dc)`.

## `CMRA`: camera (observed)

`CMRA` followed by 9 dwords from `camera_get_state` (`0x478890`) on `game_camera` (`0x60a018`). The read
(`savegame_read_camera`, `0x4d3100`) fails if `cur_character` is 0 or the tag is wrong. It then calls
`camera_reset_default`, `camera_set_state` (`0x4788e0`), `camera_set_target(cur_character)`,
`camera_set_mode(dword 7)`, stores dword 8 at camera `+0x04` and snaps the camera to its target. In the saves
the first dword is 35, the next six are floats (6.0, a yaw, 0.8, 2.0, 66.55, 3.3), and dwords 7 and 8 are 2 and 2.
The meanings of dwords 0..6 are *unknown*.

## `CONT`: containers and gold piles (observed)

| Part | Layout |
|---|---|
| header | `CONT`, int32 container count |
| per container | for each of the 40 records of `0x2302fe0` (0xe8 bytes each) that is in use (`+0x0c`), has items (`+0xd8`), and whose first item has `+0xa0 < 0` (not a level item): float x, y, z (`+0x10`), int32 item count, byte `lock` (`+0xe4` x 100, truncated by `_ftol` at `0x4d243a`), then per item `{int32 item-type index, int32 item_obj +0x9c}` |
| gold piles | int32 count (at most 32, `cmp ebp, 0x20` at `0x4d24e3`), then `{float x, y, z, int32 amount +0x9c}` for gold item objects (`obj_is_gold`) of `obj_active_list` (`0x970770`) that are not in a container (`+0xa4 == 0`) |

There is no trailing length. On load (`savegame_read_item_objects`, `0x4d31e0`), each container is created
with `container_obj_create`, its lock is the byte x 0.01, and each item is made with `item_create` and
`item_obj_spawn(item, pos, matrix, count = saved +0x9c, -1, -1)`, so container contents keep their counts.
The gold piles are **not** restored with their amount: after x, y and z the reader skips z and the amount
(`add esi, 8` at `0x4d33a7`) and spawns every pile with a count of 1 (`push 1` at `0x4d33c2`). `item_obj_spawn`
stores that argument at `+0x9c` (`0x46a619`..`0x46a622`), and `item_obj_pickup` adds `+0x9c` to `party_gold`
(`0x46b6ff`..`0x46b70d`). A loose pile comes from a monster whose only loot is gold (`monster_drop_loot`,
`0x452e38`..`0x452e48`). Reproduced in game, game bug G40; SummonerFix restores the saved amount
(`[Fixes] GoldPileAmount`). Piles beyond the 32nd are not saved at all. All eight saves have no containers
(count 0).

## Quest bits (observed)

200 bits (`savegame_get_quest_bit` / `savegame_set_quest_bit`, `+0x103c`). Bit n is quest n of the journal
(`journal_init`, `journal_draw_quest_list`). The layout function calls `quest_journal_set_entry(n, 0)` for
n = 0..199.

## Level items (observed)

An int32 count plus up to 512 (0x200) name hashes. At start-up the list is filled from `level_items.tbl`
(`#Level Items`, `$Name:`; `savegame_level_item_add` from `level_items_parse_table` and
`savegame_build_layout`); there are 177 entries in all eight saves. `item_obj_pickup` removes a level item
from the list (`savegame_level_item_remove`, `0x46b6ef`), and `level_items_parse_table` tests each item
against it (`savegame_level_item_has`, `0x4cb69c`) before it sets the item up (*inferred*: only items still in
the list are placed). So the list holds the unique level items that have not
been picked up yet.

`savegame_read_level_items` (`0x4d33f0`) walks `obj_active_list` and deletes every item object (type 3) with a
level-item index (`+0xa0 >= 0`) whose name hash is no longer in the list. If the object is in a container
(`+0xa4`), the reader empties and deletes that container instead (`0x4d3465`..`0x4d34a0`). It always succeeds.

## Two-stage objects (observed)

51 int32 counts, then 51 x 121 (0x79) name hashes, then 2 bits for each (level, entry) pair at byte
`0x6138`. An entry's hash is `name_hash_nocase(level_script_name + object name)`.
`savegame_two_stage_set` (`0x4d0e20`) and `savegame_two_stage_get` (`0x4d1110`) are called by
`character_use_skill`, `object_try_unlock_or_open`, the door code and `lc_entry_mark_used`. Bit 0 is "open"
(second stage). Bit 1 is "locked": `object_try_unlock_or_open` clears it with the key and shows "Unlocked!"
(`0x43c0eb`..`0x43c10b`). For a door that has no entry, `savegame_two_stage_get` reports bit 3 of the door's
`+0x54` as "locked".

`savegame_read_two_stage` (`0x4d34e0`) handles only the current level's entries. For each hash it walks the
doors (`doors`, `0xa5a304`, 0x8c bytes each). For every door it compares, the door's name plus
`level_script_name` must be shorter than 64 characters, or the load fails (`cmp ebx, 0x40; jae` at `0x4d3588`
and `0x4d35ac`). A matching door's mesh is moved to its open (`+0x28`, `+0x34`) or closed (`+0x0c`, `+0x18`)
position and orientation from bit 0. Otherwise `lc_find_by_hash` looks in `lc_entries` (`0x248ce40`, 50
records of 0x10c bytes, `"checking lc"`); if bit 0 is set, the entry gets flag `+0x104` bit 0 (`"disabled"`). A
hash that matches neither is only formatted into `string_buffer` (`"WARNING: two-stage mesh chksum %d doesn't
correspond..."`, `0x4d370c`), not printed; the load continues.

## Automap labels (observed)

An int32 count plus up to 128 (0x80) labels of 0x34 bytes:

| Offset | Type | Field |
|---|---|---|
| `+0x00` | int32 | level index (`cur_level_index`) |
| `+0x04` | int32 | script slot of the level (`level_script_find_index(level, level_script_name)`) |
| `+0x08` | float | x |
| `+0x0c` | float | y |
| `+0x10` | int32 | automap layer (`automap_layer`, `0x5b2cb8`) |
| `+0x14` | char[32] | text |

They are added by `savegame_automap_label_add` (`0x4d12b0`) from the automap label editor, which refuses a new
label at 128. Editing a label calls `savegame_automap_label_remove` (`0x4d1390`, matches level, slot, x and y)
and then the add. Deleting calls the remove. `cmd_reset_map_labels` calls `savegame_automap_labels_reset_level`
(`0x4d14f0`). Both the remove and the reset return at once while the list holds 128 labels (`0x4d13ca`,
`0x4d1526`).

`savegame_read_automap_labels` (`0x4d3740`) rebuilds the labels of the current level and script slot with
`automap_label_alloc`. It runs after a load and also on every level entry without a pending load
(`state_game_enter`, `0x438233`). It always succeeds. The live table holds 32 labels per level; see bug K6
on [Bugs in the original game](game-bugs.md).

## Hot buttons (observed)

5 characters x 6 buttons x 6 bytes = 180: `{byte used, byte flag, int32 id}`. `savegame_hot_button_set`
(`0x4d16a0`) clears the same id from the character's other buttons. Joseph had no hot buttons in any of the
saves. No load-time reader exists: the HUD (`0x40f340`) reads the buffer directly.

## Saving and loading

**Save** (`savegame_write`, *observed*): `conversation_save_state` runs first (from the caller), then the
header, thumbnail, `ITEM`, `INVT`, `LENT`, `CMRA` and `CONT` writers rebuild their sections. Seven calls
to a stub (`savegame_chunk_stub`, `0x4d25a0`, `return 1`; five more in `savegame_read`) mark where the
untagged sections would be written; those sections are already current in the buffer. Then
`file_make_path_dirs` runs, the file is opened with mode 2 and the whole used buffer (`+0x08` bytes) is
written in one `File__write`. Writers fail the save if a section would overflow its capacity (for example
more than 175 entities). Callers: `quicksave`, `saveload_save_to_slot`, `cmd_savegame`. Slots are named
`summoner%d.sav`, plus `quicksave.sav`.

**When saving is refused** (`savegame_location_allowed`, `0x4d4a50`, *observed*; callers `hud_key_save`,
`quicksave`, `playmenu_open_save`, `saveload_update_save_button`, `saveload_try_save`, `cmd_savegame`). It returns
0 when saving is allowed, otherwise a reason code:

| Code | When |
|---|---|
| 1 | `level_script_event_active` is set (a scripted boss or event), or a living entity of type 5 or 9 in AI mode 3 targets a party member |
| 3 | a summon is out (`cur_summon`) while in AI mode 6 or without bit 0 of its `+0x84` |
| 2 | a per-level rule, a jump table indexed by `cur_level_index` + 1 (`0x4d4b98`, targets `0x4d4b7c`): index 0 (no level) always; the first Khosani lab visit (level 14) until the story flag `room8` is set; Khosani lab 2 (level 27) once `pyrulstart` is set; the Jade Temple (level 34) once `got_laharah_spirit` is set; TempleINT2, the last temple (level 37), once `mach_intro` is set |

Code 2 showed "Cannot save in The Temple of Urath." in agent mode (2026-10-16). The per-level rule is what keeps
G130 and G131 out of reach ([Bugs](game-bugs.md)).

**Load** (*observed*): the header only is read first (`savegame_read_info`, `0x4d2800`, used by the Save/Load
list, `quickload` and `cmd_loadgame`). `savegame_set_pending_load` stores the filename. `game_change_level`
then switches to the saved level, and after the level has loaded, `state_game_enter` calls
`savegame_load_pending` (`0x438208`), which calls `savegame_read`. That function reads the file in three
`File__read` calls into the same offsets of the buffer: the header (`+0x10` = 0x1f8 bytes), which it checks
first, then the next 0xed1e bytes, then the rest up to the used size. The read results are not checked; only
`File__close` is. It then runs the steps below in order. The first failure prints its marker and aborts.

| Marker | Step |
|---|---|
| (none, returns -1 or 0) | `savegame_read_header`: tag `SAVg`; version == 0x27; the level name is in `level_script_table` and the script name is one of that level's slots (else -1); every one of the 13 section sizes equals the current layout (else 0, with `"i = %d, <name>_size = %d"`); the header-length field == 0x1ec. The script-table checksum is skipped (`add edx, 4` at `0x4cf4e3`), not read |
| `1a`, `3a`, `4a` | stubs (always succeed) |
| `5a` | `ITEM`: tag and item-type count; every hash must resolve |
| `6a` | `INVT`: tag, every hash must resolve, trailing length |
| `7a` | `LENT`: tag, at most 175 records, every record matches an existing entity, the groups resolve, a leader is found, trailing length |
| `8a` | `CMRA`: a current character and the tag |
| `9a` | `CONT`: tag; container and item creation must succeed |
| `10a` | stub |
| `11a` | level items (always succeeds) |
| `12a` | two-stage objects: fails when a compared door name plus `level_script_name` has 64 characters or more |
| `13a` | automap labels (always succeeds) |
| `14a` | stub |

When `savegame_read` fails, `state_game_enter` calls `gameseq_set_state(1)` (`0x438211`), the boot state that
opens the main menu. By then the buffer already holds the file's data.

Because the sizes are checked against the running game's tables, a save only loads into a game whose
`items.tbl`, `events.tbl`, automap table and conversation tables give the same section sizes. A modded table that
adds an item type or a flag makes all old saves fail the header check (*inferred*).

## Changes (2026-10-15)

- `LENT`: the second list is the list of dead characters, and its entities are written when they do **not**
  have flag `0x100`, or have `0x40000` (the page had the test reversed). Only type 7 has the two extra dwords.
  The reader searches only `living_list`. Party-state bits, counters and group details were added.
- Header: the checksum is skipped by `savegame_report_read_sizes` (`0x4cf4e3`), not read. Descriptions of 256
  characters or more fail the save.
- Load: three reads, header checked first; a full marker table (`1a`..`14a`); the two-stage step can fail
  (`12a`); a failed load returns to the main menu.
- `CONT`: the lock byte is truncated, not rounded. Loose gold piles are reloaded with amount 1, and at most 32
  are saved.
- Level data: flag, int and float names come from `events.tbl`, not from `Sum.exe`. The visit count only rises
  on entries that are not loads. `savegame_find_flag_level` returns a flag value, not a level. The twin-level
  map reveal and the eight-times bit bound were added.
- Conversation, `ITEM`, two-stage and level items: bit meanings now observed (identified items, `CFLAG_IMPORTANT`,
  `initial`, `hidden`, hail topic, door open / locked); the level-item reader also deletes containers.
- Automap labels: field 2 is the level's script slot and field 5 the automap layer (the page had "layer" in
  field 2). Labels are also rebuilt on every level entry. Remove and reset do nothing at 128 labels.
