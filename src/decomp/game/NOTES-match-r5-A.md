# VC6 match pass 11 (28 units)

Base: `snapF/src/decomp` (= repo src/decomp at b74be27). Measured with VC6 SP5 (`vc6sp5`, c2 12.00.8966, the
build in Sum.exe's Rich header), per-unit flags from `src/vc6-flags.txt` (unchanged), headers unchanged
(`match11/src/include` = repo include plus the DECISIONS block). `before8966.txt` / `after8966.txt` are whole-unit
sweeps (sweep.mjs); `before/` holds the first 8168 run (superseded), `after/` -v outputs; `v/` the variant files
(try.mjs format), `log.txt` the change log.

**Total under 8966: 270 / 398 -> 303 / 398 (+33; 31 source wins and 2 destructors whose markers were mis-paired).
No match lost** (address-by-address join of the two sweeps; the only addresses that left the list are the two
deleting-destructor notes 0x493f40 / 0x493fa0, which were wrongly compared against the destructors, D6904).
Every change was first made under 8168 and re-checked under 8966: all of them hold there; none was 8168-only.

| Unit | Before (8966) | After (8966) |
|---|---|---|
| game/conversation.cpp | 18/27 | 24/27 |
| game/items.cpp | 25/31 | 27/31 |
| game/level_script_catacombs.cpp | 10/15 | 14/15 |
| game/level_script_khosanistrng.cpp | 4/4 | 4/4 |
| game/level_script_rand_forest01.cpp | 5/6 | 5/6 |
| game/level_script_slums.cpp | 5/7 | 5/7 |
| game/levelrender.cpp | 5/12 | 6/12 |
| game/multisave.cpp | 4/8 | 4/8 |
| game/multitrade.cpp | 12/22 | 13/22 |
| game/new_pxochat.cpp | 32/57 | 39/57 |
| game/new_tutorial.cpp | 13/15 | 15/15 |
| game/unnamed_ai_groups_0x492b00.cpp | 10/18 | 13/18 |
| game/unnamed_alt_textures_0x4aac10.cpp | 3/4 | 3/4 |
| game/unnamed_containers_0x489730.cpp | 1/4 | 4/4 |
| game/unnamed_cutscene_camera_0x493e50.cpp | 7/12 | 9/12 |
| game/unnamed_giant_salamanka_0x4c8030.cpp | 15/19 | 17/19 |
| game/unnamed_kick_0x471df0.cpp | 3/3 | 3/3 |
| game/unnamed_mp_join_0x4ab0f0.cpp | 14/17 | 14/17 |
| game/unnamed_states_0x431870.cpp | 9/14 | 9/14 |
| vsdk/cmdline.cpp | 12/16 | 12/16 |
| vsdk/frametime.cpp | 10/12 | 10/12 |
| vsdk/gr_font.cpp | 10/17 | 10/17 |
| vsdk/plainstring.cpp | 14/20 | 14/20 |
| vsdk/unnamed_bezier_0x558a30.cpp | 12/15 | 12/15 |
| vsdk/unnamed_ca_render_0x520420.cpp | 0/1 | 0/1 |
| vsdk/unnamed_inetgetfile_0x51b760.cpp | 8/10 | 8/10 |
| vsdk/unnamed_movie_0x5172f0.cpp | 1/2 | 1/2 |
| vsdk/unnamed_ui_static_0x527e20.cpp | 8/10 | 8/10 |

62 likely / unsure markers of matching functions raised to `exact (VC6 byte match, match11; was ...)` (marks.txt);
15 markers that say exact but differ are left as they were (D6905). mingw `-fsyntax-only`: 0 `error:` lines in the
28 files; no header changed.

## Won (per function)
- new_pxochat 0x4251a0 append_motd: '<= 1023' signed (jg)
- new_pxochat 0x422790 banner_has_url: if-form bool
- new_pxochat 0x425010 draw_caption: x,y locals before set_color (was marked exact, differed)
- containers 0x489790 reset: index loop (jl)
- containers 0x4897c0 free_all: index loop, containers[i].deinit() direct call (was virtual through pointer)
- cutscene_camera 0x493f60/0x493fc0: COMDAT marker comments separated by a blank line (the tool paired the dtors with the scalar deleting dtor addresses and skipped their own)
- giant_salamanka 0x4c8c40 target_in_reach: vector3 *pt = &offense_points[..].pos
- giant_salamanka 0x4c8280 set_flinch: LOGIC FIX + GAME BUG while loop whose continue skips m = m->next (null action hangs)
- ai_groups 0x493040 character_is_helpless: LOGIC FIX NULL c returns 1 (je 0x493074 -> mov eax,1)
- ai_groups 0x493080 skips_group_orders: bool r = (x >> 8) & 1; return r; (and al,1)
- items 0x44b4f0 weapon_is_ranged: bool r = flags & 1; return r; (and al,1: overturns D6306 (7))
- items 0x44b6d0 item_has_magic_info: ? true : false (no xor)
- new_tutorial 0x42ef00 tutorial_frame: LOGIC FIX used=true once tutorial_current is set (before the pages test); next through bool: if(!next) next = mouse && hit
- new_tutorial 0x42f020 tutorial_draw: y uses i * font_h (strength-reduced, init after the count test)
- levelrender 0x48e520 level_lights_setup: texture != -1 branch first; ambient z as level_light_scale * z (fld order of the relocations)
- multitrade 0x470ed0 mp_trade_frame: state != 1 branch first (the 2..4 block is the fall-through)
- new_pxochat 0x422dd0 load_bitmaps: element pointers b / d (D6403)
- new_pxochat 0x4246d0 pxochat_draw: element pointers in both loops
- new_pxochat 0x424410 update_buttons: release-first template (D6702), off = -1 for 25 as an if
- conversation 0x4d7d50 set_speaker,0x4d7e40 set_topic,0x4d8140 first_topic,0x4d8170 next_topic,0x4d8220 set_topic_available: (bool) casts of one-bit bitfield reads give shr/test cl,1 (overturns D6703)
- conversation 0x4d8620 build_topic_list: (bool)t->fresh + list insert written out with head->next stored before e->prev
- catacombs 0x4fdfc0/0x4fe030 ball1/2_used: obj *o = obj_from_handle(..) local, then o->pos = lobj->pos
- catacombs 0x4fe4c0 carados_cutscene: int music = music_find_by_name(..) local (D6702)
- catacombs 0x4fe460 sync_object_positions: obj *o locals
- new_pxochat 0x424ec0 draw_chat_input: if/else of two gr_string calls (cross-jumped)
- containers 0x4897f0 container_obj_create: if/else of two item_type_find calls (8966 merges the call: one call with a selected constant, D6405 no longer applies)
- ai_groups 0x492d30 ai_groups_frame: index loop with element pointer; random timeout branch first

## What worked (D6900)

- **One-bit bitfields read as `(bool)t->field`** reproduce the binary's `mov / shr / test reg8, 1` (five
  conversation functions at once). This was listed as not reproducible (D6703); it is a source shape. Likely to
  help in every unit with bitfield tests (other agents / the main session may want to sweep for it).
- `bool r = bit & 1; return r;` gives `and al, 1` (D6306 (7) no longer holds); `x ? true : false` for a setne
  without xor.
- If / else of two calls for a selected constant argument (8966 cross-jumps them into one call; D6405 no longer
  holds): container_obj_create, pxochat_draw_chat_input.
- Call results into locals before a member copy (obj_from_handle, music_find_by_name).
- Element pointers / index loops / branch order as in earlier passes (D6403, D6702).
- Frames: a file_find_node as the list head (state_loadgame_enter), function-scope locals that the original does not
  overlap with a block's buffer (pxochat_handle_click).

## Still differs (D6905)
Register choice and scheduling (multitrade handlers' first compare, pxochat page_down / drag / scrollbar / user
list, bezier evals, gr_font, plainstring, cmdline, the levelrender draw lists), identical-store hoisting in the
three pxochat popup frames, item_create's early NULL in EAX, the max(x, 0) AND of pxochat_init, FPU sum order in
levelrender_sum_point_lights (all six permutations and len() give the same code under 8966), stack-pop merging in
mp_remove_player, EH functions whose differences start in the body (multisave, items_parse_table, movie_play,
alt_textures, InetGetFile ctor). The main session's vector tips (len of a difference in place, permutations of
sums of squares) were tried on sum_point_lights without effect.

## Logic errors and game bugs
- **GAME BUG + decomp logic error, giant_salamanka_set_flinch (0x4c8280, D6901):** a flinch-table move whose
  action is NULL hangs the game in an endless loop (the `continue` at 0x4c829d skips the next-link load at
  0x4c8381). The decomp had a for loop; now the binary's while loop. Harmless with the shipped data only if every
  move of moves[4] has an action.
- **Decomp logic error, character_is_helpless (0x493040, D6902):** NULL counts as helpless (returns 1).
- **Decomp logic error, tutorial_frame (0x42ef00, D6903):** returns true for any current tutorial, also one without
  pages.
- **Marker fix, cutscene camera (D6904):** two destructors were never compared (their marker lines followed the
  deleting-destructor notes in one comment block).
