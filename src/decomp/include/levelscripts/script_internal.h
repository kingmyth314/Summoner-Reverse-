// levelscripts/script_internal.h - the script object parser's navpoint lookup (sum.symbols module
// script_internal.cpp, 0x4dc600; directory guessed, decomp34). Not original source.
#ifndef LEVELSCRIPTS_SCRIPT_INTERNAL_H
#define LEVELSCRIPTS_SCRIPT_INTERNAL_H

#include "vsdk/vsdk.h"

class vector3;
class matrix;

// (name, out position, out orientation): _stricmp over the level navpoints (0x24e4950, count 0x2539f2c)
// and the script navpoints (0x256b750, count 0x257e96c); false when not found (xor al, al at 0x4dc6db).
bool navpoint_get_position(const char *name, vector3 *pos, matrix *orient);   // 0x4dc600

#endif
