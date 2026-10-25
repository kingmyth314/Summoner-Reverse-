// unnamed_bitarray_0x55bd80.cpp - Sum.exe v1.40, code 0x55bd80-0x55bf88: BitArray (vsdk\geom in
// sum.symbols). The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): after the vfx channel code (0x55bd77), before the DirectSound unit's
// static initialiser (0x55bf90, table entry 649). No .data, no .bss. Only get has callers (the light
// tracks, vfx_mesh__frame_bit, all_equal); the effect files are relocated in place, so nothing builds a
// BitArray.

#include <string.h>
#include "vsdk/geom/vfx.h"
#include "vsdk/os/norlsallocpool.h"

// 0x55bd80  Confidence: exact.
BitArray::BitArray()
{
    data = NULL;
}

// 0x55bd90  Confidence: exact (tail jump).
BitArray::~BitArray()
{
    free();
}

// 0x55bda0  Confidence: exact.
int BitArray::alloc(int bits, bool own)
{
    int n = (bits + 7) >> 3;

    data = new char[n];
    if (data == NULL)
        return -1;
    num_bytes = n;
    owned = own;
    return 0;
}

// 0x55bde0  Confidence: exact.
void BitArray::attach(void *buf, int bits)
{
    data = (char *)buf;
    num_bytes = (bits + 7) >> 3;
    owned = false;
}

// 0x55be00  Confidence: exact.
int BitArray::alloc_from_pool(NoRlsAllocPool *pool, int bits)
{
    int n = (bits + 7) >> 3;

    data = (char *)pool->Alloc(n);
    if (data == NULL)
        return -1;
    num_bytes = n;
    owned = false;
    return 0;
}

// 0x55be40  Confidence: exact (inline memset).
void BitArray::fill(bool on)
{
    memset(data, on ? 0xff : 0, num_bytes);
}

// 0x55be80  Confidence: exact.
int BitArray::get(int bit)
{
    return ((ubyte)data[bit >> 3] & (1 << (bit & 7))) != 0;
}

// 0x55beb0  Confidence: exact.
void BitArray::set(int bit, int on)
{
    if (on)
        data[bit >> 3] |= 1 << (bit & 7);
    else
        data[bit >> 3] &= ~(1 << (bit & 7));
}

// 0x55bef0  Confidence: exact.
void BitArray::free()
{
    if (data != NULL && owned) {
        delete (char *)data;                  // (an ordinary pointer from a PTR32 field, vsdk.h)
        data = NULL;
    }
}

// 0x55bf20  Confidence: exact (the wanted bit in an int local after the bounds). As written: the whole bytes must be 0 for either value, and the tail
// loop runs from bits & ~7 to (bits & ~7) + bits, mod 8 (empty past the first byte); no caller.
bool BitArray::all_equal(unsigned int bits, bool value)
{
    unsigned int i;
    unsigned int start;
    unsigned int end;

    if (bits > num_bytes * 8)
        bits = num_bytes * 8;
    for (i = 0; i < bits >> 3; i++) {
        if (data[i] != 0)
            return false;
    }
    start = bits & ~7;
    end = (start + bits) & 7;
    int want = value != 0;
    for (i = start; i < end; i++) {
        if (get(i) != want)
            return false;
    }
    return true;
}
