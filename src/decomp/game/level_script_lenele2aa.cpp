// level_script_lenele2aa.cpp - Sum.exe v1.40, code 0x4f12c0-0x4f1481 (levelscripts\Scripts\
// level_script_lenele2aa.cpp; strings.tbl ids 501-502 are this unit's). Written from the Sum.exe
// disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function:
// exact (control flow, calls, constants and globals match), likely (one detail of statement order or
// expression form is a guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "lenele2aa" (occupied Lenele, stage 8: Joseph alone, Kothudin and the
// Orenian soldier, Sir Horum and the Lord of Logaros). The bare RET at 0x4f13e0 (empty_function, the one
// copy every empty function of the game was folded into) lies inside this unit (D2701).
// No static initialiser, no .bss, no .data of its own (the literals only).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/localize/localize.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

void lsf_lenele2aa_flag_victim_dead();
void lsf_lenele2aa_flag_vengence_done();

// 0x4f12c0  Confidence: exact. level_script_table init (stage 8). The bridge (lenele1b) and the gate
// (lenele2d) are closed; Rosalind and Jekhar leave the party.
void ls_lenele2aa_init()
{
    level_music = music_find_by_name("lenele2");
    game_stage_set(8);
    level_trigger_set_script_use("lenele1b", NULL);
    level_trigger_set_script_use("lenele2d", NULL);
    party_remove_member("Rosalind");
    party_remove_member("Jekhar");
}

// 0x4f1310  Confidence: exact (ends in a tail jump to lsf_lenele2aa_flag_vengence_done).
void ls_lenele2aa_frame()
{
    lsf_lenele2aa_flag_victim_dead();
    lsf_lenele2aa_flag_vengence_done();
}

// 0x4f1320  Confidence: exact. The soldier dead: Kothudin to label 1.
void lsf_lenele2aa_flag_victim_dead()
{
    if (!savegame_state.get_flag(cur_level_index, "lenele2aa_flag_victim_dead")
        && script_npc_is_dead("Orenian Soldier#victim")) {
        savegame_state.set_flag(cur_level_index, "lenele2aa_flag_victim_dead", true);
        script_npc_goto_label("Kothudin", 1);
    }
}

// 0x4f1380  Confidence: exact. Sir Horum dead: the Lord of Logaros's "avenged" topic.
void lsf_lenele2aa_flag_vengence_done()
{
    if (!savegame_state.get_flag(cur_level_index, "lenele2aa_flag_vengence_done")
        && script_npc_is_dead("Sir Horum")) {
        savegame_state.set_flag(cur_level_index, "lenele2aa_flag_vengence_done", true);
        conversation_db.set_topic_available("Lord of Logaros", "\"You have been avenged!\"", true);
    }
}

// 0x4f13e0  Confidence: likely (that this unit's copy is the one the linker kept: the address lies between
// lsf_lenele2aa_flag_vengence_done and ls_lenele2aa_action; which empty function of the unit it was, most
// likely lenele2aa's leave hook, is not visible, D2701). The bare RET of every folded empty function.
void empty_function()
{
}

// 0x4f13f0  Confidence: exact. level_script_table action.
void ls_lenele2aa_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "psycho_chat_done"))
        script_npc_goto_label("Kothudin", 2);
}

// 0x4f1420  Confidence: exact. level_script_table use.
char ls_lenele2aa_use(const char *name)
{
    if (!_stricmp(name, "lenele1b")) {
        script_party_member_say("Joseph", XSTR("Too many soldiers guard the bridge. We can't cross here.", 501), 5.0f);
        return 0;
    }
    if (!_stricmp(name, "lenele2d"))
        script_party_member_say("Joseph", XSTR("Too many soldiers guard the gate. We can't go that way.", 502), 5.0f);
    return 0;
}
