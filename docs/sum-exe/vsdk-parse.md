---
title: "vsdk: parser (TextFile)"
parent: Sum.exe
nav_order: 5.5
---

# vsdk: parser (`TextFile`)

**Summary:** Phase 1, module 2. `vsdk\parse\parse.cpp` reads the game's text tables
(`.tbl`). Sum.exe has the same `TextFile` class as the
[Summoner 2 decomp](../reference/engine-references.md): 33 methods at
`0x515960`-`0x516bd0`, in the same order as Summoner 2's source. They are named in
`symbols/sum.symbols`, and the partial layout is in `symbols/types.h`.

## How the methods were identified

- **`Error`** (`0x515ee0`) builds "Parsing file "%s", at line #%d / Expected %s but found
  this text: [...] / Prior text was this: [...]" (**observed**).
- Each `Parse*` method passes its own expectation to `Error`, so the string identifies the
  method: "an integer" is `ParseInt`, "a uint" `ParseUInt`, "a vector" `ParseVector`, "a matrix"
  `ParseMatrix`, "a string" `ParseString`, "a string list" `ParseStringList`, "an integer list"
  `ParseIntList`, "a float list" `ParseFloatList`, and "a valid type string" `ParseTypeString`.
- The rest were placed by behaviour and position:
  - `Peek` / `Accept` / `Expect`: whitespace skip, then `_strnicmp` at the cursor; `Accept`
    consumes the match, `Expect` calls `Error` on failure.
  - `SkipTo` / `SkipPast`: `strstr` with a stop string; `SkipPast` = `SkipTo` + `Expect`.
  - `GetPosition` / `SetPosition`: copy `{line, cursor}`.
  - `Rewind`: line 1, cursor at the buffer start.
  - `Close`: frees the buffer.
  - `SkipBlockComment`: runs to `*/`, counting lines.

## Differences from Summoner 2

| PC-only method | Address | Notes |
|---|---|---|
| `ParseBool` | `0x516230` | "a boolean value" |
| `SkipLine` | `0x515f90` | Skips the rest of the line |
| `CopyUntil` | `0x515c80` | Copies text up to a `SkipTo` match |
| `ParseString_b` | `0x516630` | A second string parser (*inferred*: another destination type) |
| `SkipWhitespaceAndLine`, `ReadNextLine` | `0x516090`, `0x5160b0` | Whitespace skip + `SkipLine` / `ReadLine` |

Strings in the same block mention a flags-list parser ("a flags list", "a valid flag name")
that is not yet located. Writer helpers just before the class (`0x515500`-`0x515800`:
formatting ints and `< %.*f, %.*f, %.*f >` vectors) are not yet named.

## Layout (partial)

| Offset | Field |
|---|---|
| +0x00 | `buffer` |
| +0x04 | `cursor` |
| +0x48 | `line` |
| +0x4C | `size` |

## Uncertain

`Open` (`0x515d20`) is now confirmed (**V**): it loads the file into a common-block buffer, as Summoner 2 does ([memory](vsdk-os.md#mempools-and-the-common-block)). `SkipWhitespace` (`0x515a80`) and
`SkipSpaces` (`0x515a20`) may be the other way round. The one with comment handling should be
`SkipWhitespace`; this needs a closer read.
