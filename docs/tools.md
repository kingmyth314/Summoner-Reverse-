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
