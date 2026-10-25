// unnamed_vfile_rw_0x525720.cpp - Sum.exe v1.40, code 0x525720-0x52693c (vsdk\vfile; the unit's file
// name is not in the binary, so the name is a placeholder). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact = expect
// identical code, likely = same semantics, small layout or scheduling risk, unsure = the source shape is
// a guess.
//
// File's typed readers (each checks the slot version first and returns the default when it is too old
// or the read failed), its string and line readers, the buffered File::read, the writers and the sync
// calls that read or write by the open mode. The object is built without inline expansion like the
// other vsdk units: the vector / quaternion / matrix copies call their COMDATs (vector3__assign 0x413190,
// quaternion__assign 0x50f9b0, matrix3__copy 0x4a4320, matrix43__copy 0x51b6c0).
//
// Unit bounds (NOTES.md): its own initialiser run 0x525720-0x525790 (table 0x588964-0x588970, after the
// vector math unit's), then the File methods to File::sync_bytes; ui_button's first function 0x526940
// follows. .bss 0x2cbf4c8-0x2cbf553: the four default values (the File defaults that file_init sets)
// and the six all-zero "" literals 0x2cbf53c-0x2cbf550 (read_String, read_String8, read_line_String,
// write_string, write_string_z, write_line). .data 0x5a8b64-0x5a8b6b: write_line's two "\n" literals
// (window.cpp's start at 0x5a8b6c).

#include <stdio.h>
#include <string.h>
#include "vsdk/vfile/file.h"
#include "vsdk/vfile/packfile/file_packfile.h"
#include "vsdk/os/vstring.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"

// ---- .bss ------------------------------------------------------------------------------------------
// The constructors are the empty default ones (0x505530 for all three small types, folded; 0x50f660
// matrix::matrix). Initialiser order = definition order: 0x525720, 0x525740, 0x525760, 0x525780 (each a
// jmp to the constructor call).
// 0x525720  init_file_zero_vector. Confidence: exact (compiler-generated).
// 0x525740  init_file_identity_quaternion. Confidence: exact (compiler-generated).
// 0x525760  init_file_default_matrix. Confidence: exact (compiler-generated).
// 0x525780  init_vector3_2cbf4e8. Confidence: exact (compiler-generated). The type is a guess: a matrix43
//           by the gap size (0x2cbf4e8-0x2cbf517, 0x30 bytes; any type whose constructor folds into
//           0x505530); nothing reads or writes it (read_matrix43 takes its default as an argument).
vector3 file_zero_vector;                  // 0x2cbf4d8
quaternion file_identity_quaternion;       // 0x2cbf4c8
matrix file_default_matrix;                // 0x2cbf518
matrix43 file_default_matrix43;            // 0x2cbf4e8

// 0x5257a0  Confidence: exact (cdecl: a static member). uint16 length, then the text.
void File::read_String_raw(File *f, String *s)
{
    s->free();
    s->max_len = 0;
    int len = f->read_uint16(0, 0);
    if (len) {
        s->alloc(len);
        f->read(s->buf, len, 0, 0);
        s->buf[len] = '\0';
    }
}

// 0x525800  Confidence: exact (VC6 byte match, cl 8966; was likely: the byte is read into the dead min_ver slot, as
// in the readers below).
bool File::read_bool(int min_ver, bool def)
{
    char v;

    if (!check_version(min_ver))
        return def;
    read(&v, 1, 0, 0);
    if (error())
        return def;
    return v ? true : false;
}

// 0x525850  Confidence: exact.
char File::read_int8(int min_ver, char def)
{
    char v;

    if (!check_version(min_ver))
        return def;
    read(&v, 1, 0, 0);
    return error() ? def : v;
}

// 0x5258a0  Confidence: exact.
ubyte File::read_uint8(int min_ver, ubyte def)
{
    ubyte v;

    if (!check_version(min_ver))
        return def;
    read(&v, 1, 0, 0);
    return error() ? def : v;
}

// 0x5258f0  Confidence: exact.
short File::read_int16(int min_ver, short def)
{
    short v;

    if (!check_version(min_ver))
        return def;
    read(&v, 2, 0, 0);
    if (error())
        return def;
    return v;
}

// 0x525940  Confidence: exact.
ushort File::read_uint16(int min_ver, ushort def)
{
    ushort v;

    if (!check_version(min_ver))
        return def;
    read(&v, 2, 0, 0);
    if (error())
        return def;
    return v;
}

// 0x525990  Confidence: exact.
int File::read_int(int min_ver, int def)
{
    int v;

    if (!check_version(min_ver))
        return def;
    read(&v, 4, 0, 0);
    return error() ? def : v;
}

// 0x5259e0  Confidence: exact.
unsigned int File::read_uint(int min_ver, unsigned int def)
{
    unsigned int v;

    if (!check_version(min_ver))
        return def;
    read(&v, 4, 0, 0);
    return error() ? def : v;
}

// 0x525a30  Confidence: exact (two fld returns, as with read_bool's if).
float File::read_float(int min_ver, float def)
{
    float v;

    if (!check_version(min_ver))
        return def;
    read(&v, 4, 0, 0);
    if (error())
        return def;
    return v;
}

// 0x525a80  Confidence: exact. The 12 bytes are read with min_ver passed on (read checks it again).
void File::read_vector(vector3 *v, int min_ver, const vector3 *def)
{
    if (!check_version(min_ver)) {
        *v = *def;
        return;
    }
    read(v, 12, min_ver, 0);
}

// 0x525ad0  Confidence: exact.
void File::read_quaternion(quaternion *q, int min_ver, const quaternion *def)
{
    if (!check_version(min_ver)) {
        *q = *def;
        return;
    }
    q->x = read_float(0, 0.0f);
    q->y = read_float(0, 0.0f);
    q->z = read_float(0, 0.0f);
    q->w = read_float(0, 0.0f);
}

// 0x525b40  Confidence: exact. Forward vector first (the order of File::write_matrix).
void File::read_matrix(matrix *m, int min_ver, const matrix *def)
{
    if (!check_version(min_ver)) {
        *m = *def;
        return;
    }
    read_vector(&m->fvec, 0, &file_zero_vector);
    read_vector(&m->rvec, 0, &file_zero_vector);
    read_vector(&m->uvec, 0, &file_zero_vector);
}

// 0x525ba0  Confidence: exact. The rotation column by column, then the position; then it sets the
// fourth column of a 4x4 matrix: with matrix43's m[4][3] those are m[1][0], m[2][0], m[3][0] (just read,
// now 0) and one float past the end (1.0f at +0x30). Only File::sync_matrix43 calls it, and nothing calls
// that (NOTES.md).
void File::read_matrix43(matrix43 *m, int min_ver, const matrix43 *def)
{
    if (!check_version(min_ver)) {
        *m = *def;
        return;
    }
    m->m[0][0] = read_float(0, 0.0f);
    m->m[1][0] = read_float(0, 0.0f);
    m->m[2][0] = read_float(0, 0.0f);
    m->m[0][1] = read_float(0, 0.0f);
    m->m[1][1] = read_float(0, 0.0f);
    m->m[2][1] = read_float(0, 0.0f);
    m->m[0][2] = read_float(0, 0.0f);
    m->m[1][2] = read_float(0, 0.0f);
    m->m[2][2] = read_float(0, 0.0f);
    m->m[3][0] = read_float(0, 0.0f);
    m->m[3][1] = read_float(0, 0.0f);
    m->m[3][2] = read_float(0, 0.0f);
#ifdef MODERN_BUILD
    // The same in-bounds stores; the 1.0f one float past the matrix is left out (no caller).
    m->m[1][0] = 0.0f;
    m->m[2][0] = 0.0f;
    m->m[3][0] = 0.0f;
#else
    m->m[0][3] = 0.0f;
    m->m[1][3] = 0.0f;
    m->m[2][3] = 0.0f;
    m->m[3][3] = 1.0f;
#endif
}

// 0x525c90  Confidence: exact.
void File::read_String(String *s, int min_ver, const char *def)
{
    if (!check_version(min_ver)) {
        if (def)
            *s = def;
        else
            *s = "";
        return;
    }
    read_String_raw(this, s);
}

// 0x525ce0  Confidence: exact. Not length-prefixed (sum.symbols says int8 length): bytes up to and with
// the NUL, into a 256-byte stack buffer without a bound. No caller.
void File::read_String8(String *s, int min_ver, const char *def)
{
    char buf[256];

    if (!check_version(min_ver)) {
        if (def)
            *s = def;
        else
            *s = "";
        return;
    }
    char c = read_int8(0, 0);
    buf[0] = c;
    if (c) {
        char *p = &buf[1];
        do {
            c = read_int8(0, 0);
            *p++ = c;
        } while (c);
    }
    *s = buf;
}

// 0x525d80  Confidence: exact. One line into s; every CR / LF after it is skipped (blank lines too) and
// the first byte of the next line is pushed back with seek(-1, 1). Returns the length. No caller.
int File::read_line_String(String *s, int min_ver)
{
    *s = "";
    if (!check_version(min_ver))
        return 0;
    int n = 0;
    char c = read_int8(0, 0);
    while (c && c != '\r' && c != '\n') {
        s->append_char(c);
        c = read_int8(0, 0);
        n++;
    }
    while (c == '\r' || c == '\n')
        c = read_int8(0, 0);
    if (c)
        seek(-1, 1);
    return n;
}

// 0x525e20  Confidence: exact. -1 = 0xffff. Too long: max - 1 chars are kept and the rest is skipped
// byte by byte. The default is strncpy'd (no terminator when it is max chars or longer).
void File::read_string_buf(char *buf, int max, int min_ver, const char *def)
{
    if (max == -1)
        max = 0xffff;
    if (!check_version(min_ver)) {
        if (def)
            strncpy(buf, def, max);
        return;
    }
    int len = read_uint16(0, 0);
    if (len != 0) {
        if (len < max) {
            read(buf, len, 0, 0);
            buf[len] = '\0';
            return;
        }
        read(buf, max - 1, 0, 0);
        buf[max - 1] = '\0';
        for (int skip = len - max + 1; skip > 0; skip--)
            read_int8(0, 0);
        return;
    }
    buf[0] = '\0';
}

// 0x525ee0  Confidence: exact. NUL-terminated; up to max chars are kept and the terminator then goes to
// buf[max], one byte past a max-sized buffer (the .plr readers pass the buffer size; NOTES.md).
void File::read_string(char *buf, int max, int min_ver, const char *def)
{
    if (max == -1)
        max = 0xffff;
    if (!check_version(min_ver)) {
        if (def)
            strcpy(buf, def);
        return;
    }
    int i = 0;
    while (1) {
        int c = read_int8(0, 0);
        if (c == 0)
            break;
        if (i < max)
            buf[i++] = c;
    }
    buf[i] = '\0';
}

// 0x525f60  Confidence: exact. The size argument is never used: a line longer than the buffer
// overflows it (NOTES.md). Line endings as read_line_String.
int File::read_line(char *buf, int size, int min_ver)
{
    buf[0] = '\0';
    if (!check_version(min_ver))
        return 0;
    int n = 0;
    char c = read_int8(0, 0);
    while (c && c != '\r' && c != '\n') {
        buf[n] = c;
        c = read_int8(0, 0);
        n++;
    }
    buf[n] = '\0';
    while (c == '\r' || c == '\n')
        c = read_int8(0, 0);
    if (c)
        seek(-1, 1);
    return n;
}

// 0x525fe0  Confidence: exact (VC6 byte match, D18502: the refill's `left` from a `pos` local). Was unsure (the refill block; m-1020 w11, D12008: len copied at entry and decremented, as the
// binary's frame has it; D16202: the request copied before the slot lookup, the remainder in its own
// variable and the refill size written back into len, as the binary's slots show: 84 -> 54 differing lines).
// The slot's 32 KB buffer first; the rest of a request of
// 32 KB or more is read directly (the buffer is dropped), a smaller one refills the buffer from the
// current position. After the fread refill buf_len is the requested count, not the count read (a short
// read sets -99999 unless at end of file). Decrypts in place, counts, and sets -1 when fewer than len
// bytes came. The binary passes a fifth argument 0 to packfile_fread (never read; D4106).
int File::read(void *buf, int len, int min_ver, int unused)
{
    int total = len;
    file_slot *s = &file_slots[slot];

    s->num_ops++;
    if (s->version < min_ver || s->error)
        return 0;

    int avail = s->buf_len - s->buf_pos;
    int n = imin(avail, len);
    memcpy(buf, s->buffer + s->buf_pos, max(0, n));
    s->buf_pos += n;
    int rem = len - n;
    if (rem > 0) {
        char *dst = (char *)buf + n;
        int got;
        if (rem >= FILE_BUFFER_SIZE) {
            s->buf_pos = 0;
            s->buf_len = 0;
            s->buf_start = -1;
            if (s->fp)
                got = fread(dst, 1, rem, s->fp);
            else
                got = packfile_fread(dst, 1, rem, packentry, 0);
        } else {
            int pos = s->pos;                 // D18502: pos loaded first, size - n - pos in this order
            int left = s->size - n - pos;
            len = imin(left, FILE_BUFFER_SIZE);
            if (s->fp) {
                s->buf_len = fread(s->buffer, 1, len, s->fp);
                if (s->buf_len < len && !eof())
                    file_slots[slot].error = FILE_ERR_IO;
                got = len;
            } else {
                got = packfile_fread(s->buffer, 1, len, packentry, 0);
            }
            s->buf_len = got;
            s->buf_start = n + s->pos;
            got = imin(left, rem);
            if (got < 0)
                got = 0;
            memcpy(dst, s->buffer, got);
            s->buf_pos = got;
        }
        n += got;
    }
    if (s->flags & 1)
        s->cipher.crypt(buf, n);
    s->pos += n;
    s->num_bytes += n;
    if (total != n)
        s->error = FILE_ERR_SHORT;
    return n;
}

// 0x5261f0  Confidence: exact. Writes 1 for true, 0 for any other byte value.
void File::write_bool(bool v)
{
    v = (v == 1);
    write(&v, 1);
}

// 0x526210  Confidence: exact.
void File::write_int8(char v)
{
    write(&v, 1);
}

// 0x526220  Confidence: exact.
void File::write_uint8(ubyte v)
{
    write(&v, 1);
}

// 0x526230  Confidence: exact.
void File::write_int16(short v)
{
    write(&v, 2);
}

// 0x526240  Confidence: exact.
void File::write_uint16(ushort v)
{
    write(&v, 2);
}

// 0x526250  Confidence: exact.
void File::write_int(int v)
{
    write(&v, 4);
}

// 0x526260  Confidence: exact.
void File::write_uint(unsigned int v)
{
    write(&v, 4);
}

// 0x526270  Confidence: exact.
void File::write_float(float v)
{
    write(&v, 4);
}

// 0x526280  Confidence: exact.
void File::write_vector(const vector3 *v)
{
    write(v, 12);
}

// 0x526290  Confidence: exact.
void File::write_quaternion(const quaternion *q)
{
    write_float(q->x);
    write_float(q->y);
    write_float(q->z);
    write_float(q->w);
}

// 0x5262d0  Confidence: exact.
void File::write_matrix(const matrix *m)
{
    write_vector(&m->fvec);
    write_vector(&m->rvec);
    write_vector(&m->uvec);
}

// 0x526300  Confidence: exact. The order read_matrix43 reads.
void File::write_matrix43(const matrix43 *m)
{
    write_float(m->m[0][0]);
    write_float(m->m[1][0]);
    write_float(m->m[2][0]);
    write_float(m->m[0][1]);
    write_float(m->m[1][1]);
    write_float(m->m[2][1]);
    write_float(m->m[0][2]);
    write_float(m->m[1][2]);
    write_float(m->m[2][2]);
    write_float(m->m[3][0]);
    write_float(m->m[3][1]);
    write_float(m->m[3][2]);
}

// 0x526390  Confidence: exact. NULL writes an empty string; len < 0 = strlen.
void File::write_string(const char *s, int len)
{
    if (s == NULL)
        s = "";
    if (len < 0)
        len = strlen(s);
    write_uint16(len);
    if (len)
        write(s, len);
}

// 0x5263e0  Confidence: exact.
void File::write_string_z(const char *s)
{
    if (s == NULL)
        s = "";
    write(s, strlen(s) + 1);
}

// 0x526410  Confidence: exact (the key copy's stack slot). An encrypted slot encrypts the caller's
// buffer in place for the fwrite and then restores the key and decrypts it again.
void File::write(const void *buf, int len)
{
    file_slot *s = &file_slots[slot];
    unsigned int key[3];

    s->num_ops++;
    if (s->error || len == 0)
        return;
    if (s->flags & 1) {
        s->cipher.get_key(key);
        s->cipher.crypt((void *)buf, len);
    }
    int n = fwrite(buf, len, 1, s->fp);
    if (s->flags & 1) {
        s->cipher.set_key(key);
        s->cipher.crypt((void *)buf, len);
    }
    if (n != 0) {
        s->pos += len;
        s->num_bytes += len;
        return;
    }
    s->error = FILE_ERR_WRITE;
}

// 0x5264e0  Confidence: exact. Straight to the FILE (no count, no cipher): text mode files.
void File::write_line(const char *text, int len)
{
    file_slot *s = &file_slots[slot];

    if (s->error)
        return;
    if (text == NULL)
        text = "";
    if (len < 0)
        len = strlen(text);
    if (len == 0 || fwrite(text, len, 1, s->fp)) {
        if (fwrite("\n", strlen("\n"), 1, s->fp))
            return;
    }
    s->error = FILE_ERR_WRITE;
}

// 0x526580  Confidence: exact. No caller.
void File::sync_bool(bool *v, int min_ver, bool def)
{
    if (check_flags(FILE_READ))
        *v = read_bool(min_ver, def);
    else
        write_bool(*v);
}

// 0x5265c0  Confidence: exact. No caller.
void File::sync_int8(char *v, int min_ver, char def)
{
    if (check_flags(FILE_READ))
        *v = read_int8(min_ver, def);
    else
        write_int8(*v);
}

// 0x526600  Confidence: exact.
void File::sync_uint8(ubyte *v, int min_ver, ubyte def)
{
    if (check_flags(FILE_READ))
        *v = read_uint8(min_ver, def);
    else
        write_uint8(*v);
}

// 0x526640  Confidence: exact.
void File::sync_int16(short *v, int min_ver, short def)
{
    if (check_flags(FILE_READ))
        *v = read_int16(min_ver, def);
    else
        write_int16(*v);
}

// 0x526680  Confidence: exact.
void File::sync_uint16(ushort *v, int min_ver, ushort def)
{
    if (check_flags(FILE_READ))
        *v = read_uint16(min_ver, def);
    else
        write_uint16(*v);
}

// 0x5266c0  Confidence: exact.
void File::sync_int(int *v, int min_ver, int def)
{
    if (check_flags(FILE_READ))
        *v = read_int(min_ver, def);
    else
        write_int(*v);
}

// 0x526700  Confidence: exact.
void File::sync_uint(unsigned int *v, int min_ver, unsigned int def)
{
    if (check_flags(FILE_READ))
        *v = read_uint(min_ver, def);
    else
        write_uint(*v);
}

// 0x526740  Confidence: exact.
void File::sync_float(float *v, int min_ver, float def)
{
    if (check_flags(FILE_READ))
        *v = read_float(min_ver, def);
    else
        write_float(*v);
}

// 0x526780  Confidence: exact.
void File::sync_vector(vector3 *v, int min_ver, const vector3 *def)
{
    if (check_flags(FILE_READ))
        read_vector(v, min_ver, def);
    else
        write_vector(v);
}

// 0x5267c0  Confidence: exact.
void File::sync_quaternion(quaternion *q, int min_ver, const quaternion *def)
{
    if (check_flags(FILE_READ))
        read_quaternion(q, min_ver, def);
    else
        write_quaternion(q);
}

// 0x526800  Confidence: exact.
void File::sync_matrix(matrix *m, int min_ver, const matrix *def)
{
    if (check_flags(FILE_READ))
        read_matrix(m, min_ver, def);
    else
        write_matrix(m);
}

// 0x526840  Confidence: exact. No caller.
void File::sync_matrix43(matrix43 *m, int min_ver, const matrix43 *def)
{
    if (check_flags(FILE_READ))
        read_matrix43(m, min_ver, def);
    else
        write_matrix43(m);
}

// 0x526880  Confidence: exact. No caller.
void File::sync_String(String *s, int min_ver, const char *def)
{
    if (check_flags(FILE_READ))
        read_String(s, min_ver, def);
    else
        write_string(s->c_str(), -1);
}

// 0x5268c0  Confidence: exact.
void File::sync_string_buf(char *buf, int size, int min_ver, const char *def)
{
    if (check_flags(FILE_READ))
        read_string_buf(buf, size, min_ver, def);
    else
        write_string(buf, -1);
}

// 0x526900  Confidence: exact.
void File::sync_bytes(void *buf, int len, int min_ver)
{
    if (check_flags(FILE_READ))
        read(buf, len, min_ver, 0);
    else
        write(buf, len);
}
