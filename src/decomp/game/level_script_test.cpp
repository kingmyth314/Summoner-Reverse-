// level_script_test.cpp - Sum.exe v1.40, code 0x4fdde0-0x4fde7f. Written from the Sum.exe disassembly
// (tools/asm.mjs; the float 0.5 is the immediate 0x3f000000). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1
// /GX. Confidence per function: exact (control flow, calls, constants and globals match), likely (one detail
// of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "test" (index 2; a developer test map: maximum skills, all summons, all
// items at stage 22, and a debug line every frame). FILE NAME NOT PROVEN (D5300): no strings.tbl block, no
// path string; the name follows the level_script_<level>.cpp convention. A unit of its own (D5301): .bss
// 0x257ed60-0x257ed61, a 4-byte step after masad's 0x257ed5c-0x257ed5d and before catacombs' 0x257ed64.
// The record's use hook, ls_test_use (0x4f29c0), is the folded copy in the tancredhouse run; it is not this
// unit's code.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h.

#include "engine/characterinfo/characterinfo.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"

// ---- .bss (0x257ed60-0x257ed61) ------------------------------------------------------------------------
bool ls_test_saved_max_skills;                       // 0x257ed60
bool ls_test_saved_all_summons;                      // 0x257ed61

// 0x4fdde0  Confidence: likely (the GF_SINGLE_PLAYER test is compiled as `mov al, 1; test al, cl`, which
// keeps the 1 in AL for the stores; the source form that gives this is a guess). level_script_table init:
// single player only, the two console toggles are saved and set, Give_all_items is set (never read), stage 22.
void ls_test_init()
{
    if (game_flags & GF_SINGLE_PLAYER) {
        ls_test_saved_max_skills = Max_skills;
        Max_skills = true;
        ls_test_saved_all_summons = All_summons;
        All_summons = true;
        Give_all_items = true;
        game_stage_set(0x16);
    }
}

// 0x4fde20  Confidence: exact. Every frame "Testing" over James Agay (a developer's name), and a line from
// Joseph while he wields a katana.
void ls_test_frame()
{
    script_party_member_say("James Agay", "Testing", 0.5f);
    if (script_player_has_equipped("Katana"))
        script_party_member_say("Joseph", "I'm wielding a katana.", 0.5f);
}

// 0x4fde60  Confidence: exact. level_script_table leave: the toggles restored (Give_all_items stays set).
void ls_test_leave()
{
    if (game_flags & GF_SINGLE_PLAYER) {
        All_summons = ls_test_saved_all_summons;
        Max_skills = ls_test_saved_max_skills;
    }
}
