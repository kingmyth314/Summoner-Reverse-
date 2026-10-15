// engine/spells/spells.h - the spell table (spells.cpp in sum.symbols / strings.tbl: spells_parse_table
// 0x47a190, spell_get_group 0x47a830, the display name slots 0x22e9338). Directory guessed (the unit has no
// path string). Not original source. Layout: symbols/types.h spell_info.
#ifndef ENGINE_SPELLS_SPELLS_H
#define ENGINE_SPELLS_SPELLS_H

#include "vsdk/vsdk.h"
#include "engine/gamesound/gamesound.h"   // sound_ref

// One spells.tbl record (0x80 bytes, spells 0x22e4f28).
struct spell_info {
    int spell_class;                 // +0x00 +Class: 0 heal, 1 dark, 2 energy, 3 holy, 4 fire, 5 ice, 6 summon, 7 misc
    int index;                       // +0x04 record number
    const char *name;                // +0x08 +Name:
    const char *cast_effect;         // +0x0C +Cast effect:
    sound_ref cast_sound;            // +0x10
    const char *hit_effect;          // +0x1C +Hit effect:
    sound_ref hit_sound;             // +0x20
    const char *preloaded_hit_effect;// +0x2C +Preloaded Hit effect:
    const char *misc_effect;         // +0x30 +Misc effect:
    sound_ref misc_sound;            // +0x34
    const char *banish_effect;       // +0x40 +Banish effect:
    ubyte _unk44[0x0c];
    int variation;                   // +0x50 +Variation:
    float cost;                      // +0x54 +Cost:
    float skill;                     // +0x58 +Skill: x0.01
    float hit_chance;                // +0x5C +Hit Chance: x0.01
    int die;                         // +0x60 +Die:
    short duration;                  // +0x64 +Duration:
    short recovery_time;             // +0x66 +Recovery time:
    short ai_recovery_time;          // +0x68 +AI Recovery time:
    short loop_time;                 // +0x6A +Loop Time:
    int emerge_time;                 // +0x6C +Emerge time:
    float distance;                  // +0x70 +Distance:
    unsigned int flags;              // +0x74 1 offensive, 2 target character, 4 target location, ...
    int damage_type;                 // +0x78 +Damage type:
    float target_radius;             // +0x7C +Target radius:
};
LAYOUT_CHECK(spell_info, sizeof(spell_info) == 0x80);
#define MAX_SPELLS 108

extern spell_info spells[MAX_SPELLS];                // 0x22e4f28
int spell_find_by_name(const char *name);     // 0x47a7c0, spells index or -1
int damage_type_to_status(int spell);         // 0x47a8c0, status_effect_id or -1 (items_parse_table passes
                                              // the spell index of "+Effect:")
extern char **spell_display_names[MAX_SPELLS];       // 0x22e9338 (localize_strings slots)

#endif
