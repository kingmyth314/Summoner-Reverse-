// unnamed_build_stamp_0x52f450.cpp - Sum.exe v1.40, code 0x52f450-0x52f568 (vsdk\ui in sum.symbols; the
// unit's file name is not in the binary, so the name is a placeholder). Written from the Sum.exe
// disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function:
// exact = expect identical code, likely = same semantics, small layout or scheduling risk, unsure = the
// source shape is a guess.
//
// The press / beta build stamp: a recipient name with a checksum and an expiry time that a stamping tool
// patches into the .data block; the retail exe has the placeholders (build_stamp_check returns 3).
//
// Unit bounds (NOTES.md): after the list box object's last COMDAT (0x52f440) and before the UI manager's
// initialiser run (0x52f570, table 0x588974). .data 0x5a9604-0x5a966f (the three stamp fields, then the
// four literals, two copies each: no string pooling), right after the list box literal (..0x5a9601). No
// .bss, no initialisers. It may also be the head of the UI manager object (that object has no .data to
// compare); kept apart because the stamp data is a block of its own.

#include <string.h>
#include <time.h>
#include "vsdk/ui/ui.h"
#include "vsdk/os/vstring.h"

// ---- .data (0x5a9604-0x5a966f) --------------------------------------------------------------------
int build_stamp_checksum = 0x01010101;   // 0x5a9604
long build_stamp_expiry = 0x02020202;    // 0x5a9608
// 0x5a960c. The size is 69 to 72 (the first literal follows at 0x5a9654, 4-aligned); 72 assumed (D4204).
char build_stamp_name[72] = "Read the Foundation Novels from Asimov.  I liked them.";

// 0x52f450  Confidence: exact (VC6 8966 byte match: one result variable, 3 set before the test, one return; D12300; the memcmp / strlen intrinsics on the placeholder literals, each written
// twice as the two copies in .data show; the rotate written with a carry variable). 3 = not stamped,
// 2 = expired, 1 = the name does not match its checksum, 0 = valid.
int build_stamp_check()
{
    int result;
    int sum;
    int i;

    result = 3;
    if (memcmp(&build_stamp_checksum, "\x01\x01\x01\x01", strlen("\x01\x01\x01\x01")) != 0) {
        result = 0;
        sum = 0;
        for (i = 0; i < (int)strlen(build_stamp_name); i++) {
            int carry;
            sum += build_stamp_name[i];
            carry = 0;
            if (sum & 1)
                carry = 1;
            sum >>= 1;
            if (carry)
                sum |= 0x80000000;
        }
        sum |= 0x80000000;
        if (sum != build_stamp_checksum)
            result = 1;

        if (memcmp(&build_stamp_expiry, "\x02\x02\x02\x02", strlen("\x02\x02\x02\x02")) != 0) {
            long expiry = build_stamp_expiry;
            if (time(NULL) > expiry)
                result = 2;
        }
    }
    return result;
}

// 0x52f530  Confidence: exact. No caller.
void build_stamp_get_name_String(String *out)
{
    *out = build_stamp_name;
}

// 0x52f540  Confidence: exact. The size is not read.
void build_stamp_get_name(char *out, int size)
{
    strcpy(out, build_stamp_name);
}
