// cheats.c - [Cheats] (2026-11-03, at the owner's request; docs/summonerfix/index.md). Off by default.
//   XpMultiplier=1      every experience award is multiplied (kills, quests, the add_xp console command): a hook at
//                       the entry of character_add_xp (0x44fab0, thiscall (xp, quiet, from_net)). Read at start.
//   GoldKey=            a key that adds GoldAmount (1000) gold to the party (party_gold, 0x971868), in play.
//   SkillPointKey=      a key that gives the selected character SkillPointAmount (1) skill points to spend on the
//                       character screen (its Player record's skill_points, +0x17c), in play.
// The keys run from keys.c's message handler, so on the game thread, and are re-read with the other key sections
// when the ini changes. Each shows a HUD message. In a rebuilt exe whose source has the multiplier (the .fixes
// key XpMultiplier) the hook stands down; the keys work there through the address table.
#include "summonerfix.h"
#include "keynames.h"
#include <stdio.h>
#include <stdlib.h>

#define VA_CHARACTER_ADD_XP 0x44fab0u      // push ebp ; mov ebp, esp ; sub esp, 0x90 (9 bytes)
#define VA_CHARACTER_ADD_XP_BODY 0x44fab9u // push esi
#define VA_PARTY_GOLD 0x971868u
#define VA_CUR_CHARACTER 0x5fcc88u
#define VA_PLAYER_FROM_HANDLE 0x433eb0u    // cdecl (object handle) -> Player*
#define VA_HUD_SHOW_MESSAGE 0x414a20u      // cdecl (const char *text), at most 63 characters, 4 s
#define VA_GAMESEQ_STATE_CUR 0x5b27e0u     // 4 = in game
#define CHAR_HANDLE 0x80                    // Character (object) handle
#define PLAYER_SKILL_POINTS 0x17c

__attribute__((used)) static volatile float g_xpMult = 1.0f;
__attribute__((used)) static volatile DWORD g_addXpBodyVa;
static volatile int g_goldScan, g_goldAmount = 1000, g_skillScan, g_skillAmount = 1;

__attribute__((used)) static int __cdecl CheatScaleXp(int xp)
{
    float v = (float)xp * g_xpMult;
    return (int)(v >= 0.0f ? v + 0.5f : v - 0.5f);
}
void AddXpStub(void);
__asm__(".text\n"
        "_AddXpStub:\n"
        "    pushl %ecx\n"        // this (thiscall)
        "    pushl 8(%esp)\n"     // xp: [esp+4] at entry
        "    call _CheatScaleXp\n"
        "    addl $4, %esp\n"
        "    popl %ecx\n"
        "    movl %eax, 4(%esp)\n"
        "    pushl %ebp\n"        // the replaced prologue
        "    movl %esp, %ebp\n"
        "    subl $0x90, %esp\n"
        "    jmp *_g_addXpBodyVa\n");

static int KeyFromIni(const char *key)
{
    char name[32];
    GetPrivateProfileStringA("Cheats", key, "", name, sizeof name, g_cfg.iniPath);
    if (!name[0] || !_stricmp(name, "none"))
        return 0;
    int scan = KeyParse(name);
    if (scan <= 0 || scan > 0xff) {
        Log("cheats: [Cheats] %s=%s is not a key name", key, name);
        return 0;
    }
    return scan;
}

static int Clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

void CheatsLoadKeys(void)
{
    int gold = KeyFromIni("GoldKey"), skill = KeyFromIni("SkillPointKey");
    int goldAmount = Clamp(GetPrivateProfileIntA("Cheats", "GoldAmount", 1000, g_cfg.iniPath), 1, 1000000);
    int skillAmount = Clamp(GetPrivateProfileIntA("Cheats", "SkillPointAmount", 1, g_cfg.iniPath), 1, 100);
    // Log only on a change: the log is in the watched folder (see keys.c).
    if (gold != g_goldScan || skill != g_skillScan || goldAmount != g_goldAmount || skillAmount != g_skillAmount)
        Log("cheats: gold key %s (%d), skill point key %s (%d)", gold ? "set" : "none", goldAmount,
            skill ? "set" : "none", skillAmount);
    g_goldScan = gold;
    g_skillScan = skill;
    g_goldAmount = goldAmount;
    g_skillAmount = skillAmount;
}

static void Message(const char *text)
{
    ((void(__cdecl *)(const char *))(UINT_PTR)SumVa(VA_HUD_SHOW_MESSAGE))(text);
}

// From keys.c's message handler (game thread), for every key press.
void CheatsKey(int scan)
{
    if (!scan || (scan != g_goldScan && scan != g_skillScan))
        return;
    if (*(volatile int *)(UINT_PTR)SumVa(VA_GAMESEQ_STATE_CUR) != 4)
        return;
    char msg[64];
    if (scan == g_goldScan) {
        int *gold = (int *)(UINT_PTR)SumVa(VA_PARTY_GOLD);
        *gold += g_goldAmount;
        _snprintf(msg, sizeof msg, "Cheat: +%d gold (%d)", g_goldAmount, *gold);
        Message(msg);
        Log("cheats: +%d gold, now %d", g_goldAmount, *gold);
    }
    if (scan == g_skillScan) {
        BYTE *ch = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
        BYTE *pl = ch ? ((BYTE * (__cdecl *)(int))(UINT_PTR)SumVa(VA_PLAYER_FROM_HANDLE))(*(int *)(ch + CHAR_HANDLE)) : NULL;
        if (!pl)
            return;
        int *points = (int *)(pl + PLAYER_SKILL_POINTS);
        *points += g_skillAmount;
        _snprintf(msg, sizeof msg, "Cheat: +%d skill points (%d)", g_skillAmount, *points);
        Message(msg);
        Log("cheats: +%d skill points, now %d", g_skillAmount, *points);
    }
}

void CheatsInstall(void)
{
    char v[32];
    GetPrivateProfileStringA("Cheats", "XpMultiplier", "1", v, sizeof v, g_cfg.iniPath);
    float m = (float)atof(v);
    if (m < 0.1f || m > 100.0f) {
        if (m != 1.0f)
            Log("cheats: XpMultiplier=%s outside 0.1-100, ignored", v);
        m = 1.0f;
    }
    if (m != 1.0f) {
        if (RebuiltFixInSource("XpMultiplier")) {
            Log("cheats: XpMultiplier %.2f is in this exe's source; hook not applied", (double)m);
        } else {
            g_xpMult = m;
            g_addXpBodyVa = SumVa(VA_CHARACTER_ADD_XP_BODY);
            static const BYTE o[9] = {0x55, 0x8B, 0xEC, 0x81, 0xEC, 0x90, 0x00, 0x00, 0x00};
            BYTE r[9] = {0xE9, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90};
            DWORD rel = (DWORD)(UINT_PTR)AddXpStub - (SumVa(VA_CHARACTER_ADD_XP) + 5);
            memcpy(r + 1, &rel, 4);
            char what[64];
            _snprintf(what, sizeof what, "cheats: experience x%.2f", (double)m);
            PatchBytes(VA_CHARACTER_ADD_XP, o, r, 9, what);
        }
    }
    CheatsLoadKeys();
}
