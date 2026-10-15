---
title: "Localization"
parent: Sum.exe
nav_order: 5.8
---

# Localization (`0x503f60`-`0x505547`)

**Summary:** Phase 1, module 4. This code sits between `bmpman` and `gr` and holds all
translated text. 29 functions in module `localize`, five static-initialiser helpers (module `-`) and
their tables are named in `symbols/sum.symbols` (confidence **O**). There is no source-file string, so
the file name is unknown; the decompiled unit is `src/decomp/vsdk/unnamed_localize_0x503f60.cpp`
(placeholder name). The block lies in the `vsdk` address range, but it hard-codes Summoner character
names, so it may be game code linked there (*inferred*). v1.40 always runs in English: the only call
of `localize_set_language` passes 0 (**observed**).

{: .important }
Corrected 2026-10-15 against the disassembly: the range starts at the static initialisers (`0x503f60`),
the pool names are "loc: non-level" and "loc: level", and the per-level tables come from sections of
`topic_names.tbl` / `script_names.tbl` (not from `topic_names` / `script_names` sections of the level files).

## Languages

`localize_set_language` (`0x5050b0`) prints "Setting language to %s" and sets
`localize_language` (`0x25d8878`): **0 English, 1 German, 2 French**
(`localize_language_names`, `0x5a5c38`: three 0x42-byte entries, the name at +0 and a code at +0x21,
"" / "gr" / "fr"). It also sets one flag per language (`localize_french_punct` `0x25d8870`, read by the font
code; a German flag `0x25d8874` and an English flag `0x5a5d3c` that nothing reads). Its only caller is
`localize_init` (`push esi` with `esi = 0` at `0x504f64`), and nothing else writes `localize_language`, so
v1.40 always loads the `#english` sections; the German and French text in the tables is never read
(**observed**).

`localize_remap_charset` (`0x5051a0`) converts accented characters between Windows-1252 and
the font's own code points. For German these are the old DOS (CP437) positions: for
example, `ä` 0xE4 becomes 0x84 and `ü` 0xFC becomes 0x81 (**observed** in the switch tables).
`ß` 0xDF becomes 0xBC, although the reverse table maps 0xE1 (CP437's `ß`) back to 0xDF. French has
one table, used whatever the direction argument says: accented capitals mostly become the lower-case
glyph (`É` 0xC9 and `é` 0xE9 both become 0x82) or 0xFF, and `Ä` 0xC4 becomes 0x90 (CP437 `É`). All 11
callers pass direction 1 (to the font), so the German font-to-Windows table is never used either.
This matters for any future font or translation work.

## Two kinds of text

**Numbered UI strings.**
- `localize_load_strings` (`0x504230`) reads `strings.tbl` from `#<language>` to `#end`.
  Each line is `<id>, {text}`.
- The strings go into `localize_strings` (`0x25cd4dc`), an array of 1,000 `char*`. This is
  the same design as Red Faction's `rf::strings::array` in
  [Dash Faction](../reference/engine-references.md).
- `localize_unescape` (`0x503fe0`) handles `\n`, `\r`, `\"` and `\xHH`. The id after the last
  line gets a "NULL Format String" entry, and every id after it up to 999 points at that entry; ids
  skipped below the last line stay NULL.
- The text itself is copied into one fixed 40 KB buffer (`0x25ce47c`-`0x25d847b`, write pointer
  `localize_pool_used` `0x5a5c30`), not into a mempool. The retail English section needs about 24.7 KB.
  Ids 935, 936, 942 and 944 skip the charset remap.

**Name translation tables.**
- Names are looked up by their English text: `localize_hash` (`0x504200`, `rol(h, 6) ^ c`,
  case-sensitive) indexes chained hash tables of 253 buckets.
- `localize_table_lookup` (`0x5041b0`) returns the English text unchanged when no
  translation is listed.

| Table | Address | Source | Lookup |
|---|---|---|---|
| Items | `0x25d847c` | `item_names.tbl` | `localize_item_name` (`0x505040`) |
| Quests | `0x25c70cc` | `quest_names.tbl` | `localize_quest_name` (`0x505070`) |
| Topics | `0x25c68e4` | `topic_names.tbl`, per level section `<level>_dlg.tbl` | `localize_topic_name` (`0x505020`) |
| Script names | `0x25c6cd8` | `script_names.tbl`, per level section `<level>_script.tbl` | `localize_script_name` (`0x505090`) |

`localize_load_level_table` (`0x504870`) reads the per-level tables. It looks for the line
`#Start File: "<level>_dlg.tbl"` (topics, text in `{}`) or `"<level>_script.tbl"` (script names, text in
quotes) after `#english` and reads up to `#Stop File`; then it finds the same section after
`#<language>` and pairs the n-th translated line with the n-th English line (the ids are parsed and
ignored). Translated topics are cut at a `#`. The item and quest tables pair by id instead
(`localize_load_names`, `0x504520`, `#english` to `#End`, then `#<language>` to `#end`). The lookup
compares only the 32-bit hash, not the text; the retail tables have no collisions (checked for every
table and level section). It also adds a few
fixed pairs if they are missing: "Farewell." for topics, and character and summon names
(Joseph, Jekhar, Flece, Rosalind, Golem, Red Minotaur, Blade of Urath, the Elementals,
Wraith, Jade Golem, Spirit Of Laharah, Celestial Samurai) for scripts. A missing section
prints "!!!Not found!!! section: %s in file: %s".

## Memory

`localize_init` (`0x504f60`) creates two [mempools](vsdk-os.md#mempools-and-the-common-block)
named "loc: non-level" (pool `0x25c68b0`, 0x3800-byte buffer at `0x25c9cdc`) and "loc: level" (pool
`0x25c74c0`, 0x2800-byte buffer at `0x25c74dc`), once (flag `0x25d887c`). The non-level pool holds the
item and quest table nodes; their strings go into a separate 0xa800-byte `StringPool` (`0x25c68d0`).
The level pool holds both the nodes and the strings of the topic and script tables (16-byte aligned
allocations; the largest retail level, `khosanistrng_v6`, needs about 7 KB of the 10 KB).
`localize_level_init` (`0x504f10`) calls `localize_clear_level` (`0x504ed0`), which resets the level
pool and the two level tables, and then loads the new level's text.

## Neighbours named at the same time

- `progress_set_callback` / `progress_update` (`0x503dd0` / `0x503e80`): a loading-progress
  callback called at most every 100 ms. It can report a key press to abort (*inferred*).
- `TextFile::SkipToNextLine` (`0x515af0`) and `Mempool::ResetOffset` (`0x505540`).
- C runtime: `printf` (`0x56da2b`), `strtol` (`0x56ed51`), `_chkstk` (`0x56d7c0`).
