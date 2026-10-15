// engine/rendering/unnamed_lamp_glow_0x4c0a40.h - lamp glows (halo sprites on lamps): the unit
// 0x4c0a40-0x4c0c3a (source game/unnamed_lamp_glow_0x4c0a40.cpp, decomp59). File and directory names are
// placeholders: no path string, no strings.tbl block. Not original source. docs/sum-exe/effects.md
// "Lamp glows". Callers: shadows_init (load), levelrender_level_load and fx_create_instance (add / set),
// lamp_glows_draw 0x484380 (draw, fade).
#ifndef ENGINE_RENDERING_UNNAMED_LAMP_GLOW_0X4C0A40_H
#define ENGINE_RENDERING_UNNAMED_LAMP_GLOW_0X4C0A40_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "engine/rendering/levelrender.h"   // lamp_glow_bitmap

// One glow (0x18 bytes; sum.symbols lamp_glow[300]). Field names ours.
#define MAX_LAMP_GLOWS 300
struct lamp_glow {
    vector3 pos;                      // +0x00
    float size;                       // +0x0c the radius given * 5
    float alpha;                      // +0x10 0..1, faded by lamp_glows_draw
    int _unk14;                       // +0x14 zeroed by lamp_glow_add, not read here
};
LAYOUT_CHECK(lamp_glow, sizeof(lamp_glow) == 0x18);

// lamp_glow_bitmap (0x59db70, -1 until lamp_glow_load) is declared in engine/rendering/levelrender.h (decomp57,
// for level_preload_effects); this header includes it.
extern lamp_glow lamp_glows[MAX_LAMP_GLOWS];          // 0x24893e8
extern int lamp_glow_count;                           // 0x248b008

void lamp_glow_load();                                // 0x4c0a60
int  lamp_glow_add(const vector3 *pos, float radius); // 0x4c0a90 the new index (no bound check)
void lamp_glow_forget();                              // 0x4c0af0 count = 0
void lamp_glow_set(int index, const vector3 *pos, float radius);   // 0x4c0b00
void lamp_glow_draw(int index);                       // 0x4c0b40
void lamp_glow_fade_out(int index, float frametime);  // 0x4c0ba0 alpha -= 3 per second
void lamp_glow_fade_in(int index, float frametime);   // 0x4c0bf0 alpha += 6 per second

#endif
