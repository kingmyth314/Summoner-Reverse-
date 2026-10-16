// level_script_lenele2d.cpp - Sum.exe v1.40, code 0x4ee730-0x4ee831. Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "lenele2d" (index 0x31; Lenele's gate district at stage 7, Pedrog the guide).
// FILE NAME NOT PROVEN (D5300): strings.tbl has no "// level_script_lenele2d.cpp" block and no path string
// names the file; the name follows the level_script_<level>.cpp convention of the tagged units. The
// refusal text reuses id 502 of the lenele2aa block (the same English line), so the id proves nothing.
// No static initialiser, no .bss, no .data of its own (the literals 0x5a1918-0x5a1937 only).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h.

#include <string.h>
#include "vsdk/localize/localize.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

// 0x4ee730  Confidence: exact. level_script_table init (stage 7): the lenele2aa exit is a script use.
void ls_lenele2d_init()
{
    level_music = music_find_by_name("lenele2");
    game_stage_set(7);
    level_trigger_set_script_use("lenele2aa", NULL);
}

// 0x4ee760  Confidence: exact. Pedrog talks to the party once, when it comes within 6 m and no
// conversation runs.
void ls_lenele2d_frame()
{
    if (!savegame_state.get_flag(cur_level_index, "pedrog_talk") && conversation_active.is_idle()
        && script_npc_near_player("Pedrog", 6.0f)) {
        script_npc_talk("Pedrog");
        savegame_state.set_flag(cur_level_index, "pedrog_talk", true);
    }
}

// 0x4ee7d0  Confidence: exact. level_script_table action: "PedrogLead" sends Pedrog on (his "go" action).
void ls_lenele2d_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "PedrogLead"))
        script_npc_send_go("Pedrog");
}

// 0x4ee800  Confidence: exact. level_script_table use: the lenele2aa gate is refused.
char ls_lenele2d_use(const char *name)
{
    if (!_stricmp("lenele2aa", name))
        script_party_member_say("Joseph", XSTR("Too many soldiers guard the gate. We can't go that way.", 502), 5.0f);
    return 0;
}
