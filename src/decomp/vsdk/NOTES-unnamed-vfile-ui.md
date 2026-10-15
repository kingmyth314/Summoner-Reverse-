# decomp61: the vfile units, ui_button and ui_static

Written from Sum.exe v1.40 only (`tools/asm.mjs fn / callers / strings`, `build/sum.asm`, raw bytes of
`Summoner/Sum.exe` for literals, vtables and the jump table). Summoner 2 was used for names only (its PS2
`ngps_file.cpp` has the same reader set in one file, but on PC the readers are a separate object, so its
name is not evidence for either unit). Every file passes
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include <file>`
(warnings only: read_matrix43's intentional `m[i][3]`), and all 143 `.cpp` files of `src/decomp` compile
against this header copy with no new error (the copy includes the 6 headers changed by commit 1f46d85).

179 functions: 135 exact / 42 likely / 2 unsure (13 of them compiler-generated).

## Units and evidence

| File | Range | Functions | Evidence |
|---|---|---|---|
| `vsdk/unnamed_vfile_0x5128b0.cpp` | 0x5128b0-0x514eb8 | 90 | Starts after memory.cpp's last function (0x512880) with its own initialisers (table 0x5888f8 the path StringPool, 0x5888fc the file slots); ends before plainstring.cpp's is_blank (0x514ec0) with the Crc32 COMDAT. .bss 0x2693068-0x26b638f in one block (path table, slots, install_path, the pool and its 8 KB buffer, cd_root_path, file_inited, access level, root flag, two "" literals); .data 0x5a77a4-0x5a783b (literals, file_last_path_id, the "none" buffer; 0x5a783c is plainstring's wide_charmap_table). The EH funclets of its four EH functions form one block 0x57d0b0-0x57d12b. |
| `vsdk/unnamed_vfile_rw_0x525720.cpp` | 0x525720-0x52693c | 55 | Own initialiser run 0x525720-0x525790 (table 0x588964-0x588970) after the vector math unit's; .bss 0x2cbf4c8-0x2cbf553 (the four defaults and six "" literals); .data 0x5a8b64-0x5a8b6b (write_line's two "\n"); window.cpp's literals start at 0x5a8b6c. No EH. |
| `vsdk/unnamed_ui_button_0x526940.cpp` | 0x526940-0x527e11 | 24 | The ui_button__* run; no data. Its constructor's EH funclets 0x57d370-0x57d3c7 are followed by int3 padding to 0x57d3d0, where ui_static's start: padding never appears inside one object's funclet block, so the two are separate objects. |
| `vsdk/unnamed_ui_static_0x527e20.cpp` | 0x527e20-0x528429 | 10 | The ui_static__* run up to window.cpp (tagged from 0x528430); no data. Its funclets end at 0x57d3ef and window.cpp's begin at 0x57d3f0, a 16-byte boundary (no padding needed either way): kept as its own file because window.cpp's first tagged function is 0x528430. |

`docs/sum-exe/source-map.md` "Units without a file name" rows 0x525780-0x525ba0 and 0x525c90-0x5280e0 ("two
files") are three objects: the readers / writers (0x525720-0x52693c, including the init run and
read_matrix43 0x525ba0, which neither row covers), ui_button and ui_static. Its "Engine ranges" rows (0x525720,
0x526940, 0x527e20) are right. No file name has hard evidence, so `modules.txt` is empty.

## Open questions

1. ui_button::draw and ui_static::draw return at once when `is_disabled()` (slot 4) and use the disabled
   bitmap and colour set when `is_hidden()` (0x52c4b0). Read literally a disabled gadget is invisible and a
   hidden one looks disabled; more likely the names of ui_element +0x1c / +0x1d (hidden / disabled) or of the
   two tests are swapped. Not changed (window.cpp and the gadget base use the current names).
2. File::read's refill (unsure): after the fread refill the slot's buf_len is the requested count, not the
   count read; the source shape that gives `buf_len = fread(...); ...; buf_len = want` is a guess.
3. packfile_fread gets a fifth argument 0 from File::read (D4106); the prototype needs it before the call can
   match.
4. 0x2cbf4e8 (init_vector3_2cbf4e8): any type whose default constructor folds into 0x505530 fits; matrix43
   by the 0x30-byte gap. Nothing uses it.
5. file_list_files (unsure): the 240-byte directory buffer and the local order.
6. File::open with an explicit path id and a new file (write): find(name, path_id) fails, then the PATH_ANY
   find decides full_path (the first path whose extension list has the extension, else the root). Whether a
   caller writes a new file to a path that does not list its extension was not checked.
7. The ui_button flag names are ours (UI_BUTTON_*); 0x200 is only seen in the 0x3c0 arrow test, and the 0x40 /
   0x80 / 0x100 meanings are what draw_arrow draws (which way each arrow points was not checked in game).

## Doc and symbol disagreements

- sum.symbols `File__read_String8` (0x525ce0): not "int8 length then that many chars"; it reads chars up to
  and with the NUL (the first byte is the first char) into a 256-byte buffer.
- sum.symbols `File__read_line_String` / `File__read_line`: every CR / LF run after the line is skipped (blank
  lines too), not only "the LF after CR"; read_line never uses its size argument.
- sum.symbols `ui_static__create`, `__set_justification`, `__set_text_offset`: +0x374 / +0x378 are the state
  bitmap's clip size (create stores w / h), +0x37c / +0x380 its offset; the justification is +0x3bc / +0x3c0,
  the font +0x384 (D4107). The two setters keep their sum.symbols names.
- sum.symbols `file_slots` (0x2694068): +0 is the open mode (0 = free, check_flags), +4 the cipher flag, then
  version +8, error +0xc, position +0x10, op count +0x14, byte count +0x18, size +0x1c, buffer start / position
  / length +0x20 / +0x24 / +0x28, buffer +0x2c, FILE* +0x802c, owner File* +0x8030, cipher +0x8034
  (`file_slot`, vsdk/vfile/file.h).
- sum.symbols `File__set_cipher_key`: bit 0 (value 1) of slot +4, called "bit 1".
- sum.symbols `File__check_stream_error` says CRT 0x56f813 is ferror; the symbol file's own label (shown by
  asm.mjs) is fflush.
- sum.symbols `lfsr_cipher__lfsr_cipher` (0x513eb0) is lfsr_cipher's inline constructor; `file_slot__file_slot`
  (0x513e90) the implicit slot constructor.
- sum.symbols `File__is_absolute_path`, `File__find_free_slot`: thiscall (File members, D4105).
- sum.symbols `ui_drag_gadget` / `ui_drop_target` type ui_element*: the dragged gadget is a ui_button (its
  +0x40c..+0x41c are read).
- sum.symbols `init_vector3_2cbf4e8`: probably a matrix43, never used.
- sum.symbols `ui_button__create_in_parent`: also ORs UI_BUTTON_REPEATS (2) into the flags.
- asm.mjs: `fn 0x526f30` starts the listing at 0x526f31 (llvm-objdump runs through ui_button::process's jump
  table at 0x526f04 and resyncs one byte late); the function starts at 0x526f30 (`push esi`), as the vtable says.
- vsdk/vfile/file.h before this batch: the readers' range 0x525850-0x526900 and File 0x5130a0-0x514ae0;
  FileReader::seek / read returned void (they return 0 / -1 and the byte count).

## Game bugs (all from the code; none checked in game; ids proposed)

| Id | Where | What | Live? |
|---|---|---|---|
| F610 | File::read_string 0x525ee0, FileReader::read_string 0x513e00 | up to max chars are stored and the NUL then goes to buf[max]: one byte past a max-sized buffer (the .plr readers pass the buffer size 0x20 / 0x100) | a 32-char name in a .plr |
| F611 | File::read_line 0x525f60 | the size argument is ignored: a line longer than the buffer overflows it (effects_parse_arr_line passes 0x400) | malformed .arr |
| F612 | FileReader::read* 0x513b40-0x513e00 | memory mode: a read that would end exactly at the end of the block fails (pos + n must be < size), so the last bytes of a preloaded .mvf / .vim read as 0 | only if a file's last field ends at its end |
| F613 | FileReader::seek 0x513a90 | memory mode: seek(0, 0) fails, origin 2 counts back from the current position, not the end | skeleton_read / vim_read use origins 0 and 1 |
| F614 | File::find 0x5141c0 | with an explicit path id that is not found, the write-path loop reads the uninitialised ext buffer | every failed explicit-path find |
| F615 | file_make_path 0x512b30 | PATH_ANY passes file_path_id_valid, then file_path_table[9999999] is read | no caller passes PATH_ANY |
| F616 | File::open 0x514630 | strcpy of the extension into the 8-byte file_last_ext (overflow for an extension of 8+ chars) | long extensions only |
| F617 | file_add_path 0x512e60 | a directory added again never gets its new extensions (the stristr result is dropped) | setup_data_paths |
| F618 | File::read_matrix43 0x525ba0 | sets m[i][3] of a 4x3 matrix: zeroes three elements it just read and writes 1.0f 4 bytes past it | no caller chain (sync_matrix43 is unreferenced) |
| F619 | File::find_on_cd 0x513f70 | passes &name as the packfile entry out pointer | no caller |
| F620 | file_list_files 0x513590 | a find with no match splices its stack head into the caller's list | no caller |
| F621 | File::read_String8 0x525ce0 | unbounded copy into a 256-byte stack buffer | no caller |
| F622 | ui_button::draw_arrow 0x527440 | divides by the inner height / width, 0 for a 5-pixel arrow button: integer divide trap | arrow buttons are larger |
| F623 | ui_button::set_tooltip 0x5271c0, ui_static::set_tooltip 0x527fa0 | the replaced tooltip is unlinked but not deleted (0x120 bytes leaked) | each re-set tooltip |
| F624 | ui_button::process 0x526a50 | a state above 9 loops for ever (the switch's default goes round again) | states never exceed 9 |
| F625 | ui_button::create_in_parent 0x527100 | always sets UI_BUTTON_REPEATS: every button inside a container auto-repeats while held | buttons in UI table containers |

## What is left

Nothing in 0x5128b0-0x514eb8 or 0x525720-0x528429. Neighbours not written: lfsr_cipher (0x559db0-0x559e10,
vsdk\vfile), the file array helpers 0x544ee0-0x545080, the gadget base (0x52c040-0x52c9a0, ui_state_bitmap),
the listbox (0x52ca60-), the UI manager (0x52f570-0x530b80, where ui_drag_gadget / ui_static_colors /
ui_play_sound live), String (0x5566e0-).

## Functions

unnamed_vfile_0x5128b0.cpp: 90 functions, 65 / 24 / 1

| Address | sum.symbols | Confidence |
|---|---|---|
| 0x512900 | init_file_slots | exact |
| 0x513e90 | file_slot__file_slot | exact |
| 0x513eb0 | lfsr_cipher__lfsr_cipher | exact |
| 0x5128b0 | init_stringpool_26b427c_thunk | exact (compiler-generated) |
| 0x5128c0 | init_stringpool_26b427c | exact (compiler-generated) |
| 0x5128e0 | stringpool_26b4268_register_dtor | exact (compiler-generated) |
| 0x5128f0 | stringpool_26b4268_dtor | exact (compiler-generated) |
| 0x512930 | file_get_path_string | likely |
| 0x5129e0 | file_get_dir_string | likely |
| 0x512a70 | file_get_full_dir | exact |
| 0x512af0 | file_get_dir | exact |
| 0x512b30 | file_make_path | exact |
| 0x512c00 | file_make_cd_path | exact |
| 0x512cd0 | file_path_id_valid | likely |
| 0x512d00 | file_shutdown | exact |
| 0x512d60 | file_init | likely |
| 0x512e60 | file_add_path | likely |
| 0x513080 | file_remove_path | exact |
| 0x5130a0 | file_change_ext | exact |
| 0x513100 | file_strip_ext | exact |
| 0x513160 | file_get_filename | exact |
| 0x5131c0 | file_get_ext | exact |
| 0x5131e0 | file_make_path_dirs | exact |
| 0x513210 | file_make_dirs | likely |
| 0x513280 | file_delete_all | likely |
| 0x513370 | vfile_raise_access_level | exact |
| 0x513390 | vfile_lower_access_level | exact |
| 0x5133b0 | file_find_files | likely |
| 0x513590 | file_list_files | unsure |
| 0x513710 | file_exists_on_disk | exact |
| 0x513770 | file_delete | exact |
| 0x5137e0 | file_delete_small_files | likely |
| 0x5138c0 | file_getcwd | exact |
| 0x5138e0 | file_chdrive | exact |
| 0x513900 | file_chdir | likely |
| 0x5139d0 | file_dir_exists | exact |
| 0x513a20 | FileReader__FileReader | exact |
| 0x513a30 | FileReader__attach_memory | exact |
| 0x513a50 | FileReader__attach_file | exact |
| 0x513a70 | FileReader__tell | exact |
| 0x513a90 | FileReader__seek | likely |
| 0x513b00 | FileReader__eof | exact |
| 0x513b30 | FileReader__error | exact |
| 0x513b40 | FileReader__read | likely |
| 0x513bb0 | FileReader__read_int | exact |
| 0x513bf0 | FileReader__read_float | exact |
| 0x513c40 | FileReader__read_int16 | exact |
| 0x513c80 | FileReader__read_uint16 | exact |
| 0x513cc0 | FileReader__read_int8 | exact |
| 0x513d00 | FileReader__read_uint8 | exact |
| 0x513d40 | FileReader__read_vector | likely |
| 0x513da0 | FileReader__read_quaternion | likely |
| 0x513e00 | FileReader__read_string | exact |
| 0x513e60 | file_set_cd_root | exact |
| 0x513ec0 | File__find_at | likely |
| 0x513f30 | File__find_in_path | exact |
| 0x513f70 | File__find_on_cd | likely |
| 0x513fb0 | File__is_absolute_path | exact |
| 0x513fd0 | File__get_packfile_path | exact |
| 0x5140b0 | File__find_free_slot | exact |
| 0x5140d0 | File__File | likely |
| 0x5140f0 | File__dtor | exact |
| 0x514100 | File__check_flags | exact |
| 0x514120 | File__check_version | exact |
| 0x514150 | File__get_version | exact |
| 0x514170 | File__set_version | exact |
| 0x514190 | File__get_full_path | exact |
| 0x5141a0 | File__get_filename | exact |
| 0x5141c0 | File__find | likely |
| 0x5143e0 | File__set_cipher_key | likely |
| 0x514460 | File__open_found | likely |
| 0x514630 | File__open | likely |
| 0x514740 | File__close | likely |
| 0x5147c0 | File__get_async_status | exact |
| 0x514810 | File__check_stream_error | exact |
| 0x514860 | File__size | exact |
| 0x5148f0 | File__is_in_packfile | exact |
| 0x514910 | File__seek | likely |
| 0x5149f0 | File__tell | exact |
| 0x514a10 | File__eof | exact |
| 0x514a40 | File__error | exact |
| 0x514a60 | File__get_path_id | exact |
| 0x514a70 | File__is_open | exact |
| 0x514a80 | File__delete | exact |
| 0x514ae0 | File__make_backup | exact |
| 0x514bf0 | File__get_attributes | exact |
| 0x514c60 | File__set_attributes | exact |
| 0x514ce0 | File__clear_attributes | exact |
| 0x514d60 | File__calc_crc32 | likely |
| 0x514eb0 | Crc32__Crc32_b | exact |

unnamed_vfile_rw_0x525720.cpp: 55 functions, 47 / 7 / 1

| Address | sum.symbols | Confidence |
|---|---|---|
| 0x525720 | init_file_zero_vector | exact (compiler-generated) |
| 0x525740 | init_file_identity_quaternion | exact (compiler-generated) |
| 0x525760 | init_file_default_matrix | exact (compiler-generated) |
| 0x525780 | init_vector3_2cbf4e8 | exact (compiler-generated) |
| 0x5257a0 | File__read_String_raw | likely |
| 0x525800 | File__read_bool | likely |
| 0x525850 | File__read_int8 | exact |
| 0x5258a0 | File__read_uint8 | exact |
| 0x5258f0 | File__read_int16 | exact |
| 0x525940 | File__read_uint16 | exact |
| 0x525990 | File__read_int | exact |
| 0x5259e0 | File__read_uint | exact |
| 0x525a30 | File__read_float | likely |
| 0x525a80 | File__read_vector | exact |
| 0x525ad0 | File__read_quaternion | exact |
| 0x525b40 | File__read_matrix | exact |
| 0x525ba0 | File__read_matrix43 | exact |
| 0x525c90 | File__read_String | exact |
| 0x525ce0 | File__read_String8 | likely |
| 0x525d80 | File__read_line_String | likely |
| 0x525e20 | File__read_string_buf | exact |
| 0x525ee0 | File__read_string | exact |
| 0x525f60 | File__read_line | exact |
| 0x525fe0 | File__read | unsure |
| 0x5261f0 | File__write_bool | exact |
| 0x526210 | File__write_int8 | exact |
| 0x526220 | File__write_uint8 | exact |
| 0x526230 | File__write_int16 | exact |
| 0x526240 | File__write_uint16 | exact |
| 0x526250 | File__write_int | exact |
| 0x526260 | File__write_uint | exact |
| 0x526270 | File__write_float | exact |
| 0x526280 | File__write_vector | exact |
| 0x526290 | File__write_quaternion | exact |
| 0x5262d0 | File__write_matrix | exact |
| 0x526300 | File__write_matrix43 | exact |
| 0x526390 | File__write_string | exact |
| 0x5263e0 | File__write_string_z | exact |
| 0x526410 | File__write | likely |
| 0x5264e0 | File__write_line | likely |
| 0x526580 | File__sync_bool | exact |
| 0x5265c0 | File__sync_int8 | exact |
| 0x526600 | File__sync_uint8 | exact |
| 0x526640 | File__sync_int16 | exact |
| 0x526680 | File__sync_uint16 | exact |
| 0x5266c0 | File__sync_int | exact |
| 0x526700 | File__sync_uint | exact |
| 0x526740 | File__sync_float | exact |
| 0x526780 | File__sync_vector | exact |
| 0x5267c0 | File__sync_quaternion | exact |
| 0x526800 | File__sync_matrix | exact |
| 0x526840 | File__sync_matrix43 | exact |
| 0x526880 | File__sync_String | exact |
| 0x5268c0 | File__sync_string_buf | exact |
| 0x526900 | File__sync_bytes | exact |

unnamed_ui_button_0x526940.cpp: 24 functions, 16 / 8 / 0

| Address | sum.symbols | Confidence |
|---|---|---|
| 0x526940 | ui_button__fire_click | exact |
| 0x526990 | ui_button__set_latched | exact |
| 0x5269c0 | ui_button__toggle_latched | exact |
| 0x5269e0 | ui_button__is_latched | exact |
| 0x526a00 | ui_button__can_accept_drop | exact |
| 0x526a50 | ui_button__process | likely |
| 0x526f30 | ui_button__vfunc0 | likely |
| 0x526f70 | ui_button__ui_button | likely |
| 0x527050 | ui_button__create | likely |
| 0x527100 | ui_button__create_in_parent | likely |
| 0x5271c0 | ui_button__set_tooltip | likely |
| 0x527280 | ui_button__set_repeats | exact |
| 0x5272c0 | ui_button__set_text | exact |
| 0x5272f0 | ui_button__was_clicked | exact |
| 0x527330 | ui_button__was_left_clicked | exact |
| 0x527360 | ui_button__was_double_clicked | exact |
| 0x527370 | ui_button__is_pressed | exact |
| 0x5273c0 | ui_button__get_flags | exact |
| 0x5273d0 | ui_button__get_text | exact |
| 0x5273e0 | ui_button__get_drag_origin | exact |
| 0x527420 | ui_button__press | exact |
| 0x527440 | ui_button__draw_arrow | likely |
| 0x527690 | ui_button__draw | likely |
| 0x527df0 | ui_button__is_selectable | exact |

unnamed_ui_static_0x527e20.cpp: 10 functions, 7 / 3 / 0

| Address | sum.symbols | Confidence |
|---|---|---|
| 0x527e20 | ui_static__vfunc0 | exact |
| 0x527e50 | ui_static__process | exact |
| 0x527ea0 | ui_static__ui_static | exact |
| 0x527f10 | ui_static__create | likely |
| 0x527fa0 | ui_static__set_tooltip | likely |
| 0x528060 | ui_static__set_text | exact |
| 0x528090 | ui_static__get_text | exact |
| 0x5280a0 | ui_static__set_justification | exact |
| 0x5280c0 | ui_static__set_text_offset | exact |
| 0x5280e0 | ui_static__draw | likely |
