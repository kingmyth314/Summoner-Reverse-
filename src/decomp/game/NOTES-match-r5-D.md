# match14: VC6 byte-match pass over 29 units (game and vsdk)

Tree: `match14/src` (a copy of `snapF/src/decomp`). Compiler: cl 12.00.8966 (VC6 SP5, `vc6sp5`, the build named in
Sum.exe's Rich header). The pass began under 8168; after the coordinator's switch, every change was re-checked
under 8966, and all counts below are 8966 counts. Flags come from this tree's `vc6-flags.txt` (one line added).
Decisions D7200-D7204 are appended to `src/include/DECISIONS.md`.

## Counts (functions with an address comment; 8168 base / 8966 base / 8966 after)

| unit | 8168 before | 8966 before | 8966 after | of |
|---|---:|---:|---:|---:|
| game/unnamed_mp_objects_0x47f860.cpp | 17 | 17 | 18 | 31 |
| game/unnamed_party_inventory_0x46b460.cpp | 19 | 20 | 26 | 30 |
| vsdk/unnamed_sound_emitter_0x546150.cpp | 14 | 14 | 18 | 23 |
| game/multispell.cpp | 13 | 13 | 15 | 22 |
| game/unnamed_decals_0x4bbe80.cpp | 7 | 7 | 9 | 15 |
| vsdk/unnamed_localize_0x503f60.cpp | 21 | 21 | 21 | 28 |
| game/multi_chattracker.cpp | 13 | 13 | 13 | 20 |
| vsdk/timestamp.cpp | 17 | 17 | 23 | 23 |
| game/new_playmenu.cpp | 10 | 10 | 12 | 16 |
| vsdk/unnamed_attached_lights_0x51fb10.cpp | 6 | 7 | 8 | 11 |
| game/unnamed_effectstream_0x440400.cpp | 9 | 9 | 11 | 14 |
| game/level_script_jadetemple.cpp | 6 | 6 | 10 | 11 |
| vsdk/norlsallocpool.cpp | 10 | 10 | 13 | 14 |
| game/unnamed_lobj_raycast_0x4ae890.cpp | 3 | 3 | 3 | 7 |
| game/summoner.cpp | 11 | 11 | 13 | 15 |
| vsdk/unnamed_linefile_0x558340.cpp | 11 | 11 | 11 | 14 |
| vsdk/stringpool.cpp | 13 | 13 | 15 | 16 |
| game/unnamed_experience_0x489930.cpp | 5 | 5 | 5 | 8 |
| game/level_script_courtyard.cpp | 8 | 8 | 9 | 11 |
| vsdk/ui_container.cpp | 8 | 8 | 8 | 10 |
| game/unnamed_lamp_glow_0x4c0a40.cpp | 5 | 5 | 7 | 7 |
| game/multi_door.cpp | 4 | 4 | 4 | 6 |
| game/level_script_ikaemosu.cpp | 5 | 5 | 6 | 7 |
| vsdk/unnamed_jpeg_write_0x5565e0.cpp | 0 | 0 | 0 | 1 |
| game/unnamed_utrack_0x48d7c0.cpp | 3 | 3 | 3 | 4 |
| game/unnamed_equip_packets_0x4878a0.cpp | 3 | 3 | 3 | 4 |
| game/level_script_templeint2.cpp | 15 | 15 | 16 | 16 |
| game/level_script_liangshan.cpp | 19 | 19 | 20 | 20 |
| game/level_script_ikaemosl2.cpp | 4 | 4 | 5 | 5 |
| **my 29 units** | **279** | **281** | **325** | **448** |
| game/level_script_masad.cpp (D7201) | - | 11 | 12 | 13 |
| game/level_script_rand_hills01.cpp (D7201) | - | 5 | 6 | 7 |
| game/level_script_temple_inside.cpp (D7201) | - | 8 | 9 | 10 |

Whole tree (all 259 .cpp, `sweep.mjs` with 8966): **4231 -> 4278** (+47), no match lost at any address, no unit
fails to compile. Out files: `out/allbase.txt` (snapF tree), `out/allcur3.txt` (final), `out/newmatch.txt` (the 47).
`sweep.mjs` and `cmp.mjs` here read this tree's `vc6-flags.txt` (the tool itself reads the repository's).

Checks: mingw `i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Iinclude`: 259 files, 0 `error:` lines.

## Header / flags changes (each whole-tree checked: none lost)

- **D7201** new `summoner/level_info.h`: gameplay.cpp's .bss 0x60a2c8-0x60a40b (the three level names, session
  time, party starts, entry point, layer maps) is one object `level_info`; the old names are macros. Edits:
  `summoner/gameplay.h` (externs replaced by the include and comments), `levelscripts/level_scripts.h` (its
  duplicate extern of level_player_starts became a comment; includes level_info.h), `game/gameplay.cpp` (one
  definition), and parameters named `level_filename` renamed `level_name` in `engine/rendering/levelrender.h`
  (level_vis_load), `levelscripts/conversation.h` (load_for_level), `game/conversationctl.cpp`,
  `game/unnamed_render_scene_0x481680.cpp`. Evidence: 8966 keeps level_entry_point's store after the inline
  strcpys only for members of one object. Won 6 (jadetemple x2, courtyard, masad, rand_hills01, temple_inside).
- **D7202** `vsdk/geom/vfx.h`: `attached_light::type` is `char` (the binary's `or cl, -1`); the `0xff` tests and
  stores in the attached-lights unit and in `game/unnamed_fx_0x489c40.cpp` (line 1619) are now `-1`. Won 1.
- `vc6-flags.txt`: `vsdk/unnamed_sound_emitter_0x546150.cpp /O2 /Ob0 /GX /MT` (D6602's proposal), 15 -> 18.

Other agents: the D7201 edits touch gameplay.h, level_scripts.h, levelrender.h, conversation.h and four .cpp
outside my list (gameplay, conversationctl, unnamed_render_scene, unnamed_fx) only as described.

## What worked (D7200 has the full list)

- Ternary clamps on the stored value (travel samples, lamp alpha), the max into a local for min().
- The found work inside the search loop instead of `break` and a re-test (party inventory, 3 functions).
- One shared return written as such (`goto done`, a single trailing `return 1`, `goto none`).
- Separate bodies for cases the binary keeps separate (8966 merges identical bodies into one table target):
  unequip_slot's species switch, GameInit's `case 0` / `default`; case bodies in the binary's order.
- `x ? true : false` for setcc without xor (timers, gr_init's vsync), `return -d` for the NEG.
- Store order (stringpool, norlsallocpool, decals' set axis), `music_find_by_name` into a local (D6702 again).
- `(float)fabs` in an /Ob0 unit; a pointer loop for `jb`; `bool save[4]` for the byte at the slot's bottom.

## What still differs (D7203)

Dead argument slots reused for temporaries (several mp handlers, the multispell writers), D5812's
`mov eax, [eax]` (an int-returning handler reproduces mp_handle_summon_allowed, not the others), frame layouts
and register choices, store scheduling in the light / emitter constructors, the big functions (equip_item,
decal_build_clip_planes, decals_draw_for_lobj, the localize loaders with EH, chat_process_irc_message,
lobj raycasts) which were not attempted in depth.

## Findings

- **Decomp detail, not behaviour:** sound_emitter_play's binary pushes five arguments to snd_play_looping
  (0x5463f7-0x546406) while the callee reads four and the snd unit's own call (0x521590) pushes four: the emitter's
  source had a five-parameter declaration. No behaviour difference (cdecl, the extra 0 is ignored).
- The D7201 struct explains level_find_player_starts reading `level_player_starts[4]` as level_entry_point
  (already noted in sum.symbols as a game bug: "Unable to locate player starts" never fires).
- No decomp logic errors found in this pass; every change is behaviour-preserving (the attached-light `-1`
  edits keep the old meaning under the new signed type).
