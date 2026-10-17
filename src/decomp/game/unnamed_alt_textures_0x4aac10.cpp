// unnamed_alt_textures_0x4aac10.cpp - Sum.exe v1.40, code 0x4aac10-0x4ab0ef (alt_textures.tbl: the
// alternate texture sets of the character models).
// FILE NAME NOT KNOWN (placeholder). No assert path, no strings.tbl block, no reference name; module "-".
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit: multi_door.cpp ends with mp_send_door_lockpick (0x4aab90, code to 0x4aac05); the join / tracker
// unit starts with its initialiser at 0x4ab0f0 (D5206). Data: .data 0x59d39c-0x59d3c3 ("+Textures:",
// "$Base:", "alt_textures.tbl"; "#End" 0x58bd08 and "" 0x5bd0d0 are pooled), right after multi_door.cpp's
// 0x59d34c-0x59d39b; .bss 0x2476bb8-0x24794bb (the avoidance unit's link records end at 0x2476bb7).
//
// The table: "$Base: <character>" followed by "+Textures: <set name> <texture> <texture> ...". Each base
// gets an array of sets (alt_texture_set, 0x2d4 bytes); characterinfo_parse_table looks the sets up by
// the character name (alt_textures_find) and by "<character>" + set name (alt_textures_find_entry), and
// living_entity_init loads the bitmaps.

#include <string.h>
#include "vsdk/parse/parse.h"
#include "vsdk/os/memory.h"
#include "engine/characterinfo/characterinfo.h"
#include "summoner/summoner.h"

#define MAX_ALT_TEXTURE_BASES 256
#define ALT_TEXTURE_BASE_LEN  0x21
#define MAX_ALT_TEXTURES      15         // alt_texture_set::bitmaps / names (not checked, D5207)

// ---- .bss (0x2476bb8-0x24794bb), address order. Names ours. ---------------------------------------------
alt_texture_set *alt_texture_lists[MAX_ALT_TEXTURE_BASES];          // 0x2476bb8 operator new'd arrays
char alt_texture_bases[MAX_ALT_TEXTURE_BASES][ALT_TEXTURE_BASE_LEN]; // 0x2476fb8 "$Base:" names
int  alt_texture_counts[MAX_ALT_TEXTURE_BASES];                     // 0x24790b8 sets per base
int  num_alt_texture_bases;                                         // 0x24794b8

// 0x4aac10  Confidence: likely (the two passes are read from the loop rotations; the second pass's counter
// runs down in [esp + 0x10] with the entry offset in ebp, which is VC6's strength reduction of the j loop).
// The first pass counts the "+Textures:" lines of a base and skips their names, then the position is
// restored and the second pass fills the array. The base name buffer is 0x21 bytes but ParseString is
// given 0x28 (D5207).
void alt_textures_parse_table()
{
    TextFile tf;
    TextFilePosition pos;
    char skip[0x100];
    char texture[0x2c];
    int n, i, j;

    memset(alt_texture_counts, 0, sizeof(alt_texture_counts));
    memset(alt_texture_lists, 0, sizeof(alt_texture_lists));
    for (i = 0; i < MAX_ALT_TEXTURE_BASES; i++)
        strcpy(alt_texture_bases[i], "");

    if (tf.Open("alt_textures.tbl", 9999999) == 0) {
        num_alt_texture_bases = 0;
        while (!tf.Accept("#End") && !tf.AtEnd() && num_alt_texture_bases < MAX_ALT_TEXTURE_BASES) {
            tf.Expect("$Base:");
            tf.ParseString(alt_texture_bases[num_alt_texture_bases], 0x28, '"', '"');
            tf.GetPosition(&pos);

            n = 0;
            while (!tf.Accept("#End") && !tf.AtEnd() && !tf.Accept("$Base:")) {
                n++;
                tf.Expect("+Textures:");
                tf.ParseString(skip, 0xff, '"', '"');
                while (!tf.Peek("#End") && !tf.AtEnd() && !tf.Peek("$Base:") && !tf.Peek("+Textures:"))
                    tf.ParseString(skip, 0xff, '"', '"');
            }
            tf.SetPosition(&pos);

            if (n > 0) {
                alt_texture_lists[num_alt_texture_bases] =
                    (alt_texture_set *)operator new(n * sizeof(alt_texture_set));
                alt_texture_counts[num_alt_texture_bases] = n;
                for (j = 0; j < n; j++) {
                    tf.Expect("+Textures:");
                    alt_texture_set *e = &alt_texture_lists[num_alt_texture_bases][j];
                    tf.ParseString(e->name, 0x28, '"', '"');
                    e->count = 0;
                    while (!tf.Peek("#End") && !tf.AtEnd() && !tf.Peek("$Base:") && !tf.Peek("+Textures:")) {
                        tf.ParseString(texture, 0x29, '"', '"');
                        e->bitmaps[e->count] = -1;
                        strcpy(e->names[e->count], texture);
                        e->count++;
                    }
                }
            }
            num_alt_texture_bases++;
        }
        tf.Close();
    }
}

// 0x4aafb0  Confidence: exact. The counts and names are kept.
void alt_textures_free()
{
    for (int i = 0; i < num_alt_texture_bases; i++) {
        if (alt_texture_lists[i]) {
            operator delete(alt_texture_lists[i]);
            alt_texture_lists[i] = NULL;
        }
    }
}

// 0x4aaff0  Confidence: exact. The set array of a base and its length; NULL / 0 when the base is unknown.
void *alt_textures_find(const char *base, int *count)
{
    for (int i = 0; i < num_alt_texture_bases; i++) {
        if (_stricmp(alt_texture_bases[i], base) == 0) {
            *count = alt_texture_counts[i];
            return alt_texture_lists[i];
        }
    }
    *count = 0;
    return NULL;
}

// 0x4ab050  Confidence: exact (VC6 byte match, match11; was likely) (the base pointer lives in a stack slot, [esp + 0x10]). The set of that name
// in the base's array; every base of that name is searched.
void *alt_textures_find_entry(const char *base, const char *name)
{
    for (int i = 0; i < num_alt_texture_bases; i++) {
        if (_stricmp(alt_texture_bases[i], base) == 0) {
            for (int j = 0; j < alt_texture_counts[i]; j++) {
                if (_stricmp(alt_texture_lists[i][j].name, name) == 0)
                    return &alt_texture_lists[i][j];
            }
        }
    }
    return NULL;
}
