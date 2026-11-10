// Agent mode ([Debug] Agent=1): lets a script play the game without the real mouse and keyboard.
// A thread polls <game>\agent\cmd.txt (written atomically by tools/agent.mjs), runs its commands and
// writes agent\result.txt. Input goes straight into the game (key_mark / mouse_mark_button, see keys.c),
// the cursor position replaces the real one (display.c ScreenToClient hook), and screenshots are taken
// from the back buffer. Coordinates are in screenshot pixels (half the window's client size).
// The game is kept "in the foreground" so it does not pause when another window has focus.
//
// Commands (one per line, ';' also separates):
//   key NAME [ms]      press and release a game key (names as in keynames.h, or 0xNN), default 80 ms
//   keydown NAME / keyup NAME
//   move X Y           set the cursor
//   click [left|right|middle] [X Y] [ms]   move (optional), press, hold (default 80 ms), release
//   down BUTTON / up BUTTON                mouse button 0-2 or left/right/middle
//   drag X1 Y1 X2 Y2   left-button drag
//   pad LX LY RX RY BUTTONS [MS]   a synthetic controller state for MS ms (default 100), through controller.c's
//                      own code: sticks -32768..32767 (LY/RY up is positive), BUTTONS the XINPUT_GAMEPAD bit mask
//                      in hex (1000 A, 2000 B, 4000 X, 8000 Y, 100 LB, 200 RB, 10 Start, 20 Back, 1/2/4/8 D-pad
//                      up/down/left/right, 40/80 stick clicks) plus 10000 LT and 20000 RT. Waits until the state has
//                      ended and its keys are released. Works with [Controller] Enabled=0; uses its mapping.
//   wait MS
//   capture NAME       screenshot to agent\NAME.bmp (waits until written)
//   info               game state, screen, fps, cursor
//   peek ADDR [N]      hex dump of game memory (N bytes, default 16)
//   poke ADDR BYTE     write one byte of (writable) game data, both in hex
//   quit               ask the game to exit (game state 3)
#include "summonerfix.h"
#include "keynames.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VA_GAMESEQ_STATE_CUR 0x5b27e0u
#define VA_INTERFACE_CUR_SCREEN 0x5bd814u
#define VA_FPS 0x5a5744u
#define VA_IS_MAIN_WND_ACTIVE 0x2cbff1cu
#define VA_GAMESEQ_SET_STATE 0x401020u
#define VA_SAVEGAME_STATE 0x2493a38u    // savegame_state, the story flags of every level
#define VA_SAVEGAME_GET_FLAG 0x4cfca0u // thiscall (level, name) -> int
#define VA_SAVEGAME_SET_FLAG 0x4cfd30u // thiscall (level, name, bool value)
#define VA_CUR_LEVEL_INDEX 0x59f120u
#define VA_NAVPOINT_GET_POSITION 0x4dc600u // cdecl (name, vector3 *pos, matrix *orient) -> bool
#define VA_CUR_CHARACTER 0x5fcc88u         // Character *; position at +0x10
#define VA_CUR_SUMMON 0x5fcc8cu            // Character *
#define VA_SCRIPT_OBJECT_FIND_CHARACTER 0x4cde70u // cdecl (name) -> Character * of a level script object

static char g_dir[MAX_PATH];
static char g_out[16384];
static size_t g_outLen;

// Game-thread calls: KeysAgentRun posts fn to the game window, and this waits (up to 10 s) until it has run, so the
// next command cannot change the shared arguments (g_callAddr ...) before the game thread reads them.
static void (*volatile g_syncFn)(void *);
static volatile LONG g_syncDone;
static void SyncTrampoline(void *arg)
{
    g_syncFn(arg);
    InterlockedExchange(&g_syncDone, 1);
}
static BOOL AgentRunSync(void (*fn)(void *), void *arg)
{
    g_syncFn = fn;
    InterlockedExchange(&g_syncDone, 0);
    if (!KeysAgentRun(SyncTrampoline, arg))
        return FALSE;
    for (int i = 0; i < 2000 && !g_syncDone; i++)
        Sleep(5);
    if (!g_syncDone)
        Log("agent: a game-thread call has not run after 10 s; the next command may change its arguments");
    return TRUE;
}

static void Out(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = _vsnprintf(g_out + g_outLen, sizeof g_out - g_outLen - 2, fmt, ap);
    va_end(ap);
    if (n > 0)
        g_outLen += (size_t)n;
    g_out[g_outLen++] = '\n';
    g_out[g_outLen] = 0;
}

static void Path(char *out, const char *name)
{
    _snprintf(out, MAX_PATH, "%s\\%s", g_dir, name);
    out[MAX_PATH - 1] = 0;
}

static int Button(const char *s)
{
    if (!s || !*s) return 0;
    if (!_stricmp(s, "left")) return 0;
    if (!_stricmp(s, "right")) return 1;
    if (!_stricmp(s, "middle")) return 2;
    return atoi(s);
}

static void Move(int x, int y) { DisplaySetAgentCursor(x * 2, y * 2); }

static void Capture(const char *name)
{
    char path[MAX_PATH], file[MAX_PATH];
    _snprintf(file, sizeof file, "%s.bmp", name);
    file[sizeof file - 1] = 0;
    Path(path, file);
    DisplayRequestCapture(path);
    for (int i = 0; i < 200 && DisplayCapturePending(); i++)
        Sleep(10);
    Out(DisplayCapturePending() ? "capture %s: timed out (is the game drawing?)" : "capture %s: ok", file);
}

static void Info(void)
{
    int cx, cy;
    DisplayGetAgentCursor(&cx, &cy);
    Out("state=%d screen=%d fps=%.1f foreground=%d cursor=%d,%d",
        *(volatile int *)(UINT_PTR)SumVa(VA_GAMESEQ_STATE_CUR), *(volatile int *)(UINT_PTR)SumVa(VA_INTERFACE_CUR_SCREEN),
        (double)*(volatile float *)(UINT_PTR)SumVa(VA_FPS), *(volatile BYTE *)(UINT_PTR)SumVa(VA_IS_MAIN_WND_ACTIVE),
        cx / 2, cy / 2);
}

static void Peek(DWORD va, int n)
{
    if (n <= 0 || n > 256) n = 16;
    char line[3 * 256 + 32];
    int len = _snprintf(line, sizeof line, "%06lX:", va);
    const BYTE *p = (const BYTE *)(UINT_PTR)SumVa(va);
    if (IsBadReadPtr(p, (UINT_PTR)n)) {
        Out("peek %06lX: unreadable", va);
        return;
    }
    for (int i = 0; i < n; i++)
        len += _snprintf(line + len, sizeof line - (size_t)len, " %02X", p[i]);
    Out("%s", line);
}

// Game-thread bodies of "level" and "goldpile" (run through KeysAgentRun).
#define VA_GAME_CHANGE_LEVEL 0x431a40u  // cdecl (level, script)
#define VA_ITEM_CREATE_BY_NAME 0x44b320u // cdecl (name) -> item
#define VA_ITEM_OBJ_SPAWN 0x46a1b0u      // cdecl (item, pos, matrix, amount, -1, -1) -> item object
static char g_level[32], g_script[32];
static volatile int g_goldAmount;

static void ChangeLevelOnGameThread(void *unused)
{
    (void)unused;
    Log("agent: game_change_level(%s, %s)", g_level, g_script);
    ((void(__cdecl *)(const char *, const char *))(UINT_PTR)SumVa(VA_GAME_CHANGE_LEVEL))(g_level, g_script);
}

// exitto LEVEL SCRIPT [ENTRY] (2026-11-03): leave the level as a level exit or a story cutscene does (the end of
// Nar-Leaving-Masad, ls_masad_leaving_cs_end 0x4fdd30: level_prev_filename = level_filename, the new level and
// script names, level_entry_point, gameseq_set_state(4)), so the game state goes along. "level" instead goes
// through game_change_level, whose state 2 (state_title_enter) resets the party inventory and rebuilds the save
// layout: the journal and the identified items were lost (2026-11-03).
#define VA_LEVEL_FILENAME 0x60a2c8u      // char[32]
#define VA_LEVEL_SCRIPT_NAME 0x60a2e8u   // char[32]
#define VA_LEVEL_PREV_FILENAME 0x60a308u // char[32]
#define VA_LEVEL_ENTRY_POINT 0x60a33cu   // int
#define VA_GAMESEQ_SET_STATE 0x401020u   // cdecl (state)
static volatile int g_exitEntry;

static void ExitToOnGameThread(void *unused)
{
    (void)unused;
    char *cur = (char *)(UINT_PTR)SumVa(VA_LEVEL_FILENAME);
    lstrcpynA((char *)(UINT_PTR)SumVa(VA_LEVEL_PREV_FILENAME), cur, 32);
    lstrcpynA(cur, g_level, 32);
    lstrcpynA((char *)(UINT_PTR)SumVa(VA_LEVEL_SCRIPT_NAME), g_script, 32);
    *(int *)(UINT_PTR)SumVa(VA_LEVEL_ENTRY_POINT) = g_exitEntry;
    ((void(__cdecl *)(int))(UINT_PTR)SumVa(VA_GAMESEQ_SET_STATE))(4);
    Log("agent: exitto %s %s entry %d", g_level, g_script, g_exitEntry);
}

// A loose gold pile 2 units beside the current character, as monster_drop_loot leaves one (game-bugs G40).
static void GoldPileOnGameThread(void *unused)
{
    (void)unused;
    static const float ident[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    BYTE *ch = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
    void *item = ch ? ((void *(__cdecl *)(const char *))(UINT_PTR)SumVa(VA_ITEM_CREATE_BY_NAME))("Gold") : NULL;
    if (!item) {
        Log("agent: goldpile: no character or no Gold item");
        return;
    }
    float pos[3];
    memcpy(pos, ch + 0x10, sizeof pos);
    pos[0] += 2.0f;
    void *obj = ((void *(__cdecl *)(void *, const float *, const float *, int, int, int))(UINT_PTR)SumVa(VA_ITEM_OBJ_SPAWN))(
        item, pos, ident, g_goldAmount, -1, -1);
    Log("agent: goldpile %d at %.2f %.2f %.2f: object %p", g_goldAmount, pos[0], pos[1], pos[2], obj);
}

// "summon SPELL": summon_create(cur_character, a point 2 units beside it, its layer, SPELL, -1), as the summon spells
// call it (spells.cpp 0x47e900), so a summon can be raised without the cast animation. Testing only.
#define VA_SUMMON_CREATE 0x47e900u // cdecl (caster, pos, layer, spell, existing)
static volatile int g_summonSpell;
static void SummonOnGameThread(void *unused)
{
    (void)unused;
    BYTE *ch = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
    if (!ch) {
        Log("agent: summon: no character");
        return;
    }
    float pos[3];
    memcpy(pos, ch + 0x10, sizeof pos);
    pos[0] += 2.0f;
    int layer = *(int *)(ch + 0x68c);
    ((void(__cdecl *)(void *, float *, int, int, int))(UINT_PTR)SumVa(VA_SUMMON_CREATE))(ch, pos, layer, g_summonSpell, -1);
    Log("agent: summon spell %d at %.2f %.2f %.2f layer %d: cur_summon %08lX", g_summonSpell, pos[0], pos[1], pos[2], layer,
        *(DWORD volatile *)(UINT_PTR)SumVa(VA_CUR_SUMMON));
}

// join NAME LEVEL / xpto NAME LEVEL (2026-11-03, for the test saves): party_member_join (0x439ba0, cdecl (name, level,
// leader, place at start, char_infos index -1)) adds a hero, and above level 1 awards xp_for_level(level) through
// character_add_xp with quiet set, as the game does for a joining hero: HP, AP and skills as from play. A hero already in
// the party is returned as he is, so xpto awards the difference to LEVEL the same way (Joseph included).
#define VA_PARTY_MEMBER_JOIN 0x439ba0u
#define VA_CHARACTER_ADD_XP 0x44fab0u // thiscall (xp, quiet, from_net)
#define VA_XP_FOR_LEVEL 0x489a60u // cdecl (level) -> experience needed
static char g_joinName[32];
static volatile int g_joinLevel, g_joinXpOnly;

static void JoinOnGameThread(void *unused)
{
    (void)unused;
    BYTE *c = ((BYTE * (__cdecl *)(const char *, int, int, int, int))(UINT_PTR)SumVa(VA_PARTY_MEMBER_JOIN))(
        g_joinName, g_joinXpOnly ? 1 : g_joinLevel, 0, 1, -1);
    if (!c) {
        Log("agent: join %s: party_member_join returned NULL (unknown name, a load pending, or no Player record free)", g_joinName);
        return;
    }
    if (g_joinXpOnly) {
        int want = ((int(__cdecl *)(int))(UINT_PTR)SumVa(VA_XP_FOR_LEVEL))(g_joinLevel), have = *(int *)(c + 0x604);
        if (want > have)
            ((void(__thiscall *)(void *, int, int, int))(UINT_PTR)SumVa(VA_CHARACTER_ADD_XP))(c, want - have, 1, 0);
    }
    Log("agent: %s %s: level %d, xp %d, HP %.0f, AP %d", g_joinXpOnly ? "xpto" : "join", g_joinName, *(short *)(c + 0x600),
        *(int *)(c + 0x604), (double)*(float *)(c + 0x5f8), *(short *)(c + 0x5fe));
}

// A loot bag with N items 2 units beside the current character, built as monster_drop_loot builds one: item objects
// from item_obj_spawn, then container_obj_create (unlocked) and container_add_item (game-bugs K12).
#define VA_CONTAINER_OBJ_CREATE 0x4897f0u // cdecl (pos, lock) -> container
#define VA_CONTAINER_ADD_ITEM 0x46aa30u   // thiscall (container, item object)
static volatile int g_bagCount;
static char g_bagItem[48] = "Health Tonic";

static void BagOnGameThread(void *unused)
{
    (void)unused;
    static const float ident[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    BYTE *ch = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
    if (!ch) {
        Log("agent: bag: no character");
        return;
    }
    float pos[3];
    memcpy(pos, ch + 0x10, sizeof pos);
    pos[0] += 2.0f;
    void *bag = ((void *(__cdecl *)(const float *, float))(UINT_PTR)SumVa(VA_CONTAINER_OBJ_CREATE))(pos, 0.0f);
    for (int i = 0; bag && i < g_bagCount; i++) {
        void *item = ((void *(__cdecl *)(const char *))(UINT_PTR)SumVa(VA_ITEM_CREATE_BY_NAME))(g_bagItem);
        void *obj = item ? ((void *(__cdecl *)(void *, const float *, const float *, int, int, int))(UINT_PTR)SumVa(
                               VA_ITEM_OBJ_SPAWN))(item, pos, ident, 1, -1, -1)
                         : NULL;
        if (!obj)
            break;
        ((void(__attribute__((thiscall)) *)(void *, void *))(UINT_PTR)SumVa(VA_CONTAINER_ADD_ITEM))(bag, obj);
    }
    Log("agent: bag of %d x %s at %.2f %.2f %.2f: container %p, %d items", g_bagCount, g_bagItem, pos[0], pos[1], pos[2],
        bag, bag ? *(int *)((BYTE *)bag + 0xd8) : 0);
}

// "shop NAME": opens the shop of a level script character as a click on a shopkeeper does (WORLD_CLICK_SHOP:
// shop_set_shopkeeper_from_char 0x406360, then interface_set_screen(7, NULL) 0x412620), on the game thread.
#define VA_SHOP_SET_SHOPKEEPER 0x406360u // cdecl (Character *)
#define VA_INTERFACE_SET_SCREEN 0x412620u // cdecl (id, arg)
#define VA_LEVEL_SHOPKEEPER_FIND 0x4ce950u // cdecl (name) -> shopkeeper *
static char g_shopName[64];
static void ShopOnGameThread(void *unused)
{
    (void)unused;
    BYTE *npc = ((BYTE *(__cdecl *)(const char *))(UINT_PTR)SumVa(VA_SCRIPT_OBJECT_FIND_CHARACTER))(g_shopName);
    if (!npc) {
        Log("agent: shop %s: no such NPC", g_shopName);
        return;
    }
    void *keeper = ((void *(__cdecl *)(const char *))(UINT_PTR)SumVa(VA_LEVEL_SHOPKEEPER_FIND))((const char *)(npc + 0x2f4));
    Log("agent: shop %s: %s", g_shopName, keeper ? "a shopkeeper" : "not a shopkeeper");
    ((void(__cdecl *)(void *))(UINT_PTR)SumVa(VA_SHOP_SET_SHOPKEEPER))(npc);
    ((void(__cdecl *)(int, void *))(UINT_PTR)SumVa(VA_INTERFACE_SET_SCREEN))(7, NULL);
}

// "savenow": writes quicksave.sav as quicksave (0x40e7c0) does once its checks pass: the thumbnail, the
// conversation state, savegame::write. Without savegame_location_allowed, so combat and the area rules do not refuse
// it; the save checks use it (tools/link-savecheck.mjs). The result is logged.
#define VA_SAVELOAD_CAPTURE_THUMBNAIL 0x427100u // cdecl ()
#define VA_CONVERSATION_SAVE_STATE 0x4d8390u    // thiscall (conversation_db)
#define VA_CONVERSATION_DB 0x25461a0u
#define VA_SAVEGAME_WRITE 0x4d1c40u             // thiscall (savegame_state, filename, 0, description) -> int
#define VA_SAVEGAME_STATE 0x2493a38u
static void SaveNowOnGameThread(void *unused)
{
    (void)unused;
    ((void(__cdecl *)(void))(UINT_PTR)SumVa(VA_SAVELOAD_CAPTURE_THUMBNAIL))();
    ((void(__attribute__((thiscall)) *)(void *))(UINT_PTR)SumVa(VA_CONVERSATION_SAVE_STATE))(
        (void *)(UINT_PTR)SumVa(VA_CONVERSATION_DB));
    int r = ((int(__attribute__((thiscall)) *)(void *, const char *, int, const char *))(UINT_PTR)SumVa(
        VA_SAVEGAME_WRITE))((void *)(UINT_PTR)SumVa(VA_SAVEGAME_STATE), "quicksave.sav", 0, "Quick save");
    Log("agent: savenow: %s", r ? "quicksave.sav written" : "write failed");
}

// Knock the current character down for MS milliseconds as a melee hit does (ai_knock_down_local on its ai_state at
// +0x324), logging the AI mode before and after and the result (game-bugs K80: knocked down while casting).
#define VA_AI_KNOCK_DOWN_LOCAL 0x4742b0u // thiscall (ai_state, ms) -> int
static volatile int g_knockMs;

static void KnockOnGameThread(void *unused)
{
    (void)unused;
    BYTE *ch = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
    if (!ch) {
        Log("agent: knock: no character");
        return;
    }
    int before = *(int *)(ch + 0x324);
    int r = ((int(__attribute__((thiscall)) *)(void *, int))(UINT_PTR)SumVa(VA_AI_KNOCK_DOWN_LOCAL))(ch + 0x324, g_knockMs);
    Log("agent: knock %d ms: ai mode %d -> %d (sub %d), result %d", g_knockMs, before, *(int *)(ch + 0x324),
        *(int *)(ch + 0x328), r);
}

// N wall of fire areas (spell 0x1d) in a row 3 units apart, starting START units (default 6) beside the current
// character, as spell_cast_execute places them. The caster is the current character, or the active summon
// (cur_summon) with "summon" (game-bugs C270: how many areas can burn; G270: an area whose caster is gone).
#define VA_SPELL_PLACE_GROUND_EFFECT 0x47e100u // cdecl (spell, caster handle, pos, layer, orient) -> fx handle
static volatile int g_fireWalls, g_fireWallsBySummon;
static volatile float g_fireWallStart = 6.0f;

static void FireWallsOnGameThread(void *unused)
{
    (void)unused;
    static const float ident[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    BYTE *ch = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
    BYTE *caster = g_fireWallsBySummon ? *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_SUMMON) : ch;
    if (!ch || !caster) {
        Log("agent: firewall: no character or no summon");
        return;
    }
    int made = 0;
    for (int i = 0; i < g_fireWalls; i++) {
        float pos[3];
        memcpy(pos, ch + 0x10, sizeof pos);
        pos[0] += g_fireWallStart + 3.0f * (float)i;
        int h = ((int(__cdecl *)(int, int, const float *, int, const float *))(UINT_PTR)SumVa(VA_SPELL_PLACE_GROUND_EFFECT))(
            0x1d, *(int *)(caster + 0x80), pos, *(int *)(ch + 0x68c), ident);
        if (h != -1)
            made++;
    }
    Log("agent: firewall: %d of %d areas placed (caster %s)", made, g_fireWalls, g_fireWallsBySummon ? "summon" : "leader");
}

// "callstr ADDR STRING [ARG [ARG2]]": cdecl fn(string, arg, arg2) (a ~ in STRING stands for a space) on the game thread, eax logged. ARG with a '.' is passed as a
// float's bits (e.g. a fraction), otherwise as an int. Testing only: the caller must know the function takes that.
static DWORD g_callAddr;
static char g_callStr[64];
static DWORD g_callArg, g_callArg2;

static void CallStrOnGameThread(void *unused)
{
    (void)unused;
    DWORD r = ((DWORD(__cdecl *)(const char *, DWORD, DWORD))(UINT_PTR)SumVa(g_callAddr))(g_callStr, g_callArg, g_callArg2);
    Log("agent: callstr %06lX(\"%s\", 0x%lX, 0x%lX) = 0x%lX", g_callAddr, g_callStr, g_callArg, g_callArg2, r);
}

// "spellhit TARGET SPELL POWER": living_entity_get_hit_by_spell (0x45dea0, thiscall) on the character at TARGET
// (hex address) with the current character as the source and source type 6, as combat_trigger_special_ability and
// the self-cast abilities call it. Testing only (game-bugs G100: spell 0x3e "leech").
#define VA_GET_HIT_BY_SPELL 0x45dea0u
#define VA_PLAYER_FREE_LIST 0x5fc1c8u   // Player sentinel; its next (+0) points at itself when no record is free
static DWORD g_hitTarget;
static int g_hitSpell;
static float g_hitPower;

static void SpellHitOnGameThread(void *unused)
{
    (void)unused;
    typedef void(__attribute__((thiscall)) * HitBySpell)(void *, int, int, void *, void *, int, void *, float, int);
    BYTE *ch = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
    if (!ch || !g_hitTarget) {
        Log("agent: spellhit: no character or target");
        return;
    }
    ((HitBySpell)(UINT_PTR)SumVa(VA_GET_HIT_BY_SPELL))((void *)(UINT_PTR)g_hitTarget, g_hitSpell, *(int *)(ch + 0x80), NULL,
                                                        NULL, 6, NULL, g_hitPower, 0);
    Log("agent: spellhit %06lX spell 0x%x power %.1f from %p", g_hitTarget, g_hitSpell, (double)g_hitPower, (void *)ch);
}

// "npckill NAME": the character takes 1,000,000 damage through character_add_hp (thiscall (amount, source, 0, 0)).
static void NpcKillOnGameThread(void *arg)
{
    char *name = (char *)arg; // a copy per request: several kills can be queued before the game thread runs
    typedef BYTE *(__cdecl * FindChar)(const char *);
    typedef char(__attribute__((thiscall)) * AddHp)(void *, float, int, int, int);
    BYTE *npc = ((FindChar)(UINT_PTR)SumVa(VA_SCRIPT_OBJECT_FIND_CHARACTER))(name);
    if (!npc) {
        Log("agent: npckill %s: no such character", name);
    } else {
        // character_add_hp ignores type 4 (non-hostile NPCs; wave enemies stay type 4 until "turn hostile"), so such a
        // character becomes type 5 (monster) first and then dies through the normal death handling.
        if (*(int *)(npc + 0xc) == 4)
            *(int *)(npc + 0xc) = 5;
        ((AddHp)(UINT_PTR)SumVa(0x450850u))(npc, -1000000.0f, -1, 0, 0);
        // A character killed in the middle of a move stays "dying" (+0x320 bit 0x8, not dead 0x80) until the move ends;
        // a walking wave soldier never ended it (Liangshan's first wave, 2026-11-04). No current move (+0x6cc = -1)
        // lets the death finish, as for the party members in the boss runs.
        if ((*(DWORD *)(npc + 0x320) & 0x88) == 0x08)
            *(int *)(npc + 0x6cc) = -1;
        Log("agent: npckill %s: %p, hp %.0f, flags %08lX", name, (void *)npc, (double)*(float *)(npc + 0x5f8),
            *(DWORD *)(npc + 0x320));
    }
    HeapFree(GetProcessHeap(), 0, name);
}

// "useitem NAME[@TARGET]": the current character uses a party inventory item as the inventory screen's "Use" does,
// through character_use_inventory_item (0x46bc00, cdecl (Character *, entry, target handle, point, arg) -> bool;
// inventory_use_selected_item passes the user's own handle, NULL and -1). TARGET is a level script character's name
// for an item whose spell needs a target (default: the user). The entry's count and the item's charges before and
// after are logged. Testing only (2026-11-08: the items the save editors add, which had 0 charges).
static void UseItemOnGameThread(void *arg)
{
    char *name = (char *)arg, *target = strchr(name, '@');
    typedef BYTE *(__cdecl * FindChar)(const char *);
    typedef char(__cdecl * UseItem)(void *, void *, int, void *, int);
    int atPoint = 0; // "NAME@@[TARGET]": a spell aimed at a place (Blizzard, ...), cast at the target's feet and layer
                     // with handle -1, as world_use_item_at_point (0x418540) does; a NULL point there crashes the game
    if (target) {
        *target++ = 0;
        if (*target == '@') {
            atPoint = 1;
            target++;
        }
        if (!*target)
            target = NULL;
    }
    BYTE *ch = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
    int n = *(int *)(UINT_PTR)SumVa(0x97186cu);              // party_inventory_count
    BYTE *inv = (BYTE *)(UINT_PTR)SumVa(0x970820u);          // party_inventory: {int count; item_info *item}
    BYTE *entry = NULL;
    for (int i = 0; i < n && !entry; i++) {
        BYTE *item = *(BYTE **)(inv + i * 8 + 4);
        const char *nm = item ? **(const char ***)(item + 4) : NULL; // item_info +4 ItemType *, ItemType +0 name
        if (nm && !_stricmp(nm, name))
            entry = inv + i * 8;
    }
    BYTE *tc = target ? ((FindChar)(UINT_PTR)SumVa(VA_SCRIPT_OBJECT_FIND_CHARACTER))(target) : ch;
    if (!ch || !entry || !tc) {
        Log("agent: useitem %s: %s", name, !ch ? "no character" : !entry ? "not in the party inventory" : "no such target");
    } else {
        BYTE *item = *(BYTE **)(entry + 4);
        int count = *(int *)entry, charges = *(int *)(item + 8);
        float point[3];
        memcpy(point, tc + 0x10, sizeof point);
        char ok = atPoint ? ((UseItem)(UINT_PTR)SumVa(0x46bc00u))(ch, entry, -1, point, *(int *)(tc + 0x68c)) // layer
                          : ((UseItem)(UINT_PTR)SumVa(0x46bc00u))(ch, entry, *(int *)(tc + 0x80), NULL, -1);
        // the entry may have been removed (the last one used): look the item up again
        int after = -1, chAfter = -1;
        n = *(int *)(UINT_PTR)SumVa(0x97186cu);
        for (int i = 0; i < n; i++)
            if (*(BYTE **)(inv + i * 8 + 4) == item) {
                after = *(int *)(inv + i * 8);
                chAfter = *(int *)(item + 8);
            }
        Log("agent: useitem %s: used %d, count %d -> %d, charges %d -> %d", name, ok, count, after, charges, chAfter);
    }
    HeapFree(GetProcessHeap(), 0, name);
}

// "npchp FRACTION NAME": damage a level script character down to FRACTION of its base max HP (+0x5f4) through
// character_add_hp, as npckill does, so a boss passes its HP stages one at a time (its stage checks and transition
// cutscenes run as after real damage). The request block is a float followed by the name. Testing only.
static void NpcHpOnGameThread(void *arg)
{
    float frac = *(float *)arg;
    char *name = (char *)arg + sizeof(float);
    typedef BYTE *(__cdecl * FindChar)(const char *);
    typedef char(__attribute__((thiscall)) * AddHp)(void *, float, int, int, int);
    BYTE *npc = ((FindChar)(UINT_PTR)SumVa(VA_SCRIPT_OBJECT_FIND_CHARACTER))(name);
    if (!npc) {
        Log("agent: npchp %s: no such character", name);
    } else {
        float hp = *(float *)(npc + 0x5f8), want = frac * *(float *)(npc + 0x5f4);
        if (want < hp)
            ((AddHp)(UINT_PTR)SumVa(0x450850u))(npc, want - hp, -1, 0, 0);
        Log("agent: npchp %s %.2f: %p, hp %.0f -> %.0f of %.0f", name, (double)frac, (void *)npc, (double)hp,
            (double)*(float *)(npc + 0x5f8), (double)*(float *)(npc + 0x5f4));
    }
    HeapFree(GetProcessHeap(), 0, arg);
}

// "calli ADDR A [B]": cdecl fn(A, B) with two integer arguments; eax logged.
static DWORD g_callIntA, g_callIntB;
static void CallIntOnGameThread(void *unused)
{
    (void)unused;
    DWORD r = ((DWORD(__cdecl *)(DWORD, DWORD))(UINT_PTR)SumVa(g_callAddr))(g_callIntA, g_callIntB);
    Log("agent: calli %06lX(0x%lX, 0x%lX) = 0x%lX", g_callAddr, g_callIntA, g_callIntB, r);
}

// "callt ADDR THIS [ARG ...]": a thiscall game function with up to six int arguments. The callee pops its own
// arguments, however many it takes, so esp is saved around the call.
static DWORD g_callThis;
static int g_callArgc;
__attribute__((used)) static DWORD g_callArgs[6]; // named in CallThisRaw's asm
static DWORD CallThisRaw(DWORD fn, DWORD self)
{
    // The arguments are read from the globals (absolute addresses), not from the stack frame, because esp moves
    // with each push.
    DWORD r, ecxOut;
    __asm__ volatile("movl %%esp, %%esi\n\t"
                     "movl %3, %%edx\n"
                     "1:\n\t"
                     "testl %%edx, %%edx\n\t"
                     "jz 2f\n\t"
                     "decl %%edx\n\t"
                     "pushl _g_callArgs(,%%edx,4)\n\t"
                     "jmp 1b\n"
                     "2:\n\t"
                     "call *%%eax\n\t"
                     "movl %%esi, %%esp\n\t"
                     : "=a"(r), "=c"(ecxOut)
                     : "0"(fn), "m"(g_callArgc), "1"(self)
                     : "esi", "edx", "memory", "cc");
    (void)ecxOut;
    return r;
}
static void CallThisOnGameThread(void *unused)
{
    (void)unused;
    DWORD r = CallThisRaw(SumVa(g_callAddr), g_callThis);
    Log("agent: callt %06lX(this 0x%lX, %d args: 0x%lX 0x%lX 0x%lX ...) = 0x%lX", g_callAddr, g_callThis, g_callArgc,
        g_callArgs[0], g_callArgs[1], g_callArgs[2], r);
}

// "call0 ADDR [N]": a void cdecl game function with no arguments, N times; after each call level_filename and
// level_script_name are logged (the encounter pickers write them; game-bugs G6).
static int g_call0Count;
static void Call0OnGameThread(void *unused)
{
    (void)unused;
    char *names = (char *)(UINT_PTR)SumVa(0x60a2c8u); // level_filename, level_script_name, level_prev_filename (3 x 32)
    char saved[0x60];
    memcpy(saved, names, sizeof saved);
    for (int i = 0; i < g_call0Count; i++) {
        ((void(__cdecl *)(void))(UINT_PTR)SumVa(g_callAddr))();
        Log("agent: call0 %06lX #%d: level %s script %s", g_callAddr, i + 1, names, names + 0x20);
    }
    memcpy(names, saved, sizeof saved); // the pickers change the current level's names; put them back
}

// navgoto / npcgoto move the party as the game's own teleport does (party_teleport_to): the current character and
// its layer, the others lined up 0.75 apart, then camera_reset_to_player. Writing only the current character's
// position (before 2026-11-06) kept its old layer and left the others and the camera behind; test saves made after
// it had a leader in a pit or followers 100+ units away.
#define VA_NAVPOINT_FIND 0x4dc570u     // cdecl (name) -> navpoint * (0x30 bytes: position +0x14, layer +0x20)
#define VA_PARTY_TELEPORT_TO 0x439750u // cdecl (vector3 *pos, int layer)
static float g_tpPos[3];
static int g_tpLayer;
static void TeleportOnGameThread(void *unused)
{
    (void)unused;
    ((void(__cdecl *)(float *, int))(UINT_PTR)SumVa(VA_PARTY_TELEPORT_TO))(g_tpPos, g_tpLayer);
}

// Write one byte of game data (not code: only already-writable memory). For experiments such as
// setting a debug flag bit; prints the old and new value.
static void Poke(DWORD va, unsigned value)
{
    BYTE *p = (BYTE *)(UINT_PTR)SumVa(va);
    if (IsBadWritePtr(p, 1)) {
        Out("poke %06lX: not writable", va);
        return;
    }
    BYTE old = *p;
    *p = (BYTE)value;
    Out("poke %06lX: %02X -> %02X", va, old, *p);
}

static void RunLine(char *line)
{
    char *argv[12];
    int argc = 0;
    for (char *t = strtok(line, " \t\r\n"); t && argc < 12; t = strtok(NULL, " \t\r\n"))
        argv[argc++] = t;
    if (!argc || argv[0][0] == '#')
        return;
    const char *c = argv[0];
    if (!_stricmp(c, "key") || !_stricmp(c, "keydown") || !_stricmp(c, "keyup")) {
        int code = argc > 1 ? KeyParse(argv[1]) : -1;
        if (code < 0 || code >= 256) {
            Out("%s: unknown key", c);
            return;
        }
        if (!_stricmp(c, "keyup")) {
            KeysInjectKey(code, 0);
        } else {
            KeysInjectKey(code, 1);
            if (!_stricmp(c, "key")) {
                Sleep(argc > 2 ? (DWORD)atoi(argv[2]) : 80);
                KeysInjectKey(code, 0);
            }
        }
        Out("%s %s: ok", c, argv[1]);
    } else if (!_stricmp(c, "postkey") && argc > 1) {
        // A real WM_KEYDOWN/WM_KEYUP through the game's window procedure (unlike "key", which calls
        // key_mark directly), so window-message handlers such as [ConsoleKeys] see it.
        int code = KeyParse(argv[1]);
        HWND wnd = *(volatile HWND *)(UINT_PTR)SumVa(0x2cbff10);
        if (code < 0 || code >= 256 || !wnd) {
            Out("postkey: unknown key or no window");
            return;
        }
        UINT vk = MapVirtualKeyA((code & 0x7f) | (code & 0x80 ? 0xe000 : 0), 3 /* MAPVK_VSC_TO_VK_EX */);
        LPARAM lp = 1 | ((LPARAM)(code & 0x7f) << 16) | (code & 0x80 ? 0x01000000 : 0);
        PostMessageA(wnd, WM_KEYDOWN, vk, lp);
        Sleep(argc > 2 ? (DWORD)atoi(argv[2]) : 80);
        PostMessageA(wnd, WM_KEYUP, vk, lp | 0xC0000000);
        Out("postkey %s: ok (vk 0x%02X)", argv[1], vk);
    } else if (!_stricmp(c, "speedkey") && argc > 1) {
        KeysAgentSpeed(atoi(argv[1]));
        Out("speedkey %s: ok", argv[1]);
    } else if (!_stricmp(c, "postmsg") && argc >= 4) {
        // postmsg MSG WPARAM LPARAM (hex), e.g. "postmsg 20b 10000 0" = WM_XBUTTONDOWN with XBUTTON1
        HWND wnd = *(volatile HWND *)(UINT_PTR)SumVa(0x2cbff10);
        UINT msg = (UINT)strtoul(argv[1], NULL, 16);
        PostMessageA(wnd, msg, (WPARAM)strtoul(argv[2], NULL, 16), (LPARAM)strtoul(argv[3], NULL, 16));
        Out("postmsg 0x%X: ok", msg);
    } else if (!_stricmp(c, "move") && argc >= 3) {
        Move(atoi(argv[1]), atoi(argv[2]));
        Out("move: ok");
    } else if (!_stricmp(c, "click")) {
        int a = 1, b = 0;
        if (argc > 1 && (argv[1][0] < '0' || argv[1][0] > '9')) {
            b = Button(argv[1]);
            a = 2;
        }
        if (argc >= a + 2) {
            Move(atoi(argv[a]), atoi(argv[a + 1]));
            Sleep(60); // let the game see the new position for a frame or two
            a += 2;
        }
        KeysInjectMouse(b, 1);
        Sleep(argc > a ? (DWORD)atoi(argv[a]) : 80);
        KeysInjectMouse(b, 0);
        Out("click: ok");
    } else if ((!_stricmp(c, "down") || !_stricmp(c, "up")) && argc >= 2) {
        KeysInjectMouse(Button(argv[1]), !_stricmp(c, "down"));
        Out("%s: ok", c);
    } else if (!_stricmp(c, "drag") && argc >= 5) {
        Move(atoi(argv[1]), atoi(argv[2]));
        Sleep(60);
        KeysInjectMouse(0, 1);
        Sleep(80);
        for (int i = 1; i <= 10; i++) {
            Move(atoi(argv[1]) + (atoi(argv[3]) - atoi(argv[1])) * i / 10, atoi(argv[2]) + (atoi(argv[4]) - atoi(argv[2])) * i / 10);
            Sleep(30);
        }
        Sleep(60);
        KeysInjectMouse(0, 0);
        Out("drag: ok");
    } else if (!_stricmp(c, "pad") && argc >= 6) {
        // pad LX LY RX RY BUTTONS [MS]: e.g. "pad 32767 0 0 0 0 500" (cursor right for 0.5 s), "pad 0 0 0 0 1000"
        // (A: a left click). The game thread polls it once per frame (the SwapBuffers hook).
        DWORD ms = argc > 6 ? (DWORD)atoi(argv[6]) : 100;
        ControllerSetSynthetic(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
                               (unsigned)strtoul(argv[5], NULL, 16), ms);
        Sleep(ms);
        int i;
        for (i = 0; i < 100 && !ControllerSyntheticDone(); i++)
            Sleep(10);
        int cx, cy;
        DisplayGetAgentCursor(&cx, &cy);
        Out(ControllerSyntheticDone() ? "pad: ok (cursor %d,%d)" : "pad: not released after 1 s (is the game drawing?) (cursor %d,%d)",
            cx / 2, cy / 2);
    } else if (!_stricmp(c, "wait") && argc >= 2) {
        Sleep((DWORD)atoi(argv[1]));
    } else if (!_stricmp(c, "capture") && argc >= 2) {
        Capture(argv[1]);
    } else if (!_stricmp(c, "hybridcov") && argc >= 2) {
        // hybridcov NAME: the call counts of the hybrid game's replaced functions ([Hybrid] Coverage=1) to NAME.txt
        char path[MAX_PATH], file[MAX_PATH];
        _snprintf(file, sizeof file, "%s.txt", argv[1]);
        file[sizeof file - 1] = 0;
        Path(path, file);
        int ran = HybridWriteCoverage(path);
        if (ran < 0)
            Out("hybridcov %s: no coverage (needs [Hybrid] Dll and Coverage=1)", file);
        else
            Out("hybridcov %s: %d functions ran", file, ran);
    } else if (!_stricmp(c, "info")) {
        Info();
    } else if (!_stricmp(c, "dumpdata") && argc >= 3) {
        // dumpdata OUT TABLE: the current bytes of every data symbol of an address table (tools/link-build.mjs's
        // Sum-rebuilt.addr: "orig size rebuilt" lines) to OUT, as records (Sum.exe address, size, bytes), read in this
        // process at the symbol's own address (through SumVa, so in Sum.exe or the rebuilt exe alike). For comparing
        // the hybrid's and the rebuilt exe's memory (tools/link-memcheck.mjs).
        HANDLE in = CreateFileA(argv[2], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        HANDLE out = CreateFileA(argv[1], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        int rows = 0;
        if (in != INVALID_HANDLE_VALUE && out != INVALID_HANDLE_VALUE) {
            DWORD size = GetFileSize(in, NULL), got = 0, w;
            char *text = (char *)HeapAlloc(GetProcessHeap(), 0, size + 1);
            ReadFile(in, text, size, &got, NULL);
            text[got] = 0;
            for (char *p = text; *p;) {
                char *end;
                DWORD orig = strtoul(p, &end, 16), n = strtoul(end, &end, 16);
                while (*end && *end != '\n')
                    end++;
                p = *end ? end + 1 : end;
                if (orig < 0x57db40 || n == 0 || n > 0x10000)
                    continue; // code, or too large to be worth comparing
                const BYTE *src = (const BYTE *)(UINT_PTR)SumVa(orig);
                if (!src || IsBadReadPtr(src, n))
                    continue;
                WriteFile(out, &orig, 4, &w, NULL);
                WriteFile(out, &n, 4, &w, NULL);
                WriteFile(out, src, n, &w, NULL);
                rows++;
            }
            HeapFree(GetProcessHeap(), 0, text);
        }
        if (in != INVALID_HANDLE_VALUE)
            CloseHandle(in);
        if (out != INVALID_HANDLE_VALUE)
            CloseHandle(out);
        Out("dumpdata: %d symbols", rows);
    } else if (!_stricmp(c, "peek") && argc >= 2) {
        Peek((DWORD)strtoul(argv[1], NULL, 16), argc > 2 ? atoi(argv[2]) : 16);
    } else if (!_stricmp(c, "count") && argc >= 5) {
        // count ADDR STRIDE N MASK: how many of N records (STRIDE bytes apart, hex) have (first byte & MASK) != 0,
        // e.g. count 2f077b8 74 128 1 for the used sound emitters.
        const BYTE *p = (const BYTE *)(UINT_PTR)SumVa((DWORD)strtoul(argv[1], NULL, 16));
        DWORD stride = (DWORD)strtoul(argv[2], NULL, 16);
        int n = atoi(argv[3]), used = 0;
        BYTE mask = (BYTE)strtoul(argv[4], NULL, 16);
        for (int i = 0; i < n; i++)
            if (p[i * stride] & mask)
                used++;
        Out("count %s: %d of %d", argv[1], used, n);
    } else if (!_stricmp(c, "k310")) {
        // k310: the TraceGlLights counters (faces on the unclipped lightmap path, how many had a coloured light).
        long faces, coloured;
        float rgb[3];
        TraceGlLightStats(&faces, &coloured, rgb);
        long calls, lit, maxLights;
        TraceGlDrawStats(&calls, &lit, &maxLights);
        Out("k310: %ld faces, %ld with a coloured light; last coloured rgb %.3f %.3f %.3f; draw-list calls %ld, %ld with lights (max %ld)", faces, coloured, rgb[0], rgb[1], rgb[2], calls, lit, maxLights);
    } else if (!_stricmp(c, "pokecode") && argc >= 3) {
        // pokecode ADDR BYTE: one byte of Sum.exe code (made writable around the write). Testing only, e.g. to
        // switch a fix on and off in a running game for a before / after capture.
        DWORD va = (DWORD)strtoul(argv[1], NULL, 16);
        BYTE *p = (BYTE *)(UINT_PTR)SumVa(va), old = *p;
        DWORD prot;
        if (VirtualProtect(p, 1, PAGE_EXECUTE_READWRITE, &prot)) {
            *p = (BYTE)strtoul(argv[2], NULL, 16);
            VirtualProtect(p, 1, prot, &prot);
            FlushInstructionCache(GetCurrentProcess(), p, 1);
            Out("pokecode %06lX: %02X -> %02X", va, old, *p);
        } else {
            Out("pokecode %06lX: VirtualProtect failed", va);
        }
    } else if (!_stricmp(c, "poke") && argc >= 3) {
        Poke((DWORD)strtoul(argv[1], NULL, 16), (unsigned)strtoul(argv[2], NULL, 16));
    } else if (!_stricmp(c, "flag") && argc >= 2) {
        // flag NAME [0|1]: read or set a story flag of the current level, as the level scripts do
        // (savegame_get_flag / savegame_set_flag on savegame_state). Testing only; runs on the agent thread.
        typedef int(__attribute__((thiscall)) * GetFlag)(void *, int, const char *);
        typedef void(__attribute__((thiscall)) * SetFlag)(void *, int, const char *, int);
        void *state = (void *)(UINT_PTR)SumVa(VA_SAVEGAME_STATE);
        int level = *(volatile int *)(UINT_PTR)SumVa(VA_CUR_LEVEL_INDEX);
        if (argc >= 3)
            ((SetFlag)(UINT_PTR)SumVa(VA_SAVEGAME_SET_FLAG))(state, level, argv[1], atoi(argv[2]) != 0);
        Out("flag %s (level %d) = %d", argv[1], level, ((GetFlag)(UINT_PTR)SumVa(VA_SAVEGAME_GET_FLAG))(state, level, argv[1]));
    } else if (!_stricmp(c, "flagl") && argc >= 3) {
        // flagl LEVEL NAME [0|1]: a story flag of another level (level_script_table index, as savedit's LEVEL).
        typedef int(__attribute__((thiscall)) * GetFlag)(void *, int, const char *);
        typedef void(__attribute__((thiscall)) * SetFlag)(void *, int, const char *, int);
        void *state = (void *)(UINT_PTR)SumVa(VA_SAVEGAME_STATE);
        int level = atoi(argv[1]);
        if (argc >= 4)
            ((SetFlag)(UINT_PTR)SumVa(VA_SAVEGAME_SET_FLAG))(state, level, argv[2], atoi(argv[3]) != 0);
        Out("flag %s (level %d) = %d", argv[2], level, ((GetFlag)(UINT_PTR)SumVa(VA_SAVEGAME_GET_FLAG))(state, level, argv[2]));
    } else if (!_stricmp(c, "calli") && argc >= 3) {
        // calli ADDR A [B]: cdecl fn(A, B) with integer arguments (hex with 0x), on the game thread; eax logged.
        g_callAddr = (DWORD)strtoul(argv[1], NULL, 16);
        g_callIntA = (DWORD)strtoul(argv[2], NULL, 0);
        g_callIntB = argc >= 4 ? (DWORD)strtoul(argv[3], NULL, 0) : 0;
        Out("calli %06lX: %s", g_callAddr, AgentRunSync(CallIntOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "callt") && argc >= 3) {
        // callt ADDR THIS [ARG ...]: thiscall fn(THIS, ARG...) with 0-6 integer arguments, on the game thread; eax logged.
        g_callAddr = (DWORD)strtoul(argv[1], NULL, 16);
        g_callThis = (DWORD)strtoul(argv[2], NULL, 0);
        g_callArgc = argc - 3 > 6 ? 6 : argc - 3;
        for (int i = 0; i < g_callArgc; i++)
            g_callArgs[i] = (DWORD)strtoul(argv[3 + i], NULL, 0);
        Out("callt %06lX: %s", g_callAddr, AgentRunSync(CallThisOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "savenow")) {
        Out("savenow: %s", AgentRunSync(SaveNowOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "call0") && argc >= 2) {
        // call0 ADDR [N]: call the cdecl void game function at ADDR with no arguments, N times, on the game thread.
        g_callAddr = (DWORD)strtoul(argv[1], NULL, 16);
        g_call0Count = argc >= 3 ? atoi(argv[2]) : 1;
        Out("call0 %06lX x%d: %s", g_callAddr, g_call0Count, AgentRunSync(Call0OnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "navgoto") && argc >= 2) {
        // navgoto NAVPOINT: put the party on a level navpoint (e.g. $end2trig), on its layer. Testing only.
        BYTE *ch = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
        BYTE *nav = ((BYTE * (__cdecl *)(const char *))(UINT_PTR)SumVa(VA_NAVPOINT_FIND))(argv[1]);
        if (!ch || !nav) {
            Out("navgoto %s: no character or no such navpoint", argv[1]);
        } else {
            memcpy(g_tpPos, nav + 0x14, sizeof g_tpPos);
            g_tpLayer = *(int *)(nav + 0x20);
            AgentRunSync(TeleportOnGameThread, NULL);
            Out("navgoto %s: %.2f %.2f %.2f layer %d", argv[1], g_tpPos[0], g_tpPos[1], g_tpPos[2], g_tpLayer);
        }
    } else if (!_stricmp(c, "posgoto") && argc >= 5) {
        // posgoto X Y Z LAYER: the party to a point and layer (party_teleport_to), e.g. a follower's position read
        // from a save. Testing only.
        for (int i = 0; i < 3; i++)
            g_tpPos[i] = (float)atof(argv[1 + i]);
        g_tpLayer = atoi(argv[4]);
        Out("posgoto %.2f %.2f %.2f layer %d: %s", g_tpPos[0], g_tpPos[1], g_tpPos[2], g_tpLayer,
            AgentRunSync(TeleportOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "shop") && argc >= 2) {
        g_shopName[0] = 0;
        for (int i = 1; i < argc; i++) {
            if (i > 1)
                strncat(g_shopName, " ", sizeof g_shopName - strlen(g_shopName) - 1);
            strncat(g_shopName, argv[i], sizeof g_shopName - strlen(g_shopName) - 1);
        }
        Out("shop %s: %s", g_shopName, AgentRunSync(ShopOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "npcgoto") && argc >= 2) {
        // npcgoto NAME: put the party 4 units from a level script NPC (e.g. Luminar), as
        // script_object_find_character finds it, on the NPC's layer. Testing only.
        // The words after npcgoto are joined with spaces, so "npcgoto Canis the Mercenary" works.
        typedef BYTE *(__cdecl * FindChar)(const char *);
        char name[64] = "";
        for (int i = 1; i < argc; i++) {
            if (i > 1)
                strncat(name, " ", sizeof name - strlen(name) - 1);
            strncat(name, argv[i], sizeof name - strlen(name) - 1);
        }
        BYTE *ch = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
        BYTE *npc = ((FindChar)(UINT_PTR)SumVa(VA_SCRIPT_OBJECT_FIND_CHARACTER))(name);
        if (!ch || !npc) {
            Out("npcgoto %s: no character or no such NPC", name);
        } else {
            float pos[3];
            memcpy(pos, npc + 0x10, sizeof pos);
            Out("npcgoto %s: NPC at %.2f %.2f %.2f", name, pos[0], pos[1], pos[2]);
            g_tpPos[0] = pos[0] + 4.0f;
            g_tpPos[1] = pos[1];
            g_tpPos[2] = pos[2];
            g_tpLayer = *(int *)(npc + 0x68c);
            AgentRunSync(TeleportOnGameThread, NULL);
        }
    } else if (!_stricmp(c, "exitto") && argc >= 3) {
        // exitto LEVEL SCRIPT [ENTRY]: change level as a level exit does, keeping the game state (ExitToOnGameThread).
        lstrcpynA(g_level, argv[1], sizeof g_level);
        lstrcpynA(g_script, argv[2], sizeof g_script);
        g_exitEntry = argc > 3 ? atoi(argv[3]) : 1;
        Out("exitto %s %s %d: %s", argv[1], argv[2], g_exitEntry, AgentRunSync(ExitToOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "level") && argc >= 3) {
        // level NAME SCRIPT: change level as a level exit does (game_change_level), e.g.
        // "level IonaEXT02 IonaEXT02_v2". Loads the level's own script objects, which the
        // play_and_load_cutscene warp does not. Testing only.
        // Runs on the game thread (KeysAgentRun): from this thread the load crashed mid-frame.
        lstrcpynA(g_level, argv[1], sizeof g_level);
        lstrcpynA(g_script, argv[2], sizeof g_script);
        Out("level %s %s: %s", argv[1], argv[2], AgentRunSync(ChangeLevelOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "callstr") && argc >= 3) {
        // callstr ADDR STRING [ARG]: e.g. "callstr 4f95a0 puzzle-flippers31" (ls_khosanilab_use) or
        // "callstr 4773d0 puzzle-flippers31 0.01" (door_is_open_by_name). Result in the log.
        g_callAddr = (DWORD)strtoul(argv[1], NULL, 16);
        lstrcpynA(g_callStr, argv[2], sizeof g_callStr);
        for (char *p = g_callStr; *p; p++)
            if (*p == '~')
                *p = ' ';
        g_callArg = 0;
        g_callArg2 = argc >= 5 ? (DWORD)strtol(argv[4], NULL, 0) : 0;
        if (argc >= 4) {
            if (strchr(argv[3], '.')) {
                float f = (float)atof(argv[3]);
                memcpy(&g_callArg, &f, 4);
            } else {
                g_callArg = (DWORD)strtol(argv[3], NULL, 0);
            }
        }
        Out("callstr %06lX %s: %s", g_callAddr, g_callStr, AgentRunSync(CallStrOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "spellhit") && argc >= 4) {
        // spellhit TARGET SPELL POWER: e.g. "spellhit 90a4e8 3e 100" (leech on Joseph from the leader).
        g_hitTarget = (DWORD)strtoul(argv[1], NULL, 16);
        g_hitSpell = (int)strtol(argv[2], NULL, 16);
        g_hitPower = (float)atof(argv[3]);
        Out("spellhit: %s", AgentRunSync(SpellHitOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "firewall") && argc >= 2) {
        // firewall N [START] [summon]: N wall of fire areas beside the current character (game thread; result in
        // the log), the first START units away, cast by the leader or by the active summon.
        g_fireWalls = atoi(argv[1]);
        g_fireWallStart = argc >= 3 ? (float)atof(argv[2]) : 6.0f;
        g_fireWallsBySummon = argc >= 4 && !_stricmp(argv[3], "summon");
        Out("firewall %d: %s", g_fireWalls, AgentRunSync(FireWallsOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "goldpile") && argc >= 2) {
        // goldpile N: a loose gold pile with N gold beside the current character (game thread; result in the log).
        g_goldAmount = atoi(argv[1]);
        Out("goldpile %d: %s", g_goldAmount, AgentRunSync(GoldPileOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "summon") && argc >= 2) {
        // summon SPELL: summon_create for the current character with spell index SPELL (decimal; 45 = summon mist
        // elemental in spells.tbl order), game thread, result in the log.
        // A summon takes a Player record (5: the party's four and one summon): with none free, summon_create stops
        // the game with "No more player slots" (player.cpp line 421), as a second summon of the full party did on
        // 2026-11-03. Refused here instead.
        g_summonSpell = atoi(argv[1]);
        UINT_PTR freeList = (UINT_PTR)SumVa(VA_PLAYER_FREE_LIST);
        if (*(volatile UINT_PTR *)freeList == freeList)
            Out("summon %d: refused, no free Player record (5: the party and one summon)", g_summonSpell);
        else
            Out("summon %d: %s", g_summonSpell, AgentRunSync(SummonOnGameThread, NULL) ? "requested" : "no window");
    } else if ((!_stricmp(c, "join") || !_stricmp(c, "xpto")) && argc >= 3) {
        // join NAME LEVEL: the hero joins the party at LEVEL; xpto NAME LEVEL: a party member gets the experience up to
        // LEVEL (game thread; the result is in the log).
        lstrcpynA(g_joinName, argv[1], sizeof g_joinName);
        g_joinLevel = atoi(argv[2]);
        g_joinXpOnly = !_stricmp(c, "xpto");
        Out("%s %s %d: %s", c, g_joinName, g_joinLevel, AgentRunSync(JoinOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "knock") && argc >= 2) {
        // knock MS: knock the current character down for MS ms (game thread; mode and result in the log).
        g_knockMs = atoi(argv[1]);
        Out("knock %d: %s", g_knockMs, AgentRunSync(KnockOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "bag") && argc >= 2) {
        // bag N [ITEM]: an unlocked loot bag with N items (default Health Tonic, ~ for spaces) beside the current
        // character (game thread; result in the log).
        g_bagCount = atoi(argv[1]);
        if (argc >= 3) {
            lstrcpynA(g_bagItem, argv[2], sizeof g_bagItem);
            for (char *p = g_bagItem; *p; p++)
                if (*p == '~')
                    *p = ' ';
        }
        Out("bag %d: %s", g_bagCount, AgentRunSync(BagOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "nudge") && argc >= 3) {
        // nudge DX DZ: move the current character by DX, DZ world units (after a navgoto, to stand at a distance).
        BYTE *ch = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
        if (ch) {
            *(float *)(ch + 0x10) += (float)atof(argv[1]);
            *(float *)(ch + 0x18) += (float)atof(argv[2]);
            Out("nudge: %.2f %.2f %.2f", *(float *)(ch + 0x10), *(float *)(ch + 0x14), *(float *)(ch + 0x18));
        }
    } else if (!_stricmp(c, "npckill") && argc >= 2) {
        // npckill NAME: character_add_hp(-1000000) on a level script character (name may contain spaces), on the
        // game thread, so it dies as from damage (a type 4 NPC is made type 5 first). Testing only (G132).
        char *name = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 64); // freed on the game thread
        for (int i = 1; i < argc; i++) {
            if (i > 1)
                strncat(name, " ", 63 - strlen(name));
            strncat(name, argv[i], 63 - strlen(name));
        }
        Out("npckill %s: %s", name, AgentRunSync(NpcKillOnGameThread, name) ? "requested" : "no window");
    } else if (!_stricmp(c, "useitem") && argc >= 2) {
        // useitem NAME[@TARGET]: the current character uses a party inventory item as "Use" does (game thread).
        char *name = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 96); // freed on the game thread
        for (int i = 1; i < argc; i++) {
            if (i > 1)
                strncat(name, " ", 95 - strlen(name));
            strncat(name, argv[i], 95 - strlen(name));
        }
        Out("useitem %s: %s", name, AgentRunSync(UseItemOnGameThread, name) ? "requested" : "no window");
    } else if (!_stricmp(c, "npchp") && argc >= 3) {
        // npchp FRACTION NAME: damage a script character down to FRACTION of its base max HP (game thread).
        char *block = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(float) + 64); // freed there
        *(float *)block = (float)atof(argv[1]);
        char *name = block + sizeof(float);
        for (int i = 2; i < argc; i++) {
            if (i > 2)
                strncat(name, " ", 63 - strlen(name));
            strncat(name, argv[i], 63 - strlen(name));
        }
        Out("npchp %s %s: %s", argv[1], name, AgentRunSync(NpcHpOnGameThread, block) ? "requested" : "no window");
    } else if (!_stricmp(c, "dlg")) {
        // dlg [N] (2026-11-04): the open conversation as text, for playing the story from a script: dialogue_mode
        // (0x5bc48c: 0 closed, 1 text, 3 topics), the speaker, the last N lines of the text box (0x2544408, default
        // 8) and the offered topics of the topic box (0x2545b68) with the point to click (capture pixels, the
        // game's mouse coordinates). In mode 1 "more" sits at the topic box's origin (new_dialogue.cpp).
        const BYTE *tb = (const BYTE *)(UINT_PTR)SumVa(0x2544408u), *tp = (const BYTE *)(UINT_PTR)SumVa(0x2545b68u);
        const BYTE *ctl = (const BYTE *)(UINT_PTR)SumVa(0x25464b0u);
        const char *who = *(const char *const *)(ctl + 0x2c);
        int want = argc > 1 ? atoi(argv[1]) : 8, first = *(const int *)(tb + 0x20), count = *(const int *)(tb + 0x24);
        Out("dlg: mode %d, state %d, speaker %s", *(const int *)(UINT_PTR)SumVa(0x5bc48cu), *(const int *)(ctl + 8),
            who && !IsBadReadPtr(who, 1) ? who : "-");
        for (int i = count > want ? count - want : 0; i < count; i++) {
            const BYTE *ln = tb + 0x2c + 0x1c * ((first + i) % 64);
            const char *t = *(const char *const *)(ln + 0x14);
            if (t && !IsBadReadPtr(t, 1))
                Out("  line %d: %.300s", *(const int *)(ln + 0x10), t);
        }
        int bx = *(const int *)(tp + 4), by = *(const int *)(tp + 8), n = *(const int *)(tp + 0x24);
        int scroll = (int)*(const float *)(tp + 0xc);
        Out("  box %d %d (more / end here in text mode), %d topics", bx, by, n);
        for (int i = 0; i < n && i < 64; i++) {
            const BYTE *e = tp + 0x2c + 0x18 * i;
            const BYTE *topic = *(const BYTE *const *)(e + 0x14);
            const char *t = topic && !IsBadReadPtr(topic, 0x10) ? *(const char *const *)(topic + 0xc) : NULL;
            Out("  topic %d at %d %d: %.200s", i, bx + 20, by + *(const int *)(e + 4) + scroll + *(const int *)(e + 0xc) / 2,
                t && !IsBadReadPtr(t, 1) ? t : "-");
        }
    } else if (!_stricmp(c, "items")) {
        // items [R] (2026-11-04): the item objects (type 3) and containers (type 11) of obj_active_list (0x970770)
        // within R (default 15) of the current character: address, distance, flags +0x84 and the items (ItemType
        // name through item_info +4), for loot that is hard to see on screen. Open a container with "open ADDR".
        const BYTE *head = (const BYTE *)(UINT_PTR)SumVa(0x970770u);
        const BYTE *me = *(const BYTE *const *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
        float r = argc > 1 ? (float)atof(argv[1]) : 15.0f;
        int shown = 0;
        for (const BYTE *o = *(const BYTE *const *)(head + 4); me && o && o != head && shown < 40; o = *(const BYTE *const *)(o + 4)) {
            int type = *(const int *)(o + 0xc);
            if (type != 3 && type != 11)
                continue;
            float dx = *(const float *)(o + 0x10) - *(const float *)(me + 0x10);
            float dy = *(const float *)(o + 0x14) - *(const float *)(me + 0x14);
            float dz = *(const float *)(o + 0x18) - *(const float *)(me + 0x18);
            float d = sqrtf(dx * dx + dy * dy + dz * dz);
            if (d > r)
                continue;
            char names[400];
            int len = 0, n = type == 3 ? 1 : *(const int *)(o + 0xd8);
            names[0] = 0;
            for (int i = 0; i < n && i < 16; i++) {
                const BYTE *it = type == 3 ? o : *(const BYTE *const *)(o + 0x98 + 4 * i);
                const BYTE *info = it ? *(const BYTE *const *)(it + 0x98) : NULL;
                const BYTE *ty = info ? *(const BYTE *const *)(info + 4) : NULL;
                const char *nm = ty ? *(const char *const *)ty : NULL;
                len += _snprintf(names + len, sizeof names - len - 1, "%s%s x%d", i ? ", " : "",
                    nm && !IsBadReadPtr(nm, 1) ? nm : "?", it ? *(const int *)(it + 0x9c) : 0);
                if (len >= (int)sizeof names - 40)
                    break;
            }
            Out("items: %s %08lX at %.1f (flags %X): %s", type == 3 ? "item" : "container", (unsigned long)(UINT_PTR)o, d,
                *(const unsigned *)(o + 0x84), names);
            shown++;
        }
        if (!shown)
            Out("items: none within %.1f", r);
    } else if (!_stricmp(c, "party")) {
        // party (2026-11-10): every party member (living_list entries of type 7, +0x0c) with its position (+0x10),
        // layer (+0x68c), HP (+0x5f8) and flags (+0x320); '*' marks cur_character. The same line in the 64-bit
        // host, so test scripts need no structure offsets of their own.
        const BYTE *head = (const BYTE *)(UINT_PTR)SumVa(0x8eca60u);
        const BYTE *me = *(const BYTE *const *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
        int n = 0;
        for (const BYTE *o = *(const BYTE *const *)(head + 0x318); o && o != head && n < 16; o = *(const BYTE *const *)(o + 0x318)) {
            if (*(const int *)(o + 0x0c) != 7)
                continue;
            n++;
            Out("party %s%s pos %.2f %.2f %.2f layer %d hp %.0f flags %x", o == me ? "*" : "", (const char *)(o + 0x2f4),
                (double)*(const float *)(o + 0x10), (double)*(const float *)(o + 0x14), (double)*(const float *)(o + 0x18),
                *(const int *)(o + 0x68c), (double)*(const float *)(o + 0x5f8), (unsigned)*(const DWORD *)(o + 0x320));
        }
        if (!n)
            Out("party: none");
    } else if (!_stricmp(c, "talkto") && argc >= 2) {
        // talkto NAME (2026-11-04): the nearest character of living_list (0x8eca60, linked at +0x318) whose name
        // (+0x2f4, character_get_name) starts with NAME ('~' for spaces, case ignored) is talked to as a click
        // on it does: player_talk_to (0x43c5a0) on the game thread (the party walks up as after a click).
        // For shopkeepers and others that npcgoto / script_npc_talk cannot find (not script objects).
        char want[64];
        lstrcpynA(want, argv[1], sizeof want);
        for (char *q = want; *q; q++)
            if (*q == '~')
                *q = ' ';
        const BYTE *head = (const BYTE *)(UINT_PTR)SumVa(0x8eca60u);
        const BYTE *me = *(const BYTE *const *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
        const BYTE *best = NULL;
        float bestd = 1e30f;
        size_t wl = strlen(want);
        // 2026-11-09: a hidden or disabled character (flags +0x320 with 0x2, as a script hide and
        // script_npc_set_disabled set it), a dying (0x8) or a dead one (0x80) is skipped; before this, talkto reached
        // Torvel's hidden double in lenele1ab (the false bug G312). Not obj_flags bit 0 or flag 0x20: standing NPCs
        // such as Torva have both cleared / set (AI inactive) and are talked to by a click.
        int skipped = 0;
        for (const BYTE *o = *(const BYTE *const *)(head + 0x318); me && o && o != head; o = *(const BYTE *const *)(o + 0x318)) {
            if (_strnicmp((const char *)(o + 0x2f4), want, wl))
                continue;
            if (*(const DWORD *)(o + 0x320) & 0x8a) {
                skipped++;
                continue;
            }
            float dx = *(const float *)(o + 0x10) - *(const float *)(me + 0x10);
            float dz = *(const float *)(o + 0x18) - *(const float *)(me + 0x18);
            if (dx * dx + dz * dz < bestd) {
                bestd = dx * dx + dz * dz;
                best = o;
            }
        }
        if (!best) {
            Out("talkto %s: no such character (%d hidden or disabled skipped)", want, skipped);
        } else {
            g_callAddr = 0x43c5a0u;
            g_callIntA = (DWORD)(UINT_PTR)best;
            g_callIntB = 0;
            Out("talkto %s: %s at %.1f, %s", want, (const char *)(best + 0x2f4), sqrtf(bestd),
                AgentRunSync(CallIntOnGameThread, NULL) ? "requested" : "no window");
        }
    } else if (!_stricmp(c, "open") && argc >= 2) {
        // open ADDR (2026-11-04): open a container from "items" as clicking it does: pickup_container (0x5c3b6c) =
        // ADDR, then interface_set_screen(4, &pickup_container) (0x412620) on the game thread (the "bag" recipe).
        *(DWORD *)(UINT_PTR)SumVa(0x5c3b6cu) = (DWORD)strtoul(argv[1], NULL, 16);
        g_callAddr = 0x412620u;
        g_callIntA = 4;
        g_callIntB = SumVa(0x5c3b6cu);
        Out("open %s: %s", argv[1], AgentRunSync(CallIntOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "npcdead") && argc >= 2) {
        // npcdead NAME: script_object_find (0x4cddf0) and script_npc_is_dead (0x4ce600) for a script character.
        char name[64] = "";
        for (int i = 1; i < argc; i++) {
            if (i > 1)
                strncat(name, " ", sizeof name - strlen(name) - 1);
            strncat(name, argv[i], sizeof name - strlen(name) - 1);
        }
        int index = ((int(__cdecl *)(const char *))(UINT_PTR)SumVa(0x4cddf0u))(name);
        int dead = ((char(__cdecl *)(const char *))(UINT_PTR)SumVa(0x4ce600u))(name);
        Out("npcdead %s: script object %d, dead %d", name, index, dead);
    } else if (!_stricmp(c, "quit")) {
        ((void(__cdecl *)(int))(UINT_PTR)SumVa(VA_GAMESEQ_SET_STATE))(3);
        Out("quit: requested");
    } else {
        Out("unknown command: %s", c);
    }
}

static void WriteResult(void)
{
    char tmp[MAX_PATH], dst[MAX_PATH];
    Path(tmp, "result.tmp");
    Path(dst, "result.txt");
    HANDLE f = CreateFileA(tmp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return;
    DWORD wr;
    WriteFile(f, g_out, (DWORD)g_outLen, &wr, NULL);
    CloseHandle(f);
    MoveFileExA(tmp, dst, MOVEFILE_REPLACE_EXISTING);
}

static DWORD WINAPI AgentThread(LPVOID unused)
{
    char cmd[MAX_PATH], run[MAX_PATH], ready[MAX_PATH];
    Path(cmd, "cmd.txt");
    Path(run, "cmd.run");
    Path(ready, "ready.txt");
    HANDLE r = CreateFileA(ready, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (r != INVALID_HANDLE_VALUE)
        CloseHandle(r);
    for (;;) {
        Sleep(20);
        *(volatile BYTE *)(UINT_PTR)SumVa(VA_IS_MAIN_WND_ACTIVE) = 1; // never pause for lost focus
        if (!MoveFileExA(cmd, run, MOVEFILE_REPLACE_EXISTING))
            continue;
        static char text[16384];
        HANDLE f = CreateFileA(run, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        DWORD n = 0;
        if (f != INVALID_HANDLE_VALUE) {
            ReadFile(f, text, sizeof text - 1, &n, NULL);
            CloseHandle(f);
        }
        text[n] = 0;
        DeleteFileA(run);
        g_outLen = 0;
        g_out[0] = 0;
        for (char *p = text; *p;) {
            char *end = p + strcspn(p, ";\n");
            char save = *end;
            *end = 0;
            char line[512];
            lstrcpynA(line, p, sizeof line);
            RunLine(line);
            if (!save)
                break;
            p = end + 1;
        }
        Out("done");
        WriteResult();
    }
    return 0;
}

void AgentInstall(void)
{
    if (!GetPrivateProfileIntA("Debug", "Agent", 0, g_cfg.iniPath))
        return;
    lstrcpynA(g_dir, g_cfg.iniPath, MAX_PATH);
    char *slash = strrchr(g_dir, '\\');
    strcpy(slash ? slash + 1 : g_dir, "agent");
    CreateDirectoryA(g_dir, NULL);
    char ready[MAX_PATH];
    Path(ready, "ready.txt");
    DeleteFileA(ready);
    CloseHandle(CreateThread(NULL, 0, AgentThread, NULL, 0, NULL));
    Log("agent mode: commands from %s\\cmd.txt", g_dir);
}
