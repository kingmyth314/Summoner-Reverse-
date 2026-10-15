// level_script_templeint2.cpp - Sum.exe v1.40, code 0x4ef190-0x4ef849 (levelscripts\Scripts\
// level_script_templeint2.cpp; strings.tbl id 559 is this unit's). Written from the Sum.exe disassembly
// (tools/asm.mjs; the float constant read from the image). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (one detail of
// statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// The Temple of Urath, second visit (act 4, the end of the game): Nar-TempleAct4, Machival's intro, Evil
// Urath and Evil Joseph (Machival-Trans1 / -Trans2), Machival's death, the two endings (End1-EnterForge ->
// End1-Montage at $end2trig, End2-LeaveTemple -> End2-Montage at the lenele3a exit) and the credits.
// The Black Knights get up again while the player looks at them. No static initialiser.
// Data: .bss 0x257e9b4-0x257e9b8.
//
// Header (D1400): the level script units have no header of their own; the hooks and goto_credits are
// declared in levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/os/frametime.h"
#include "vsdk/math/vector.h"
#include "vsdk/localize/localize.h"
#include "vsdk/ca/character_instance.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/camera/camera.h"
#include "engine/effects/fx.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

void ls_templeint2_act4_cs_end();
void ls_templeint2_machival_intro_cs_end();
void lsf_urath_killed();
void ls_templeint2_machival_trans1_cs_end();
void lsf_joseph_killed();
void ls_templeint2_machival_trans2_cs_end();
void lsf_mach_killed();
void ls_templeint2_machival_death_cs_end();
void ls_templeint2_enter_forge();
void ls_templeint2_enter_forge_cs_end();
void ls_templeint2_black_knight_revive();
void ls_templeint2_leave_temple_cs_end();

// ---- .bss (0x257e9b4-0x257e9b8) ------------------------------------------------------------------------
bool ls_templeint2_mach_killed_done;                 // 0x257e9b4
// Cleared by ls_templeint2_init and never read (`mov byte ptr [0x257e9b5], bl` at 0x4ef1b7 is its only
// reference). Not in sum.symbols; name ours.
bool ls_templeint2_unused_9b5;                       // 0x257e9b5
bool ls_templeint2_forge_entered;                    // 0x257e9b6
bool ls_templeint2_act4_played;                      // 0x257e9b7
bool ls_templeint2_end2_ready;                       // 0x257e9b8

// 0x4ef190  Confidence: exact. level_script_table init: stage 21 (act 4), the lenele3a exit is a
// script-use trigger (ls_templeint2_use).
void ls_templeint2_init()
{
    level_music = music_find_by_name("TempleInt2");
    game_stage_set(0x15);
    level_trigger_set_script_use("lenele3a", NULL);
    ls_templeint2_unused_9b5 = false;
    ls_templeint2_mach_killed_done = false;
    ls_templeint2_act4_played = false;
    ls_templeint2_forge_entered = false;
    ls_templeint2_end2_ready = false;
}

// 0x4ef1e0  Confidence: exact (ends in a tail jump to ls_templeint2_black_knight_revive). level_script_table
// frame. Nar-TempleAct4 plays on every entry (the flag is not saved).
void ls_templeint2_frame()
{
    if (!ls_templeint2_act4_played) {
        script_play_cutscene("Nar-TempleAct4", ls_templeint2_act4_cs_end);
        ls_templeint2_act4_played = true;
    }
    lsf_urath_killed();
    lsf_joseph_killed();
    lsf_mach_killed();
    ls_templeint2_enter_forge();
    ls_templeint2_black_knight_revive();
}

// 0x4ef220  Confidence: exact. End of Nar-TempleAct4: the first time, the three wooden doors open and
// Machival-intro plays.
void ls_templeint2_act4_cs_end()
{
    if (!savegame_state.get_flag(cur_level_index, "mach_intro")) {
        door_apply_saved_state("T-BRoom-L0WoodDoor01", true);
        door_apply_saved_state("T-CRoom-L0WoodDoor01", true);
        door_apply_saved_state("T-CRoom-L0WoodDoor02", true);
        script_play_cutscene("Machival-intro", ls_templeint2_machival_intro_cs_end);
        savegame_state.set_flag(cur_level_index, "mach_intro", true);
        level_script_event_active = false;
    }
}

// 0x4ef290  Confidence: exact. End of Machival-intro: Evil Urath fights first.
void ls_templeint2_machival_intro_cs_end()
{
    script_npc_goto_label("Machival", 1);
    script_npc_goto_label("Evil Urath", 2);
    evil_urath_combat_enabled = true;
    music_play(music_find_by_name("Machival"), true);
}

// 0x4ef2d0  Confidence: exact.
void lsf_urath_killed()
{
    if (!savegame_state.get_flag(cur_level_index, "urath_killed") && script_npc_is_dead("Evil Urath")) {
        script_play_cutscene("Machival-Trans1", ls_templeint2_machival_trans1_cs_end);
        savegame_state.set_flag(cur_level_index, "urath_killed", true);
    }
}

// 0x4ef330  Confidence: exact. End of Machival-Trans1: Evil Joseph fights next.
void ls_templeint2_machival_trans1_cs_end()
{
    script_npc_drop_loot("Evil Urath");
    script_npc_goto_label("Evil Urath", 1);
    script_npc_goto_label("Evil Joseph", 2);
    evil_joseph_combat_enabled = true;
}

// 0x4ef360  Confidence: exact.
void lsf_joseph_killed()
{
    if (!savegame_state.get_flag(cur_level_index, "joseph_killed") && script_npc_is_dead("Evil Joseph")) {
        evil_joseph_destroy_effects();
        script_play_cutscene("Machival-Trans2", ls_templeint2_machival_trans2_cs_end);
        savegame_state.set_flag(cur_level_index, "joseph_killed", true);
    }
}

// 0x4ef3c0  Confidence: exact. End of Machival-Trans2: Machival himself.
void ls_templeint2_machival_trans2_cs_end()
{
    script_npc_drop_loot("Evil Joseph");
    script_npc_goto_label("Evil Joseph", 1);
    script_npc_goto_label("Machival", 2);
    machival_combat_enabled = true;
}

// 0x4ef3f0  Confidence: exact. Before mach_killed: Machival dead -> four Black Knights disabled,
// Machival-Death. After: the Questions music once no cutscene runs (the flag test is repeated).
void lsf_mach_killed()
{
    if (savegame_state.get_flag(cur_level_index, "mach_killed")) {
        if (!ls_templeint2_mach_killed_done && savegame_state.get_flag(cur_level_index, "mach_killed")
            && !(game_flags & GF_CUTSCENE)) {
            ls_templeint2_mach_killed_done = true;
            music_play(music_find_by_name("Questions"), true);
        }
        return;
    }
    if (script_npc_is_dead("Machival")) {
        script_npc_set_disabled("Black Knight#1", true);
        script_npc_set_disabled("Black Knight#2", true);
        script_npc_set_disabled("Black Knight#5", true);
        script_npc_set_disabled("Black Knight#6", true);
        script_play_cutscene("Machival-Death", ls_templeint2_machival_death_cs_end);
        savegame_state.set_flag(cur_level_index, "mach_killed", true);
    }
}

// 0x4ef4d0  Confidence: likely (the summon loop: the next pointer is read before the body). End of
// Machival-Death: Jekhar, Flece and Rosalind leave the party (their NPC copies take label 2), every summon is
// dismissed, and the party (Joseph) is placed at entry 2.
void ls_templeint2_machival_death_cs_end()
{
    Character *c, *next;

    script_npc_goto_label("Machival", 1);
    party_remove_member("Jekhar");
    party_remove_member("Flece");
    party_remove_member("Rosalind");
    hud_show_message("");
    script_npc_goto_label("Jekhar#npc", 2);
    script_npc_goto_label("Flece#npc", 2);
    script_npc_goto_label("Rosalind#npc", 2);
    c = living_list.live_next;
    next = c->live_next;
    while (c != &living_list) {
        if (c && (c->flags & CF_SUMMON)) {
            fx_end_owned(c->handle, false);
            c->summon_return_lent_max_hp();
            c->deinit();
        }
        c = next;
        next = next->live_next;
    }
    level_entry_point = 2;
    ls_templeint2_end2_ready = true;
    script_place_party_at_starts();
    game_camera.set_target(cur_character);
    game_camera.snap_to_target(1, 1);
    game_camera.set_mode(2);
}

// 0x4ef5d0  Confidence: exact. Ending 1: back at $end2trig after Machival's death, the companions rejoin
// and End1-EnterForge plays.
void ls_templeint2_enter_forge()
{
    if (savegame_state.get_flag(cur_level_index, "mach_killed") && ls_templeint2_end2_ready
        && script_player_near_navpoint("$end2trig", 5.0f) && !ls_templeint2_forge_entered) {
        party_add_member("Jekhar");
        party_add_member("Flece");
        party_add_member("Rosalind");
        script_npc_goto_label("Jekhar#npc", 1);
        script_npc_goto_label("Flece#npc", 1);
        script_npc_goto_label("Rosalind#npc", 1);
        script_play_cutscene("End1-EnterForge", ls_templeint2_enter_forge_cs_end);
        ls_templeint2_forge_entered = true;
    }
}

// 0x4ef680  Confidence: exact. End of End1-EnterForge.
void ls_templeint2_enter_forge_cs_end()
{
    ls_templeint2_forge_entered = true;
    script_play_cutscene("End1-Montage", goto_credits);
}

// 0x4ef6a0  Confidence: exact. The end of both montages: game sequence state 9 (the credits).
void goto_credits()
{
    gameseq_set_state(9);
}

// 0x4ef6b0  Confidence: likely (the selection keeps the last match of the list walk). A dead Black Knight
// in view gets up again with a chance of 0.1 per second of frame time (frand() < frametime * 0.1).
void ls_templeint2_black_knight_revive()
{
    Character *c, *knight;

    knight = NULL;
    for (c = living_list_inactive.live_next; c != &living_list_inactive; c = c->live_next) {
        if (!_strnicmp(c->name, "Black Knight", 11) && c->is_in_view(1.0f)
            && frand() < reported_frametime * 0.1f)
            knight = c;
    }
    if (knight) {
        knight->resurrect();
        knight->hp = (float)knight->get_max_stat(0);
        knight->instance->clear_state_anims();
        knight->instance->clear_action_anims();
        knight->update_locomotion_anim();
        knight->instance->process(0, 0, 0);
    }
}

// 0x4ef770  Confidence: exact. level_script_table use. Ending 2: leaving by the lenele3a exit after
// Machival's death.
char ls_templeint2_use(const char *name)
{
    if (!_stricmp(name, "lenele3a")) {
        if (savegame_state.get_flag(cur_level_index, "mach_killed")) {
            if (!ls_templeint2_forge_entered) {
                party_add_member("Jekhar");
                party_add_member("Flece");
                party_add_member("Rosalind");
                script_npc_goto_label("Jekhar#npc", 1);
                script_npc_goto_label("Flece#npc", 1);
                script_npc_goto_label("Rosalind#npc", 1);
                script_play_cutscene("End2-LeaveTemple", ls_templeint2_leave_temple_cs_end);
                ls_templeint2_forge_entered = true;
                return 0;
            }
        } else {
            script_party_member_say("Joseph", XSTR("We're not leaving until we've captured Machival!", 559),
                                    5.0f);
        }
    }
    return 0;
}

// 0x4ef830  Confidence: exact. End of End2-LeaveTemple.
void ls_templeint2_leave_temple_cs_end()
{
    ls_templeint2_forge_entered = true;
    script_play_cutscene("End2-Montage", goto_credits);
}
