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

#define NUM_LOCO_ANIMS 18             // locomotion_anim_names, Character +0x5ac..+0x5f0

// Character +0x320 flags
#define CF_SUMMONLIKE    0x2          // cleared by party_add_member (party_cycle skips it)
#define CF_HIDDEN        0x8
#define CF_DEAD          0x80
#define CF_LEFT_PARTY    0x800000
#define CF_ANIM_LOD      0x8000000    // character_anim_lod_skip: low-LOD frame skipping this frame
#define CF_ANIM_LOD_PREV 0x10000000   // ... and the previous frame

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
    bool  skill_allowed(int skill);   // 0x462400 character_skill_allowed
    float get_skill(int skill);       // 0x462440 character_get_skill
    float resist_equip_bonus(int type);   // 0x462660 character_resist_equip_bonus
    float get_resistance(int type);   // 0x4626b0 character_get_resistance (3 = magic)
    int   equip_bonus(int stat);      // 0x4627c0 character_equip_bonus (0 HP, 1 AP)
    int   get_max_stat(int stat);     // 0x462840 character_get_max_stat (0 HP, 1 AP)
    const char *hero_model_prefix();  // 0x462ec0 character_hero_model_prefix
    // decomp20 (conversationctl.cpp). Argument lists from its call sites.
    float size_radius();              // 0x4542b0 character_size_radius
    float size_height();              // 0x4542f0 character_size_height
    float distance_between_edges(Character *other, bool a, int b);  // 0x454330 (the calls pass false, -1)
    void  status_effect_cure(int status, int a, int b);   // 0x4553a0 (conversation_begin: 8, 1, 0)
    void  walk_to_point(vector3 pos, int priority);       // 0x455e70 character_walk_to_point (by value)
    void  ai_add_goal9(vector3 *pos, int layer, int priority, int a);   // 0x455f50 character_ai_add_goal9
    float stopping_distance(vector3 *out_vel);            // 0x45a840 character_stopping_distance (NULL)
    int   clip_move_to_navmesh(vector3 *move);            // 0x45ab00 character_clip_move_to_navmesh
    int   play_move(int move, float speed, int a, int b, int c);   // 0x460f20 character_play_move
    bool  move_finished(int move);                        // 0x4614d0 character_move_finished
    int   pick_talk_move();                               // 0x461540 character_pick_talk_move, 0 = none
    int   is_moving();                                    // 0x462e40 character_is_moving
    // Inline in the original header (DECISIONS.md C3): expanded in conversation_frame and
    // conversation_camera_begin, called as the COMDAT 0x4da830 character_get_center elsewhere.
    vector3 get_center() { vector3 c; c.x = pos.x + centre_offset.x; c.y = pos.y + centre_offset.y;
                           c.z = pos.z + centre_offset.z; return c; }
    void  set_6ac(float v);           // 0x4570d0 character_set_6ac
    void  update_locomotion_anim();   // 0x45a120 character_update_locomotion_anim
    void  set_locomotion_speed(float speed);  // 0x45a6e0 character_set_locomotion_speed
    void *get_current_move();         // 0x461500 character_get_current_move (0 = none)
    bool  is_in_view(float radius_scale); // 0x44f020 character_is_in_view
    void  move_frame();               // 0x45cd00 living_entity_move_frame
    void  update_footsteps();         // 0x45d690 character_update_footsteps
    // objects.cpp
    bool  anim_lod_skip();            // 0x46ad10 character_anim_lod_skip
    void  process_anim(float frametime);  // 0x46adf0 character_process_anim
    void  fill_locomotion_anims();    // 0x46af70 character_fill_locomotion_anims

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
    // +0x5AC: character_fill_locomotion_anims (0x46af70) fills them as one array indexed by the
    // locomotion_anim_names index ([edx + 4*ecx + 0x5ac]); the other code uses the names
    // (DECISIONS.md D60). Anonymous struct in a union: an MS extension VC6 accepts.
    union {
        int loco_anims[NUM_LOCO_ANIMS];
        struct {
            int anim_stand;               // +0x5AC locomotion animation ids (locomotion_anim_names, 18)
            int anim_norm_walk;           // +0x5B0
            int anim_norm_run;            // +0x5B4
            int anim_ready;               // +0x5B8
            int anim_walk;                // +0x5BC
            int anim_run;                 // +0x5C0
            int anim_stand_hurt;          // +0x5C4
            int anim_norm_walk_hurt;      // +0x5C8
            int anim_norm_run_hurt;       // +0x5CC
            int anim_ready_hurt;          // +0x5D0
            int anim_walk_hurt;           // +0x5D4
            int anim_run_hurt;            // +0x5D8
            int anim_sneak_ready;         // +0x5DC
            int anim_hide_ready;          // +0x5E0
            int anim_tripped_liedown;     // +0x5E4
            int anim_cloaked_ready;       // +0x5E8
            int anim_cloaked_sneak;       // +0x5EC
            int anim_statscreen;          // +0x5F0
        };
    };
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
// The same shims under the names of new_hud.cpp / new_inventory.cpp (DECISIONS.md D45).
#define CHAR_HM(c)   (c)
#define AI_HM(c)     (&(c)->ai)
#define AI_GOAL_M(c) (&(c)->ai)
#define INV_EQUIP(c, i) ((c)->equipment[i])

bool character_is_locally_controlled(Character *c);   // 0x475780 (unit not known)

extern Character living_list;         // 0x8eca60 sentinel (sum.symbols' living_list_first 0x8ecd78 = live_next)
extern Character living_list_inactive; // 0x8ed1e8 second sentinel (live_next 0x8ed500), walked by
                                      // living_entities_move_all
extern char *locomotion_anim_names[NUM_LOCO_ANIMS];   // 0x599648 "stand", "norm_walk", ... "statscreen"

// Speech bubbles (character_say 0x461dc0, counted down by objects_do_frame). Type name ours.
struct character_speech_record {
    char text[0x100];                 // +0x000
    int speaker;                      // +0x100 character handle, -1 free
    float time_left;                  // +0x104 seconds
};
LAYOUT_CHECK(character_speech_record, sizeof(character_speech_record) == 0x108);
#define MAX_CHARACTER_SPEECH 16
extern character_speech_record character_speech[MAX_CHARACTER_SPEECH];   // 0x940438

// Per-character spell cooldowns (spell_cooldown_start 0x44dbd0, ticked by objects_do_frame). Type name ours.
struct spell_cooldown {
    int handle;                       // +0x0 character handle, -1 free
    int spell;                        // +0x4
    GameTimer timer;                  // +0x8
};
LAYOUT_CHECK(spell_cooldown, sizeof(spell_cooldown) == 0xc);
#define MAX_SPELL_COOLDOWNS 100
extern spell_cooldown spell_cooldowns[MAX_SPELL_COOLDOWNS];   // 0x93ff78

void living_entity_reset_tables();    // 0x462d80

Character *living_entity_create(int type, int char_info, const vector3 *pos, const matrix *orient,
                                int a, int b, int c, int d, int e);   // 0x461b80 (last five: 1, 0, 0, 0, 0)
void character_xp_death_penalty(Character *c);        // 0x441710
// Stops or attaches the looping effect of a status (unit not known; conversation_begin passes the
// character's handle, Character +0x80; decomp20).
void status_effect_update_fx(int handle, int status); // 0x48c470

#endif
