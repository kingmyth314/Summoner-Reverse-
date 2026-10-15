# Phase 3 batch: registry.cpp, gamesound.cpp, item_paging.cpp (source half)

Written from build/sum.asm (`tools/asm.mjs`) and raw .data bytes of Sum.exe only. Summoner 2 and
Dash Faction were not consulted. Names from symbols/sum.symbols; labels marked "ours" in the
source where the symbol file has none. Syntax-checked with
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wextra -I <this folder> -I src/decomp/vsdk` (clean, all
three). `engine_stubs.h` declares the engine functions/classes called (signatures from call sites;
not original). Not compiled with VC6.

## registry.cpp (vsdk\os\registry.cpp, 0x511d80-0x5124b5)

| Address | Function | Confidence |
|---|---|---|
| 0x511d80 | os_registry_write | likely |
| 0x511e50 | os_registry_resolve_key | likely |
| 0x511f90 | os_registry_hkey_from_string | exact |
| 0x512050 | os_registry_read | likely |
| 0x512110 | os_registry_set_app_key | exact |
| 0x5121c0 | os_registry_read_String | exact |
| 0x512210 | os_registry_read_string_nodefault | exact |
| 0x512280 | os_registry_read_string | exact |
| 0x512310 | os_registry_write_string | exact |
| 0x512340 | os_registry_read_dword | exact |
| 0x512370 | os_registry_write_dword | exact |
| 0x512390 | os_registry_read_int | exact |
| 0x5123c0 | os_registry_write_binary_int | exact |
| 0x5123e0 | os_registry_delete | likely |

14 functions: 10 exact, 4 likely. No static initialisers. .bss: os_registry_app_key 0x2692ef8,
os_registry_app_key_set 0x2692ff8 (byte store, so `bool`), two 4-byte lpClass objects 0x2692ffc /
0x2693000 (labels ours).

## gamesound.cpp (Engine\gamesound\gamesound.cpp, 0x468c80-0x4697d6)

| Address | Function | Confidence |
|---|---|---|
| 0x468c80 / 0x468c90 / 0x468cb0 / 0x468cc0 | sinit / init / register_dtor / dtor of `StringPool feedback_string_pool(buf, 0x1400, false)` | exact |
| 0x468cd0 | sound_ref_load | likely |
| 0x468d90 | game_play_sound | exact |
| 0x468dc0 | sound_play_2d | exact |
| 0x468de0 | sound_play_3d_at | likely |
| 0x468e00 | surface_play_sound | exact |
| 0x468e30 | surfaces_parse_table | likely |
| 0x469010 | lobj_face_get_surface | exact |
| 0x469080 | music_play | exact |
| 0x469140 | music_is_playing | exact |
| 0x469160 | music_get_current | exact |
| 0x469180 | music_stop | exact |
| 0x4691b0 | music_set_volume | exact |
| 0x4691e0 | music_feedback_parse_tables | likely |
| 0x469580 | music_feedback_reset_tables | unsure |
| 0x4696d0 | music_find_by_name | exact |
| 0x469720 | feedback_play | likely |

20 functions (4 compiler-generated + 16): 14 exact, 5 likely, 1 unsure. The unit is 11 functions
longer than the module column of symbols/sum.symbols says (only 0x469080-0x469720 are tagged).

## item_paging.cpp (Engine\objects\item_paging.cpp, 0x46c5d0-0x46cc7a)

| Address | Function | Confidence |
|---|---|---|
| 0x46c5d0 / 0x46c5e0 | init_mempools_ip_general_thunk / init_mempools_ip_general (array of 19 Mempools) | unsure |
| 0x46c7c0 | item_page_in_stub | exact |
| 0x46c7d0 | item_page_in_mesh | exact |
| 0x46c880 | item_type_page_in | exact |
| 0x46c8f0 | items_reset_paging | exact |
| 0x46c940 | item_page_in | likely |
| 0x46ca50 | item_page_out_all | likely |
| 0x46cb30 | item_type_is_paged_in | exact |
| 0x46cb50 | item_paging_find_player_slot | likely |
| 0x46cc60 / 0x46cc70 | init_realtimetimer_a59a84_thunk / init (RealtimeTimer mp_player_autosave_timer) | unsure (placement) |

12 functions (4 compiler-generated + 8): 5 exact, 3 likely, 2 unsure pairs (counted as 4).

Totals: 46 functions; 29 exact, 12 likely, 5 unsure (each initialiser pair counted as 2).

## Facts used

- ADVAPI32 IAT order from `llvm-objdump -p`: RegCreateKeyExA 0x57e000, RegQueryValueExA 0x57e004,
  RegDeleteKeyA 0x57e008, RegCloseKey 0x57e00c, RegSetValueExA 0x57e010, RegOpenKeyExA 0x57e014,
  RegDeleteValueA 0x57e018. KEY_ALL_ACCESS = 0xf003f, KEY_QUERY_VALUE = 1; types REG_SZ 1,
  REG_BINARY 3, REG_DWORD 4.
- Error macro: every DebugError call site in Sum.exe jumps back to itself, so the source writes
  `for (;;) DebugError(file, line, msg);`. Lines: registry 185, gamesound 661/680/695,
  item_paging 671. The file literals are spelled out (`#define ..._CPP`) because a rebuild must
  push the original `D:\projects\Summoner\pccode\...` strings.
- Floats: 1/255 = 0x3b808081 (0x57f030, multiplied, so not written as a division), 100.0f
  (0x57e6b4), 5.0f / 1.0f immediates. All float, no double. `_ftol` once (feedback roll).
- .data (gamesound, 0x59a31c): surface names {"wood","stone","grass","sand","swamp"} ("wood" and
  "stone" pooled with 0x5991e8/0x5991f0, so /Gf), surface_default = 1, music signature / handle /
  index = -1, music_looping = 1, 16 feedback event names 0x59a344. .data (item_paging, 0x59a750):
  19 slots, 18 initialised {-1, -1, &pool[i], 0}, slot 18 all zero.
- .bss sizes from neighbouring symbols: surface_types 64 x 8, feedback_sounds 255 x 4,
  music_tracks 64 x 8 (symbols say `char*[704][2]`, wrong), string buffer 0x1400, feedback byte
  tables [4][16]. Item paging: level buffer 0x38000 at 0x971870, general buffers 19 x 0x8000 at
  0x9a9870 (17 used), popup buffer 0x18000 at 0xa41870, 19 pools x 0x1c at 0xa59870.
- symbols/sum.symbols corrections suggested: item_page_slots is 19 slots (int[76]), not 17;
  ip_general_mempool is Mempool[19]; music_tracks is {name, filename}[64];
  music_feedback_reset_tables fills surface_sound_table only (name is misleading);
  num_feedback_sounds is compared unsigned.

## Open questions

1. **gamesound.cpp start.** I put 0x468c80-0x469072 (stringpool initialiser, sound_ref_load,
   game_play_sound, sound_play_2d/3d_at, surface_play_sound, surfaces_parse_table,
   lobj_face_get_surface) in gamesound.cpp on .bss interleaving and the sound.md listing; the
   symbol file has them as module "-".
2. **init_mempools_ip_general.** One initialiser with 19 Copy calls means one definition. I wrote
   `Mempool ip_general_mempool[19] = { Mempool(buf, size, name), ... }` with an inline
   constructor that calls Copy. VC6 may instead build temporaries and copy them; another shape
   (a static object whose inline constructor calls Copy 19 times) gives the same code with an
   extra .bss byte somewhere. Needs the compiler.
3. **mp_player_autosave_timer (0xa59a84) placement**: end of item_paging.cpp or start of the
   next unit (mp_player_file_create 0x46cc80..., its only users). Same addresses either way;
   kept here per the symbol file.
4. **.bss definition order vs. constructor argument**: feedback_string_pool (0x941740) precedes
   its buffer (0x9423d8); the source forward-declares the buffer. Same question as frametime.cpp
   open question 1.
5. **music_feedback_reset_tables statement order**: the binary groups equal constants; I wrote
   lower-triangle row order, which matches the first-use order except where single immediates are
   hoisted. The loop bound `i <= 16` follows the JLE against row 16.
6. **sound_ref_load argument order**: floats computed params[0], [1], [2] but passed as
   (p[1], p[0]/255, p[2]); written with locals. snd_get_handle's three floats are unnamed.
7. **snd_play_3d trailing parameter types** (two floats and an int here) are a guess; only
   zeroes are passed. Same for snd_play's third/fourth parameters.
8. **os_registry_resolve_key nesting**: `if (subkey) { if (!strnicmp) ... }` vs early returns;
   the NULL case is laid out last. The two lpClass objects might be function-local statics.
9. **item_page_out_all, popup slot**: for slot 18 with a sharing slot, the *other* slot is cleared
   and slot 18 keeps its type, while the type's mesh pointer is zeroed. Reproduced as is; looks
   like a defect but I did not check the runtime effect.
10. **Defects reproduced**: os_registry_delete formats "Error removing registry key" with the NULL
    name; os_registry_resolve_key reads past the end of an "HKEY_xxx" string without '\';
    surfaces_parse_table re-parses into a 64-byte buffer with size 256.
