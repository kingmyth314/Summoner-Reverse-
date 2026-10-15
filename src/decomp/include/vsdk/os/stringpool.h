// vsdk/os/stringpool.h - string pools (vsdk\os\stringpool.cpp, path reference 0x544c81; 0x544b80-0x544e20).
// Not original source.
#ifndef VSDK_OS_STRINGPOOL_H
#define VSDK_OS_STRINGPOOL_H

#include "vsdk/vsdk.h"

class StringPool {                    // 0x14 bytes (feedback_string_pool 0x941740..0x941754)
public:
    StringPool();                     // 0x544dc0 (zeroes the five fields)
    StringPool(char *pool, int max_length, bool allocated);   // 0x544de0
    ~StringPool();                    // 0x544e20 (Release)
    void Init(char *pool, int max_length, bool allocated);    // 0x544b80
    void Reset();                     // 0x544bd0
    const char *AddUnique(const char *s);                     // 0x544cd0

    char *pool;                       // +0x00
    bool allocated;                   // +0x04
    int max_length;                   // +0x08
    int length;                       // +0x0C
    int count;                        // +0x10
};
LAYOUT_CHECK(StringPool, sizeof(StringPool) == 0x14);

#endif
