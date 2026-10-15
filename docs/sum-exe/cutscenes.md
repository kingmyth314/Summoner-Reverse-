---
title: Cutscenes
parent: Sum.exe
nav_order: 6.33
---

# Cutscenes
{: .no_toc }

Every story cutscene in Summoner runs in the game engine, not as a movie. A cutscene is a
`cutscene.tbl` entry. It has camera files (`.csc`), effects, a streamed soundtrack, fades, fog
keys, and an optional `<name>_cscript.tbl` script that moves the actors. `cutscene_play`
(`0x4361d0`) saves and hides the gameplay state and sets `game_flags` bit `0x20`.
`cutscene_sequence_update` (`0x436a90`) then steps through the camera shots on wall-clock time,
and `cutscene_cleanup` (`0x436620`) puts everything back. Esc skips a cutscene unless it has
`+NoAbort`. **Observed** (code and the retail tables), not yet tested in agent mode.

1. TOC
{:toc}

## Cutscenes and Bink movies

The game uses Bink only for the logo movies and the credits. `movie_play` (`0x5172f0`) has three
callers: `mainmenu_init` (`vlogo.bik`, `eaxlogo.bik`) and `credits_frame`. `data\movies` holds only
`vlogo.bik`, `eaxlogo.bik` and `geeks.bik`. **Observed.** The cutscene code never calls
`movie_play`. **Observed.** So the intro story and all boss scenes are in-engine. SummonerFix
`SkipIntroMovies` skips only the logo movies, see [Video settings](video-settings.md).

## The cutscene table

`GameInit` calls `cutscene_parse_table` (`0x4355e0`) once. It reads `cutscene.tbl` (in
`tables.vpp`) into `cutscene_entries` (`0x6034c8`): up to 85 records of `0x140` bytes, count in
`cutscene_num_entries` (`0x609f30`). Strings go into `cutscene_alloc_pool` (`0x5fccc0`), a
non-releasing pool over a `0x4c00`-byte buffer. **Observed.** The retail table has 84 entries.
**Observed.**

| Token | Entry field | Meaning |
|---|---|---|
| `$Cutscene: "name"` | `+0x0` | name used by `cutscene_play` |
| `+NoAbort` | `+0x4` bit 0 | Esc cannot skip it (9 retail entries: the Escape and Dismount scenes, Flece-Intro-p2) |
| `+HideUnscripted` | `+0x4` bit 1 | characters without a cutscene script entry are not drawn |
| `$Level: "level"` | `+0x8` | level the entry belongs to; `"any"` matches every level |
| `$Script: "name"` | `+0xc` | level script for `play_and_load_cutscene`; defaults to `$Level` |
| `$Camera: "file.csc"` (repeat, max 4) | `+0x10` count, `+0x14` names | camera files, played in order |
| `$Animation: "fx.vfx" [shot [length]]` | see below | an effect |
| `$Character Animation: "char" "fx.vfx" [shot [length]]` | see below | an effect attached to a named character |
| `$Soundtrack: "file.wav"` | `+0x5c` | streamed sound started at shot 0 |
| `+Minitrack: "file.wav" shot` | `+0x60` count, `+0x64` | sound played at a shot (no retail entry uses it) |
| `$Fog: near far shot` | `+0x74` count, `+0x78` | fog distances from that shot on |
| `$VisSet: shot "$vis…"` | `+0xb8` count | forced visibility set (`cutscene_parse_visset`, `0x435ff0`) |
| `$Fade In:` / `$Fade Out: time length shot` | `+0xfc` count, `+0x100` | screen fade, `time` seconds after the shot starts |

All rows **observed** from `cutscene_parse_table`. An effect line with no numbers goes into the
list at `+0x24`/`+0x28`. `cutscene_bind_actors` (`0x434e20`) starts those at load time. An effect
with a shot number goes into the list at `+0x30`/`+0x34` (16-byte records: name, shot, character,
length). **Observed.**

`cutscene_play` picks the first entry whose name matches and whose level is the current
`level_filename` or `"any"`. **Observed.** The four `"any"` entries are the dragon summons, named
after the skills ("summon jade dragon" and so on). `summon_create` plays them through
`skill_names`. **Observed.** A third test matches any entry whose `$Level` is `"Placeholder"`,
whatever the name. No retail entry uses it. **Observed.**

## How a cutscene starts

| Caller | Path |
|---|---|
| Level scripts | `script_play_cutscene` (`0x4ccf00`), 77 call sites |
| Console | `play_cutscene` (`0x4374d0`), `play_and_load_cutscene` (`0x437590`), `stop_cutscene` (`0x437650`) |
| Dragon summons | `summon_create` (`0x47ef57`) |
| Pending cutscene after a level load | `cutscene_play_escape` (`0x437130`) from `state_game_enter` |

`script_play_cutscene(name, continuation)` checks three things. At least one party member must
be alive. The console toggle `No_cutscenes` (`0x2493a16`) must be off. The game must not be
multiplayer (`game_flags` bit 2). **Observed.** If cutscenes are off or it is multiplayer,
it calls the continuation at once. If no party member is alive it does nothing. **Observed.**
Otherwise it saves `level_script_frame_fn` (`0x24902ac`) in `cutscene_saved_frame_fn`
(`0x2493a04`). It installs `script_cutscene_done` (`0x4ccdf0`) as the frame function, stores the
continuation in `cutscene_done_callback` (`0x24902b0`) and calls `cutscene_play`:

```nasm
4ccf63: mov dword ptr [0x2493a04], ecx    ; cutscene_saved_frame_fn = level_script_frame_fn
4ccf69: mov dword ptr [0x24902ac], 0x4ccdf0  ; level_script_frame_fn = script_cutscene_done
4ccf73: mov dword ptr [0x24902b0], edx    ; cutscene_done_callback = continuation
4ccf79: call 0x4361d0                     ; cutscene_play
```

`level_script_frame` still runs every frame of the cutscene. `script_cutscene_done` returns while
bit `0x20` is set. Then it restores the old frame function and jumps to the continuation.
**Observed.** These continuations are the 53 `*_cs_end` functions in `symbols/sum.symbols`, for
example `ls_sewer_tb_intro_cs_end` (`0x4ee2d0`). See [Level scripts](level-scripts.md) and
[Bosses](bosses.md). If the name is not in the table, `cutscene_play` returns without setting bit
`0x20`, so the continuation runs on the next frame. *Inferred.*

## What `cutscene_play` changes

In order (**observed**):

1. If a cutscene is already running (bit `0x20`), `cutscene_cleanup` ends it. Otherwise
   `cutscene_begin_loading(6)` (`0x4149d0`) clears the screen for a 6-step loading bar.
2. `summon_stash_for_cutscene` (`0x448000`) takes the current summon out of the world and saves it
   in `stashed_summon` (`0x8b3c34`). A summon with `Character +0x320` bit `0x800` stays.
   `summon_create` sets that bit before a dragon cutscene.
3. `fx_destroy_spell_effects` removes spell effects. `fx_pause_for_cutscene` hides every effect
   that is not marked as a cutscene effect (`effect_is_cutscene_base`).
4. `cutscene_start_special_cases` (`0x4365e0`): "serpent-dismount" sets a walk anim on the rider;
   "Nar-LeneleAct4" turns snow off.
5. `ui_input_lock`. Saves fog near/far (`0x609f1c`, `0x609f08`) and `Gr_level_fog_multiplier`
   (`0x601e60`), then sets the multiplier to 1.0.
6. `level_ambient_pause_all`. Saves the music track in `cutscene_saved_music` (`0x59399c`),
   `music_stop(200)` and `snd_stop_all`. See [Sound](sound.md).
7. `cutscene_load_data` (`0x4353e0`): data path, camera files, actors and effects, sounds. The
   loading bar advances after each step.
8. `camera_set_sequence_transform(NULL)` on `game_camera`. This clears the shot transform a summon cutscene may have set ([Camera](camera.md#taking-the-camera-over)).
9. `level_vis_force_begin` when the entry has `$VisSet` lines. `cutscene_script_load(name)`.
10. For every living character: keep its AI goals, clear its status effects, send it idle when
    needed, and save four flags in `cutscene_saved_actors` (`0x5fcce8`). The flags are
    `+0x320` bits `0x2`, `0x20`, `0x2000000` (armour alt texture) and `+0x84` bit 0.
11. Clear the hover tints. `gr_set_cfg_34_flag(1)`. `frame_show_loading_icon = 1`. Set
    `game_flags |= 0x20 | 0x8000`.

Bit `0x8000` makes `FrameEnd` skip one `gr_flip` and then clears itself, so one frame is never
shown. **Observed.** Bit `0x20` means "cutscene running". It gates `cutscene_sequence_update`,
the Esc check, `HideUnscripted` drawing, and `cutscene_script_is_active`. The AI checks that
function. **Observed.** This is the unexplained bit `0x20` on the [Bosses](bosses.md) page.

{: .warning }
If `cutscene_load_data` fails (for example a missing `.csc`), `cutscene_play` returns after steps
2 to 6 without setting bit `0x20`. `cutscene_cleanup` returns at once when bit `0x20` is clear. So
input stays locked, the summon stays stashed and the music stays off. **Observed** code path; the
in-game effect is *inferred*.

The actor table has 175 slots (it ends at the pool buffer `0x5fd260`). The loop has no bound
check. **Observed.** More than 175 living characters would overwrite the table pool. *Inferred.*

## Shots, timing and events

`cutscene_load_files` (`0x434b40`) opens each `$Camera` file from the cutscene path. It checks
the magic `0x52435343` ("CSCR") and a version of at least `0x20000`. It reads two floats and a
shot count, then each shot with `cutscene_camera_read_track` (`0x493fd0`). **Observed.** The
shots fill `cutscene_sequences` (`0x601e80`), 50 records of `0x70` bytes, with the count in
`cutscene_num_sequences` (`0x601e7c`). Record `+0xc` is the shot start and `+0x10` the shot end,
in seconds. Each shot starts where the previous one ended. **Observed.** So the
`0x601e90 + i*0x70` value in [Timing bugs](timing-bugs.md) is the end time of shot `i`.

`state_game_frame` calls `cutscene_sequence_update` (`0x436a90`) each frame while bit `0x20` is
set, and skips `automap_reveal_around_party`. **Observed.** It does nothing while paused. It works
out `t = (timeGetTime - sequence_start_time) / 1000` (`0x601e78`). The difference from the last
frame becomes `cutscene_frametime` (`0x257f094`), which drives AI and objects during the
cutscene. When `end[i] - cutscene_time_offset - t <= 0` it moves to the next shot. **Observed.**

- Shot 0 starts the clock, streams `$Soundtrack` (handle in `cutscene_stream_sound`,
  `0x5939a0`) and resets the frame delta.
- Every shot calls `camera_start_sequence` (`0x4792c0`) with its record.
  `cutscene_camera_sequence_sample` (`0x494310`) samples the position and rotation tracks
  (`anim_track__get_position` / `get_rotation`) from the same `timeGetTime` clock.
- After the last shot it calls `cutscene_cleanup`. If `game_flags` bit `0x8` is set it then
  calls `gameseq_set_state(1)`.

The clock is unsigned time put through a signed 64-bit load. That is the ~24.9-day uptime bug in
[Timing bugs](timing-bugs.md). **Observed** there.

`cutscene_events_frame` (`0x436c00`) runs after `level_script_frame` each cutscene frame
(**observed**):

| Event | Rule |
|---|---|
| Fades | at shot 0: instant black if the first fade is a Fade In, else instant clear. Then each `$Fade` fires once its shot is reached and `time` seconds have passed in it. Length is seconds (`gr_fade_out` / `gr_fade_in`). |
| Effects | at their shot: `fx_attach_to_character` for a character effect, `fx_create_at_camera` when the name contains "camera", else `fx_create_world`. Handles go in `cutscene_fx_handles` (`0x603470`). With a length, rendering stops at shot + length ("Stopping render of effect"). |
| Minitracks | `snd_play` at their shot |
| Fog | `cutscene_get_fog_distances` picks the `$Fog` key for the current shot |
| Visibility | `cutscene_apply_vis_overrides` for the current shot |
| Script | `cutscene_script_entries_frame` (`0x4ed680`) |

## Cutscene scripts

`cutscene_set_data_path` (`0x435320`) adds `data\cutscenes\<name>` (any `_cscript.tbl` suffix is
cut off) as a search path for `.csc .mvf .vfx`. Its id is kept in `cutscene_path_id` (`0x601e6c`).
**Observed.** In the retail files these sit flat in `cutscene.vpp` (94 `.csc`, 291 `.mvf`, 77
`.wav`, 119 `.tga`). **Observed.**

`cutscene_script_load` (`0x4ed0f0`) opens `<name>_cscript.tbl` from the scripts path. **Observed.**
66 of them ship in `tables.vpp`. Format (**observed**):

- `$Level:` must equal `level_filename`, or no script is loaded.
- `#Navpoints`, optional `$Num navpoints:`, then navpoints read by `script_parse_navpoint`
  (`$Name`, `$Type: "cutscene"`, `+Plane`, `$Position`, `$Orientation`). These are the `$cut`
  navpoints of the [Script editor](script-editor.md). The count is in `0x257e96c`.
- `#Objects`: actors read by `script_parse_objects`, with `$Name`, `+No create`,
  `$Start position` and `+Action:` lines, then `#End`.

Actions such as `"play .mvf"`, `"show/hide"`, `"synch" <shot>` and `"synch teleport" <shot>
"$nav"` drive the actors. `script_entry_jump_to_sequence` (`0x4ddb10`) looks for an action of
type `0x17` or `0x18` whose parameter equals the current shot (`0x603460`). For `0x18` it warps
the actor to the navpoint. **Observed.** That `0x17` is "synch" and `0x18` is "synch teleport" is
*inferred* from the files. `cutscene_time` (`0x257e970`) is the script clock. The editor shows it
as "Cutscene time".

## Ending, skipping and restore

`interface_frame` handles keys first. While bit `0x20` is set, key code 1 (Esc) calls
`cutscene_cleanup`, unless the entry has `+NoAbort`. Other keys are returned without UI
handling. **Observed.**

```nasm
4119cb: test al, 0x20                     ; cutscene running?
4119cf: cmp esi, 0x1                      ; key 1 = Esc
4119d8: mov eax, dword ptr [0x601e68]     ; cutscene_data (the table entry)
4119dd: test byte ptr [eax + 0x4], 0x1    ; +NoAbort
4119e7: call 0x436620                     ; cutscene_cleanup
```

A skip runs the same cleanup as a normal end, but does not do the bit-`0x8` state change. Later
shot events, effects and `synch` moves never run. The level script continuation still runs on
the next frame. *Inferred.* `stop_cutscene` also calls `cutscene_cleanup`.

`cutscene_cleanup` (`0x436620`) returns unless bit `0x20` is set and data is loaded. Then
(**observed**):

- Hides the loading icon. Releases cutscene sounds (`snd_level_release`). Fades out the
  soundtrack over 200 ms. Clears bit `0x20`.
- Restarts `cutscene_saved_music`. Sets bit `0x8000`. Calls `gr_set_cfg_34_flag(0)` and
  `cutscene_end_special_cases` (snow back on).
- `cutscene_script_unload` restores the actors' equipment and AI.
- For each character: speed scale, AI, and the four saved flags, matched by name hash. The first
  saved record with the same name wins. *Inferred* effect for characters that share a name.
- Unpauses ambient sounds. `camera_end_sequence(1)` pops the saved camera. Restores fog and the
  fog multiplier. Calls `level_vis_force_end`.
- Destroys the cutscene effects ("killing off %d effects") and calls `cutscene_unload`,
  `effects_release_cutscene_memory` and `frametime_restore_limits`.
- `ui_input_unlock` (skipped when bit `0x8` is set), `fx_resume_after_cutscene`,
  `summon_restore_after_cutscene`, `gr_fade_in(0)`, `levelrender_fade_reset`, and combat mode
  from the leader's `+0x320` bit 0.

## Console cutscenes and pending playback

`play_and_load_cutscene` calls `cutscene_change_level_for` (`0x437400`). It finds the entry,
calls `game_change_level($Level, $Script)`, sets `game_flags |= 0x18`, and stores the name
in `cutscene_pending_name` (`0x609f38`). **Observed.** When the level is entered,
`cutscene_play_escape` clears bit `0x10`. If a name is pending it locks input and plays it.
**Observed.** For "Tiger-Escape", "Serpent-Escape" and "PHNX-Escape" it first sets the boss stage
to 3 and takes the rider off its mount. **Observed.** Only this console path sets a pending name,
so those boss tweaks do not run in normal play. *Inferred.* Bit `0x8` then returns to state 1
after the cutscene and keeps input locked. Bit `0x10` is cleared again on entry; its other uses
are *unknown*.

## Open questions

- The two floats at the start of a `.csc` file and the exact setup of `cutscene_time_offset`
  (`0x601e70`).
- What `gr_set_cfg_34_flag` (`0x506160`) changes during a cutscene.
- What `cutscene_playing` (`0x609f34`) was for. It is set only when the entry is named
  "Placeholder", and nothing reads it. **Observed.**
- Whether the missing-file path above can happen with retail data.

## Evidence

These are function and instruction addresses read with `tools/asm.mjs`, not REA evidence IDs.
Table facts come from `cutscene.tbl` and `*_cscript.tbl`, listed with `tools/vpp.mjs` (not
committed).

| Claim | Where |
|---|---|
| Table format and flags | `cutscene_parse_table` `0x4355e0` (`0x435719` `or cl,1`, `0x435748` `or cl,2`) |
| Entry match, save order, flags `0x20`/`0x8000` | `cutscene_play` `0x4361d0`-`0x4365dc` |
| Load-failure return | `0x43634a`-`0x43634c` |
| Frame flip skip | `FrameEnd` `0x432d31`-`0x432d43` |
| Script hook and continuation | `script_play_cutscene` `0x4ccf00`, `script_cutscene_done` `0x4ccdf0` |
| Shot timing, end, bit `0x8` | `cutscene_sequence_update` `0x436a90` |
| Shot start/end chain, CSCR magic | `cutscene_load_files` `0x434c3a`, `0x434da5` |
| Events | `cutscene_events_frame` `0x436c00` |
| Esc skip | `interface_frame` `0x4119cb`-`0x4119e7` |
| Restore | `cutscene_cleanup` `0x436620`-`0x436a5f` |
| Data path | `cutscene_set_data_path` `0x435320` |
| Script file | `cutscene_script_load` `0x4ed0f0` |
| Pending cutscene | `cutscene_change_level_for` `0x437400`, `cutscene_play_escape` `0x437130` |
| Bink use | `movie_play` `0x5172f0` callers |
