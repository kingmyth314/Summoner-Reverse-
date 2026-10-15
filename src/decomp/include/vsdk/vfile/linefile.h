// vsdk/vfile/linefile.h - LineFile, a buffered stdio text file read or written by lines (0x558340-0x558a40;
// class name from sum.symbols, unit file and directory not known, file name ours; decomp44). Not original
// source. Only what console.cpp's load_script uses is declared with its layout.
#ifndef VSDK_VFILE_LINEFILE_H
#define VSDK_VFILE_LINEFILE_H

#include <stdio.h>
#include "vsdk/vsdk.h"

// 0x1c bytes (LineFile_open: operator new(0x1c)). The destructor is not virtual: `delete` calls the
// scalar deleting destructor 0x50af60, a COMDAT that the linker kept from console.obj (load_script is
// its first user in link order).
class LineFile {
public:
    LineFile();                                   // 0x558580 FILE and buffer = NULL
    ~LineFile();                                  // 0x558590 fclose + buffer free
    int  open_read(const char *name, int buffer_size);           // 0x558340 1 = failed
    void close();                                 // 0x5585c0
    // (buf, size): one line without its terminator; 0 at the end of the file.
    int  read_line(char *buf, int size);          // 0x558600

    FILE *fp;                                     // +0x00
    char *buffer;                                 // +0x04 allocate_in_commonblock
    int _unk08[4];                                // +0x08
    unsigned int flags;                           // +0x18 bit 0 end of file, bits 2-3 the line ending
};
LAYOUT_CHECK(LineFile, sizeof(LineFile) == 0x1c);

LineFile *LineFile_open(const char *name, int buffer_size);   // 0x558910 NULL when it cannot be opened

#endif
