// objects.cpp - Sum.exe v1.40, code 0x4697e0-0x46b45b (Engine\Objects\objects.cpp)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: sum.symbols tags 48 functions from 0x4697e0 (sinit_obj_list_970770) to 0x46b3d0
// (objects_update_frame) with objects.cpp. The previous unit (gamesound.cpp) ends with feedback_play's
// code before 0x4697e0; the next function, party_inventory_find_next_in_category (0x46b460), is
// optimised code (no frame pointer, 0x90 padding) and starts another unit.
//
// Build: this unit was compiled WITHOUT optimisation (every function opens an ebp frame, 0xcc padding,
// each statement reloads its operands; docs/sum-exe/source-map.md "Version 2"). The code is a literal
// translation, so statement order, locals and temporaries follow the disassembly. Three details show
// that inline functions WERE expanded here (/Ob1 with /Od, or the inline keyword honoured):
//   - vector3's (x, y, z) constructor, operator= (returning a copy), operator- and operator+= appear
//     as field-by-field moves through a pointer local (obj__obj, obj_link_new, item_obj_spawn, ...),
//     while some nested calls stay out of line (vector3 copy constructor 0x410cd0 and operator=
//     0x413190 inside matrix::operator= in item_obj_spawn, vector3::unrotate 0x5245f0);
//   - the scalar deleting destructor of obj is expanded at both `delete` sites (objects_free_all:
//     `mov eax, 1 / and eax, 1 / test` = the flags test with the constant 1);
//   - profile_label's constructor and destructor leave no call in character_process_anim, only the
//     EH state stores; the unwind funclet 0x57c3f0 calls the out-of-line destructor 0x434650.
// Local variables: VC6 12.00.8168 lays out /Od locals by a hash of their names (DECISIONS.md D6101): the
// names and declaration order are chosen so that it reproduces the frame offsets noted per function.

#include <stdio.h>
#include <string.h>
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/ai/ai.h"
#include "engine/rendering/levelrender.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/profile.h"
#include "vsdk/os/memory.h"
#include "vsdk/gr/gr.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/ca/skeleton.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "levelscripts/level_scripts.h"

// Original __FILE__ of this unit (literal 0x59a5a8; note "Objects" with a capital O).
#define OBJECTS_CPP "D:\\projects\\Summoner\\pccode\\Engine\\Objects\\objects.cpp"

// ---- .rdata ----------------------------------------------------------------------------------------
// vtables 0x5819c4 (obj: 0x469e10), 0x5819c8 (item_obj: 0x46a710), 0x5819cc (container: 0x46a920), one
// slot each: compiler-generated from the virtual deinit. 0x5819d0 = 225.0f (character_process_anim) is
// this unit's own constant; 3.0f (0x57ef1c), 0.2f (0x57ef10), 0.0005f (0x581940) and 0.0f (0x57eb84)
// are shared __real@ constants of earlier units.

// ---- .data (0x59a5a0-0x59a5a7, the literals follow) ----------------------------------------------
int obj_handle_next = 1;                       // 0x59a5a0
int obj_handle_max = 0x7fffffff / MAX_OBJECTS; // 0x59a5a4 = 0x20c49b (label ours)

// ---- .bss (0x9438a0-0x970809) --------------------------------------------------------------------
int num_objects;                      // 0x9438a0
obj_slot *obj_used_list;              // 0x9438a4
int obj_slots_unk_9438a8;             // 0x9438a8 (label ours)
obj_slot *obj_free_list;              // 0x9438ac
obj_slot obj_slots[MAX_OBJECTS];      // 0x9438b0
// The two objects with dynamic initialisers. The initialiser table runs obj_active_list (entry 202)
// before item_obj_pool (entry 203), so they are defined in this order; their .bss addresses are the
// other way round (0x970770 > 0x946790), the open .bss-order question of player.cpp / gamesound.cpp.
// 0x4697e0 sinit_obj_list_970770 (call init, call register; frame-pointer wrapper; entry 202). Confidence: exact.
// 0x4697f0 init_obj_list_970770 (obj::obj(0) on 0x970770). Confidence: exact.
// 0x469810 register_dtor_obj_list_970770 (atexit(0x469830)). Confidence: exact.
// 0x469830 dtor_obj_list_970770 (obj::~obj). Confidence: exact.
obj obj_active_list(0);               // 0x970770
// 0x469840 sinit_item_obj_pool_946790 (entry 203). Confidence: exact.
// 0x469850 init_item_obj_pool_946790 (eh vector constructor iterator: 1000 x 0xac, item_obj::item_obj,
//          item_obj::~item_obj). Confidence: exact.
// 0x469880 register_dtor_item_obj_pool_946790 (atexit(0x4698a0)). Confidence: exact.
// 0x4698a0 dtor_item_obj_pool_946790 (eh vector destructor iterator). Confidence: exact.
item_obj item_obj_pool[MAX_ITEM_OBJS]; // 0x946790
bool objects_initialized;             // 0x970809 (0x970808, after the 0x98-byte head, is not referenced)

// 0x4698c0  Confidence: exact. c at ebp-4.
void living_entities_update_footsteps()
{
    Character *c;

    for (c = living_list.live_next; c != &living_list; c = c->live_next)
        c->update_footsteps();
}

// 0x469900  Confidence: exact. The list macros keep their while (0) tests (vsdk.h). The living-list
// sentinels link through live_next / live_prev (+0x318), so list_init is written out for them (a
// macro of their own in the original, not known). i at ebp-4.
void objects_init()
{
    int i;

    list_init(&obj_active_list);
    do { living_list.live_next = &living_list; living_list.live_prev = &living_list; } while (0);
    do {
        living_list_inactive.live_next = &living_list_inactive;
        living_list_inactive.live_prev = &living_list_inactive;
    } while (0);
    num_objects = 0;
    plist_init(obj_free_list);
    plist_init(obj_used_list);
    for (i = 0; i < MAX_OBJECTS; i++) {
        obj_slots[i].object = NULL;
        plist_append(obj_free_list, &obj_slots[i]);
    }
    obj_slots_unk_9438a8 = 0;
    objects_initialized = true;
}

// 0x469a70  Confidence: exact (the vector3(0, 0, 0) temporary at ebp-0xc and operator='s returned
// copy at ebp-0x18 are the inline expansions; the store order type, handle, flags, navpoint is the
// code's).
obj::obj()
{
    type = 0;
    handle = 0;
    obj_flags = 0;
    start_navpoint = NULL;
    centre_offset = vector3(0.0f, 0.0f, 0.0f);
}

// 0x469b10  Confidence: exact (`jne body / jmp end` is the /Od shape of `if (x == 0) return;`, as
// the `break` in item_obj_spawn's first loop; `if (type) link_new(type);` would be a single je).
obj::obj(int type)
{
    if (type == 0)
        return;
    link_new(type);
}

// 0x469b40  Confidence: exact (only the compiler's vtable store).
obj::~obj()
{
}

// 0x469b60  Confidence: exact (same inline vector3 expansion as obj::obj).
void obj::link_new(int type)
{
    this->type = type;
    handle = 0;
    obj_flags = 0;
    start_navpoint = NULL;
    centre_offset = vector3(0.0f, 0.0f, 0.0f);
    list_append(&obj_active_list, this);
    num_objects++;
    obj_create(this);
}

// 0x469c40  Confidence: exact. slot at ebp-4, the ?: temporary at ebp-8. The handle sum keeps the
// slot index in ESI across the call (left operand first). The error macro is the endless loop.
int obj_create(obj *o)
{
    obj_slot *slot;

    if (o == NULL)
        return -1;
    if (obj_free_list == NULL)
        VSDK_ERROR(OBJECTS_CPP, 384, "No more free objects.  Please increase MAX_OBJECTS");
    slot = obj_free_list;
    plist_remove(obj_free_list, slot);
    slot->object = o;
    plist_append(obj_used_list, slot);
    o->handle = (slot - obj_slots) + obj_handle_base_next();
    o->net_handle = ((game_flags & GF_MULTIPLAYER) && mp_local_player && (mp_local_player->flags & 1))
                    ? o->handle : -1;
    return o->handle;
}

// 0x469dd0  Confidence: exact. base at ebp-4.
int obj_handle_base_next()
{
    int base = obj_handle_next;

    obj_handle_next++;
    if (obj_handle_next > obj_handle_max)
        obj_handle_next = 1;
    return base * MAX_OBJECTS;
}

// 0x469e10  Confidence: exact. Frame: this ebp-8, slot ebp-4, the ?: temporary ebp-0xc.
void obj::deinit()
{
    obj_slot *slot;

    if (next != NULL && prev != NULL)
        list_remove(this);
    num_objects--;
    for (slot = obj_used_list; slot != NULL; slot = (slot->next == obj_used_list) ? NULL : slot->next) {
        if (slot->object == this) {
            plist_remove(obj_used_list, slot);
            plist_append(obj_free_list, slot);
            slot->object = NULL;
            break;
        }
    }
}

// 0x469fb0  Confidence: exact. The only body is the call (name from sum.symbols; callers
// objects_reset and 0x43e1f0).
void objects_free_all_thunk()
{
    objects_free_all();
}

// 0x469fc0  Confidence: exact. Frame: p ebp-4, o ebp-8; ebp-0x10 / -0xc / -0x1c and -0x18 / -0x14 /
// -0x20 are the temporaries of the two expanded `delete` expressions (obj has a non-virtual destructor,
// 0x469b40 is called directly). The first loop never ends unless the list is empty: obj::~obj does not
// unlink, so it relies on every object having been deinit-ed before.
void objects_free_all()
{
    Player *p;
    obj *o;

    while (obj_active_list.next != &obj_active_list) {
        o = obj_active_list.next;
        delete o;
    }
    for (p = player_list_other.next; p != &player_list_other; p = p->next) {
        o = obj_from_handle(p->handle);
        if (o) {
            delete o;
            p->handle = -1;
        }
    }
}

// 0x46a0b0  Confidence: exact. Switch on type - 4 through the 6-entry table 0x46a0e5 (4, 5, 7, 9 true;
// 6, 8 and everything else false); the switch value lives in ebp-8.
bool obj::is_living()
{
    switch (type) {
    case 4:
    case 5:
    case 7:
    case 9:
        return true;
    }
    return false;
}

// 0x46a100  Confidence: exact (three compares, no table for 6 / 8 / 10).
bool obj::is_dead_type()
{
    switch (type) {
    case 6:
    case 8:
    case 10:
        return true;
    }
    return false;
}

// 0x46a130  Confidence: exact.
item_obj::~item_obj()
{
}

// 0x46a150  Confidence: exact.
item_obj::item_obj()
{
    item = NULL;
    count = 0;
    type = 0;
    level_item = -1;
    in_container = NULL;
}

// 0x46a1b0  Confidence: likely. Frame: ray ebp-0x4c (its inline constructor is the store to ebp-0x1c =
// ray+0x30 before the first loop), floor ebp-0x50, found ebp-0x54, hit ebp-0x80, io ebp-0x84, i ebp-0x88,
// best ebp-0x8c, lo ebp-0x90; ebp-0x9c..-0xf0 are the temporaries of the expanded vector3 / matrix
// assignments. Two literal quirks: the free-slot search has no overflow check (i = 1000 indexes past the
// pool) and, because the lobj loop has no break, `i >= num_lobjs` always holds after it, so the floor is
// always probed again against every level object.
item_obj *item_obj_spawn(item_info *item, const vector3 *pos, const matrix *orient, int count,
                         int level_item, int layer)
{
    lobj_ray ray;
    int floor;
    bool found;
    lobj_hit hit;
    item_obj *io;
    int i;
    float best;
    lobj *lo;

    for (i = 0; i < MAX_ITEM_OBJS; i++) {
        if (item_obj_pool[i].type == 0)
            break;
    }
    io = &item_obj_pool[i];
    io->type = OBJ_TYPE_ITEM;
    io->link_new(io->type);
    io->level_item = level_item;
    io->item = item;
    io->pos = *pos;
    io->orient = *orient;
    io->obj_flags |= 1;
    if (layer >= 0) {
        found = false;
        best = -1000000.0f;
        floor = -1;
        for (i = 0; i < num_lobjs; i++) {
            lo = &lobjs[i];
            if (lo->pick_layer(&io->pos, 1) != layer)
                continue;
            ray.start = io->pos;
            ray.start.y += 3.0f;
            ray.dir.x = 0.0f;
            ray.dir.y = -10.0f;
            ray.dir.z = 0.0f;
            ray._unk18 = 0.0f;
            ray.flags = 0;
            ray.require_flags = 0;
            ray.exclude_flags = 0x400000;
            lobj_raycast(i, &ray, &hit, 1);
            if (hit.num_hits > 0 && ray.dir.y * hit.t > best) {
                best = ray.dir.y * hit.t;
                floor = hit.face;
                found = true;
            }
        }
        if (found) {
            io->pos.y = ray.start.y + best;
            io->ground_face = floor;
        }
        if (i >= num_lobjs)
            layer = -1;
    }
    if (layer < 0) {
        ray.start = io->pos;
        ray.start.y += 3.0f;
        ray.dir.x = 0.0f;
        ray.dir.y = -10.0f;
        ray.dir.z = 0.0f;
        ray._unk18 = 0.0f;
        ray.flags = 0;
        ray.require_flags = 0;
        ray.exclude_flags = 0x400000;
        lobj_raycast_all(&ray, &hit);
        if (hit.num_hits > 0) {
            io->pos.y = ray.dir.y * hit.t + ray.start.y;
            io->ground_face = hit.face;
        } else {
            io->ground_face = -1;
        }
    }
    io->count = count;
    return io;
}

// 0x46a640  Confidence: exact. c at ebp-4, io ebp-8; the do / while (0) block (first at ebp-0xc,
// operator= temporaries ebp-0x10..-0x20) is a macro expansion in the original (its name is not known):
// it moves the container onto its first item, 0.2 higher.
container *level_item_spawn(item_info *item, const vector3 *pos, const matrix *orient, int count,
                            int level_item, int layer)
{
    container *c;
    item_obj *io;

    c = container_obj_create(pos, 0);
    io = item_obj_spawn(item, pos, orient, count, level_item, layer);
    c->add_item(io);
    do {
        item_obj *first = c->get_item(0);
        if (first) {
            c->pos = first->pos;
            c->pos.y += 0.2f;
        }
    } while (0);
    return c;
}

// 0x46a710  Confidence: exact.
void item_obj::deinit()
{
    if (in_container) {
        in_container->remove_item(this);
        in_container = NULL;
    }
    if (item)
        item_free(item);
    item = NULL;
    count = 0;
    type = 0;
    obj::deinit();
}

// 0x46a790  Confidence: exact ("gold" 0x594430 is pooled with an earlier unit's literal).
int item_obj::is_gold()
{
    if (item && item->type && !_stricmp(item->type->name, "gold"))
        return 1;
    return 0;
}

// 0x46a7e0  Confidence: exact. centre ebp-0xc (declared first, assigned by the expanded operator=
// whose returned copy is ebp-0x1c), radius ebp-0x10; += is expanded, unrotate is a call.
bool item_obj::is_in_view()
{
    vector3 centre;
    float radius;

    centre = item->type->mesh->center;
    radius = item->type->mesh->radius;
    centre.unrotate(orient);
    centre += pos;
    return gr_cull_sphere(&centre, radius, 0);
}

// 0x46a890  Confidence: exact.
container::container()
{
    mesh_item_type = -1;
    num_items = 0;
    type = 0;
}

// 0x46a8e0  Confidence: exact.
container::~container()
{
    type = 0;
    num_items = 0;
}

// 0x46a920  Confidence: exact (printf to stdout, 0x56da2b).
void container::deinit()
{
    printf("deinit for container with handle %d\n", handle);
    clear();
    obj::deinit();
    type = 0;
}

// 0x46a960  Confidence: exact (as item_obj::is_in_view).
bool container::is_in_view()
{
    vector3 centre;
    float radius;

    if (mesh_item_type < 0)
        return true;
    centre = item_types[mesh_item_type].mesh->center;
    radius = item_types[mesh_item_type].mesh->radius;
    centre.unrotate(orient);
    centre += pos;
    return gr_cull_sphere(&centre, radius, 0);
}

// 0x46aa30  Confidence: exact. i at ebp-4, this ebp-8. Literal: a gold object merged into the gold
// already in slot 0 is deinit-ed, and is then still stored in slot 0 and counted (the merge branch
// falls into the common tail).
void container::add_item(item_obj *it)
{
    int i;

    if (num_items >= MAX_CONTAINER_ITEMS)
        return;
    if (it->is_gold()) {
        if (num_items > 0) {
            if (items[0]->is_gold()) {
                items[0]->count += it->count;
                it->deinit();
            } else {
                for (i = num_items; i > 0; i--)
                    items[i] = items[i - 1];
            }
        }
        it->in_container = this;
        items[0] = it;
        num_items++;
    } else {
        it->in_container = this;
        items[num_items] = it;
        num_items++;
    }
}

// 0x46ab60  Confidence: exact (frame: the found index at ebp-8, the shift index at ebp-4: VC6 orders
// the locals by the hash of their names, D6101).
void container::remove_item(item_obj *it)
{
    int index;
    int i;

    for (index = 0; index < num_items; index++) {
        if (items[index] == it) {
            items[index]->in_container = NULL;
            break;
        }
    }
    for (i = index; i < num_items - 1; i++)
        items[i] = items[i + 1];
    num_items--;
}

// 0x46ac20  Confidence: exact.
void container::clear()
{
    int i;

    if (num_items <= 0 || num_items > MAX_CONTAINER_ITEMS) {
        num_items = 0;
        for (i = 0; i < MAX_CONTAINER_ITEMS; i++)
            items[i] = NULL;
    } else {
        for (i = 0; i < num_items; i++)
            items[i]->in_container = NULL;
        num_items = 0;
    }
}

// 0x46acd0  Confidence: exact.
item_obj *container::get_item(int i)
{
    if (i < 0 || i >= num_items)
        return NULL;
    return items[i];
}

// 0x46ad10  Confidence: exact.
bool Character::anim_lod_skip()
{
    if (flags & CF_ANIM_LOD)
        flags |= CF_ANIM_LOD_PREV;
    else
        flags &= ~CF_ANIM_LOD_PREV;
    flags &= ~CF_ANIM_LOD;
    if (loco_blend < 0.0005f && instance->action_anim == -1) {
        flags |= CF_ANIM_LOD;
        anim_lod_counter++;
        if (anim_lod_counter > 4) {
            anim_lod_counter = 0;
            return false;
        }
        return true;
    }
    return false;
}

// 0x46adf0  Confidence: exact for the profile label (its text is not in the binary: the inline
// constructor leaves no reference to it; the object at ebp-0x10 is known only from the EH state and the
// unwind funclet), likely for the rest. Frame: profiler ebp-0x10, visible ebp-0x14, lod ebp-0x18, the
// expanded operator- result ebp-0x24..-0x1c, read in place by the inline len_sq (z, y, x; D6107). VC6
// 12.00.8168 copies the inline return value once more (12 bytes), which shifts the temporaries.
// Two returns: each path ends with its own EH state -1 store.
void Character::process_anim(float frametime)
{
    profile_label profiler("character_process_anim");
    bool visible;
    int lod;

    lod = 1;
    if ((game_flags & 0x8) || (game_flags & 0x20))
        lod = 0;
    visible = false;
    if (is_in_view(1.0f) || (obj_flags & 0x20))
        visible = true;
    if (level_is_town && anim_lod_skip() && (flags & CF_ANIM_LOD_PREV)) {
        if ((cur_character->pos - pos).len_sq() > 225.0f) {
            instance->process(frametime, true, lod);
            return;
        }
    }
    instance->process(frametime, visible, lod);
}

// 0x46af70  Confidence: exact. i at ebp-4, name ebp-8 (a block local), this ebp-0xc. Fallbacks in the
// code's order: the hurt and walk/run variants borrow from their pairs.
void Character::fill_locomotion_anims()
{
    int i;

    for (i = 0; i < NUM_LOCO_ANIMS; i++) {
        char *name = locomotion_anim_names[i];
        loco_anims[i] = charinfo_find_anim(info, name);
    }
    if (anim_stand < 0)
        anim_stand = anim_ready;
    if (anim_ready < 0)
        anim_ready = anim_stand;
    if (anim_walk < 0)
        anim_walk = anim_norm_walk;
    if (anim_norm_walk < 0)
        anim_norm_walk = anim_walk;
    if (anim_run < 0)
        anim_run = anim_norm_run;
    if (anim_norm_run < 0)
        anim_norm_run = anim_run;
    if (anim_stand_hurt < 0)
        anim_stand_hurt = anim_stand;
    if (anim_ready_hurt < 0)
        anim_ready_hurt = anim_ready;
    if (anim_walk_hurt < 0)
        anim_walk_hurt = anim_norm_walk;
    if (anim_norm_walk_hurt < 0)
        anim_norm_walk_hurt = anim_walk;
    if (anim_run_hurt < 0)
        anim_run_hurt = anim_norm_run;
    if (anim_norm_run_hurt < 0)
        anim_norm_run_hurt = anim_run;
}

// 0x46b130  Confidence: exact. i at ebp-4. The index is computed inline, not with handle_to_index.
obj *obj_from_handle(int handle)
{
    int i;

    if (handle == -1)
        return NULL;
    i = handle % MAX_OBJECTS;
    if (i < 0 || i >= MAX_OBJECTS)
        return NULL;
    if (obj_slots[i].object && obj_slots[i].object->handle == handle)
        return obj_slots[i].object;
    return NULL;
}

// 0x46b1a0  Confidence: exact (`and eax, 0xff / cmp eax, 1` is the comparison with true).
Character *living_from_handle(int handle)
{
    obj *o = obj_from_handle(handle);

    if (!o)
        return NULL;
    if (o->is_living() == true)
        return (Character *)o;
    return NULL;
}

// 0x46b1e0  Confidence: exact (the remainder goes through a local at ebp-4).
int handle_to_index(int handle)
{
    int i = handle % MAX_OBJECTS;

    return i;
}

// 0x46b200  Confidence: exact. state_title_enter.
void objects_reset()
{
    objects_free_all_thunk();
    items_init_pool();
}

// 0x46b210  Confidence: exact. state_game_enter.
void living_entity_level_start()
{
    living_entity_reset_tables();
    cast_effect_tracks_reset();
}

// 0x46b220  Confidence: exact. i at ebp-4. Speech bubbles time out, finished cooldowns are freed.
void objects_do_frame()
{
    int i;

    objects_delete_marked();
    ai_do_frame();
    living_entities_update_footsteps();
    living_entities_move_all();
    objects_update_frame();
    for (i = 0; i < MAX_CHARACTER_SPEECH; i++) {
        if (character_speech[i].speaker != -1) {
            character_speech[i].time_left -= reported_frametime;
            if (character_speech[i].time_left < 0.0f) {
                character_speech[i].time_left = 0.0f;
                character_speech[i].speaker = -1;
            }
        }
    }
    for (i = 0; i < MAX_SPELL_COOLDOWNS; i++) {
        if (spell_cooldowns[i].handle != -1 && spell_cooldowns[i].timer.elapsed())
            spell_cooldowns[i].handle = -1;
    }
}

// 0x46b330  Confidence: exact. objp ebp-4, next ebp-8 (D6101); a while loop (no jump over a step
// expression).
void objects_delete_marked()
{
    obj *objp;
    obj *next;

    if (!objects_initialized)
        objects_init();
    objp = obj_active_list.next;
    while (objp != &obj_active_list) {
        next = objp->next;
        if (objp->obj_flags & 4)
            objp->deinit();
        objp = next;
    }
}

// 0x46b390  Confidence: exact (a for loop whose condition loads the next link first: the step is
// `c = next`, the condition `next = c->live_next, c != head`).
void living_entities_move_all()
{
    Character *c;
    Character *next;

    for (c = living_list_inactive.live_next; next = c->live_next, c != &living_list_inactive; c = next)
        c->move_frame();
}

// 0x46b3d0  Confidence: exact. objp ebp-4, i ebp-8, l ebp-0xc (the names give that order, D6101).
// Type-1 objects are animated as characters; level objects with flag 0x100 are doors.
void objects_update_frame()
{
    obj *objp;
    int i;
    lobj *l;

    for (objp = obj_active_list.next; objp != &obj_active_list; objp = objp->next) {
        if (objp->type == 1)
            ((Character *)objp)->process_anim(reported_frametime);
    }
    for (i = 0; i < num_lobjs; i++) {
        l = &lobjs[i];
        if (l->flags & 0x100)
            door_update(l);
    }
}
