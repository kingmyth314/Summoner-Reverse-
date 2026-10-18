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

## Byte-matching workflow tools

Added 2026-10-17 (they were scratch scripts during the 2026-10-16 rounds). All need `VC6_DIR` set to the Service
Pack 5 `VC98` folder ([Setup](setup.md)) and run from PowerShell or cmd.

| Tool | What it does |
|---|---|
| `tools/vc6-sweep.mjs [--root DIR] [--out FILE] [--only REGEX] [--jobs N] [--check LIST]` | Runs `vc6-compare.mjs` on every unit of a tree (the repo's `src/decomp` by default, or a merge stage / worker copy with its own `include/` and `vc6-flags.txt`) and writes `<file> <address> <match\|differs\|not_found>` lines. `--check src/decomp/vc6-matches.txt` lists every address that matched there and no longer does, and exits 1 if there is one: the "no match lost" check before every commit. |
| `src/decomp/vc6-matches.txt` | The committed per-address result of the last full strict sweep (2026-10-17: 4,956 of 5,925 match, call and data targets included). Regenerate with `--out src/decomp/vc6-matches.txt` after a strict sweep that `--check` passed. |
| `tools/decomp-confidence.mjs [list] [--apply]` | Sets the `Confidence:` marker of every compared function from the sweep list: match -> `exact`, an `exact` that differs -> `likely`. Without `--apply` it lists the changes. Run after each committed sweep. |
| `tools/decomp-uncompared.mjs <list> [--list]` | Written functions (a sum.symbols address in a `// 0xADDR` comment) that the sweep never compared, by reason: inside another marker's comment block (mostly compiler-generated initialisers and destructors of globals), a note, or not a definition. |
| `tools/vc6-targets.mjs <sweep list>` | For a strict sweep list, every function marked `target` and which reference is wrong (symbol, the address or bytes ours expects, what Sum.exe has). |
| `tools/vc6-side.mjs <listing.cod> <name> <address>` | One function's VC6 output next to Sum.exe's disassembly, differing mnemonics marked `*`. The listing comes from `vc6-compare.mjs --keep` (or `--json`'s `tmp`). |
| `tools/vc6-try.mjs [--root DIR] <file> <address> <variants.json> [--side NAME]` | Tries source variants (`[{"name", "from", "to"}]`, exact text replacements) one at a time and reports whether the function then matches and the file's match count; the file is always restored. `--side` prints each failing variant's first differing lines. |
| `tools/vc6-flagsweep.mjs <root> <out.json> [dir]` | Compiles every unit of `dir` (default `vsdk`) with its flags, with `/O2` -> `/Ox /Ow` and with `/Ow` added, and records each match set. A unit switches only when the new set contains every old match (DECISIONS.md D7502). |
| `tools/decomp-merge.mjs <worker> <base> <stage>` | Three-way merge of a worker's copy of `src/decomp` into a stage: takes, merges (`git merge-file`) or adds each changed file; DECISIONS.md conflicts keep both appended blocks; other conflicts are listed (exit 1). |
| `tools/decomp-sigcheck.mjs [src/decomp]` | Free functions whose header declaration and definition differ in parameters (a silent overload). One known, accepted: `charinfo_load_move_anim`. |

The round workflow (2026-10-16): snapshot the tree for each worker, let it work in its own copy, merge it into a
stage with `decomp-merge.mjs` against its snapshot, compile everything with mingw
(`i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Iinclude`), run `decomp-sigcheck.mjs`, check for duplicate
`// 0xADDR` markers, run `vc6-sweep.mjs --root <stage> --check src/decomp/vc6-matches.txt` with `VC6_STRICT=1`, then copy the stage
into `src/decomp`, rewrite `vc6-matches.txt`, run `decomp-confidence.mjs --apply` and `decomp-progress.mjs`, and commit.

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
hit and counts those frames (C250), `[Debug] TraceBlessCount=1` logs the AI healer's Bless candidate count and `BlessPassAlways=1` makes that pass run in any fight (C210), `[Debug] TraceGroupAttack=1` logs the Support-style attack orders (G211), `[Debug] TraceSupportPick=1` logs AI support spell picks (G229), `[Debug] TraceDetectCone=1` logs the party AI's detection cone tests (G210), `[Debug] TraceMusic=1` logs every track started (K130, K150), `[Debug] TraceAreaSpell=1` logs enemy casters' area-spell candidate distances (G241), `[Debug] TraceHitSound=1` logs where spell hit sounds play (K300), and the HUD's Solo button keeps the other party members out of a fight.
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
