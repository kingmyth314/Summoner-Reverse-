// vsdk/os/profile.h - profiler scope objects (ours: the class and file names are not in the binary).
// Not original source.
#ifndef VSDK_OS_PROFILE_H
#define VSDK_OS_PROFILE_H

#include "vsdk/vsdk.h"

// A one-byte object whose constructor returns this (0x51b6b0, a COMDAT in character_instance.cpp) and
// whose destructor is empty (folded into the 91-caller bare ret 0x434650). Both are inline (D6102): only
// the /Ob0 units (character_instance.cpp, the ix unit) call them; every /Ob1 unit, the unoptimised
// living_entity.cpp and objects.cpp included, expands them to nothing and keeps only the EH states, and
// the unwind funclets call the destructor's COMDAT.
class profile_label {
public:
    profile_label(const char *label) {} // 0x51b6b0
    ~profile_label() {}                 // 0x434650 empty_hook_434650
};

#endif
