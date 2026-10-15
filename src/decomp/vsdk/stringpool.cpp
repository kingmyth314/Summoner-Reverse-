// stringpool.cpp - Sum.exe v1.40, code 0x544b80-0x544e3e (vsdk\os\stringpool.cpp)
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// No data of its own except the two literals (0x5ac9e4 message, 0x5aca04 path). The global pools are
// defined, constructed and destroyed in the units that own them (their atexit handlers jump to
// StringPool::~StringPool).
//
// Function order in the binary: Init .. AddUniqueQuoted, then the three constructors, the destructor and
// SetLength (0x544dc0-0x544e30). The constructors may be inline functions emitted at the end of the
// object (as Mempool's in effect_mem.cpp, DECISIONS.md D12); they are written out of line here, in
// address order (decomp22 D203).

#include <string.h>
#include "vsdk/os/stringpool.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/debug.h"
#include "vsdk/parse/parse.h"

// Original __FILE__ of this unit (literal 0x5aca04).
#define STRINGPOOL_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\os\\stringpool.cpp"

// 0x544b80  Confidence: likely (store order allocated, max_length, length, pool is the scheduler's).
// count is not cleared.
void StringPool::Init(char *pool, int max_length, bool allocated)
{
    this->pool = pool;
    this->allocated = allocated;
    this->max_length = max_length;
    length = 0;
}

// 0x544ba0  Confidence: exact. new char[] reaches operator new (0x512550).
void StringPool::Allocate(int max_length)
{
    allocated = true;
    this->max_length = max_length;
    length = 0;
    count = 0;
    pool = new char[max_length];
}

// 0x544bd0  Confidence: exact.
void StringPool::Reset()
{
    length = 0;
    count = 0;
}

// 0x544be0  Confidence: exact. Nothing is cleared when pool is NULL.
void StringPool::Release()
{
    if (pool != NULL) {
        if (allocated)
            delete[] pool;
        pool = NULL;
        allocated = false;
        length = 0;
        max_length = 0;
        count = 0;
    }
}

// 0x544c10  Confidence: exact.
int StringPool::GetLength()
{
    return length;
}

// 0x544c20  Confidence: exact.
int StringPool::GetRemaining()
{
    return max_length - length;
}

// 0x544c30  Confidence: exact.
char *StringPool::GetBuffer()
{
    return pool;
}

// 0x544c40  Confidence: exact. Error line 233 (0xe9).
char *StringPool::Add(const char *s)
{
    if (s == NULL)
        return NULL;

    int len = strlen(s);
    char *dest = pool + length;
    int new_length = length + len + 1;
    if (new_length > max_length)
        VSDK_ERROR(STRINGPOOL_CPP, 233, "Length of string pool exceeded.");

    length = new_length;
    strcpy(dest, s);
    count++;
    return dest;
}

// 0x544cd0  Confidence: exact.
const char *StringPool::AddUnique(const char *s)
{
    if (s == NULL)
        return NULL;

    char *p = pool;
    int i;
    for (i = 0; i < count; i++) {
        if (_stricmp(p, s) == 0)
            break;
        p += strlen(p) + 1;
    }
    if (i != count)
        return p;
    return Add(s);
}

// 0x544d40  Confidence: exact.
char *StringPool::AddQuoted(TextFile *tf)
{
    char buf[256];

    tf->ParseString(buf, 256, '"', '"');
    return Add(buf);
}

// 0x544d80  Confidence: exact.
const char *StringPool::AddUniqueQuoted(TextFile *tf)
{
    char buf[256];

    tf->ParseString(buf, 256, '"', '"');
    return AddUnique(buf);
}

// 0x544dc0  Confidence: exact.
StringPool::StringPool()
{
    pool = NULL;
    allocated = false;
    max_length = 0;
    length = 0;
    count = 0;
}

// 0x544de0  Confidence: exact.
StringPool::StringPool(char *pool, int max_length, bool allocated)
{
    Init(pool, max_length, allocated);
}

// 0x544e00  Confidence: exact. StringPool__StringPool_size (localize_init_string_pool).
StringPool::StringPool(int max_length)
{
    Allocate(max_length);
}

// 0x544e20  Confidence: exact (jmp Release).
StringPool::~StringPool()
{
    Release();
}

// 0x544e30  Confidence: likely (the compare is unsigned: ja). Truncates back to a saved length.
void StringPool::SetLength(unsigned int length)
{
    if (length <= (unsigned int)max_length)
        this->length = length;
}
