// [Cheats] in the source (2026-11-03, at the owner's request; SummonerFix's cheats.c for the rebuilt exes). Off by
// default, in every rebuilt build that has the experimental folder (fixed and experimental):
//   XpMultiplier=1      Character::add_xp multiplies every award (the .fixes key XpMultiplier: SummonerFix's hook
//                       stands down). Read at start.
//   GoldKey=, GoldAmount=1000, SkillPointKey=, SkillPointAmount=1
//                       keys that add party gold, or skill points to the selected character's Player record, in
//                       play. The 32-bit rebuilt exes get them from SummonerFix (its address table reaches the same
//                       data); the in-source host of the clang builds calls exp_cheat_key from its key handler.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "experimental/experimental.h"

#ifdef FIXED_BUILD
#include <windows.h>
#include "../../summonerfix/keynames.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "summoner/player/player.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/objects.h"

static float cheat_xp_mult = 1.0f;
static int cheat_gold_scan, cheat_gold_amount = 1000, cheat_skill_scan, cheat_skill_amount = 1;

static int cheat_clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static int cheat_key_from_ini(const char *key)
{
    char name[32];

    exp_ini_string("Cheats", key, "", name, sizeof(name));
    if (!name[0] || !_stricmp(name, "none"))
        return 0;
    int scan = KeyParse(name);
    if (scan <= 0 || scan > 0xff) {
        exp_log("cheats: [Cheats] %s=%s is not a key name", key, name);
        return 0;
    }
    return scan;
}

void exp_cheats_load_keys()
{
    cheat_gold_scan = cheat_key_from_ini("GoldKey");
    cheat_skill_scan = cheat_key_from_ini("SkillPointKey");
    cheat_gold_amount = cheat_clamp(exp_ini_int("Cheats", "GoldAmount", 1000), 1, 1000000);
    cheat_skill_amount = cheat_clamp(exp_ini_int("Cheats", "SkillPointAmount", 1), 1, 100);
}

void exp_cheats_init()
{
    char v[32];

    exp_ini_string("Cheats", "XpMultiplier", "1", v, sizeof(v));
    float m = (float)atof(v);
    if (m < 0.1f || m > 100.0f)
        m = 1.0f;
    cheat_xp_mult = m;
    exp_cheats_load_keys();
    if (m != 1.0f || cheat_gold_scan || cheat_skill_scan)
        exp_log("cheats: experience x%.2f, gold key %s (%d), skill point key %s (%d)", (double)m,
                cheat_gold_scan ? "set" : "none", cheat_gold_amount, cheat_skill_scan ? "set" : "none",
                cheat_skill_amount);
}

int exp_cheat_scale_xp(int xp)
{
    if (cheat_xp_mult == 1.0f)
        return xp;
    float v = (float)xp * cheat_xp_mult;
    return (int)(v >= 0.0f ? v + 0.5f : v - 0.5f);
}

void exp_cheat_key(int scan)
{
    char msg[64];

    if (!scan || (scan != cheat_gold_scan && scan != cheat_skill_scan) || gameseq_state_cur != 4)
        return;
    if (scan == cheat_gold_scan) {
        party_gold += cheat_gold_amount;
        _snprintf(msg, sizeof(msg), "Cheat: +%d gold (%d)", cheat_gold_amount, party_gold);
        hud_show_message(msg);
        exp_log("cheats: +%d gold, now %d", cheat_gold_amount, party_gold);
    }
    if (scan == cheat_skill_scan && cur_character) {
        Player *p = player_from_handle(cur_character->handle);
        if (!p)
            return;
        p->skill_points += cheat_skill_amount;
        _snprintf(msg, sizeof(msg), "Cheat: +%d skill points (%d)", cheat_skill_amount, p->skill_points);
        hud_show_message(msg);
        exp_log("cheats: +%d skill points, now %d", cheat_skill_amount, p->skill_points);
    }
}

#endif // FIXED_BUILD
