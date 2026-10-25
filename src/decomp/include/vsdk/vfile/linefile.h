// vsdk/vfile/linefile.h - LineFile, a buffered stdio text file read or written by lines (0x558340-0x558a23;
// class name from sum.symbols, unit file and directory not known, file name ours; decomp44, completed by
// decomp66: source at src/decomp/vsdk/unnamed_linefile_0x558340.cpp). Not original source.
#ifndef VSDK_VFILE_LINEFILE_H
#define VSDK_VFILE_LINEFILE_H

#include <stdio.h>
#include "vsdk/vsdk.h"

// flags: bit 0 the end of the file was read, bit 1 open for reading, bits 2-3 the line ending found by
// detect_eol (LINEFILE_EOL_*, the read_line terminator search).
#define LINEFILE_EOF        0x1
#define LINEFILE_READ       0x2
#define LINEFILE_EOL_MASK   0xc
#define LINEFILE_EOL_CRLF   0x0
#define LINEFILE_EOL_LF     0x4
#define LINEFILE_EOL_CR     0x8
#define LINEFILE_EOL_ANY    0xc       // mixed, or none in the first buffer: the first of '\n' / '\r'

// 0x1c bytes (LineFile_open: operator new(0x1c)). The destructor is not virtual: `delete` calls the
// scalar deleting destructor 0x50af60, a COMDAT that the linker kept from console.obj (load_script is
// its first user in link order).
class LineFile {
public:
    LineFile();                                   // 0x558580 FILE and buffer = NULL
    ~LineFile();                                  // 0x558590 fclose + buffer free
    int  open_read(const char *name, int buffer_size);           // 0x558340 1 = failed
    // (name, append, stdio buffer size): "ab" / "wb", fully buffered; 1 = failed.
    int  open_write(const char *name, bool append, int buffer_size);   // 0x5583d0
    int  detect_eol();                            // 0x558430 0
    int  fill_buffer();                           // 0x5584f0 0
    void close();                                 // 0x5585c0
    // (buf, size): one line without its terminator; NULL at the end of the file, else buf (decomp66,
    // D4600: was int).
    char *read_line(char *buf, int size);         // 0x558600
    // Writes s with each '\n', '\r' or "\r\n" turned into LineFile_eol; 1 = a write failed.
    int  write(const char *s);                    // 0x5587c0
    int  putc(char c);                            // 0x5589b0 1 = failed
    void flush();                                 // 0x558a10 files open for writing only

    FILE *fp;                                     // +0x00
    char *buffer;                                 // +0x04 allocate_in_commonblock
    char *end;                                    // +0x08 end of the buffered data
    char *cur;                                    // +0x0c read position
    int count;                                    // +0x10 bytes left from cur
    int buffer_size;                              // +0x14
    unsigned int flags;                           // +0x18 LINEFILE_*
};
LAYOUT_CHECK_32(LineFile, sizeof(LineFile) == 0x1c);

extern const char *LineFile_eol;                        // 0x5adbe8 "\r\n" (name ours)
LineFile *LineFile_open(const char *name, int buffer_size);   // 0x558910 NULL when it cannot be opened
LineFile *LineFile_create(const char *name, bool append, int buffer_size);   // 0x558960
int LineFile_printf(LineFile *f, const char *fmt, ...);       // 0x5588d0 (1 KB buffer) write's result

#endif
