# FIXED_BUILD round, 2026-10-21

Fourteen agents ported SummonerFix's [Fixes] patches into the source as `#ifdef FIXED_BUILD` blocks, one file set each
(docs/builds.md). Their notes follow, one section per worker, unedited apart from this header. None of these fixes
has run in game yet: Windows Smart App Control began blocking the SummonerFix DLL on 2026-10-21, so the test harness
could not start. The reproduction steps in each note are the in-game tests still to do.

## Worker 1

### FIXED_BUILD ports: RobeOfRestoration, MetalHitEffects, PhoenixInferno

Checks (both files): the shipped build's vc6-compare output is identical before and after the edits
(combat.cpp 52 match / 11 differ of 63; unnamed_phoenix 33 match / 3 differ of 36; the three touched functions keep
their status: combat_spawn_hit_vfx match, trigger_reflex_ability differs as before, phoenix_special_strike match).
fixed-compile: both files compile with FIXED_BUILD. combat.cpp has no diagnostics; unnamed_phoenix has three C4244
warnings from include/vsdk/math/vector.h and include/vsdk/geom/vfx.h, and the unedited tree gives the same three.

## G300 RobeOfRestoration

- Function: `living_combat::trigger_reflex_ability` (`combat_trigger_reflex_ability`, 0x467670), game/combat.cpp.
- Bug id: the fix list said "?". bugfix.c, game-bugs.md and docs/summonerfix/index.md all call it G300, so the
  marker is `// FIXED_BUILD: G300 RobeOfRestoration`.
- Source fix: in the armour-triggered-effect loop, the beneficial branch's `COMBAT_TRIGGER_CAST` gets
  `opponent->owner->handle` (the wearer) as the last argument, which is the single-player `spell_cast_execute` target. It was
  `owner->handle` (the attacker). The whole macro call is inside the #ifdef because a preprocessor directive cannot
  go inside a macro's argument list.
- Compared with the patch: the patch turns `mov ecx, [ebp]` at 0x467b0f into `mov ecx, eax` (eax = the wearer), so
  the handle read at 0x467b17 is the wearer's. Same behaviour. The multiplayer and "+Hit only" paths of the macro do
  not use that argument, so only the single-player spell_cast_execute target changes, as with the patch.
- Left alone, as the patch leaves it: the offensive armour branch also crosses its arguments (its single-player target is the
  defender's own handle, with the attacker's position; the "K300 candidate"). No retail armour has an offensive
  triggered effect.
- In game (from game-bugs.md / bugfix.c): give Rosalind the Robe of Restoration (`+Triggered Effect: "heal" 20 20`) and
  let enemies hit her. In the original the Heal lands on the enemy that hit her (seen with `[Debug] TraceArmourSpells`).
  With the fix it lands on Rosalind.

## K20 MetalHitEffects

- Function: `combat_spawn_hit_vfx` (0x4669c0), game/combat.cpp.
- Source fix: after the weapon tag test, skins 1 and 2 (flesh, leather) spawn `blood.vfx` and return. Every other skin
  that got a name in the switch (3, 9..16: stone, ice, the metals) spawns `sparkhit01.vfx`.
- Compared with the patch: the patch makes `cmp edi, 1 ; jne` into `cmp edi, 2 ; ja` (blood for skins 0..2; only 1 and 2
  reach it because skin 0 gets no name), and replaces the stone/ice test with a jmp to the spark spawn. Same behaviour.
  `sparkhit.vfx` (the switch's name) is still never spawned, in both the patch and the source. The switch result is
  still used only as "has an effect".
- In game: sewer boss room (`play_and_load_cutscene TB-Intro`), `[Debug] TraceHitVfx`. Joseph's hits on the Brass /
  Broken Brass / Steel Golems (bronze, skin 12) and the Iron Golem show no effect in the original. With the fix they show
  sparkhit01.vfx, as stone golems already do. No retail character is leather, so that half shows nothing new.

## G215 PhoenixInferno

- Function: `phoenix_special_strike` (0x499890), game/unnamed_phoenix_0x497ac0.cpp, case SPELL_PHOENIX_BLAST (0x1e).
- Source fix: in the party loop, the distance is `c->pos - boss->pos` (the loop member, not the target), and the effect's
  target handle and the `get_hit_by_spell` this are `c`. The knock-down on `c` was already per member. Each member within
  the spell's distance is hit and knocked down once.
- Compared with the patch: the patch changes esi (target) to ebx (the loop member `c`) in the three distance loads
  (0x499900/06/0c), the effect target (0x499933) and the hit's this (0x499951). The source makes the same five changes.
  The `target` parameter is now unused in this case, which is harmless. The boss's own handle is still the hit's
  source, as before.
- The #ifdef block opens the `if (...) {` in both branches and the closing brace is shared after `#endif`. The
  braces balance in either configuration.
- In game: agent mode `full_party`, then `play_and_load_cutscene PHNX-Intro`, `[Debug] TracePhoenixInferno`. In the
  original one Inferno hits the target (Flece) once per party member (four times) and knocks the whole party down
  wherever it stands. With the fix each member within 6 m (spells.tbl "inferno" `+Distance:` 6) is hit once and knocked
  down, and members out of range are not affected. index.md notes that the fight becomes easier than in the original.

## Worker 2

### FIXED_BUILD ports (w2)

Checks for all three: `node tools/vc6-compare.mjs` on game/new_saveload.cpp (36 match, 1 differ) and
game/unnamed_serpent_0x495290.cpp (38 match, 5 differ) gives output identical before and after the edits
(before-*.txt / after-*.txt in my work folder). `node tools/fixed-compile.mjs` says "compiles with FIXED_BUILD" for
both files. The only diagnostics are three C4244 warnings from include/vsdk/math/vector.h and include/vsdk/geom/vfx.h,
which come from the headers and not from these edits.

## G4 SaveOverwriteQuestion

- Function: `saveload_try_save` (0x426320), game/new_saveload.cpp.
- Source fix: the overwrite test uses `entry = saveload_selected_row + saveload_scroll_pos` for the `>= 0` test and
  for `saveload_entries[entry].level != -1`. That is the entry `saveload_save_to_slot` writes to.
- Compared with SummonerFix: the patch turns the `mov eax, [saveload_selected_row]` at 0x426340 into a call that
  returns selected_row + scroll_pos. The `test eax, eax ; jl` that follows and the index both use that value.
  The source does the same.
- In game: make enough saves to scroll the Save list, scroll it down (3 rows in the repro), click a save on the
  top visible row and press Save. Shipped: "Game Saved" appears at once and the save is overwritten. Fixed: the
  "Overwrite existing savegame?" question appears, and No leaves the file as it was. `<New Savegame>` (list entry 0)
  still saves with no question. The rows below the top one now ask about the save they will overwrite.
- Uncertain: nothing. The earlier `saveload_selected_row < 0` return means `entry >= 0` always holds when
  scroll_pos >= 0. I kept the test because the patch keeps it.

## K5 SaveListThumbnail

- Functions: `saveload_load_thumbnail` (0x425c10) and `saveload_draw_thumbnail` (0x426fd0), game/new_saveload.cpp.
  The marker is in load_thumbnail. The draw block carries "// part of FIXED_BUILD K5".
- Source fix: load_thumbnail reads the save's thumbnail file unless save mode is on and `e` is
  `&saveload_entries[0]` (`<New Savegame>`). Before, it tested `row > 0`, the visible row. draw_thumbnail uses the
  captured picture when `saveload_selected_row + saveload_scroll_pos == 0`. Before, it tested `saveload_selected_row == 0`.
- Compared with SummonerFix: the load patch's stub returns flags from `cmp saveload_entries, edx` (edx = the
  entry pointer), and the jle skips the read for entry 0. In load mode it always reads. The draw patch reuses the G4
  stub (row + scroll). The source does the same.
- In game: make enough saves to scroll the Save list. Move the party so the scene differs from every save picture.
  Open the Save screen, scroll 2 rows and select the top visible row. Shipped: the current scene is shown. Fixed:
  that save's picture is shown, and `<New Savegame>` (scroll back to the top) still shows the current scene.
- Uncertain: if draw_thumbnail runs while `saveload_selected_row` is -1 and the list is scrolled exactly 1 row, the
  sum is 0 and the captured picture is drawn. Shipped draws `savegame_thumbnail_bm` there. The patch does the same
  as the source, so I kept it. I did not check whether the draw is reached with no row selected.

## G213 SerpentSleepTest

- Function: `ai_state::serpent_choose_attack` (0x496330), game/unnamed_serpent_0x495290.cpp. This is the stage 1
  normal-attack path, after the `boss_target_list_pick` re-pick.
- Source fix: `asleep = serpent_part_is_asleep(list, cur);` gets the new candidate index from
  `boss_target_list_find_b`. Before, it passed the previous `asleep` flag.
- Compared with SummonerFix: the patch replaces `mov eax, [esp+0x2c] ; and eax, 0xff ; push eax` at 0x49667f with
  `push esi`, and esi holds the find_b result, which is `cur` in the source. The two do the same thing.
- In game: the mounted Serpent fight (stage 1) in Rand-Forest01_v2 with a full party. `[Debug] TraceSerpentAsleep`
  in SummonerFix logs the index passed and the new candidate. Shipped: after a re-pick the logged index was 0 while
  the candidate was 2. Fixed: they match. To see it in play, a sleeping new target (put to sleep by the Serpent's
  Nightmare) should no longer get the Rider Stun knockback, and an awake one should no longer be left alone.
- Uncertain: if find_b returned -1 here (the picked handle not in the list), the fixed call would get -1. The patch
  passes esi in the same case, so the behaviour is the same. pick returns a handle from the same list, so -1 is not
  expected.

## Worker 3

### FIXED_BUILD ports (w3)

Checks for all three: `tools/vc6-compare.mjs` output of both files is identical before and after the edits
(level_scripts.cpp 0x4d1390 still "differs" as it did before, 0x4d14f0 and 0x4d31e0 still match; the ds file
56 match / 4 differ, unchanged). `tools/fixed-compile.mjs` says "compiles with FIXED_BUILD" for both files; the
warnings it lists come from headers (vector.h, vfx.h) and from level_scripts.cpp lines 843/845 (unused locals in
another function), none from the new code.

## GoldPileAmount (G40)

- Function: `savegame::read_item_objects` (0x4d31e0), game/level_scripts.cpp.
- Source fix: in the loose gold pile loop, the pile is spawned with the amount saved after z
  (`*(int *)(item_objects.data + off - 4)`, read after the `off += 8` that skips z and the amount), or 1 if that
  value is not positive, instead of the constant 1. Reuses the existing local `num`.
- Compared with the patch: the same. The stub reads `[save buffer + esi - 4]` (esi = offset after `add esi, 8`,
  buffer = `[edi+0x1034]`, which is `item_objects.data` in the source) and pushes it, or 1 if not > 0.
- In game: make a loose gold pile (`goldpile` console command, or a monster that drops only gold), do not pick
  it up, save, load, pick it up: the shipped game gives 1 gold, the fixed build the pile's amount.
- Uncertain: nothing; `item_objects.data` being the buffer at `+0x1034` is taken from the function's own reads.

## MapLabelsFull (K40)

- Functions: `savegame::automap_label_remove` (0x4d1390) and `savegame::automap_labels_reset_level` (0x4d14f0),
  game/level_scripts.cpp. Marker in the remove; the reset block carries "part of FIXED_BUILD K40".
- Source fix: the `if (count >= SAVEGAME_MAX_LABELS) return;` test is left out of both functions. The add
  (`automap_label_add`) keeps its own test.
- Compared with the patch: the same (the patch nops both `jge`). Removing from a full list only shrinks it, so
  there is no overflow risk.
- Note: an edit (remove, then add) at 128 labels now works too, because the remove frees a slot before the add.
- In game: have 128 map labels saved over all levels, delete (or edit) one, leave the level and come back (or
  save and load): in the shipped game the old label returns, in the fixed build it stays deleted / edited.
  The fix was checked by function call in SummonerFix, not by a full in-game run.

## PrimarySoundBuffer (C2)

- Function: `ds_init` (0x55c1a0), vsdk/unnamed_ds_0x55bf90.cpp.
- Source fix: when the first primary `CreateSoundBuffer` fails, DirectSound is no longer released first. With
  3D on, the buffer is created again without 3D (`DSBCAPS_PRIMARYBUFFER`, ds3d and EAX off) on the same
  DirectSound object; if that succeeds, ds_init goes on (SetFormat and the rest). If 3D was off, or the retry
  fails, DirectSound is released, set to NULL, and ds_init returns -1 (no sound).
- Compared with the patch: the same behaviour as `DsPrimaryRetry` + its stub (retry on the live object, continue
  at 0x55c337 on success, else release and leave through `or eax, -1` at 0x55c32c). The patch also tests that
  the DirectSound pointer is not NULL; in the source it cannot be NULL at that point (SetCooperativeLevel was just
  called through it), so the test is left out. The patch logs its outcome; the source fix does not (the
  function's other failures write only to a local `msg`).
- In game: needs a sound card that refuses the primary buffer. SummonerFix's `[Debug] FakePrimaryBufferFailure=1`
  makes the first attempt fail; with `Soundcard=EAX` the shipped game crashes at start-up, with the fix it starts
  with sound without 3D. The fixed build has no such fake switch, so seeing it in game needs that hook (or a
  debugger forcing the HRESULT).

## Worker 4

### FIXED_BUILD ports: AreaSpellDistances, SupportSpellPick, SpellHitSoundPosition, SpellRecoveryTimes

Checks (both edited files): the shipped vc6-compare output is identical before and after (byte-identical
listing, so every function's status is unchanged); fixed-compile says "compiles" for both, with only the three
header warnings (vsdk/math/vector.h(73), vsdk/geom/vfx.h(550/551)) that the unedited files give too. No header
was changed. vsdk/unnamed_vector_0x523f30.cpp (distance) needed no change: the fix only calls distance().

## G241 AreaSpellDistances

- Function / file: ai_state::combat_cast_frame (0x4bfaa0, ai_combat_cast_frame), game/unnamed_ai_combat_0x4bce60.cpp.
- Source fix: in the offensive-candidate loop for a caster outside the party (the `cur_character` group branch), each
  candidate record also gets `dist = distance(&owner->pos, &c->pos)`, as the healer list above it does.
  ai_consider_area_spell (meteor storm, inferno) reads that field.
- Versus the patch: same behaviour. The stub at 0x4bfdbd stores the candidate and `distance(caster +0x10, target
  +0x10)` at +4; ecx/eax/edx are restored, so nothing else changes. The party-member branch (which also stores only
  the character) is left alone, as in the patch (ai_consider_area_spell is used only for non-party casters).
- In game: put a party member next to an Orenian Mage ("Generic Mage", knows meteor storm and inferno), e.g. in
  Rand-Forest01; with SummonerFix's `[Debug] TraceAreaSpell` the original shows unwritten stack values (6.11e-41) in
  the distance field; with the fix the field is the real distance. The visible effect needs two party members
  within range (the two-target test).
- Uncertain: nothing beyond the patch's own "inferred effect" status in game-bugs.md.

## G229 SupportSpellPick

- Function / file: ai_state::choose_support_spell (0x4bea80, ai_choose_support_spell),
  game/unnamed_ai_combat_0x4bce60.cpp. Covers both callers (ai_group_think_caster and ai_combat_cast_frame), as the
  patch does.
- Source fix: after `r = frand()`, one pass computes the running-sum pick (first i with r <= sum of shares; if none,
  the last candidate with a positive share), and the game's own pick (first i with r <= its own share). A list whose
  first record is a party member (type 7) takes the running-sum pick; otherwise the game's own pick, and when that is
  none a second frand() < 0.5 decides between the running-sum pick and nobody. The take is the original's: heal for
  a hurt record, cure for a curable one, else -1.
- Versus the patch: same behaviour as SupportPickRunning plus the take branch at 0x4beb6e, including the extra
  frand() call only in the enemy "nobody picked" case (it consumes one random number, as the patch does). The
  `[Debug] TraceSupportPick` log line of the DLL is not reproduced (debug only).
- In game: the C210 test save (Joseph, Rosalind, Flece, Jekhar with 3000 HP each in IkaemosExt), Rosalind on AI
  with two hurt members: the original casts nothing about half of the time; with the fix the party healer heals
  each time it decides to. `[Debug] TraceSupportPick=1` in SummonerFix logs the shares and picks.
- Uncertain: the "party side" test looks at the first candidate's type, as the patch does (the list's first record).

## K300 SpellHitSoundPosition

- Function / file: spell_cast_execute (0x47c5d0), game/spells.cpp.
- Source fix: at the start of the single-player path (right after `spell_vec where;`), `where` is set to the target
  character's position (target handle != -1 and living_from_handle finds it), else `*pos` when pos is non-NULL and
  not (0, 0, 0), else the caster's position. The cases that set `where` later still overwrite it.
- Versus the patch: same order and tests as HitSoundStartPos (stub at 0x47c74c). This is a pure insertion, so the
  block has no #else (there are no original lines to keep).
- In game: Lenele, a character casting curse, wrath of god, a meteor storm or banish summon (e.g. through the AI
  goals); in the original the hit sound comes from a leftover position (often out of earshot), with the fix from
  where the spell lands. SummonerFix's `[Debug] TraceHitSound` logs the position.
- Uncertain: none beyond the patch's; the existing source comment calls the bug "K301 candidate" (the old
  numbering).

## G12 SpellRecoveryTimes

- Function / file: spells_parse_table (0x47a190), game/spells.cpp.
- Source fix: `+Recovery time:`, `+AI Recovery time:` and `+Loop Time:` store `(short)tf.ParseInt()` instead of
  `(short)i`. Three #ifdef blocks: the first carries the marker, the other two "part of FIXED_BUILD G12".
- Versus the patch: identical (mov [esi-N], bp -> mov [esi-N], ax: the low 16 bits of the parsed value).
- In game: fireball's cooldown is 20 s (spells.tbl), not 26 s (its record number); also wall of fire 20 not 29,
  blizzard 25 not 35, and the AI recovery times (curse 10 not 23 ...). Changes the balance.
- Uncertain: nothing.

## Worker 5

### FIXED_BUILD ports, batch w5 (2026-10-21)

Checks (VC6 SP5, VC6_STRICT=1, my copy's include and vc6-flags.txt):
- shipped build: `tools/vc6-compare.mjs` output before and after the edit is identical for all three files
  (no function changed status).
- fixed build: `tools/fixed-compile.mjs` says "compiles with FIXED_BUILD" for all three; the only diagnostics are
  the existing C4244 warnings from include/vsdk/math/vector.h(73) and include/vsdk/geom/vfx.h(550-551), none from
  the new code.

## ConversationHover (K292)

- Function: `dialogue_topic_entry::draw` (0x4d6e70), game/unnamed_dialogue_box_0x4d6660.cpp.
- Source fix: in cases 5 ("more") and 6 ("end") the last term of the hover test, `mx < dialogue_topic_box.y + h`,
  becomes `my < dialogue_topic_box.y + h`. Two #ifdef blocks: the marker `// FIXED_BUILD: K292 ConversationHover`
  on "more", `// part of FIXED_BUILD K292` on "end". Each block wraps only the `if` condition.
- Against SummonerFix: the patch replaces `mov edx, [esp+h] ; add ecx, edx ; cmp eax, ecx` (eax = mouse x) with
  `add ecx, [esp+h] ; cmp [esp+y], ecx` at 0x4d6fe8 and 0x4d707c, keeping the jump: the same test with the mouse
  y. Same behaviour.
- In game: Canis's conversation in Lenele2ab, ask "Tavern?" (the answer ends in "More"). Shipped: with the mouse
  far below the conversation panel, "More" is drawn in the highlight colour. Fixed: it lights only with the mouse
  on the text.
- Uncertain: nothing.

## ZaneEncounter (G6)

- Function: `ls_worldmap1_pick_encounter_stage4` (0x4fc4b0), game/level_script_worldmap.cpp.
- Source fix: a `return;` after the Zane case's two strcpy calls (`rand-grassland01` / `rand-grassland01_v3`), as
  the Korel case returns. Insert-only block (no original lines are replaced, so no #else).
- Against SummonerFix: the patch jumps from the end of the Zane copy (0x4fca1b) to a stub that finishes the copy and
  leaves through the function's epilogue (0x4fcae1). Same behaviour: the general grassland rolls are skipped.
  Like the shipped Zane case (and the Korel case), it does not set level_prev_filename; the ring-boss cases do.
- In game (agent mode, from game-bugs G6): `level worldmap1 worldmap1`, `navgoto $grassland001`, set `unhide_zane`
  and clear the ring-boss and Korel cases with `flagl`, then `call0 4fc4b0 40` (the picker 40 times, logging the
  names). Shipped: rand-grassland01 / _v3 about 10 % (4 in 40). Fixed: about 55 % (SummonerFix gave 19 in 40).
- Uncertain: nothing.

## LongInstallPath (C310)

- Function: `file_init` (0x512d60), vsdk/unnamed_vfile_0x5128b0.cpp.
- Source fix: after both branches have set install_path (with its trailing backslash), and before
  `file_zero_vector.zero()` (the patch point 0x512e0b), a path of 64 characters or more is replaced by its 8.3
  form from GetShortPathNameA, given a trailing backslash if it lacks one. It is kept when GetShortPathNameA fails,
  when the result does not fit MAX_PATH - 1, or when it is not shorter. Insert-only block (no #else).
- Against SummonerFix: the stub calls ShortenInstallPath with the same threshold (64) and the same conditions, then
  does the replaced `mov ecx, file_zero_vector`. Same behaviour, except the logging: SummonerFix writes to its log
  when the path is shortened, when it cannot be shortened, and when the short form is still 116 characters or
  more. The source fix logs nothing (I did not look for a log or console that is usable this early in
  file_init); a message could be added if wanted.
- Note (not changed): the root cause stays, the 128-byte sprintf buffers in packfile_add and packfile_fopen.
  A short form of 116 characters or more, or a volume without 8.3 names, still crashes, as with SummonerFix.
  A fuller source fix would size those buffers (or use _snprintf); that is outside this patch's behaviour.
- In game (game-bugs C310): put the game folder behind a directory junction whose path, with the trailing `\`,
  is 116 characters or more (the reproduction used 125) and start the exe from it. Shipped: an access violation
  during start-up. Fixed: starts normally (on a volume with 8.3 names).
- Uncertain: GetShortPathNameA comes from kernel32 (windows.h is already included in this file), so the fixed exe
  gains one import; fine for the fixed build.

## Worker 6

### FIXED_BUILD ports (w6): CanisTenGold, MountedRiders, KnockDownMessage

Checks (all three files): vc6-compare shipped output is identical before and after the edit (before-*.txt /
after-*.txt in my work folder, 0 differing lines); fixed-compile says "compiles" for each. The only warnings are
from headers (vector.h, vfx.h) and, in boss_ai.cpp, lines 297, 311 and 1544, none of them in the new code.

## G301 CanisTenGold

- Function / file: `ls_lenele2ab_action` (0x4f1220), game/level_script_lenele2ab.cpp.
- Source fix: in the "payoff" branch, `party_gold -= 10;` in place of `party_gold -= 100;`. The clamp at 0 after
  it stays as it is.
- Compared with SummonerFix: same behaviour. The patch changes `sub eax, 0x64` to `sub eax, 0x0a` at 0x4f124d.
- In game: occupied Lenele (`level Lenele2ab Lenele2ab`), talk to Canis the Mercenary: Hail ... "Massacre?" ...
  "Give him 10 gold pieces." With 500 gold the shipped game leaves 400 and the fixed build should leave 490.
- Uncertain: we do not know which amount the designers meant (game-bugs.md). The fix follows the text the player
  picks, as the patch does.

## G32 MountedRiders

- Function / file: `boss_species_mounted_stage` (0x4a2480), game/boss_ai.cpp.
- Source fix: the switch reads each rider's own stage. Tiger is `tiger_stage == 1 || == 2` (the shared case is
  unchanged, outside the block). Phoenix is `phoenix_stage == 1`. Serpent is `serpent_stage == 1`. Ghost
  (`SPECIES_GHOST_RIDER`) is `ghost_stage == 1`. Every other species still gets false.
- Compared with SummonerFix: same behaviour as the replacement function `MountedStage` (bugfix.c), which tests
  species 0x57 to 0x5a against the stage variables at 0x2452a90, 0x2452420, 0x2451d98 and 0x2480310. In the
  source these are the existing macros `tiger_stage`, `phoenix_stage`, `serpent_stage` and `ghost_stage`
  (ghost.stage). The patch returns an int in eax and the source returns a bool. Callers use the result as a bool,
  so nothing changes.
- In game: the mounted Phoenix fight (rand-hills01, stage 1) with no Serpent stage 1 left over. Melee party
  members should now fight at range. The Ghost Rider's mounted stage is also treated as mounted. The direct check
  from game-bugs.md: call the function with species 0x58 while `phoenix_stage` = 1 and `serpent_stage` = 0, and
  with species 0x5a while the ghost stage = 1. Both should give "mounted".
- Uncertain: "Ghost mounted = stage 1" comes from bugfix.c and bosses.md. I did not check it again.

## K80 KnockDownMessage

- Function / file: `ai_state::knock_down_local` (0x4742b0), game/ai.cpp.
- Source fix: the "Knocked Down!" message is shown only when `set_mode(AI_STATE_DISABLED, AIS_TRIPPED_FALL)`
  returns non-zero and the owner is `cur_character`. The function still returns 1. The stored duration and the
  timer invalidation do not change.
- Compared with SummonerFix: same behaviour. The patch's stub makes the cur_character test fail when eax (the
  set_mode result) is 0. Because the function still returns 1, `ai_knock_down` still sends
  `mp_broadcast_ai_disable` in multiplayer even when the local knock-down was refused. The patch keeps that
  behaviour on purpose, and so does this port. Pushes and stuns that are dropped while casting are not touched
  (they show no message).
- In game: give the selected character a spell to cast (for example Joseph casting a heal) and have a melee hit
  knock it down during the cast. The shipped game shows "Knocked Down!" while the character keeps casting on its
  feet. The fixed build should show nothing in that case, and should still show the message when a character that
  is not casting falls.

## Worker 7

### FIXED_BUILD ports (batch w7)

Checks (both files of BossMusic, and gametarget.cpp):
- Shipped build: `tools/vc6-compare.mjs` output before and after the edits is identical, line for line
  (before-*.txt / after-*.txt in my work folder).
- Fixed build: `tools/fixed-compile.mjs` says "compiles with FIXED_BUILD" for all three files. The only diagnostics
  are C4244 warnings in include/vsdk/math/vector.h(73) and include/vsdk/geom/vfx.h(550-551), which are header
  warnings that every file gets. None come from the new code.

## BossMusic (K130, with K150)

- Functions: `ls_liangshan_init` (0x4f4930, game/level_script_liangshan.cpp) and `ls_ikaemostopint_init` (0x4f9ec0,
  game/level_script_ikaemosu.cpp).
- Source fix: Liangshan sets `ls_liangshan_phoenix_dead_done` to
  `get_flag("phoenix_dead") && get_flag("serpent_dead")` instead of `true`. The Ikaemos upper hall sets
  `ls_ikaemostopint_found_boss_done` and `ls_ikaemostopint_boss_killed_done` to `get_flag("ikaemos_boss_killed") != 0`
  instead of `true`. The `ls_ikaemostopint_joseph_spoke = false` line is left as it is. The marker
  `// FIXED_BUILD: K130 BossMusic` is in the Liangshan file. The Ikaemos block carries
  `// part of FIXED_BUILD K130`, because the key has only one marker and the fix list gives K130 as its id.
- Compared with SummonerFix: the patch replaces the `mov byte [phoenix_dead_done], 1` at 0x4f49be, and the two
  `mov [..], bl` at 0x4f9f2e/0x4f9f34, with calls to `LiangshanMusicPreset` / `IkaemosMusicPreset`. Those call
  `savegame_state.get_flag(cur_level_index, name)` with the same flag names and store the same values. The behaviour
  is the same. The store happens at the same point in each init: after the phoenix_serpent branch in Liangshan, and
  after the boss_killed / disable test in Ikaemos. One small difference: the C `&&` skips the serpent_dead read
  when phoenix_dead is 0, and the patch's `&&` does the same, so there is no difference in practice. get_flag has no
  side effects that matter here.
- How to see it in game (game-bugs K130/K150, reproduced 2026-10-17 with `[Debug] TraceMusic=1`):
  - Liangshan: set `phoenix_serpent`, and "Boss" starts. Then set `phoenix_dead` + `serpent_dead`. Shipped build:
    "Boss" keeps playing. Fixed build: "Liangshan" plays again.
  - Ikaemos upper hall: set `ikaemos_found_boss`, then `ikaemos_boss_killed`. Shipped build: no music change.
    Fixed build: "Boss", then "IkaemosPalace".
- Uncertain: nothing about the behaviour. One consequence, the same as the patch's: a game loaded in the Ikaemos
  hall after `ikaemos_found_boss` but before `ikaemos_boss_killed` starts the Boss music again on entry (outside a
  cutscene). That is intended, as in LPalaceInt02.

## LootHighlightColour (K61)

- Function: `world_tint_gold_object` (0x43a600, game/gametarget.cpp).
- Source fix: the blue argument reads `world_tint_gold[2]` (0.51) instead of `*(world_tint_gold + 3)`, which is one
  past the array: the "gold" string as a float, about 1.7e22, which the mesh renderers turn into 0.
- Compared with SummonerFix: the patch changes `mov ecx, [0x594430]` at 0x43a62e to read 0x59442c, which is
  `world_tint_gold[2]`. Same behaviour.
- How to see it in game (game-bugs K61): kill a monster that drops loot. The dropped item flashes for 5 s.
  Shipped build: yellow (red and green only). Fixed build: pale gold, with some blue.
- Uncertain: K61's in-game observation is recorded. In the rebuilt exe the out-of-bounds read depends on what the
  linker puts after `world_tint_gold`, so the shipped rebuild may not show exactly the same yellow. The fixed build
  does not depend on that.

## Worker 8

### FIXED_BUILD ports (w8): G8, K6, K191

Checks for all three files: the shipped build's vc6-compare output is identical before and after
the edits (saved as before-*.txt / after-*.txt in my work folder; 0 differing lines). The fixed build
compiles each file (tools/fixed-compile.mjs), and none of its warnings comes from the new code. The
remaining warnings are in headers or in older code.

## G8 SummonKeepsLeader

- Function: `party_remove_player` (0x4335a0), file game/player.cpp.
- Source fix: before the leader re-pick loop, if `game_flags & GF_SINGLE_PLAYER`, `party_leader != NULL`
  and `party_leader != p` (another record leads), the loop starts at `&party_list`. The empty-list test
  ends it before the first pass, so the leader stays. Otherwise the loop starts at `party_list.next`,
  as in the original. This is one `#ifdef` block around the `for` header.
- Compared with SummonerFix: same behaviour. The patch's stub replaces `mov edi, [party_list]` and returns
  the list head under the same three conditions (game_flags bit 0, leader non-null, leader != esi). It
  returns `party_list.next` otherwise. When the removed record led, or no record leads, the original
  re-pick runs. Multiplayer is unchanged, because the bypass needs the single-player flag.
- In game: load a single-player save and select a hero other than Joseph (for example Rosalind, F7). Use
  `summon golem`, then `kill_summon` (`[ConsoleKeys]`). In the original, control jumps to Joseph. With the
  fix, Rosalind stays in control.
- Uncertain: the same as the patch. The branch where the removed record leads was not tried in game.
  It is the original code.

## K6 MapLabelLimit

- Function: `automap_label_alloc` (0x430230), file game/unnamed_worldmap_automap_0x42f1e0.cpp.
- Source fix: when all 32 labels are used, it returns -1 instead of 0.
- Compared with SummonerFix: same. The patch turns `xor eax, eax ; ret` into `or eax, -1 ; ret`. All four
  callers already test for `< 0`: `automap_add_label_at_mouse`, `automap_labels_parse_table`,
  `automap_labels_reset_default` and `savegame_read_automap_labels`. The comments that call this latent
  still describe the shipped behaviour (the header comment of `automap_labels_parse_table` and the
  "0 (not -1)" comment above the function), so I left them.
- In game: place 33 labels on one level's map. In the original, the 33rd replaces the first. With the
  fix, it is refused. A quicker test is to call the function directly with `automap_num_labels` set to
  31 (the 2026-10-17 test): the 33rd call returns -1, not 0.

## K191 EdgeTurnAfterDrag

- Function: `camera_process_input` (0x434660), file game/unnamed_input_0x4343a0.cpp.
- Source fix: a local `was_held` keeps last frame's `camera_edge_mouse_held` before this frame's bit is
  stored. With the cursor at the edge, the release test is then `was_held && !((in[0] >> 4) & 1)`
  ("held last frame, not held now"), not this frame's own bit. Two `#ifdef` blocks: the marker block
  saves the old value, and a "part of FIXED_BUILD K191" block holds the test. The block body is unchanged:
  it clears the held byte and sets `camera_edge_turn_blocked`.
- Compared with SummonerFix: same behaviour. The patch's stub at 0x4346f1 swaps the byte (it stores this
  frame's bit and returns last frame's in al). The right-edge reload at 0x43470a reads a copy of last
  frame's bit, so both edges test the same "held last frame, not now" condition. As in the patch, the byte
  is updated only on frames that get past the early returns (conversation, script editor). The test is
  written with `&&` and `!` instead of the original `&` on a bool, which avoids a C4805 warning. The
  result is the same.
- In game: hold the left button in the world to walk, move the cursor to the left or right screen edge,
  and release it there. In the original, the camera starts turning at once. With the fix,
  `camera_edge_turn_blocked` (0x5fccb8) becomes 1 and the camera does not turn until the cursor leaves
  the edge (game-bugs K191, reproduced 2026-10-18). Watch the yaw at `game_camera + 0x180`.
- Uncertain: the comment above the function still describes the shipped bug. I left it.

## Worker 9

### FIXED_BUILD ports: WorldSoundVolume (K260), ShopCategoryButtons (K3), PickupScrollbar (K12)

Checks for all three files: `tools/vc6-compare.mjs` output is identical before and after the edit (every function's
status unchanged; snd_play_3d, buysell_category_first_index and pickup_update_scrollbar still match).
`tools/fixed-compile.mjs` says "compiles with FIXED_BUILD" for each; the one diagnostic (C4244 in
include/vsdk/math/vector.h line 73) comes from the header, not from the new code.

## K260 WorldSoundVolume

- Function: `snd_play_3d` (0x521480), vsdk/unnamed_snd_0x5204c0.cpp.
- Source fix: on the 2D path (DirectSound without DirectSound3D), instead of `v /= s->volume` it divides `v` by
  `s->volume * snd_group_volumes[s->group]`, giving 0 when that product is 0. The outer `s->volume != 0` test is
  kept, so a sound with volume 0 still passes `v` through as before.
- Against SummonerFix: same behaviour. The patch replaces the fld / fdiv [esi+0x28] / fstp at 0x52156e with a call
  to SndPlay3dUnscale, which does exactly this (group read as the 27-bit signed field, which `s->group` is in the
  source), and the caller's skip when the sound volume is 0 is the same `if`.
- In game: sound setting `DirectSound` (the default, no DirectSound3D), sound-effects slider at half. Before the fix
  world sounds (footsteps, weapon hits, doors, loot drops) play at about a quarter while menu clicks follow the
  slider; with it they follow the slider linearly. At full volume nothing changes.
- Uncertain: game-bugs.md says the patch was checked in the log only, not judged by ear. Ambient emitters are later
  set by `sound_emitter_update_volume` / `snd_set_volume`, which this does not touch (as in the patch).

## K3 ShopCategoryButtons

- Function: `buysell_category_first_index` (0x404f70), game/new_buysell.cpp, the sell branch (buysell_mode != 0).
- Source fix: walks the shown rows with `buysell_get_list_item(row)` until it returns 0 and returns the first row
  whose `item_type_shop_category(entry->item_type)` is the category, else -1.
- Against SummonerFix: same behaviour. The stub (SellCategoryFirstRow) loops `row < 0x209` and breaks on a null
  entry; 0x209 = 521 is the size of party_inventory, and in sell mode `buysell_get_list_item` returns 0 after the
  last sellable item (at most one row per inventory slot), so the null test alone ends the loop at the same row.
- In game: open a shop (for example the Lenele docks trader), switch to sell. Before the fix the misc (gems) button
  is enabled and scrolls the list though the party has no gems (the trader's buy list has gems at index 18); with
  the fix it is disabled, and the magic button still goes to the first owned magic item (Revive Scroll, row 1, in the
  game-bugs reproduction).
- Uncertain: buysell_get_list_item is linear in the inventory, so the loop is quadratic (up to 521 x 521 steps on a
  button press); the patch does the same.

## K12 PickupScrollbar

- Function: `pickup_update_scrollbar` (0x420850), game/new_pickup.cpp; both `pickup_thumb_h` stores (top and
  scrolled branches), plus a new static helper `pickup_thumb_height` above the function (in its own `#ifdef
  FIXED_BUILD` block marked "part of FIXED_BUILD K12", with no `#else` because it replaces no original lines).
- Source fix: thumb height = track * page / items (integer), raised to the thumb bitmap height when it would be
  shorter and the bitmap fits in the track. The thumb y position and pickup_drag_scrollbar are unchanged (they
  already map the thumb top by scroll / items).
- Against SummonerFix: same behaviour (PickupThumbHeight). The stub's `container ? ... : 0` null test is dropped
  because the function has already read `pickup_container->num_items`; the helper keeps the `count > page` test
  though the stores are only reached when it holds.
- In game: open a container with more than 7 items (for example a loot bag of 12 Health Tonics), scroll down. Before
  the fix the thumb stays the full 56 px track height and runs past the bottom over the down arrow (1024 x 768,
  scrolled 5 rows: top 23 px down, still 56 px tall); with it the thumb is 32 px and ends inside the track
  (335..391).

## Worker 10

### FIXED_BUILD ports (w10)

Checks for all three files: `tools/vc6-compare.mjs` output identical before and after (shipped statuses unchanged);
`tools/fixed-compile.mjs` says "compiles" for each. Its warnings come from headers (vector.h, vfx.h) or older code
(level_scripts_common.cpp line 1591, unused `it`), not from these blocks.

## K64 OpenGLTextures

- Function: `gr_get_video_memory_mb` (0x5079f0), vsdk/gr.cpp.
- Source fix: the `GR_OPENGL` case returns 64 instead of 8.
- Compared with SummonerFix: the same thing. The patch changes `mov eax, 8` at 0x507a03 to `mov eax, 64`.
- Bug id: the fix list said "?". bugfix.c and game-bugs.md both call this bug K64, so the marker uses K64.
- In game: use the OpenGL renderer and start with no `summoner.cfg` (or one shorter than 64 bytes).
  `summoner_cfg_load` turns on the reduced textures when the reported memory is <= 8. With the fix, the full
  textures stay. No one has tested this in game; bugfix.c says the patch applies, but the texture choice was
  never checked.
- Uncertain: 64 is SummonerFix's number. Any value above 8 behaves the same at this one caller.

## K4 InventoryCategoryButtons

- Function: `inventory_category_first_index` (0x4153d0), game/new_inventory.cpp.
- Source fix: in identify mode (`inventory_open_mode`), when the first unidentified item of the category is found
  at party-inventory index i, the function returns the count of unidentified items before i. That count is the
  item's row in the shown list. The -1 return and open mode are unchanged.
- Compared with SummonerFix: the same behaviour. The patch sends `mov eax, esi` at 0x41543e to `IdentifyRowStub`,
  which calls `IdentifyRowOf(esi)` (the same count using `item_is_identified` on `party_inventory[j].item`). The -1
  path (0x415437) does not go through 0x41543e in either version.
- In game (game-bugs K4): mark some item types unidentified, open the inventory in identify mode, scroll to the
  bottom, and press a category button (for example magic). Without the fix, the list jumps too far down (to the
  party-inventory index). With the fix it goes to the first unidentified item of the category (Revive Scroll,
  row 0, in the reproduction save).
- Note: the block wraps only the `return i;` of an unbraced `if`. The fixed branch is a braced block in its place.

## K110 AmbientSounds

- Function: `level_script_leave` (0x4ccc70), game/level_scripts_common.cpp.
- Source fix: `while (level_ambient_count > 0) level_ambient_remove(0);` replaces the loop that removed by a
  growing index from a table that shrinks.
- Compared with SummonerFix: the same behaviour. The patch turns `inc esi` (0x4cccc9) into a nop, so the loop
  keeps removing record 0 while `0 < level_ambient_count`. `level_ambient_remove` always lowers the count, so the
  loop ends.
- In game (game-bugs K110): change levels back and forth between a small level and a big town (for example
  lenele1d and lenele1aa), and count the used `sound_emitters` (128 records) after each change. Without the fix
  the count climbs (48, 34, 74, ... 128) and lenele1aa ends up with only 20 of its 44 ambient sounds. With the fix
  every leave frees all of the level's emitters.

## Worker 11

### FIXED_BUILD ports: game/unnamed_fx_0x489c40.cpp

Checks (my copy): `tools/vc6-compare.mjs` on the file gives the same output before and after the edits (51 match,
6 differ, of 57; no line changed). `tools/fixed-compile.mjs` says "compiles with FIXED_BUILD"; its four warnings
come from headers (vector.h, vfx.h) and from an unused local `s` at line 919, none from the new code.

## G270 AreaCasterGone

- Function: `fx_frame` (0x48a230), game/unnamed_fx_0x489c40.cpp, the second pass over the collected areas for
  each living character.
- Source fix: `if (!caster) continue;` instead of `break;`, so an area whose caster no longer resolves is skipped
  and the later areas still burn / freeze and still set `in_fire` / `in_blizzard` / `in_dark`.
- Compared with SummonerFix: the patch retargets the `je` at 0x48b40a from 0x48b5a1 (after the loop) to 0x48b581
  (the loop increment). `continue` is that jump. Same behaviour.
- In game (game-bugs G270): a save with a summon; `summon golem`, `firewall 1 30 summon` (a wall cast by the golem,
  away from the party), `firewall 5 -6` (walls by the leader across the party). Watch Joseph burn (72 HP every 2 s),
  then `kill_summon`: shipped, he stops burning at once; fixed, he keeps burning.
- Uncertain: nothing. The `if (duration < frametime) break;` later in the same loop is left as it is (not part of
  this fix).

## G271 EffectSlotSpell

- Function: `fx_new` (0x48c3e0), game/unnamed_fx_0x489c40.cpp.
- Source fix: after the other stores, `fx->spell = -1;` (no spell, as `fx_create_instance` stores), so a status
  holder (kind 9) or item effect (kind 16) that reuses a burnt-out fireball / wall of fire object no longer counts in
  `fx_count_spells_1a_1d_4e`.
- Compared with SummonerFix: the patch adds `mov [eax+0xa8], edx` before the `ret`, where `edx` is -1 (set by
  `or edx, -1` at 0x48c458). Same value, same place (last store). Spell creators that call `fx_new` store their own
  spell afterwards, as before.
- In game (game-bugs G271): in Lenele, `firewall 1`; once it has burnt out, `fx_count_spells_1a_1d_4e` returns 0.
  Give Joseph a 15 s status (`status_effect_add`) so `character_update_status_effects` makes his holder: shipped,
  the holder takes the wall's object with spell 0x1d and the count is 1; fixed, spell -1 and the count stays 0.
- Uncertain: the function's header comment still says the spell keeps the slot's last value; that describes the
  shipped code and was left unchanged.

## C272 EffectSearchGuard

- Function: `fx_disable_by_effect_name` (0x48d660), game/unnamed_fx_0x489c40.cpp.
- Source fix: the test becomes `fx->instance && fx->instance->category == 0`, so objects without an instance
  (arrows, spell areas, status holders, item effects) are skipped; and `if (!base) continue;` instead of `return;`,
  so an effect without a base no longer ends the search.
- Compared with SummonerFix: the stub at 0x48d674 makes `ecx` non-zero when the instance is NULL, so the following
  `test ecx, ecx; jne` skips the object (the same as failing the `category == 0` test); the `je` at 0x48d691 goes to
  the next object (0x48d6b1) instead of the return (0x48d6bf). Same behaviour, both parts in one #ifdef block.
- In game (game-bugs C272): in Lenele give Joseph a status (`status_effect_add`) so he has a status holder, then
  `callstr 48d660 masad-firedoor2.vfx`: shipped, access violation reading 0x8 at 0x48d67a; fixed, the call returns
  and the game runs on. In normal play: the Masad combat tutorial's part 2 with an arrow in flight, a spell area
  burning or a status active.
- Uncertain: nothing.

## Worker 12

### FIXED_BUILD ports: AI groups (game/unnamed_ai_groups_0x492b00.cpp)

Checks (both run on my copy):
- Shipped build (`tools/vc6-compare.mjs`): every function's status is identical before and after the edit
  (15 match, 3 differ, same lines; the 3 differing functions were already differing: style1, style2, caster).
- Fixed build (`tools/fixed-compile.mjs`): compiles. Two warnings, neither from the new code:
  include/vsdk/math/vector.h(73) C4244, and game/unnamed_ai_groups_0x492b00.cpp(182) C4244 in
  `ai_group_combat_think` (untouched).

## AiBlessCount (C210, also fixes G212)

- Function: `ai_group_think_caster` (0x4936c0), game/unnamed_ai_groups_0x492b00.cpp.
- Source fix: in the second pass (Bless scoring), the first `count++` (the one right after
  `list[i].flags |= 4`) is left out, so `count` goes up only when the member's score is above 0.
  Marker `// FIXED_BUILD: C210 AiBlessCount`. The fixlist gave the bug id as "?"; bugfix.c and
  game-bugs.md name it C210 (G212 is the same count's everyday side), so C210 is used.
- Compared with SummonerFix: the same. The patch turns the first `inc ebx` (0x49395c) into a `nop`;
  the second `inc ebx` (0x493988, under score > 0) stays.
- In game: a party with Rosalind (Healer/Caster) who can cast Bless but not Protect (holy skill set to
  20 in a test save), a hard fight where the enemies' attack rating is over 3x the party's (the Luminar
  with `[Debug] BlessPassAlways=1` to lower that threshold); `[Debug] TraceBlessCount=1` logged count 5
  for 4 members before the fix and 1 after. G212: Protect should now need three members that need it.
- Uncertain: the source declares `boss_target list[5]`, but the binary's list is six 0x10-byte records
  (frame `sub esp, 0x80`; the shipped compile gives `sub esp, 0x6c`, which is one of this function's
  existing differences). With the fix the count is at most the number of members, so the list is in
  bounds in the binary; in our source a sixth member would still write past `list[5]` (also in the
  init loop, which is unchanged code). The party group holds at most five (four heroes and a summon), so
  this should not be reached, but making the array `[6]` (the match fix for this function) would remove
  the doubt. Not changed here, as it is outside the fix and affects the shipped build.

## SupportAiTarget (G211)

- Function: `ai_group_think_style1` (0x493290), same file.
- Source fix: after ordering the attack on the chosen target, `return`, so the "first enemy" fallback
  order (`ai_think_enemies[0]`, mark true) runs only when no target was picked.
  Marker `// FIXED_BUILD: G211 SupportAiTarget`.
- Compared with SummonerFix: the same. The patch replaces the `add esp, 0x10` after the first order
  (0x49354a) with `jmp 0x493564`, the fallback's own stack cleanup followed by the epilogue, i.e. a
  return after the first order.
- In game: IkaemosExt2 (Onyx golems and gargoyles), Joseph leading, Flece on her default Support style;
  set Rosalind's style to 0 (`poke`) so no enemy targets a style 2-5 member; `[Debug] TraceGroupAttack=1`
  logged "chosen ..." then "fallback (first enemy) ..." in one think before the fix, the chosen order
  alone after.

## DetectionCone (G210)

- Function: `ai_group_collect_enemies` (0x493ac0), same file.
- Source fix: beyond detection range (and not blind), the enemy is accepted when the dot product of the
  direction to it with the member's forward vector is `>= 0.71f` (inside the 45-degree cone ahead)
  instead of `< 0.71f`. Marker `// FIXED_BUILD: G210 DetectionCone`.
- Compared with SummonerFix: the same. The patch turns `jne` into `je` after `fcomp [0x581ae0]; fnstsw;
  test ah, 1`, so the enemy is accepted when C0 is clear, i.e. dot >= 0.71 (an unordered/NaN dot, which
  sets C0, is then rejected; `>=` rejects NaN as well).
- In game: IkaemosExt2, `summon blackfire` (field of view 10, detection 3), the leader placed about
  7 units from a monster group (`navgoto`, `nudge`), `[Debug] TraceDetectCone=1`: before the fix
  enemies behind the summon (dot -0.61, -0.46) were accepted; after, an enemy straight ahead (dot 0.98).
  Affects only party AI members with field of view wider than detection range (mostly summons).

## Worker 13

### FIXED_BUILD ports (w13)

Checks for all three files (game/living_entity.cpp, game/unnamed_luminar_0x4c8d10.cpp, game/characterinfo.cpp):
the shipped vc6-compare output is identical before and after (whole output, byte for byte), and
tools/fixed-compile.mjs says "compiles" for each. The only warnings are the existing header ones (vector.h,
vfx.h) and the existing C4101 'spare' at living_entity.cpp(2898), which is not in my code.

## C250 LuminarBeamTarget

- Function: `ai_state::luminar_beam_hit` (0x4ca3a0), game/unnamed_luminar_0x4c8d10.cpp.
- Source fix: the trigger-7 hit becomes `if (!t || !t->add_hp(-125.0f, owner->handle, 1, false)) return;`.
  A NULL target takes no damage, and the flinch and target effect after it are skipped.
- Compared with the patch: the patch sends the `character_add_hp` call at 0x4ca502 through a stub that returns 0
  ("no damage") for ECX = NULL. The function then returns at the same test, so the behaviour is the same.
  `luminar_beam_hit_done = true` is still set before the test in both.
- In game: Luminar fight (`level IonaEXT02 IonaEXT02_v2`, `npcgoto Luminar`); the beam's target dies or goes
  (summon dismissed) during the 88 frames before the impact. Original: crash at the impact frame. SummonerFix's
  `[Debug] LuminarBeamTargetGone=1` forces the NULL target for testing.
- Uncertain: nothing.

## G100 LeechDrain

- Function: `Character::get_hit_by_spell` (0x45dea0), game/living_entity.cpp, case 0x3e and the common tail.
- Source fix: after `amount = power * 0.05f * level;` (the not-resisted path), if amount > 0 the character hit
  gets `add_hp(-amount, source, 0, false)`, `living_from_handle(source)` (when not NULL and not this character)
  gets `add_hp(amount, source, 0, false)`, and amount becomes -amount; otherwise amount = 0. Then
  `goto leech_drained`, a label placed after the tail's `spell_roll_amount` + `add_hp` (0x45fed8), so the rest
  of the tail (flinch test, hit effect, spell 0x21 block, retaliate) runs as for other spells. Two blocks: the
  marker block in case 0x3e (its #else keeps the original `break;`), and a "part of FIXED_BUILD G100" block that
  only adds the label (no #else: nothing original is replaced there).
- Compared with the patch: the same steps as the stub `LeechDrain` (amount <= 0 -> 0; drain, then heal the
  source unless it is NULL or the target; store -amount; resume at 0x45fed8). The resisted path (`break`
  before the amount) still goes through the roll, as in the patch.
- Note: bugfix.c says the target then "flinches as for other damage", but case 0x3e leaves `flinch` at 0, so
  the tail's `play_flinch` does not run (in the patch or here); any flinch comes from `add_hp` itself, if it does
  one. Behaviour kept as the patch has it; only the comment's wording looks off.
- In game: game-bugs G100 reproduction (agent command `spellhit <Joseph> 3e 100` with Rosalind leading as the
  source): original, HP unchanged; fixed, Joseph loses power x 0.05 x level and Rosalind gains it. Natural cases:
  the leech special ability of the Blood Elemental, Crystal Rokhul and Wraith in single player.

## K100 RingSixtyThousand

- Functions: `character_can_use_summon` (0x446e90), game/characterinfo.cpp, and `ring_check_summon_unlock`
  (0x450220), game/living_entity.cpp.
- Source fix: the seven `get_int(4, "<ring>_ring_xp") > 60000` tests in character_can_use_summon become
  `>= 60000` (seven small blocks: the first carries the marker, the others "part of FIXED_BUILD K100"; the
  compiler merges them into the one `jle` the patch changes at 0x447390), and ring_check_summon_unlock's
  `new_xp > 60000` becomes `new_xp >= 60000` (one "part of" block). `old_xp < 60000` is unchanged.
- Compared with the patch: the same (both jle -> jl); ring_format_info already uses 60,000 or more.
- In game: game-bugs K100 (agent `callstr 450220 Ring~of~Light 59000 60000`): original, no message for a gain
  landing on exactly 60,000; fixed, "Joseph acquired Summon Wraith!", and Summon Wraith is castable at 60,000.
- Uncertain: nothing.

## Worker 14

### FIXED_BUILD ports (w14): K170, G170, K193

Checks for both files: the shipped-build vc6-compare output is identical before and after the edit
(game/unnamed_interface_0x411270.cpp and game/unnamed_frame_0x4323e0.cpp; before-*.txt / after-*.txt in
the work folder, 0 differing lines; the frame file is 21/21 match). fixed-compile says "compiles" for both.
The only diagnostics are C4244 warnings in include/vsdk/math/vector.h and include/vsdk/geom/vfx.h,
none from the new code. No header was changed.

## K170 PopupRowOffset

- Function: `ui_choice_popup_hit_test` (0x413970), game/unnamed_interface_0x411270.cpp.
- Source fix: in the titled branch, the first row's y also adds `ui_choice_popup_title_gap[resolution_index]`,
  the gap `ui_choice_popup_draw` adds below the title (5 at every resolution). One block, with the marker.
- Compared with SummonerFix: the patch swaps `mov esi, [4*edx + margin_y] ; add esi, ecx` at 0x4139b3 for a
  call to a stub that computes margin_y[res] + popup_y + title_gap[res], and the code after it adds the title
  height. The result is the same: margin + y + title height + gap. The untitled branch is unchanged in both.
- In game: multiplayer only ("You have died." Restart / Quit, and the two PXO popups). Point at the bottom
  5 pixels of "Restart": the original lights "Quit"; the fixed build keeps "Restart" lit. (game-bugs K170 was
  reproduced by forcing the death popup in single player.)
- Uncertain: nothing. The drag rectangle in `ui_choice_popup_update_buttons` also ignores the title, but the
  patch does not touch it and no titled popup uses the drag flag, so it is left as it is.

## G170 MessageBoxPause

- Functions: `ui_message_box_show` (0x413e40) and `ClearFrameFlag5bd504` (0x414020),
  game/unnamed_interface_0x411270.cpp. `game_pause`, `game_unpause` and `game_is_paused`
  (game/unnamed_frame_0x4323e0.cpp) are called, not changed.
- Source fix, three blocks:
  1. a file-scope `static bool ui_message_box_paused` just before `ui_message_box_show`
     ("part of FIXED_BUILD G170"; a pure addition, so this block has no `#else`);
  2. in `ui_message_box_show` (the marker block): when the box pauses a single-player game (flag 1), it
     sets `ui_message_box_paused` if `game_is_paused()` is false, then calls `game_pause(0, 0)` as before;
  3. in `ClearFrameFlag5bd504` ("part of FIXED_BUILD G170"): under the same condition it calls
     `game_unpause()` only when `ui_message_box_paused` is set, and clears it.
- Compared with SummonerFix: the patch redirects the two calls (0x413f96, 0x414034) to stubs with one byte
  flag: the pause stub sets it when the game was not paused and then jumps to `game_pause`; the unpause stub
  unpauses and clears it only when it is set. The source does the same, at the same two call sites and under
  the same conditions (the flag is set and cleared only inside the flag-1 / single-player tests, as in the
  patch). Like the patch, a second box shown before the first is closed does not reset the flag, so the
  close after a box that did pause still unpauses.
- In game (game-bugs G170): in a single-player fight, Space to pause, then the save key (or Ctrl+S quick save
  while a summon is emerging): "Cannot save in combat." / "Cannot save while summon is emerging." appears.
  Close it with OK: the original resumes the fight, the fixed build stays paused. A box shown while the game
  runs still pauses and unpauses as before.
- Uncertain: the flag is not reset on a level change or load; if a flag-1 box were ever dismissed without
  `ClearFrameFlag5bd504`, a later box would unpause. That is also how the patch behaves.

## K193 ScreenshotNumbers

- Function: `screenshot_save` (0x4324f0), game/unnamed_frame_0x4323e0.cpp.
- Source fix: in the non-empty branch, the `sscanf(list.prev->name, "screen%d", &n); n++;` is replaced by a
  walk over the whole file list: n = 1 + the largest N among the names that are exactly "screen" + digits
  (the list holds names without the extension: mode 1, no flag 4, `file_strip_ext`). Other names
  (screenshot.jpg, screen12a) are ignored, so n is always set; with none valid n is 0. The empty branch
  (n = 0) and the printf lines are unchanged.
- Compared with SummonerFix: the patch replaces `mov eax, [esp]` (n) before the `sprintf` at 0x4325ad with
  a stub that runs its own FindFirstFile over "<install dir>screen*.jpg" (or .tga) and returns 1 + the
  largest N, with the same 100000000 digit cap. The source reads the list `file_find_files` has already
  built for the same folder and pattern instead of searching again; the numbers are the same. Two edge
  differences: (a) the patch accepts "screenN." followed by more ("screen5.old.jpg" counts as 5), the source
  accepts it only if `file_strip_ext` leaves exactly "screenN"; (b) the list's names are copied into a
  0x28-byte field, so a very long file name behaves as in the original scan. Neither affects normal use.
- In game (game-bugs K193): put a `screenshot.jpg` in the game folder and press Print Screen three times:
  the original rewrites one `screen001.jpg` (or an unset number); the fixed build writes screen001,
  screen002, screen003. Past screen999 the original keeps rewriting screen1000; the fixed build goes on to
  screen1001.
- Not fixed (not part of the patch either): the file list is never freed, one small leak per screenshot
  (game-bugs notes it).
