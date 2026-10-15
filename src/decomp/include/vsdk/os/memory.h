// vsdk/os/memory.h - bump allocator pools and the memory report (vsdk\os\memory.cpp, 0x512516-0x512852
// path references). Not original source. Layout: symbols/types.h Mempool (name length from the
// 0x1c-byte stride of ip_general_mempool and effects_mempool).
#ifndef VSDK_OS_MEMORY_H
#define VSDK_OS_MEMORY_H

#include "vsdk/vsdk.h"

// operator new (0x512550) -> operator_new_dbg(size, 0, 0); operator delete (0x512570) -> free. Both are
// the global operators, so plain new / delete in the source reach them.
void *operator_new(unsigned int size);     // 0x512550 (the same function, called by name in credits)

class Mempool {                            // 0x1c bytes
public:
    // Inline (DECISIONS.md D12): item_paging.cpp's static initialiser expands it to a bare Copy call,
    // effect_mem.cpp (built /Ob0) calls the out-of-line COMDAT copy at 0x51dff0 at the end of its unit.
    Mempool(char *dest, int size, const char *name) { Copy(dest, size, name); }
    void Copy(char *dest, int size, const char *name);   // 0x512770 (returns void; EAX is not `this`)
    // Inline for the same reason: the COMDAT 0x51e010 follows the constructor in effect_mem.cpp.
    unsigned int getPoolSpaceLeft() { return size - offset; }

    char *destination;                     // +0x00 (char*: effect_mem.cpp stores char pointers; types.h uint8_t*)
    unsigned int size;                     // +0x04
    unsigned int offset;                   // +0x08 (credits saves and restores it)
    char name[0x10];                       // +0x0c ("Going into pool %s")
};
LAYOUT_CHECK(Mempool, sizeof(Mempool) == 0x1c);

void set_current_mempool(Mempool *pool);   // 0x5127f0
Mempool *get_current_mempool();            // 0x512800
extern Mempool level_mempool;              // 0x22e43c0
extern Mempool summon_mempool;             // 0x7592c0

// Memory report helpers (used by character_instance::report_memory).
void mem_report_indent();                  // 0x512600
void mem_report_unindent();                // 0x512610
int  mem_report_is_verbose();              // 0x512630
void mem_report_line(const char *label, const char *fmt, ...);   // 0x512640

#endif
