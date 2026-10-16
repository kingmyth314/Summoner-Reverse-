// level_script_rand_desert.cpp - Sum.exe v1.40, code 0x4f4910-0x4f4922. Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact (control
// flow, calls, constants and globals match), likely (one detail of statement order or expression form is a
// guess), unsure (the shape itself is a guess).
//
// One level_script_table record: "Rand-Desert" (index 0x15; a random-encounter desert; the other hooks are
// the folded empty one or none). FILE NAME NOT PROVEN (D5300): no strings.tbl block, no path string, no
// .bss; its one literal "rand-desert" (0x5a3054) lies between level_script_iona_ext2.cpp's and
// level_script_liangshan.cpp's (source-map.md). The name follows the level_script_<record>.cpp convention
// (lower case, the record's dash as an underscore). Unlike the other random levels it sets no game stage.
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h.

#include "engine/gamesound/gamesound.h"
#include "levelscripts/level_scripts.h"

// 0x4f4910  Confidence: exact. level_script_table init: the music only.
void ls_rand_desert_init()
{
    level_music = music_find_by_name("rand-desert");
}
