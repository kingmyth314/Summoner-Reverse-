// engine/spells/spells.h - the spell table and spell casting (spells.cpp in sum.symbols / strings.tbl,
// code 0x479b60-0x47f857: init_strptrs_22e92a0_thunk .. cmd_summon). Directory guessed (the unit has no
// path string). Not original source. Layout: symbols/types.h spell_info. Extended by decomp23 (the
// spells.cpp pilot); the old declarations keep their text.
#ifndef ENGINE_SPELLS_SPELLS_H
#define ENGINE_SPELLS_SPELLS_H

#include "vsdk/vsdk.h"
#include "vsdk/math/matrix.h"
#include "vsdk/os/stringpool.h"
#include "engine/gamesound/gamesound.h"   // sound_ref

class Character;
class TextFile;
class console_command;
class effect_object;
struct ItemType;

// spell_info::spell_class (+Class:, spell_class_parse_names)
#define SPELL_CLASS_HEAL   0
#define SPELL_CLASS_DARK   1
#define SPELL_CLASS_ENERGY 2
#define SPELL_CLASS_HOLY   3
#define SPELL_CLASS_FIRE   4
#define SPELL_CLASS_ICE    5
#define SPELL_CLASS_SUMMON 6
#define SPELL_CLASS_MISC   7
#define NUM_SPELL_CLASSES  8

// spell_info::flags
#define SPELL_OFFENSIVE        0x01
#define SPELL_TARGET_CHARACTER 0x02
#define SPELL_TARGET_LOCATION  0x04
#define SPELL_SERVER_AUTHORIZE 0x08
#define SPELL_FADE_PROPS       0x10
#define SPELL_BENEFICIAL       0x20
#define SPELL_STATUS_CHANGE    0x40
#define SPELL_HIT_ONLY         0x80

// Spell indices the code uses by number (spells.tbl order; docs/sum-exe/game-tables.md "Spells").
#define SPELL_HEAL          0x00
#define SPELL_VITALIZE      0x04
#define SPELL_EMPOWER       0x10
#define SPELL_BLESS         0x13
#define SPELL_PROTECT       0x15
#define SPELL_FAITH         0x16
#define SPELL_FIRE_ARROW    0x19
#define SPELL_FIREBALL      0x1a
#define SPELL_BLIZZARD      0x23
#define SPELL_SUMMON_GOLEM  0x26
#define SPELL_SUMMON_BLACKFIRE 0x2a
#define SPELL_GENERIC_ARROW 0x37
#define SPELL_TRIP          0x3b
#define SPELL_GHOST_PARALYZING_FEAR 0x50
#define SPELL_NIGHTMARE     0x48      // decomp29 (boss_attack_trigger_dispatch: 20 s serpent attack timer)
#define SPELL_GHOST_DEATH   0x54      // decomp29 (boss_counter_hit_dispatch)

// One spells.tbl record (0x80 bytes, spells 0x22e4f28).
struct spell_info {
    int spell_class;                 // +0x00 +Class: SPELL_CLASS_*
    int index;                       // +0x04 record number
    const char *name;                // +0x08 +Name:
    const char *cast_effect;         // +0x0C +Cast effect:
    sound_ref cast_sound;            // +0x10 $Sound: after +Cast effect:
    const char *hit_effect;          // +0x1C +Hit effect:
    sound_ref hit_sound;             // +0x20 $Sound: after +Hit effect: or +Preloaded Hit effect:
    const char *preloaded_hit_effect;// +0x2C +Preloaded Hit effect:
    const char *misc_effect;         // +0x30 +Misc effect:
    sound_ref misc_sound;            // +0x34 $Sound: after +Misc effect: or +Banish effect:
    const char *banish_effect;       // +0x40 +Banish effect:
    sound_ref banish_sound;          // +0x44 never parsed (the $Sound: after +Banish effect: goes to +0x34);
                                     //       spells_reset_runtime resets its handle with the others
    int variation;                   // +0x50 +Variation: (variation_names, 0xc2 entries; default 0)
    float cost;                      // +0x54 +Cost:
    float skill;                     // +0x58 +Skill: x0.01
    float hit_chance;                // +0x5C +Hit Chance: x0.01 (default 1.0)
    int die;                         // +0x60 +Die:
    short duration;                  // +0x64 +Duration: (default -1)
    short recovery_time;             // +0x66 +Recovery time: (the parser stores the record index; NOTES)
    short ai_recovery_time;          // +0x68 +AI Recovery time: (likewise)
    short loop_time;                 // +0x6A +Loop Time: (likewise)
    int emerge_time;                 // +0x6C +Emerge time:
    float distance;                  // +0x70 +Distance:
    unsigned int flags;              // +0x74 SPELL_*
    int damage_type;                 // +0x78 +Damage type: (default -1)
    float target_radius;             // +0x7C +Target radius:
};
LAYOUT_CHECK(spell_info, sizeof(spell_info) == 0x80);
#define MAX_SPELLS 108

// (count, first record) per spell class, built while parsing: one class's records are consecutive.
struct spell_group {
    int count;                       // +0x0
    spell_info *first;               // +0x4
};
#define NUM_SPELL_GROUPS 8

// A pending missile of spell_launch_missile (20 x 0x34 bytes at 0x22e4b18). Type name ours.
struct spell_missile {
    int handle;                      // +0x00 effect handle, -1 free
    matrix orient;                   // +0x04 launch orientation ($missile tag)
    vector3 pos;                     // +0x28 launch position
};
#define MAX_SPELL_MISSILES 20

// A spell effect record (64 x 0x34 bytes at 0x22e8528): bit 0 of the first word is "used", bits 1-6
// are set to all ones whenever the pool is cleared or a record is taken. The rest is the caller's data
// (spell_effect_add copies the whole record). Type and field names ours.
struct spell_effect {
    unsigned int used : 1;
    unsigned int slot : 6;
    unsigned int _bits : 25;
    int data[12];                    // +0x04
};
#define MAX_SPELL_EFFECTS 64

// The result of spell_prepare_cast (0x34 bytes, returned by value). The first word's fields as
// mp_spell_cast_sync / mp_handle_release_spell (multispell.cpp) send them: bits 1-6 the target mode
// (sign-extended: shl 0x19 / sar 0x1a), bits 7-28 and 29-31 two unsigned values; bit 0 is kept by the
// reader (not sent). target[] by mode (mp_write_spell_targets 0x4afd60): 1, 2, 9 target[0]; 3, 6, 7
// target[0..1]; 4 net-converted handles target[0..5] + values target[6..11]; 5 handles target[0..5] +
// target[6..7]; 10 target[0..4] + target[5..9]; 8 nothing. Same size and first-word split as spell_effect
// (bits 1-6). Field names ours (decomp35, D1501).
struct spell_cast_params {
    unsigned int flag0 : 1;          // bit 0
    int mode : 6;                    // bits 1-6 target mode 1..10
    unsigned int value : 22;         // bits 7-28
    unsigned int kind : 3;           // bits 29-31
    int target[12];                  // +0x04 (request +0x2c)
};
LAYOUT_CHECK(spell_cast_params, sizeof(spell_cast_params) == 0x34);

// The request combat_melee_damage builds for its self-casts (0x5c bytes on its stack): spell_prepare_cast's
// result goes to +0x28, then mp_spell_cast_sync and spell_cast_launch_effects read it. The inline
// constructor is the store group 0x46564f-0x465667 (target -1, no position, layer -1, 0, 0). Type and
// field names ours.
// A resolved spell hit (0x50 bytes on the callers' stacks: spell_cast_launch_effects copies
// spell_prepare_cast's 0x34-byte result to +0x1c, 0x47d878-0x47d894) as character_spell_apply_hit_effects
// 0x460190 reads it (decomp37). The result's first dword is a bit field (shl / sar 0x19 / 0x1a for the
// kind, shr 7 for the 22-bit mask, shr 0x1d for the flinch); the rest depends on the spell. Names ours.
struct spell_hit {
    int spell;                       // +0x00
    int source;                      // +0x04 caster handle
    vector3 *pos;                    // +0x08 effect position (missiles), or NULL
    matrix *orient;                  // +0x0C
    int _unk10[3];                   // +0x10
    unsigned int _bit0 : 1;          // +0x1C
    int kind : 6;                    //   1 HP, 2 AP, 3 / 5 flinch only, 4 chain HP, 6 AP + sleep, 7 drain, 8 level, 9 AP drain
    unsigned int hit_mask : 22;      //   bit n: target n was hit (bit 0 the target itself)
    unsigned int flinch : 3;         //   flinch variant, 0 none
    union {                          // +0x20
        struct {
            union { float amount; int iamount; int ms; };   // +0x20
            union { float value; int duration; };           // +0x24
        };
        struct {
            int handles[6];          // +0x20 the targets of a chain (curse 0x17, kind 4)
            union {
                struct { int curse_ms; float curse_value; };   // +0x38
                float amounts[6];    // +0x38 per target (kind 4)
            };
        };
        struct {
            int durations[5];        // +0x20 spell 0x67's five statuses
            float values[5];         // +0x34
        };
    };
};
LAYOUT_CHECK(spell_hit, sizeof(spell_hit) == 0x50);

struct spell_cast_request {
    spell_cast_request() { target = -1; target_pos = 0; layer = -1; _unk14 = 0; source_type = 0; }
    Character *caster;               // +0x00
    int spell;                       // +0x04
    int target;                      // +0x08 target handle
    vector3 *target_pos;             // +0x0C
    int layer;                       // +0x10
    union {                          // +0x14 the item that casts: mp_spell_cast_sync sends its item_types
        int _unk14;                  //       index (-1 none; decomp35, D1500). combat.cpp clears _unk14.
        ItemType *item;
    };
    int source_type;                 // +0x18 6 for a combat skill
    int _unk1c[3];                   // +0x1C sent for spells 0x1d and 0x1f only (mp_write_spell_target_point)
    spell_cast_params params;        // +0x28
};

// ---- spells.cpp data ----
extern const int spell_group_skill[7];               // 0x59b988 skill per spell group (heal .. ice, summon)
extern const char *spell_class_parse_names[NUM_SPELL_CLASSES];   // 0x59b9c8 "heal" .. "misc" (+Class:)
extern const char *spell_default_hit_effect;         // 0x59b9e8 "GenericHit01.vfx"
extern spell_missile spell_missiles[MAX_SPELL_MISSILES];         // 0x22e4b18
extern spell_info spells[MAX_SPELLS];                // 0x22e4f28
extern spell_effect spell_effects[MAX_SPELL_EFFECTS];            // 0x22e8528
extern StringPool spell_string_pool;                 // 0x22e9228
extern int spell_sound_path;                         // 0x22e925c file_add_path("data\sounds\spells", ".wav")
extern spell_group spell_groups[NUM_SPELL_GROUPS];   // 0x22e9260
extern char **spell_descriptions[38];                // 0x22e92a0 (localize_strings slots)
extern char **spell_display_names[MAX_SPELLS];       // 0x22e9338 (localize_strings slots)

// ---- spells.cpp functions ----
void  spells_parse_table();                          // 0x47a190
void  spells_parse_sound(TextFile *tf, spell_info *spell, sound_ref *ref);   // 0x47a6c0 (spell not read)
void  spells_free_strings();                         // 0x47a760
void  spells_reset_runtime();                        // 0x47a770
int spell_find_by_name(const char *name);     // 0x47a7c0, spells index or -1
int   spell_find_by_cast_id(int id);                 // 0x47a800 first spell whose +Variation: is id, -1
int   spell_get_group(int spell);                    // 0x47a830 group of the spell or -1
const char *spell_get_cast_effect(int spell);        // 0x47a870
const char *spell_get_effect_1c(int spell);          // 0x47a880 (+Hit effect:)
const char *spell_get_hit_effect(int spell);         // 0x47a890
int damage_type_to_status(int spell);         // 0x47a8c0, status_effect_id or -1 (items_parse_table passes
                                              // the spell index of "+Effect:")
float status_effect_modifier(int status, int level); // 0x47a9c0
int   spell_get_duration_ms(int spell, int level);   // 0x47aa80
int   status_effect_duration(int status);            // 0x47ab60 ms
spell_cast_params spell_prepare_cast(Character *caster, int spell, int target, int a, int source_type,
                                     int b);         // 0x47abe0 (not written)
void  spell_compute_resist(float *out_magic_resist, float *out_resist, int spell, Character *caster,
                           Character *target, int power, int source_type);   // 0x47c510
void  spell_cast_execute(Character *caster, int spell, int target, vector3 *pos, int layer, int a,
                         int source_type, int b);    // 0x47c5d0 (not written)
void  spell_cast_launch_effects(spell_cast_request *req);   // 0x47d420 (not written)
bool  spell_effect_pool_full();                      // 0x47dfc0 (true when a record is FREE; NOTES)
void  spell_play_cast_sound(int spell, Character *c);       // 0x47dfe0
void  spell_play_hit_sound(int spell, const vector3 *pos);  // 0x47e020
void  spell_play_sound_34(int spell, const vector3 *pos);   // 0x47e090
int   spell_place_ground_effect(int spell, int caster, const vector3 *pos, int owner,
                                const matrix *orient);      // 0x47e100 effect handle or -1
void  spell_find_ground_points(vector3 *out, int first, int last, effect_object *fx);   // 0x47e280
bool  spell_probe_ground_point(effect_object *fx, int node, vector3 *point);          // 0x47e330
void  spell_effect_obj_set_timer(int caster, int handle, const vector3 *pos, int owner,
                                 const matrix *orient);     // 0x47e4d0
int   spell_spawn_hit_fx(int caster, const vector3 *pos, const matrix *orient, int owner);   // 0x47e590
int   spell_missile_remove(int handle);              // 0x47e680 slot or -1
void  spell_launch_missile(Character *caster, int handle);  // 0x47e6c0
int   spell_fire_missile(int spell, int missile, Character *caster, Character *target,
                         const vector3 *start, const matrix *orient);   // 0x47e830 effect handle or -1
Character *summon_create(Character *caster, vector3 *pos, int a, int spell, int b);   // 0x47e900 (not written)
// Every caller pushes a handle (mp_game_frame, spell_cast_execute, spell_cast_launch_effects,
// mp_handle_summon_quick, ai_think_party_update); the body never reads it (D1502).
void  summon_dismiss(int handle);                    // 0x47f270
float spell_target_value(Character *target, int spell);     // 0x47f2a0
float spell_roll_amount(Character *target, int spell, int count, int die, float resist);   // 0x47f4d0
spell_effect *spell_effect_add(const spell_effect *rec);    // 0x47f660
spell_effect *spell_effect_alloc();                  // 0x47f690
void  spell_effect_free(spell_effect *e);            // 0x47f6d0
void  cmd_summon();                                  // 0x47f730

#endif
