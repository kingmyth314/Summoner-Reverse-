---
title: Sources
parent: Reference
nav_order: 2
---

# Sources

## Tools

- [REA](https://github.com/morluto/rea) - `rea-agents` 6.4.0 on npm
- [Ghidra](https://github.com/NationalSecurityAgency/ghidra) - 12.1.4, released 2026-09-21
- [Eclipse Temurin](https://adoptium.net/) - JDK 21.0.12
- [Bug report: desktop app cancels stdio MCP tool calls at ~60 s and ignores MCP_TOOL_TIMEOUT](https://claudeissues.com/issue/63379-bug-desktop-app-cancels-stdio-mcp-tool-calls-at-60s-and-ignores-mcp-tool-timeout)
- [Bug report: timeout field in claude_desktop_config.json not honored](https://claudeissues.com/issue/43791-bug-timeout-field-in-claude-desktop-config-json-not-honored-for-mcp-servers)

## Compiler (Visual C++ 6.0)

- [Internet Archive: Microsoft Visual C++ 6 Processor Pack + Service Pack 5](https://archive.org/details/vcpp5) (item
  `vcpp5`, community upload 2020) - downloaded 2026-10-16 with the owner's approval: `vs6sp5.exe` (136,410,832 bytes,
  MD5 971e5b1791ee90e6f397ec7fae520ba9) and `vcpp5.exe` (1,180,896 bytes, MD5 574e04a0bc863e8e1e8e28606da9d12f).
  Extracted with 7-Zip, never run. SP5's `msvcep.dll` is the `c2.dll` 12.00.8966 that built Sum.exe
  ([Decompilation plan](../decomp-plan.md)). Kept outside the repository ([Setup](../setup.md)).
- The base Visual C++ 6.0 Standard (Japanese) and Professional (German, X04-63216) editions are the owner's own
  discs, placed in the project folder as archives (git-ignored).
- [MCPmag: Microsoft Releases Visual Studio 6.0 SP5](https://mcpmag.com/articles/2001/02/26/microsoft-releases-visual-studio-60-sp5.aspx) - SP5's release date (February 2001)
- [Microsoft Learn: MSVC compiler versioning](https://learn.microsoft.com/en-us/cpp/overview/compiler-versions?view=msvc-170)

## libjpeg 6b (the standalone link)

- [Independent JPEG Group: jpegsrc.v6b.tar.gz](https://www.ijg.org/files/jpegsrc.v6b.tar.gz) - downloaded
  2026-10-19 with the owner's approval (613,261 bytes, SHA-256
  75c3ec241e9996504fe02a9ed4d12f16b74ade713972f3db9e65ce95cd27e35d, the published value; dated 1998-03-27).
  Sum.exe links libjpeg 6b statically; `tools/link-build.mjs` builds its common and compression objects with VC6 from
  `third_party/jpeg6b/jpeg-6b` (git-ignored, not in the repository).

## llvm-mingw (the clang builds and SummonerFix)

- [mstorsjo/llvm-mingw](https://github.com/mstorsjo/llvm-mingw) - release 20260908, UCRT, x86_64 host (clang 23.1.1),
  installed with `winget install MartinStorsjo.LLVM-MinGW.UCRT`. `CLANG_DIR` names its `bin` folder. It builds
  SummonerFix and the clang configurations, 32 and 64-bit ([Modern C++ port](../future/modern-port.md)); LLVM 16 or
  later is needed for `-ffp-eval-method=double`.

## FFmpeg (movies in the 64-bit build)

- [BtbN/FFmpeg-Builds](https://github.com/BtbN/FFmpeg-Builds/releases) - `ffmpeg-n9.0-latest-win64-lgpl-shared-9.0.zip`
  from the `latest` release, downloaded 2026-10-25 with the owner's approval (77,222,798 bytes, SHA-256
  a87174b0bc935e5d944d8c28a74c7099ea82d1de6bc8f271e0e6dd3fa645781b; "latest" is rebuilt over time, so a later download
  differs). LGPL build, shared DLLs: `FFMPEG_DIR` names the extracted folder, and `avformat-63`, `avcodec-63`,
  `avutil-61`, `swscale-10` and `swresample-7` go beside the 64-bit exe. Kept outside the repository.

## Vulkan SDK (the Vulkan renderer)

- [LunarG Vulkan SDK](https://vulkan.lunarg.com/sdk/home) for Windows, 1.4.363.0 - `vulkansdk-windows-X64-1.4.363.0.exe`,
  downloaded and installed by the owner on 2026-10-26 (303,082,720 bytes, SHA-256
  94a82d378f7a5e3e54c9db7d2fb7016af136e14ac0a18dbf0f2f67a36352d141; installed to `C:\VulkanSDK\1.4.363.0`, which
  the installer names in `VULKAN_SDK`). The build uses its headers and `glslc`; test runs can load its validation
  layer. Only for building the Vulkan renderer of the 64-bit exe ([Vulkan renderer](../future/vulkan-renderer.md));
  players need only a graphics driver with Vulkan.

## dgVoodoo 2 (the Glide and Direct3D renderers)

- [dgVoodoo2 v2.87.5 on GitHub (dege-diosg/dgVoodoo2, the author's releases)](https://github.com/dege-diosg/dgVoodoo2/releases/tag/v2.87.5)
  - `dgVoodoo2_87_5.zip`, downloaded 2026-10-21 with the owner's approval (9,268,078 bytes, SHA-256
  5ffde6927f7355ca3fdd5d785b581256a8e6539fa13e395a891ade6ba1040850; released 2026-09-15). Only the x86
  `Glide2x.dll`, `DDraw.dll`, `D3DImm.dll` and `dgVoodoo.conf` are used, in a test folder outside the repository;
  they are never committed ([dgVoodoo 2](../future/dgvoodoo.md)).

## Engine reference projects

- [mikegoforth/summoner-2-decomp](https://github.com/mikegoforth/summoner-2-decomp) - matching decompilation of Summoner 2 (PS2); no license stated, local reference only
- [rafalh/dashfaction](https://github.com/rafalh/dashfaction) - Red Faction community patch (MPL-2.0)
- [Dash Faction installation guide (Red Faction Wiki)](https://redfactionwiki.com/wiki/Dash_Faction_Installation_Guide)

## Timeslip's patch

- [Internet Archive copy of Timeslip's site](https://web.archive.org/web/2015/http://timeslip.users.sourceforge.net/) - "Summoner patch" download link
- [Timeslip's SourceForge profile](https://sourceforge.net/u/timeslip/profile/) - where the original site now redirects
- [GOG forum: Summoner Patch (by Timeslip)](https://www.gog.com/forum/summoner/summoner_patch_by_timeslip) - readme text and changelog
- [GOG forum: Summoner Patch (by Timeslip), page 2](https://www.gog.com/forum/summoner/summoner_patch_by_timeslip/page2) - user reports on keys and ini options
- [PCGamingWiki community: Summoner - Timeslip's Patch](https://community.pcgamingwiki.com/files/file/4090-summoner-timeslips-patch) - mirror (blocks automated download)

## Speed and cutscene bug reports

- [Steam discussion: intro cutscene speed bug](https://steamcommunity.com/app/275570/discussions/0/2958293022038459625)
- [GOG forum: Uncovered potential speed issue solution](https://gog.com/forum/summoner/uncovered_potential_speed_issue_solution)
- [Steam guide: Summoner Mod Guide](https://steamcommunity.com/sharedfiles/filedetails/?id=1796345125)
- [PCGamingWiki: Summoner](https://www.pcgamingwiki.com/wiki/Summoner)
