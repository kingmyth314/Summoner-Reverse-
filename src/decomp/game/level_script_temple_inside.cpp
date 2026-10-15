// level_script_temple_inside.cpp - Sum.exe v1.40, code 0x4f5e90-0x4f6611 (levelscripts\Scripts\
// level_script_temple_inside.cpp; strings.tbl id 558 is this unit's). Written from the Sum.exe
// disassembly (tools/asm.mjs; the float 0x57eb84 = 0.0 read from the image). Target: VC++ 6.0
// (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control flow, calls, constants and
// globals match), likely (one detail of statement order or expression form is a guess), unsure (the shape
// itself is a guess).
//
// One level_script_table record: "TempleInt" (the temple of Urath in Lenele: the Hierophant's window and
// mosaic stories UrathMyth1a-2d; the variant templeint_v2 (stage 8) has the Eavesdropping scene and the
// Tiger Rider, whose escape ends act 2 and sends Flece alone to Eleh). No static initialiser.
// Data: .bss 0x257ea6c-0x257ea76. No .data of its own (the literals only).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <stdio.h>
#include <string.h>
#include "vsdk/localize/localize.h"
#include "vsdk/ca/character_instance.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

#define LEVEL_LPALACEINT   0x18                      // "got_dagger_quest" (0x4f5efc)

void ls_templeint_urathmyth_cs_end();
void ls_templeint_tiger_intro_cs_end();
void ls_templeint_tiger_dismount_cs_end();
void ls_templeint_tiger_escape_cs_end();
void ls_templeint_act2_outro_cs_end();
void ls_templeint_eavesdropping_cs_end();

// ---- .bss (0x257ea6c-0x257ea76) ------------------------------------------------------------------------
bool ls_templeint_tiger_intro_done;                  // 0x257ea6c
bool ls_templeint_tiger_killed_done;                 // 0x257ea6d
// Set by ls_templeint_action (WindowCutscene1-4, MosaicCutscene1-4), played by ls_templeint_frame.
bool ls_templeint_play_urathmyth1a;                  // 0x257ea6e
bool ls_templeint_play_urathmyth1b;                  // 0x257ea6f
bool ls_templeint_play_urathmyth1c;                  // 0x257ea70
bool ls_templeint_play_urathmyth1d;                  // 0x257ea71
bool ls_templeint_play_urathmyth2a;                  // 0x257ea72
bool ls_templeint_play_urathmyth2b;                  // 0x257ea73
bool ls_templeint_play_urathmyth2c;                  // 0x257ea74
bool ls_templeint_play_urathmyth2d;                  // 0x257ea75
bool ls_templeint_eavesdrop_pending;                 // 0x257ea76

// 0x4f5e90  Confidence: likely (the music presets as if / else with the door call after it: the binary has
// the door call at the end of both paths). level_script_table init. templeint_v2: stage 8, the way back to
// lenele1c closed, Jekhar and Rosalind out of the party, Eavesdropping pending. The first visit: stage 0
// and, with the dagger quest of the palace, the Hierophant's Blade of Tarun-Khor topic.
void ls_templeint_init()
{
    level_music = music_find_by_name("TempleInt");
    if (!_stricmp(level_script_name, "templeint_v2")) {
        game_stage_set(8);
        level_trigger_set_script_use("lenele1c", NULL);
        party_remove_member("Jekhar");
        party_remove_member("Rosalind");
        ls_templeint_eavesdrop_pending = true;
    } else {
        game_stage_set(0);
        if (savegame_state.get_flag(LEVEL_LPALACEINT, "got_dagger_quest"))
            conversation_db.set_topic_available("Hierophant", "\"We seek the Blade of Tarun-Khor.\"", true);
    }
    if (savegame_state.get_flag(cur_level_index, "tiger_killed")) {
        ls_templeint_tiger_intro_done = true;
        ls_templeint_tiger_killed_done = true;
    } else {
        ls_templeint_tiger_intro_done = false;
        ls_templeint_tiger_killed_done = false;
    }
    door_apply_saved_state("T-Hall-Gate05", true);
}

// 0x4f5f80  Confidence: exact. level_script_table frame. Outside a conversation the pending Urath stories
// play (each ends with the Hierophant talking again). templeint_v2 only: the boss and level music, the
// Eavesdropping scene once, Tiger-Intro at $tigertrigger (5), Tiger-Dismount when the rider reaches his
// third stage, and Tiger-Escape (tiger_killed) when his HP is down to 0.
void ls_templeint_frame()
{
    if (!(game_flags & GF_CONVERSATION)) {
        if (ls_templeint_play_urathmyth1a) {
            script_play_cutscene("UrathMyth1a", ls_templeint_urathmyth_cs_end);
            ls_templeint_play_urathmyth1a = false;
        }
        if (ls_templeint_play_urathmyth1b) {
            script_play_cutscene("UrathMyth1b", ls_templeint_urathmyth_cs_end);
            ls_templeint_play_urathmyth1b = false;
        }
        if (ls_templeint_play_urathmyth1c) {
            script_play_cutscene("UrathMyth1c", ls_templeint_urathmyth_cs_end);
            ls_templeint_play_urathmyth1c = false;
        }
        if (ls_templeint_play_urathmyth1d) {
            script_play_cutscene("UrathMyth1d", ls_templeint_urathmyth_cs_end);
            ls_templeint_play_urathmyth1d = false;
        }
        if (ls_templeint_play_urathmyth2a) {
            script_play_cutscene("UrathMyth2a", ls_templeint_urathmyth_cs_end);
            ls_templeint_play_urathmyth2a = false;
        }
        if (ls_templeint_play_urathmyth2b) {
            script_play_cutscene("UrathMyth2b", ls_templeint_urathmyth_cs_end);
            ls_templeint_play_urathmyth2b = false;
        }
        if (ls_templeint_play_urathmyth2c) {
            script_play_cutscene("UrathMyth2c", ls_templeint_urathmyth_cs_end);
            ls_templeint_play_urathmyth2c = false;
        }
        if (ls_templeint_play_urathmyth2d) {
            script_play_cutscene("UrathMyth2d", ls_templeint_urathmyth_cs_end);
            ls_templeint_play_urathmyth2d = false;
        }
    }
    if (_stricmp(level_script_name, "templeint_v2"))
        return;
    if (!ls_templeint_tiger_intro_done && savegame_state.get_flag(cur_level_index, "tiger_intro_played")
        && !(game_flags & GF_CUTSCENE)) {
        ls_templeint_tiger_intro_done = true;
        music_play(music_find_by_name("Boss"), true);
    }
    if (!ls_templeint_tiger_killed_done && savegame_state.get_flag(cur_level_index, "tiger_killed")
        && !(game_flags & GF_CUTSCENE)) {
        ls_templeint_tiger_killed_done = true;
        music_play(level_music, true);
    }
    if (ls_templeint_eavesdrop_pending && !savegame_state.get_flag(cur_level_index, "sornehan_played")) {
        script_play_cutscene("Eavesdropping", ls_templeint_eavesdropping_cs_end);
        ls_templeint_eavesdrop_pending = false;
        savegame_state.set_flag(cur_level_index, "sornehan_played", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "tiger_intro_played")
        && script_player_near_navpoint("$tigertrigger", 5.0f)) {
        script_play_cutscene("Tiger-Intro", ls_templeint_tiger_intro_cs_end);
        savegame_state.set_flag(cur_level_index, "tiger_intro_played", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "tiger_dismount_played") && tiger_dismount_stage_reached == 1) {
        script_play_cutscene("Tiger-Dismount", ls_templeint_tiger_dismount_cs_end);
        savegame_state.set_flag(cur_level_index, "tiger_dismount_played", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "tiger_killed") && tiger_find_rider()->hp <= 0.0f) {
        script_play_cutscene("Tiger-Escape", ls_templeint_tiger_escape_cs_end);
        savegame_state.set_flag(cur_level_index, "tiger_killed", true);
    }
}

// 0x4f6290  Confidence: exact. End of every UrathMyth cutscene: the Hierophant talks again.
void ls_templeint_urathmyth_cs_end()
{
    script_npc_goto_label("Hierophant", 1);
    script_npc_talk("Hierophant");
}

// 0x4f62b0  Confidence: exact. End of Tiger-Intro.
void ls_templeint_tiger_intro_cs_end()
{
    tiger_combat = true;
    level_script_event_active = true;
}

// 0x4f62c0  Confidence: exact. End of Tiger-Dismount: tiger_check_stage may go on.
void ls_templeint_tiger_dismount_cs_end()
{
    tiger_stage_trigger = true;
}

// 0x4f62d0  Confidence: exact. End of Tiger-Escape: once, the rider and his horse go and Act2-Outro plays.
void ls_templeint_tiger_escape_cs_end()
{
    if (!savegame_state.get_flag(cur_level_index, "act2_done")) {
        script_npc_set_disabled("Tiger Rider", true);
        script_npc_set_disabled("WarHorse1#1", true);
        script_play_cutscene("Act2-Outro", ls_templeint_act2_outro_cs_end);
        savegame_state.set_flag(cur_level_index, "act2_done", true);
        level_script_event_active = false;
    }
}

// 0x4f6340  Confidence: exact (the three strcpy are inline rep movs). End of Act2-Outro: the next level is
// eleh (entry 1) with the gear stripped and Joseph out of the party; every party member left is made the
// leader in turn, resurrected at full HP, its animations reset and its AI handed back to the player
// (debug printf for each); then game sequence state 4 (the level change).
void ls_templeint_act2_outro_cs_end()
{
    Player *p;
    Character *c;

    strcpy(level_prev_filename, level_filename);
    strcpy(level_filename, "eleh");
    strcpy(level_script_name, "eleh");
    level_entry_point = 1;
    script_eleh_strip_party_gear(true);
    party_remove_member("Joseph");
    for (p = party_list.next; p != &party_list; p = p->next) {
        printf("Switching char.\n");
        party_set_leader(p);
        c = living_from_handle(p->handle);
        if (c) {
            c->resurrect();
            c->hp = (float)c->get_max_stat(0);
            c->instance->clear_state_anims();
            c->instance->clear_action_anims();
            c->update_locomotion_anim();
            c->instance->process(0.0f, false, 0);
            c->set_ai_active(true);
            c->ai.flags = (c->ai.flags & ~0x42) | AI_FLAG_PLAYER_CONTROL;
            c->flags &= ~0x20;
        } else {
            printf("No character!\n");
        }
    }
    gameseq_set_state(4);
}

// 0x4f64b0  Confidence: exact. End of Eavesdropping ("Heirophant": the spelling of the binary).
void ls_templeint_eavesdropping_cs_end()
{
    script_npc_set_disabled("Heirophant", true);
    script_npc_set_disabled("Sornehan", true);
    script_npc_set_disabled("Pijian", true);
    script_npc_set_disabled("Sornehan's Knight", true);
}

// 0x4f64f0  Confidence: exact (the literal is _stricmp's first argument here). level_script_table action:
// the Hierophant's story choices.
void ls_templeint_action(dialogue_action *action)
{
    if (!_stricmp("WindowCutscene1", action->name))
        ls_templeint_play_urathmyth1a = true;
    else if (!_stricmp("WindowCutscene2", action->name))
        ls_templeint_play_urathmyth1b = true;
    else if (!_stricmp("WindowCutscene3", action->name))
        ls_templeint_play_urathmyth1c = true;
    else if (!_stricmp("WindowCutscene4", action->name))
        ls_templeint_play_urathmyth1d = true;
    else if (!_stricmp("MosaicCutscene1", action->name))
        ls_templeint_play_urathmyth2a = true;
    else if (!_stricmp("MosaicCutscene2", action->name))
        ls_templeint_play_urathmyth2b = true;
    else if (!_stricmp("MosaicCutscene3", action->name))
        ls_templeint_play_urathmyth2c = true;
    else if (!_stricmp("MosaicCutscene4", action->name))
        ls_templeint_play_urathmyth2d = true;
}

// 0x4f65e0  Confidence: exact. level_script_table use.
char ls_templeint_use(const char *name)
{
    if (!_stricmp(name, "lenele1c"))
        script_party_member_say("Joseph", XSTR("We're supposed to wait for Yago.", 558), 5.0f);
    return 0;
}
