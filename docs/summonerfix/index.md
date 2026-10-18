---
title: SummonerFix
nav_order: 4.5
has_children: false
---

# SummonerFix
{: .no_toc }

1. TOC
{:toc}

**Summary:** SummonerFix is our own open-source replacement for Timeslip's patch: a
`dinput.dll` proxy plus `summonerfix.ini` placed next to `Sum.exe`. It makes the game start
on Windows 11 without the original launcher, using the game's built-in but never-enabled
OpenGL renderer. The game keeps its native 1024x768 layout, and the image is scaled to fill a
borderless fullscreen window (4:3, centred), with the 3D world rasterised at the monitor's
native resolution. **Status (2026-10-14):** user-tested on Windows 11 at 2560x1440: key and mouse rebinding, the config app, Alt+Tab,
widescreen (`GameWidth=1366`), shops, looting, and the optional zoom and range settings all work. Fast mode (toggle,
"FAST x3" badge), the cutscene-uptime fix and the save-thumbnail fix were verified in agent mode. Earlier play-tests
confirmed 120 FPS at 1:1 game speed.

Source: `src/summonerfix/`. Build: `tools/build-summonerfix.ps1 [-Install]` (LLVM-MinGW,
`i686-w64-mingw32-gcc`). Test harness: `tools/test-run.ps1`.

## How it works

Sum.exe imports `DINPUT.dll!DirectInputCreateA`, so Windows loads `dinput.dll` from the game
folder before any game code runs. `DllMain`:

1. Reads `summonerfix.ini` and starts `summonerfix.log`.
2. Checks the v1.40 signature (`0x8B52106A` at `0x53983f`). On another build, it applies no
   fixes and only forwards DirectInput.
3. Hooks Sum.exe imports and applies guarded byte patches. Each patch verifies the
   original bytes first and logs a skip if they differ.

`DirectInputCreateA` forwards to the system `dinput.dll` (`SysWOW64`). Sum.exe v1.40 never calls it: the mouse
always runs in cursor mode ([Mouse](../sum-exe/vsdk-gr.md#mouse-0x500a30-0x501468)), so the import only serves
to get the proxy loaded.

## Fixes

| Fix | Mechanism | Why |
|---|---|---|
| No launcher needed | IAT hooks on `RegOpenKeyExA`, `RegCreateKeyExA`, `RegQueryValueExA`, `RegSetValueExA`, `RegDeleteValueA`, `RegDeleteKeyA`, `RegCloseKey` | The launcher can't detect modern cards, so the registry key stays empty. Display and sound values now come from the ini, and values the game writes are kept in ini `[Registry]`. Other keys pass through. See [Video settings](../sum-exe/video-settings.md). |
| Game resolution | `GameWidth`/`GameHeight` (default 1024x768); other sizes are written into resolution-table slot 2 (`0x580e48`, `0x580e4c`) | `LoadSettings` only accepts sizes in the table. Keep 1024x768: UI, clip rectangles and menus are laid out for it |
| OpenGL renderer | `0x43da64`: `BF 67` -> `BF 68` (`MOV EDI, 0x68`) | The default renderer is Glide; the OpenGL renderer `0x68` is never selected by the game |
| Windowed mode | `0x43db45`: `68 C9` -> `68 C8` (`PUSH 200`), always for OpenGL | `gr_opengl.cpp` refuses fullscreen mode 201 |
| Fullscreen output and scaling | Hooks on `SetWindowLongA`/`SetWindowPos` (only calls from `GrInit`), `glViewport`, `SwapBuffers` | See [Display scaling](#display-scaling) |
| Mouse mapping | Hooks on `ScreenToClient` (game poll at `0x500b50` in [`mouse_update`](../sum-exe/vsdk-gr.md#mouse-0x500a30-0x501468)) and `ClientToScreen` (cursor warp at `0x500ca7`) | Converts window pixels to game pixels and back |
| Correct 3D aspect | Hook on `GetSystemMetrics` for calls from `GrInit` | Windowed `GrInit` takes the aspect from the desktop (16:9) instead of the game's 4:3 |
| 2x game speed | `CALL SleepMs` at `0x5015bc` -> `FrameSleep` | See [Frame limiter](#frame-limiter-2x-speed) |
| Cutscenes skip after ~24.9 days of uptime | IAT hooks on `timeGetTime` and `GetMessageTime` return time since start-up (both rebased by the same amount, because `0x509880` subtracts one from the other for key timing) | `cutscene_sequence_update` subtracts its start time as a signed 32-bit value; see [Timing bugs](../sum-exe/timing-bugs.md#2-cutscene-timer-sign-bug-249-day-uptime). Tested 2026-10-14 in agent mode: with `[Debug] FakeUptimeDays=30` the new-game intro is skipped within 2 s (the reported bug), and with the fix it plays normally |
| Crash on Start New Game without sound (`bugfix.c`) | `mov eax, [esi+0xc]; test eax, eax` at `0x55c5db` in `ds_channel_release` -> `CALL` to a stub that returns -1 (no stream) unless `ds_init` finished (flag byte `0x2f8f73c` = 1) | When DirectSound cannot start (`DirectSoundCreate` fails, e.g. no audio output device connected), `ds_init` returns before `ds_init_channels` and `snd_stream_init`. The channel records stay zero and the stream lock (`0x2fbd998`) is never initialised. The level start then calls `ds_channel_release`, takes stream index 0 as valid and enters the zeroed lock: Access Violation in ntdll (write to `0x14`). Found 2026-10-14 in agent mode with every audio endpoint disconnected: the crash happened on every try, at 1024 and 1366. With the fix the intro cutscene plays. With sound working nothing changes. With `Soundcard=No Sound` a new game also starts (tested). That setting never had the bug: it sets `snd_disabled`, so no `ds_*` code runs. The bug is that `snd_init` skips setting `snd_disabled` when `ds_init` fails; see [Sound and music](../sum-exe/sound.md#start-up) |
| Low textures on a fresh install with OpenGL (`bugfix.c`) | `mov eax, 8` at `0x507a03` in `gr_get_video_memory_mb` -> `mov eax, 64` | The OpenGL back end reports a constant 8 MB of video memory. Its only caller, `summoner_cfg_load`, turns on reduced textures when `summoner.cfg` is missing or shorter than 64 bytes and the value is 8 or less, so a first start under SummonerFix chose low-resolution textures until the player saved the options. The patch applies (log, 2026-10-15); the texture choice was not tested, because that needs a missing `summoner.cfg`. See [Bugs in the original game](../sum-exe/game-bugs.md) K64 |
| World sounds at the sfx volume squared (`bugfix.c`) | `fld` / `fdiv [esi+0x28]` / `fstp` at `0x52156e` in `snd_play_3d` -> `CALL` to a stub that divides out the sound's group volume too | Without DirectSound3D (the default `Soundcard=DirectSound`) the game removed only the sound's own volume before `snd_play` multiplied group * sound volume in again, so footsteps, hits, spells and doors played at the sound-effects slider squared (0.5 gave 0.25). Now the slider is linear, as with DirectSound3D ([K260](../sum-exe/game-bugs.md)) |
| Brightness under OpenGL (`gamma.c`) | `SwapBuffers` hook: copy the frame to a texture and draw it back through `pow(c, 1/gr_gamma)` with a GLSL shader; fallback `SetDeviceGammaRamp` from `gr_gamma_ramp` (`0x25d8e0c`), restored when the window is not in front and at exit | The options screen's brightness slider and the `gamma` command did nothing under OpenGL ([K90](../sum-exe/game-bugs.md)). Nothing runs at gamma 1.0. `[Display] Brightness=Auto` (default), `Shader`, `Ramp` or `Off` |
| Flickering strip right of centred menus (`display.c`) | `Hook_SwapBuffers` clears the game area right of the 1024-wide layout on menu screens | With a wide `GameWidth` the menus are centred and that strip is part of the picture; the game never clears it, so old frames of the last level flickered there after a cutscene ([K280](../sum-exe/game-bugs.md)) |
| Print Screen above 1024 rows (`gameopts.c`) | With `GameHeight` above 1024, `Print_screen_as_targa` (`0x5fc022`) is set at start-up | `jpeg_write_file` keeps one row pointer per line in a 1024-entry stack table, so a taller JPEG screenshot overwrites its return address ([C290](../sum-exe/game-bugs.md)); the Targa writer goes row by row. Not tested in game |
| Robe of Restoration heals the attacker (`bugfix.c`) | `mov ecx, [ebp]` at `0x467b0f` in `combat_trigger_reflex_ability` -> `mov ecx, eax; nop` | In single player an armour's beneficial triggered spell targeted the attacker, so the robe healed the enemy that hit Rosalind ([G300](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Save over a save without the question (`bugfix.c`) | `mov eax, [saveload_selected_row]` at `0x426340` in `saveload_try_save` -> `CALL` to a stub returning `selected_row + saveload_scroll_pos` | With the save list scrolled, saving on the top row overwrote an existing save without "Overwrite existing savegame?"; the other rows asked about the wrong entry ([G4](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Phoenix Inferno hits its target once per member (`bugfix.c`, `[Fixes] PhoenixInferno`) | `phoenix_special_strike`: `esi` (the target) -> `ebx` (the loop member) in the three distance loads (`0x499900`, `0x499906`, `0x49990c`), the effect target (`0x499933`) and the hit (`0x499951`) | The Inferno compared the target's distance once per party member, hit the target that many times and knocked the whole party down ([G215](../sum-exe/game-bugs.md)). Reproduced and checked in game with `[Debug] TracePhoenixInferno` |
| No hit effect on metal characters (`bugfix.c`) | `cmp edi, 1; jne` at `0x466a0c` -> `cmp edi, 2; ja`; the stone / ice test at `0x466a2f` -> `jmp 0x466a39` | Hits on the Brass, Broken Brass, Steel and Iron Golems showed nothing, because only flesh, stone and ice spawned an effect. Now every skin that the game names an effect for shows one: blood for flesh and leather, sparks for stone, ice and the metals ([K20](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Choice popup rows 5 px above their text (`bugfix.c`) | `mov esi, [4*edx + 0x57ef70]; add esi, ecx` at `0x4139b3` -> `CALL` to a stub that also adds the title gap (`0x57ef7c`) | In the multiplayer "You have died." popup and the two PXO popups, the bottom of each entry selected the next one ([K170](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Luminar beam crash on a target that is gone (`bugfix.c`, `[Fixes] LuminarBeamTarget`) | The `character_add_hp` call at `0x4ca502` in `luminar_beam_hit` -> `CALL` to a stub that returns 0 when ECX (the target) is NULL | When the beam's target died or disappeared before the impact, the hit called `character_add_hp` on a NULL pointer and the game crashed ([C250](../sum-exe/game-bugs.md)). Reproduced and checked in game with `[Debug] LuminarBeamTargetGone` |
| Control jumps to the first member when a summon goes (`bugfix.c`, `[Fixes] SummonKeepsLeader`) | `mov edi, [party_list]` at `0x4335d9` in `party_remove_player` -> `CALL` to a stub that returns the list head (loop skipped) in single player while another record leads | Removing any party record handed control to the first living member, so a summon that died or was banished took control away from the hero the player had selected ([G8](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Spell cooldowns are the record number (`bugfix.c`, `[Fixes] SpellRecoveryTimes`) | `mov [esi-N], bp` -> `mov [esi-N], ax` at `0x47a49c`, `0x47a4c1`, `0x47a50b` in `spells_parse_table` | The parser stored the spell's record number instead of `+Recovery time:`, `+AI Recovery time:` and `+Loop Time:`, so fireball recovered in 26 s instead of 20 ([G12](../sum-exe/game-bugs.md)). The parsed values were checked in game, fix off and on; changes the balance |
| Loose gold pile holds 1 gold after a load (`bugfix.c`, `[Fixes] GoldPileAmount`) | `lea ecx, [esp+0x50]; push 1` at `0x4d33be` in `savegame_read_item_objects` -> `CALL` to a stub that pushes the pile's saved amount | The loader skipped the amount it had saved and created every loose pile with 1 gold ([G40](../sum-exe/game-bugs.md)). Reproduced and checked in game (`goldpile`, save, load, pick up) |
| A warning box resumes a paused game (`bugfix.c`, `[Fixes] MessageBoxPause`) | The `game_pause` call at `0x413f96` in `ui_message_box_show` and the `game_unpause` call at `0x414034` in `ClearFrameFlag5bd504` -> `CALL`s to stubs that unpause only when the box itself paused | A "Cannot save..." warning shown while the player had paused the game resumed it when closed ([G170](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Canis takes 100 gold for "10 gold pieces" (`bugfix.c`, `[Fixes] CanisTenGold`) | `sub eax, 0x64` -> `sub eax, 0x0a` at `0x4f124d` in `ls_lenele2ab_action` | The payoff topic in occupied Lenele offered 10 gold and took 100 ([G301](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| AI healer's Bless candidates counted twice (`bugfix.c`, `[Fixes] AiBlessCount`) | The first `inc ebx` at `0x49395c` in `ai_group_think_caster` -> `nop` | Each unblessed member counted twice, so an AI healer cast Protect as soon as two members needed it, and with four needy members the count ran past its six-entry stack list into saved registers and the return address ([C210, G212](../sum-exe/game-bugs.md)). The doubled count was seen in game (5 for 4 members) and was 1 with the fix; the crash itself was not reached |
| A burning area whose caster is gone switches off the rest (`bugfix.c`, `[Fixes] AreaCasterGone`) | `je 0x48b5a1` -> `je 0x48b581` at `0x48b40a` in `fx_frame` | When the caster of one fire / blizzard / dark wall area was removed (a summon that left), the loop over the areas ended there, so every later area stopped burning and its status was cured ([G270](../sum-exe/game-bugs.md)). Reproduced and checked in game (`firewall`, `kill_summon`) |
| Support AI drops the target it picked (`bugfix.c`, `[Fixes] SupportAiTarget`) | `add esp, 0x10` -> `jmp 0x493564` at `0x49354a` in `ai_group_think_style1` | After ordering an attack on the enemy it chose, a Support member (Flece by default) fell through to the "first enemy" order, which replaced it with a random enemy ([G211](../sum-exe/game-bugs.md)). Seen and checked in game with `[Debug] TraceGroupAttack` |
| Detection cone the wrong way round (`bugfix.c`, `[Fixes] DetectionCone`) | `jne` -> `je` at `0x493c5c` in `ai_group_collect_enemies` | Beyond detection range, party AI members (in practice summons with a wide field of view) noticed enemies outside the 45-degree cone ahead and not those inside it ([G210](../sum-exe/game-bugs.md)). Reproduced and checked in game with `[Debug] TraceDetectCone` |
| Leech does nothing in single player (`bugfix.c`, `[Fixes] LeechDrain`) | `fstp [ebp-0x24]; jmp 0x45fe7b` at `0x45f47d` (case `0x3e` of `living_entity_get_hit_by_spell`) -> `jmp` to a stub that drains the amount from the target into the source and resumes at `0x45fed8` | The leech special ability (Blood Elemental, Crystal Rokhul, Joseph's Wraith) computed `power x 0.05 x level` and then lost it to a dice roll of 0 dice ([G100](../sum-exe/game-bugs.md)). Leech now drains that much HP from the creature hit into the attacker. Reproduced and checked in game (`spellhit`) |
| Conversation "more" / "end" lit far from the mouse (`bugfix.c`, `[Fixes] ConversationHover`) | `mov edx, [esp+h]; add ecx, edx; cmp eax, ecx` -> `add ecx, [esp+h]; cmp [esp+y], ecx` at `0x4d6fe8` and `0x4d707c` in `dialogue_topic_entry__draw` | The hover test compared the mouse x with the entry's bottom edge, so "more" and "end" lit up anywhere below them ([K292](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Scrolled save list shows the scene for its top row (`bugfix.c`, `[Fixes] SaveListThumbnail`) | `test al, al; je; test ebx, ebx; jle` at `0x425c61` in `saveload_load_thumbnail` -> `CALL` to a stub that tests the entry (0 = `<New Savegame>`) + `jle`; `mov eax, [saveload_selected_row]` at `0x426fda` in `saveload_draw_thumbnail` -> the G4 stub (row + scroll) | In save mode both used the visible row, so with the list scrolled the top row's save showed the current scene ([K5](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Zane's encounter overwritten (`bugfix.c`, `[Fixes] ZaneEncounter`) | `mov ecx, edx; and ecx, 3; rep movsb` at `0x4fca1b` in `ls_worldmap1_pick_encounter_stage4` -> `jmp` to a stub that finishes the copy and returns through `0x4fcae1` | At game stages 4-8 on grassland, Zane's 50 % encounter roll had no return and the general rolls replaced it ([G6](../sum-exe/game-bugs.md)). Reproduced and checked in game (`call0`, 40 picks: 4 before, 19 after) |
| Mounted-rider test reads the wrong stage (`bugfix.c`, `[Fixes] MountedRiders`) | `boss_species_mounted_stage` (`0x4a2480`) -> `jmp` to a replacement that reads `phoenix_stage` for the Phoenix and the Ghost's stage (`0x2480310`) | The party AI's "target is a mounted rider" test read the Serpent's stage for the Phoenix and never saw the Ghost as mounted ([G32](../sum-exe/game-bugs.md)). Checked by calling the function in game, fix off and on |
| Boss music never switched (`bugfix.c`, `[Fixes] BossMusic`) | `mov byte [ls_liangshan_phoenix_dead_done], 1` at `0x4f49be` and the two `mov byte [..._done], bl` at `0x4f9f2e` -> `CALL`s that preset the bytes from the saved flags | The Ikaemos upper hall never started the Boss music for the Priest King or put its own track back after him, and Liangshan kept the Boss music after both riders died ([K130, K150](../sum-exe/game-bugs.md)). Reproduced and checked in game with `[Debug] TraceMusic` |
| Testing: music (`trace.c`) | `[Debug] TraceMusic=1` moves `music_play`'s prologue (`0x469080`) into a stub that logs the track index and its `music.tbl` name | Used for K130 / K150; off by default |
| Ring power of exactly 60,000 (`bugfix.c`, `[Fixes] RingSixtyThousand`) | `jle` -> `jl` at `0x447390` (`character_can_use_summon`) and `0x45023a` (`ring_check_summon_unlock`) | A ring that reached exactly 60,000 never announced its new summon, and its pop-up listed the summon one gain before it could be cast ([K100](../sum-exe/game-bugs.md)). Reproduced and checked in game (the unlock message) |
| The 33rd map label overwrites the first (`bugfix.c`, `[Fixes] MapLabelLimit`) | `xor eax, eax; ret` -> `or eax, -1; ret` at `0x430245` in `automap_label_alloc` | With all 32 labels used, the allocator returned label 0 instead of "none", so a new label replaced the first one ([K6](../sum-exe/game-bugs.md)). Checked by calling the allocator in game |
| Shop category buttons in sell mode (`bugfix.c`, `[Fixes] ShopCategoryButtons`) | `mov eax, [buysell_list_count]` at `0x404f98` (sell branch of `buysell_category_first_index`) -> `jmp` to a stub that returns the first shown row of the category | In sell mode the category buttons used the shop's buy list: they jumped to the wrong row and stayed enabled for categories the party had nothing in ([K3](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Inventory category buttons in identify mode (`bugfix.c`, `[Fixes] InventoryCategoryButtons`) | `mov eax, esi` at `0x41543e` (identify branch of `inventory_category_first_index`) -> `jmp` to a stub that turns the party-inventory index into its row among the unidentified items | In identify mode the category buttons used the party-inventory index as a row of the shorter unidentified list, so they jumped too far down ([K4](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Ambient sounds on level changes (`bugfix.c`, `[Fixes] AmbientSounds`) | `inc esi` at `0x4cccc9` in `level_script_leave` -> `nop` | The ambient-sound removal loop removed by an index into a table that shrinks, so every level leave freed only half the emitters; after a few town visits the 128 `sound_emitters` were full and levels lost ambient sounds ([K110](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Container scrollbar (`bugfix.c`, `[Fixes] PickupScrollbar`) | `mov [pickup_thumb_h], reg` at `0x4208f8` and `0x420943` -> `call` a stub that stores track x page / items | The pickup screen's thumb was always as tall as the track, so a scrolled container list drew it past the bottom ([K12](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Edge turn after a drag (`bugfix.c`, `[Fixes] EdgeTurnAfterDrag`) | `mov [0x5fccb7], al` at `0x4346f1` -> `call` a stub that swaps the held-drag byte and returns last frame's bit; `mov al, [0x5fccb7]` at `0x43470a` -> reads that copy | The block that should hold the edge camera turn back after a drag released at the screen edge tested the bit it had just stored and never engaged ([K191](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Loot highlight colour (`bugfix.c`, `[Fixes] LootHighlightColour`) | `mov ecx, [0x594430]` at `0x43a62e` -> `[0x59442c]` | The 5-second highlight of dropped loot read its blue from the string "gold" next to the colour, which the renderers turn into 0, so it flashed yellow instead of pale gold ([K61](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Knock-down message (`bugfix.c`, `[Fixes] KnockDownMessage`) | `mov eax, [esi+0x4c] ; mov ecx, [cur_character]` at `0x47431b` in `ai_knock_down_local` -> `call` a stub that makes the current-character test fail when `ai_set_mode` refused | "Knocked Down!" showed for a character who kept casting and stayed up ([K80](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Long game folder path (`bugfix.c`, `[Fixes] LongInstallPath`) | `mov ecx, file_zero_vector` at `0x512e0b` in `file_init` -> `call` a stub that replaces an `install_path` of 64+ characters with its 8.3 short form | The game joins its folder path and file names in 128-byte buffers, so a folder path of 116+ characters crashed it at start-up ([C310](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Effect slot spell (`bugfix.c`, `[Fixes] EffectSlotSpell`) | `fx_new`'s tail at `0x48c461` -> also `mov [eax+0xa8], edx` (-1) before the `ret`, in its padding | A status holder or item effect that reused the object of a burnt-out fireball or wall of fire kept that spell, so the party AI (and Pyrul and Murod) saw a fire burning and did not choose fireball ([G271](../sum-exe/game-bugs.md)). Reproduced and checked in game |
| Enemy area-spell distances (`bugfix.c`, `[Fixes] AreaSpellDistances`) | `mov ecx, [esp+0x10] ; mov [ebp], eax` at `0x4bfdbd` in `ai_combat_cast_frame` -> `call` a stub that also stores `distance(caster, target)` at `+4` | Enemy casters chose meteor storm and inferno by a distance field that was never written ([G241](../sum-exe/game-bugs.md)). Reproduced (traced) and checked in game |
| Testing: enemy area spells (`trace.c`) | `[Debug] TraceAreaSpell=1` moves `ai_consider_area_spell`'s prologue (`0x4befe0`) into a stub that logs, for casters outside the party, the spell, its range and each candidate's distance field next to the real distance | Used for G241; off by default |
| Testing: Giant Salamanka reach (`trace.c`) | `[Debug] TraceBossReach=1` wraps the `giant_salamanka_target_in_reach` call at `0x4a0f8c` and logs the current move's offense point and the answer | Used to check G30 (withdrawn: the points are right); off by default |
| Testing: detection cone (`trace.c`) | `[Debug] TraceDetectCone=1` turns `fcomp [0x581ae0]` at `0x493c51` into a call that logs the dot product and whether the enemy is accepted | Used for G210; off by default |
| Testing: AI support spell picks (`trace.c`) | `[Debug] TraceSupportPick=1` turns the `frand` call at `0x4beb3d` in `ai_choose_support_spell` into a call that draws the number itself and logs the shares, the number, the game's pick and a running-sum pick | For G229; off by default |
| Testing: style-1 attack orders (`trace.c`) | `[Debug] TraceGroupAttack=1` turns the two `ai_group_order_attack` calls at `0x493545` and `0x49355f` into calls that log the member and the target | Used for G211; off by default |
| Testing: AI Bless count (`trace.c`) | `[Debug] TraceBlessCount=1` turns `mov eax, [esp+0x2c]; test eax, eax` at `0x4939bd` into a call that logs the candidate count and the member count; `[Debug] BlessPassAlways=1` points the 3x attack-rating compare at `0x4938cf` at 0.0 so the pass runs in any fight | Used for C210; off by default |
| Testing: Luminar beam target gone (`trace.c`) | `[Debug] LuminarBeamTargetGone=1` turns the `living_from_handle` call at `0x4c95fc` in `luminar_ai_frame` into a call to a stub that returns NULL while the AI mode is `0x32` (the beam), and counts those frames (address in the log) | Used to reproduce C250; off by default |
| Testing: trace Phoenix Inferno hits (`trace.c`) | `[Debug] TracePhoenixInferno=1` turns the `living_entity_get_hit_by_spell` call at `0x499962` into a call to a stub that logs the character hit and the party member the loop is on | Used to reproduce G215; off by default |
| Testing: trace hit effects (`trace.c`) | `[Debug] TraceHitVfx=1` turns the entry of `combat_spawn_hit_vfx` (`0x4669c0`) and its two `fx_attach_to_character` calls into jumps to stubs that log the target, its skin material and the effect spawned | Used to reproduce K20; off by default |
| Testing: trace armour spells (`trace.c`) | `[Debug] TraceArmourSpells=1` redirects the calls at `0x467c86` and to `combat_trigger_reflex_ability` to stubs that log who hit whom and which spell an armour cast on whom | Used to reproduce G300; off by default |
| Testing: count the K310 colour path (`trace.c`) | `[Debug] TraceGlLights=1` turns the first colour load at `0x491f9e` into a call that counts the faces on the unclipped lightmap path and those with a coloured light (agent command `k310`) | Showed that this path did not run in the Ikaemos tests ([K310](../sum-exe/game-bugs.md)); off by default |
| Optional gameplay settings (`gameopts.c`) | `CameraZoomMax` writes `camera_zoom_max` (`0x59b800`, game 1.0). `SkipIntroMovies` clears `play_intro_movies` (`0x5930bd`). `InteractRange` and `ContainerRange` point the distance operands at `0x40e696` (`hud_interact_nearest`, 5.0) and `0x43ad38` (`world_mouse_frame`, 25.0 = 5 squared) at our own floats, because those constants are shared with many other sites | Timeslip's camera zoom, movie skip and interaction-range options, reimplemented from our own analysis. All are off by default. Checked 2026-10-14 in agent mode: the values are patched, and with `SkipIntroMovies=1` the main menu shows at once instead of the logo movies. User-tested 2026-10-14: zoom, talk/use range and container range all work |
| Console-command keys | `[ConsoleKeys] Key=command`: our window-message handler (registered with `os_add_msg_handler`, so it runs on the game thread) calls `execute_command_line` (`0x50a210`) on a key press. Live reload like the bindings | Timeslip's `[console] k###=command`. Tested 2026-10-14 in agent mode with a real posted `WM_KEYDOWN`: `F9=add_gold 1000` raised the party's gold from 61 to 1061. Keys pressed while the game is still starting (game state 0) are ignored: running a command then crashed the game (2026-10-16) |
| Savegame thumbnails | Hook on `glReadPixels` (only caller: `gr_opengl_read_backbuffer`, `0x5502c0`): reads the game-space rectangle mapped to the scaled output and box-filters it to the requested size | `saveload_capture_thumbnail` (`0x427100`) renders the scene into a 201x151 clip and reads it back in game pixels, but our framebuffer is the scaled output, so thumbnails came out black. Fixed 2026-10-14 and checked in agent mode (the Save screen preview shows the scene). Saves made before the fix keep their black thumbnails |
| Frame cap (optional) | `MaxFPS` writes `MIN_DT` into the immediate at `0x5014ea` (`MOV [0x5a574c], imm` at `0x5014e4`) | Default 0 keeps the game's 120 FPS |
| 1 GB allocation | IAT hook on `glGetIntegerv`; caps `GL_MAX_TEXTURE_SIZE` (default 2048) | See below |
| Custom key bindings | `CALL key_remap_layout` at `0x5093a6` in `key_mark` -> `KeyRemap`: the original layout swap, then a 256-entry table from `[KeyBindings]`. A watcher thread reloads the table when `summonerfix.ini` changes, so bindings change live | The game key map is hard-coded; see [Controls](../sum-exe/controls.md). Tested 2026-10-13 (E=I, A=Left) |
| Centred menus at wide resolutions | `GetMap` (viewport, bar clearing and mouse mapping) shifts the picture by `(GameWidth - 1024) / 2` game pixels while the game state is not 4 (gameplay) | Menus are a fixed 1024-wide layout; at `GameWidth=1366` they sat at the left with an empty right side. Checked by frame capture 2026-10-13: centred with even bars. `[Display] CenterMenus=0` turns it off |
| Centred in-game UI at wide resolutions (user-tested 2026-10-13/14) | `src/summonerfix/widescreen.c`. In game state 4, calls from UI code (`0x401000`-`0x430000`, vsdk ui, conversation text widgets `0x4d6600`-`0x4d7ab0`) to the 2D primitives are redirected (about 400 call sites) and x is shifted by `(GameWidth - 1024) / 2`. UI-set clips get the shift instead of their draws. UI code sees `gr_screen_width() = 1024`. The mouse is given to UI code in layout space, and the 9 world-code reads use the raw position. `interface_set_view_clip` (3D view) is excluded. Full-screen panels (inventory, character, journal, options, save/load) instead shift the whole output like the menus, because their item and character models are rendered in screen coordinates. Side panels (map, spells, skills, item trade) stay flush left (their calls in `interface_draw`/`interface_frame` run unshifted with the raw cursor). In `interface_set_view_clip`, full panels get a 1024-wide 3D view, so models in the panel are projected around the layout centre. Side panels keep the world full width behind them (`[Display] SidePanelsOverWorld`), so the scene does not move. While a full panel is open, `gr_screen.aspect` is set to 1.0, because the projection divides by it and models were 25% too small. Checked with agent-mode screenshots 2026-10-13: the inventory matches 1024x768 exactly | HUD and in-game screens are a fixed 1024-wide layout at the left; the 3D world already uses the full width. `[Display] CenterHud=0` turns it off |
| Mouse bindings | The six `CALL mouse_mark_button` sites in the mouse message handler (`0x500f79`-`0x500fba`) -> `MouseMark`; our own handler registered with `os_add_msg_handler` (`0x530c20`) for `WM_XBUTTON*` (side buttons) and for keys bound to mouse buttons; mouse-to-key bindings are injected through `key_mark` | `[KeyBindings]` accepts `MouseLeft`, `MouseRight`, `MouseMiddle`, `Mouse4`, `Mouse5`, e.g. `Mouse4=M`, `Q=MouseRight`, `MouseMiddle=Space`. Tested 2026-10-14 in agent mode with posted window messages: `Q=MouseLeft` over the chest icon opened the inventory, and `Mouse4=R` (`WM_XBUTTONDOWN`) opened the Character screen |
| Speed-up key (fast travel) | `[Game] SpeedKey` (default F11) turns fast mode on and off (`SpeedMode=Toggle`, the default since 2026-10-14) or works while held (`SpeedMode=Hold`). While fast mode is on during gameplay, a "FAST x3" badge is drawn at the top left of the picture (OpenGL, in the `SwapBuffers` hook). In fast mode, `frametime_divisor` (`0x5a5748`) = `1 / SpeedFactor` | `FrameDone` reports frame time / divisor, so the whole game advances faster. Tested 2026-10-14 in agent mode (`speedkey 1`): in one real second the game clock `Timestamp_ticker` advanced 968 ms normally and 3000 ms with the key held (`SpeedFactor=3`). Combat and AI at high factors are not play-tested |
| Built-in script editor (`[Debug] ScriptEditorKey`, off by default) | A key handled on the game thread (our `os_add_msg_handler` handler) toggles bit `0x80` of `game_flags` (`0x5fbfd8`) during gameplay. `script_editor_frame` runs Volition's level script editor only while that bit is set, and nothing in the retail game sets it | For modders. Tested 2026-10-14 in agent mode with `F12`: the editor's main bar opens and closes, and the HUD comes back. **Its Save button writes `.tbl` files into the game's data folders**, so back up first. See [Script editor](../sum-exe/script-editor.md) |
| Agent mode (`[Debug] Agent=1`) | `agent.c`: commands from `Summoner\agent\cmd.txt` inject game keys/mouse buttons, set the cursor, take screenshots, report state; keeps the game from pausing | Lets scripts (`tools/agent.mjs`) play and test the game; tested 2026-10-13 (menu, load, inventory) |
| Typing ignores key bindings | `key_to_ascii` (`0x509ca0`) -> `KeyToAscii`: `KeyRemap` records which physical key produced each game key code; text entry translates back before the character lookup | With `E=I` bound, typing a save name with E gave "i" (user report 2026-10-13). Tested 2026-10-14 in agent mode: with W, A, D and E all rebound, posted key presses typed "wade" in the save description (cancelled, nothing saved) |
| Alt+Tab works | `SetWindowPos` hook: the call from the game's focus-loss handler (return address `0x5310ef`) becomes `HWND_BOTTOM` + `SWP_NOACTIVATE`, and `SWP_NOACTIVATE` is added to the focus-gain call (`0x5310b7`). The F10 hotkey (`[Game] ConfigKey`) opens the key binding app, which is topmost | The game re-activated itself on losing focus, so other programs (such as `SummonerFixConfig.exe`) could not be brought forward; see [Controls](../sum-exe/controls.md#switching-away-from-the-game-alttab). Tested 2026-10-13 |
| Diagnostics (optional) | Hooks `MessageBoxA`, `ExitProcess`, `HeapAlloc`, `VirtualAlloc`, `GlobalMemoryStatus`, `Sleep`; logs them with a heuristic stack trace; logs access violations (address, registers, stack; the CRT otherwise exits silently with code 0xC0000005); logs FPS, sleep and game-clock statistics every 5 s; optional frame captures from the back buffer | Tracing failures to game code |

### Switching original-game fixes off (`[Fixes]`)

Added 2026-10-17 (owner's decision). Every fix in `bugfix.c` that changes how the game plays, sounds or looks
has a line in `summonerfix.ini [Fixes]`: 1 (the default, also when the line or the section is missing) applies the
fix, 0 leaves the original game's behaviour. They are read when the game starts; `summonerfix.log` notes each
one that is off. The no-sound-device crash fix is always on. `Enabled=0` turns all of them off at once, whatever
their own lines say (added 2026-10-17, owner's design).

`SummonerFixConfig.exe` shows one checkbox, **Fix bugs in the original game** (`Enabled`, saved with **Save
options**), and a **Choose fixes...** button. That opens a second window with a checkbox per fix, saved with its own
**Save** button. Tested 2026-10-17: unticking one fix and saving wrote its key as 0, unticking the main box and
saving options wrote `Enabled=0`, and the game then applied none of the 20 fixes (the crash fix still patched).

| Key | Fix |
|---|---|
| `Enabled` | 0 = none of the fixes below (default 1) |
| `RobeOfRestoration` | The Robe of Restoration heals its wearer ([G300](../sum-exe/game-bugs.md)) |
| `WorldSoundVolume` | World sounds follow the sound-effects slider linearly ([K260](../sum-exe/game-bugs.md)) |
| `OpenGLTextures` | Full-detail textures on a fresh OpenGL install ([K64](../sum-exe/game-bugs.md)) |
| `MetalHitEffects` | Hit effects on metal golems ([K20](../sum-exe/game-bugs.md)) |
| `PopupRowOffset` | Popup rows light up under the cursor ([K170](../sum-exe/game-bugs.md)) |
| `SaveOverwriteQuestion` | Saving on the top row of a scrolled save list asks first ([G4](../sum-exe/game-bugs.md)) |
| `PhoenixInferno` | The Phoenix Rider's Inferno hits each party member in range once, not its target once per member; the fight becomes easier than in the original ([G215](../sum-exe/game-bugs.md)) |
| `LuminarBeamTarget` | No crash when the Luminar's beam target dies or disappears before the hit ([C250](../sum-exe/game-bugs.md)) |
| `SummonKeepsLeader` | When a summon goes, the hero you control stays in control ([G8](../sum-exe/game-bugs.md)) |
| `MessageBoxPause` | A warning box shown while you have paused the game leaves it paused when closed ([G170](../sum-exe/game-bugs.md)) |
| `CanisTenGold` | "Give him 10 gold pieces." to Canis in occupied Lenele takes 10 gold, not 100 ([G301](../sum-exe/game-bugs.md)) |
| `AiBlessCount` | An AI healer counts each party member once when choosing Bless or Protect: no stack overwrite, and Protect only when three members need it ([C210, G212](../sum-exe/game-bugs.md)) |
| `AreaCasterGone` | Fire, blizzard and dark wall areas keep burning when the caster of an earlier one has gone ([G270](../sum-exe/game-bugs.md)) |
| `SupportAiTarget` | A Support-style party member (Flece by default) keeps the enemy it chose instead of a random one ([G211](../sum-exe/game-bugs.md)) |
| `DetectionCone` | Summons notice enemies ahead of them, not behind, beyond their detection range ([G210](../sum-exe/game-bugs.md)) |
| `LeechDrain` | The leech special ability drains HP from the creature hit into the attacker ([G100](../sum-exe/game-bugs.md)); changes the balance |
| `ConversationHover` | A conversation's "more" and "end" light up only under the mouse ([K292](../sum-exe/game-bugs.md)) |
| `SaveListThumbnail` | With the save list scrolled, the top row shows its save's picture, not the current scene ([K5](../sum-exe/game-bugs.md)) |
| `ZaneEncounter` | While Zane is about (game stages 4-8), grassland encounters bring his encounter about half the time, as the code intends ([G6](../sum-exe/game-bugs.md)) |
| `MountedRiders` | The party AI knows when the Phoenix and Ghost riders are mounted, from their own fight stage ([G32](../sum-exe/game-bugs.md)) |
| `BossMusic` | The Priest King fight in the Ikaemos upper hall plays the Boss music, and Liangshan returns to its own music after both riders die ([K130, K150](../sum-exe/game-bugs.md)) |
| `RingSixtyThousand` | A ring's higher summon is usable and announced from a power of 60,000 on, as its pop-up lists it ([K100](../sum-exe/game-bugs.md)) |
| `MapLabelLimit` | A 33rd label on a map is refused instead of replacing the first ([K6](../sum-exe/game-bugs.md)) |
| `ShopCategoryButtons` | In a shop's sell list the category buttons jump to your own items of that category, and are off when you have none ([K3](../sum-exe/game-bugs.md)) |
| `InventoryCategoryButtons` | When identifying items, the category buttons jump to the unidentified items of that category ([K4](../sum-exe/game-bugs.md)) |
| `AmbientSounds` | Leaving a level frees all of its ambient sounds, so levels keep their sounds after many level changes ([K110](../sum-exe/game-bugs.md)) |
| `PickupScrollbar` | A container's scrollbar thumb is sized to the shown part of the list and stays inside its track ([K12](../sum-exe/game-bugs.md)) |
| `EdgeTurnAfterDrag` | Letting go of a drag with the cursor at a screen edge does not turn the camera until the cursor leaves the edge ([K191](../sum-exe/game-bugs.md)) |
| `LootHighlightColour` | A monster's dropped loot flashes pale gold, as in the game's data, instead of yellow ([K61](../sum-exe/game-bugs.md)) |
| `KnockDownMessage` | "Knocked Down!" is shown only when your character really falls, not while it casts ([K80](../sum-exe/game-bugs.md)) |
| `LongInstallPath` | The game starts from a folder with a long path (116+ characters) instead of crashing, by using the folder's 8.3 short name ([C310](../sum-exe/game-bugs.md)) |
| `EffectSlotSpell` | A status effect's glow no longer counts as a burning fireball, so party AI casters keep choosing fireball ([G271](../sum-exe/game-bugs.md)) |
| `AreaSpellDistances` | Enemy mages cast meteor storm and inferno only when two of your party are really within reach ([G241](../sum-exe/game-bugs.md)) |
| `GoldPileAmount` | A gold pile left on the ground keeps its amount after a load ([G40](../sum-exe/game-bugs.md)) |
| `SpellRecoveryTimes` | Spell cooldowns use the recovery times in `spells.tbl` (fireball 20 s, not 26); this changes the balance ([G12](../sum-exe/game-bugs.md)) |

Tested 2026-10-17: with `MetalHitEffects=0` the log shows that fix off and the others applied.

## Problems found and fixed

### OpenGL: "can only be used in windowed modes for now"

**Observed** at runtime: in fullscreen mode (201) the OpenGL renderer stops with
`Error: OpenGl can only be used in windowed modes for now` (`gr_opengl.cpp` line 657).
Workaround: always use windowed mode (200) and turn that window into a borderless fullscreen
window (see below).

### OpenGL: "Failed to allocate 1073741824 bytes"

**Observed.** `GlTextureInit` (`FUN_0054e690`, reached from `GrInit` via
`0x54f410`) calls `glGetIntegerv(GL_MAX_TEXTURE_SIZE)` and allocates a scratch buffer of
`size x size x 4` bytes. The test PC's driver reports **16384**, so the request is
16384² x 4 = exactly 1 GB, which fails in a 32-bit process (`memory.cpp` line 223). Stack
from diagnostics: `HeapAlloc` <- `0x5124F2` <- `0x51255E` <- `0x54E72D` <- `0x54F66D` <-
`0x54F73E` <- `0x5060A9` (`GrInit`) <- `0x43DB52` (`LoadSettings`).

Fix: report at most `MaxTextureSize` (2048, so a 16 MB buffer). The 2000-era game textures
are far smaller. *Inferred:* this should not reduce quality; not yet checked in-game.

### Direct3D: silent exit (not fixed)

**Observed.** With `Renderer=Direct3D`, the game exits with code 0 about 4 s after start.
Diagnostics show `exit(0)` called from `0x539B4D` in the Direct3D init
`FUN_005395c0`. Every failed DirectDraw/Direct3D call there jumps to one shared
`exit` block at `0x539b41`, so which call fails is **unknown**. Not pursued while OpenGL
works.

### Display scaling

**Why:** the game lays out everything in fixed pixels for its internal resolution: UI
positions, menu bitmaps, and `SetClip(x, y, w, h)` (`FUN_00506840`, 27 callers). Timeslip's
patch moved UI elements around (a position table at `0x58acbc`-`0x58bc40`), which left menus
misplaced. Running the OpenGL renderer at a larger size than 1024x768 drew everything in
the top-left 1024x768, and the uncovered area showed garbage under the mouse (user report).

**How:** the game keeps its 1024x768 coordinates. `glOrtho` stays in game pixels; only
`glViewport` is changed, mapped to a centred, aspect-correct rectangle of the real window
(`scale = min(clientW/1024, clientH/768)`). OpenGL then rasterises all geometry at the
window's native resolution. Before each `SwapBuffers` the unused bars are cleared with a
scissored `glClear`. Mouse coordinates are mapped back to game pixels.

**Observed:** main menu centred at 2560x1440 with black bars (frame capture, 2026-10-12).

### Frame limiter: 2x speed

**Observed.** The game's frame limiter double-counts its sleep, so on a fast PC game time
runs at twice real time. Details: [Timing bugs](../sum-exe/timing-bugs.md#3-frame-limiter-double-counts-its-sleep-2x-game-speed).

`FrameSleep` (in `timing.c`) replaces the limiter's sleep. It waits with a high-resolution
waitable timer until exactly one `MIN_DT` after the previous frame's end (kept internally,
because `FrameTimer` has already overwritten its own timestamp), then sets the game's
`last` timestamp (`0x257f098`) to that time.

| Build | Real FPS | Game's own FPS | Game speed |
|---|---|---|---|
| Original | ~218 | ~115 | about 2x |
| `last` = wake-up time | ~113 | 120 | 0.94x |
| Final (`last` = scheduled time) | **120.0** | **120.0** | **1x** |

The game clock (`FrameClockRead`) was checked against QPC: 5001.2 ms vs 5001.2 ms over 5 s.

## Key binding app (`SummonerFixConfig.exe`)

`SummonerFixConfig.exe` (source: `src/summonerfix-config/config.c`) is installed next to `Sum.exe`. It lists the game's keyboard actions, including the four chain-attack slots (held modifier keys) and SummonerFix's own speed-up and open-this-window hotkeys, with their default keys and any extra keys bound to them:

1. Select an action and click **Change key...** (or double-click it).
2. Press the key you want. The app writes `[KeyBindings]` in `summonerfix.ini` straight away.
3. A running game reloads the bindings within a fraction of a second, so you can switch to the app, rebind and return.

**Clear** removes the extra keys from one action, and **Reset all** restores the game defaults. Mouse buttons, including the side buttons Mouse4/Mouse5, can be bound like keys: click them while the app waits for a key. The rows for left, right and middle click let a key act as a mouse button, for example a key for right-click chain attacks. Rebinding the left button away also removes it from the menus; Reset all undoes that.

**Game options** (right-hand column, added 2026-10-14) edit the optional `[Game]` settings: **Skip intro movies**,
**Speed key toggles**, the zoom-out limit, the talk/use range and the container range (each shows the standard game's value). Leave a field empty for the game's own value.
**Save options** writes them; they take effect the next time the game starts.

**Console keys** (added 2026-10-14): type a console command (for example `add_gold 1000`) in the box under the
options and click **Bind key...**, then press the key. The binding is written to `[ConsoleKeys]` and shows as a
"Console: ..." row in the list, where **Change key...** and **Clear** work on it. Checked with a real key press. The same
change fixed a bug: rebinding the speed or config hotkey in the app saved a garbled key name, because the key name was
taken from an unused buffer.

A binding adds a key; the default key keeps working. If the new key already belonged to another action, that action loses it, and the app says so. The list shows "(moved)" next to a default key that now does something else.

## Configuration (`summonerfix.ini`)

| Key | Default | Meaning |
|---|---|---|
| `[Display] Renderer` | `OpenGL` | `OpenGL`, `Direct3D` or `Glide` |
| `[Display] Mode` | `Fullscreen` | OpenGL output: `Fullscreen` (borderless, monitor size) or `Window` |
| `[Display] WindowWidth`, `WindowHeight` | 1600, 1200 | Client size for `Mode=Window` (resizable) |
| `[Display] GameWidth`, `GameHeight` | 1024, 768 | The game's internal layout resolution; e.g. 1366x768 for 16:9 |
| `[Display] CenterMenus` | 1 | With a wider `GameWidth`, centre the 1024-wide menus |
| `[Display] SidePanelsOverWorld` | 1 | Wide screens: the 3D view stays full width behind the map/spells/skills panels |
| `[Display] CenterHud` | 1 | With a wider `GameWidth`, centre the HUD and in-game screens (OpenGL) |
| `[Display] Windowed` | 0 | Direct3D/Glide only: use the game's own windowed mode |
| `[Game] MaxFPS` | 0 | Frame cap; 0 = game default 120 |
| `[Display] MaxTextureSize` | 2048 | OpenGL texture-size cap |
| `[Display] Brightness` | Auto | OpenGL: how the brightness slider is applied (`Auto` shader pass, else monitor ramp; `Shader`; `Ramp`; `Off`) |
| `[Sound] Soundcard` | `DirectSound` | or `No Sound` |
| `[Debug] Diagnostics` | 0 | 1 = log dialogs, exits, access violations, large allocations, FPS and sleep statistics |
| `[Debug] CaptureEvery` | 0 | Seconds between back-buffer captures (`summonerfix-capture-N.bmp`) |
| `[Debug] FakeUptimeDays` | 0 | Testing only: the game sees this many days of Windows uptime (25+ reproduces the cutscene timer bug) |
| `[Debug] TestSaves` | 0 | Testing: 1 = save and load in `Summoner/savetest` (the `"savegame"` folder name at `0x594b00` becomes `"savetest"`), so the player's saves are never touched |
| `[Debug] ScriptEditorKey` | none | Key that toggles the built-in level script editor in game (for modders; its Save writes into the data folders) |
| `[Game] CameraZoomMax` | (empty) | Camera zoom-out limit; the game's is 1.0, Timeslip used 3 |
| `[Game] SkipIntroMovies` | 0 | 1 = skip the Volition and EAX logo movies at start-up (the credits movie still plays) |
| `[Game] InteractRange`, `ContainerRange` | (empty) | How far away you can talk to or use things, and open containers, in world units (game: 5) |
| `[ConsoleKeys]` | (empty) | `Key=command` lines, e.g. `F9=add_gold 1000`; commands are listed in [Console](../sum-exe/console.md) |
| `[Game] SpeedKey`, `SpeedFactor`, `SpeedMode` | `F11`, 3, `Toggle` | Run the game faster (1-8x): `Toggle` = press to switch on/off, `Hold` = while held; `none` disables the key |
| `[Game] ConfigKey` | `F10` | Opens `SummonerFixConfig.exe` while playing; `none` disables it |
| `[KeyBindings]` | (empty) | `PhysicalKey=GameKey` lines, e.g. `E=I`; key names or hex scan codes |
| `[Registry]` | | Values the game saved (`Version\*`, and later options) |

Settings the game asked for at startup that were not set: `CmdLineOptions`,
`ConnectionSpeed`, `PackedPixels`, `ComputerSpeed`, `D3D Optimizations` (Direct3D only).
The game uses its defaults for these.

## Test log

| Date | Config | Result |
|---|---|---|
| 2026-10-12 | OpenGL, fullscreen, 1920x1080 | Error: OpenGL only in windowed modes |
| 2026-10-12 | OpenGL, windowed, 1600x900 | Error: failed to allocate 1 GB |
| 2026-10-12 | Direct3D, fullscreen, 1920x1080 | Silent `exit(0)` from D3D init |
| 2026-10-12 | OpenGL, windowed, 1600x900, texture cap 2048 | **Main menu shown**; drawn at 1024x768 in the top-left corner |
| 2026-10-12 | Same, user play-test (about 5 min) | Sound and cutscenes work; image limited to 1024x768 top-left; display glitches over the uncovered area; game "super fast" |
| 2026-10-12 | OpenGL, borderless fullscreen 2560x1440, viewport scaling | Menu centred 4:3 with bars (frame capture). 218 FPS measured, 2x speed |
| 2026-10-12 | Same + FrameSleep (final) | **120.0 FPS real = 120.0 game**; 1:1 speed |
| 2026-10-12 | Same, user play-test | Everything works; game feels better |
| 2026-10-13 | Key bindings (E=I, A=Left, WASD camera, CapsLock/Home remaps) | Bindings work, including chain-attack modifiers (hold + right-click) |
| 2026-10-13 | Alt+Tab and F10 to `SummonerFixConfig.exe` while playing; rebind live | Works: the app comes forward and new bindings apply on return |

## Still to do

See the [Roadmap](../roadmap.md).
