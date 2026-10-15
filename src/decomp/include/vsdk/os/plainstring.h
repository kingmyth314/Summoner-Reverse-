// vsdk/os/plainstring.h - plain C string helpers (plainstring.cpp, 0x514ec0-0x515955, between vfile and
// parse; file name from Summoner 2's plainstring.cpp, directory guessed). Not original source.
#ifndef VSDK_OS_PLAINSTRING_H
#define VSDK_OS_PLAINSTRING_H

#include "vsdk/vsdk.h"

class vector3;

// Blank = space, tab, newline (not CR).
bool is_blank(const char *s);                                                       // 0x514ec0
void trim_leading_whitespace(char *dest, const char *src);                          // 0x514f00
// src without its trailing spaces, tabs and newlines into dest.
void trim_trailing_whitespace(char *dest, const char *src);                         // 0x514f60
// First / last non-blank of src into dest; max_length is not used. Return value never used.
void trim_whitespace(char *dest, const char *src, int max_length);                  // 0x514fb0
// (dest, indent, width, label, fmt, ...): indent spaces, label, padding to width, then the text.
void format_indented_column(char *dest, int indent, int width, const char *label, const char *fmt, ...);   // 0x515030
void format_indented_column_b(char *dest, int indent, int width, const char *label, const char *fmt, ...); // 0x515170
// NULL when dest (size) is too small, else dest.
char *int_to_string_with_commas(char *dest, int size, int value);                   // 0x5152a0
// font goes to gr_get_string_size's length slot (every caller passes -1; decomp22 D204).
char *gr_string_truncate_ellipsis(char *dest, const char *s, int max_w, int font);  // 0x5153b0
char *strcat_char(char *string, char c);                                            // 0x5154d0
// Case-insensitive strstr: pointer into str, NULL when not found (decomp20, conversation_farewell).
char *stristr(const char *str, const char *substr);                                 // 0x515500
char *strlwr(char *s);                                                              // 0x5155e0
char *strupr(char *s);                                                              // 0x515620
// The size arguments are not used (plain strcpy).
char *int_to_string(char *dest, int size, int value);                               // 0x515660
char *int_to_hex_string(char *dest, int size, int value);                           // 0x5156b0
char *float_to_string(char *dest, int size, float value, int precision);            // 0x515700
char *byte_count_to_string(char *dest, int size, int bytes);                        // 0x515760
char *vector3_to_string_buf(char *dest, int size, const vector3 *v, int precision); // 0x515800
char *ptr_to_hex_string(char *dest, int size, const void *value);                   // 0x515880
// Text <-> the 16-bit (Shift-JIS full-width) table for chars 0x20..0x75.
void string_to_wide_charmap(ushort *dest, const char *src);                         // 0x5158d0
void wide_charmap_to_string(char *dest, const ushort *src);                         // 0x515910
extern ushort wide_charmap_table[86];                                               // 0x5a783c

#endif
