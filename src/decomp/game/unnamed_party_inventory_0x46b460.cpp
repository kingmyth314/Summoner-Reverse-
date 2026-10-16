// unnamed_party_inventory_0x46b460.cpp - Sum.exe v1.40, code 0x46b460-0x46c5cd (the party inventory list,
// the shop list helpers, equipping and using items, the add_gold console command).
// FILE NAME NOT KNOWN (placeholder). No assert path, no strings.tbl block and no reference name points to a
// file; every function is module "-" in sum.symbols (the old items.cpp tags were removed by commit 04c1de1).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit: objects.cpp (unoptimised, ebp frames, 0xcc padding) ends with objects_update_frame (0x46b3d0); this
// code is optimised (0x90 padding), so a new object starts at 0x46b460. item_paging.cpp starts with its
// initialiser at 0x46c5d0. The add_gold console command (0x46c540-0x46c5cd) is the last thing here
// (D5201): its object 0x970810 is the lowest address of this block's .bss and it edits party_gold.
// Data: .data 0x59a638-0x59a74b (the servant disguise list, two unreferenced category tables, the
// literals; "Weapons" 0x597b3c, "Armor" 0x59946c and "Misc" 0x599474 are pooled from earlier units, /Gf),
// .bss 0x970810-0x97186f. 1.0f (0x57e990) and 0.0f (0x57eb84) are shared constants.
//
// The party inventory is an array of {count, item} entries. Stackable item types (items without a
// multi-charge or breakable charge effect, item_type_is_stackable) share one entry and count up; charged
// items of the same type merge their charges into the first one (capped at the maximum; the rest is lost).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vsdk/os/console.h"
#include "vsdk/math/vector.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/spells/spells.h"
#include "engine/effects/fx.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

#define MAX_PARTY_INVENTORY 0x209
#define CF_ALT_ARMOUR_TEXTURE 0x2000000      // +0x320 (cutscene record bit "alt_texture"; name ours)

// Shop categories (item_type_shop_category).
#define SHOP_CAT_WEAPONS 0
#define SHOP_CAT_ARMOR   1
#define SHOP_CAT_MAGIC   2
#define SHOP_CAT_JEWELRY 3
#define SHOP_CAT_MISC    4

// ItemType::slot values (item_slot_names).
#define ITEM_SLOT_NECK       0
#define ITEM_SLOT_FINGER     1
#define ITEM_SLOT_HANDS      2
#define ITEM_SLOT_FEET       3
#define ITEM_SLOT_WEAPON     4
#define ITEM_SLOT_SHIELD     5
#define ITEM_SLOT_TORSO      6
#define ITEM_SLOT_LEGS       7
#define ITEM_SLOT_HEAVY      8
#define ITEM_SLOT_TWO_HANDED 9

// ---- .data -------------------------------------------------------------------------------------------
// 0x59a63c: per equipment index, the item that the disguise needs there (gloves 3, boots 4, blouse 7,
// leggings 8). Name ours.
const char *servant_disguise_items[9] = {
    NULL, NULL, NULL, "Servant's Gloves", "Servant's Boots", NULL, NULL, "Servant's Blouse",
    "Servant's Leggings",
};
// 0x59a660 / 0x59a674: the shop category names and ids. Nothing in Sum.exe reads either (no reference to
// the addresses); kept because they are this unit's initialised data. Names ours.
const char *shop_category_names[5] = { "Weapons", "Armor", "Magic", "Jewelry", "Misc" };
int shop_category_ids[5] = { SHOP_CAT_WEAPONS, SHOP_CAT_ARMOR, SHOP_CAT_MAGIC, SHOP_CAT_JEWELRY,
                             SHOP_CAT_MISC };

// ---- .bss --------------------------------------------------------------------------------------------
party_inventory_entry party_inventory[MAX_PARTY_INVENTORY];   // 0x970820
int party_gold;                                               // 0x971868
int party_inventory_count;                                    // 0x97186c

// 0x46b460  Confidence: exact. The next entry after start whose item is in the category, -1 none.
int party_inventory_find_next_in_category(int start, int category, party_inventory_entry *list, int count)
{
    for (int i = start + 1; i < count; i++) {
        if (item_type_shop_category(list[i].item->type - item_types) == category)
            return i;
    }
    return -1;
}

// 0x46b4b0  Confidence: exact.
int shop_inventory_find_next_in_category(int start, int category, shop_item *list, int count)
{
    for (int i = start + 1; i < count; i++) {
        if (item_type_shop_category(list[i].item_type) == category)
            return i;
    }
    return -1;
}

// 0x46b500  Confidence: likely (VC6 schedules the second index computation around the first call; the
// order of the two locals is a guess). qsort comparator: category, then item type index; never 0.
int party_inventory_compare(const void *pa, const void *pb)
{
    int ta = ((const party_inventory_entry *)pa)->item->type - item_types;
    int tb = ((const party_inventory_entry *)pb)->item->type - item_types;
    int ca = item_type_shop_category(ta);
    int cb = item_type_shop_category(tb);

    if (ca == cb)
        return ta < tb ? -1 : 1;
    return ca < cb ? -1 : 1;
}

// 0x46b560  Confidence: exact.
int shop_inventory_compare(const void *pa, const void *pb)
{
    int ta = ((const shop_item *)pa)->item_type;
    int tb = ((const shop_item *)pb)->item_type;
    int ca = item_type_shop_category(ta);
    int cb = item_type_shop_category(tb);

    if (ca == cb)
        return ta < tb ? -1 : 1;
    return ca < cb ? -1 : 1;
}

// 0x46b5b0  Confidence: exact.
int party_inventory_find_first_in_category(int category, party_inventory_entry *list, int count)
{
    return party_inventory_find_next_in_category(-1, category, list, count);
}

// 0x46b5d0  Confidence: exact.
int shop_inventory_find_first_in_category(int category, shop_item *list, int count)
{
    return shop_inventory_find_next_in_category(-1, category, list, count);
}

// 0x46b5f0  Confidence: exact. Weapons, armour, charged items (magic), wearables (jewelry), the rest.
int item_type_shop_category(int type)
{
    if (type < 0 || type >= num_item_types)
        return -1;
    ItemType *t = &item_types[type];
    if (t->kind == 0)
        return SHOP_CAT_WEAPONS;
    if (t->kind == 1)
        return SHOP_CAT_ARMOR;
    if (t->charge_effect)
        return SHOP_CAT_MAGIC;
    return t->slot == -1 ? SHOP_CAT_MISC : SHOP_CAT_JEWELRY;
}

// 0x46b640  Confidence: exact.
void party_inventory_sort(party_inventory_entry *inv, int count)
{
    qsort(inv, count, sizeof(party_inventory_entry), party_inventory_compare);
}

// 0x46b660  Confidence: exact.
void shop_inventory_sort(shop_item *list, int count)
{
    qsort(list, count, sizeof(shop_item), shop_inventory_compare);
}

// 0x46b680  Confidence: exact. A new game starts with 30 gold.
void party_inventory_reset()
{
    party_gold = 30;
    party_inventory_count = 0;
}

// 0x46b6a0  Confidence: exact. Forgets the entries without freeing their items.
void party_inventory_zero_count()
{
    party_inventory_count = 0;
}

// 0x46b6b0  Confidence: likely (the shared tails: VC6 merged "it->item = NULL; deinit" of the full and the
// append paths). Takes an item object into the party: gold into party_gold, a known type merged into its
// entry, otherwise appended (or freed when the list is full). The object is deinitialised in every case.
// Only the append and full paths sort.
void item_obj_pickup(item_obj *it, int sort)
{
    int i;

    if (!it || it->count <= 0)
        return;
    if (it->level_item >= 0)
        savegame_state.level_item_remove(level_items[it->level_item].name);

    if (it->is_gold()) {
        party_gold += it->count;
        item_free(it->item);
        it->item = NULL;
        it->deinit();
        return;
    }

    for (i = 0; i < party_inventory_count; i++) {
        if (items_same_type(party_inventory[i].item, it->item))
            break;
    }
    if (i < party_inventory_count) {
        if (item_type_is_stackable(it->item->type)) {
            party_inventory[i].count += it->count;
        } else if (party_inventory[i].item->type->charge_effect) {
            party_inventory[i].item->charges += it->item->charges;
            party_inventory[i].item->charges = min(party_inventory[i].item->charges,
                                                   party_inventory[i].item->type->charge_effect->max_charges);
        }
        item_free(it->item);
        it->item = NULL;
        it->deinit();
        return;
    }

    if (party_inventory_count >= MAX_PARTY_INVENTORY) {
        item_free(it->item);
    } else {
        party_inventory[party_inventory_count].item = it->item;
        party_inventory[party_inventory_count].count = it->count;
        party_inventory_count++;
    }
    it->item = NULL;
    it->deinit();
    if (sort)
        party_inventory_sort(party_inventory, party_inventory_count);
}

// 0x46b870  Confidence: likely (as item_obj_pickup). The item is freed whenever it was merged.
void party_inventory_add_item(item_info *item, int count, int sort)
{
    int i;

    if (!item || count <= 0)
        return;
    for (i = 0; i < party_inventory_count; i++) {
        if (items_same_type(party_inventory[i].item, item))
            break;
    }
    if (i < party_inventory_count) {
        if (item_type_is_stackable(item->type)) {
            party_inventory[i].count += count;
            item_free(item);
            return;
        }
        if (party_inventory[i].item->type->charge_effect) {
            party_inventory[i].item->charges += item->charges;
            party_inventory[i].item->charges = min(party_inventory[i].item->charges,
                                                   party_inventory[i].item->type->charge_effect->max_charges);
        }
        item_free(item);
        return;
    }
    if (party_inventory_count >= MAX_PARTY_INVENTORY) {
        item_free(item);
        return;
    }
    party_inventory[party_inventory_count].item = item;
    party_inventory[party_inventory_count].count = count;
    party_inventory_count++;
    if (sort)
        party_inventory_sort(party_inventory, party_inventory_count);
}

// 0x46b980  Confidence: likely (the two clamps are written as the min() macro; the found test compares
// ItemType pointers, not items_same_type). Adds count of an item type; a type with more than one charge
// is always added once. charges < 0 means "full" (the charge effect's maximum).
void party_inventory_add_type(int type, int count, int charges, int sort)
{
    int i;

    if (type < 0 || type >= num_item_types)
        return;
    ItemType *t = &item_types[type];
    if (count <= 0)
        return;
    if (t->charge_effect && t->charge_effect->max_charges > 1)
        count = 1;

    for (i = 0; i < party_inventory_count; i++) {
        if (party_inventory[i].item->type == t)
            break;
    }
    if (i < party_inventory_count) {
        if (item_type_is_stackable(t)) {
            party_inventory[i].count += count;
            return;
        }
        item_info *item = party_inventory[i].item;
        item_charge_effect *ce = item->type->charge_effect;
        if (!ce)
            return;
        item->charges += charges >= 0 ? charges : ce->max_charges;
        party_inventory[i].item->charges = min(party_inventory[i].item->charges,
                                               party_inventory[i].item->type->charge_effect->max_charges);
        return;
    }

    if (party_inventory_count >= MAX_PARTY_INVENTORY)
        return;
    item_info *item = item_create(type);
    if (!item)
        return;
    party_inventory[party_inventory_count].item = item;
    party_inventory[party_inventory_count].count = count;
    if (item->type->charge_effect) {
        party_inventory[party_inventory_count].item->charges =
            charges >= 0 ? charges : item->type->charge_effect->max_charges;
        party_inventory[party_inventory_count].item->charges =
            min(party_inventory[party_inventory_count].item->charges, item->type->charge_effect->max_charges);
    } else {
        party_inventory[party_inventory_count].item->charges = 0;
    }
    party_inventory_count++;
    if (sort)
        party_inventory_sort(party_inventory, party_inventory_count);
}

// 0x46bb00  Confidence: exact.
void party_inventory_add_by_name(const char *name, int count, int charges, int sort)
{
    int type = item_type_find(name);
    if (type >= 0)
        party_inventory_add_type(type, count, charges, sort);
}

// 0x46bb30  Confidence: exact (free_item is a byte: `mov al, [esp + 8]`, D5203).
void party_inventory_remove(int slot, bool free_item, int sort)
{
    if (free_item)
        item_free(party_inventory[slot].item);
    if (slot + 1 < party_inventory_count)
        memmove(&party_inventory[slot], &party_inventory[slot + 1],
                (party_inventory_count - (slot + 1)) * sizeof(party_inventory_entry));
    party_inventory_count--;
    if (sort)
        party_inventory_sort(party_inventory, party_inventory_count);
}

// 0x46bbb0  Confidence: exact.
void party_inventory_clear()
{
    for (int i = 0; i < party_inventory_count; i++)
        item_free(party_inventory[i].item);
    party_inventory_count = 0;
}

// 0x46bc00  Confidence: likely (statement split of the two decrements: the count one uses the
// pre-decremented value, the charges one reloads). Casts the item's charge spell. A disposable value of
// 1.0 (or more) uses up a unit (stack) or a charge; a fraction is the chance that the item breaks on this
// use, otherwise one charge goes (the item stays at 0 charges, D5204).
bool character_use_inventory_item(Character *c, party_inventory_entry *entry, int target_handle,
                                  void *point, int arg)
{
    if (entry->item->charges <= 0 || !entry->item->type->charge_effect)
        return false;
    if (!character_species_can_use_item(c, entry->item))
        return false;

    spell_cast_execute(c, entry->item->type->charge_effect->spell, target_handle, (vector3 *)point, arg,
                       (int)entry->item->type, 5, 0);

    item_charge_effect *ce = entry->item->type->charge_effect;
    if (ce->disposable >= 1.0f) {
        if (item_type_is_stackable(entry->item->type)) {
            if (--entry->count > 0)
                return true;
        } else {
            entry->item->charges--;
            if (entry->item->charges > 0)
                return true;
        }
    } else if (ce->disposable <= 0.0f || !(frand() < entry->item->type->charge_effect->disposable)) {
        entry->item->charges--;
        return true;
    }
    party_inventory_remove(entry - party_inventory, true, 1);
    return true;
}

// 0x46bd00  Confidence: exact.
int party_inventory_find_type(ItemType *type)
{
    for (int i = 0; i < party_inventory_count; i++) {
        if (party_inventory[i].item->type == type)
            return i;
    }
    return -1;
}

// 0x46bd30  Confidence: exact.
bool character_has_equipped_type(Character *c, ItemType *type)
{
    for (int i = 0; i < 10; i++) {
        if (c->equipment[i] && c->equipment[i]->type == type)
            return true;
    }
    return false;
}

// 0x46bd60  Confidence: exact.
int party_inventory_find_type_index(int type)
{
    for (int i = 0; i < party_inventory_count; i++) {
        if (party_inventory[i].item->type - item_types == type)
            return i;
    }
    return -1;
}

// 0x46bda0  Confidence: exact.
void equipment_clear(item_info **equipment)
{
    if (equipment)
        memset(equipment, 0, 10 * sizeof(item_info *));
}

// 0x46bdc0  Confidence: exact.
void equipment_free(item_info **equipment)
{
    if (!equipment)
        return;
    for (int i = 0; i < 10; i++) {
        if (equipment[i]) {
            item_free(equipment[i]);
            equipment[i] = NULL;
        }
    }
}

// 0x46bdf0  Confidence: likely (the case bodies: VC6 shares the "*out_old2 = NULL" and the second
// unequip calls between cases; the two item_is_identified calls are in the binary, see D5205).
// Equipment indices: 0 neck, 1-2 rings, 3 hands, 4 feet, 5 weapon, 6 shield, 7 torso, 8 legs. A heavy
// suit sits in 7 and frees 8, a two-handed weapon sits in 5 and frees 6. The items taken off come back in
// *out_old1 / *out_old2. The main hero's model switches between the light and heavy meshes when a heavy
// suit goes on or a torso / legs piece replaces one; otherwise armour sets the underwear textures.
void character_equip_item(item_info **out_old1, item_info **out_old2, Character *c, int slot,
                          item_info *item)
{
    bool heavy;
    bool change_model;

    *out_old1 = NULL;
    *out_old2 = NULL;
    if (!item)
        return;
    if (c->type == 7 && (!item_is_identified(item) || !item_is_identified(item)))
        return;

    item_info *body = c->equipment[7];
    if (c->is_main_hero() && body && body->type->slot == ITEM_SLOT_HEAVY)
        heavy = true;
    else
        heavy = false;

    if (c->is_main_hero()) {
        int s = item->type->slot;
        if (((s == ITEM_SLOT_TORSO || s == ITEM_SLOT_LEGS) && heavy) || (s == ITEM_SLOT_HEAVY && !heavy))
            change_model = true;
        else
            change_model = false;
    } else {
        change_model = false;
    }

    switch (item->type->slot) {
    case ITEM_SLOT_NECK:
        *out_old1 = character_unequip_slot(c, 0, false);
        *out_old2 = NULL;
        break;
    case ITEM_SLOT_FINGER:
    case ITEM_SLOT_WEAPON:
        *out_old1 = character_unequip_slot(c, slot, false);
        *out_old2 = NULL;
        break;
    case ITEM_SLOT_HANDS:
        *out_old1 = character_unequip_slot(c, 3, false);
        *out_old2 = NULL;
        break;
    case ITEM_SLOT_FEET:
        *out_old1 = character_unequip_slot(c, 4, false);
        *out_old2 = NULL;
        break;
    case ITEM_SLOT_SHIELD:
        *out_old1 = character_unequip_slot(c, 6, false);
        if (c->equipment[5] && c->equipment[5]->type->slot == ITEM_SLOT_TWO_HANDED) {
            if (*out_old1)
                *out_old2 = character_unequip_slot(c, 5, false);
            else {
                *out_old1 = character_unequip_slot(c, 5, false);
                *out_old2 = NULL;
            }
        } else {
            *out_old2 = NULL;
        }
        break;
    case ITEM_SLOT_TORSO:
        *out_old1 = character_unequip_slot(c, 7, false);
        *out_old2 = NULL;
        break;
    case ITEM_SLOT_LEGS:
        *out_old1 = character_unequip_slot(c, 8, false);
        if (c->equipment[7] && c->equipment[7]->type->slot == ITEM_SLOT_HEAVY) {
            if (*out_old1)
                *out_old2 = character_unequip_slot(c, 7, false);
            else {
                *out_old1 = character_unequip_slot(c, 7, false);
                *out_old2 = NULL;
            }
        } else {
            *out_old2 = NULL;
        }
        break;
    case ITEM_SLOT_HEAVY:
        *out_old1 = character_unequip_slot(c, 7, false);
        if (*out_old1)
            *out_old2 = character_unequip_slot(c, 8, false);
        else {
            *out_old1 = character_unequip_slot(c, 8, false);
            *out_old2 = NULL;
        }
        break;
    case ITEM_SLOT_TWO_HANDED:
        *out_old1 = character_unequip_slot(c, 5, false);
        if (*out_old1)
            *out_old2 = character_unequip_slot(c, 6, false);
        else {
            *out_old1 = character_unequip_slot(c, 6, false);
            *out_old2 = NULL;
        }
        break;
    default:
        *out_old1 = NULL;
        *out_old2 = NULL;
        return;
    }

    c->equipment[slot] = item;

    if (c->is_main_hero()) {
        if (change_model) {
            switch (c->info->species) {
            case SPECIES_JOSEPH:
                c->apply_hero_model(!heavy);
                break;
            case SPECIES_JEKHAR:
                c->apply_hero_model(!heavy);
                break;
            case SPECIES_FLECE:
                c->apply_hero_model(!heavy);
                break;
            case SPECIES_ROSALIND:
                c->apply_hero_model(0);
                break;
            }
        } else if (item->type->kind == 1) {
            char **tex = NULL;
            switch (item->type->slot) {
            case ITEM_SLOT_HANDS:
            case ITEM_SLOT_FEET:
                tex = item->type->armour_textures[0][c->info->species];
                break;
            case ITEM_SLOT_TORSO:
            case ITEM_SLOT_LEGS:
            case ITEM_SLOT_HEAVY:
                tex = item->type->armour_textures[heavy][c->info->species];
                break;
            }
            switch (item->type->slot) {
            case ITEM_SLOT_HANDS:
                if (c->flags & CF_ALT_ARMOUR_TEXTURE) {
                    if (c->info->species == SPECIES_JOSEPH)
                        c->set_underwear_textures(5, tex[1]);
                    else
                        c->set_underwear_textures(5, tex[0]);
                } else {
                    c->set_underwear_textures(5, tex[0]);
                }
                break;
            case ITEM_SLOT_FEET:
                c->set_underwear_textures(4, tex[0]);
                break;
            case ITEM_SLOT_HEAVY:
                c->set_underwear_textures(3, tex[3]);
                // fall through
            case ITEM_SLOT_TORSO:
                c->set_underwear_textures(1, tex[0]);
                c->set_underwear_textures(0, tex[2]);
                c->set_underwear_textures(2, tex[1]);
                break;
            case ITEM_SLOT_LEGS:
                c->set_underwear_textures(3, tex[0]);
                break;
            }
        }
    }

    if (item->type->kind == 0)
        characterinfo_load_party_anims(c->info, item->type->weapon_class, true, false);

    if (cur_level_index != -1 && cur_level_index != 4) {
        for (int i = 0; i < 9; i++) {
            int level = item->type->status_effects[i];
            if (level >= 0) {
                if (level == 0)
                    level = c->level;
                c->status_effect_add_from_item(i, item, status_effect_modifier(i, level));
            }
        }
    }

    if (item->type->animation && c->type != 9)
        item->_unk0c = character_equip_item_fx(c->handle, item->type);
    else
        item->_unk0c = -1;
}

// 0x46c240  Confidence: likely (the Rosalind-to-Joseph species switch is four cases on one call, as
// in the jump table 0x46c39c). Takes the item off and returns it; HP / AP drop to the new maxima of the
// item's stat mods; the item's effect stops.
item_info *character_unequip_slot(Character *c, int slot, bool update_model)
{
    item_info *item = c->equipment[slot];
    c->equipment[slot] = NULL;
    if (!item)
        return NULL;

    if (update_model && c->is_main_hero()) {
        switch (item->type->slot) {
        case ITEM_SLOT_HANDS:
            c->set_underwear_textures(5, NULL);
            break;
        case ITEM_SLOT_FEET:
            c->set_underwear_textures(4, NULL);
            break;
        case ITEM_SLOT_TORSO:
            c->set_underwear_textures(1, NULL);
            c->set_underwear_textures(0, NULL);
            c->set_underwear_textures(2, NULL);
            break;
        case ITEM_SLOT_LEGS:
            c->set_underwear_textures(3, NULL);
            break;
        case ITEM_SLOT_HEAVY:
            switch (c->info->species) {
            case SPECIES_JOSEPH:
            case SPECIES_JEKHAR:
            case SPECIES_FLECE:
            case SPECIES_ROSALIND:
                c->apply_hero_model(0);
                break;
            }
            break;
        }
    }

    for (int i = 0; i < 2; i++) {
        item_stat_mod *m = item->type->stat_mods[i];
        if (m) {
            switch (m->stat) {
            case 0: {
                int max = c->get_max_stat(0);
                if (max < c->hp)
                    c->hp = (float)max;
                break;
            }
            case 1: {
                int max = c->get_max_stat(1);
                if (c->ap > max)
                    c->ap = (short)max;
                break;
            }
            }
        }
    }

    if (item->_unk0c != -1) {
        fx_stop(item->_unk0c);
        item->_unk0c = -1;
    }
    return item;
}

// 0x46c3b0  Confidence: exact.
int character_find_equipped_type(Character *c, ItemType *type)
{
    for (int i = 0; i < 10; i++) {
        if (c->equipment[i] && c->equipment[i]->type == type)
            return i;
    }
    return -1;
}

// 0x46c3e0  Confidence: exact.
int items_same_type(item_info *a, item_info *b)
{
    if (!a || !b)
        return 0;
    return a->type == b->type;
}

// 0x46c410  Confidence: likely (as character_use_inventory_item). An item that breaks or runs out is
// taken off with the model update and freed.
bool character_use_equipped_item(Character *c, int slot, int target_handle, void *point, int arg)
{
    item_info *item = c->equipment[slot];
    if (!item || item->charges <= 0 || !item->type->charge_effect)
        return false;

    spell_cast_execute(c, item->type->charge_effect->spell, target_handle, (vector3 *)point, arg,
                       (int)item->type, 5, 0);

    item_charge_effect *ce = item->type->charge_effect;
    if (ce->disposable >= 1.0f) {
        if (--item->charges > 0)
            return true;
    } else if (ce->disposable <= 0.0f || !(frand() < item->type->charge_effect->disposable)) {
        item->charges--;
        return true;
    }
    item_free(character_unequip_slot(c, slot, true));
    return true;
}

// 0x46c4e0  Confidence: exact. Every named slot must hold that item type (lpalaceint's disguise).
bool character_wearing_servant_disguise(Character *c)
{
    for (int i = 0; i < 9; i++) {
        if (servant_disguise_items[i]) {
            item_info *it = c->equipment[i];
            if (!it || it->type - item_types != item_type_find(servant_disguise_items[i]))
                return false;
        }
    }
    return true;
}

// ---- console command -----------------------------------------------------------------------------------
// 0x46c540 init_ccmd_add_gold_thunk (jmp) / 0x46c550 init_ccmd_add_gold: the compiler's dynamic
// initialiser pair of the object (static_initializers entry 204). Confidence: exact.
void cmd_add_gold();
console_command ccmd_add_gold("add_gold", "Adds gold to party inventory", cmd_add_gold);   // 0x970810

// 0x46c570  Confidence: exact. No check of the amount (a negative one subtracts).
void cmd_add_gold()
{
    if (calling_a_command_function) {
        process_command(ARG_INT);
        party_gold += command_argument_int;
        sprintf(string_buffer, "Added %d gold to party.", command_argument_int);
        print_to_console(string_buffer, 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: add_gold <amount of gold>", 0);
}
