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
// table (modern_sum_va). pad drives the build's own controller code (experimental/controller.cpp, FIXED_BUILD;
// since 2026-11-03). Not here: hybridcov, k310 and pokecode, which belong to SummonerFix's own features. exitto,
// join / xpto, dlg, items, talkto and open (the story commands of the test saves) since 2026-11-05.
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
#include <math.h>
#include "summoner/gameplay.h"
#include "summoner/gametarget.h"
#include "levelscripts/conversation.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/characterinfo/characterinfo.h"

extern int dialogue_mode;                // 0x5bc48c new_dialogue.cpp
extern container *pickup_container;      // 0x5c3b6c new_pickup.cpp

#ifdef _WIN64
#define AGENT_THISCALL
#else
#define AGENT_THISCALL __attribute__((thiscall))
#endif
typedef size_t agent_word;   // one integer argument of a call command (a pointer below 2 GB fits in either build)

#ifdef FIXED_BUILD
// experimental/controller.cpp's synthetic pad state, the exports SummonerFix's pad forwards to.
extern "C" void __cdecl ExpControllerSetSynthetic(int lx, int ly, int rx, int ry, unsigned buttons, unsigned long ms);
extern "C" int __cdecl ExpControllerSyntheticDone();
#endif

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

// navgoto / npcgoto: the game's own teleport (party_teleport_to): the whole party, the layer, camera_reset_to_player.
// Writing only cur_character->pos (before 2026-11-06) kept the old layer and left the others and the camera behind.
static vector3 tp_pos;
static int tp_layer;
static void teleport_body(void *)
{
    party_teleport_to(&tp_pos, tp_layer);
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
        // Left "dying" mid-move (a Liangshan wave soldier killed while walking stayed so for good): no current
        // move, so the death finishes (SummonerFix's npckill, 2026-11-04).
        if ((npc->flags & (CF_HIDDEN | CF_DEAD)) == CF_HIDDEN)
            npc->cur_anim = -1;
        modern_log("agent: npckill %s: hp %.0f, flags %08X", r->name, npc->hp, npc->flags);
    } else {
        float want = r->frac * npc->base_max_hp;
        if (want < npc->hp)
            npc->add_hp(want - npc->hp, -1, 0, false);
        modern_log("agent: npchp %s %.2f: hp %.0f of %.0f", r->name, r->frac, npc->hp, npc->base_max_hp);
    }
    free(r);
}

// exitto LEVEL SCRIPT [ENTRY]: leave the level as the game's exits do (the names, the entry point, state 4), so the
// inventory and the journal stay (level goes through the title state, which resets them).
static int exit_entry;
static void exitto_body(void *)
{
    lstrcpynA(level_info.prev_filename, level_info.filename, sizeof level_info.prev_filename);
    lstrcpynA(level_info.filename, level_name, sizeof level_info.filename);
    lstrcpynA(level_info.script_name, level_script, sizeof level_info.script_name);
    level_info.entry_point = exit_entry;
    gameseq_set_state(4);
    modern_log("agent: exitto %s %s entry %d", level_name, level_script, exit_entry);
}

// join NAME LEVEL: the hero joins the party at LEVEL; xpto NAME LEVEL: a party member gets the experience up to LEVEL.
static char join_name[32];
static int join_level;
static bool join_xp_only;
static void join_body(void *)
{
    Character *c = party_member_join(join_name, join_xp_only ? 1 : join_level, false, true, -1);
    if (!c) {
        modern_log("agent: join %s: party_member_join returned NULL (unknown name, a load pending, or no Player record free)",
                   join_name);
        return;
    }
    if (join_xp_only) {
        int want = xp_for_level(join_level);
        if (want > c->xp)
            c->add_xp(want - c->xp, true, false);
    }
    modern_log("agent: %s %s: level %d, xp %d, HP %.0f, AP %d", join_xp_only ? "xpto" : "join", join_name, c->level, c->xp,
               c->hp, c->ap);
}

// talkto: player_talk_to on the character found (as a click on it); open: a container opened as a click on it.
static Character *talk_target;
static void talkto_body(void *) { player_talk_to(talk_target); }
static container *open_target;
static void open_body(void *)
{
    pickup_container = open_target;
    interface_set_screen(4, &pickup_container);
}

// useitem NAME[@TARGET] / NAME@@[TARGET]: the current character uses a party inventory item as the inventory screen's
// "Use" does (character_use_inventory_item with the target's handle, NULL, -1); "@@" casts at the target's feet and
// layer with handle -1, as world_use_item_at_point does (a NULL point there crashes). TARGET is a level script
// character (default: the user). SummonerFix's useitem (2026-11-08), here since 2026-11-09.
static char use_name[96];
static void useitem_body(void *)
{
    char *target = strchr(use_name, '@');
    bool at_point = false;
    if (target) {
        *target++ = 0;
        if (*target == '@') {
            at_point = true;
            target++;
        }
        if (!*target)
            target = NULL;
    }
    Character *ch = cur_character;
    party_inventory_entry *entry = NULL;
    for (int i = 0; i < party_inventory_count && !entry; i++) {
        item_info *it = party_inventory[i].item;
        if (it && it->type && it->type->name && !_stricmp(it->type->name, use_name))
            entry = &party_inventory[i];
    }
    Character *tc = target ? script_object_find_character(target) : ch;
    if (!ch || !entry || !tc) {
        modern_log("agent: useitem %s: %s", use_name,
                   !ch ? "no character" : !entry ? "not in the party inventory" : "no such target");
        return;
    }
    item_info *item = entry->item;
    int count = entry->count, charges = item->charges;
    vector3 point = tc->pos;
    bool ok = at_point ? character_use_inventory_item(ch, entry, -1, &point, tc->layer)
                       : character_use_inventory_item(ch, entry, tc->handle, NULL, -1);
    // the entry may have been removed (the last one used): look the item up again
    int after = -1, ch_after = -1;
    for (int i = 0; i < party_inventory_count; i++)
        if (party_inventory[i].item == item) {
            after = party_inventory[i].count;
            ch_after = item->charges;
        }
    modern_log("agent: useitem %s: used %d, count %d -> %d, charges %d -> %d", use_name, (int)ok, count, after, charges,
               ch_after);
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
            modern_key_mark_raw(code, 0);
        } else {
            modern_key_mark_raw(code, 1);
            if (!_stricmp(c, "key")) {
                Sleep(argc > 2 ? (DWORD)atoi(argv[2]) : 80);
                modern_key_mark_raw(code, 0);
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
    } else if (!_stricmp(c, "pad") && argc >= 6) {
        // pad LX LY RX RY BUTTONS [MS], as in SummonerFix (src/summonerfix/agent.c): the game thread reads the state
        // once per frame (exp_controller_frame), with [Controller] Enabled=0 too.
#ifdef FIXED_BUILD
        DWORD ms = argc > 6 ? (DWORD)atoi(argv[6]) : 100;
        ExpControllerSetSynthetic(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
                                  (unsigned)strtoul(argv[5], NULL, 16), ms);
        Sleep(ms);
        for (int i = 0; i < 100 && !ExpControllerSyntheticDone(); i++)
            Sleep(10);
        int cx, cy;
        modern_get_agent_cursor(&cx, &cy);
        out(ExpControllerSyntheticDone() ? "pad: ok (cursor %d,%d)" : "pad: not released after 1 s (is the game drawing?) (cursor %d,%d)",
            cx / 2, cy / 2);
#else
        out("pad: no controller code in this build (fixed and experimental builds only)");
#endif
    } else if (!_stricmp(c, "wait") && argc >= 2) {
        Sleep((DWORD)atoi(argv[1]));
    } else if ((!_stricmp(c, "capture") || !_stricmp(c, "capturefull")) && argc >= 2) {
        // capturefull: every pixel of the window (capture keeps every second one, the size of agent coordinates).
        char path[MAX_PATH], file[MAX_PATH];
        _snprintf(file, sizeof file, "%s.bmp", argv[1]);
        file[sizeof file - 1] = 0;
        path_of(path, file);
        modern_request_capture(path, !_stricmp(c, "capturefull"));
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
        // navgoto NAVPOINT: the party onto a navpoint and its layer, as SummonerFix's agent (teleport_body).
        navpoint *nav = navpoint_find(argv[1]);
        if (!cur_character || !nav) {
            out("navgoto %s: no character or no such navpoint", argv[1]);
        } else {
            tp_pos = nav->pos;
            tp_layer = nav->layer;
            run_sync(teleport_body, NULL);
            out("navgoto %s: %.2f %.2f %.2f layer %d", argv[1], tp_pos.x, tp_pos.y, tp_pos.z, tp_layer);
        }
    } else if (!_stricmp(c, "posgoto") && argc >= 5) {
        // posgoto X Y Z LAYER: the party to a point and layer, as SummonerFix's agent.
        tp_pos.x = (float)atof(argv[1]);
        tp_pos.y = (float)atof(argv[2]);
        tp_pos.z = (float)atof(argv[3]);
        tp_layer = atoi(argv[4]);
        out("posgoto %.2f %.2f %.2f layer %d: %s", tp_pos.x, tp_pos.y, tp_pos.z, tp_layer,
            run_sync(teleport_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "speedkey") && argc >= 2) {
        modern_keys_agent_speed(atoi(argv[1]) != 0);
        out("speedkey %s: ok", argv[1]);
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
            tp_pos = pos;
            tp_pos.x += 4.0f;
            tp_layer = npc->layer;
            run_sync(teleport_body, NULL);
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
        // A summon takes a Player record (5: the party's four and one summon); with none free summon_create stops the
        // game with "No more player slots" (player.cpp line 421), as a second summon of the full party did on
        // 2026-11-03. Refused here instead.
        summon_spell = atoi(argv[1]);
        if (player_free_list.next == &player_free_list)
            out("summon %d: refused, no free Player record (5: the party and one summon)", summon_spell);
        else
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
    } else if (!_stricmp(c, "exitto") && argc >= 3) {
        lstrcpynA(level_name, argv[1], sizeof level_name);
        lstrcpynA(level_script, argv[2], sizeof level_script);
        exit_entry = argc > 3 ? atoi(argv[3]) : 1;
        out("exitto %s %s %d: %s", argv[1], argv[2], exit_entry, run_sync(exitto_body, NULL) ? "requested" : "no window");
    } else if ((!_stricmp(c, "join") || !_stricmp(c, "xpto")) && argc >= 3) {
        lstrcpynA(join_name, argv[1], sizeof join_name);
        join_level = atoi(argv[2]);
        join_xp_only = !_stricmp(c, "xpto");
        out("%s %s %d: %s", c, join_name, join_level, run_sync(join_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "dlg")) {
        // dlg [N]: the open conversation (dialogue_mode 0 closed, 1 text, 3 topics), the last N lines of the text box
        // and the offered topics with the point to click (capture pixels). In text mode "more" sits at the topic box.
        dialogue_box &tb = dialogue_text_box;
        dialogue_topic_list &tp = dialogue_topic_box;
        int want = argc > 1 ? atoi(argv[1]) : 8;
        out("dlg: mode %d, state %d, speaker %s", dialogue_mode, conversation_active.state,
            conversation_active.name ? conversation_active.name : "-");
        for (int i = tb.count > want ? tb.count - want : 0; i < tb.count; i++) {
            const dialogue_line &ln = tb.line[(tb.first + i) % 64];
            if (ln.text)
                out("  line %d: %.300s", ln.kind, ln.text);
        }
        out("  box %d %d (more / end here in text mode), %d topics", tp.x, tp.y, tp.count);
        for (int i = 0; i < tp.count && i < 64; i++) {
            const dialogue_topic_entry &e = tp.entry[i];
            out("  topic %d at %d %d: %.200s", i, tp.x + 20, tp.y + e.top + (int)tp.scroll + e.height / 2,
                e.topic && e.topic->text ? e.topic->text : "-");
        }
    } else if (!_stricmp(c, "items")) {
        // items [R]: the item objects (type 3) and containers (type 11) within R (15) of the current character, for
        // loot that is hard to see on screen; "open ADDR" opens a container.
        Character *me = cur_character;
        float r = argc > 1 ? (float)atof(argv[1]) : 15.0f;
        int shown = 0;
        for (obj *o = obj_active_list.next; me && o && o != &obj_active_list && shown < 40; o = o->next) {
            if (o->type != 3 && o->type != 11)
                continue;
            float dx = o->pos.x - me->pos.x, dy = o->pos.y - me->pos.y, dz = o->pos.z - me->pos.z;
            float d = sqrtf(dx * dx + dy * dy + dz * dz);
            if (d > r)
                continue;
            char names[400];
            int len = 0, n = o->type == 3 ? 1 : ((container *)o)->num_items;
            names[0] = 0;
            for (int i = 0; i < n && i < MAX_CONTAINER_ITEMS; i++) {
                item_obj *it = o->type == 3 ? (item_obj *)o : ((container *)o)->items[i];
                const char *nm = it && it->item && it->item->type ? it->item->type->name : NULL;
                len += _snprintf(names + len, sizeof names - len - 1, "%s%s x%d", i ? ", " : "", nm ? nm : "?",
                                 it ? it->count : 0);
                if (len >= (int)sizeof names - 40)
                    break;
            }
            out("items: %s %p at %.1f (flags %X): %s", o->type == 3 ? "item" : "container", (void *)o, d, o->obj_flags,
                names);
            shown++;
        }
        if (!shown)
            out("items: none within %.1f", r);
    } else if (!_stricmp(c, "party")) {
        // party (2026-11-10): every party member (living_list entries of type 7) with position, layer, HP and flags;
        // '*' marks cur_character. The same line as SummonerFix's agent prints.
        int n = 0;
        for (Character *o = living_list.live_next; o && o != &living_list && n < 16; o = o->live_next) {
            if (o->type != 7)
                continue;
            n++;
            out("party %s%s pos %.2f %.2f %.2f layer %d hp %.0f flags %x", o == cur_character ? "*" : "", o->name,
                (double)o->pos.x, (double)o->pos.y, (double)o->pos.z, o->layer, (double)o->hp, o->flags);
        }
        if (!n)
            out("party: none");
    } else if (!_stricmp(c, "talkto") && argc >= 2) {
        // talkto NAME: the nearest living character whose name starts with NAME ('~' for spaces) is talked to as a
        // click on it does (player_talk_to), also shopkeepers that npcgoto cannot find.
        char want[64];
        lstrcpynA(want, argv[1], sizeof want);
        for (char *q = want; *q; q++)
            if (*q == '~')
                *q = ' ';
        Character *me = cur_character, *best = NULL;
        float bestd = 1e30f;
        int skipped = 0;
        // 2026-11-09: hidden or disabled (CF_SUMMONLIKE), dying or dead characters are skipped (before this, talkto
        // reached Torvel's hidden double, the false bug G312). Not obj_flags bit 0: standing NPCs have it clear.
        for (Character *o = living_list.live_next; me && o && o != &living_list; o = o->live_next) {
            if (_strnicmp(o->name, want, strlen(want)))
                continue;
            if (o->flags & (CF_SUMMONLIKE | CF_HIDDEN | CF_DEAD)) {
                skipped++;
                continue;
            }
            float dx = o->pos.x - me->pos.x, dz = o->pos.z - me->pos.z;
            if (dx * dx + dz * dz < bestd) {
                bestd = dx * dx + dz * dz;
                best = o;
            }
        }
        if (!best) {
            out("talkto %s: no such character (%d hidden or disabled skipped)", want, skipped);
        } else {
            talk_target = best;
            out("talkto %s: %s at %.1f, %s", want, best->name, sqrtf(bestd), run_sync(talkto_body, NULL) ? "requested" : "no window");
        }
    } else if (!_stricmp(c, "open") && argc >= 2) {
        // open ADDR: ADDR is the container's address in this process, as items prints it (below 2 GB in both builds).
        // strtoul, not strtoull: VC6's stdlib.h (the 32-bit clang build's headers) has no strtoull.
        open_target = (container *)(size_t)strtoul(argv[1], NULL, 16);
        out("open %s: %s", argv[1], run_sync(open_body, NULL) ? "requested" : "no window");
    } else if (!_stricmp(c, "npcdead") && argc >= 2) {
        char name[64];
        join_words(name, sizeof name, argv, 1, argc);
        out("npcdead %s: script object %d, dead %d", name, script_object_find(name), (int)script_npc_is_dead(name));
    } else if (!_stricmp(c, "identify") && argc >= 2) {
        // identify NAME ('~' for spaces): clears bit 0 (not identified) of the item type's flags, standing in for the
        // Lore skill (party_count_item_by_name counts identified items only). The test tools poke item_types +0x5c
        // in Sum.exe; in the 64-bit build the record is larger, so the host does it by field (2026-11-09).
        char name[96];
        join_words(name, sizeof name, argv, 1, argc);
        for (char *q = name; *q; q++)
            if (*q == '~')
                *q = ' ';
        int t = item_type_find(name);
        if (t < 0) {
            out("identify %s: no such item type", name);
        } else {
            unsigned before = item_types[t].flags;
            item_types[t].flags &= (ushort)~1u;
            out("identify %s: type %d, flags %X -> %X", name, t, before, item_types[t].flags);
        }
    } else if (!_stricmp(c, "useitem") && argc >= 2) {
        join_words(use_name, sizeof use_name, argv, 1, argc);
        out("useitem %s: %s", use_name, run_sync(useitem_body, NULL) ? "requested" : "no window");
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
