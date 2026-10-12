---
title: Tooling setup
nav_order: 2
---

# Tooling setup
{: .no_toc }

1. TOC
{:toc}

Setup performed on 2026-10-12 on Windows 11 Pro (10.0.26100), x64.

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

## Known issue: first-query timeout on large binaries

Ghidra's initial auto-analysis of `Sum.exe` (1.7 MB, 6,612 functions) takes about
**100 seconds**. The MCP client in Claude Code gives up first (`Request timed out`), and
REA discards the ephemeral analysis when a request is cancelled, so direct MCP calls on
`Sum.exe` never succeed.

Workaround: `tools/rea-daemon.mjs` keeps a single REA MCP session open with no per-call
timeout and takes requests from a queue folder, and `tools/rea-query.ps1` submits calls to
it. After the first 100-second analysis, every query in the same session returns in about
a second. See [Analysis tools](tools.md).

Small binaries (for example the 90 KB patch DLL) analyse quickly enough to use the
normal MCP tools directly.
