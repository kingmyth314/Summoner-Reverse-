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
| `vsdk/os/timer.h` | timer.cpp | `timer_get`, `timer_wait` |
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
| `vsdk/inetfile/inetgetfile.h` | InetGetFile (0x51b760-0x51bb10) | `InetGetFile` (directory and file name guessed) |
| `vsdk/vfile/file.h` | vsdk\vfile | `File`, `PATH_ANY`, `file_*`, `file_find_*` |
| `vsdk/vfile/packfile/file_packfile.h` | `vsdk\vfile\packfile\file_packfile.cpp` | `packfile_read_whole_file`, `packfile_find_in_packfile` |
| `vsdk/parse/parse.h` | `vsdk\parse\parse.cpp` (decomp22) | `TextFile`, to-text helpers, `file_blast_*` |
| `vsdk/bmpman/bmpman.h` | `vsdk\bmpman\bmpman.cpp` | `bitmap_entry`, `bm_*`, `progress_update` |
| `vsdk/bmpman/targautils.h` | `vsdk\bmpman\targautils.cpp` (pilot) | TGA / PCX / m2v, `m2v_bitstream` |
| `vsdk/gr/gr.h` | `vsdk\gr\gr.cpp` + vsdk\gr | `Color`, `gr_mode`, `gr__Screen`, `gr_*`, `gr::` clip, `gr_light_desc`; includes gr_font.h |
| `vsdk/gr/gr_font.h` | `vsdk\gr\gr_font.cpp` (pilot) | `Font`, `FontCharInfo`, `gr_string`, `gr_split_str`, ... |
| `vsdk/localize/localize.h` | localize | `localize_strings`, `XSTR`, `localize_*` |
| `vsdk/snd/snd.h` | snd (0x5207d0) | `snd_*`, `eax_*` |
| `vsdk/net/net.h` | vsdk\net | `nw_*` globals |
| `vsdk/ca/skeleton.h` | `vsdk\ca\skeleton.cpp` | `anim_track`, `skeleton`, `vim` (= `ca_vim`), `vim_load` |
| `vsdk/ca/character.h` | `vsdk\ca\character.cpp` | `ca_character`, `ca_mesh`, `ca_skin`, `ca_bone`, `ca_render_*` |
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
| `engine/ai/ai.h` | ai.cpp (`Engine\ai\`) | `ai_state`, `ai_group` |
| `engine/ai/pathfinding.h` | `Engine\ai\pathfinding.cpp` | `pf_find_nearest_navigable_point`, `ai_path_find` / `ai_path_free`, `pf_trace_edge_crossings` |
| `engine/gamesound/gamesound.h` | `Engine\gamesound\gamesound.cpp` (pilot) | `sound_ref`, game sound and music |
| `engine/rendering/levelrender.h` | `Engine\rendering\levelrender.cpp` | `lobj`, level render calls |
| `engine/camera/camera.h` | camera (0x478ff0) | `camera`, `game_camera` |
| `engine/effects/effect_stream.h` | effect streamer (0x4404a0) | `effect_stream_*` |
| `engine/spells/spells.h` | spells.cpp (0x47a190..) | `spell_info`, `spells`, `spell_display_names` |
| `summoner/summoner.h` | summoner.cpp, gameplay.cpp, gametarget.cpp, gameseq | game flags, pause, level change, path ids, `SummonerOptions` |
| `summoner/automap.h` | automap.cpp (0x42ffe0-0x4302b0) | `automap_label`, map labels, corners, reveal grid |
| `summoner/player/player.h` | `Summoner\player\player.cpp` (pilot) | `Player`, party lists |
| `summoner/multi/multi.h` | multi*.cpp (gathering header) | `mp_player`, packets (`mp_packet_table`, `MP_IS_HOST`, build macros), session, game list, lobby, character select records; includes the four unit headers below |
| `summoner/multi/multisave.h` | multisave.cpp (0x46cc60-0x46dd2f) | `.plr` files, `mp_char_summary`, autosave timer, `pxo_config` |
| `summoner/multi/multitrade.h` | multitrade.cpp (0x470e30-0x471def) | trade state, `mp_trade_item`, IT_* senders and handlers |
| `summoner/multi/multichat.h` | multichat.cpp (0x487b90-0x488223) | `chat_entry` ring, slash commands, CHAT_LN |
| `summoner/multi/multicontainer.h` | multicontainer.cpp (0x488c00-0x48972a) | loot drops, container open / close, lock picking |
| `summoner/interface/interface.h` | interface core, new_hud / new_inventory helpers | screen switch, button records, popups, colours, `new_icam` |
| `summoner/interface/new_pxo_banner.h` | `new_pxo_banner.cpp` | `open_url` |
| `summoner/interface/new_int_multi_update.h` | `new_int_multi_update.cpp` | `version_check_run` |
| `summoner/interface/new_*.h` | `new_*.cpp` (pilots) | the screen units' prototypes |
| `summoner/interface/new_skill.h`, `new_spell.h`, `new_tutorial.h` | new_skill.cpp, new_spell.cpp, new_tutorial.cpp (not pilots) | the functions and data the HUD and the inventory call |
| `levelscripts/level_scripts.h` | level_scripts(_common).cpp | `level_script_info`, level tables, shops, quests |
| `levelscripts/conversation.h` | `levelscripts\conversation.cpp`, conversationctl.cpp | dialogue boxes; speakers, topics, lines; `conversation` (0x25461a0), `conversation_controller` (0x25464b0) |
| `levelscripts/savegame.h` | savegame (0x4cfca0) | `savegame`, `savegame_state` |
