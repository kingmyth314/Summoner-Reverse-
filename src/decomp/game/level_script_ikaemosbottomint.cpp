// level_script_ikaemosbottomint.cpp - Sum.exe v1.40, code 0x4fa400-0x4fa419. Written from the Sum.exe
// disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact
// (control flow, calls, constants and globals match), likely (one detail of statement order or expression form
// is a guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "IkaemosBottomInt" (index 0xb; the lower Ikaemos palace, first visit; frame,
// leave, use and action are the folded empty hooks). FILE NAME NOT PROVEN (D5300, D5303): no strings.tbl
// block, no path string, no .bss and no literal of its own ("IkaemosPalace" is pooled); it lies between
// level_script_ikaemosu.cpp (IkaemosTopInt) and rand-hills01, and may be the tail of either. By the pair
// ikaemosu.cpp / ikaemosl2.cpp (IkaemosBottomInt2) its original file may have been level_script_ikaemosl.cpp;
// that is a guess, so the name follows the level_script_<level>.cpp convention instead.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h.

#include "engine/gamesound/gamesound.h"
#include "levelscripts/level_scripts.h"

// 0x4fa400  Confidence: exact. level_script_table init (stage 5).
void ls_ikaemosbottomint_init()
{
    level_music = music_find_by_name("IkaemosPalace");
    game_stage_set(5);
}
