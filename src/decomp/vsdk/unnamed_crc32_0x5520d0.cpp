// unnamed_crc32_0x5520d0.cpp - Sum.exe v1.40, code 0x5520d0-0x552192: Crc32 (vsdk/os/crc32.h; module
// "-" in sum.symbols). The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: after the polygon clip object (its .bss ends 0x2f136e7) and before the ui_inputbox
// gadget (0x5521a0). .bss 0x2f136e8-0x2f13ae8 (the table and its flag), no .data. The constructor is
// inline (crc32.h; its COMDAT is 0x514eb0 in the vfile unit).
//
// The CRC has no initial or final inversion (value starts at 0 and is returned as is), so it is not
// the zlib / PKZIP CRC-32 of the same polynomial.

#include "vsdk/os/crc32.h"

unsigned int crc32_table[256];        // 0x2f136e8
bool crc32_table_inited;              // 0x2f13ae8

// 0x5520d0  Confidence: exact. value is the scratch register of the table build (stored every step).
void Crc32::init_table()
{
    unsigned int i;
    int bit;

    for (i = 0; i < 256; i++) {
        value = i;
        for (bit = 8; bit; bit--) {
            if (value & 1)
                value = (value >> 1) ^ 0xedb88320;
            else
                value = value >> 1;
        }
        crc32_table[i] = value;
    }
    crc32_table_inited = true;
    value = 0;
}

// 0x552120  Confidence: exact. sum.symbols Crc32__Crc32 (a reset; the constructor is inline).
void Crc32::init()
{
    value = 0;
}

// 0x552130  Confidence: exact.
unsigned int Crc32::update(const void *buf, int len)
{
    const unsigned char *p = (const unsigned char *)buf;

    if (!crc32_table_inited)
        init_table();
    while (len--)
        value = crc32_table[(value & 0xff) ^ *p++] ^ (value >> 8);
    return value;
}

// 0x552190  Confidence: exact. No caller.
unsigned int Crc32::get()
{
    return value;
}
