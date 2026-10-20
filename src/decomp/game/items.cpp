// items.cpp - Sum.exe v1.40, code 0x449d90-0x44d9ee (Engine\Objects\items.cpp)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs; llvm-objdump with --start-address
// where the cached listing is out of step after a jump table: 0x44b200, 0x44b7a0). Target: VC++ 6.0
// (cl 12.00.8966), /O2 /Ob1 like the other optimised units (no frame pointer, 0x90 padding).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: sum.symbols tags 0x449d90 (init_strptrs_8ec9c4_thunk) to 0x44d940 (hero_model_index) with
// items.cpp. The previous function, cmd_levelchars (0x449c00, "levelchars.txt"), is characterinfo.cpp's
// last; the next, sinit_living_entity_8eca60 (0x44d9f0), is the unoptimised living_entity.cpp's head.
// item_obj_pickup and the party_inventory_* code (0x46b460-0x46c530) are a separate optimised unit after
// objects.cpp (its file name is not known; sum.symbols module "-"), so they are not written here.
//
// Data: .data 0x598f5c-0x599643 (the name tables, then this unit's literals: VC6 emits them in reverse
// order of first use, "multi hard only" first, "joseph" last); .bss 0x8b50b8-0x8eca2f. Literals pooled
// with earlier units (/Gf): "gold", "fire", "ice", "energy", "shield", "" (0x5bd0d0), ", ", "\n",
// "Joseph", "Jekhar", "Flece", "Rosalind", "jekhar", "flece", "rosalind", "Gold", "#", "$Skill:",
// "$Damage:", "$Attack Radius:", "$Size:", ":", the ring names and the *_ring_xp variable names.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "engine/objects/items.h"
#include "engine/objects/item_paging.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/spells/spells.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/norlsallocpool.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/parse/parse.h"
#include "vsdk/localize/localize.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/ca/skeleton.h"
#include "summoner/summoner.h"
#include "levelscripts/savegame.h"
#include "levelscripts/level_scripts.h"

// Original __FILE__ of this unit (literal 0x59954c).
#define ITEMS_CPP "D:\\projects\\Summoner\\pccode\\Engine\\Objects\\items.cpp"

// ---- .data (0x598f5c-0x599043; the literals follow) ----------------------------------------------
const char *material_names[MAX_MATERIALS] = {          // 0x598f5c
    "default", "flesh", "leather", "stone", "wood", "bone", "fire", "water", "gas", "ice",
    "copper", "gold", "bronze", "iron", "silver", "admantium", "volitionite",
};
const char *anim_class_names[9] = {                    // 0x598fa0
    "none", "swords", "large swords", "maces", "staves", "pole arms", "bows", "crossbows", "daggers",
};
const char *damage_type_names[7] = {                   // 0x598fc4
    "fire", "ice", "energy", "magic", "slashing", "piercing", "blunt",
};
const char *item_stat_names[2] = { "MaxHP", "MaxAP" }; // 0x598fe0 (label ours)
const char *item_slot_names[10] = {                    // 0x598fe8 (label ours)
    "neck", "finger", "hands", "feet", "weapon", "shield", "torso", "legs", "heavy", "two-handed",
};
const char *weapon_flag_names[4] = { "ranged", "no drop", "prop only", "magical" };   // 0x599010
// The weapon +Flags: parser reads the count from memory (push [0x599020]); the item +Flags: one uses
// the constant 8, so only this table has a count variable. Label ours.
int num_weapon_flag_names = sizeof(weapon_flag_names) / sizeof(weapon_flag_names[0]);  // 0x599020
const char *item_flag_names[8] = {                     // 0x599024 (bits of ItemType +0x5c)
    "not identified", "shadow", "no destroy", "", "", "multi easy only", "multi medium only",
    "multi hard only",
};

// ---- .bss (0x8b50b8-0x8eca2f), in address order ----------------------------------------------------
item_info *item_free_list;                    // 0x8b50b8
item_info item_records[MAX_ITEMS];            // 0x8b50c8 (0x8b50bc-0x8b50c7 unreferenced; label ours)
material materials[MAX_MATERIALS];            // 0x8b9d48
unsigned int items_table_checksum;            // 0x8b9e9c
// 0x449ef0 sinit_norlsallocpool_8b9ea0 (call init, jmp register; static_initializers entry 186). Confidence: exact.
// 0x449f00 init_norlsallocpool_8b9ea0 (NoRlsAllocPool()). Confidence: exact.
// 0x449f10 register_dtor_norlsallocpool_8b9ea0 (atexit(0x449f20)). Confidence: exact.
// 0x449f20 dtor_norlsallocpool_8b9ea0 (jmp NoRlsAllocPool::~NoRlsAllocPool). Confidence: exact.
NoRlsAllocPool items_alloc_pool;              // 0x8b9ea0
char items_alloc_buffer[0x12000];             // 0x8b9ebc (label ours)
int items_num_allocated;                      // 0x8cbebc
ItemType item_types[MAX_ITEM_TYPES];          // 0x8cbec0
int num_item_types;                           // 0x8ec7c0
char ring_info_text[0x200];                   // 0x8ec7c4
// localize_strings slot tables: dynamic initialisers because &localize_strings[id] is stored (one
// initialiser per table; the order of the stores is the element order).
// 0x449d90 init_strptrs_8ec9c4_thunk (jmp, static_initializers entry 183). Confidence: exact.
// 0x449da0 init_strptrs_8ec9c4 (damage_type_loc_names). Confidence: likely.
char **damage_type_loc_names[7] = {           // 0x8ec9c4
    &XSTR("fire", 106), &XSTR("ice", 108), &XSTR("energy", 102), &XSTR("magic", 250),
    &XSTR("slashing", 251), &XSTR("piercing", 252), &XSTR("blunt", 253),
};
// 0x449df0 init_strptrs_8ec9e0_thunk (jmp, entry 184). Confidence: exact.
// 0x449e00 init_strptrs_8ec9e0 (ring_stat_names). Confidence: likely.
char **ring_stat_names[2] = { &XSTR("MaxHP", 254), &XSTR("MaxAP", 255) };   // 0x8ec9e0
// 0x449e20 init_strptrs_8ec9e8_thunk (jmp, entry 185). Confidence: exact.
// 0x449e30 init_strptrs_8ec9e8 (status_effect_names). Confidence: likely.
char **status_effect_names[18] = {            // 0x8ec9e8
    &XSTR("Blind", 256), &XSTR("Silenced", 257), &XSTR("Faith", 258), &XSTR("Cursed", 259),
    &XSTR("Blessed", 260), &XSTR("Regenerating", 261), &XSTR("Empowered", 262), &XSTR("Light", 263),
    &XSTR("Invisible", 264), &XSTR("Sleeping", 265), &XSTR("Paralyzed", 266), &XSTR("Poisoned", 267),
    &XSTR("Arms attacked", 268), &XSTR("Legs attacked", 269), &XSTR("Firewall", 270),
    &XSTR("Blizzard", 271), &XSTR("Ice coffin", 272), &XSTR("Dark wall", 273),
};

// The table parsers copy every string into items_alloc_pool with strlen + 1 / strcpy, expanded inline
// each time (repne scasb / rep movsd). Written as this macro; whether the original had a helper is not
// visible. Name ours.
#define ITEMS_STRDUP(dest, src) { \
        char *_s = (char *)items_alloc_pool.Alloc(strlen(src) + 1); \
        strcpy(_s, src); \
        (dest) = _s; \
    }

// 0x449f30  Confidence: exact (the checksum store is scheduled before the Init call; tail call).
void items_init()
{
    items_table_checksum = 0;
    items_alloc_pool.Init(items_alloc_buffer, 0x12000);
    materials_parse_table();
    items_parse_table();
}

// 0x449f60  Confidence: exact. The record name after "$Material:" is not read (Expect only); the
// index is stored in +0.
void materials_parse_table()
{
    TextFile tf;
    int i;

    tf.Open("materials.tbl", 0x98967f);
    items_table_checksum += tf.Checksum();
    tf.Expect("#Materials");
    for (i = 0; i < MAX_MATERIALS; i++) {
        tf.Expect("$Material:");
        materials[i].id = i;
        tf.Expect("+Damage Modifier:");
        materials[i].damage_mult = tf.ParseFloat();
        tf.Expect("+Weight Multiplier:");
        materials[i].weight_mult = tf.ParseFloat();
        tf.Expect("+Speed Multiplier:");
        materials[i].speed_mult = tf.ParseFloat();
        tf.Expect("+Protection Multiplier:");
        materials[i].protection_mult = tf.ParseFloat();
    }
    tf.Expect("#End");
    tf.Close();
}

// 0x44a090  Confidence: likely (m-1020, D11104: 888 -> 29 differing lines; left: the register choice of the
// four armour-texture case blocks; control flow and stores checked block by block; the order of a few
// independent stores is the scheduler's). Notes:
// - The language section "#<language>" (lower case) is found with SkipTo and then Expect-ed.
// - Past MAX_ITEM_TYPES records the parser keeps reading into a stack ItemType (esp+0x310).
// - The +Skill Mod: / +Resist Mod: / +Stat Mod: arrays are cleared from the count on (rep stosd).
// - Armour "+<Hero> [Heavy] [Head|One Hand|Cape|Leg] Texture:" lines fill armour_textures
//   [heavy][hero][part]; for slots 2 and 3 the light body / cape names are copied to the heavy row.
// - A weapon with the "prop only" flag (bit 2) skips +Description: and +RenderMask: (`continue`
//   jumps to the loop test).
// - The two 0.01 multipliers are separate constants (0x5818ec for the integer mods, 0x5818f0 for
//   $Weight / $Protection); both are 0x3c23d70a.
void items_parse_table()
{
    TextFile tf;
    char lang[0x40];
    char buf[0x200];
    int list[8];
    char model[0x40];
    ItemType dummy;
    ItemType *t;
    int i, n;
    char *p;

    tf.Open("items.tbl", 0x98967f);
    items_table_checksum += tf.Checksum();
    strcpy(lang, "#");
    strcat(lang, localize_get_language_name());
    if (lang[0])
        for (p = lang; *p; p++)
            *p = tolower(*p);
    tf.Rewind();
    tf.SkipTo(lang, NULL);
    tf.Expect(lang);
    tf.Expect("#Items");
    while (!tf.Accept("#End")) {
        if (num_item_types >= MAX_ITEM_TYPES)
            t = &dummy;
        else
            t = &item_types[num_item_types++];
        t->flags = 0;
        tf.Expect("$Name:");
        tf.ParseString(buf, 0x200, '"', '"');
        ITEMS_STRDUP(t->name, buf);
        memset(t->render_mask, 0, sizeof(t->render_mask));

        tf.Expect("$Type:");
        tf.ParseString(buf, 0x200, '"', '"');
        if (!_stricmp(buf, "Misc"))
            t->kind = 2;
        else if (!_stricmp(buf, "Armor"))
            t->kind = 1;
        else if (!_stricmp(buf, "Weapon"))
            t->kind = 0;

        if (tf.Accept("+Slot:"))
            t->slot = tf.ParseTypeString(item_slot_names, 10);
        else if (t->kind == 0)
            t->slot = 4;
        else
            t->slot = -1;

        if (tf.Accept("+Flags:")) {
            do {
                t->flags |= 1 << tf.ParseTypeString(item_flag_names, 8);
            } while (!tf.Peek("$Value:"));
        }
        if (t->flags & ITF_NOT_IDENTIFIED)
            t->flags |= ITF_UNIDENTIFIED_TBL;

        tf.Expect("$Value:");
        t->value = tf.ParseInt();
        tf.Expect("$Icon:");
        tf.ParseString(buf, 0x200, '"', '"');
        tf.Expect("$Model:");
        tf.ParseString(buf, 0x200, '"', '"');
        ITEMS_STRDUP(t->model, buf);
        t->mesh = NULL;
        t->animation = NULL;
        if (tf.Accept("+Animation:")) {
            tf.ParseString(buf, 0x200, '"', '"');
            if (t->kind == 0 || (t->kind == 1 && t->slot == 5))
                ITEMS_STRDUP(t->animation, buf);
        }
        if (tf.Accept("+Bitmap:")) {
            tf.ParseString(buf, 0x200, '"', '"');
            t->bitmap = bm_load(buf, -1, -1);
        }

        n = 0;
        if (tf.Accept("+Skill Mod:")) {
            do {
                int value;
                tf.ParseString(buf, 0x200, '"', '"');
                for (i = 0; i < 36; i++) {
                    if (!_stricmp(skill_names[i], buf))
                        break;
                }
                value = tf.ParseInt();
                float f = value * 0.01f;
                t->skill_mods[n] = (item_mod *)items_alloc_pool.Alloc(sizeof(item_mod));
                t->skill_mods[n]->id = i;
                t->skill_mods[n]->value = f;
                n++;
            } while (tf.Accept("+Skill Mod:"));
        }
        for (; n < 4; n++)
            t->skill_mods[n] = NULL;

        n = 0;
        if (tf.Accept("+Resist Mod:")) {
            do {
                int type = tf.ParseTypeString(damage_type_names, 7);
                int value = tf.ParseInt();
                float f = value * 0.01f;
                t->resist_mods[n] = (item_mod *)items_alloc_pool.Alloc(sizeof(item_mod));
                t->resist_mods[n]->id = type;
                t->resist_mods[n]->value = f;
                n++;
            } while (tf.Accept("+Resist Mod:"));
        }
        for (; n < 4; n++)
            t->resist_mods[n] = NULL;

        n = 0;
        if (tf.Accept("+Stat Mod:")) {
            do {
                int stat = tf.ParseTypeString(item_stat_names, 2);
                int value = tf.ParseInt();
                t->stat_mods[n] = (item_stat_mod *)items_alloc_pool.Alloc(sizeof(item_stat_mod));
                t->stat_mods[n]->stat = stat;
                t->stat_mods[n]->value = value;
                n++;
            } while (tf.Accept("+Stat Mod:"));
        }
        for (; n < 2; n++)
            t->stat_mods[n] = NULL;

        for (i = 0; i < 9; i++)
            t->status_effects[i] = -1;
        t->flags &= ~ITF_STEALTH;
        t->ap_regen = 1.0f;
        for (;;) {
            if (tf.Accept("+AP Regen:")) {
                t->ap_regen = tf.ParseInt() * 0.01f;
            } else if (tf.Accept("+Stealth")) {
                t->flags |= ITF_STEALTH;
            } else if (tf.Accept("+Effect:")) {
                int spell, value;
                tf.ParseString(buf, 0x200, '"', '"');
                spell = spell_find_by_name(buf);
                value = 0;
                tf.ReadLine(buf, 0x200);
                if (!is_blank(buf))
                    value = atoi(buf);
                t->status_effects[damage_type_to_status(spell)] = (signed char)value;
            } else {
                break;
            }
        }

        {
            bool self_only;
            bool charge = true;
            if (tf.Accept("+Charge Effect:"))
                self_only = false;
            else if (tf.Accept("+Self Only Charge Effect:"))
                self_only = true;
            else
                charge = false;
            if (charge) {
                int spell, max_charges, b, c;
                float disposable;
                tf.ParseString(buf, 0x200, '"', '"');
                spell = spell_find_by_name(buf);
                max_charges = tf.ParseInt();
                b = tf.ParseInt();
                c = -1;
                tf.ReadLine(buf, 0x200);
                if (!is_blank(buf))
                    c = atoi(buf);
                disposable = 0.0f;
                if (tf.Accept("+Disposable")) {
                    if (tf.Accept(":"))
                        disposable = tf.ParseInt() * 0.01f;
                    else
                        disposable = 1.0f;
                }
                t->charge_effect = (item_charge_effect *)items_alloc_pool.Alloc(sizeof(item_charge_effect));
                t->charge_effect->spell = spell;
                t->charge_effect->max_charges = max_charges;
                t->charge_effect->_unk08 = b;
                t->charge_effect->_unk0c = c;
                t->charge_effect->self_only = self_only;
                t->charge_effect->disposable = disposable;
            } else {
                t->charge_effect = NULL;
            }
        }

        if (tf.Accept("+Restricted:")) {
            t->restricted = 0;
            for (;;) {
                if (tf.Accept("Joseph"))
                    t->restricted |= 1;
                else if (tf.Accept("Flece"))
                    t->restricted |= 4;
                else if (tf.Accept("Rosalind"))
                    t->restricted |= 2;
                else if (tf.Accept("Jekhar"))
                    t->restricted |= 8;
                else
                    break;
            }
        } else {
            t->restricted = 0xf;
        }

        switch (t->kind) {
        case 1:
            for (i = 0; i < 32; i++)
                (&t->armour_textures[0][0][0])[i] = NULL;
            for (;;) {
                int hero, heavy, part;
                bool ok;
                char **tex;

                if (tf.Accept("+Joseph")) {
                    hero = 0;
                    ok = t->restricted & 1;
                } else if (tf.Accept("+Flece")) {
                    hero = 2;
                    ok = (t->restricted >> 2) & 1;
                } else if (tf.Accept("+Jekhar")) {
                    hero = 1;
                    ok = (t->restricted >> 3) & 1;
                } else if (tf.Accept("+Rosalind")) {
                    hero = 3;
                    ok = (t->restricted >> 1) & 1;
                } else {
                    break;
                }
                heavy = (tf.Accept("Heavy") || t->slot == 8) ? 1 : 0;
                switch (t->slot) {
                case 6:
                case 7:
                case 8:
                    ok = ok && (!heavy || hero != 3);
                    break;
                case 2:
                case 3:
                    ok = ok && !heavy;
                    break;
                default:
                    ok = false;
                    break;
                }
                if (tf.Accept("Head")) {
                    part = 0;
                    ok = ok && (t->slot == 6 || t->slot == 8);
                } else if (tf.Accept("One Hand")) {
                    part = 5;
                    ok = ok && t->slot == 2;
                } else if (tf.Accept("Cape")) {
                    part = 2;
                    ok = ok && (t->slot == 6 || t->slot == 8);
                } else if (tf.Accept("Leg")) {
                    part = 3;
                    ok = ok && t->slot == 8;
                } else {
                    part = -1;
                }
                tf.Expect("Texture:");
                tf.ParseString(buf, 0x200, '"', '"');
                if (!ok)
                    continue;
                tex = NULL;
                switch (part) {
                case -1:
                    tex = &t->armour_textures[heavy][hero][0];
                    break;
                case 0:
                    tex = &t->armour_textures[heavy][hero][2];
                    break;
                case 2:
                case 5:
                    tex = &t->armour_textures[heavy][hero][1];
                    break;
                case 3:
                    tex = &t->armour_textures[heavy][hero][3];
                    break;
                }
                if (tex) {
                    *tex = (char *)items_alloc_pool.Alloc(strlen(buf) + 1);
                    strcpy(*tex, buf);
                    if (t->slot == 2 || t->slot == 3) {
                        t->armour_textures[1][hero][0] = t->armour_textures[0][hero][0];
                        t->armour_textures[1][hero][1] = t->armour_textures[0][hero][1];
                    }
                }
            }
            tf.Expect("$Speed:");
            t->armour_speed = tf.ParseFloat();
            tf.Expect("$Weight:");
            t->armour_weight = tf.ParseFloat() * 0.01f;
            tf.Expect("$Protection:");
            t->protection = tf.ParseFloat() * 0.01f;
            tf.Expect("$Default Material:");
            t->material = tf.ParseTypeString(material_names, MAX_MATERIALS);
            if (tf.Accept("+Triggered Effect:")) {
                int spell, value;
                float chance;
                tf.ParseString(buf, 0x200, '"', '"');
                spell = spell_find_by_name(buf);
                chance = tf.ParseInt() * 0.01f;
                value = 0;
                tf.ReadLine(buf, 0x200);
                if (!is_blank(buf))
                    value = atoi(buf);
                t->armour_triggered_effect =
                    (item_triggered_effect *)items_alloc_pool.Alloc(sizeof(item_triggered_effect));
                t->armour_triggered_effect->spell = spell;
                t->armour_triggered_effect->chance = chance;
                t->armour_triggered_effect->value = value;
            } else {
                t->armour_triggered_effect = NULL;
            }
            break;

        case 0:
            t->drop_tag1 = -1;
            t->drop_tag2 = -1;
            if (tf.Accept("+Render distance:"))
                t->render_distance = tf.ParseFloat();
            else
                t->render_distance = 1.0f;
            t->weapon_flags = 0;
            if (tf.Accept("+Flags:")) {
                do {
                    t->weapon_flags |= 1 << tf.ParseTypeString(weapon_flag_names, num_weapon_flag_names);
                } while (!tf.Peek("$Skill:") && !(t->weapon_flags & 4));
            }
            if (t->weapon_flags & 4)
                continue;
            tf.Expect("$Skill:");
            tf.ParseString(buf, 0x200, '"', '"');
            for (i = 0; i < 36; i++) {
                if (!_stricmp(skill_names[i], buf))
                    break;
            }
            t->weapon_skill = i;
            tf.Expect("$Class:");
            t->weapon_class = tf.ParseTypeString(anim_class_names, 9);
            tf.Expect("$Damage:");
            t->damage = (ushort)tf.ParseInt();
            tf.Expect("$Damage Type:");
            t->damage_type = tf.ParseTypeString(damage_type_names, 7);
            if (tf.Accept("+Elemental Damage:"))
                t->elemental_damage = tf.ParseTypeString(damage_type_names, 7);
            else
                t->elemental_damage = -1;
            tf.Expect("$Speed:");
            t->speed = tf.ParseFloat();
            if (tf.Accept("$Size:"))
                tf.ParseString(buf, 0x200, '"', '"');
            tf.Expect("$Attack Radius:");
            t->attack_radius = tf.ParseFloat();
            tf.Expect("$Weight:");
            t->weight = tf.ParseFloat() * 0.01f;
            tf.Expect("$Default Material:");
            t->material = tf.ParseTypeString(material_names, MAX_MATERIALS);
            if (tf.Accept("+Triggered Effect:")) {
                int spell, value;
                float chance;
                tf.ParseString(buf, 0x200, '"', '"');
                spell = spell_find_by_name(buf);
                chance = tf.ParseInt() * 0.01f;
                value = 0;
                tf.ReadLine(buf, 0x200);
                if (!is_blank(buf))
                    value = atoi(buf);
                t->weapon_triggered_effect =
                    (item_triggered_effect *)items_alloc_pool.Alloc(sizeof(item_triggered_effect));
                t->weapon_triggered_effect->spell = spell;
                t->weapon_triggered_effect->chance = chance;
                t->weapon_triggered_effect->value = value;
            } else {
                t->weapon_triggered_effect = NULL;
            }
            break;
        }

        t->description = NULL;
        if (tf.Accept("+Description:")) {
            tf.ParseString(buf, 0x200, '"', '"');
            localize_remap_charset(buf, 1);
            t->description = (const char *)items_alloc_pool.Alloc(strlen(buf) + 1);
            strcpy((char *)t->description, buf);
        }
        while (tf.Accept("+RenderMask:")) {
            int model_index, count;
            tf.ParseString(model, 0x40, '"', '"');
            model_index = hero_model_index(model);
            count = tf.ParseIntList(list, 8, 0);
            if (model_index >= 0) {
                t->render_mask[model_index] = 0;
                for (i = 0; i < count; i++)
                    t->render_mask[model_index] |= 1 << (list[i] - 1);
            }
        }
    }
    tf.Close();
}

// 0x44b200  Confidence: exact.
void item_types_free()
{
    int i;

    for (i = 0; i < num_item_types; i++) {
        if (item_types[i].mesh) {
            vim_free(item_types[i].mesh);
            item_types[i].mesh = NULL;
        }
    }
    items_alloc_pool.Reset();
}

// 0x44b250  Confidence: exact (new game: every type is unidentified again as in the table).
void item_types_reset_identified()
{
    int i;

    for (i = 0; i < num_item_types; i++) {
        if (item_types[i].flags & ITF_UNIDENTIFIED_TBL)
            item_types[i].flags |= ITF_NOT_IDENTIFIED;
        else
            item_types[i].flags &= ~ITF_NOT_IDENTIFIED;
    }
}

// 0x44b280  Confidence: exact (VC6 byte match, match11; was likely) (the list head is kept in ECX and stored once after the loop, so the
// records end up in reverse order: the last record is the first free one).
void items_init_pool()
{
    int i;

    item_free_list = NULL;
    for (i = 0; i < MAX_ITEMS; i++) {
        item_records[i].next = item_free_list;
        item_free_list = &item_records[i];
    }
    items_num_allocated = 0;
}

// 0x44b2b0  Confidence: exact.
void item_free(item_info *item)
{
    item->next = item_free_list;
    item_free_list = item;
    items_num_allocated--;
}

// 0x44b2d0  Confidence: exact (D10400: `it = NULL` after the range test; the zeroed EAX is both the switch base
// and the NULL returned for other kinds).
item_info *item_create(int type)
{
    item_info *it;

    if (type < 0 || type >= MAX_ITEM_TYPES)
        return NULL;
    it = NULL;
    switch (item_types[type].kind) {
    case 0:
        it = item_create_weapon(type);
        break;
    case 1:
        it = item_create_armor(type);
        break;
    case 2:
        it = item_create_misc(type);
        break;
    }
    return it;
}

// 0x44b320  Confidence: exact.
item_info *item_create_by_name(const char *name)
{
    int type = item_type_find(name);

    if (type == -1)
        return NULL;
    return item_create(type);
}

// 0x44b340  Confidence: exact. Armour and misc items (0x44b3b0, 0x44b3f0) are the same code.
item_info *item_create_weapon(int type)
{
    item_info *item;

    if (type < 0 || type >= num_item_types)
        return NULL;
    item = item_alloc();
    item->type = &item_types[type];
    if (item->type->charge_effect)
        item->charges = item->type->charge_effect->max_charges;
    else
        item->charges = 0;
    return item;
}

// 0x44b380  Confidence: exact (no check for an empty free list).
item_info *item_alloc()
{
    item_info *item = item_free_list;

    item_free_list = item->next;
    item->next = NULL;
    item->_unk0c = -1;
    items_num_allocated++;
    return item;
}

// 0x44b3b0  Confidence: exact.
item_info *item_create_armor(int type)
{
    item_info *item;

    if (type < 0 || type >= num_item_types)
        return NULL;
    item = item_alloc();
    item->type = &item_types[type];
    if (item->type->charge_effect)
        item->charges = item->type->charge_effect->max_charges;
    else
        item->charges = 0;
    return item;
}

// 0x44b3f0  Confidence: exact.
item_info *item_create_misc(int type)
{
    item_info *item;

    if (type < 0 || type >= num_item_types)
        return NULL;
    item = item_alloc();
    item->type = &item_types[type];
    if (item->type->charge_effect)
        item->charges = item->type->charge_effect->max_charges;
    else
        item->charges = 0;
    return item;
}

// 0x44b430  Confidence: exact.
item_info *item_create_misc_by_name(const char *name)
{
    int type = item_type_find(name);

    if (type == -1)
        return NULL;
    return item_create_misc(type);
}

// 0x44b450  Confidence: exact (num_item_types is re-read every pass).
int item_type_find(const char *name)
{
    int i;

    for (i = 0; i < num_item_types; i++) {
        if (!_stricmp(name, item_types[i].name))
            return i;
    }
    return -1;
}

// 0x44b4a0  Confidence: exact.
int item_type_find_by_hash(int hash)
{
    int i;

    for (i = 0; i < num_item_types; i++) {
        if (name_hash_nocase(item_types[i].name) == hash)
            return i;
    }
    return -1;
}

// 0x44b4f0  Confidence: exact (callers pass &type->weapon, ItemType +0x68).
bool weapon_is_ranged(weapon_info *w)
{
    bool r = w->flags & 1;
    return r;
}

// 0x44b500  Confidence: exact. 0x57e990 = 1.0f.
bool item_type_is_stackable(ItemType *type)
{
    item_charge_effect *ce = type->charge_effect;

    if (ce && (ce->max_charges > 1 || ce->disposable < 1.0f))
        return false;
    return true;
}

// 0x44b530  Confidence: exact.
bool character_can_equip_item(Character *c, item_info *item)
{
    return character_meets_item_requirements(c, item->type);
}

// 0x44b550  Confidence: exact (D10400: the non-party early return). Skill 8 is Heavy Arms; party members (type 7
// without flag 0x100) also need the species, the weapon class bit and the weapon skill.
bool character_meets_item_requirements(Character *c, ItemType *type)
{
    if (!item_type_is_identified(type))
        return false;
    if (type->slot == -1)
        return false;
    if (c->type != 7 || (c->flags & 0x100))
        return true;
    if (!character_species_can_use_type(c, type))
        return false;
    switch (type->kind) {
    case 1:
        if (type->armour_weight > 0.0f) {
            if (!c->skill_allowed(8) || c->get_skill(8) < type->armour_weight)
                return false;
        }
        break;
    case 0:
        if (!(c->info->anim_class_mask & (1 << type->weapon_class)))
            return false;
        if (c->get_skill(type->weapon_skill) <= 0.0f)
            return false;
        if (type->weight > 0.0f) {
            if (!c->skill_allowed(8) || c->get_skill(8) < type->weight)
                return false;
        }
        break;
    }
    return true;
}

// 0x44b670  Confidence: exact.
bool item_can_destroy(item_info *item)
{
    return !(item->type->flags & ITF_NO_DESTROY);
}

// 0x44b690  Confidence: exact (game_flags bit 1, multiplayer, shows every item identified).
bool item_is_identified(item_info *item)
{
    if (game_flags & GF_MULTIPLAYER)
        return true;
    return !(item->type->flags & ITF_NOT_IDENTIFIED);
}

// 0x44b6b0  Confidence: exact.
bool item_type_is_identified(ItemType *type)
{
    if (game_flags & GF_MULTIPLAYER)
        return true;
    return !(type->flags & ITF_NOT_IDENTIFIED);
}

// 0x44b6d0  Confidence: exact.
bool item_has_magic_info(item_info *item)
{
    if (!item_is_identified(item))
        return false;
    return item->type->charge_effect ? true : false;
}

// 0x44b700  Confidence: exact.
bool character_species_can_use_item(Character *c, item_info *item)
{
    return character_species_can_use_type(c, item->type);
}

// 0x44b720  Confidence: exact. Jump table 0x44b78c (species 0..3). Joseph with flag 0x2000000 cannot
// use slots 5 (shield) and 9 (two-handed).
bool character_species_can_use_type(Character *c, ItemType *type)
{
    switch (c->info->species) {
    case 0:
        if (!(type->restricted & 1))
            return false;
        if ((c->flags & 0x2000000) && (type->slot == 5 || type->slot == 9))
            return false;
        break;
    case 1:
        if (!(type->restricted & 8))
            return false;
        break;
    case 2:
        if (!(type->restricted & 4))
            return false;
        break;
    case 3:
        if (!(type->restricted & 2))
            return false;
        break;
    }
    return true;
}

// The "+N / -N" rounding of ring_format_info: x * 10 rounded to an int, converted to float and back
// again (fadd 0.5 / _ftol / fild / _ftol; the negative branch uses 0.5 - x * 10). Name ours; a macro so
// that the code stays inline.
#define RING_ROUND10(x) ((int)(float)(int)((x) * 10.0f + 0.5f))
#define RING_ROUND10_NEG(x) ((int)(float)(int)(0.5f - (x) * 10.0f))

// 0x44b7a0  Confidence: exact (D14000: the speed tests as `!= 0` with the 1.00 text in the else, the
// multiplayer skill remap as a switch, the resist and charge / effect format strings hoisted into locals,
// two flags in the weapon requirement (comma, has), block-scoped values in the stat lines). Builds the
// item info popup text in ring_info_text (name from sum.symbols; it formats every item kind, not only
// rings). widths are the popup text widths per resolution (0x124, 0x16d, 0x1d3). The buffers: text at
// esp+0x20 (also the status / "Equippable by" line), lens / offs at 0x120 / 0x160, num / req at 0x1a0.
char *ring_format_info(item_info *item)
{
    int widths[3];
    char text[0x100];
    int lens[16];
    int offs[16];
    int i, n;

    ring_info_text[0] = 0;
    widths[0] = 0x124;
    widths[1] = 0x16d;
    widths[2] = 0x1d3;

    switch (item->type->kind) {
    case 2: {
        char num[0x80];
        if (!_stricmp(item->type->name, "Ring of Darkness")) {
            sprintf(text, XSTR("Summoning Ring\n", 281));
            strcat(ring_info_text, text);
            strcat(ring_info_text, XSTR("Ring Power: ", 276));
            int_to_string_with_commas(num, 0x7f, savegame_state.get_int(4, "dark_ring_xp"));
            strcat(ring_info_text, num);
            strcat(ring_info_text, "\n");
            sprintf(text, XSTR("Available Summons:\n", 282));
            strcat(ring_info_text, text);
            sprintf(text, XSTR(" Blackfire Elemental\n", 283));
            strcat(ring_info_text, text);
            if (savegame_state.get_flag(0x22, "got_laharah_spirit")) {
                sprintf(text, XSTR(" Spirit of Laharah\n", 284));
                strcat(ring_info_text, text);
            }
        } else if (!_stricmp(item->type->name, "Ring of Light")) {
            sprintf(text, XSTR("Summoning Ring\n", 281));
            strcat(ring_info_text, text);
            strcat(ring_info_text, XSTR("Ring Power: ", 276));
            int_to_string_with_commas(num, 0x7f, savegame_state.get_int(4, "light_ring_xp"));
            strcat(ring_info_text, num);
            strcat(ring_info_text, "\n");
            sprintf(text, XSTR("Available Summons:\n", 282));
            strcat(ring_info_text, text);
            sprintf(text, XSTR(" Mist Elemental\n", 285));
            strcat(ring_info_text, text);
            if (savegame_state.get_int(4, "light_ring_xp") >= 60000) {
                sprintf(text, XSTR(" Wraith\n", 286));
                strcat(ring_info_text, text);
            }
        } else if (!_stricmp(item->type->name, "Ring of Fire")) {
            sprintf(text, XSTR("Summoning Ring\n", 281));
            strcat(ring_info_text, text);
            strcat(ring_info_text, XSTR("Ring Power: ", 276));
            int_to_string_with_commas(num, 0x7f, savegame_state.get_int(4, "fire_ring_xp"));
            strcat(ring_info_text, num);
            strcat(ring_info_text, "\n");
            sprintf(text, XSTR("Available Summons:\n", 282));
            strcat(ring_info_text, text);
            sprintf(text, XSTR(" Red Minotaur\n", 287));
            strcat(ring_info_text, text);
            if (savegame_state.get_int(4, "fire_ring_xp") >= 60000) {
                sprintf(text, XSTR(" Blood Elemental\n", 288));
                strcat(ring_info_text, text);
            }
        } else if (!_stricmp(item->type->name, "Ring of Stone")) {
            sprintf(text, XSTR("Summoning Ring\n", 281));
            strcat(ring_info_text, text);
            strcat(ring_info_text, XSTR("Ring Power: ", 276));
            int_to_string_with_commas(num, 0x7f, savegame_state.get_int(4, "stone_ring_xp"));
            strcat(ring_info_text, num);
            strcat(ring_info_text, "\n");
            sprintf(text, XSTR("Available Summons:\n", 282));
            strcat(ring_info_text, text);
            sprintf(text, XSTR(" Stone Golem\n", 289));
            strcat(ring_info_text, text);
            if (savegame_state.get_int(4, "stone_ring_xp") >= 60000) {
                sprintf(text, XSTR(" Blade of Urath\n", 290));
                strcat(ring_info_text, text);
            }
        } else if (!_stricmp(item->type->name, "Ring of Water")) {
            sprintf(text, XSTR("Summoning Ring\n", 281));
            strcat(ring_info_text, text);
            strcat(ring_info_text, XSTR("Ring Power: ", 276));
            int_to_string_with_commas(num, 0x7f, savegame_state.get_int(4, "water_ring_xp"));
            strcat(ring_info_text, num);
            strcat(ring_info_text, "\n");
            sprintf(text, XSTR("Available Summons:\n", 282));
            strcat(ring_info_text, text);
            sprintf(text, XSTR(" Blue Imp\n", 291));
            strcat(ring_info_text, text);
            if (savegame_state.get_int(4, "water_ring_xp") >= 60000) {
                sprintf(text, XSTR(" Dragon of Water\n", 292));
                strcat(ring_info_text, text);
            }
        } else if (!_stricmp(item->type->name, "Ring of Four Winds")) {
            sprintf(text, XSTR("Summoning Ring\n", 281));
            strcat(ring_info_text, text);
            strcat(ring_info_text, XSTR("Ring Power: ", 276));
            int_to_string_with_commas(num, 0x7f, savegame_state.get_int(4, "wind_ring_xp"));
            strcat(ring_info_text, num);
            strcat(ring_info_text, "\n");
            sprintf(text, XSTR("Available Summons:\n", 282));
            strcat(ring_info_text, text);
            sprintf(text, XSTR(" Celestial Samurai\n", 293));
            strcat(ring_info_text, text);
            if (savegame_state.get_int(4, "wind_ring_xp") >= 60000) {
                sprintf(text, XSTR(" Dragon of Four Winds\n", 294));
                strcat(ring_info_text, text);
            }
        } else if (!_stricmp(item->type->name, "Ring of Forest")) {
            sprintf(text, XSTR("Summoning Ring\n", 281));
            strcat(ring_info_text, text);
            strcat(ring_info_text, XSTR("Ring Power: ", 276));
            int_to_string_with_commas(num, 0x7f, savegame_state.get_int(4, "forest_ring_xp"));
            strcat(ring_info_text, num);
            strcat(ring_info_text, "\n");
            sprintf(text, XSTR("Available Summons:\n", 282));
            strcat(ring_info_text, text);
            sprintf(text, XSTR(" Poison Elemental\n", 295));
            strcat(ring_info_text, text);
            if (savegame_state.get_int(4, "forest_ring_xp") >= 60000) {
                sprintf(text, XSTR(" Dragon of the Forest\n", 296));
                strcat(ring_info_text, text);
            }
        } else if (!_stricmp(item->type->name, "Ring of Jade")) {
            sprintf(text, XSTR("Summoning Ring\n", 281));
            strcat(ring_info_text, text);
            strcat(ring_info_text, XSTR("Ring Power: ", 276));
            int_to_string_with_commas(num, 0x7f, savegame_state.get_int(4, "jade_ring_xp"));
            strcat(ring_info_text, num);
            strcat(ring_info_text, "\n");
            sprintf(text, XSTR("Available Summons:\n", 282));
            strcat(ring_info_text, text);
            sprintf(text, XSTR(" Jade Golem\n", 297));
            strcat(ring_info_text, text);
            if (savegame_state.get_int(4, "jade_ring_xp") >= 60000) {
                sprintf(text, XSTR(" Dragon of Jade\n", 298));
                strcat(ring_info_text, text);
            }
        } else {
            strcat(ring_info_text, XSTR("Value: ", 274));
            int_to_string_with_commas(num, 0x7f, item->type->value);
            strcat(ring_info_text, num);
            strcat(ring_info_text, XSTR(" gp\n", 275));
        }
        break;
    }
    case 1: {
        char num[0x80];
        strcat(ring_info_text, XSTR("Value: ", 274));
        int_to_string_with_commas(num, 0x7f, item->type->value);
        strcat(ring_info_text, num);
        strcat(ring_info_text, XSTR(" gp\n", 275));
        sprintf(text, XSTR("Protection: %d\n", 280), (int)(item->type->protection * 100.0f + 0.5f));
        strcat(ring_info_text, text);
        if (item->type->armour_speed != 0.0f)
            sprintf(text, XSTR("Speed: %3.2f\n", 278), 1.0f / item->type->armour_speed);
        else
            sprintf(text, XSTR("Speed: 1.00\n", 279));
        strcat(ring_info_text, text);
        break;
    }
    case 0: {
        char num[0x80];
        strcat(ring_info_text, XSTR("Value: ", 274));
        int_to_string_with_commas(num, 0x7f, item->type->value);
        strcat(ring_info_text, num);
        strcat(ring_info_text, XSTR(" gp\n", 275));
        sprintf(text, XSTR("Damage: %d\n", 277), item->type->damage);
        strcat(ring_info_text, text);
        if (item->type->speed != 0.0f)
            sprintf(text, XSTR("Speed: %3.2f\n", 278), 1.0f / item->type->speed);
        else
            sprintf(text, XSTR("Speed: 1.00\n", 279));
        strcat(ring_info_text, text);
        break;
    }
    default:
        return ring_info_text;
    }

    // Skill mods (multiplayer shows skills 0x14 / 0x15 as 0x11 / 0x1d).
    for (i = 0; i < 4; i++) {
        item_mod *m = item->type->skill_mods[i];
        if (m) {
            int skill = m->id;
            if (game_flags & GF_MULTIPLAYER) {
                switch (skill) {
                case 0x14:
                    skill = 0x11;
                    break;
                case 0x15:
                    skill = 0x1d;
                    break;
                }
            }
            if (m->value >= 0.0f)
                sprintf(text, "%s: +%d\n", localize_string_or_invalid(skill_table[skill].name),
                        RING_ROUND10(m->value));
            else
                sprintf(text, "%s: -%d\n", localize_string_or_invalid(skill_table[skill].name),
                        RING_ROUND10_NEG(m->value));
            strcat(ring_info_text, text);
        }
    }
    for (i = 0; i < 4; i++) {
        item_mod *m = item->type->resist_mods[i];
        if (m) {
            const char *fmt;
            if (m->value >= 0.0f) {
                n = RING_ROUND10(m->value);
                fmt = XSTR("Resist %s: +%d\n", 299);
            } else {
                n = RING_ROUND10_NEG(m->value);
                fmt = XSTR("Resist %s: -%d\n", 300);
            }
            sprintf(text, fmt, localize_string_or_invalid(damage_type_loc_names[m->id]), n);
            strcat(ring_info_text, text);
        }
    }
    for (i = 0; i < 2; i++) {
        item_stat_mod *m = item->type->stat_mods[i];
        if (m) {
            if (m->value >= 0.0f) {
                int v = m->value;
                sprintf(text, "%s: +%d\n", localize_string_or_invalid(ring_stat_names[m->stat]), v);
            } else {
                int v = -m->value;
                sprintf(text, "%s: -%d\n", localize_string_or_invalid(ring_stat_names[m->stat]), v);
            }
            strcat(ring_info_text, text);
        }
    }

    // Status line: "Status: " then the effects, ", " between them.
    {
        bool comma = false;
        bool first = true;
        text[0] = 0;
        if (item->type->flags & ITF_STEALTH) {
            strcpy(text, XSTR("Status: ", 301));
            first = false;
            strcat(text, XSTR("Stealth", 302));
            comma = true;
        }
        for (i = 0; i < 9; i++) {
            if (item->type->status_effects[i] >= 0) {
                if (first) {
                    strcpy(text, XSTR("Status: ", 301));
                    first = false;
                }
                if (comma)
                    strcat(text, ", ");
                comma = true;
                strcat(text, localize_string_or_invalid(status_effect_names[i]));
            }
        }
        if (text[0]) {
            n = gr_split_str(lens, offs, text, widths[resolution_index], 16, 0, -1);
            for (i = 0; i < n; i++) {
                strncat(ring_info_text, text + offs[i], lens[i]);
                strcat(ring_info_text, "\n");
            }
        }
    }

    // Charges.
    if (item->type->charge_effect) {
        item_charge_effect *ce = item->type->charge_effect;
        const char *fmt = XSTR("Charges: %s (%d/%d)", 303);
        sprintf(text, fmt, localize_string_or_invalid(spell_display_names[ce->spell]),
                item->charges, ce->max_charges);
        n = gr_split_str(lens, offs, text, widths[resolution_index], 16, 0, -1);
        for (i = 0; i < n; i++) {
            strncat(ring_info_text, text + offs[i], lens[i]);
            strcat(ring_info_text, "\n");
        }
    }

    // Triggered effect.
    switch (item->type->kind) {
    case 1:
        if (item->type->armour_triggered_effect) {
            const char *fmt = XSTR("Effect: %s\n", 304);
            sprintf(text, fmt, localize_string_or_invalid(spell_display_names[item->type->armour_triggered_effect->spell]));
            strcat(ring_info_text, text);
        }
        break;
    case 0:
        if (item->type->weapon_triggered_effect) {
            const char *fmt = XSTR("Effect: %s\n", 304);
            sprintf(text, fmt, localize_string_or_invalid(spell_display_names[item->type->weapon_triggered_effect->spell]));
            strcat(ring_info_text, text);
        }
        break;
    }

    // Skill requirements; continuation lines are indented by two spaces.
    switch (item->type->kind) {
    case 1:
        if (item->type->armour_weight > 0.0f) {
            char req[0x100] = "";
            sprintf(req, XSTR("Skill Req: Heavy Arms %d\n", 312), (int)(item->type->armour_weight * 10.0f + 0.5f));
            n = gr_split_str(lens, offs, req, widths[resolution_index], 16, 0, -1);
            for (i = 0; i < n; i++) {
                strncat(ring_info_text, req + offs[i], lens[i]);
                strcat(ring_info_text, "\n");
                if (i < n - 1)
                    strcat(ring_info_text, "  ");
            }
        }
        break;
    case 0: {
        bool has = false;
        char req[0x100] = "";
        bool comma = false;
        switch (item->type->weapon_skill) {
        case 3:
            strcat(req, XSTR("Skill Req: Sword", 305));
            comma = true;
            has = true;
            break;
        case 4:
            strcat(req, XSTR("Skill Req: Axe", 306));
            comma = true;
            has = true;
            break;
        case 5:
            strcat(req, XSTR("Skill Req: Blunt", 307));
            comma = true;
            has = true;
            break;
        case 6:
            strcat(req, XSTR("Skill Req: Staff", 308));
            comma = true;
            has = true;
            break;
        case 7:
            strcat(req, XSTR("Skill Req: Bow", 309));
            comma = true;
            has = true;
            break;
        }
        if (item->type->weight > 0.0f) {
            if (comma)
                strcat(req, ", ");
            else
                strcat(req, XSTR("Skill Req: ", 310));
            sprintf(text, XSTR("Heavy Arms %d", 311), (int)(item->type->weight * 10.0f + 0.5f));
            strcat(req, text);
            has = true;
        }
        if (has) {
            n = gr_split_str(lens, offs, req, widths[resolution_index], 16, 0, -1);
            for (i = 0; i < n; i++) {
                strncat(ring_info_text, req + offs[i], lens[i]);
                strcat(ring_info_text, "\n");
                if (i < n - 1)
                    strcat(ring_info_text, "  ");
            }
        }
        break;
    }
    }

    // Who may equip it, unless all four heroes can.
    {
        ItemType *t = item->type;
        if (!((t->restricted & 1) && (t->restricted & 4) && (t->restricted & 2) && (t->restricted & 8))) {
            int count = 0;
            strcpy(text, XSTR("Equippable by: ", 313));
            if (t->restricted & 1) {
                strcat(text, "Joseph");
                count = 1;
            }
            if (t->restricted & 8) {
                if (count)
                    strcat(text, ", ");
                strcat(text, "Jekhar");
                count++;
            }
            if (t->restricted & 4) {
                if (count)
                    strcat(text, ", ");
                strcat(text, "Flece");
                count++;
            }
            if (t->restricted & 2) {
                if (count)
                    strcat(text, ", ");
                strcat(text, "Rosalind");
            }
            strcat(ring_info_text, "\n");
            n = gr_split_str(lens, offs, text, widths[resolution_index], 16, 0, -1);
            for (i = 0; i < n; i++) {
                strncat(ring_info_text, text + offs[i], lens[i]);
                strcat(ring_info_text, "\n");
            }
        }
    }

    // Description, after an empty line.
    if (item->type->description) {
        strcat(ring_info_text, "\n");
        n = gr_split_str(lens, offs, item->type->description, widths[resolution_index], 16, 0, -1);
        for (i = 0; i < n; i++) {
            strncat(ring_info_text, item->type->description + offs[i], lens[i]);
            strcat(ring_info_text, "\n");
        }
    }
    return ring_info_text;
}

// 0x44d540  Confidence: exact (VC6 byte match, match11; was likely) (the third loop passes its two slot variables in the other order than
// the second, (c, &[esp+0xc], &[esp+8]) against (c, &[esp+8], &[esp+0xc]): written as separate block
// locals). Literal: the slot-9 page-in of a hired / scripted character uses equipment[5]'s type.
// Species 0x57 also pages the "TigerAxe"; species 0xce pages its first attachment's item type.
void items_level_load()
{
    Character *c;
    int type, i;

    printf("============================== ITEM LEVEL LOAD!\n");
    items_reset_paging();
    type = item_type_find("Backpack");
    if (type < 0)
        VSDK_ERROR(ITEMS_CPP, 2340, "Cannot find backpack!");
    if (!item_page_in(type, ITEM_PAGE_SLOT_TYPE, 0))
        VSDK_ERROR(ITEMS_CPP, 2343, "Cannot page backpack!");
    type = item_type_find("Leather Bag");
    if (type < 0)
        VSDK_ERROR(ITEMS_CPP, 2347, "Cannot find leather bag!");
    if (!item_page_in(type, ITEM_PAGE_SLOT_TYPE, 0))
        VSDK_ERROR(ITEMS_CPP, 2350, "Cannot page leather bag!");
    type = item_type_find("Gold");
    if (type < 0)
        VSDK_ERROR(ITEMS_CPP, 2354, "Cannot find gold!");
    if (!item_page_in(type, ITEM_PAGE_SLOT_TYPE, 0))
        VSDK_ERROR(ITEMS_CPP, 2357, "Cannot page gold!");

    // Monsters and NPCs (not heroes, not flag 0x100): every equipped item into the level slot.
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (!(c->flags & 0x100) && (c->info->species < 0 || c->info->species > 3)) {
            for (i = 0; i < 10; i++) {
                if (c->equipment[i])
                    item_page_in(c->equipment[i]->type - item_types, ITEM_PAGE_SLOT_TYPE, 0);
            }
            if (c->info->species == 0x57)
                item_page_in(item_type_find("TigerAxe"), ITEM_PAGE_SLOT_TYPE, 0);
        }
    }
    // Weapons and shields into the per-character slots.
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->flags & 0x100) {
            if (c->equipment[5])
                item_page_in(c->equipment[5]->type - item_types, 8, 0);
            if (c->equipment[9])
                item_page_in(c->equipment[5]->type - item_types, 9, 0);
            if (c->info->species == 0xce)
                item_page_in(c->attachments[0].item_type, 9, 0);
        } else {
            int slot1, slot2;
            item_paging_find_player_slot(c, &slot1, &slot2);
            if (slot1 != -1) {
                if (c->equipment[5])
                    item_page_in(c->equipment[5]->type - item_types, slot1, 0);
                if (c->equipment[6])
                    item_page_in(c->equipment[6]->type - item_types, slot2, 0);
            }
        }
    }
    for (c = living_list_inactive.live_next; c != &living_list_inactive; c = c->live_next) {
        int slot1, slot2;
        item_paging_find_player_slot(c, &slot1, &slot2);
        if (slot1 != -1) {
            if (c->equipment[5])
                item_page_in(c->equipment[5]->type - item_types, slot1, 0);
            if (c->equipment[6])
                item_page_in(c->equipment[6]->type - item_types, slot2, 0);
        }
    }
}

// 0x44d870  Confidence: exact.
void items_level_unload()
{
    int i;

    printf("============================== ITEM LEVEL UNLOAD!\n");
    for (i = 0; i < MAX_ITEM_PAGE_SLOTS; i++)
        item_page_out_all(i);
    items_reset_paging();
}

// 0x44d8a0  Confidence: exact. "Unknown#Known" names: the part after '#' once identified.
void item_get_display_name(char *buf, ItemType *type)
{
    char *name = localize_item_name(type->name);
    char *hash = strchr(name, '#');

    if (hash) {
        if (item_type_is_identified(type)) {
            strcpy(buf, hash + 1);
            return;
        }
        strncpy(buf, name, hash - name);
        buf[hash - name] = 0;
        return;
    }
    strcpy(buf, name);
}

// 0x44d940  Confidence: exact (the last compare is the branch-free neg / sbb / and / add of
// `? 6 : -1`).
int hero_model_index(const char *name)
{
    if (!_stricmp(name, "joseph"))
        return 0;
    if (!_stricmp(name, "heavy joseph"))
        return 1;
    if (!_stricmp(name, "jekhar"))
        return 2;
    if (!_stricmp(name, "heavy jekhar"))
        return 3;
    if (!_stricmp(name, "flece"))
        return 4;
    if (!_stricmp(name, "heavy flece"))
        return 5;
    if (!_stricmp(name, "rosalind"))
        return 6;
    return -1;
}
