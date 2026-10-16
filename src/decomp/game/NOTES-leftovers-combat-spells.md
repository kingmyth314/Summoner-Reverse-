# decomp75 notes

Base: snap7441 (commit 744149e). Work in decomp75/src. The session was stopped early by the coordinator.

Checks: mingw (`-fsyntax-only -std=c++98 -w -Iinclude`) gives 0 "error:" lines for game/combat.cpp and
game/spells.cpp, the only .cpp files changed. VC6 (/O2 /GX /MT): no function lost a match it had. combat.cpp went
from 32 to 33 matches and spells.cpp stayed at 18. Decisions D5700-D5703 are appended to include/DECISIONS.md.

## Done (5 functions: 1 exact, 4 likely)

| Address | Function | Unit | Confidence | VC6 |
|---|---|---|---|---|
| 0x4670a0 | living_combat::trigger_special_ability | game/combat.cpp | likely | 1479 bytes, same size; the prologue register order differs |
| 0x468360 | living_combat::update_boss_attack | game/combat.cpp | exact | byte match |
| 0x468760 | living_combat::blood_splot_place | game/combat.cpp | likely | 869 vs 810 bytes (inline expansion per case, D5701) |
| 0x47bd30 | spell_spawn_creature | game/spells.cpp | likely | 2016 bytes, same size, 444 vs 436 instructions |
| 0x47e900 | summon_create | game/spells.cpp | likely | 2195 vs 2415 bytes (tail merging, D5703) |

Also updated:
- The header comments in combat.h and spells.h. This covers the spell_spawn_creature comment and the stale "not
  written" notes on spell_cast_execute and spell_cast_launch_effects.
- The "not written" lines at the heads of combat.cpp and spells.cpp. They started with an address, so they counted
  as markers.

## Bugs noticed

- 0x47bd30 spell_spawn_creature (latent): the walk that picks an already-picked target again can store the list head
  (a local Character on the stack). That happens only if the random index runs past the end, and it cannot, because
  the index is below the count.
- 0x42fd40 automap_labels_parse_table (read, not written, latent): automap_label_alloc returns 0 when full. The `< 0`
  test therefore never stops the loop, and every label after the 32nd overwrites label 0.
- 0x430580 automap_labels_save (read, not written, latent): the copy loop ignores ReadLine's result. If
  automap_labels.tbl has no "#End", the loop never ends. Only type-0 (Permanent) labels are written.
- 0x4302b0 automap_labels_reset_default (read, not written): shopkeepers are matched by comparing name pointers
  (shopkeeper.name == character_get_name()), not strings.
- 0x430110 automap_reveal_cell (read): the same cell is also revealed in the twin levels 7/49, 30/35 and 31/36 when
  their layer counts match.

## Disagreements with docs / symbols

- spell_spawn_creature (0x47bd30) creates no creature. It picks four strike targets (D5702). The name is kept.
- summon_create returns nothing (D5703, SIGNATURE).
- automap_reveal_all (0x42fc00) calls automap_reveal_cell with value 0 for every cell of every level. It clears the
  reveal bits at GameInit, the opposite of what its name says.
- decal_place's colour argument receives a float: blood_splot_place passes its float bit for bit. The argument is
  declared int.
- worldmap_frame sets both bits 2 and 4 (`or al, 6`) for an off-screen location. worldmap.md says bit 2 is the arrow
  and bit 4 the label.

## Left (not written)

- levelrender.cpp: levelrender_draw_list_glide 0x48e6c0, levelrender_draw_list_d3d 0x48fdb0 and
  levelrender_draw_list_opengl 0x491500. Each is 1300-1700 instructions; only part of the asm was read.
- Unit 0x42f1e0-0x430bb0, 41 functions. It is one unit: its .data 0x592c8c-0x5930b7, .bss 0x5f9670-0x5fa273 and .rdata
  0x580d98-0x580df3 are contiguous. Proposed name: game/unnamed_worldmap_automap_0x42f1e0.cpp.
  - Functions: pxo_read_login 0x42f1e0, pxo_login_reset 0x42f280, pxo_login_step 0x42f290; worldmap_* 0x42f320-0x42fac0;
    the init_empty pairs 0x42fbc0-0x42fbf0 (the two vector3 corner globals); automap_* 0x42fc00-0x4302b0; the
    init_ccmd_save_map_labels pair 0x430400 / 0x430410; cmd_save_map_labels 0x430430; automap_labels_save 0x430580;
    state_debug_help_enter 0x430ad0, state7_leave 0x430b10, state7_frame 0x430b20, state_credits_enter 0x430b80,
    state_credits_frame 0x430b90.
  - Status: all of the asm was read and analysed, including the jump tables. pxo_login_step is {0x42f2a1, 0x42f2ac,
    2-3 0x42f2bc}. worldmap_init_locations has cases 0-7. cmd_save_map_labels returns 0-6. automap_reveal_cell's twins
    are 7<->49, 30<->35 and 31<->36. No source is written yet.
  - The unit must also define automap_labels, automap_num_labels, automap_label_type_names and the corner globals.
    summoner/automap.h declares them, but no file defines them yet.
- vsdk/unnamed_net_0x517590.cpp: float_compare 0x517dc0, nw_NewReceiveReliable 0x518bb0, nw_NewSendReliable 0x518d70,
  nw_NewCloseSocket 0x5191c0.
- vsdk/unnamed_os_0x530b90.cpp: os_poll 0x530ca0, os_pump_messages 0x530cb0.
- Marker only: atexit_dtor_savegame_state 0x4ceef0 in level_scripts.cpp. The line that describes it starts with
  0x4ceee0.
- Initialiser markers to split one per line: new_pxochat.cpp 0x4227c0-0x422970, multisave.cpp 0x46cc60 / 0x46cc70,
  multitrade.cpp 0x470e30-0x470e60. Also new_pxo_banner open_url 0x421de0.
- Multiplayer runs (none started):
  - 0x4705b0-0x470e20 (mp_gamelist / join, 20 functions)
  - 0x471df0-0x471f90 (3)
  - 0x47f860-0x481150 (31 mp_* packet functions)
  - 0x487b10
  - 0x488230-0x488ba0 (14 swi packet functions)
  - 0x4ab100 init_mp_join_timer
- Tool note: lines 10-12 at the head of levelrender.cpp start with "//   0x48e6c0 ..." and similar. Because of this,
  tools/decomp-progress.mjs and the run lists count these three functions as written.
