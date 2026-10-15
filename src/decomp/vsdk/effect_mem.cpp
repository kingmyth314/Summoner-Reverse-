// effect_mem.cpp - Sum.exe v1.40, code 0x51bb70-0x51e018 (vsdk\geom\effect_mem.cpp)
// Written from the Sum.exe disassembly (build/sum.asm). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range. symbols/sum.symbols tags 11 functions (0x51bcf0-0x51cf60) with this file. Everything
// between them (24 untagged functions) and the static initialisers before them (0x51bb70-0x51bcc0)
// belong to the same object: effects_init_pools threads the records the 0x51bb70.. initialisers
// construct, and the compiler-generated record constructors (0x51df10-0x51dfee) and the out-of-line
// Mempool members (0x51dff0, 0x51e010) close the unit. This file holds 0x51bb70-0x51d240 (all of it)
// and gets 0x51df10-0x51e018 from the definitions and declarations. NOT written yet, although in the
// unit: 0x51d250 vfx_instance_create .. 0x51def0 effects_release_cutscene_memory (13 functions; see
// NOTES.md). The previous unit ends with ftp_get__scalar_deleting_dtor (0x51bb50).
//
// Build flags of this unit differ from the game code: no string pooling (the __FILE__ literal and
// " \r\n\t:," each appear more than once in .data) and no inline expansion (cmd_show_effect_mem
// jumps to an empty function, the record constructors call vector3() out of line, and the Mempool
// constructor and getPoolSpaceLeft are called). /O2 without /Gf and with /Ob0 fits; see NOTES.md.

#include <stdio.h>
#include <string.h>
#include "effect_mem.h"

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
// 0x51bd20 init_effects_string_pool (call 0x51bd30, jmp 0x51bd40), 0x51bd30 effects_string_pool_ctor
// (mov ecx; jmp StringPool::StringPool), 0x51bd40 effects_string_pool_register_dtor (atexit 0x51bd50),
// 0x51bd50 effects_string_pool_dtor (mov ecx; jmp StringPool::~StringPool). Confidence: exact.
StringPool effects_string_pool;                   // 0x2c7dd98
// 0x51bd60 init_ccmd_show_effect_mem_thunk / 0x51bd70 init_ccmd_show_effect_mem. Confidence: exact.
console_command ccmd_show_effect_mem("show_effect_mem", "Shows how much memory the effects system is using.",
                                     cmd_show_effect_mem); // 0x2c90a60

// The rest in address order.
ubyte effect_base_slot[MAX_EFFECT_NAMES];         // 0x2722820 node index per effect name, 0xff unknown
const char *effect_base_names[MAX_EFFECT_NAMES];  // 0x27281d8
int effects_stream_slots[NUM_STREAM_SLOTS];       // 0x27288b8
static char effects_string_buffer[0x3000];        // 0x27288d8 effects_string_pool storage (label ours)
int effects_stream_slots_saved[NUM_STREAM_SLOTS]; // 0x272b8dc
int effects_level_mem_mark;                       // 0x2c7dd90 (effects_load_preload_rfx, not written yet)
int effects_stream_slots_b_saved[NUM_BIG_SLOTS];  // 0x2c8e7b0
ubyte effect_base_record_index[MAX_EFFECT_NAMES]; // 0x2c8e7c8 effect_bases index per name
static int effects_name_order[MAX_EFFECT_NAMES];  // 0x2c8e980 set to 0..439 by effects_load_bases, no reader found (label ours)
effect_base effect_bases[MAX_EFFECT_BASES];       // 0x2c8f060
ubyte effect_base_stream_users[MAX_EFFECT_BASES]; // 0x2c9f1b0
int effects_stream_slots_b[NUM_BIG_SLOTS];        // 0x2c9f270
static int effects_name_map[MAX_EFFECT_NAMES];    // 0x2c9f288 set to -1 by effects_load_bases, no reader found (label ours)
int effects_vfx_counter_2c9f968;                  // 0x2c9f968 fx_frame, vfx_instance_free, vfx_mesh_instance__render (label ours)
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
bool effects_preload_done;                        // 0x2c9f994 (effects_load_preload_rfx, not written yet)
int effects_num_bases;                            // 0x2c9f998
int effects_num_names;                            // 0x2c9f99c
// Not placed: 0x2c90a70-0x2c98a6f and 0x2c98a8c-0x2c9f1af lie inside this unit's .bss but are only
// used by the functions not written yet.

// Append to a circular list given by a head pointer (NULL = empty).
#define fx_list_append(head, rec) \
    if ((head) == NULL) { (rec)->next = (rec); (rec)->prev = (rec); (head) = (rec); } \
    else { (rec)->prev = (head)->prev; (rec)->next = (head); (head)->prev->next = (rec); (head)->prev = (rec); }

// 0x51bd90  Confidence: likely (JMP to the empty function 0x51d240: a call that /Ob0 does not
// expand; the function's own body was compiled out).
void cmd_show_effect_mem()
{
    effects_show_memory_usage();
}

// 0x51bda0  Confidence: likely (the limit is a nested conditional: NEG/SBB for the big_slot choice).
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

    if (rem)
        value = value - rem + 0x40;
    return value;
}

// 0x51bea0  Confidence: likely (the two flag stores are merged into AND 0x3effffff / OR 0x81000000).
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

    if (id < 0)
        return -1;
    index = effect_base_record_index[id];
    if (effect_bases[index].resident || effect_bases[index].streamed)
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
    int slot, i;

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

// 0x51c370  Confidence: likely (the saved-slot loops are pointer loops with signed compares, as VC6
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
            b = &effect_bases[effect_base_record_index[i]];
            if (b->resident && b->bitmaps_pending) {
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

// 0x51c490  Confidence: likely (version > 9 and version < 9 are two tests of one compare: two
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

// 0x51c520  Confidence: likely. Six slots, but the index is masked with 5 (not taken modulo 6), as
// the binary does: only slots 0, 1, 4 and 5 are ever used.
int effects_alloc_stream_slot(int base)
{
    int i, slot;

    for (i = effects_stream_slot_b_next; i < effects_stream_slot_b_next + NUM_BIG_SLOTS; i++) {
        slot = i & 5;
        if (effects_stream_slots_b[slot] == -1) {
            effects_stream_slots_b[slot] = base;
            effects_stream_slot_b_next = (slot + 1) & 5;
            return slot;
        }
    }
    for (i = effects_stream_slot_b_next; i < effects_stream_slot_b_next + NUM_BIG_SLOTS; i++) {
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

// 0x51c5c0  Confidence: likely. Eight slots scanned six at a time with mask 7; the cursor is masked
// with 5, so it only ever restarts at 0, 1, 4 or 5.
int effects_find_stream_slot(int base)
{
    int i, slot;

    for (i = effects_stream_slot_next; i < effects_stream_slot_next + 6; i++) {
        slot = i & 7;
        if (effects_stream_slots[slot] == -1) {
            effects_stream_slots[slot] = base;
            effects_stream_slot_next = (slot + 1) & 5;
            return slot;
        }
    }
    for (i = effects_stream_slot_next; i < effects_stream_slot_next + 6; i++) {
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

// 0x51c6a0  Confidence: likely. *out_offset is always 0 here.
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

// 0x51c720  Confidence: likely. The node index of an effect name is cached in effect_base_slot.
// The node pointer is loaded before the range tests, and the pointer test is signed (JLE), so the
// pointers were compared as ints.
vfx_base *effect_get_base(int id)
{
    int index = effect_base_record_index[id];
    effect_base *b = &effect_bases[index];
    vfx_node_entry *nodes;
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
        nodes = (vfx_node_entry *)b->data;
        for (i = 0; i < b->num_nodes; i++) {
            if (nodes[i].hash == hash) {
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

// 0x51ca30  Confidence: likely (vfx_node_relocate gets the two arguments swapped; their meaning is
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

// 0x51ccd0  Confidence: likely (memset as REP STOSD with -1; the loop runs over all 192 records).
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

// 0x51cd40  Confidence: likely (the first five heads stay in registers through their loops, the
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
        r48->_unk18 = 0;
        r48->_unk1c = 0;
        r48->_unk20 = 0;
        r48->_unk28 = 0;
        r48->flags &= ~0x40;
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

// 0x51d240  Confidence: likely. Empty: its body (a memory report) is compiled out in this build.
// Name ours (no symbol).
void effects_show_memory_usage()
{
}
