# decomp49: file_packfile.cpp, mouse, exceptionhandler, multi_door.cpp, new_int_multi_update.cpp

Written from Sum.exe v1.40's disassembly (`tools/asm.mjs`). Summoner 2 / Dash Faction: names only. Target
VC++ 6.0 (cl 12.00.8966), /O2 /Ob1. Every file passes
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include <file>`, and all
106 existing src/decomp .cpp files (82 game, 24 vsdk, at commit 4224f69) compile against `include/` with the same warnings
as against the repo's headers.

The repo moved from a2d447c to 4224f69 while this batch ran; `include/` is the 4224f69 tree plus this batch's
changes (multi.h and file.h were re-edited on the new versions).

| File | Original unit | Functions | Exact / likely / unsure |
|---|---|---|---|
| `vsdk/file_packfile.cpp` | `vsdk\vfile\packfile\file_packfile.cpp` (assert path 0x5a9be0) | 24 (4 compiler-generated) | 14 / 10 / 0 |
| `vsdk/mouse.cpp` | sum.symbols `vsdk\os\mouse`; **file name a placeholder** (no path string) | 30 (+ clamp_int, + 2 initialiser bodies) | 27 / 3 / 0 (clamp_int exact) |
| `vsdk/exceptionhandler.cpp` | sum.symbols `exceptionhandler`; **directory and file name placeholders** | 8 | 5 / 3 / 0 |
| `game/multi_door.cpp` | `multi_door.cpp` (strings.tbl id 246) | 6 | 3 / 3 / 0 |
| `game/new_int_multi_update.cpp` | `new_int_multi_update.cpp` (strings.tbl ids 932 / 933) | 5 | 1 / 4 / 0 |
| total | | 73 (+3) | 50 / 23 / 0 |

## Functions

### file_packfile.cpp (code 0x533db0-0x534a1b; .data 0x5a9aac-0x5a9cf7 literals only; .bss 0x2d0bf98-0x2d90ed3)

| Address | Name | Conf. | Note |
|---|---|---|---|
| 0x533db0 | init_packfile_string_pool | exact | compiler-generated (`StringPool packfile_string_pool;`) |
| 0x533dc0 | packfile_string_pool_ctor | exact | compiler-generated |
| 0x533dd0 | packfile_string_pool_register_dtor | exact | compiler-generated (atexit) |
| 0x533de0 | packfile_string_pool_dtor | exact | compiler-generated |
| 0x533df0 | packfile_init | likely | load time `(float)elapsed * 0.001f` from an int local |
| 0x533ef0 | packfile_release_string_pool | exact | |
| 0x533f10 | packfile_lookup_insert | exact | |
| 0x533f60 | packfile_check_header | likely | returns the file count (EAX), 0 for a bad header |
| 0x533f90 | packfile_read_directory | likely | 60-byte name memcpy, down-counting loop |
| 0x534090 | packfile_hash | likely | `_rotl` intrinsic, `(char)tolower`, abs intrinsic |
| 0x5340d0 | packfile_assign_offsets | exact | |
| 0x534110 | packfile_alloc_directory | likely | error line 423 |
| 0x5341a0 | packfile_load_cd | likely | |
| 0x5342e0 | packfile_load_hdd | likely | |
| 0x534420 | packfile_add | likely | returns int (D2900) |
| 0x5345a0 | packfile_find_packfile | exact | |
| 0x5345f0 | packfile_find | likely | |
| 0x534690 | packfile_fopen | likely | the two fseek tails merged |
| 0x5347e0 | packfile_fclose | exact | |
| 0x534830 | packfile_fseek | exact | |
| 0x5348a0 | packfile_fread | exact | |
| 0x5348f0 | packfile_fstatus | exact | |
| 0x534900 | packfile_read_whole_file | exact | |
| 0x5349a0 | packfile_find_in_packfile | exact | (vpp, name, &size, &offset) confirmed |

Globals: packfile_string_pool 0x2d0bf98, packfile_entries 0x2d0bfb0 (name ours, 0x34bc x 0x1c), Packfiles
0x2d68440, Packfile_lookup 0x2d68780, packfile_num_entries 0x2d90ec8, Num_packfiles 0x2d90ecc,
packfile_initialized 0x2d90ed0.

### mouse (vsdk/mouse.cpp; code 0x500a30-0x501468; .data 0x5a5734-0x5a573b; .bss 0x257ef84-0x257f03f)

| Address | Name | Conf. |
|---|---|---|
| 0x500a30 (+ body 0x500a40) | init_mouse_buttons | exact (compiler-generated) |
| 0x500a60 (+ body 0x500a70) | init_mouse_buttons_live | exact (compiler-generated) |
| 0x500a90 | mouse_update | likely (latch-loop store order) |
| 0x500c80 | mouse_set_pos | exact |
| 0x500cd0 | mouse_di_read | likely (retry loop shape) |
| 0x500de0 | mouse_init | exact |
| 0x500e90 | mouse_di_init | exact |
| 0x500f60 | mouse_msg_handler | exact |
| 0x501020 | mouse_close | exact |
| 0x501050 | mouse_di_close | exact |
| 0x501090 | mouse_flush | likely (delta stores vs. EnterCriticalSection) |
| 0x501100 | mouse_mark_button | exact |
| 0x5011a0 | mouse_clear_clicks | exact |
| 0x5011e0 | mouse_get_pos | exact |
| 0x501240 | mouse_move_to | exact |
| 0x501280 | mouse_set_pos_xyz | exact |
| 0x5012c0-0x501430 | mouse_is_down ... mouse_keep_centered_disable (13) | exact |
| 0x501440 | mouse_button::mouse_button | exact (implicit constructor) |
| 0x501450 | clamp_int (tag "-", not counted in the 30) | exact (D2902) |

### exceptionhandler (vsdk/exceptionhandler.cpp; code 0x5419a0-0x5424fa; .data 0x5ac368-0x5ac8bf; .bss 0x2ee250c-0x2ee2517)

| Address | Name | Conf. |
|---|---|---|
| 0x5419a0 | RecordExceptionInfo | likely (SEH frame: local slots, the stored 50-byte slack) |
| 0x541ec0 | hprintf | exact |
| 0x541f10 | RecordModuleList | likely (`mov esi, 0`) |
| 0x541fb0 | ShowModuleInfo | likely (returns inside __try) |
| 0x542130 | PrintTime | exact |
| 0x5421d0 | RecordSystemInformation | exact |
| 0x5422f0 | GetExceptionDescription | exact |
| 0x5424e0 | GetFilePart | exact |

mingw has no `__try` / `__except` or MSVC `__asm`: the file maps `__try` to `if (1)` and `__except(f)` to
`else if ((f), 0)` under `#ifndef _MSC_VER`, and reads fs:[4] with `__readfsdword` there.

### multi_door.cpp (code 0x4aa780-0x4aac05; .data 0x59d34c-0x59d39b; no .bss)

| Address | Name | Conf. |
|---|---|---|
| 0x4aa780 | mp_handle_door_toggle | exact |
| 0x4aa7d0 | mp_handle_door_lockpick | likely (`locked` in the dead argument slot) |
| 0x4aa8e0 | door_find_by_mesh | likely |
| 0x4aa9c0 | mp_send_door_toggle | exact |
| 0x4aaa10 | door_pick_by_mesh | likely (`open` reuses the door pointer's slot) |
| 0x4aab90 | mp_send_door_lockpick | exact |

### new_int_multi_update.cpp (code 0x410cf0-0x411260; .data 0x58bd34-0x58bda2; .bss 0x5bd0d4-0x5bd2db)

| Address | Name | Conf. |
|---|---|---|
| 0x410cf0 | version_check_fetch | likely (EH frame of `new InetGetFile`) |
| 0x410e20 | version_check_poll_download | likely (the connecting / receiving tests decide nothing) |
| 0x410f10 | version_check_set_error_text | exact |
| 0x410f40 | version_check_run | likely (switch -1..2, shared tails) |
| 0x4110b0 | version_read_nfo_file | likely (frame has 8 unexplained bytes) |

Globals: version_check_need_registry 0x58bd34 = 1 and version_check_need_fetch 0x58bd38 = 1 (names ours),
version_check_result 0x58bd3c = 999, version_check_status_text 0x5bd0d4, version_check_download 0x5bd2d4,
version_check_done 0x5bd2d8.

## Open questions

- packfile_hash: the inline `rol` needs `_rotl` as an intrinsic (/Oi or `#pragma intrinsic`); a plain
  `(h << 5) | (h >> 27)` may also give `rol` in VC6. Same for abs.
- packfile_check_header returns the file count; the source may have returned `pf->num_files` or a local.
- version_read_nfo_file's frame is 0x414 bytes: 2 x 512 + 12 for the three numbers leaves 8 bytes unexplained
  (`line` at frame +0xc, `last` at +0x210).
- new_int_multi_update.cpp's literal order (reversed within each function, "version.nfo" once) suggests /Gf
  for the interface units, unlike the vsdk units (file_packfile.cpp has six "rb").
- door_find_by_mesh for the host's own click calls door_set_open(l, -1, 0, 1) / door_set_closed(l) on a door
  door_toggle_state has already set moving; whether the third argument (0 here, 1 in the DOOR_TG handler)
  makes the host's door restart its movement is not known.
- The mouse unit's 0x257efe4 (between mouse_x and mouse_buttons_live) has no reference.
- clamp_int may be an out-of-line inline from a header (gr / math) that the mouse object happened to emit.

## Doc disagreements

docs/sum-exe/vsdk-packfile.md:
- "18 functions": the unit has 24 (20 plus the four string-pool initialiser / destructor helpers).
- packfile_fread: "FREAD BAIL" is printed for a NULL entry or an entry without a packfile, not on a short read.
- packfile_find_in_packfile is `(vpp, name, &size, &offset)` (not `&offset, &size`), and the role is no longer
  inferred: TextFile::Open uses it.
- The header's fourth dword is the size of the whole .vpp (the page says "total size", right); symbols/types.h
  and the sum.symbols comment of packfile_check_header call Packfile +0x30 the directory size (D2901).
- packfile_add looks on the hard drive (install_path) first, then the CD; the page says CD or hard drive.
  Limits: 16 packfiles, 13,500 files in all (0x34bc; "Packfile static alloc" prints 0x5c490 bytes), 31-character
  packfile names.
- "levele/levelm/levelt are presumably added elsewhere, for example per level (inferred)": level_load adds
  them (0x4385fa-0x43860e); a second add of the same name is ignored.
- Lookups use only the file part of the name (file_get_filename), so files with the same name in two
  packfiles shadow each other: the first one added wins.

docs/sum-exe/vsdk-gr.md (section Mouse) and docs/sum-exe/controls.md:
- "DirectInput mode ... Cursor mode: used if DirectInput fails": v1.40 always uses cursor mode.
  mouse_use_cursor is 1 in .data and nothing clears it, so mouse_di_init returns -1 before DirectInputCreateA
  (D2903). controls.md's "only the mouse [uses DirectInput]" is wrong too: nothing does. SummonerFix's
  dinput.dll is loaded because DINPUT.dll!DirectInputCreateA is a static import (IAT 0x57e02c), not because
  mouse_di_init calls it.
- "Mouse look: optionally warps the pointer back to the centre": dead, keep_mouse_centered and mouse_hidden
  are never set (their setters have no callers).
- The module ends at 0x501468 (clamp_int), not 0x50144f.
- Cursor mode clamps the cursor to the client area and moves the OS cursor back inside (mouse_set_pos) while
  the window is in the foreground.
- Double clicks are the game's own (a 400 ms RealtimeTimer per button, mouse_double_click_time 0x5a5738);
  WM_*BUTTONDBLCLK are ignored.

docs/sum-exe/vsdk-os.md (errorlog.txt) and src/decomp/game/summoner.cpp:
- RecordExceptionInfo returns EXCEPTION_CONTINUE_SEARCH on every path, so WinMain's __except body never runs
  and the crash continues to the system handler after the report; summoner.cpp's comment "the result stays -1
  then" describes a path that is not taken (D2905). Only the first exception is reported (static flag
  0x2ee250c). The report also has the 16 bytes at EIP ("??" when unreadable) and at most 8 KB of stack.

## Game bugs and defects (all latent or cosmetic)

- packfile_read_whole_file opens the .vpp by its bare name (relative to the current directory, not
  install_path) and reports success for a CD packfile without reading anything. Its only caller,
  file_blast_into_buffer, has no caller in v1.40, so it never runs.
- packfile_add tests `name == NULL` only after using name; a name of 32+ characters is refused only after the
  file was found and opened.
- packfile_fseek adds the entry's offset for every origin, so SEEK_END would seek from the end of the .vpp.
- packfile_fclose / packfile_fseek / packfile_fread do not test the CD stream for NULL.
- multi_door.cpp: mp_handle_door_toggle / mp_handle_door_lockpick index lobjs with the packet's value without a
  range check (a malformed packet reads out of bounds). The pick never clears the door's lock value, only the
  two-stage locked flag, so DOR_PCK result 2 ("This door already unlocked.") is only sent for doors without a
  lock value; a door that was picked once is rolled again (and its level use hook run again) if a pick is
  requested for it once more.
- new_int_multi_update.cpp: the feof loop processes the last line twice (harmless); the check compares
  against 1.40.00 (14000) and the pxo.net URL is gone, so today the check ends in result 3 ("There was an
  error verifying your version ...") unless the SkipVerify registry DWORD is set.
- mouse: the button accessors do not range-check the button; mouse_time_since_press has no mouse_inited test.
