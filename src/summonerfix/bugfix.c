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
#define VA_SAVE_OVERWRITE_ROW 0x426340u     // mov eax, [0x5e360c] (saveload_selected_row) in saveload_try_save
#define VA_SAVELOAD_SELECTED_ROW 0x5e360cu  // int saveload_selected_row
#define VA_SAVELOAD_SCROLL_POS 0x5c5b9cu    // int saveload_scroll_pos (first visible row)

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
}
