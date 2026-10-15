// engine/objects/living_entity.h - characters (engine\objects\living_entity.cpp, path references
// 0x4569a3-0x461bcd; the class is living_entity, constructor living_entity__living_entity 0x44e060).
// Not original source. Layout: symbols/types.h Character (0x788 bytes), on top of obj (objects.h), with
// the +0x324 / +0x558 sub-objects typed (ai_state, living_combat) and the thiscall methods the pilots
// call. The stubs' Character copies and the Character_M shims are merged here (DECISIONS.md D1).
#ifndef ENGINE_OBJECTS_LIVING_ENTITY_H
#define ENGINE_OBJECTS_LIVING_ENTITY_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/ai/ai.h"
#include "engine/characterinfo/characterinfo.h"

class Character;
class character_instance;

// The living_combat sub-object at Character +0x558 (living_combat__ctor 0x4681b0, set_owner: [0] = the
// character; 0x54 bytes). character_get_protection is thiscall on it (combat.cpp range).
class living_combat {
public:
    float get_protection();           // 0x464de0 character_get_protection
    Character *owner;                 // +0x00
    ubyte _unk04[0x50];
};
LAYOUT_CHECK(living_combat, sizeof(living_combat) == 0x54);

// Character +0x320 flags
#define CF_SUMMONLIKE    0x2          // cleared by party_add_member (party_cycle skips it)
#define CF_HIDDEN        0x8
#define CF_DEAD          0x80
#define CF_LEFT_PARTY    0x800000

// Status effect ids: Character.status_slots index (status_effect_add 0x454700). 7, 8 and 14-16 show no
// text; the id must be <= 0x10.
enum status_effect_id {
    STATUS_BLIND = 0, STATUS_SILENCE = 1, STATUS_FAITH = 2, STATUS_CURSE = 3, STATUS_BLESS = 4,
    STATUS_REGEN = 5, STATUS_POWER = 6, STATUS_SLEEP = 9, STATUS_PARALYZE = 10, STATUS_POISON = 11,
    STATUS_CRIPPLE = 12, STATUS_SLOW = 13,
};

// A character (party member, NPC or monster), 0x788 bytes (the living-entity pool stride).
class Character : public obj {
public:
    // living_entity.cpp
    char *get_name();                 // 0x44ed90 character_get_name
    void  set_name(const char *name); // 0x44edb0 character_set_name
    void  resurrect();                // 0x454070 character_resurrect
    void  set_team(int team);         // 0x454610 character_set_team
    bool  has_status(int status);     // 0x455950 character_has_status
    void  clear_all_effects();        // 0x455a90 character_clear_all_effects
    void  get_display_name(char *buf);// 0x462070 character_get_display_name
    float get_skill(int skill);       // 0x462440 character_get_skill
    float resist_equip_bonus(int type);   // 0x462660 character_resist_equip_bonus
    float get_resistance(int type);   // 0x4626b0 character_get_resistance (3 = magic)
    int   equip_bonus(int stat);      // 0x4627c0 character_equip_bonus (0 HP, 1 AP)
    int   get_max_stat(int stat);     // 0x462840 character_get_max_stat (0 HP, 1 AP)
    const char *hero_model_prefix();  // 0x462ec0 character_hero_model_prefix

    int team;                         // +0x098
    int move_start_time;              // +0x09C
    ubyte trigger_latch[3];           // +0x0A0
    ubyte _padA3;                     // +0x0A3
    int death_move_sub;               // +0x0A4
    void *death_move;                 // +0x0A8
    ubyte status_slots[18][0x20];     // +0x0AC status effects by status_effect_id
    GameTimer timer2ec;               // +0x2EC
    unsigned int enemy_team_mask;     // +0x2F0
    char name[0x20];                  // +0x2F4
    character_instance *instance;     // +0x314 character instance (vsdk ca)
    Character *live_next;             // +0x318 living-entity list
    Character *live_prev;             // +0x31C
    unsigned int flags;               // +0x320 CF_*
    ai_state ai;                      // +0x324
    // +0x558. new_status.cpp reaches it as `combat` (COMBAT_M), new_assess.cpp as `protection`; one
    // member with two names until the code is changed (DECISIONS.md D1).
    union {
        living_combat combat;
        living_combat protection;
    };
    int anim_stand;                   // +0x5AC locomotion animation ids (locomotion_anim_names, 18)
    int anim_norm_walk;               // +0x5B0
    int anim_norm_run;                // +0x5B4
    int anim_ready;                   // +0x5B8
    int anim_walk;                    // +0x5BC
    int anim_run;                     // +0x5C0
    int anim_stand_hurt;              // +0x5C4
    int anim_norm_walk_hurt;          // +0x5C8
    int anim_norm_run_hurt;           // +0x5CC
    int anim_ready_hurt;              // +0x5D0
    int anim_walk_hurt;               // +0x5D4
    int anim_run_hurt;                // +0x5D8
    int anim_sneak_ready;             // +0x5DC
    int anim_hide_ready;              // +0x5E0
    int anim_tripped_liedown;         // +0x5E4
    int anim_cloaked_ready;           // +0x5E8
    int anim_cloaked_sneak;           // +0x5EC
    int anim_statscreen;              // +0x5F0
    float base_max_hp;                // +0x5F4
    float hp;                         // +0x5F8 current HP
    short base_max_ap;                // +0x5FC
    short ap;                         // +0x5FE current AP
    short level;                      // +0x600
    short _pad602;
    int xp;                           // +0x604
    ubyte hit_by_party;               // +0x608
    ubyte _pad609[3];
    int move_fx;                      // +0x60C
    float frametime;                  // +0x610
    ubyte skill[0x60];                // +0x614 by skill id, value * 0.01 (character_get_skill)
    int move_sound;                   // +0x674
    short foot_tag_l;                 // +0x678
    short foot_tag_r;                 // +0x67A
    float ground_hit[3];              // +0x67C
    short ground_lobj;                // +0x688
    short ground_face;                // +0x68A
    int layer;                        // +0x68C
    short _unk690;                    // +0x690
    short _pad692;
    float stance_blend;               // +0x694
    float forward_speed;              // +0x698
    float velocity[3];                // +0x69C
    float loco_blend;                 // +0x6A8
    float target_speed;               // +0x6AC
    float slowdown_rate;              // +0x6B0
    float run_speed;                  // +0x6B4
    float walk_speed;                 // +0x6B8
    float speed_scale;                // +0x6BC
    float _vec6c0[3];                 // +0x6C0
    int cur_anim;                     // +0x6CC
    int cur_move;                     // +0x6D0
    GameTimer timer6d4;               // +0x6D4
    int cutscene_object;              // +0x6D8
    int script_object;                // +0x6DC
    int mount_link;                   // +0x6E0
    unsigned int mount_flags;         // +0x6E4
    float hp_overflow;                // +0x6E8
    float pos_mod_text;               // +0x6EC
    float hp_text_timer;              // +0x6F0
    float pos_mod_text_timer;         // +0x6F4
    char float_text[16];              // +0x6F8
    char float_text_nul;              // +0x708
    ubyte float_text_rgb[3];          // +0x709
    float float_text_scale;           // +0x70C
    int _unk710;                      // +0x710
    float move_turn_angle;            // +0x714
    float move_turn_applied;          // +0x718
    char_info *info;                  // +0x71C
    int prop_tag;                     // +0x720
    int default_prop_tag;             // +0x724
    int num_attachments;              // +0x728
    struct { int slot; int item_type; } attachments[5];   // +0x72C
    item_info *equipment[10];         // +0x754 equipped items; [5] (+0x768) the weapon
    int anim_lod_counter;             // +0x77C
    float light_level;                // +0x780
    GameTimer net_update_timer;       // +0x784
};
LAYOUT_CHECK(Character, sizeof(Character) == 0x788);

// The pilots wrote the thiscall calls through these shims while Character was a C struct (types.h).
// They are now plain member accesses (DECISIONS.md D1); the macros stay until the code drops them.
#define CHAR_M(c)   (c)
#define COMBAT_M(c) (&(c)->combat)
#define AI_M(c)     (&(c)->ai)

extern Character living_list;         // 0x8eca60 sentinel (sum.symbols' living_list_first 0x8ecd78 = live_next)

Character *living_entity_create(int type, int char_info, const vector3 *pos, const matrix *orient,
                                int a, int b, int c, int d, int e);   // 0x461b80 (last five: 1, 0, 0, 0, 0)
void character_xp_death_penalty(Character *c);        // 0x441710

#endif
