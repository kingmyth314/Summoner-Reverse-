# src/decomp/include: engine and game headers

One header set for every pilot `.cpp` (it replaces the per-batch `stubs_*.h`, `ui_screen.h` and
`dialogue_box.h`). Not original source: written from Sum.exe v1.40's disassembly and the repo's symbols;
Summoner 2 and Dash Faction give names only.

## Scheme

- **One include root.** Compile with `-I src/decomp/include` only. Every `#include` names a path from the
  root: `#include "vsdk/os/memory.h"`. No header includes `symbols/types.h`.
- **Directories follow the original source tree.** Sum.exe's assert paths are
  `D:\projects\Summoner\pccode\{vsdk,Engine,Summoner,levelscripts}\...` (docs/sum-exe/source-map.md). The
  tree uses the same top-level directories in lower case: `vsdk/`, `engine/`, `summoner/`, `levelscripts/`.
- **One header per original unit**, named after its `.cpp` when the name is known (assert path,
  `strings.tbl` section, or the module column of `sum.symbols`). A header whose file or directory name is
  guessed says so in its first comment ("directory guessed", "file name ours"). A few headers
  (`summoner/summoner.h`, `summoner/interface/interface.h`, `summoner/multi/multi.h`,
  `levelscripts/level_scripts.h`) gather several small units for now; their first comment lists them.
- **Pilot unit headers** (`timestamp.h`, `window.h`, `new_status.h`, ...) are in the tree next to the engine
  headers; the `.cpp` files stay in `src/decomp/vsdk/` and `src/decomp/game/`.
- **Every header is self-contained** (it compiles on its own) and starts with `vsdk/vsdk.h`
  (ubyte / ushort / uint, `LAYOUT_CHECK`, min / max, list macros, the CRT headers).
- **Layouts mirror symbols/types.h** (same offsets and names; `types.h` stays the C reference and is not
  edited). Sizes are checked with `LAYOUT_CHECK(name, cond)` (a negative array typedef: VC6 has no
  `static_assert`), e.g. Character 0x788, Player 0x188, ItemType 0x100, ai_state 0x234, obj 0x98.
- **Classes**, not C structs, wherever the game calls thiscall members: the members carry their address.
  Virtuals are declared in vtable slot order where the vtable is known (ui_element 0x581ea0, ui_gadget
  0x581e08, ui_container 0x581f40, ui_window 0x581df4, ...). Slot 0 of the widget vtables is `dtor()`, not
  a C++ destructor (it takes no flags and never frees).
- **Names**: functions and globals from `symbols/sum.symbols`; `Class__method` symbols become members.
  Names that are ours are marked. Include guards are `<DIR>_<FILE>_H` for the new headers; the moved
  unit headers keep their old guards.
- **Every decision between conflicting stub declarations** is in [DECISIONS.md](DECISIONS.md).

## Headers

| Header | Original unit(s) | Contents |
|---|---|---|
| `vsdk/vsdk.h` | (ours) | basic typedefs, `LAYOUT_CHECK`, min/max, list macros, CRT headers |
| `vsdk/os/debug.h` | debug (0x531cb0) | `DebugError`, `VSDK_ERROR` loop macro |
| `vsdk/os/memory.h` | `vsdk\os\memory.cpp` (decomp22) | `Mempool`, `Pool`, common block, level memory counters, memory report |
| `vsdk/os/crc32.h` | Crc32 (0x5520d0-0x552190) | `Crc32` (directory and file name guessed) |
| `vsdk/os/timestamp.h` | timestamp.cpp (pilot) | `GameTimer`, `RealtimeTimer`, tickers |
| `vsdk/os/frametime.h` | frametime.cpp (pilot) | frame time globals and functions |
| `vsdk/os/timer.h` | timer.cpp | `timer_init`, `timer_get`, `timer_wait` |
| `vsdk/os/os.h` | vsdk\os (one object 0x530b90-0x531bea, source at src/decomp/vsdk/unnamed_os_0x530b90.cpp, file name a placeholder, decomp50; the two -1 stubs 0x531bf0 / 0x531c00 in unnamed_os_0x531bf0.cpp) | `os_init`, `os_poll`, `os_get_main_wnd`, `os_foreground`, `os_wndproc`, the window / mutex / message-handler globals, `os_clipboard_get_text` (D3006, D3007) |
| `vsdk/os/exceptionhandler.h` | exceptionhandler (0x5419a0-0x5424fa; source at src/decomp/vsdk/exceptionhandler.cpp, directory and file name placeholders, decomp26 / decomp49) | `RecordExceptionInfo` (the helpers are file-static, D2905) |
| `vsdk/os/registry.h` | `vsdk\os\registry.cpp` (pilot) | `os_registry_*` |
| `vsdk/os/cmdline.h` | `vsdk\os\cmdline.cpp` (pilot) | `CmdLineParam`, `cmdline_*` |
| `vsdk/os/console.h` | console.cpp (0x50a000-0x50af59, decomp44) | `console_command`, `ARG_*` / `TOKEN_*` / `CHAR_*`, the console globals and functions, `string_buffer` |
| `vsdk/os/norlsallocpool.h` | norlsallocpool.cpp (0x532360-0x532653; source at src/decomp/vsdk/norlsallocpool.cpp, decomp44; directory guessed) | `NoRlsAllocPool` |
| `vsdk/os/stringpool.h` | `vsdk\os\stringpool.cpp` (decomp22) | `StringPool` |
| `vsdk/os/mouse.h` | vsdk\os\mouse (0x500a30-0x501468; source at src/decomp/vsdk/mouse.cpp, file name a placeholder, decomp49) | `mouse_button`, the unit globals, every `mouse_*`, `clamp_int` (D2902) |
| `vsdk/os/key.h` | key (0x509ca0) | `KEY_*` scan codes, `key_to_ascii`, `key_names` |
| `vsdk/os/vstring.h` | String (0x5567f0) | `String`, the String `operator+` |
| `vsdk/os/plainstring.h` | plainstring.cpp (0x514ec0-0x515955, decomp22; directory guessed) | `is_blank`, `trim_*`, `stristr`, `int_to_string*`, `wide_charmap_*`, ... |
| `vsdk/os/profile.h` | (ours) | `profile_label` |
| `vsdk/os/movie.h` | movie (0x5172f0) | `movie_play` |
| `vsdk/math/vector.h` | vsdk\math (the vector unit 0x523f30-0x525710, source at src/decomp/vsdk/unnamed_vector_0x523f30.cpp, file name a placeholder, decomp60) | `vector3`, `vector2` and every member, the free vector functions, `imax`, `imin`, `frand`, inline `float_max` / `float_min` (D4004) |
| `vsdk/math/matrix.h` | `vsdk\math\matrix.cpp` | `matrix` (= `matrix3`), `matrix43`; includes quaternion.h |
| `vsdk/math/quaternion.h` | vsdk\math (the quaternion unit 0x5225b0-0x523f29, source at src/decomp/vsdk/unnamed_quaternion_0x5225b0.cpp, file name a placeholder, decomp60) | `quaternion`, `cquat` (moved from matrix.h / skeleton.h, D4013), slerp / squad / log-of-delta, the identities |
| `vsdk/math/fixed.h` | vsdk\math (file name ours, decomp60) | inline `fixed8`, `float_to_fixed8`, `fixed8_to_float`, `fixed4_to_float` (D4004) |
| `vsdk/math/ix.h` | vsdk\math (ix_ray_plane 0x53ce30, ix_point_linesegment 0x53da80; file name ours, decomp35) | `plane` (inline set_normal_point), `ix_ray`, `ix_*` |
| `vsdk/inetfile/inetgetfile.h` | InetGetFile (0x51b760-0x51bb10) | `InetGetFile` (directory and file name guessed) |
| `vsdk/vfile/file.h` | vsdk\vfile (decomp61: `unnamed_vfile_0x5128b0.cpp`, `unnamed_vfile_rw_0x525720.cpp`) | `File`, `FileReader`, `file_slot`, `lfsr_cipher`, `file_path_entry`, `PATH_ANY`, `FILE_*`, `file_*`, `file_find_*` |
| `vsdk/vfile/linefile.h` | LineFile (0x558340-0x558a40; unit, directory and file name not known, decomp44) | `LineFile` (what load_script uses), `LineFile_open` |
| `vsdk/vfile/packfile/file_packfile.h` | `vsdk\vfile\packfile\file_packfile.cpp` (0x533db0-0x534a1b, decomp49) | `Packfile`, `PackfileEntry`, `PackfileLookup`, `packfile_header`, the unit globals, every `packfile_*` (D2900, D2901) |
| `vsdk/parse/parse.h` | `vsdk\parse\parse.cpp` (decomp22) | `TextFile`, to-text helpers, `file_blast_*` |
| `vsdk/bmpman/bmpman.h` | `vsdk\bmpman\bmpman.cpp` (decomp31) | `bitmap_entry`, `BM_TYPE_*` / `BM_ANIM_*` / `BM_FORMAT_*`, the unit's globals, every `bm_*`, `progress_update` |
| `vsdk/bmpman/targautils.h` | `vsdk\bmpman\targautils.cpp` (pilot) | TGA / PCX / m2v, `m2v_bitstream` |
| `vsdk/gr/gr.h` | `vsdk\gr\gr.cpp` (decomp40) + vsdk\gr | `Color`, `gr_mode`, `gr::Mode` (D1300, D2001, D2002), `gr__Screen` (0xa8 with the colour / fog / tint fields, D2000), the 30 mode globals, `gr_*`, `gr::` clip, `gr_light_desc`, `tcache_slot` (the texture stage slot, decomp45); includes gr_font.h |
| `vsdk/gr/gr_font.h` | `vsdk\gr\gr_font.cpp` (pilot) | `Font`, `FontCharInfo`, `gr_string`, `gr_split_str`, ... |
| `vsdk/gr/opengl/gl_min.h` | (ours; stands in for `<GL/gl.h>` / `<GL/glu.h>`, D1305) | the GL 1.1 / GLU functions and constants Sum.exe imports, the multitexture pointer type |
| `vsdk/gr/opengl/gr_opengl.h` | `vsdk\gr\opengl\gr_opengl.cpp` (decomp33) | `gl_tcache_entry`, the OpenGL back end and its globals |
| `vsdk/gr/glide/glide_min.h` | (ours; stands in for `<glide.h>`, D2500) | the Glide 2 types (GrVertex, GrTexInfo, GrLfbInfo_t, GrHwConfiguration), the constants the unit pushes, the 112 __stdcall entry point pointer types |
| `vsdk/gr/glide/gr_glide.h` | `vsdk\gr\glide\gr_glide.cpp` (decomp45; decomp40 declared the dispatch targets) | `glide_section`, `glide_tcache_entry`, `glide_block`, `glide_record` (TMU memory, derives from `tcache_slot`), the entry point pointers and the unit globals, all `gr_glide_*` / `glide_*` |
| `vsdk/gr/directx/d3d_min.h` | (ours; stands in for `<ddraw.h>` / `<d3d.h>`, D1600) | the DirectDraw 7 / Direct3D 7 structures, C-style interfaces and the SDK call macros, IIDs and constants the Direct3D back end uses; not combinable with the system DirectX headers |
| `vsdk/gr/directx/gr_direct3d.h` | `vsdk\gr\directx\gr_direct3d.cpp` and the state / drawing object before it (decomp36, D1601) | `d3d_tlvertex`, `d3d_device_record`, `d3d_color_channel`, the Direct3D back end and its globals, the D3D texture cache calls |
| `vsdk/localize/localize.h` | localize (0x503f60-0x505547, source at src/decomp/vsdk/unnamed_localize_0x503f60.cpp, file name a placeholder, decomp50) | `localize_strings`, `XSTR`, `localize_node` (the name hash tables), `localize_language_info`, every `localize_*` (D3000-D3003) |
| `vsdk/snd/snd.h` | the sound unit 0x5204c0-0x521e38 and the EAX unit 0x521e40-0x5225a1 (src/decomp/vsdk/unnamed_snd_0x5204c0.cpp, unnamed_eax_0x521e40.cpp, file names placeholders, decomp60; D4010) | `snd_sound`, the sound globals, every `snd_*` / `eax_*`, the sound emitter calls, `clamp_uint` |
| `vsdk/snd/ds.h` | (ours; the ds_* layer 0x55c000-0x55dfe0 and EAX 2.0, D4008) | `ds_*` prototypes, `IKsPropertySet`, `EAXLISTENERPROPERTIES`, the EAX GUIDs and constants |
| `vsdk/net/net.h` | vsdk\net (0x517590-0x5194ab, source at src/decomp/vsdk/unnamed_net_0x517590.cpp, file name a placeholder, decomp50) | `net_addr`, the packet buffers, `reliable_header`, `reliable_socket`, `RNT_*` / `RNF_*`, every `nw_*` (D3004, D3005) |
| `vsdk/ca/skeleton.h` | `vsdk\ca\skeleton.cpp` | `anim_track`, `skeleton`, `vim` (= `ca_vim`), `vim_load` |
| `vsdk/ca/character.h` | `vsdk\ca\character.cpp` (decomp30) | `ca_character` (0x1f8, members), `ca_mesh` (mesh slots), `ca_vertex_weights`, preload index, `ca_*` / `character_*`, `ca_render_*` |
| `vsdk/ca/character_instance.h` | `vsdk\ca\character_instance.cpp` (pilot) | `character_instance` |
| `vsdk/geom/vfx.h` | vsdk\geom; the attached-light unit 0x51fb10-0x520413 (src/decomp/vsdk/unnamed_attached_lights_0x51fb10.cpp, file name a placeholder, decomp60) | `vfx_base`, `vfx_emitter_desc`, the key channels, `vfx_light_track`, `vfx_light_desc`, `vfx_instance` (D4000, D4001), `attached_light` and `attached_light_*`, `ptr_relocate` |
| `vsdk/gr/unnamed_lights_0x51e070.h` | the light unit 0x51e070-0x51fb08 (src/decomp/vsdk/unnamed_lights_0x51e070.cpp, file name a placeholder, decomp60; included by gr.h) | `light` (0x94), the light table, selection levels and ambient, every `light_*` / `lights_*`, `gr_env_map_uv`, the gr 3D state it reads |
| `vsdk/geom/effect_mem.h` | `vsdk\geom\effect_mem.cpp` (pilot) | effect bases and pools |
| `vsdk/ui/ui.h` | vsdk\ui | `ui_tooltip`, `ui_element`, `ui_gadget`, UI globals and events |
| `vsdk/ui/ui_gadgets.h` | vsdk\ui (decomp61: `unnamed_ui_button_0x526940.cpp`, `unnamed_ui_static_0x527e20.cpp`) | button (`UI_BUTTON_*`, `ui_drag_gadget`), static, checkbox, radio, inputbox, listbox, slider |
| `vsdk/ui/ui_container.h` | `vsdk\ui\ui_container.cpp` (pilot) | `ui_container` |
| `vsdk/ui/window.h` | `vsdk\ui\window.cpp` (pilot) | `ui_window` |
| `engine/objects/objects.h` | `Engine\objects\objects.cpp` | `obj`, `item_obj`, `container`, party inventory |
| `engine/objects/items.h` | `Engine\objects\items.cpp` | `ItemType`, `item_info` (= `inv_item`), `item_*` |
| `engine/objects/item_paging.h` | `Engine\objects\item_paging.cpp` (pilot) | item mesh paging |
| `engine/objects/living_entity.h` | `Engine\objects\living_entity.cpp` | `Character`, `living_combat`, `CF_*`, status ids |
| `engine/characterinfo/characterinfo.h` | `Engine\characterinfo\characterinfo.cpp` | `char_info`, skills, chain attacks, experience |
| `engine/ai/ai.h` | ai.cpp (`Engine\ai\`) | `ai_state` (all ai.cpp members, decomp39), `ai_goal`, `ai_group`, the unit's name tables, console variables, profiler (`ai_profile`) and cast effect tracks; includes the avoidance header below (decomp57) |
| `engine/ai/unnamed_avoidance_0x4a6710.h` | obstacle avoidance 0x4a6710-0x4aa77f (game/unnamed_avoidance_0x4a6710.cpp, file name a placeholder; decomp57) | `avoid_area_desc`, `avoid_obstacle` (= `avoid_prim`), `avoid_poly`, `avoid_seglist` with its segment refs, corners and link blocks, the areas and splines, every `avoid_*` (D3701-D3706) |
| `engine/ai/pathfinding.h` | `Engine\ai\pathfinding.cpp` (decomp32) and the geometry unit 0x4a4ee0-0x4a670f (game/unnamed_pfgeom_0x4a4ee0.cpp, placeholder; decomp57) | .pfg graph types (`pf_graph`, `pf_vis_node`, `pf_edge`, `pf_cell`, `pf_bsp`, `pf_grid`), `pf_sweep`, `ai_path`, the unit's functions and globals, the geometry calls and tables |
| `engine/ai/pathfinding_visibility.h` | `engine\ai\pathfinding_visibility.cpp` (decomp47) | the visibility sweep: `pathvis_edge`, the unit's .bss, `pathvis_*`, `determine_point_visibility` |
| `engine/gamesound/gamesound.h` | `Engine\gamesound\gamesound.cpp` (pilot) | `sound_ref`, game sound and music |
| `engine/rendering/levelrender.h` | `Engine\rendering\levelrender.cpp` (decomp28, partial) | `lobj`, `lobj_face`, `door_info`, level render calls, wave / light helpers; decomp58: the door, lobj and level-record helpers of game/unnamed_goals_doors_0x475180.cpp (placeholder), the `door_info` state bit field |
| `engine/rendering/unnamed_decals_0x4bbe80.h` | decal unit 0x4bbe80-0x4bce55 (source game/unnamed_decals_0x4bbe80.cpp, decomp56; file and directory names placeholders) | `decal_record`, `decal_node`, the decal rings and `decal_*` / `decals_*` |
| `engine/rendering/unnamed_projectors_0x4c0250.h` | projector unit 0x4c0250-0x4c0a3e (source game/unnamed_projectors_0x4c0250.cpp, decomp59; names placeholders, decomp56) | `projector_record`, `shadow_record` and `shadow_frame` (decomp57), `geom_bsp_node`, `lobj_face_marks`, the face walk and `lobj_project_records` (D3900) |
| `engine/rendering/unnamed_lamp_glow_0x4c0a40.h` | lamp glow unit 0x4c0a40-0x4c0c3a (source game/unnamed_lamp_glow_0x4c0a40.cpp, decomp59; names placeholders) | `lamp_glow`, `lamp_glows`, every `lamp_glow_*` (D3902) |
| `engine/rendering/unnamed_shadows_0x4a37d0.h` | obstacle debug drawing and blob shadows 0x4a37d0-0x4a4ed7 (game/unnamed_shadows_0x4a37d0.cpp, placeholder; decomp57) | `blob_shadow`, the debug switches, `shadow_*`, `cmd_obstacle_height_offset` |
| `engine/rendering/unnamed_render_scene_0x481680.h` | the level frame 0x481680-0x48743f (game/unnamed_render_scene_0x481680.cpp, placeholder; decomp63; included by levelrender.h) | `level_layer_vis_rec`, `detail_level`, `lobj_glow_ref`, `overhead_label_pos`, `render_scope`, the unit globals (fog, layers, vis sets, draw lists, labels), the draw-list, label, lamp-glow and .vis functions (D4301, D4305) |
| `engine/rendering/unnamed_shadow_table_0x487440.h` | shadows.tbl and character shadows 0x487440-0x48789f (game/unnamed_shadow_table_0x487440.cpp, placeholder; decomp63; included by levelrender.h) | `shadow_entry` (D4308), the shadow .data floats, `shadow_get_character_anchor`, `shadow_draw_character`, `shadows_draw_characters` |
| `engine/s3d/s3d.h` | `Engine\s3d\s3d.cpp` (decomp28) | `s3d_level level_s3d` and its record types, the `num_lobjs` / `lobjs` / ... field macros (D800) |
| `engine/camera/camera.h` | the camera unit 0x4778b0-0x479b59 (game/unnamed_camera_0x4778b0.cpp, file name a placeholder, decomp58) | `camera` (all members), `game_camera`, `CAMF_*`, the unit's globals and console commands, `camera_approach_value` |
| `engine/camera/camera_sequence.h` | camera sequences 0x493f20-0x4946a0 (unit not known; file name ours, decomp52) | `camera_sequence`, `cutscene_camera_sequence` (the 0x70-byte cutscene shot) |
| `engine/effects/effect_stream.h` | effect streamer (0x440400-0x440b3a; source game/unnamed_effectstream_0x440400.cpp, file name a placeholder; decomp53) | `effect_stream_entry`, the unit's globals, every `effect_stream_*`, `file_type_from_ext` (D3300) |
| `engine/effects/fx.h` | the effect object unit 0x489c40-0x48d7bf (game/unnamed_fx_0x489c40.cpp, file name a placeholder; decomp64) | `effect_object` (ctor / dtor / destroy, the +0xbc bit fields), `fx_char_link`, the unit's .data / .bss (pool, lists, sky list, blob shadow bitmaps), every `fx_*` and the creators |
| `engine/spells/spells.h` | spells.cpp (0x479b60-0x47f857, pilot decomp23) | `spell_info`, `spells`, groups, missiles, spell effects, the cast request, `spell_*` / `summon_*` |
| `engine/combat/combat.h` | combat.cpp (0x463540-0x468c77, pilot decomp23; directory guessed) | `living_combat` (Character +0x558) and the combat helpers |
| `engine/ai/boss_ai.h` | boss_ai.cpp (0x4a04b0-0x4a3580) and the per-boss units | the boss hooks combat.cpp calls, `rand_int_range`, `ghost_stage` |
| `engine/ai/unnamed_giant_salamanka_0x4c8030.h` | the Giant Salamanka unit 0x4c8030-0x4c8d03 (game/unnamed_giant_salamanka_0x4c8030.cpp, decomp59; placeholder; included by boss_ai.h) | `giant_salamanka_boss gsal` and the sum.symbols names as macros (D3904), the unit's commands and helpers |
| `summoner/summoner.h` | summoner.cpp, gameplay.cpp, gametarget.cpp, gameseq | game flags, pause, level change, path ids, `SummonerOptions` |

| `engine/spells/spells.h` | spells.cpp (0x47a190..) | `spell_info`, `spells`, `spell_display_names` |
| `summoner/summoner.h` | summoner.cpp (decomp26), summoner_cd.cpp, gameplay.cpp, gametarget.cpp, gameseq | game flags, pause, level change, MainLoop / LoadSettings / GameInit, path ids, fonts, command-line flags, `SummonerOptions` |
| `summoner/unnamed_input_0x4343a0.h` | the player input unit 0x4343a0-0x4348f6 (`game/unnamed_input_0x4343a0.cpp`, file name a placeholder; decomp52; included by summoner.h) | slew ccmd, `player_input_frame`, the mouse input record bits, the camera keys |
| `summoner/unnamed_script_editor_0x4de0b0.h` | the script editor unit 0x4de0b0-0x4ecf90 (`game/unnamed_script_editor_0x4de0b0.cpp`, file name a placeholder; decomp55) | editor modes, pick records, script_action_defaults, all editor gadgets and globals, the editor functions and console commands |
| `summoner/unnamed_cutscene_0x434900.h` | the cutscene file 0x434900-0x437659 (`game/unnamed_cutscene_0x434900.cpp`, file name a placeholder; decomp52; included by summoner.h and level_scripts.h) | `cutscene_entry` and its records, `class cutscene` (`the_cutscene`, field macros `cutscene_data` ...), every `cutscene_*` function, the cutscene ccmds (D3200) |
| `summoner/gameplay.h` | gameplay.cpp (0x437660-0x43a32f, decomp42; included by summoner.h) | the cheat commands, level load / unload, state 4 and state 5 (help), party control, the level names and layer maps, the loading bar and help tables |
| `summoner/gametarget.h` | gametarget.cpp (0x43a330-0x43cd6d, decomp35; included by summoner.h) | world picking and clicks, `WORLD_CLICK_*`, the tints, the unit's .bss / .data |
| `summoner/automap.h` | automap.cpp (0x42ffe0-0x4302b0) | `automap_label`, map labels, corners, reveal grid |
| `summoner/player/player.h` | `Summoner\player\player.cpp` (pilot) | `Player`, party lists |
| `summoner/multi/multi.h` | multi*.cpp (gathering header) | `mp_player`, packets (`mp_packet_table`, `MP_IS_HOST`, build macros), session, game list, lobby, character select records; includes the four unit headers below |
| `summoner/multi/unnamed_mpmenus_0x43eb00.h` | multiplayer menu states 0x43eb00-0x43ef24 (game/unnamed_mpmenus_0x43eb00.cpp, file name a placeholder; decomp53; included by multi.h) | states 10 / 12 / 13 / 14, the level availability checks, `mp_host_game`, `All_multi_levels` |
| `summoner/multi/unnamed_lobby_0x43ef30.h` | PXO lobby 0x43ef30-0x43f9cc (game/unnamed_lobby_0x43ef30.cpp, placeholder; decomp53) | `lobby_channel`, the lobby globals, state 11, every `lobby_*` |
| `summoner/multi/unnamed_netsession_0x440b40.h` | net session 0x440b40-0x441817 (game/unnamed_netsession_0x440b40.cpp, placeholder; decomp53) | `mp_netgame_info mp_netgame` and the old names as macros (D3305), `MP_STATUS_*`, the player lists and records, the host / client frames, GM_STAT / CT_STAT |
| `summoner/multi/unnamed_netpackets_0x441820.h` | packet layer 0x441820-0x4425a6 (game/unnamed_netpackets_0x441820.cpp, placeholder; decomp53) | `mp_packet_table`, `mp_send_buffer`, the receive / send / broadcast / forward calls, the net statistics, `cmd_disp_rate` |
| `summoner/multi/unnamed_utrack_0x48d7c0.h` | PXO user tracker login 0x48d7c0-0x48db2f (game/unnamed_utrack_0x48d7c0.cpp, placeholder; decomp64; included by multi.h) | `utrack_packet`, `utrack_login_request`, the login globals, `pxo_login_send` / `utrack_init` / `pxo_login_request` / `pxo_login_poll` |
| `summoner/unnamed_mlayout_0x43f9d0.h` | state 6 and the monster layouts 0x43f9d0-0x4403f2 (game/unnamed_mlayout_0x43f9d0.cpp, placeholder; decomp53) | the state 6 gadgets, `monster_layout_save` / `_load`, the load_mlayout / save_mlayout commands |
| `summoner/multi/multisave.h` | multisave.cpp (0x46cc60-0x46dd2f) | `.plr` files, `mp_char_summary`, autosave timer, `pxo_config` |
| `summoner/multi/multitrade.h` | multitrade.cpp (0x470e30-0x471def) | trade state, `mp_trade_item`, IT_* senders and handlers |
| `summoner/multi/multichat.h` | multichat.cpp (0x487b90-0x488223) | `chat_entry` ring, slash commands, CHAT_LN |
| `summoner/multi/multicontainer.h` | multicontainer.cpp (0x488c00-0x48972a) | loot drops, container open / close, lock picking |
| `summoner/multi/multispell.h` | multispell.cpp (0x4afca0-0x4b0957, decomp35) | RLS_SPL / CST_ANM casts, the one-summon claim (SMN_*) |
| `summoner/multi/multi_chattracker.h` | multi_chattracker.cpp (0x4abb80-0x4ae32d, decomp35) | the PXO lobby's IRC client: `irc_command`, `irc_user`, `irc_channel`, `irc_*` |
| `summoner/multi/multiplayerdata.h` | multiplayerdata.cpp (0x4af340-0x4afc9f, decomp47) | NET_INF / NEW_INF / PLY_DAT, the player record packing |
| `summoner/multi/multiendgame.h` | multiendgame.cpp (0x4811a0-0x481673, decomp47) | leaving a game, GM_OVER / LV_GAME, the Game Over box, `MP_LEAVE_*`, Multi_path_type |
| `summoner/interface/new_multi.h` | new_multi.cpp (0x41dde0-0x41e3cf, decomp47) | the multiplayer elevators (ui_screen_stop_player stays in interface.h) |
| `summoner/interface/interface.h` | the interface manager 0x411270-0x414cb0 (`game/unnamed_interface_0x411270.cpp`, file name a placeholder; decomp48), new_hud / new_inventory helpers | screen switch and dispatch, view clip, button records, popups, tooltip, HUD message lines, colours, model lights, `new_icam`, show_ui |
| `summoner/interface/new_pxo_banner.h` | `new_pxo_banner.cpp` | `open_url` |
| `summoner/interface/new_int_multi_update.h` | `new_int_multi_update.cpp` (0x410cf0-0x411260, decomp49) | `version_check_*`, `version_read_nfo_file` |
| `summoner/interface/new_*.h` | `new_*.cpp` (pilots) | the screen units' prototypes |
| `summoner/interface/new_skill.h`, `new_spell.h`, `new_tutorial.h` | new_skill.cpp, new_spell.cpp, new_tutorial.cpp (not pilots) | the functions and data the HUD and the inventory call |
| `levelscripts/level_scripts.h` | level_scripts(_common).cpp | `level_script_info`, level tables, shops, quests, `level_trigger`, `script_action` (decomp41) |
| `levelscripts/conversation.h` | `levelscripts\conversation.cpp`, conversationctl.cpp | dialogue boxes; speakers, topics, lines; `conversation` (0x25461a0), `conversation_controller` (0x25464b0) |
| `levelscripts/savegame.h` | savegame (0x4cfca0) | `savegame`, `savegame_state` |
| `levelscripts/script_internal.h` | `levelscripts\script_internal.cpp` (0x4db340-0x4de0a3, assert path; decomp44) | `SCRIPT_ACTION_*`, `NAVPOINT_TYPE_*`, the action / navpoint type names, `cutscene_saved_item`, every function of the unit |
| `summoner/interface/new_multichat.h` | new_multichat.cpp (0x41e3d0-0x41ee8f, decomp44; included by interface.h) | `chat_line`, the chat overlay's globals, layout tables and functions |
