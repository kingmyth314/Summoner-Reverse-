// Fixes for bugs in the original game (docs/sum-exe/game-bugs.md). The crash fix (no sound device) is always on;
// each other fix changes how the game behaves or looks and has an on/off key in [Fixes] (default on, FixOn()).
//
// No-sound new game crash. When DirectSound cannot start (DirectSoundCreate fails, e.g. no audio output device
// is connected), ds_init (0x55c1a0) returns before ds_init_channels and snd_stream_init, so the channel records
// (0x2f798b8, 0x2c bytes each) stay zero and the stream lock (CRITICAL_SECTION 0x2fbd998) is never initialised.
// Starting a new game then calls ds_channel_release, which reads the channel's stream index (+0xc). Zero passes
// its `test eax, eax; jl` check, so it calls EnterCriticalSection on the zeroed lock and crashes in ntdll
// (write to 0x14). Fix: the `mov eax, [esi+0xc]; test eax, eax` at 0x55c5db becomes a call to a stub that
// returns -1 (no stream) unless ds_init finished (its flag byte 0x2f8f73c is 1). With sound working nothing
// changes. ECX is free at that point (the next use reloads it).
//
// OpenGL video memory (not a crash; docs/sum-exe/game-bugs.md K64). gr_get_video_memory_mb (0x5079f0) returns a
// constant 8 for the OpenGL back end (`mov eax, 8` at 0x507a03). Its only caller, summoner_cfg_load, turns on
// low-resolution textures when summoner.cfg is missing or short and the value is <= 8 (0x43e36c..0x43e376), so a
// fresh install under SummonerFix's OpenGL renderer starts with reduced textures. Fix: report 64 instead.
//
// World sounds at the sound-effects volume squared (not a crash; game-bugs K260). snd_calc_volume_3d returns
// group volume * sound volume * attenuation. Without DirectSound3D (the default Soundcard=DirectSound), snd_play_3d
// divides only the sound volume out (fdiv [esi+0x28] at 0x521572) and hands the rest to snd_play, which multiplies
// group * sound volume in again, so the slider value counts twice (0.5 gives 0.25). Fix: the fld / fdiv / fstp at
// 0x52156e becomes a call that divides the group volume out as well, so the slider scales world sounds linearly,
// like the DirectSound3D path. At full volume nothing changes.
//
// Robe of Restoration heals the attacker (not a crash; game-bugs G300). In single player
// combat_trigger_reflex_ability casts an armour's beneficial "+Triggered Effect:" spell with the attacker's handle
// as the target ([ebp] at 0x467b0f, then [ecx+0x80] at 0x467b17), so Rosalind's robe heals the enemy that hit her
// (reproduced 2026-10-16 with [Debug] TraceArmourSpells). eax holds the wearer there (0x467afc; it is also the caster
// pushed at 0x467c85), so mov ecx, [ebp] becomes mov ecx, eax: the heal lands on the wearer, as the reflex-ability
// branch and the multiplayer branch already do.
//
// Choice popup rows 5 pixels above their text (not a crash; game-bugs K170). Below a title, ui_choice_popup_draw
// moves down by the title height plus ui_choice_popup_title_gap[resolution] (0x57ef7c, 5 at every resolution), but
// ui_choice_popup_hit_test adds only the title height, so the bottom 5 pixels of each entry select the next one
// ("You have died.": the bottom of Restart lights Quit; reproduced 2026-10-16). The titled popups are all
// multiplayer (death_popup and the two PXO popups). Fix: the mov esi, [4*edx + margin] ; add esi, ecx at 0x4139b3
// becomes a call that also adds the gap, followed by the original jmp. eax (the title height) and ecx are kept.
//
// No hit effect on metal characters (not a crash; game-bugs K20). combat_spawn_hit_vfx first picks blood for
// flesh and leather (skins 1, 2) and sparks for stone, ice and the seven metals (3, 9..16), and returns for the
// rest; it then spawns blood.vfx only for skin 1 and sparkhit01.vfx only for skins 3 and 9, so Joseph's hits on
// the Brass, Broken Brass and Steel Golems (bronze, skin 12) and the Iron Golem show nothing (reproduced
// 2026-10-16 with [Debug] TraceHitVfx in the sewer boss room). Fix: cmp edi, 1 ; jne at 0x466a0c becomes
// cmp edi, 2 ; ja (blood for 1 and 2), and the stone / ice test at 0x466a2f becomes a jmp to the spark spawn, so
// every skin that got a name now gets the effect stone golems already show. Flesh, stone and ice are unchanged.
//
// Save over an existing save without the question (not a crash; game-bugs G4). saveload_try_save asks "Overwrite
// existing savegame?" when the selected entry is a save, but reads saveload_entries[selected_row] without the
// scroll position (0x426340..0x426352). Entry 0 is "<New Savegame>", so with the list scrolled the top visible row
// skips the question, and saveload_save_to_slot, which does add the scroll position, writes over that save
// (reproduced 2026-10-17: scrolled 3 rows, Save on the top row overwrote "G4 test save 12" with no question). Fix:
// the mov eax, [saveload_selected_row] at 0x426340 becomes a call that returns selected_row + scroll_pos, so the
// test looks at the entry the save goes to. The rows below the top one now ask about the right save too.
//
// Phoenix Rider's Inferno hits its target once per party member (not a crash; game-bugs G215). For spell 0x1e,
// phoenix_special_strike loops over party_list (ebx = the member) but tests the distance from the boss to the
// target (esi) and hits the target, then knocks the member down, so the target takes the hit once for every member
// and the whole party falls wherever it stands (reproduced 2026-10-17 with [Debug] TracePhoenixInferno: one
// Inferno, Flece hit four times). Fix ([Fixes] PhoenixInferno): the three distance loads (0x499900 / 0x499906 /
// 0x49990c), the effect's target handle (0x499933) and the hit's this (0x499951) use ebx, so each member within
// the spell's distance is hit and knocked down once.
//
// Luminar beam crash (game-bugs C250). luminar_ai_frame drops the target when it is gone or dying, but
// luminar_beam_hit's impact (trigger 7) calls character_add_hp(-125) on it with no NULL test (ecx = esi at
// 0x4ca4fa, call at 0x4ca502), and character_add_hp reads [ecx+0x320] first: a target that dies or disappears
// during the 88-frame beam crashes the game at the impact frame. Fix ([Fixes] LuminarBeamTarget): the call goes
// through a stub that, for a NULL target, returns 0 ("no damage", so the flinch and effect after it are skipped)
// and pops the four arguments as character_add_hp would.
//
// Control jumps to the first party member when a summon goes (not a crash; game-bugs G8). After unlinking any
// party record, party_remove_player hands the leadership in single player to the first living member of party_list
// (0x4335d9..0x43363d), whether or not the removed record led (reproduced 2026-10-17: Rosalind selected, summon
// golem, kill_summon: Joseph took control). Fix ([Fixes] SummonKeepsLeader): the mov edi, [party_list] that starts
// the loop becomes a call that, in single player while another record leads, returns the list head, so the
// loop's empty-list test skips it. When the removed record led (or none does) the leader is re-picked as before.
// Multiplayer is unchanged.
//
// Spell recovery times are the spell's record number (not a crash; game-bugs G12). spells_parse_table parses
// "+Recovery time:", "+AI Recovery time:" and "+Loop Time:" with TextFile__ParseInt (value in eax) and then stores
// bp, the record index (0x47a49c, 0x47a4c1, 0x47a50b), so fireball (record 26) recovers in 26 s, not the table's
// 20. Fix ([Fixes] SpellRecoveryTimes): mov [esi-N], bp becomes mov [esi-N], ax at all three. The table is read
// at start-up, after this DLL's DllMain has patched the code. Spells without the lines are unchanged.
//
// A loose gold pile holds 1 gold after a load (not a crash; game-bugs G40). savegame_write_item_objects saves each
// loose gold pile as x, y, z and its amount; savegame_read_item_objects reads x, y, z, skips the amount (add esi, 8
// at 0x4d33a7) and spawns the pile with item_obj_spawn(item, pos, matrix, 1, -1, -1) (push 1 at 0x4d33c2). Fix
// ([Fixes] GoldPileAmount): the lea ecx, [esp+0x50] ; push 1 at 0x4d33be becomes a call to a stub that does the
// lea and pushes the saved amount ([save buffer + esi - 4], buffer = [edi+0x1034]), or 1 if it is not positive.
//
// A warning box unpauses a game the player had paused (not a crash; game-bugs G170). A message box with flag 1
// pauses a single-player game (ui_message_box_show calls game_pause at 0x413f96, which does nothing when the game is
// already paused), and closing it calls game_unpause (ClearFrameFlag5bd504, 0x414034) whether or not its pause took
// effect: pausing a fight and pressing the save key gave "Cannot save in combat." and the fight went on once it was
// closed. Fix ([Fixes] MessageBoxPause): the game_pause call notes whether the game was running, and the
// game_unpause call is skipped when the box did not pause it.
//
// "Give him 10 gold pieces." takes 100 (not a crash; game-bugs G301). In occupied Lenele (Lenele2ab) the topic
// triggers "payoff", and ls_lenele2ab_action subtracts 100 from party_gold (sub eax, 0x64 at 0x4f124d), clamped at
// 0. Fix ([Fixes] CanisTenGold): sub eax, 10, the amount the topic offers.
//
// AI healer's Bless candidates counted twice (game-bugs C210, and G212 its everyday side). ai_group_think_caster's
// second pass increments its candidate count once for every unblessed member (inc ebx at 0x49395c) and again when
// that member's score is above 0 (0x493988). The count then indexes a six-record list on the stack: past six it
// overwrites the function's argument, ai_group_combat_think's saved ebp and its return address; at three or more it
// casts Protect, so two needy members were enough. Seen 2026-10-17 with [Debug] TraceBlessCount: count 5 with 4
// members. Fix ([Fixes] AiBlessCount): the first inc becomes a nop, so each member with a positive score counts once
// and the count stays within the members (at most six records).
//
// One burning area whose caster is gone switches off the rest (not a crash; game-bugs G270). For each living
// character fx_frame walks the burning areas; when an area's caster no longer resolves (living_from_handle of
// +0xb0) it leaves the loop (je 0x48b5a1 at 0x48b40a) instead of going to the next area (0x48b581), and the code
// after the loop cures the fire / blizzard / dark wall status unless an area of that kind was seen. Fix ([Fixes]
// AreaCasterGone): the je goes to 0x48b581, so that area is skipped and the later ones still count.
//
// Support-style AI drops its chosen target (not a crash; game-bugs G211). ai_group_think_style1 (Flece's default
// style) orders an attack on the target it picked (0x493545) and then falls through to the "first enemy" fallback
// (0x49355f), whose order replaces the first; the enemy list is shuffled every think, so the target is random. Fix
// ([Fixes] SupportAiTarget): the add esp, 0x10 after the first order (0x49354a) becomes jmp 0x493564, the fallback's
// own add esp, 0x10 before the function's epilogue, so the fallback runs only when no target was picked.
//
// Detection cone the wrong way round (not a crash; game-bugs G210). Beyond $Detection range, ai_group_collect_enemies
// accepts an enemy in field of view range and line of sight when the dot of the direction to it with the member's
// forward vector is below 0.71 (jne after fcomp [0x581ae0], 0x493c5c), i.e. outside the 45-degree cone ahead.
// Fix ([Fixes] DetectionCone): jne -> je, so enemies ahead are noticed and those behind are not.
//
// Leech does nothing in single player (not a crash; game-bugs G100). Case 0x3e of living_entity_get_hit_by_spell
// computes power x 0.05 x level into amount (fstp [ebp-0x24] at 0x45f47d) and jumps to the common tail, where
// spell_roll_amount with dice count 0 overwrites it with 0. Fix ([Fixes] LeechDrain; decided 2026-10-17: leech
// drains): the fstp + jmp at 0x45f47d jump to a stub that takes the amount from the character hit
// (character_add_hp(-amount, source)) and gives it to the source (character_add_hp(+amount)), stores -amount and
// continues after the roll and add_hp (0x45fed8), so the target flinches as for other damage.
//
// Zane's encounter overwritten (not a crash; game-bugs G6). At game stages 4-8 on grassland, when Zane is active
// (unhide_zane, not no_more_zane) ls_worldmap1_pick_encounter_stage4 picks rand-grassland01 / _v3 half of the time
// (0x4fc9d8..0x4fca20) but has no return, so the general rolls that follow overwrite both names. Fix ([Fixes]
// ZaneEncounter): the end of that copy (mov ecx, edx ; and ecx, 3 ; rep movsb at 0x4fca1b) jumps to a stub that
// finishes the copy and leaves through the function's epilogue (0x4fcae1), as the Korel case returns.
//
// Shop category buttons in sell mode jump to the wrong row (cosmetic; game-bugs K3). The sell list shows the party's
// items that the shop buys (buysell_get_list_item), but buysell_category_first_index's sell branch (0x404f98) returns
// the index of the first item of the category in the shop's buy list, and enables the button when the shop buys
// that category at all. Fix ([Fixes] ShopCategoryButtons): the branch jumps to a stub that returns the first row of
// the shown list in that category (item_type_shop_category of each row's item), or -1.
//
// Inventory category buttons in identify mode jump to the wrong row (cosmetic; game-bugs K4). In identify mode the list
// shows only unidentified items, but inventory_category_first_index returns the party-inventory index of the first
// unidentified item of the category (mov eax, esi at 0x41543e). Fix ([Fixes] InventoryCategoryButtons): that return
// goes through a stub that turns the index into the row of the shown list (the unidentified items before it).
//
// Ambient sounds leak on every level change (gameplay; game-bugs K110). level_script_leave removes the level's ambient
// sounds with for (i = 0; i < level_ambient_count; i++) level_ambient_remove(i), but each removal moves the later
// records down and lowers the count, so only every other emitter is freed. The 128 sound_emitters fill after a few
// visits to the big towns, and then new ambient and ground-effect sounds are dropped. Fix ([Fixes] AmbientSounds):
// the loop's inc esi (0x4cccc9) becomes a nop, so it removes record 0 until the table is empty.
//
// Pickup scrollbar thumb runs past its track (cosmetic; game-bugs K12). pickup_update_scrollbar moves the thumb down
// by scroll / items x track but always makes it as tall as the track (0x4208f8, 0x420943), so a scrolled container
// list draws the thumb past the bottom. Fix ([Fixes] PickupScrollbar): both stores call a stub that sets the height
// to track x page / items, as the assess screen sizes its thumb; pickup_drag_scrollbar already maps the thumb top
// the same way.
//
// Edge turn right after a drag (cosmetic; game-bugs K191). camera_process_input stores this frame's held-drag bit in
// 0x5fccb7 and then, with the cursor at a screen edge, tests "not held now and 0x5fccb7", which is always false, so
// the block byte 0x5fccb8 that should hold the edge turn back after a drag released at the edge is never set: the
// camera starts turning as soon as the button comes up. Fix ([Fixes] EdgeTurnAfterDrag): the store (0x4346f1)
// calls a stub that swaps the byte and hands back last frame's bit, and the right-edge reload (0x43470a) reads that
// copy, so the test is "held last frame, not held now" and the turn waits until the cursor leaves the edge.
//
// Loot highlight has no blue (cosmetic; game-bugs K61). world_tint_gold_object passes the colour at 0x594424 (0.98,
// 1.0, 0.51) but reads blue from 0x594430, the string "gold" (about 1.7e22 as a float), which the mesh renderers turn
// into 0. Fix ([Fixes] LootHighlightColour): mov ecx, [0x594430] at 0x43a62e reads 0x59442c.
//
// "Knocked Down!" for a character who stays up (cosmetic; game-bugs K80). ai_knock_down_local shows the message for
// the current character without looking at ai_set_mode's result, and ai_set_mode refuses the disabled state while
// the character casts (or fights in combat sub-mode 0xe / sneak-attacks). Fix ([Fixes] KnockDownMessage): the loads
// before the "is it the current character" test (0x47431b) call a stub that makes the test fail when ai_set_mode
// returned 0. The function's own result (1) is unchanged.
//
// A long game folder crashes at start-up (crash; game-bugs C310). packfile_add, packfile_fopen and others join
// install_path and a file name with sprintf into 128-byte stack buffers, so a folder path of 116 characters or more
// overwrites a return address. Fix ([Fixes] LongInstallPath): where file_init has set install_path (0x512e0b), a
// path of 64 characters or more is replaced by its 8.3 short form (GetShortPathNameA), the same folder. Volumes
// without 8.3 names keep the long path (logged).
//
// A ring power of exactly 60,000 (cosmetic; game-bugs K100). character_can_use_summon unlocks a ring's higher summon
// above 60,000 (jle at 0x447390), ring_check_summon_unlock announces it when the new total is above 60,000 (jle at
// 0x45023a), and ring_format_info lists it from 60,000 on. A gain landing on exactly 60,000 gets no message (and the
// next gain starts at 60,000, so none ever comes), and the pop-up offers a summon that cannot be cast. Fix ([Fixes]
// RingSixtyThousand): both jle become jl, so all three agree on 60,000 or more.
//
// The 33rd map label overwrites the first (cosmetic; game-bugs K6). automap_label_alloc returns 0 instead of -1 when all
// 32 labels are used (xor eax, eax at 0x430245); its four callers test for a negative result, so they write over
// label 0. Fix ([Fixes] MapLabelLimit): or eax, -1 ; ret (the padding after the function takes the extra byte), so a
// 33rd label is refused instead.
//
// Boss music never switched (cosmetic; game-bugs K130, K150). Two level scripts preset their "done" bytes to 1 on every
// entry, so the branches they guard never run: Liangshan's ls_liangshan_phoenix_dead_done (0x4f49be), which puts
// the level music back once both riders are dead, and the Ikaemos upper hall's found_boss_done / boss_killed_done
// (0x4f9f2e, 0x4f9f34), which start the Boss music for the Priest King and put the level music back after him.
// Fix ([Fixes] BossMusic): the bytes are preset from the saved flags instead, as LPalaceInt02 does: Liangshan's
// to phoenix_dead && serpent_dead, the Ikaemos pair to ikaemos_boss_killed.
//
// Mounted-rider test reads the wrong stage (not a crash; game-bugs G32). boss_species_mounted_stage tells the party
// AI that its target is a mounted rider (it then fights at range, ai_get_combat_style). It tests tiger_stage 1-2 for
// the Tiger, but serpent_stage == 1 for both the Phoenix and the Serpent, and nothing for the Ghost. Fix ([Fixes]
// MountedRiders): its entry (0x4a2480) jumps to MountedStage, which reads each rider's own stage: Tiger 1-2,
// Phoenix (phoenix_stage), Serpent (serpent_stage) and Ghost (0x2480310) 1, as the stage tables in bosses.md give
// the mounted stages and character_target_point_count reads them.
//
// Scrolled save list shows the current scene for its top row (cosmetic; game-bugs K5). In save mode
// saveload_load_thumbnail loads no file for row 0 (test ebx, ebx ; jle at 0x425c65) and saveload_draw_thumbnail
// draws the current scene for row 0 (0x426fda), both by the visible row, so with the list scrolled the top row's
// save shows the scene. Entry 0 is "<New Savegame>". Fix ([Fixes] SaveListThumbnail): the load test looks at the
// entry the row points to (edx, saveload_entries + 0x130 x (scroll + row)), and the draw test reads
// selected_row + scroll_pos (the G4 stub).
//
// Conversation "more" / "end" lit for the wrong mouse positions (cosmetic; game-bugs K292). dialogue_topic_entry__draw
// tests the mouse against the entry's box, but its last test compares the mouse x with box y + text height
// (mov edx, [esp+h] ; add ecx, edx ; cmp eax, ecx with eax the x; "more" 0x4d6fe8, "end" 0x4d707c). Fix ([Fixes]
// ConversationHover): add ecx, [esp+h] ; cmp [esp+y], ecx, the mouse y as the click test does.
#include "summonerfix.h"
#include <string.h>

#define VA_DS_CHANNEL_STREAM_CHECK 0x55c5dbu // mov eax, [esi+0xc] ; test eax, eax  (8B 46 0C 85 C0)
#define VA_DS_INITED 0x2f8f73cu              // byte, set to 1 at the end of ds_init
#define VA_GL_VIDEO_MEMORY_MB 0x507a03u      // mov eax, 8 in gr_get_video_memory_mb (OpenGL case)
#define VA_ARMOUR_SPELL_TARGET 0x467b0fu    // mov ecx, [ebp] (the attacker) in combat_trigger_reflex_ability
#define VA_SND_PLAY_3D_DIVIDE 0x52156eu      // fld [esp+0x1c] ; fdiv [esi+0x28] ; fstp [esp+0x1c] in snd_play_3d
#define VA_SND_GROUP_VOLUMES 0x2cb542cu      // float snd_group_volumes[], indexed by the sound's group
#define VA_HIT_VFX_BLOOD_TEST 0x466a0cu     // cmp edi, 1 ; jne 0x466a2f in combat_spawn_hit_vfx
#define VA_HIT_VFX_SPARK_TEST 0x466a2fu     // cmp edi, 3 ; je 0x466a39 ; cmp edi, 9 ; jne 0x466a54
#define VA_POPUP_HIT_TITLE 0x4139b3u       // mov esi, [4*edx + 0x57ef70] ; add esi, ecx ; jmp 0x4139cf
#define VA_POPUP_MARGIN_Y 0x57ef70u         // int ui_choice_popup_margin_y[3]
#define VA_POPUP_TITLE_GAP 0x57ef7cu        // int ui_choice_popup_title_gap[3] = {5, 5, 5}
#define VA_INFERNO_DIST_X 0x499900u         // fld [esi+0x10] in phoenix_special_strike (also +0x14 at 0x499906, +0x18 at 0x49990c)
#define VA_INFERNO_FX_TARGET 0x499933u      // mov ecx, [esi+0x80]: the effect's target handle
#define VA_INFERNO_HIT_THIS 0x499951u       // mov ecx, esi: this of living_entity_get_hit_by_spell
#define VA_LUMINAR_BEAM_ADD_HP 0x4ca502u    // call character_add_hp (thiscall, 4 args) in luminar_beam_hit
#define VA_CHARACTER_ADD_HP 0x450850u
#define VA_SAVE_OVERWRITE_ROW 0x426340u   // mov eax, [0x5e360c] (saveload_selected_row) in saveload_try_save
#define VA_SAVELOAD_SELECTED_ROW 0x5e360cu  // int saveload_selected_row
#define VA_SAVELOAD_SCROLL_POS 0x5c5b9cu    // int saveload_scroll_pos (first visible row)
#define VA_PARTY_REMOVE_LEADER_LOOP 0x4335d9u // mov edi, [party_list] in party_remove_player (8B 3D 50 C3 5F 00)
#define VA_PARTY_LIST 0x5fc350u             // Player party_list (list head; +0 = first record)
#define VA_PARTY_LEADER 0x5fcc80u           // Player *party_leader
#define VA_GAME_FLAGS 0x5fbfd8u             // bit 0x1 = single player
#define VA_GOLD_PILE_SPAWN_AMOUNT 0x4d33beu // lea ecx, [esp+0x50] ; push 1 in savegame_read_item_objects
#define VA_AREA_CASTER_GONE_JE 0x48b40au    // je 0x48b5a1 after living_from_handle(area caster) in fx_frame
#define VA_DIALOGUE_MORE_HOVER 0x4d6fe8u    // last hover test of the "more" entry in dialogue_topic_entry__draw
#define VA_DIALOGUE_END_HOVER 0x4d707cu     // the same for "end"
#define VA_SAVELOAD_ENTRIES 0x5e3620u       // saveload_entries (0x130 bytes each)
#define VA_SAVE_THUMB_LOAD_TEST 0x425c61u   // test al, al ; je ; test ebx, ebx ; jle in saveload_load_thumbnail
#define VA_SAVE_THUMB_DRAW_ROW 0x426fdau    // mov eax, [saveload_selected_row] in saveload_draw_thumbnail
#define VA_INVENTORY_CATEGORY_RET 0x41543eu // mov eax, esi (identify-mode result) in inventory_category_first_index
#define VA_AMBIENT_REMOVE_INC 0x4cccc6u     // add esp, 4 ; inc esi in level_script_leave's ambient removal loop
#define VA_PICKUP_THUMB_H_TOP 0x4208f8u     // mov [pickup_thumb_h], edx (list at the top) in pickup_update_scrollbar
#define VA_PICKUP_THUMB_H_SCROLLED 0x420943u // mov [pickup_thumb_h], edi (list scrolled)
#define VA_PICKUP_THUMB_H 0x5c3b34u
#define VA_PICKUP_CONTAINER 0x5c3b6cu       // container *; item count at +0xd8
#define VA_PICKUP_TRACK_HEIGHT 0x58f3f4u    // int[3] by resolution_index
#define VA_PICKUP_PAGE_SIZE 0x58f400u       // int[3] by resolution_index
#define VA_RESOLUTION_INDEX_FIX 0x60ad68u
#define VA_CAMERA_EDGE_STORE 0x4346f1u      // mov [0x5fccb7], al in camera_process_input
#define VA_CAMERA_EDGE_RELOAD 0x43470au     // mov al, [0x5fccb7] (right-edge path, after gr_screen_width)
#define VA_CAMERA_EDGE_HELD 0x5fccb7u       // byte: left button held for a drag, last stored
#define VA_GOLD_TINT_BLUE 0x43a62eu         // mov ecx, [0x594430] (blue) in world_tint_gold_object
#define VA_KNOCK_MESSAGE_TEST 0x47431bu     // mov eax, [esi+0x4c] ; mov ecx, [cur_character] in ai_knock_down_local
#define VA_CUR_CHARACTER_FIX 0x5fcc88u
#define VA_FILE_INIT_PATH_SET 0x512e0bu     // mov ecx, file_zero_vector, after file_init has set install_path
#define VA_FILE_ZERO_VECTOR 0x2cbf4d8u
#define VA_INSTALL_PATH 0x26b4168u          // char[], ends with a backslash
#define VA_ITEM_IS_IDENTIFIED 0x44b690u     // cdecl (item) -> bool
#define VA_PARTY_INVENTORY 0x970824u        // 8-byte records, item pointer first
#define VA_BUYSELL_SELL_CATEGORY 0x404f98u // sell branch of buysell_category_first_index (mov eax, [buysell_list_count])
#define VA_BUYSELL_GET_LIST_ITEM 0x404960u  // cdecl (row) -> shop_item * of the shown list, NULL past the end
#define VA_ITEM_TYPE_SHOP_CATEGORY 0x46b5f0u // cdecl (item type index) -> 0 weapons .. 4 misc
#define VA_MAP_LABEL_FULL 0x430245u        // xor eax, eax ; ret (+ nop) in automap_label_alloc
#define VA_CAN_USE_SUMMON_60K 0x447390u    // jle after cmp eax, 60000 in character_can_use_summon
#define VA_SUMMON_UNLOCK_60K 0x45023au      // jle after cmp [new total], 60000 in ring_check_summon_unlock
#define VA_LIANGSHAN_PRESET 0x4f49beu      // mov byte [ls_liangshan_phoenix_dead_done], 1 in ls_liangshan_init
#define VA_LIANGSHAN_DEAD_DONE 0x257ea61u
#define VA_IKAEMOS_PRESET 0x4f9f2eu         // the two mov byte [..._done], bl in ls_ikaemostopint_init
#define VA_IKAEMOS_FOUND_DONE 0x257ecfcu    // found_boss_done; boss_killed_done follows
#define VA_SAVEGAME_STATE_FIX 0x2493a38u
#define VA_SAVEGAME_GET_FLAG_FIX 0x4cfca0u  // thiscall (level, name) -> int
#define VA_CUR_LEVEL_INDEX_FIX 0x59f120u
#define VA_MOUNTED_STAGE 0x4a2480u         // boss_species_mounted_stage
#define VA_TIGER_STAGE 0x2452a90u
#define VA_PHOENIX_STAGE 0x2452420u
#define VA_SERPENT_STAGE 0x2451d98u
#define VA_GHOST_STAGE 0x2480310u
#define VA_ZANE_COPY_END 0x4fca1bu         // end of the Zane script-name copy in ls_worldmap1_pick_encounter_stage4
#define VA_ZANE_EXIT 0x4fcae1u              // the picker's epilogue (pop edi ; pop esi ; add esp, 0xc ; ret)
#define VA_LEECH_STORE 0x45f47du         // fstp [ebp-0x24] ; jmp 0x45fe7b at the end of get_hit_by_spell case 0x3e
#define VA_LEECH_RESUME 0x45fed8u           // after spell_roll_amount + character_add_hp (the flinch test)
#define VA_LIVING_FROM_HANDLE_FIX 0x46b1a0u
#define VA_DETECT_CONE_JCC 0x493c5cu       // jne 0x493c71 after the 0.71 cone compare in ai_group_collect_enemies
#define VA_STYLE1_AFTER_ORDER 0x49354au    // add esp, 0x10 after the chosen-target order in ai_group_think_style1
#define VA_AI_BLESS_COUNT_INC 0x49395cu  // the first inc ebx in ai_group_think_caster's Bless pass
#define VA_CANIS_PAYOFF_SUB 0x4f124du      // sub eax, 0x64 (party_gold) in ls_lenele2ab_action "payoff"
#define VA_BOX_PAUSE_CALL 0x413f96u        // call game_pause in ui_message_box_show
#define VA_BOX_UNPAUSE_CALL 0x414034u       // call game_unpause in ClearFrameFlag5bd504
#define VA_GAME_IS_PAUSED 0x432da0u
#define VA_GAME_PAUSE 0x432660u
#define VA_GAME_UNPAUSE 0x4326c0u

__attribute__((used)) volatile BYTE *g_dsInited;
void DsChannelStreamGuard(void);
__asm__(".text\n"
        "_DsChannelStreamGuard:\n"
        "    movl 0xc(%esi), %eax\n"
        "    movl _g_dsInited, %ecx\n"
        "    cmpb $1, (%ecx)\n"
        "    je 1f\n"
        "    orl $-1, %eax\n"
        "1:  testl %eax, %eax\n"
        "    ret\n");

__attribute__((used)) volatile float *g_sndGroupVolumes;

// (volume at [esp+0x1c] of snd_play_3d, the sound record in esi): volume / (sound volume * group volume).
// The caller already skipped this when the sound volume is 0.
__attribute__((used)) static void __cdecl SndPlay3dUnscale(float *vol, const BYTE *snd)
{
    int group = (*(const int *)(snd + 0x3c) << 5) >> 5; // the 27-bit signed group field
    float d = *(const float *)(snd + 0x28) * g_sndGroupVolumes[group];
    *vol = d != 0.0f ? *vol / d : 0.0f;
}
void SndPlay3dUnscaleStub(void);
__asm__(".text\n"
        "_SndPlay3dUnscaleStub:\n"
        "    leal 0x20(%esp), %eax\n" // snd_play_3d's [esp+0x1c] (our return address is on top)
        "    pushl %esi\n"
        "    pushl %eax\n"
        "    call _SndPlay3dUnscale\n"
        "    addl $8, %esp\n"
        "    ret\n");

__attribute__((used)) DWORD g_popupMarginY, g_popupTitleGap;
void PopupHitTitleStub(void);
__asm__(".text\n"
        "_PopupHitTitleStub:\n"
        "    movl _g_popupMarginY, %esi\n"
        "    movl (%esi,%edx,4), %esi\n" // margin_y[resolution_index]
        "    addl %ecx, %esi\n"          // + ui_choice_popup_y
        "    pushl %ecx\n"
        "    movl _g_popupTitleGap, %ecx\n"
        "    addl (%ecx,%edx,4), %esi\n" // + title_gap[resolution_index], as ui_choice_popup_draw does
        "    popl %ecx\n"
        "    ret\n");

// eax = saveload_selected_row + saveload_scroll_pos (flags are recomputed by the test eax, eax that follows).
__attribute__((used)) DWORD g_saveSelectedRow, g_saveScrollPos;
void SaveOverwriteRowStub(void);
__asm__(".text\n"
        "_SaveOverwriteRowStub:\n"
        "    movl _g_saveSelectedRow, %eax\n"
        "    movl (%eax), %eax\n"
        "    pushl %ecx\n"
        "    movl _g_saveScrollPos, %ecx\n"
        "    addl (%ecx), %eax\n"
        "    popl %ecx\n"
        "    ret\n");

// The first row of the shop's sell list whose item is in category cat, or -1 (K3).
__attribute__((used)) static int __cdecl SellCategoryFirstRow(int cat)
{
    typedef const int *(__cdecl * GetListItem)(int);
    typedef int(__cdecl * ShopCategory)(int);
    GetListItem item = (GetListItem)(UINT_PTR)SumVa(VA_BUYSELL_GET_LIST_ITEM);
    ShopCategory category = (ShopCategory)(UINT_PTR)SumVa(VA_ITEM_TYPE_SHOP_CATEGORY);
    for (int row = 0; row < 0x209; row++) {
        const int *e = item(row);
        if (!e)
            break;
        if (category(e[0]) == cat)
            return row;
    }
    return -1;
}
// Entered by jmp at buysell_category_first_index's sell branch (esi and edi pushed, the category at [esp+0xc]).
void SellCategoryStub(void);
__asm__(".text\n"
        "_SellCategoryStub:\n"
        "    pushl 0xc(%esp)\n"
        "    call _SellCategoryFirstRow\n"
        "    addl $4, %esp\n"
        "    popl %edi\n"
        "    popl %esi\n"
        "    ret\n");

// Party-inventory index -> row of the identify-mode list: the unidentified entries before it (K4). party_inventory
// is 8-byte records with the item pointer first (0x970824); item_is_identified (0x44b690) takes the item.
__attribute__((used)) static int __cdecl IdentifyRowOf(int index)
{
    typedef char(__cdecl * IsIdentified)(void *);
    IsIdentified identified = (IsIdentified)(UINT_PTR)SumVa(VA_ITEM_IS_IDENTIFIED);
    void *const *inv = (void *const *)(UINT_PTR)SumVa(VA_PARTY_INVENTORY);
    int row = 0;
    for (int i = 0; i < index; i++)
        if (!identified(inv[2 * i]))
            row++;
    return row;
}
// Replaces mov eax, esi ; pop edi ; pop esi ; pop ebx ; ret at 0x41543e (entered by jmp).
void IdentifyRowStub(void);
__asm__(".text\n"
        "_IdentifyRowStub:\n"
        "    pushl %esi\n"
        "    call _IdentifyRowOf\n"
        "    addl $4, %esp\n"
        "    popl %edi\n"
        "    popl %esi\n"
        "    popl %ebx\n"
        "    ret\n");

// Pickup scrollbar thumb height (K12): the track height scaled by page / items, as the assess screen does.
__attribute__((used)) static void __cdecl PickupThumbHeight(void)
{
    int res = *(volatile int *)(UINT_PTR)SumVa(VA_RESOLUTION_INDEX_FIX);
    int track = ((const int *)(UINT_PTR)SumVa(VA_PICKUP_TRACK_HEIGHT))[res];
    int page = ((const int *)(UINT_PTR)SumVa(VA_PICKUP_PAGE_SIZE))[res];
    const BYTE *container = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_PICKUP_CONTAINER);
    int count = container ? *(const int *)(container + 0xd8) : 0;
    int h = count > page ? track * page / count : track;
    *(volatile int *)(UINT_PTR)SumVa(VA_PICKUP_THUMB_H) = h;
}
// Replaces mov [pickup_thumb_h], reg (6 bytes, entered by call + nop): all registers kept.
void PickupThumbStub(void);
__asm__(".text\n"
        "_PickupThumbStub:\n"
        "    pushal\n"
        "    call _PickupThumbHeight\n"
        "    popal\n"
        "    ret\n");

// Edge-turn block (K191): camera_process_input's held-drag byte is swapped, so the release test sees last frame's bit.
__attribute__((used)) static BYTE g_edgePrevHeld;
__attribute__((used)) static int __cdecl EdgeHeldSwap(int held)
{
    volatile BYTE *store = (volatile BYTE *)(UINT_PTR)SumVa(VA_CAMERA_EDGE_HELD);
    BYTE prev = *store;
    *store = (BYTE)held;
    g_edgePrevHeld = prev;
    return prev;
}
// Replaces mov [0x5fccb7], al (5 bytes, entered by call) between cmp ecx, 3 and jl: keeps the flags and ecx, edx;
// returns last frame's bit in eax.
void EdgeHeldStub(void);
__asm__(".text\n"
        "_EdgeHeldStub:\n"
        "    pushfl\n"
        "    pushl %ecx\n"
        "    pushl %edx\n"
        "    movzbl %al, %eax\n"
        "    pushl %eax\n"
        "    call _EdgeHeldSwap\n"
        "    addl $4, %esp\n"
        "    popl %edx\n"
        "    popl %ecx\n"
        "    popfl\n"
        "    ret\n");

// "Knocked Down!" only when the knock-down happened (K80). Replaces mov eax, [esi+0x4c] ; mov ecx, [cur_character]
// (9 bytes, entered by call + 4 nops) before cmp eax, ecx ; jne: with ai_set_mode's result 0 in eax the two differ.
__attribute__((used)) static volatile DWORD g_curCharacterVa;
void KnockMessageStub(void);
__asm__(".text\n"
        "_KnockMessageStub:\n"
        "    testl %eax, %eax\n"
        "    jz 1f\n"
        "    movl 0x4c(%esi), %eax\n"
        "    movl _g_curCharacterVa, %ecx\n"
        "    movl (%ecx), %ecx\n"
        "    ret\n"
        "1:\n"
        "    xorl %eax, %eax\n"
        "    movl $1, %ecx\n"
        "    ret\n");

// Long install folder (C310): file_init has just set install_path (with its trailing backslash). The game joins it
// with file names in 128-byte stack buffers (packfile_add, packfile_fopen, snd_load ...), so a long path is replaced
// by its 8.3 short form, which names the same folder.
__attribute__((used)) static volatile DWORD g_fileZeroVectorVa;
__attribute__((used)) static void __cdecl ShortenInstallPath(void)
{
    char *path = (char *)(UINT_PTR)SumVa(VA_INSTALL_PATH);
    size_t len = strlen(path);
    if (len < 64)
        return;
    char shortPath[MAX_PATH];
    DWORD n = GetShortPathNameA(path, shortPath, sizeof shortPath);
    if (n == 0 || n >= sizeof shortPath - 1 || n >= len) {
        Log("install path is %u characters and has no shorter 8.3 form: names longer than 127 characters will crash "
            "the game (game-bugs C310)",
            (unsigned)len);
        return;
    }
    if (shortPath[n - 1] != '\\') {
        shortPath[n++] = '\\';
        shortPath[n] = 0;
    }
    Log("install path shortened from %u to %u characters: %s", (unsigned)len, (unsigned)n, shortPath);
    memcpy(path, shortPath, n + 1);
}
// Replaces mov ecx, file_zero_vector (5 bytes, entered by call) where file_init's two path branches meet.
void InstallPathStub(void);
__asm__(".text\n"
        "_InstallPathStub:\n"
        "    pushal\n"
        "    call _ShortenInstallPath\n"
        "    popal\n"
        "    movl _g_fileZeroVectorVa, %ecx\n"
        "    ret\n");

// Boss music presets (K130, K150), called from the two level inits.
typedef int(__attribute__((thiscall)) * GetFlagFn)(void *, int, const char *);
static int CurLevelFlag(const char *name)
{
    int level = *(volatile int *)(UINT_PTR)SumVa(VA_CUR_LEVEL_INDEX_FIX);
    return ((GetFlagFn)(UINT_PTR)SumVa(VA_SAVEGAME_GET_FLAG_FIX))((void *)(UINT_PTR)SumVa(VA_SAVEGAME_STATE_FIX), level, name);
}
__attribute__((used)) static void __cdecl LiangshanMusicPreset(void)
{
    *(volatile BYTE *)(UINT_PTR)SumVa(VA_LIANGSHAN_DEAD_DONE) = CurLevelFlag("phoenix_dead") && CurLevelFlag("serpent_dead");
}
__attribute__((used)) static void __cdecl IkaemosMusicPreset(void)
{
    BYTE killed = CurLevelFlag("ikaemos_boss_killed") != 0;
    *(volatile BYTE *)(UINT_PTR)SumVa(VA_IKAEMOS_FOUND_DONE) = killed;
    *(volatile BYTE *)(UINT_PTR)SumVa(VA_IKAEMOS_FOUND_DONE + 1) = killed;
}
// Liangshan's preset sits between the pushed arguments of a get_flag call (ecx = savegame_state): all registers kept.
void LiangshanMusicStub(void);
__asm__(".text\n"
        "_LiangshanMusicStub:\n"
        "    pushal\n"
        "    call _LiangshanMusicPreset\n"
        "    popal\n"
        "    ret\n");

// Replaces boss_species_mounted_stage (cdecl (species) -> bool in al) (G32).
__attribute__((used)) static int __cdecl MountedStage(int species)
{
    int stage;
    switch (species) {
    case 0x57: stage = *(volatile int *)(UINT_PTR)SumVa(VA_TIGER_STAGE); return stage == 1 || stage == 2;
    case 0x58: return *(volatile int *)(UINT_PTR)SumVa(VA_PHOENIX_STAGE) == 1;
    case 0x59: return *(volatile int *)(UINT_PTR)SumVa(VA_SERPENT_STAGE) == 1;
    case 0x5a: return *(volatile int *)(UINT_PTR)SumVa(VA_GHOST_STAGE) == 1;
    }
    return 0;
}

// The rest of the Zane script-name copy, then the picker's epilogue (G6). Entered by jmp.
__attribute__((used)) DWORD g_zaneExit;
void ZaneEncounterStub(void);
__asm__(".text\n"
        "_ZaneEncounterStub:\n"
        "    movl %edx, %ecx\n"
        "    andl $3, %ecx\n"
        "    rep movsb\n"
        "    jmp *_g_zaneExit\n");

// Replaces test al, al ; je ; test ebx, ebx in saveload_load_thumbnail (al = save mode, edx = the entry); the jle
// after the call skips the file when this sets "less or equal": in save mode for entry 0 only.
__attribute__((used)) DWORD g_saveEntries;
void SaveThumbEntryStub(void);
__asm__(".text\n"
        "_SaveThumbEntryStub:\n"
        "    testb %al, %al\n"
        "    je 1f\n"
        "    cmpl _g_saveEntries, %edx\n" // equal for entry 0 -> jle taken
        "    ret\n"
        "1:  pushl %eax\n"
        "    movl $1, %eax\n"
        "    cmpl $0, %eax\n"            // greater -> jle not taken (load mode: always load)
        "    popl %eax\n"
        "    ret\n");

// character_add_hp(this = ecx, 4 args, ret 0x10) unless ecx is NULL: then al = 0 and the arguments are popped.
__attribute__((used)) DWORD g_characterAddHp;
void LuminarBeamAddHpStub(void);
__asm__(".text\n"
        "_LuminarBeamAddHpStub:\n"
        "    testl %ecx, %ecx\n"
        "    je 1f\n"
        "    jmp *_g_characterAddHp\n"
        "1:  xorl %eax, %eax\n"
        "    ret $0x10\n");

// Replaces lea ecx, [esp+0x50] ; push 1 in savegame_read_item_objects: pops our return address into edx (edx is
// loaded right after), pushes the pile's saved amount where push 1 went, sets ecx, and returns by jmp.
void GoldPileAmountStub(void);
__asm__(".text\n"
        "_GoldPileAmountStub:\n"
        "    popl %edx\n"
        "    movl 0x1034(%edi), %ecx\n" // the save buffer
        "    movl -4(%ecx,%esi), %ecx\n" // the amount saved after z
        "    testl %ecx, %ecx\n"
        "    jg 1f\n"
        "    movl $1, %ecx\n"
        "1:  pushl %ecx\n"
        "    leal 0x54(%esp), %ecx\n" // the caller's [esp+0x50] before the push
        "    jmp *%edx\n");

// game_pause (cdecl, 2 args left on the stack for it) from ui_message_box_show: g_boxPaused |= the game was running.
// game_unpause from ClearFrameFlag5bd504: only when g_boxPaused, which it clears. Both callers ignore eax.
__attribute__((used)) DWORD g_gameIsPaused, g_gamePause, g_gameUnpause;
__attribute__((used)) BYTE g_boxPaused;
void BoxPauseStub(void);
void BoxUnpauseStub(void);
__asm__(".text\n"
        "_BoxPauseStub:\n"
        "    call *_g_gameIsPaused\n"
        "    testb %al, %al\n"
        "    jne 1f\n"
        "    movb $1, _g_boxPaused\n"
        "1:  jmp *_g_gamePause\n"
        "_BoxUnpauseStub:\n"
        "    cmpb $0, _g_boxPaused\n"
        "    je 1f\n"
        "    movb $0, _g_boxPaused\n"
        "    jmp *_g_gameUnpause\n"
        "1:  ret\n");

// Leech drain (G100). Entered by jmp with the amount in st0 and get_hit_by_spell's frame in ebp: this at
// [ebp-0x1a0], the source handle at [ebp+0xc]. Leaves -amount in [ebp-0x24] and jumps past the roll (0x45fed8).
__attribute__((used)) DWORD g_leechResume, g_livingFromHandleFix, g_characterAddHpFix;
typedef char(__attribute__((thiscall)) * AddHpFn)(void *, float, int, int, int);
__attribute__((used)) static float __cdecl LeechDrain(void *self, int source, float amount)
{
    if (amount <= 0.0f)
        return 0.0f;
    ((AddHpFn)(UINT_PTR)g_characterAddHpFix)(self, -amount, source, 0, 0);
    void *src = ((void *(__cdecl *)(int))(UINT_PTR)g_livingFromHandleFix)(source);
    if (src && src != self)
        ((AddHpFn)(UINT_PTR)g_characterAddHpFix)(src, amount, source, 0, 0);
    return -amount;
}
void LeechDrainStub(void);
__asm__(".text\n"
        "_LeechDrainStub:\n"
        "    subl $4, %esp\n"
        "    fstps (%esp)\n"          // amount (third argument)
        "    pushl 0xc(%ebp)\n"       // source handle
        "    pushl -0x1a0(%ebp)\n"    // the character hit
        "    call _LeechDrain\n"
        "    addl $12, %esp\n"
        "    fstps -0x24(%ebp)\n"     // amount = -drained (0 if none), read by the flinch test
        "    jmp *_g_leechResume\n");

// esi = the removed Player. edi = party_list.next as before, or &party_list (loop skipped) in single player while
// a leader other than esi is set. eax is reloaded before its next use; it is kept anyway.
__attribute__((used)) DWORD g_partyList, g_partyLeader, g_gameFlags;
void PartyRemoveLeaderStub(void);
__asm__(".text\n"
        "_PartyRemoveLeaderStub:\n"
        "    movl _g_partyList, %edi\n"
        "    pushl %eax\n"
        "    movl _g_gameFlags, %eax\n"
        "    testb $1, (%eax)\n"
        "    je 1f\n"
        "    movl _g_partyLeader, %eax\n"
        "    movl (%eax), %eax\n"
        "    testl %eax, %eax\n"
        "    je 1f\n"
        "    cmpl %esi, %eax\n"
        "    je 1f\n"
        "    popl %eax\n"
        "    ret\n"
        "1:  movl (%edi), %edi\n"
        "    popl %eax\n"
        "    ret\n");

// [Fixes] Key=0 turns one of the fixes below off (default on); the no-sound crash fix is always on (2026-10-17).
// [Fixes] Enabled=0 (the config app's "Fix bugs in the original game") turns every one of them off.
static BOOL FixOn(const char *key)
{
    if (!GetPrivateProfileIntA("Fixes", "Enabled", 1, g_cfg.iniPath)) {
        Log("fix %s is off ([Fixes] Enabled=0)", key);
        return FALSE;
    }
    if (GetPrivateProfileIntA("Fixes", key, 1, g_cfg.iniPath))
        return TRUE;
    Log("fix %s is off ([Fixes] %s=0)", key, key);
    return FALSE;
}

void BugFixesInstall(void)
{
    g_dsInited = (volatile BYTE *)(UINT_PTR)SumVa(VA_DS_INITED);
    DWORD site = SumVa(VA_DS_CHANNEL_STREAM_CHECK);
    static const BYTE orig[5] = {0x8B, 0x46, 0x0C, 0x85, 0xC0};
    BYTE repl[5] = {0xE8};
    DWORD rel = (DWORD)(UINT_PTR)DsChannelStreamGuard - (site + 5);
    memcpy(repl + 1, &rel, 4);
    PatchBytes(VA_DS_CHANNEL_STREAM_CHECK, orig, repl, 5, "no-sound new game crash (ds_channel_release stream check)");

    if (FixOn("OpenGLTextures")) {
        static const BYTE glMemOrig[5] = {0xB8, 0x08, 0x00, 0x00, 0x00}; // mov eax, 8
        static const BYTE glMemRepl[5] = {0xB8, 0x40, 0x00, 0x00, 0x00}; // mov eax, 64
        PatchBytes(VA_GL_VIDEO_MEMORY_MB, glMemOrig, glMemRepl, 5, "OpenGL video memory reported as 8 MB (low textures on a fresh install)");
    }

    if (FixOn("RobeOfRestoration")) {
        static const BYTE robeOrig[3] = {0x8B, 0x4D, 0x00}; // mov ecx, [ebp]
        static const BYTE robeRepl[3] = {0x8B, 0xC8, 0x90}; // mov ecx, eax ; nop
        PatchBytes(VA_ARMOUR_SPELL_TARGET, robeOrig, robeRepl, 3, "armour heal aimed at the attacker (G300, Robe of Restoration)");
    }

    if (FixOn("WorldSoundVolume")) {
        // eax, ecx and edx are reloaded before their next use in snd_play_3d; the FPU stack is empty here.
        g_sndGroupVolumes = (volatile float *)(UINT_PTR)SumVa(VA_SND_GROUP_VOLUMES);
        static const BYTE sndOrig[11] = {0xD9, 0x44, 0x24, 0x1C, 0xD8, 0x76, 0x28, 0xD9, 0x5C, 0x24, 0x1C};
        BYTE sndRepl[11] = {0xE8, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
        DWORD sndSite = SumVa(VA_SND_PLAY_3D_DIVIDE);
        DWORD sndRel = (DWORD)(UINT_PTR)SndPlay3dUnscaleStub - (sndSite + 5);
        memcpy(sndRepl + 1, &sndRel, 4);
        PatchBytes(VA_SND_PLAY_3D_DIVIDE, sndOrig, sndRepl, 11, "world sounds at the sfx volume squared without DS3D (snd_play_3d)");
    }

    if (FixOn("PopupRowOffset")) {
        g_popupMarginY = SumVa(VA_POPUP_MARGIN_Y);
        g_popupTitleGap = SumVa(VA_POPUP_TITLE_GAP);
        static const BYTE popOrig[11] = {0x8B, 0x34, 0x95, 0x70, 0xEF, 0x57, 0x00, 0x03, 0xF1, 0xEB, 0x11};
        BYTE popRepl[11] = {0xE8, 0, 0, 0, 0, 0xEB, 0x15, 0x90, 0x90, 0x90, 0x90}; // call stub ; jmp 0x4139cf
        DWORD popSite = SumVa(VA_POPUP_HIT_TITLE);
        DWORD popRel = (DWORD)(UINT_PTR)PopupHitTitleStub - (popSite + 5);
        memcpy(popRepl + 1, &popRel, 4);
        PatchBytes(VA_POPUP_HIT_TITLE, popOrig, popRepl, 11, "choice popup rows 5 px above their text (K170)");
    }

    if (FixOn("MetalHitEffects")) {
        static const BYTE bloodOrig[5] = {0x83, 0xFF, 0x01, 0x75, 0x1E}; // cmp edi, 1 ; jne 0x466a2f
        static const BYTE bloodRepl[5] = {0x83, 0xFF, 0x02, 0x77, 0x1E}; // cmp edi, 2 ; ja 0x466a2f
        PatchBytes(VA_HIT_VFX_BLOOD_TEST, bloodOrig, bloodRepl, 5, "blood hit effect for leather (K20)");
        static const BYTE sparkOrig[10] = {0x83, 0xFF, 0x03, 0x74, 0x05, 0x83, 0xFF, 0x09, 0x75, 0x1B};
        static const BYTE sparkRepl[10] = {0xEB, 0x08, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}; // jmp 0x466a39
        PatchBytes(VA_HIT_VFX_SPARK_TEST, sparkOrig, sparkRepl, 10, "spark hit effect for metal characters (K20)");
    }

    if (FixOn("SaveOverwriteQuestion")) {
        g_saveSelectedRow = SumVa(VA_SAVELOAD_SELECTED_ROW);
        g_saveScrollPos = SumVa(VA_SAVELOAD_SCROLL_POS);
        static const BYTE rowOrig[5] = {0xA1, 0x0C, 0x36, 0x5E, 0x00}; // mov eax, [0x5e360c]
        BYTE rowRepl[5] = {0xE8};
        DWORD rowSite = SumVa(VA_SAVE_OVERWRITE_ROW);
        DWORD rowRel = (DWORD)(UINT_PTR)SaveOverwriteRowStub - (rowSite + 5);
        memcpy(rowRepl + 1, &rowRel, 4);
        PatchBytes(VA_SAVE_OVERWRITE_ROW, rowOrig, rowRepl, 5, "save overwrite question on a scrolled list (G4)");
    }

    if (FixOn("LuminarBeamTarget")) {
        g_characterAddHp = SumVa(VA_CHARACTER_ADD_HP);
        DWORD s = SumVa(VA_LUMINAR_BEAM_ADD_HP);
        BYTE o[5] = {0xE8}, r[5] = {0xE8};
        DWORD ro = g_characterAddHp - (s + 5), rn = (DWORD)(UINT_PTR)LuminarBeamAddHpStub - (s + 5);
        memcpy(o + 1, &ro, 4);
        memcpy(r + 1, &rn, 4);
        PatchBytes(VA_LUMINAR_BEAM_ADD_HP, o, r, 5, "Luminar beam hit on a target that is gone (C250)");
    }

    if (FixOn("SummonKeepsLeader")) {
        g_partyList = SumVa(VA_PARTY_LIST);
        g_partyLeader = SumVa(VA_PARTY_LEADER);
        g_gameFlags = SumVa(VA_GAME_FLAGS);
        static const BYTE o[6] = {0x8B, 0x3D, 0x50, 0xC3, 0x5F, 0x00}; // mov edi, [party_list]
        BYTE r[6] = {0xE8, 0, 0, 0, 0, 0x90};
        DWORD s = SumVa(VA_PARTY_REMOVE_LEADER_LOOP);
        DWORD rel = (DWORD)(UINT_PTR)PartyRemoveLeaderStub - (s + 5);
        memcpy(r + 1, &rel, 4);
        PatchBytes(VA_PARTY_REMOVE_LEADER_LOOP, o, r, 6, "party leader kept when another record is removed (G8)");
    }

    if (FixOn("SpellRecoveryTimes")) {
        // mov [esi-0x12], bp (+Recovery time), [esi-0x10] (+AI Recovery time), [esi-0xe] (+Loop Time)
        static const DWORD sites[3] = {0x47a49cu, 0x47a4c1u, 0x47a50bu};
        static const BYTE disp[3] = {0xEE, 0xF0, 0xF2};
        for (int i = 0; i < 3; i++) {
            BYTE o[4] = {0x66, 0x89, 0x6E, disp[i]}; // mov [esi-N], bp
            BYTE r[4] = {0x66, 0x89, 0x46, disp[i]}; // mov [esi-N], ax (the parsed value)
            PatchBytes(sites[i], o, r, 4, "spell recovery times from spells.tbl (G12)");
        }
    }

    if (FixOn("GoldPileAmount")) {
        static const BYTE o[6] = {0x8D, 0x4C, 0x24, 0x50, 0x6A, 0x01}; // lea ecx, [esp+0x50] ; push 1
        BYTE r[6] = {0xE8, 0, 0, 0, 0, 0x90};
        DWORD s = SumVa(VA_GOLD_PILE_SPAWN_AMOUNT);
        DWORD rel = (DWORD)(UINT_PTR)GoldPileAmountStub - (s + 5);
        memcpy(r + 1, &rel, 4);
        PatchBytes(VA_GOLD_PILE_SPAWN_AMOUNT, o, r, 6, "loose gold pile keeps its amount after a load (G40)");
    }

    if (FixOn("MessageBoxPause")) {
        g_gameIsPaused = SumVa(VA_GAME_IS_PAUSED);
        g_gamePause = SumVa(VA_GAME_PAUSE);
        g_gameUnpause = SumVa(VA_GAME_UNPAUSE);
        static const struct { DWORD va, target; void (*stub)(void); } calls[2] = {
            {VA_BOX_PAUSE_CALL, VA_GAME_PAUSE, BoxPauseStub},
            {VA_BOX_UNPAUSE_CALL, VA_GAME_UNPAUSE, BoxUnpauseStub},
        };
        for (int i = 0; i < 2; i++) {
            DWORD s = SumVa(calls[i].va);
            BYTE o[5] = {0xE8}, r[5] = {0xE8};
            DWORD ro = SumVa(calls[i].target) - (s + 5), rn = (DWORD)(UINT_PTR)calls[i].stub - (s + 5);
            memcpy(o + 1, &ro, 4);
            memcpy(r + 1, &rn, 4);
            PatchBytes(calls[i].va, o, r, 5, "message box unpauses only what it paused (G170)");
        }
    }

    if (FixOn("CanisTenGold")) {
        static const BYTE o[3] = {0x83, 0xE8, 0x64}, r[3] = {0x83, 0xE8, 0x0A}; // sub eax, 100 -> sub eax, 10
        PatchBytes(VA_CANIS_PAYOFF_SUB, o, r, 3, "Canis's payoff takes 10 gold, as offered (G301)");
    }

    if (FixOn("AiBlessCount")) {
        static const BYTE o[1] = {0x43}, r[1] = {0x90}; // inc ebx -> nop
        PatchBytes(VA_AI_BLESS_COUNT_INC, o, r, 1, "AI Bless candidates counted once (C210, G212)");
    }

    if (FixOn("AreaCasterGone")) {
        static const BYTE o[6] = {0x0F, 0x84, 0x91, 0x01, 0x00, 0x00}; // je 0x48b5a1 (end of the area loop)
        static const BYTE r[6] = {0x0F, 0x84, 0x71, 0x01, 0x00, 0x00}; // je 0x48b581 (next area)
        PatchBytes(VA_AREA_CASTER_GONE_JE, o, r, 6, "burning area with its caster gone skipped, not the rest (G270)");
    }

    if (FixOn("SupportAiTarget")) {
        static const BYTE o[3] = {0x83, 0xC4, 0x10}, r[3] = {0xEB, 0x18, 0x90}; // add esp, 0x10 -> jmp 0x493564
        PatchBytes(VA_STYLE1_AFTER_ORDER, o, r, 3, "support AI keeps the target it picked (G211)");
    }

    if (FixOn("DetectionCone")) {
        static const BYTE o[1] = {0x75}, r[1] = {0x74}; // jne -> je (rel8 0x13 to 0x493c71 kept)
        PatchBytes(VA_DETECT_CONE_JCC, o, r, 1, "enemies seen inside the forward cone, not outside (G210)");
    }

    if (FixOn("LeechDrain")) {
        g_leechResume = SumVa(VA_LEECH_RESUME);
        g_livingFromHandleFix = SumVa(VA_LIVING_FROM_HANDLE_FIX);
        g_characterAddHpFix = SumVa(VA_CHARACTER_ADD_HP);
        static const BYTE o[8] = {0xD9, 0x5D, 0xDC, 0xE9, 0xF6, 0x09, 0x00, 0x00}; // fstp [ebp-0x24] ; jmp 0x45fe7b
        BYTE r[8] = {0xE9, 0, 0, 0, 0, 0x90, 0x90, 0x90};
        DWORD s = SumVa(VA_LEECH_STORE);
        DWORD rel = (DWORD)(UINT_PTR)LeechDrainStub - (s + 5);
        memcpy(r + 1, &rel, 4);
        PatchBytes(VA_LEECH_STORE, o, r, 8, "leech drains the target into the attacker (G100)");
    }

    if (FixOn("ConversationHover")) {
        // "more": height [esp+0x18], mouse y [esp+0x1c]; "end": height [esp+0x10], mouse y [esp+0x14]
        static const BYTE mo[8] = {0x8B, 0x54, 0x24, 0x18, 0x03, 0xCA, 0x3B, 0xC1};
        static const BYTE mr[8] = {0x03, 0x4C, 0x24, 0x18, 0x39, 0x4C, 0x24, 0x1C};
        static const BYTE eo[8] = {0x8B, 0x54, 0x24, 0x10, 0x03, 0xCA, 0x3B, 0xC1};
        static const BYTE er[8] = {0x03, 0x4C, 0x24, 0x10, 0x39, 0x4C, 0x24, 0x14};
        PatchBytes(VA_DIALOGUE_MORE_HOVER, mo, mr, 8, "conversation more / end highlight under the mouse (K292)");
        PatchBytes(VA_DIALOGUE_END_HOVER, eo, er, 8, "conversation more / end highlight under the mouse (K292)");
    }

    if (FixOn("SaveListThumbnail")) {
        g_saveSelectedRow = SumVa(VA_SAVELOAD_SELECTED_ROW);
        g_saveScrollPos = SumVa(VA_SAVELOAD_SCROLL_POS);
        g_saveEntries = SumVa(VA_SAVELOAD_ENTRIES);
        static const BYTE lo[8] = {0x84, 0xC0, 0x74, 0x04, 0x85, 0xDB, 0x7E, 0x0D}; // test al,al; je; test ebx,ebx; jle
        BYTE lr[8] = {0xE8, 0, 0, 0, 0, 0x7E, 0x0E, 0x90};                         // call stub; jle 0x425c76
        DWORD s = SumVa(VA_SAVE_THUMB_LOAD_TEST);
        DWORD rel = (DWORD)(UINT_PTR)SaveThumbEntryStub - (s + 5);
        memcpy(lr + 1, &rel, 4);
        PatchBytes(VA_SAVE_THUMB_LOAD_TEST, lo, lr, 8, "scrolled save list thumbnails (K5)");
        static const BYTE dor[5] = {0xA1, 0x0C, 0x36, 0x5E, 0x00}; // mov eax, [saveload_selected_row]
        BYTE drr[5] = {0xE8};
        s = SumVa(VA_SAVE_THUMB_DRAW_ROW);
        rel = (DWORD)(UINT_PTR)SaveOverwriteRowStub - (s + 5);
        memcpy(drr + 1, &rel, 4);
        PatchBytes(VA_SAVE_THUMB_DRAW_ROW, dor, drr, 5, "scrolled save list thumbnails (K5)");
    }

    if (FixOn("ZaneEncounter")) {
        g_zaneExit = SumVa(VA_ZANE_EXIT);
        static const BYTE o[7] = {0x8B, 0xCA, 0x83, 0xE1, 0x03, 0xF3, 0xA4}; // mov ecx, edx ; and ecx, 3 ; rep movsb
        BYTE r[7] = {0xE9, 0, 0, 0, 0, 0x90, 0x90};
        DWORD s = SumVa(VA_ZANE_COPY_END);
        DWORD rel = (DWORD)(UINT_PTR)ZaneEncounterStub - (s + 5);
        memcpy(r + 1, &rel, 4);
        PatchBytes(VA_ZANE_COPY_END, o, r, 7, "Zane's encounter kept, not overwritten by the general rolls (G6)");
    }

    if (FixOn("MountedRiders")) {
        static const BYTE o[5] = {0x8B, 0x44, 0x24, 0x04, 0x83}; // mov eax, [esp+4] ; (cmp eax, 0x57)
        BYTE r[5] = {0xE9};
        DWORD s = SumVa(VA_MOUNTED_STAGE);
        DWORD rel = (DWORD)(UINT_PTR)MountedStage - (s + 5);
        memcpy(r + 1, &rel, 4);
        PatchBytes(VA_MOUNTED_STAGE, o, r, 5, "each rider's own stage in the mounted test (G32)");
    }

    if (FixOn("BossMusic")) {
        static const BYTE lo[7] = {0xC6, 0x05, 0x61, 0xEA, 0x57, 0x02, 0x01}; // mov byte [phoenix_dead_done], 1
        BYTE lr[7] = {0xE8, 0, 0, 0, 0, 0x90, 0x90};
        DWORD s = SumVa(VA_LIANGSHAN_PRESET);
        DWORD rel = (DWORD)(UINT_PTR)LiangshanMusicStub - (s + 5);
        memcpy(lr + 1, &rel, 4);
        PatchBytes(VA_LIANGSHAN_PRESET, lo, lr, 7, "boss music presets from the saved flags (K130)");
        static const BYTE io[12] = {0x88, 0x1D, 0xFC, 0xEC, 0x57, 0x02, 0x88, 0x1D, 0xFD, 0xEC, 0x57, 0x02}; // mov [..], bl x2
        BYTE ir[12] = {0xE8, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
        s = SumVa(VA_IKAEMOS_PRESET);
        rel = (DWORD)(UINT_PTR)IkaemosMusicPreset - (s + 5);
        memcpy(ir + 1, &rel, 4);
        PatchBytes(VA_IKAEMOS_PRESET, io, ir, 12, "boss music presets from the saved flags (K150)");
    }

    if (FixOn("RingSixtyThousand")) {
        static const BYTE co[2] = {0x7E, 0x04}, cr[2] = {0x7C, 0x04};                  // jle -> jl (character_can_use_summon)
        static const BYTE uo[2] = {0x0F, 0x8E}, ur[2] = {0x0F, 0x8C};                  // jle -> jl (ring_check_summon_unlock)
        PatchBytes(VA_CAN_USE_SUMMON_60K, co, cr, 2, "ring summons from 60,000 on (K100)");
        PatchBytes(VA_SUMMON_UNLOCK_60K, uo, ur, 2, "ring summons from 60,000 on (K100)");
    }

    if (FixOn("MapLabelLimit")) {
        static const BYTE o[4] = {0x33, 0xC0, 0xC3, 0x90}, r[4] = {0x83, 0xC8, 0xFF, 0xC3}; // xor eax,eax; ret -> or eax,-1; ret
        PatchBytes(VA_MAP_LABEL_FULL, o, r, 4, "a 33rd map label is refused, not written over the first (K6)");
    }

    if (FixOn("ShopCategoryButtons")) {
        static const BYTE o[5] = {0xA1, 0x00, 0x64, 0x5B, 0x00}; // mov eax, [buysell_list_count]
        BYTE r[5] = {0xE9};
        DWORD s = SumVa(VA_BUYSELL_SELL_CATEGORY);
        DWORD rel = (DWORD)(UINT_PTR)SellCategoryStub - (s + 5);
        memcpy(r + 1, &rel, 4);
        PatchBytes(VA_BUYSELL_SELL_CATEGORY, o, r, 5, "shop sell list category buttons use the shown rows (K3)");
    }

    if (FixOn("InventoryCategoryButtons")) {
        static const BYTE o[6] = {0x8B, 0xC6, 0x5F, 0x5E, 0x5B, 0xC3}; // mov eax, esi ; pop edi ; pop esi ; pop ebx ; ret
        BYTE r[6] = {0xE9, 0, 0, 0, 0, 0x90};
        DWORD s = SumVa(VA_INVENTORY_CATEGORY_RET);
        DWORD rel = (DWORD)(UINT_PTR)IdentifyRowStub - (s + 5);
        memcpy(r + 1, &rel, 4);
        PatchBytes(VA_INVENTORY_CATEGORY_RET, o, r, 6, "identify-mode category buttons use the shown rows (K4)");
    }

    if (FixOn("AmbientSounds")) {
        static const BYTE o[4] = {0x83, 0xC4, 0x04, 0x46}, r[4] = {0x83, 0xC4, 0x04, 0x90}; // inc esi -> nop
        PatchBytes(VA_AMBIENT_REMOVE_INC, o, r, 4, "leaving a level frees all its ambient sounds (K110)");
    }

    if (FixOn("PickupScrollbar")) {
        static const BYTE oTop[6] = {0x89, 0x15, 0x34, 0x3B, 0x5C, 0x00};      // mov [0x5c3b34], edx
        static const BYTE oScrolled[6] = {0x89, 0x3D, 0x34, 0x3B, 0x5C, 0x00}; // mov [0x5c3b34], edi
        const DWORD at[2] = {VA_PICKUP_THUMB_H_TOP, VA_PICKUP_THUMB_H_SCROLLED};
        const BYTE *orig[2] = {oTop, oScrolled};
        for (int i = 0; i < 2; i++) {
            BYTE r[6] = {0xE8, 0, 0, 0, 0, 0x90};
            DWORD rel = (DWORD)(UINT_PTR)PickupThumbStub - (SumVa(at[i]) + 5);
            memcpy(r + 1, &rel, 4);
            PatchBytes(at[i], orig[i], r, 6, "pickup scrollbar thumb sized by page / items (K12)");
        }
    }

    if (FixOn("EdgeTurnAfterDrag")) {
        static const BYTE so[5] = {0xA2, 0xB7, 0xCC, 0x5F, 0x00}; // mov [0x5fccb7], al
        static const BYTE ro[5] = {0xA0, 0xB7, 0xCC, 0x5F, 0x00}; // mov al, [0x5fccb7]
        BYTE sr[5] = {0xE8}, rr[5] = {0xA0};
        DWORD rel = (DWORD)(UINT_PTR)EdgeHeldStub - (SumVa(VA_CAMERA_EDGE_STORE) + 5), prev = (DWORD)(UINT_PTR)&g_edgePrevHeld;
        memcpy(sr + 1, &rel, 4);
        memcpy(rr + 1, &prev, 4);
        if (PatchBytes(VA_CAMERA_EDGE_STORE, so, sr, 5, "edge turn held back after a drag ends at the edge (K191)"))
            PatchBytes(VA_CAMERA_EDGE_RELOAD, ro, rr, 5, "edge turn: right edge reads last frame's drag bit (K191)");
    }

    if (FixOn("LootHighlightColour")) {
        static const BYTE o[6] = {0x8B, 0x0D, 0x30, 0x44, 0x59, 0x00}, r[6] = {0x8B, 0x0D, 0x2C, 0x44, 0x59, 0x00};
        PatchBytes(VA_GOLD_TINT_BLUE, o, r, 6, "loot highlight reads its blue, not the string \"gold\" (K61)");
    }

    if (FixOn("KnockDownMessage")) {
        static const BYTE o[9] = {0x8B, 0x46, 0x4C, 0x8B, 0x0D, 0x88, 0xCC, 0x5F, 0x00};
        BYTE r[9] = {0xE8, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90};
        DWORD rel = (DWORD)(UINT_PTR)KnockMessageStub - (SumVa(VA_KNOCK_MESSAGE_TEST) + 5);
        memcpy(r + 1, &rel, 4);
        g_curCharacterVa = SumVa(VA_CUR_CHARACTER_FIX);
        PatchBytes(VA_KNOCK_MESSAGE_TEST, o, r, 9, "\"Knocked Down!\" only when the character fell (K80)");
    }

    if (FixOn("LongInstallPath")) {
        static const BYTE o[5] = {0xB9, 0xD8, 0xF4, 0xCB, 0x02}; // mov ecx, 0x2cbf4d8
        BYTE r[5] = {0xE8};
        DWORD rel = (DWORD)(UINT_PTR)InstallPathStub - (SumVa(VA_FILE_INIT_PATH_SET) + 5);
        memcpy(r + 1, &rel, 4);
        g_fileZeroVectorVa = SumVa(VA_FILE_ZERO_VECTOR);
        PatchBytes(VA_FILE_INIT_PATH_SET, o, r, 5, "a long game folder path is shortened to its 8.3 form (C310)");
    }

    if (FixOn("PhoenixInferno")) {
        static const BYTE dOrig[3][3] = {{0xD9, 0x46, 0x10}, {0xD9, 0x46, 0x14}, {0xD9, 0x46, 0x18}}; // fld [esi+N]
        static const BYTE dRepl[3][3] = {{0xD9, 0x43, 0x10}, {0xD9, 0x43, 0x14}, {0xD9, 0x43, 0x18}}; // fld [ebx+N]
        for (int i = 0; i < 3; i++)
            PatchBytes(VA_INFERNO_DIST_X + 6 * i, dOrig[i], dRepl[i], 3, "Phoenix Inferno distance per party member (G215)");
        static const BYTE fxOrig[6] = {0x8B, 0x8E, 0x80, 0x00, 0x00, 0x00}; // mov ecx, [esi+0x80]
        static const BYTE fxRepl[6] = {0x8B, 0x8B, 0x80, 0x00, 0x00, 0x00}; // mov ecx, [ebx+0x80]
        PatchBytes(VA_INFERNO_FX_TARGET, fxOrig, fxRepl, 6, "Phoenix Inferno effect on each member (G215)");
        static const BYTE thisOrig[2] = {0x8B, 0xCE}; // mov ecx, esi
        static const BYTE thisRepl[2] = {0x8B, 0xCB}; // mov ecx, ebx
        PatchBytes(VA_INFERNO_HIT_THIS, thisOrig, thisRepl, 2, "Phoenix Inferno hits each member once (G215)");
    }
}
