// level_script_lenele1ab.cpp - Sum.exe v1.40, code 0x4ef850-0x4efa20. Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "lenele1ab" (index 0x24; the Lenele marketplace, second half; frame, leave
// and action are the folded empty hooks, use is ls_test_use). FILE NAME NOT PROVEN (D5300): no strings.tbl
// block, no path string, no .bss; its literals (0x5a1cb4-0x5a1d04) lie between level_script_templeint2.cpp's
// and level_script_lenele1aa.cpp's (source-map.md). The name follows the level_script_<record>.cpp
// convention. "lenele marketplace" and the "vNN" names other than "v14", "v24", "v34" are pooled copies of
// earlier units.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h.

#include <string.h>
#include "engine/gamesound/gamesound.h"
#include "summoner/gameplay.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

#define LEVEL_LPALACEINT   0x18                      // "lpalaceint_mq_gave_lanterns" (0x4ef8a7)

// 0x4ef850  Confidence: exact (VC6 byte match; the three game_stage_set calls, and the two script_npc_set_disabled
// calls, are one call each with the argument pushed per branch: tail merging; the >= 4 and < 4 branches
// share the four "v3x" script-use calls). level_script_table init: the
// stage from the script version (_v3 6, _v2 4, else 0); Torvel is disabled once the lanterns were given in
// LPalaceInt, his #part2 copy before; the doors v11..v34 open row by row with the stage.
void ls_lenele1ab_init()
{
    level_music = music_find_by_name("lenele marketplace");
    if (!_stricmp(level_script_name, "lenele1ab_v3"))
        game_stage_set(6);
    else if (!_stricmp(level_script_name, "lenele1ab_v2"))
        game_stage_set(4);
    else
        game_stage_set(0);

    if (savegame_state.get_flag(LEVEL_LPALACEINT, "lpalaceint_mq_gave_lanterns"))
        script_npc_set_disabled("Torvel", true);
    else
        script_npc_set_disabled("Torvel#part2", true);

    if (game_stage_get() >= 6) {
        level_trigger_set_script_use_by_refname("v11");
        level_trigger_set_script_use_by_refname("v12");
        level_trigger_set_script_use_by_refname("v13");
        level_trigger_set_script_use_by_refname("v14");
        level_trigger_set_script_use_by_refname("v21");
        level_trigger_set_script_use_by_refname("v22");
        level_trigger_set_script_use_by_refname("v23");
        level_trigger_set_script_use_by_refname("v24");
        level_trigger_set_exit_by_refname("v31");
        level_trigger_set_exit_by_refname("v32");
        level_trigger_set_exit_by_refname("v33");
        level_trigger_set_exit_by_refname("v34");
    } else if (game_stage_get() >= 4) {
        level_trigger_set_script_use_by_refname("v11");
        level_trigger_set_script_use_by_refname("v12");
        level_trigger_set_script_use_by_refname("v13");
        level_trigger_set_script_use_by_refname("v14");
        level_trigger_set_exit_by_refname("v21");
        level_trigger_set_exit_by_refname("v22");
        level_trigger_set_exit_by_refname("v23");
        level_trigger_set_exit_by_refname("v24");
        level_trigger_set_script_use_by_refname("v31");
        level_trigger_set_script_use_by_refname("v32");
        level_trigger_set_script_use_by_refname("v33");
        level_trigger_set_script_use_by_refname("v34");
    } else {
        level_trigger_set_exit_by_refname("v11");
        level_trigger_set_exit_by_refname("v12");
        level_trigger_set_exit_by_refname("v13");
        level_trigger_set_exit_by_refname("v14");
        level_trigger_set_script_use_by_refname("v21");
        level_trigger_set_script_use_by_refname("v22");
        level_trigger_set_script_use_by_refname("v23");
        level_trigger_set_script_use_by_refname("v24");
        level_trigger_set_script_use_by_refname("v31");
        level_trigger_set_script_use_by_refname("v32");
        level_trigger_set_script_use_by_refname("v33");
        level_trigger_set_script_use_by_refname("v34");
    }
}
