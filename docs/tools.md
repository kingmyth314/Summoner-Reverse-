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
