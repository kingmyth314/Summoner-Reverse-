// unnamed_icam_0x410760.cpp - Sum.exe v1.40, code 0x410760-0x410ce6: the UI model camera tables
// (new_icam_load / new_icam_find / new_icam_get: bs_cameras.tbl, inv_cameras.tbl, status_cameras.tbl).
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name is not known (no assert path, no strings.tbl
// section, sum.symbols module "-"; the function prefix suggests new_icam.cpp, but nothing confirms it).
// Rename the file when the name turns up. docs/sum-exe/camera.md "UI model cameras (icam)".
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range: new_hud.cpp ends at 0x41075a (NOTES-hud.md), new_int_multi_update.cpp's version check starts
// at 0x410cf0. The last function is the COMDAT of vector3's inline copy constructor (0x410cd0).
// Data: .data 0x58bcbc-0x58bd1f, the literals only ("Unable to parse item ..." 0x58bcbc, "$PBH:", "$Offset:",
// "$Name:", "#End", "DIDN'T FIND ..." 0x58bd10), after new_hud.cpp's (.data up to 0x58bcbb) and before
// new_int_multi_update.cpp's version_check_result (0x58bd3c). Pooled: "" (0x5bd0d0). .rdata: 0.017453
// (0x57ef38, degrees to radians) and 180 (0x57ef3c). No .bss.

#include <stdio.h>
#include <string.h>
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/parse/parse.h"
#include "engine/objects/items.h"
#include "engine/characterinfo/characterinfo.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"

#define ICAM_MAX_ENTRIES 0x200
#define ICAM_DEG2RAD     0.017453f   // 0x57ef38

// The table behind new_icam_table's 0x3628 bytes (interface.h keeps the byte array; layout from the stores
// of new_icam_load). One entry per item type or character: index is the item_types / char_infos index,
// is_item tells which. Field names ours (D5503).
struct new_icam_data {
    int count;                                  // +0x0000
    char filename[0x21];                        // +0x0004 the .tbl name (strcpy, never read)
    bool is_item[ICAM_MAX_ENTRIES];             // +0x0025
    ushort index[ICAM_MAX_ENTRIES];             // +0x0226
    vector3 offset[ICAM_MAX_ENTRIES];           // +0x0628 $Offset:
    vector3 pbh[ICAM_MAX_ENTRIES];              // +0x1e28 $PBH: pitch, bank, heading in degrees
};
LAYOUT_CHECK(new_icam_data, sizeof(new_icam_data) == 0x3628);

// 0x410760  Confidence: exact (VC6 bytes match; the stores of an entry: offset as one vector3 assignment, the three PBH
// floats one by one, as the fld / fstp pairs read; the first float load is scheduled before the flag
// store). Two sections, each up to "#End": items (item_type_find), then characters (charinfo_find_by_name);
// an entry whose name is not found is skipped. The character section formats "Unable to parse item" into a
// local buffer that is never shown (the item section not even that). The 0x200 limit is tested before the
// lookup, so the entries beyond it are parsed and dropped.
void new_icam_load(void *table_arg, const char *filename)
{
    new_icam_data *table = (new_icam_data *)table_arg;
    TextFile tf;
    char msg[64];
    char name[512] = "";
    vector3 offset;
    float p, b, h;
    int idx;

    strcpy(table->filename, filename);
    tf.Open(filename, path_tables);
    table->count = 0;
    while (!tf.Accept("#End")) {
        tf.Expect("$Name:");
        tf.ParseString(name, 0x1ff, '"', '"');
        tf.Expect("$Offset:");
        offset.x = tf.ParseFloat();
        offset.y = tf.ParseFloat();
        offset.z = tf.ParseFloat();
        tf.Expect("$PBH:");
        p = tf.ParseFloat();
        b = tf.ParseFloat();
        h = tf.ParseFloat();
        if (table->count < ICAM_MAX_ENTRIES) {
            idx = item_type_find(name);
            if (idx >= 0) {
                table->is_item[table->count] = true;
                table->index[table->count] = (ushort)idx;
                table->offset[table->count] = offset;
                table->pbh[table->count].x = p;
                table->pbh[table->count].y = b;
                table->pbh[table->count].z = h;
                table->count++;
            }
        }
    }
    while (!tf.Accept("#End")) {
        tf.Expect("$Name:");
        tf.ParseString(name, 0x1ff, '"', '"');
        tf.Expect("$Offset:");
        offset.x = tf.ParseFloat();
        offset.y = tf.ParseFloat();
        offset.z = tf.ParseFloat();
        tf.Expect("$PBH:");
        p = tf.ParseFloat();
        b = tf.ParseFloat();
        h = tf.ParseFloat();
        if (table->count < ICAM_MAX_ENTRIES) {
            idx = charinfo_find_by_name(name);
            if (idx >= 0) {
                table->is_item[table->count] = false;
                table->index[table->count] = (ushort)idx;
                table->offset[table->count] = offset;
                table->pbh[table->count].x = p;
                table->pbh[table->count].y = b;
                table->pbh[table->count].z = h;
                table->count++;
            } else {
                sprintf(msg, "Unable to parse item \"%s\" in new_icam_load()\n", name);
            }
        }
    }
    tf.Close();
}

// 0x410af0  Confidence: exact (VC6 bytes match; `is_item == true`: cmp byte ptr, 1). The entry for a name (item type or
// character name, case-insensitive), -1 when there is none.
int new_icam_find(void *table_arg, const char *name)
{
    new_icam_data *table = (new_icam_data *)table_arg;
    int i;

    for (i = 0; i < table->count; i++) {
        if (table->is_item[i] == true) {
            if (_stricmp(name, item_types[table->index[i]].name) == 0)
                return i;
        } else {
            if (_stricmp(name, char_infos[table->index[i]].name) == 0)
                return i;
        }
    }
    return -1;
}

// 0x410b70  Confidence: exact (VC6 bytes match; the product order p * (b * h) from the two multiply calls; the copy
// constructor call of 0x410cd0 is the result of the inline matrix assignment's last vector3::operator=,
// D5504). The model's orientation from the entry's PBH (heading turned by 180 degrees) and its offset;
// orient and pos are left unchanged when the name has no entry (the message goes to a local buffer).
void new_icam_get(void *table_arg, const char *name, matrix *orient, vector3 *pos)
{
    new_icam_data *table = (new_icam_data *)table_arg;
    matrix hm, bm, pm;
    int idx = new_icam_find(table, name);

    if (idx < 0) {
        char msg[64];
        sprintf(msg, "DIDN'T FIND %s in new_icam_get()\n", name);
        return;
    }
    float p = table->pbh[idx].x;
    float b = table->pbh[idx].y;
    hm.set_from_angles(0.0f, 0.0f, (table->pbh[idx].z + 180.0f) * ICAM_DEG2RAD);
    bm.set_from_angles(0.0f, b * ICAM_DEG2RAD, 0.0f);
    pm.set_from_angles(p * ICAM_DEG2RAD, 0.0f, 0.0f);
    *orient = pm * (bm * hm);
    *pos = table->offset[idx];
}

// 0x410cd0 vector3::vector3(const vector3 &) (vsdk/math/vector.h): the COMDAT of the inline copy
// constructor, emitted after this unit's last function (its first out-of-line use, new_icam_get).
// Confidence: exact (compiler-generated from the header).
