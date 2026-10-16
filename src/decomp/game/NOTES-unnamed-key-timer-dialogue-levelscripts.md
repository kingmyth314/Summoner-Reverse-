# decomp72: keyboard, debug / movement / timer / vif, dialogue table, cutscene scripts, eleven level scripts

Written from Sum.exe v1.40's disassembly only (`node tools/asm.mjs fn`; tables, floats and literals read from
`Summoner/Sum.exe`; import names from the PE import table). No Summoner 2 or Dash Faction code was read.
`include/` is the clean snapshot of 9147a03 plus the changes in CHANGED-HEADERS.txt; decisions D5300-D5317 are
appended to `include/DECISIONS.md`.

Check: `i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -I include <file>`: 0 lines with "error:" for each of the
19 new files and for every `.cpp` of `snap9147/src/decomp/game` and `/vsdk` against this include tree.

**Every sum.symbols function of the eleven ranges is written** (169) plus two hidden functions (0x4fa6f0,
0x5329c1): 171 address lines, 132 exact, 37 likely, 2 unsure. No address of the snapshot's `.cpp` files is written
again; no header address is declared twice.

| File | Original unit | Functions | Exact / likely / unsure |
|---|---|---|---|
| `vsdk/unnamed_key_0x5093a0.cpp` | keyboard (placeholder, D5313) | 26 | 20 / 6 / 0 |
| `vsdk/unnamed_debug_stubs_0x531c10.cpp` | _stricmp wrapper, debug hooks (placeholder, D5314) | 8 | 5 / 3 / 0 |
| `vsdk/debug.cpp` | `debug.cpp` (sum.symbols module) | 2 | 0 / 2 / 0 |
| `vsdk/unnamed_movement_0x531d20.cpp` | free movement / slew (placeholder, D5315) | 12 | 9 / 3 / 0 |
| `vsdk/unnamed_time_format_0x532660.cpp` | time_format_hms (placeholder, D5316) | 1 | 0 / 1 / 0 |
| `vsdk/timer.cpp` | `timer.cpp` (sum.symbols module) | 4 + 1 hidden | 2 / 3 / 0 |
| `vsdk/unnamed_vif_0x5329f0.cpp` | vif particles (placeholder, D5317) | 17 | 7 / 9 / 1 |
| `game/unnamed_dialogue_table_0x4feb50.cpp` | conversation records' members, `_dlg.tbl` parser (placeholder, D5310) | 21 | 20 / 0 / 1 |
| `game/unnamed_cutscene_script_0x4ed050.cpp` | `_cscript.tbl` cutscene scripts (placeholder, D5306) | 14 | 10 / 4 / 0 |
| `game/level_script_lenele2d.cpp` | record "lenele2d" (name by convention, D5300) | 4 | 4 / 0 / 0 |
| `game/unnamed_rand_levels_0x4ee840.cpp` | records 0x28-0x2f, random encounters (placeholder, D5300) | 8 | 7 / 1 / 0 |
| `game/level_script_lenele2ab.cpp` | "lenele2ab" (D5300) | 6 | 6 / 0 / 0 |
| `game/level_script_rand_forestnite1.cpp` | "rand-forestnite1" (D5300, D5301) | 5 | 4 / 1 / 0 |
| `game/level_script_rand_forest01.cpp` | "rand-forest01" (D5300, D5301) | 6 | 5 / 1 / 0 |
| `game/level_script_ionaext.cpp` | "IonaExt" (D5300) | 12 | 12 / 0 / 0 |
| `game/level_script_ikaemosbottomint.cpp` | "IkaemosBottomInt" (D5300, D5303) | 1 | 1 / 0 / 0 |
| `game/level_script_rand_hills01.cpp` | "rand-hills01" (D5300) | 6 + 1 hidden | 6 / 1 / 0 |
| `game/level_script_masad.cpp` | "masad" (D5300, D5301) | 13 | 12 / 1 / 0 |
| `game/level_script_test.cpp` | "test" (D5300, D5301) | 3 | 2 / 1 / 0 |
| **total** | | **171** | **132 / 37 / 2** |

None of these ranges continues an existing unit, so there are no `-additions.cpp` files. `debug.cpp` and
`timer.cpp` are new files under names the module column already gives; all other file names are placeholders or
follow the level_script_<record>.cpp convention (D5300); modules.txt is empty.

## Per unit

**Keyboard (0x5093a0-0x509ffb).** key_mark turns the window messages into states, counters, held times and the
16-entry queue (modifier bits 0x1000 / 0x2000 / 0x4000, 0x800 for ` with the debug modifier); key_remap_layout's four
layouts (1 UK backslash, 2 AZERTY, 3 QWERTZ, 4 scan code 0 -> A); the Windows 9x NumLock fix; key_names and the
ASCII tables are this unit's .data.
- Bug (latent, sure): key_init calls key_flush before setting key_inited (0x5097b5), so the flush returns at once;
  the tables are still the zeroed .bss, so nothing shows.
- Bug (latent, sure; no caller): key_putkey (0x509d40) does not check for a full queue; the 17th key makes the tail
  meet the head and the queue reads as empty.
- Oddity: layout 4 maps scan code 0 (no key) to A (0x509582); probably meant another code.
- key_getch, key_clear, key_get_release_count, key_putkey and stricmp_wrapper have no caller.

**Debug stubs, debug.cpp (0x531c10-0x531d16).** The _stricmp wrapper, two unreferenced bare RETs, the compiled-out
hooks (net rate, mp frame, frame end, cleanup, disp rate), then DebugError (message box, exit(1)) and its
gr_deactivate helper. The empty hooks are not folded into empty_function.

**Free movement (0x531d20-0x532353).** The debug camera's slew: number pad / A / Z move, keypad 8 2 6 4 7 9 turn,
shift x5, ctrl x0.1 (turns x0.25) while movement_vel_change is on; exponential smoothing (smooth_approach_exp with
the inline exp). No bug found.

**Timer (0x532660-0x5329d1).** time_format_hms is dead code. timer.cpp: the QPC clock keeps only the low dword of the
frequency (the known ">4 GHz" truncation); timer_get's __asm reduces the high dword by repeated subtraction, so a
frequency whose low dword is 0 would hang it (latent), and only the low 32 bits of the quotient come back (the
millisecond clock wraps after 49.7 days). The TSC calibration reads the counter three times 250 ms apart and drops
the middle reading.

**Vif particles (0x5329f0-0x533dae).** See the unit header. Findings:
- Bug (live when an interval curve has two non-zero keys; sure about the code, the visible effect not checked):
  vif_curve__evaluate's reciprocal mode (0x533d20-0x533d60), used for the spawn interval curve, computes the
  fraction as (t - 1/v0) / (t1 - t0) instead of (t - t0) / (t1 - t0): the stack slot it subtracts holds the
  reciprocal it just made. The interval between two keys is therefore wrong (and can leave the key range).
- Bug (live, fairly sure): a level effect's particles come from operator new (0x5337ed) and vif_record_release
  (0x5339b0) only takes their size off vif_data_top and clears the pointer; neither it nor
  vfx_instance::release_nodes (0x5369f0) frees them: every level unload leaks count * 48 bytes per level vif record.
- vif_min_nonzero_float and vif_stub_ret_533c60 have no caller.

**Dialogue table (0x4feb50-0x500a22).** The speaker / topic / line / action members and conversation::parse_table.
- Bug (latent, sure): parse_table has no return value on success (0x500856-0x5008c0); it returns 0 only because the
  TextFile destructor's Close leaves the NULL buffer in EAX (conversation::load tests for 0).
- Bug (live if the tables use it; sure about the code): an +NPCText: line naming a speaker not seen before
  (0x4ff3d0-0x4ff418) pools the name but leaves the line's who NULL; only names already known are used.
- Bug (live if the tables use it; sure): "+Set important :" / "+Clear important :" naming a speaker defined later
  in the file is only checked at the end (0x500835-0x500854); the reference's speaker stays NULL.
- The quest names of +Hide quest: / +Unhide quest: are trimmed and lose their first and last character (the
  braces); "Illegal quest name" is printed only while savegame_state.used > 0.

**Cutscene scripts (0x4ed050-0x4ed7be).** _cscript.tbl load / unload / rewind / frame and the data they own (D5306).
- Bug (latent, sure): cutscene_script_load counts navpoints until #Objects with no check against the 0x200 slots
  (0x4ed281-0x4ed2b7); a longer table overwrites the string pool behind them.
- unload deletes the cutscene-only actors (start navpoint type 1, not a party member) through the obj vtable.

**Level scripts.**
- lenele2d: Pedrog's greeting and lead, the lenele2aa gate refused (id 502 shared with lenele2aa).
- Random encounters: the music of the seven maps; Rand-Orenia01 / -OreniaNite01 _v3 remove Ahlang / Death after
  their quests; Rand-Grassland01 builds the US Open demo party and, on _v3, keeps one of three encounters (Korel,
  Zane's bandits, the merchant). Latent bug (demo only): the two weapon swaps check only the first displaced item
  (0x4ee9ed, 0x4eea8e).
- lenele2ab: Adeslani's escort, Canis's payoff (100 gold, clamped at 0 without a check).
- rand-forestnite1 / rand-forest01 / rand-hills01: the Ghost, Serpent and Phoenix Rider fights on the _v2 scripts;
  forest01 _v6 places the Tome / Gesualdo / King of Talas / Crone NPCs; forest01 "EnterTome" goes to jadetemple_v2,
  hills01 "GotoKhosani" to khosanistrng. Latent: ghost_find_rider / phoenix_find_rider results are not checked for
  NULL (0x4f583e, 0x4fa5fe; forest01 checks serpent_get_rider), a crash if the rider is gone before its flag is set.
- IonaExt: Rosalind joins (level 6, weapon paged in), the aspirants' camera scenes, the lift (the Eleh lift's code).
  "GoHome" is set by the action and cleared by the frame with no other effect (0x4f729f).
- IkaemosBottomInt: music and stage 5 only.
- masad: the game start (Tunic, Game-Pre-Intro / Game-Intro, Nath, the barbarian), the tutorial fire doors and the
  three invisible doors. Observation: masad-firedoor1.vfx is switched off unconditionally (0x4fd8cd) and again after
  the dialogue tutorial, and the combat tutorial flag is tested twice. Latent: lsf_masad_intro drops what the armour
  slot held when it equips the Tunic (0x4fdad3).
- test: a developer map (Max_skills / All_summons / Give_all_items, "Testing" over James Agay every frame).

## Disagreements with the docs and sum.symbols

- source-map.md lists rand-forestnite1 + rand-forest01 and masad + test as one run each; the .bss steps make them two
  units each (D5301).
- sum.symbols sizes cutscene_navpoints uint8[0x8818]; it is 0x6000 (0x200 records), the string pool lies inside the
  old size (D5308). level_scripts.h's MAX_CUTSCENE_NAVPOINTS changed from 0x2d6 to 0x200.
- sum.symbols key_ascii_table is int[127]; both tables are int[128] (the shifted one at 0x5a66c0).
- sum.symbols movement_velocity / movement_rotvel: the first is the wanted velocity, the second the smoothed linear
  velocity (not a rotation); smooth_approach_exp's comment swaps its second and third arguments (D5315).
- sum.symbols conversation_speaker__refresh_topic_flags "except the greeting": the greeting is offered too (greeting
  or initial -> listed | fresh, else cleared).
- sum.symbols vif_emitter_spawn_point "(out, record)": the arguments are (out, &desc->path, time); docs/sum-exe/
  effects.md's "shape byte +0xc0" is the spawn path's point count (desc +0x218).
- sum.symbols timer_tsc_ticks "never read": 0x5329c1 reads it (that function has no caller).
- level_scripts.h declared cutscene_script_load void (it returns bool, D5307); key.h declared key_init's second
  argument int (bool, D5312).

## Left

Nothing in the eleven ranges. Unsure: vif_emitter_update (the source shape of the spawn arithmetic and the
locals) and conversation::parse_table (which tests were inline helpers). Not proven: every file name except
debug.cpp and timer.cpp; whether time_format_hms belongs to timer.cpp, the debug stubs to debug.cpp,
IkaemosBottomInt to ikaemosu.cpp / rand-hills01, and the random-encounter records to lenele2d.
