// level_script_iona_ext2.cpp - Sum.exe v1.40, code 0x4f4550-0x4f4905 (levelscripts\Scripts\
// level_script_iona_ext2.cpp; strings.tbl id 489 is this unit's). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "IonaExt02" (Iona in the snow; the variant Ionaext02_v2 is the stage 16
// visit with Luminar the light guardian and the Ring of Light; the Great Library cutscene after Yago is
// found). Static initialiser table entry 485 (0x588798) is this unit's. The leave hook (snow_stop) was
// folded into lenele3d's identical one, 0x4f0f00 ls_ionaext02_leave (D2700); it is defined there.
// Data: .bss 0x257ea58-0x257ea5d. No .data of its own (the literals only).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h, the helpers at the top of this file.

#include <string.h>
#include "vsdk/os/timestamp.h"
#include "vsdk/localize/localize.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

#define LEVEL_KHOSANISTRNG 0x17                      // "open_yago" (0x4f4633)

void ls_ionaext02_luminar_intro_cs_end();
void ls_ionaext02_luminar_death_cs_end();
void ls_ionaext02_rosalind_remark();

// ---- .bss (0x257ea58-0x257ea5d) ------------------------------------------------------------------------
// 0x4f4550  Confidence: exact. static_init_ls_ionaext02_timer: "jmp 0x4f4560" (initialiser entry 485).
// 0x4f4560  Confidence: exact. init_ls_ionaext02_timer: "mov ecx, 0x257ea58 / jmp GameTimer::GameTimer".
GameTimer ls_ionaext02_timer;                        // 0x257ea58
bool ls_ionaext02_trapped_luminar_done;              // 0x257ea5c
bool ls_ionaext02_luminar_intro_done;                // 0x257ea5d

// 0x4f4570  Confidence: exact (the 20 s SetTimeout is emitted at the end of both trapped_luminar paths, at
// 0x4f45e2 and at 0x4f4614: written once after the if / else, tail-duplicated). level_script_table init.
// Ionaext02_v2 (stage 16): until Luminar is trapped his intro is forgotten (luminar_intro cleared, as Titus's in ikaemosl2); the first remark in 20 s. The
// first visit (stage 15): Yago is there only once open_yago is set in KhosaniStrng.
void ls_ionaext02_init()
{
    snow_start();
    ls_ionaext02_luminar_intro_done = false;
    ls_ionaext02_trapped_luminar_done = false;
    level_music = music_find_by_name("IonaExt2");
    if (!_stricmp(level_script_name, "Ionaext02_v2")) {
        game_stage_set(0x10);
        if (!savegame_state.get_flag(cur_level_index, "trapped_luminar")) {
            savegame_state.set_flag(cur_level_index, "luminar_intro", false);
        } else {
            script_npc_set_disabled("Luminar", true);
            ls_ionaext02_luminar_intro_done = true;
            ls_ionaext02_trapped_luminar_done = true;
        }
        ls_ionaext02_timer.SetTimeout(20000);
        return;
    }
    game_stage_set(0xf);
    if (!savegame_state.get_flag(LEVEL_KHOSANISTRNG, "open_yago"))
        script_npc_set_disabled("Yago", true);
}

// 0x4f4650  Confidence: exact. level_script_table frame. Luminar-Intro when Luminar is within 20, the
// Luminar music after it; without the Ring of Light Rosalind's remark every 25 s; Luminar dead with the
// ring: his loot, Luminar-Death, trapped_luminar, the level music. Once Yago is found (and no conversation
// runs) ROSL-GreatLibrary plays once.
void ls_ionaext02_frame()
{
    if (script_npc_near_player("Luminar", 20.0f) && !savegame_state.get_flag(cur_level_index, "luminar_intro")) {
        script_play_cutscene("Luminar-Intro", ls_ionaext02_luminar_intro_cs_end);
        savegame_state.set_flag(cur_level_index, "luminar_intro", true);
    }
    if (!ls_ionaext02_luminar_intro_done && savegame_state.get_flag(cur_level_index, "luminar_intro")
        && !(game_flags & GF_CUTSCENE)) {
        ls_ionaext02_luminar_intro_done = true;
        int music = music_find_by_name("Luminar");
        music_play(music, true);
    }
    if (!savegame_state.get_flag(cur_level_index, "trapped_luminar")) {
        if (savegame_state.get_flag(cur_level_index, "luminar_intro") && !party_count_item_by_name("Ring of Light")) {
            ls_ionaext02_rosalind_remark();
        } else if (savegame_state.get_flag(cur_level_index, "luminar_intro") && script_npc_is_dead("Luminar")
                   && party_count_item_by_name("Ring of Light")) {
            script_npc_drop_loot("Luminar");
            script_play_cutscene("Luminar-Death", ls_ionaext02_luminar_death_cs_end);
            savegame_state.set_flag(cur_level_index, "trapped_luminar", true);
        }
    }
    if (!ls_ionaext02_trapped_luminar_done && savegame_state.get_flag(cur_level_index, "trapped_luminar")
        && !(game_flags & GF_CUTSCENE)) {
        ls_ionaext02_trapped_luminar_done = true;
        music_play(music_find_by_name("IonaExt2"), true);
    }
    if (!(game_flags & GF_CONVERSATION) && savegame_state.get_flag(cur_level_index, "found_yago")
        && !savegame_state.get_flag(cur_level_index, "rosl_gl")) {
        savegame_state.set_flag(cur_level_index, "rosl_gl", true);
        script_play_cutscene("ROSL-GreatLibrary", NULL);
    }
}

// 0x4f4860  Confidence: exact. End of Luminar-Intro: Luminar goes, first remark in 30 s.
void ls_ionaext02_luminar_intro_cs_end()
{
    script_npc_send_go("Luminar");
    ls_ionaext02_timer.SetTimeout(30000);
    level_script_event_active = true;
}

// 0x4f4890  Confidence: exact. End of Luminar-Death: the level music again (0.2 s fade).
void ls_ionaext02_luminar_death_cs_end()
{
    script_npc_set_disabled("Luminar", true);
    level_script_event_active = false;
    music_stop(200);
    music_play(music_find_by_name("IonaExt2"), true);
}

// 0x4f48d0  Confidence: exact. Every 25 s while Luminar cannot be hurt.
void ls_ionaext02_rosalind_remark()
{
    if (ls_ionaext02_timer.elapsed()) {
        script_party_member_say("Rosalind", XSTR("We're not hurting him at all!", 489), 5.0f);
        ls_ionaext02_timer.SetTimeout(25000);
    }
}
