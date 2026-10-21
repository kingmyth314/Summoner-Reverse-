---
title: Fixed build changes
nav_order: 6.475
---

# Fixed build: the source fixes

**Status (2026-10-21):** 43 of the game's bugs are fixed in the source under `FIXED_BUILD` (the fixed build of
[Fixed and experimental builds](builds.md)): every gameplay fix SummonerFix's `bugfix.c` makes, plus C317. Each one
is compiled and reviewed; **none has run in game yet.** Windows Smart App Control began blocking the SummonerFix DLL
on this PC the same morning, and every in-game test goes through SummonerFix's agent mode. The tests to run are the
reproductions in [Bugs in the original game](sum-exe/game-bugs.md); the workers' notes with the steps per fix are in
`src/decomp/NOTES-fixed-2026-10-21.md`.

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
| C2 | No crash when 3D sound fails | `PrimarySoundBuffer` | `ds_init (0x55c1a0)` | `vsdk/unnamed_ds_0x55bf90.cpp` |
| C210 | AI healer counts members once | `AiBlessCount` | `ai_group_think_caster (0x4936c0)` | `game/unnamed_ai_groups_0x492b00.cpp` |
| C250 | No Luminar beam crash | `LuminarBeamTarget` | `ai_state::luminar_beam_hit (0x4ca3a0)` | `game/unnamed_luminar_0x4c8d10.cpp` |
| C272 | No crash in the Masad tutorial step | `EffectSearchGuard` | `fx_disable_by_effect_name (0x48d660)` | `game/unnamed_fx_0x489c40.cpp` |
| C310 | Game folder with a long path starts | `LongInstallPath` | `file_init (0x512d60)` | `vsdk/unnamed_vfile_0x5128b0.cpp` |
| C317 | No Mist Elemental crash at cutscenes | `SummonStashNoMove` | `summon_stash_for_cutscene (0x448000)` | `game/characterinfo.cpp` |
| G4 | Ask before overwriting a save | `SaveOverwriteQuestion` | `saveload_try_save (0x426320)` | `game/new_saveload.cpp` |
| G6 | Zane's encounters as intended | `ZaneEncounter` | `ls_worldmap1_pick_encounter_stage4 (0x4fc4b0)` | `game/level_script_worldmap.cpp` |
| G8 | Control stays when a summon goes | `SummonKeepsLeader` | `party_remove_player (0x4335a0)` | `game/player.cpp` |
| G12 | Spell cooldowns from spells.tbl | `SpellRecoveryTimes` | `spells_parse_table (0x47a190)` | `game/spells.cpp` |
| G32 | AI sees mounted Phoenix / Ghost | `MountedRiders` | `boss_species_mounted_stage (0x4a2480)` | `game/boss_ai.cpp` |
| G40 | Gold piles keep their amount on load | `GoldPileAmount` | `savegame::read_item_objects (0x4d31e0)` | `game/level_scripts.cpp` |
| G100 | Leech drains HP | `LeechDrain` | ` (0x45dea0)` | `game/living_entity.cpp` |
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
| K260 | World sounds follow the volume slider | `WorldSoundVolume` | `snd_play_3d (0x521480)` | `vsdk/unnamed_snd_0x5204c0.cpp` |
| K292 | Conversation more/end hover | `ConversationHover` | `dialogue_topic_entry::draw (0x4d6e70)` | `game/unnamed_dialogue_box_0x4d6660.cpp` |
| K300 | Spell hit sounds where spells land | `SpellHitSoundPosition` | `spell_cast_execute (0x47d348)` | `game/spells.cpp` |

## Notes from the port

- Each fix reproduces SummonerFix's patch (its `bugfix.c` comment and stub are the specification), written as plain
  C++ rather than as the patch's bytes. Where the source form differs, it is noted in the workers' notes: K3 stops at
  the end of the sell list instead of a fixed 521 rows; K193 numbers screenshots from the file list the function
  already builds instead of searching the folder again; C310 keeps the long path when the short one is not shorter.
- Not changed, as SummonerFix also leaves them: the offensive armour branch of `trigger_reflex_ability`, and the
  128-byte buffers behind C310 (a volume without 8.3 names still crashes with a long path).
- Found during the port, outside the fixes: `ai_group_think_caster`'s target list has 5 records in our source and 6
  in the binary (its frame), while an AI group can hold 6 members; and K61's original out-of-bounds read means the
  shipped rebuild shows whatever the linker put after the colour table. Both are checks for the reference build.
