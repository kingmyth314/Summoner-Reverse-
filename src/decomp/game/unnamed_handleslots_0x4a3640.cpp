// unnamed_handleslots_0x4a3640.cpp - Sum.exe v1.40, code 0x4a3640-0x4a37c9: eight handle slots (a handle
// at +0x30 and a GameTimer, released by ai_set_mode for the character's ai_state +0x80 and reset by
// GameInit / state_game_leave together with the effect streamer and four buffers). What the slots
// hold is not known: no other code reads their other fields.
// FILE NAME NOT KNOWN: a placeholder (no path string, no strings.tbl block; docs/sum-exe/source-map.md
// hand cut 0x4a3640-0x4a3700). Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0
// (cl 12.00.8966), optimised (the constructor is expanded into the initialiser loop).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: static_init_handle_slots_245c9e8 (0x4a3640, initialiser table entry 291) to
// handle_slots_reset (0x4a3700-0x4a37c9). boss_ai.cpp ends at 0x4a363a (level_page_in_characters);
// the shadow / obstacle-debug unit starts at 0x4a37d0.
// Data: .bss 0x245c9e8-0x245cd7b (after boss_ai.cpp's 0x245c9e0), .data 0x59cd88 (-1). Names ours.

#include <string.h>
#include "engine/objects/objects.h"
#include "engine/effects/effect_stream.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"

// A slot (0x38 bytes). The constructor clears the fields the initialiser stores; +0x0c..+0x1b and
// +0x20 are left alone (a vector3 at +0x0c is assumed for the gap).
struct handle_slot {
    handle_slot()
    {
        _unk00 = 0;
        _unk04 = 0;
        _unk08 = 0;
        _unk1c = 0;
        _unk24 = 0;
        _unk28 = 0;
        active = 0;
        handle = -1;
        timer.invalidate();
    }
    int _unk00;                       // +0x00
    int _unk04;                       // +0x04
    int _unk08;                       // +0x08
    vector3 _unk0c;                   // +0x0c
    int _unk18;                       // +0x18
    int _unk1c;                       // +0x1c
    int _unk20;                       // +0x20
    int _unk24;                       // +0x24
    int _unk28;                       // +0x28
    int active;                       // +0x2c
    int handle;                       // +0x30 -1 = free
    GameTimer timer;                  // +0x34
};
LAYOUT_CHECK(handle_slot, sizeof(handle_slot) == 0x38);
#define NUM_HANDLE_SLOTS 8

// 0x4a3640 static_init_handle_slots_245c9e8 (jmp, initialiser table entry 291). Confidence: exact.
// 0x4a3650 init_handle_slots_245c9e8  Confidence: likely (the store order). The constructor above for
// the eight slots (GameTimer constructed first, then the stores, then invalidate).
handle_slot handle_slots[NUM_HANDLE_SLOTS];   // 0x245c9e8

char handle_slot_name_a[0x40];                // 0x245cba8
char handle_slot_name_b[0x40];                // 0x245cbe8
int handle_slot_table[0x40];                  // 0x245cc28
int handle_slot_cd28;                         // 0x245cd28
int handle_slot_cd2c;                         // 0x245cd2c
char handle_slot_name_c[0x44];                // 0x245cd30
int handle_slot_cd74;                         // 0x245cd74
int handle_slot_cd78;                         // 0x245cd78
int handle_slot_current = -1;                 // 0x59cd88 (.data)

// 0x4a36a0  Confidence: exact. ai_set_mode: frees the slot holding the handle.
void handle_slot_release(int handle)
{
    int i;

    if (handle < 0)
        return;
    for (i = 0; i < NUM_HANDLE_SLOTS; i++) {
        if (handle_slots[i].handle == handle) {
            handle_slots[i].active = 0;
            handle_slots[i].timer.invalidate();
            handle_slots[i].handle = -1;
            return;
        }
    }
}

// 0x4a36f0  Confidence: exact (a tail call). GameInit.
void handle_slots_reset_thunk()
{
    handle_slots_reset();
}

// 0x4a3700  Confidence: exact (VC6 byte match, cl 8966; was likely: the order of the clears). state_game_leave.
void handle_slots_reset()
{
    int i;

    effect_stream_flush();
    for (i = 0; i < NUM_HANDLE_SLOTS; i++) {
        handle_slots[i].handle = -1;
        handle_slots[i].active = 0;
        handle_slots[i].timer.invalidate();
    }
    strcpy(handle_slot_name_c, "");
    handle_slot_cd74 = 0;
    handle_slot_cd78 = 0;
    handle_slot_cd28 = 0;
    handle_slot_cd2c = 0;
    memset(handle_slot_table, 0, sizeof(handle_slot_table));
    strcpy(handle_slot_name_a, "");
    handle_slot_current = -1;
    strcpy(handle_slot_name_b, "");
}
