# match16: VC6 8966 byte matching, nine units

Compiler: VC6 SP5, cl 12.00.8966 (`scratchpad\vc6sp5\VC98`). Tree: copy of `snapF\src\decomp` in `match16\src`.
Decisions D7500-D7506 are appended to `match16\src\include\DECISIONS.md`.

## Counts (tools/vc6-compare.mjs, 8966)

Official flags = the repo's `src/decomp/vc6-flags.txt` (what the tool uses). "vsdk flags" = the flags this pass
recommends for the vsdk library (D7502): `/Ox /Ow /GX /MT` (`/Ox /Ob0 /Ow /GX /MT` for units that have /Ob0 now).

| Unit | official before -> after | vsdk flags before -> after |
|---|---|---|
| game/level_scripts.cpp | 77 -> 85 / 124 | |
| game/unnamed_avoidance_0x4a6710.cpp | 36 -> 41 / 67 | |
| game/new_inventory.cpp | 51 -> 56 / 69 | |
| game/new_hud.cpp | 42 -> 46 / 60 | |
| vsdk/unnamed_vfile_0x5128b0.cpp | 58 -> 59 / 82 | 63 -> 64 |
| vsdk/gr_opengl.cpp | 31 -> 31 / 54 (unchanged) | 34 -> 34 |
| vsdk/bmpman.cpp | 37 -> 38 / 56 | 42 -> 43 |
| vsdk/parse.cpp | 32 -> 35 / 51 | 40 -> 45 |
| vsdk/unnamed_listbox_0x52ca60.cpp | 51 -> 55 / 69 | 52 -> 57 |
| **Total** | **415 -> 446 / 632 (+31)** | vsdk 231 -> 243 / 312 |

No match lost, checked address by address in every unit under the official flags and, for the vsdk units, under
the vsdk flags. Every function that matches under the official flags now says `Confidence: exact` (raised where it
said likely); functions that match only under the vsdk flags say `Confidence: likely (a byte match under /Ox /Ow,
D7502)`. Comments already saying exact that differ under 8966 were left (62; 13 match under the vsdk flags), list
below.

New matches (official flags): savegame_refresh_character_flags, savegame::set_float,
automap_labels_reset_level, read_info, read_header, read_camera, level_script_select, quest_journal_set_entry;
avoid_clear_segment_flags, avoid_seglists_clear_refs, rect2d_add_point, avoid_collect_segment_refs,
avoid_obstacles_overlap; inventory_frame, inventory_show_item_info, inventory_drag_scrollbar, inventory_draw,
inventory_draw_character; hud_key_save, hud_key_inventory, hud_use_hot_button, hud_hot_buttons_handle_click;
file_chdir; bm_is_reduced; ParseStringList, ParseIntList, ParseFloatList; ui_listbox::entry_at, entry_above,
entry_at_y, scroll_line_up. Under the vsdk flags also SkipSpaces, SkipWhitespace, ui_listbox::set_tooltip.

## Big finding: the vsdk library flags (D7502)

A whole-tree sweep (snapF tree, `match16\sweepow.mjs` -> `match16\sweepow.txt`; `sweepgf.txt` for /Ox alone):

| | game units | vsdk units |
|---|---|---|
| vc6-flags.txt lines | 2690 | 1502 |
| + /Ow | 2402 | 1595 |
| /O2 -> /Ox, + /Ow | 2358 | **1606** (+104) |

Only three vsdk addresses lose (gr_glide 0x549c80 and unnamed_net 0x517780 with /Ow, unnamed_sndstream 0x566d70
with /Ox); per unit, no vsdk unit loses in total. `/Ox` = `/O2` without `/GF /Gy`: without string pooling 8966
turns `strcat(p, "\\")` into a word move (file_make_path, file_make_cd_path, file_change_ext match; this is the
"strcat of a short literal" difference of D6005 / D6603 / D6803). `/Ow` (assume aliasing across calls only) lets
loads move above stores through pointers: most of the parse cursor walks, GetPosition / SetPosition, bm_get_data,
bm_alloc_entries, File::close, gr_opengl delete_texture / tcache_set / get_texture_scale. The game units keep /O2
(both changes lose there). Recommendation for the merge: set the vsdk lines of vc6-flags.txt to
`/Ox /Ow /GX /MT` (keeping /Ob0 where present) and re-run the sweep.

Also tried on these units: /Op (loses one in level_scripts), /G3 /G4 /G5 (no change), /G6 (much worse), /O1.

## What worked (D7500 has the list)

- Index loops instead of pointer loops over global arrays (`jl` in the binary): five functions.
- 8966 orders independent statements against the source in places: level_script_select matched only with frame
  before leave and with direct `level_script_table[i].x` reads (its pointer local blocked the load hoisting).
- Merged calls for an if / else that differs only in a constant (quest_journal_set_entry,
  savegame_refresh_character_flags): the D6405 entry was a 8168 limitation.
- Re-reading members instead of caching them (ui_listbox entries.next), locals for nested call arguments
  (scroll_line_up), `count++` inside the argument (ParseStringList), late initialisation (list parsers,
  inventory_equip_item's flag), `while (1) { if (..) break; }` (SkipSpaces / SkipWhitespace).
- new_hud: hud_member() is a macro, not an inline function (8966 evaluates call arguments first); the
  interface_set_screen flag is the first byte of a 4-byte block; w / h locals for point_in_rect; the switch case
  order of hud_use_hot_button.
- Float min / max as inline functions with by-value arguments (rect2d_add_point); the same helpers are used in
  avoid_merge_obstacle_polys and avoid_add_primitive (both closer, still differ elsewhere).
- Operand / load order in int sums (inventory_drag_scrollbar now matches without the relocation warning).

## Still differing (D7506)

- The `max(x, 0)` select ANDs into the value register in Sum.exe (inventory_page_up, destroy_selected_item,
  identify_item, equip_item differ only there); every form tried ANDs into the mask register. The idiom occurs 45
  times in Sum.exe (`match16\clamps.txt`: address and AND operands); the functions holding the other 41 are in
  new_buysell, new_journal, new_assess, new_status, new_pickup, new_saveload, ... and most of them differ too.
- FileReader::read_int / int16 / uint16 / int8 / uint8 / float: ESI pushed at the entry in Sum.exe.
- Register allocation / frame slots: get_int, level_item_has, hot_button_get, layout_conversation,
  hud_hot_buttons_drag, avoid_circle_contains_point, gl_convert_*; write_character's float copy through the FPU.
- avoid_spline_point_get / avoid_spline_get: the NULL return is laid out between the two range tests in the
  binary; nine forms tried.
- avoid_area_rebuild: closer (frame and loops right, score 105 -> 9), the not-found exit layout differs.

Markers saying exact that differ under 8966 (62): level_scripts 0x4cf6c0, 0x4cfbd0, 0x4cff80, 0x4d0030, 0x4d0590,
0x4d05f0, 0x4d06e0, 0x4d0870, 0x4d09e0, 0x4d0d00, 0x4d0d60, 0x4d0dd0, 0x4d12b0, 0x4d1770, 0x4d2150, 0x4d2370,
0x4d2b70, 0x4d2c90, 0x4d3740, 0x4d4a50, 0x4d6100; avoidance 0x4a7720, 0x4a77d0, 0x4a7830, 0x4aa630, 0x4aa6b0; vfile
0x512b30*, 0x512c00*, 0x5130a0*, 0x513bb0, 0x513bf0, 0x513c40, 0x513c80, 0x513cc0, 0x513d00, 0x513e00, 0x5147c0;
gr_opengl 0x54d5c0, 0x54d860, 0x54deb0, 0x54df20, 0x54df90, 0x54e050, 0x54e740*, 0x54ed50*, 0x54ed90*, 0x54efe0,
0x54f8c0; bmpman 0x502240, 0x502650, 0x503110, 0x503480, 0x503610*, 0x503aa0; parse 0x5159e0*, 0x515af0*,
0x515ba0*, 0x515ce0*, 0x515d00*, 0x515f90*; new_inventory 0x416290, 0x4165c0 (* = matches under the vsdk flags).

## Logic errors fixed and game bugs found

- **Decomp fix (D7504):** savegame::automap_labels_reset_level (0x4d14f0) skipped 0x10 bytes for x / z / layer
  instead of 0xc, so it compared the wrong text and walked 0x38-byte records over 0x34-byte labels.
- **Decomp fix (D7504):** savegame::read_info (0x4d2800) passes NULL to read_header as the version pointer, not
  its own argument (all callers pass NULL; same behaviour).
- **GAME BUG (D7503):** avoid_area_rebuild (0x4a7cb0) copies the polygon's other obstacle indices into a 3-entry
  stack array (frame 0x18, array at +0xc, stores at 0x4a7d5d); a polygon holds up to 8 obstacles, so removing one
  from a polygon of 5 or more overwrites the saved EDI / ESI / EBP / EBX of the caller with obstacle indices.
- Header change (D7505): bitmap_entry::reduced is a bool (bm_is_reduced returns the raw byte).

## Checks

- mingw `i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Iinclude`: 0 `error:` lines in all 259 .cpp.
- Header users: the 70 other units that include bmpman.h give identical VC6 results before / after
  (`hdr_before.txt`, `hdr_after.txt`).
- Per-unit result files: `base\*.txt` (before), `after\*.txt` (after); `*_ow.txt` under the vsdk flags.

## Tools written for this pass (match16)

`c.mjs` (compile + compare one unit, keeps the .cod in `out\`), `try.mjs` (try function variants in place, with an
instruction-difference score), `t.sh` / `rep.mjs` / `ex.mjs` / `ap.sh` (variant edits), `score.mjs` (LCS score of
opcode + registers against Sum.exe), `rank2.sh`, `sweepgf.mjs` / `sweepow.mjs` (flag sweeps), `conf.mjs`
(Confidence markers). Note: another process deletes `%TEMP%\vc6cmp-*` folders, so `--keep` listings must be
copied at once (c.mjs does).
