// level_script_lenele2ab.cpp - Sum.exe v1.40, code 0x4f1070-0x4f12bf. Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "lenele2ab" (index 0x1f; occupied Lenele, stage 8: Adeslani's escort, Canis
// the Mercenary). FILE NAME NOT PROVEN (D5300): no strings.tbl block, no path string; the name follows the
// level_script_<level>.cpp convention of the tagged units. Its literals 0x5a2204-0x5a231f lie between
// lenele3a's and lenele2aa's (source-map.md). No static initialiser, no .bss.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "engine/objects/objects.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

void lsf_lenele2ab_flag_escort_1();
void lsf_lenele2ab_flag_ask_help();
void lsf_lenele2ab_flag_escort_complete();

// 0x4f1070  Confidence: exact. level_script_table init (stage 8): Jekhar and Rosalind leave the party.
void ls_lenele2ab_init()
{
    game_stage_set(8);
    level_music = music_find_by_name("lenele2");
    party_remove_member("Jekhar");
    party_remove_member("Rosalind");
}

// 0x4f10a0  Confidence: exact (ends in a tail jump to lsf_lenele2ab_flag_escort_complete).
void ls_lenele2ab_frame()
{
    lsf_lenele2ab_flag_escort_1();
    lsf_lenele2ab_flag_ask_help();
    lsf_lenele2ab_flag_escort_complete();
}

// 0x4f10b0  Confidence: exact. After the party agreed to the escort, the first ambush (three soldiers) dead:
// Adeslani to label 2.
void lsf_lenele2ab_flag_escort_1()
{
    if (!savegame_state.get_flag(cur_level_index, "lenele2ab_flag_escort_1")
        && savegame_state.get_flag(cur_level_index, "lenele2ab_flag_escort_agree")
        && script_npc_is_dead("Orenian Soldier#kill1-1")
        && script_npc_is_dead("Orenian Samurai#kill1-3")
        && script_npc_is_dead("Orenian Soldier#kill1-2")) {
        savegame_state.set_flag(cur_level_index, "lenele2ab_flag_escort_1", true);
        script_npc_goto_label("Adeslani", 2);
    }
}

// 0x4f1140  Confidence: exact. The alarm soldier dead: Adeslani to label 3.
void lsf_lenele2ab_flag_ask_help()
{
    if (!savegame_state.get_flag(cur_level_index, "lenele2ab_flag_ask_help")
        && script_npc_is_dead("Orenian Soldier#1alarum17")) {
        savegame_state.set_flag(cur_level_index, "lenele2ab_flag_ask_help", true);
        script_npc_goto_label("Adeslani", 3);
    }
}

// 0x4f11a0  Confidence: exact. Adeslani within 2 m of $adel2-10: the "We made it?" topic, label 4.
void lsf_lenele2ab_flag_escort_complete()
{
    if (!savegame_state.get_flag(cur_level_index, "lenele2ab_flag_escort_complete")
        && script_check_navpoint(script_object_find("Adeslani"), "$adel2-10", 2.0f)) {
        savegame_state.set_flag(cur_level_index, "lenele2ab_flag_escort_complete", true);
        conversation_db.set_topic_available("Adeslani", "\"We made it?\"", true);
        script_npc_goto_label("Adeslani", 4);
    }
}

// 0x4f1220  Confidence: exact. level_script_table action: "payoff" (Canis leaves for 100 gold, never below
// 0), "attack" (Canis attacks), "agree" (the escort starts).
void ls_lenele2ab_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "payoff")) {
        script_npc_goto_label("Canis the Mercenary", 1);
        party_gold -= 100;
        if (party_gold < 0)
            party_gold = 0;
    }
    if (!_stricmp(action->name, "attack"))
        script_npc_goto_label("Canis the Mercenary", 2);
    if (!_stricmp(action->name, "agree")) {
        savegame_state.set_flag(cur_level_index, "lenele2ab_flag_escort_agree", true);
        script_npc_goto_label("Adeslani", 1);
    }
}
