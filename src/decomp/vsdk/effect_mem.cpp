// effect_mem.cpp - Sum.exe v1.40, code 0x51bb70-0x51e06a (vsdk\geom\effect_mem.cpp)
// Written from the Sum.exe disassembly (build/sum.asm). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range. symbols/sum.symbols tags 11 functions (0x51bcf0-0x51cf60) with this file. Everything
// between them (24 untagged functions) and the static initialisers before them (0x51bb70-0x51bcc0)
// belong to the same object: effects_init_pools threads the records the 0x51bb70.. initialisers
// construct, and the compiler-generated record constructors (0x51df10-0x51dfee) and the out-of-line
// Mempool members (0x51dff0, 0x51e010) close the unit, followed by three more COMDATs of inline
// members it calls (vfx_get_length_seconds 0x51e020, fixed8::set 0x51e040, float_to_fixed8 0x51e060;
// the light unit's static initialiser follows at 0x51e070). This file holds 0x51bb70-0x51df0f; the
// COMDATs 0x51df10-0x51e06a come from the definitions in effect_mem.h, vfx.h, fixed.h and memory.h
// (listed at the end of the file). The previous unit ends with ftp_get__scalar_deleting_dtor
// (0x51bb50). decomp60 wrote the last 12 functions (0x51d250-0x51def0).
//
// Build flags of this unit differ from the game code: no string pooling (the __FILE__ literal and
// " \r\n\t:," each appear more than once in .data) and no inline expansion (cmd_show_effect_mem
// jumps to an empty function, the record constructors call vector3() out of line, and the Mempool
// constructor and getPoolSpaceLeft are called). /O2 without /Gf and with /Ob0 fits; see NOTES.md.

#include <stdio.h>
#include <string.h>
#include "vsdk/geom/effect_mem.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/console.h"
#include "vsdk/bmpman/bmpman.h"
#include "engine/effects/effect_stream.h"

// Original __FILE__ of this unit (literals 0x5a8770 and 0x5a88a8, not pooled).
#define EFFECT_MEM_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\geom\\effect_mem.cpp"

// ---- .bss ------------------------------------------------------------------------------------------
// Definitions with dynamic initialisers are written in initialiser order (static_initializers
// 0x58890c-0x588934); their addresses are not in that order (open .bss question, as in frametime.cpp).
// Each initialiser below is a thunk (jmp) plus `vector constructor iterator` over the array.

// 0x51bb70 init_vertex_buf_272b900. Confidence: exact.
vector3 vfx_mesh_verts[1820];                     // 0x272b900
// 0x51bba0 init_fx_rec48_array. Confidence: exact.
fx_rec48 fx_rec48_array[48];                      // 0x2c7d010
// 0x51bbd0 init_fx_rec98_array. Confidence: exact.
fx_rec98 fx_rec98_array[384];                     // 0x2c7ddb0
// 0x51bc00 init_fx_rec30_array_2730e50. Confidence: exact.
fx_rec30 fx_rec30_array[32];                      // 0x2730e50
// 0x51bc30 init_fx_rec2c_array_2c8ff60. Confidence: exact.
fx_rec2c fx_rec2c_array[64];                      // 0x2c8ff60
// 0x51bc60 init_fx_rec4c_array. Confidence: exact.
fx_rec4c fx_rec4c_array[128];                     // 0x2c8c1b0
// 0x51bc90 init_fx_rec28_array_2c42690. Confidence: exact.
fx_rec28 fx_rec28_array[6000];                    // 0x2c42690
// 0x51bcc0 init_fx_recb0_array. Confidence: exact.
fx_recb0 fx_recb0_array[128];                     // 0x27229d8

// One buffer of 0x511240 bytes: the Mempool is built over all of it, the stream / cutscene area
// (effects_stream_buffer 0x277d910) is its second part. symbols/sum.symbols gives 0x4c4c0 for
// effects_mem_buffer and a separate 0x4c4d80 effects_stream_buffer; two adjacent arrays would give
// the same code if the size argument were the sum.
char effects_mem_buffer[EFFECTS_PRELOAD_SIZE + EFFECTS_CUTSCENE_SIZE];   // 0x2731450
// 0x51bcf0 init_effects_mempool (jmp 0x51bd00: push "Effects", 0x511240, buffer; Mempool::Mempool
// called, not inlined). Confidence: likely (inline expansion, see the file comment).
Mempool effects_mempool(effects_mem_buffer, sizeof(effects_mem_buffer), "Effects"); // 0x2c98a70
// (decomp60: one comment line per compiler-generated function.)
// 0x51bd20 init_effects_string_pool (call 0x51bd30, jmp 0x51bd40). Confidence: exact.
// 0x51bd30 effects_string_pool_ctor (mov ecx; jmp StringPool::StringPool). Confidence: exact.
// 0x51bd40 effects_string_pool_register_dtor (atexit 0x51bd50). Confidence: exact.
// 0x51bd50 effects_string_pool_dtor (mov ecx; jmp StringPool::~StringPool). Confidence: exact.
StringPool effects_string_pool;                   // 0x2c7dd98
// 0x51bd60 init_ccmd_show_effect_mem_thunk. Confidence: exact.
// 0x51bd70 init_ccmd_show_effect_mem. Confidence: exact.
console_command ccmd_show_effect_mem("show_effect_mem", "Shows how much memory the effects system is using.",
                                     cmd_show_effect_mem); // 0x2c90a60

// The rest in address order.
ubyte effect_base_slot[MAX_EFFECT_NAMES];         // 0x2722820 node index per effect name, 0xff unknown
const char *effect_base_names[MAX_EFFECT_NAMES];  // 0x27281d8
int effects_stream_slots[NUM_STREAM_SLOTS];       // 0x27288b8
static char effects_string_buffer[0x3000];        // 0x27288d8 effects_string_pool storage (label ours)
int effects_stream_slots_saved[NUM_STREAM_SLOTS]; // 0x272b8dc
int effects_level_mem_mark;                       // 0x2c7dd90 Effects mempool offset after preload.rfx
int effects_stream_slots_b_saved[NUM_BIG_SLOTS];  // 0x2c8e7b0
ubyte effect_base_record_index[MAX_EFFECT_NAMES]; // 0x2c8e7c8 effect_bases index per name
static int effects_name_order[MAX_EFFECT_NAMES];  // 0x2c8e980 set to 0..439 by effects_load_bases, no reader found (label ours)
effect_base effect_bases[MAX_EFFECT_BASES];       // 0x2c8f060
ubyte effect_base_stream_users[MAX_EFFECT_BASES]; // 0x2c9f1b0
int effects_stream_slots_b[NUM_BIG_SLOTS];        // 0x2c9f270
static int effects_name_map[MAX_EFFECT_NAMES];    // 0x2c9f288 set to -1 by effects_load_bases, no reader found (label ours)
// decomp60: a node pointer, not a counter (vfx_mesh_instance__render compares it with its this and
// stores it; vfx_instance_free clears it when the node is freed). Was effects_vfx_counter_2c9f968.
fx_rec98 *vfx_last_mesh_instance;                 // 0x2c9f968 (label ours)
int effects_preload_count;                        // 0x2c9f96c
int effects_stream_slot_next;                     // 0x2c9f970
int effects_stream_slot_b_next;                   // 0x2c9f974
fx_rec48 *vfx_instance_free_list;                 // 0x2c9f978
fx_rec98 *fx_rec98_free_list;                     // 0x2c9f97c
fx_rec30 *fx_rec30_free_list;                     // 0x2c9f980
fx_rec2c *fx_rec2c_free_list;                     // 0x2c9f984
fx_rec4c *fx_rec4c_free_list;                     // 0x2c9f988
fx_rec28 *fx_rec28_list;                          // 0x2c9f98c
fx_recb0 *fx_recb0_list;                          // 0x2c9f990
bool effects_preload_done;                        // 0x2c9f994 set by effects_load_preload_rfx
int effects_num_bases;                            // 0x2c9f998
int effects_num_names;                            // 0x2c9f99c
// Not placed: 0x2c90a70-0x2c98a6f and 0x2c98a8c-0x2c9f1af lie inside this unit's .bss; none of this
// unit's functions uses them (0x2c98a90 is read by vfx_mesh_render / vfx_mesh_instance__update_frame
// in another vsdk\geom unit).

// Append to a circular list given by a head pointer (NULL = empty).
#define fx_list_append(head, rec) \
    if ((head) == NULL) { (rec)->next = (rec); (rec)->prev = (rec); (head) = (rec); } \
    else { (rec)->prev = (head)->prev; (rec)->next = (head); (head)->prev->next = (rec); (head)->prev = (rec); }

// 0x51bd90  Confidence: exact (JMP to the empty function 0x51d240: a call that /Ob0 does not
// expand; the function's own body was compiled out).
void cmd_show_effect_mem()
{
    effects_show_memory_usage();
}

// 0x51bda0  Confidence: exact (the limit is a nested conditional: NEG/SBB for the big_slot choice).
// Allocates the buffer of a streamed base and points the Effects mempool behind its aligned data.
char *effect_base_alloc_stream_buffer(int index)
{
    effect_base *b = &effect_bases[index];
    char *buf;
    int aligned;
    unsigned int limit;

    if (b->resident || b->streamed)
        return NULL;
    buf = b->data;
    if (buf == NULL) {
        buf = new char[b->size + b->table_size];
        b->data = buf;
        if (buf == NULL) {
            effect_base_stream_users[index]--;
            return NULL;
        }
    }
    if (!b->cutscene) {
        aligned = align_up_64(b->size + b->table_size);
        if (b->category == EFFECT_CAT_PRELOAD)
            limit = EFFECTS_PRELOAD_SIZE;
        else if (b->category == EFFECT_CAT_CUTSCENE)
            limit = EFFECTS_CUTSCENE_SIZE;
        else
            limit = b->big_slot ? BIG_SLOT_SIZE : STREAM_SLOT_SIZE;
        effects_mempool.offset = 0;
        effects_mempool.destination = b->data + aligned;
        effects_mempool.size = limit - aligned;
    }
    b->streamed = 1;
    return buf;
}

// 0x51be80  Confidence: exact.
int align_up_64(int value)
{
    int rem = value & 0x3f;

    if (rem != 0)
        return value - rem + 0x40;
    return value;
}

// 0x51bea0  Confidence: exact (the two flag stores are merged into AND 0x3effffff / OR 0x81000000).
void effect_base_finish_stream(int index)
{
    effect_base *b = &effect_bases[index];

    if (!b->resident && b->streamed) {
        effect_base_stream_users[index] = 0;
        b->relocate(b->data, b->data + b->table_size);
        if (effects_mempool.getPoolSpaceLeft() >= 0x40 && (effects_mempool.offset & 0x3f))
            effects_mempool.offset += -(int)effects_mempool.offset & 0x3f;
        b->streamed = 0;
        b->bitmaps_pending = 1;
        b->resident = 1;
    }
}

// 0x51bf20  Confidence: exact.
int vfx_preload_by_name(const char *name)
{
    int id = vfx_find_name(name);
    int index;
    effect_base *b;

    if (id < 0)
        return -1;
    index = effect_base_record_index[id];
    b = &effect_bases[index];
    if (b->resident || b->streamed)
        return 0;
    return rfx_preload(index);
}

// 0x51bf70  Confidence: exact (unsigned counter: JBE / JB against effects_num_names).
int vfx_find_name(const char *path)
{
    const char *p;
    unsigned int i;

    if (path != NULL) {
        p = strrchr(path, '\\');
        if (p != NULL)
            path = p + 1;
        for (i = 0; i < (unsigned int)effects_num_names; i++) {
            if (!_stricmp(path, effect_base_names[i]))
                return i;
        }
    }
    return -1;
}

// 0x51bfd0  Confidence: likely. The "AS n" checkpoints are release printf calls. `offset` lives in
// the dead parameter slot in the binary. Sizes compare unsigned (JA / JBE).
int rfx_preload(int index)
{
    effect_base *b = &effect_bases[index];
    int num_nodes, table_size, size, version;
    unsigned int buf_size, offset, total;
    Mempool *pool;
    int slot;
    unsigned int i;

    if (b->resident)
        return 0;
    if (effect_stream_queue_contains(b->name))
        return 0;

    printf("*************** ADD STREAMING EFFECT *****************\n");
    effect_base_read(index, &num_nodes, &table_size, &size, &version);

    if (b->category == EFFECT_CAT_PRELOAD) {
        pool = &effects_mempool;
    } else {
        pool = get_current_mempool();
        if (pool == NULL) {
            switch (b->category) {
            case EFFECT_CAT_LEVEL:
                pool = &level_mempool;
                break;
            case EFFECT_CAT_CHARACTER:
                pool = &summon_mempool;
                break;
            default:
                pool = &effects_mempool;
                break;
            }
        }
    }
    if (pool != NULL)
        printf("Going into pool %s\n", pool->name);

    b->level = 0;
    if (effect_base_queue_stream(index, &buf_size, &offset, false, pool) != 0)
        return -1;
    total = buf_size + offset;
    b->data = NULL;

    if (pool == &effects_mempool) {
        printf("AS 0\n");
        if (b->category == EFFECT_CAT_PRELOAD) {
            printf("AS 1\n");
            b->data = effects_mem_buffer;
            if (total > EFFECTS_PRELOAD_SIZE)
                VSDK_ERROR(EFFECT_MEM_CPP, 1095, "Preload.rfx is too big for the current preload size! I refuse to run!\n");
        } else {
            b->cutscene = 0;
            if (total <= BIG_SLOT_SIZE && b->category != EFFECT_CAT_CUTSCENE) {
                if (total > STREAM_SLOT_SIZE) {
                    printf("AS 4 (%x)\n", (unsigned int)b);
                    slot = effects_alloc_stream_slot(index);
                    if (slot >= 0) {
                        b->data = effects_preload_slot_ptr(slot);
                        b->big_slot = 1;
                    } else {
                        printf("AS 4b\n");
                    }
                } else {
                    printf("AS 5\n");
                    slot = effects_find_stream_slot(index);
                    if (slot >= 0) {
                        printf("AS 6 (%x)\n", (unsigned int)b);
                        b->data = effects_stream_slot_ptr(slot);
                        b->big_slot = 0;
                    } else {
                        slot = effects_alloc_stream_slot(index);
                        if (slot >= 0) {
                            printf("AS 7 (%x)\n", (unsigned int)b);
                            b->data = effects_preload_slot_ptr(slot);
                            b->big_slot = 1;
                        } else {
                            printf("AS AIEEEEEEEEEEE\n");
                        }
                    }
                }
            } else {
                printf("AS 2\n");
                if (b->category == EFFECT_CAT_CUTSCENE) {
                    b->big_slot = 0;
                    if (effects_preload_count) {
                        b->data = effects_mempool.destination + effects_mempool.offset;
                    } else {
                        for (i = 0; i < NUM_BIG_SLOTS; i++) {
                            effects_stream_slots_b_saved[i] = effects_stream_slots_b[i];
                            if (effects_stream_slots_b[i] >= 0) {
                                effect_base_release(effects_stream_slots_b[i]);
                                effects_stream_slots_b[i] = -1;
                            }
                        }
                        for (i = 0; i < NUM_STREAM_SLOTS; i++) {
                            effects_stream_slots_saved[i] = effects_stream_slots[i];
                            if (effects_stream_slots[i] >= 0) {
                                effect_base_release(effects_stream_slots[i]);
                                effects_stream_slots[i] = -1;
                            }
                        }
                        b->data = effects_stream_buffer;
                    }
                    effects_preload_count++;
                } else {
                    printf("AS 3\n");
                    return -1;
                }
            }
        }
        if (b->data == NULL)
            return -1;
        effects_mempool.destination = b->data;
        effects_mempool.size = 0;
        effects_mempool.offset = 0;
    } else {
        printf("AS 9\n");
        b->big_slot = 0;
        b->cutscene = 1;
    }

    if (effect_base_queue_stream(index, &buf_size, &offset, true, pool) != 0)
        return -1;
    effect_base_stream_users[index]++;
    return 0;
}

// 0x51c370  Confidence: exact (VC6 byte match, match31; was likely) (the saved-slot loops are pointer loops with signed compares, as VC6
// makes of index loops). When the last cutscene base goes, the saved stream bases are reloaded and
// the bitmaps of every resident base flagged bitmaps_pending are loaded.
void effect_base_release(int index)
{
    effect_base *b = &effect_bases[index];
    vfx_base *v;
    int i;

    b->resident = 0;
    b->data = NULL;
    if (b->category == EFFECT_CAT_CUTSCENE && effects_preload_count && --effects_preload_count == 0) {
        for (i = 0; i < NUM_BIG_SLOTS; i++) {
            if (effects_stream_slots_b_saved[i] >= 0)
                effect_base_load(effects_stream_slots_b_saved[i]);
        }
        for (i = 0; i < NUM_STREAM_SLOTS; i++) {
            if (effects_stream_slots_saved[i] >= 0)
                effect_base_load(effects_stream_slots_saved[i]);
        }
        bm_set_preload_on_load(1);
        for (i = 0; i < effects_num_names; i++) {
            if (effect_bases[effect_base_record_index[i]].resident
                && effect_bases[effect_base_record_index[i]].bitmaps_pending) {
                v = effect_get_base(i);
                if (v != NULL)
                    v->load_bitmaps(-1);
            }
        }
        bm_set_preload_on_load(0);
        for (i = 0; i < effects_num_bases; i++) {
            if (effect_bases[i].bitmaps_pending)
                effect_bases[i].bitmaps_pending = 0;
        }
    }
}

// 0x51c490  Confidence: exact (version > 9 and version < 9 are two tests of one compare: two
// source checks whose messages are compiled out).
int effect_base_load(int index)
{
    effect_base *b = &effect_bases[index];

    if (b->resident)
        return 0;
    if (effect_stream_update())
        return -1;
    if (!b->streamed) {
        if (b->version > 9)
            return -1;
        if (b->version < 9)
            return -1;
        if (rfx_preload(index))
            return -1;
        effect_stream_start();
    }
    while (effect_stream_update() == 1)
        ;
    effect_stream_reset();
    return 0;
}

// 0x51c520  Confidence: exact. Six slots, but the index is masked with 5 (not taken modulo 6), as
// the binary does: only slots 0, 1, 4 and 5 are ever used.
int effects_alloc_stream_slot(int base)
{
    int i, slot;
    int next = effects_stream_slot_b_next;

    for (i = next; i < next + NUM_BIG_SLOTS; i++) {
        slot = i & 5;
        if (effects_stream_slots_b[slot] == -1) {
            effects_stream_slots_b[slot] = base;
            effects_stream_slot_b_next = (slot + 1) & 5;
            return slot;
        }
    }
    for (i = next; i < next + NUM_BIG_SLOTS; i++) {
        slot = i & 5;
        if (!effect_base_stream_users[effects_stream_slots_b[slot]]) {
            effect_base_release(effects_stream_slots_b[slot]);
            effects_stream_slots_b[slot] = base;
            effects_stream_slot_b_next = (slot + 1) & 5;
            return slot;
        }
    }
    return -1;
}

// 0x51c5c0  Confidence: exact. Eight slots scanned six at a time with mask 7; the cursor is masked
// with 5, so it only ever restarts at 0, 1, 4 or 5.
int effects_find_stream_slot(int base)
{
    int i, slot;
    int next = effects_stream_slot_next;

    for (i = next; i < next + 6; i++) {
        slot = i & 7;
        if (effects_stream_slots[slot] == -1) {
            effects_stream_slots[slot] = base;
            effects_stream_slot_next = (slot + 1) & 5;
            return slot;
        }
    }
    for (i = next; i < next + 6; i++) {
        slot = i & 7;
        if (!effect_base_stream_users[effects_stream_slots[slot]]) {
            effect_base_release(effects_stream_slots[slot]);
            effects_stream_slots[slot] = base;
            effects_stream_slot_next = (slot + 1) & 5;
            return slot;
        }
    }
    return -1;
}

// 0x51c660  Confidence: exact (slot * 500032 as a LEA chain, base 0x2965f10).
char *effects_preload_slot_ptr(int slot)
{
    return effects_stream_buffer + NUM_STREAM_SLOTS * STREAM_SLOT_SIZE + slot * BIG_SLOT_SIZE;
}

// 0x51c680  Confidence: exact (slot * 250048, base 0x277d910).
char *effects_stream_slot_ptr(int slot)
{
    return effects_stream_buffer + slot * STREAM_SLOT_SIZE;
}

// 0x51c6a0  Confidence: exact. *out_offset is always 0 here.
int effect_base_queue_stream(int index, unsigned int *out_size, unsigned int *out_offset, bool queue, Mempool *pool)
{
    effect_base *b;

    if (index >= 0 && index < effects_num_bases) {
        b = &effect_bases[index];
        *out_offset = 0;
        *out_size = align_up_64(b->size + b->table_size);
        if (!queue || effect_stream_queue_add(b->name, pool, 0))
            return 0;
        return -1;
    }
    *out_offset = 0;
    *out_size = 0;
    return -1;
}

// 0x51c720  Confidence: exact (VC6 byte match, match31; was likely). The node index of an effect name is cached in effect_base_slot.
// The node pointer is loaded before the range tests, and the pointer test is signed (JLE), so the
// pointers were compared as ints.
vfx_base *effect_get_base(int id)
{
    int index = effect_base_record_index[id];
    effect_base *b = &effect_bases[index];
    vfx_base *v;
    unsigned int i;
    int hash, slot;

    if (!b->resident) {
        if (effects_preload_count && !b->cutscene && b->category != EFFECT_CAT_CUTSCENE)
            return NULL;
        if (effect_base_load(index))
            return NULL;
    }
    if (effect_base_slot[id] == 0xff) {
        hash = effects_hash_name(effect_base_names[id]);
        for (i = 0; i < b->num_nodes; i++) {
            if (((vfx_node_entry *)b->data)[i].hash == hash) {
                effect_base_slot[id] = (ubyte)i;
                break;
            }
        }
    }
    if (effect_base_slot[id] != 0xff) {
        slot = effect_base_slot[id];
        v = ((vfx_node_entry *)b->data)[slot].base;
        if (slot >= 0 && slot < b->num_nodes && v != NULL && (int)v > (int)b->data) {
            v->category = b->category;
            return v;
        }
    }
    return NULL;
}

// 0x51c800  Confidence: exact.
int effects_get_num_bases()
{
    return effects_num_bases;
}

// 0x51c810  Confidence: exact.
int effects_find_base(const char *name)
{
    unsigned int i;

    for (i = 0; i < (unsigned int)effects_num_bases; i++) {
        if (!_stricmp(name, effect_bases[i].name))
            return i;
    }
    return -1;
}

// 0x51c860  Confidence: exact.
void effect_base_set_info(unsigned int index, ushort num_nodes, int table_size, int size, ushort version)
{
    effect_base *b;

    if (index < (unsigned int)effects_num_bases) {
        b = &effect_bases[index];
        b->num_nodes = num_nodes;
        b->table_size = table_size;
        b->size = size;
        b->version = version;
    }
}

// 0x51c8b0  Confidence: likely (EH frame for the File destructor; the header is read into an
// effect_base-shaped local, 0x14 bytes).
void effect_base_read(unsigned int index, int *out_nodes, int *out_table_size, int *out_size, int *out_version)
{
    effect_base *b;
    effect_base hdr;

    if (index < (unsigned int)effects_num_bases) {
        b = &effect_bases[index];
        File f;
        if (f.open(b->name, 1, PATH_ANY) == 0 && f.read(&hdr, sizeof(hdr), 0, 0) == sizeof(hdr)) {
            b->num_nodes = hdr.num_nodes;
            *out_nodes = hdr.num_nodes;
            b->table_size = hdr.table_size;
            *out_table_size = hdr.table_size;
            b->size = hdr.size;
            *out_size = hdr.size;
            b->version = hdr.version;
            *out_version = hdr.version;
        }
    }
}

// 0x51c9c0  Confidence: exact.
vfx_base *vfx_preload(const char *name, int *out_index)
{
    int id = vfx_find_name(name);

    if (id < 0)
        return NULL;
    if (out_index != NULL)
        *out_index = id;
    return effect_get_base(id);
}

// 0x51c9f0  Confidence: exact (thiscall on the base record; counter unsigned, data reloaded each pass).
void effect_base::relocate(char *base_addr, char *end)
{
    unsigned int i;

    for (i = 0; i < num_nodes; i++)
        ((vfx_node_entry *)data)[i].relocate(base_addr, end);
}

// 0x51ca30  Confidence: exact (vfx_node_relocate gets the two arguments swapped; their meaning is
// not settled).
void vfx_node_entry::relocate(char *base_addr, char *end)
{
    ptr_relocate(&base, base_addr);
    base->relocate_node(end, base_addr);
}

// 0x51ca60  Confidence: likely. Reads lines until one has a name, a known category and at least one
// effect name; returns 0 with the names in names[0..*num], -1 at the end of the file or when a line
// has more than MAX_ARR_NAMES names. Three separate " \r\n\t:," literals per call site (no /Gf).
int effects_parse_arr_line(File *f, int *line, char *name, int *category, unsigned int *num,
                           char names[][ARR_NAME_LEN])
{
    char buf[0x400];
    char *p, *tok1, *tok2, *tok3, *tok;

    *num = 0;
    while (f->read_line(buf, sizeof(buf), 0)) {
        p = strchr(buf, ';');
        if (p != NULL)
            *p = 0;
        (*line)++;
        tok1 = strtok(buf, " \r\n\t:,");
        tok2 = strtok(NULL, " \r\n\t:,");
        tok3 = strtok(NULL, " \r\n\t:,");
        if (tok1 == NULL || tok2 == NULL || tok3 == NULL)
            continue;

        strcpy(name, tok1);
        if (!_stricmp(tok2, "preload"))
            *category = EFFECT_CAT_PRELOAD;
        else if (!_stricmp(tok2, "cutscene"))
            *category = EFFECT_CAT_CUTSCENE;
        else if (!_stricmp(tok2, "level"))
            *category = EFFECT_CAT_LEVEL;
        else if (!_stricmp(tok2, "spell"))
            *category = EFFECT_CAT_SPELL;
        else if (!_stricmp(tok2, "character"))
            *category = EFFECT_CAT_CHARACTER;
        else if (!_stricmp(tok2, "summon"))
            *category = EFFECT_CAT_SUMMON;
        else
            continue;

        strcpy(names[*num], tok3);
        while (*num < MAX_ARR_NAMES) {
            (*num)++;
            tok = strtok(NULL, " \r\n\t:,");
            if (tok == NULL)
                return 0;
            strcpy(names[*num], tok);
        }
        return -1;
    }
    return -1;
}

// 0x51ccd0  Confidence: exact (memset as REP STOSD with -1; the loop runs over all 192 records).
void effects_reset_level()
{
    int i;

    memset(effects_stream_slots, 0xff, sizeof(effects_stream_slots));
    memset(effects_stream_slots_b, 0xff, sizeof(effects_stream_slots_b));
    for (i = 0; i < MAX_EFFECT_BASES; i++) {
        effect_bases[i].level = 0;
        if (effect_bases[i].category != EFFECT_CAT_PRELOAD && effect_bases[i].resident) {
            effect_base_release(i);
            effect_base_stream_users[i] = 0;
        }
    }
    effects_init_pools();
}

// 0x51cd40  Confidence: exact (the first five heads stay in registers through their loops, the
// last two are read and written in memory each pass: register pressure, same macro).
void effects_init_pools()
{
    int i;
    fx_rec48 *r48;

    vfx_instance_free_list = NULL;
    fx_rec98_free_list = NULL;
    fx_rec30_free_list = NULL;
    fx_rec2c_free_list = NULL;
    fx_rec4c_free_list = NULL;
    fx_rec28_list = NULL;
    fx_recb0_list = NULL;

    for (i = 0; i < 48; i++) {
        r48 = &fx_rec48_array[i];
        fx_list_append(vfx_instance_free_list, r48);
        r48->nodes = NULL;
        r48->lights = NULL;
        r48->emitters = NULL;
        r48->rec2c_list = NULL;
        r48->id_bits &= ~0x40;
    }
    for (i = 0; i < 384; i++) {
        fx_list_append(fx_rec98_free_list, &fx_rec98_array[i]);
    }
    for (i = 0; i < 32; i++) {
        fx_list_append(fx_rec30_free_list, &fx_rec30_array[i]);
    }
    for (i = 0; i < 64; i++) {
        fx_list_append(fx_rec2c_free_list, &fx_rec2c_array[i]);
    }
    for (i = 0; i < 128; i++) {
        fx_list_append(fx_rec4c_free_list, &fx_rec4c_array[i]);
    }
    for (i = 0; i < 6000; i++) {
        fx_list_append(fx_rec28_list, &fx_rec28_array[i]);
    }
    for (i = 0; i < 128; i++) {
        fx_list_append(fx_recb0_list, &fx_recb0_array[i]);
    }
}

// 0x51cf60  Confidence: likely. Returns -1 (File destructor only, no close) when a parse fails,
// which includes reaching the end of the file through lines without entries. The inner _stricmp
// loop's result is unused (a duplicate check whose warning was compiled out); it compares against
// the first k names only, k being the position on the current line.
int effects_load_bases()
{
    unsigned int num;
    int category;
    int line;
    char name[0x2c];
    char names[MAX_ARR_NAMES][ARR_NAME_LEN];
    unsigned int i;
    int j, found;

    memset(effects_stream_slots, 0xff, sizeof(effects_stream_slots));
    memset(effects_stream_slots_b, 0xff, sizeof(effects_stream_slots_b));
    memset(effect_base_stream_users, 0, sizeof(effect_base_stream_users));
    memset(effects_name_map, 0xff, sizeof(effects_name_map));
    memset(effect_base_names, 0, sizeof(effect_base_names));
    for (i = 0; i < MAX_EFFECT_NAMES; i++)
        effects_name_order[i] = i;
    effects_string_pool.Init(effects_string_buffer, sizeof(effects_string_buffer), false);
    effects_init_pools();
    effects_num_bases = 0;

    File f;
    if (f.open("effects.arr", 1, PATH_ANY))
        return -1;
    line = 0;
    while (!f.eof()) {
        num = 0;
        category = 0;
        if (effects_parse_arr_line(&f, &line, name, &category, &num, names))
            return -1;

        found = -1;
        for (i = 0; i < (unsigned int)effects_num_bases; i++) {
            if (!_stricmp(effect_bases[i].name, name)) {
                found = i;
                if (category != (int)effect_bases[i].category)
                    category = effect_bases[i].category;
            }
        }
        if (found < 0) {
            if ((unsigned int)effects_num_bases >= MAX_EFFECT_BASES)
                return -1;
            found = effects_num_bases;
            effect_bases[found].name = effects_string_pool.AddUnique(name);
            effect_bases[found].category = category;
            effect_bases[found].resident = 0;
            effect_bases[found].data = NULL;
            effect_bases[found].level = 0;
            effect_bases[found].bitmaps_pending = 0;
            effects_num_bases++;
        }

        // Outer counter unsigned (JB against num), inner bound signed (TEST / JLE).
        for (i = 0; i < num; i++) {
            for (j = 0; j < (int)i; j++)
                _stricmp(effect_base_names[j], names[i]);
            if ((unsigned int)effects_num_names >= MAX_EFFECT_NAMES)
                VSDK_ERROR(EFFECT_MEM_CPP, 1783, "Too many effects! MAX_EFFECT_BASES needs to be increased!");
            _strlwr(names[i]);
            effect_base_names[effects_num_names] = effects_string_pool.AddUnique(names[i]);
            effect_base_record_index[effects_num_names] = (ubyte)found;
            effect_base_slot[effects_num_names] = 0xff;
            effects_num_names++;
        }
    }
    f.close();
    return 0;
}

// 0x51d240  Confidence: exact. Empty: its body (a memory report) is compiled out in this build.
// Name ours (no symbol).
void effects_show_memory_usage()
{
}

// ---- decomp60: 0x51d250-0x51def0 -----------------------------------------------------------------

// 0x51d250  Confidence: likely. Takes an instance and, from the free pools, the base's node, light,
// fx_rec4c, emitter (with their particles) and fx_rec2c records; each loop stops early when its pool
// runs dry. An emitter's links are resolved to the instance's n-th fx_rec4c record. Level bases
// (category 0) build their nodes in the level mempool; `level` lives in flag's dead argument slot.
vfx_instance *vfx_instance_create(vfx_base *base, int a, bool flag)
{
    vfx_instance *vi;
    fx_rec98 *node;
    vfx_light_desc *light;
    fx_rec4c *r4c;
    fx_recb0 *em;
    fx_rec28 *p;
    fx_rec2c *r2c;
    vfx_emitter_desc *desc;
    int i, j, k, idx;
    bool level;

    vi = vfx_instance_free_list;
    if (vi == NULL)
        return NULL;
    plist_remove(vfx_instance_free_list, vi);
    vi->id_bits |= 0x40;

    vi->nodes = NULL;
    for (i = 0; fx_rec98_free_list != NULL && i < base->num_nodes; i++) {
        node = fx_rec98_free_list;
        node->level = flag;
        plist_remove(fx_rec98_free_list, node);
        plist_append(vi->nodes, node);
    }
    vi->lights = NULL;
    for (i = 0; fx_rec30_free_list != NULL && i < base->num_lights; i++) {
        light = fx_rec30_free_list;
        plist_remove(fx_rec30_free_list, light);
        plist_append(vi->lights, light);
    }
    vi->rec4c_list = NULL;
    for (i = 0; fx_rec4c_free_list != NULL && i < base->num_rec4c; i++) {
        r4c = fx_rec4c_free_list;
        r4c->clear();
        plist_remove(fx_rec4c_free_list, r4c);
        plist_append(vi->rec4c_list, r4c);
    }
    vi->length = 0.0f;
    vi->emitters = NULL;
    for (i = 0; fx_recb0_list != NULL && i < base->num_emitters; i++) {
        desc = &base->emitters[i];
        em = fx_recb0_list;
        plist_remove(fx_recb0_list, em);
        plist_append(vi->emitters, em);
        vi->length = float_max(vi->length, desc->get_length_seconds());
        em->particles = NULL;
        for (j = 0; fx_rec28_list != NULL && j < base->emitters[i].num_particles; j++) {
            p = fx_rec28_list;
            p->life.set(-1.0f);
            plist_remove(fx_rec28_list, p);
            plist_append(em->particles, p);
        }
        for (k = 0; k < (int)desc->num_links; k++) {
            if (desc->links[k] != NULL) {
                idx = (desc->links[k] - base->rec4c_table) / 0xa0;
                r4c = vi->rec4c_list;
                for (j = 0; r4c != NULL && j < idx; j++)
                    r4c = r4c->next;
                em->links[k] = r4c;
            } else {
                em->links[k] = NULL;
            }
        }
    }
    vi->rec2c_list = NULL;
    for (i = 0; fx_rec2c_free_list != NULL && i < base->num_rec2c; i++) {
        r2c = fx_rec2c_free_list;
        plist_remove(fx_rec2c_free_list, r2c);
        plist_append(vi->rec2c_list, r2c);
    }

    level = false;
    if (base->category == EFFECT_CAT_LEVEL) {
        level = true;
        set_current_mempool(&level_mempool);
    }
    vi->init_nodes(base, a, level);
    if (level)
        set_current_mempool(NULL);
    return vi;
}

// 0x51d7b0  Confidence: exact. Every list of a live instance goes back to its pool (lights after
// unlinking free their attached light, emitters return their particles); the last user of a streamed
// cutscene base (not marked for the level) or of a cutscene-category base while preloads exist
// releases it.
void vfx_instance_free(vfx_instance *vi)
{
    int index = vi->id_bits >> 9;
    int rec;
    fx_rec98 *node;
    vfx_light_desc *light;
    fx_recb0 *em;
    fx_rec28 *p;
    fx_rec4c *r4c;
    fx_rec2c *r2c;
    effect_base *b;

    if (!(vi->id_bits & 0x40))
        return;
    vi->release_nodes();
    plist_append(vfx_instance_free_list, vi);
    vi->id_bits &= ~0x40;

    while ((node = vi->nodes) != NULL) {
        if (vfx_last_mesh_instance == node)
            vfx_last_mesh_instance = NULL;
        plist_remove(vi->nodes, node);
        plist_append(fx_rec98_free_list, node);
    }
    while ((light = vi->lights) != NULL) {
        plist_remove(vi->lights, light);
        plist_append(fx_rec30_free_list, light);
        light->free_light();
    }
    while ((em = vi->emitters) != NULL) {
        plist_remove(vi->emitters, em);
        plist_append(fx_recb0_list, em);
        while ((p = em->particles) != NULL) {
            plist_remove(em->particles, p);
            plist_append(fx_rec28_list, p);
        }
    }
    while ((r4c = vi->rec4c_list) != NULL) {
        plist_remove(vi->rec4c_list, r4c);
        plist_append(fx_rec4c_free_list, r4c);
    }
    while ((r2c = vi->rec2c_list) != NULL) {
        plist_remove(vi->rec2c_list, r2c);
        plist_append(fx_rec2c_free_list, r2c);
    }

    rec = effect_base_record_index[index];
    if (--effect_base_stream_users[rec] == 0) {
        b = &effect_bases[rec];
        if (b->cutscene) {
            if (!b->level)
                effect_base_release(rec);
        } else if (b->category == EFFECT_CAT_CUTSCENE && effects_preload_count != 0) {
            effect_base_release(rec);
        }
    }
}

// 0x51db50  Confidence: exact. The low bit of each lower-cased character shifted in from the right
// (high word) xor the sum of the characters as written.
int effects_hash_name(const char *name)
{
    int sum = 0;
    int bits = 0;
    int i;
    int len = strlen(name);

    for (i = 0; i < len; i++) {
        bits = (tolower(name[i]) & 1) | (bits << 1);
        sum += name[i];
    }
    return (bits << 16) ^ sum;
}

// 0x51dbb0  Confidence: exact (unsigned counter; the loop runs on after a load). Loads the base named
// "preload.rfx" once and marks the Effects mempool behind it.
int effects_load_preload_rfx()
{
    unsigned int i;

    if (!effects_preload_done) {
        for (i = 0; i < (unsigned int)effects_num_bases; i++) {
            if (!effect_bases[i].resident && !_stricmp("preload.rfx", effect_bases[i].name)) {
                if (effect_base_load(i))
                    return -1;
                effects_preload_done = true;
                effects_level_mem_mark = effects_mempool.offset;
            }
        }
    }
    return 0;
}

// 0x51dc20  Confidence: exact (NEG / SBB / AND for the conditional). Releases everything behind
// preload.rfx (or the whole buffer when it is not loaded).
void effects_release_level_memory()
{
    effects_mempool.offset = effects_preload_done ? effects_level_mem_mark : 0;
    effects_release_range(effects_mem_buffer + effects_mempool.offset,
                          sizeof(effects_mem_buffer) - effects_mempool.offset);
}

// 0x51dc50  Confidence: exact (JS / JGE on the offset: signed).
void effects_release_range(char *start, int size)
{
    unsigned int i;
    int off;

    for (i = 0; i < (unsigned int)effects_num_bases; i++) {
        if (effect_bases[i].resident) {
            off = effect_bases[i].data - start;
            if (off >= 0 && off < size)
                effect_base_release(i);
        }
    }
}

// 0x51dca0  Confidence: exact. No caller.
int effects_stub_ret1()
{
    return 1;
}

// 0x51dcb0  Confidence: exact. True when each of the 25 names is empty, unknown or resident. No caller;
// it indexes effect_bases with the name index (vfx_find_name), not through effect_base_record_index.
int vfx_all_resident(const char **names)
{
    int i, id;

    for (i = 0; i < 25; i++) {
        if (names[i] != NULL && (id = vfx_find_name(names[i])) >= 0 && !effect_bases[id].resident)
            return 0;
    }
    return 1;
}

// 0x51dd00  Confidence: likely (the result in vi, NULL when the base or the instance is missing, as the
// binary's shared epilogue, match31; still differs in the copy of the base's two flag bits; the prefix test is written as a chain of ||; the copy of the base's two
// flag bits into the instance is a guess at the source shape). An instance of a named effect: effects
// whose names start with "nar-", "mi-" or "worl" pass flag 1 to vfx_instance_create. The base's
// bitmaps are loaded with preload-on-load set.
vfx_instance *vfx_load(const char *name, int a)
{
    char prefix[5];
    int index;
    vfx_base *base;
    vfx_instance *vi;
    effect_base *b;
    bool flag;

    strncpy(prefix, name, 4);
    prefix[4] = '\0';
    _strlwr(prefix);
    flag = !strncmp(prefix, "nar-", 4) || !strncmp(prefix, "mi-", 3) || !strncmp(prefix, "worl", 4);
    base = vfx_preload(name, &index);
    vi = NULL;
    if (base != NULL) {
        vi = vfx_instance_create(base, a, flag);
        if (vi != NULL) {
            b = &effect_bases[effect_base_record_index[index]];
            vi->id_bits = (vi->id_bits & 0x1ff) | (index << 9);
            vi->pos = base->pos;
            vi->_unk3c = base->_unk54;
            vi->category = b->category;
            vi->id_bits = (vi->id_bits & ~0x190) | ((((base->flags & 4) ? 4 : 0) | ((base->flags & 2) ? 2 : 0)) << 6);
            bm_set_preload_on_load(1);
            base->load_bitmaps(-1);
            bm_set_preload_on_load(0);
            effect_base_stream_users[effect_base_record_index[index]]++;
            b->bitmaps_pending = 0;
        }
    }
    return vi;
}

// 0x51de90  Confidence: exact. True for names that are not a base.
bool effects_base_is_resident(const char *name)
{
    int i = effects_find_base(name);

    if (i >= 0 && (unsigned int)i < (unsigned int)effects_num_bases)
    {
        bool r = effect_bases[i].resident;
        return r;
    }
    return true;
}

// 0x51dec0  Confidence: exact (the record index is a byte, so the < 0 test never fires).
bool effect_is_cutscene_base(int name_index)
{
    int i = effect_base_record_index[name_index];

    if (i >= 0 && i < effects_num_bases)
    {
        bool r = effect_bases[i].cutscene;
        return r;
    }
    return false;
}

// 0x51def0  Confidence: exact.
void effects_release_cutscene_memory()
{
    if (effects_preload_count)
        effects_release_range(effects_stream_buffer, EFFECTS_CUTSCENE_SIZE);
}

// ---- COMDATs at the end of the object (bodies in the headers) --------------------------------------
// 0x51df10 fx_rec48__ctor: vfx_instance's implicit constructor (vector3 at +0x30). Confidence: exact.
// 0x51df20 fx_rec98__ctor: fx_rec98's implicit constructor. Confidence: exact.
// 0x51df60 fx_rec_2vec__ctor: one body for vfx_light_desc, fx_rec2c and fx_rec28 (identical code folded). Confidence: exact.
// 0x51df80 fx_rec4c__ctor. Confidence: exact.
// 0x51dfb0 fx_recb0__ctor. Confidence: exact.
// 0x51dff0 Mempool__Mempool (vsdk/os/memory.h, inline). Confidence: exact.
// 0x51e010 Mempool__getPoolSpaceLeft (memory.h, inline). Confidence: exact.
// 0x51e020 vfx_get_length_seconds = vfx_emitter_desc::get_length_seconds (vfx.h, inline). Confidence: exact.
// 0x51e040 fixed8__set (vsdk/math/fixed.h, inline). Confidence: exact.
// 0x51e060 float_to_fixed8 (fixed.h, inline; `jmp _ftol`). Confidence: exact.
