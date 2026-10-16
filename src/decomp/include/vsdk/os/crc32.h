// vsdk/os/crc32.h - CRC-32 (0x5520d0-0x552190; unit path not in the binary, directory and file name
// guessed). Not original source.
#ifndef VSDK_OS_CRC32_H
#define VSDK_OS_CRC32_H

#include "vsdk/vsdk.h"

class Crc32 {                         // 4 bytes
public:
    // Inline: name_hash_nocase expands it (mov [esp], 0); vfile's COMDAT copy 0x514eb0 (Crc32__Crc32_b)
    // is called by File::calc_crc32 and TextFile::Checksum (decomp22 D207).
    Crc32() { value = 0; }
    // sum.symbols Crc32__Crc32: value = 0 again, called after the constructor (TextFile::Checksum,
    // name_hash_nocase), so a reset, not the constructor (name ours).
    void init();                                     // 0x552120
    unsigned int update(const void *buf, int len);   // 0x552130 returns the value
    unsigned int get();                              // 0x552190 (no caller)
    // decomp70: builds crc32_table with value as its scratch, then value = 0 (update calls it once).
    void init_table();                               // 0x5520d0

    unsigned int value;               // +0x00
};
LAYOUT_CHECK(Crc32, sizeof(Crc32) == 4);

// decomp70 (vsdk/unnamed_crc32_0x5520d0.cpp).
extern unsigned int crc32_table[256]; // 0x2f136e8 polynomial 0xedb88320 (reflected)
extern bool crc32_table_inited;       // 0x2f13ae8

#endif
