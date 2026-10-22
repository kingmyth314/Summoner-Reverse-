// unnamed_fx_0x489c40.cpp - Sum.exe v1.40, code 0x489c40-0x48d7bf: the game's effect objects (pool, list,
// character links, the per-frame update and every fx_* creator) and the blob shadow bitmaps. THE FILE NAME
// IS A PLACEHOLDER: no assert path, no strings.tbl block, module "-" in sum.symbols.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit evidence (NOTES.md): static initialiser entries 251 / 252 open the code; .data 0x59c25c-0x59c357
// (shadow_bm, the 16 blob handles, then the literals in function order up to "sky"; fx_obj_destroy's
// "Destroying AA" lies after the handles that shadow_create_blob_bitmap writes, so both are one file);
// .bss 0x2305618-0x2318e43. "Scene Root" is not in this .data: the code uses the .rdata copy 0x581e5c.

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/geom/effect_mem.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/snd/snd.h"
#include "vsdk/math/ix.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/skeleton.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/spells/spells.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/camera/camera.h"
#include "engine/effects/fx.h"
#include "engine/rendering/levelrender.h"
#include "engine/rendering/unnamed_lamp_glow_0x4c0a40.h"
#include "engine/rendering/unnamed_snow_0x4946a0.h"
#include "summoner/summoner.h"

// The effect list: vsdk.h's list_append / list_remove shape with the fx_next / fx_prev names (names ours).
#define fx_list_append(head, elem) do { (elem)->fx_prev = (head)->fx_prev; (elem)->fx_next = (head); \
                                        (head)->fx_prev->fx_next = (elem); (head)->fx_prev = (elem); } while (0)
#define fx_list_remove(elem) do { (elem)->fx_prev->fx_next = (elem)->fx_next; \
                                  (elem)->fx_next->fx_prev = (elem)->fx_prev; \
                                  (elem)->fx_next = NULL; (elem)->fx_prev = NULL; } while (0)

// ---- .data (0x59c25c-0x59c29f; the literals 0x59c2a0-0x59c357 follow) -------------------------------------
int shadow_bm = -1;                                   // 0x59c25c
int shadow_blob_bms[NUM_SHADOW_BLOBS] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};                                                    // 0x59c260 (written, never read)

// ---- .bss (0x2305618-0x2318e43) ---------------------------------------------------------------------------
int fx_num_sky;                                       // 0x2305618
ubyte shadow_blob_palette[256 * 3];                   // 0x230561c

// 0x489c40  Confidence: exact. sinit_fx_obj_2305920: static initialiser entry 251 (compiler-generated).
// 0x489c50  Confidence: exact. init_fx_obj_2305920: the constructor on fx_active_list (compiler-generated).
// 0x489c60  Confidence: exact. register_dtor_fx_obj_2305920: atexit(dtor_fx_obj_2305920) (compiler-generated).
// 0x489c70  Confidence: exact. dtor_fx_obj_2305920: the destructor on fx_active_list (compiler-generated).
effect_object fx_active_list;                         // 0x2305920
fx_char_link fx_char_link_nodes[MAX_FX_CHAR_LINKS];   // 0x2305a30

// 0x489c80  Confidence: exact. sinit_fx_pool: static initialiser entry 252 (compiler-generated).
// 0x489c90  Confidence: exact. fx_pool_construct: `eh vector constructor iterator`(fx_pool, 0x10c, 0x30,
//           fx_obj__ctor, fx_obj__dtor) (compiler-generated).
// 0x489cb0  Confidence: exact. register_dtor_fx_pool: atexit(dtor_fx_pool) (compiler-generated).
// 0x489cc0  Confidence: exact. dtor_fx_pool: `eh vector destructor iterator` (compiler-generated).
effect_object fx_pool[MAX_FX_OBJECTS];                // 0x2305bc0
fx_char_link fx_char_link_free;                       // 0x2308e00
fx_char_link fx_char_link_used;                       // 0x2308e18
ubyte shadow_blob_pixels[NUM_SHADOW_BLOBS][SHADOW_BLOB_SIZE * SHADOW_BLOB_SIZE];   // 0x2308e2c
effect_object *fx_sky_list[MAX_FX_SKY];               // 0x2318e2c
int fx_count;                                         // 0x2318e40

// 0x489ce0  Confidence: exact. The first free (type 0) object of the pool, linked as an effect (type 2).
effect_object *fx_alloc()
{
    int i;

    for (i = 0; i < MAX_FX_OBJECTS; i++) {
        if (fx_pool[i].type == 0)
            break;
    }
    if (i == MAX_FX_OBJECTS)
        return NULL;
    fx_count++;
    fx_pool[i].link_new(2);
    return &fx_pool[i];
}

// 0x489d30  Confidence: exact (VC6 byte match) (the order of the stores after the timers). Bits 20-23 cleared, 24-31 set.
effect_object::effect_object()
{
    type = 0;
    _bits20 = 0;
    _bits24 = -1;
    instance = NULL;
}

// 0x489db0  Confidence: exact. The element destructor of fx_pool and fx_active_list's atexit destructor.
effect_object::~effect_object()
{
}

// 0x489dc0  Confidence: exact (VC6 byte match) (the two link loops' form). The virtual destroy: frees the spell effect
// record and the instance (a weapon missile has none of its own), removes the avoidance area, gives the
// character links of this effect back (and clears the characters' link flag, which the second loop sets
// again for every character still linked), unlinks the object and frees it.
void effect_object::deinit()
{
    fx_char_link *l;
    fx_char_link *next;

    if (spell_effect) {
        spell_effect_free((::spell_effect *)spell_effect);
        spell_effect = NULL;
    }
    if (instance && !(fx_flags & 8)) {
        vfx_instance_free(instance);
        instance = NULL;
    }
    if (avoid_area >= 0) {
        printf("Destroying AA\n");
        avoid_area_remove(avoid_area);
    }
    for (l = fx_char_link_used.next, next = l->next; l != &fx_char_link_used; l = next, next = next->next) {
        if (l->fx == this) {
            list_remove(l);
            if (l->character)
                l->character->flags &= ~0x200000;
            list_append(&fx_char_link_free, l);
        }
    }
    for (l = fx_char_link_used.next; l != &fx_char_link_used; l = l->next) {
        if (l->character)
            l->character->flags |= 0x200000;
    }
    fx_list_remove(this);
    fx_count--;
    type = 0;
    obj::deinit();
}

// 0x489f00  Confidence: exact. 17 callers.
obj *obj_from_handle_b(int handle)
{
    return obj_from_handle(handle);
}

// 0x489f10  Confidence: exact (VC6 byte match, D24500: the ramps indexed shadow_blob_pixels[i][row * 64 + x]
// in a for over i, so 8966 makes the row pointer its own induction register and keeps y in the frame; y-- before
// row++. The scale's unsigned conversion: fild qword of 32 - i; D15009: dx as an int local). A grey palette and
// 16 radial ramps of 64 x 64 (alpha 255 at the centre, falling to 0 at radius 32; ramp i is steeper by
// 32 / (32 - i)), each made a bitmap. Nothing reads shadow_blob_bms (shadows use Shadow01.tga).
int shadow_create_blob_bitmap()
{
    unsigned int i;
    unsigned int x;
    int y;
    int row;
    ubyte *p;
    int *bm;
    float scale;
    float v;

    for (i = 0; i < 256; i++) {
        shadow_blob_palette[i * 3] = (ubyte)i;
        shadow_blob_palette[i * 3 + 1] = (ubyte)i;
        shadow_blob_palette[i * 3 + 2] = (ubyte)i;
    }
    for (i = 0; i < NUM_SHADOW_BLOBS; i++) {
        scale = 32.0f / (32 - i);
        for (row = 0, y = 32; row < 64; y--, row++) {
            for (x = 0; x < 64; x++) {
                int dx = 32 - x;
                v = 32.0f - (float)sqrt((float)(dx * dx + y * y));
                if (0.0f > v)
                    v = 0.0f;
                v *= scale;
                if (32.0f < v)
                    v = 32.0f;
                v *= 255.0f;
                shadow_blob_pixels[i][row * 64 + x] = (ubyte)(int)(v * 0.03125f);
            }
        }
    }
    for (bm = shadow_blob_bms, p = shadow_blob_pixels[0];
         p < shadow_blob_pixels[NUM_SHADOW_BLOBS - 1] + SHADOW_BLOB_SIZE * SHADOW_BLOB_SIZE;
         p += SHADOW_BLOB_SIZE * SHADOW_BLOB_SIZE, bm++)
        *bm = bm_create(1, 64, 64, p, shadow_blob_palette, -1);
    return 0;
}

// 0x48a060  Confidence: exact. GameInit.
void shadows_init()
{
    lamp_glow_load();
    attached_lights_init_slots();
    fx_lists_init();
    shadow_create_blob_bitmap();
    shadow_bm = bm_load("Shadow01.tga", -1, -1);
}

// 0x48a090  Confidence: exact (VC6 byte match) (the order of the head stores). The two link lists and the effect list
// empty, the 20 link records on the free list.
void fx_lists_init()
{
    int i;

    list_init(&fx_char_link_free);
    fx_active_list.fx_next = &fx_active_list;
    fx_active_list.fx_prev = &fx_active_list;
    list_init(&fx_char_link_used);
    for (i = 0; i < MAX_FX_CHAR_LINKS; i++)
        list_append(&fx_char_link_free, &fx_char_link_nodes[i]);
}

// 0x48a0e0  Confidence: exact. game_cleanup.
void fx_shutdown()
{
    lamp_glow_forget();
    attached_lights_clear_slots();
    fx_destroy_all();
}

// 0x48a0f0  Confidence: exact.
void fx_destroy_all()
{
    effect_object *fx;
    effect_object *next;
    fx_char_link *l;
    fx_char_link *lnext;

    for (fx = fx_active_list.fx_next, next = fx->fx_next; fx != &fx_active_list; fx = next, next = next->fx_next)
        fx_destroy(fx->handle);
    for (l = fx_char_link_used.next, lnext = l->next; l != &fx_char_link_used; l = lnext, lnext = lnext->next) {
        list_remove(l);
        list_append(&fx_char_link_free, l);
    }
    vfx_active_instance = NULL;
}

// 0x48a190  Confidence: exact. state_game_enter.
void fx_level_enter()
{
    lamp_glow_load();
    fx_sky_list_clear();
}

// 0x48a1a0  Confidence: exact.
void fx_sky_list_clear()
{
    fx_sky_list[0] = NULL;
    fx_sky_list[1] = NULL;
    fx_sky_list[2] = NULL;
    fx_sky_list[3] = NULL;
    fx_sky_list[4] = NULL;
    fx_num_sky = 0;
}

// 0x48a1d0  Confidence: exact. state_game_leave.
void fx_level_leave()
{
    lamp_glow_forget();
    fx_destroy_all();
    attached_lights_clear_slots();
    fx_sky_list_clear_thunk();
    effects_release_level_memory();
}

// 0x48a1f0  Confidence: exact (VC6 byte match) (a second function with fx_sky_list_clear's call as its body; compiled to
// a jmp). Called by fx_level_leave and fx_level_restart.
void fx_sky_list_clear_thunk()
{
    fx_sky_list_clear();
}

// 0x48a200  Confidence: exact. level_script_leave (restart).
void fx_level_restart()
{
    fx_destroy_all();
    attached_lights_clear_slots();
    fx_sky_list_clear_thunk();
    effects_release_level_memory();
    attached_lights_init_slots();
    fx_lists_init();
    fx_sky_list_clear();
}

// The hit record that the network game sends for a spell effect: the effect's spell, caster, position /
// orientation or none, the source type and item of the casting item, then the spell effect record.
// Expanded at each use (fx_frame, fx_missile_hit); a macro of ours.
#define FX_FILL_HIT(h, fx, p, o) do { (h).spell = (fx)->spell; (h).source = (fx)->caster; (h).pos = (p); \
        (h).orient = (o); (h).source_type = (fx)->target ? 5 : 0; (h).item = (ItemType *)(fx)->target; \
        (h).power = 0.0f; memcpy((ubyte *)&(h) + 0x1c, (fx)->spell_effect, sizeof(::spell_effect)); } while (0)

// 0x48a230  Confidence: likely (the case bodies follow the jump table 0x48b600 (kinds 0-16) and the byte
// table 0x48b64c (spells 0x1a-0x4e); the compiler merged their common tails, so where a case ends with the
// stop test is from the jump targets; the cases are in kind order, which also gives the order of the two
// literals, "stone-fade.vfx" before "spell-status.vfx"). Each effect object: its sound follows it, then
// the kind's update; a finished one is stopped. The fire / blizzard / dark wall areas are collected
// (room for 10 on the stack, no bound check) and put their status on the living entities inside them;
// leaving every area of a kind cures that status.
void fx_frame(float frametime, bool paused)
{
    effect_object *areas[10];
    effect_object *fx;
    int num_areas;
    bool done;
    bool keep;
    float progress;
    obj *o;
    Character *c;
    Character *t;
    int h;
    int i;

    vfx_last_mesh_instance = NULL;
    if (paused)
        return;
    num_areas = 0;
    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        done = false;
        if (fx->sound != -1)
            sound_emitter_update_position(fx->sound, &fx->pos, NULL, 1.75f);
        if (fx->obj_flags & 0x44)
            continue;
        switch (fx->kind) {
        case 0:
            done = fx_update_instance(fx, frametime);
            if (done == true) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        case 1:
        case 2:
        case 10:
            // on a character / object: follow it (kind 2: its body centre), keep the orientation unless
            // the effect base has flag 1
            o = obj_from_handle(fx->owner);
            if (o) {
                if (fx->kind == 2)
                    fx->pos = o->pos + o->centre_offset;
                else
                    fx->pos = o->pos;
                if (!effect_get_base(fx->instance->id_bits >> 9)) {
                    fx_stop(fx->handle);
                    continue;
                }
                if (!(effect_get_base(fx->instance->id_bits >> 9)->flags & 1))
                    fx->orient = o->orient;
            }
            done = fx_update_instance(fx, frametime);
            if (fx->kind == 10) {
                // stone: when the paralysis is over, the fade effect replaces it
                if (o && !((Character *)o)->has_status(STATUS_PARALYZE)) {
                    fx_attach_to_character("stone-fade.vfx", fx->owner, -1, -1, false, false);
                    fx_stop(fx->handle);
                    continue;
                }
                break;
            }
            if (fx->spell == 0xe) {
                // light: when the status is gone, the spell's hit effect replaces it
                if (o && !((Character *)o)->has_status(STATUS_LIGHT)) {
                    fx_attach_to_character(spells[fx->spell].hit_effect, fx->owner, -1, -1, false, false);
                    fx_stop(fx->handle);
                    continue;
                }
            } else if (fx->spell == 0x24 && !(fx->fx_flags & 0x100) && done && o) {
                h = fx_attach_to_character(spells[0x24].misc_effect, fx->owner, -1, -1, true, true);
                ((Character *)o)->status_set_8(STATUS_ICE_COFFIN, h);
            }
            if (done == true) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        case 3: {
            // at the camera, turned around: right negated, up and forward swapped and negated (0x48a6c6-0x48a7df:
            // the old up and forward are copied first; D9815)
            fx->pos = camera_view_pos;
            fx->orient = camera_view_orient;
            vector3 u(fx->orient.uvec);
            vector3 f(fx->orient.fvec);
            fx->orient.rvec = -fx->orient.rvec;
            fx->orient.uvec = -f;
            fx->orient.fvec = -u;
            done = fx_update_instance(fx, frametime);
            if (done == true) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        }
        case 4:
            done = fx_update_missile(fx, frametime);
            if (done == true) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        case 5:
        case 6:
            // a status effect fading in (5) or out (6): the character's alpha follows the progress
            fx_is_finished(fx->handle, &progress);
            if (fx->kind == 6)
                progress = 1.0f - progress;
            c = living_from_handle(fx->owner);
            if (!c) {
                fx_stop(fx->handle);
                continue;
            }
            fx->pos = c->pos;
            if (!effect_get_base(fx->instance->id_bits >> 9)) {
                fx_stop(fx->handle);
                continue;
            }
            if (!(effect_get_base(fx->instance->id_bits >> 9)->flags & 1))
                fx->orient = c->orient;
            done = fx_update_instance(fx, frametime);
            c->instance->_unkb4 = (ubyte)(int)(255.0f - progress * 235.0f);
            if (done == true) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        case 7: {
            // ground effect: the sound stops after 20 s
            int ms = fx->timer.GetElapsedTime();
            if (fx->sound != -1 && ms > 20000) {
                sound_emitter_delete(fx->sound, 200);
                fx->sound = -1;
            }
            done = fx_update_instance(fx, frametime);
            if (done == true) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        }
        case 8:
            // summon appearance: the summon emerges when the timer runs out
            done = fx_update_instance(fx, frametime);
            if (fx->timer.elapsed() == true) {
                fx->timer.invalidate();
                c = living_from_handle(fx->owner);
                if (c)
                    c->ai.begin_emerge();
            }
            if (done == true) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        case 9:
            // status holder: every 10 s a "spell-status.vfx" on the owner
            if (fx->timer.elapsed()) {
                h = fx_attach_to_character("spell-status.vfx", fx->owner, -1, -1, false, false);
                if (h != -1) {
                    ((effect_object *)obj_from_handle(h))->fx_flags |= 0x200;
                    fx_link_character(h, (Character *)obj_from_handle(fx->owner), false);
                    fx->timer.SetTimeout(10000);
                }
            }
            break;
        case 11:
            // spell area: the radius grows 4.72 per second to 5.003; the targets inside are hit once
            fx->radius += frametime * 4.72f;
            if (fx->radius >= 5.003f)
                done = true;
            if (game_flags & 2) {
                if (!fx->spell_effect) {
                    if (done == true) {
                        fx_stop(fx->handle);
                        continue;
                    }
                    break;
                }
                for (i = 0; i < 6; i++) {
                    if (fx->spell == 0x17 || fx->spell == 0x1e) {
                        t = living_from_handle(((::spell_effect *)fx->spell_effect)->data[i]);
                        if (t) {
                            float d = distance(&t->pos, &fx->pos);
                            if (done || d <= fx->radius) {
                                spell_hit hit;
                                FX_FILL_HIT(hit, fx, NULL, NULL);
                                t->spell_apply_hit_effects(&hit);
                                if (fx->spell == 0x17)
                                    ((::spell_effect *)fx->spell_effect)->data[i] = -1;
                                else if (fx->spell == 0x1e)
                                    ((::spell_effect *)fx->spell_effect)->data[i] = -1;
                            }
                        }
                    }
                }
            } else {
                for (i = 0; i < (int)fx->hit_count; i++) {
                    if (fx->hit_handles[i] != -1) {
                        t = living_from_handle(fx->hit_handles[i]);
                        if (t) {
                            if (distance(&t->pos, &fx->pos) > fx->radius)
                                continue;
                            fx_create_spell_effect_facing(fx->spell, fx->caster, t->handle);
                            t->get_hit_by_spell(fx->spell, fx->caster, NULL, NULL, fx->target ? 5 : 0,
                                                (ItemType *)fx->target, 0.0f, true);
                        }
                        fx->hit_handles[i] = -1;
                    }
                }
            }
            if (done == true) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        case 12:
            // near: the hit lands at frame 14
            if (!fx->timer.elapsed())
                break;
            done = fx_update_instance(fx, frametime);
            if (fx->instance->time >= 14.0f && !(fx->fx_flags & 0x10)) {
                fx->fx_flags |= 0x10;
                c = living_from_handle(fx->owner);
                if (c) {
                    if (game_flags & 2) {
                        spell_hit hit;
                        FX_FILL_HIT(hit, fx, NULL, NULL);
                        c->spell_apply_hit_effects(&hit);
                    } else {
                        c->get_hit_by_spell(fx->spell, fx->caster, NULL, NULL, fx->target ? 5 : 0,
                                            (ItemType *)fx->target, 0.0f, true);
                    }
                }
            }
            if (done == true) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        case 13: {
            // facing: stays on the target's centre, turned toward the caster in the horizontal plane
            Character *caster = living_from_handle(fx->caster);
            t = living_from_handle(fx->owner);
            if (!caster || !t) {
                fx_update_instance(fx, frametime);
                fx_stop(fx->handle);
                continue;
            }
            vector3 dir;
            dir.x = caster->pos.x - t->pos.x;
            dir.y = 0.0f;
            dir.z = caster->pos.z - t->pos.z;
            dir.normalize_safe();
            vector3 up(0.0f, 1.0f, 0.0f);
            vector3 right = dir.cross(up);
            matrix m(right, up, dir);
            fx->pos = t->pos + t->centre_offset;
            fx->orient = m;
            done = fx_update_instance(fx, frametime);
            if (done == true) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        }
        case 14:
            // at: the hit lands when the timer runs out; spell 0x66 hits every living entity but the caster
            if (fx->timer.elapsed() && !(fx->fx_flags & 0x10)) {
                if (fx->spell == 0x66) {
                    Character *caster = living_from_handle(fx->caster);
                    if (caster) {
                        if (game_flags & 2) {
                            spell_hit hit;
                            for (i = 0; i < 6; i++) {
                                t = living_from_handle(((::spell_effect *)fx->spell_effect)->data[i]);
                                if (t) {
                                    FX_FILL_HIT(hit, fx, NULL, NULL);
                                    t->spell_apply_hit_effects(&hit);
                                    ((::spell_effect *)fx->spell_effect)->data[i] = -1;
                                }
                            }
                        } else {
                            for (t = living_list.live_next; t != &living_list; t = t->live_next) {
                                if (t != caster)
                                    t->get_hit_by_spell(fx->spell, fx->caster, NULL, NULL, fx->target ? 5 : 0,
                                                        (ItemType *)fx->target, 0.0f, true);
                            }
                        }
                    }
                } else {
                    c = living_from_handle(fx->owner);
                    if (c) {
                        if (game_flags & 2) {
                            spell_hit hit;
                            FX_FILL_HIT(hit, fx, &fx->aim, &fx->orient);
                            c->spell_apply_hit_effects(&hit);
                        } else {
                            c->get_hit_by_spell(fx->spell, fx->caster, NULL, NULL, fx->target ? 5 : 0,
                                                (ItemType *)fx->target, 0.0f, true);
                        }
                    }
                }
                fx->fx_flags |= 0x10;
            }
            done = fx_update_instance(fx, frametime);
            if (done == true) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        case 15:
            // Titus' rock eruption: the damage at frame 55.25
            done = fx_update_instance(fx, frametime);
            if (fx->instance->time >= 55.25f && !(fx->fx_flags & 0x10)) {
                titus_rock_erupt_damage(&fx->pos, fx->owner);
                fx->fx_flags |= 0x10;
            }
            if (done == true) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        case 16: {
            // an equipped item's effect: a weapon's while it is in the hand (no prop tag), else it runs
            // out (the longest mesh / emitter time) and is freed; a shield's on its prop bone
            keep = true;
            c = (Character *)obj_from_handle(fx->owner);
            if (!c) {
                fx_stop(fx->handle);
                continue;
            }
            int slot = character_find_equipped_type(c, (ItemType *)fx->target);
            if (slot < 0) {
                keep = false;
            } else if (c->equipment[slot]) {
                ItemType *type = c->equipment[slot]->type;
                if (type->kind == 0) {
                    if (c->prop_tag == 0 && fx->instance == NULL) {
                        fx->instance = vfx_load(type->animation, 0);
                        if (fx->instance) {
                            fx_bind_character_tags(fx->instance, c->instance, c->info->prop_tag_bones[0]);
                            fx->instance->reset_flags(1, 0);
                            fx->fx_flags &= ~1;
                        }
                    } else if (c->prop_tag != 0) {
                        if (fx->instance == NULL)
                            goto follow;
                        if (!fx->timer.valid()) {
                            vfx_base *base = effect_get_base(fx->instance->id_bits >> 9);
                            int longest;
                            int ms;
                            if (!base)
                                break;
                            longest = -1;
                            for (i = 0; i < base->num_emitters; i++) {
                                ms = (int)(base->emitters[i].get_length_seconds() * 1000.0f);
                                if (longest <= ms)
                                    longest = ms;
                            }
                            for (i = 0; i < base->num_desc21c; i++) {
                                ms = (int)((base->descs21c[i].time_b + base->descs21c[i].time_a) * 1000.0f);
                                if (longest <= ms)
                                    longest = ms;
                            }
                            if (longest <= 0) {
                                vfx_instance_free(fx->instance);
                                fx->instance = NULL;
                                fx->fx_flags |= 1;
                            } else {
                                fx->instance->id_bits &= ~2;
                                fx->instance->stop();
                                fx->timer.SetTimeout(longest);
                            }
                        }
                    }
                } else if (fx->instance == NULL) {
                    fx->instance = vfx_load(type->animation, 0);
                    if (fx->instance) {
                        int tag = c->info->prop_tag_bones[15];
                        if (tag < 0)
                            tag = c->info->prop_tag_bones[1];
                        fx_bind_character_tags(fx->instance, c->instance, tag);
                        fx->instance->reset_flags(1, 0);
                        fx->fx_flags &= ~1;
                    }
                }
                if (fx->instance && fx->timer.elapsed()) {
                    vfx_instance_free(fx->instance);
                    fx->instance = NULL;
                    fx->fx_flags |= 1;
                    fx->timer.invalidate();
                }
            follow:
                fx->pos = c->pos;
                fx->orient = c->orient;
                fx_update_instance(fx, frametime);
            }
            if ((c->flags & CF_DEAD) || !keep) {
                fx_stop(fx->handle);
                continue;
            }
            break;
        }
        }

        switch (fx->spell) {
        case 0x1a:
        case 0x1d:
        case 0x1f:
        case 0x23:
        case 0x4e:
            if (fx->avoid_area != -1)
                areas[num_areas++] = fx;
            break;
        }
    }

    snow_update(frametime);
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        bool in_fire;
        bool in_dark;
        bool in_blizzard;

        if (c->type == 4)
            continue;
        in_fire = false;
        in_dark = false;
        in_blizzard = false;
        for (i = 0; i < num_areas; i++) {
            effect_object *a = areas[i];
            Character *caster = living_from_handle(a->caster);
            ItemType *it;
            int ms;
            int duration;
            int status;
            float value;

#ifdef FIXED_BUILD
            // FIXED_BUILD: G270 AreaCasterGone. An area whose caster is gone is skipped; the original left the
            // loop, so every later area stopped counting and its status was cured below.
            if (!caster)
                continue;
#else
            if (!caster)
                break;
#endif
            ms = a->timer.GetElapsedTime();
            duration = 0;
            if (a->spell == 0x23 && ms < 3000)
                continue;
            if (avoid_area_contains_point(&c->pos, c->layer, c->info->size, a->avoid_area) != true)
                continue;
            status = -1;
            if (a->spell == 0x1d) {
                duration = 20000;
                status = STATUS_FIREWALL;
                in_fire = true;
            } else if (a->spell == 0x1f) {
                duration = 20000;
                status = STATUS_DARK_WALL;
                in_dark = true;
            } else if (a->spell == 0x1a || a->spell == 0x4e) {
                duration = 11000;
                status = STATUS_FIREWALL;
                in_fire = true;
            } else if (a->spell == 0x23) {
                duration = 20000;
                status = STATUS_BLIZZARD;
                in_blizzard = true;
            }
            if (duration < frametime)
                break;
            it = (ItemType *)a->target;
            if (a->spell == 0x1a || a->spell == 0x4e) {
                if (it && it->charge_effect->_unk0c > 0)
                    value = (float)-it->charge_effect->_unk0c;
                else
                    value = (float)-caster->level;
            } else {
                if (it && it->charge_effect->_unk0c > 0)
                    value = status_effect_modifier(status, it->charge_effect->_unk0c);
                else
                    value = status_effect_modifier(status, caster->level);
            }
            c->status_effect_add(status, 100000, a->spell, value, caster->handle, false);
        }
        if (!in_fire)
            c->status_effect_cure(STATUS_FIREWALL, true, false);
        if (!in_blizzard)
            c->status_effect_cure(STATUS_BLIZZARD, true, false);
        if (!in_dark)
            c->status_effect_cure(STATUS_DARK_WALL, true, false);
    }
}

// 0x48b690  Confidence: exact (VC6 byte match) (strcmp expanded inline). Binds the nodes of an effect to a character's
// mesh tags: all to the given tag, or (tag -1) each node but "Scene Root" that can be bound to the tag of
// its own name.
void fx_bind_character_tags(vfx_instance *vi, character_instance *ci, int tag)
{
    int n;
    int i;
    int t;

    if (!effect_get_base(vi->id_bits >> 9))
        return;
    n = effect_get_base(vi->id_bits >> 9)->get_num_tags();
    for (i = 0; i < n; i++) {
        t = tag;
        if (t == -1) {
            if (strcmp(vi->get_node_name(i), "Scene Root") == 0)
                continue;
            if (!vi->node_can_bind(i))
                continue;
            t = ci->character->find_mesh_tag(vi->get_node_name(i));
            if (t < 0)
                continue;
        }
        vi->bind_node(i, ci, t);
    }
}

// 0x48b750  Confidence: exact. Advances the instance at the object; finished when it has no time left
// (or no instance).
bool fx_update_instance(effect_object *fx, float frametime)
{
    if (fx->instance) {
        fx->instance->process(frametime, &fx->pos, &fx->orient);
        if (fx->instance->get_time_left() != 0.0f)
            return false;
    }
    return true;
}

// 0x48b7a0  Confidence: likely (the plane is written out: its d sums z, y, x, unlike ix.h's inline
// constructor). Kind 4: flies toward the target's root bone height (spell missiles 12 / s, weapon
// missiles 20 / s); it arrives when the next step would cross the plane through the aim point or the
// direction turns by more than ~143 degrees; a weapon missile's trail start follows 3 behind.
bool fx_update_missile(effect_object *fx, float frametime)
{
    Character *t;
    vector3 step;
    vector3 aim;
    vector3 bone_pos;
    matrix bone_orient;
    matrix m;
    plane pl;
    float speed;

    t = living_from_handle(fx->owner);
    if (!t)
        return true;
    if (!(fx->fx_flags & 0x80)) {
        aim = t->pos + t->centre_offset;
        t->instance->get_bone_transform(&bone_pos, &bone_orient, t->instance->character->mesh->skin->root_bone);
        aim.y += bone_pos.y;
        step.x = aim.x - fx->pos.x;
        step.y = aim.y - fx->pos.y;
        step.z = aim.z - fx->pos.z;
        step.normalize();
        speed = frametime * ((fx->fx_flags & 8) ? 20.0f : 12.0f);
        step.x *= speed;
        step.y *= speed;
        step.z *= speed;
        pl.normal = fx->orient.fvec;
        pl.d = (aim.z * fx->orient.fvec.z + aim.y * fx->orient.fvec.y + aim.x * fx->orient.fvec.x) * -1.0f;
        if (pl.normal.z * (fx->pos.z + step.z) + pl.normal.y * (fx->pos.y + step.y) +
            pl.normal.x * (fx->pos.x + step.x) + pl.d >= 0.0f) {
            fx->aim = t->pos;
            fx->aim.y = fx->pos.y;
            fx->fx_flags |= 0x80;
        } else {
            m.set_from_forward(step);
            if (m.fvec.y * fx->orient.fvec.y + m.fvec.z * fx->orient.fvec.z + m.fvec.x * fx->orient.fvec.x < -0.8f) {
                fx->aim = t->pos;
                fx->aim.y = fx->pos.y;
                fx->fx_flags |= 0x80;
            } else {
                fx->pos.x += step.x;
                fx->pos.y += step.y;
                fx->pos.z += step.z;
                if (!(fx->fx_flags & 8))
                    fx->instance->process(frametime, &fx->pos, &fx->orient);
                else if (distance_squared(&fx->pos, &fx->start) >= 9.0f)
                    fx->start += step;
                fx->orient.rvec = m.rvec;
                fx->orient.uvec = m.uvec;
                fx->orient.fvec = m.fvec;
            }
        }
    }
    if (fx->fx_flags & 0x80)
        return fx_missile_hit(fx, frametime);
    return false;
}

// Inlined into fx_missile_hit (not a function of the binary; D24503): the spell-missile part. As a helper
// 8966 saves EBP / ESI only around it, as the binary does. Name ours.
static inline bool fx_missile_spell_part(effect_object *fx, float frametime)
{
    vfx_instance *vi;
    fx_recb0 *n;
    Character *c;

    vi = fx->instance;
    if (!vi->is_stopped()) {
        c = living_from_handle(fx->owner);
        vi->stop();
        if (fx->spell == 0x1a || fx->spell == 0x4e)
            fx->aim.y += 0.6f;
        if (game_flags & 2) {
            if (fx->spell_effect) {
                spell_hit hit;
                FX_FILL_HIT(hit, fx, &fx->aim, &fx->orient);
                c->spell_apply_hit_effects(&hit);
            }
        } else {
            c->get_hit_by_spell(fx->spell, fx->caster, &fx->aim, &fx->orient, fx->target ? 5 : 0,
                                (ItemType *)fx->target, 0.0f, true);
        }
    }
    vi->process(frametime, NULL, NULL);
    for (n = vi->emitters; n; ) {
        if (n->num_live > 0)
            return false;
        n = n->next;
        if (n == vi->emitters)
            break;
    }
    return true;
}

// 0x48bac0  Confidence: likely (the result variable: BL holds true for the weapon branch; D15009: the
// weapon tail as vector operations ending in `done = false; return done;`; D24503: the spell part as an
// inline helper and the timeout as ((d * 0.75f) * 1000.0f) * 0.05f, the binary's three fmuls; left: vi /
// c in EBP / EBX, the binary has them the other way round). A spell missile
// hits once (its instance stops, fireballs 0.6 higher) and is finished when no node has live particles; a
// weapon missile resolves the melee hit of its caster once, then its trail start catches up with the aim
// point (20 / s) for 0.75 * distance / 20 s.
bool fx_missile_hit(effect_object *fx, float frametime)
{
    bool done = true;
    Character *c;
    vector3 d;
    float s;

    if (!(fx->fx_flags & 8))
        return fx_missile_spell_part(fx, frametime);
    if (!(fx->fx_flags & 0x10)) {
        c = living_from_handle(fx->caster);
        if (c)
            c->combat.resolve_hit();
        fx->fx_flags |= 0x10;
        fx->timer.SetTimeout((int)(((distance(&fx->aim, &fx->start) * 0.75f) * 1000.0f) * 0.05f));
    }
    if (!fx->timer.elapsed()) {
        d = fx->aim - fx->start;
        d.normalize();
        d *= frametime * 20.0f;
        fx->start += d;
        done = false;
    }
    return done;
}

// 0x48bd40  Confidence: exact. Callers: the Pyrul and AI combat code (0x4b8da0, 0x4bf700, 0x4c57b0).
int fx_count_spells_1a_1d_4e()
{
    effect_object *fx;
    int n = 0;

    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (fx->spell == 0x1a || fx->spell == 0x4e || fx->spell == 0x1d)
            n++;
    }
    return n;
}

// 0x48bd80  Confidence: exact.
int fx_add_effect(const char *effect, const vector3 *pos, const matrix *orient, bool flag)
{
    effect_object *fx = fx_create_instance(effect, 0, pos, orient, flag);

    if (!fx) {
        printf("FAILED TO CREATE EFFECT IN FX_ADD_EFFECT()\n");
        return -1;
    }
    return fx->handle;
}

// 0x48bdc0  Confidence: exact (VC6 8966 byte match, D16601: the default orientation as a conditional
// expression (the binary copies from the constructor's return), the yaw as a float and a named matrix, the
// reset_flags call as if / else; the default position as a local, the node walk's form). Takes an object, links it last, loads the effect; nodes whose descriptor has
// flag 0x40 get a lamp glow; base flag 1 turns the orientation by a random yaw.
effect_object *fx_create_instance(const char *effect, int kind, const vector3 *pos, const matrix *orient, bool flag)
{
    effect_object *fx;
    vfx_base *base;
    fx_rec98 *n;
    vfx_node_desc *desc;
    int i;
    vector3 zero;

    fx = fx_alloc();
    if (!fx)
        return NULL;
    fx->obj_flags |= 1;
    fx->_bits20 = 0;
    fx->_bits24 = -1;
    fx->target = 0;
    fx_list_append(&fx_active_list, fx);
    fx->instance = vfx_load(effect, 0);
    if (!fx->instance) {
        printf("FAILED EFFECT CREATION!\n");
        fx->avoid_area = -1;
        fx->deinit();
        return NULL;
    }
    base = effect_get_base(fx->instance->id_bits >> 9);
    if (!base) {
        fx->deinit();
        return NULL;
    }
    n = fx->instance->nodes;
    if (n) {
        i = 0;
        do {
            desc = &base->node_descs[i];
            if (desc && (desc->flags & 0x40))
                n->glow = lamp_glow_add((const vector3 *)n, desc->glow_radius);
            n = n->next;
            i++;
        } while (n != fx->instance->nodes && n);
    }
    fx->kind = kind;
    if (!pos) {
        zero.x = 0.0f;
        zero.y = 0.0f;
        zero.z = 0.0f;
        pos = &zero;
    }
    fx->pos = *pos;
    fx->orient = orient ? *orient : matrix(vector3(1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f), vector3(0.0f, 0.0f, 1.0f));
    if (base->flags & 1) {
        float a = frand() * 6.2831855f;
        matrix rot(0.0f, 0.0f, a);
        fx->orient = rot * fx->orient;
    }
    fx->spell = -1;
    fx->owner = -1;
    fx->avoid_area = -1;
    fx->sound = -1;
    fx->fx_flags = 0;
    if (flag)
        fx->instance->reset_flags(0, 0);
    else
        fx->instance->reset_flags(1, 0);
    fx->spell_effect = NULL;
    return fx;
}

// 0x48c080  Confidence: exact. 30 callers. Created at the character (kind b ? 2 : 1), bound to its tags,
// processed once (1 ms) and linked to it.
int fx_attach_to_character(const char *effect, int handle, int tag, int source, bool a, bool b)
{
    Character *c;
    effect_object *fx;

    c = living_from_handle(handle);
    if (c) {
        fx = fx_create_instance(effect, b ? 2 : 1, &c->pos, &c->orient, !a);
        if (fx) {
            fx_bind_character_tags(fx->instance, c->instance, tag);
            fx->owner = handle;
            fx->source = source;
            fx->instance->process(0.001f, &fx->pos, &fx->orient);
            fx_link_character(fx->handle, c, false);
            return fx->handle;
        }
    }
    return -1;
}

// 0x48c140  Confidence: exact (VC6 8966 byte match, D16600: the turn written as fx_frame's kind 3, old up and
// forward copied first). cutscene_events_frame. The camera orientation is copied and turned around (right
// negated, up and forward swapped and negated, 0x48c14b-0x48c1ff), but the copy is never used: the instance is
// made with camera_view_orient itself (fx_frame turns kind 3 around every frame).
int fx_create_at_camera(const char *effect, bool flag)
{
    effect_object *fx;
    matrix m(camera_view_orient);

    vector3 u(m.uvec);
    vector3 f(m.fvec);
    m.rvec = -m.rvec;
    m.uvec = -f;
    m.fvec = -u;
    fx = fx_create_instance(effect, 3, &camera_view_pos, &camera_view_orient, flag);
    if (!fx)
        return -1;
    return fx->handle;
}

// 0x48c220  Confidence: exact (VC6 8966 byte match, w10: the two handle tests nested with one shared -1, dir as
// a vector3 difference; the orient argument is never read). spell_fire_missile and the Giant
// Salamanka. Spell 0x37 (the generic arrow) is a weapon missile without an instance (trail bitmap of
// spells[0x37].misc_effect, radius 0.1); the others load the spell's misc effect.
int fx_create_missile(int spell, const vector3 *start, const matrix *orient, int caster, int target)
{
    Character *t;
    Character *c;
    effect_object *fx;
    vector3 dir;
    matrix m;

    t = living_from_handle(target);
    if (t) {
        c = living_from_handle(caster);
        if (c) {
            dir = t->pos - c->pos;
            dir.normalize();
            m.set_from_forward(dir);
            if (spell == SPELL_GENERIC_ARROW) {
                fx = fx_new(4);
                if (!fx)
                    return -1;
                fx->instance = NULL;
                fx->fx_flags |= 8;
                fx->pos = *start;
                fx->start = *start;
                fx->orient = m;
                fx->radius = 0.1f;
                fx->trail_bitmap = bm_load(spells[SPELL_GENERIC_ARROW].misc_effect, -1, -1);
            } else {
                fx = fx_create_instance(spells[spell].misc_effect, 4, start, &m, false);
                if (!fx)
                    return -1;
            }
            fx->spell = spell;
            fx->caster = caster;
            fx->owner = target;
            return fx->handle;
        }
    }
    return -1;
}

// 0x48c3e0  Confidence: exact (the order of the stores: spell_effect last, D7700). An object without
// instance, linked last. The spell and owner keep the values of the slot's last use.
effect_object *fx_new(int kind)
{
    effect_object *fx = fx_alloc();

    if (!fx)
        return NULL;
    fx_list_append(&fx_active_list, fx);
    fx->instance = NULL;
    fx->target = 0;
    fx->obj_flags |= 1;
    fx->kind = kind;
    fx->fx_flags = 1;
    fx->_bits20 = 0;
    fx->_bits24 = -1;
    fx->sound = -1;
    fx->avoid_area = -1;
    fx->spell_effect = NULL;
#ifdef FIXED_BUILD
    // FIXED_BUILD: G271 EffectSlotSpell. A new object starts with no spell (-1, as fx_create_instance stores);
    // the original kept the slot's last spell, so a status holder could count as a burning fireball.
    fx->spell = -1;
#endif
    return fx;
}

// 0x48c470  Confidence: exact (VC6 byte match) (the second test's early return). Nothing while the character wears a stone
// effect; else its status holders stop and the looping status effect (spells[11].hit_effect) is attached
// with the given kind (5 / 6, fx_frame's fading kinds).
int status_effect_update_fx(int handle, int status)
{
    effect_object *fx;
    effect_object *o;
    int h;

    if (!living_from_handle(handle))
        return -1;
    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (fx->kind == 10 && fx->owner == handle)
            return -1;
    }
    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (fx->kind == 9 && fx->owner == handle)
            fx_stop(fx->handle);
    }
    h = fx_attach_to_character(spells[11].hit_effect, handle, -1, -1, false, false);
    if (h == -1)
        return -1;
    o = (effect_object *)obj_from_handle_b(h);
    if (!o)
        return -1;
    o->kind = status;
    return h;
}

// 0x48c540  Confidence: exact. spell_place_ground_effect: kind 7 with sound 11 (volume 100).
int fx_create_ground_effect(int spell, int caster, const vector3 *pos, const matrix *orient)
{
    effect_object *fx = fx_create_instance(spells[spell].hit_effect, 0, pos, orient, true);

    if (!fx)
        return -1;
    fx->kind = 7;
    fx->sound = sound_create_emitter(11, 100, 0);
    fx->timer.SetTimeout(0);
    fx->spell = spell;
    fx->caster = caster;
    return fx->handle;
}

// 0x48c5c0  Confidence: exact. status_effect_add, status_effect_add_from_item: spell 14's misc effect for
// spell 14's duration.
int fx_create_status_effect(int handle)
{
    effect_object *fx;
    int h;

    h = fx_attach_to_character(spells[14].misc_effect, handle, -1, -1, true, true);
    if (h == -1)
        return -1;
    fx = (effect_object *)obj_from_handle_b(h);
    if (!fx)
        return -1;
    fx->spell = 0xe;
    fx->timer.SetTimeout(spells[14].duration * 1000);
    return fx->handle;
}

// 0x48c630  Confidence: exact (D15004: the network branch and the enemy search as if / else with one
// return, so 8966 shrink-wraps the pushes behind the NULL test; the two identical target loops are kept). spell_cast_execute,
// spell_cast_launch_effects. Kind 11 for 1060 ms; the targets are the cast parameters' handles in a
// network game, else the caster's enemies within 10.0 (at most 6).
int fx_create_spell_area(int spell, int caster, const vector3 *pos, spell_cast_params *params)
{
    effect_object *fx;
    Character *c;
    Character *t;
    int i;

    fx = fx_new(11);
    if (!fx)
        return -1;
    fx->timer.SetTimeout(1060);
    fx->pos = *pos;
    fx->radius = 0.0f;
    fx->hit_count = 0;
    fx->caster = caster;
    fx->spell = spell;
    c = living_from_handle(caster);
    if ((game_flags & 2) && params) {
        if (spell == 0x17) {
            for (i = 0; i < 6; i++) {
                if (params->target[i] != -1) {
                    fx->hit_handles[fx->hit_count] = params->target[i];
                    fx->hit_count++;
                }
            }
        } else {
            for (i = 0; i < 6; i++) {
                if (params->target[i] != -1) {
                    fx->hit_handles[fx->hit_count] = params->target[i];
                    fx->hit_count++;
                }
            }
        }
    } else {
        for (t = living_list.live_next; t != &living_list; t = t->live_next) {
            if (c->is_enemy(t) && distance(pos, &t->pos) < 10.0f) {
                fx->hit_handles[fx->hit_count] = t->handle;
                fx->hit_count++;
                if (fx->hit_count == 6)
                    break;
            }
        }
    }
    return fx->handle;
}

// 0x48c800  Confidence: exact. spell_cast_execute: the hit effect (kind 12) at the owner, turned to a free
// side 4.24 out.
int fx_create_spell_effect_near(int spell, int caster, int owner, int ms)
{
    Character *c;
    effect_object *fx;
    matrix m;

    c = living_from_handle(owner);
    if (!c)
        return -1;
    fx_find_spawn_point(&m, c, 4.24f);
    fx = fx_create_instance(spells[spell].hit_effect, 12, &c->pos, &m, true);
    if (!fx)
        return -1;
    fx->caster = caster;
    fx->owner = owner;
    fx->spell = spell;
    fx->timer.SetTimeout(ms);
    return fx->handle;
}

// 0x48c8a0  Confidence: likely (the right vector as dir x up with the constant up folded, as in fx_frame
// kind 13). Kind 13 at the target, facing the caster.
int fx_create_spell_effect_facing(int spell, int caster, int target)
{
    Character *c;
    Character *t;
    effect_object *fx;
    vector3 dir;

    c = living_from_handle(caster);
    t = living_from_handle(target);
    if (!c || !t)
        return -1;
    vector3 *tp = &t->pos;
    dir.set(c->pos.x - tp->x, 0.0f, c->pos.z - tp->z);
    dir.normalize_safe();
    vector3 up(0.0f, 1.0f, 0.0f);
    vector3 right = dir.cross(up);
    matrix m(right, up, dir);
    fx = fx_create_instance(spells[spell].hit_effect, 13, &t->pos, &m, true);
    if (!fx)
        return -1;
    fx->spell = spell;
    fx->owner = target;
    fx->caster = caster;
    return fx->handle;
}

// 0x48c9a0  Confidence: exact. The Titus unit (0x4c3240): kind 15 at the character, bound to all its tags.
int titus_rock_erupt_fx(int handle)
{
    Character *c;
    effect_object *fx;

    c = living_from_handle(handle);
    if (!c)
        return -1;
    fx = fx_create_instance("Titus-RockErrupt.VFX", 15, &c->pos, &c->orient, true);
    if (!fx)
        return -1;
    fx_bind_character_tags(fx->instance, c->instance, -1);
    fx->owner = handle;
    fx->source = -1;
    fx->instance->process(0.001f, &fx->pos, &fx->orient);
    fx_link_character(fx->handle, c, false);
    return fx->handle;
}

// 0x48ca40  Confidence: exact (VC6 byte match) (switch on the item kind: `sub eax, 0`). character_equip_item: a weapon, or
// an armour item of slot 5 (the shield), with an animation effect gets a kind 16 object (fx_frame loads
// the effect).
int character_equip_item_fx(int handle, ItemType *type)
{
    effect_object *fx;

    if (!type || !type->animation || !living_from_handle(handle))
        return -1;
    switch (type->kind) {
    case 0:
        break;
    case 1:
        if (type->slot == 5)
            break;
    default:
        return -1;
    }
    fx = fx_new(16);
    if (!fx)
        return -1;
    fx->owner = handle;
    fx->target = (int)type;
    fx->timer.invalidate();
    return fx->handle;
}

// 0x48cab0  Confidence: exact. Kind 14 at pos / orient, by default the owner's.
int fx_create_spell_effect_at(int spell, int caster, int owner, int ms, const vector3 *pos, const matrix *orient)
{
    Character *c;
    effect_object *fx;

    c = living_from_handle(owner);
    if (!c)
        return -1;
    if (!orient)
        orient = &c->orient;
    fx = fx_create_instance(spells[spell].hit_effect, 14, pos ? pos : &c->pos, orient, true);
    if (!fx)
        return -1;
    fx->caster = caster;
    fx->owner = owner;
    fx->spell = spell;
    fx->timer.SetTimeout(ms);
    return fx->handle;
}

// 0x48cb40  Confidence: exact (the order of the record stores; the two extents in one chained assignment,
// D7700). summon_create: the appearance effect
// (kind 8) linked to the summon; with a radius, a prohibited circle (first field 1) for the avoidance code
// around it on the given layer.
int summon_spawn_effect(const char *effect, const vector3 *pos, const matrix *orient, Character *c, int ms,
                        float radius, int layer)
{
    int h;
    effect_object *fx;
    avoid_area_desc d;

    h = fx_add_effect(effect, pos, orient, true);
    if (h == -1)
        return -1;
    fx_link_character(h, c, false);
    fx = (effect_object *)obj_from_handle_b(h);
    if (!fx)
        return -1;
    fx_bind_character_tags(fx->instance, c->instance, -1);
    fx->kind = 8;
    fx->owner = c->handle;
    if (radius != 0.0f) {
        d._unk00 = 1;
        d.shape = 1;
        d.cx = pos->x;
        d.cz = pos->z;
        d.ax = orient->rvec.x;
        d.az = orient->rvec.z;
        d.bx = orient->fvec.x;
        d.bz = orient->fvec.z;
        d.extent_a = d.extent_b = radius;
        d._unk28 = 0.5f;
        d.owner = layer;
        fx->avoid_area = avoid_area_add(&d);
    }
    fx->timer.SetTimeout(ms);
    return h;
}

// 0x48cc60  Confidence: exact. character_update_status_effects: -1 while invisible (status 8) or while a
// stone effect is on the character; the existing holder (kind 9), else a new one.
int fx_get_status_holder(int handle)
{
    Character *c;
    effect_object *fx;

    c = living_from_handle(handle);
    if (!c)
        return -1;
    if (c->has_status(STATUS_INVISIBLE))
        return -1;
    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (fx->kind == 10 && fx->owner == c->handle)
            return -1;
        if (fx->kind == 9 && fx->owner == c->handle)
            return fx->handle;
    }
    fx = fx_new(9);
    if (!fx)
        return -1;
    fx->owner = handle;
    fx->timer.SetTimeout(0);
    return fx->handle;
}

// 0x48cd20  Confidence: exact. character_update_status_effects (petrified): the existing stone effect, or
// the status holders stop and "stone-loop.vfx" (kind 10) is attached.
int status_stone_fx(int handle)
{
    effect_object *fx;
    effect_object *o;
    int h;

    if (!living_from_handle(handle))
        return -1;
    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (fx->kind == 10 && fx->owner == handle)
            return fx->handle;
    }
    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (fx->kind == 9 && fx->owner == handle)
            fx_stop(fx->handle);
    }
    h = fx_attach_to_character("stone-loop.vfx", handle, -1, -1, true, false);
    if (h != -1) {
        o = (effect_object *)obj_from_handle_b(h);
        if (!o)
            return -1;
        o->kind = 10;
    }
    return h;
}

// 0x48cdf0  Confidence: exact. status_effect_add, character_clear_all_effects: the holders and the status
// effects (kind 1 with flag 0x200) of the owner.
void fx_stop_status_effects(int handle)
{
    effect_object *fx;

    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (fx->owner == handle && (fx->kind == 9 || (fx->kind == 1 && (fx->fx_flags & 0x200))))
            fx_stop(fx->handle);
    }
}

// 0x48ce50  Confidence: exact. The owner's stone effects (kind 10).
void fx_stop_status_aura(int handle)
{
    effect_object *fx;

    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (fx->kind == 10 && fx->owner == handle)
            fx_stop(fx->handle);
    }
}

// 0x48cea0  Confidence: exact. Level effects and traps: kind 0 at the origin; "sky" effects go in the
// sky list (5 entries, no bound check).
int fx_create_world(const char *effect, bool flag)
{
    effect_object *fx = fx_create_instance(effect, 0, &vector3_zero, &identity_matrix, flag);

    if (!fx)
        return -1;
    fx->fx_flags |= 2;
    if (stristr(effect, "sky")) {
        fx_sky_list[fx_num_sky] = fx;
        fx->fx_flags |= 4;
        fx_num_sky++;
    }
    return fx->handle;
}

// 0x48cf50  Confidence: exact. spell_cast_execute, spell_cast_launch_effects: the casting item, once.
void fx_set_target(int fx, int target)
{
    obj *o = obj_from_handle(fx);

    if (o && o->type == 2 && ((effect_object *)o)->target == 0)
        ((effect_object *)o)->target = target;
}

// 0x48cf80  Confidence: exact. The sound fades out (200 ms); the object ends with the frame (flag 4).
void fx_stop(int fx)
{
    effect_object *o = (effect_object *)obj_from_handle_b(fx);

    if (o) {
        if (o->sound != -1)
            sound_emitter_delete(o->sound, 200);
        o->obj_flags |= 4;
    }
}

// 0x48cfc0  Confidence: exact.
void fx_destroy(int handle)
{
    effect_object *o = (effect_object *)obj_from_handle_b(handle);

    if (o) {
        if (o->sound != -1)
            sound_emitter_delete(o->sound, 200);
        o->deinit();
    }
}

// 0x48d000  Confidence: exact (VC6 byte match) (the progress expression's form). Finished when the object is gone or its
// instance has no time left; progress = 1 - left / (frames / 15). The instance is not checked for NULL.
bool fx_is_finished(int fx, float *progress)
{
    effect_object *o = (effect_object *)obj_from_handle_b(fx);
    vfx_instance *vi;
    float left;

    if (!o) {
        if (progress)
            *progress = 1.0f;
        return true;
    }
    vi = o->instance;
    left = vi->get_time_left() - vi->length;
    if (0.0f > left)
        left = 0.0f;
    if (progress)
        *progress = 1.0f - left / (effect_get_base(vi->id_bits >> 9)->num_frames * (1.0f / 15.0f));
    if (left <= 0.0f)
        return true;
    return false;
}

// 0x48d0a0  Confidence: exact. character_on_death, character_play_move.
bool fx_remove_missiles_from(int handle)
{
    effect_object *fx;
    bool found = false;

    if (handle < 0)
        return found;
    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (fx->source == handle) {
            spell_missile_remove(fx->handle);
            if (fx->instance)
                fx->instance->stop();
            found = true;
        }
    }
    return found;
}

// 0x48d100  Confidence: exact. Weapon missiles are stopped, the rest destroyed or let run out.
void fx_end_owned(int handle, int destroy)
{
    effect_object *fx;
    effect_object *next;

    for (fx = fx_active_list.fx_next, next = fx->fx_next; fx != &fx_active_list; fx = next, next = next->fx_next) {
        if (fx->owner == handle) {
            if (!(fx->fx_flags & 8)) {
                if (fx->instance) {
                    if (!destroy)
                        fx->instance->stop();
                    else
                        fx_destroy(fx->handle);
                }
            } else
                fx_stop(fx->handle);
        }
    }
}

// 0x48d180  Confidence: exact. levelrender_setup_level_lights.
attached_light *level_light_get(int i)
{
    return attached_light_slots[i].type != -1 ? &attached_light_slots[i] : NULL;
}

// 0x48d1b0  Confidence: exact. summon_create.
void fx_summon_hook_stub(int fx, int arg)
{
    effect_object *o = (effect_object *)obj_from_handle_b(fx);

    if (o)
        fx_summon_hook(o, arg);
}

// 0x48d1d0  Confidence: exact (VC6 byte match) (the order of the record stores). Only effects whose base has +0x40 set,
// and only while a link record is free.
void fx_link_character_obj(effect_object *fx, Character *c, bool hook)
{
    vfx_base *base;
    fx_char_link *l;

    if (!fx->instance)
        return;
    base = effect_get_base(fx->instance->id_bits >> 9);
    if (!base || !base->_unk40)
        return;
    l = fx_char_link_free.next;
    if (l == &fx_char_link_free)
        return;
    l->_unk0c = 0;
    l->character = c;
    l->fx = fx;
    c->flags |= 0x200000;
    list_remove(l);
    list_append(&fx_char_link_used, l);
    if (hook)
        fx_link_hook(fx, c);
}

// 0x48d280  Confidence: exact.
void fx_link_character(int fx, Character *c, bool hook)
{
    obj *o = obj_from_handle(fx);

    if (o)
        fx_link_character_obj((effect_object *)o, c, hook);
}

// 0x48d2b0  Confidence: likely (the end points' expression forms; the turn of each side is written as row
// assignments). spell_cast_execute, fx_create_spell_effect_near. The four sides (forward, right, back,
// left at the distance) are tested against the walls of the grid cells they cross; a random free side is
// picked (any side when none is free) and the character's orientation turned to face it.
void fx_find_spawn_point(matrix *out, Character *c, float distance)
{
    vector2 ends[4];
    vector2 start;
    unsigned int free_sides = 0xf;
    int num_free = 4;
    float fx_, fz, rx, rz;
    int side;
    int n;
    int j;
    int k;
    int r;
    vector3 tmp;

    fx_ = distance * c->orient.fvec.x;
    fz = distance * c->orient.fvec.z;
    rx = distance * c->orient.rvec.x;
    rz = distance * c->orient.rvec.z;
    ends[0].x = fx_ + c->pos.x;
    ends[0].y = fz + c->pos.z;
    ends[1].x = rx + c->pos.x;
    ends[1].y = rz + c->pos.z;
    ends[2].x = c->pos.x - fx_;
    ends[2].y = c->pos.z - fz;
    ends[3].x = c->pos.x - rx;
    ends[3].y = c->pos.z - rz;
    start.x = c->pos.x;
    start.y = c->pos.z;
    for (side = 0; side < 4; side++) {
        n = pf_cells_along_segment(c->layer, 1, &start, &ends[side]);
        for (j = 0; j < n; j++) {
            pf_grid_cell *cell = pf_grid_cell_results[j];
            for (k = 0; k < cell->num_walls; k++) {
                if (seg2d_intersect(&start, &ends[side], *cell->walls[k]->a, *cell->walls[k]->b)) {
                    free_sides &= ~(1 << side);
                    num_free--;
                    break;
                }
            }
            if (!(free_sides & (1 << side)))
                break;
        }
    }
    if (num_free == 0) {
        side = rand() % 4;
    } else {
        r = rand() % num_free;
        for (side = 0; side < 4; side++) {
            if ((free_sides & (1 << side)) && --r < 0)
                break;
        }
    }
    out->rvec = c->orient.rvec;
    out->uvec = c->orient.uvec;
    out->fvec = c->orient.fvec;
    switch (side) {
    case 1:
        // right
        tmp = -out->fvec;
        out->fvec = c->orient.rvec;
        out->rvec = tmp;
        break;
    case 2:
        // back
        out->fvec = -out->fvec;
        out->rvec = -out->rvec;
        break;
    case 3:
        // left
        tmp = out->fvec;
        out->fvec = vector3(-out->rvec.x, -out->rvec.y, -out->rvec.z);
        out->rvec = tmp;
        break;
    }
}

// 0x48d660  Confidence: exact. Level scripts, cmd_Ignore_tutorial: clears the active bit of the effects of
// that file (only instances of type 0; the walk ends at the first effect whose base is gone).
void fx_disable_by_effect_name(const char *name)
{
    effect_object *fx;
    vfx_base *base;

    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
#ifdef FIXED_BUILD
        // FIXED_BUILD: C272 EffectSearchGuard. Objects without an instance (arrows, spell areas, status holders,
        // item effects) are skipped, and an effect without a base goes on to the next object instead of ending
        // the search.
        if (fx->instance && fx->instance->category == 0) {
            base = effect_get_base(fx->instance->id_bits >> 9);
            if (!base)
                continue;
#else
        if (fx->instance->category == 0) {
            base = effect_get_base(fx->instance->id_bits >> 9);
            if (!base)
                return;
#endif
            if (!_stricmp(base->name, name))
                fx->obj_flags &= ~1;
        }
    }
}

// 0x48d6d0  Confidence: exact. cutscene_play.
void fx_pause_for_cutscene()
{
    effect_object *fx;

    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (fx->instance && !effect_is_cutscene_base(fx->instance->id_bits >> 9))
            fx->obj_flags = (fx->obj_flags & ~1) | 0x40;
    }
}

// 0x48d720  Confidence: exact. cutscene_play: spell effects and instances of type 2.
void fx_destroy_spell_effects()
{
    effect_object *fx;
    effect_object *next;

    for (fx = fx_active_list.fx_next, next = fx->fx_next; fx != &fx_active_list; fx = next, next = next->fx_next) {
        if (fx->spell != -1 || (fx->instance && fx->instance->category == 2))
            fx_destroy(fx->handle);
    }
}

// 0x48d770  Confidence: exact. cutscene_cleanup.
void fx_resume_after_cutscene()
{
    effect_object *fx;

    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
        if (fx->instance && !effect_is_cutscene_base(fx->instance->id_bits >> 9))
            fx->obj_flags = (fx->obj_flags & ~0x40) | 1;
    }
}

// ---- the hooks (their code is the shared empty function 0x4f13e0, so no address of their own) ----------

// folded (0x4f13e0, empty_function)  Confidence: likely. fx_link_character_obj's hook (fx, character).
void fx_link_hook(effect_object *fx, Character *c)
{
}

// folded (0x4f13e0, empty_function)  Confidence: likely. fx_summon_hook_stub's hook (fx, argument).
void fx_summon_hook(effect_object *fx, int arg)
{
}
