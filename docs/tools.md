---
title: Analysis tools
nav_order: 3
---

# Analysis tools

Helper scripts live in the repository's `tools/` folder.

## `tools/rea-daemon.mjs`

A long-lived REA MCP session with no client-side timeout. **This is the required route for
`Sum.exe` inside the Claude desktop app**, which cuts off MCP calls at about 60 s no matter
what the timeout settings say (see [Tooling setup](setup.md#timeouts-on-large-binaries)).
It is also handy for scripted batches of queries.

- Spawns `rea mcp` from the global npm install.
- Watches a queue folder (default `.rea-queue/` in the repository root).
- For each `<id>.req.json` file containing `{"name": "<tool>", "arguments": {...}}`, it
  sends one `tools/call` and writes `<id>.res.json` with `isError`, `elapsedMs` and the
  result text.
- Processes one request at a time, so all calls share one Ghidra analysis.

```bash
node tools/rea-daemon.mjs
```

## `tools/rea-query.ps1`

Submits one or more calls to the daemon and prints the results with REA's repeated
boilerplate removed: evidence ID, result and any non-generic limitations.

```powershell
.\tools\rea-query.ps1 -Calls '[{"name":"open_binary","arguments":{"path":"C:\\...\\Summoner\\Sum.exe","provider_id":"ghidra"}},{"name":"procedure_pseudo_code","arguments":{"procedure":"0x501510"}}]'
```

| Parameter | Default | Meaning |
|---|---|---|
| `-Calls` | (required) | JSON array of `{name, arguments}` objects |
| `-Max` | 6000 | Maximum characters printed per result; the full result stays in the queue folder |
| `-TimeoutSec` | 900 | Overall wait limit |
| `-Queue` | `.rea-queue` | Queue folder, relative to the repository root |

## Typical session

1. Start the daemon in a background terminal.
2. Send `open_binary` for `Summoner\Sum.exe` with `provider_id: "ghidra"`.
3. The first analysis query takes about 100 s; after that, queries take about 1 s.
4. Send `close_binary` when finished, then stop the daemon.

## `tools/start-x32dbg.ps1`

Starts x32dbg, if it is not already running, and waits until its MCP server listens on
`127.0.0.1:9095`. With `-Game`, it also loads `Summoner\Sum.exe` into the debugger. See
[Live debugging](setup.md#live-debugging-x64dbg--mcp).

## `tools/build-summonerfix.ps1` and `tools/test-run.ps1`

Build SummonerFix (`-Install` copies it next to Sum.exe), and start the game for a timed
test with screenshots, dialog text and the SummonerFix log. See [SummonerFix](summonerfix/index.md).

## `tools/symbols.mjs`

Works with the symbol file `symbols/sum.symbols` (format described in its header).

| Command | What it does |
|---|---|
| `check` | Validates format, duplicates and sort order |
| `apply-x32dbg` | Labels every symbol in x32dbg (Sum.exe must be loaded; see `start-x32dbg.ps1 -Game`) |
| `apply-rea` | Not possible: REA's Windows Ghidra provider is read-only. Use `tools/decomp.mjs` |
| `fetch-functions` | Saves Ghidra's function list to `symbols/ghidra-functions.tsv` |
| `progress` | Named and unnamed functions per 64 KB of `.text` |

## `tools/tu-map.mjs`

Builds the [Source-file map](sum-exe/source-map.md) by scanning `.text` for references to the
source-path strings. No Ghidra needed. `--md <path>` also writes the Markdown table.

## `tools/lib/rea-queue.mjs`

Node client for the REA daemon queue (`reaCall(name, args)`), used by `symbols.mjs`. It avoids
the PowerShell argument-quoting problems of `rea-query.ps1`. Pass Windows paths with
backslashes: a forward-slash path is accepted by `open_binary` but doesn't bind the session.

## `tools/decomp.mjs`

Decompiles functions through the REA daemon and substitutes names from `symbols/`. That covers
functions, globals and fields of known structures (`DAT_025d8d34` becomes
`gr_screen.max_w`). It also lists the unnamed functions each one calls.

```bash
node tools/decomp.mjs FrameDone 0x5328d0
node tools/decomp.mjs --asm timer_get
node tools/decomp.mjs --callees GameTick
```

## `tools/rea-open.mjs`

Opens `Summoner\Sum.exe` in the running REA daemon session (with the backslash path it needs).
The first analysis query afterwards takes about 100 s.

## `tools/vpp.mjs`

Lists or extracts `.vpp` packfiles, using the format worked out in
[Packfiles](sum-exe/vsdk-packfile.md).

```
node tools/vpp.mjs list Summoner/tables.vpp [filter]
node tools/vpp.mjs extract Summoner/tables.vpp "*.tbl" [--out <dir>]
```

The filter is a case-insensitive substring, or a glob with `*` and `?`. Files are extracted
to `extracted/<packfile>/` by default. That folder is git-ignored: **extracted game data must
never be committed**.

Other tools:
- `tools/console-commands.mjs`, `tools/console-vars.mjs`, `tools/make-console-page.mjs`:
  the [console command](sum-exe/console.md) tables.
- `tools/name-from-strings.mjs`: functions that reference given strings.

## `tools/strings-tu.mjs`

Places game functions in their original source files. It uses the `// file.cpp` sections of
`strings.tbl`: any function that reads `localize_strings[id]` belongs to the file that owns
that id. Extract `strings.tbl` first with `tools/vpp.mjs`.

```
node tools/strings-tu.mjs            # function -> file(s)
node tools/strings-tu.mjs --md       # address ranges per file (used in the source map)
node tools/strings-tu.mjs --symbols  # unnamed functions as I-confidence stubs
```

## `tools/disasm.mjs`

Linear disassembly through REA's `inspect_native_instruction`. Use it for code Ghidra did not
turn into a function (game-state handlers, console callbacks), where `tools/decomp.mjs`
fails. It stops at the first `ret` past any forward branch. Call targets are labelled with
symbol names, and immediates that point at strings are printed.

```
node tools/disasm.mjs 0x431c60 [max_instructions]
```

REA makes one call per instruction, so a large function takes minutes. Prefer `tools/asm.mjs`.

## `tools/asm.mjs`

Fast local disassembly with `llvm-objdump`, which comes with LLVM-MinGW (already needed for the
SummonerFix build). The whole of Sum.exe takes about 2 seconds. The listing is cached in
`build/sum.asm` (git-ignored) and rebuilt when Sum.exe changes. Output is annotated with names from
`symbols/sum.symbols` and with the strings that immediates point at.

```
node tools/asm.mjs fn <addr|name> [max_lines]   # one function, up to the next known function start
node tools/asm.mjs grep <regex> [max]           # matching instructions in game code, with the containing function
node tools/asm.mjs callers <addr|name>          # call sites of a function
node tools/asm.mjs strings <lo> <hi> [--unnamed] # strings each function references (finds names quickly)
```

References to `localize_strings[id]` are shown as `L<id> "text"` when `extracted/tables/strings.tbl` has
been extracted (`node tools/vpp.mjs extract Summoner/tables.vpp strings.tbl`).

Limitation: llvm-objdump disassembles linearly, so right after a jump table embedded in the code it can be out of step
for a few instructions (seen just before `0x500990`). If a listing looks wrong there, check it against `tools/disasm.mjs`
or Ghidra.

For example, `node tools/asm.mjs grep "\+ 0x5f8\]"` lists every access to a character's current HP.

## `tools/agent.mjs` (agent mode)

Plays the game from scripts, through SummonerFix's agent mode (`[Debug] Agent=1`,
`src/summonerfix/agent.c`). The real mouse and keyboard are not used:
- key presses go into the game's `key_mark`, mouse buttons into `mouse_mark_button`;
- the cursor position replaces the real one in the `ScreenToClient` hook;
- screenshots come from the back buffer;
- the game is kept "in the foreground", so it does not pause.

```
node tools/agent.mjs start [--width 1366]    # set Agent=1 in the game's ini, launch, wait until ready
node tools/agent.mjs run "info; click left 858 368; wait 1500; key I; wait 1000; capture inv"
node tools/agent.mjs shot [name]             # screenshot -> Summoner\agent\name.png
node tools/agent.mjs stop                    # quit (kill after 15 s)
```

**Commands:**
- `key NAME [ms]`, `keydown NAME`, `keyup NAME` (straight into `key_mark`);
- `postkey NAME [ms]`: a real `WM_KEYDOWN`/`WM_KEYUP` posted to the game window, for window-message paths such as `[ConsoleKeys]`;
- `postmsg MSG WPARAM LPARAM` (hex): any window message, e.g. `postmsg 20b 10000 0` for side button 1 (`WM_XBUTTONDOWN`);
- `speedkey 1|0`: acts as if the speed key were held or released (the real key is read with `GetAsyncKeyState`, which agent mode cannot set);
- `move X Y`, `click [left|right|middle] [X Y] [ms]`, `down/up BUTTON`, `drag X1 Y1 X2 Y2`;
- `wait MS`, `capture NAME`;
- `info` (game state, screen, fps, cursor), `peek ADDR [N]`, `poke ADDR BYTE` (one byte of writable game
  data, hex; used to test the script editor flag), `quit`.

Coordinates are screenshot pixels, which are half the window size (1280x720 on a 2560x1440
screen). Key names are the ones `summonerfix.ini [KeyBindings]` uses.

**Useful positions** at 1280x720 screenshots: main menu Load Saved Game (858, 368), then
the load screen's Load (798, 368).

Set `Agent=0` again for normal play.

## `tools/savinfo.mjs`

Read-only savegame inspector, based on [Savegame format](sum-exe/savegame.md). It prints the header (level, description,
play time, section sizes) and the party members from the `LENT` chunk: level, experience, HP/AP, skill points and
equipped items. Item names are shown when `items.tbl` has been extracted. `--all` also lists the other entity records.

```
node tools/vpp.mjs extract Summoner/tables.vpp items.tbl
node tools/savinfo.mjs Summoner/savegame/summoner7.sav
```

## `tools/savedit.mjs`

Savegame editor. It **never overwrites a file**: it writes a new output file, refuses if that file exists or is the
input, and leaves the original untouched. It can change the description, the party's gold, and a party member's
level, experience, HP/max HP, AP/max AP and skill points. The game does not recompute anything on load, so raising
the level does not add the HP, AP or skill points of the skipped levels. Tested 2026-10-14: an edited copy
(5000 gold, 10 skill points) loaded and showed both values.

```
node tools/savedit.mjs Summoner/savegame/summoner7.sav edited.sav --gold 5000 --skillpoints 10 --desc "edited"
```

To use the result, copy it into `Summoner/savegame` under an unused name such as `summoner15.sav`.

## `tools/tu-split.mjs`

Proposes translation-unit (source file) boundaries for the whole of `.text` from data locality: which `.bss`
globals each function uses, plus the `.cpp` module tags. The method and its limits are on the
[Source-file map](sum-exe/source-map.md#translation-units-from-data-locality) page. `--score` reports units that
mix two files, files split over several units, and the tags that look stray.

```
node tools/tu-split.mjs --out units.txt
node tools/tu-split.mjs --score
```

## `tools/tu-split2.mjs`

The better unit splitter: it finds file boundaries from data, calls, initialiser groups and code style, without
needing module tags (98% of known boundaries found blind). `--score` measures it against the `.cpp` tags, `--blind`
ignores the tags, `--loo` hides one file at a time, `--eval` prints every measurement. See the
[Source-file map](sum-exe/source-map.md#version-2-boundaries-from-the-binary-alone-2026-10-15).

```
node tools/tu-split2.mjs --out units.txt
node tools/tu-split2.mjs --score
```

## `tools/addsym.mjs`

Merges proposed symbols (`0xADDR|name|kind|module|conf|comment` lines) into `symbols/sum.symbols` in address
order. Lines with a known address or name, or a bad field, are skipped and listed. `--dry-run` only reports.

```
node tools/addsym.mjs proposals.txt --dry-run
```

## `tools/tbl-fields.mjs`

Pairs each `$Field:` / `+Field:` string in a `.tbl` parser with the offset its value is stored at. It works
on a listing from `tools/asm.mjs`. See [Game data tables](sum-exe/game-tables.md) for the method and its limits.

```
node tools/asm.mjs fn spells_parse_table 6000 > spells.txt
node tools/tbl-fields.mjs spells.txt
```
