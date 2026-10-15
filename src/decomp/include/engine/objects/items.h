// engine/objects/items.h - item types and item records (engine\objects\items.cpp, path references
// 0x44d56d-0x44d63f; item_type_* 0x44b450-0x44b6b0 are items.cpp in sum.symbols). Not original source.
// Layout: symbols/types.h ItemType (fields +0x10 mesh and the unsigned +0x68 damage as in types.h
// 2026-10-15; +0x7c / +0x80 named after their use by item_paging.cpp).
#ifndef ENGINE_OBJECTS_ITEMS_H
#define ENGINE_OBJECTS_ITEMS_H

#include "vsdk/vsdk.h"
#include "vsdk/ca/skeleton.h"   // vim

class Character;

// ItemType +0x58 (+Charge Effect: / +Self Only Charge Effect:), 0x18 bytes.
struct item_charge_effect {
    int spell;                        // +0x00
    int max_charges;                  // +0x04 (buysell_buy_item compares the owned item's charges with it)
    int _unk08;                       // +0x08
    int _unk0c;                       // +0x0c
    ubyte self_only;                  // +0x10
    ubyte _pad11[3];
    float disposable;                 // +0x14 +Disposable
};
LAYOUT_CHECK(item_charge_effect, sizeof(item_charge_effect) == 0x18);

// An item type, shared by every item of that kind (items.tbl). item_types (0x8cbec0), stride 0x100.
struct ItemType {
    const char *name;        // +0x00 $Name: "Short Sword"
    int kind;                // +0x04 $Type: 0 Weapon, 1 Armor, 2 Misc
    int material;            // +0x08 $Default Material:
    const char *model;       // +0x0C $Model: "prop-sword-short.v3d"
    vim *mesh;               // +0x10 paged-in mesh: vim_load by item_page_in_mesh, NULL when paged out
    const char *animation;   // +0x14 +Animation:
    int value;               // +0x18 $Value:
    int slot;                // +0x1C +Slot: equipment slot
    void *skill_mods[4];     // +0x20 +Skill Mod:
    void *resist_mods[4];    // +0x30 +Resist Mod:
    void *stat_mods[2];      // +0x40 +Stat Mod:
    signed char status_effects[9]; // +0x48 +Effect:
    ubyte _pad51[3];
    float ap_regen;          // +0x54 +AP Regen:
    item_charge_effect *charge_effect; // +0x58 0 none
    ushort flags;            // +0x5C +Flags: bit 0 = identified
    short bitmap;            // +0x5E +Bitmap: bm_load handle
    ubyte _unk60[0x07];
    ubyte restricted;        // +0x67 +Restricted: 1 Joseph, 2 Rosalind, 4 Flece, 8 Jekhar
    // weapon fields (armour reuses +0x68..+0xE7 for per-character data)
    ushort damage;           // +0x68 $Damage: (every read is zero-extended)
    ushort weapon_flags;     // +0x6A weapon +Flags:
    int weapon_skill;        // +0x6C $Skill:
    float speed;             // +0x70 $Speed:
    float attack_radius;     // +0x74 $Attack Radius:
    float weight;            // +0x78 $Weight:
    int drop_tag1;           // +0x7C "$drop-ground01" mesh tag (weapons; -1 otherwise) (types.h _unk7c)
    int drop_tag2;           // +0x80 "$drop-ground02" (types.h _unk80)
    float render_distance;   // +0x84 +Render distance:
    int weapon_class;        // +0x88 $Class:
    int damage_type;         // +0x8C $Damage Type:
    int elemental_damage;    // +0x90 +Elemental Damage:
    void *weapon_triggered_effect; // +0x94
    ubyte _unmapped98[0x50];
    // armour fields
    float armour_speed;      // +0xE8 $Speed:
    float armour_weight;     // +0xEC $Weight:
    float protection;        // +0xF0 $Protection:
    unsigned int _unkf4;
    void *armour_triggered_effect; // +0xF8
    const char *description; // +0xFC +Description:
};
LAYOUT_CHECK(ItemType, sizeof(ItemType) == 0x100);

extern ItemType item_types[];         // 0x8cbec0
extern int num_item_types;            // 0x8ec7c0

// An item record: the equipped items (Character.equipment), the item objects (item_obj +0x98) and the
// party inventory entries point at one. The stubs called it item_info and inv_item (DECISIONS.md D16).
struct item_info {
    int _unk00;                       // +0x00
    ItemType *type;                   // +0x04
    int charges;                      // +0x08 (read by buysell_buy_item and mp_send_it_addi)
};
typedef item_info inv_item;

int  item_type_find(const char *name);                         // 0x44b450, index or -1
bool item_type_is_stackable(ItemType *type);                   // 0x44b500
bool character_meets_item_requirements(Character *c, ItemType *type);   // 0x44b550
bool item_type_is_identified(ItemType *type);                  // 0x44b6b0
void item_get_display_name(char *buf, ItemType *type);         // 0x44d8a0

// localize_strings slot tables of items.cpp (stored values are slot addresses).
extern char **damage_type_loc_names[7];   // 0x8ec9c4 (init_strptrs_8ec9c4)
extern char **resistance_level_names[6];  // 0x8b5098 (init_strptrs_8b5098)
extern char **status_effect_names[18];    // 0x8ec9e8 (init_strptrs_8ec9e8)

#endif
