// norlsallocpool.cpp - Sum.exe v1.40, code 0x532360-0x532653 (NoRlsAllocPool, the no-release bump allocator)
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// The file name is Summoner 2's (sum.symbols module column); the folder is not known. Placed at
// src/decomp/vsdk/norlsallocpool.cpp next to the other vsdk\os allocators; the header stays
// vsdk/os/norlsallocpool.h.
//
// No data of its own: the pools are defined, constructed and destroyed by the units that own them
// (items.cpp items_alloc_pool, cutscene_alloc_pool 0x5fccc0, conversation +0x210). The unit ends at
// 0x532653; time_format_hms (0x532660) is the next object.
//
// An owned block is block_size + 8 bytes from operator new: +0 the link to the next block, the data from
// +4 (start). The four spare bytes cover the 4-byte alignment of the first allocation.

#include <string.h>
#include "vsdk/os/norlsallocpool.h"

// 0x532360  Confidence: exact (VC6 byte match, match14; was likely) (store order start, blocks, the two bits, last_allocation).
NoRlsAllocPool::NoRlsAllocPool()
{
    start = NULL;
    blocks = NULL;
    owns_blocks = 0;
    can_grow = 0;
    last_allocation = NULL;
}

// 0x532380  Confidence: exact (jmp Release).
NoRlsAllocPool::~NoRlsAllocPool()
{
    Release();
}

// 0x532390  Confidence: exact. Pools set up with Init (a caller's buffer) are left alone.
void NoRlsAllocPool::Release()
{
    if (owns_blocks) {
        void **block = (void **)blocks;
        while (block != NULL) {
            void **next = (void **)block[0];
            operator delete(block);
            block = next;
        }
        blocks = NULL;
        owns_blocks = 0;
        start = NULL;
    }
}

// 0x5323d0  Confidence: exact (VC6 byte match, match14; was exact). 0 = a new block is current, 1 = not owned or out of memory.
int NoRlsAllocPool::AddBlock()
{
    if (!owns_blocks)
        return 1;

    void **block = (void **)operator new(block_size + 8);
    if (block == NULL)
        return 1;

    void *prev = blocks;
    blocks = block;
    block[0] = prev;
    start = (char *)(block + 1);
    Reset();
    return 0;
}

// 0x532420  Confidence: exact (VC6 byte match, match14; was exact) (neg / sbb / neg is the != 0).
int NoRlsAllocPool::InitBlocks(int block_size, bool can_grow)
{
    this->block_size = block_size;
    this->can_grow = can_grow != 0;
    owns_blocks = 1;
    return AddBlock() != 0;
}

// 0x532460  Confidence: exact.
void NoRlsAllocPool::Init(void *buffer, int size)
{
    start = (char *)buffer;
    owns_blocks = 0;
    can_grow = 0;
    block_size = size;
    Reset();
}

// 0x532480  Confidence: exact (tail jump to Align).
void NoRlsAllocPool::Reset()
{
    if (start != NULL) {
        current = start;
        Align();
    }
}

// 0x532490  Confidence: exact. A growing pool starts a new block when the current one is short; the
// rest of the old block is lost.
void *NoRlsAllocPool::Alloc(int size)
{
    if (remaining < (unsigned int)size) {
        if (!can_grow || AddBlock() != 0 || remaining < (unsigned int)size)
            return NULL;
    }

    char *p = current;
    current = p + size;
    Align();
    last_allocation = p;
    return p;
}

// 0x5324e0  Confidence: exact (memset expanded: rep stosd / rep stosb).
void *NoRlsAllocPool::AllocZeroed(int size)
{
    void *p = Alloc(size);
    if (p == NULL)
        return NULL;
    memset(p, 0, size);
    return p;
}

// 0x532510  Confidence: likely (the clamp: sets / dec / and is `r < 0 ? 0 : r`).
void NoRlsAllocPool::Align()
{
    int misalign = (int)current & 3;
    if (misalign)
        current = current - misalign + 4;

    int r = (int)(start + block_size - current);
    remaining = r < 0 ? 0 : r;
}

// 0x532540  Confidence: likely (loop shape). The current block counts its used bytes, every older block
// counts in full (its tail is not known any more).
int NoRlsAllocPool::GetUsed()
{
    int used = block_size - remaining;
    void **block = (void **)blocks;

    if (block != NULL) {
        for (block = (void **)block[0]; block != NULL; block = (void **)block[0])
            used += block_size;
    }
    return used;
}

// 0x532570  Confidence: exact (the two "fits in place" tests share one tail; memcpy written before the last_allocation store). Only the last allocation
// can be resized. It grows in place while the block has room; otherwise a growing pool starts a new
// block and copies the old bytes (old size = current - ptr) into a fresh allocation. No caller.
void *NoRlsAllocPool::Grow(void *ptr, int size)
{
    if ((char *)ptr != last_allocation)
        return NULL;

    char *end = (char *)ptr + size;
    if (end <= current || (unsigned int)((char *)ptr - current + size) <= remaining) {
        current = end;
        Align();
        return ptr;
    }

    if (!can_grow)
        return NULL;

    int old_size = current - (char *)ptr;
    if (AddBlock() != 0)
        return NULL;
    if (remaining < (unsigned int)size)
        return NULL;

    char *p = (char *)Alloc(size);
    if (p == NULL)
        return NULL;
    memcpy(p, ptr, old_size);
    last_allocation = p;
    return p;
}

// 0x532620  Confidence: likely (a second member whose body is the GetUsed call: jmp GetUsed). No caller.
int NoRlsAllocPool::GetUsed_thunk()
{
    return GetUsed();
}

// 0x532630  Confidence: exact. Moves the cursor of a fixed (non-growing) pool; offsets past the end are
// ignored.
void NoRlsAllocPool::SetOffset(int offset)
{
    if (!can_grow && (unsigned int)offset <= block_size) {
        current = start + offset;
        remaining = block_size - offset;
    }
}
