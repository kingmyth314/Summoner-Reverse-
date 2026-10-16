# decomp74 notes (runs-b.txt)

Snapshot: snap3927 (commit 3927ca2) copied to `decomp74/src`. All 69 runs (187 functions) now have a line-start
`// 0xADDR` marker in a src/decomp .cpp; 165 exact / 19 likely / 3 unsure. Libjpeg was not in the list. Four more
functions were written on the way (0x4f22b0 lsf_khosanilab2_flag_pyrulstart and 0x51ba70 InetGetFile::GetErrorCode,
neither in sum.symbols; 0x5312c0 cpu_read_tsc and 0x5193a0 net_get_reliable_socket_status, which had a marker but
no code).

Checks: mingw `-fsyntax-only` over all 251 .cpp under game and vsdk: 0 errors. VC6 (`/O2 /GX /MT`, a scratch copy of
tools/vc6-compare.mjs with `DECOMP_INCLUDE` pointing at decomp74's include, `decomp74/tools/vc6c.mjs`; the repo
tool hard-codes the repo's include dir): 46 files compiled old vs new (every touched .cpp plus every caller of the
changed bmpman.h / inetgetfile.h declarations): **no function lost a match**; gained 0x4386c0 (gameplay.cpp, from
progress_update's bool return), 0x4db250, 0x4db2c0, 0x503dc0, 0x51b750, 0x543030, 0x5193a0, 0x5193f0, 0x5312c0;
the ten new units: 28 of 32 functions match.

## Per run

| Run | Where | Confidence |
|---|---|---|
| 0x4b5150 init_empty_4b5150 | game/unnamed_ai_think_0x4b5140.cpp, marker only (not reproduced, D5600) | unsure |
| 0x4ceed0-0x4ceee0 savegame_state initialiser | level_scripts.cpp, comment split into markers | exact |
| 0x4cef10, 0x4d4bd0, 0x4d4c60, 0x4d4e80, 0x4d5e50, 0x4d6560, 0x4d65e0 ccmd initialisers | level_scripts.cpp, markers | exact |
| 0x4d5000 init_script_object_timers | level_scripts.cpp, marker | exact |
| 0x4d5030 init_empty_4d5030 | level_scripts.cpp, marker only (D5600) | unsure |
| 0x4d5050-0x4d5140 the three StringPools, level table | level_scripts.cpp, markers | exact; 0x4d5140 init_level_table likely (as before) |
| 0x4d6670-0x4d68b0 dialogue boxes, colours, more / end text | unnamed_dialogue_box_0x4d6660.cpp, markers | exact; the two text slots likely (as before) |
| 0x4d7b40-0x4d7b70 conversation_db | conversation.cpp, markers | exact |
| 0x4d87e0-0x4d8810 conversation_active | conversationctl.cpp, markers | exact |
| 0x4d8f00-0x4d8f40, 0x4d9a80-0x4d9aa0, 0x4da830 header-inline COMDATs | conversationctl.cpp, markers (bodies in conversation.h, camera.h, vector.h, living_entity.h) | exact |
| 0x4daff0, 0x4db0e0, 0x4db1d0 ccmd initialisers | conversationctl.cpp, markers | exact |
| 0x4db250-0x4db2c0 str_copy_append_32, filename_has_cscript_suffix | script_internal.cpp, written (D5601) | exact (byte match) |
| 0x4ef120-0x4ef140 IkaemosExt2 | new game/level_script_ikaemosext2.cpp | exact (match) |
| 0x4ef850 lenele1ab | new game/level_script_lenele1ab.cpp | exact (match) |
| 0x4f2250-0x4f2430 KhosaniLab2 | new game/level_script_khosanilab2.cpp (+0x4f22b0, D5603) | exact (all 5 match) |
| 0x4f2830-0x4f29c0 tancredhouse, ls_test_use | new game/level_script_tancredhouse.cpp (D5604) | exact (match) |
| 0x4f3a80-0x4f4210 KhosaniStrng | new game/level_script_khosanistrng.cpp | 3 exact (match), init likely (D5612) |
| 0x4f4910 Rand-Desert | new game/level_script_rand_desert.cpp | exact (match) |
| 0x4f9e50-0x4f9e70 IkaemosExt | new game/level_script_ikaemosext.cpp | exact (match) |
| 0x501490 init_frametime_saved_max | vsdk/frametime.cpp, marker | likely (as before) |
| 0x503dc0 imin | vsdk/bmpman.cpp, defined at the end (D5606) | exact (match) |
| 0x503dd0-0x503e80 progress | new vsdk/unnamed_progress_0x503dd0.cpp (D5605) | exact (both match) |
| 0x50fa30 init_identity_matrix43 | vsdk/matrix.cpp, marker | likely (as before) |
| 0x511c10-0x511d30 vector3 / matrix43 inlines | vsdk/matrix.cpp, markers | exact |
| 0x5172f0-0x517430 movie_play, movie_blit_frame | new vsdk/unnamed_movie_0x5172f0.cpp (D5607) | blit exact (match), play likely |
| 0x5193f0 net_get_reliable_socket_ping | vsdk/unnamed_net_0x517590.cpp, written with _status (D5610) | exact (both match) |
| 0x5194e0, 0x51b5c0-0x51b5d0 initialisers | vsdk/character_instance.cpp, markers | likely (as before) |
| 0x51b6c0 matrix43__copy, 0x51b700 vector3__negated | vsdk/character_instance.cpp, markers | exact |
| 0x51b750 imax | vsdk/character_instance.cpp, defined at the end (D5606) | exact (match) |
| 0x51b760-0x51bb50 InetGetFile + 2 scalar deleting dtors | new vsdk/unnamed_inetgetfile_0x51b760.cpp (D5611) | 10 exact (8 match + the 2 COMDAT markers), ctor and dtor likely |
| 0x52f5a0, 0x52f5c0 ui_window sentinel | vsdk/unnamed_ui_manager_0x52f570.cpp, markers | exact |
| 0x5312d6-0x531aa8 cpu_identify, PrintSystemInfo, sysinfo ccmd | vsdk/unnamed_os_0x530b90.cpp, written under /Od (D5608) | likely (5) |
| 0x543030 ui_stub_ret_543030 | vsdk/skeleton.cpp, first function (D5609) | unsure (placement; body matches) |
| 0x544b20-0x544b70 key constructors | vsdk/skeleton.cpp, markers | exact |
| 0x5504f0 glFinish import thunk | vsdk/gr_opengl.cpp, marker | exact |
| 0x551790-0x551940 anim_track accessors | vsdk/unnamed_anim_track_0x550660.cpp, markers one per line | exact |
| 0x562310-0x562a20 vfx emitter / particle accessors | vsdk/unnamed_vfx_emitter_0x55ff00.cpp, markers one per line | exact |
| 0x5647a0-0x564d50 vfx mesh accessors | vsdk/unnamed_vfx_mesh_0x562a30.cpp, markers one per line | exact; 0x564ae0, 0x564d20 likely (as before) |
| 0x564d80 vfx_material_layer__dtor | vsdk/unnamed_vfx_material_0x564d70.cpp, marker | exact |

## Bugs noticed (none fixed)

- **cpu_identify 0x5312d6** (live, cosmetic, sysinfo console command only): `mmx_ok` is initialised to 1 and never
  cleared, so `*mmx` is always 1 after the feature test; the AMD and Intel name strings test the pointer `mmx`
  (`cmp [ebp+0xc], 0`), not `*mmx`, so they always say "MMX: Yes" / "w/ MMX" / "Pentium II w/ MMX"; the
  GetPriorityClass / GetThreadPriority / SetPriorityClass / SetThreadPriority calls pass handle 0, so the speed
  measurement runs at normal priority.
- **PrintSystemInfo 0x53171f** (live, cosmetic): "RAM Speed: %d MB/s" prints frequency / ticks-per-copy, i.e. 256 KB
  copies per second, four times the MB/s value.
- **InetGetFile::InetGetFile 0x51b780** (latent): a NULL URL or file name sets error 1 but both are still passed to
  strstr; no caller passes NULL.
- **ls_khosanistrng_init 0x4f3a80** (cosmetic): two debug `printf`s ("enabled kl1\n" / "enabled kl2\n") to a console
  that does not exist in the Windows build.
- **movie_play 0x5172f0**: constructs and destroys a File object it never uses (harmless).

## Disagreements with symbols / docs

- sum.symbols ls_test_use (0x4f29c0): the comment ("reacts (opens gates, speaks a refusal)") does not fit the body
  `xor al, al; ret`; it is tancredhouse's (folded) use hook (D5604).
- sum.symbols vfx_particle__set_velocity / set_pos / get_velocity / get_pos (0x562340-0x562420): the header
  (effect_mem.h, D4607) calls +0x14 pos and +8 vel, sum.symbols the other way round; the markers give both.
- sum.symbols has no entries for 0x4f22b0 (khosanilab2's frame body) and 0x51ba70 (InetGetFile::GetErrorCode).
- source-map.md says str_copy_append_32 / filename_has_cscript_suffix are "probably the end of conversationctl.cpp";
  conversationctl.cpp's own header said they start the next unit. Written in script_internal.cpp (D5601), not proven.
- inetgetfile.h said the class spans 0x51b760-0x51bb10; it ends with its COMDATs at 0x51bb6b.
- The ai_think unit defines two vector3 arrays at file scope; under VC6 each gives an empty initialiser pair, the
  binary has one (0x4b5140/0x4b5150), so one of them is probably not a vector3 array in the original (D5600).

## Left

- ls_khosanistrng_init: one `add esp, 0x10` scheduled before a printf (D5612).
- movie_play: the File local's frame slot (D5607).
- InetGetFile constructor (EH layout) and destructor (the original calls the scalar deleting destructors, D5611).
- cpu_identify / PrintSystemInfo / cmd_sysinfo: /Od local layout depends on the locals' names (D5608).
- The two empty initialisers (which objects, D5600) and 0x543030's placement (D5609).
- Libjpeg (0x5664a0-) not touched, as instructed. Nothing multiplayer was left (the reliable-socket getters and
  InetGetFile were done last).
