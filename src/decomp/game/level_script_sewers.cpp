// level_script_sewers.cpp - Sum.exe v1.40, code 0x4ed7c0-0x4ee722 (levelscripts\Scripts\level_script_sewers.cpp;
// strings.tbl ids 549-556 are this unit's). Written from the Sum.exe disassembly (tools/asm.mjs; floats read
// from the image). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// Two level_script_table records: "sewer" (the Lenele sewers: the three visits sewer / sewer_v2 / sewer_v3,
// the gate wheels and the party remarks) and "sewerboss" (the tentacle beast's lair, TB-Intro). Static
// initialiser table entry 477 (0x588778) is this unit's.
// Data: .bss 0x257e998-0x257e9a7. No .data of its own (the literals only).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/localize/localize.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/camera/camera.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

// The level indices the boss lair reads its flags from (level_script_table order).
#define LEVEL_SEWER        9                         // "sewer_found_tb", "tb_killed" (0x4ee265, 0x4ee177)
#define LEVEL_LPALACEINT   0x18                      // "got_ring_of_darkness" (0x4edc56)
#define LEVEL_LENELE1AA    0x23                      // "unhide_cerval" (0x4ed8a1)

void lsf_sewer_flag_scene_a();
void lsf_sewer_flag_found_crate();
void lsf_sewers_flag_crate_comment();
void lsf_sewers_flag_different_comment();
void lsf_sewers_flag_passage_comment();
void lsf_sewers_flag_doll_comment();
void lsf_sewers_flag_where_comment();
void lsf_sewers_flag_smell_comment();
bool ls_sewerboss_golems_dead();
void lsf_sewer_found_tb();
void ls_sewer_tb_intro_cs_end();
void ls_sewer_create_avoid_spline();

// ---- .bss (0x257e998-0x257e9a7) ----------------------------------------------------------------------
bool ls_sewerboss_golems_done;                       // 0x257e998
// 0x4ed7c0  Confidence: exact. static_init_ls_sewerboss_timer: "jmp 0x4ed7d0" (initialiser entry 477).
// 0x4ed7d0  Confidence: exact. init_ls_sewerboss_timer: "mov ecx, 0x257e99c / jmp GameTimer::GameTimer".
GameTimer ls_sewerboss_music_timer;                  // 0x257e99c
int ls_sewerboss_avoid_spline;                       // 0x257e9a0
// 1 first visit (comments), 2 sewer_v2, 3 sewer_v3 (ls_sewer_init).
int ls_sewer_arrival;                                // 0x257e9a4

// 0x4ed7e0  Confidence: exact (each stage branch writes the whole v11-v52 list, which 8966 tail-merges with the stack pops; the first visit: the _stricmp against level_prev_filename "lenele1d" is
// called and its result tested (0x4ed88b), then both ways call game_stage_set(0); written as an if / else
// with equal bodies that the compiler merged). level_script_table init. The gate and wheel triggers by game
// stage: from stage 9 everything is open but $GateC3; stage 7-8 the second visit's layout.
void ls_sewer_init()
{
    level_music = music_find_by_name("sewer");
    if (!_stricmp(level_script_name, "sewer_v3")) {
        ls_sewer_arrival = 3;
        if (game_stage_get() < 0x11)
            game_stage_set(0x10);
        level_trigger_set_script_use("sewerboss", NULL);
        level_trigger_set_script_use("lenele1b", NULL);
    } else if (!_stricmp(level_script_name, "sewer_v2")) {
        ls_sewer_arrival = 2;
        game_stage_set(7);
    } else {
        ls_sewer_arrival = 1;
        if (!_stricmp(level_prev_filename, "lenele1d"))
            game_stage_set(0);
        else
            game_stage_set(0);
        if (!savegame_state.get_flag(LEVEL_LENELE1AA, "unhide_cerval")
            || savegame_state.get_flag(cur_level_index, "cerval_done"))
            script_npc_set_disabled("Cerval", true);
    }

    if (game_stage_get() >= 9) {
        level_trigger_set_script_use_by_refname("v11");
        level_trigger_set_script_use_by_refname("v12");
        level_trigger_set_script_use_by_refname("v13");
        level_trigger_set_script_use_by_refname("v21");
        level_trigger_set_script_use_by_refname("v22");
        level_trigger_set_script_use_by_refname("v23");
        level_trigger_set_script_use_by_refname("v31");
        level_trigger_set_script_use_by_refname("v32");
        level_trigger_set_script_use_by_refname("v33");
        level_trigger_set_script_use_by_refname("v41");
        level_trigger_set_script_use_by_refname("v42");
        level_trigger_set_script_use_by_refname("v43");
        level_trigger_set_exit_by_refname("v51");
        level_trigger_set_exit_by_refname("v52");
        door_apply_saved_state("$GateC3", false);
        door_apply_saved_state("$GateB2", true);
        door_apply_saved_state("$GateA3", true);
        door_apply_saved_state("$GateA2", true);
        door_apply_saved_state("$GateA1", true);
        door_apply_saved_state("$GateB1", true);
        door_apply_saved_state("$GateC1", true);
        return;
    }
    if (game_stage_get() >= 7) {
        level_trigger_set_script_use_by_refname("v11");
        level_trigger_set_script_use_by_refname("v12");
        level_trigger_set_script_use_by_refname("v13");
        level_trigger_set_script_use_by_refname("v21");
        level_trigger_set_script_use_by_refname("v22");
        level_trigger_set_script_use_by_refname("v23");
        level_trigger_set_script_use_by_refname("v31");
        level_trigger_set_script_use_by_refname("v32");
        level_trigger_set_script_use_by_refname("v33");
        level_trigger_set_exit_by_refname("v41");
        level_trigger_set_exit_by_refname("v42");
        level_trigger_set_exit_by_refname("v43");
        level_trigger_set_script_use_by_refname("v51");
        level_trigger_set_script_use_by_refname("v52");
        door_apply_saved_state("$GateC3", false);
        door_apply_saved_state("$GateB2", false);
        door_apply_saved_state("$GateA3", true);
        door_apply_saved_state("$GateA2", true);
        if (game_stage_get() >= 8) {
            door_apply_saved_state("$GateA1", true);
            door_apply_saved_state("$GateB1", true);
            door_apply_saved_state("$GateC1", false);
        } else {
            door_apply_saved_state("$GateA1", false);
            door_apply_saved_state("$GateB1", false);
            door_apply_saved_state("$GateC1", true);
        }
        return;
    }
    if (game_stage_get() >= 6) {
        level_trigger_set_script_use_by_refname("v11");
        level_trigger_set_script_use_by_refname("v12");
        level_trigger_set_script_use_by_refname("v13");
        level_trigger_set_script_use_by_refname("v21");
        level_trigger_set_script_use_by_refname("v22");
        level_trigger_set_script_use_by_refname("v23");
        level_trigger_set_exit_by_refname("v31");
        level_trigger_set_exit_by_refname("v32");
        level_trigger_set_exit_by_refname("v33");
        level_trigger_set_script_use_by_refname("v41");
        level_trigger_set_script_use_by_refname("v42");
        level_trigger_set_script_use_by_refname("v43");
        level_trigger_set_script_use_by_refname("v51");
        level_trigger_set_script_use_by_refname("v52");
    } else if (game_stage_get() >= 4) {
        level_trigger_set_script_use_by_refname("v11");
        level_trigger_set_script_use_by_refname("v12");
        level_trigger_set_script_use_by_refname("v13");
        level_trigger_set_exit_by_refname("v21");
        level_trigger_set_exit_by_refname("v22");
        level_trigger_set_exit_by_refname("v23");
        level_trigger_set_script_use_by_refname("v31");
        level_trigger_set_script_use_by_refname("v32");
        level_trigger_set_script_use_by_refname("v33");
        level_trigger_set_script_use_by_refname("v41");
        level_trigger_set_script_use_by_refname("v42");
        level_trigger_set_script_use_by_refname("v43");
        level_trigger_set_script_use_by_refname("v51");
        level_trigger_set_script_use_by_refname("v52");
    } else {
        level_trigger_set_exit_by_refname("v11");
        level_trigger_set_exit_by_refname("v12");
        level_trigger_set_exit_by_refname("v13");
        level_trigger_set_script_use_by_refname("v21");
        level_trigger_set_script_use_by_refname("v22");
        level_trigger_set_script_use_by_refname("v23");
        level_trigger_set_script_use_by_refname("v31");
        level_trigger_set_script_use_by_refname("v32");
        level_trigger_set_script_use_by_refname("v33");
        level_trigger_set_script_use_by_refname("v41");
        level_trigger_set_script_use_by_refname("v42");
        level_trigger_set_script_use_by_refname("v43");
        level_trigger_set_script_use_by_refname("v51");
        level_trigger_set_script_use_by_refname("v52");
    }
}

// 0x4edc30  Confidence: exact. level_script_table init of "sewerboss". The two exits are script-use
// triggers (the party cannot leave) only while the beast was found and not killed.
void ls_sewerboss_init()
{
    ls_sewerboss_avoid_spline = -1;
    level_music = music_find_by_name("sewer");
    if (savegame_state.get_flag(LEVEL_LPALACEINT, "got_ring_of_darkness")) {
        level_trigger_set_script_use("lpalaceint", NULL);
    } else if (savegame_state.get_flag(LEVEL_SEWER, "sewer_found_tb")
               && !savegame_state.get_flag(LEVEL_SEWER, "tb_killed")) {
        level_trigger_set_script_use("lpalaceint", NULL);
        level_trigger_set_script_use("sewer", NULL);
    } else {
        level_trigger_set_exit("lpalaceint", NULL);
        level_trigger_set_exit("sewer", NULL);
    }
    if (savegame_state.get_flag(LEVEL_SEWER, "tb_killed")) {
        script_npc_set_disabled("Broken Brass Golem#boss1", true);
        script_npc_set_disabled("Broken Brass Golem#boss2", true);
        script_npc_set_disabled("Brass Golem#block1", true);
        script_npc_set_disabled("Brass Golem#block2", true);
        script_npc_set_disabled("Iron Golem#boss", true);
    }
}

// 0x4edd30  Confidence: exact. level_script_table leave.
void ls_sewerboss_leave()
{
    if (ls_sewerboss_avoid_spline != -1)
        avoid_spline_free(ls_sewerboss_avoid_spline);
}

// 0x4edd50  Confidence: exact (ends in a tail jump to lsf_sewer_flag_found_crate). level_script_table frame
// of "sewer": the remarks run on the first visit only.
void ls_sewer_frame()
{
    if (ls_sewer_arrival == 1) {
        lsf_sewers_flag_where_comment();
        lsf_sewers_flag_passage_comment();
        lsf_sewers_flag_smell_comment();
        lsf_sewers_flag_doll_comment();
        lsf_sewers_flag_crate_comment();
        lsf_sewers_flag_different_comment();
        lsf_sewer_flag_scene_a();
        empty_frame_hook_40acc0();
        lsf_sewer_flag_found_crate();
    }
}

// 0x4edd90  Confidence: exact. Only sets the flag ($ControlA within 5).
void lsf_sewer_flag_scene_a()
{
    if (!savegame_state.get_flag(cur_level_index, "sewer_flag_scene_a")
        && script_player_near_navpoint("$ControlA", 5.0f))
        savegame_state.set_flag(cur_level_index, "sewer_flag_scene_a", true);
}

// 0x4edde0  Confidence: exact. The three thieves dead.
void lsf_sewer_flag_found_crate()
{
    if (!savegame_state.get_flag(cur_level_index, "sewer_flag_found_crate")
        && script_npc_is_dead("Thief#$npc092") && script_npc_is_dead("Thief#$npc093")
        && script_npc_is_dead("Thief#$npc094"))
        savegame_state.set_flag(cur_level_index, "sewer_flag_found_crate", true);
}

// 0x4ede50  Confidence: exact.
void lsf_sewers_flag_crate_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "sewers_flag_crate_comment")
        && script_player_near_navpoint("$npc100", 4.0f)) {
        savegame_state.set_flag(cur_level_index, "sewers_flag_crate_comment", true);
        script_party_member_say("Flece", XSTR("Hmm. What are those crates doing there?", 549), 5.0f);
    }
}

// 0x4edec0  Confidence: exact.
void lsf_sewers_flag_different_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "sewers_flag_different_comment")
        && script_player_near_navpoint("$npc099", 4.0f)) {
        savegame_state.set_flag(cur_level_index, "sewers_flag_different_comment", true);
        script_party_member_say("Flece", XSTR("The architecture looks different here.", 550), 5.0f);
    }
}

// 0x4edf30  Confidence: exact.
void lsf_sewers_flag_passage_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "sewers_flag_passage_comment")
        && script_player_near_navpoint("$npc098", 4.0f)) {
        savegame_state.set_flag(cur_level_index, "sewers_flag_passage_comment", true);
        script_party_member_say("Flece", XSTR("Where does this passage lead?", 551), 5.0f);
    }
}

// 0x4edfa0  Confidence: exact. When the bacite "doll" is dead.
void lsf_sewers_flag_doll_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "sewers_flag_doll_comment")
        && script_npc_is_dead("Brown Bacite#doll")) {
        savegame_state.set_flag(cur_level_index, "sewers_flag_doll_comment", true);
        script_party_member_say("Flece", XSTR("What's that on the ground?", 552), 5.0f);
    }
}

// 0x4ee000  Confidence: exact.
void lsf_sewers_flag_where_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "sewers_flag_where_comment")
        && script_player_near_navpoint("$npc097", 4.0f)) {
        savegame_state.set_flag(cur_level_index, "sewers_flag_where_comment", true);
        script_party_member_say("Flece", XSTR("Where are all the sewer cleaners?", 553), 5.0f);
    }
}

// 0x4ee070  Confidence: exact. Said for 4 s (the others 5 s).
void lsf_sewers_flag_smell_comment()
{
    if (!savegame_state.get_flag(cur_level_index, "sewers_flag_smell_comment")
        && script_player_near_navpoint("$npc096", 4.0f)) {
        savegame_state.set_flag(cur_level_index, "sewers_flag_smell_comment", true);
        script_party_member_say("Flece", XSTR("Ugh! And you thought pigs smelled bad.", 554), 4.0f);
    }
}

// 0x4ee0e0  Confidence: exact. level_script_table frame of "sewerboss". When the three boss golems are
// dead: the camera and the sewer music come back, then (once) tb_killed, the two block golems leave, the
// exits open, the spline goes and every tentacle sinks for 7 s.
void ls_sewerboss_frame()
{
    int i;

    lsf_sewer_found_tb();
    tentacle_beast_battle_update();
    if (ls_sewerboss_music_timer.valid() && ls_sewerboss_music_timer.elapsed()) {
        ls_sewerboss_music_timer.invalidate();
        music_play(music_find_by_name("Sewer Boss"), true);
    }
    if (ls_sewerboss_golems_dead() && !ls_sewerboss_golems_done) {
        game_camera.set_mode(4);
        ls_sewerboss_golems_done = true;
        music_play(music_find_by_name("Sewer"), true);
    }
    if (ls_sewerboss_golems_dead() && !savegame_state.get_flag(LEVEL_SEWER, "tb_killed")) {
        savegame_state.set_flag(LEVEL_SEWER, "tb_killed", true);
        script_npc_goto_label("Brass Golem#block1", 2);
        script_npc_goto_label("Brass Golem#block2", 2);
        level_trigger_set_exit("lpalaceint", NULL);
        level_trigger_set_exit("sewer", NULL);
        level_script_event_active = false;
        if (ls_sewerboss_avoid_spline != -1) {
            avoid_spline_free(ls_sewerboss_avoid_spline);
            ls_sewerboss_avoid_spline = -1;
        }
        for (i = 0; i < NUM_TENTACLES; i++)
            tentacle_begin_sink(&tentacle_beast_tentacles[i], 7000);
        tentacle_beast_battle_started = false;
    }
}

// 0x4ee220  Confidence: exact. The two broken brass golems and the iron golem (the beast's guards).
bool ls_sewerboss_golems_dead()
{
    if (script_npc_is_dead("Broken Brass Golem#boss1") && script_npc_is_dead("Broken Brass Golem#boss2")
        && script_npc_is_dead("Iron Golem#boss"))
        return true;
    return false;
}

// 0x4ee260  Confidence: exact. $tbintro within 10: TB-Intro, the exits become script-use triggers.
void lsf_sewer_found_tb()
{
    if (!savegame_state.get_flag(LEVEL_SEWER, "sewer_found_tb") && script_player_near_navpoint("$tbintro", 10.0f)) {
        script_play_cutscene("TB-Intro", ls_sewer_tb_intro_cs_end);
        savegame_state.set_flag(LEVEL_SEWER, "sewer_found_tb", true);
        level_trigger_set_script_use("lpalaceint", NULL);
        level_trigger_set_script_use("sewer", NULL);
    }
}

// 0x4ee2d0  Confidence: exact (the two loops' form). End of TB-Intro: the four tentacles go to their ready
// stance and idle, the battle starts with all four up, every party member drops its prop and gets ready, the
// boss music follows in 0.5 s, the avoidance spline and the golems' first labels.
void ls_sewer_tb_intro_cs_end()
{
    int i;
    Player *p;
    Character *c;

    for (i = 0; i < NUM_TENTACLES; i++) {
        c = tentacle_beast_tentacles[i].character;
        c->flags |= CF_COMBAT_READY;
        c->ai.go_idle();
    }
    tentacle_beast_tentacles_up = 4;
    tentacle_beast_battle_started = true;
    game_camera.set_mode(3);
    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        c->prop_tag = 0;
        c->flags |= CF_COMBAT_READY;
    }
    ls_sewerboss_music_timer.SetTimeout(500);
    ls_sewerboss_golems_done = false;
    ls_sewer_create_avoid_spline();
    level_script_event_active = true;
    script_npc_goto_label("Broken Brass Golem#boss1", 1);
    script_npc_goto_label("Broken Brass Golem#boss2", 1);
    script_npc_goto_label("Brass Golem#block1", 1);
    script_npc_goto_label("Brass Golem#block2", 1);
    script_npc_goto_label("Iron Golem#boss", 1);
}

// 0x4ee3c0  Confidence: likely (the four corner locals and the store order are a guess). The avoidance
// spline around the beast's pool: two segments (x 234.7 at z -155 .. -145, x 258 at z -145 .. -156) given
// twice, with cell bytes (1, 2) and (2, 2).
void ls_sewer_create_avoid_spline()
{
    avoid_spline_def def;
    vector2 a, b, c, d;

    a.x = 258.0f;
    a.y = -145.0f;
    b.x = 258.0f;
    b.y = -156.0f;
    c.x = 234.7f;
    c.y = -155.0f;
    d.x = 234.7f;
    d.y = -145.0f;
    def.cell_x[0] = 1;
    def.cell_y[0] = 2;
    def.p0[0] = c;
    def.p1[0] = d;
    def.cell_x[1] = 1;
    def.cell_y[1] = 2;
    def.p0[1] = a;
    def.p1[1] = b;
    def.cell_x[2] = 2;
    def.cell_y[2] = 2;
    def.p0[2] = c;
    def.p1[2] = d;
    def.cell_x[3] = 2;
    def.cell_y[3] = 2;
    def.p0[3] = a;
    def.p1[3] = b;
    def.count = 4;
    ls_sewerboss_avoid_spline = avoid_spline_create(&def);
}

// 0x4ee4f0  Confidence: exact. level_script_table use (both records).
char ls_sewer_use(const char *name)
{
    if (!_stricmp(name, "S1-CRmB-LevB02")) {
        door_apply_saved_state("$GateB2", true);
        return 0;
    }
    if (!_stricmp(name, "lpalaceint")) {
        script_party_member_say("Joseph", XSTR("The lift doesn't work.", 555), 5.0f);
        return 0;
    }
    if (!_stricmp(name, "lenele3d"))
        script_party_member_say("Joseph", XSTR("That passage is blocked.", 556), 5.0f);
    return 0;
}

// 0x4ee580  Confidence: exact. level_script_table action: the gate wheels of the conversation scripts.
void ls_sewer_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "OpenGateA1")) {
        door_apply_saved_state("$GateA1", true);
        door_apply_saved_state("$GateB1", false);
        door_apply_saved_state("ControlWheelA01", true);
    }
    if (!_stricmp(action->name, "OpenGateB1")) {
        door_apply_saved_state("$GateB1", true);
        door_apply_saved_state("$GateA1", false);
        door_apply_saved_state("ControlWheelA01", false);
    }
    if (!_stricmp(action->name, "OpenGateA2")) {
        door_apply_saved_state("$GateA2", true);
        door_apply_saved_state("ControlWheelA02", true);
    }
    if (!_stricmp(action->name, "OpenGateA3")) {
        door_apply_saved_state("$GateA3", true);
        door_apply_saved_state("ControlWheelA03", true);
    }
    if (!_stricmp(action->name, "OpenGateC1")) {
        door_apply_saved_state("$GateC1", true);
        door_apply_saved_state("ControlWheelC01", true);
    }
    if (!_stricmp(action->name, "OpenGateC3")) {
        door_apply_saved_state("$GateC3", true);
        door_apply_saved_state("ControlWheelC02", true);
    }
    if (!_stricmp(action->name, "OpenAllGates")) {
        door_apply_saved_state("$GateA1", true);
        door_apply_saved_state("$GateB1", true);
        door_apply_saved_state("$GateA2", true);
        door_apply_saved_state("$GateA3", true);
    }
    if (!_stricmp(action->name, "CervalHostile"))
        script_npc_goto_label("Cerval", 1);
}
