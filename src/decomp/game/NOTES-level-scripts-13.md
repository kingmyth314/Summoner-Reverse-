# Phase 3: thirteen more level script units (decomp46)

`game/level_script_temple_inside.cpp`, `_slums`, `_iona_ext2`, `_ikaemosu`, `_ikaemosl2`, `_courtyard`,
`_lenele2aa`, `_lenele1aa`, `_lpalaceint02`, `_eleh`, `_wolong`, `_lenele3d` and `_lenele3a` are the
levelscripts\Scripts units of the module column of sum.symbols, written from Sum.exe v1.40's disassembly only
(`node tools/asm.mjs fn`; the level_script_table records, the float constants and the short trigger names read
from the raw bytes of `Summoner/Sum.exe`). No Summoner 2 or Dash Faction code was read. The retail tables
(tables.vpp: events.tbl, lenele3a.tbl, lenele1b_dlg.tbl, Wolong_dlg.tbl, TempleInt_v2_script.tbl) were read
only to judge the story-logic findings below.

Each file passes `i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include
<file>` with no diagnostics. `include/` is the repo's tree at a2d447c (the repo moved twice while this batch
was written: 09e45e3 and a2d447c) plus CHANGED-HEADERS.txt; `node reapply.mjs <include dir>` re-applies the
additions to a fresh copy (skips what is there). Every existing `src/decomp/game/*.cpp` and
`src/decomp/vsdk/*.cpp` (87 files) gives the same diagnostics with this tree as with the repo's. Decisions
D2700-D2706 are appended to `include/DECISIONS.md`. No declared signature changed.

**Every sum.symbols address of the 13 modules is written**, plus 7 untagged functions that belong to them
(modules.txt): 86 function comment lines, 80 exact, 6 likely, 0 unsure.

| File | Functions | Exact | Likely | Unsure |
|---|---|---|---|---|
| level_script_temple_inside.cpp | 10 | 9 | 1 | 0 |
| level_script_slums.cpp | 7 | 7 | 0 | 0 |
| level_script_iona_ext2.cpp | 7 | 6 | 1 | 0 |
| level_script_ikaemosu.cpp | 7 | 7 | 0 | 0 |
| level_script_ikaemosl2.cpp | 7 | 7 | 0 | 0 |
| level_script_courtyard.cpp | 11 | 10 | 1 | 0 |
| level_script_lenele2aa.cpp | 7 | 6 | 1 | 0 |
| level_script_lenele1aa.cpp | 6 | 5 | 1 | 0 |
| level_script_lpalaceint02.cpp | 5 | 5 | 0 | 0 |
| level_script_eleh.cpp | 6 | 6 | 0 | 0 |
| level_script_wolong.cpp | 4 | 4 | 0 | 0 |
| level_script_lenele3d.cpp | 6 | 5 | 1 | 0 |
| level_script_lenele3a.cpp | 3 | 3 | 0 | 0 |
| **total** | **86** | **80** | **6** | **0** |

## Scheme

As decomp34 / decomp43 (D1400): no unit header; the helpers are declared at the top of each .cpp, the
table hooks come from levelscripts/level_scripts.h. Functions in address order. The eleh timer is a
function-local static (D2405 pattern: guard byte 0x257e9e4, atexit 0x4f1880). Constant level indices are
per-unit #defines (D2407, D2705). Empty hooks of the table (frame / leave / action slots holding 0x4f13e0,
use slots holding ls_test_use 0x4f29c0) have no code of their own and are not written, as in the earlier
units; the two folded copies that lie inside these units are (D2700 ls_ionaext02_leave in lenele3d, D2701
empty_function in lenele2aa).

## Untagged functions included (modules.txt has the evidence)

| Address | Function | File |
|---|---|---|
| 0x4f0f00 | ls_ionaext02_leave (lenele3d's copy of the folded leave hook) | level_script_lenele3d.cpp |
| 0x4f13e0 | empty_function | level_script_lenele2aa.cpp |
| 0x4f1880 | atexit_nop_4f1880 | level_script_eleh.cpp |
| 0x4fa890 / 0x4fa970 / 0x4fa9d0 | ls_lenele1d_init / _frame / _use | level_script_courtyard.cpp |
| 0x4fab90 | lsf_lenele1c_flag_found_flece (not in sum.symbols; name ours) | level_script_courtyard.cpp |

Not included: 0x4ef120 / 0x4ef140 ls_ikaemosext2_init / _frame (between ikaemosl2 and templeint2, but their
own record "IkaemosExt2"; nothing ties them to ikaemosl2), and the other untagged records between these units
(lenele1ab 0x4ef850, lenele2ab 0x4f1070.., khosanilab2 0x4f2250.., tancredhouse 0x4f2830.., khosanistrng
0x4f3a80.., rand_desert 0x4f4910, rand_forest* 0x4f5670.., ikaemosbottomint 0x4fa400, rand_hills01
0x4fa420..): each is its own level and most likely its own .cpp.

## Function tables

### level_script_temple_inside.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4f5e90 | ls_templeint_init | likely |
| 0x4f5f80 | ls_templeint_frame | exact |
| 0x4f6290 | ls_templeint_urathmyth_cs_end | exact |
| 0x4f62b0 | ls_templeint_tiger_intro_cs_end | exact |
| 0x4f62c0 | ls_templeint_tiger_dismount_cs_end | exact |
| 0x4f62d0 | ls_templeint_tiger_escape_cs_end | exact |
| 0x4f6340 | ls_templeint_act2_outro_cs_end | exact |
| 0x4f64b0 | ls_templeint_eavesdropping_cs_end | exact |
| 0x4f64f0 | ls_templeint_action | exact |
| 0x4f65e0 | ls_templeint_use | exact |

### level_script_slums.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4fad90 | ls_lenele1b_init | exact |
| 0x4faf90 | ls_lenele1b_frame | exact |
| 0x4fafa0 | lsf_lenele_slums_flag_prost1_speaks | exact |
| 0x4fb000 | lsf_lenele_slums_flag_prost2_speaks | exact |
| 0x4fb060 | lsf_lenele_slums_flag_ken_speaks | exact |
| 0x4fb0c0 | ls_lenele1b_use | exact |
| 0x4fb160 | ls_lenele1b_action | exact |

### level_script_iona_ext2.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4f4550 | static_init_ls_ionaext02_timer | exact |
| 0x4f4560 | init_ls_ionaext02_timer | exact |
| 0x4f4570 | ls_ionaext02_init | likely |
| 0x4f4650 | ls_ionaext02_frame | exact |
| 0x4f4860 | ls_ionaext02_luminar_intro_cs_end | exact |
| 0x4f4890 | ls_ionaext02_luminar_death_cs_end | exact |
| 0x4f48d0 | ls_ionaext02_rosalind_remark | exact |

### level_script_ikaemosu.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4f9ec0 | ls_ikaemostopint_init | exact |
| 0x4f9f50 | ls_ikaemostopint_frame | exact |
| 0x4f9fa0 | ls_ikaemostopint_main_door | exact |
| 0x4fa050 | lsf_ikaemos_found_boss | exact |
| 0x4fa110 | ls_ikaemos_pking_intro_cs_end | exact |
| 0x4fa140 | lsf_ikaemos_boss_killed | exact |
| 0x4fa230 | ls_ikaemostopint_use | exact |

### level_script_ikaemosl2.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4eee40 | static_init_ls_ikaemosbottomint2_timer | exact |
| 0x4eee50 | init_ls_ikaemosbottomint2_timer | exact |
| 0x4eee60 | ls_ikaemosbottomint2_init | exact |
| 0x4eeef0 | ls_ikaemosbottomint2_frame | exact |
| 0x4ef090 | ls_ikaemosbottomint2_titus_intro_cs_end | exact |
| 0x4ef0c0 | ls_ikaemosbottomint2_titus_death_cs_end | exact |
| 0x4ef0e0 | ls_ikaemosbottomint2_jekhar_remark | exact |

### level_script_courtyard.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4fa7c0 | ls_lenele1e_init | exact |
| 0x4fa850 | ls_lenele1e_use | exact |
| 0x4fa890 | ls_lenele1d_init | exact |
| 0x4fa970 | ls_lenele1d_frame | exact |
| 0x4fa9d0 | ls_lenele1d_use | exact |
| 0x4faa60 | ls_lenele1c_init | exact |
| 0x4fab80 | ls_lenele1c_frame | exact |
| 0x4fab90 | lsf_lenele1c_flag_found_flece | likely |
| 0x4faca0 | ls_lenele1c_flece_intro_cs_end | exact |
| 0x4facc0 | ls_lenele1c_flece_intro_p2_cs_end | exact |
| 0x4fad50 | ls_lenele1c_use | exact |

### level_script_lenele2aa.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4f12c0 | ls_lenele2aa_init | exact |
| 0x4f1310 | ls_lenele2aa_frame | exact |
| 0x4f1320 | lsf_lenele2aa_flag_victim_dead | exact |
| 0x4f1380 | lsf_lenele2aa_flag_vengence_done | exact |
| 0x4f13e0 | empty_function | likely |
| 0x4f13f0 | ls_lenele2aa_action | exact |
| 0x4f1420 | ls_lenele2aa_use | exact |

### level_script_lenele1aa.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4efa30 | ls_lenele1aa_init | likely |
| 0x4efdb0 | ls_lenele1aa_frame | exact |
| 0x4efde0 | lsf_lenele_flag_cart_played | exact |
| 0x4efe80 | lsf_lenele1aa_jekhar_talk | exact |
| 0x4efef0 | ls_lenele1aa_use | exact |
| 0x4eff30 | ls_lenele1aa_action | exact |

### level_script_lpalaceint02.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4f2450 | ls_lpalaceint02_init | exact |
| 0x4f2550 | ls_lpalaceint02_frame | exact |
| 0x4f27a0 | ls_lpalaceint02_sg_intro_cs_end | exact |
| 0x4f27c0 | ls_lpalaceint02_leave | exact |
| 0x4f27f0 | ls_lpalaceint02_use | exact |

### level_script_eleh.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4f1490 | ls_eleh_init | exact |
| 0x4f1560 | ls_eleh_frame | exact |
| 0x4f16e0 | ls_eleh_fade_place_party_frame | exact |
| 0x4f1880 | atexit_nop_4f1880 | exact |
| 0x4f1890 | ls_eleh_action | exact |
| 0x4f1910 | ls_eleh_use | exact |

### level_script_wolong.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4fe8f0 | ls_wolong_init | exact |
| 0x4fe9a0 | ls_wolong_frame | exact |
| 0x4fea40 | ls_wolong_use | exact |
| 0x4fea80 | ls_wolong_action | exact |

### level_script_lenele3d.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4f0d80 | static_init_ls_lenele3d_timer | exact |
| 0x4f0d90 | init_ls_lenele3d_timer | exact |
| 0x4f0da0 | ls_lenele3d_init | exact |
| 0x4f0eb0 | ls_lenele3d_frame | likely |
| 0x4f0f00 | ls_ionaext02_leave | exact |
| 0x4f0f10 | ls_lenele3d_use | exact |

### level_script_lenele3a.cpp

| Address | Function | Confidence |
|---|---|---|
| 0x4f0f70 | ls_lenele3a_init | exact |
| 0x4f1000 | ls_lenele3a_leave | exact |
| 0x4f1030 | ls_lenele3a_use | exact |

## Data

| Unit | .bss | .data |
|---|---|---|
| ikaemosl2 | 0x257e9a8-0x257e9b0 (the timer at 0x257e9ac) | literals only |
| lenele3d | 0x257e9d0 timer, 0x257e9dc ls_lenele3d_unused_9dc (int, name ours, written 0 and never read); 0x257e9d4-0x257e9db is referenced by nothing | literals only |
| eleh | 0x257e9e4 guard, 0x257e9e8 static timer, 0x257e9ec-0x257e9ef the four choices, 0x257e9f0 state | literals only |
| lpalaceint02 | 0x257e9fc-0x257e9fd | literals only |
| iona_ext2 | 0x257ea58-0x257ea5d | literals only |
| temple_inside | 0x257ea6c-0x257ea76 | literals only |
| ikaemosu | 0x257ecfc-0x257ecfe | literals only |
| wolong | 0x257ed7c | literals only |
| lenele1aa, lenele3a, lenele2aa, courtyard, slums | none | literals only |

## Open questions

- ls_lenele3d_frame ends with `jmp game_stage_get` whose result nobody uses (0x4f0efa): written as a bare
  `game_stage_get();` (likely), probably an emptied `if (game_stage_get() ...)` of the original.
- ls_lenele3d_timer is set to 1 s by ls_lenele3d_init and never read; 0x257e9dc is written 0 and never read.
  Both look like the remains of a removed lenele3d sequence.
- empty_function: which empty function of lenele2aa.cpp the linker kept (its leave slot is the best guess).
- ls_ionaext02_init: the 20 s SetTimeout at the end of both trapped_luminar paths is written once (tail
  duplication assumed); ls_templeint_init: the door call after both music presets, the same.
- lsf_lenele1c_flag_found_flece: the order of the flece_joined store and the cutscene call is not visible.
- ls_lenele1aa_init: whether the first-visit early return (Jekhar talked or the ring held: no stage and no
  triggers at all) is a return in the source or an else block around the rest.

## Disagreements with docs and symbols

- sum.symbols ls_eleh_go_ground / go_bottom / go_other / go_top (0x257e9ec-ef): shifted by one choice. The
  action sets 9ec for "TopFloor", 9ed "GroundFloor", 9ee "BottomFloor", 9ef "JoinJoseph"; the fade maps 9ec to
  entry 5, 9ed to 3, else 4 (D2703). Proposed renames: go_top -> ls_eleh_join_joseph, go_ground -> go_top,
  go_bottom -> go_ground, go_other -> go_bottom. ls_eleh_cutscene_timer's comment still names
  "ls_eleh_cutscene_frame" (now ls_eleh_fade_place_party_frame).
- sum.symbols init_ls_ikaemosbottomint2_timer "used by ... ls_rand_hillsnite01_init": no, only the
  ikaemosbottomint2 functions touch 0x257e9ac (asm grep). init_ls_ionaext02_timer "used by ... and
  ls_khosanistrng_action": no, only ls_ionaext02_*. init_ls_jadetemple_timers "used by ...
  ls_lenele1aa_action": no (lenele1aa has no timer).
- sum.symbols ls_ionaext02_leave "shared with ls_lenele3d_leave": the code lies in lenele3d's range, so it is
  lenele3d's copy (D2700); ls_lenele3d_leave would be the better name.
- sum.symbols ls_templeint_urathmyth_cs_end "six UrathMyth1a..2b cutscenes (8 pushes)": eight, 1a-1d and 2a-2d.
- sum.symbols ls_ikaemostopint_found_boss_done / _boss_killed_done "set when ... starts the Boss music /
  restores the level music": ls_ikaemostopint_init sets both to 1 on every entry, so neither branch runs
  (bug 1).
- sum.symbols camera_cut_to_view_stub "17.0, 4.0, 4.0": lsf_lenele_flag_cart_played passes 17.0, 2.0, 2.0.
- sum.symbols: 0x4fab90 has no entry (lsf_lenele1c_flag_found_flece); ls_lenele1d_* are module - but belong
  to level_script_courtyard.cpp (D2704).
- level-scripts.md record table, `use`: "It returns 0". ls_ikaemostopint_use returns 1 for the four slots and
  the statue / box, ls_lenele1b_use returns 1 for the sewer grate bar.
- level-scripts.md record table, `+0x14`: "0 in every record". Record 0xf IonaExt holds 0x4f29c0 (ls_test_use)
  there (level_scripts.h already says so).
- bosses.md level-script table, Tiger Rider row: the tiger code of ls_templeint_frame runs only on
  `templeint_v2` (`_stricmp(level_script_name, "templeint_v2")`, 0x4f6091); Tiger-Intro at $tigertrigger (5),
  Dismount when tiger_dismount_stage_reached == 1, Escape when tiger_find_rider()->hp <= 0 (unchecked).
- bosses.md Titus row: Titus-Intro plays on the first frame (no navpoint), titus_intro is cleared by every
  entry until trapped_titus (the intro replays), Titus-Death needs the Ring of Stone in the party, and without
  it Jekhar says "He's immune to our attacks!" every 25 s. Luminar row: the same with the Ring of Light,
  Luminar-Intro within 20 of Luminar, only meaningful on Ionaext02_v2, Rosalind's "We're not hurting him at
  all!" every 25 s.
- bosses.md: no row for the Ikaemos Priest King (IkaemosTopInt: IkmsPKing-Intro at $ipktrigger (3) once the
  four dagger slots have opened Door02-01; his death with the Ring of Stone gives got_ring_of_stone and Summon
  Golem) or for Sornehan and Galienne (LPalaceInt02: SG-Intro when Sornehan is within 10; bothdead needs both
  deaths and the Khosani Scrolls).
- worldmap.md: nothing in these units disagrees (the Iona row's `ionaext02_v2` is compared case-insensitively
  as "Ionaext02_v2").

## Game bugs and oddities (story logic)

1. **IkaemosTopInt: the Priest King's boss music never plays.** ls_ikaemostopint_init sets found_boss_done and
   boss_killed_done to 1 unconditionally (`mov ebx, 1` at 0x4f9ef1, stored at 0x4f9f2e), so
   lsf_ikaemos_found_boss never starts "Boss" and lsf_ikaemos_boss_killed never restores the level music.
   Same kind as the Liangshan bug (K130). Intended were presets from the saved flags (as lpalaceint02 /
   templeint do).
2. **lenele3a: the temple refusal is never said.** ls_lenele3a_use tests "templeint" (0x4f1034), but the
   script-use trigger is "templeint2" (lenele3a.tbl, no +RefName:), so before stage 21 the temple exit is
   blocked silently instead of Joseph saying "We're not ready to confront Machival yet."
3. **lenele1b: Gorbus's release order topic flips on other actions.** ls_lenele1b_action's CheckReleaseOrder
   has an else (0x4fb227) that calls script_topic_toggle_by_item (not _inv) for "Give him the release
   order.": every other lenele1b action makes that topic available when the party lacks the Forged Document and
   hides it when the party has it. The "Give him the release order." topic itself runs CheckGold
   (lenele1b_dlg.tbl), so after "From the palace." the topic is gone while the document is still held, until the
   next Hail (CheckReleaseOrder) restores it.
4. **Titus and Luminar intros replay.** ls_ikaemosbottomint2_init and ls_ionaext02_init (v2) clear titus_intro /
   luminar_intro on every entry until the boss is trapped, so re-entering or loading a save replays
   Titus-Intro / Luminar-Intro. Titus-Intro starts on the first frame wherever the party stands.
5. **LPalaceInt02: after a reload the world map exit is open without the scrolls.** ls_lpalaceint02_init closes
   worldmap1 only while sornehan_dead or galienne_dead is missing; the frame's bothdead (which opens the exit)
   also needs got_scrolls. A save made after both deaths but before taking the scrolls loads with a live exit,
   and ls_lpalaceint02_use's "We're not leaving without the Khosani scrolls." is no longer reached.
6. **IkaemosTopInt: the dagger slots test no dagger.** ls_ikaemostopint_use sets ikaemos_slotN on any use of
   Puzzle-SlotN; a second use says "We've unlocked it already." and sets the flag again. Whether the slot
   objects are usable only with a dagger (object data) was not checked.
7. **TempleInt_v2: the Tiger Rider lookup is unchecked.** `tiger_find_rider()->hp` (0x4f6244) reads through
   NULL while tiger_killed is unset if the rider is not in the level; a latent crash like C130.
8. **Flece joins unchecked.** lsf_lenele1c_flag_found_flece passes party_member_join's result straight to
   character_equip_item (NULL when a load is pending or the party is full, D2203). Eleh's JoinJoseph and
   Wolong's AddRosalind use party_add_member's result unchecked (like wolong2, C130).
9. **Act2-Outro debug output.** ls_templeint_act2_outro_cs_end prints "Switching char." / "No character!" with
   printf for every party member (no console in retail; harmless).
10. **lenele3d: a timer nobody reads** (ls_lenele3d_timer, 1 s) and a dword written once (0x257e9dc).
11. **Checked, not bugs:** every flag these units read or write is declared for its level in events.tbl
    (eleh's "extra_4" on the "test" record is documented there: "used by eleh script to keep door shut");
    got_draught is set by the Wolong_dlg.tbl draught topic (+Event:) together with GotDraught; "Heirophant" in
    ls_templeint_eavesdropping_cs_end is the spelling of the object in TempleInt_v2_script.tbl.
