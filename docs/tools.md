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

## `tools/vc6-compare.mjs`

Compiles a `src/decomp` unit with Visual C++ 6.0 and compares each function that has a `// 0xADDR` comment with
Sum.exe's bytes at that address. Relocated fields (call / jmp targets, absolute addresses) are masked, using the
relocations of the `.obj`; the `/FAsc` listing gives the demangled names and the instruction text. Object and
listing go to a temporary folder outside the repository. It needs `VC6_DIR`, the `VC98` folder of an extracted
VC6 Professional or Enterprise (Standard Edition has no optimiser) with **Service Pack 5's compiler files** in `BIN`
(Sum.exe's build, `c2.dll` 12.00.8966; see [Decompilation plan](decomp-plan.md), "Build 8966 is Visual Studio 6.0
Service Pack 5", for which files and how to get them without installing). With the release build (8168) the
results differ in register choice and branch layout. It runs from PowerShell or cmd (Git Bash rewrites
`/switches`). `DECOMP_INCLUDE` points it at another header tree (a merge stage) instead of `src/decomp/include`, and
`VC6_FLAGS_FILE` at that tree's flags file instead of `src/decomp/vc6-flags.txt`.
Without `--flags`, each file is compiled with its line in `src/decomp/vc6-flags.txt` (game units `/O2 /GX /MT` by
default; the vsdk library `/Ox /Ow /GX /MT`, some with `/Ob0`, D7502; `/Od /Ob1 /GX /MT` for the unoptimised unit),
otherwise `/O2 /GX /MT`.

```powershell
$env:VC6_DIR = 'C:\path\to\VC98'
node tools/vc6-compare.mjs src/decomp/vsdk/frametime.cpp                     # default flags /O2 /MT
node tools/vc6-compare.mjs src/decomp/vsdk/matrix.cpp --flags "/O2 /Ob0 /MT"  # a unit built without inlining
node tools/vc6-compare.mjs src/decomp/game/objects.cpp --flags "/Od /Ob1 /GX /MT" --json
```

| Option | Meaning |
|---|---|
| `--flags "<cl flags>"` | compiler flags (default `/O2 /MT`; Sum.exe links the multithreaded CRT, DECISIONS.md D4904) |
| `--json` | one JSON record per file and function instead of the table |
| `--keep` | keep the temporary folder (printed) with the `.obj` and `.cod` |
| `-v` | also list relocation targets that differ from the expected address, and other notes |
| `--score` | for each function that differs, its distance from a match: the instructions outside the longest common subsequence of ours and the original's, relocations and branch displacements masked (what `vc6-nearmiss.mjs` ranks by) |
| `--align ADDR` | (implies `--score`) the function at ADDR (hex) as an aligned listing of ours and the original's, one row per instruction: `=` same bytes, `~` a pair with the same mnemonic and different bytes, `-` only ours, `+` only the original's. Shows where the differences sit, which `vc6-side.mjs` (row by row, no alignment) does not once one side has an extra instruction |

Per function it prints `match`, `differs` (the first differing offset, both sizes and instruction counts, and the
two instructions at that offset) or `not found` (no compiled function of that name: inlined, or the comment is not
on a definition), then a summary line. The k-th definition of a name in the source is paired with the k-th compiled
function of that name. A function is a match when its bytes equal the original's up to its length and the
original has only padding (nop / int3) before the next known function start.

Relocation targets (calls, global addresses, constants) are checked too (2026-10-17, DECISIONS.md D9100): each is
resolved through the file's own markers, every function marker and global with an address comment in the tree,
`symbols/sum.symbols`, labels of the same function, or, for constants, the bytes at the original's target. With
`--strict` (or `VC6_STRICT=1`, which `vc6-sweep.mjs` passes on) a function whose bytes match but whose target is
wrong counts as `target`, not `match`; other overloads of the same name, byte-identical folded functions and `jmp`
thunks are accepted. The summary counts checked and unchecked targets (`-v` lists the unchecked ones).
`tools/vc6-targets.mjs <sweep list>` lists every wrong target of a strict sweep. Strict mode is the standard since
2026-10-17: `src/decomp/vc6-matches.txt` holds strict results. The original's instruction counts come from `build/sum.asm`
(`tools/asm.mjs`'s cache; run that once first).

With `--score`, a function with an exception frame (`/GX` and a local with a destructor) is scored only up to its
own end: VC6 puts the unwind funclets in `text$x`, after all the functions, and the listing gives them offsets that
start again from the function's own. Our side is cut at the first instruction whose offset goes backwards
(2026-10-22; before that the funclets counted as extra instructions: `cutscene_script_load` 32, now 25,
`editor_dialog_show` 137, now 71). The ranking of round 5 and earlier overrated such functions.

Two pitfalls: a helper function placed between a `// 0xADDR` comment and the function it names takes the marker,
so the named function is reported `not found` (keep the comment directly above its definition); and the k-th
definition of a name pairs with the k-th compiled function of that name, so overloads have to stay in source order.

## Byte-matching workflow tools

Added 2026-10-17 (they were scratch scripts during the 2026-10-16 rounds). The source-side techniques the rounds found
are on [Matching devices](matching-devices.md). All need `VC6_DIR` set to the Service
Pack 5 `VC98` folder ([Setup](setup.md)) and run from PowerShell or cmd.

| Tool | What it does |
|---|---|
| `tools/vc6-sweep.mjs [--root DIR] [--out FILE] [--only REGEX] [--jobs N] [--check LIST]` | Runs `vc6-compare.mjs` on every unit of a tree (the repo's `src/decomp` by default, or a merge stage / worker copy with its own `include/` and `vc6-flags.txt`) and writes `<file> <address> <match\|differs\|not_found>` lines. `--check src/decomp/vc6-matches.txt` lists every address that matched there and no longer does, and exits 1 if there is one: the "no match lost" check before every commit. |
| `src/decomp/vc6-matches.txt` | The committed per-address result of the last full strict sweep (2026-10-22: 5,613 of 5,926 match, call and data targets included). Regenerate with `--out src/decomp/vc6-matches.txt` after a strict sweep that `--check` passed. |
| `tools/decomp-confidence.mjs [list] [--apply]` | Sets the `Confidence:` marker of every compared function from the sweep list: match -> `exact`, an `exact` that differs -> `likely`. Without `--apply` it lists the changes. Run after each committed sweep. |
| `tools/decomp-uncompared.mjs <list> [--list]` | Written functions (a sum.symbols address in a `// 0xADDR` comment) that the sweep never compared, by reason: inside another marker's comment block (mostly compiler-generated initialisers and destructors of globals), a note, or not a definition. |
| `tools/vc6-targets.mjs <sweep list>` | For a strict sweep list, every function marked `target` and which reference is wrong (symbol, the address or bytes ours expects, what Sum.exe has). |
| `tools/vc6-side.mjs <listing.cod> <name> <address>` | One function's VC6 output next to Sum.exe's disassembly, differing mnemonics marked `*`. The listing comes from `vc6-compare.mjs --keep` (or `--json`'s `tmp`). |
| `tools/vc6-try.mjs [--root DIR] <file> <address> <variants.json> [--side NAME]` | Tries source variants (`[{"name", "from", "to"}]`, exact text replacements) one at a time and reports whether the function then matches and the file's match count; the file is always restored. `--side` prints each failing variant's first differing lines; NAME is a substring of the mangled name and the first `PROC` containing it wins, so for a member or an overloaded name pass the mangled form (`?draw@dialogue_box@@`), or a bare `draw` may show another class's function. |
| `tools/vc6-nearmiss.mjs [--root DIR] [--only REGEX] [--jobs N] [--top N] [--max D] [--out FILE]` | Ranks every function that still differs by its distance from a match (`vc6-compare.mjs --score`: the instructions outside the longest common subsequence of ours and the original's, relocations and branch displacements masked; 0 means only branch distances differ). The closest ones are where a byte-match round starts. The whole tree takes under a minute with `--jobs 8`. |
| `tools/vc6-localhash.mjs [--probe] name...` | The frame order VC6 gives the locals of an unoptimised (`/Od`, `#pragma optimize` off) function: by a hash of their names, not their declaration order (DECISIONS.md D6101, D23500, D25200). Prints the predicted order, highest address first, with each name's bucket; `--probe` also compiles the names as int locals and prints VC6's order. A wrong slot order in such a function is fixed by renaming locals. |
| `tools/texture-upscale.mjs --game DIR --out DIR --esrgan EXE [--model M] [--only F] [--vpp LIST] [--limit N]` | Builds a loose-file texture pack from the player's own install with Real-ESRGAN: 4x up to 256 pixels, 2x up to 512, alpha and the green colour key handled separately, new mip chains, the VBM textures (16-bit, frame by frame) and the fonts (`<font>-hires.tga`, used by the experimental build), files placed where the game reads them ([Texture upscaling](future/texture-upscale.md)). The output is derived from the game's art: never commit it. |
| `tools/texture-pack-original.mjs --game DIR --pack DIR --out DIR [--list]` | From a full texture pack, the pack the original game can use (hard links): without the interface art of `summoner.vpp`, the tables' icon and automap images, 2D cutscene stills and the experimental build's fonts ([Texture upscaling](future/texture-upscale.md#the-pack-for-the-original-game-2026-10-25)). |
| `tools/vc6-flagsweep.mjs <root> <out.json> [dir]` | Compiles every unit of `dir` (default `vsdk`) with its flags, with `/O2` -> `/Ox /Ow` and with `/Ow` added, and records each match set. A unit switches only when the new set contains every old match (DECISIONS.md D7502). |
| `tools/decomp-merge.mjs <worker> <base> <stage>` | Three-way merge of a worker's copy of `src/decomp` into a stage: takes, merges (`git merge-file`) or adds each changed file; DECISIONS.md conflicts keep both appended blocks; other conflicts are listed (exit 1). |
| `tools/decomp-sigcheck.mjs [src/decomp]` | Free functions whose header declaration and definition differ in parameters (a silent overload). One known, accepted: `charinfo_load_move_anim`. |

The round workflow (2026-10-16): snapshot the tree for each worker, let it work in its own copy, merge it into a
stage with `decomp-merge.mjs` against its snapshot, compile everything with mingw
(`i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Iinclude`), run `decomp-sigcheck.mjs`, check for duplicate
`// 0xADDR` markers, run `vc6-sweep.mjs --root <stage> --check src/decomp/vc6-matches.txt` with `VC6_STRICT=1`, then copy the stage
into `src/decomp`, rewrite `vc6-matches.txt`, run `decomp-confidence.mjs --apply` and `decomp-progress.mjs`, and commit.

## `tools/link-build.mjs` (the standalone rebuilt exe)

Added 2026-10-19. Compiles every unit of `src/decomp` with its VC6 flags (`vc6-flags.txt`) and links them, with
the link support in `src/link`, into `build/link/Sum-rebuilt.exe`: an executable built entirely from the
decompiled source and VC6 SP5's own libraries (LIBCMT, the Windows and DirectX import libraries, MSACM32).

```powershell
$env:VC6_DIR = 'C:\path\to\VC98'
node tools/link-build.mjs --define SHIPPED_NOCD --define REBUILT_OPENGL
node tools/link-build.mjs --config fixed        # build/link-fixed/Sum-fixed.exe (docs/builds.md)
node tools/link-build.mjs --config experimental # build/link-exp/Sum-exp.exe (docs/experimental-build.md)
```

The experimental configuration also compiles `src/decomp/experimental/*.cpp` (not part of Sum.exe, so not in
`vc6-matches.txt` and never swept). `<exe>.fixes` lists the `// FIXED_BUILD: <bug> <Key>` markers, and for the
experimental build also the `// EXPERIMENTAL_BUILD: <feature> <Key>` ones; SummonerFix leaves those parts out.

| Option | Meaning |
|---|---|
| `--config shipped\|fixed\|experimental` | a build configuration of [Fixed and experimental builds](builds.md): its define set (`fixed` = `FIXED_BUILD SHIPPED_NOCD REBUILT_OPENGL`, `experimental` adds `EXPERIMENTAL_BUILD`), its own folder (`build/link`, `build/link-fixed`, `build/link-exp`) and exe name (`Sum-rebuilt`, `Sum-fixed`, `Sum-exp`). A `FIXED_BUILD` build also writes `<exe>.fixes`, the `[Fixes]` keys its source already fixes, from the `// FIXED_BUILD: <bug> <key>` markers; SummonerFix leaves those patches out. Without `--config` the build is as before |
| `--define NAME` | a `/D` for every unit. `SHIPPED_NOCD`: the no-CD edit the shipped exe carries (D2600, D9812). `REBUILT_OPENGL`: the game's OpenGL renderer in a window, with SummonerFix's texture-size cap (D9812) |
| `--compiler clang` | build with clang and lld instead of VC6 ([Modern C++ port](future/modern-port.md#l1-done-the-clang-build-2026-10-25), rung L1). `CLANG_DIR` is the folder with `clang++.exe` and `ld.lld.exe` (LLVM 16 or later; llvm-mingw works, since its clang has the MSVC target). VC6_DIR is still needed: the headers and libraries are VC6's. Output: `build/<folder>-clang`, `<exe>-clang.exe`, with its `.addr` and `.fixes`. It defines `MODERN_BUILD` (rung L2: the plain C++ form where the matching source has a VC6-only device) and writes its warnings to `warnings.txt` (the count is printed; [L2](future/modern-port.md#l2-warning-clean-2026-10-25)) |
| `--writable-literals` | clang build only: merge `.rdata` into `.data`, so string literals are writable as in VC6's exe. Off by default since L2: a write into a literal then faults where it happens |
| `--mix FILE` | 32-bit clang build only: the units listed in FILE (one per line, as in `vc6-matches.txt`) are compiled by clang, and every other unit links the VC6 object from the same configuration's VC6 build (`build/<folder>/obj`, which must be current). For bisecting a behaviour only the clang build shows, down to the unit; within a unit, `#pragma clang optimize off` around halves of the functions narrows it to one ([the NPC goal case](future/modern-port.md#the-clang-builds-extra-npc-goals-two-float-roundings-2026-10-25)). Use `--out` for a separate folder |
| `--arch x64` | clang build only: the x86_64 build ([L3](future/modern-port.md#the-64-bit-spike-2026-10-25)), with llvm-mingw's 64-bit headers, C runtime and import libraries (none of VC6's), debug information (`-g`), a stand-in for EAX (`src/link/x64_stubs.c`) and the Bink calls through FFmpeg (`src/decomp/modern/bink.cpp`; set `FFMPEG_DIR` to an FFmpeg shared build for its headers, and put its five DLLs beside the exe; without it movies are skipped). Output: `build/<folder>-clang-x64`, `<exe>-clang-x64.exe` and its `.addr` (joined through demangled qualified names, rows against the base 0x400000; for the in-source agent mode). OpenGL renderer only. Linked at 0x400000 without large-address awareness, so every address stays below 2 GB (the 4-byte `PTR32` fields of the relocated file formats rely on it). Runs without SummonerFix through the in-source host ([L3b](future/modern-port.md#l3b-the-in-source-host-2026-10-25)), so `tools/agent.mjs` and the check tools drive it as they drive the 32-bit builds |
| (clang builds) | also compile `src/decomp/modern/*.cpp`, the in-source host: settings, the output window, agent mode. It stays off while SummonerFix is loaded |
| `--force` | link with unresolved symbols (`/FORCE:UNRESOLVED`), to list what is missing |
| `--recompile` | compile every unit again (objects are cached in `build/link/obj` by date; a header change needs this) |
| `--out DIR` | output folder (default `build/link`) |

`src/link` holds what Sum.exe takes from outside its own code: the three DirectX 7 GUIDs (from the DX7 SDK's
dxguid.lib; VC6's is older), stub sources whose only use is an import library with the right names for
binkw32.dll (decorated stdcall names) and EAX.DLL (ordinal 5), and a no-op libjpeg stub. Sum.exe links libjpeg 6b
statically: when the IJG source is in `third_party/jpeg6b/jpeg-6b` (git-ignored; [sources](reference/sources.md))
the tool builds its common and compression objects with VC6 instead (`--jpeg DIR` for another place). Nothing of
these is a third-party binary in the repository.

After a full `hybrid-build` (which writes `build/hybrid-all/hybrid-addrs.txt`, every name it resolved to a Sum.exe
address) the tool also writes `Sum-rebuilt.addr`: Sum.exe address, size and rebuilt address of every symbol both
builds know. **SummonerFix runs in the rebuilt exe through it** (`src/summonerfix/rebuilt.c`): when `<exe>.addr`
sits next to the exe, `SumVa` translates Sum.exe addresses (an address inside a symbol keeps its offset) and
`OrigVa` translates return addresses back, and only the registry layer, the OpenGL texture-size fix, the display
module, the key bindings, agent mode and the bug-fix module are installed. `PatchBytes` checks the original bytes
first, so only patch sites in byte-identical code apply; `[Fixes] Enabled=0` keeps the gameplay fixes off and leaves
the unconditional no-sound crash guard (DirectSound fails on a machine without a sound device, in the original too).
Run it from a separate folder holding hard links to the `.vpp` archives and the Bink and EAX DLLs, copies of
`data`, `summoner.cfg` and a save, SummonerFix's `dinput.dll` and an ini, and the exe with its `.addr` file;
`tools/agent.mjs` drives it with `SUMMONER_GAME=<folder>` and `SUMMONER_EXE=Sum-rebuilt.exe`. Without SummonerFix
the exe reads the real registry (`ConnectionSpeed` there decides whether it starts WinSock, and Windows Firewall then
asks about the new exe).

| Tool | What it compares |
|---|---|
| `tools/link-datacheck.mjs [--show N]` | the rebuilt exe's initial data against Sum.exe's, symbol by symbol through the address table: pointers count as equal when they translate, or point to the same string; each object up to the next symbol in either build. Found D9814 |
| `tools/link-memcheck.mjs A B [--also A2,A3] [--static] [--exe EXE]` | two run-time dumps written by the agent command `dumpdata OUT TABLE` (every data symbol of an address table, read at its own address) at the same point in both games, compared the same way. Bytes both processes still hold as in their exe file are skipped (static data is the datacheck's; `--static` keeps them), and two pointers without a table row (vtables) are equal when the first dword they point at translates. `--also` takes more original dumps: a symbol then counts only when the rebuilt value differs from every one and the original runs agree in it. `--exe` names the rebuilt exe the dump came from: its `.addr` sits next to it, its `.map` next to it or in the build folder that made it (default `build/link/Sum-rebuilt.exe`). Until 2026-10-25 the tool always read `Sum-rebuilt`'s table, so the checks of `Sum-fixed`, `Sum-exp` and the clang builds translated pointers with the wrong one, and counted hundreds of symbols as different that are equal (the clang build on Wolong2: 474 with the wrong table, 10 with its own); `link-savecheck` now passes its `--exe` |
| `tools/link-screencheck.mjs OUT [SCENARIO ...] [--runs 3]` | the same screens (16 scenarios: the main menu and its screens, the prologue at two moments, the village cinematic, the game view and its panels) in the original (`--runs` times) and the rebuilt exe, compared pixel by pixel; pixels that differ between the original runs are masked with a 6-pixel margin. Writes each capture as PNG, a `-diff.png` with the differing pixels in red, and `results.txt`. Walking characters move differently from run to run, so the game scenarios need three or more original runs |
| `tools/link-savecheck.mjs OUT LEVEL:SCRIPT ... [--runs 3]` | the level save check of the hybrid, for the standalone exe in its test folder: per level, the original `--runs` times and the rebuilt exe once, each from the newest test save (`[Debug] TestSaves=1`, the test folder's `savetest/`), `level` with the full party, a `dumpdata` and a quicksave; then the save comparison (`tools/save-compare.mjs`, shared with `hybrid-test.mjs`) and `link-memcheck.mjs --also`. Results go to `OUT/results.txt` level by level. The save shows game state, the dump the globals no save holds; neither sees rendering or timing (the prologue camera effect, D9815, and the Create Game centring, D9817, would pass) |
| `tools/standalone-probe.ps1 -Exe EXE -Map MAP [-Load] [-Lldb FILE]` | runs a rebuilt exe without SummonerFix (its `dinput.dll` moved aside and put back) and reads the game's state from outside every 3 s with `ReadProcessMemory`: sequence state, interface screen, fps, the real-time clock, level time, mouse position. MAP is a 64-bit exe's link map (plus its load address) or a 32-bit rebuilt exe's `.addr` table. `-Load` clicks Load Saved Game and the newest save with real mouse input (it takes the foreground for a moment); `-Lldb FILE` attaches llvm-mingw's lldb (`CLANG_DIR`) and writes the backtrace if the game crashes. The 64-bit harness until agent mode is in the source ([L3](future/modern-port.md#l3-64-bit-begun-2026-10-25)); screen captures of the OpenGL window come back black |
| `--exe NAME --fixes` (both checks) | `--exe` tests another exe than `Sum-rebuilt.exe` (its `.addr` next to it), for example `Sum-fixed.exe`; `--fixes` runs both exes with SummonerFix's fixes on (`[Fixes] Enabled=1`), the comparison for the fixed build ([Fixed and experimental builds](builds.md)) |
| `tools/fixed-compile.mjs [--root DIR] FILE [--experimental]` | compiles one unit of a `src/decomp` tree as the fixed build does (its `vc6-flags.txt` flags plus `/DFIXED_BUILD`) and lists the errors and warnings, with paths shortened to the tree. The check for `FIXED_BUILD` code; the byte-matching tools never see that define |
| `tools/build-saveeditor.ps1` | builds the save editor (`src/saveeditor`) into `build/saveeditor/`: `SummonerSaveEditor.exe` (the window, [Save editor](save-editor.md)), `saveedit.exe` (the command line of `tools/savedit.mjs`) and `app_test.exe` |
| `tools/saveedit-test.mjs --corpus DIR` | the save editor's acceptance tests on a folder of copies of saves: a byte-identical round trip of every save, and 71 edit cases run with `savedit.mjs` and `saveedit.exe`, outputs compared byte for byte |
| `tools/saveeditor-uitest.ps1 -Corpus DIR` | starts `SummonerSaveEditor.exe` on copies and drives it with window messages (no mouse): edits on four pages, the save compared with `savedit.mjs`, a refusal, a clean close |

The hybrid cannot see data errors in byte-matching functions (they use Sum.exe's own data there) or globals that
the code uses as one record but the source declares apart (D9813); the standalone exe shows both.

## `tools/hybrid-build.mjs` (the hybrid game)

Added 2026-10-18. It compiles `src/decomp` files with VC6 into a DLL whose functions replace the original ones
inside the running Sum.exe, so decompiled code runs in the real game one file, or one function, at a time. SummonerFix
loads the DLL when `[Hybrid] Dll` names it (`src/summonerfix/hybrid.c`) and writes a `jmp` at the start of each
replaced original. Everything else (the other functions, the globals, the C runtime) stays the game's own. See
[Decompilation plan](decomp-plan.md), "The hybrid game".

```powershell
$env:VC6_DIR = 'C:\path\to\VC98'
node tools/hybrid-build.mjs src/decomp/game/new_credits.cpp            # one file -> build/hybrid/hybrid.dll
node tools/hybrid-build.mjs src/decomp/game/*.cpp src/decomp/vsdk/*.cpp --out build/hybrid-all
node tools/hybrid-build.mjs src/decomp/game/new_credits.cpp --only 0x408620   # inject one function
```

Copy `hybrid.dll` into the game folder and set `[Hybrid] Dll=hybrid.dll`. Without rebuilding, `Skip=0x408620,...`
leaves functions original and `Only=<file>` replaces only the addresses the file lists (for a search: build the
whole tree once, then halve the list until the one function that breaks something is left). The log says how
many functions were replaced. Functions SummonerFix patches (any `PatchBytes` inside them) and ranges its hooks
recognise their callers by (`KeepOriginal`, display.c) stay original, so no fix is lost.

| Option | Meaning |
|---|---|
| `--only 0xADDR,...` | inject only these functions; the others of the files stay the original |
| `--out DIR` | output folder (default `build/hybrid`) |
| `--force` | link even with names that have no address (calls to them jump to 0; for experiments only) |
| `-v` | list every global kept in the DLL and every function too short for the jmp |

**Coverage.** With `[Hybrid] Coverage=1` the loader sends each replaced function through an 11-byte stub
(`inc dword [counter]; jmp function`), and the agent command `hybridcov NAME` writes `agent\NAME.txt` with
`0xADDRESS calls` for every replaced function. `node tools/hybrid-coverage.mjs <file>... [--list ran|never]` joins
one or more of these files with `src/decomp/vc6-matches.txt`: of the functions that do not match byte for byte,
which a run executed (tested in the game when the run's checks passed) and which none did (the scenarios to find).

**`tools/hybrid-test.mjs`** runs the checks the hybrid work uses. It needs `SUMMONERFIX_INI_BACKUP` (or
`--ini-backup`) naming a copy of the game's `summonerfix.ini`, which it puts back after each run, and works only in
`Summoner/savetest` (`[Debug] TestSaves=1`); the player's own saves are never touched.

| Command | What it does |
|---|---|
| `session [KEY=VALUE@Section ...]` | start the game in agent mode on the newest test save with those ini keys (the stale `agent\cmd.txt` of a killed session is removed first: it would run at the next start) |
| `scenario <dir> <tag> [--runs 3] [--level NAME:SCRIPT \| --steps "..."]` | the scenario (a level change with the full party by default, or agent commands) on the original game `--runs` times and on the hybrid once; a quicksave after each (on the hybrid the coverage dump follows it, so the save chain counts) (`savetest/quicksave.sav`, back-dated so the next session loads the same save), copied to `<dir>/<tag>-orig<N>.sav` / `-hyb.sav`, then `compare` |
| `compare <dir> <tag> [--runs 3] [--show 25]` | the hybrid's save against the original runs, dword by dword after the thumbnail: a value that varies between the original runs may vary as much in the hybrid; LENT records are walked (party members' records are 8 bytes longer) and named by record, type and name hash |
| `search <orig.sav> --level NAME:SCRIPT --region lo-hi\|a,b,c [--majority]` | the halving search over `[Hybrid] Only` for the functions that change the given save bytes; `--majority` counts a step as failing on two of three runs (for a difference that comes and goes) |

Three original runs are the minimum: NPC positions and timers vary between runs of the original.

**Dual calls** (`src/summonerfix/dualcall.c`). `[Hybrid] Dual=<file>` lists functions to check against the
original on the game's own calls, for code where a whole run cannot be compared (a fight never plays out the same
twice). For the first `DualSamples` calls of each, the checker copies the game's data (Sum.exe's `.data`/`.bss`,
44 MB), the caller's stack above the arguments and the C runtime's `rand` state; calls the original (its first
5 bytes restored for the call) with the caller's registers and arguments; keeps the result (memory, `eax`, `st0`,
the bytes the callee popped, `rand`); puts the copies back and calls the decompiled function the same way; and logs
each difference as `dual: 0xADDR call N differs` with the memory ranges, or `... same`. The game continues with the
decompiled result. Sound is switched off for the two calls (`snd_disabled`), and state the two calls cannot share
is left out of the comparison and of the restore: open files, OpenGL texture ids, the sound and music streaming
that another thread writes, the C runtime's own data (its heap bookkeeping, locks, FILE and atexit tables: putting
it back would rewind the heap under blocks allocated meanwhile), the frame clock and the AI profiler.
`DualIgnore=ADDR:LEN,...` adds ranges. Both calls run on a virtual clock (`QueryPerformanceCounter`,
`timeGetTime`, `GetTickCount` start where the real clock stood and advance one step per read, restarted for the
second call), and the frame clock's state is put back afterwards. The list file has one function per line,
`0xADDRESS kind`, the kind from `hybrid.txt`'s second column (what the function returns, from its mangled name:
`v` nothing, `f` a float in st0, `b` a bool or char in al, `s` a short in ax, `i` eax), so only the real return
value is compared. Heap memory is not copied: functions that load files, construct or destroy objects, or
allocate or free memory must not be listed (a constructor and a level loader hung or corrupted the heap in
tests). `[Debug] TraceDual=1` logs the start of each check, which names a call that never returns. A check costs
about 20 ms. Calls made inside a check run unchecked (no nesting), so a callee listed with its caller is checked
only where something else calls it; listing both also produced a false difference once (`spell_probe_ground_point`
with `get_node_world_transform`, 2026-10-18): check a function and its callees in separate runs. Keys the agent
presses during a check can land in one of the two calls (`key_down_state` and its counters differ): not an error.

How the objects are rewritten (all in the COFF symbol table; the code is VC6's as compiled):

| Symbol | Becomes |
|---|---|
| a decompiled function (`// 0xADDR` marker) | kept, and exported through an alias `f_<address>_<size>` (the size of the original, so the loader can see SummonerFix patches inside it) |
| every reference to a decompiled function, from any object | its **original** address: the call goes through the loader's jmp, so `Skip` / `Only` decide for each function on its own (the function's own symbol stays: an inline function's COMDAT section needs it) |
| an undefined name (another file's function or global, a CRT function) | absolute, with its Sum.exe address: the tree's address comments (sources and headers, `// folded (0x...)` and folded-declaration comment blocks), else `symbols/sum.symbols`, else Sum.exe's import table (`__imp__Name@N` -> the IAT slot) |
| a name another object of the same build defines | left alone: the linker joins them |
| a global, function-local static (`static T x;  // 0xADDR`) or its construction guard (`(guard 0xADDR)`), a table whose comment block starts with its address, a global named after its address | absolute: the game's own variable |
| in a function that byte-matches (listed in `vc6-matches.txt` and checked again against Sum.exe at build time), every reference to data (strings, floats, globals) | the address the original has at the same offset: the game's own data, including data SummonerFix patched. A listed function that no longer matches is reported and keeps its data in the DLL |
| other constants (strings, floats of non-matching functions; jump tables) | stay in the DLL |

Functions the shipped Sum.exe has edited in the file stay original: the no-CD edit in `cd_find_drive` and
`cd_set_root` (DECISIONS.md D2600), which the decomp writes unpatched, so their decompiled form asks for the disc.

Three linker facts it works around (**observed** with LINK 6.00):
- LINK adds the image base to an absolute symbol in a `rel32` relocation (a call), as if it were an RVA. Each such
  call is pointed at a shadow symbol of value `address - base`.
- LINK reads an absolute symbol that is static (storage class 3) as 0, in `dir32` relocations too. Those symbols are
  made external under a unique name.
- A static function cannot be exported by name, and thousands of `/EXPORT` options overflow the command line: every
  injected function gets an external alias `_f_<address>_<size>`, listed in a `.def` file.

The DLL has no entry point, no imports, no C runtime and no relocations (`/FIXED /BASE:0x50000000`; `binkw32.dll`
sits at `0x30000000` in Sum.exe). The build reports names with no address (the link fails unless `--force`), globals
that keep a copy in the DLL (`hybrid-globals.txt`: harmless for a constant table, a divergence for a variable), and
names that got an address of the wrong kind (a function outside the code, data inside it), and global variables (`.bss`, not function-local statics) that have no address: replaced and original functions would each use their own copy, so give them one. Originals shorter than
5 bytes stay original.

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
  data, hex; used to test the script editor flag), `quit`;
- `flag NAME [0|1]` reads or sets a story flag of the current level (`savegame_get_flag` / `savegame_set_flag`),
  `navgoto NAVPOINT` puts the current character on a level navpoint (for example `$end2trig`),
  `npcgoto NAME` puts it 4 units from a level script character (`script_object_find_character`; the name may contain spaces, for example
  `Luminar`) and prints where that character is. Testing only;
- `level NAME SCRIPT` changes level as a level exit does (`game_change_level`, run on the game thread), for example
  `level IonaEXT02 IonaEXT02_v2`. Unlike the `play_and_load_cutscene` warp it loads the level's own script
  characters, so story bosses are there. Start it from a loaded save: from a cutscene warp the load crashed in
  `skeleton_read_tag_record` (2026-10-17);
- `goldpile N` leaves a loose gold pile of N gold 2 units beside the current character (`item_create_by_name("Gold")`,
  `item_obj_spawn`, on the game thread; the object's address is in the log). For G40;
- `pad LX LY RX RY BUTTONS [MS]` feeds a synthetic XInput controller state through SummonerFix's controller code for MS ms (default 100): sticks -32768..32767, BUTTONS the XINPUT_GAMEPAD mask in hex plus 10000 for LT and 20000 for RT. Works with `[Controller] Enabled=0`; see [SummonerFix](summonerfix/index.md#controller-gamepad-support-controller) (2026-10-21, not yet run);
- `summon SPELL` raises a summon for the current character, 2 units beside it, as the summon spells do
  (`summon_create(cur_character, pos, layer, SPELL, -1)`, game thread; `cur_summon` is in the log). SPELL is the
  decimal index in `spells.tbl` order (38 golem ... 45 mist elemental ... 53 forest dragon). The summon then waits
  for its appearance effect before it emerges, as after a cast. Used to reproduce C317 (2026-10-20);
- `npchp FRACTION NAME` damages a level script character (a boss, by its script object name, e.g. `Giant Salamanka`)
  down to FRACTION of its base max HP, through `character_add_hp` as `npckill` does, so a boss passes its HP stages one
  at a time and its stage checks and transition cutscenes run as after real damage (game thread; HP before and after in
  the log). Used for the boss progression runs of 2026-10-20;
- `firewall N [START] [summon]` places N walls of fire (spell `0x1d`, `spell_place_ground_effect`) in a row, the first START units (default 6) beside the current
  character, cast by it or with `summon` by the active summon (game thread; the count placed is in the log). Used to show that C270 cannot happen and to reproduce G270;
- `spellhit TARGET SPELL POWER` runs `living_entity_get_hit_by_spell` on the character at TARGET (hex address,
  from `peek 5fcc88` after selecting it) with spell SPELL (hex) and POWER, cast by the current character with source
  type 6, as special abilities are applied (game thread). For G100, `spellhit 90a4e8 3e 100`;
- `flagl LEVEL NAME [0|1]` reads or sets a story flag of any level (the `level_script_table` index, as in savedit);
- `calli ADDR A [B]` calls a cdecl game function with integer arguments on the game thread and logs eax (G32:
  `calli 4a2480 0x58`, `boss_species_mounted_stage`);
- `call0 ADDR [N]` calls a void game function with no arguments N times on the game thread and logs
  `level_filename` / `level_script_name` after each call, then restores the three level names (for the world map
  encounter pickers, G6: `call0 4fc4b0 40`);
- `npckill NAME` gives a level script character 1,000,000 damage through `character_add_hp` (a type 4 NPC is made
  type 5 first, since type 4 ignores damage); `npcdead NAME` prints `script_object_find` and `script_npc_is_dead`
  for it. A character still hidden behind "wait for go" only reaches "dying" (`+0x320` bit `0x8`), not dead (`0x80`);
- `nudge DX DZ` moves the current character by DX, DZ world units, for example after `navgoto` to stand just outside
  a monster group's detection range;
- `callstr ADDR STRING [ARG [ARG2]]` calls the cdecl game function at ADDR with a string (`~` stands for a space) and up to two more arguments (ints; the first may also be
  a float's bits when ARG contains a dot) on the game thread and logs the returned eax. For example `callstr 4f95a0
  puzzle-flippers31` (the Khosani lab's use hook) or `callstr 4773d0 puzzle-flippers32 0.01`
  (`door_is_open_by_name`). The caller must know the function's arguments;
- `pokecode ADDR BYTE` writes one byte of game code (the page is made writable around the write), to switch a fix on
  and off in a running game for before / after captures; `k310` prints the `[Debug] TraceGlLights` counters;
- Commands that run on the game thread (`level`, `calli`, `callt`, `bag`, `knock` ...) now wait until it has run them
  (up to 10 s), so the next command cannot change their arguments first (a `calli` straight after a `callt` used to
  run with the `callt` address);
- **Boss fights with the whole party (2026-10-18).** `level` goes through `game_change_level`, which removes the party
  and spawns only Joseph unless `Full_party` (`0x609fe0`) is 1: `poke 609fe0 1` before `level` brings all four (fresh,
  at the level the script's story stage gives). Murod: `level jadetemple jadetemple`, then `navgoto $murodtrigger`.
  Luminar: `level IonaEXT02 IonaEXT02_v2`. To kill a member properly, `callt 450850 CHAR 0xC9742400 -1 0 0`
  (`character_add_hp(-1000000)`) and then write `FF FF FF FF` to its current move (`+0x6cc`): the death waits for the
  move otherwise, and the member stays "dying" (`0x8`). Switch `immortal` off first. Dead heroes leave the living
  list but stay in `party_list`;
- `count ADDR STRIDE N MASK` counts the records of a table (N records, STRIDE bytes apart, hex) whose first byte has a
  bit of MASK set, for example `count 2f077b8 74 128 1` for the used `sound_emitters` (K110);
- `bag N [ITEM]` leaves an unlocked loot bag with N items (default Health Tonic, `~` for spaces) beside the current
  character, built as `monster_drop_loot` builds one; the log gives the container address. Open it with
  `interface_set_screen(4, &container)`: write the address into `pickup_container` (`0x5c3b6c`) with `poke` and run
  `calli 412620 4 0x5c3b6c` (K12).
- `callt ADDR THIS [ARG ...]` calls a thiscall game function with up to six int arguments on the game thread and
  logs eax (esp is restored after the call, so a wrong argument count does not unbalance the stack); for example
  `callt 4740c0 0x91208c 0 5` queues a heal on the current character's AI (`ai_goal_add_cast_spell` on Character
  `+0x324`), and `callt 454700 0x911d68 0 15000 -1 0 HANDLE 0` adds a 15 s status as a melee status hit does (G271);
- `knock MS` knocks the current character down for MS ms through `ai_knock_down_local` and logs the AI mode before
  and after and the result (K80).

For combat tests: `[Debug] TraceArmourSpells=1` logs every hit that reaches the armour roll and every spell an
armour casts (caster, target, team), `[Debug] TraceHitVfx=1` logs each melee hit's target with its skin material
and the hit effect spawned, `[Debug] TracePhoenixInferno=1` logs each Phoenix Inferno hit with the party member
the loop was on (G215), `[Debug] LuminarBeamTargetGone=1` makes the Luminar's beam lose its target before the
hit and counts those frames (C250), `[Debug] TraceBlessCount=1` logs the AI healer's Bless candidate count and `BlessPassAlways=1` makes that pass run in any fight (C210), `[Debug] TraceGroupAttack=1` logs the Support-style attack orders (G211), `[Debug] TraceSupportPick=1` logs AI support spell picks (G229), `[Debug] TraceDetectCone=1` logs the party AI's detection cone tests (G210), `[Debug] TraceMusic=1` logs every track started (K130, K150), `[Debug] TraceAreaSpell=1` logs enemy casters' area-spell candidate distances (G241), `[Debug] TraceHitSound=1` logs where spell hit sounds play (K300), `[Debug] TraceTentacle=1` logs the experimental Tentacle Beast grab (G250), and the HUD's Solo button keeps the other party members out of a fight.
The inventory equips by drag only when the button is held about 0.4 s before the mouse moves (`down left; wait
400; move ...; up left`); `drag` is too fast. The console cheat `Give_all_items` does nothing in retail levels (only
the test level's script writes it, nothing reads it); add items with `savedit.mjs --add-item` instead.

Console commands need `postkey` with a `[ConsoleKeys]` binding (`key` goes through DirectInput and never reaches
the console keys). `[Debug] TestSaves=1` keeps test saves in `Summoner/savetest`. To reach a story level, bind
`play_and_load_cutscene <cutscene>` (cutscene names and levels are in `cutscene.tbl`); it sets `game_flags` bit
`0x8`, which returns to the main menu after the cutscene, so clear it while the cutscene plays
(`poke 0x5fbfd8` with the value minus 8) to keep playing. Starting a new game from the menu it returns to crashed
once (game-bugs C280). `TB-Intro` puts Joseph alone in the sewer boss room with the Iron Golem and two hostile
Broken Brass Golems (metal targets); `immortal` keeps him alive.

Coordinates are screenshot pixels, which are half the window size (1280x720 on a 2560x1440
screen). Key names are the ones `summonerfix.ini [KeyBindings]` uses.

**Useful positions** at 1280x720 screenshots: main menu Load Saved Game (858, 368), then
the load screen's Load (798, 368).

Set `Agent=0` again for normal play.

## `tools/savinfo.mjs`

Read-only savegame inspector, based on [Savegame format](sum-exe/savegame.md). It prints the header (level, description,
play time, section sizes), the gold and the party inventory, the party members from the `LENT` chunk (level,
experience, HP/AP, skill points and equipped items), the story flags that are set and the non-zero variables of each
level, and the active journal quests. `--all` also lists the other entity records, and `--flags-all` lists every level
and every flag, set or not.

Names come from the game tables, read at run time from `Summoner/tables.vpp` in memory (nothing is extracted):
item names from `items.tbl`, flag, variable and quest names from `events.tbl`, and the automap layer counts, which
the flag offsets depend on, from the `<level>.tbl` files. `--tables FILE.vpp` names another packfile, `--tables-dir DIR`
a folder of extracted `.tbl` files, and `--items-tbl FILE` / `--events-tbl FILE` single files; `--lang` picks the
language section (default `english`). Without `items.tbl` the items are shown by hash. The flags are shown only when
the tables reproduce the save's level-data size exactly (25,035 bytes in v1.40); otherwise savinfo says why.

`--json` prints everything as one JSON document for a GUI or a script: `{tool, format, tables, saves: [...]}`. Each
save has the header fields, `inventory` (gold and entries with name, hash, count), `party` (with all 36 skill bytes
and the equipment by slot), and `levels`: all 51 levels with every flag (0 or 1), int and float variable. The exit
code is 2 when a file could not be read or is not a v1.40 save.

```
node tools/savinfo.mjs Summoner/savegame/summoner7.sav
node tools/savinfo.mjs --json Summoner/savegame/summoner7.sav > save.json
```

## `tools/savedit.mjs`

Savegame editor. It **never overwrites a file**: it writes a new output file, refuses if that file exists or is the
input, and leaves the original untouched. Every edit is checked before anything is written, so one bad edit (an
unknown item, flag or level, a bad number) writes nothing. It can change the description, the party's gold, the
party inventory, story flags, and a party member's level, experience, HP/max HP, AP/max AP, skill points and skills.
On load the game recomputes only the skill cap from the level, so raising the level does not add the HP, AP or skill
points of the skipped levels. Only party members (record type 7, which dead heroes keep) are counted by `--member`;
type 8 is a dying summon, with no level, XP or skill block. The chunks are found from the size table, not by a tag
search (fixed 2026-10-15). Tested 2026-10-14: an edited copy (5000 gold, 10 skill points) loaded and showed both values.

Options are applied in command-line order (2026-10-16). `--skill`, `--add-item`, `--remove-item`, `--set-item-count`
and `--flag` may be repeated. `--member I` (an index from 0, or `Joseph`, `Flece`, `Jekhar`, `Rosalind`) selects the
party member for the field options after it, and may be given again for the next member. A field given twice for
the same member, a skill id given twice, or `--desc` / `--gold` given twice is refused instead of letting the last
value win.

**Items.** Items are found by `name_hash_nocase` (`0x4d5f50`): CRC-32 of the lower-cased name, table polynomial
`0xedb88320`, starting at 0 with no final inversion (checked against the Ring of Light and Ring of Darkness entries
of a fresh save). A name is accepted only if its hash is one of the item types in the save's own `ITEM` chunk, the
list the loader resolves against, so an item the game does not know (which would fail the load) is refused, with
similar names from `items.tbl` as hints. Case does not matter.

| Option | Effect |
|---|---|
| `--add-item "Name[=N]"` | one more inventory entry of N items (default 1). Tested: a save with `--add-item "Robe of Restoration"` loaded with the Robe in the party inventory |
| `--remove-item "Name[=N]"` | removes N of the item (default all), from the last entries first; refused if the party has fewer |
| `--set-item-count "Name=N"` | the party holds exactly N: the first entry gets N, other entries of the item are removed, 0 removes it, and an item the party lacks is added |

The entries, the count and the trailing length of `INVT` are rewritten together (at most 0x209 entries).

**Story flags.** `--flag LEVEL:NAME=0|1` clears or sets a flag; the level is a name from `level_script_table`
(case does not matter) or its index 0..50. A flag of another level is refused with the level it belongs to. The
flag is bit `i & 7` of byte `i >> 3` of the level's block, where i is the flag's position in that level's list
(`savegame_data_offset` `0x4cfbd0`, `savegame_set_flag` `0x4cfd30`). The block offsets come from `events.tbl` and the
automap layer counts, and the edit is refused unless they add up to the save's level-data size. Setting a flag to 1
also clears the journal bit of every quest whose `+Invalidate:` flag it is, as `quest_evaluate` (`0x4d0c90`) does in
game. Not done offline: the game's refresh of item objects and scripted characters tied to the flag; the item objects
are refreshed again on load (`savegame_read_level_items`), the characters are not (*inferred*).

The same table options as savinfo (`--tables`, `--tables-dir`, `--items-tbl`, `--events-tbl`, `--lang`) apply.

```
node tools/savedit.mjs Summoner/savegame/summoner7.sav edited.sav --gold 5000 --skillpoints 10 --desc "edited"
node tools/savedit.mjs in.sav out.sav --member Rosalind --skill 20=100 --skill 21=90 --member Joseph --hp 500
node tools/savedit.mjs in.sav out.sav --add-item "Healing Draught=5" --remove-item "Ring of Light" --flag masad:masad_flag_run_away=1
```

To use the result, copy it into `Summoner/savegame` under an unused name such as `summoner15.sav`.

The two tools share `tools/lib/savedata.mjs`, which reads the tables and the save into plain objects
(`loadTables`, `describeSave`, `levelBlocks`, the inventory and flag helpers) and never writes a file; a GUI can use it
directly.

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

## `tools/decomp-progress.mjs`

Counts which functions of each original source file have rewritten source in `src/decomp/` (by the `// 0xADDR` comments)
and their confidence (exact / likely / unsure). Since 2026-10-17 it also reads `src/decomp/vc6-matches.txt` and
shows, per file and in total, how many written functions VC6 compares and how many of those byte-match; on functions
VC6 does not compare, the marker is a reading of the code, not a compiler check. `--md` prints a Markdown table.

```
node tools/decomp-progress.mjs
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
