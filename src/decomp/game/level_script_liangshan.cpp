// level_script_liangshan.cpp - Sum.exe v1.40, code 0x4f4930-0x4f5661 (levelscripts\Scripts\
// level_script_liangshan.cpp; strings.tbl ids 505-517 are this unit's). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// Liangshan forest: the Serpent and Phoenix Riders (SERP-PHNX-Intro at $sptrigger), the poison marsh, the
// Guardian of the Shrine and the defence of the shrine against General Wentao's four waves, the Ring of
// Forest, and the party's remarks. No static initialiser.
// Data: .bss 0x257ea60-0x257ea61.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/localize/localize.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/boss_ai.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

#define LEVEL_WOLONGCAVERNS 0x10                     // "wc_piaobo_human" (0x4f49fc)

void lsf_liangshan_flag_pit_comment();
void lsf_liangshan_flag_stone_comment();
void lsf_liangshan_flag_village_comment();
void lsf_liangshan_flag_lair_comment();
void lsf_liangshan_flag_shrine_comment();
void lsf_liangshan_flag_grave_comment();
void lsf_liangshan_flag_noise_comment();
void lsf_liangshan_flag_bones_comment();
void lsf_liangshan_flag_wave2();
void lsf_liangshan_flag_wave3();
void lsf_liangshan_flag_wave4();
void lsf_liangshan_flag_finished_quest();
void ls_liangshan_poison_check();
void ls_liangshan_serp_phnx_intro_cs_end();
void lsf_li_intro();
void ls_liangshan_start_wave1();

// ---- .bss (0x257ea60-0x257ea61) ------------------------------------------------------------------------
bool ls_liangshan_phoenix_serpent_done;              // 0x257ea60
bool ls_liangshan_phoenix_dead_done;                 // 0x257ea61

// 0x4f4930  Confidence: exact (the phoenix_serpent preset as if / else). level_script_table init (stage
// 13). ls_liangshan_phoenix_dead_done is set on every entry, so the level music never comes back after the
// two riders (see NOTES).
void ls_liangshan_init()
{
    level_music = music_find_by_name("Liangshan");
    game_stage_set(0xd);
    serpent_set_solo(script_object_find_character("Serpent Rider#1"));
    phoenix_set_solo(script_object_find_character("Phoenix Rider#1"));
    if (savegame_state.get_flag(cur_level_index, "phoenix_serpent")) {
        script_npc_set_disabled("Serpent Rider#1", true);
        script_npc_set_disabled("Phoenix Rider#1", true);
        ls_liangshan_phoenix_serpent_done = true;
    } else {
        ls_liangshan_phoenix_serpent_done = false;
    }
    ls_liangshan_phoenix_dead_done = true;
    if (savegame_state.get_flag(cur_level_index, "got_ring_of_forest")
        && savegame_state.get_flag(cur_level_index, "liangshan_flag_give_sword"))
        script_npc_set_disabled("Guardian of the Shrine", true);
    if (savegame_state.get_flag(LEVEL_WOLONGCAVERNS, "wc_piaobo_human"))
        script_npc_set_disabled("Piaobo#oni", true);
}

// 0x4f4a20  Confidence: exact (VC6 byte match, match14; was exact). level_script_table frame. The riders (boss music, the intro, their deaths),
// the wounded soldier and the guardian talk by themselves when the player comes near, the marsh poison is
// renewed on every party member, and the guardian's death with the ring in the party ends the quest.
void ls_liangshan_frame()
{
    Player *p;
    Character *c;

    lsf_li_intro();
    lsf_liangshan_flag_lair_comment();
    lsf_liangshan_flag_shrine_comment();
    lsf_liangshan_flag_bones_comment();
    lsf_liangshan_flag_grave_comment();
    lsf_liangshan_flag_noise_comment();
    lsf_liangshan_flag_pit_comment();
    lsf_liangshan_flag_stone_comment();
    lsf_liangshan_flag_village_comment();
    lsf_liangshan_flag_wave2();
    lsf_liangshan_flag_wave3();
    lsf_liangshan_flag_wave4();
    lsf_liangshan_flag_finished_quest();
    ls_liangshan_poison_check();
    if (!ls_liangshan_phoenix_serpent_done && savegame_state.get_flag(cur_level_index, "phoenix_serpent")
        && !(game_flags & GF_CUTSCENE)) {
        ls_liangshan_phoenix_serpent_done = true;
        level_script_event_active = true;
        int music = music_find_by_name("Boss");
        music_play(music, true);
    }
    if (!ls_liangshan_phoenix_dead_done && savegame_state.get_flag(cur_level_index, "phoenix_dead")
        && savegame_state.get_flag(cur_level_index, "serpent_dead") && !(game_flags & GF_CUTSCENE)) {
        ls_liangshan_phoenix_dead_done = true;
        music_play(level_music, true);
    }
    if (!savegame_state.get_flag(cur_level_index, "phoenix_serpent")) {
        if (script_player_near_navpoint("$sptrigger", 5.0f)) {
            script_play_cutscene("SERP-PHNX-Intro", ls_liangshan_serp_phnx_intro_cs_end);
            savegame_state.set_flag(cur_level_index, "phoenix_serpent", true);
        }
    } else {
        if (!savegame_state.get_flag(cur_level_index, "phoenix_dead") && script_npc_is_dead("Phoenix Rider#1")) {
            script_npc_goto_label("Phoenix Rider#1", 2);
            savegame_state.set_flag(cur_level_index, "phoenix_dead", true);
        }
        if (!savegame_state.get_flag(cur_level_index, "serpent_dead") && script_npc_is_dead("Serpent Rider#1")) {
            script_npc_goto_label("Serpent Rider#1", 2);
            savegame_state.set_flag(cur_level_index, "serpent_dead", true);
        }
        if (savegame_state.get_flag(cur_level_index, "serpent_dead")
            && savegame_state.get_flag(cur_level_index, "phoenix_dead"))
            level_script_event_active = false;
    }
    if (!savegame_state.get_flag(cur_level_index, "soldier_talk") && script_npc_near_player("Wounded Soldier", 8.0f)) {
        script_npc_talk("Wounded Soldier");
        savegame_state.set_flag(cur_level_index, "soldier_talk", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "shrine_talk")
        && script_npc_near_player("Guardian of the Shrine", 5.0f)) {
        script_npc_talk("Guardian of the Shrine");
        savegame_state.set_flag(cur_level_index, "shrine_talk", true);
    }
    if (savegame_state.get_flag(cur_level_index, "poisoned")) {
        for (p = party_list.next; p != &party_list; p = p->next) {
            c = living_from_handle(p->handle);
            if (c && !c->has_status(STATUS_POISON))
                c->status_effect_add(STATUS_POISON, 2400000, -1, -24.0f, -1, true);
        }
    }
    if (!savegame_state.get_flag(cur_level_index, "got_ring_of_forest") && script_npc_is_dead("Guardian of the Shrine")
        && party_count_item_by_name("Ring of Forest")) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_give_sword", true);
        savegame_state.set_flag(cur_level_index, "got_ring_of_forest", true);
        level_trigger_set_exit("worldmap1", NULL);
        hud_show_message(XSTR("Joseph acquired Summon Poison Elemental!", 516));
    }
}

// 0x4f4df0  Confidence: exact.
void lsf_liangshan_flag_pit_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_pit_comment")
        && script_player_near_navpoint("$pit", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_pit_comment", true);
        script_party_member_say("Joseph", XSTR("Looks like a pit trap. I see spikes at the bottom!", 505), 5.0f);
    }
}

// 0x4f4e60  Confidence: exact.
void lsf_liangshan_flag_stone_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_stone_comment")
        && script_player_near_navpoint("$stone", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_stone_comment", true);
        script_party_member_say("Rosalind", XSTR("What is this place? It's so cold here...", 506), 5.0f);
    }
}

// 0x4f4ed0  Confidence: exact.
void lsf_liangshan_flag_village_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_village_comment")
        && script_player_near_navpoint("$village", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_village_comment", true);
        script_party_member_say("Flece", XSTR("We found the old village, but where's the shrine?", 507), 5.0f);
    }
}

// 0x4f4f40  Confidence: exact. Sets its flag and says nothing (no line in the code).
void lsf_liangshan_flag_lair_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_lair_comment")
        && script_player_near_navpoint("$lair", 3.0f))
        savegame_state.set_flag(cur_level_index, "liangshan_flag_lair_comment", true);
}

// 0x4f4f90  Confidence: exact. Only before the guardian has talked.
void lsf_liangshan_flag_shrine_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_shrine_comment")
        && script_player_near_navpoint("$shrine", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_shrine_comment", true);
        if (!savegame_state.get_flag(cur_level_index, "shrine_talk"))
            script_party_member_say("Joseph", XSTR("The shrine of the forest! We made it!", 508), 5.0f);
    }
}

// 0x4f5010  Confidence: exact.
void lsf_liangshan_flag_grave_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_grave_comment")
        && script_player_near_navpoint("$grave", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_grave_comment", true);
        script_party_member_say("Joseph", XSTR("The names are faded from the stones.", 509), 5.0f);
    }
}

// 0x4f5080  Confidence: exact.
void lsf_liangshan_flag_noise_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_noise_comment")
        && script_player_near_navpoint("$noise", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_noise_comment", true);
        script_party_member_say("Rosalind", XSTR("What was that? Did you hear something?", 510), 5.0f);
    }
}

// 0x4f50f0  Confidence: exact.
void lsf_liangshan_flag_bones_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_bones_comment")
        && script_player_near_navpoint("$bones", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_bones_comment", true);
        script_party_member_say("Flece", XSTR("By the sword of Urath, look at those bones!", 511), 5.0f);
    }
}

// 0x4f5160  Confidence: exact. Wave 1 (one soldier) dead: three soldiers.
void lsf_liangshan_flag_wave2()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_wave2") && script_npc_is_dead("Orenian Soldier#w1")) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_wave2", true);
        script_npc_send_go("Orenian Soldier#w2a");
        script_npc_send_go("Orenian Soldier#w2b");
        script_npc_send_go("Orenian Soldier#w2c");
        script_party_member_say("Joseph", XSTR("I see three more soldiers coming!", 512), 5.0f);
    }
}

// 0x4f51e0  Confidence: exact. Wave 2 dead: two samurai and a mage.
void lsf_liangshan_flag_wave3()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_wave3") && script_npc_is_dead("Orenian Soldier#w2a")
        && script_npc_is_dead("Orenian Soldier#w2b") && script_npc_is_dead("Orenian Soldier#w2c")) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_wave3", true);
        script_npc_send_go("Orenian Samurai#w3a");
        script_npc_send_go("Orenian Samurai#w3b");
        script_npc_send_go("Orenian Mage#w3");
        script_party_member_say("Rosalind", XSTR("Those are scouts. The army won't be far behind!", 513), 5.0f);
    }
}

// 0x4f5280  Confidence: exact. Wave 3 "dead": only Samurai#w3b and the mage are tested, not Samurai#w3a.
void lsf_liangshan_flag_wave4()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_wave4") && script_npc_is_dead("Orenian Samurai#w3b")
        && script_npc_is_dead("Orenian Mage#w3")) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_wave4", true);
        script_npc_send_go("Orenian Greater Mage#w4");
        script_npc_send_go("Orenian Soldier#w4a");
        script_npc_send_go("Orenian Soldier#w4b");
        script_npc_send_go("General Wentao#w4");
        script_party_member_say("Flece", XSTR("Here comes Wentao and his soldiers!", 514), 5.0f);
    }
}

// 0x4f5320  Confidence: exact. Wave 4 dead: the guardian's label 2 and his "Wentao has been defeated." topic.
void lsf_liangshan_flag_finished_quest()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_finished_quest")
        && script_npc_is_dead("Orenian Greater Mage#w4") && script_npc_is_dead("General Wentao#w4")
        && script_npc_is_dead("Orenian Soldier#w4a") && script_npc_is_dead("Orenian Soldier#w4b")) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_finished_quest", true);
        script_npc_goto_label("Guardian of the Shrine", 2);
        conversation_db.set_topic_available("Guardian of the Shrine", "\"Wentao has been defeated.\"", true);
        script_party_member_say("Joseph", XSTR("That must be the last of them.", 515), 5.0f);
    }
}

// 0x4f53e0  Confidence: exact. Any of the five marsh navpoints within 13: poisoned (cured by the
// CurePoison action).
void ls_liangshan_poison_check()
{
    if (!savegame_state.get_flag(cur_level_index, "poisoned")
        && (script_player_near_navpoint("$poison1", 13.0f) || script_player_near_navpoint("$poison2", 13.0f)
            || script_player_near_navpoint("$poison3", 13.0f) || script_player_near_navpoint("$poison4", 13.0f)
            || script_player_near_navpoint("$poison5", 13.0f)))
        savegame_state.set_flag(cur_level_index, "poisoned", true);
}

// 0x4f5490  Confidence: exact. End of SERP-PHNX-Intro.
void ls_liangshan_serp_phnx_intro_cs_end()
{
    script_npc_send_go("Serpent Rider#1");
    script_npc_send_go("Phoenix Rider#1");
}

// 0x4f54b0  Confidence: exact. Liangshan-Narrative on the first entry.
void lsf_li_intro()
{
    if (!savegame_state.get_flag(cur_level_index, "li_intro")) {
        script_play_cutscene("Liangshan-Narrative", NULL);
        savegame_state.set_flag(cur_level_index, "li_intro", true);
    }
}

// 0x4f54f0  Confidence: exact. level_script_table action. EnemyAttack starts the defence (the world map exit
// becomes a script-use trigger), CurePoison cures every party member (spell 1, source type 7), GotRing
// opens the exit again, OniHostile turns Piaobo.
void ls_liangshan_action(dialogue_action *action)
{
    Player *p;
    Character *c;

    if (!_stricmp(action->name, "EnemyAttack")) {
        level_trigger_set_script_use("worldmap1", NULL);
        ls_liangshan_start_wave1();
        script_npc_goto_label("Guardian of the Shrine", 1);
    }
    if (!_stricmp(action->name, "CurePoison")) {
        savegame_state.set_flag(cur_level_index, "poisoned", false);
        for (p = party_list.next; p != &party_list; p = p->next) {
            c = living_from_handle(p->handle);
            if (c)
                c->get_hit_by_spell(1, p->handle, NULL, NULL, 7, NULL, 0, false);
        }
    }
    if (!_stricmp(action->name, "GotRing")) {
        level_trigger_set_exit("worldmap1", NULL);
        hud_show_message(XSTR("Joseph acquired Summon Poison Elemental!", 516));
    }
    if (!_stricmp(action->name, "OniHostile"))
        script_npc_goto_label("Piaobo#Oni", 1);
}

// 0x4f55f0  Confidence: exact. The first wave: one soldier.
void ls_liangshan_start_wave1()
{
    if (!savegame_state.get_flag(cur_level_index, "liangshan_flag_wave1")) {
        savegame_state.set_flag(cur_level_index, "liangshan_flag_wave1", true);
        script_npc_send_go("Orenian Soldier#w1");
    }
}

// 0x4f5630  Confidence: exact. level_script_table use.
char ls_liangshan_use(const char *name)
{
    if (!_stricmp(name, "worldmap1"))
        script_party_member_say("Joseph", XSTR("We need to defend the shrine!", 517), 5.0f);
    return 0;
}
