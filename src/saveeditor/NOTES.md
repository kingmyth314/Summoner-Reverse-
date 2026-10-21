# saveedit: the savegame editor engine in C

A port of the engine of `tools/savedit.mjs` (with `tools/lib/savedata.mjs` and the packfile reader of
`tools/vpp.mjs`) to C, so that a Windows app can read and edit Summoner v1.40 saves without Node. Format:
`docs/sum-exe/savegame.md`.

| File | What |
|---|---|
| `src/saveedit.h`, `src/saveedit.c` | the library |
| `src/saveedit_cli.c` | console program with the command line of `savedit.mjs`, plus `--roundtrip` |
| `tools/build-saveeditor.ps1` | builds `build/saveeditor/saveedit.exe` with LLVM-MinGW (`i686-w64-mingw32-gcc -O2 -Wall -Wextra -static -municode`) |
| `tools/saveedit-test.mjs` | the acceptance tests (round trip and differential against the Node engine) |

## Command line

```
saveedit <in.sav> <out.sav> [edits...] [--tables FILE.vpp] [--tables-dir DIR] [--items-tbl FILE] [--events-tbl FILE] [--lang NAME]
saveedit --roundtrip <in.sav> <out.sav>
```

The edits and their rules are those of `savedit.mjs`: `--desc`, `--gold`, `--add-item Name[=N]`,
`--remove-item Name[=N]`, `--set-item-count Name=N`, `--flag LEVEL:NAME=0|1`, `--member I|NAME`, `--level`,
`--xp`, `--hp`, `--maxhp`, `--ap`, `--maxap`, `--skillpoints`, `--skill ID=VALUE`. The output must not exist
and must be another file than the input; every edit is checked before anything is written. Exit code 0 when
the file was written, 1 when refused. `--roundtrip` loads and checks the save, then writes it with no edit.

## Library API (`saveedit.h`)

Conventions: functions that can fail return 0 or -1 and fill `err`/`errlen` with a message. Strings from or
for the save and the tables are Latin-1 byte strings (`char *`), as in the Node tools; file paths are
`wchar_t *` (Windows).

Names and hashes
- `se_name_hash(s)`, `se_name_hash_n(s, n)`: `name_hash_nocase`, Crc32 (table polynomial 0xedb88320, start 0, no
  final inversion) of the lower-cased name.
- `se_level_names[51]`, `se_section_names[13]`, `se_find_level(name)` (name, case-insensitive, or index).

Tables
- `se_table_opts {tables_vpp, tables_dir, items_tbl, events_tbl, lang}`: sources, first match wins per file
  (explicit file, folder of extracted `.tbl` files, packfile), as `loadTables`. In the app `tables_vpp` is the
  game folder's `tables.vpp`, next to `Sum.exe`.
- `se_tables_load(&t, &opts)` never fails; what is missing is in `t.warnings`. `se_tables_free`.
- `se_tables`: `items` (hash -> name, `have_items`), `levels[51]` (`se_level_def`: flag, int and float names in
  `events.tbl` order, automap `layers`), `quests` (`se_quest`: index, name, main, `+Invalidate:` flag),
  `have_events`, `stopped`, `nmissing`. `se_item_name_of(t, hash)`.

Saves
- `se_save {data, size}`; `se_load_file(&s, path)`, `se_load_mem(&s, data, size)` (both check the save),
  `se_free`; `se_check(data, size, msg, n)` (NULL = fine); `se_write_new(&s, path)` (creates the file with
  `CREATE_NEW`, so an existing file is never overwritten; a partial file it created is deleted on a write error).
- `se_section_offsets(&s, &sections)`, `se_chunk_at(&s, "INVT")`.
- `se_read_header` -> `se_header` (version, level, script, level time, description, section sizes).
- `se_read_inventory` -> `se_inventory` (gold, entries `{hash, count, field}`, `length_ok`); `se_inventory_free`.
- `se_item_types` -> the ITEM chunk's hashes (malloc'd).
- `se_read_entities` -> `se_entities` (LENT records); `se_party(&s, out, max)` -> `se_member` (record, offset,
  hash, name, level, xp, hp, maxhp, ap, maxap, skill points, dead, skills[36], equip[9]).
- `se_level_blocks(&s, &t, blocks[51])` -> `se_level_block` (offset, size, ints_at, visits, level), refused unless
  the tables reproduce the save's level-data size; `se_get_flag`, `se_level_int`, `se_level_float`, `se_quest_bit`.

Edits
- `se_op {kind, value}` with `se_op_kind` = `SE_OP_DESC` ... `SE_OP_SKILL`; `value` is the text the command
  line would carry. `se_op_kind_from_name("add-item")`, `se_op_name`.
- `se_apply(&s, &t, ops, n, log, loglen, err, errlen)`: applies the list in order on a working copy and copies
  it back only if every edit succeeded (all or nothing). `log` gets one line per change. Checks the same rules
  as `savedit.mjs`: at least one edit besides `--member`; `--desc` and `--gold` once; each member field and
  each skill id once per member.

## Design choices

- One engine, text values: the edits take the same strings as the command line, so the CLI is a thin parser and
  the app can build the same list from its controls; the validation is the same code for both.
- Copy-on-apply: `se_apply` never leaves a half-edited save behind; a failed edit leaves `s` unchanged.
- Bounds: where Node's `Buffer.read*/write*` would throw on a damaged save, the C code checks the offset and
  refuses (Node crashes with a stack trace; both write nothing).
- JavaScript semantics kept where they decide bytes: `toLowerCase` and `trim` on Latin-1, the regular
  expressions of the option values (item `Name=N`, `LEVEL:NAME=0|1`, `ID=VALUE`, whole numbers, decimals
  without exponent), `Number()` of the automap layer numbers and `>>> 3` in the block size, `Map` semantics
  (a later duplicate name in `items.tbl` or the packfile directory replaces the earlier one), quest index 200
  after the 200th quest, Buffer indexing outside the buffer for the quest bits (read 0, write ignored), double
  to float rounding for HP.
- No game data in the code: names come from the tables at run time; only the level-name table of
  `level_script_table` and the four party names are built in, as in `savedata.mjs`.
- `se_apply` keeps a small static buffer for item labels, so the library is not thread-safe.

## Tests (`tools/saveedit-test.mjs`)

Run from the repository folder: `node tools/saveedit-test.mjs --corpus DIR` (DIR: copies of saves; options: `--saves N` for the first N saves,
`--only TEXT` for the cases whose name contains TEXT). Every run writes into a new `run-<time>/` (under `--out`, default the system temp folder) folder
and `results.txt` there; a test output is deleted by its exact path once its case passed. Both tools get
`--tables` with the repository's `Summoner/tables.vpp`.

a. Round trip: `saveedit --roundtrip` on each corpus save, output compared byte for byte with the input.

b. Differential: per save, each case runs with `node tools/savedit.mjs` and with `saveedit.exe` on the same
input. An accepted case passes when both write and the two outputs are byte-identical; a refused case when
both exit with an error and neither writes anything (for "output exists" a placeholder must be unchanged; for
"same file" a private copy given as input and output must be unchanged). Item, flag and member names are taken
from each save at run time (inventory, ITEM chunk, level blocks, party). Cases:

- `--desc`: ASCII, Latin-1 accents, characters outside Latin-1 (stored as the low byte, as Node does), 255
  characters, empty; refused: 256 characters, `--desc` twice.
- `--gold`: 5000, int32 max and min; refused: 2147483648, `12x`.
- Items (a held item of the save): `--add-item` plain, `=3`, upper case, padded with spaces and lower case;
  `--remove-item` all, `=1` on an item held at least twice; `--set-item-count` 7 and 0; an item type of the
  save that is not held: `--set-item-count =0` (still rewrites INVT) and `=2`; refused: `=0` for add,
  more than held, set without a count, remove of a not-held item, an unknown name.
- `--flag`: by level name, by index and by lower-case level name, on; by name and by index, off; a set flag
  set again; a flag that is the `+Invalidate:` flag of a quest active in the save (journal bit cleared), where
  a save has one; refused: a flag of no level, value 2, unknown level, index 51.
- Members: every numeric field on member 0 (`--member 0`), every field on the default member with negative,
  boundary and `-0` values, `--hp` 12.5, a long decimal and a 60-digit number (float rounding), `--level 32767`,
  `--skill` single and bounds (35=255, 0=0); `--member` by name (and lower case) and name then index in one run;
  refused: `--hp 1e3`, `--level 32768`, a field twice, `--skill 36=1`, `0=256`, a skill twice, a member index
  past the party, an unknown name, `--member` alone (no edits).
- Combined: two edits; seven edits of all kinds; refused: unknown option, missing value, output exists, same
  file as input.
- Table options (the `.tbl` files are extracted into the run folder at the start and deleted at the end):
  `--lang ENGLISH`; `--lang` with no such section (gold accepted, flag refused); `--events-tbl` naming a missing
  file (flag refused); `--tables-dir` (flag, add-item); `--items-tbl` with `--events-tbl` (flag).

Last result (41 saves): round trip 41/41; differential 71 cases, 2813 runs, all pass (a case runs only on
the saves where it applies, e.g. the quest-invalidating flag on 3 saves).

## Differences from savedit.mjs

- Default tables: without `--tables`, Node reads `Summoner/tables.vpp` of the repository; the C program reads
  `tables.vpp` next to `saveedit.exe`, else `Summoner\tables.vpp` under the current folder.
- Messages are worded like Node's but are not identical (for example the unknown-level message does not list
  the levels); the log of changes has the same lines.
- Characters outside Latin-1 in names: Node lower-cases the UTF-16 string before taking the low byte for the
  hash; the C CLI takes the low byte first, so such a name hashes differently (only names that cannot be in
  the Latin-1 tables are affected, and Node would refuse them too unless the low bytes happen to match).
  `trim` of non-Latin-1 white space (U+FEFF, U+2000...) likewise.
- Same-file check: Node compares the resolved paths as strings (case-sensitive); the C program compares full
  paths without regard to case. Either way an existing output is refused.
- A table file name in `--tables-dir` is matched with Windows' ordinal case-insensitive compare instead of
  JavaScript `toLowerCase` (the names are ASCII).
- A damaged save that makes Node throw (an INVT or ITEM count past the end of the file, a party record cut off
  before its skill points) is refused with a message instead of a stack trace.
- `--roundtrip` is new.
