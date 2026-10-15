// vsdk/geom/vfx.h - effect bases as the effect code sees them (vsdk\geom in sum.symbols, 0x536bc0-0x536e50
// and ptr_relocate 0x564d50; file names not known). Not original source.
#ifndef VSDK_GEOM_VFX_H
#define VSDK_GEOM_VFX_H

#include "vsdk/vsdk.h"

// The loaded effect base ("vfx base") an effect id resolves to. Only +0x1c is written by effect_mem.cpp.
class vfx_base {
public:
    void load_bitmaps(int flag);              // 0x536e50 vfx_base__load_bitmaps
    void relocate_node(char *a, char *b);     // 0x536bc0 vfx_node_relocate (argument meaning unsure)
    char _unk00[0x1c];
    int category;                             // +0x1c copied from the base record's category
};

void ptr_relocate(void *p, char *base);       // 0x564d50 (*p += base unless NULL)

#endif
