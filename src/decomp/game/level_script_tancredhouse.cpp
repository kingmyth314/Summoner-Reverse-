// level_script_tancredhouse.cpp - Sum.exe v1.40, code 0x4f2830-0x4f29c2. Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "tancredhouse" (index 0x19; Tancred's house in Lenele: Yago, Galienne,
// Tancred's end). FILE NAME NOT PROVEN (D5300): no strings.tbl block, no path string, no .bss; its literals
// (0x5a2894-0x5a28e8) follow level_script_lpalaceint02.cpp's (source-map.md). The name follows the
// level_script_<record>.cpp convention. The "vNN" names, "Jekhar", "Rosalind", "Jekhar#npc",
// "Rosalind#npc", "Tancred" and "found_galienne" are pooled copies.
//
// ls_test_use (0x4f29c0) is this unit's use hook: the body `xor al, al; ret` ends the run, and every record
// whose use hook does nothing shares it (identical-code folding; sum.symbols names the folded copy after the
// "test" record, so the definition keeps that name, D5604).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h.

#include <string.h>
#include "engine/gamesound/gamesound.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

// 0x4f2830  Confidence: exact (VC6 byte match; the two game_stage_set calls are one call with the stage pushed
// per branch;
// the stage >= 6, >= 4 and < 4 branches share the "v41" / "v31" tails). level_script_table init: stage 7
// for _v3, else 1; the doors v11..v41 open one by one with the stage; from stage 2 Mahri can be asked about
// the woman in white.
void ls_tancredhouse_init()
{
    level_music = music_find_by_name("tancredHouse");
    if (!_stricmp(level_script_name, "tancredhouse_v3"))
        game_stage_set(7);
    else
        game_stage_set(1);

    if (game_stage_get() >= 7) {
        level_trigger_set_script_use_by_refname("v11");
        level_trigger_set_script_use_by_refname("v21");
        level_trigger_set_script_use_by_refname("v31");
        level_trigger_set_exit_by_refname("v41");
    } else if (game_stage_get() >= 6) {
        level_trigger_set_script_use_by_refname("v11");
        level_trigger_set_script_use_by_refname("v21");
        level_trigger_set_exit_by_refname("v31");
        level_trigger_set_script_use_by_refname("v41");
    } else if (game_stage_get() >= 4) {
        level_trigger_set_script_use_by_refname("v11");
        level_trigger_set_exit_by_refname("v21");
        level_trigger_set_script_use_by_refname("v31");
        level_trigger_set_script_use_by_refname("v41");
    } else {
        level_trigger_set_exit_by_refname("v11");
        level_trigger_set_script_use_by_refname("v21");
        level_trigger_set_script_use_by_refname("v31");
        level_trigger_set_script_use_by_refname("v41");
    }
    if (game_stage_get() >= 2)
        conversation_db.set_topic_available("Mahri the Crooked", "\"A woman in white?\"", true);
}

// 0x4f2930  Confidence: exact (two separate tests; the action kind is not checked). level_script_table
// action: SpokeYago sends Jekhar and Rosalind away (Galienne found), DieTancred sends Tancred to label 1.
void ls_tancredhouse_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "SpokeYago")) {
        party_remove_member("Jekhar");
        party_remove_member("Rosalind");
        savegame_state.set_flag(cur_level_index, "found_galienne", true);
        script_npc_goto_label("Jekhar#npc", 1);
        script_npc_goto_label("Rosalind#npc", 1);
    }
    if (!_stricmp(action->name, "DieTancred"))
        script_npc_goto_label("Tancred", 1);
}

// 0x4f29c0  Confidence: exact. level_script_table use: nothing reacts (D5604). The sum.symbols comment
// ("opens gates, speaks a refusal") does not match this body.
char ls_test_use(const char *name)
{
    return 0;
}
