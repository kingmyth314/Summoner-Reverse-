# match25 notes (cl 12.00.8966, SP5; per-unit flags from src/vc6-flags.txt)

Tree: copy of scratchpad/snapP/src/decomp. Files: the 73 listed in the task (files.txt). Before: 773 / 894
functions match; after: 805 / 896 (the two new ones are the D8403 getters). No address lost (checked address by
address, before.json / after2.json). Decisions D8400-D8406 appended to src/include/DECISIONS.md.

## Header changes (each: all callers updated, the other users compared old / new header, mingw 0 errors in 259 files)

1. include/vsdk/snd/snd.h: `snd_play_looping(int handle, float volume, float pan, int a, int unused)` (D8400).
   Callers push five arguments (0x521590 snd_play_3d, 0x546407 sound_emitter_play); definition and the
   snd_play_3d call in vsdk/unnamed_snd_0x5204c0.cpp updated. unnamed_snd stays 48/58 (snd_play_3d 2 bytes
   closer); the 18 other units including snd.h: 901 functions, status and first offset identical.
2. include/vsdk/ui/ui_container.h, include/vsdk/ui/window.h: get_mask_buffer / get_mask_data no longer inline,
   defined at the end of vsdk/unnamed_ui_manager_0x52f570.cpp (D8403). ui_draw calls 0x530b70 / 0x530b80 in the
   binary but inlines gr::Mode's conversion, so the getters were out of line. The other 9 units that include the
   two headers: 292 functions identical.

## vc6-flags.txt (D8402)

/Ob0 added for unnamed_ui_element_0x542cd0, console, unnamed_inetgetfile_0x51b760, unnamed_vfx_dummy_0x55fc70
(+1 each, no loss). Every other vsdk unit of the list: toggling /Ob0 gains nothing or loses (ob0.txt).

## Per file (before -> after)

| file | before | after |
|---|---|---|
| vsdk/unnamed_ui_slider_0x55e040.cpp | 21/26 | 23/26 |
| vsdk/unnamed_sound_emitter_0x546150.cpp | 21/23 | 22/23 |
| vsdk/unnamed_ui_element_0x542cd0.cpp | 15/16 | 16/16 |
| vsdk/unnamed_d3d_tcache_0x54bb30.cpp | 10/14 | 10/14 |
| vsdk/unnamed_inputbox_0x5521a0.cpp | 8/13 | 9/13 |
| vsdk/unnamed_key_0x5093a0.cpp | 22/26 | 22/26 |
| vsdk/unnamed_ui_button_0x526940.cpp | 20/24 | 22/24 |
| vsdk/unnamed_floatmath_0x542500.cpp | 16/20 | 16/20 |
| vsdk/unnamed_ftp_0x559ea0.cpp | 12/16 | 15/16 |
| vsdk/cmdline.cpp | 15/16 | 15/16 |
| vsdk/unnamed_checkbox_0x553ba0.cpp | 8/12 | 9/12 |
| vsdk/unnamed_ca_mesh_0x50af80.cpp | 8/12 | 8/12 |
| vsdk/unnamed_vfx_channel_0x55b440.cpp | 16/19 | 16/19 |
| vsdk/unnamed_bezier_0x558a30.cpp | 14/15 | 14/15 |
| vsdk/unnamed_linefile_0x558340.cpp | 11/14 | 11/14 |
| vsdk/unnamed_attached_lights_0x51fb10.cpp | 11/11 | 11/11 |
| vsdk/unnamed_poly_clip_0x5519c0.cpp | 6/8 | 6/8 |
| vsdk/unnamed_exceptionhandler_0x5419a0.cpp | 6/8 | 6/8 |
| vsdk/timer.cpp | 3/5 | 5/5 |
| game/unnamed_editor_dialogs_0x430bc0.cpp | 3/5 | 3/5 |
| game/new_int_multi_update.cpp | 3/5 | 3/5 |
| vsdk/unnamed_crc32_0x5520d0.cpp | 3/4 | 3/4 |
| vsdk/unnamed_lfsr_0x559db0.cpp | 2/3 | 3/3 |
| game/player.cpp | 21/23 | 21/23 |
| game/new_pickup.cpp | 18/20 | 18/20 |
| vsdk/memory.cpp | 17/19 | 18/19 |
| game/unnamed_giant_salamanka_0x4c8030.cpp | 17/19 | 17/19 |
| game/level_script_sewers.cpp | 17/19 | 18/19 |
| game/new_assess.cpp | 16/18 | 16/18 |
| vsdk/console.cpp | 15/17 | 16/17 |
| game/gamesound.cpp | 14/16 | 14/16 |
| game/summoner.cpp | 13/15 | 13/15 |
| vsdk/registry.cpp | 13/14 | 13/14 |
| game/unnamed_swi_0x488230.cpp | 12/14 | 12/14 |
| vsdk/frametime.cpp | 10/12 | 11/12 |
| game/unnamed_projectors_0x4c0250.cpp | 9/11 | 9/11 |
| game/level_script_courtyard.cpp | 9/11 | 11/11 |
| vsdk/unnamed_ui_static_0x527e20.cpp | 8/10 | 9/10 |
| vsdk/unnamed_ui_manager_0x52f570.cpp | 8/10 | 11/12 |
| vsdk/unnamed_inetgetfile_0x51b760.cpp | 8/10 | 9/10 |
| vsdk/ui_container.cpp | 8/10 | 9/10 |
| game/level_script_rand_hills01.cpp | 7/7 | 7/7 |
| game/level_script_ikaemosu.cpp | 6/7 | 6/7 |
| game/unnamed_cutscene_script_0x4ed050.cpp | 5/6 | 5/6 |
| game/level_script_lenele1aa.cpp | 5/6 | 6/6 |
| game/level_script_lpalaceint02.cpp | 5/5 | 5/5 |
| game/objects.cpp | 39/40 | 39/40 |
| vsdk/unnamed_movement_0x531d20.cpp | 3/4 | 3/4 |
| game/unnamed_utrack_0x48d7c0.cpp | 3/4 | 3/4 |
| game/unnamed_equip_packets_0x4878a0.cpp | 3/4 | 3/4 |
| game/unnamed_alt_textures_0x4aac10.cpp | 3/4 | 3/4 |
| vsdk/unnamed_vfx_dummy_0x55fc70.cpp | 2/3 | 3/3 |
| vsdk/unnamed_build_stamp_0x52f450.cpp | 2/3 | 2/3 |
| game/unnamed_handleslots_0x4a3640.cpp | 3/3 | 3/3 |
| game/unnamed_cutscene_0x434900.cpp | 25/26 | 25/26 |
| game/unnamed_item_info_popup_0x418580.cpp | 22/23 | 22/23 |
| game/unnamed_dialogue_table_0x4feb50.cpp | 20/21 | 20/21 |
| vsdk/unnamed_movie_0x5172f0.cpp | 1/2 | 1/2 |
| vsdk/debug.cpp | 2/2 | 2/2 |
| vsdk/stringpool.cpp | 15/16 | 15/16 |
| game/level_script_catacombs.cpp | 14/15 | 14/15 |
| vsdk/unnamed_eax_0x521e40.cpp | 13/14 | 13/14 |
| vsdk/norlsallocpool.cpp | 13/14 | 14/14 |
| game/unnamed_mpmenus_0x43eb00.cpp | 13/14 | 13/14 |
| vsdk/unnamed_anim_track_0x550660.cpp | 12/13 | 12/13 |
| game/level_script_jadetemple.cpp | 10/11 | 10/11 |
| game/level_script_ionaext.cpp | 10/11 | 10/11 |
| vsdk/unnamed_bitarray_0x55bd80.cpp | 9/10 | 10/10 |
| game/new_multi.cpp | 10/10 | 10/10 |
| game/level_script_temple_inside.cpp | 10/10 | 10/10 |
| vsdk/unnamed_time_format_0x532660.cpp | 0/1 | 1/1 |
| vsdk/unnamed_jpeg_write_0x5565e0.cpp | 0/1 | 1/1 |
| vsdk/unnamed_ca_render_0x520420.cpp | 1/1 | 1/1 |

## What worked (each a byte match; details in D8404)

- vsdk/unnamed_ui_slider_0x55e040.cpp 0x55e4c0 ui_slider_base::process: rect test first (page-down shared last), drag_state read directly (no state local), len = w/h - marker_size, if/else rounding of num
- slider 0x55ea50 set_tooltip: owner before strcpy, then delay, x, last, next, tooltips.prev=t, colors, y, font, prev, last->next
- button 0x5271c0, inputbox 0x5527b0, checkbox 0x553e00, static 0x527fa0 set_tooltip: same as slider (owner, strcpy, delay, x, last, next, prev=t, colors, y, font, prev, last->next); static's unlink with prev/next locals
- container 0x5535f0 set_tooltip: delay, x, y, font (8966 emits these field stores in reverse pairs)
- HEADER D8400 snd.h snd_play_looping 5th unused param (binary pushes 5 at 0x546407 and 0x521590); definition + call in unnamed_snd_0x5204c0.cpp updated; snd unit 48/58 unchanged, snd_play_3d closer. sound_emitter_play 0x546320 matched: float vol local per branch
- timer.cpp: timer_get under #pragma optimize off (D8401) with locals ticks/elapsed/reading/result (/Od hash frame); timer_init flag after ticks
- frametime FrameDone 0x501510: delta = now; if (now<0) delta = ...; set_limits marker exact->likely
- ftp read_data_channel 0x55a780: do { } while (true) with the inner returns; read_control_channel 0x55a850: fd_array[0] before fd_count
- ftp read_server_reply 0x55a6a0: memcpy(code, recv_buffer, 3), char code[8] (frame 0x10; 5..8 all match)
- lfsr crypt 0x559e10: index loop over buf (the pointer is loaded after the zero-length test)
- crc32 update 0x552130: index form, closer (9 lines), exact->likely
- cmdline_take_argument: nested next local, int len, strcpy before is_done: 6 lines (scheduling)
- bitarray all_equal 0x55bf20: int want = value != 0 after start/end
- norlsallocpool Grow 0x532570: char *p, memcpy then last_allocation = p (8966 schedules the store into the inline memcpy)
- jpeg_write_file 0x5565e0: rows[i] = &rgb[i * w * 3]
- time_format_hms 0x532660: int total = (int)t; secs = total % 60; minutes = total / 60; char part[12] (frame)
- memory allocate_in_mempool 0x5127c0: size rounded in place, test offset + size, then new_offset
- courtyard ls_lenele1d_init 0x4fa890: else-if chain with v31 in each branch (8966 tail-merges it and the stack pops)
- courtyard ls_lenele1c_init 0x4faa60: same else-if chain
- lenele1aa init 0x4efa30: _v3 tail written in both branches (8966 tail-merges into the first)
- sewers ls_sewer_init 0x4ed7e0: else-if chain, each branch writes v11..v52 in full (tail-merged, one add esp 0x38)
- ui_button is_latched 0x5269e0: (flags & UI_BUTTON_LATCHED) ? true : false
- build_stamp_check 0x52f450: carry as if, stamped body nested, expiry read into a local; only the return-3 register differs
- HEADER D8403 ui_container.h get_mask_buffer / window.h get_mask_data out of line, defined at the end of unnamed_ui_manager (ui_draw calls 0x530b70 / 0x530b80): ui_draw + both getters match; 9 other users unchanged (292 functions), mingw 0 errors
- vsdk/unnamed_ui_element / console / inetgetfile / vfx_dummy: /Ob0 (D8402).

## Still different (forms tried)

- floatmath fast_inv_sqrt / fast_sqrt_refined / fast_sqrt: EAX / ECX swapped; tried locals, OR operand order, int /
  long / pointer casts, two statements, /O2 /Ox /O1: no change. float_format_buf not tried.
- key_flush: 8966 converts the zeroing loops (index, pointer, while, do) into rep stosd; volatile arrays lose
  five other functions. key_clear: the lock push placement. key_mark / key_remap_layout: register choice (switch
  and if forms tried).
- cmdline_take_argument: now 6 differing lines (was 58): only the is_done store, which the binary schedules inside
  the inline strcpy; written before the strcpy 8966 CSEs the element address instead.
- Crc32::update: 9 lines (was 19): the table word is XORed into the shifted value in the binary; operand order,
  temporaries and `>>=` / `^=` forms all canonicalise the same way.
- StringPool::Add, ui_slider get_marker_rect / create_scrollbar / ui_draw_bevel, pickup_draw_item_list,
  ftp_get::ftp_get (no zero register in the binary), Player::create, mp_handle_swi_sneak:
  register choice; declaration order / scope did not move them.
- sound_emitter_delete: the binary pushes ESI / EDI before the enabled test (D7506 shrink-wrap case).
- eax_init: the binary places the success block last; if / else, early return, goto and inverted forms all give
  8966's order.
- init_char_types: `je ret / jmp body` prologue; if-not / if-else wrappers give the same code as the early return.
- mem_report_line: `65 - strlen(text) - len`: every grouping and two-statement form gives len subtracted first.
- frametime_set_limits (D6905): b copied as an integer in the binary; six forms tried.
- movie_play: the File object is at [esp] (below the path buffer) in the binary; declaration order, inner scopes
  and buffer size do not move it.
- Player::is_leader: `shr al, 3 / and al, 1` (byte shift) not reproduced by casts, bool locals or ternaries.
- ls_ionaext_frame: the negated played_intro() is materialised with sete; `== false`, bool locals, inline helpers
  are folded.
- ls_ikaemostopint_init: the constant 1 sits in EBX (push ebx / mov [..], bl); a bool local is spilled, int / char
  locals are propagated.
- mp_handle_swi_path: the walk flag goes through the data argument slot in the binary.
- ui_process: the restore compares' registers (AL / DL alternation) and the `mov ecx, esi` placement; store orders
  of the restore block and the false initialisers tried.
- ui_static::draw: 62 bytes short (the binary keeps more code per branch); colors per branch and the text tail per
  branch tried (closer: 192 lines from 239, not kept).
- build_stamp_check: only the "not stamped" return differs now (the binary loads 3 into EDX before the test and
  returns it from a tail block).
- ls_sewer_create_avoid_spline: 8966 calls the vector constructor iterator for avoid_spline_def's vector2 arrays;
  the binary has none (D8406, header question, not changed).
- version_check_run, version_read_nfo_file, editor dialogs, projectors, giant_salamanka, gamesound, summoner,
  cutscene files, objects, dialogue_table, ca_mesh, d3d_tcache, inputbox, checkbox draw / create, vfx_channel,
  bezier, linefile, poly_clip, exceptionhandler, movement, registry, anim_track, mpmenus, utrack, equip_packets,
  alt_textures, catacombs, jadetemple, new_assess format_info: not reached in this pass.

## Markers lowered from exact to likely (they differ under 8966)

0x55e420 (ui_slider get_marker_rect), 0x54d000 (gr_d3d_get_section_rect), 0x509ab0 key_flush, 0x509b50 key_clear,
0x420d20 pickup_page_up, 0x401a20 assess_page_up, 0x418ba0 item_info_popup_page_up (all three D7506's max(x, 0)),
0x4f9ec0 ls_ikaemostopint_init, 0x544c40 StringPool::Add, 0x4f7270 ls_ionaext_frame; also 0x5016f0
frametime_set_limits and 0x552130 Crc32::update.

## Logic errors / game bugs

None found in this pass. (Behaviour unchanged by every edit: reorderings of independent stores, equivalent loop
and branch forms, the unused fifth argument.)

## Tools (scratchpad/match25)

base.mjs (all files, per-address status JSON), t2.mjs (variants with differing-line counts and LOST checks; replace
with a function so `$` in code is literal), sd.mjs (status + side-by-side, FRAME=1 prints /FAsc frame offsets),
ob0.mjs (/Ob0 toggle per vsdk unit), bucket.js (D6101 /Od name bucket). Heredocs in Git Bash drop one backslash of
`\`: generator scripts with escapes were written with the Write tool.
