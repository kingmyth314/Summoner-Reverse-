// unnamed_vfile_array_0x544e50.cpp - Sum.exe v1.40, code 0x544e50-0x5450bb: the File array helpers (vector3,
// float pair, int and int16 arrays read, written or synced by the open mode; vsdk\vfile in sum.symbols).
// The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp69 NOTES.md): stringpool.cpp (vsdk\os) ends with StringPool::SetLength 0x544e30; this
// object has no data and no initialiser, and the vsdk\gr polygon object follows with its two initialisers
// (0x5450c0). One sync for each type pair is all the save code needs; the int array has a reader and a
// writer but no sync (0x544f80 has no symbol and no caller).

#include "vsdk/vfile/file.h"
#include "vsdk/math/vector.h"

// 0x544e50  Confidence: exact.
int file_read_vector_array(File *f, vector3 *v, int count, int min_ver, const vector3 *def)
{
    int i, err;

    for (i = 0; i < count; i++) {
        f->read_vector(&v[i], min_ver, def);
        if ((err = f->error()) != 0)
            return err;
    }
    return 0;
}

// 0x544ea0  Confidence: exact.
int file_write_vector_array(File *f, const vector3 *v, int count)
{
    int i, err;

    for (i = 0; i < count; i++) {
        f->write_vector(&v[i]);
        if ((err = f->error()) != 0)
            return err;
    }
    return 0;
}

// 0x544ee0  Confidence: exact (void or the callee's int; the callers ignore it).
void file_sync_vector_array(File *f, vector3 *v, int count, int min_ver, const vector3 *def)
{
    if (f->check_flags(1))
        file_read_vector_array(f, v, count, min_ver, def);
    else
        file_write_vector_array(f, v, count);
}

// 0x544f30  Confidence: exact. Two File::sync_float per element, the default's x and y; no error check.
void file_sync_float_pair_array(File *f, vector2 *v, int count, int min_ver, const vector2 *def)
{
    int i;

    for (i = 0; i < count; i++) {
        f->sync_float(&v[i].x, min_ver, def->x);
        f->sync_float(&v[i].y, min_ver, def->y);
    }
}

// 0x544f80  Confidence: exact. No symbol and no caller (name ours).
int file_read_int_array(File *f, int *v, int count)
{
    int i, err;

    for (i = 0; i < count; i++) {
        v[i] = f->read_int(0, 0);
        if ((err = f->error()) != 0)
            return err;
    }
    return 0;
}

// 0x544fc0  Confidence: exact.
int file_write_int_array(File *f, const int *v, int count)
{
    int i, err;

    for (i = 0; i < count; i++) {
        f->write_int(v[i]);
        if ((err = f->error()) != 0)
            return err;
    }
    return 0;
}

// 0x545000  Confidence: exact.
int file_read_int16_array(File *f, short *v, int count)
{
    int i, err;

    for (i = 0; i < count; i++) {
        v[i] = f->read_int16(0, 0);
        if ((err = f->error()) != 0)
            return err;
    }
    return 0;
}

// 0x545040  Confidence: exact.
int file_write_int16_array(File *f, const short *v, int count)
{
    int i, err;

    for (i = 0; i < count; i++) {
        f->write_int16(v[i]);
        if ((err = f->error()) != 0)
            return err;
    }
    return 0;
}

// 0x545080  Confidence: exact (as file_sync_vector_array).
void file_sync_int16_array(File *f, short *v, int count)
{
    if (f->check_flags(1))
        file_read_int16_array(f, v, count);
    else
        file_write_int16_array(f, v, count);
}
