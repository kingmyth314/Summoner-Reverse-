// unnamed_lfsr_0x559db0.cpp - Sum.exe v1.40, code 0x559db0-0x559e9e: lfsr_cipher, the three-register
// LFSR stream cipher of the open-file slots (vsdk\vfile in sum.symbols). The file name is a placeholder:
// no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): after the clip unit's closing COMDAT (vector3::operator[] 0x559da0),
// before the FTP client (0x559ea0, whose .data starts at 0x5adbfc). No .data, no .bss. Its constructor is
// the inline COMDAT 0x513eb0 in the vfile unit (file.h).
//
// The generator is the three-LFSR one of Schneier's Applied Cryptography (register A picks the output bit
// of B or C), with the textbook masks 0x80000062 / 0x40000020 / 0x10000002, but the feedback branch has no
// shift: the code XORs with the masks shifted right by one (0xc0000031 / 0x20000010 / 0x08000001), which is
// what `r ^ mask >> 1` gives (the shift binds to the mask), and the plain branch is an arithmetic shift
// without the rotation masks. It stays a symmetric XOR stream, so files still round-trip (NOTES.md).

#include "vsdk/vfile/file.h"

#define LFSR_MASK_A 0x80000062
#define LFSR_MASK_B 0x40000020
#define LFSR_MASK_C 0x10000002

// 0x559db0  Confidence: exact.
void lfsr_cipher::set_key(const unsigned int *key)
{
    regs[0] = key[0];
    if (regs[0] == 0)
        regs[0] = 0x891e7682;
    regs[1] = key[1];
    if (regs[1] == 0)
        regs[1] = 0x6c485f72;
    regs[2] = key[2];
    if (regs[2] == 0)
        regs[2] = 0x5bd48e3a;
}

// 0x559df0  Confidence: exact.
void lfsr_cipher::get_key(unsigned int *key)
{
    key[0] = regs[0];
    key[1] = regs[1];
    key[2] = regs[2];
}

// 0x559e10  Confidence: likely (the signed shifts: the registers are shifted with SAR).
void lfsr_cipher::crypt(void *buf, int len)
{
    ubyte *p = (ubyte *)buf;
    ubyte key = 0;
    int bit;
    int i;

    while (len-- != 0) {
        for (i = 8; i != 0; i--) {
            bit = 0;
            if (regs[0] & 1) {
                regs[0] = (regs[0] ^ LFSR_MASK_A >> 1) | 0x80000000;
                if (regs[1] & 1) {
                    regs[1] = (regs[1] ^ LFSR_MASK_B >> 1) | 0x40000000;
                    bit = 1;
                } else {
                    regs[1] = (int)regs[1] >> 1;
                }
            } else {
                regs[0] = (int)regs[0] >> 1;
                if (regs[2] & 1) {
                    regs[2] = (regs[2] ^ LFSR_MASK_C >> 1) | 0x10000000;
                    bit = 1;
                } else {
                    regs[2] = (int)regs[2] >> 1;
                }
            }
            key = (key << 1) | bit;
        }
        *p++ ^= key;
    }
}
