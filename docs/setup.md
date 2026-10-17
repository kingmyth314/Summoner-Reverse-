---
title: Tooling setup
nav_order: 2
---

# Tooling setup
{: .no_toc }

1. TOC
{:toc}

Setup performed on 2026-10-12 on Windows 11 Pro (10.0.26100), x64.

**Added 2026-10-16.** The repository has a private GitHub remote (`origin`, pushed only on the owner's request).
The real compiler, Visual C++ 6.0 Professional (cl 12.00.8168), is used from an extracted copy outside the
repository, never installed or committed; `tools/vc6-compare.mjs` needs its `VC98` folder in `VC6_DIR` (see
[Tools](tools.md) and the toolchain notes in the [decompilation plan](decomp-plan.md)). Standard Edition has no
optimiser and cannot be used for the optimised code. Since 2026-10-16 that folder has Visual Studio 6.0 Service
Pack 5's compiler files in `BIN`, which give Sum.exe's build 8966: from the SP5 package (`vs6sp5.exe`, extracted with
7-Zip, its multi-volume `VS6sp5*.cab` set too, never run), copy `vc98\bin\cl.exe`, `c1.dll`, `c1xx.dll`, `link.exe`
and the root file `msvcep.dll` renamed to `c2.dll` over the Professional Edition's `VC98\BIN`.

## Components

| Component | Version | Install method | Location |
|---|---|---|---|
| Node.js | 24.20.0 (npm 11.19.0) | `winget install OpenJS.NodeJS.LTS` | `C:\Program Files\nodejs` |
| REA (`rea-agents`) | 6.4.0 | `npm install -g rea-agents@6.4.0` | `%APPDATA%\npm\node_modules\rea-agents` |
| Eclipse Temurin JDK | 21.0.12 | `winget install EclipseAdoptium.Temurin.21.JDK` | `C:\Program Files\Eclipse Adoptium\jdk-21.0.12.101-hotspot` |
| Ghidra | 12.1.4 | Official release zip from NSA's GitHub | `%LOCALAPPDATA%\Programs\Ghidra\ghidra_12.1.4_PUBLIC` |

The Ghidra archive `ghidra_12.1.4_PUBLIC_20260921.zip` was verified against the published
SHA-256 `ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db` before extraction.

REA was installed globally rather than run through `npx`, so that MCP registrations point
at a stable path instead of the npm cache.

### Live debugging: x64dbg + MCP

Added 2026-10-12 for real-time inspection of the running game.

| Component | Version | Source | Location |
|---|---|---|---|
| x64dbg (includes 32-bit x32dbg) | snapshot 2026-05-27 (`snapshot_2026-05-27_12-11.zip`, 30.1 MB) | [x64dbg/x64dbg releases](https://github.com/x64dbg/x64dbg/releases), SHA-256 verified against GitHub's digest | `%LOCALAPPDATA%\Programs\x64dbg` (portable, 85 MB) |
| x64dbg-MCP Server plugin | v1.6 (`x64dbg-MCP-Server-v1.6.zip`, 1.4 MB, MIT) | [duty1g/x64dbg-mcp-server](https://github.com/duty1g/x64dbg-mcp-server), SHA-256 `3165390c…868bde` verified | `x32\plugins\x64dbg-MCP-Server.dp32`, `x64\plugins\x64dbg-MCP-Server.dp64` |

Sum.exe is 32-bit, so **x32dbg** is the debugger to use. The plugin starts an MCP server
inside x32dbg using Streamable HTTP with a required bearer token. Its config is
`x32\mcp_config.json`: `IpAddress` **`127.0.0.1`** (local only), `Port` **9095**,
`AutoStart` true. The token was regenerated after the first run.

Claude Code registration (user scope, `%USERPROFILE%\.claude.json`):

```json
"x32dbg": { "type": "http", "url": "http://127.0.0.1:9095/",
            "headers": { "Authorization": "Bearer <token from mcp_config.json>" } }
```

{: .important }
The MCP server only exists while **x32dbg is running**. Start it before Claude Code with
`tools/start-x32dbg.ps1` (add `-Game` to also load Sum.exe). If the token is changed in the
plugin's config dialog, update the header in `.claude.json` to match.

The plugin offers 80 tools: load/attach, run, step, breakpoints (software, hardware, memory,
conditional, with commands), registers, call stack, memory read/write, disassembly, pattern
and string search, module imports/exports, and more.

**Verified 2026-10-12** (via a raw HTTP MCP client, before restarting Claude Code): loaded
Sum.exe, set a breakpoint at `FrameDone` (`0x501510`) and ran to it. The game's live state
matched the static analysis: `gr::Screen` 1024x768, `mode` `0x68` (OpenGL), `window_mode`
200; `reported_frametime` clamped to 0.25 s on a 0.65 s loading frame; `frametime_divisor` 1.0.

Practical notes:

- x32dbg by default also stops at the system breakpoint and on DLL entry. Expect several
  pauses (system DLLs, the graphics driver) before a breakpoint in Sum.exe is reached.
- Breaking inside the game freezes its fullscreen window. Prefer breakpoints that log and
  auto-resume (`SetBreakpointCommand` + `SetBreakpointFastResume`) for anything in the
  per-frame path.
- Debugging runs the same `dinput.dll` (SummonerFix), so the game behaves as it does normally.
- Most debugger calls return at once, but `run`, `WaitForPause` and `RunToAddress` wait for the
  game. In the Claude desktop app keep their `timeoutMs` below 60000, because calls over
  about 60 s are cut off (see [Timeouts](#timeouts-on-large-binaries)).

## Environment variables

| Variable | Scope | Value |
|---|---|---|
| `GHIDRA_INSTALL_DIR` | User | `%LOCALAPPDATA%\Programs\Ghidra\ghidra_12.1.4_PUBLIC` |
| `JAVA_HOME` | Machine (set by the JDK installer) | `C:\Program Files\Eclipse Adoptium\jdk-21.0.12.101-hotspot` |

## Agent integration

```bash
rea setup --client claude_code --client claude_desktop --yes
```

This registered REA as an MCP server in:

- Claude Code: `%USERPROFILE%\.claude.json`
- Claude Desktop: `%APPDATA%\Claude\claude_desktop_config.json`

Both entries include `GHIDRA_INSTALL_DIR` and `JAVA_HOME` in their `env` block, so the
server does not depend on the GUI app inheriting new environment variables. It also
installed the `reverse-engineer-anything` skill to `~\.claude\skills\` and `~\.agents\skills\`.

{: .note }
Claude Desktop on this machine is the Microsoft Store (MSIX) build. Its `%APPDATA%\Claude`
folder is virtualised into
`%LOCALAPPDATA%\Packages\Claude_pzs8sxrjxfjjc\LocalCache\Roaming\Claude`; the two paths
were confirmed to hold the same file.

Backups of the original client configs are kept as `*.pre-rea.backup` next to each file.

## Verification

```bash
rea doctor --provider ghidra --client claude_code --client claude_desktop --skill
```

Result: `healthy: true`, with all checks passing: Node, Ghidra 12.1.4 install, headless
analyzer, native decompiler, Java 21, both client registrations and the skill identity.
A live end-to-end test opened `whoami.exe` with `provider_id: ghidra` and decompiled
`entry` successfully from both Claude Code and a Desktop-equivalent launch.

## Timeouts on large binaries

Ghidra's initial auto-analysis of `Sum.exe` (1.7 MB, 6,612 functions) takes about
**100 seconds**. Before the change below, Claude Code's MCP client gave up first
(`Request timed out`). REA discards the ephemeral analysis when a request is cancelled,
so direct MCP calls on `Sum.exe` never succeeded.

There are two separate limits:

| Limit | Default | Set to | Where |
|---|---|---|---|
| Claude Code MCP tool-call timeout (`MCP_TOOL_TIMEOUT`) | client default; observed to cut off before 100 s (this was the one hit) | 900,000 ms (15 min) | `env` block of `%USERPROFILE%\.claude\settings.json` |
| REA Ghidra startup deadline (`REA_GHIDRA_STARTUP_TIMEOUT_MS`) | 330,000 ms | 840,000 ms (14 min) | `env` of the `rea` server in `.claude.json` and `claude_desktop_config.json` |

The REA deadline is set just below the client timeout, so an over-long analysis returns a
clear REA error instead of a silent client cutoff. Backups of the client configs from before
the change are kept as `*.2026-10-12T21-13-50-511Z.bak`.

{: .important }
**Tested 2026-10-12: this does not fix the timeout inside the Claude desktop app.** After a
restart, `MCP_TOOL_TIMEOUT=900000` was confirmed active, yet a direct `binary_overview` on
`Sum.exe` still failed with `Request timed out` about 60 s after the call began. The desktop
app then restarted the REA server, which threw away the Ghidra analysis. This matches a
[reported bug](https://claudeissues.com/issue/63379-bug-desktop-app-cancels-stdio-mcp-tool-calls-at-60s-and-ignores-mcp-tool-timeout):
the desktop app cancels stdio MCP calls at about 60 s and ignores `MCP_TOOL_TIMEOUT`, while
the `claude` terminal CLI is reported to honor it (not tested here).

What works, by where you run Claude:

| Where | Large binaries such as `Sum.exe` |
|---|---|
| Claude desktop app (Code tab or chat) | Use the daemon below; direct REA calls time out at about 60 s |
| `claude` CLI in a terminal | Direct REA calls are expected to work with the settings above (*reported*, not tested) |
| Any client, small binaries (under 60 s to analyze) | Direct REA calls work |

The settings are kept, because they are harmless and help the CLI and REA's own deadline.

### Fallback: the long-lived session daemon

If a direct call still times out: `tools/rea-daemon.mjs` keeps a single REA MCP session open with no per-call
timeout and takes requests from a queue folder, and `tools/rea-query.ps1` submits calls to
it. After the first 100-second analysis, every query in the same session returns in about
a second. See [Analysis tools](tools.md).

Small binaries (for example the 90 KB patch DLL) analyse quickly enough to use the
normal MCP tools directly.
