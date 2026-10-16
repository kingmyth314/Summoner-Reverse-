// level_script_ikaemosext2.cpp - Sum.exe v1.40, code 0x4ef120-0x4ef180. Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "IkaemosExt2" (index 0x26; outside the Ikaemos palace, act 4: the
// Nar-IkaemosAct4 narration once). FILE NAME NOT PROVEN (D5300, D5602): no strings.tbl block, no path string,
// no .bss; its two literals ("Nar-IkaemosAct4" 0x5a1b18, "ikaemos2_intro" 0x5a1b28) follow
// level_script_ikaemosl2.cpp's, so it may be that file's tail (source-map.md). The name follows the
// level_script_<record>.cpp convention.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h.

#include "engine/gamesound/gamesound.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

// 0x4ef120  Confidence: exact. level_script_table init (stage 15; the stage is set before the music here).
void ls_ikaemosext2_init()
{
    game_stage_set(0xf);
    level_music = music_find_by_name("IkaemosExt");
}

// 0x4ef140  Confidence: exact. level_script_table frame: the act 4 narration on the first frame.
void ls_ikaemosext2_frame()
{
    if (!savegame_state.get_flag(cur_level_index, "ikaemos2_intro")) {
        savegame_state.set_flag(cur_level_index, "ikaemos2_intro", true);
        script_play_cutscene("Nar-IkaemosAct4", 0);
    }
}
