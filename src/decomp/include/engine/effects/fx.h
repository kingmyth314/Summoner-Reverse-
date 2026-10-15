// engine/effects/fx.h - spell / visual effect objects (fx_* in sum.symbols; the unit's file name is not known:
// module "-"). File name and directory ours. Not original source.
// decomp64: the unit is 0x489c40-0x48d7bf (source game/unnamed_fx_0x489c40.cpp, file name a placeholder):
// the effect object pool and list, the character links, the blob shadow bitmaps, fx_frame and the fx_*
// creators. Its .data is 0x59c25c-0x59c357, its .bss 0x2305618-0x2318e43.
// Layout: symbols/types.h effect_object (0x10c bytes, fx_pool 0x2305bc0). Declared for spells.cpp and
// combat.cpp (decomp23); argument lists from their call sites, corrected against the bodies in decomp64.
#ifndef ENGINE_EFFECTS_FX_H
#define ENGINE_EFFECTS_FX_H

#include "vsdk/vsdk.h"
#include "vsdk/math/matrix.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/os/timestamp.h"
#include "engine/objects/objects.h"

class Character;
class character_instance;
struct ItemType;
struct spell_cast_params;

// effect_object::kind (+0xa4): the fx_frame case of the object (decomp64; names of the creators).
//  0 fx_add_effect / fx_create_world, 1 / 2 fx_attach_to_character (2: follows the body centre),
//  3 fx_create_at_camera, 4 fx_create_missile, 5 / 6 status_effect_update_fx (fading in / out), 7 ground
//  effect, 8 summon_spawn_effect, 9 status holder (fx_get_status_holder), 10 stone (status_stone_fx),
//  11 spell area, 12 / 13 / 14 fx_create_spell_effect_near / _facing / _at, 15 Titus' rock eruption,
//  16 the effect of an equipped item (character_equip_item_fx).

// An effect object: obj (+0x00..+0x97) then the effect fields (types.h effect_object). The constructor,
// destructor and virtual destroy are in the fx unit (decomp64); vtable 0x581aac has the one obj slot.
class effect_object : public obj {
public:
    effect_object();                  // 0x489d30 fx_obj__ctor
    ~effect_object();                 // 0x489db0 fx_obj__dtor (stores the vtable, then obj::~obj)
    virtual void deinit();            // 0x489dc0 fx_obj_destroy (vtable 0x581aac slot 0)

    effect_object *fx_next;           // +0x98 fx_active_list
    effect_object *fx_prev;           // +0x9C
    vfx_instance *instance;           // +0xA0
    int kind;                         // +0xA4 see above
    int spell;                        // +0xA8 spell index, -1 none (fx_new leaves the old value)
    int source;                       // +0xAC
    int caster;                       // +0xB0 casting character's handle
    int owner;                        // +0xB4
    int trail_bitmap;                 // +0xB8
    union {                           // +0xBC
        unsigned int bits;
        struct {                      // a signed 17-bit field (shl 15 / sar 15 at 0x45dbfd; decomp37)
            int fx_flags : 17;        //   1 no instance, 2 world, 4 sky, 8 weapon missile, 0x10 hit done,
                                      //   0x80 missile arrived, 0x100 set by character_perform_spell_cast,
                                      //   0x200 status effect (decomp64)
            unsigned int hit_count : 3;   // bits 17-19: entries of hit_handles (fx_create_spell_area; decomp64)
            int _bits20 : 4;          // bits 20-23: cleared by the constructor and the creators
            int _bits24 : 8;          // bits 24-31: set to -1 by the constructor and the creators
        };
    };
    int target;                       // +0xC0 the ItemType of the casting item (fx_set_target; fx_frame reads
                                      //       its charge effect, decomp64), 0 none
    int hit_handles[6];               // +0xC4
    GameTimer timer;                  // +0xDC
    int sound;                        // +0xE0
    float radius;                     // +0xE4
    vector3 start;                    // +0xE8
    vector3 aim;                      // +0xF4
    int avoid_area;                   // +0x100 avoid_area_add result
    GameTimer timer2;                 // +0x104
    void *spell_effect;               // +0x108 spell_effect_add record (struct spell_effect, spells.h)
};
LAYOUT_CHECK(effect_object, sizeof(effect_object) == 0x10c);

// A link between an effect and a character (0x14 bytes; 20 at fx_char_link_nodes). The two list heads
// are whole records (0x2308e00 and 0x2308e18; the second ends at 0x2308e2c where the blob shadow pixels
// start). Type and field names ours (decomp64).
struct fx_char_link {
    fx_char_link *next;               // +0x00
    fx_char_link *prev;               // +0x04
    effect_object *fx;                // +0x08
    int _unk0c;                       // +0x0C 0 when linked, never read
    Character *character;             // +0x10 gets Character::flags 0x200000 while linked
};
LAYOUT_CHECK(fx_char_link, sizeof(fx_char_link) == 0x14);

#define MAX_FX_OBJECTS    48
#define MAX_FX_CHAR_LINKS 20
#define MAX_FX_SKY        5
#define SHADOW_BLOB_SIZE  64
#define NUM_SHADOW_BLOBS  16

// ---- the unit's globals (decomp64) ------------------------------------------------------------------
extern int shadow_bm;                                  // 0x59c25c bm_load("Shadow01.tga"), init -1
extern int shadow_blob_bms[NUM_SHADOW_BLOBS];          // 0x59c260 bm_create handles, init -1 (name ours)
extern int fx_num_sky;                                 // 0x2305618
extern ubyte shadow_blob_palette[256 * 3];             // 0x230561c grey ramp (name ours)
extern effect_object fx_active_list;                   // 0x2305920 sentinel (fx_next = 0x23059b8)
extern fx_char_link fx_char_link_nodes[MAX_FX_CHAR_LINKS];   // 0x2305a30
extern effect_object fx_pool[MAX_FX_OBJECTS];          // 0x2305bc0
extern fx_char_link fx_char_link_free;                 // 0x2308e00 list head
extern fx_char_link fx_char_link_used;                 // 0x2308e18 list head (levelrender_draw_list reads it)
extern ubyte shadow_blob_pixels[NUM_SHADOW_BLOBS][SHADOW_BLOB_SIZE * SHADOW_BLOB_SIZE];   // 0x2308e2c (name ours)
extern effect_object *fx_sky_list[MAX_FX_SKY];         // 0x2318e2c
extern int fx_count;                                   // 0x2318e40

// ---- the unit's functions in address order (decomp64). shadows_init, fx_shutdown and
// fx_disable_by_effect_name are declared in engine/rendering/levelrender.h, status_effect_update_fx in
// engine/objects/living_entity.h. ------------------------------------------------------------------------
effect_object *fx_alloc();                             // 0x489ce0 NULL when the 48 are in use
obj *obj_from_handle_b(int handle);                    // 0x489f00 (thin obj_from_handle wrapper)
int  shadow_create_blob_bitmap();                      // 0x489f10 returns 0
void fx_lists_init();                                  // 0x48a090
void fx_destroy_all();                                 // 0x48a0f0
// The effect object frame and level hooks (0x48a190-0x48a230; decomp42).
void fx_level_enter();                                 // 0x48a190
void fx_sky_list_clear();                              // 0x48a1a0
void fx_level_leave();                                 // 0x48a1d0
void fx_sky_list_clear_thunk();                        // 0x48a1f0 (jmp fx_sky_list_clear)
// decomp41 (level_scripts_common.cpp): level_script_leave's reset while the level restarts.
void fx_level_restart();                               // 0x48a200
void fx_frame(float frametime, bool paused);           // 0x48a230 (bool: mov al, [esp + 8])
// (instance, character instance, mesh tag or -1 = each node to the tag of its own name).
void fx_bind_character_tags(vfx_instance *vi, character_instance *ci, int tag);   // 0x48b690
bool fx_update_instance(effect_object *fx, float frametime);   // 0x48b750 true = finished
bool fx_update_missile(effect_object *fx, float frametime);    // 0x48b7a0 true = finished
bool fx_missile_hit(effect_object *fx, float frametime);       // 0x48bac0 true = finished
// decomp56 (Pyrul unit, AI combat unit): effect objects with spell 0x1a (fireball), 0x1d (wall of fire)
// or 0x4e (Pyrul fireball).
int  fx_count_spells_1a_1d_4e();                       // 0x48bd40
// decomp64: the last argument is the flag of fx_create_instance (a bool passed through as a dword; was int).
int  fx_add_effect(const char *effect, const vector3 *pos, const matrix *orient, bool flag);   // 0x48bd80 handle or -1
// (effect file, kind, position or NULL = origin, orientation or NULL = identity, flag): flag false
// passes 1 to vfx_instance::reset_flags. NULL when the pool is full or the file does not load.
effect_object *fx_create_instance(const char *effect, int kind, const vector3 *pos, const matrix *orient,
                                  bool flag);          // 0x48bdc0
// (effect, character handle, tag, source, a, b): kind b ? 2 : 1, flag !a. Handle or -1. decomp64: a and
// b are bytes (bool; were int).
int  fx_attach_to_character(const char *effect, int handle, int tag, int source, bool a, bool b);   // 0x48c080
// decomp52 (the cutscene file): an effect at the camera (kind 3, camera_view_pos / camera_view_orient;
// handle or -1), and the pause / resume of the effects that are not cutscene effects (bit 0x40).
int  fx_create_at_camera(const char *effect, bool flag); // 0x48c140
// (spell, start, orient (not read), caster handle, target handle).
int  fx_create_missile(int spell, const vector3 *start, const matrix *orient, int caster, int target);  // 0x48c220
effect_object *fx_new(int kind);                       // 0x48c3e0 (no instance; NULL when full)
int  fx_create_ground_effect(int spell, int caster, const vector3 *pos, const matrix *orient);   // 0x48c540 handle or -1
// decomp37 (living_entity.cpp): the status / ownership calls, argument lists from its call sites.
int  fx_create_status_effect(int handle);              // 0x48c5c0 effect handle
// (spell, caster handle, position, cast parameters or NULL): the targets of the parameters in a network
// game, else the caster's enemies within 10.0. Handle or -1.
int  fx_create_spell_area(int spell, int caster, const vector3 *pos, spell_cast_params *params);   // 0x48c630
// (spell, caster handle, owner handle, ms): the hit effect at the owner, turned toward a free side.
int  fx_create_spell_effect_near(int spell, int caster, int owner, int ms);   // 0x48c800 handle or -1
// (spell, caster handle, target handle): the spell's hit effect at the target, facing the caster
// (the push order of 0x460806; sum.symbols lists the caster first).
int  fx_create_spell_effect_facing(int spell, int caster, int target);   // 0x48c8a0
// decomp59 (the Titus unit): "Titus-RockErrupt.VFX" at the character (handle): effect handle or -1.
int  titus_rock_erupt_fx(int handle);                  // 0x48c9a0
int  character_equip_item_fx(int handle, ItemType *type);   // 0x48ca40 handle or -1
// (spell, caster +0xb0, owner +0xb4 whose position / orientation are the defaults, timer ms +0xdc, pos or
// NULL, orient or NULL): handle or -1. The timer is the 4th argument (sum.symbols lists it last; decomp29).
int  fx_create_spell_effect_at(int spell, int caster, int owner, int ms, const vector3 *pos,
                               const matrix *orient);                       // 0x48cab0
// (effect, pos, orient, summon, ms, avoidance radius or 0, avoidance layer): handle or -1.
int  summon_spawn_effect(const char *effect, const vector3 *pos, const matrix *orient, Character *c, int ms,
                         float radius, int layer);     // 0x48cb40
int  fx_get_status_holder(int handle);                 // 0x48cc60
int  status_stone_fx(int handle);                      // 0x48cd20 "stone-loop.vfx" (decomp64: returns the handle; was void)
void fx_stop_status_effects(int handle);               // 0x48cdf0
void fx_stop_status_aura(int handle);                  // 0x48ce50
// (effect file, flag): fx_create_instance(name, 0, vector3_zero, identity_matrix, flag); names containing
// "sky" go to the sky list. Handle or -1 (level_create_world_effects; decomp27).
int  fx_create_world(const char *effect, bool flag);   // 0x48cea0
void fx_set_target(int fx, int target);                // 0x48cf50
void fx_stop(int fx);                                  // 0x48cf80
// decomp38 (characterinfo.cpp):
void fx_destroy(int handle);                           // 0x48cfc0
bool fx_is_finished(int fx, float *progress);          // 0x48d000 (progress may be NULL)
// decomp64: true when an effect was found (was void; the callers drop it).
bool fx_remove_missiles_from(int handle);              // 0x48d0a0
// decomp64: destroy is tested as a dword (test edi, edi; was bool).
void fx_end_owned(int handle, int destroy);            // 0x48d100
attached_light *level_light_get(int i);                // 0x48d180 attached_light_slots[i], NULL when free
void fx_summon_hook_stub(int fx, int arg);             // 0x48d1b0
void fx_link_character_obj(effect_object *fx, Character *c, bool hook);   // 0x48d1d0
void fx_link_character(int fx, Character *c, bool hook);  // 0x48d280
// (out orientation, character, distance): the character's orientation turned toward the first free of
// forward / right / back / left (random among the free ones).
void fx_find_spawn_point(matrix *out, Character *c, float distance);   // 0x48d2b0
void fx_pause_for_cutscene();                          // 0x48d6d0
void fx_destroy_spell_effects();                       // 0x48d720 (cutscene_play)
void fx_resume_after_cutscene();                       // 0x48d770
// The hooks the unit calls through the empty function 0x4f13e0 (identical-code folding; names ours).
void fx_link_hook(effect_object *fx, Character *c);    // folded into empty_function 0x4f13e0
void fx_summon_hook(effect_object *fx, int arg);       // folded into empty_function 0x4f13e0
#endif
