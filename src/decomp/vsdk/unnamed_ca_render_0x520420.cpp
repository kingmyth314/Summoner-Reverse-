// unnamed_ca_render_0x520420.cpp - Sum.exe v1.40, code 0x520420-0x5204b8: character_instance::render
// (vsdk\ca in sum.symbols). The file name is a placeholder (no path string, no data).
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
//
// Unit range (D4007): one non-inline function after the attached-light object's COMDAT block
// (0x520280-0x520413) and before the sound unit's first static initialiser (0x5204c0). VC6 emits an
// object's inline / implicit functions after its regular ones (effect_mem.cpp, the light and the
// attached-light units all end that way), and nothing in the attached-light unit calls this member, so
// it is not one of that unit's COMDATs: a separate object, likely; which file it came from is not known
// (character_instance.cpp is 0x5194b0-0x51b6a1).

#include "vsdk/ca/character_instance.h"
#include "vsdk/ca/character.h"
#include "vsdk/gr/gr.h"

// 0x520420  Confidence: exact (thiscall, ret 0xc). Skips everything when light->_unk08 is 0; sets the
// matrices, copies the tint into the light record and makes this instance current; then (unless bit 0
// of light->_unk00, which returns with the instance still current) draws the current LOD's mesh, or
// its shadow mesh when light->_unk04 is neither 0 nor -1 and there is one.
void character_instance::render(vector3 *pos, matrix *orient, gr_light_desc *light)
{
    if (light->_unk08 == 0)
        return;
    setup_matrices_b();
    light->tint[0] = tint_r;
    light->tint[1] = tint_g;
    light->tint[2] = tint_b;
    ca_render_set_instance(character, this);
    if (light->_unk00 & 1)
        return;
    if (light->_unk04 != 0 && light->_unk04 != -1 && character->mesh->shadow != NULL)
        ca_render_set_mesh(character->mesh->shadow);
    else
        ca_render_set_mesh(character->mesh->vim);
    ca_render_mesh(pos, orient, light);
    ca_render_set_instance(NULL, NULL);
}
