// vsdk/os/norlsallocpool.h - the no-release bump allocator (norlsallocpool.cpp, 0x532360-0x532650;
// file name from sum.symbols, Summoner 2 gives the names; directory guessed). Not original source.
#ifndef VSDK_OS_NORLSALLOCPOOL_H
#define VSDK_OS_NORLSALLOCPOOL_H

#include "vsdk/vsdk.h"

// 0x1c bytes (items_alloc_pool 0x8b9ea0 is followed by its buffer at 0x8b9ebc). The fields are not
// needed by the pilots yet.
class NoRlsAllocPool {
public:
    NoRlsAllocPool();                          // 0x532360
    ~NoRlsAllocPool();                         // 0x532380 (jmp Release)
    void Release();                            // 0x532390
    void Init(void *buffer, int size);         // 0x532460
    void Reset();                              // 0x532480
    void *Alloc(int size);                     // 0x532490
    void *AllocZeroed(int size);               // 0x5324e0

    ubyte _data[0x1c];
};
LAYOUT_CHECK(NoRlsAllocPool, sizeof(NoRlsAllocPool) == 0x1c);

#endif
