// memory.cpp - Sum.exe v1.40, code 0x5124c0-0x5128ab (vsdk\os\memory.cpp)
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// The unit is built without string pooling: the three __FILE__ uses are three literals (0x5a76e0,
// 0x5a7744, 0x5a7774), and the two `= ""` buffer initialisers of mem_report_line read their own
// zero bytes (0x269305c, 0x2693060, in this unit's .bss range).
//
// The global allocation operators live here. operator new(size) is the 0 / 0 form of the debug
// operator new; the file / line arguments are never used (the error names memory.cpp itself).
// The two operator delete carry throw() only so that a C++11 checker accepts the redeclaration; VC6
// accepts the empty specification too (neither function has an EH frame, so the code should not change).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "vsdk/os/memory.h"
#include "vsdk/os/debug.h"

// Original __FILE__ of this unit (literals 0x5a76e0, 0x5a7744, 0x5a7774: not pooled).
#define MEMORY_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\os\\memory.cpp"

// ---- .bss, in address order ----------------------------------------------------------------
// 0x2693004-0x2693034: counters zeroed together by level_load (0x43859f..0x4385c9) and grown by
// level_page_in_textures / level_load_s3d from mem_bytes_allocated. They sit between registry.cpp's last
// object (0x2693000) and this unit's report globals, so they are placed here (decomp22 D202). The six
// without a sum.symbols name have names of ours.
int level_texture_mem;                  // 0x2693004
int level_mem_2693008;                  // 0x2693008 (only zeroed)
int level_s3d_mem;                      // 0x269300c
int level_mem_2693010;                  // 0x2693010 (only zeroed)
int level_mem_2693014;                  // 0x2693014 (only zeroed)
int level_mem_2693018;                  // 0x2693018 (only zeroed)
int level_mem_269301c;                  // 0x269301c (only zeroed)
int level_mem_2693020;                  // 0x2693020 (only zeroed)
// 0x2693024-0x2693030: no reference.
int mem_bytes_allocated;                // 0x2693034 (read only: the PC allocator never counts)
// 0x2693038: no reference.
FILE *mem_report_file;                  // 0x269303c
int mem_report_depth;                   // 0x2693040
int mem_report_verbose;                 // 0x2693044
Mempool *Pool;                          // 0x2693048
// 0x269304c: no reference.
int Common_block_index;                 // 0x2693050
void *Common_block_allocations[2];      // 0x2693054

// 0x5124c0  Confidence: exact. operator_new_dbg (Summoner 2: operator new(size, file, line)). Error line
// 223 (0xdf). The 128-byte buffer is allocated before the pool test (sub esp, 0x80 first).
void *operator new(size_t size, const char *file, int line)
{
    char error[128];

    if (Pool != NULL)
        return Pool->allocate_in_mempool(size);

    void *p = malloc(size);
    if (p == NULL) {
        sprintf(error, "Failed to allocate %d bytes\n", size);
        VSDK_ERROR(MEMORY_CPP, 223, error);
    }
    return p;
}

// 0x512530  Confidence: exact. operator_new_array_dbg; only caller allocate_in_commonblock.
void *operator new[](size_t size, const char *file, int line)
{
    return ::operator new(size, file, line);
}

// 0x512550  Confidence: exact. operator_new.
void *operator new(size_t size)
{
    return ::operator new(size, NULL, 0);
}

// 0x512570  Confidence: exact. operator_delete.
void operator delete(void *p) throw()
{
    free(p);
}

// 0x512580  Confidence: exact. operator_delete_array (no caller: the other units' delete[] reach
// operator delete, see memory.h).
void operator delete[](void *p) throw()
{
    ::operator delete(p);
}

// 0x512590  Confidence: exact. atexit_nop_512590: a bare ret, the empty atexit handler MainLoop pushes
// before LoadSettings. Its name and why it is in this unit are unknown (name from sum.symbols).
void atexit_nop_512590()
{
}

// 0x5125a0  Confidence: exact. mode "wt" (0x5a7710).
void mem_report_open(const char *filename)
{
    if (filename != NULL && mem_report_file == NULL) {
        mem_report_file = fopen(filename, "wt");
        mem_report_verbose = 1;
        mem_report_depth = 0;
    }
}

// 0x5125e0  Confidence: exact.
void mem_report_close()
{
    if (mem_report_file != NULL) {
        fclose(mem_report_file);
        mem_report_file = NULL;
    }
}

// 0x512600  Confidence: exact.
void mem_report_indent()
{
    mem_report_depth++;
}

// 0x512610  Confidence: exact (dec / store / jns: the store happens before the sign test).
void mem_report_unindent()
{
    mem_report_depth--;
    if (mem_report_depth < 0)
        mem_report_depth = 0;
}

// 0x512630  Confidence: exact.
int mem_report_is_verbose()
{
    return mem_report_verbose;
}

// 0x512640  Confidence: likely (the label is copied to line + 3 * depth, computed from the depth rather
// than from strlen; strlen(line) is taken before the vsprintf). Both 2000-byte buffers are initialised
// before the file test. The label and the padding to column 65 come first, then the formatted text.
void mem_report_line(const char *label, const char *fmt, ...)
{
    char line[2000] = "";
    char text[2000] = "";
    va_list args;
    int i;

    if (mem_report_file == NULL)
        return;

    for (i = 0; i < mem_report_depth; i++)
        strcat(line, "   ");
    strcpy(line + mem_report_depth * 3, label);
    int len = strlen(line);

    va_start(args, fmt);
    vsprintf(text, fmt, args);
    va_end(args);

    int pad = 65 - (int)strlen(text) - len;
    for (i = 0; i < pad; i++)
        strcat(line, " ");

    fprintf(mem_report_file, "%s%s\n", line, text);
}

// 0x512770  Confidence: exact. Summoner 2's Mempool::Copy minus the alignment of dest.
void Mempool::Copy(char *dest, int size, const char *name)
{
    strcpy(this->name, name);
    destination = dest;
    this->size = size;
    offset = 0;
}

// 0x5127c0  Confidence: exact. Only the size is rounded to 16 bytes; the offset is assumed aligned.
void *Mempool::allocate_in_mempool(unsigned int size)
{
    unsigned int new_offset = offset + ((size + 15) & ~15);

    if (new_offset > this->size)
        return NULL;

    char *p = destination + offset;
    offset = new_offset;
    return p;
}

// 0x5127f0  Confidence: exact.
void set_current_mempool(Mempool *pool)
{
    Pool = pool;
}

// 0x512800  Confidence: exact.
Mempool *get_current_mempool()
{
    return Pool;
}

// 0x512810  Confidence: exact.
bool is_mempool_active()
{
    return Pool ? true : false;
}

// 0x512820  Confidence: exact. Lines 598 (0x256) and 600 (0x258). The pointer stays in EAX as the
// return value. Index 2 is never checked: a third nested lock would write past the array.
void *allocate_in_commonblock(int size)
{
    if (Common_block_allocations[Common_block_index] != NULL)
        VSDK_ERROR(MEMORY_CPP, 598, "Common block is already locked.");

    void *p = new(MEMORY_CPP, 600) char[size];
    Common_block_allocations[Common_block_index++] = p;
    return p;
}

// 0x512880  Confidence: exact (the index is stored before the call and reloaded after it).
void deallocate_from_commonblock(void *p)
{
    Common_block_index--;
    ::operator delete(p);   // delete of a void* in the source; same call
    Common_block_allocations[Common_block_index] = NULL;
}
