// vsdk/os/memory.h - bump allocator pools and the memory report (vsdk\os\memory.cpp, 0x512516-0x512852
// path references; code 0x5124c0-0x5128ab). Not original source. Layout: symbols/types.h Mempool (name
// length from the 0x1c-byte stride of ip_general_mempool and effects_mempool).
#ifndef VSDK_OS_MEMORY_H
#define VSDK_OS_MEMORY_H

#include "vsdk/vsdk.h"

// operator new (0x512550) -> operator_new_dbg(size, 0, 0); operator delete (0x512570) -> free. Both are
// the global operators, so plain new / delete in the source reach them. memory.cpp also defines
// operator new(size, file, line) (0x5124c0), operator new[](size, file, line) (0x512530) and
// operator delete[] (0x512580); they are not declared here because no other unit calls them (the only
// caller of the array new is allocate_in_commonblock, operator delete[] has no caller; decomp22 D200).
void *operator_new(unsigned int size);     // 0x512550 (the same function, called by name in credits)

class Mempool {                            // 0x1c bytes
public:
    // Inline (DECISIONS.md D12): item_paging.cpp's static initialiser expands it to a bare Copy call,
    // effect_mem.cpp (built /Ob0) calls the out-of-line COMDAT copy at 0x51dff0 at the end of its unit.
    Mempool(char *dest, int size, const char *name) { Copy(dest, size, name); }
    // Inline and empty (decomp38, D1801): the 15 pools of characterinfo.cpp have no initialiser, and the
    // unit has 15 empty dynamic initialisers (init_empty_444850 .. init_empty_444a10).
    Mempool() {}
    void Copy(char *dest, int size, const char *name);   // 0x512770 (returns void; EAX is not `this`)
    // Summoner 2's allocate_in_mempool(pool, size); thiscall on the pool in Sum.exe, so a member here
    // (decomp22 D201). 16-byte rounded size, NULL (no error) when the pool is full.
    void *allocate_in_mempool(unsigned int size);         // 0x5127c0
    // Inline for the same reason: the COMDAT 0x51e010 follows the constructor in effect_mem.cpp.
    unsigned int getPoolSpaceLeft() { return size - offset; }
    // Inline as well: the out-of-line COMDAT 0x505540 is called by character.cpp (built /Ob0; decomp30).
    void ResetOffset() { offset = 0; }

    char *destination;                     // +0x00 (char*: effect_mem.cpp stores char pointers; types.h uint8_t*)
    unsigned int size;                     // +0x04
    unsigned int offset;                   // +0x08 (credits saves and restores it)
    char name[0x10];                       // +0x0c ("Going into pool %s")
};
LAYOUT_CHECK(Mempool, sizeof(Mempool) == 0x1c);

void set_current_mempool(Mempool *pool);   // 0x5127f0
Mempool *get_current_mempool();            // 0x512800
void atexit_nop_512590();                  // 0x512590 bare ret; MainLoop registers it with atexit
bool is_mempool_active();                  // 0x512810
extern Mempool level_mempool;              // 0x22e43c0
extern Mempool summon_mempool;             // 0x7592c0
extern Mempool *Pool;                      // 0x2693048 current pool, NULL = malloc (Summoner 2 name)

// The common block: one heap buffer per nesting level (TextFile::Open loads table files into it).
void *allocate_in_commonblock(int size);   // 0x512820 ("Common block is already locked.")
void deallocate_from_commonblock(void *p); // 0x512880
extern int Common_block_index;             // 0x2693050
extern void *Common_block_allocations[2];  // 0x2693054

// Level memory counters (memory.cpp .bss 0x2693004-0x2693034, written by level code; decomp22 D202).
extern int level_texture_mem;              // 0x2693004
extern int level_s3d_mem;                  // 0x269300c
extern int mem_bytes_allocated;            // 0x2693034 (read only: never counted on PC)

// Memory report helpers (used by character_instance::report_memory).
void mem_report_open(const char *filename);// 0x5125a0
void mem_report_close();                   // 0x5125e0
void mem_report_indent();                  // 0x512600
void mem_report_unindent();                // 0x512610
int  mem_report_is_verbose();              // 0x512630
void mem_report_line(const char *label, const char *fmt, ...);   // 0x512640
extern FILE *mem_report_file;              // 0x269303c
extern int mem_report_depth;               // 0x2693040
extern int mem_report_verbose;             // 0x2693044

// The six unnamed level counters (D202), zeroed by level_load (decomp42).
extern int level_mem_2693008;
extern int level_mem_2693010;
extern int level_mem_2693014;
extern int level_mem_2693018;
extern int level_mem_269301c;
extern int level_mem_2693020;

#endif
