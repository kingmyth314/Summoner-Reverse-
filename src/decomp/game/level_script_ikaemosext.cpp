// level_script_ikaemosext.cpp - Sum.exe v1.40, code 0x4f9e50-0x4f9eb0. Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "IkaemosExt" (index 0xd; outside the Ikaemos palace, first visit: the
// Nar-Ikaemos narration once). FILE NAME NOT PROVEN (D5300): no strings.tbl block, no path string, no .bss;
// its literals ("Nar-Ikaemos" 0x5a4530, "ikaemos_intro" 0x5a453c) lie between level_script_khosani_lab.cpp's
// and level_script_ikaemosu.cpp's (source-map.md). The name follows the level_script_<record>.cpp
// convention.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h.

#include "engine/gamesound/gamesound.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

// 0x4f9e50  Confidence: exact. level_script_table init (stage 5).
void ls_ikaemosext_init()
{
    level_music = music_find_by_name("IkaemosExt");
    game_stage_set(5);
}

// 0x4f9e70  Confidence: exact. level_script_table frame: the narration on the first frame.
void ls_ikaemosext_frame()
{
    if (!savegame_state.get_flag(cur_level_index, "ikaemos_intro")) {
        savegame_state.set_flag(cur_level_index, "ikaemos_intro", true);
        script_play_cutscene("Nar-Ikaemos", 0);
    }
}
