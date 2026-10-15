# decomp52: the player input unit and the cutscene file

Two placeholder files (original names not known: no assert path, no strings.tbl block, sum.symbols module "-"):

| File | Range | Functions | Exact / likely / unsure |
|---|---|---|---|
| `game/unnamed_input_0x4343a0.cpp` | 0x4343a0-0x4348f6 (optimised) | 12 (incl. 2 initialiser helpers and the profile_label COMDAT) | 9 / 3 / 0 |
| `game/unnamed_cutscene_0x434900.cpp` | 0x434900-0x437659 (3 optimised objects, then `#pragma optimize("", off)`) | 42 (incl. 16 compiler-generated initialiser / atexit helpers) | 37 / 5 / 0 |

Total 54 functions, 46 exact / 8 likely / 0 unsure. Both files pass the mingw syntax check (the cutscene file
has three warnings that mirror the binary: two unused `.csc` floats and the `short` read with "%d"). All 118
existing `src/decomp` .cpp files (repo at db44615) compile against this header copy with exactly the
warnings they have against the repo headers.

The header copy is based on the repo at db44615 (decomp51 merged; re-based twice during the task: the first
copy was taken at 4224f69). `headers-vs-repo.patch` is the diff against db44615's src/decomp/include.
decomp51 already declares PollInputEvents and frame_show_loading_icon (summoner.h, frame-end unit), so they
are not added again.

## Cut points and evidence

- **0x4343a0** (start of the input unit): player.cpp ends with `ret` at 0x43439f (party count loop); 0x4343a0
  is `jmp init_ccmd_slew` (static_initializers entry 110). Agrees with the source map.
- **0x434900 (moved from 0x4349b0, D3201).** The source map gives the optimised initialisers 0x434900-0x4349a5
  (entries 111-113: cutscene object 0x601e68, 85 cutscene_entry 0x6034c8, NoRlsAllocPool 0x5fccc0) to the input
  unit. Their objects are inside the cutscene file's .bss and interleave with variables only the unoptimised
  code uses: 0x5fccc0 pool (optimised init) / 0x5fcce8 cutscene_saved_actors (cutscene_play / cleanup only) /
  0x5fd260 pool buffer / 0x601e60 / 0x601e68 object (optimised init) / 0x6034b0 / 0x6034b8 ccmd_play_cutscene
  (unoptimised init 0x4374b0) / 0x6034c8 entries (optimised init) / 0x609f08..0x609f38 (unoptimised users).
  One object's .bss is contiguous, so the three objects are the cutscene file's, and that object switches to
  `#pragma optimize("", off)` after them (precedent D3007, D2603). Padding follows the function (nop after
  0x4349a5, int3 from the first unoptimised body). Intrinsics stay on (repne scasb strlen, rep movs strcpy,
  inline strcmp), as in the other unoptimised files.
- **0x437659** (end): cmd_stop_cutscene's `ret`, then int3 padding; gameplay.cpp starts at 0x437660 (its .bss
  0x609f40, .data 0x593c34). Agrees.
- .data: input 0x593948-0x593999 (the 5 literals); cutscene 0x59399c (cutscene_saved_music = -1),
  0x5939a0 (cutscene_stream_sound = -1), literals 0x5939a4-0x593c31. Literals are allocated in reverse order
  of their code within a function, forward across functions (both files).
- .bss: input 0x5fcca8-0x5fccb8 (ccmd_slew, combat_mode, player_mouse_held, 0x5fccb7, 0x5fccb8; 0x5fccb5 is
  not referenced); cutscene 0x5fccc0-0x609f3b.
- Open: whether the input code and the cutscene file are ONE translation unit (adjacent in .text, .data and
  .bss; the input part and the three objects are both optimised). Written as two placeholder files.

## Functions

### game/unnamed_input_0x4343a0.cpp

| Address | Function | Confidence | Note |
|---|---|---|---|
| 0x4343a0 / 0x4343b0 | init_ccmd_slew_thunk / init_ccmd_slew | exact | compiler-generated (`console_command ccmd_slew(...)`) |
| 0x4343d0 | cmd_slew | likely | the two `not al; test al, 1` written as an inline `slew_mode_on()` (name ours) |
| 0x434440 | input_drain_events | exact | |
| 0x434470 | player_input_frame | exact | |
| 0x4344b0 | player_read_mouse_input | likely | profile_label text ours; the three zero stores may be an inline clear |
| 0x434650 | profile_label::~profile_label | exact | inline COMDAT emitted here, 91 callers (D3205); not written |
| 0x434660 | camera_process_input | likely | locals / expression order |
| 0x434840 | camera_key_yaw | exact | Left - Right |
| 0x434870 | camera_key_zoom | exact | Down - Up |
| 0x4348a0 | camera_key_extra_axis | exact | returns 0 |
| 0x4348b0 | slew_set_enabled | exact | |

### game/unnamed_cutscene_0x434900.cpp

| Address | Function | Confidence | Note |
|---|---|---|---|
| 0x434900-0x434930 | sinit / init / register / dtor of the cutscene object | exact | compiler-generated |
| 0x434940 / 0x434950 | init_cutscene_entries_6034c8(_thunk) | exact | compiler-generated (85 ctor calls, no atexit) |
| 0x434970-0x4349a0 | sinit / init / register / dtor of cutscene_alloc_pool | exact | compiler-generated |
| 0x4349b0 | cutscene_entry::cutscene_entry | exact | |
| 0x434b40 | cutscene::load_files | likely | File and the first float as loop-block locals (frame order) |
| 0x434e20 | cutscene::bind_actors | exact | |
| 0x435000 | cutscene::load_sounds | exact | |
| 0x4350b0 | cutscene::cutscene | exact | eh vector ctor of sequences[] compiler-generated |
| 0x4351c0 | cutscene::~cutscene | exact | empty body |
| 0x4351e0 | cutscene::unload | exact | |
| 0x435320 | cutscene::set_data_path | exact | |
| 0x4353b0 | cutscene::clear_data_path | exact | |
| 0x4353e0 | cutscene::load_data | likely | three failure stubs after the return jump back to a tail inside the innermost else: reproduced with `goto done` after each if / else; the original spelling is not known |
| 0x4354a0 | cutscene::get_fog_distances | exact | |
| 0x435520 | cutscene::apply_vis_overrides | exact | |
| 0x4355e0 | cutscene_parse_table | likely | the short temporaries of the length ternary |
| 0x435ff0 | cutscene_parse_visset | exact | |
| 0x4361d0 | cutscene_play | exact | |
| 0x4365e0 | cutscene_start_special_cases | exact | |
| 0x436620 | cutscene_cleanup | exact | |
| 0x436a60 | cutscene_end_special_cases | exact | |
| 0x436a90 | cutscene_sequence_update | likely | `fst / fmul 1.0 / fst` as `t = (t = x / 1000) * 1.0f` (G3 kept) |
| 0x436c00 | cutscene_events_frame | likely | same clock expression; switch with case 1 first |
| 0x437120 | cutscene_shutdown_stub | exact | |
| 0x437130 | cutscene_play_escape | exact | block locals in declaration order |
| 0x437400 | cutscene_change_level_for | exact | |
| 0x4374a0 / 0x4374b0 | init_ccmd_play_cutscene(_thunk) | exact | |
| 0x4374d0 | cmd_play_cutscene | exact | an unused File local |
| 0x437560 / 0x437570 | init_ccmd_play_and_load_cutscene(_thunk) | exact | |
| 0x437590 | cmd_play_and_load_cutscene | exact | |
| 0x437620 / 0x437630 | init_ccmd_stop_cutscene(_thunk) | exact | |
| 0x437650 | cmd_stop_cutscene | exact | |

Not in these units but declared for them: camera_sequence / cutscene_camera_sequence (0x493f20-0x4946a0, new
header `engine/camera/camera_sequence.h`).

## Decisions (include/DECISIONS.md D3200-D3207)

- D3200 the cutscene object 0x601e68 is `class cutscene` (`the_cutscene`, names ours); `cutscene_data`,
  `cutscene_path_id`, `cutscene_cur_sequence` and the other field symbols are macros; level_scripts.h's
  head-only `cutscene_entry` became the full class (char* fields).
- D3201 the cut moves to 0x434900; `#pragma optimize("", off)` after the three objects.
- D3202 summoner.h's cutscene / input declarations moved to the two unit headers (no signature change).
- D3203 camera_sequence classes; cutscene_camera_read_track takes (File *, float start).
- D3204: the saved-fog names were swapped in sum.symbols; fixed there after the merge.
- D3205 combat_mode is defined in the input unit; player_input stays in player.cpp; 0x434650 is the
  profile_label destructor COMDAT.
- D3206 config_invert_camera / config_camera_speed are macros onto Summoner_options.
- D3207 the new declarations.

## Open questions

- One TU or two (input + cutscene), see above.
- cutscene::load_data's real spelling (the jmp-to-jmp layout).
- The `1.0f` factor of the shot clock (a time-scale constant?), and why events_frame tests the shot range twice
  (a macro?).
- The meaning of Character +0x320 bits 0x10 (AI suspended for the cutscene?), 0x20, 0x400, 0x800 (summon kept
  through a cutscene; cleanup also destroys that summon's own effects) and the AI mode 9 that cutscene_play
  sends idle.
- player_read_mouse_input assigns PI_LEFT_CLICK (= 4) on a left click, dropping Shift / Alt / right-button
  bits set earlier in the same call; whether a reader needs them on that frame was not checked.
- The two floats at the start of each .csc file are read and dropped (answers half of cutscenes.md's open
  question); cutscene_time_offset is the running start before the first file, i.e. 0, and +0xc
  (end_time, name ours) the end of the last shot; nothing in this file reads +0xc.
- cutscene_script_load's return type (declared void; the result is not used).

## Doc disagreements (docs/ and symbols/ left untouched)

1. docs/sum-exe/source-map.md, third hand-cut table: row 0x4343a0-0x4349a0 includes "the static objects of
   the cutscene system" with ".bss 0x5fcca8-0x609f08"; row 0x4349b0 says "own .bss 0x609f30-0x609f38", and the
   "Tags to check" paragraph says the initialisers "end the 0x4343a0 unit". The objects, and .bss
   0x5fccc0-0x609f3b, are the cutscene file's (D3201); the input unit's .bss is 0x5fcca8-0x5fccb8. (The
   sum.symbols notes on 0x4343a0.. already give these .bss ranges.)
2. sum.symbols had 0x609f08 and 0x609f1c the wrong way round (fixed after the merge, D3204);
   docs/sum-exe/cutscenes.md step 5 "Saves fog near/far (0x609f1c, 0x609f08)" inherits it.
3. sum.symbols camera_key_yaw: the summary says "key_down_time(Right) - key_down_time(Left)"; the code (and the
   same entry's "checked" note) is Left - Right.
4. sum.symbols cutscene_camera_read_track "thiscall (file)": it takes (file, start time).
5. sum.symbols cutscene_time_offset "subtracted from the sequence start in cutscene_sequence_update": it is
   subtracted from the shot END (0x436b35) and added to the clock in cutscene_events_frame (0x437063).
6. sum.symbols player_input / player_mouse_held keep module `gameplay.cpp`, which the source map rules out.
7. sum.symbols player_read_mouse_input omits the ` (grave) click bits 0x40000000 (with Shift) / 0x80000000
   and that the click assigns 4.
8. docs/sum-exe/cutscenes.md, "Ending, skipping and restore": cleanup also sets cutscene_active_count - 1 and
   cutscene_frametime = 0, and destroys every active effect owned by the current summon when the summon has
   +0x320 bit 0x800 (0x43690b-0x436977). "Minitracks: snd_play at their shot" plays handle -1 (B3).
9. docs/sum-exe/cutscenes.md, "What cutscene_play changes" step 10: the "clear status effects" is
   character_clear_all_effects; a scripted actor (cutscene_object >= 0) also gets prop_tag = default_prop_tag
   and loses CF_COMBAT_READY when its stand and ready anims differ; bit 0x400 is cleared.

## Game bugs (new; IDs to assign in docs/sum-exe/game-bugs.md)

Checked against the retail cutscene.tbl (84 entries, extracted to the scratchpad only).

- **B1 (latent, cosmetic). The edge-turn block after a held drag can never trigger.** camera_process_input
  sets camera_edge_mouse_held (0x5fccb7) from PI_LEFT_HELD and immediately tests
  `held && !(in[0] & PI_LEFT_HELD)` (0x4346e7-0x43471b: `(~flags >> 4) & held`), which is always false, so
  camera_edge_turn_blocked (0x5fccb8) is never set and its reset (0x4347c2) is dead. Effect (*inferred*):
  releasing a long left-drag with the mouse at the screen edge starts the edge turn at once.
- **B3 (latent, cosmetic). +Minitrack sounds never play.** cutscene::load_sounds sets minitrack_sounds[i] to -1
  and nothing else writes them (0x43508a-0x435090); cutscene_events_frame passes them to snd_play (0x436fa0).
  No retail entry has +Minitrack.
- **B4 (live, effect inferred: invisible or a stray sound). Stale soundtrack handle.** cutscene::unload stops
  and frees soundtrack_sound (0x4352c4-0x4352f1) but does not reset it; load_sounds assigns it only for an
  entry with $Soundtrack (0x43502b). Seven retail entries have none (the four dragon summons, "test", "Test",
  "WolongFlythrough"), so a dragon summon cutscene after any story cutscene streams the freed handle at shot 0
  (0x436bb4, usually a failed snd_load: silence) and frees it again at its end (snd_stop + snd_free on a slot
  that snd_get_handle may have given to another sound since).
- **B5 (latent, crash). fogs[-1].** cutscene::get_fog_distances uses fogs[i - 1] after the loop
  (0x4354eb-0x435510); before the first key's shot (or with no $Fog) it reads the num_fogs word as a
  pointer (NULL without $Fog: a crash), every cutscene frame. Every retail entry has a $Fog key at shot 0.
- **B6 (latent, cosmetic). play_and_load_cutscene's help prints play_cutscene's usage** (the same literal
  0x593b90 at 0x4375fd). The console is not reachable in v1.40.
- **B7 (latent, crash with modded data). cutscene_parse_table checks no list bound** except $VisSet: more than
  85 entries, 4 $Camera, 2 plain / 10 shot $Animation lines, 4 +Minitrack, 16 $Fog or 16 $Fade lines overwrite
  the next fields / entries / pool records. Retail maxima: 4 cameras, 1 / 7 animations, 13 fogs, 16 fades,
  13 vissets. (C5, the 175 saved actors, is already listed.)
- G3 (the timer sign bug) is kept in cutscene_sequence_update and cutscene_events_frame and commented.

Not a bug: cutscene_active_count (incremented in load_sounds) does not leak on a failed load, since every
failing step returns before load_sounds and load_data ignores its result.
