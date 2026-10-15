// engine/objects/objects.h - the base object, the object list, containers, item objects and the party
// inventory (engine\objects\objects.cpp, path reference 0x469c68; objects.cpp functions 0x4697e0-0x46b45b,
// pilot game/objects.cpp; the party_inventory_* functions 0x46b460-0x46c530 follow in an optimised unit
// of their own, file not known).
// Not original source.
#ifndef ENGINE_OBJECTS_OBJECTS_H
#define ENGINE_OBJECTS_OBJECTS_H

#include "vsdk/vsdk.h"
#include "vsdk/math/matrix.h"
#include "engine/objects/items.h"

class Character;
class container;
struct shop_item;

#define MAX_OBJECTS    1000           // obj_slots, handle % 1000 (obj_from_handle, handle_to_index)
#define MAX_ITEM_OBJS  1000           // item_obj_pool (eh vector constructor count 0x3e8)
#define MAX_CONTAINER_ITEMS 16        // container +0x98 (container_add_item compares with 0x10)

// Object types (+0xc) seen in objects.cpp: 0 free, 1 animated (character_process_anim in
// objects_update_frame), 3 item object (item_obj_spawn), 4/5/7/9 living, 6/8/10 their dead forms.
#define OBJ_TYPE_NONE  0
#define OBJ_TYPE_ITEM  3
#define OBJ_TYPE_CONTAINER 11         // decomp24: the multicontainer.cpp handlers test it (0xb)

// The base object (0x98 bytes, DECISIONS.md D13): vtable obj__vtable (0x5819c4, one slot), the object
// list links, then the common fields that obj_link_new (0x469b60) initialises (+0xc type, +0x34, +0x80,
// +0x84). The derived objects start their own fields at +0x98 (container items, item_obj item,
// effect_object next, Character team). obj_active_list (0x970770, up to 0x970808) is a whole object used
// as the list head. The destructor is not virtual: objects_free_all calls 0x469b40 directly.
class obj {
public:
    obj();                            // 0x469a70 obj__obj
    obj(int type);                    // 0x469b10 obj__ctor_link (links when type != 0)
    ~obj();                           // 0x469b40 obj__dtor
    virtual void deinit();            // slot 0, 0x469e10 obj__deinit (objects_delete_marked);
                                      // 0x44edf0 living_entity, 0x46a710 item_obj, 0x46a920 container
    void link_new(int type);          // 0x469b60 obj_link_new
    bool is_living();                 // 0x46a0b0 obj_is_living (types 4, 5, 7, 9)
    bool is_dead_type();              // 0x46a100 character_is_dead_type (types 6, 8, 10)

    obj *next;                        // +0x004 obj_active_list
    obj *prev;                        // +0x008
    int type;                         // +0x00C object type: 2 effect, 3 item, 7 party member (Character)
    vector3 pos;                      // +0x010 world position
    vector3 last_pos;                 // +0x01C previous position
    vector3 centre_offset;            // +0x028 offset from pos to the body centre (inferred); zeroed by
                                      //        obj__obj and obj_link_new
    void *start_navpoint;             // +0x034 start navpoint / marker
    matrix orient;                    // +0x038 orientation
    matrix last_orient;               // +0x05C previous orientation
    int handle;                       // +0x080 object handle (obj_create)
    unsigned int obj_flags;           // +0x084 bit 0 active, bit 2 delete (objects_delete_marked),
                                      //        0x20 force visible (character_process_anim)
    float bound_radius;               // +0x088
    int net_handle;                   // +0x08C multiplayer handle, -1 unless multiplayer
    int _unk90;                       // +0x090
    int _unk94;                       // +0x094
};
LAYOUT_CHECK(obj, sizeof(obj) == 0x98);

// One handle slot (12 bytes): circular lists through next / prev whose head is a pointer (NULL when
// empty; vsdk.h plist_* macros). The slot index plus obj_handle_base_next() is the object handle.
struct obj_slot {
    obj_slot *next;                   // +0x0
    obj_slot *prev;                   // +0x4
    obj *object;                      // +0x8
};
LAYOUT_CHECK(obj_slot, sizeof(obj_slot) == 0xc);

extern obj obj_active_list;           // 0x970770 head (next 0x970774, prev 0x970778)
extern int obj_handle_next;           // 0x59a5a0 = 1, saved by savegame_write_living
extern int obj_handle_max;            // 0x59a5a4 = 0x7fffffff / MAX_OBJECTS (label ours)
extern int num_objects;               // 0x9438a0
extern obj_slot *obj_used_list;       // 0x9438a4
extern int obj_slots_unk_9438a8;      // 0x9438a8 zeroed by objects_init, never read (label ours)
extern obj_slot *obj_free_list;       // 0x9438ac
extern obj_slot obj_slots[MAX_OBJECTS];   // 0x9438b0
extern bool objects_initialized;      // 0x970809

void living_entities_update_footsteps();              // 0x4698c0
void objects_init();                                  // 0x469900
int  obj_create(obj *o);                              // 0x469c40, the new handle or -1
int  obj_handle_base_next();                          // 0x469dd0
void objects_free_all_thunk();                        // 0x469fb0
void handle_slots_reset_thunk();                      // 0x4a36f0 jmp handle_slots_reset (GameInit; unit not known)
void handle_slot_release(int handle);                 // 0x4a36a0 (ai_set_mode releases ai_state +0x80; unit not known, decomp39)
void objects_free_all();                              // 0x469fc0
obj *obj_from_handle(int handle);                     // 0x46b130
Character *living_from_handle(int handle);            // 0x46b1a0
int  handle_to_index(int handle);                     // 0x46b1e0
void objects_reset();                                 // 0x46b200
void living_entity_level_start();                     // 0x46b210
void objects_do_frame();                              // 0x46b220
void objects_delete_marked();                         // 0x46b330
void living_entities_move_all();                      // 0x46b390
void objects_update_frame();                          // 0x46b3d0
void obj_mark_for_delete(void *obj);                  // 0x489920

// An item lying in the level or in a container (0xac bytes, vtable 0x5819c8; item_obj_pool, 1000).
class item_obj : public obj {
public:
    item_obj();                       // 0x46a150 item_obj__ctor
    ~item_obj();                      // 0x46a130 item_obj__dtor
    virtual void deinit();            // 0x46a710 item_obj__deinit
    int is_gold();                    // 0x46a790 obj_is_gold (thiscall)
    bool is_in_view();                // 0x46a7e0 item_obj_is_in_view (bounding sphere of the type's mesh)
    item_info *item;                 // +0x98
    int count;                        // +0x9c stack size
    int level_item;                   // +0xa0 level_items index (level_item_spawn), -1 none
    container *in_container;          // +0xa4 the container holding it (container_add_item)
    int ground_face;                  // +0xa8 hit record +0x24 of the floor probe, -1 none (meaning unsure)
};
LAYOUT_CHECK(item_obj, sizeof(item_obj) == 0xac);
extern item_obj item_obj_pool[MAX_ITEM_OBJS];        // 0x946790

item_obj *item_obj_spawn(item_info *item, const vector3 *pos, const matrix *orient, int count,
                         int level_item, int layer);  // 0x46a1b0
container *level_item_spawn(item_info *item, const vector3 *pos, const matrix *orient, int count,
                            int level_item, int layer);   // 0x46a640
void item_obj_pickup(item_obj *it, int flag);          // 0x46b6b0

// A container object (0xe8 bytes, pool 0x2302fe0, 40 records; container__ctor 0x46a890, vtable 0x5819cc).
class container : public obj {
public:
    container();                      // 0x46a890 container__ctor
    ~container();                     // 0x46a8e0 container__dtor
    virtual void deinit();            // 0x46a920 container_deinit
    bool is_in_view();                // 0x46a960 container_is_in_view
    void add_item(item_obj *it);      // 0x46aa30 container_add_item
    void remove_item(item_obj *it);   // 0x46ab60 container_remove_item (thiscall)
    void clear();                     // 0x46ac20 container_clear
    item_obj *get_item(int i);        // 0x46acd0 container_get_item (thiscall)
    item_obj *items[MAX_CONTAINER_ITEMS]; // +0x98 (each item's +0xa4 points back here)
    int num_items;                    // +0xD8
    int user;                         // +0xDC -1 when not in use
    int mesh_item_type;               // +0xE0 item type index of the container mesh, -1 none
    float lock;                       // +0xE4 lock value, 0 unlocked
};
LAYOUT_CHECK(container, sizeof(container) == 0xe8);

// The second argument is the lock value (monster_drop_loot pushes a float; decomp37, D1709).
container *container_obj_create(const vector3 *pos, float lock);   // 0x4897f0
void equipment_clear(item_info **equipment);          // 0x46bda0 zeroes the 10 slots
void equipment_free(item_info **equipment);           // 0x46bdc0 item_free on the 10 slots

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
#define MAX_CONTAINERS 40
extern container containers[MAX_CONTAINERS];         // 0x2302fe0 (containers_construct; decomp27)

int  party_inventory_find_first_in_category(int category, party_inventory_entry *list, int count); // 0x46b5b0
int  shop_inventory_find_first_in_category(int category, shop_item *list, int count);  // 0x46b5d0
int  item_type_shop_category(int type);              // 0x46b5f0 (0 weapons .. 4 misc, -1)
void party_inventory_sort(party_inventory_entry *inv, int count);   // 0x46b640
void party_inventory_add_item(item_info *item, int a, int b);       // 0x46b870 (called (item, 1, 1))
void party_inventory_add_type(int type, int count, int a, int b);   // 0x46b980 (called (type, 1, -1, 1); return unused)
void party_inventory_remove(int slot, int a, int b); // 0x46bb30 (called (slot, 1, 1) and (slot, 0, 1))
int  party_inventory_find_type(ItemType *type);      // 0x46bd00, index or -1
// decomp35: (name, count, -1, 1) from container_examine; return unused. The last two not known.
void party_inventory_add_by_name(const char *name, int count, int a, int b);   // 0x46bb00
int  character_find_equipped_type(Character *c, ItemType *type);    // 0x46c3b0 slot or -1
void party_inventory_clear();                        // 0x46bbb0 item_free on every entry (decomp24)
int  party_inventory_find_type_index(int type);      // 0x46bd60 by item type index, -1 none (decomp24)
// decomp41 (level_scripts_common.cpp): qsort of shop entries; any of the ten slots holds that type.
void shop_inventory_sort(shop_item *list, int count);              // 0x46b660
bool character_has_equipped_type(Character *c, ItemType *type);    // 0x46bd30
// The container pool (unit not known; state_game_enter / state_game_leave / level_script_restart).
void containers_reset();                             // 0x489790
void containers_free_all();                          // 0x4897c0

// Using and equipping items (the party-inventory block after objects.cpp, 0x46bc00-0x46c4e0).
bool character_use_inventory_item(Character *c, party_inventory_entry *entry, int target_handle,
                                  void *point, int arg);              // 0x46bc00 (point type guessed)
void character_equip_item(item_info **out_old1, item_info **out_old2, Character *c, int slot,
                          item_info *item);                           // 0x46bdf0 (return unused)
item_info *character_unequip_slot(Character *c, int slot, int update_model); // 0x46c240
int  items_same_type(item_info *a, item_info *b);    // 0x46c3e0
bool character_use_equipped_item(Character *c, int slot, int target_handle,
                                 void *point, int arg);               // 0x46c410 (point type guessed)
bool character_wearing_servant_disguise(Character *c);              // 0x46c4e0

// .rdata 0x5819d4 (name ours, after the container vtable 0x5819cc): the item paging slot of each
// equipment index, read by inventory_page_in_equipped and 0x4afc4d. [5] and [6] are -1 (weapon and
// shield use item_paging_find_player_slot instead).
extern const int equip_page_slots[9];                // {10, 13, 14, 15, 12, -1, -1, 16, 11}

#endif
