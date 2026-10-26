// agent.cpp - agent mode in the source (include/modern/modern.h; rung L3b). Not original source.
//
// SummonerFix's agent mode (src/summonerfix/agent.c) for a build that runs without it, with the same protocol, so
// tools/agent.mjs and the check tools work unchanged: with [Debug] Agent=1 a thread polls <game>\agent\cmd.txt
// (written atomically), runs its commands (';' or newlines between them) and writes agent\result.txt; ready.txt says
// it is listening. Coordinates are capture pixels (half the window's client size). The game is kept "in the
// foreground" so it does not pause when another window has the focus.
//
// The commands are SummonerFix's. Characters are reached through their members (the layouts differ in a 64-bit
// build); peek, poke, count, dumpdata and the call commands take Sum.exe addresses, translated through the address
// table (modern_sum_va). Not here: pad (the controller), speedkey, hybridcov, k310 and pokecode, which belong to
// SummonerFix's own features.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "modern/modern.h"
#include "../../summonerfix/keynames.h"
#include "vsdk/os/os.h"
#include "vsdk/os/key.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/frametime.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "summoner/player/player.h"
#include "summoner/level_info.h"
#include "engine/objects/living_entity.h"
#include "engine/spells/spells.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/script_internal.h"
#include "summoner/interface/new_buysell.h"

#ifdef _WIN64
#define AGENT_THISCALL
#else
#define AGENT_THISCALL __attribute__((thiscall))
#endif
typedef size_t agent_word;   // one integer argument of a call command (a pointer below 2 GB fits in either build)

static char agent_dir[MAX_PATH];
static char agent_out[16384];
static size_t agent_out_len;

static void out(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = _vsnprintf(agent_out + agent_out_len, sizeof agent_out - agent_out_len - 2, fmt, ap);
    va_end(ap);
    if (n > 0)
        agent_out_len += (size_t)n;
    agent_out[agent_out_len++] = '\n';
    agent_out[agent_out_len] = 0;
}

static void path_of(char *dst, const char *name)
{
    _snprintf(dst, MAX_PATH, "%s\\%s", agent_dir, name);
    dst[MAX_PATH - 1] = 0;
}

// ---- game-thread calls: posted to the main window and run by the game's message pump (os_add_msg_handler) ----
#define WM_MODERN_AGENT_RUN (WM_APP + 0x5a1)
static void (*volatile sync_fn)(void *);
static volatile LONG sync_done;

static void agent_msg_handler(UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_MODERN_AGENT_RUN) {
        ((void (*)(void *))wparam)((void *)lparam);
        InterlockedExchange((LONG *)&sync_done, 1);
    }
}

// Runs fn(arg) on the game thread and waits (up to 10 s) until it has run, so the next command cannot change the
// arguments it reads.
static bool run_sync(void (*fn)(void *), void *arg)
{
    HWND wnd = os_get_main_wnd();
    InterlockedExchange((LONG *)&sync_done, 0);
    if (!wnd || !PostMessageA(wnd, WM_MODERN_AGENT_RUN, (WPARAM)fn, (LPARAM)arg))
        return false;
    for (int i = 0; i < 2000 && !sync_done; i++)
        Sleep(5);
    if (!sync_done)
        modern_log("agent: a game-thread call has not run after 10 s");
    return true;
}

static void *sum_va(unsigned int addr)
{
    void *p = modern_sum_va(addr);
    if (!p)
        modern_log("agent: no address for Sum.exe 0x%06X in this exe's table", addr);
    return p;
}

// ---- game-thread bodies ----
static char level_name[32], level_script[32];
static void change_level(void *) { modern_log("agent: game_change_level(%s, %s)", level_name, level_script);
                                   game_change_level(level_name, level_script); }

static unsigned int call_addr;
static int call_count;
static agent_word call_args[8];
static int call_argc;
static char call_str[64];

static void call0_body(void *)
{
    char saved[sizeof level_info.filename * 3];
    memcpy(saved, level_info.filename, sizeof saved);   // the encounter pickers change the current level's names
    void *fn = sum_va(call_addr);
    for (int i = 0; fn && i < call_count; i++) {
        ((void (*)())fn)();
        modern_log("agent: call0 %06X #%d: level %s script %s", call_addr, i + 1, level_info.filename,
                   level_info.filename + sizeof level_info.filename);
    }
    memcpy(level_info.filename, saved, sizeof saved);
}

static void calli_body(void *)
{
    void *fn = sum_va(call_addr);
    if (fn) {
        agent_word r = ((agent_word (*)(agent_word, agent_word))fn)(call_args[0], call_args[1]);
        modern_log("agent: calli %06X(0x%I64X, 0x%I64X) = 0x%I64X", call_addr, (unsigned __int64)call_args[0],
                   (unsigned __int64)call_args[1], (unsigned __int64)r);
    }
}

static void callstr_body(void *)
{
    void *fn = sum_va(call_addr);
    if (fn) {
        agent_word r = ((agent_word (*)(const char *, agent_word, agent_word))fn)(call_str, call_args[0], call_args[1]);
        modern_log("agent: callstr %06X(\"%s\", 0x%I64X, 0x%I64X) = 0x%I64X", call_addr, call_str,
                   (unsigned __int64)call_args[0], (unsigned __int64)call_args[1], (unsigned __int64)r);
    }
}

// thiscall with 0-6 integer arguments: the exact type per count, since a 32-bit thiscall callee pops its own.
static void callt_body(void *)
{
    void *fn = sum_va(call_addr);
    if (!fn)
        return;
    agent_word t = call_args[7], *a = call_args, r = 0;
    typedef agent_word w;
    switch (call_argc) {
    case 0: r = ((w (AGENT_THISCALL *)(w))fn)(t); break;
    case 1: r = ((w (AGENT_THISCALL *)(w, w))fn)(t, a[0]); break;
    case 2: r = ((w (AGENT_THISCALL *)(w, w, w))fn)(t, a[0], a[1]); break;
    case 3: r = ((w (AGENT_THISCALL *)(w, w, w, w))fn)(t, a[0], a[1], a[2]); break;
    case 4: r = ((w (AGENT_THISCALL *)(w, w, w, w, w))fn)(t, a[0], a[1], a[2], a[3]); break;
    case 5: r = ((w (AGENT_THISCALL *)(w, w, w, w, w, w))fn)(t, a[0], a[1], a[2], a[3], a[4]); break;
    default: r = ((w (AGENT_THISCALL *)(w, w, w, w, w, w, w))fn)(t, a[0], a[1], a[2], a[3], a[4], a[5]); break;
    }
    modern_log("agent: callt %06X(this 0x%I64X, %d args) = 0x%I64X", call_addr, (unsigned __int64)t, call_argc,
               (unsigned __int64)r);
}

// savenow: quicksave.sav written as quicksave (0x40e7c0) writes it once its checks pass (thumbnail, conversation
// state, savegame::write), without savegame_location_allowed, so combat and the area rules do not refuse it.
static void savenow_body(void *)
{
    if (!sum_va(0x427100) || !sum_va(0x4d8390) || !sum_va(0x25461a0) || !sum_va(0x4d1c40) || !sum_va(0x2493a38)) {
        modern_log("agent: savenow: not in the address table");
        return;
    }
    ((void (*)())sum_va(0x427100))();                                                  // saveload_capture_thumbnail
    ((void (AGENT_THISCALL *)(void *))sum_va(0x4d8390))(sum_va(0x25461a0));           // conversation_db.save_state
    int r = ((int (AGENT_THISCALL *)(void *, const char *, int, const char *))sum_va(0x4d1c40))(
        sum_va(0x2493a38), "quicksave.sav", 0, "Quick save");                          // savegame_state.write
    modern_log("agent: savenow: %s", r ? "quicksave.sav written" : "write failed");
}

// shop NAME: opens the shop of a level script character as a click on a shopkeeper does (gametarget.cpp,
// WORLD_CLICK_SHOP: shop_set_shopkeeper_from_char, then screen 7), on the game thread.
static char shop_name[64];
static void shop_body(void *)
{
    Character *npc = script_object_find_character(shop_name);
    if (!npc) {
        modern_log("agent: shop %s: no such NPC", shop_name);
        return;
    }
    modern_log("agent: shop %s: %s", shop_name, level_shopkeeper_find(npc->name) ? "a shopkeeper" : "not a shopkeeper");
    shop_set_shopkeeper_from_char(npc);
    interface_set_screen(7, NULL);
}

static int summon_spell;
static void summon_body(void *)
{
    Character *c = cur_character;
    if (!c) { modern_log("agent: summon: no character"); return; }
    vector3 pos = c->pos;
    pos.x += 2.0f;
    summon_create(c, &pos, c->layer, summon_spell, -1);
    modern_log("agent: summon spell %d at %.2f %.2f %.2f layer %d: cur_summon %p", summon_spell, pos.x, pos.y, pos.z,
               c->layer, (void *)cur_summon);
}

static int knock_ms;
static void knock_body(void *)
{
    Character *c = cur_character;
    if (!c) { modern_log("agent: knock: no character"); return; }
    int r = c->ai.knock_down_local(knock_ms);
    modern_log("agent: knock %d ms: result %d", knock_ms, r);
}

// Sum.exe functions the test helpers call by address (as SummonerFix does).
#define VA_ITEM_CREATE_BY_NAME 0x44b320u
#define VA_ITEM_OBJ_SPAWN 0x46a1b0u
#define VA_CONTAINER_OBJ_CREATE 0x4897f0u
#define VA_CONTAINER_ADD_ITEM 0x46aa30u
#define VA_SPELL_PLACE_GROUND_EFFECT 0x47e100u
#define VA_GET_HIT_BY_SPELL 0x45dea0u
static const float ident[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };

static int gold_amount;
static void goldpile_body(void *)
{
    Character *c = cur_character;
    void *create = sum_va(VA_ITEM_CREATE_BY_NAME), *spawn = sum_va(VA_ITEM_OBJ_SPAWN);
    void *item = c && create && spawn ? ((void *(*)(const char *))create)("Gold") : NULL;
    if (!item) { modern_log("agent: goldpile: no character or no Gold item"); return; }
    vector3 pos = c->pos;
    pos.x += 2.0f;
    void *obj = ((void *(*)(void *, const vector3 *, const float *, int, int, int))spawn)(item, &pos, ident, gold_amount, -1, -1);
    modern_log("agent: goldpile %d: object %p", gold_amount, obj);
}

static int bag_count;
static char bag_item[48] = "Health Tonic";
static void bag_body(void *)
{
    Character *c = cur_character;
    void *create = sum_va(VA_ITEM_CREATE_BY_NAME), *spawn = sum_va(VA_ITEM_OBJ_SPAWN);
    void *make = sum_va(VA_CONTAINER_OBJ_CREATE), *add = sum_va(VA_CONTAINER_ADD_ITEM);
    if (!c || !create || !spawn || !make || !add) { modern_log("agent: bag: no character or no address"); return; }
    vector3 pos = c->pos;
    pos.x += 2.0f;
    void *bag = ((void *(*)(const vector3 *, float))make)(&pos, 0.0f);
    int n = 0;
    for (; bag && n < bag_count; n++) {
        void *item = ((void *(*)(const char *))create)(bag_item);
        void *obj = item ? ((void *(*)(void *, const vector3 *, const float *, int, int, int))spawn)(item, &pos, ident, 1, -1, -1) : NULL;
        if (!obj)
            break;
        ((void (AGENT_THISCALL *)(void *, void *))add)(bag, obj);
    }
    modern_log("agent: bag of %d x %s: container %p", n, bag_item, bag);
}

static int firewall_count, firewall_by_summon;
static float firewall_start = 6.0f;
static void firewall_body(void *)
{
    Character *c = cur_character, *caster = firewall_by_summon ? cur_summon : cur_character;
    void *place = sum_va(VA_SPELL_PLACE_GROUND_EFFECT);
    if (!c || !caster || !place) { modern_log("agent: firewall: no character, summon or address"); return; }
    int made = 0;
    for (int i = 0; i < firewall_count; i++) {
        vector3 pos = c->pos;
        pos.x += firewall_start + 3.0f * (float)i;
        int h = ((int (*)(int, int, const vector3 *, int, const float *))place)(0x1d, caster->handle, &pos, c->layer, ident);
        if (h != -1)
            made++;
    }
    modern_log("agent: firewall: %d of %d areas placed", made, firewall_count);
}

static Character *hit_target;
static int hit_spell;
static float hit_power;
static void spellhit_body(void *)
{
    Character *c = cur_character;
    void *fn = sum_va(VA_GET_HIT_BY_SPELL);
    if (!c || !hit_target || !fn) { modern_log("agent: spellhit: no character, target or address"); return; }
    typedef void (AGENT_THISCALL *hit_fn)(void *, int, int, void *, void *, int, void *, float, int);
    ((hit_fn)fn)(hit_target, hit_spell, c->handle, NULL, NULL, 6, NULL, hit_power, 0);
    modern_log("agent: spellhit %p spell 0x%x power %.1f", (void *)hit_target, hit_spell, hit_power);
}

// npckill / npchp: a request block (a fraction, then the name) per call, freed on the game thread.
struct npc_request { float frac; char name[64]; };
static void npc_body(void *arg)
{
    npc_request *r = (npc_request *)arg;
    Character *npc = script_object_find_character(r->name);
    if (!npc) {
        modern_log("agent: npc %s: no such character", r->name);
    } else if (r->frac < 0.0f) {
        // add_hp ignores type 4 (non-hostile NPCs), so such a character becomes a monster first.
        if (npc->type == 4)
            npc->type = 5;
        npc->add_hp(-1000000.0f, -1, 0, false);
        modern_log("agent: npckill %s: hp %.0f, flags %08X", r->name, npc->hp, npc->flags);
    } else {
        float want = r->frac * npc->base_max_hp;
        if (want < npc->hp)
            npc->add_hp(want - npc->hp, -1, 0, false);
        modern_log("agent: npchp %s %.2f: hp %.0f of %.0f", r->name, r->frac, npc->hp, npc->base_max_hp);
    }
    free(r);
}

static void join_words(char *dst, size_t size, char **argv, int from, int argc)
{
    dst[0] = 0;
    for (int i = from; i < argc; i++) {
        if (i > from)
            strncat(dst, " ", size - strlen(dst) - 1);
        strncat(dst, argv[i], size - strlen(dst) - 1);
    }
}

static int button_of(const char *s)
{
    if (!s || !*s || !_stricmp(s, "left")) return 0;
    if (!_stricmp(s, "right")) return 1;
    if (!_stricmp(s, "middle")) return 2;
    return atoi(s);
}

static void move_to(int x, int y) { modern_set_agent_cursor(x * 2, y * 2); }

static void run_line(char *line)
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
        if (code < 0 || code >= 256) { out("%s: unknown key", c); return; }
        if (!_stricmp(c, "keyup")) {
            key_mark(code, 0, 0);
        } else {
            key_mark(code, 1, 0);
            if (!_stricmp(c, "key")) {
                Sleep(argc > 2 ? (DWORD)atoi(argv[2]) : 80);
                key_mark(code, 0, 0);
            }
        }
        out("%s %s: ok", c, argv[1]);
    } else if (!_stricmp(c, "postkey") && argc > 1) {
        // A real WM_KEYDOWN / WM_KEYUP through the game's window procedure (unlike "key"), for message handlers.
        int code = KeyParse(argv[1]);
        HWND wnd = os_get_main_wnd();
        if (code < 0 || code >= 256 || !wnd) { out("postkey: unknown key or no window"); return; }
        UINT vk = MapVirtualKeyA((code & 0x7f) | (code & 0x80 ? 0xe000 : 0), 3 /* MAPVK_VSC_TO_VK_EX */);
        LPARAM lp = 1 | ((LPARAM)(code & 0x7f) << 16) | (code & 0x80 ? 0x01000000 : 0);
        PostMessageA(wnd, WM_KEYDOWN, vk, lp);
        Sleep(argc > 2 ? (DWORD)atoi(argv[2]) : 80);
        PostMessageA(wnd, WM_KEYUP, vk, lp | 0xC0000000);
        out("postkey %s: ok (vk 0x%02X)", argv[1], vk);
    } else if (!_stricmp(c, "postmsg") && argc >= 4) {
        UINT msg = (UINT)strtoul(argv[1], NULL, 16);
        PostMessageA(os_get_main_wnd(), msg, (WPARAM)strtoul(argv[2], NULL, 16), (LPARAM)strtoul(argv[3], NULL, 16));
        out("postmsg 0x%X: ok", msg);
    } else if (!_stricmp(c, "move") && argc >= 3) {
        move_to(atoi(argv[1]), atoi(argv[2]));
        out("move: ok");
    } else if (!_stricmp(c, "click")) {
        int a = 1, b = 0;
        if (argc > 1 && (argv[1][0] < '0' || argv[1][0] > '9')) { b = button_of(argv[1]); a = 2; }
        if (argc >= a + 2) {
            move_to(atoi(argv[a]), atoi(argv[a + 1]));
            Sleep(60);   // let the game see the new position for a frame or two
            a += 2;
        }
        mouse_mark_button(b, true);
        Sleep(argc > a ? (DWORD)atoi(argv[a]) : 80);
        mouse_mark_button(b, false);
        out("click: ok");
    } else if ((!_stricmp(c, "down") || !_stricmp(c, "up")) && argc >= 2) {
        mouse_mark_button(button_of(argv[1]), !_stricmp(c, "down"));
        out("%s: ok", c);
    } else if (!_stricmp(c, "drag") && argc >= 5) {
        int x0 = atoi(argv[1]), y0 = atoi(argv[2]), x1 = atoi(argv[3]), y1 = atoi(argv[4]);
        move_to(x0, y0);
        Sleep(60);
        mouse_mark_button(0, true);
        Sleep(80);
        for (int i = 1; i <= 10; i++) {
            move_to(x0 + (x1 - x0) * i / 10, y0 + (y1 - y0) * i / 10);
            Sleep(30);
        }
        Sleep(60);
        mouse_mark_button(0, false);
        out("drag: ok");
    } else if (!_stricmp(c, "wait") && argc >= 2) {
        Sleep((DWORD)atoi(argv[1]));
    } else if (!_stricmp(c, "capture") && argc >= 2) {
        char path[MAX_PATH], file[MAX_PATH];
        _snprintf(file, sizeof file, "%s.bmp", argv[1]);
        file[sizeof file - 1] = 0;
        path_of(path, file);
        modern_request_capture(path);
        for (int i = 0; i < 200 && modern_capture_pending(); i++)
            Sleep(10);
        out(modern_capture_pending() ? "capture %s: timed out (is the game drawing?)" : "capture %s: ok", file);
    } else if (!_stricmp(c, "info")) {
        int cx, cy;
        modern_get_agent_cursor(&cx, &cy);
        out("state=%d screen=%d fps=%.1f foreground=%d cursor=%d,%d", gameseq_state_cur, interface_cur_screen, fps,
            (int)is_main_wnd_active, cx / 2, cy / 2);
    } else if (!_stricmp(c, "peek") && argc >= 2) {
        unsigned int va = (unsigned int)strtoul(argv[1], NULL, 16);
        int n = argc > 2 ? atoi(argv[2]) : 16;
        if (n <= 0 || n > 256) n = 16;
        const unsigned char *p = (const unsigned char *)modern_sum_va(va);
        if (!p) { out("peek %06X: not in the address table", va); return; }
        char line[3 * 256 + 32];
        int len = _snprintf(line, sizeof line, "%06X:", va);
        for (int i = 0; i < n; i++)
            len += _snprintf(line + len, sizeof line - (size_t)len, " %02X", p[i]);
        out("%s", line);
    } else if (!_stricmp(c, "poke") && argc >= 3) {
        unsigned int va = (unsigned int)strtoul(argv[1], NULL, 16);
        unsigned char *p = (unsigned char *)modern_sum_va(va);
        if (!p) { out("poke %06X: not in the address table", va); return; }
        unsigned char old = *p;
        *p = (unsigned char)strtoul(argv[2], NULL, 16);
        out("poke %06X: %02X -> %02X", va, old, *p);
    } else if (!_stricmp(c, "count") && argc >= 5) {
        const unsigned char *p = (const unsigned char *)modern_sum_va((unsigned int)strtoul(argv[1], NULL, 16));
        unsigned int stride = (unsigned int)strtoul(argv[2], NULL, 16);
        int n = atoi(argv[3]), used = 0;
        unsigned char mask = (unsigned char)strtoul(argv[4], NULL, 16);
        for (int i = 0; p && i < n; i++)
            if (p[i * stride] & mask)
                used++;
        out("count %s: %d of %d", argv[1], used, n);
    } else if (!_stricmp(c, "dumpdata") && argc >= 3) {
        // dumpdata OUT TABLE: every data symbol of an address table, as records (Sum.exe address, size, bytes) read
        // at its address in this exe (tools/link-memcheck.mjs). In a 64-bit build the layouts differ by design.
        FILE *in = fopen(argv[2], "r"), *o = fopen(argv[1], "wb");
        int rows = 0;
        unsigned int orig, size;
        unsigned long long va;
        while (in && o && fscanf(in, "%x %x %I64x", &orig, &size, &va) == 3) {
            if (orig < 0x57db40 || size == 0 || size > 0x10000)
                continue;   // code, or too large to be worth comparing
            const void *src = modern_sum_va(orig);
            if (!src)
                continue;
            fwrite(&orig, 4, 1, o);
            fwrite(&size, 4, 1, o);
            fwrite(src, size, 1, o);
            rows++;
        }
        if (in) fclose(in);
        if (o) fclose(o);
        out("dumpdata: %d symbols", rows);
    } else if (!_stricmp(c, "flag") && argc >= 2) {
        if (argc >= 3)
            savegame_state.set_flag(cur_level_index, argv[1], atoi(argv[2]) != 0);
        out("flag %s (level %d) = %d", argv[1], cur_level_index, savegame_state.get_flag(cur_level_index, argv[1]));
    } else if (!_stricmp(c, "flagl") && argc >= 3) {
        int level = atoi(argv[1]);
        if (argc >= 4)
            savegame_state.set_flag(level, argv[2], atoi(argv[3]) != 0);
        out("flag %s (level %d) = %d", argv[2], level, savegame_state.get_flag(level, argv[2]));
    } else if (!_stricmp(c, "savenow")) {
        out("savenow: %s", run_sync(savenow_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "call0") && argc >= 2) {
        call_addr = (unsigned int)strtoul(argv[1], NULL, 16);
        call_count = argc >= 3 ? atoi(argv[2]) : 1;
        out("call0 %06X x%d: %s", call_addr, call_count, run_sync(call0_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "calli") && argc >= 3) {
        call_addr = (unsigned int)strtoul(argv[1], NULL, 16);
        call_args[0] = (agent_word)strtoul(argv[2], NULL, 0);
        call_args[1] = argc >= 4 ? (agent_word)strtoul(argv[3], NULL, 0) : 0;
        out("calli %06X: %s", call_addr, run_sync(calli_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "callt") && argc >= 3) {
        call_addr = (unsigned int)strtoul(argv[1], NULL, 16);
        call_args[7] = (agent_word)strtoul(argv[2], NULL, 0);
        call_argc = argc - 3 > 6 ? 6 : argc - 3;
        for (int i = 0; i < call_argc; i++)
            call_args[i] = (agent_word)strtoul(argv[3 + i], NULL, 0);
        out("callt %06X: %s", call_addr, run_sync(callt_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "callstr") && argc >= 3) {
        call_addr = (unsigned int)strtoul(argv[1], NULL, 16);
        lstrcpynA(call_str, argv[2], sizeof call_str);
        for (char *p = call_str; *p; p++)
            if (*p == '~')
                *p = ' ';
        call_args[0] = 0;
        call_args[1] = argc >= 5 ? (agent_word)strtol(argv[4], NULL, 0) : 0;
        if (argc >= 4) {
            if (strchr(argv[3], '.')) {
                float f = (float)atof(argv[3]);
                unsigned int bits;
                memcpy(&bits, &f, 4);
                call_args[0] = bits;   // a float argument passed as its bits, as SummonerFix does
            } else {
                call_args[0] = (agent_word)strtol(argv[3], NULL, 0);
            }
        }
        out("callstr %06X %s: %s", call_addr, call_str, run_sync(callstr_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "navgoto") && argc >= 2) {
        vector3 pos;
        matrix orient;
        Character *ch = cur_character;
        if (!ch || !navpoint_get_position(argv[1], &pos, &orient)) {
            out("navgoto %s: no character or no such navpoint", argv[1]);
        } else {
            ch->pos = pos;
            out("navgoto %s: %.2f %.2f %.2f", argv[1], pos.x, pos.y, pos.z);
        }
    } else if (!_stricmp(c, "shop") && argc >= 2) {
        join_words(shop_name, sizeof shop_name, argv, 1, argc);
        out("shop %s: %s", shop_name, run_sync(shop_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "npcgoto") && argc >= 2) {
        char name[64];
        join_words(name, sizeof name, argv, 1, argc);
        Character *ch = cur_character, *npc = script_object_find_character(name);
        if (!ch || !npc) {
            out("npcgoto %s: no character or no such NPC", name);
        } else {
            vector3 pos = npc->pos;
            out("npcgoto %s: NPC at %.2f %.2f %.2f", name, pos.x, pos.y, pos.z);
            pos.x += 4.0f;
            ch->pos = pos;
        }
    } else if (!_stricmp(c, "nudge") && argc >= 3) {
        Character *ch = cur_character;
        if (ch) {
            ch->pos.x += (float)atof(argv[1]);
            ch->pos.z += (float)atof(argv[2]);
            out("nudge: %.2f %.2f %.2f", ch->pos.x, ch->pos.y, ch->pos.z);
        }
    } else if (!_stricmp(c, "level") && argc >= 3) {
        // level NAME SCRIPT: change level as a level exit does; on the game thread (from this one the load crashed
        // mid-frame under SummonerFix).
        lstrcpynA(level_name, argv[1], sizeof level_name);
        lstrcpynA(level_script, argv[2], sizeof level_script);
        out("level %s %s: %s", argv[1], argv[2], run_sync(change_level, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "summon") && argc >= 2) {
        summon_spell = atoi(argv[1]);
        out("summon %d: %s", summon_spell, run_sync(summon_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "knock") && argc >= 2) {
        knock_ms = atoi(argv[1]);
        out("knock %d: %s", knock_ms, run_sync(knock_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "goldpile") && argc >= 2) {
        gold_amount = atoi(argv[1]);
        out("goldpile %d: %s", gold_amount, run_sync(goldpile_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "bag") && argc >= 2) {
        bag_count = atoi(argv[1]);
        if (argc >= 3) {
            lstrcpynA(bag_item, argv[2], sizeof bag_item);
            for (char *p = bag_item; *p; p++)
                if (*p == '~')
                    *p = ' ';
        }
        out("bag %d: %s", bag_count, run_sync(bag_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "firewall") && argc >= 2) {
        firewall_count = atoi(argv[1]);
        firewall_start = argc >= 3 ? (float)atof(argv[2]) : 6.0f;
        firewall_by_summon = argc >= 4 && !_stricmp(argv[3], "summon");
        out("firewall %d: %s", firewall_count, run_sync(firewall_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "spellhit") && argc >= 4) {
        // spellhit TARGET SPELL POWER: TARGET is the character's address in this process (hex), as info prints it.
        hit_target = (Character *)(size_t)strtoul(argv[1], NULL, 16);
        hit_spell = (int)strtol(argv[2], NULL, 16);
        hit_power = (float)atof(argv[3]);
        out("spellhit: %s", run_sync(spellhit_body, NULL) ? "requested" : "no window");
    } else if ((!_stricmp(c, "npckill") && argc >= 2) || (!_stricmp(c, "npchp") && argc >= 3)) {
        npc_request *r = (npc_request *)calloc(1, sizeof(npc_request));   // freed on the game thread
        bool kill = !_stricmp(c, "npckill");
        r->frac = kill ? -1.0f : (float)atof(argv[1]);
        join_words(r->name, sizeof r->name, argv, kill ? 1 : 2, argc);
        out("%s %s: %s", c, r->name, run_sync(npc_body, r) ? "requested" : "no window");
    } else if (!_stricmp(c, "npcdead") && argc >= 2) {
        char name[64];
        join_words(name, sizeof name, argv, 1, argc);
        out("npcdead %s: script object %d, dead %d", name, script_object_find(name), (int)script_npc_is_dead(name));
    } else if (!_stricmp(c, "quit")) {
        gameseq_set_state(3);
        out("quit: requested");
    } else {
        out("unknown command: %s", c);
    }
}

static void write_result()
{
    char tmp[MAX_PATH], dst[MAX_PATH];
    path_of(tmp, "result.tmp");
    path_of(dst, "result.txt");
    HANDLE f = CreateFileA(tmp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return;
    DWORD wr;
    WriteFile(f, agent_out, (DWORD)agent_out_len, &wr, NULL);
    CloseHandle(f);
    MoveFileExA(tmp, dst, MOVEFILE_REPLACE_EXISTING);
}

static DWORD WINAPI agent_thread(LPVOID)
{
    char cmd[MAX_PATH], run[MAX_PATH], ready[MAX_PATH];
    path_of(cmd, "cmd.txt");
    path_of(run, "cmd.run");
    path_of(ready, "ready.txt");
    HANDLE r = CreateFileA(ready, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (r != INVALID_HANDLE_VALUE)
        CloseHandle(r);
    static char text[16384];
    for (;;) {
        Sleep(20);
        is_main_wnd_active = true;   // never pause for a lost focus
        if (!MoveFileExA(cmd, run, MOVEFILE_REPLACE_EXISTING))
            continue;
        HANDLE f = CreateFileA(run, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        DWORD n = 0;
        if (f != INVALID_HANDLE_VALUE) {
            ReadFile(f, text, sizeof text - 1, &n, NULL);
            CloseHandle(f);
        }
        text[n] = 0;
        DeleteFileA(run);
        agent_out_len = 0;
        agent_out[0] = 0;
        for (char *p = text; *p;) {
            char *end = p + strcspn(p, ";\n");
            char save = *end;
            *end = 0;
            char line[512];
            lstrcpynA(line, p, sizeof line);
            run_line(line);
            if (!save)
                break;
            p = end + 1;
        }
        out("done");
        write_result();
    }
    return 0;
}

void modern_agent_init()
{
    if (!modern_host() || !modern_ini_int("Debug", "Agent", 0))
        return;
    _snprintf(agent_dir, MAX_PATH, "%s\\agent", modern_dir());
    CreateDirectoryA(agent_dir, NULL);
    char ready[MAX_PATH];
    path_of(ready, "ready.txt");
    DeleteFileA(ready);
    os_add_msg_handler(agent_msg_handler);
    CloseHandle(CreateThread(NULL, 0, agent_thread, NULL, 0, NULL));
    modern_log("modern agent: commands from %s\\cmd.txt", agent_dir);
}
