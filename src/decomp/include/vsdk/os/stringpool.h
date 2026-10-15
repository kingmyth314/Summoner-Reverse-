// vsdk/os/stringpool.h - string pools (vsdk\os\stringpool.cpp, path reference 0x544c81; 0x544b80-0x544e3e).
// Not original source.
#ifndef VSDK_OS_STRINGPOOL_H
#define VSDK_OS_STRINGPOOL_H

#include "vsdk/vsdk.h"

class TextFile;

// The pool is a run of NUL-terminated strings; Add appends, AddUnique returns an existing match
// (_stricmp) first. The constructors, the destructor and SetLength follow the other members in the
// unit (0x544dc0-0x544e30), so they may be inline functions emitted at the end of the object; they are
// declared out of line here like the other members (decomp22 D203).
class StringPool {                    // 0x14 bytes (feedback_string_pool 0x941740..0x941754)
public:
    StringPool();                     // 0x544dc0 (zeroes the five fields)
    StringPool(char *pool, int max_length, bool allocated);   // 0x544de0
    StringPool(int max_length);       // 0x544e00 StringPool__StringPool_size (Allocate)
    ~StringPool();                    // 0x544e20 (Release)
    void Init(char *pool, int max_length, bool allocated);    // 0x544b80 (count is not reset)
    void Allocate(int max_length);    // 0x544ba0 (operator new)
    void Reset();                     // 0x544bd0
    void Release();                   // 0x544be0
    int  GetLength();                 // 0x544c10
    int  GetRemaining();              // 0x544c20 (no caller)
    char *GetBuffer();                // 0x544c30 (no caller)
    char *Add(const char *s);         // 0x544c40 "Length of string pool exceeded."
    const char *AddUnique(const char *s);                     // 0x544cd0
    char *AddQuoted(TextFile *tf);    // 0x544d40 ParseString between quotes (0x100 max) + Add
    const char *AddUniqueQuoted(TextFile *tf);                // 0x544d80
    void SetLength(unsigned int length);                      // 0x544e30 (ignored when > max_length)

    char *pool;                       // +0x00
    bool allocated;                   // +0x04
    int max_length;                   // +0x08
    int length;                       // +0x0C
    int count;                        // +0x10
};
LAYOUT_CHECK(StringPool, sizeof(StringPool) == 0x14);

#endif
