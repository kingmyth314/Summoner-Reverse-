// vsdk/os/plainstring.h - plain C string helpers (0x514fb0-0x5153b0, between vfile and parse; file name
// from Summoner 2's plainstring.cpp, directory guessed). Not original source.
#ifndef VSDK_OS_PLAINSTRING_H
#define VSDK_OS_PLAINSTRING_H

#include "vsdk/vsdk.h"

// First / last non-blank of src into dest, at most max_length. Return value never used.
bool is_blank(const char *s);                      // 0x514ec0 (plainstring.cpp)
void trim_whitespace(char *dest, const char *src, int max_length);                 // 0x514fb0
void int_to_string_with_commas(char *buf, int size, int value);                    // 0x5152a0 (return unused)
// src without its trailing spaces, tabs and newlines into dest.
void trim_trailing_whitespace(char *dest, const char *src);                         // 0x514f60
void gr_string_truncate_ellipsis(char *dest, const char *s, int max_w, int font);  // 0x5153b0 (return unused)

#endif
