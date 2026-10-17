// level_script_masad.cpp - Sum.exe v1.40, code 0x4fd6f0-0x4fddd8. Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "masad" (index 3; Joseph's home village, the game start: the tutorial fire
// doors, Nath, the barbarian; masad_v2 is the burnt village of the later visit). FILE NAME NOT PROVEN
// (D5300): no strings.tbl block, no path string; the name follows the level_script_<level>.cpp convention.
// Data: .bss 0x257ed5c-0x257ed5d, between worldmap's 0x257ed50.. and the "test" record's 0x257ed60 (a
// 4-byte step: the test hooks after this code are a unit of their own, D5301). No static initialiser.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/math/quaternion.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

void ls_masad_reset_fire();
void lsf_masad_flag_gathering();
void lsf_masad_talked_to_nath();
void lsf_masad_intro();
void ls_masad_game_pre_intro_cs_end();
void ls_masad_game_intro_cs_end();
void lsf_masad_barb();
void ls_masad_leaving_cs_end();

// ---- .bss (0x257ed5c-0x257ed5d) ------------------------------------------------------------------------
bool ls_masad_barb_done;                             // 0x257ed5c the barbarian dead: level music again
bool ls_masad_barb_music_done;                       // 0x257ed5d the boss music started

// 0x4fd6f0  Confidence: exact. masad_v2 (stage 0): the tutorial fires are out, the barbarian's two flags
// count as done and the three invisible doors take their saved state (open).
void ls_masad_reset_fire()
{
    game_stage_set(0);
    fx_disable_by_effect_name("masad-firedoor1.vfx");
    fx_disable_by_effect_name("masad-firedoor2.vfx");
    fx_disable_by_effect_name("masad-fire1.vfx");
    ls_masad_barb_music_done = true;
    ls_masad_barb_done = true;
    door_apply_saved_state("invis-door01", true);
    door_apply_saved_state("invis-door02", true);
    door_apply_saved_state("invis-door03", true);
}

// 0x4fd750  Confidence: exact (the three door blocks as written out; they may be a macro or a local helper
// the compiler would not have expanded under /Ob1, D5305). level_script_table init. The three invisible doors
// are snapped to their open pose; door01's saved "open" bit is the combat tutorial's part 2, doors 02 and 03
// are saved open, the locked bits are kept. The first fire door's effect goes out unconditionally, then again
// after the dialogue tutorial (see NOTES); the second after the combat tutorial (tested twice). masad_v2
// resets the fires, the first visit makes the world map exit a script use.
void ls_masad_init()
{
    lobj *l;
    door_info *door;
    bool open, locked;
    bool part2;

    ls_masad_barb_music_done = false;
    ls_masad_barb_done = false;
    part2 = savegame_state.get_flag(cur_level_index, "masad_basic_combat_tutorial_part2") != 0;

    l = &lobjs[lobj_find_by_name("invis-door01")];
    door = door_from_lobj(l);
    savegame_state.two_stage_get(&open, &locked, l);
    savegame_state.two_stage_set(part2, locked, l);
    l->pos = door->open_pos;
    door->open_orient.to_matrix_b(&l->orient);

    l = &lobjs[lobj_find_by_name("invis-door02")];
    door = door_from_lobj(l);
    savegame_state.two_stage_get(&open, &locked, l);
    savegame_state.two_stage_set(true, locked, l);
    l->pos = door->open_pos;
    door->open_orient.to_matrix_b(&l->orient);

    l = &lobjs[lobj_find_by_name("invis-door03")];
    door = door_from_lobj(l);
    savegame_state.two_stage_get(&open, &locked, l);
    savegame_state.two_stage_set(true, locked, l);
    l->pos = door->open_pos;
    door->open_orient.to_matrix_b(&l->orient);

    fx_disable_by_effect_name("masad-firedoor1.vfx");
    if (part2)
        fx_disable_by_effect_name("masad-firedoor2.vfx");
    if (savegame_state.get_flag(cur_level_index, "masad_dialogue_tutorial_part3"))
        fx_disable_by_effect_name("masad-firedoor1.vfx");
    if (savegame_state.get_flag(cur_level_index, "masad_basic_combat_tutorial_part2"))
        fx_disable_by_effect_name("masad-firedoor2.vfx");
    level_music = music_find_by_name("masad");
    if (!_stricmp(level_script_name, "masad_v2"))
        ls_masad_reset_fire();
    else
        level_trigger_set_script_use("worldmap1", NULL);
}

// 0x4fd980  Confidence: exact (both paths end in tail jumps). masad_v2 runs only a folded empty hook.
void ls_masad_frame()
{
    if (!_stricmp(level_script_name, "masad_v2")) {
        empty_function();
        return;
    }
    lsf_masad_flag_gathering();
    lsf_masad_talked_to_nath();
    lsf_masad_intro();
    lsf_masad_barb();
}

// 0x4fd9b0  Confidence: exact. Within 10 m of Nath: Khag, Uvala and Nath gather (label 2).
void lsf_masad_flag_gathering()
{
    if (!savegame_state.get_flag(cur_level_index, "masad_flag_gathering")
        && script_npc_near_player("Nath", 10.0f)) {
        savegame_state.set_flag(cur_level_index, "masad_flag_gathering", true);
        script_npc_goto_label("Khag", 2);
        script_npc_goto_label("Uvala", 2);
        script_npc_goto_label("Nath", 2);
    }
}

// 0x4fda20  Confidence: exact. At $nath_trigger (3 m, no conversation): Nath talks.
void lsf_masad_talked_to_nath()
{
    if (!savegame_state.get_flag(cur_level_index, "masad_talked_to_nath") && conversation_active.is_idle()
        && script_player_near_navpoint("$nath_trigger", 3.0f)) {
        savegame_state.set_flag(cur_level_index, "masad_talked_to_nath", true);
        script_npc_talk("Nath");
    }
}

// 0x4fda90  Confidence: exact. The first frame of a new game: Joseph gets a Tunic (what the slot held is
// dropped, not put in the inventory), then Game-Pre-Intro and Game-Intro.
void lsf_masad_intro()
{
    item_info *old1;
    item_info *old2;

    if (savegame_state.get_flag(cur_level_index, "masad_intro"))
        return;
    if (cur_character)
        character_equip_item(&old1, &old2, cur_character, 7, item_create_by_name("Tunic"));
    savegame_state.set_flag(cur_level_index, "masad_intro", true);
    script_play_cutscene("Game-Pre-Intro", ls_masad_game_pre_intro_cs_end);
}

// 0x4fdb10  Confidence: exact. End of Game-Pre-Intro.
void ls_masad_game_pre_intro_cs_end()
{
    script_play_cutscene("Game-Intro", ls_masad_game_intro_cs_end);
}

// 0x4fdb30  Confidence: exact. End of Game-Intro: the cutscene-only characters ("CS#...") stop thinking.
void ls_masad_game_intro_cs_end()
{
    for (Character *c = living_list.live_next; c != &living_list; c = c->live_next)
        if (!_strnicmp(c->name, "CS#", 3))
            c->set_ai_active(0);
}

// 0x4fdb80  Confidence: exact. The barbarian: the level music back when he is dead; once masad_barb is set
// the boss music (outside cutscenes); before that, after Nath's talk, at $cuttrig (2.5 m, no conversation)
// Barbarian-Intro plays and he attacks.
void lsf_masad_barb()
{
    if (!ls_masad_barb_done && script_npc_is_dead("Barbarian Fighter#$npc037")) {
        ls_masad_barb_done = true;
        music_play(level_music, true);
    }
    if (savegame_state.get_flag(cur_level_index, "masad_barb")) {
        if (!ls_masad_barb_music_done && !(game_flags & GF_CUTSCENE)) {
            ls_masad_barb_music_done = true;
            int music = music_find_by_name("Boss");
            music_play(music, true);
        }
    } else if (!(game_flags & GF_CONVERSATION) && savegame_state.get_flag(cur_level_index, "masad_talked_to_nath")
               && script_player_near_navpoint("$cuttrig", 2.5f)) {
        savegame_state.set_flag(cur_level_index, "masad_barb", true);
        script_play_cutscene("Barbarian-Intro", NULL);
        script_npc_send_go("Barbarian Fighter#$npc037");
    }
}

// 0x4fdc70  Confidence: exact (a tail jump). level_script_table leave: masad_v2 runs a folded empty hook.
void ls_masad_leave()
{
    if (!_stricmp(level_script_name, "masad_v2"))
        empty_function();
}

// 0x4fdc90  Confidence: exact. level_script_table use: the world map exit after the barbarian is dead plays
// Nar-Leaving-Masad (which changes the level). Always 0.
char ls_masad_use(const char *name)
{
    if (!_stricmp(name, "worldmap1") && script_npc_is_dead("Barbarian Fighter#$npc037"))
        script_play_cutscene("Nar-Leaving-Masad", ls_masad_leaving_cs_end);
    return 0;
}

// 0x4fdcd0  Confidence: exact (VC6 byte match, match14; was exact) (the three strcpy are inline rep movs). End of Nar-Leaving-Masad: to the world
// map (entry 2), game sequence state 4.
void ls_masad_leaving_cs_end()
{
    strcpy(level_prev_filename, level_filename);
    strcpy(level_filename, "worldmap1");
    strcpy(level_script_name, "worldmap1");
    level_entry_point = 2;
    gameseq_set_state(4);
}

// 0x4fdd60  Confidence: exact. level_script_table action. "TalkNath" sets the flag by its upper-case name
// (the flag names are hashed without case).
void ls_masad_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "TalkNath"))
        savegame_state.set_flag(cur_level_index, "MASAD_TALKED_TO_NATH", true);
    if (!_stricmp(action->name, "LaekhoCall"))
        script_npc_goto_label("Orenian Scout#$npc014", 1);
    if (!_stricmp(action->name, "KillAesik"))
        script_npc_goto_label("Aesik", 1);
}
