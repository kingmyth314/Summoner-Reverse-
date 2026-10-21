// level_script_ikaemosu.cpp - Sum.exe v1.40, code 0x4f9ec0-0x4fa3f0 (levelscripts\Scripts\
// level_script_ikaemosu.cpp; strings.tbl ids 484-488 are this unit's). Written from the Sum.exe
// disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function:
// exact (control flow, calls, constants and globals match), likely (one detail of statement order or
// expression form is a guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "IkaemosTopInt" (the upper Ikaemos palace, stage 5: the four dagger slots
// of the statue that open Door02-01, the Ikaemos Priest King (IkmsPKing-Intro) and the Ring of Stone).
// No static initialiser.
// Data: .bss 0x257ecfc-0x257ecfe. No .data of its own (the literals only).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/localize/localize.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

void ls_ikaemostopint_main_door();
void lsf_ikaemos_found_boss();
void ls_ikaemos_pking_intro_cs_end();
void lsf_ikaemos_boss_killed();

// ---- .bss (0x257ecfc-0x257ecfe) ------------------------------------------------------------------------
bool ls_ikaemostopint_found_boss_done;               // 0x257ecfc
bool ls_ikaemostopint_boss_killed_done;              // 0x257ecfd
bool ls_ikaemostopint_joseph_spoke;                  // 0x257ecfe

// 0x4f9ec0  Confidence: likely (the binary keeps the constant 1 in EBX; 8966 uses immediates). level_script_table init (stage 5). Both music flags are set to 1 on every
// entry (`mov ebx, 1` before the flag tests, stored as bl at 0x4f9f2e), so the boss music of
// lsf_ikaemos_found_boss and the restore of lsf_ikaemos_boss_killed never run (see NOTES).
void ls_ikaemostopint_init()
{
    level_music = music_find_by_name("IkaemosPalace");
    game_stage_set(5);
    if (savegame_state.get_flag(cur_level_index, "ikaemos_main_door"))
        door_apply_saved_state("Door02-01", true);
    if (savegame_state.get_flag(cur_level_index, "ikaemos_boss_killed"))
        script_npc_set_disabled("Ikaemos Priest King", true);
#ifdef FIXED_BUILD
    // part of FIXED_BUILD K130 (the K150 half of BossMusic)
    ls_ikaemostopint_found_boss_done = savegame_state.get_flag(cur_level_index, "ikaemos_boss_killed") != 0;
    ls_ikaemostopint_boss_killed_done = ls_ikaemostopint_found_boss_done;
#else
    ls_ikaemostopint_found_boss_done = true;
    ls_ikaemostopint_boss_killed_done = true;
#endif
    ls_ikaemostopint_joseph_spoke = false;
}

// 0x4f9f50  Confidence: exact. level_script_table frame; Joseph's remark at $ipktrigger (5) once a visit.
void ls_ikaemostopint_frame()
{
    ls_ikaemostopint_main_door();
    lsf_ikaemos_found_boss();
    lsf_ikaemos_boss_killed();
    if (!ls_ikaemostopint_joseph_spoke && script_player_near_navpoint("$ipktrigger", 5.0f)) {
        ls_ikaemostopint_joseph_spoke = true;
        script_party_member_say("Joseph", XSTR("There's something strange about this statue.", 485), 5.0f);
    }
}

// 0x4f9fa0  Confidence: exact. All four slots used: Door02-01 opens (ikaemos_main_door).
void ls_ikaemostopint_main_door()
{
    if (!savegame_state.get_flag(cur_level_index, "ikaemos_main_door")
        && savegame_state.get_flag(cur_level_index, "ikaemos_slot1")
        && savegame_state.get_flag(cur_level_index, "ikaemos_slot2")
        && savegame_state.get_flag(cur_level_index, "ikaemos_slot3")
        && savegame_state.get_flag(cur_level_index, "ikaemos_slot4")) {
        door_apply_saved_state("Door02-01", true);
        savegame_state.set_flag(cur_level_index, "ikaemos_main_door", true);
    }
}

// 0x4fa050  Confidence: exact (VC6 byte match, match14; was exact). The boss music after IkmsPKing-Intro; the intro at $ipktrigger (3) once the
// main door is open.
void lsf_ikaemos_found_boss()
{
    if (!ls_ikaemostopint_found_boss_done && savegame_state.get_flag(cur_level_index, "ikaemos_found_boss")
        && !(game_flags & GF_CUTSCENE)) {
        ls_ikaemostopint_found_boss_done = true;
        int music = music_find_by_name("Boss");
        music_play(music, true);
    }
    if (!savegame_state.get_flag(cur_level_index, "ikaemos_found_boss")
        && script_player_near_navpoint("$ipktrigger", 3.0f)
        && savegame_state.get_flag(cur_level_index, "ikaemos_main_door")) {
        savegame_state.set_flag(cur_level_index, "ikaemos_found_boss", true);
        script_play_cutscene("IkmsPKing-Intro", ls_ikaemos_pking_intro_cs_end);
    }
}

// 0x4fa110  Confidence: exact. End of IkmsPKing-Intro: the king goes, the exit is closed.
void ls_ikaemos_pking_intro_cs_end()
{
    script_npc_send_go("Ikaemos Priest King");
    level_script_event_active = true;
    level_trigger_set_script_use("ikaemosext", NULL);
}

// 0x4fa140  Confidence: exact. The level music after the king; the king dead with the Ring of Stone in the
// party: ikaemos_boss_killed, got_ring_of_stone, the exit opens, Summon Golem.
void lsf_ikaemos_boss_killed()
{
    if (!ls_ikaemostopint_boss_killed_done && savegame_state.get_flag(cur_level_index, "ikaemos_boss_killed")
        && !(game_flags & GF_CUTSCENE)) {
        ls_ikaemostopint_boss_killed_done = true;
        music_play(level_music, true);
    }
    if (!savegame_state.get_flag(cur_level_index, "ikaemos_boss_killed")
        && savegame_state.get_flag(cur_level_index, "ikaemos_found_boss")
        && script_npc_is_dead("Ikaemos Priest King") && party_count_item_by_name("Ring of Stone")) {
        savegame_state.set_flag(cur_level_index, "ikaemos_boss_killed", true);
        savegame_state.set_flag(cur_level_index, "got_ring_of_stone", true);
        level_script_event_active = false;
        level_trigger_set_exit("ikaemosext", NULL);
        hud_show_message(XSTR("Joseph acquired Summon Golem!", 484));
    }
}

// 0x4fa230  Confidence: exact (the four slot cases share one set_flag call, 0x4fa36d). level_script_table
// use. A slot is filled by using it (no dagger test here); the statue and the box give the hint; the exit
// refuses until the ring is taken.
char ls_ikaemostopint_use(const char *name)
{
    if (!_stricmp(name, "Puzzle-Slot01")) {
        if (savegame_state.get_flag(cur_level_index, "ikaemos_slot1"))
            script_party_member_say("Joseph", XSTR("We've unlocked it already.", 486), 5.0f);
        savegame_state.set_flag(cur_level_index, "ikaemos_slot1", true);
        return 1;
    }
    if (!_stricmp(name, "Puzzle-Slot02")) {
        if (savegame_state.get_flag(cur_level_index, "ikaemos_slot2"))
            script_party_member_say("Joseph", XSTR("We've unlocked it already.", 486), 5.0f);
        savegame_state.set_flag(cur_level_index, "ikaemos_slot2", true);
        return 1;
    }
    if (!_stricmp(name, "Puzzle-Slot03")) {
        if (savegame_state.get_flag(cur_level_index, "ikaemos_slot3"))
            script_party_member_say("Joseph", XSTR("We've unlocked it already.", 486), 5.0f);
        savegame_state.set_flag(cur_level_index, "ikaemos_slot3", true);
        return 1;
    }
    if (!_stricmp(name, "Puzzle-Slot04")) {
        if (savegame_state.get_flag(cur_level_index, "ikaemos_slot4"))
            script_party_member_say("Joseph", XSTR("We've unlocked it already.", 486), 5.0f);
        savegame_state.set_flag(cur_level_index, "ikaemos_slot4", true);
        return 1;
    }
    if (!_stricmp(name, "Bellias02") || !_stricmp(name, "Box01")) {
        script_party_member_say("Joseph", XSTR("There are four dagger slots at the base of this statue.", 487), 5.0f);
        return 1;
    }
    if (!_stricmp(name, "ikaemosext"))
        script_party_member_say("Joseph", XSTR("I need to get the ring of stone!", 488), 5.0f);
    return 0;
}
