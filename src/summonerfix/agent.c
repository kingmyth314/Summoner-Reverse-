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
//   wait MS
//   capture NAME       screenshot to agent\NAME.bmp (waits until written)
//   info               game state, screen, fps, cursor
//   peek ADDR [N]      hex dump of game memory (N bytes, default 16)
//   poke ADDR BYTE     write one byte of (writable) game data, both in hex
//   quit               ask the game to exit (game state 3)
#include "summonerfix.h"
#include "keynames.h"
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
#define VA_SCRIPT_OBJECT_FIND_CHARACTER 0x4cde70u // cdecl (name) -> Character * of a level script object

static char g_dir[MAX_PATH];
static char g_out[16384];
static size_t g_outLen;

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

// N wall of fire areas (spell 0x1d) in a row 3 units apart, starting START units (default 6) beside the current
// character, as spell_cast_execute places them. The caster is the current character, or the active summon
// (cur_summon) with "summon" (game-bugs C270: how many areas can burn; G270: an area whose caster is gone).
#define VA_SPELL_PLACE_GROUND_EFFECT 0x47e100u // cdecl (spell, caster handle, pos, layer, orient) -> fx handle
#define VA_CUR_SUMMON 0x5fcc8cu                // Character *
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
        Log("agent: npckill %s: %p, hp %.0f, flags %08lX", name, (void *)npc, (double)*(float *)(npc + 0x5f8),
            *(DWORD *)(npc + 0x320));
    }
    HeapFree(GetProcessHeap(), 0, name);
}

// "calli ADDR A [B]": cdecl fn(A, B) with two integer arguments; eax logged.
static DWORD g_callIntA, g_callIntB;
static void CallIntOnGameThread(void *unused)
{
    (void)unused;
    DWORD r = ((DWORD(__cdecl *)(DWORD, DWORD))(UINT_PTR)SumVa(g_callAddr))(g_callIntA, g_callIntB);
    Log("agent: calli %06lX(0x%lX, 0x%lX) = 0x%lX", g_callAddr, g_callIntA, g_callIntB, r);
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
    char *argv[8];
    int argc = 0;
    for (char *t = strtok(line, " \t\r\n"); t && argc < 8; t = strtok(NULL, " \t\r\n"))
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
    } else if (!_stricmp(c, "wait") && argc >= 2) {
        Sleep((DWORD)atoi(argv[1]));
    } else if (!_stricmp(c, "capture") && argc >= 2) {
        Capture(argv[1]);
    } else if (!_stricmp(c, "info")) {
        Info();
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
        Out("calli %06lX: %s", g_callAddr, KeysAgentRun(CallIntOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "call0") && argc >= 2) {
        // call0 ADDR [N]: call the cdecl void game function at ADDR with no arguments, N times, on the game thread.
        g_callAddr = (DWORD)strtoul(argv[1], NULL, 16);
        g_call0Count = argc >= 3 ? atoi(argv[2]) : 1;
        Out("call0 %06lX x%d: %s", g_callAddr, g_call0Count, KeysAgentRun(Call0OnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "navgoto") && argc >= 2) {
        // navgoto NAVPOINT: put the current character on a level navpoint (e.g. $end2trig). Testing only.
        float pos[3], orient[9];
        typedef char(__cdecl * GetNav)(const char *, float *, float *);
        BYTE *ch = *(BYTE *volatile *)(UINT_PTR)SumVa(VA_CUR_CHARACTER);
        if (!ch || !((GetNav)(UINT_PTR)SumVa(VA_NAVPOINT_GET_POSITION))(argv[1], pos, orient)) {
            Out("navgoto %s: no character or no such navpoint", argv[1]);
        } else {
            memcpy(ch + 0x10, pos, sizeof pos);
            Out("navgoto %s: %.2f %.2f %.2f", argv[1], pos[0], pos[1], pos[2]);
        }
    } else if (!_stricmp(c, "npcgoto") && argc >= 2) {
        // npcgoto NAME: put the current character 4 units from a level script NPC (e.g. Luminar), as
        // script_object_find_character finds it. Testing only.
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
            pos[0] += 4.0f;
            memcpy(ch + 0x10, pos, sizeof pos);
        }
    } else if (!_stricmp(c, "level") && argc >= 3) {
        // level NAME SCRIPT: change level as a level exit does (game_change_level), e.g.
        // "level IonaEXT02 IonaEXT02_v2". Loads the level's own script objects, which the
        // play_and_load_cutscene warp does not. Testing only.
        // Runs on the game thread (KeysAgentRun): from this thread the load crashed mid-frame.
        lstrcpynA(g_level, argv[1], sizeof g_level);
        lstrcpynA(g_script, argv[2], sizeof g_script);
        Out("level %s %s: %s", argv[1], argv[2], KeysAgentRun(ChangeLevelOnGameThread, NULL) ? "requested" : "no window");
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
        Out("callstr %06lX %s: %s", g_callAddr, g_callStr, KeysAgentRun(CallStrOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "spellhit") && argc >= 4) {
        // spellhit TARGET SPELL POWER: e.g. "spellhit 90a4e8 3e 100" (leech on Joseph from the leader).
        g_hitTarget = (DWORD)strtoul(argv[1], NULL, 16);
        g_hitSpell = (int)strtol(argv[2], NULL, 16);
        g_hitPower = (float)atof(argv[3]);
        Out("spellhit: %s", KeysAgentRun(SpellHitOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "firewall") && argc >= 2) {
        // firewall N [START] [summon]: N wall of fire areas beside the current character (game thread; result in
        // the log), the first START units away, cast by the leader or by the active summon.
        g_fireWalls = atoi(argv[1]);
        g_fireWallStart = argc >= 3 ? (float)atof(argv[2]) : 6.0f;
        g_fireWallsBySummon = argc >= 4 && !_stricmp(argv[3], "summon");
        Out("firewall %d: %s", g_fireWalls, KeysAgentRun(FireWallsOnGameThread, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "goldpile") && argc >= 2) {
        // goldpile N: a loose gold pile with N gold beside the current character (game thread; result in the log).
        g_goldAmount = atoi(argv[1]);
        Out("goldpile %d: %s", g_goldAmount, KeysAgentRun(GoldPileOnGameThread, NULL) ? "requested" : "no window");
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
        Out("bag %d: %s", g_bagCount, KeysAgentRun(BagOnGameThread, NULL) ? "requested" : "no window");
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
        Out("npckill %s: %s", name, KeysAgentRun(NpcKillOnGameThread, name) ? "requested" : "no window");
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
