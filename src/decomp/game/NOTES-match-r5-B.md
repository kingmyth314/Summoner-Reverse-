# match12: VC6 byte-match pass over 29 units (8 vsdk, 21 game)

Tree: `match12/src` (copy of `snapF/src/decomp`). Compiler: **cl 12.00.8966** (VC6 SP5, `vc6sp5\VC98`, switched to
mid-pass on the coordinator's instruction); every count below is under 8966. The "before" counts were re-taken with
8966 on the unchanged snapF sources (`base/src`): they are identical, address for address, to the 8168 counts of the
same files (291 / 421). Flags per unit from `vc6-flags.txt` (the "after" run uses this tree's copy, which adds two
lines, D7000). Decisions D7000-D7006 are appended to `src/include/DECISIONS.md`.

Out files: `out/b8966.txt` (before, per address), `out/after.txt` (after), `out/newmatch.txt` (the 46 new matches).
Runner: `run.mjs <src root> <out> files.txt` (8966, flags from the tree's vc6-flags.txt).

## Counts (functions with an address comment)

| unit | before | after | of |
|---|---:|---:|---:|
| vsdk/unnamed_gr_poly_0x5450c0.cpp | 6 | 12 | 24 |
| game/pathfinding_visibility.cpp | 1 | 3 | 11 |
| game/new_gamecreate.cpp | 11 | 15 | 20 |
| game/unnamed_titus_0x4c20e0.cpp | 19 | 20 | 27 |
| vsdk/unnamed_vfile_rw_0x525720.cpp | 44 | 49 | 51 |
| game/unnamed_snow_0x4946a0.cpp | 4 | 8 | 11 |
| vsdk/unnamed_vfx_mesh_0x562a30.cpp | 3 | 7 | 9 |
| vsdk/unnamed_os_0x530b90.cpp | 17 | 19 | 23 |
| game/unnamed_netsession_0x440b40.cpp | 15 | 16 | 21 |
| vsdk/unnamed_poly_split_0x55f110.cpp | 0 | 1 | 5 |
| game/unnamed_frame_0x4323e0.cpp | 16 | 18 | 21 |
| game/multicontainer.cpp | 8 | 9 | 13 |
| vsdk/unnamed_ftp_0x559ea0.cpp | 12 | 12 | 16 |
| game/unnamed_worldmap_automap_0x42f1e0.cpp | 31 | 31 | 35 |
| game/unnamed_gamelist_0x4705b0.cpp | 10 | 12 | 14 |
| game/multichat.cpp | 9 | 13 | 13 |
| game/unnamed_lobby_0x43ef30.cpp | 22 | 24 | 25 |
| game/new_dialogue.cpp | 3 | 4 | 6 |
| vsdk/unnamed_ui_manager_0x52f570.cpp | 8 | 8 | 10 |
| vsdk/unnamed_crc32_0x5520d0.cpp | 2 | 2 | 4 |
| game/unnamed_swi_0x488230.cpp | 12 | 12 | 14 |
| game/new_int_multi_update.cpp | 3 | 3 | 5 |
| game/level_script_rand_hills01.cpp | 5 | 6 | 7 |
| vsdk/unnamed_movement_0x531d20.cpp | 3 | 3 | 4 |
| vsdk/unnamed_build_stamp_0x52f450.cpp | 2 | 2 | 3 |
| game/unnamed_handleslots_0x4a3640.cpp | 2 | 3 | 3 |
| game/new_multi.cpp | 9 | 10 | 10 |
| game/level_script_lpalaceint02.cpp | 4 | 5 | 5 |
| game/level_script_ionaext.cpp | 10 | 10 | 11 |
| **total** | **291** | **337** | **421** |

No match lost (address-by-address join of before / after). No address added or dropped. The 46 new matches have
their markers raised to "exact (VC6 byte match, cl 8966; ...)" where they were likely / unsure.

Without the flags change (repo vc6-flags.txt): vfx_mesh stays at 3 and poly_split at 0, i.e. 332 / 421.

## Checks

- mingw `i686-w64-mingw32-g++ -fsyntax-only -std=c++98 -w -Iinclude`: the 29 files, 0 `error:` lines. No header was
  changed, so the other units are unaffected.
- VC6 (8966) before / after per file: the table above.

## Header changes

None. Candidates left for the owners (D7006): boss_ai.h (the Titus state as one object, as D6510 / D6511 / D6701:
three functions reload their counts after array stores), vsdk.h's list_remove (the binary forms `&e->prev` once),
gr_font.h (gr_string takes gr::Mode by value: the /Ob0 new_dialogue passes gr_string_mode without the conversion
operator), summoner.h's game_flags (game_is_paused reads it as a byte bit-field).

## Flags change (D7000)

`vc6-flags.txt`: `vsdk/unnamed_vfx_mesh_0x562a30.cpp /O2 /Ob0 /GX /MT` (3 -> 7) and
`vsdk/unnamed_poly_split_0x55f110.cpp /O2 /Ob0 /GX /MT` (0 -> 1; the other four functions only reach the binary's
instruction counts under /Ob0). gr_poly is also an /Ob0 unit by its calls, but stays at the default for now (11 under
/Ob0 against 12, the vector3 copies of its buffer helpers become operator= calls).

## What worked (D7001 has the full list)

- 8966 lays the then-block out as the fall-through: inverting conditions to the binary's fall-through gave lobby
  add_channel / add_user, chat_cmd_ping, chat_process_command, multi_set_netgame_state, titus_knock_down_party and
  File::read_string_buf.
- Separate epilogues: `key = 0; break;` with one `return key;` (PollInputEvents), a `return out;` per branch
  (chat_format_entry), `if (error()) return def; return v;` (read_int16 / read_uint16), `v ? true : false`
  (read_bool), the if-form bool (float_strictly_between), `while (1)` (read_string).
- Switches in the binary's compare order: elevator_popup_frame, cursor_draw (separate case 0 / default bodies).
- Store order: the global store first (gamecreate_set_difficulty, mp_send_jn_rqst), integer stores written after an
  inline strcpy that 8966 interleaves (handle_slots_reset), WNDCLASSEX fields in the binary's order
  (os_create_window), `uv = rgb = alpha = vertices` (the rightmost is stored first), gr_poly_end.
- Local copies of the buffer cursor (gr_poly_buffer_put_byte / put_int / get_vector), element pointers (gamecreate,
  dialogue, gr_poly), index expressions where the binary re-reads (pathvis inserts, snow_update / fill).
- vector3 fields assigned one by one where a float global is copied with mov (snow_cell::update / fill), the sum of
  squares in reverse order (titus), `int music = music_find_by_name(..)` before music_play (two level scripts;
  about 20 more level scripts outside this pass have the same inline call), header macros for mp_send_gm_qury,
  `ushort len` for chat_build_packet, a block-scope copy for `&c` (mp_handle_container_open, D7004).
- /Od frames: the D6101 hash-bucket rule holds for 8966 (probe of 21 locals); os_clipboard_get_text matches with
  `text` declared before `h`; cpu_identify's slots all match after renames (D7003).

## What still differs (D7006)

- Dead argument slots reused by 8966 for locals: gr_poly set_texture / rgb / clip_and_draw / clip_edge,
  gr_poly_get_section, mp_handle_actv_gm, mp_handle_swi_path.
- Scheduling of the cursor store in gr_poly put_float / put_vector / put_data / get_data and of
  level_entry_point in ls_rand_hills01_goto_khosanistrng.
- strcat of a literal expanded with its length in the binary (cpu_identify, PrintSystemInfo): not reproduced by
  8966 under /O2 or /Od in a probe.
- snow_cell::fill_count's fdiv by 8/3 (8966 multiplies by the reciprocal), snow_update's atan argument kept on the
  stack, snow_start's row pointer increment placement.
- Header-bound: titus boss_load / collect_area_hits / update_offense_points (state as one object),
  pathvis_add_edge_node and mp_player_alloc (list_remove), game_is_paused (bit-field), new_dialogue draw / frame
  (gr::Mode by value).
- Register choice: Crc32 update / init_table (store after pop), gr_poly_uv / alpha, gamecreate_frame,
  mp_player_disconnect, mp_session_reset, mp_handle_deny_jn, version_check_run (the binary keeps 0 in ESI and
  shares one "result 3" tail), ls_ionaext_frame (a `sete` normalisation no form gave).
- Not looked at in depth (time): ftp (4), worldmap / automap (4), ui_manager (2), movement (1), build_stamp (1),
  version_read_nfo_file, multicontainer's other three, swi_sneak, the large pathvis / titus / vfx_mesh functions.

## Logic errors, game bugs

None new. No behaviour was changed by this pass (all edits are source shapes with the same semantics; the Crc32
index became signed, which is the same for 0..255).
