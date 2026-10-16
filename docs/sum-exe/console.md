---
title: Console commands
parent: Sum.exe
nav_order: 6
---

# Console commands
{: .no_toc }

1. TOC
{:toc}

**Summary:** Sum.exe registers **149 developer console commands** through static C++
objects, each built by one constructor, `console_command::console_command` (`0x50a180`).
The retail build has **no way to open the console**: `print_to_console` is empty and nothing
in the game calls the command-line entry point. The commands run only when a patch calls
`execute_command_line` (`0x50a210`) itself, as Timeslip's patch and SummonerFix's
`[ConsoleKeys]` do. Every command's callback and object are in `symbols/sum.symbols`
(`cmd_<name>`, `ccmd_<name>`), except the four noted below (**observed**: 149 calls to the
constructor, all from `init_ccmd_*` initialisers).

## How commands are registered

Each command is a global `console_command` object built by a small static initialiser that
the C runtime runs at startup. Example for `show_framerate` (`0x4323f0`):

```nasm
push 0x432410        ; callback  -> cmd_show_framerate
push 0x5931d0        ; "Toggles Gamewide_show_framerate"
push 0x5931c0        ; "show_framerate"
mov  ecx, 0x5fbfe8   ; the console_command object -> ccmd_show_framerate
call 0x50a180        ; console_command::console_command(name, description, callback)
ret
```

All initialisers are named `init_ccmd_<command>` in the symbol file (2026-10-14). Ghidra does not
make them into functions, because only the C runtime's initialiser table points at them. Some are
built without a stack frame, as above; others start with `push ebp; mov ebp, esp`.

The constructor (`FUN_0050a180`, **observed** `0x50a180`..`0x50a20e`) stores `name`, `description`
and `callback` at `+0x0`, `+0x4` and `+0x8`. It inserts the object into `Console_commands`
(`0x25db3e4`, 300 pointers), kept sorted by `_stricmp`. When the table is full, or a command with
the same name (ignoring case) is already registered, it returns at once: the object stays out of
the table and its fields stay zero. The 149 retail names are all different, so none is dropped.

`node tools/console-commands.mjs` finds the initialisers by their byte pattern and reports all 149 commands.
Four of them (`antialiasing`, `lmap_off`, `lmap_reset`, `whee`) push a description pointer into `.bss`
(`0x25d9230`, `0x5bd0d0`, `0x2f132d0`): the empty-string literal that VC6 puts there. The tool skipped
them until 2026-10-15; their objects are now in the symbol file. **Observed**: `node tools/asm.mjs grep "call 0x50a180"`
lists 149 call sites.

{: .note }
The four have no description, and their callbacks do not test the mode flags, so they also run
for `? <command>` and `help <command>`. `lmap_off` / `lmap_reset` set / clear flag `0x80000` on
every level object (`0x46e150`, `0x46e0d0`). `whee` stores the parsed float in `0x5ad544`, which
nothing reads (in the Glide unit; initial value 0.1).

## Command type (argument checking)

The PC constructor takes three arguments. Summoner 2's takes four (name, description, type,
callback; see [Engine references](../reference/engine-references.md)). On PC there is no stored
type: each callback states the argument types it accepts when it parses its argument
(**observed**). It calls `process_command(mask)` (`FUN_0050a5b0`), which reads the next token,
sets `arg_type` (`0x25dae04`) and compares it with the mask:

| Bit | Meaning | Set when the token is |
|---|---|---|
| `0x001` | none | the end of the line |
| `0x002` | string | any word or quoted string (quotes removed) |
| `0x004` | quoted | a quoted string (only inside `process_command`; it becomes `0x002`) |
| `0x008` | int | all digit-class characters and no `.` (`atoi`), or `0x...` read in full by `strtol` |
| `0x010` | float | all digit-class characters (`atof`) |
| `0x020` | hex | `0x...` read in full by `strtol` |
| `0x040` | true | `on`, `true`, or an int other than 0 |
| `0x080` | false | `off`, `false`, or the int 0 |
| `0x100` / `0x200` / `0x400` | `+` / `-` / `,` | that one character |

The digit class is `0-9`, `+`, `-` and `.` (`init_char_types`, `0x50ab60`), so `-5` and `1.5`
are numbers, but so is a lone `-` (int 0, and so also false). Letters, `_`, `:`, `\` and `$`
form words; `"` starts a quoted string; any other character is a one-character token. When no
bit of the mask matches, the command is abandoned: `process_command` reports "Error: Not enough
parameters." or "Error: '%s' invalid type" and `longjmp`s back into `execute_command_line`, which
returns -1.

```nasm
50a93e: mov eax, [esp+0x8]       ; the mask
50a942: test eax, ecx            ; ecx = arg_type
50a944: jne 0x50a98f             ; accepted
...
50a983: push 1
50a985: push 0x25db29c           ; process_command's jmp_buf
50a98a: call 0x56f20c            ; longjmp
```

Examples: the toggles pass `0xc1` (true, false or nothing), `add_gold` passes `0x08`, `script`
passes `0x02`, `whee` passes `0x10`.

A toggle callback (`show_framerate`, `0x432410`): bit `0x40` sets the variable to 1, `0x80`
sets it to 0, `0x01` (no argument) flips it. With `doing_help_for_comand` set it prints its
usage, with `checking_status_for_command` set it prints `"<command> is TRUE/FALSE"`.

## Running a command line

`execute_command_line` (`FUN_0050a210`) takes a line of 1 to 254 characters (an empty or longer
line returns -1), copies it and reads the first word (**observed**):

| Line | Mode flags the callback sees |
|---|---|
| `<command> [args]` | `calling_a_command_function` = 1 and `checking_status_for_command` = 1 |
| `? <command>` | only `checking_status_for_command` |
| `help <command>`, `man <command>`, or a `?` anywhere after the command name | only `doing_help_for_comand` |
| `debug <command> ...` | as above, plus a trace of the parse (`debug_command_prefixed`) |

The first word must be a string token, or the line fails with "Invalid keyword". The name is
looked up case-insensitively. After a normal call, leftover arguments give "Ignoring the unused
command line tail". Callbacks without their own `calling_a_command_function` test (the four
above, `cmd_summon`) run in every mode.

The tokenizer copies each word into 128-byte buffers (`token` `0x25db894`, `lowercase_token`
`0x25db21c`) without a length check, so a word of 128 or more characters overwrites the
variables after them (see [Bugs](game-bugs.md)).

## No console in the retail build

Every message the console code formats goes to `print_to_console` (`0x50a5a0`), which is a bare
`ret`. `console_set_font` (`0x50af50`) stores a font in `0x5a6f28` that nothing reads.
`execute_command_line` has one caller, `load_script` (`0x50acd0`), whose only caller is
`cmd_script`, itself a command. No code or data holds the address `0x50a210` (byte scan of the
image). So in v1.40 the commands cannot be typed in, and all their output, including "Unknown
command" and the argument errors, is lost (**observed**).

## Console scripts (`.vcs`)

`script <name>` runs a console script (`load_script`, `FUN_0050acd0`, **observed**):

- The path is the install directory (`install_path`, the registry `InstallPath`), `\`, the
  lower-cased name cut at its first `.`, then `.vcs`. The cut uses `strchr` on the whole path, so a
  `.` anywhere in the install directory breaks every script (see [Bugs](game-bugs.md)).
- Each line is trimmed, and lines starting with `//` are skipped. A line containing `/*` starts a
  comment that lasts up to the next line containing `*/`, even when both are on the same line.
- Every other non-empty line goes to `execute_command_line`. The script goes on after a failed
  line, and the line number in "Error processing script line %d" counts only empty and executed
  lines.

No `.vcs` file ships with the game.

`print_to_file` (`0x50a030`) writes `"<name> - <description>"` for every registered command to
`console_commands.txt`, not `console.txt` as its description says. The `console_commands.txt` that
ships with Timeslip's patch has exactly these lines for the 149 commands of v1.40, including
`antialiasing - `, `lmap_off - `, `lmap_reset - ` and `whee - `
(see [Timeslip's patch](../patches/timeslip.md)).

## Variables named from command descriptions

64 commands are described as `Toggles <Variable>` or `Sets <Variable>`, which gives the
original name of the global they control. `node tools/console-vars.mjs` scans each callback
for absolute memory operands into `.data`/`.bss`. It drops addresses shared by more than
three callbacks (output buffers, console state) and keeps a variable when exactly one
candidate remains: 63 of 64 resolve this way. They are in `symbols/sum.symbols` with
confidence **I**. The scan is byte-based rather than a full instruction decoder, and the
types (`bool` for toggles, `int` for setters) are guesses.

Check case: `show_framerate` resolves to `0x5fc01f`, which is already known from
`FrameEnd`. The one ambiguous result is that same command, where the scanner also hit a
misaligned match. Its variable is named `Gamewide_show_framerate` from its own description.

## Using the commands

The descriptions name many internal variables ("Toggles Players_immortal", "Sets Gr_far_min"),
which makes this table a map of debug features. Two patches give a way in:

- **Timeslip's patch** binds commands to keys (`[console] k070=show_framerate`). Its `dinput.dll`
  contains the address `0x50a210` (*inferred*: it calls `execute_command_line`).
- **SummonerFix** `[ConsoleKeys]` (`src/summonerfix/keys.c`, **observed** in the source):
  `Key=command` lines (for example `F9=add_gold 1000`) are read into a 256-entry table by scan
  code, each command cut to 95 characters. SummonerFix's window-message handler, registered with
  the game's `os_add_msg_handler` (`0x530c20`), runs on the game thread. On a key press (not an
  auto-repeat) it calls `execute_command_line` with a copy of the command. The key still reaches
  the game, and the section is reloaded live when the ini changes. The output stays invisible, so
  a command shows only through its effect. A binding such as `F10=script test` runs a `.vcs` file. Keys are
  ignored until the first game state (a command run during start-up crashed the game, 2026-10-16).
- **`play_and_load_cutscene`** is a cutscene viewer: it loads the cutscene's level, and `game_flags` bit `0x8`
  returns to the main menu when the cutscene ends ([Cutscenes](cutscenes.md#console-cutscenes-and-pending-playback)).
  Clearing that bit while the cutscene plays keeps the game in the level, which is how agent-mode tests reach story
  levels ([Tools](../tools.md)). Starting a new game from the menu it returns to crashed once (game-bugs C280).

The planned [custom key bindings](../roadmap.md#custom-key-bindings-for-player-actions) can use the
same route.

Callbacks are reached only through the object's function pointer, so Ghidra's
auto-analysis didn't create functions for most of them: 136 of the 145 found by the tool were
missing from `symbols/ghidra-functions.tsv`.

## All commands

| Command | Callback | Object | Description |
|---|---|---|---|
| `add_gold` | `0x46c570` | `0x970810` | Adds gold to party inventory |
| `add_xp` | `0x4379e0` | `0x609f90` | Adds experience points to currently selected character |
| `ai_frame_update_rate` | `0x4720e0` | `0x22e4990` | Set the rate at which AI is updated in frames for offscreen creatures |
| `ai_state` | `0x474d10` | `0x22e48c0` | Change the AI state of the study object |
| `ai_wander` | `0x474f00` | `0x22e48d0` | Toggles Ai_path_wandering_enabled |
| `all_multi_levels` | `0x43ecc0` | `0x60ae58` | Toggles All_multi_levels |
| `All_summons` | `0x444a50` | `0x68e200` | Toggles All_summons |
| `alpha_fog` | `0x505c00` | `0x25d9210` | Sets Gr_alpha_fog_ratio |
| `antialiasing` | `0x505db0` | `0x25d8e00` | (none) |
| `attack_point` | `0x473260` | `0x22e4930` | attack point |
| `avoidance` | `0x4a6810` | `0x246b918` | Sets avoidance area parameters |
| `bloodspots` | `0x463390` | `0x941730` | Toggles Bloodspots |
| `chance_gold` | `0x463080` | `0x8eca30` | Sets Chance_to_drop_gold |
| `change_texture_res` | `0x501c90` | `0x25c5c80` | Toggle the texture resolution between high and low resolutions. |
| `character` | `0x439930` | `0x60a420` | Set the current main character |
| `charinfo` | `0x4f13e0` | `0x65e968` | dump info on all characters in the game |
| `Cull_ground` | `0x481ea0` | `0x22ec4c8` | Toggles Cull_ground |
| `Cull_ground_too` | `0x481f90` | `0x22f9dd0` | Toggles Cull_ground_too |
| `darkjo_combat` | `0x4c34e0` | `0x248b980` | Sets dark josesph in combat state |
| `dcamera` | `0x4db1f0` | `0x2546750` | Change the dialog camera options. |
| `death` | `0x463480` | `0x941710` | Toggles Debug_death |
| `dialogue_make_topic_available` | `0x4db010` | `0x2546740` | mark a specific dialogue topic as available. |
| `dialogue_make_topic_unavailable` | `0x4db100` | `0x2546730` | mark a specific dialogue topic as unavailable. |
| `disp_rate` | `0x442380` | `0x65e0c0` | Toggle whether net rate info for a packet is displayed |
| `do_frame_callback` | `0x4ca880` | `0x2490c68` | Toggles Do_frame_callback |
| `dont_load_fx` | `0x4ca790` | `0x2490c80` | Toggles Dont_load_fx |
| `door_close` | `0x4d6580` | `0x253ef30` | closes a specified door in the level. |
| `door_open` | `0x4d5e70` | `0x25388b0` | open a specified door in the level. |
| `door_toggle` | `0x4d6600` | `0x24ed5f0` | opens or closes a specified door in the level. |
| `dump_mesh_frames` | `0x562a60` | `0x2f91a60` | Toggles Dump_mesh_frames |
| `dump_sound_usage` | `0x4378f0` | `0x609fb0` | Toggles Dump_sound_usage |
| `eax_save_props` | `0x4ca970` | `0x24902b8` | Saves the EAX properties to disk |
| `far_max` | `0x50db80` | `0x2692d28` | Sets Gr_far_max |
| `far_min` | `0x50dac0` | `0x2692880` | Sets Gr_far_min |
| `flece` | `0x439d80` | `0x60a410` | Add Flece to the game |
| `full_party` | `0x437800` | `0x609f70` | Toggles Full_party |
| `gamestage` | `0x4d4bf0` | `0x24dfb30` | Set the important event flags to different stages of the game. |
| `gamma` | `0x43d520` | `0x60abe8` | Sets Summoner_options.gamma_correction |
| `geom_avoid` | `0x477990` | `0x22e4b00` | Toggles Camera_geometry_avoidance |
| `ghost_combat` | `0x4b9fc0` | `0x2480300` | Puts ghost rider in combat state |
| `ghost_solo` | `0x4ba050` | `0x2480938` | Set ghost without horse |
| `ghost_stage` | `0x4b9ea0` | `0x2480928` | Set Ghost rider's current combat stage |
| `ghost_unlink` | `0x4b9f00` | `0x2480910` | Unlinks ghost from mount |
| `Give_all_items` | `0x438070` | `0x609f60` | Toggles Give_all_items |
| `gr_info` | `0x433280` | `0x5fbf30` | Toggles Gamewide_show_gr_info |
| `gr_show_stats` | `0x505cc0` | `0x25d8de0` | Toggles Gr_show_stats |
| `gs_stage` | `0x4c80a0` | `0x248c7c0` | Set Giant salamanka's current combat stage |
| `gsal_combat` | `0x4c8100` | `0x248c7b0` | Sets gsal in combat state |
| `health` | `0x4619b0` | `0x941508` | Sets the percentage health for the debug object |
| `high_lod` | `0x51b5f0` | `0x2710ec0` | Toggles High_lod |
| `Ignore_tutorial` | `0x42e910` | `0x5f9610` | Toggles Ignore_tutorial |
| `immortal` | `0x461890` | `0x8ed970` | Toggles Players_immortal |
| `iw` | `0x4fd5f0` | `0x257ed50` | reinits worldmap |
| `jekhar` | `0x439dd0` | `0x60a638` | Add Jekhar to the game |
| `kill_summon` | `0x463140` | `0x8eca50` | kills off friendly summoned creature |
| `level_fog` | `0x505b40` | `0x25d8cf0` | Sets Gr_level_fog_multiplier |
| `levelchars` | `0x449c00` | `0x8b3c38` | dump info on all characters in the level |
| `lmap_off` | `0x46e140` | `0x22e4408` | (none) |
| `lmap_reset` | `0x46e0c0` | `0x22e4398` | (none) |
| `load_lightmaps` | `0x46dee0` | `0x22c4388` | Toggles S3d_load_lightmaps |
| `load_mipmaps` | `0x501b50` | `0x25c3048` | Toggles Load_mipmaps |
| `load_mlayout` | `0x4401a0` | `0x60f928` | Load a monster layout file |
| `loadgame` | `0x4d4ea0` | `0x2493a18` | Load a previously saved game |
| `lod_dist` | `0x481710` | `0x22ec4d8` | Sets Character_lod_dist |
| `machival_combat` | `0x4c0cf0` | `0x248b010` | Sets machival in combat state |
| `main_lod_dist` | `0x4817d0` | `0x22f8d78` | Sets Main_character_lod_dist |
| `map_grid` | `0x4045c0` | `0x5b2a00` | Toggles Map_grid |
| `map_mask` | `0x4044d0` | `0x5b2da8` | Toggles Map_mask |
| `max_skills` | `0x433430` | `0x5fc1b8` | Toggles Max_skills |
| `mlook_sensitivity` | `0x477a80` | `0x22e4ae0` | Sets the sensitivity value for mouse look. |
| `movement_vel_change` | `0x531e10` | `0x2cbff78` | Toggles Allow_movement_vel_change |
| `Multi_path_type` | `0x4811d0` | `0x22e94e8` | Sets the method used to generate paths on multiplayer machiens |
| `net_stats` | `0x433190` | `0x5fc008` | Toggles Gamewide_show_net_stats |
| `No_cutscenes` | `0x4cce40` | `0x2490c90` | Toggles No_cutscenes |
| `no_resist` | `0x4631b0` | `0x940428` | Toggles No_resist |
| `obstacle_height_offset` | `0x4a3e10` | `0x245cd90` | Set the height level to draw obstacles relative to main player pos |
| `offense_point` | `0x473300` | `0x22e47d0` | offense point |
| `p3d_files` | `0x46dfd0` | `0x22e43a8` | Toggles P3D_files |
| `party_controller_check` | `0x493d90` | `0x243e388` | Toggles Party_controller_check |
| `pause_ai` | `0x474e10` | `0x22e48b0` | Toggles Ai_paused |
| `phoenix_combat` | `0x497ca0` | `0x2452a48` | Sets phoenix rider in combat state |
| `phoenix_solo` | `0x497c50` | `0x2452a38` | Set phoenix without horse |
| `phoenix_stage` | `0x497af0` | `0x24529f8` | Set phoenix rider's current combat stage |
| `phoenix_unlink` | `0x497b50` | `0x2452a58` | Unlinks phoenix to mount |
| `play_and_load_cutscene` | `0x437590` | `0x609f10` | Play a cutscene and loads the level associated with it |
| `play_cutscene` | `0x4374d0` | `0x6034b8` | Play a cutscene |
| `play_sound` | `0x43d5e0` | `0x60ac20` | Streams a wav file from disk |
| `precalc_vis_method` | `0x49c870` | `0x2452d40` | Toggles Precalc_vis_method |
| `Preparse_scripts` | `0x4cef30` | `0x2493a28` | Toggles Preparse_scripts |
| `print_to_file` | `0x50a030` | `0x25dadf8` | Print out console commands to the text file console.txt |
| `pyrul_combat` | `0x4b88c0` | `0x247fd18` | Sets pyrul in combat state |
| `pyrul_stage` | `0x4b8850` | `0x247fcf0` | Set Pyrul's current combat stage |
| `Quick_combo` | `0x449590` | `0x65e998` | Toggles Quick_combo |
| `random_encounters` | `0x4fd630` | `0x257ed10` | Toggles Random_encounters_enabled |
| `remove_nav` | `0x4e0070` | `0x254a8a0` | Remove redundant $npc navpoints |
| `render_3d_obstacles` | `0x439840` | `0x60a648` | Toggles Render_3d_obstacles |
| `render_characters` | `0x481980` | `0x22faed0` | Toggles Render_characters |
| `render_effects` | `0x481a70` | `0x22f8d00` | Toggles Render_effects |
| `render_level` | `0x481890` | `0x22f8cf0` | Toggles Render_level |
| `render_s3d_navpoints` | `0x4ecf90` | `0x2549d30` | Toggles Render_s3d_navpoints |
| `reset_map_labels` | `0x4047a0` | `0x5b2ca8` | Reset automap labels for this level/script |
| `rosalind` | `0x439e20` | `0x60a658` | Add Rosalind to the game |
| `save_map_labels` | `0x430430` | `0x5f9e38` | Save automap labels for this level/script |
| `save_mlayout` | `0x4402f0` | `0x60f4f0` | Save a monster layout file |
| `savegame` | `0x4d4c80` | `0x2494b20` | Save the game to a file |
| `script` | `0x50ac60` | `0x25dae08` | Runs a console script file (.vcs) |
| `serpent_combat` | `0x4954a0` | `0x2452408` | Sets serpent rider in combat state |
| `serpent_solo` | `0x495450` | `0x24523d8` | Set serpent without horse |
| `serpent_stage` | `0x495340` | `0x24523e8` | Set Serpent rider's current combat stage |
| `serpent_unlink` | `0x4953a0` | `0x24523f8` | Unlinks serpent to mount |
| `set_fog` | `0x481b60` | `0x22f9d80` | Set the fogging clip plane. |
| `show_coordinates` | `0x4046b0` | `0x5b2b60` | Toggles Show_coordinates |
| `show_effect_mem` | `0x51bd90` | `0x2c90a60` | Shows how much memory the effects system is using. |
| `show_framerate` | `0x432410` | `0x5fbfe8` | Toggles Gamewide_show_framerate |
| `show_hud_info` | `0x432fb0` | `0x5fbf20` | Toggles Gamewide_show_hud_info |
| `show_ui` | `0x414cb0` | `0x5bd780` | Toggles the visibility state of the HUD |
| `slew` | `0x4343d0` | `0x5fcca8` | Toggles Slew Mode |
| `slowdown_scaler` | `0x472190` | `0x22e47e0` | Set the slowdown scaler |
| `small_mesh_threshold` | `0x48ddd0` | `0x23f81d8` | Sets Small_mesh_threshold |
| `snow` | `0x4950e0` | `0x2451d50` | Toggles Snow_enabled |
| `snow_render` | `0x4951d0` | `0x24445a0` | Toggles Snow_render |
| `space_mult` | `0x48dd10` | `0x2432478` | Sets Space_mult |
| `Spell_ap` | `0x4632a0` | `0x8eca40` | Toggles Use_spell_ap |
| `stage` | `0x4c8d90` | `0x248ce30` | Set Luminar's current combat stage |
| `stop_cutscene` | `0x437650` | `0x609f20` | Stop the current cutscene |
| `stop_sound` | `0x43d780` | `0x60abd0` | Stops all sounds that are playing |
| `summon` | `0x47f730` | `0x22e9240` | Summon a creature |
| `Super_power` | `0x464040` | `0x941720` | Toggles Super_power |
| `sysinfo` | `0x531aa8` | `0x2cbff00` | Shows system info |
| `targa` | `0x4330a0` | `0x5fbff8` | Toggles Print_screen_as_targa |
| `tentacle_test` | `0x4c7e90` | `0x248c778` | Makes tentacles go in and out of water quickly -- no attacks |
| `tentacles` | `0x4c7cc0` | `0x248c788` | Start battle with tentacle beasts |
| `test_meshes_on_load` | `0x46ddf0` | `0x22e43e8` | Toggles Test_meshes_on_load |
| `tiger_combat` | `0x499ff0` | `0x2452c98` | Sets tiger in combat state |
| `tiger_solo` | `0x49a080` | `0x2452a70` | Set tiger without horse |
| `tiger_stage` | `0x499ec0` | `0x2452a80` | Set Tiger rider's current combat stage |
| `tiger_unlink` | `0x499f30` | `0x2452ca8` | Unlinks tiger to mount |
| `time_mult` | `0x48dc50` | `0x23fc838` | Sets Time_mult |
| `titus_combat` | `0x4c21f0` | `0x248b958` | Sets titus in combat state |
| `titus_stage` | `0x4c2180` | `0x248b968` | Set Titus's current combat stage |
| `Turn_anims` | `0x4b1440` | `0x247e890` | Toggles Use_turn_anims |
| `turn_scaler` | `0x4b1530` | `0x247e880` | Set the turn scaler |
| `urath_combat` | `0x4c41e0` | `0x248bf28` | Sets urath in combat state |
| `us_open_demo` | `0x431ba0` | `0x5fade0` | Toggles US_open_demo |
| `use_buffered_alpha_polys` | `0x53e6a0` | `0x2dc7060` | Toggles Use_buffered_alpha_polys |
| `whee` | `0x54b960` | `0x2f0f1e8` | (none) |
| `zmax` | `0x50dd30` | `0x2692e18` | Sets Gr_3d_zmax |
| `zmult` | `0x50dc70` | `0x2682a90` | Sets Gr_3d_zmult |
| `zoom_limit` | `0x4778e0` | `0x22e4af0` | Toggle camera zoom limiting. |

The objects of the four rows without a description are not yet named in `symbols/sum.symbols`;
the `lmap_*` callbacks are named `ccmd_lmap_*_handler` there.

## Evidence

Function and instruction addresses (no REA evidence ids; listings from `tools/asm.mjs`).

| Claim | Where |
|---|---|
| 149 registrations | `node tools/asm.mjs grep "call 0x50a180"`: 149 sites, all in `init_ccmd_*`; `init_ccmd_antialiasing` `0x505d90`, `init_ccmd_lmap_reset` `0x46e0a0`, `init_ccmd_lmap_off` `0x46e120`, `init_ccmd_whee` `0x54b940` |
| constructor, sorted insert, silent drop | `console_command__console_command` `0x50a180`..`0x50a20e` (`cmp eax, 0x12c` at `0x50a188`, `je 0x50a1df` at `0x50a1b1`) |
| argument types and the `longjmp` | `process_command` `0x50a661`..`0x50a98a` |
| character classes | `init_char_types` `0x50ab70`..`0x50ac22`; `get_token` jump table `0x50aa18` |
| modes, line length | `execute_command_line` `0x50a22b`..`0x50a244`, `0x50a2a3`..`0x50a389`, `0x50a435`..`0x50a4fd` |
| no entry point | callers of `0x50a210`: only `0x50ae7c` (`load_script`); of `0x50acd0`: only `0x50ac8b` (`cmd_script`); no dword `0x50a210` in the image; `print_to_console` `0x50a5a0` is `ret` |
| `.vcs` path and comments | `load_script` `0x50ace4`..`0x50ad79`, `0x50ae09`..`0x50aeac` |
| `console_commands.txt` | `cmd_print_to_file` `0x50a06e`, `0x50a0c8`..`0x50a11e`; Timeslip's `console_commands.txt` (149 commands) |
| unbounded tokens | `read_word` `0x50aacd`..`0x50aadf`; `make_lowercase_token` `0x50aa52`..`0x50aa75` |
| SummonerFix `[ConsoleKeys]` | `src/summonerfix/keys.c`: `LoadConsoleKeys`, `RunConsoleKey`, `MsgHandler` |

## Changes (2026-10-15, console.cpp)

{: .important }
The page said 145 commands; there are 149 (four have no description, so the tool skipped them).
The command type is not "not yet analysed": each callback passes the argument types it accepts
to `process_command`. `print_to_file` writes `console_commands.txt`, not `console.txt`. The
page did not say that nothing in v1.40 opens the console.

- Added the argument table, the modes of `execute_command_line`, [No console in the retail
  build](#no-console-in-the-retail-build), [Console scripts](#console-scripts-vcs) and how
  SummonerFix's `[ConsoleKeys]` runs commands. Checked while decompiling
  `src/decomp/vsdk/console.cpp`.
