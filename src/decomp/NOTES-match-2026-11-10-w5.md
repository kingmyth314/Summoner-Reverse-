# Byte-match round 2026-11-10, worker 5 (D26800-D26899)

Batch: 45 differing functions in 16 units (combat, levelrender, pathfinding_visibility, bmpman, gametarget, phoenix,
summoner_cd, pyrul, window, multi_chattracker, new_int_multi_update, s3d, level_complete, titus, anim_track,
poly_clip). Distances are `vc6-compare --score` under cl 8966, strict. Helper: `v/tryd.mjs` in the worktree (applies
variant edits, prints the function's distance, restores the file; not committed). About 2.5 hours.

## New matches

None.

## Closer, kept

| file | address | name | D | what did it | distance before -> after |
|---|---|---|---|---|---|
| game/multi_chattracker.cpp | 0x4ac240 | chat_process_irc_message | D26800-D26804 | shared NULL return through `goto none`, the `\1` strip in both arms, the NICK format local, the MOTD word first, the PING reply returning NULL | 387 -> 66 |

Steps (each measured on top of the previous): D26801 PRIVMSG 387 -> 293, NOTICE 293 -> 290; D26802 290 -> 272;
D26800 272 -> 79; D26803 79 -> 66; D26804 66 -> 66 by the score (one `xor eax, eax` line fewer in `--align`).

**D26800 (game/multi_chattracker.cpp; closer). chat_process_irc_message 0x4ac240: every one-line
`if (...) return NULL;` leaves through `goto none;`, with `none: return NULL;` after the final
`strcpy(chat_irc_format_buf, line)`.** 26 sites (the length test, the empty word, `mode == 1`, VERSION, the
NOTICE PING, MODE, the 322 state test, 324, 366 / 333 / 329, the 353 empty list and the numerics 001 ... 375).
Observed: Sum.exe jumps from each of these tests to one shared NULL epilogue (`je 0x4ad1d2`, which has its own
`xor eax, eax`). With `return NULL` 8966 knows EAX is 0 after `test eax, eax / je` and gives every such return its
own copy of the epilogue without the xor, so it cannot share it (about 200 of the 272). With the goto the tests
jump to one block, and as a side effect the frame slots of `nick` and `msg` (and `nick_change`) come out in the
binary's order (8966 orders /O2 slots by use; declaration order did not move them: swapped nick / msg
declarations 389, nick_change first or before rest 387). Only one-liners: also turning the block-ending
`return NULL`s into gotos is worse (103); gotos only after line 480 give 81, after 340 81. Same behaviour: the
label is the function's last statement, `return NULL`. Inferred: the original probably had these tests in some
form that reaches a common return (an else-if chain or a result variable), not 26 gotos; the goto form is the one
measured.

**D26801 (game/multi_chattracker.cpp; closer). The trailing `\1` strip of msg written in both arms of
`if (rest[0] == ':')`** (PRIVMSG and NOTICE). Sum.exe tests `cmp byte [rest], ':' / jne` with the then-arm first
and the strip duplicated after each `strcpy(msg, ...)`. The plain form (one strip after the if / else) lays the
else-arm first (`je`), and the inverted test or a `?:` argument do not change it (387 / 386). Same behaviour:
the same statements on each path.

**D26802 (game/multi_chattracker.cpp; closer). NICK: `char *fmt = XSTR(..., 242);` before
`sprintf(chat_irc_format_buf, fmt, nick, irc_get_word(0, rest));`.** Sum.exe loads `localize_strings[242]` into
ESI before the irc_get_word call (`mov esi, [0x25cd8a4]`, 0x4ad0..); with the macro in the argument list 8966
loads it after the call, into EDX, and the EAX / ECX / EDX rotation is off from there to the end of the
function. Same behaviour: nothing between the two reads writes localize_strings.

**D26803 (game/multi_chattracker.cpp; closer). The MOTD case fetches the word first: `word = irc_get_word(3,
line); strcpy(buf, "!MOTD$#!"); strcat(buf, word);`.** Sum.exe calls irc_get_word before the inline strcpy
(`push ebx / push 3 / call` then `mov edx, eax` held across it). `strcat(strcpy(buf, "!MOTD$#!"),
irc_get_word(3, line))` is 71, the word local 66. Same behaviour: irc_get_word reads line and writes its own
buffer, not chat_irc_format_buf.

**D26804 (game/multi_chattracker.cpp; behaviour fix in the decomp). The CTCP PING reply of PRIVMSG returns NULL.**
Observed: Sum.exe at 0x4ac85c calls irc_process_input("/NOTICE ... PING ...", 1), then `add esp, 0x18 / xor eax,
eax` (0x4ac864) and returns 0. The decomp returned irc_process_input's result. Now `irc_process_input(...);
return NULL;`. This changes the shipped-configuration behaviour to the binary's (the return value of a
PRIVMSG PING line is NULL, nothing is shown). Inferred: harmless either way in practice (the /NOTICE goes through
chat_process_irc_message in mode 1, which returns NULL for NOTICE), but the source now says what the exe does.
Not a game bug (the exe was always right); a decompilation error, for docs/hybrid-review.md if the lead wants it.

**D26805 (notes only). chat_process_irc_message, left at 66.**
- The prologue: Sum.exe stores the first bytes of the two `{0}` buffers (prefix, nick) after the seven fills
  (`mov byte [esp+0xc34], 0` / `[esp+0x784], 0` scheduled late); ours stores them in declaration order. Tried
  `{'\0'}`, `{NULL}` (66), `= {}` (C2059).
- The 311 / 319 / last strcpy tails: Sum.exe cross-jumps the 319 and the final strcpy into 311's tail
  (`jmp 0x4ad1c6`) and 311's epilogue keeps `xor eax, eax` (it is the shared NULL block at 0x4ad1d2). Ours keeps
  EAX = 0 known after the inline strcpy and drops the xor, so the tails are not merged; `goto none` in 311 / 319
  (or in any one of the other block-ending returns) changes nothing (79 at the time), 444 / 545 worse (101).
- The 353 loop: Sum.exe has `add esp, 4` after irc_add_user and `add esp, 8` after strtok; ours merges them
  (`add esp, 12`). `for (tok = strtok(...); tok; tok = strtok(NULL, " "))` 69, `while (tok)` 69, the for with
  `goto none` 66, `while ((tok = strtok(NULL, " ")) != NULL)` 66.

## Still differs (tried; do not repeat)

- **cd_find_drive 0x43e660 (3), cd_set_root 0x43e8e0 (3)**: the no-CD patch bytes (D2600, D17900, D19905), not a
  source difference. Not tried.
- **pathvis_add_edge_node 0x4b7d30 (22)**: the list unlink through `lea edx, [eax+0x20] / lea ecx, [eax+0x24]`
  (D7006 / D10203 / D25104). New forms, all 22: a `pathvis_edge **pp = &e->prev, **pn = &e->next` pair (after the
  error test, or taken right after the load), the pair declared in reverse order, `pathvis_edge *&p / *&n`
  references, `pathvis_edge **l = &e->prev` with `l[0]` / `l[1]`. 8966 folds every form back to `[eax+0x20]`.
  The same unlink differs in mp_player_alloc and mp_handle_actv_gm; a source form for it would give three.
- **phoenix_anim_remap_in 0x499ad0 (36)**: not retried; the four copies (tiger / serpent / phoenix / ghost) have
  about 60 forms between them (D19401, D20506, D20800, D22100, 21e lists). Same rotation in all four.
- **titus_ai_select_action 0x4c2790 (34)**: the stage-2 else-if block is laid out after `goto done` in ours, before
  the move-10 path (falling into it) in Sum.exe. New: `if (valid13) goto move12_13; SetTimeout(20000);` (62),
  the same with `== true` (62), `if (valid13) goto ...; else SetTimeout` (62); the move-11 flags load through an
  `int f = titus_flags` local (34).
- **living_combat::get_protection 0x464de0 (36)**: Sum.exe loads e6 into EDX with the other four slots (8966 sinks
  it into the fifth test) and places the armour block after the epilogue, jumping back to the bless tail. New:
  the inverted `if (e3 || e4 || e7 || e8 || e6) { armour } else { skill }` (36), the test through a bool (36), e6
  in p6's test (48).
- **version_check_run 0x410f40 (32)**: looked only. Sum.exe cross-jumps cases 1 / 2 into the SkipVerify tail
  (its `pop edi` is after the stores); a goto into that tail was worse in an earlier round (D11049's list).
- **mp_handle_combat_sequence 0x4b1280 (90)**: looked only. Sum.exe reads the bytes in source order, keeps
  variant / skill as bytes in the frame and loads them zero-extended into EBX / EBP (`mov ebx, [esp+0x14] / and
  ebx, 0xff`), special as a raw byte in data's dead argument slot, t in a frame slot (`ai_state *` spilled to
  [esp+0x28]). Earlier rounds' int-copy forms are 91-101.
- **bm_read_header 0x502a00 (193)**: looked only. Sum.exe keeps `type` in a frame slot ([esp+0x18], between
  mip_bpp and mip_w; reloaded at each `return type`) and gives ESI / EBX / ECX to fps / num_frames / size; ours
  keeps type in ESI. Not attempted.
- Not retried (exhausted by earlier rounds, read in the notes): combat_get_weapon_tag_position (2),
  pick_test_character (2), calc_rot_tangents (2), combat_position_modifier (4), blood_splot_place (4),
  bm_set_reduced (4, 576 orders), bm_lock_tga (10), pathvis_bsp_contains (12), do_single_vis_check (34),
  determine_point_visibility (26), ai_choose_special (26), chain_check_window (47), handle_arrow_key (83, mostly
  the jump table), ui_window::init (69), polygon_clip_2d (84), the levelrender draw lists (1,287-1,517),
  melee_damage (382), world_mouse_frame (306), world_pick_under_mouse (215, the inline budget), s3d_read_meshes
  (178), bm_load_internal (178), the phoenix / pyrul choose_attack and reposition functions (63-341).

## Device for the next round

**A one-line `if (cond) return 0;` after a test that leaves EAX = 0 (`!stricmp(...)`, `!strcmp`, a zero test of
a call result) gets its own epilogue copy without `xor eax, eax` under 8966.** Where Sum.exe instead jumps to one
shared epilogue that has the xor, route those returns to a label at the end (`goto none; ... none: return
NULL;`). This also changes the use-ordered frame slots. Candidates: any long string-dispatch function with
many `if (!stricmp(cmd, "...")) return NULL;` lines whose `--align` shows `je <shared epilogue>` in the binary.

## Behaviour errors / bug candidates

- D26804: the decomp returned irc_process_input's result for a PRIVMSG CTCP PING; Sum.exe returns NULL. Fixed in
  the source (shipped configuration now matches the exe there). No game bug.

## Header changes

None.

## Changed files

- src/decomp/game/multi_chattracker.cpp (chat_process_irc_message and its comment; D26800-D26804)

## --check

`VC6_STRICT=1 node tools/vc6-sweep.mjs --root <W>/src/decomp --only "<the 16 batch units>" --check
<W>/src/decomp/vc6-matches.txt`: 16 units: 347 match, 0 wrong target, 39 differ, 0 not found, 0 errors;
347 matched in vc6-matches.txt, 347 now, **0 lost**. vc6-matches.txt is unchanged (no new matches).

`node tools/link-build.mjs --config fixed` in the worktree: 259 units (269 compiled, 0 failed); unresolved 0;
multiply defined 0.
