// vsdk/os/vstring.h - the vsdk String class (0x5567f0-0x557ab0; unit path not in the binary, file name
// ours: "string.h" would shadow the C header). Not original source.
#ifndef VSDK_OS_VSTRING_H
#define VSDK_OS_VSTRING_H

#include "vsdk/vsdk.h"

// 8 bytes. __thiscall members; returns by value through a hidden pointer.
class String {
public:
    String();                                   // 0x5567f0
    String(const char *s);                      // 0x556810
    String(const String &other);                // 0x556850
    ~String();                                  // 0x5568b0
    const char *c_str() const;                  // 0x5568c0
    int size() const;                           // 0x5568d0
    String substr(int begin, int end) const;    // 0x5569d0
    String left(int n) const;                   // 0x556a80
    String &operator=(const String &other);     // 0x556e40
    String &operator=(const char *s);           // 0x556ea0 (NULL -> empty)
    char &operator[](int n);                    // 0x557ab0 (String__char_at, bounds-checked)

    int max_len;                                // +0x0
    char *buf;                                  // +0x4
};
LAYOUT_CHECK(String, sizeof(String) == 8);

#endif
