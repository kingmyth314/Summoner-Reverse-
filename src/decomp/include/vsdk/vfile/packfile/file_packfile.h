// vsdk/vfile/packfile/file_packfile.h - packfile (.vpp) access (vsdk\vfile\packfile\file_packfile.cpp in
// sum.symbols). Not original source; only the two functions parse.cpp calls (decomp22).
#ifndef VSDK_VFILE_PACKFILE_FILE_PACKFILE_H
#define VSDK_VFILE_PACKFILE_FILE_PACKFILE_H

#include "vsdk/vsdk.h"

// Reads the whole packfile `name` into buf (size bytes); 0 = failure (it printf's the error).
int packfile_read_whole_file(char *buf, int size, const char *name);                  // 0x534900
// Entry `name` of packfile `vpp`: its size and its offset in the packfile; 0 = not found. Argument order
// from TextFile::Open, which stores the third into TextFile::size and adds the fourth to the buffer
// (the sum.symbols comment has &offset, &size; decomp22 D206).
int packfile_find_in_packfile(const char *vpp, const char *name, int *size, int *offset);   // 0x5349a0

void packfile_add(const char *name);  // 0x534420 (level_load: levelm / levele / levelt.vpp; return unused, decomp42)

#endif
