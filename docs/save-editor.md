---
title: Save editor
nav_order: 6.46
---

# Summoner Save Editor (`SummonerSaveEditor.exe`)

**Status (2026-10-21):** built and tested without the game. The engine gives the same bytes as
`tools/savedit.mjs` on 41 test saves, and the window was driven through window messages on copies of saves. A
person has not yet clicked through it, and no save it wrote has been loaded in the game yet.

A window for players to change a save: description, party gold, each party member's level, experience, HP, AP,
skill points and skills, the party inventory, and story flags. **It always writes a new save file and never
overwrites one**: the engine opens its output with `CREATE_NEW`, and the opened save is refused as a target.

{: .warning }
With Windows 11's Smart App Control on, Windows may refuse to start this unsigned program (on the development PC
it blocked a fresh build on 2026-10-21; see [SummonerFix](summonerfix/index.md)).

Build: `tools/build-saveeditor.ps1` (LLVM-MinGW, as SummonerFix) into `build/saveeditor/`: `SummonerSaveEditor.exe`
(the window), `saveedit.exe` (the same command line as `tools/savedit.mjs`) and `app_test.exe`. Source:
`src/saveeditor/` (the library `saveedit.c` / `.h`, the window's model and the app; the library's API and
design are in `src/saveeditor/NOTES.md`).

## Use

Copy `SummonerSaveEditor.exe` into the game folder, next to `Sum.exe`. It uses:

- `savegame\` in its folder: the Saves page lists every `*.sav` there, newest first (file, description,
  level, date). Double-click or **Open**; **Open other file...** for a save anywhere else.
- `tables.vpp` in its folder: names of items and story flags. Without it the Saves page says so; description,
  gold and the party can still be edited, item counts cannot (an item is named for the library by its name),
  and story flags are not offered (the library needs `events.tbl` to place them).

Command line (optional, mainly for tests): `--game-dir DIR` (instead of the program's folder), `--tables
FILE.vpp` (instead of `tables.vpp` in the game folder), `--open FILE.SAV`.

Pages:

- **Description and gold**: the description shown on the Save/Load screens (up to 255 Latin-1 characters;
  others are refused with a note in the status line) and the party gold.
- **Party**: one entry per party member (type-7 record, named Joseph, Flece, Jekhar or Rosalind when the hash
  matches). Level, XP, HP, max HP, AP, max AP, skill points (disabled when the record has no skill-point
  dword) and the 36 saved skill bytes. The library knows no skill names (they are strings of `Sum.exe`, not of
  the tables), so skills are shown by id (`skill 0x03 (3)`); pick one, type 0-255, **Set**. 10 = level 1.
- **Inventory**: one row per item type held (counts of repeated entries added up), with **Set count** and
  **Remove**; **Add** takes a name from the save's own item types (the ITEM chunk; the combo box can be typed
  into, the name must match one of the list). Rows show "added", "changed" or "removed". Items without a name
  in the tables are shown by hash and cannot be changed.
- **Story flags**: the levels that have flags (the save's level is selected first) and their flags with
  check boxes, named as in `events.tbl` (`flag N` when a name is empty). A flag whose name is empty, has
  leading or trailing blanks, or repeats an earlier flag's name in its level cannot be named for the library
  and is marked "cannot be changed". Setting a flag that is a quest's `+Invalidate:` flag also removes that
  quest from the journal (the library does it, as the game does).

Status line: the number of changes not saved yet. **Undo all changes** goes back to the save's values.
Closing or opening another save with unsaved changes asks first.

Saving:

- **Save as summonerN.sav**: the first `summonerN.sav` (N = 0, 1, 2 ...) that does not exist in the folder of
  the opened save. The game names its slots `summoner%d.sav` plus `quicksave.sav`
  (`docs/sum-exe/savegame.md`, "Saving and loading").
- **Save As...**: any new name. An existing file, or the opened file, is refused (never overwritten).
- Every change becomes one edit of the library's list, in this order: `desc`, `gold`, then for each changed
  member `member I` and its fields and `skill ID=VALUE`, then `set-item-count Name=N` for each item type whose
  total changed (the save's rows in order, then new rows), then `flag LEVELINDEX:NAME=0|1`. A field is sent
  as the text typed, so the library's checks and messages apply (`--gold needs a whole number ...`).
  `se_apply` applies the list all or nothing on a copy; on a refusal the message box shows the library's
  message and nothing is written. On success the message box shows where the file went and the library's
  change log, and the editor opens the new save (so the next save goes to the next slot).

## Tests

All three run without the game, on a folder of **copies** of saves (never a game's `savegame` folder):

1. `node tools/saveedit-test.mjs --corpus DIR`: (a) a round trip, load and write with no edit, must give a
   byte-identical file for every save; (b) a differential test: 71 cases (description, gold, items, flags by name
   and by index, every member field, skills, combined edits, the table options, and refusals) run with the Node
   engine and with `saveedit.exe` on every save, and the outputs must be byte-identical. Results on 41 test saves:
   41/41 round trips, 2,813 runs, 0 failures.
2. `build/saveeditor/app_test.exe --tables Summoner/tables.vpp --out DIR SAVES...`: the window's own "changes to
   edit list" code against `tools/savedit.mjs` with the same edits: 398 pass, 0 fail.
3. `tools/saveeditor-uitest.ps1 -Corpus DIR`: starts the built window on copies and drives it with window messages
   (no mouse): seven changes across four pages, saved to the offered slot and compared with `savedit.mjs`; a
   refused value writes nothing; it closes cleanly. It passed on 2026-10-21 from the first build; from the
   repository's build Smart App Control refused to start the program, so it waits for that decision.

## Still needs a person


- Real mouse and keyboard use: clicking check boxes (the test sets the state image by message), typing in the
  item combo box, Tab order, Enter in the edit boxes, scrolling the long lists.
- The two file dialogs (Save As..., Open other file...) and the Save As refusals (existing file, the opened
  file); the Undo button's question; switching party members in the list box and editing a second member's
  fields by hand; Remove on the Inventory page.
- High DPI: the app is not DPI-aware, so Windows scales it (blurry at 150 %, but laid out right).
- Loading a save written by the app in the game (back up the savegame folder first; the new file goes next to the opened save): the bytes are
  those of `savedit.mjs` for the same edits, but the app's own files have not been loaded in the game.
