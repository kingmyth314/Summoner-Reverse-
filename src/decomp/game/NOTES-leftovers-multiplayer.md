# decomp76 notes

Base: snapB (commit 95bd3d5). Decisions D5800-D5812 are in src/include/DECISIONS.md. README rows are updated.

## Verification

- Full-tree VC6 comparison (tools/vc6-compare.mjs): 3510 byte matches before, 3593 after. No function that matched before has lost its match.
- VC6 compile errors: only game/new_help.cpp, which already fails in the baseline. levelrender.cpp failed before and now compiles.
- mingw: 259 of 259 .cpp files compile with 0 errors.

## What went where

| Task | Where | Functions | Exact / likely / unsure |
|---|---|---|---|
| 1. levelrender draw lists and VC6 build | game/levelrender.cpp: draw_list_glide 0x48e6c0, _d3d 0x48fdb0, _opengl 0x491500; .bss arrays; s3d.h render groups and vertex_dup; gr_direct3d.h colour union | 3 new (33 of 39 in the unit) | 0 / 3 / 0. The call sequences match one to one; the unit's VC6 matches are now 5 of 12 compiled |
| 2. unit 0x42f1e0-0x430baf | game/unnamed_worldmap_automap_0x42f1e0.cpp + header (PXO login, worldmap, automap labels and grid, cmd_save_map_labels, states 7 / 9) | 35 | 31 / 4 / 0 |
| 3. net / os | vsdk/unnamed_net_0x517590.cpp: float_compare 0x517dc0, nw_NewReceiveReliable 0x518bb0, nw_NewSendReliable 0x518d70, nw_NewCloseSocket 0x5191c0. vsdk/unnamed_os_0x530b90.cpp: os_poll 0x530ca0, os_pump_messages 0x530cb0. level_scripts.cpp 0x4ceef0 marker split | 6 | 4 / 2 / 0 |
| 4. multiplayer runs | new game/new_pxo_banner.cpp (open_url), unnamed_gamelist_0x4705b0.cpp, unnamed_kick_0x471df0.cpp, unnamed_mp_objects_0x47f860.cpp, unnamed_swi_0x488230.cpp; mp_handle_unequip_item added to unnamed_equip_packets_0x4878a0.cpp; initialiser-marker splits in new_pxochat, multitrade, multisave, unnamed_mp_join | 1 + 14 + 3 + 31 + 14 + 1 | 1/0/0, 10/4/0, 2/1/0, 17/12/2, 12/2/0, 1/0/0 |

The combat and spells entries in runs-final.txt were already written in snapB. Nothing more was needed there.

## Bugs (address, status)

- **Live, OpenGL renderer** (0x491f9e-0x491fe2). Lightmapped faces lit by point lights use the red component for all three colour bytes, so coloured dynamic lights look grey. A SummonerFix candidate. (D5804)
- **Live, multiplayer** (`push esi` at 0x47fa55). mp_send_local_updates sends the leader's OBJ_UPD a second time where it should send the locally controlled summon's, so this path never updates a client's summon position. (D5810)
- **Latent** (after the call at 0x47ff96). mp_handle_new_lep uses the NULL result of a failed living_entity_create after calling mp_player_disconnected.
- **Latent.** mp_respawn_player resurrects a NULL character when the id is unknown.
- **Latent, draw lists** (D5804 items 2-5):
  - The Direct3D batch restart keeps the vertex-buffer position.
  - The OpenGL restart tests count + 3.
  - Clipped polygon vertices are appended without a bound, which can overrun the 4512-entry arrays.
  - levelrender_fan_tables is indexed without the n <= 8 test.
- **Latent, automap.**
  - automap_label_alloc returns 0 when the label table is full, so the next label overwrites label 0.
  - automap_labels_save's first loop does not test ReadLine, so it never ends on a file without "#End".
- **Harmless.** nw_NewCloseSocket clears rsequence[i] where it means ssequence[i].

## Disagreements with docs and symbols

- The sum.symbols `levelrender_*_vert_lit` arrays hold gr_code_point clip codes, not lighting. The names are kept.
- automap_clear_all writes 0 into every cell, which hides the grid. That is the opposite of what the name says.
- worldmap_frame sets location flag bits 2 and 4 together (`or al, 6`). docs worldmap.md describes them separately.
- open_url belongs to new_pxo_banner.cpp (strings.tbl id 834 and sum.symbols). NOTES-menus-dialogue suggested it was the head of new_pxochat.cpp.
- 0x47f860-0x481196 may be the head of multiendgame.cpp: it runs into that file's initialiser, it reads multi_path_type, and it sits next to ccmd_Multi_path_type in .bss. It is not merged because D2604 says otherwise. Placeholder name.
- 0x471df0-0x471fe5 may be the tail of multitrade.cpp. No data decides it.
- SIGNATURE change: mp_send_jn_rqst's last parameter is `bool retry` (it was `int flag`).
- mp_player layout: swi_pos (+0x6c), swi_samples[16] (+0x78) and swi_num_samples (+0xb8) replace `_unk6c[]`. The names are ours.
- PXO login name and password are the two fields of one utrack_login_request. The sum.symbols names are macros.

## Left open

- Two of levelrender's six empty static initialisers (0x48db70-0x48dc10) have no array assigned. It is also unsure which array has which guard bit (D5803).
- D5812: in six small handlers the original emits `mov eax,[eax]` where VC6 gives `mov ecx,[eax]`. No source form or compiler flag tried reproduces it. Those handlers are marked likely.
- The functions marked likely or unsure differ only in register and frame-slot choices, or in instruction counts (the draw lists: 1277/1342, 1627/1704 and 1480/1553).
- The int at 0x68e1bc (mp_join_unk_68e1bc) is defined by no written unit.
- libjpeg is still not written.
