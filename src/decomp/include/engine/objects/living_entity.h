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
#include "engine/combat/combat.h"

class Character;
class character_instance;

// The living_combat sub-object at Character +0x558 is declared in engine/combat/combat.h with its
// fields (combat.cpp, decomp23; DECISIONS.md D300).

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
    // decomp27 (level_scripts.cpp): the LENT record and the flag-bound characters.
    int   save_state(ubyte *out);     // 0x457150 character_save_state, bytes written
    int   load_state(const ubyte *in);// 0x458800 character_load_state, bytes read
    void  set_ai_active(bool on);     // 0x462200 character_set_ai_active
    void  add_xp(int xp, int a, int b);   // 0x44fab0 character_add_xp (conversation_execute_action: xp, 0, 0)
    void  update_locomotion_anim();   // 0x45a120 character_update_locomotion_anim
    void  set_locomotion_speed(float speed);  // 0x45a6e0 character_set_locomotion_speed
    void *get_current_move();         // 0x461500 character_get_current_move (0 = none)
    int   spell_on_cooldown(int spell);// 0x44dd30 character_spell_on_cooldown
    void  add_ap(float delta);        // 0x450c70 character_add_ap (return unused)
    bool  is_in_view(float radius_scale); // 0x44f020 character_is_in_view
    void  move_frame();               // 0x45cd00 living_entity_move_frame
    void  update_footsteps();         // 0x45d690 character_update_footsteps
    // decomp23 (combat.cpp, spells.cpp). Argument lists from the call sites. play_move, move_finished,
    // size_radius, distance_between_edges, status_effect_cure and is_moving use the same text as
    // decomp20's copy (merge).
    bool  add_hp(float delta, int source, int a, int b);  // 0x450850 character_add_hp -> still alive (combat: handle / -1, 1, 0)
    void  status_effect_add(int status, int ms, int a, int b, int source, int c);   // 0x454700 (combat: id, 15000, -1, 0, handle, 0)
    float status_value(int status);                       // 0x455a30 character_status_value
    void  get_hit_by_spell(int spell, int source, int a, int b, int source_type, int c, int d, int e);  // 0x45dea0 living_entity_get_hit_by_spell
    // The first argument is the move record (character_find_move's result); decomp20 declared it int,
    // kept for the merge (DECISIONS.md D303).
    move_info *find_move(int category, int id);           // 0x461370 character_find_move
    move_info *find_move_alt(int category, int id);       // 0x4613b0 character_find_move_alt
    bool  move_playing(int anim, float *t);               // 0x461450 character_move_playing (t may be 0)
    float move_trigger_time_left(int trigger);            // 0x461670 character_move_trigger_time_left
    int   move_trigger_fired(int trigger, int latch);     // 0x461740 1 once when passed, 0 before, -1 no move/trigger
    // objects.cpp
    bool  anim_lod_skip();            // 0x46ad10 character_anim_lod_skip
    void  process_anim(float frametime);  // 0x46adf0 character_process_anim
    void  fill_locomotion_anims();    // 0x46af70 character_fill_locomotion_anims
    // decomp29 (boss_ai.cpp)
    Character *get_mount();           // 0x462d20 character_get_mount (+0x6e0 when that entity has +0x6e4 bit 1)
    Character *get_rider();           // 0x462d40 character_get_rider (+0x6e0 when that entity has +0x6e4 bit 0)
    float get_698();                  // 0x456f80 character_get_698 (1.0 or forward_speed)
    // decomp34 (level_script_khosani_lab.cpp, level_script_worldmap.cpp; D1404)
    void  place_at_party_start(int slot);         // 0x44f1a0 character_place_at_party_start (level_player_starts[slot])
    void  set_speed_scale(float scale, bool flag);// 0x462260 character_set_speed_scale (+0x6bc; flag: +0x320 bit 0x200)

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
    move_info *cur_move;              // +0x6D0 current move record (combat reads +0x18; D301)
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
// Per char_info size (+0x90): character_size_height / character_size_radius and charinfo_boss_height /
// _radius (boss_ai.cpp, decomp29). The heights {0.9, 1.8, 2.5, 99.9} are in .rdata (name ours), the
// radii {0.15, 0.47, 1.0, 99.9} in living_entity.cpp's .data.
extern const float character_size_heights[4];          // 0x5818d8
extern float character_size_radii[4];                  // 0x59d7c8

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

// Console toggles in living_entity.cpp's .bss (decomp23: spell_roll_amount reads No_resist).
extern bool Players_immortal;         // 0x941704 "immortal"
extern bool No_resist;                // 0x941705 "no_resist"

void living_entity_reset_tables();    // 0x462d80
int  character_save_state_size();     // 0x4587f0 0x17c, the fixed LENT record size (decomp27)
#define CF_SUMMON 0x100               // +0x320: the summon (savegame LENT code, level_scripts.cpp; decomp27)

Character *living_entity_create(int type, int char_info, const vector3 *pos, const matrix *orient,
                                int a, int b, int c, int d, int e);   // 0x461b80 (last five: 1, 0, 0, 0, 0)
void character_xp_death_penalty(Character *c);        // 0x441710
// Stops or attaches the looping effect of a status (unit not known; conversation_begin passes the
// character's handle, Character +0x80; decomp20).
void status_effect_update_fx(int handle, int status); // 0x48c470

#endif
