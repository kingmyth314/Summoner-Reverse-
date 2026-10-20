// unnamed_experience_0x489930.cpp - Sum.exe v1.40, code 0x489930-0x489c3f: experience.tbl and the level-up
// table queries. THE FILE NAME IS A PLACEHOLDER: no assert path, no strings.tbl block, module "-" in
// sum.symbols (see NOTES.md; the block may belong to the same file as the containers before it or the
// effect objects after it).
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .data 0x59c248-0x59c25b ("experience.tbl", "D"); .bss 0x2305420-0x2305617 (xp_level_table).
// Float constants 1.0f (0x57e990), 3.0f (0x57ef1c), 10.0f (0x57eb7c) are shared .rdata.

#include <math.h>
#include "vsdk/math/vector.h"
#include "vsdk/parse/parse.h"
#include "engine/characterinfo/characterinfo.h"
#include "summoner/summoner.h"

xp_level xp_level_table[NUM_XP_LEVELS];               // 0x2305420

// 0x489930  Confidence: exact. GameInit. The number that starts each line (the level) is read and dropped.
void experience_parse_table()
{
    TextFile tf;
    xp_level *lv;
    int i;

    tf.Open("experience.tbl", path_tables);
    for (i = NUM_XP_LEVELS, lv = xp_level_table; i != 0; i--, lv++) {
        tf.ParseInt();
        experience_parse_level(&tf, lv);
    }
    tf.Close();
}

// 0x4899d0  Confidence: exact (VC6 byte match, m-1019 w1, D10000). One line after the level number: xp, skill points, skill cap, then per
// species group the HP+ and AP+ gains.
void experience_parse_level(TextFile *tf, xp_level *level)
{
    int i;

    level->xp = tf->ParseInt();
    level->skill_points = (ubyte)tf->ParseInt();
    level->skill_cap = (ubyte)tf->ParseInt();
    xp_level_gain *g = level->groups;
    for (i = 0; i < 4; i++, g++) {
        experience_parse_stat_gain(tf, &g->hp, &g->hp_random);
        experience_parse_stat_gain(tf, &g->ap, &g->ap_random);
    }
}

// 0x489a30  Confidence: exact. An optional "D" (random gain) before the value.
void experience_parse_stat_gain(TextFile *tf, ubyte *value, ubyte *random)
{
    *random = tf->Accept("D");
    *value = (ubyte)tf->ParseInt();
}

// 0x489a60  Confidence: likely (the loop form; the second level test is the loop's entry check). Below 20
// the table; from 20 on the level 19 value plus 50000, 60000, ... per level. Five times as much in a
// network game.
int xp_for_level(int level)
{
    int xp;
    int step;
    int i;

    if (level < 20) {
        xp = xp_level_table[level].xp;
    } else {
        xp = xp_level_table[19].xp;
        step = 50000;
        for (i = 19; i < level; i++) {
            xp += step;
            step += 10000;
        }
    }
    if (game_flags & 2)
        xp *= 5;
    return xp;
}

// 0x489aa0  Confidence: exact.
bool xp_level_reached(int level, int xp)
{
    return xp >= xp_for_level(level + 1);
}

// 0x489ac0  Confidence: likely (the clamp of the random gains is written as a comparison with 3.0f; the
// level index, not the level, is capped at 20). The level-up: level + 1, the new level's skill points and
// cap, and the species group's HP / AP gains (random ones at least 3).
void xp_level_up_gains(int species, short *level, float *max_hp, short *max_ap, int *points, int *cap)
{
    int i;
    xp_level *lv;
    float gain;

    i = ++*level;
    if (i > 20)
        i = 20;
    lv = &xp_level_table[i];
    *points = lv->skill_points;
    *cap = lv->skill_cap;
    if (lv->groups[species].hp_random) {
        gain = (float)(int)floor(frand() * lv->groups[species].hp + 1.0f);
        if (3.0f > gain)
            gain = 3.0f;
    } else {
        gain = (float)lv->groups[species].hp;
    }
    *max_hp += gain;
    if (lv->groups[species].ap_random) {
        gain = (float)(int)floor(frand() * lv->groups[species].ap + 1.0f);
        if (3.0f > gain)
            gain = 3.0f;
        *max_ap = (short)(*max_ap + gain);
    } else {
        *max_ap += lv->groups[species].ap;
    }
}

// 0x489bf0  Confidence: exact. 1.0 at level 0 and below, 10.0 above 20.
float xp_level_skill_cap(int level)
{
    if (level <= 0)
        return 1.0f;
    if (level > 20)
        return 10.0f;
    return (float)xp_level_table[level].skill_cap;
}

// 0x489c20  Confidence: exact.
int xp_max_level()
{
    return (game_flags & 2) ? 75 : 50;
}
