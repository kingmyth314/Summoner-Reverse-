// level_script_wolong2.cpp - Sum.exe v1.40, code 0x4f1950-0x4f224d (levelscripts\Scripts\
// level_script_wolong2.cpp; strings.tbl ids 561-563 are this unit's). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// Wolong under siege ("wolong2"): the Jade Temple soldiers recruit the party, the four street fights
// against the Orenians, the lieutenant's reinforcements, Pijian (PIJN-Intro / PIJN-Death, then
// Truth-About-Aoqi), Jekhar joining with the Ring of Wind; Caishi's statue. "wolong2_v2" (after the
// caverns) only opens the caverns exit and lets Haidi go. No static initialiser.
// Data: .bss 0x257e9f4.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/localize/localize.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"
#include "levelscripts/script_internal.h"

#define LEVEL_WOLONG 0                               // "die_haidi_die" (0x4f2119)

void lsf_wolong2_fight1_won();
void lsf_wolong2_fight2_won();
void lsf_wolong2_fight3_won();
void lsf_wolong2_fight4_won();
void ls_wolong2_reinforcements();
void ls_wolong2_force_recruit();
void lsf_wolong2_pijian_intro();
void ls_wolong2_pijian_death_cs_end();
void ls_wolong2_pijian_intro_cs_end();
void ls_wolong2_force_alert();
void ls_wolong2_force_alert_action(dialogue_action *action);

// ---- .bss (0x257e9f4) ------------------------------------------------------------------------------------
bool ls_wolong2_alert_pending;                       // 0x257e9f4

// 0x4f1950  Confidence: exact. Fight 1 won: the lieutenant to label 2, soldiers 2 and 3 to label 1.
void lsf_wolong2_fight1_won()
{
    if (!savegame_state.get_flag(cur_level_index, "wolong2_fight1_won") && script_npc_is_dead("Orenian Soldier#1")
        && script_npc_is_dead("Orenian Soldier#2") && script_npc_is_dead("Orenian Soldier#3")
        && script_npc_is_dead("Orenian Soldier#4")) {
        savegame_state.set_flag(cur_level_index, "wolong2_fight1_won", true);
        script_npc_goto_label("Jade Temple Soldier#LT", 2);
        script_npc_goto_label("Jade Temple Soldier#2", 1);
        script_npc_goto_label("Jade Temple Soldier#3", 1);
    }
}

// 0x4f1a00  Confidence: exact.
void lsf_wolong2_fight2_won()
{
    if (!savegame_state.get_flag(cur_level_index, "wolong2_fight2_won") && script_npc_is_dead("Orenian Samurai#1")
        && script_npc_is_dead("Orenian Soldier#5") && script_npc_is_dead("Orenian Archer#1")
        && script_npc_is_dead("Orenian Soldier#6")) {
        savegame_state.set_flag(cur_level_index, "wolong2_fight2_won", true);
        script_npc_goto_label("Jade Temple Soldier#LT", 3);
        script_npc_goto_label("Jade Temple Soldier#2", 2);
        script_npc_goto_label("Jade Temple Soldier#3", 2);
    }
}

// 0x4f1ab0  Confidence: exact.
void lsf_wolong2_fight3_won()
{
    if (!savegame_state.get_flag(cur_level_index, "wolong2_fight3_won")
        && script_npc_is_dead("Orenian Soldier#$npc038") && script_npc_is_dead("Orenian Soldier#$npc037")
        && script_npc_is_dead("Orenian Soldier#$npc036") && script_npc_is_dead("Orenian Soldier#$npc035")) {
        savegame_state.set_flag(cur_level_index, "wolong2_fight3_won", true);
        script_npc_goto_label("Jade Temple Soldier#LT", 4);
        script_npc_goto_label("Jade Temple Soldier#2", 3);
        script_npc_goto_label("Jade Temple Soldier#3", 3);
    }
}

// 0x4f1b60  Confidence: exact.
void lsf_wolong2_fight4_won()
{
    if (!savegame_state.get_flag(cur_level_index, "wolong2_fight4_won")
        && script_npc_is_dead("Orenian Soldier#$npc039") && script_npc_is_dead("Orenian Soldier#$npc040")
        && script_npc_is_dead("Orenian Soldier#$npc041")) {
        savegame_state.set_flag(cur_level_index, "wolong2_fight4_won", true);
        script_npc_goto_label("Jade Temple Soldier#LT", 5);
        script_npc_goto_label("Jade Temple Soldier#2", 4);
        script_npc_goto_label("Jade Temple Soldier#3", 4);
    }
}

// 0x4f1bf0  Confidence: exact. The lieutenant and soldiers 2 and 3 dead: soldiers 4-6 come.
void ls_wolong2_reinforcements()
{
    if (!savegame_state.get_flag(cur_level_index, "wolong2_reinforce") && script_npc_is_dead("Jade Temple Soldier#LT")
        && script_npc_is_dead("Jade Temple Soldier#2") && script_npc_is_dead("Jade Temple Soldier#3")) {
        savegame_state.set_flag(cur_level_index, "wolong2_reinforce", true);
        script_npc_goto_label("Jade Temple Soldier#4", 1);
        script_npc_goto_label("Jade Temple Soldier#5", 1);
        script_npc_goto_label("Jade Temple Soldier#6", 1);
    }
}

// 0x4f1c80  Confidence: exact. The lieutenant within 4 of $npc016: the priest talks to the party.
void ls_wolong2_force_recruit()
{
    if (!savegame_state.get_flag(cur_level_index, "wolong2_force_recruit") && conversation_active.is_idle()
        && script_check_navpoint(script_object_find("Jade Temple Soldier#LT"), "$npc016", 4.0f)) {
        savegame_state.set_flag(cur_level_index, "wolong2_force_recruit", true);
        script_npc_talk("Jade Temple Priest#leader");
        level_script_event_active = true;
    }
}

// 0x4f1d00  Confidence: exact. Pijian within 15: PIJN-Intro. Pijian "dead": brought back to life for
// PIJN-Death.
void lsf_wolong2_pijian_intro()
{
    if (!savegame_state.get_flag(cur_level_index, "wolong2_pijian")) {
        if (!savegame_state.get_flag(cur_level_index, "wolong2_pijian_intro")
            && script_npc_near_player("Pijian", 15.0f)) {
            savegame_state.set_flag(cur_level_index, "wolong2_pijian_intro", true);
            script_play_cutscene("PIJN-Intro", ls_wolong2_pijian_intro_cs_end);
        }
        if (script_npc_is_dead("Pijian")) {
            script_object_find_character("Pijian")->resurrect();
            script_play_cutscene("PIJN-Death", ls_wolong2_pijian_death_cs_end);
            savegame_state.set_flag(cur_level_index, "wolong2_pijian", true);
        }
    }
}

// 0x4f1dd0  Confidence: exact. End of PIJN-Death: Truth-About-Aoqi (the level music becomes "wolong" without
// being played).
void ls_wolong2_pijian_death_cs_end()
{
    script_npc_set_disabled("Pijian", true);
    script_npc_goto_label("Jekhar#npc", 1);
    script_npc_goto_label("Aoqi#npc", 1);
    level_script_event_active = false;
    level_music = music_find_by_name("wolong");
    script_play_cutscene("Truth-About-Aoqi", NULL);
}

// 0x4f1e20  Confidence: exact. End of PIJN-Intro.
void ls_wolong2_pijian_intro_cs_end()
{
    int music = music_find_by_name("Boss");
    music_play(music, true);
}

// 0x4f1e40  Confidence: exact. $npc004 within 4: the alert soldier talks.
void ls_wolong2_force_alert()
{
    if (!savegame_state.get_flag(cur_level_index, "wolong2_force_alert") && conversation_active.is_idle()
        && script_player_near_navpoint("$npc004", 4.0f)) {
        savegame_state.set_flag(cur_level_index, "wolong2_force_alert", true);
        script_npc_talk("Jade Temple Soldier#alert");
    }
}

// 0x4f1eb0  Confidence: exact. The conversation actions. Charge sets wolong2_charge to false (see NOTES);
// JekharJoin gives the Ring of Wind's summon and opens the world map exit; CheckStatue offers Caishi's
// "Your statue?" until all five parts are back, then "How is the statue coming?".
void ls_wolong2_force_alert_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "FollowMe")) {
        savegame_state.set_flag(cur_level_index, "wolong2_force_alert", true);
        script_npc_goto_label("Jade Temple Soldier#alert", 1);
        return;
    }
    if (!_stricmp(action->name, "Charge")) {
        savegame_state.set_flag(cur_level_index, "wolong2_charge", false);
        return;
    }
    if (!_stricmp(action->name, "JekharJoin")) {
        savegame_state.set_flag(cur_level_index, "got_ring_of_wind", true);
        hud_show_message(XSTR("Joseph acquired Summon Celestial Samurai!", 561));
        ls_wolong2_alert_pending = true;
        level_trigger_set_exit("worldmap1", NULL);
        return;
    }
    if (!_stricmp(action->name, "CheckStatue")) {
        if (savegame_state.get_flag(cur_level_index, "return_head")
            && savegame_state.get_flag(cur_level_index, "return_arm1")
            && savegame_state.get_flag(cur_level_index, "return_arm2")
            && savegame_state.get_flag(cur_level_index, "return_leg1")
            && savegame_state.get_flag(cur_level_index, "return_leg2")) {
            conversation_db.set_topic_available("Caishi the Sculptor", "\"Your statue?\"", false);
            conversation_db.set_topic_available("Caishi the Sculptor", "\"How is the statue coming?\"", true);
        } else {
            conversation_db.set_topic_available("Caishi the Sculptor", "\"Your statue?\"", true);
            conversation_db.set_topic_available("Caishi the Sculptor", "\"How is the statue coming?\"", false);
        }
    }
}

// 0x4f2060  Confidence: exact. level_script_table init. wolong2: stage 12; before stage 13 Jekhar is not in
// the party and both exits are script-use triggers. wolong2_v2: stage 13.
void ls_wolong2_init()
{
    level_music = music_find_by_name("wolong");
    ls_wolong2_alert_pending = false;
    if (!_stricmp(level_script_name, "wolong2")) {
        savegame_state.set_flag(cur_level_index, "wolong2_charge", true);
        game_stage_set(0xc);
        if (game_stage_get() < 0xd) {
            party_remove_member("Jekhar");
            level_trigger_set_script_use("worldmap1", NULL);
            level_trigger_set_script_use("wolongcaverns", NULL);
            return;
        }
        script_npc_set_disabled("Jekhar#npc", true);
        level_trigger_set_exit("worldmap1", NULL);
        return;
    }
    game_stage_set(0xd);
    level_trigger_set_exit("wolongcaverns", NULL);
    if (savegame_state.get_flag(LEVEL_WOLONG, "die_haidi_die"))
        script_npc_goto_label("Haidi", 1);
}

// 0x4f2140  Confidence: likely (party_add_member's result is used unchecked). level_script_table frame
// (wolong2 only). After JekharJoin and the conversation, Jekhar joins and is placed at $npc093.
void ls_wolong2_frame()
{
    Character *c;

    if (!_stricmp(level_script_name, "wolong2")) {
        ls_wolong2_force_recruit();
        ls_wolong2_force_alert();
        lsf_wolong2_fight1_won();
        lsf_wolong2_fight2_won();
        lsf_wolong2_fight3_won();
        lsf_wolong2_pijian_intro();
        lsf_wolong2_fight4_won();
        ls_wolong2_reinforcements();
        if (ls_wolong2_alert_pending && !(game_flags & GF_CONVERSATION)) {
            script_npc_set_disabled("Jekhar#npc", true);
            c = living_from_handle(party_add_member("Jekhar")->handle);
            level_entry_point = 2;
            c->place_at_marker(navpoint_find("$npc093"));
            ls_wolong2_alert_pending = false;
        }
    }
}

// 0x4f21e0  Confidence: exact. level_script_table action.
void ls_wolong2_action(dialogue_action *action)
{
    ls_wolong2_force_alert_action(action);
}

// 0x4f21f0  Confidence: exact. level_script_table use.
char ls_wolong2_use(const char *name)
{
    if (!_stricmp(name, "wolongcaverns"))
        script_party_member_say("Joseph", XSTR("We're not going back in there. Not yet.", 562), 5.0f);
    if (!_stricmp(name, "worldmap1"))
        script_party_member_say("Joseph", XSTR("No boats are leaving Wolong now. We'll have to wait.", 563), 5.0f);
    return 0;
}
