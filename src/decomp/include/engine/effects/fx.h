// engine/effects/fx.h - spell / visual effect objects (fx_* in sum.symbols, 0x48bd80-0x48d720; the unit's
// file name is not known: module "-"). File name and directory ours. Not original source.
// Layout: symbols/types.h effect_object (0x10c bytes, fx_pool 0x2305bc0). Declared for spells.cpp and
// combat.cpp (decomp23); argument lists from their call sites.
#ifndef ENGINE_EFFECTS_FX_H
#define ENGINE_EFFECTS_FX_H

#include "vsdk/vsdk.h"
#include "vsdk/math/matrix.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/os/timestamp.h"
#include "engine/objects/objects.h"

// An effect object: obj (+0x00..+0x97) then the effect fields (types.h effect_object).
class effect_object : public obj {
public:
    effect_object *fx_next;           // +0x98 fx_active_list
    effect_object *fx_prev;           // +0x9C
    vfx_instance *instance;           // +0xA0
    int kind;                         // +0xA4 7 ground / timed (fx_create_ground_effect, spell_effect_obj_set_timer)
    int spell;                        // +0xA8 spell index, -1 none
    int source;                       // +0xAC
    int caster;                       // +0xB0 casting character's handle
    int owner;                        // +0xB4
    int trail_bitmap;                 // +0xB8
    unsigned int bits;                // +0xBC
    int target;                       // +0xC0
    int hit_handles[6];               // +0xC4
    GameTimer timer;                  // +0xDC
    int sound;                        // +0xE0
    float radius;                     // +0xE4
    vector3 start;                    // +0xE8
    vector3 aim;                      // +0xF4
    int avoid_area;                   // +0x100 avoid_area_add result
    GameTimer timer2;                 // +0x104
    void *spell_effect;               // +0x108 spell_effect_add record
};
LAYOUT_CHECK(effect_object, sizeof(effect_object) == 0x10c);

obj *obj_from_handle_b(int handle);                    // 0x489f00 (thin obj_from_handle wrapper)
int  fx_add_effect(const char *effect, const vector3 *pos, const matrix *orient, int a);   // 0x48bd80 handle or -1
int  fx_attach_to_character(const char *effect, int handle, int tag, int source, int a, int b);  // 0x48c080 handle or -1
int  fx_create_missile(int spell, const vector3 *start, const matrix *orient, int caster, int target);  // 0x48c220
int  fx_create_ground_effect(int spell, int caster, const vector3 *pos, const matrix *orient);   // 0x48c540 handle or -1
// (spell, caster +0xb0, owner +0xb4 whose position / orientation are the defaults, timer ms +0xdc, pos or
// NULL, orient or NULL): handle or -1. The timer is the 4th argument (sum.symbols lists it last; decomp29).
int  fx_create_spell_effect_at(int spell, int caster, int owner, int ms, const vector3 *pos,
                               const matrix *orient);                       // 0x48cab0
// (effect file, flag): fx_create_instance(name, 0, vector3_zero, identity_matrix, flag); names containing
// "sky" go to the sky list. Handle or -1 (level_create_world_effects; decomp27).
int  fx_create_world(const char *effect, bool flag);   // 0x48cea0
// decomp38 (characterinfo.cpp):
extern effect_object fx_active_list;                  // 0x2305920 sentinel (fx_next 0x23059b8)
void fx_destroy(int handle);                          // 0x48cfc0
void fx_end_owned(int owner, bool destroy);           // 0x48d100 every effect owned (+0xb4) by the handle

#endif
