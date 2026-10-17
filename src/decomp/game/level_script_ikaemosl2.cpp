// level_script_ikaemosl2.cpp - Sum.exe v1.40, code 0x4eee40-0x4ef115 (levelscripts\Scripts\
// level_script_ikaemosl2.cpp; strings.tbl id 483 is this unit's). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "IkaemosBottomInt2" (the lower Ikaemos palace, second visit: Titus the
// stone guardian, the Ring of Stone). Static initialiser table entry 478 (0x58877c) is this unit's.
// Data: .bss 0x257e9a8-0x257e9b0. No .data of its own (the literals only).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "engine/ai/boss_ai.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

void ls_ikaemosbottomint2_titus_intro_cs_end();
void ls_ikaemosbottomint2_titus_death_cs_end();
void ls_ikaemosbottomint2_jekhar_remark();

// ---- .bss (0x257e9a8-0x257e9b0) ------------------------------------------------------------------------
bool ls_ikaemosbottomint2_trapped_titus_done;        // 0x257e9a8
// 0x4eee40  Confidence: exact. static_init_ls_ikaemosbottomint2_timer: "jmp 0x4eee50" (initialiser entry 478).
// 0x4eee50  Confidence: exact. init_ls_ikaemosbottomint2_timer: "mov ecx, 0x257e9ac / jmp GameTimer::GameTimer".
GameTimer ls_ikaemosbottomint2_timer;                // 0x257e9ac
bool ls_ikaemosbottomint2_titus_intro_done;          // 0x257e9b0

// 0x4eee60  Confidence: exact. level_script_table init (stage 15). Until Titus is trapped the intro is
// forgotten on every entry (titus_intro cleared), so Titus-Intro plays again on the next frame.
void ls_ikaemosbottomint2_init()
{
    level_music = music_find_by_name("IkaemosPalace");
    game_stage_set(0xf);
    ls_ikaemosbottomint2_timer.SetTimeout(20000);
    if (!savegame_state.get_flag(cur_level_index, "trapped_titus")) {
        savegame_state.set_flag(cur_level_index, "titus_intro", false);
        titus_combat_enabled = false;
        ls_ikaemosbottomint2_titus_intro_done = false;
        ls_ikaemosbottomint2_trapped_titus_done = false;
        return;
    }
    script_npc_set_disabled("Titus", true);
    ls_ikaemosbottomint2_titus_intro_done = true;
    ls_ikaemosbottomint2_trapped_titus_done = true;
}

// 0x4eeef0  Confidence: exact (VC6 byte match, match14; was exact). level_script_table frame. Titus-Intro plays at once (no navpoint), the
// Titus music after it; without the Ring of Stone Jekhar repeats his remark every 25 s; Titus dead with the
// ring in the party: his loot, Titus-Death, trapped_titus, and the palace music back.
void ls_ikaemosbottomint2_frame()
{
    if (!savegame_state.get_flag(cur_level_index, "titus_intro")) {
        script_play_cutscene("Titus-Intro", ls_ikaemosbottomint2_titus_intro_cs_end);
        savegame_state.set_flag(cur_level_index, "titus_intro", true);
    }
    if (!ls_ikaemosbottomint2_titus_intro_done && savegame_state.get_flag(cur_level_index, "titus_intro")
        && !(game_flags & GF_CUTSCENE)) {
        ls_ikaemosbottomint2_titus_intro_done = true;
        int music = music_find_by_name("Titus");
        music_play(music, true);
    }
    if (!savegame_state.get_flag(cur_level_index, "trapped_titus")) {
        if (savegame_state.get_flag(cur_level_index, "titus_intro") && !party_count_item_by_name("Ring of Stone")) {
            ls_ikaemosbottomint2_jekhar_remark();
        } else if (savegame_state.get_flag(cur_level_index, "titus_intro") && script_npc_is_dead("Titus")
                   && party_count_item_by_name("Ring of Stone")) {
            script_npc_drop_loot("Titus");
            script_play_cutscene("Titus-Death", ls_ikaemosbottomint2_titus_death_cs_end);
            savegame_state.set_flag(cur_level_index, "trapped_titus", true);
        }
    }
    if (!ls_ikaemosbottomint2_trapped_titus_done && savegame_state.get_flag(cur_level_index, "trapped_titus")
        && !(game_flags & GF_CUTSCENE)) {
        ls_ikaemosbottomint2_trapped_titus_done = true;
        music_play(music_find_by_name("IkaemosPalace"), true);
    }
}

// 0x4ef090  Confidence: exact. End of Titus-Intro: Titus goes, fights, first remark in 30 s.
void ls_ikaemosbottomint2_titus_intro_cs_end()
{
    script_npc_send_go("Titus");
    titus_combat_enabled = true;
    ls_ikaemosbottomint2_timer.SetTimeout(30000);
    level_script_event_active = true;
}

// 0x4ef0c0  Confidence: exact. End of Titus-Death.
void ls_ikaemosbottomint2_titus_death_cs_end()
{
    script_npc_set_disabled("Titus", true);
    level_script_event_active = false;
}

// 0x4ef0e0  Confidence: exact. Every 25 s while Titus cannot be hurt.
void ls_ikaemosbottomint2_jekhar_remark()
{
    if (ls_ikaemosbottomint2_timer.elapsed()) {
        script_party_member_say("Jekhar", XSTR("He's immune to our attacks!", 483), 5.0f);
        ls_ikaemosbottomint2_timer.SetTimeout(25000);
    }
}
