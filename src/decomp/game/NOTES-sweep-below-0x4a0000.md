# decomp73: work list runs-a.txt (92 runs, 263 functions)

Copy: `src/` = snapshot 3927ca2 plus the changes below. Decisions D5500-D5510 are appended to
`src/include/DECISIONS.md`. All 263 functions of the list now carry a line-start `// 0xADDR` marker:
218 exact, 32 likely, 13 unsure.

Checks
- mingw (`-fsyntax-only -std=c++98 -w -Iinclude`): 0 errors on all 243 .cpp files of game/ and vsdk/.
- VC6 (`tools/vc6-compare.mjs`, a copy in `decomp73/tools/` that takes the include directory from
  `DECOMP_INCLUDE`; the repo's tool hard-codes `src/decomp/include`): the whole tree, against the snapshot built the same
  way (/O2 /GX /MT; objects.cpp, living_entity.cpp and the cutscene file /Od /Ob1 /GX /MT): 3,347 of 5,747 marked
  functions match, against 3,316 of 5,715. Nothing that matched before was lost. The 31 new matches are the ten new
  functions of the gameseq and icam units and 21 existing functions that vc6-compare had been reading under the
  wrong address (D5510). new_help.cpp and levelrender.cpp do not compile under VC6 before or after (help_line
  reference members; D3D7 types), so their markers are not byte-checked.

## Per run

| Run (first..last, count) | Where it went | Confidence | Notes |
|---|---|---|---|
| 0x401000..0x401190 (7) gameseq_init.. | NEW `game/unnamed_gameseq_0x401000.cpp`; gameseq_state_table (0x588a70) and the four state variables are defined there; GameStateHandlers, GameStateSwap and the state 7 / 9 handlers declared in summoner.h | 7 exact (VC6 byte match) | D5500, D5501 |
| 0x4011c0 (1) | new_assess.cpp, the initialiser comment split into markers (the thunk 0x4011b0 too) | exact | |
| 0x401e80..0x401eb0 (4), 0x4044b0, 0x4045a0, 0x404690, 0x404780, 0x404870..0x4048c0 (4) | new_automap.cpp markers; `vector constructor iterator` 0x404880 marked there; 0x4048b0 / 0x4048c0 in new_buysell.cpp | exact; 0x404870 unsure (which object), 0x404880 likely (placement) | D5502; the cmd functions 0x4045c0 / 0x4046b0 / 0x4047a0 now byte-checked (match) |
| 0x405c90..0x405d50 (4) buysell_goto_* | new_buysell.cpp: the functions existed, one marker each | exact (match) | |
| 0x4063b0..0x4063e0, 0x408590..0x4085a0, 0x4099b0..0x4099c0, 0x40a660..0x40a690 | new_charselect / new_credits / new_gamecreate / new_gamelist: timer initialiser pairs | exact | |
| 0x40bf30..0x40c250 (10) | new_help.cpp: thunk exact, the slot-table initialisers likely | 5 / 5 | |
| 0x40cb60..0x40cc00 (10) | new_hud.cpp | exact | |
| 0x410760..0x410cd0 (4) new_icam_* | NEW `game/unnamed_icam_0x410760.cpp` (new_icam_load / find / get) + the vector3 copy constructor COMDAT 0x410cd0 marker; new_icam_find declared in interface.h | 4 exact (3 VC6 byte matches) | D5503, D5504 |
| 0x414d30..0x414db0 (6), 0x416910..0x4169d0 (4) | new_inventory.cpp markers; the goto_category functions match | exact / 0x414d60 likely / 0x414d40 unsure | |
| 0x418590 | unnamed_item_info_popup_0x418580.cpp | exact | |
| 0x419170..0x4191a0, 0x41a940..0x41a950, 0x41bea0..0x41bed0, 0x41ee90..0x41eea0, 0x4205b0..0x4205c0 | itemtrade / journal / levelselect / options / pickup initialisers | exact | |
| 0x425410..0x4254b0, 0x4254d0 | new_saveload.cpp | exact; 0x425420 likely | |
| 0x4286c0..0x428700 (5), 0x42a990..0x42a9d0 (5) | new_skill.cpp / new_spell.cpp: one marker per page function (all match) | exact | |
| 0x42b1c0..0x42b230 (6) | new_status.cpp | exact; 0x42b1d0 unsure, 0x42b1f0 likely | |
| 0x42e630..0x42e8f0 (14) | new_tutorial.cpp | thunks exact, slot / entry initialisers likely | |
| 0x431880..0x431a20 (5 runs, 15) | unnamed_states_0x431870.cpp: one marker per sinit / init / register / dtor | windows, static exact; list boxes / buttons likely (one declaration) | |
| 0x433350, 0x4333a0, 0x4333d0..0x433410 | player.cpp | exact; 0x433350 / 0x4333d0 likely | |
| 0x4343b0 | unnamed_input_0x4343a0.cpp | exact | |
| 0x434910..0x4349a0, 0x4374b0, 0x437570, 0x437630 | unnamed_cutscene_0x434900.cpp | exact | |
| 0x43b580..0x43b5a0 (2) | gametarget.cpp: COMDAT markers vector3(x, y, z), plane::set_normal_point | exact | |
| 0x43cd70..0x43cd80, 0x43d190..0x43d500 (39), 0x43d5c0, 0x43d760 | summoner.cpp: the 9 CmdLineParam objects get four markers each | exact; 0x43cd80 likely | |
| 0x449d90..0x449e30, 0x449f00, 0x449f20 | items.cpp | thunks / pool exact, slot tables likely | |
| 0x45b680..0x45b6b0 (2) | living_entity.cpp: vector3 operator+ / operator- COMDAT markers | exact | D5505 |
| 0x464020 | combat.cpp | exact | |
| 0x467670 combat_trigger_reflex_ability | combat.cpp, written (macro COMBAT_TRIGGER_CAST) | likely (same size, register allocation differs) | D5506, bug K300 candidate |
| 0x468c90, 0x468cc0 | gamesound.cpp | exact | |
| 0x4697f0..0x469830, 0x469850..0x469880 | objects.cpp | exact | |
| 0x46c550, 0x46c5e0 | unnamed_party_inventory_0x46b460.cpp, item_paging.cpp | exact; 0x46c5e0 unsure (kept) | |
| 0x46dd40..0x46e120 (9 runs) | s3d.cpp; 0x46dd70 / 0x46dd80 are s3d_level's compiler-generated destructor / constructor | exact | |
| 0x4778c0, 0x477970, 0x477a60, 0x478d10 | unnamed_camera_0x4778b0.cpp | exact; 0x478d10 unsure (D3805) | |
| 0x479b60, 0x479cf0, 0x47a140, 0x47a160, 0x47a180, 0x47e8f0, 0x47f710 | spells.cpp | exact; slot tables likely; empty initialisers unsure | |
| 0x47abe0 spell_prepare_cast | spells.cpp, written | likely | D5507, SIGNATURE change |
| 0x47c5d0..0x47d420 (2) spell_cast_execute, spell_cast_launch_effects | spells.cpp, written | likely | D5508, D5509 |
| 0x48db40..0x48dc10 (13), 0x48dc30, 0x48dcf0, 0x48ddb0 | levelrender.cpp | exact; the six empty initialisers unsure | |
| 0x48e440..0x48e460, 0x48fd90..0x48fda0, 0x4914e0..0x4914f0, 0x492ae0..0x492af0 | levelrender.cpp: vector3::operator+= COMDAT, the atexit nops (marked next to the unwritten draw lists) | exact | |

No run was multiplayer-only code (the gamelist / charselect / levelselect timers are multiplayer screens but
trivial initialisers); nothing was skipped.

## Bugs noticed (addresses in Sum.exe)

- **K300 candidate, live in single player if any armour has an offensive, not hit-only +Triggered Effect:**
  combat_trigger_reflex_ability's item branch calls spell_cast_execute with the defender's own handle as the target
  (0x467c7c), with the attacker's position and layer. The reflex-ability branch and the multiplayer branches target
  the attacker.
- **K301 candidate, live:** spell_cast_execute plays the hit sound at an uninitialised stack vector for spells 0x17 /
  0x58 (0x47ca5b), the creature spells 0x1c / 0x4f / 0x64 (0x47ced2), 0x36 (0x47cde5), 0x4c (0x47d08c), every spell
  without a case, and the side buffs 0x04 / 0x15 / 0x16 cast by a non-party caster when there is no cur_character or
  group (0x47c8c6 / 0x47c8d4). Only the 3D position of the sound is wrong.
- Latent NULL reads: spell_prepare_cast 0x17 with source types 1-4 (0x47b094) and 0x18 (0x47b9dc) read the target
  without a test; spell_cast_launch_effects 0x0c / 0x65 / 0x67 read living_from_handle(target)->pos (0x47d987);
  spell_cast_execute 0x69 / 0x6b call get_hit_by_spell on living_from_handle's result for every party slot (0x47d2c9).
- spell_prepare_cast: with the No_resist console toggle on, spell 0x65 has no effect at all (0x47b31a).
- spell_cast_launch_effects: the creature spells 0x4f and 0x64 create spell 0x1c's effect (0x47da55); the weapon /
  armour triggered missile copies the target's orientation into a matrix nothing reads (0x47d7b3-0x47d84d).
- GameTick formats the per-packet send counters into string_buffer and never prints them (0x40114a); new_icam_load
  formats "Unable to parse item" for unknown characters into a local buffer only (0x410a7b), unknown items are skipped
  silently; new_icam_get's "DIDN'T FIND" likewise (0x410b94). Debug leftovers, no effect.

## Disagreements with docs / symbols

- sum.symbols' spell_prepare_cast comment lists "(out request, caster, spell, target handle, source, ...)": the
  fifth argument is the casting item's type (not the source), the sixth the source type and the seventh a float
  power (counting the hidden result pointer; D5507).
- living_combat::trigger_reflex_ability's sum.symbols comment ("as combat_trigger_special_ability for the defender's
  reflex ability") is right; the beneficial-spell test is SPELL_BENEFICIAL (0x20), which sum.symbols' special-ability
  comment calls "missile".
- sum.symbols' level_names__ctor / level_names__dtor (0x46dd80 / 0x46dd70) are s3d_level's compiler-generated
  constructor and destructor (s3d.cpp already said so; now marked).
- The vector3 operator+ / - COMDATs in living_entity.cpp's /Od range are optimised code (D5505).
- Comments of the form `// 0xADDR name: not written yet` made the run lists treat unwritten functions as written:
  combat_trigger_special_ability 0x4670a0, living_combat_update_boss_attack 0x468360, blood_splot_place 0x468760,
  summon_create 0x47e900, levelrender_draw_list_glide / _d3d / _opengl 0x48e6c0 / 0x48fdb0 / 0x491500, and the
  state 7 / 9 handlers 0x430ad0-0x430b90 (mentioned in other comments). The touched files now word them so that the
  scanner sees them (D5510); unnamed_net_0x517590.cpp and unnamed_os_0x530b90.cpp still have such lines.

## Left

- spell_spawn_creature (0x47bd30, 436 instructions) and summon_create (0x47e900), combat_trigger_special_ability
  (0x4670a0), the three draw lists of levelrender.cpp, states 7 / 9 (0x430ad0-0x430b90): not in this list (hidden by
  the comments above), not written.
- The four functions written here at "likely" do not byte-match: reflex (register allocation around frand),
  prepare_cast (tail merges / register choices), execute (frame size: VC6 does not overlap the case-scoped locals),
  launch_effects (the original interleaves the second switch with the first one's cases). Their flow, calls,
  constants and stores follow the disassembly.
