// engine/objects/objects.h - the base object, the object list, containers, item objects and the party
// inventory (engine\objects\objects.cpp, path reference 0x469c68; objects.cpp functions 0x4697f0-0x46b330
// in sum.symbols; the party_inventory_* functions 0x46b5d0-0x46bd00 lie in the same range).
// Not original source.
#ifndef ENGINE_OBJECTS_OBJECTS_H
#define ENGINE_OBJECTS_OBJECTS_H

#include "vsdk/vsdk.h"
#include "vsdk/math/matrix.h"
#include "engine/objects/items.h"

class Character;
struct shop_item;

// The base object (0x98 bytes, DECISIONS.md D13): vtable obj__vtable, the object list links, then the
// common fields that obj_link_new (0x469b60) initialises (+0xc type, +0x34, +0x80, +0x84). The derived
// objects start their own fields at +0x98 (container items, item_obj item, effect_object next,
// Character team). obj_active_list (0x970770, up to 0x970808) is a whole object used as the list head.
class obj {
public:
    virtual void vfunc0();            // slot 0, deinit (objects_delete_marked); 0x44edf0 for living_entity
    obj *next;                        // +0x004 obj_active_list
    obj *prev;                        // +0x008
    int type;                         // +0x00C object type: 2 effect, 7 party member (Character)
    vector3 pos;                      // +0x010 world position
    vector3 last_pos;                 // +0x01C previous position
    vector3 centre_offset;            // +0x028 offset from pos to the body centre (inferred)
    void *start_navpoint;             // +0x034 start navpoint / marker
    matrix orient;                    // +0x038 orientation
    matrix last_orient;               // +0x05C previous orientation
    int handle;                       // +0x080 object handle (obj_create)
    unsigned int obj_flags;           // +0x084 bit 0 active, bit 2 delete (objects_delete_marked)
    float bound_radius;               // +0x088
    int net_handle;                   // +0x08C multiplayer handle, -1 unless multiplayer
    int _unk90;                       // +0x090
    int _unk94;                       // +0x094
};
LAYOUT_CHECK(obj, sizeof(obj) == 0x98);

extern obj obj_active_list;           // 0x970770 head (next 0x970774, prev 0x970778)

obj *obj_from_handle(int handle);                     // 0x46b130
Character *living_from_handle(int handle);            // 0x46b1a0
void obj_mark_for_delete(void *obj);                  // 0x489920

// An item lying in the level or in a container (item obj pool of objects.cpp).
class item_obj : public obj {
public:
    int is_gold();                    // 0x46a790 obj_is_gold (thiscall)
    item_info *item;                  // +0x98
    int count;                        // +0x9c stack size
};
void item_obj_pickup(item_obj *it, int flag);          // 0x46b6b0

// A container object (0xe8 bytes, pool 0x2302fe0, 40 records; container__ctor 0x46a890, vtable 0x5819cc).
class container : public obj {
public:
    item_obj *get_item(int i);        // 0x46acd0 container_get_item (thiscall)
    void remove_item(item_obj *it);   // 0x46ab60 container_remove_item (thiscall)
    item_obj *items[16];              // +0x98 (each item's +0xa4 points back here)
    int num_items;                    // +0xD8
    int user;                         // +0xDC -1 when not in use
    int mesh_item_type;               // +0xE0 item type index of the container mesh, -1 none
    float lock;                       // +0xE4 lock value, 0 unlocked
};
LAYOUT_CHECK(container, sizeof(container) == 0xe8);

// ---- the party inventory ---------------------------------------------------------------------------
// One party_inventory entry (8 bytes). The stubs disagreed (int[521][2] vs this struct): DECISIONS.md D16.
struct party_inventory_entry {
    int count;                        // +0x0
    item_info *item;                  // +0x4
};
LAYOUT_CHECK(party_inventory_entry, sizeof(party_inventory_entry) == 8);
extern party_inventory_entry party_inventory[521];   // 0x970820
extern int party_gold;                               // 0x971868
extern int party_inventory_count;                    // 0x97186c

int  shop_inventory_find_first_in_category(int category, shop_item *list, int count);  // 0x46b5d0
int  item_type_shop_category(int type);              // 0x46b5f0 (0 weapons .. 4 misc, -1)
void party_inventory_sort(party_inventory_entry *inv, int count);   // 0x46b640
void party_inventory_add_type(int type, int count, int a, int b);   // 0x46b980 (called (type, 1, -1, 1); return unused)
void party_inventory_remove(int slot, int a, int b); // 0x46bb30 (called (slot, 1, 1))
int  party_inventory_find_type(ItemType *type);      // 0x46bd00, index or -1

#endif
