// engine/objects/items.h - item types and item records (engine\objects\items.cpp, path references
// 0x44d56d-0x44d63f; code 0x449d90-0x44d9ee, pilot game/items.cpp). Not original source.
// Layout: symbols/types.h ItemType (fields +0x10 mesh and the unsigned +0x68 damage as in types.h
// 2026-10-15; +0x7c / +0x80 named after their use by item_paging.cpp; the rest from items_parse_table).
#ifndef ENGINE_OBJECTS_ITEMS_H
#define ENGINE_OBJECTS_ITEMS_H

#include "vsdk/vsdk.h"
#include "vsdk/ca/skeleton.h"   // vim
#include "vsdk/os/norlsallocpool.h"

class Character;

#define MAX_ITEM_TYPES   0x209        // item_types (items_parse_table, item_create)
#define MAX_ITEMS        1224         // item records 0x8b50c8..0x8b9d48 (16 bytes each)
#define MAX_MATERIALS    17           // materials.tbl records, material_names
#define NUM_HERO_MODELS  7            // hero_model_index: joseph, heavy joseph, ..., rosalind

// ItemType +0x5c flags (item_flag_names 0x599024, bit n = name n).
#define ITF_NOT_IDENTIFIED  0x01      // "not identified" (cleared once identified)
#define ITF_SHADOW          0x02      // "shadow"
#define ITF_NO_DESTROY      0x04      // "no destroy"
#define ITF_UNIDENTIFIED_TBL 0x08     // copy of bit 0 as read from the table (item_types_reset_identified)
#define ITF_STEALTH         0x10      // "+Stealth"
// Bits 5-7: the item drops only at that multiplayer difficulty (mp_game_difficulty 0 / 1 / 2;
// monster_drop_loot, decomp37). Names ours.
#define ITF_DIFFICULTY_EASY   0x20
#define ITF_DIFFICULTY_NORMAL 0x40
#define ITF_DIFFICULTY_HARD   0x80

// One +Skill Mod: / +Resist Mod: entry (8 bytes in items_alloc_pool; names ours).
struct item_mod {
    int id;                           // +0x0 skill_names / damage_type_names index
    float value;                      // +0x4 table value * 0.01
};
// One +Stat Mod: entry (8 bytes): 0 MaxHP, 1 MaxAP.
struct item_stat_mod {
    int stat;                         // +0x0
    int value;                        // +0x4
};
// +Triggered Effect: (0xc bytes; name ours).
struct item_triggered_effect {
    int spell;                        // +0x0 spell_find_by_name
    float chance;                     // +0x4 table value * 0.01
    int value;                        // +0x8 rest of the line (atoi), 0 when blank
};
LAYOUT_CHECK(item_triggered_effect, sizeof(item_triggered_effect) == 0xc);

// ItemType +0x58 (+Charge Effect: / +Self Only Charge Effect:), 0x18 bytes.
struct item_charge_effect {
    int spell;                        // +0x00
    int max_charges;                  // +0x04 (buysell_buy_item compares the owned item's charges with it)
    int _unk08;                       // +0x08 second integer of the line
    int _unk0c;                       // +0x0c rest of the line (atoi), -1 when blank
    ubyte self_only;                  // +0x10
    ubyte _pad11[3];
    float disposable;                 // +0x14 +Disposable: 0 without, 1.0, or ":" value * 0.01
};
LAYOUT_CHECK(item_charge_effect, sizeof(item_charge_effect) == 0x18);

// The weapon block at ItemType +0x68 (weapon_is_ranged and character_get_attack_range take its
// address). Name ours.
struct weapon_info {
    ushort damage;                    // +0x00 $Damage:
    ushort flags;                     // +0x02 +Flags: (weapon_flag_names) bit 0 ranged, bit 2 prop only
    int skill;                        // +0x04 $Skill:
    float speed;                      // +0x08 $Speed:
    float attack_radius;              // +0x0C $Attack Radius:
    float weight;                     // +0x10 $Weight: * 0.01
    int drop_tag1;                    // +0x14
    int drop_tag2;                    // +0x18
    float render_distance;            // +0x1C +Render distance: (1.0 default)
    int weapon_class;                 // +0x20 $Class:
    int damage_type;                  // +0x24 $Damage Type:
    int elemental_damage;             // +0x28 +Elemental Damage: (-1 none)
    item_triggered_effect *triggered_effect; // +0x2C
};
LAYOUT_CHECK_32(weapon_info, sizeof(weapon_info) == 0x30);

// An item type, shared by every item of that kind (items.tbl). item_types (0x8cbec0), stride 0x100.
struct ItemType {
    const char *name;        // +0x00 $Name: "Short Sword"
    int kind;                // +0x04 $Type: 0 Weapon, 1 Armor, 2 Misc
    int material;            // +0x08 $Default Material:
    const char *model;       // +0x0C $Model: "prop-sword-short.v3d"
    vim *mesh;               // +0x10 paged-in mesh: vim_load by item_page_in_mesh, NULL when paged out
    const char *animation;   // +0x14 +Animation: (weapons and shields only)
    int value;               // +0x18 $Value:
    int slot;                // +0x1C +Slot: equipment slot (item_slot_names; weapons default 4, else -1)
    item_mod *skill_mods[4]; // +0x20 +Skill Mod:
    item_mod *resist_mods[4];// +0x30 +Resist Mod:
    item_stat_mod *stat_mods[2]; // +0x40 +Stat Mod:
    signed char status_effects[9]; // +0x48 +Effect: by damage_type_to_status, -1 none
    ubyte _pad51[3];
    float ap_regen;          // +0x54 +AP Regen: * 0.01 (1.0 default)
    item_charge_effect *charge_effect; // +0x58 0 none
    ushort flags;            // +0x5C +Flags: ITF_* (bit 0 = NOT identified)
    short bitmap;            // +0x5E +Bitmap: bm_load handle
    ubyte render_mask[NUM_HERO_MODELS]; // +0x60 +RenderMask: per hero model, bit n-1 = mesh part n
    ubyte restricted;        // +0x67 +Restricted: 1 Joseph, 2 Rosalind, 4 Flece, 8 Jekhar (0xf default)
    // +0x68..+0xE7: the weapon block, or (armour) the per-hero texture names
    union {
        struct {
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
            item_triggered_effect *weapon_triggered_effect; // +0x94
        };
        weapon_info weapon;          // +0x68 the same fields as one block
        // Armour: [heavy][hero 0 Joseph, 1 Jekhar, 2 Flece, 3 Rosalind][part: 0 body, 1 cape / one
        // hand, 2 head, 3 leg] "Texture:" names (items_parse_table; names ours).
        char *PTR32 armour_textures[2][4][4];  // +0x68..+0xE7
    };
    // armour fields
    float armour_speed;      // +0xE8 $Speed:
    float armour_weight;     // +0xEC $Weight: * 0.01
    float protection;        // +0xF0 $Protection: * 0.01
    unsigned int _unkf4;
    item_triggered_effect *armour_triggered_effect; // +0xF8
    const char *description; // +0xFC +Description:
};
LAYOUT_CHECK_32(ItemType, sizeof(ItemType) == 0x100);

extern ItemType item_types[MAX_ITEM_TYPES];   // 0x8cbec0
extern int num_item_types;            // 0x8ec7c0

// An item record (16 bytes): the equipped items (Character.equipment), the item objects (item_obj +0x98)
// and the party inventory entries point at one. The stubs called it item_info and inv_item
// (DECISIONS.md D16). +0 is the free-list link (items_init_pool, item_free; item_alloc clears it).
struct item_info {
    item_info *next;                  // +0x00 item_free_list link
    ItemType *type;                   // +0x04
    int charges;                      // +0x08 (read by buysell_buy_item and mp_send_it_addi)
    int _unk0c;                       // +0x0C -1 from item_alloc
};
LAYOUT_CHECK_32(item_info, sizeof(item_info) == 0x10);
typedef item_info inv_item;

// materials.tbl (materials_parse_table): 0x14 bytes. Type name ours.
struct material {
    int id;                           // +0x00 index
    float damage_mult;                // +0x04 +Damage Modifier:
    float weight_mult;                // +0x08 +Weight Multiplier:
    float speed_mult;                 // +0x0C +Speed Multiplier:
    float protection_mult;            // +0x10 +Protection Multiplier:
};
LAYOUT_CHECK(material, sizeof(material) == 0x14);

// ---- items.cpp data ---------------------------------------------------------------------------------
extern const char *material_names[MAX_MATERIALS];  // 0x598f5c also parsed by characterinfo_parse_table
extern const char *anim_class_names[9];       // 0x598fa0
extern const char *damage_type_names[7];      // 0x598fc4
extern const char *item_stat_names[2];        // 0x598fe0 (label ours)
extern const char *item_slot_names[10];       // 0x598fe8 (label ours)
extern const char *weapon_flag_names[4];      // 0x599010 (label ours)
extern int num_weapon_flag_names;             // 0x599020 = 4 (label ours)
extern const char *item_flag_names[8];        // 0x599024 (label ours)
extern item_info *item_free_list;             // 0x8b50b8
extern item_info item_records[MAX_ITEMS];     // 0x8b50c8 (label ours)
extern material materials[MAX_MATERIALS];     // 0x8b9d48 (sum.symbols names +4, 0x8b9d4c)
extern unsigned int items_table_checksum;     // 0x8b9e9c
extern NoRlsAllocPool items_alloc_pool;       // 0x8b9ea0
extern char items_alloc_buffer[0x12000];      // 0x8b9ebc (label ours)
extern int items_num_allocated;               // 0x8cbebc
extern char ring_info_text[0x200];            // 0x8ec7c4

void items_init();                                             // 0x449f30
void materials_parse_table();                                  // 0x449f60
void items_parse_table();                                      // 0x44a090
void item_types_free();                                        // 0x44b200
void item_types_reset_identified();                            // 0x44b250
void items_init_pool();                                        // 0x44b280
void item_free(item_info *item);                               // 0x44b2b0
item_info *item_create(int type);                              // 0x44b2d0
item_info *item_create_by_name(const char *name);              // 0x44b320
item_info *item_create_weapon(int type);                       // 0x44b340
item_info *item_alloc();                                       // 0x44b380
item_info *item_create_armor(int type);                        // 0x44b3b0
item_info *item_create_misc(int type);                         // 0x44b3f0
item_info *item_create_misc_by_name(const char *name);         // 0x44b430
int  item_type_find(const char *name);                         // 0x44b450, index or -1
int  item_type_find_by_hash(int hash);                         // 0x44b4a0, index or -1
bool weapon_is_ranged(weapon_info *w);                         // 0x44b4f0
bool item_type_is_stackable(ItemType *type);                   // 0x44b500
bool character_can_equip_item(Character *c, item_info *item);  // 0x44b530
bool character_meets_item_requirements(Character *c, ItemType *type);   // 0x44b550
bool item_can_destroy(item_info *item);                        // 0x44b670
bool item_is_identified(item_info *item);                      // 0x44b690
bool item_type_is_identified(ItemType *type);                  // 0x44b6b0
bool item_has_magic_info(item_info *item);                     // 0x44b6d0
bool character_species_can_use_item(Character *c, item_info *item);     // 0x44b700
bool character_species_can_use_type(Character *c, ItemType *type);      // 0x44b720
char *ring_format_info(item_info *item);                       // 0x44b7a0, returns ring_info_text
void items_level_load();                                       // 0x44d540
void items_level_unload();                                     // 0x44d870
void item_get_display_name(char *buf, ItemType *type);         // 0x44d8a0
int  hero_model_index(const char *name);                       // 0x44d940, 0..6 or -1

// localize_strings slot tables of items.cpp (stored values are slot addresses).
extern char **damage_type_loc_names[7];   // 0x8ec9c4 (init_strptrs_8ec9c4)
extern char **ring_stat_names[2];         // 0x8ec9e0 (init_strptrs_8ec9e0)
extern char **status_effect_names[18];    // 0x8ec9e8 (init_strptrs_8ec9e8)
// characterinfo.cpp's table (init_strptrs_8b5098), declared here as before.
extern char **resistance_level_names[6];  // 0x8b5098

#endif
