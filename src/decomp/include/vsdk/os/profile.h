// vsdk/os/profile.h - profiler scope objects (ours: the class and file names are not in the binary).
// Not original source.
#ifndef VSDK_OS_PROFILE_H
#define VSDK_OS_PROFILE_H

#include "vsdk/vsdk.h"

// A one-byte object whose constructor returns this (0x51b6b0, a COMDAT in character_instance.cpp) and
// whose destructor is empty (folded into the 91-caller bare ret 0x434650). Both are out of line in the
// binary, so they are declared, not defined, here.
class profile_label {
public:
    profile_label(const char *label); // 0x51b6b0
    ~profile_label();                 // 0x434650 empty_hook_434650
};

#endif
