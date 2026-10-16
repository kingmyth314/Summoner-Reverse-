# Decompiled source (Phase 3 pilot)

C++ source rewritten from Sum.exe's disassembly, one file per original translation unit, for a future
byte-matching build with Visual C++ 6.0 (see [docs/decomp-plan.md](../../docs/decomp-plan.md), "Phase 3" and
"Toolchain"). "Exact" in the notes means "the control flow, constants and globals match the disassembly", not
"the bytes match": the notes were written before VC6 was available. 202 of the 204 units now compile with VC6
Professional (`cl /c /O2 /MT`, include/DECISIONS.md D4900-D4905), and `node tools/vc6-compare.mjs <file.cpp>`
(docs/tools.md) checks a unit's functions byte for byte against Sum.exe.

Rules:
- Everything is written from Sum.exe's own code (`tools/asm.mjs`). The Summoner 2 decompilation has no licence and
  is used for names only, never for code.
- Function and global names come from `symbols/sum.symbols`; each function carries its address and a confidence
  comment (exact / likely / unsure).
- The `NOTES*.md` files list the functions with addresses and confidence, and the open questions.

| File | Original unit | Functions | Exact / likely / unsure |
|---|---|---|---|
| `vsdk/frametime.cpp` | `frametime.cpp` (`0x501470`-`0x5017e6`) | 14 | see `vsdk/NOTES.md` (29 / 8 / 0 with timestamp) |
| `vsdk/timestamp.cpp` | `timestamp.cpp` (`0x5017f0`-`0x501b16`) | 23 | |
| `vsdk/registry.cpp` | `vsdk\os\registry.cpp` | 14 | 10 / 4 / 0 |
| `vsdk/cmdline.cpp` | `vsdk\os\cmdline.cpp` | 20 | 14 / 6 / 0 |
| `vsdk/console.cpp` | `console.cpp` (vsdk\os) | 20 + 2 helpers | 15 / 7 / 0 |
| `vsdk/norlsallocpool.cpp` | `norlsallocpool.cpp` (Summoner 2 name, folder not known) | 14 | 9 / 5 / 0 |
| `vsdk/gr_font.cpp` | `vsdk\gr\gr_font.cpp` | 17 | 9 / 7 / 1 |
| `vsdk/gr.cpp` | `vsdk\gr\gr.cpp` (`0x505550`-..) | 102 | 89 / 13 / 0 |
| `vsdk/effect_mem.cpp` | `vsdk\geom\effect_mem.cpp` (`0x51bb70`-`0x51e018`) | 38 | 20 / 18 / 0 (13 vfx_instance functions not written yet) |
| `game/player.cpp` | `player.cpp` (`0x433340`-..) | 27 | 12 / 15 / 0 |
| `game/summoner.cpp` | `summoner.cpp` (with WinMain `0x43e5e0`) | 61 | 53 / 8 / 0 |
| `game/gameplay.cpp` | `gameplay.cpp` (`0x437660`-`0x43a300`) | 71 | 56 / 13 / 2 |
| `vsdk/window.cpp` | `vsdk\ui\window.cpp` (`0x528430`-`0x52bf92`) | 40 | 22 / 15 / 3 |
| `vsdk/character_instance.cpp` | `vsdk\ca\character_instance.cpp` (`0x5194b0`-`0x51b6a1`) | 52 | 33 / 15 / 4 |
| `vsdk/ui_container.cpp` | `vsdk\ui\ui_container.cpp` (`0x5534f0`-`0x553b9b`) | 10 | 5 / 5 / 0 (vtable `0x581f40` slot order) |
| `vsdk/targautils.cpp` | `vsdk\bmpman\targautils.cpp` (`0x5549c0`-`0x5565d6`) | 21 | 8 / 13 / 0 |
| `vsdk/bmpman.cpp` | `vsdk\bmpman\bmpman.cpp` | 64 | 42 / 21 / 1 |
| `vsdk/gr_opengl.cpp` | `vsdk\gr\opengl\gr_opengl.cpp` (the renderer SummonerFix enables) | 56 | 41 / 15 / 0 |
| `vsdk/gr_direct3d.cpp` | `vsdk\gr\directx\gr_direct3d.cpp` (`0x534a20`-`0x536944` is a second file of `vsdk\gr\directx`, see source-map.md) | 65 of 66 | 37 / 25 / 3 |
| `vsdk/gr_glide.cpp` | `vsdk\gr\glide\gr_glide.cpp` (Glide 2, glide2x.dll) | 63 + 4 | 40 / 23 / 0 |
| `vsdk/memory.cpp` | `vsdk\os\memory.cpp` | 19 | 18 / 1 / 0 |
| `vsdk/stringpool.cpp` | `vsdk\os\stringpool.cpp` | 16 | 14 / 2 / 0 |
| `vsdk/plainstring.cpp` | `plainstring.cpp` (folder not known; Summoner 2 name) | 20 | 12 / 8 / 0 |
| `vsdk/parse.cpp` | `vsdk\parse\parse.cpp` (TextFile) | 51 | 32 / 18 / 1 |
| `vsdk/file_packfile.cpp` | `vsdk\vfile\packfile\file_packfile.cpp` | 24 | see `NOTES-packfile-mouse-exception.md` (50 / 23 / 0 for the five files) |
| `vsdk/unnamed_vfile_0x5128b0.cpp`, `unnamed_vfile_rw_0x525720.cpp`, `unnamed_ui_button_0x526940.cpp`, `unnamed_ui_static_0x527e20.cpp` | File paths, open, seek and FileReader; File readers, writers and sync; the ui_button and ui_static gadgets (**names not known**) | 179 | 135 / 42 / 2 (`NOTES-unnamed-vfile-ui.md`) |
| `vsdk/unnamed_lights_0x51e070.cpp`, `unnamed_attached_lights_0x51fb10.cpp`, `unnamed_ca_render_0x520420.cpp`, `unnamed_snd_0x5204c0.cpp`, `unnamed_eax_0x521e40.cpp`, `unnamed_quaternion_0x5225b0.cpp`, `unnamed_vector_0x523f30.cpp` (+12 functions in `effect_mem.cpp`) | lights, attached lights, character render state, sound system, EAX, quaternions, vectors (**names not known**) | 275 | 173 / 102 / 0 (`NOTES-unnamed-lights-snd-math.md`) |
| `vsdk/unnamed_ca_mesh_0x50af80.cpp`, `unnamed_gr_3d_0x50d9b0.cpp` | character mesh renderer (morphing; Glide, Direct3D and OpenGL back ends), 3D view state (setup, frustum, instance stack, culling, saved state) (**names not known**) | 111 | 86 / 21 / 4 (`NOTES-unnamed-ca-mesh-gr-3d.md`) |
| `vsdk/unnamed_vfx_0x536950.cpp`, `unnamed_ix_0x53ac20.cpp`, `unnamed_floatmath_0x542500.cpp`, `unnamed_ui_element_0x542cd0.cpp` | vfx instances and tags, intersection tests, float helpers, UI element base (**names not known**) | 144 | 91 / 52 / 1 (`NOTES-unnamed-vfx-ix-floatmath-ui.md`) |
| eighteen `vsdk/unnamed_*` units 0x5565e0-0x56c4d0: JPEG writer glue, strings, line files, Bezier, clipping, LFSR, FTP, HTTP, vfx channels, bit arrays, DirectSound and DS3D, UI slider, polygon split, vfx dummies, emitters and materials, sound streams (+2 functions in `unnamed_snd_0x5204c0.cpp`); libjpeg 6b identified, not rewritten; vfx mesh 0x562a30-0x564d62 left (**names not known**) | about 350 | 252 / 97 / 4 (`NOTES-unnamed-vsdk-tail.md`) |
| six `vsdk/unnamed_*` units: animation tracks (0x550660), polygon clipping (0x5519c0), CRC-32 (0x5520d0), UI input box (0x5521a0), UI check box (0x553ba0), vfx mesh (0x562a30) (**names not known**) | 62 + 48 COMDATs | 88 / 20 / 2 (`NOTES-unnamed-anim-clip-ui-vfxmesh.md`) |
| `vsdk/unnamed_gr_line_0x53e520.cpp`, `unnamed_vfile_array_0x544e50.cpp`, `unnamed_gr_poly_0x5450c0.cpp`, `unnamed_sound_emitter_0x546150.cpp`, `unnamed_perlin_0x546d00.cpp`, `unnamed_d3d_tcache_0x54bb30.cpp` (+10 gr::Mode markers in `gr_direct3d.cpp`) | 3D line and bitmap helpers, vfile vector readers, polygon builder, sound emitters, Perlin noise, Direct3D texture cache (**names not known**) | 125 | 80 / 45 / 0 (`NOTES-unnamed-gr-line-poly-emitter-tcache.md`) |
| `vsdk/debug.cpp`, `vsdk/timer.cpp`, six `vsdk/unnamed_*` units (keyboard 0x5093a0, debug stubs 0x531c10, movement 0x531d20, time format 0x532660, vif 0x5329f0), `game/unnamed_cutscene_script_0x4ed050.cpp`, `unnamed_dialogue_table_0x4feb50.cpp`, `unnamed_rand_levels_0x4ee840.cpp` and nine `game/level_script_*.cpp` (lenele2d, lenele2ab, ikaemosbottomint, ionaext, masad, test, rand_forest01, rand_forestnite1, rand_hills01) | keyboard, debug and timer helpers, vif particle curves, cutscene scripts, the dialogue table, level scripts | 171 | 132 / 37 / 2 (`NOTES-unnamed-key-timer-dialogue-levelscripts.md`) |
| `game/level_script_ikaemosext2.cpp`, `_lenele1ab`, `_khosanilab2`, `_tancredhouse`, `_khosanistrng`, `_rand_desert`, `_ikaemosext` (D5300 names), `vsdk/unnamed_progress_0x503dd0.cpp`, `unnamed_movie_0x5172f0.cpp`, `unnamed_inetgetfile_0x51b760.cpp`, plus markers and small additions in 20 existing units (initialisers, header-inline COMDATs, the sysinfo code in `unnamed_os_0x530b90.cpp`, the socket getters in `unnamed_net_0x517590.cpp`) | the last unwritten runs outside libjpeg: level scripts, loading progress, Bink movies, InetGetFile, CPU / sysinfo (**names not known**) | 187 (+2 without sum.symbols entries, +2 that were marked but not written) | 165 / 19 / 3 (decomp74; DECISIONS.md D5600-D5613) |
| `vsdk/unnamed_gadget_0x52bfe0.cpp`, `unnamed_listbox_0x52ca60.cpp`, `unnamed_build_stamp_0x52f450.cpp`, `unnamed_ui_manager_0x52f570.cpp` | UI gadget base, list box, build stamp, UI manager (**names not known**) | 130 + 32 generated | 89 / 41 / 0 (`NOTES-unnamed-ui.md`) |
| `vsdk/unnamed_mouse_0x500a30.cpp` | mouse module `0x500a30`-`0x501468` (**name not known**) | 30 | |
| `vsdk/unnamed_exceptionhandler_0x5419a0.cpp` | exception handler `0x5419a0`-`0x5424fa` (**name not known**; functions as in Bruce Dawson's ExceptionHandler.cpp) | 8 | |
| `vsdk/unnamed_localize_0x503f60.cpp`, `unnamed_net_0x517590.cpp`, `unnamed_os_0x530b90.cpp`, `unnamed_os_0x531bf0.cpp` | localize, net and os code (**names not known**) | 72 | 41 / 31 / 0 (`NOTES-localize-net-os.md`) |
| `vsdk/matrix.cpp` | `vsdk\math\matrix.cpp` | 48 | 27 / 20 / 1 |
| `vsdk/skeleton.cpp` | `vsdk\ca\skeleton.cpp` | 27 | 15 / 12 / 0 |
| `vsdk/character.cpp` | `vsdk\ca\character.cpp` | 46 + 3 | 25 / 22 / 2 |
| `game/new_credits.cpp` | `new_credits.cpp` (`0x408590`-`0x408d72`) | 11 | 7 / 4 / 0 |
| `game/new_help.cpp` | `new_help.cpp` (`0x40bf30`-`0x40cb55`) | 16 + 5 initialiser pairs | 5 / 11 / 0 |
| `game/gamesound.cpp` | `gamesound.cpp` (`0x468c80`-`0x469720`..) | 20 | 14 / 5 / 1 |
| `game/item_paging.cpp` | `item_paging.cpp` | 12 | 5 / 3 / 4 |
| `game/new_playmenu.cpp` | `new_playmenu.cpp` | 16 | 13 / 3 / 0 |
| `game/new_mainmenu.cpp` | `new_mainmenu.cpp` | 13 | 9 / 4 / 0 |
| `game/new_dialogue.cpp` | `new_dialogue.cpp` | 6 | 3 / 3 / 0 |
| `game/new_assess.cpp` | `new_assess.cpp` | 20 | 12 / 8 / 0 |
| `game/new_pickup.cpp` | `new_pickup.cpp` | 22 | 12 / 10 / 0 |
| `game/new_gamecreate.cpp` | `new_gamecreate.cpp` | 22 | 16 / 5 / 1 |
| `game/new_journal.cpp` | `new_journal.cpp` | 29 | 17 / 12 / 0 |
| `game/new_buysell.cpp` | `new_buysell.cpp` | 44 | 32 / 11 / 1 |
| `game/new_itemtrade.cpp` | `new_itemtrade.cpp` | 40 | 29 / 11 / 0 |
| `game/new_options.cpp` | `new_options.cpp` | 37 | 19 / 18 / 0 |
| `game/new_saveload.cpp` | `new_saveload.cpp` | 43 | 25 / 18 / 0 |
| `game/new_status.cpp` | `new_status.cpp` (`0x42b1c0`-`0x42e62e`) | 57 | 33 / 22 / 2 |
| `game/new_charselect.cpp` | `new_charselect.cpp` | 36 | 20 / 16 / 0 |
| `game/new_levelselect.cpp` | `new_levelselect.cpp` | 29 | 15 / 13 / 1 |
| `game/new_automap.cpp` | `new_automap.cpp` (`0x401e80`-`0x404871`) | 46 | 34 / 10 / 2 |
| `game/new_inventory.cpp` | `new_inventory.cpp` (`0x414c80`-`0x418572`) | 78 | 49 / 27 / 2 |
| `game/new_hud.cpp` | `new_hud.cpp` (`0x40cb60`-`0x41075a`) | 70 | 45 / 25 / 0 |
| `game/unnamed_interface_0x411270.cpp` | interface manager `0x411270`-`0x414cb0` (**file name not known; placeholder**) | 122 | 98 / 23 / 1 |
| `game/unnamed_editor_dialogs_0x430bc0.cpp`, `unnamed_states_0x431870.cpp`, `unnamed_frame_0x4323e0.cpp` | editor dialogs, boot/title/load-game states, frame end and input (**names not known**) | 79 | 66 / 13 / 0 (`NOTES-unnamed-states.md`) |
| `game/unnamed_script_editor_0x4de0b0.cpp` | Volition's level script editor `0x4de0b0`-`0x4ecf90` (**name not known**) | 508 | 460 / 46 / 6 (`NOTES-unnamed-script-editor.md`) |
| `game/unnamed_input_0x4343a0.cpp`, `unnamed_cutscene_0x434900.cpp` | player input / camera keys, and cutscene playback (unoptimised) (**names not known**) | 54 | 50 / 4 / 0 (`NOTES-unnamed-input-cutscene.md`; VC6 25 of 26 compared functions byte-identical, D6101) |
| `game/unnamed_mpmenus_0x43eb00.cpp`, `unnamed_lobby_0x43ef30.cpp`, `unnamed_mlayout_0x43f9d0.cpp`, `unnamed_effectstream_0x440400.cpp`, `unnamed_netsession_0x440b40.cpp`, `unnamed_netpackets_0x441820.cpp` | multiplayer menus, IRC lobby, monster layouts, effect streaming, net session and packets (**names not known**) | 129 | 94 / 35 / 0 (`NOTES-unnamed-multi-net.md`) |
| `game/unnamed_goals_doors_0x475180.cpp`, `unnamed_camera_0x4778b0.cpp` | multiplayer helpers, AI goals, doors and level objects; the game camera (**names not known**) | 122 | 87 / 33 / 2 (`NOTES-unnamed-goals-camera.md`) |
| `game/unnamed_render_scene_0x481680.cpp`, `unnamed_shadow_table_0x487440.cpp`, `unnamed_equip_packets_0x4878a0.cpp` | level frame, fog, vis sets, draw lists, labels, lamp glows, detail levels; shadows.tbl and ground decals; equip packets (**names not known**) | 109 of 110 | 69 / 40 / 0 (`NOTES-unnamed-render-shadows-equip.md`) |
| `game/unnamed_containers_0x489730.cpp`, `unnamed_experience_0x489930.cpp`, `unnamed_fx_0x489c40.cpp`, `unnamed_utrack_0x48d7c0.cpp` | containers, experience, effect objects, PXO login / user tracker (**names not known**) | 85 + 2 folded | 59 / 26 / 0 (`NOTES-unnamed-containers-xp-fx.md`) |
| `game/unnamed_ai_groups_0x492b00.cpp`, `unnamed_cutscene_camera_0x493e50.cpp`, `unnamed_snow_0x4946a0.cpp`, `unnamed_serpent_0x495290.cpp`, `unnamed_phoenix_0x497ac0.cpp`, `unnamed_tiger_0x499e50.cpp` | AI groups, cutscene camera sequences, snow, and the Serpent, Phoenix and Tiger bosses (**names not known**) | 220 | 146 / 69 / 6 (`NOTES-unnamed-aigroups-bosses.md`) |
| `game/unnamed_pyrul_0x4b87c0.cpp`, `unnamed_ghost_rider_0x4b9e70.cpp`, `unnamed_decals_0x4bbe80.cpp`, `unnamed_ai_combat_0x4bce60.cpp` | Pyrul and Ghost Rider bosses, decals and s3d geometry blocks, AI combat modes (**names not known**) | 131 | 65 / 60 / 6 (`NOTES-unnamed-pyrul-ghost-ai.md`) |
| eleven `game/unnamed_*` units 0x4c0250-0x4ca6ff: projectors, lamp glows, Machival, Titus, Evil Joseph, Evil Urath, Murod, Murod Oni, Tentacle Beast, Giant Salamanka, Luminar (**names not known**) | 256 (12 folded empty bodies) | 154 / 100 / 2 (`NOTES-unnamed-bosses-2.md`) |
| `game/unnamed_handleslots_0x4a3640.cpp`, `unnamed_shadows_0x4a37d0.cpp`, `unnamed_pfgeom_0x4a4ee0.cpp`, `unnamed_avoidance_0x4a6710.cpp` | handle slots, blob shadows, pathfinding geometry, obstacle avoidance (**names not known**); also `level_page_in_characters` added to `boss_ai.cpp` | 135 | 88 / 45 / 2 (`NOTES-unnamed-shadows-pf-avoid.md`) |
| `game/unnamed_level_complete_0x4b0960.cpp`, `unnamed_ai_move_0x4b1410.cpp`, `unnamed_ai_think_0x4b5140.cpp`, `unnamed_dialogue_box_0x4d6660.cpp` | multiplayer level completion, AI movement and goal handlers, AI think and target scoring, dialogue boxes (**names not known**) | 123 | 72 / 46 / 5 (`NOTES-unnamed-ai-move-think-dialogue.md`) |
| six `game/unnamed_*` units: party inventory lists and `add_gold` (0x46b460), item info popup (0x418580), alt textures (0x4aac10), multiplayer join (0x4ab0f0), gtrack (0x4ae330), level-object ray tests (0x4ae890) (**names not known**) | 96 | 66 / 29 / 1 (`NOTES-unnamed-inventory-popup-join-gtrack.md`) |
| `game/unnamed_gameseq_0x401000.cpp`, `unnamed_icam_0x410760.cpp` | the game state sequencer and GameTick (0x401000), the UI model camera tables new_icam_* (0x410760) (**names not known**) | 10 + 1 COMDAT | 10 / 0 / 0, all ten VC6 byte matches (include/DECISIONS.md D5500-D5504) |
| `game/new_skill.cpp` | `new_skill.cpp` | 39 | 28 / 11 / 0 |
| `game/new_spell.cpp` | `new_spell.cpp` | 38 | 31 / 7 / 0 |
| `game/new_tutorial.cpp` | `new_tutorial.cpp` | 15 + 7 initialiser pairs | 12 / 10 / 0 |
| `game/new_gamelist.cpp` | `new_gamelist.cpp` | 29 + 2 initialiser pairs | 14 / 15 / 0 |
| `game/new_pxochat.cpp` | `new_pxochat.cpp` | 57 + 14 initialiser pairs | 20 / 36 / 1 |
| `game/multisave.cpp` | `multisave.cpp` (`0x46cc60`-`0x46da80`) | 10 | see `NOTES-multi.md` (38 / 23 / 1 for the four files) |
| `game/multitrade.cpp` | `multitrade.cpp` | 26 | |
| `game/multichat.cpp` | `multichat.cpp` | 13 | |
| `game/new_multichat.cpp` | `new_multichat.cpp` | 16 | 9 / 7 / 0 |
| `game/multicontainer.cpp` | `multicontainer.cpp` | 13 | |
| `game/gametarget.cpp` | `gametarget.cpp` | 26 | 12 / 14 / 0 |
| `game/multi_chattracker.cpp` | `multi_chattracker.cpp` | 20 | 11 / 9 / 0 |
| `game/multispell.cpp` | `multispell.cpp` | 22 | 16 / 6 / 0 |
| `game/multi_door.cpp`, `game/new_int_multi_update.cpp` | | 6 + 5 | |
| `game/multiendgame.cpp`, `new_multi.cpp`, `multiplayerdata.cpp`, `summoner_cd.cpp`, `pathfinding_visibility.cpp` (`engine\ai\`) | one file each | 48 | 17 / 29 / 2 (see `NOTES-multi-small.md`; summoner_cd is written without the no-CD byte patch the shipped exe carries, D2600) |
| `game/objects.cpp` | `objects.cpp` (`0x4697e0`-`0x46b45b`, unoptimised) | 48 | 46 / 1 / 1 (VC6 38 of 40 byte-identical, D6101) |
| `game/living_entity.cpp` | `living_entity.cpp` (`0x44d9f0`-`0x46335f`; unoptimised, the tail from `0x461dc0` optimised by `#pragma optimize`, D6100) | 160 | 118 / 42 / 0 (VC6 88 of 132 byte-identical) |
| `game/items.cpp` | `items.cpp` (`0x449d90`-`0x44d9ee`) | 41 | 31 / 10 / 0 |
| `game/characterinfo.cpp` | `characterinfo.cpp` | 122 | 81 / 26 / 15 (the 15 are empty Mempool initialisers) |
| `game/s3d.cpp` | `s3d.cpp` (`0x46dd30`-`0x470560`) | 52 | 37 / 15 / 0 |
| `game/levelrender.cpp` | `levelrender.cpp` (`0x48db30`-`0x492af0`) | 30 of 39 (the three draw-list back ends left) | 13 / 5 / 12 |
| `game/spells.cpp` | `spells.cpp` | 47 of 53 (spell_spawn_creature and summon_create left; decomp73 added spell_prepare_cast, spell_cast_execute, spell_cast_launch_effects) | 30 / 17 / 0 |
| `game/combat.cpp` | `combat.cpp` (`0x463360`-`0x468c77`: the bloodspots / death console commands first, D6100) | 66 of 69 (decomp73: combat_trigger_reflex_ability) | 48 / 17 / 1 |
| `game/boss_ai.cpp` | `boss_ai.cpp` (`0x4a04b0`-`0x4a3575`) | 59 | 42 / 17 / 0 |
| `game/ai.cpp` | `ai.cpp` (+ the 3 cast_effect functions after it, D1905) | 110 | 76 / 30 / 4 |
| `game/pathfinding.cpp` | `pathfinding.cpp` (`0x49c840`-`0x4a04af`) | 40 | 20 / 20 / 0 |
| `game/conversation.cpp` | `levelscripts\conversation.cpp` (`0x4d7b30`-..) | 32 | 22 / 10 / 0 |
| `game/conversationctl.cpp` | `conversationctl.cpp` (`0x4d87d0`-`0x4daf50`) | 40 | 23 / 16 / 1 |
| `game/script_internal.cpp` | `levelscripts\script_internal.cpp` | 26 | 10 / 15 / 1 |
| `game/level_scripts.cpp` | `levelscripts\level_scripts.cpp` (`0x4ceec0`-`0x4d6651`, savegame code included) | 124 + 34 initialiser helpers | 96 / 27 / 1 |
| `game/level_scripts_common.cpp` | `levelscripts\Scripts\level_scripts_common.cpp` (`0x4ca700`-`0x4ceebf`) | 92 | 71 / 17 / 4 |
| `game/level_script_khosani_lab.cpp` | `level_script_khosani_lab.cpp` | 48 | 33 / 15 / 0 |
| `game/level_script_worldmap.cpp` | `level_script_worldmap.cpp` | 31 | 18 / 13 / 0 |
| `game/level_script_sewers.cpp` | `level_script_sewers.cpp` | 21 | 18 / 3 / 0 |
| `game/level_script_templeint2.cpp` | `level_script_templeint2.cpp` | 16 | 14 / 2 / 0 |
| `game/level_script_jadetemple.cpp` | `level_script_jadetemple.cpp` | 13 | 8 / 5 / 0 |
| `game/level_script_wolong2.cpp` | `level_script_wolong2.cpp` | 15 | 14 / 1 / 0 |
| `game/level_script_lpalaceint.cpp` | `level_script_lpalaceint.cpp` | 21 | 12 / 9 / 0 |
| `game/level_script_liangshan.cpp` | `level_script_liangshan.cpp` | 20 | 19 / 1 / 0 |
| `game/level_script_wolong_caverns.cpp` | `level_script_wolong_caverns.cpp` | 24 | 15 / 5 / 4 |
| `game/level_script_catacombs.cpp` | `level_script_catacombs.cpp` | 15 | 13 / 2 / 0 |
| 13 more `game/level_script_*.cpp` (temple_inside, slums, iona_ext2, ikaemosu, ikaemosl2, courtyard, lenele2aa, lenele1aa, lpalaceint02, eleh, wolong, lenele3d, lenele3a) | one file each | 86 | 80 / 6 / 0 (see `NOTES-level-scripts-13.md`) |


Files named `unnamed_<what>_<address>.cpp` hold a translation unit whose original file name is not known (no path string, no strings.tbl block); the name is a placeholder, never a guess at the original.

**Progress:** `node tools/decomp-progress.mjs` counts the written functions per original file.

**Headers.** `include/` is one consistent header set laid out like the original source tree
(`vsdk/os/memory.h`, `vsdk/vfile/file.h`, `engine/objects/living_entity.h`, `summoner/player/player.h`, ...):
see [include/README.md](include/README.md) for the scheme and what each header covers, and
[include/DECISIONS.md](include/DECISIONS.md) for how the conflicts between the old per-batch stub headers
were resolved (with the disassembly evidence). The unit headers (`timestamp.h`, `window.h`, `new_*.h`, ...)
live in the same tree; the `.cpp` files stay in `vsdk/` and `game/` and include headers by path from the
include root. `symbols/types.h` stays the C reference; the headers mirror its layouts (same offsets, size
checks with `LAYOUT_CHECK`) and add the thiscall methods.

Syntax check (mingw is not the target compiler; it only catches syntax and type errors; VC6 accepts string
literals as `char*`):

```
i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I src/decomp/include src/decomp/game/new_credits.cpp
```
