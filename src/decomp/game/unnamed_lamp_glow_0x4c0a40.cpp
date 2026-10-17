// unnamed_lamp_glow_0x4c0a40.cpp - Sum.exe v1.40, code 0x4c0a40-0x4c0c3a: lamp glows, the halo sprites the
// level objects and effect meshes with flag 0x40 put on lamps (docs/sum-exe/effects.md "Lamp glows").
// File name NOT known: a placeholder (src/decomp/README.md). No assert path, no strings.tbl block; cut by
// its static initialiser (entry 317, 0x5884f8, an empty one) and its data.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: static_init_empty_4c0a50 (0x4c0a40) to lamp_glow_fade_in (0x4c0bf0-0x4c0c3a). The Machival
// unit opens with its initialiser 0x4c0c40 (entry 318).
// Data: .data 0x59db70-0x59db88 (lamp_glow_bitmap = -1, "WolongLampGlow01.tga"); .bss 0x24893e8-0x248b00b
// (lamp_glows, lamp_glow_count). Floats are pooled .rdata constants.

#include "engine/rendering/unnamed_lamp_glow_0x4c0a40.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/gr/gr.h"

// ---- .data --------------------------------------------------------------------------------------------
int lamp_glow_bitmap = -1;            // 0x59db70

// ---- .bss ---------------------------------------------------------------------------------------------
// 0x4c0a40  Confidence: exact. static_init_empty_4c0a50: "jmp 0x4c0a50" (initialiser entry 317).
// 0x4c0a50  Confidence: likely. init_empty_4c0a50: a bare ret, the initialiser of lamp_glows: the array
// needs construction (vector3's empty user constructor), which optimises away.
lamp_glow lamp_glows[MAX_LAMP_GLOWS]; // 0x24893e8
int lamp_glow_count;                  // 0x248b008

// 0x4c0a60  Confidence: exact.
void lamp_glow_load()
{
    lamp_glow_bitmap = bm_load("WolongLampGlow01.tga", -1, -1);
    lamp_glow_count = 0;
}

// 0x4c0a90  Confidence: exact. No bound check (effects.md; NOTES, game bugs).
int lamp_glow_add(const vector3 *pos, float radius)
{
    lamp_glows[lamp_glow_count].pos = *pos;
    lamp_glows[lamp_glow_count].size = radius * 5.0f;
    lamp_glows[lamp_glow_count].alpha = 0.0f;
    lamp_glows[lamp_glow_count]._unk14 = 0;
    lamp_glow_count++;
    return lamp_glow_count - 1;
}

// 0x4c0af0  Confidence: exact.
void lamp_glow_forget()
{
    lamp_glow_count = 0;
}

// 0x4c0b00  Confidence: exact.
void lamp_glow_set(int index, const vector3 *pos, float radius)
{
    lamp_glows[index].pos = *pos;
    lamp_glows[index].size = radius * 5.0f;
}

// 0x4c0b40  Confidence: exact. The sprite shrinks with the alpha.
void lamp_glow_draw(int index)
{
    lamp_glow *g = &lamp_glows[index];

    if (g->alpha != 0.0f) {
        gr_set_texture(lamp_glow_bitmap, -1);
        gr_bitmap_3d(&g->pos, 0, g->size * g->alpha, gr_mode_lamp_glow);
    }
}

// 0x4c0ba0  Confidence: exact (VC6 byte match, match14; was exact).
void lamp_glow_fade_out(int index, float frametime)
{
    lamp_glow *g = &lamp_glows[index];

    if (g->alpha > 0.0f) {
        g->alpha -= frametime * 3.0f;
        g->alpha = g->alpha <= 0.0f ? 0.0f : g->alpha;
    }
}

// 0x4c0bf0  Confidence: exact (VC6 byte match, match14; was exact).
void lamp_glow_fade_in(int index, float frametime)
{
    lamp_glow *g = &lamp_glows[index];

    if (g->alpha < 1.0f) {
        g->alpha += frametime * 6.0f;
        g->alpha = g->alpha >= 1.0f ? 1.0f : g->alpha;
    }
}
