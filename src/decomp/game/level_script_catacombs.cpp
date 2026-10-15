// level_script_catacombs.cpp - Sum.exe v1.40, code 0x4fde80-0x4fe8e4 (levelscripts\Scripts\
// level_script_catacombs.cpp; strings.tbl ids 477-480 are used here). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// The catacombs of Iona: the two puzzle balls on their pedestals open the skull door, Rosalind's remarks,
// Carados (Carados-Intro at $ctrigger) and the Ring of Light, the king tombs that open the central crypt.
// No static initialiser.
// Data: .bss 0x257ed64-0x257ed7b.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/localize/localize.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/effects/fx.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

void lsf_ball1_used();
void lsf_ball2_used();
void lsf_catacombs_boss_dead();
void lsf_catacombs_flag_statues_comment();
void lsf_catacombs_flag_bones_comment();
void lsf_catacombs_flag_way_comment();
void lsf_maindoor_open();
void lsf_catacombs_flag_iona_comment();
void ls_catacombs_sync_object_positions();
void lsf_carados_cutscene();
void ls_carados_intro_cs_end();
bool ls_catacombs_lobj_state(const char *name);

// ---- .bss (0x257ed64-0x257ed7b) ------------------------------------------------------------------------
bool ls_catacombs_carados_cutscene_done;             // 0x257ed64
int ls_catacombs_ball1_fx;                           // 0x257ed68
int ls_catacombs_ball2_fx;                           // 0x257ed6c
bool ls_catacombs_boss_dead_done;                    // 0x257ed70
// sum.symbols: void*; lobjs records (lobj_find_by_name's index into lobjs, not checked for -1).
lobj *ls_catacombs_ball1_lobj;                       // 0x257ed74
lobj *ls_catacombs_ball2_lobj;                       // 0x257ed78

// 0x4fde80  Confidence: exact. level_script_table init (stage 3): the saved puzzle and door states.
void ls_catacombs_init()
{
    level_music = music_find_by_name("Catacombs");
    ls_catacombs_carados_cutscene_done = false;
    ls_catacombs_boss_dead_done = false;
    ls_catacombs_ball1_fx = -1;
    ls_catacombs_ball2_fx = -1;
    ls_catacombs_ball1_lobj = &lobjs[lobj_find_by_name("Puzzle-Ball01")];
    ls_catacombs_ball2_lobj = &lobjs[lobj_find_by_name("Puzzle-Ball02")];
    game_stage_set(3);
    if (savegame_state.get_flag(cur_level_index, "ball1_used"))
        lsf_ball1_used();
    if (savegame_state.get_flag(cur_level_index, "ball2_used"))
        lsf_ball2_used();
    if (savegame_state.get_flag(cur_level_index, "maindoor_open"))
        door_apply_saved_state("Puzzle-SkullDoor01", true);
    if (savegame_state.get_flag(cur_level_index, "main_crypt_open"))
        door_apply_saved_state("cry_tombcen", true);
    if (savegame_state.get_flag(cur_level_index, "catacombs_boss_dead")) {
        script_npc_set_disabled("Carados", true);
        ls_catacombs_carados_cutscene_done = true;
        ls_catacombs_boss_dead_done = true;
    }
}

// 0x4fdfc0  Confidence: exact. The first ball placed: its door object moves and the effect follows it.
void lsf_ball1_used()
{
    savegame_state.set_flag(cur_level_index, "ball1_used", true);
    door_apply_saved_state("Puzzle-Ball01", true);
    if (ls_catacombs_ball1_fx == -1) {
        ls_catacombs_ball1_fx = fx_create_world("CatacombsEffect.vfx", false);
        if (ls_catacombs_ball1_fx != -1)
            obj_from_handle(ls_catacombs_ball1_fx)->pos = ls_catacombs_ball1_lobj->pos;
    }
}

// 0x4fe030  Confidence: exact.
void lsf_ball2_used()
{
    savegame_state.set_flag(cur_level_index, "ball2_used", true);
    door_apply_saved_state("Puzzle-Ball02", true);
    if (ls_catacombs_ball2_fx == -1) {
        ls_catacombs_ball2_fx = fx_create_world("CatacombsEffect.vfx", false);
        if (ls_catacombs_ball2_fx != -1)
            obj_from_handle(ls_catacombs_ball2_fx)->pos = ls_catacombs_ball2_lobj->pos;
    }
}

// 0x4fe0a0  Confidence: exact. The level music after Carados; the Ring of Light in the party also sets
// catacombs_boss_dead (then the kill test is skipped). The whole function is skipped while the never
// written byte 0x5fbf19 is set (see NOTES).
void lsf_catacombs_boss_dead()
{
    if (cheat_gold_5fbf19)
        return;
    if (!ls_catacombs_boss_dead_done && savegame_state.get_flag(cur_level_index, "catacombs_boss_dead")) {
        ls_catacombs_boss_dead_done = true;
        music_play(level_music, true);
    }
    if (savegame_state.get_flag(cur_level_index, "got_ring_of_light"))
        return;
    if (party_count_item_by_name("Ring of Light")) {
        savegame_state.set_flag(cur_level_index, "got_ring_of_light", true);
        hud_show_message(XSTR("Joseph acquired Summon Mist Elemental!", 480));
        savegame_state.set_flag(cur_level_index, "catacombs_boss_dead", true);
    }
    if (!savegame_state.get_flag(cur_level_index, "catacombs_boss_dead")
        && savegame_state.get_flag(cur_level_index, "carados_cutscene") && script_npc_is_dead("Carados")) {
        savegame_state.set_flag(cur_level_index, "catacombs_boss_dead", true);
        level_script_event_active = false;
    }
}

// 0x4fe1c0  Confidence: exact. level_script_table frame. The cryptintro block needs the never written
// byte 0x5fc014 and an already set catacombs_flag_intro_played: it cannot run (see NOTES).
void ls_catacombs_frame()
{
    int id, index;

    ls_catacombs_sync_object_positions();
    lsf_catacombs_flag_statues_comment();
    lsf_catacombs_flag_bones_comment();
    lsf_catacombs_flag_way_comment();
    lsf_maindoor_open();
    lsf_carados_cutscene();
    lsf_catacombs_boss_dead();
    if (game_debug_5fc014 && savegame_state.get_flag(cur_level_index, "catacombs_flag_intro_played")) {
        savegame_state.set_flag(cur_level_index, "catacombs_flag_intro_played", true);
        script_play_cutscene("cryptintro", NULL);
    }
    level_trigger_trip(cur_character, &id, &index);
}

// 0x4fe250  Confidence: exact.
void lsf_catacombs_flag_statues_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "catacombs_flag_statues_comment")
        && script_player_near_navpoint("$npc009", 4.0f)) {
        savegame_state.set_flag(cur_level_index, "catacombs_flag_statues_comment", true);
        script_party_member_say("Rosalind", XSTR("The first kings of the thirteen clans are buried here.", 477),
                                5.0f);
    }
}

// 0x4fe2c0  Confidence: exact. Said for 6 s.
void lsf_catacombs_flag_bones_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "catacombs_flag_bones_comment")
        && script_player_near_navpoint("$npc010", 4.0f)) {
        savegame_state.set_flag(cur_level_index, "catacombs_flag_bones_comment", true);
        script_party_member_say("Rosalind", XSTR("We're inside the bones of a three-headed serpent, Vadagar's ...",
                                                 478), 6.0f);
    }
}

// 0x4fe330  Confidence: exact. Sets its flag and says nothing (no line in the code).
void lsf_catacombs_flag_way_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "CATACOMBS_FLAG_WAY_COMMENT")
        && script_player_near_navpoint("$npc011", 5.0f))
        savegame_state.set_flag(cur_level_index, "CATACOMBS_FLAG_WAY_COMMENT", true);
}

// 0x4fe380  Confidence: exact. Both balls placed, $doortrigger within 10: the skull door opens.
void lsf_maindoor_open()
{
    if (!savegame_state.get_flag(cur_level_index, "maindoor_open") && script_player_near_navpoint("$doortrigger", 10.0f)
        && savegame_state.get_flag(cur_level_index, "ball1_used")
        && savegame_state.get_flag(cur_level_index, "ball2_used")) {
        door_apply_saved_state("Puzzle-SkullDoor01", true);
        lsf_catacombs_flag_iona_comment();
        savegame_state.set_flag(cur_level_index, "maindoor_open", true);
    }
}

// 0x4fe410  Confidence: exact.
void lsf_catacombs_flag_iona_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "CATACOMBS_FLAG_IONA_COMMENT")) {
        savegame_state.set_flag(cur_level_index, "CATACOMBS_FLAG_IONA_COMMENT", true);
        script_party_member_say("Rosalind", XSTR("Here it is! The tomb of the goddess Iona!", 479), 6.0f);
    }
}

// 0x4fe460  Confidence: exact. The ball effects follow the (moving) ball objects.
void ls_catacombs_sync_object_positions()
{
    if (ls_catacombs_ball1_fx != -1)
        obj_from_handle(ls_catacombs_ball1_fx)->pos = ls_catacombs_ball1_lobj->pos;
    if (ls_catacombs_ball2_fx != -1)
        obj_from_handle(ls_catacombs_ball2_fx)->pos = ls_catacombs_ball2_lobj->pos;
}

// 0x4fe4c0  Confidence: exact. Boss music after Carados-Intro; the intro at $ctrigger (4) once the skull
// door is open.
void lsf_carados_cutscene()
{
    if (!ls_catacombs_carados_cutscene_done && savegame_state.get_flag(cur_level_index, "carados_cutscene")
        && !(game_flags & GF_CUTSCENE)) {
        ls_catacombs_carados_cutscene_done = true;
        music_play(music_find_by_name("Boss"), true);
    }
    if (!savegame_state.get_flag(cur_level_index, "carados_cutscene")
        && savegame_state.get_flag(cur_level_index, "maindoor_open")
        && script_player_near_navpoint("$ctrigger", 4.0f)) {
        savegame_state.set_flag(cur_level_index, "carados_cutscene", true);
        script_play_cutscene("Carados-Intro", ls_carados_intro_cs_end);
    }
}

// 0x4fe580  Confidence: exact. End of Carados-Intro.
void ls_carados_intro_cs_end()
{
    script_npc_goto_label("Carados", 1);
    script_npc_goto_label("Bone Mage#Guard1", 1);
    script_npc_goto_label("Bone Mage#Guard2", 1);
    level_script_event_active = true;
}

// 0x4fe5b0  Confidence: likely (the open flag lives in the argument's stack slot: VC6 reuses the dead
// parameter's slot for the local). The saved open state of a level object.
bool ls_catacombs_lobj_state(const char *name)
{
    bool open, locked;

    savegame_state.two_stage_get(&open, &locked, lobj_from_index(lobj_find_by_name(name)));
    return open;
}

// 0x4fe5e0  Confidence: likely (the shared push of "cry_kingf" before the cry_kinge branch). level_script_table
// use. The two king tombs E and F open the central crypt when the other one is open; 2000 XP the first time
// and only while every other king tomb (A-D, G-M) is closed.
char ls_catacombs_use(const char *name)
{
    if (!_stricmp(name, "Puzzle-Ball01")) {
        lsf_ball1_used();
        return 0;
    }
    if (!_stricmp(name, "Puzzle-Ball02")) {
        lsf_ball2_used();
        return 0;
    }
    if (!_stricmp(name, "Puzzle-Pedestal01")) {
        lsf_ball1_used();
        return 0;
    }
    if (!_stricmp(name, "Puzzle-Pedestal02")) {
        lsf_ball2_used();
        return 0;
    }
    if (!_stricmp(name, "cry_tombcen")) {
        savegame_state.set_flag(cur_level_index, "main_crypt_open", true);
        return 0;
    }
    if (!_stricmp(name, "cry_kinge")) {
        if (!ls_catacombs_lobj_state("cry_kingf"))
            return 0;
        if (!ls_catacombs_lobj_state("cry_kinga") && !ls_catacombs_lobj_state("cry_kingb")
            && !ls_catacombs_lobj_state("cry_kingc") && !ls_catacombs_lobj_state("cry_kingd")
            && !ls_catacombs_lobj_state("cry_kingg") && !ls_catacombs_lobj_state("cry_kingh")
            && !ls_catacombs_lobj_state("cry_kingi") && !ls_catacombs_lobj_state("cry_kingk")
            && !ls_catacombs_lobj_state("cry_kingl") && !ls_catacombs_lobj_state("cry_kingm")
            && !savegame_state.get_flag(cur_level_index, "main_crypt_open"))
            script_party_gain_xp(2000);
        door_apply_saved_state("cry_tombcen", true);
        savegame_state.set_flag(cur_level_index, "main_crypt_open", true);
        return 1;
    }
    if (!_stricmp(name, "cry_kingf")) {
        if (!ls_catacombs_lobj_state("cry_kinge"))
            return 0;
        if (!ls_catacombs_lobj_state("cry_kinga") && !ls_catacombs_lobj_state("cry_kingb")
            && !ls_catacombs_lobj_state("cry_kingc") && !ls_catacombs_lobj_state("cry_kingd")
            && !ls_catacombs_lobj_state("cry_kingg") && !ls_catacombs_lobj_state("cry_kingh")
            && !ls_catacombs_lobj_state("cry_kingi") && !ls_catacombs_lobj_state("cry_kingk")
            && !ls_catacombs_lobj_state("cry_kingl") && !ls_catacombs_lobj_state("cry_kingm")
            && !savegame_state.get_flag(cur_level_index, "main_crypt_open"))
            script_party_gain_xp(2000);
        door_apply_saved_state("cry_tombcen", true);
        savegame_state.set_flag(cur_level_index, "main_crypt_open", true);
        return 1;
    }
    return 0;
}
