# Phase 3: summoner.cpp (decomp26)

Written from Sum.exe's disassembly only (`tools/asm.mjs fn`, raw .data/.rdata bytes). No Summoner 2 or
Dash Faction code was read. `game/summoner.cpp` passes
`i686-w64-mingw32-g++ -fsyntax-only -Wall -Wno-write-strings -Wno-format-security -I include game/summoner.cpp`
with no output, and all 49 existing `src/decomp` .cpp files give the same output with `decomp26/include`
as with the repo's tree (based on commit 62b49d2, which landed during this run: the header changes were
rebased onto it). Header changes: `CHANGED-HEADERS.txt`; decisions D600-D606 in `include/DECISIONS.md`.

## Unit (0x43cd70-0x43e654; .rdata 0x580e38-0x580e4f; .data 0x594440-0x594f2b; .bss 0x60abb0-0x60ad7b)

sum.symbols tags 60 functions `summoner.cpp`, 0x43cd70-0x43e4a0. WinMain (0x43e5e0, module `-`) is
written here too: its literal "Summoner Main Thread" (0x594f14) is the last string of this unit's .data
(summoner_cd.cpp's "SUMMONER_2" follows at 0x594f2c) and its SEH scope table (0x580e50) directly follows
resolution_table (0x580e38-0x580e4f) in .rdata. Proposed tag: WinMain -> summoner.cpp.

| Address | Function | Confidence |
|---|---|---|
| 0x43cd70 / 0x43cd80 | init_level_display_names_thunk / init_level_display_names (initialiser of level_display_names) | likely (XSTR_SLOT spelling, D602) |
| 0x43d180 / 0x43d190 | init_game_config_defaults_thunk / init_game_config_defaults (Summoner_options constructor) | exact |
| 0x43d220-0x43d4e5 | sinit_ / init_ / register_dtor_ / dtor_ for the 9 CmdLineParams (port, directsound3d, eax, pack, glide, direct3d, 800, 1024, dsinit): 36 functions | exact |
| 0x43d4f0 / 0x43d500 | init_ccmd_gamma_thunk / init_ccmd_gamma | exact |
| 0x43d520 | cmd_gamma | exact |
| 0x43d5b0 / 0x43d5c0 | init_ccmd_play_sound_thunk / init_ccmd_play_sound | exact |
| 0x43d5e0 | cmd_play_sound | likely (stack order of name / File / message) |
| 0x43d710 | console_sound_stop | exact |
| 0x43d750 / 0x43d760 | init_ccmd_stop_sound_thunk / init_ccmd_stop_sound | exact |
| 0x43d780 | cmd_stop_sound | exact |
| 0x43d7b0 | MainLoop | exact |
| 0x43d7f0 | LoadSettings | likely (empty branches, see 2) |
| 0x43dc30 | cmdline_read_params | exact |
| 0x43dca0 | setup_data_paths | exact |
| 0x43df20 | game_shutdown_systems | exact |
| 0x43df40 | GameInit | likely (clamp shape; font switch `case 0: default:`) |
| 0x43e1f0 | game_cleanup | exact |
| 0x43e250 | summoner_cfg_load | likely (goto, see 3) |
| 0x43e410 | summoner_cfg_save | exact |
| 0x43e4a0 | launch_updater | likely (order of the buffer clears) |
| 0x43e5e0 | WinMain (untagged) | likely (unit membership; code exact; __try under _MSC_VER, D601) |

Totals: 61 functions, 53 exact, 8 likely, 0 unsure. Nothing left in the unit.

Data: resolution_table (.rdata, unsigned, D606); level_display_names (.data, 51 entries, D603);
Summoner_options, 9 CmdLineParams, 3 console_commands, 17 path ids, 3 fonts, resolution_index, 5
command-line bytes, the console sound handle pair (.bss). Pooled literals from earlier units: "",
"masad", "catacombs", "Eleh", "Liangshan", ".plr", ".sav".

## Open questions

1. **.bss order.** The addresses (path_maps_effects 0x60abb0 < cmdline_param_direct3d 0x60abb8 <
   ccmd_stop_sound 0x60abd0 < ... < console_sound_buffer 0x60ad78) follow neither the initialiser order
   nor any grouping; same question as frametime.cpp / player.cpp. Unreferenced 4-byte gaps at 0x60abb4,
   0x60abdc, 0x60abf4, 0x60ac50, 0x60ac64, 0x60acc4, 0x60ad14, 0x60ad4c.
2. **LoadSettings' empty branches.** `_stricmp(soundcard, "DirectSound")` equal skips everything, and the
   last `_stricmp(soundcard, "")` result is unused; `strstr(card_name, "3DFX")` true jumps over the
   Direct 3D test (`je` over `jmp` at 0x43dad3-0x43dad5). Written as empty if-bodies; they may have held
   stores to a dead local (e.g. a sound / renderer type) or `renderer = GR_GLIDE` that VC6 dropped.
3. **summoner_cfg_load's control flow.** The open-failed path and the short-file path both reach the
   video-memory test, a full file skips it; written with a `goto loaded`. The clamp is `>= 0x40` (JL at
   0x43e345), the final test unsigned (JAE at 0x43e36a: `size >= sizeof(Summoner_options)`).
4. **XSTR_SLOT.** VC6 treated `&localize_strings[id]` as non-constant (only entry 0's name is in the
   image); the original macro is not known (D602).
5. **0x60ad6d / 0x60ad6e** (labels ours, cmdline_flag_60ad6d/e): LoadSettings tests them with
   cmdline_directsound3d and cmdline_eax before reading the Soundcard key, nothing writes them. Probably
   two command-line switches removed from cmdline_read_params (A3D?).
6. **0x4f13e0** in game_cleanup is a folded empty function of another unit (written empty_function()).
7. **gr_init's float.** LoadSettings passes `(float)Summoner_options.cfg_34`, which gr_init stores at
   gr_screen +0x28 (far_zvalue in types.h). Either the field or the option is misnamed.

## Game bugs / oddities noticed

- **ComputerSpeed is dead (bug).** GameInit reads HKCU..\ComputerSpeed (0..2) and presets fog / detail
  fields of Summoner_options (0x43e080-0x43e0fe), then calls summoner_cfg_load, which first resets every
  field to its default (0x43e295-0x43e320) and then reads summoner.cfg: the Setup speed choice never
  has an effect. (Its presets also give speed 2 party detail 1 and speed 1 party detail 2.)
- **cfg_34 at gr_init** is always the constructor default (1), because summoner_cfg_load runs later in
  GameInit.
- **"-glide"** is constructed but never tested (Glide is the default renderer anyway); **"-pack"** is
  stored at 0x60ad70 and never read.
- **cmd_gamma** only stores the value; the ramp changes at the next options_apply_detail.
- **cmd_stop_sound** ("Stops all sounds that are playing") stops only the play_sound stream;
  console_sound_stop forgets the snd_get_handle sound without releasing it.
- **launch_updater** never closes the PROCESS_INFORMATION handles (harmless: the game is exiting).
