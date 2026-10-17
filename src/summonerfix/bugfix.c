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
#define VA_AI_BLESS_COUNT_INC 0x49395cu   // the first inc ebx in ai_group_think_caster's Bless pass
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
static BOOL FixOn(const char *key)
{
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
