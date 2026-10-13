---
title: "Localization"
parent: Sum.exe
nav_order: 5.8
---

# Localization (`0x503fb0`-`0x505540`)

**Summary:** Phase 1, module 4. This code sits between `bmpman` and `gr` and holds all
translated text. 28 functions and their tables are named in `symbols/sum.symbols` (module
`localize`, confidence **O**/**I**). There is no source-file string, so the file name is
unknown. The block lies in the `vsdk` address range, but it hard-codes Summoner character
names, so it may be game code linked there (*inferred*).

## Languages

`localize_set_language` (`0x5050b0`) prints "Setting language to %s" and sets
`localize_language` (`0x25d8878`): **0 English, 1 German, 2 French**
(`localize_language_names`, `0x5a5c38`).

`localize_remap_charset` (`0x5051a0`) converts accented characters between Windows-1252 and
the font's own code points. For German these are the old DOS (CP437) positions: for
example, `ä` 0xE4 becomes 0x84 and `ü` 0xFC becomes 0x81 (**observed** in the switch tables).
This matters for any future font or translation work.

## Two kinds of text

**Numbered UI strings.**
- `localize_load_strings` (`0x504230`) reads `strings.tbl` from `#<language>` to `#end`.
  Each line is `<id>, {text}`.
- The strings go into `localize_strings` (`0x25cd4dc`), an array of 1,000 `char*`. This is
  the same design as Red Faction's `rf::strings::array` in
  [Dash Faction](../reference/engine-references.md).
- `localize_unescape` (`0x503fe0`) handles `\n`, `\r`, `\"` and `\xHH`. Unused ids point
  at a "NULL Format String" entry.

**Name translation tables.**
- Names are looked up by their English text: `localize_hash` (`0x504200`, `rol(h, 6) ^ c`,
  case-sensitive) indexes chained hash tables of 253 buckets.
- `localize_table_lookup` (`0x5041b0`) returns the English text unchanged when no
  translation is listed.

| Table | Address | Source | Lookup |
|---|---|---|---|
| Items | `0x25d847c` | `item_names.tbl` | `localize_item_name` (`0x505040`) |
| Quests | `0x25c70cc` | `quest_names.tbl` | `localize_quest_name` (`0x505070`) |
| Topics | `0x25c68e4` | per level, `topic_names` section | `localize_topic_name` (`0x505020`) |
| Script names | `0x25c6cd8` | per level, `script_names` section | `localize_script_name` (`0x505090`) |

`localize_load_level_table` (`0x504870`) reads the per-level tables. It also adds a few
fixed pairs if they are missing: "Farewell." for topics, and character and summon names
(Joseph, Jekhar, Flece, Rosalind, Golem, Red Minotaur, Blade of Urath, the Elementals,
Wraith, Jade Golem, Spirit Of Laharah, Celestial Samurai) for scripts. A missing section
prints "!!!Not found!!! section: %s in file: %s".

## Memory

`localize_init` (`0x504f60`) creates two [mempools](vsdk-os.md#mempools-and-the-common-block)
named "loc  non level" (0x3800 bytes, at `0x25c68b0`) and "loc  level" (0x2800 bytes, at
`0x25c74c0`). Level text is dropped by `localize_clear_level` (`0x504ed0`) when the level
changes and reloaded by `localize_level_init` (`0x504f10`).

## Neighbours named at the same time

- `progress_set_callback` / `progress_update` (`0x503dd0` / `0x503e80`): a loading-progress
  callback called at most every 100 ms. It can report a key press to abort (*inferred*).
- `TextFile::SkipToNextLine` (`0x515af0`) and `Mempool::ResetOffset` (`0x505540`).
- C runtime: `printf` (`0x56da2b`), `strtol` (`0x56ed51`), `_chkstk` (`0x56d7c0`).
