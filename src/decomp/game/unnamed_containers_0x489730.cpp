// unnamed_containers_0x489730.cpp - Sum.exe v1.40, code 0x489730-0x48992f: the container pool (its static
// initialiser, reset, free and create) and obj_mark_for_delete. THE FILE NAME IS A PLACEHOLDER: no assert
// path, no strings.tbl block, module "-" in sum.symbols (see NOTES.md for the boundary evidence; this block
// may be one file with the experience table and the effect objects that follow).
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: after multicontainer.cpp (ends 0x48972a) and before experience_parse_table (0x489930).
// Data: .data 0x59c230-0x59c247 ("backpack", "leather bag"); .bss 0x2302fe0-0x230541f (the pool, right
// after multichat.cpp's chat_history ring 0x22fede0-0x2302fdf). Static initialiser entry 250.

#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"

// 0x489730  Confidence: exact. sinit_containers: static initialiser entry 250 (compiler-generated: calls
//           the constructor loop below, then registers the destructor loop).
// 0x489740  Confidence: exact. containers_construct: `eh vector constructor iterator`(containers, 0xe8, 0x28,
//           container__ctor, container__dtor) (compiler-generated).
// 0x489760  Confidence: exact. register_dtor_containers: atexit(dtor_containers) (compiler-generated).
// 0x489770  Confidence: exact. dtor_containers: `eh vector destructor iterator` (compiler-generated).
container containers[MAX_CONTAINERS];                 // 0x2302fe0

// 0x489790  Confidence: exact. state_game_enter, level_script_restart.
void containers_reset()
{
    container *c;

    for (c = containers; c < &containers[MAX_CONTAINERS]; c++) {
        c->num_items = 0;
        c->type = OBJ_TYPE_NONE;
        c->lock = 0.0f;
    }
}

// 0x4897c0  Confidence: exact. state_game_leave, level_script_restart. The call is direct
// (container_deinit 0x46a920): the elements are objects, not pointers.
void containers_free_all()
{
    container *c;

    for (c = containers; c < &containers[MAX_CONTAINERS]; c++) {
        if (c->type)
            c->deinit();
    }
}

// 0x4897f0  Confidence: likely (the not-found exit returns the NULL pointer without testing it; the type
// is stored before obj_link_new stores it again). A loot container at pos: a "leather bag" or a "backpack"
// (coin flip), a random yaw, the given lock value.
container *container_obj_create(const vector3 *pos, float lock)
{
    container *c = NULL;
    int i;

    for (i = 0; i < MAX_CONTAINERS; i++) {
        if (containers[i].type == OBJ_TYPE_NONE) {
            c = &containers[i];
            break;
        }
    }
    if (c) {
        c->num_items = 0;
        c->mesh_item_type = item_type_find(frand_range(-1.0f, 1.0f) < 0.0f ? "leather bag" : "backpack");
        if (c->mesh_item_type == -1)
            return NULL;
        c->type = OBJ_TYPE_CONTAINER;
        c->link_new(OBJ_TYPE_CONTAINER);
        c->pos = *pos;
        vector3 axis(0.0f, 1.0f, 0.0f);
        c->orient.set_from_axis_angle(axis, frand_range(0.0f, 359.0f) * 0.017453f);
        c->obj_flags |= 1;
        c->bound_radius = 1.0f;
        c->lock = lock;
        c->user = -1;
    }
    return c;
}

// 0x489920  Confidence: exact. Bit 2 of the object flags: objects_delete_marked removes it (callers
// pickup_take_selected, mp_send_container_close).
void obj_mark_for_delete(void *o)
{
    ((obj *)o)->obj_flags |= 4;
}
