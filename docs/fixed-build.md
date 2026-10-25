---
title: Fixed build changes
nav_order: 6.475
---

# Fixed build: the source fixes

**Status (2026-10-21):** 48 changes in the source under `FIXED_BUILD` (the fixed build of
[Fixed and experimental builds](builds.md)):
- every gameplay fix SummonerFix's `bugfix.c` makes, plus C317 (43);
- the timing and start-up layers a modern PC needs (G1 the 2x speed, G2 the QPC frequency, G3 the uptime cutscene
  timer, C1 the no-sound crash) and a 1024x768 default without the launcher (5, see "Modern-PC layers" below).

Each one is compiled and reviewed. **First in-game run (2026-10-21 evening, after Smart App Control was turned off;
the queued tests of `run-after-sac.ps1`):**
- `Sum-fixed.exe` starts, loads saves and plays; SummonerFix loads and leaves out every patch the source already has
  (the `.fixes` handshake: "fixed in this exe's source ... not patched" for the frame limiter, the cutscene timer,
  the no-sound crash and C317).
- **C317** (the Mist Elemental summoned as a cutscene starts): no crash, both on the original with SummonerFix's
  patch and on `Sum-fixed.exe` with the source fix.
- **Saves and memory** against the original, both with the fixes on (`link-savecheck`, Wolong2, WolongCaverns, sewer,
  lenele2aa, three original runs): the differences are the known kinds (positions of characters that walk, timer
  values a few milliseconds apart, values rolled from a clock-seeded random number). One looked new: six NPCs in
  lenele2aa with base max AP 7 instead of 9. A bisect showed it is random too: a build from before the four
  byte-match rounds gave 9 where that run's original gave 7.
- **Screens** (`link-screencheck`: main menu, the prologue at 12 s, inventory, character): equal apart from 4 pixels
  on the inventory screen, within what the original runs vary by elsewhere.

The per-fix reproductions in [Bugs in the original game](sum-exe/game-bugs.md) (steps per fix in
`src/decomp/NOTES-fixed-2026-10-21.md`) have not been run one by one yet.

## Modern-PC layers

SummonerFix also makes the game run on a current PC at all, with patches and hooks that are not in `bugfix.c`.
Where each one stands in the fixed build:

| SummonerFix layer | In the fixed build |
|---|---|
| 2x game speed (frame limiter) | **in the source** (G1): `FrameDone` waits on a high-resolution timer until one minimum frame after the frame's start and starts the next frame there; SummonerFix's `FrameSleep` patch stands down (`FrameLimiter` in `Sum-fixed.fixes`). Until 2026-10-25 it waited with `Sleep` and dropped late wake-ups, so with the window covered the game clock ran at 82% of real time ([Timing bugs](sum-exe/timing-bugs.md#the-fixed-builds-port-of-the-fix-lost-time-found-and-fixed-2026-10-25)) |
| QPC frequency above 4 GHz | **in the source** (G2): `timer_init` scales the counter and frequency down by the same power of two |
| Cutscenes skip after about 24.9 days of uptime | **in the source** (G3): the elapsed time is an unsigned difference in all three readers; SummonerFix's `timeGetTime` rebase stands down (`CutsceneTimer`) |
| Crash on Start New Game without sound | **in the source** (C1): the stream is only touched once `ds_init` finished; the guard patch stands down (`NoSoundGuard`) |
| Game resolution outside the three the game knows (e.g. 1366x768) | **in the source** (CustomResolution, 2026-10-24): `LoadSettings` puts the size into the 1024x768 slot of `resolution_table`, as SummonerFix patches the original. Before, the rebuilt exes fell back to 640x480 at such a size |
| No launcher needed | the game starts without the launcher's registry values (renderer and install path have defaults); **in the source**: 1024x768 by default instead of 640x480 |
| OpenGL renderer, windowed mode, texture-size cap | `REBUILT_OPENGL`, part of the `fixed` configuration |
| No CD | `SHIPPED_NOCD`, part of the `fixed` configuration |
| Movies under OpenGL | **in the source** (2026-10-25): `movie_blit_frame` copies Bink frames into the locked back buffer of Glide or Direct3D only, so under OpenGL every movie ended at once without a picture. The fixed build copies each frame into a 640x480 RGB565 bitmap and draws it centred (in the 1024-wide menu layout outside gameplay). **Observed:** the Volition and THQ logo movies play, centred, in the VC6 fixed build and the 64-bit exe |
| Fullscreen output and scaling, mouse mapping, 3D aspect, widescreen, the strip right of centred menus | not yet: SummonerFix's hooks still apply to the fixed build (they hook Windows and OpenGL calls, not game code) |
| Brightness under OpenGL, Print Screen above 1024 rows, black save thumbnails, key bindings | not yet: as above |

The rest is a larger piece of work: a native display layer for the fixed build (an OpenGL path that scales and
centres the 1024x768 picture itself) would replace most of the open rows at once.

## How a fix is written

```cpp
#ifdef FIXED_BUILD
    // FIXED_BUILD: <bug id> <[Fixes] key>
    <the fixed code>
#else
    <the original lines, unchanged>
#endif
```

- The original code stays byte for byte in the `#else` branch, so the reference build is untouched: the strict sweep
  still gives 5,445 matches, none lost, and the shipped `Sum-rebuilt.exe` is byte-identical to the one built before
  the fixes apart from its PE timestamp.
- One `// FIXED_BUILD: <bug> <key>` marker per fix; other blocks of the same fix say `// part of FIXED_BUILD <bug>`.
  `tools/link-build.mjs --config fixed` collects the markers into `Sum-fixed.fixes` next to the exe, and SummonerFix
  (`RebuiltFixInSource`, rebuilt mode) then leaves those patches out: a patch site's first bytes can survive the
  source fix while the code after it moved.
- A few fixes only add code (no original lines to keep) and have no `#else` branch: G6, G271, K300, C310 and helper
  blocks of K12, G100 and G170.
- `tools/fixed-compile.mjs <file>` compiles one unit as the fixed build does; the byte-matching tools never see
  `FIXED_BUILD`.

## The fixes

| Bug | Fix | `[Fixes]` key | Function | File |
|---|---|---|---|---|
| C1 | No crash on Start New Game without an audio device | `NoSoundGuard` | `ds_channel_release (0x55c5b0)` | `vsdk/unnamed_ds_0x55bf90.cpp` |
| C2 | No crash when 3D sound fails | `PrimarySoundBuffer` | `ds_init (0x55c1a0)` | `vsdk/unnamed_ds_0x55bf90.cpp` |
| C210 | AI healer counts members once | `AiBlessCount` | `ai_group_think_caster (0x4936c0)` | `game/unnamed_ai_groups_0x492b00.cpp` |
| C250 | No Luminar beam crash | `LuminarBeamTarget` | `ai_state::luminar_beam_hit (0x4ca3a0)` | `game/unnamed_luminar_0x4c8d10.cpp` |
| C272 | No crash in the Masad tutorial step | `EffectSearchGuard` | `fx_disable_by_effect_name (0x48d660)` | `game/unnamed_fx_0x489c40.cpp` |
| C310 | Game folder with a long path starts | `LongInstallPath` | `file_init (0x512d60)` | `vsdk/unnamed_vfile_0x5128b0.cpp` |
| G31 | A boss's move casts its spell only when the hit is in range (the Serpent's poison, the Ghost's fear) | `BossSpellInRange` | `living_combat::resolve_hit (0x467df0)` | `game/combat.cpp` |
| G252 | Murod Oni's backward bite waits 5 s between switches | `MurodBackBiteTimer` | `murod_oni_ai_select_action` | `game/unnamed_murod_oni_0x4c5c20.cpp` |
| G253 | Murod's leap and the Luminar's teleport skip dead or dying heroes | `BossAnchorsAlive` | `murod_start_leap`, `luminar_teleport_out` | `game/unnamed_murod_0x4c5140.cpp`, `game/unnamed_luminar_0x4c8d10.cpp` |
| C317 | No Mist Elemental crash at cutscenes | `SummonStashNoMove` | `summon_stash_for_cutscene (0x448000)` | `game/characterinfo.cpp` |
| G1 | The game runs at its real speed (no 2x) | `FrameLimiter` | `FrameDone (0x501510)` | `vsdk/frametime.cpp` |
| G2 | Normal speed with a 4+ GHz timer | `QpcFrequency` | `timer_init (0x532840)` | `vsdk/timer.cpp` |
| G3 | Cutscenes play after 24.9 days of Windows uptime | `CutsceneTimer` | `cutscene_sequence_update (0x436a90)` | `game/unnamed_cutscene_0x434900.cpp` |
| G4 | Ask before overwriting a save | `SaveOverwriteQuestion` | `saveload_try_save (0x426320)` | `game/new_saveload.cpp` |
| G6 | Zane's encounters as intended | `ZaneEncounter` | `ls_worldmap1_pick_encounter_stage4 (0x4fc4b0)` | `game/level_script_worldmap.cpp` |
| G8 | Control stays when a summon goes | `SummonKeepsLeader` | `party_remove_player (0x4335a0)` | `game/player.cpp` |
| G12 | Spell cooldowns from spells.tbl | `SpellRecoveryTimes` | `spells_parse_table (0x47a190)` | `game/spells.cpp` |
| G32 | AI sees mounted Phoenix / Ghost | `MountedRiders` | `boss_species_mounted_stage (0x4a2480)` | `game/boss_ai.cpp` |
| G40 | Gold piles keep their amount on load | `GoldPileAmount` | `savegame::read_item_objects (0x4d31e0)` | `game/level_scripts.cpp` |
| G100 | Leech drains HP | `LeechDrain` | `Character::get_hit_by_spell (0x45dea0)` | `game/living_entity.cpp` |
| G170 | Warnings keep the game paused | `MessageBoxPause` | `ui_message_box_show (0x413e40)` | `game/unnamed_interface_0x411270.cpp` |
| G210 | Summons notice enemies ahead | `DetectionCone` | `ai_group_collect_enemies (0x493ac0)` | `game/unnamed_ai_groups_0x492b00.cpp` |
| G211 | Support AI keeps its chosen enemy | `SupportAiTarget` | `ai_group_think_style1 (0x493290)` | `game/unnamed_ai_groups_0x492b00.cpp` |
| G213 | Serpent checks its new target asleep | `SerpentSleepTest` | `ai_state::serpent_choose_attack (0x496330)` | `game/unnamed_serpent_0x495290.cpp` |
| G215 | Phoenix Inferno hits each member once | `PhoenixInferno` | `phoenix_special_strike (0x499890)` | `game/unnamed_phoenix_0x497ac0.cpp` |
| G229 | AI healers heal when they should | `SupportSpellPick` | `ai_state::choose_support_spell (0x4bea80)` | `game/unnamed_ai_combat_0x4bce60.cpp` |
| G241 | Enemy area spells use real distances | `AreaSpellDistances` | `ai_state::combat_cast_frame (0x4bfaa0)` | `game/unnamed_ai_combat_0x4bce60.cpp` |
| G270 | Fire walls burn after a caster goes | `AreaCasterGone` | `fx_frame (0x48a230)` | `game/unnamed_fx_0x489c40.cpp` |
| G271 | AI fireball not blocked by old effects | `EffectSlotSpell` | `fx_new (0x48c3e0)` | `game/unnamed_fx_0x489c40.cpp` |
| G300 | Robe of Restoration heals its wearer | `RobeOfRestoration` | `living_combat::trigger_reflex_ability (0x467670)` | `game/combat.cpp` |
| G301 | Canis takes 10 gold, as offered | `CanisTenGold` | `ls_lenele2ab_action (0x4f1220)` | `game/level_script_lenele2ab.cpp` |
| K3 | Shop sell list category buttons | `ShopCategoryButtons` | `buysell_category_first_index (0x404f70)` | `game/new_buysell.cpp` |
| K4 | Identify list category buttons | `InventoryCategoryButtons` | `inventory_category_first_index (0x4153d0)` | `game/new_inventory.cpp` |
| K5 | Scrolled save list pictures | `SaveListThumbnail` | `saveload_load_thumbnail (0x425c10)` | `game/new_saveload.cpp` |
| K6 | 33rd map label is refused | `MapLabelLimit` | `automap_label_alloc (0x430230)` | `game/unnamed_worldmap_automap_0x42f1e0.cpp` |
| K12 | Container scrollbar stays in its track | `PickupScrollbar` | `pickup_update_scrollbar (0x420850)` | `game/new_pickup.cpp` |
| K20 | Hit effects on metal golems | `MetalHitEffects` | `combat_spawn_hit_vfx (0x4669c0)` | `game/combat.cpp` |
| K40 | Map label edits saved at 128 labels | `MapLabelsFull` | `savegame::automap_label_remove (0x4d1390)` | `game/level_scripts.cpp` |
| K61 | Loot flashes pale gold, not yellow | `LootHighlightColour` | `world_tint_gold_object (0x43a600)` | `game/gametarget.cpp` |
| K64 | Full textures on a fresh install | `OpenGLTextures` | `gr_get_video_memory_mb (0x5079f0)` | `vsdk/gr.cpp` |
| K80 | "Knocked Down!" only when the knock-down happens | `KnockDownMessage` | `ai_state::knock_down_local (0x4742b0)` | `game/ai.cpp` |
| K100 | Ring summons from 60,000 power | `RingSixtyThousand` | `character_can_use_summon (0x446e90)` | `game/characterinfo.cpp` |
| K110 | Ambient sounds survive level changes | `AmbientSounds` | `level_script_leave (0x4ccc70)` | `game/level_scripts_common.cpp` |
| K130 | Boss music starts and ends | `BossMusic` | `ls_liangshan_init (0x4f4930)` | `game/level_script_liangshan.cpp` |
| K170 | Popup rows light up under the cursor | `PopupRowOffset` | `ui_choice_popup_hit_test (0x413970)` | `game/unnamed_interface_0x411270.cpp` |
| K191 | No edge turn right after a drag | `EdgeTurnAfterDrag` | `camera_process_input (0x434660)` | `game/unnamed_input_0x4343a0.cpp` |
| K193 | Screenshots never overwrite each other | `ScreenshotNumbers` | `screenshot_save (0x4324f0)` | `game/unnamed_frame_0x4323e0.cpp` |
| K300 | Spell hit sounds where spells land | `SpellHitSoundPosition` | `spell_cast_execute (0x47d348)` | `game/spells.cpp` |
| launcher | 1024x768 by default when the launcher wrote no settings | `ResolutionDefault` | `LoadSettings (0x43d7f0)` | `game/summoner.cpp` |
| K260 | World sounds follow the volume slider | `WorldSoundVolume` | `snd_play_3d (0x521480)` | `vsdk/unnamed_snd_0x5204c0.cpp` |
| K292 | Conversation more/end hover | `ConversationHover` | `dialogue_topic_entry::draw (0x4d6e70)` | `game/unnamed_dialogue_box_0x4d6660.cpp` |

## Notes from the port

- Each fix reproduces SummonerFix's patch (its `bugfix.c` comment and stub are the specification), written as plain
  C++ rather than as the patch's bytes. Where the source form differs, it is noted in the workers' notes: K3 stops at
  the end of the sell list instead of a fixed 521 rows; K193 numbers screenshots from the file list the function
  already builds instead of searching the folder again; C310 keeps the long path when the short one is not shorter.
- Not changed, as SummonerFix also leaves them: the offensive armour branch of `trigger_reflex_ability`, and the
  128-byte buffers behind C310 (a volume without 8.3 names still crashes with a long path).
- Raised during the port and checked afterwards (2026-10-21):
  - `ai_group_think_caster`'s target list holds **six** records, and an AI group of six fits (corrected later on
    2026-10-21; D17100). A first reading put the records at `esp+0x38`, which left room for five and made a
    six-member enemy group with a caster a crash candidate. The member loop at `0x4937bc` writes each record's
    character at `[eax-8]` from `esp+0x38`, so the records start at `esp+0x30` and six end exactly at the
    return address (`esp+0x90`). The source had `list[5]` and now has `list[6]`, which gives the binary's frame
    size. The shipped build's behaviour is unchanged, and the candidate is withdrawn. The overflow that remains is
    C210's doubled count, fixed here.
  - K61's out-of-bounds read: in both exes the four bytes after `world_tint_gold` are the text "gold"
    (`67 6f 6c 64`), read as a float, so the shipped rebuild tints exactly as retail does.
