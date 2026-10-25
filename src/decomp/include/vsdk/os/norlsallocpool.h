// vsdk/os/norlsallocpool.h - the no-release bump allocator (norlsallocpool.cpp, 0x532360-0x532650;
// file name from sum.symbols, Summoner 2 gives the names; directory guessed; decomp44 writes the unit
// as src/decomp/vsdk/norlsallocpool.cpp). Not original source.
#ifndef VSDK_OS_NORLSALLOCPOOL_H
#define VSDK_OS_NORLSALLOCPOOL_H

#include "vsdk/vsdk.h"

// 0x1c bytes (items_alloc_pool 0x8b9ea0 is followed by its buffer at 0x8b9ebc; conversation +0x210 ..
// +0x22c). decomp20: the fields and the block methods, from the member functions' code.
class NoRlsAllocPool {
public:
    NoRlsAllocPool();                          // 0x532360
    ~NoRlsAllocPool();                         // 0x532380 (jmp Release)
    void Release();                            // 0x532390 deletes the owned block chain
    int  AddBlock();                           // 0x5323d0 0 = done, 1 = not owned / out of memory (decomp20)
    int  InitBlocks(int block_size, bool can_grow);  // 0x532420 nonzero = AddBlock failed (decomp20)
    void Init(void *buffer, int size);         // 0x532460
    void Reset();                              // 0x532480
    void *Alloc(int size);                     // 0x532490 NULL when full
    void *AllocZeroed(int size);               // 0x5324e0
    void Align();                              // 0x532510 (decomp20)
    int  GetUsed();                            // 0x532540 (decomp20)
    // decomp44 (vsdk/norlsallocpool.cpp). Grow and the GetUsed thunk have no caller in the binary.
    void *Grow(void *ptr, int size);           // 0x532570 resizes the last allocation, NULL on failure
    int  GetUsed_thunk();                      // 0x532620 jmp GetUsed (name from sum.symbols)
    void SetOffset(int offset);                // 0x532630 non-growing pools only

    void *blocks;                 // +0x00 chain of owned blocks (next at +0)
    char *start;                  // +0x04 first byte of the current block
    char *current;                // +0x08 next allocation
    unsigned int block_size;      // +0x0c
    unsigned int remaining;       // +0x10
    char *last_allocation;        // +0x14
    unsigned int owns_blocks : 1; // +0x18 bit 0 (set by InitBlocks, Release frees only then)
    unsigned int can_grow : 1;    //   bit 1 (Alloc adds a block when short)
};
LAYOUT_CHECK_32(NoRlsAllocPool, sizeof(NoRlsAllocPool) == 0x1c);

#endif
