# decomp71 notes

Ranges: 0x43d190-0x43d510, 0x46b460-0x46c580, 0x418580-0x4191b0, 0x4aac10-0x4abb60, 0x4ae330-0x4af330 (139 functions
listed in sum.symbols). Built on the include snapshot of commit 9147a03. Decisions D5200-D5218 in
include/DECISIONS.md. Every new file and every .cpp of the snapshot (game/, vsdk/) passes
`i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w` against this include tree with 0 "error:" lines.

| Unit (new file, all placeholders) | Range | Functions | exact / likely / unsure |
|---|---|---|---|
| game/unnamed_party_inventory_0x46b460.cpp | 0x46b460-0x46c5cd | 32 | 24 / 8 / 0 |
| game/unnamed_item_info_popup_0x418580.cpp | 0x418580-0x419166 | 25 | 19 / 6 / 0 |
| game/unnamed_alt_textures_0x4aac10.cpp | 0x4aac10-0x4ab0ef | 4 | 2 / 2 / 0 |
| game/unnamed_mp_join_0x4ab0f0.cpp | 0x4ab0f0-0x4abb73 | 19 | 13 / 6 / 0 |
| game/unnamed_gtrack_0x4ae330.cpp | 0x4ae330-0x4ae882 | 9 | 6 / 3 / 0 |
| game/unnamed_lobj_raycast_0x4ae890.cpp | 0x4ae890-0x4af335 | 7 | 2 / 4 / 1 |
| total | | 96 | 66 / 29 / 1 |

Compiler-generated initialiser pairs are counted as functions (exact); like the existing units they have no body
of their own, their source is the object definition next to the comment.

## Nothing written (already covered)

- 0x43d190-0x43d510 (39): summoner.cpp already defines `Summoner_options` (init_game_config_defaults is its inline
  constructor), the nine `CmdLineParam cmdline_param_*` objects (the 36 sinit / init / register_dtor / dtor
  functions) and `ccmd_gamma` (init_ccmd_gamma_thunk / init_ccmd_gamma), with the addresses in comments
  (D5200). No summoner.cpp-additions file is needed.
- 0x419170-0x4191a0 (4): new_itemtrade.cpp already lists init_realtimetimer_5c1858 / _5c1860 and their thunks as
  the compiler's initialisers of itemtrade_repeat_timer / itemtrade_cursor_timer.
The "no source" check misses these because they carry no `Confidence:` line of their own.

## game/unnamed_party_inventory_0x46b460.cpp

The party inventory ({count, item} entries, 521 slots, sorted by shop category then type), the shop list helpers
(sort, category search), picking up item objects, adding / removing items, using charged items from the
inventory or an equipment slot, equipping and unequipping (model switch between light and heavy armour, underwear
textures, weapon animations, item status effects, item effects), the servant disguise test, and the add_gold
console command (D5201: its initialiser and object belong here, not to item_paging.cpp). Defines party_inventory,
party_gold, party_inventory_count (declared, never defined before), the servant item list 0x59a63c and two
unreferenced .data tables 0x59a660 / 0x59a674 (D5218).

- Equipment indices from the code: 0 neck, 1-2 rings, 3 hands, 4 feet, 5 weapon, 6 shield, 7 torso, 8 legs; a
  heavy suit sits in 7 and frees 8, a two-handed weapon sits in 5 and frees 6.
- Behaviour (not bugs): merging a charged item into an owned one adds the charges and caps them at the maximum (the
  rest is lost); cmd_add_gold accepts negative amounts.
- Latent (sure of the code, no 1.40 item triggers it): an item with a fractional +Disposable chance keeps 0 charges
  instead of being removed (D5204); items.tbl 1.40 uses only plain "+Disposable" (1.0).
- character_equip_item calls item_is_identified twice (D5205).
- Left: equip_page_slots (.rdata 0x5819d4) is still only extern; it may belong here or to item_paging.cpp (D5208).

## game/unnamed_item_info_popup_0x418580.cpp

The item info popup over the inventory and shop screens (description text with scroll bar, title, 3D model with
the shop's camera table). Own unit by its initialiser and data blocks (D5202). The layout is new_assess.cpp's
template; the line counter starts at 0 (assess starts at 1), which is right for ring_format_info's text because
its lines end with "\n".

## game/unnamed_alt_textures_0x4aac10.cpp

alt_textures.tbl parser and the two lookups characterinfo_parse_table uses. alt_texture_set got its name field
and a size check (D5217). Latent bugs (sure of the code): the base name is parsed with limit 0x28 into a 0x21-byte
row, and the per-set texture count (15) is not checked (D5207).

## game/unnamed_mp_join_0x4ab0f0.cpp (multiplayer, lowest priority)

GM_QURY answer (ACTV_GM game info), JN_RQST checks and accept (ACPT_JN / DENY_JN), mp_change_level, the wait
state, chat_history_pump, and the tracker glue (tracker_init / start / end / request / frame). Unit cut D5206.
- Live but harmless (sure): the items-checksum version check can never refuse a 1.40 client, which sends the
  constant 0xf174baca, one of the host's two accepted values (D5210).
- Dead code (sure): mp_change_level's search of the level's conversation slots for "<level>_m1" has no effect
  (D5211).
- Latent (sure of the code): tracker_start_game strcpy's the 0x100-byte channel into a 0x24-byte field (D5212).
- mp_change_level tests game_flags 0x200, which summoner.h names GF_LEADER_SOUND (from party_set_leader); the name
  does not fit this use, the literal is kept.
- New header summoner/multi/unnamed_mp_join_0x4ab0f0.h (mp_join_request, MP_ACTV_GM / MP_ACPT_JN / MP_DENY_JN,
  the deny reasons, mp_send_gm_qury 0x470b70 of the game list unit). The declarations multi.h already had stay
  there.

## game/unnamed_gtrack_0x4ae330.cpp (multiplayer)

The PXO game tracker client: packet headers, heartbeat every 10 s with resend after 2 s, game over, game list
requests (all or one channel), the 20 game-list buffers, channel counts into the lobby rows, acks.
- Unit boundary (D5213): its .bss ends with the 0x5f1-byte heartbeat packet at 0x247e228-0x247e818 (the source map's
  end 0x247e23d is only the last referenced field); it may still be the tail of multi_chattracker.cpp.
- The code is structurally the PXO game tracker client of Volition's FreeSpace 2 source release; that is not
  allowed as name evidence, so no file or function name was taken from it (D5213). The lead may want to decide
  whether this changes the file-name rule for this unit.
- Unsure: a received packet is accepted only when its size equals its len field minus one (D5214).
- gtrack_send_game_over's state 2 is never set, so it never returns 1 (no effect, the return is unused).

## game/unnamed_lobj_raycast_0x4ae890.cpp

Rays and swept spheres against level objects: the object-frame transform, the 10-bit-normal BSP walk, the face
tests (ix_ray_polygon / ix_sphere_polygon_plane / ix_sphere_polygon / ix_point_in_polygon), lobj_raycast_all and
the lighting stub.
- Header changes (D5209): lobj_ray +0x28 `stop_t` (float), +0x2c `face`; lobj_hit +0x14 `normal`, +0x20 meaning.
- Unsure: back faces count for rays with face flag 0x200 but for spheres with 0x20 (alpha texture) (D5215).
- Latent (sure of the code): a full 128-entry node stack drops the current subtree (D5216).
- lobj_raycast_bsp is "unsure": the control flow and calls match, but the loop shape (two nested loops with an
  explicit stack) is my reconstruction of the jumps.

## Disagreements with docs / sum.symbols

- sum.symbols tags 0x46c540-0x46c570 item_paging.cpp (and the old items.cpp note); the code is written in the party
  inventory unit (D5201).
- source-map.md gives the tracker .bss as 0x247b0a0-0x247e23d; it is 0x247b0a0-0x247e818 (D5213).
- source-map.md puts 0x4ab0f0-0x4abb50 in no named row; the cut alt_textures / join at 0x4ab0f0 is new (D5206).
- sum.symbols' comment on lobj_ray_test_faces / lobj_sphere_test_faces says the nearest hit goes "into 0x247e81c /
  0x247e820": those two words are written, never read.
- sum.symbols comment of gtrack_send_heartbeat says "(inferred from the tracker code around it)" for the printf
  strings; both strings are referenced directly (0x4ae48e, 0x4ae4be).

## Left

Nothing in the ranges. Open questions: owner of equip_page_slots (D5208); whether the tracker client is part of
multi_chattracker.cpp (D5213).
