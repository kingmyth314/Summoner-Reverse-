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
| `vsdk/os/os.h` | vsdk\os (0x530c00-0x531280; file name guessed, decomp26) | `os_init`, `os_poll`, `os_get_main_wnd`, `os_close_main_window`, `os_seed_random` |
| `vsdk/os/exceptionhandler.h` | exceptionhandler (0x5419a0; directory and file name guessed, decomp26) | `RecordExceptionInfo` |
| `vsdk/os/registry.h` | `vsdk\os\registry.cpp` (pilot) | `os_registry_*` |
| `vsdk/os/cmdline.h` | `vsdk\os\cmdline.cpp` (pilot) | `CmdLineParam`, `cmdline_*` |
| `vsdk/os/console.h` | console.cpp | `console_command`, console globals, `string_buffer` |
| `vsdk/os/stringpool.h` | `vsdk\os\stringpool.cpp` (decomp22) | `StringPool` |
| `vsdk/os/mouse.h` | vsdk\os\mouse | `mouse_*` |
| `vsdk/os/key.h` | key (0x509ca0) | `KEY_*` scan codes, `key_to_ascii`, `key_names` |
| `vsdk/os/vstring.h` | String (0x5567f0) | `String`, the String `operator+` |
| `vsdk/os/plainstring.h` | plainstring.cpp (0x514ec0-0x515955, decomp22; directory guessed) | `is_blank`, `trim_*`, `stristr`, `int_to_string*`, `wide_charmap_*`, ... |
| `vsdk/os/profile.h` | (ours) | `profile_label` |
| `vsdk/os/movie.h` | movie (0x5172f0) | `movie_play` |
| `vsdk/math/vector.h` | vsdk\math | `vector3`, `imax`, `imin`, `frand` |
| `vsdk/math/matrix.h` | `vsdk\math\matrix.cpp` | `matrix` (= `matrix3`), `quaternion`, `matrix43` |
| `vsdk/math/ix.h` | vsdk\math (ix_ray_plane 0x53ce30, ix_point_linesegment 0x53da80; file name ours, decomp35) | `plane` (inline set_normal_point), `ix_ray`, `ix_*` |
| `vsdk/inetfile/inetgetfile.h` | InetGetFile (0x51b760-0x51bb10) | `InetGetFile` (directory and file name guessed) |
| `vsdk/vfile/file.h` | vsdk\vfile | `File`, `PATH_ANY`, `file_*`, `file_find_*` |
| `vsdk/vfile/packfile/file_packfile.h` | `vsdk\vfile\packfile\file_packfile.cpp` | `packfile_read_whole_file`, `packfile_find_in_packfile` |
| `vsdk/parse/parse.h` | `vsdk\parse\parse.cpp` (decomp22) | `TextFile`, to-text helpers, `file_blast_*` |
| `vsdk/bmpman/bmpman.h` | `vsdk\bmpman\bmpman.cpp` (decomp31) | `bitmap_entry`, `BM_TYPE_*` / `BM_ANIM_*` / `BM_FORMAT_*`, the unit's globals, every `bm_*`, `progress_update` |
| `vsdk/bmpman/targautils.h` | `vsdk\bmpman\targautils.cpp` (pilot) | TGA / PCX / m2v, `m2v_bitstream` |
| `vsdk/gr/gr.h` | `vsdk\gr\gr.cpp` + vsdk\gr | `Color`, `gr_mode`, `gr::Mode` (D1300), `gr__Screen`, `gr_*`, `gr::` clip, `gr_light_desc`; includes gr_font.h |
| `vsdk/gr/gr_font.h` | `vsdk\gr\gr_font.cpp` (pilot) | `Font`, `FontCharInfo`, `gr_string`, `gr_split_str`, ... |
| `vsdk/gr/opengl/gl_min.h` | (ours; stands in for `<GL/gl.h>` / `<GL/glu.h>`, D1305) | the GL 1.1 / GLU functions and constants Sum.exe imports, the multitexture pointer type |
| `vsdk/gr/opengl/gr_opengl.h` | `vsdk\gr\opengl\gr_opengl.cpp` (decomp33) | `gl_tcache_entry`, the OpenGL back end and its globals |
| `vsdk/localize/localize.h` | localize | `localize_strings`, `XSTR`, `localize_*` |
| `vsdk/snd/snd.h` | snd (0x5207d0) | `snd_*`, `eax_*` |
| `vsdk/net/net.h` | vsdk\net | `nw_*` globals |
| `vsdk/ca/skeleton.h` | `vsdk\ca\skeleton.cpp` | `anim_track`, `skeleton`, `vim` (= `ca_vim`), `vim_load` |
| `vsdk/ca/character.h` | `vsdk\ca\character.cpp` (decomp30) | `ca_character` (0x1f8, members), `ca_mesh` (mesh slots), `ca_vertex_weights`, preload index, `ca_*` / `character_*`, `ca_render_*` |
| `vsdk/ca/character_instance.h` | `vsdk\ca\character_instance.cpp` (pilot) | `character_instance` |
| `vsdk/geom/vfx.h` | vsdk\geom | `vfx_base`, `ptr_relocate` |
| `vsdk/geom/effect_mem.h` | `vsdk\geom\effect_mem.cpp` (pilot) | effect bases and pools |
| `vsdk/ui/ui.h` | vsdk\ui | `ui_tooltip`, `ui_element`, `ui_gadget`, UI globals and events |
| `vsdk/ui/ui_gadgets.h` | vsdk\ui | button, static, checkbox, radio, inputbox, listbox, slider |
| `vsdk/ui/ui_container.h` | `vsdk\ui\ui_container.cpp` (pilot) | `ui_container` |
| `vsdk/ui/window.h` | `vsdk\ui\window.cpp` (pilot) | `ui_window` |
| `engine/objects/objects.h` | `Engine\objects\objects.cpp` | `obj`, `item_obj`, `container`, party inventory |
| `engine/objects/items.h` | `Engine\objects\items.cpp` | `ItemType`, `item_info` (= `inv_item`), `item_*` |
| `engine/objects/item_paging.h` | `Engine\objects\item_paging.cpp` (pilot) | item mesh paging |
| `engine/objects/living_entity.h` | `Engine\objects\living_entity.cpp` | `Character`, `living_combat`, `CF_*`, status ids |
| `engine/characterinfo/characterinfo.h` | `Engine\characterinfo\characterinfo.cpp` | `char_info`, skills, chain attacks, experience |
| `engine/ai/ai.h` | ai.cpp (`Engine\ai\`) | `ai_state`, `ai_group`; the avoidance records and calls pathfinding.cpp uses (decomp32, D1205) |
| `engine/ai/pathfinding.h` | `Engine\ai\pathfinding.cpp` (decomp32) and the geometry unit 0x4a4ee0-0x4a6600 | .pfg graph types (`pf_graph`, `pf_vis_node`, `pf_edge`, `pf_cell`, `pf_bsp`, `pf_grid`), `pf_sweep`, `ai_path`, the unit's functions and globals, the geometry calls and tables |
| `engine/ai/pathfinding_visibility.h` | `engine\ai\pathfinding_visibility.cpp` | `determine_point_visibility` |
| `engine/gamesound/gamesound.h` | `Engine\gamesound\gamesound.cpp` (pilot) | `sound_ref`, game sound and music |
| `engine/rendering/levelrender.h` | `Engine\rendering\levelrender.cpp` (decomp28, partial) | `lobj`, `lobj_face`, `door_info`, level render calls, wave / light helpers |
| `engine/s3d/s3d.h` | `Engine\s3d\s3d.cpp` (decomp28) | `s3d_level level_s3d` and its record types, the `num_lobjs` / `lobjs` / ... field macros (D800) |
| `engine/camera/camera.h` | camera (0x478ff0) | `camera`, `game_camera` |
| `engine/effects/effect_stream.h` | effect streamer (0x4404a0) | `effect_stream_*` |
| `engine/effects/fx.h` | fx_* (0x48bd80-0x48d720, file not known; name ours) | `effect_object`, `fx_*`, `obj_from_handle_b` |
| `engine/spells/spells.h` | spells.cpp (0x479b60-0x47f857, pilot decomp23) | `spell_info`, `spells`, groups, missiles, spell effects, the cast request, `spell_*` / `summon_*` |
| `engine/combat/combat.h` | combat.cpp (0x463540-0x468c77, pilot decomp23; directory guessed) | `living_combat` (Character +0x558) and the combat helpers |
| `engine/ai/boss_ai.h` | boss_ai.cpp (0x4a04b0-0x4a3580) and the per-boss units | the boss hooks combat.cpp calls, `rand_int_range`, `ghost_stage` |
| `summoner/summoner.h` | summoner.cpp, gameplay.cpp, gametarget.cpp, gameseq | game flags, pause, level change, path ids, `SummonerOptions` |

| `engine/spells/spells.h` | spells.cpp (0x47a190..) | `spell_info`, `spells`, `spell_display_names` |
| `summoner/summoner.h` | summoner.cpp (decomp26), summoner_cd.cpp, gameplay.cpp, gametarget.cpp, gameseq | game flags, pause, level change, MainLoop / LoadSettings / GameInit, path ids, fonts, command-line flags, `SummonerOptions` |
| `summoner/gametarget.h` | gametarget.cpp (0x43a330-0x43cd6d, decomp35; included by summoner.h) | world picking and clicks, `WORLD_CLICK_*`, the tints, the unit's .bss / .data |
| `summoner/automap.h` | automap.cpp (0x42ffe0-0x4302b0) | `automap_label`, map labels, corners, reveal grid |
| `summoner/player/player.h` | `Summoner\player\player.cpp` (pilot) | `Player`, party lists |
| `summoner/multi/multi.h` | multi*.cpp (gathering header) | `mp_player`, packets (`mp_packet_table`, `MP_IS_HOST`, build macros), session, game list, lobby, character select records; includes the four unit headers below |
| `summoner/multi/multisave.h` | multisave.cpp (0x46cc60-0x46dd2f) | `.plr` files, `mp_char_summary`, autosave timer, `pxo_config` |
| `summoner/multi/multitrade.h` | multitrade.cpp (0x470e30-0x471def) | trade state, `mp_trade_item`, IT_* senders and handlers |
| `summoner/multi/multichat.h` | multichat.cpp (0x487b90-0x488223) | `chat_entry` ring, slash commands, CHAT_LN |
| `summoner/multi/multicontainer.h` | multicontainer.cpp (0x488c00-0x48972a) | loot drops, container open / close, lock picking |
| `summoner/multi/multispell.h` | multispell.cpp (0x4afca0-0x4b0957, decomp35) | RLS_SPL / CST_ANM casts, the one-summon claim (SMN_*) |
| `summoner/multi/multi_chattracker.h` | multi_chattracker.cpp (0x4abb80-0x4ae32d, decomp35) | the PXO lobby's IRC client: `irc_command`, `irc_user`, `irc_channel`, `irc_*` |
| `summoner/interface/interface.h` | interface core, new_hud / new_inventory helpers | screen switch, button records, popups, colours, `new_icam` |
| `summoner/interface/new_pxo_banner.h` | `new_pxo_banner.cpp` | `open_url` |
| `summoner/interface/new_int_multi_update.h` | `new_int_multi_update.cpp` | `version_check_run` |
| `summoner/interface/new_*.h` | `new_*.cpp` (pilots) | the screen units' prototypes |
| `summoner/interface/new_skill.h`, `new_spell.h`, `new_tutorial.h` | new_skill.cpp, new_spell.cpp, new_tutorial.cpp (not pilots) | the functions and data the HUD and the inventory call |
| `levelscripts/level_scripts.h` | level_scripts(_common).cpp | `level_script_info`, level tables, shops, quests |
| `levelscripts/conversation.h` | `levelscripts\conversation.cpp`, conversationctl.cpp | dialogue boxes; speakers, topics, lines; `conversation` (0x25461a0), `conversation_controller` (0x25464b0) |
| `levelscripts/savegame.h` | savegame (0x4cfca0) | `savegame`, `savegame_state` |
| `levelscripts/script_internal.h` | script_internal.cpp (0x4dc600; directory guessed, decomp34) | `navpoint_get_position` |
