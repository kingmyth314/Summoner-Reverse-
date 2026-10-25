// vsdk/os/vstring.h - the vsdk String class and TextBuffer (one object, 0x5566e0-0x558337; source at
// src/decomp/vsdk/unnamed_string_0x5566e0.cpp, decomp66; unit path not in the binary, file name ours:
// "string.h" would shadow the C header). Not original source.
#ifndef VSDK_OS_VSTRING_H
#define VSDK_OS_VSTRING_H

#include "vsdk/vsdk.h"

class vector3;

// 8 bytes. __thiscall members; returns by value through a hidden pointer. max_len is the length the
// buffer was made for (alloc(len) keeps len + 1 bytes); buf is NULL for an empty string.
class String {
public:
    String();                                   // 0x5567f0 ""
    String(const char *s);                      // 0x556810 String__String_cstr
    String(char c);                             // 0x556830 String__String_char
    String(const String &other);                // 0x556850 String__String_copy
    explicit String(int len);                   // 0x556880 String__String_len: an empty len buffer (0 below 0)
    ~String();                                  // 0x5568b0
    const char *c_str() const;                  // 0x5568c0 "" for NULL
    int size() const;                           // 0x5568d0 strlen(buf), 0 for NULL
    bool empty() const;                         // 0x5568f0 max_len <= 0 or buf[0] == 0
    bool is_blank() const;                      // 0x556910 empty, or only ' ', '\t', '\n'
    void to_lower();                            // 0x556950
    void to_upper();                            // 0x556990
    String substr(int begin, int end) const;    // 0x5569d0 [begin, end], end -1 = the last char
    String left(int n) const;                   // 0x556a80
    String right(int n) const;                  // 0x556b40
    String mid(int start, int len) const;       // 0x556c00
    // (s, from_end): index of s ignoring case, -1 when absent, 0 for NULL or "".
    int find_nocase(const char *s, bool from_end) const;   // 0x556cb0
    void trim_left();                           // 0x556d50
    void trim_right();                          // 0x556db0
    float to_float() const;                     // 0x556df0 atof, 0 for NULL
    void trim();                                // 0x556e10
    bool clear_thunk() const;                   // 0x556e30 String__clear_thunk: a second entry of empty()
    String &operator=(const String &other);     // 0x556e40
    String &operator=(const char *s);           // 0x556ea0 (NULL -> empty)
    String &operator=(char c);                  // 0x556f30 String__assign_char_op
    String &operator+=(const String &s);        // 0x557420 String__append
    String &operator+=(const char *s);          // 0x557490 String__append_cstr
    char &operator[](int n);                    // 0x557ab0 (String__char_at, bounds-checked)
    // decomp22 (parse.cpp): the first n chars of s (TextFile::ParseString and friends).
    void assign_n(const char *s, int n);        // 0x5566e0 String__assign_n
    void assign_char(char c);                   // 0x556760 String__assign_char
    // "%d", "0x%x", "%.*f" of a value; cdecl with the hidden result.
    static String from_int(int n);              // 0x557ae0 String__from_int
    static String from_hex(int n);              // 0x557b70 String__from_hex
    static String from_float(float f, int precision);   // 0x557c00 String__from_float
    static String format_bytes(int bytes);      // 0x557c90 " bytes", " KB", " MB"
    static String from_hex_b(int n);            // 0x558080 String__from_hex_b (a second "0x%x" copy)
    // decomp61 (File::read_String_raw, File::read_line_String).
    void alloc(int len);                        // 0x556790 String__alloc: free, then an empty len + 1 buffer
    void free();                                // 0x5567d0 String__free: operator delete(buf), buf = NULL
    String &append_char(char c);                // 0x557500 String__append_char (returns this; += char)

    int max_len;                                // +0x0
    char *buf;                                  // +0x4
};
LAYOUT_CHECK_32(String, sizeof(String) == 8);

// decomp22 (parse.cpp): cdecl (hidden result, left, right); the operands are passed by address.
String operator+(const String &a, const String &b);   // 0x556f70 String__concat
String operator+(const String &a, const char *s);     // 0x557070 String_plus_cstr
String operator+(const char *s, const String &b);     // 0x557180 cstr_plus_String
String operator+(const String &a, char c);            // 0x557290 String_plus_char
String operator+(char c, const String &b);            // 0x557360 char_plus_String
// Comparisons ignore case (_stricmp); an empty String or NULL / "" compares below any other text.
bool operator==(const String &a, const String &b);    // 0x557570 String_eq
bool operator==(const char *s, const String &b);      // 0x5575b0 cstr_eq_String
bool operator==(const String &a, const char *s);      // 0x5575f0 String_eq_cstr
bool operator!=(const String &a, const String &b);    // 0x557630 String_ne
bool operator!=(const char *s, const String &b);      // 0x557670 cstr_ne_String
bool operator!=(const String &a, const char *s);      // 0x5576b0 String_ne_cstr
bool operator<(const String &a, const String &b);     // 0x5576f0 String_lt
bool operator<(const char *s, const String &b);       // 0x557740 cstr_lt_String
bool operator<(const String &a, const char *s);       // 0x557790 String_lt_cstr
bool operator>(const String &a, const String &b);     // 0x5577e0 String_gt
bool operator>(const char *s, const String &b);       // 0x557830 cstr_gt_String
bool operator>(const String &a, const char *s);       // 0x557880 String_gt_cstr
bool operator<=(const String &a, const String &b);    // 0x5578d0 String_le
bool operator<=(const char *s, const String &b);      // 0x557920 cstr_le_String
bool operator<=(const String &a, const char *s);      // 0x557970 String_le_cstr (inverted for one empty side)
bool operator>=(const String &a, const String &b);    // 0x5579c0 String_ge
bool operator>=(const char *s, const String &b);      // 0x557a10 cstr_ge_String
bool operator>=(const String &a, const char *s);      // 0x557a60 String_ge_cstr
// "< x, y, z >" with the given precision (cdecl, hidden result).
String String_from_vector3(const vector3 *v, int precision);   // 0x557e10

// A text buffer that drops its oldest lines (0x18 bytes; class name from sum.symbols).
class TextBuffer {
public:
    // (buf, size, max_lines): buf NULL = own buffer of at least the overrun text's size; max_lines <= 0
    // keeps every line.
    TextBuffer(char *buf, int size, int max_lines);   // 0x558110
    ~TextBuffer();                              // 0x558180 frees an own buffer
    void clear();                               // 0x5581a0
    char *get_text();                           // 0x5581c0
    int get_length();                           // 0x5581d0
    // Appends; a full buffer is cleared to "Buffer overrun!".
    void append(const char *s);                 // 0x5581e0

    char *buf;                                  // +0x00
    bool external;                              // +0x04 buf belongs to the caller
    ubyte _pad05[3];
    int size;                                   // +0x08
    int length;                                 // +0x0c
    int max_lines;                              // +0x10
    int num_lines;                              // +0x14 1 after clear
};
LAYOUT_CHECK_32(TextBuffer, sizeof(TextBuffer) == 0x18);
void TextBuffer_printf(TextBuffer *tb, const char *fmt, ...);   // 0x558300 (1 KB vsprintf buffer)

#endif
