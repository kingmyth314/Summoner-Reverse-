# match22 notes (cl 12.00.8966, VC6 SP5)

Tree: copy of snapM/src/decomp. Compiler: scratchpad/vc6sp5/VC98. Game units on their old flags (/O2 /GX /MT);
vsdk units on the coordinator's /Ox /Ow lines (D7502, scratchpad/vc6-flags.vsdk-ow.txt; unnamed_net keeps /O2).
Decisions D8100-D8103 in src/include/DECISIONS.md.

## Before -> after per file (8966)

| file | before | after | won |
|---|---|---|---|
| game/unnamed_titus_0x4c20e0.cpp | 20/27 | 26/27 | 0x4c2380, 0x4c2630, 0x4c2dc0, 0x4c3130, 0x4c3240, 0x4c3340 |
| game/new_journal.cpp | 19/27 | 25/27 | 0x41ab10, 0x41abf0, 0x41ade0, 0x41b720, 0x41b8e0, 0x41ba60 |
| game/new_status.cpp | 43/51 | 48/51 | 0x42b260, 0x42c080, 0x42c110, 0x42c540, 0x42c630 |
| game/unnamed_luminar_0x4c8d10.cpp | 16/23 | 20/23 | 0x4c95f0, 0x4c98a0, 0x4c9cd0 (logic fix), 0x4ca3a0 |
| game/conversationctl.cpp | 18/27 | 19/27 | 0x4dac30 |
| game/new_gamecreate.cpp | 15/20 | 16/20 | 0x409b10 |
| game/new_saveload.cpp | 30/37 | 31/37 | 0x426370 |
| game/unnamed_ai_combat_0x4bce60.cpp | 19/28 | 19/28 | |
| game/multitrade.cpp | 13/22 | 13/22 | |
| game/unnamed_pfgeom_0x4a4ee0.cpp | 16/24 | 16/24 | (not reached) |
| game/new_itemtrade.cpp | 29/36 | 29/36 | |
| game/new_charselect.cpp | 25/32 | 25/32 | |
| vsdk/unnamed_sndstream_0x566560.cpp (/O2 /Ow) | 20/26 | 21/26 | 0x567350 |
| vsdk/unnamed_gr_line_0x53e520.cpp (/Ox /Ob0 /Ow) | 10/19 | 11/19 | 0x540330 |
| vsdk/unnamed_localize_0x503f60.cpp (/Ox /Ob0 /Ow) | 22/28 | 23/28 | 0x504230 |
| vsdk/file_packfile.cpp (/Ox /Ow) | 16/20 | 16/20 | |
| vsdk/unnamed_vfile_rw_0x525720.cpp (/Ox /Ob0 /Ow) | 50/51 | 50/51 | |
| vsdk/unnamed_net_0x517590.cpp (/O2) | 22/31 | 22/31 | |

Game: 263 -> 287 of 354 (+24). vsdk under the new flags: 140 -> 143 of 175 (the vsdk "before" is the
snapM source under the /Ow flags; under the old /O2 lines it was 131). None lost, address by address. (The match
counts of the vsdk files before the flags change: net 22, sndstream 17, gr_line 10, packfile 12, vfile_rw 49,
localize 21.) A packfile_check_header rewrite that was closer under /O2 lost its /Ow match and was dropped; a
File::read clamp form and the charselect drag-scrollbar term order gained nothing and were dropped.

Whole game tree after the boss_ai.h change (sweep of all 167 game units, before = snapM + old header): 2937 ->
2961 matches; every changed address is one of the 24 game wins above (titus, luminar, journal, status, saveload,
gamecreate, conversationctl); none lost in boss_ai.cpp, the level scripts, ai_combat, fx or any other user. mingw
-fsyntax-only: 259 files, 0 errors.

## Header change (one)

- include/engine/ai/boss_ai.h: struct titus_boss titus (0x248b3a8-0x248b957, LAYOUT_CHECK 0x5b0), the old global
  names (titus_stage ... titus_rock_erupt_fx_handle, titus_turn_lock, titus_combat_enabled, titus_threat_table,
  titus_current_move) are macros; the three separate externs higher up became comments (D8100). The titus unit
  defines `titus_boss titus;` instead of the 23 globals.

## What worked (details in D8102)

- Titus: the state object (three wins), `pt->pos = pos + c->centre_offset` (as pyrul / machival), the frame's
  `switch (result)` with `case -1: return;`, a goto into the eruption's failure block, a float local for the
  eruption damage.
- Luminar: `float off = d - edges` (dead value kept on the FPU), the edge distance compared first, the beam's
  side vector `vector3(-d.z, 0, d.x) * 1.05f + t->pos` with the effect locals in an else block (frame sharing).
- Screens: the page byte as `char page[4]`, `count <= 0` first in the scrollbars, the release / held update
  template, element pointers (+ a 128-byte name buffer) in status_load_bitmaps, the clamp through `last`, the list
  clears as a loop (the store order inside the loop sets the relocation order), the origins copied element by
  element, `(int)strlen`, `int ok = write(); return ok == 0;`, the reversed source order of the drag sums (also
  fixes the relocation-target warnings of status and saveload drag_scrollbar), (bool) bit-fields and a shared
  last call in conversation_controller::end.
- vsdk: index loop for tolower (localize_load_strings), the SDK WaveReadFile error variable (wav_read), the arrow
  head length in two locals (gr_draw_arrow_3d_between).

## Still differs (and why, as far as known; D8103)

- titus_ai_select_action: stage 1's `timer_up || !target` pick is laid out after the retarget block (8966 puts
  the else first for every condition form tried); the handle is now loaded before the test as in the binary.
- luminar_frame_hook: big frame / operator differences (no ebp zero register in the binary, vector operators);
  luminar_teleport_out: vector stores now match (`float z = frand() - 0.5f; d.x = ...`), the loop exit block order
  does not (goto / for(;;) / inline found code tried); luminar_boss_load: closer (`ca_character *ch`, float locals
  for the pickup reach, `back.len() + swing.len()`), the swing length's term order differs.
- status_page_up, journal_desc_page_up, saveload_page_up, itemtrade_party_page_up: max(x, 0)'s AND register
  (12 forms, D7305). status_handle_click / journal_handle_click: element pointers applied to status (closer), the
  row loop's registers still differ.
- multitrade handlers: first-load registers (D6905 / D7203); mp_handle_it_okay's unthreaded state test.
- itemtrade_count_party_rows: the binary re-reads party_gold after storing the row count and increments in memory.
- gamecreate_draw_player_count / _limits: h / 2 computed twice in the binary (no CSE) [wrong, D9817: w / 2 and h / 2]; gamecreate_frame /
  handle_click unchanged.
- saveload_capture_thumbnail: camera_fov goes through fld / fstp (an inline float accessor in the original?).
- choose_support_spell, mp_charselect page_down / drag_scrollbar, line_talker: register choice only.
- localize_load_names: the case 2 body is the fall-through in the binary (switch and if forms tried).
- wav_open: closer (SDK form with `rc = 0`, `goto ok`); the nBlockAlign product loads channels first.
- File::read, packfile_check_header, gr_line frames: scheduling / frame layout.

## Logic errors / game bugs

- D8101 (decomp logic fix): luminar_ai_release looked up move 0x95 for the ready-hurt anim; the binary sets both
  anims from one lookup of 0x93 (0x4c9d30-0x4c9d5c).
- No new original-game bug found. The titus / luminar game-bug notes already in the files are unchanged.
