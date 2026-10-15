// engine/rendering/levelrender.h - level rendering and level geometry (engine\rendering\levelrender.cpp,
// path references 0x48e4d9-0x4914c6; levelrender_render_scene 0x482200, options_apply_detail 0x4872d0 and
// the level object arrays are placed here by address and use). Not original source.
#ifndef ENGINE_RENDERING_LEVELRENDER_H
#define ENGINE_RENDERING_LEVELRENDER_H

#include "vsdk/vsdk.h"

// Level objects (0x70 bytes each); +0xC points at 8-byte face records whose first word is a 1-based
// index into level_texture_bms.
struct lobj_face {
    ushort texture;
    ushort _pad2;
    int _unk4;
};
struct lobj {
    char _pad0[0xc];
    lobj_face *faces;                 // +0x0c
    char _pad10[0x60];
};
LAYOUT_CHECK(lobj, sizeof(lobj) == 0x70);
extern lobj *lobjs;                   // 0xa5a2cc
extern int *level_texture_bms;        // 0xa5a320

void levelrender_render_scene();      // 0x482200
void options_apply_detail();          // 0x4872d0 (detail settings; name from sum.symbols)
void levelrender_clear_draw_lists();  // 0x486b00

#endif
