// engine/objects/living_entity.h - characters (engine\objects\living_entity.cpp, path references
// 0x4569a3-0x461bcd; the class is living_entity, constructor living_entity__living_entity 0x44e060).
// Not original source. Layout: symbols/types.h Character (0x788 bytes), on top of obj (objects.h), with
// the +0x324 / +0x558 sub-objects typed (ai_state, living_combat) and the thiscall methods the pilots
// call. The stubs' Character copies and the Character_M shims are merged here (DECISIONS.md D1).
// decomp37 (game/living_entity.cpp): the unit's members, the status slot record, the fields at +0x670,
// +0x69c and +0x6c0 typed, the unit's globals (D1700-D1712).
#ifndef ENGINE_OBJECTS_LIVING_ENTITY_H
#define ENGINE_OBJECTS_LIVING_ENTITY_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/ai/ai.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/s3d/s3d.h"               // s3d_vis_set (layer_lobj_list, D2202)

class Character;
class character_instance;
struct navpoint;
struct spell_hit;
class console_command;

// The living_combat sub-object at Character +0x558 is declared in engine/combat/combat.h with its
// fields (combat.cpp, decomp23; DECISIONS.md D300).

#define NUM_LOCO_ANIMS 18             // locomotion_anim_names, Character +0x5ac..+0x5f0

// Character +0x320 flags
#define CF_COMBAT_READY  0x1          // ready stance (living_entity_init: stand == ready or multiplayer; decomp37, corrected D9201)
#define CF_SUMMONLIKE    0x2          // cleared by party_add_member (party_cycle skips it); set while disabled
#define CF_HIDDEN        0x8          // dying (character_kill) or hidden
#define CF_DEAD          0x80
#define CF_HURT          0x4000       // took damage (character_add_hp; decomp37)
#define CF_ON_GROUND     0x100000     // the last floor search succeeded (character_find_ground; decomp37)
#define CF_LEFT_PARTY    0x800000
#define CF_HEAD_LOOK_DONE 0x4000000   // character_update_head_look ran (decomp37)
#define CF_ANIM_LOD      0x8000000    // character_anim_lod_skip: low-LOD frame skipping this frame
#define CF_ANIM_LOD_PREV 0x10000000   // ... and the previous frame
#define CF_NO_GROUND     0x80000000   // character_find_ground does nothing (decomp37)

// Status effect ids: Character.status index (status_effect_add 0x454700; names from strings.tbl
// 256-273). 7, 8 and 14-17 show no text. decomp37: the slots are 18 and every function walks all of
// them (the old note "the id must be <= 0x10" only describes status_effect_add's immunity switch).
enum status_effect_id {
    STATUS_BLIND = 0, STATUS_SILENCE = 1, STATUS_FAITH = 2, STATUS_CURSE = 3, STATUS_BLESS = 4,
    STATUS_REGEN = 5, STATUS_POWER = 6, STATUS_LIGHT = 7, STATUS_INVISIBLE = 8, STATUS_SLEEP = 9,
    STATUS_PARALYZE = 10, STATUS_POISON = 11, STATUS_CRIPPLE = 12, STATUS_SLOW = 13,
    STATUS_FIREWALL = 14, STATUS_BLIZZARD = 15, STATUS_ICE_COFFIN = 16, STATUS_DARK_WALL = 17,
};
#define NUM_STATUS_EFFECTS 18

// One status slot (0x20 bytes, Character +0xac; decomp37, D1703). The constructor of Character builds
// the two GameTimers of each (the loop 0x44e0a6, stride 0x20). Names ours.
struct status_effect {
    int kind;                         // +0x00 -1 none, 0 timed, 1 held by an item, 2 permanent (cure needs force)
    GameTimer timer;                  // +0x04 duration (1,000,000 ms for item statuses)
    int fx;                           // +0x08 effect handle, -1 none (character_status_set_8)
    int source;                       // +0x0C the inflicting character's handle
    union {
        int spell;                    // +0x10 the spell (kinds 0 / 2)
        item_info *item;              //       the item (kind 1; character_refresh_item_status_effects)
    };
    GameTimer tick_timer;             // +0x14 periodic effect (regen, poison, firewall, ...)
    int tick_ms;                      // +0x18 status_effect_duration
    float value;                      // +0x1C amount (character_status_value / _set_1c)
};
LAYOUT_CHECK(status_effect, sizeof(status_effect) == 0x20);

// The skill block copied whole from char_info +0x94 (character_scale_stats_to_level: rep movsd 0x15).
struct skill_levels {
    ubyte level[MAX_SKILLS];
};

// The locomotion blend of character_update_locomotion_anim (0x14 bytes on its stack), the record the
// boss remap hooks rewrite (boss_anim_remap_in / _out take its address). Names ours.
struct loco_blend_set {
    int count;
    int anim[2];
    float weight[2];
};

// A character (party member, NPC or monster), 0x788 bytes (the living-entity pool stride).
class Character : public obj {
public:
    // living_entity.cpp, in address order (decomp37). The older declarations below keep their text.
    Character();                      // 0x44e060 living_entity__living_entity
    ~Character();                     // 0x44e150 living_entity__dtor (not virtual)
    void  start_spell_recovery(int spell, int extra, int ms);   // 0x44dc70 character_start_spell_recovery
    void  scale_stats_to_level();     // 0x44ddb0 character_scale_stats_to_level
    void  init(int type, int info_index, const vector3 *pos, const matrix *orient, bool unused,
               int level);            // 0x44e1c0 living_entity_init (ret 0x18)
    virtual void deinit();            // 0x44edf0 living_entity__vfunc0 (obj vtable slot 0)
    void  place_at_party_start(int slot);         // 0x44f1a0 character_place_at_party_start
    void  place_at_marker(navpoint *marker);      // 0x44f5f0 character_place_at_marker
    bool  has_line_of_sight_to_point(vector3 *point);   // 0x44f880
    bool  has_line_of_sight_to(Character *target);      // 0x44fa20
    bool  kill(int death_sub);        // 0x450d90 character_kill
    void  monster_drop_loot();        // 0x450e30
    void  update_head_look();         // 0x4530d0 character_update_head_look
    void  summon_return_lent_max_hp();// 0x4536c0
    void  on_death();                 // 0x4537f0 character_on_death
    void  set_team_mask(int team, unsigned int mask);   // 0x454640
    void  set_flag_2_4(int arg);      // 0x454670
    void  clear_flag_2_4();           // 0x4546b0
    void  status_effect_add_from_item(int status, item_info *item, float value);   // 0x454ff0
    bool  status_active(int status);  // 0x455990 character_status_active
    int   status_time_left(int status);   // 0x4559e0
    void  status_set_1c(int status, float value);   // 0x455a50 character_status_set_1c
    void  status_set_8(int status, int fx);         // 0x455a70 character_status_set_8
    void  refresh_item_status_effects();            // 0x455b20
    void  ai_travel_offset(vector3 offset);         // 0x455e00 character_ai_travel_offset
    void  find_ground();              // 0x455fb0 character_find_ground
    void  update_layer();             // 0x4568e0 character_update_layer
    bool  find_overlap(vector3 *move, int unused, bool relative);   // 0x4569c0
    float anim_speed_at_blend(float blend);         // 0x456ed0
    void  get_forward_velocity(vector3 *out);       // 0x456fb0
    float get_speed();                // 0x457060 character_get_speed
    void  boss_anim_remap_in(int *anims);           // 0x4570f0 character_boss_anim_remap_in
    void  boss_anim_remap_out(int *anims);          // 0x457120 character_boss_anim_remap_out
    float distance_to_reach_blend(float blend);     // 0x45a930
    float distance_to_reach_speed(float speed);     // 0x45aa20
    void  update_status_effects();    // 0x45b6e0 character_update_status_effects
    void  update_regen_and_status();  // 0x45bf70 character_update_regen_and_status
    int   ai_move_frame(int unused, vector3 *plane_normal);   // 0x45c340 character_ai_move_frame (ret 8)
    void  find_foot_bones();          // 0x45d4c0
    void  play_footstep(vector3 *at, bool left_foot);   // 0x45d540
    bool  get_attack_range(float *out);             // 0x45d830 character_get_attack_range
    bool  can_cast_spell(int spell);  // 0x45d980 character_can_cast_spell
    int   perform_spell_cast(int spell, bool arg);  // 0x45da60 character_perform_spell_cast (anim or -1)
    void  play_flinch(int variant);   // 0x45de20 character_play_flinch
    void  spell_apply_hit_effects(spell_hit *h);    // 0x460190 spell_apply_hit_effects (thiscall)
    float move_progress(int anim);    // 0x4613f0 character_move_progress
    bool  move_has_trigger(int trigger);            // 0x461620
    void  move_trigger_mark(int trigger);           // 0x4616f0
    bool  is_badly_hurt();            // 0x4617f0 character_is_badly_hurt
    int   get_move_effect();          // 0x461840 character_get_move_effect (+0x60c)
    bool  is_enemy(Character *c);     // 0x461ad0 character_is_enemy
    int   get_team();                 // 0x461b60 character_get_team
    // the optimised tail 0x461dc0-0x46353f (decomp37)
    void  say(const char *text, float seconds);     // 0x461dc0 character_say
    void  set_floating_text(const char *text, float seconds, ubyte r, ubyte g, ubyte b);   // 0x461fe0
    void  set_speed_scale(float speed, bool now);   // 0x462260 character_set_speed_scale
    ubyte get_move_fade_alpha();      // 0x4622b0 the prop alpha
    int   is_main_hero();             // 0x462380 character_is_main_hero
    float skill_equip_mod(int skill); // 0x462540 character_skill_equip_mod
    float ap_regen_mult();            // 0x462810 character_ap_regen_mult
    void  apply_hero_model(int heavy);// 0x4628a0 character_apply_hero_model
    void  set_underwear_textures(int part, const char *texture);   // 0x462a20
    void  set_armour_alt_texture(bool on, bool force); // 0x462c80
    bool  is_mount();                 // 0x462d60 character_is_mount
    bool  is_rider();                 // 0x462d70 character_is_rider
    bool  ai_suspended();             // 0x462de0 character_ai_suspended

    // living_entity.cpp (earlier pilots)
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
    // Returns the hero model render mask (an int); the type is kept for the pilots' use (D1711).
    const char *hero_model_prefix();  // 0x462ec0 character_hero_model_prefix
    // decomp20 (conversationctl.cpp). Argument lists from its call sites.
    float size_radius();              // 0x4542b0 character_size_radius
    float size_height();              // 0x4542f0 character_size_height
    float distance_between_edges(Character *other, bool a, int b);  // 0x454330 (the calls pass false, -1)
    // decomp37: (status, force, restore_fx)
    void  status_effect_cure(int status, bool force, bool restore_fx);   // 0x4553a0 (conversation_begin: 8, 1, 0)
    void  walk_to_point(vector3 pos, int priority);       // 0x455e70 character_walk_to_point (by value)
    void  ai_add_goal9(vector3 *pos, int layer, int priority, bool a);   // 0x455f50 character_ai_add_goal9
    float stopping_distance(vector3 *out_vel);            // 0x45a840 character_stopping_distance (NULL)
    bool  clip_move_to_navmesh(vector3 *move);            // 0x45ab00 character_clip_move_to_navmesh
    // The move record is passed as an int (D303's text kept; living_entity.cpp casts too).
    int   play_move(int move, float speed, bool keep, bool clear_actions, bool held);   // 0x460f20
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
    void  set_ai_active(int on);      // 0x462200 character_set_ai_active (a dword test, D2302)
    void  add_xp(int xp, bool quiet, bool from_net);   // 0x44fab0 character_add_xp (conversation_execute_action: xp, 0, 0)
    void  update_locomotion_anim();   // 0x45a120 character_update_locomotion_anim
    void  set_locomotion_speed(float speed);  // 0x45a6e0 character_set_locomotion_speed
    move_info *get_current_move();    // 0x461500 character_get_current_move (NULL = none)
    int   spell_on_cooldown(int spell);// 0x44dd30 character_spell_on_cooldown
    void  add_ap(float delta);        // 0x450c70 character_add_ap
    bool  is_in_view(float radius_scale); // 0x44f020 character_is_culled
    void  move_frame();               // 0x45cd00 living_entity_move_frame
    void  update_footsteps();         // 0x45d690 character_update_footsteps
    // decomp23 (combat.cpp, spells.cpp). Argument lists from the call sites; decomp37 types.
    bool  add_hp(float delta, int source, int a, bool from_net);   // 0x450850 character_add_hp -> still alive
    void  status_effect_add(int status, int ms, int spell, float value, int source, bool permanent);   // 0x454700
    float status_value(int status);                       // 0x455a30 character_status_value
    // decomp37: (spell, source handle, effect position, orientation, source type, triggering item type,
    // power, preloaded effect); the callers pass 0 for the pointers.
    void  get_hit_by_spell(int spell, int source, vector3 *pos, matrix *orient, int source_type, ItemType *item,
                           float power, int preloaded);   // 0x45dea0 living_entity_get_hit_by_spell (preloaded: an int, D25504)
    move_info *find_move(int category, int id);           // 0x461370 character_find_move
    // (spell, flag): the cast move, effect, missile, recovery and AP cost; -1 when the move is missing.
    // mp_handle_cast_anim passes the CST_ANM byte as the flag (decomp35).
    move_info *find_move_alt(int category, int id);       // 0x4613b0 character_find_move_alt
    bool  move_playing(int anim, float *t);               // 0x461450 character_move_playing (t may be 0)
    float move_trigger_time_left(int trigger);            // 0x461670 character_move_trigger_time_left
    int   move_trigger_fired(int trigger, bool latch);    // 0x461740 1 once when passed, 0 before, -1 no move/trigger
    // objects.cpp
    bool  anim_lod_skip();            // 0x46ad10 character_anim_lod_skip
    void  process_anim(float frametime);  // 0x46adf0 character_process_anim
    void  fill_locomotion_anims();    // 0x46af70 character_fill_locomotion_anims
    // decomp29 (boss_ai.cpp)
    Character *get_mount();           // 0x462d20 character_get_mount (+0x6e0 when that entity has +0x6e4 bit 1)
    Character *get_rider();           // 0x462d40 character_get_rider (+0x6e0 when that entity has +0x6e4 bit 0)
    float get_698();                  // 0x456f80 character_get_698 (1.0 or forward_speed)
    // decomp35 (gametarget.cpp):
    // (pos, out push-out direction, relative): true when another living entity's cylinder overlaps pos.
    // decomp34 (level_script_khosani_lab.cpp, level_script_worldmap.cpp; D1404)
    // decomp39 (ai.cpp). Argument lists from its call sites.

    int team;                         // +0x098
    float move_start_time;            // +0x09C level_session_time when the move started (decomp37: a float copy)
    ubyte trigger_latch[3];           // +0x0A0
    ubyte _padA3;                     // +0x0A3
    int death_move_sub;               // +0x0A4
    move_info *death_move;            // +0x0A8
    status_effect status[NUM_STATUS_EFFECTS];   // +0x0AC status effects by status_effect_id
    GameTimer timer2ec;               // +0x2EC AP regeneration (7.5 s)
    unsigned int enemy_team_mask;     // +0x2F0
    char name[0x20];                  // +0x2F4
    character_instance *instance;     // +0x314 character instance (vsdk ca)
    Character *live_next;             // +0x318 living-entity list
    Character *live_prev;             // +0x31C
    unsigned int flags;               // +0x320 CF_*
    ai_state ai;                      // +0x324
    // +0x558. One name since D19000 (new_assess.cpp's `protection` alias of D1 dropped), so the member
    // has its constructor (0x4681b0), called between ai and timer6d4 as in the binary.
    living_combat combat;
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
    ubyte skill[0x5c];                // +0x614 by skill id, value * 0.01 (character_get_skill)
    int light;                        // +0x670 attached light handle, -1 none (decomp37, D1704)
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
    vector3 velocity;                 // +0x69C
    float loco_blend;                 // +0x6A8
    float target_speed;               // +0x6AC
    float slowdown_rate;              // +0x6B0
    float run_speed;                  // +0x6B4
    float walk_speed;                 // +0x6B8
    float speed_scale;                // +0x6BC
    vector3 _vec6c0;                  // +0x6C0
    int cur_anim;                     // +0x6CC move serial (move_play_serial), -1 none
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

// The living-entity lists link through live_next / live_prev (+0x318); the code has vsdk.h's
// list_append / list_remove shape with those names (do-while (0); decomp37, names ours).
#define live_list_append(head, elem) do { (elem)->live_prev = (head)->live_prev; (elem)->live_next = (head); \
                                          (head)->live_prev->live_next = (elem); (head)->live_prev = (elem); } while (0)
#define live_list_remove(elem) do { (elem)->live_prev->live_next = (elem)->live_next; \
                                    (elem)->live_next->live_prev = (elem)->live_prev; \
                                    (elem)->live_next = NULL; (elem)->live_prev = NULL; } while (0)

bool character_is_locally_controlled(Character *c);   // 0x475780 (decomp58: unnamed_goals_doors_0x475180.cpp)

extern Character living_list;         // 0x8eca60 sentinel (sum.symbols' living_list_first 0x8ecd78 = live_next)
extern Character living_list_inactive; // 0x8ed1e8 second sentinel (live_next 0x8ed500), walked by
                                      // living_entities_move_all
#define MAX_LIVING_ENTITIES 175       // 0xaf (eh vector constructor count; living_entity_create)
extern Character living_entity_pool[MAX_LIVING_ENTITIES];   // 0x8ed980
extern char *locomotion_anim_names[NUM_LOCO_ANIMS];   // 0x599648 "stand", "norm_walk", ... "statscreen"
// Per char_info size (+0x90): character_size_height / character_size_radius and charinfo_boss_height /
// _radius (boss_ai.cpp, decomp29). The heights {0.9, 1.8, 2.5, 99.9} are characterinfo.cpp's .rdata (name
// ours), the radii {0.15, 0.47, 1.0, 99.9} .data of the AI think unit by address (defined there), not
// living_entity.cpp's.
extern const float character_size_heights[4];          // 0x5818d8
extern float character_size_radii[4];                  // 0x59d7c8
// Default enemy team mask per team {0xc, 0, 0xb, 7} (character_set_team; .rdata before the vtable).
extern const unsigned int team_enemy_masks[4];         // 0x581904

// Speech bubbles (character_say 0x461dc0, counted down by objects_do_frame). Type name ours.
struct character_speech_record {
    char text[0x100];                 // +0x000
    int speaker;                      // +0x100 character handle, -1 free
    float time_left;                  // +0x104 seconds
};
LAYOUT_CHECK(character_speech_record, sizeof(character_speech_record) == 0x108);
#define MAX_CHARACTER_SPEECH 16
extern character_speech_record character_speech[MAX_CHARACTER_SPEECH];   // 0x940438
extern int character_speech_times[3];  // 0x599698 {250, 312, 400} line width by resolution_index (label ours)

// Per-character spell cooldowns (spell_cooldown_start 0x44dbd0, ticked by objects_do_frame). Type name ours.
struct spell_cooldown {
    int handle;                       // +0x0 character handle, -1 free
    int spell;                        // +0x4
    GameTimer timer;                  // +0x8
};
LAYOUT_CHECK(spell_cooldown, sizeof(spell_cooldown) == 0xc);
#define MAX_SPELL_COOLDOWNS 100
extern spell_cooldown spell_cooldowns[MAX_SPELL_COOLDOWNS];   // 0x93ff78
extern GameTimer spell_global_timers[108];                    // 0x941514 one per spell (MAX_SPELLS)

// The move fades of the props of party casters (character_start_move_fade, character_get_move_fade_alpha;
// 5 x 0xc at 0x9416c8). sum.symbols types them int[15]; the third dword is stored with fstp. Names ours.
struct move_fade {
    int handle;                       // -1 free
    int move;                         // the move serial
    float rate;                       // fade per second (0.5 / animation length)
};
#define MAX_MOVE_FADES 5
extern move_fade move_fades[MAX_MOVE_FADES];          // 0x9416c8
extern int move_play_serial;                          // 0x941708

// The per-layer floor object lists (level_load_layers; character_find_ground): level_load_layers
// stores the "$layerNN" vis sets of the level there (D2202). Type name ours, kept as a typedef.
typedef s3d_vis_set layer_lobj_list;
extern layer_lobj_list *level_layer_ground[20];       // 0x9414b8 (in living_entity.cpp's .bss)

// Console toggles in living_entity.cpp's .bss (decomp23: spell_roll_amount reads No_resist).
extern bool Players_immortal;         // 0x941704 "immortal"
extern bool No_resist;                // 0x941705 "no_resist"
extern bool Bloodspots;               // 0x94173c "bloodspots"
extern bool Debug_death;              // 0x59a16c "death" (init 1; 0 = nothing dies)
extern float Chance_to_drop_gold;     // 0x599690 "chance_gold" (0.25)
extern bool Use_spell_ap;             // 0x599694 "Spell_ap" (init 1)
extern int Character_los_enabled;     // 0x599644 (init 1; label ours)
extern int hero_model_prefix_table[4];// 0x5996a4 {6, 6, 2, 2} (label ours)
extern console_command ccmd_immortal;     // 0x8ed970
extern console_command ccmd_health;       // 0x941508
extern console_command ccmd_chance_gold;  // 0x8eca30
extern console_command ccmd_kill_summon;  // 0x8eca50
extern console_command ccmd_no_resist;    // 0x940428
extern console_command ccmd_Spell_ap;     // 0x8eca40
extern console_command ccmd_bloodspots;   // 0x941730
extern console_command ccmd_death;        // 0x941710
void cmd_immortal();                  // 0x461890
void cmd_health();                    // 0x4619b0
void cmd_chance_gold();               // 0x463080
void cmd_kill_summon();               // 0x463140
void cmd_no_resist();                 // 0x4631b0
void cmd_Spell_ap();                  // 0x4632a0
void cmd_bloodspots();                // 0x463390
void cmd_death();                     // 0x463480

void spell_cooldown_start(int spell, Character *c, int seconds);   // 0x44dbd0
void ring_add_xp(const char *ring, int xp, int cap);  // 0x44ffc0
void ring_check_summon_unlock(const char *ring, int old_xp, int new_xp);   // 0x450220
void rings_add_xp_all(int xp, int cap);               // 0x450420
bool character_probe_lobj_ground(unsigned int flags, int lobj_index, vector3 *pos, float *best_y,
                                 short *face, short *lobj_out, vector3 *hit_pos, bool use_face);   // 0x456750 (cdecl)
void character_update_render_flag(Character *c);      // 0x45ccb0 (cdecl)
void character_start_move_fade(Character *c, int move);   // 0x45dd80 (cdecl)
void living_entity_reset_tables();    // 0x462d80
int  character_save_state_size();     // 0x4587f0 0x17c, the fixed LENT record size (decomp27)
#define CF_SUMMON 0x100               // +0x320: the summon (savegame LENT code, level_scripts.cpp; decomp27)

// decomp37: arguments 5 and 9 are bytes (the init flag and "placed": no navigation-mesh snap).
Character *living_entity_create(int type, int char_info, const vector3 *pos, const matrix *orient,
                                bool a, int layer, int level, int unused, bool placed);   // 0x461b80
void character_xp_death_penalty(Character *c);        // 0x441710 (defined in the net session unit, game/unnamed_netsession_0x440b40.cpp; decomp53)
// Stops or attaches the looping effect of a status (unit not known; conversation_begin passes the
// character's handle, Character +0x80; decomp20). Returns the effect handle (status_effect_add keeps it).
// decomp64 (game/unnamed_fx_0x489c40.cpp): the second argument becomes the effect's kind (the callers pass
// 5 and 6, the fading kinds of fx_frame); -1 while a stone effect (kind 10) is on the character.
int  status_effect_update_fx(int handle, int status); // 0x48c470

#endif
