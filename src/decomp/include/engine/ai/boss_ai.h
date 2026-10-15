// engine/ai/boss_ai.h - boss hooks (boss_ai.cpp in sum.symbols, 0x4a04b0-0x4a3580, path string 0x4a1812
// "D:\projects\Summoner\pccode\Engine\ai\boss_ai.cpp") and the per-boss units around it whose file names
// are not known (module "-" in sum.symbols; docs/sum-exe/bosses.md gives their code ranges). Not original
// source. Declared for combat.cpp and spells.cpp (decomp23) and written out for boss_ai.cpp (decomp29); the
// argument lists are from the call sites. The thiscall dispatchers and per-boss AI functions are members
// of ai_state (engine/ai/ai.h).
#ifndef ENGINE_AI_BOSS_AI_H
#define ENGINE_AI_BOSS_AI_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"

class Character;
class living_combat;
class vector3;
struct move_info;
struct char_info;

// Boss species (char_info +0x10), as the dispatchers switch on them. Names ours, from the per-boss
// function names in sum.symbols and the species table of bosses.md (decomp29).
#define SPECIES_LUMINAR         0x51
#define SPECIES_BOSS_MOUNT      0x55  // the riders' mount (boss_ensure_mount creates one when none is free)
#define SPECIES_TIGER_RIDER     0x57
#define SPECIES_PHOENIX_RIDER   0x58
#define SPECIES_SERPENT_RIDER   0x59  // also the serpent parts (illusion copies)
#define SPECIES_GHOST_RIDER     0x5a
#define SPECIES_TENTACLE_BEAST  0x93
#define SPECIES_GIANT_SALAMANKA 0xac
#define SPECIES_EVIL_URATH      0xb1
#define SPECIES_MUROD           0xb7
#define SPECIES_MUROD_ONI       0xb8
#define SPECIES_TITUS           0xc1
#define SPECIES_PYRUL           0xc4
#define SPECIES_MACHIVAL        0xc5
#define SPECIES_EVIL_JOSEPH     0xc9

// One "$Offense point:" record of a *_boss.tbl (boss_ai_parse_table; 0x20 bytes, 10 per stage). The
// bosses' current-move pointers (phoenix_cur_attack, titus_current_move, ...) point at these. Type and
// field names ours (decomp29).
#define BOSS_MAX_OFFENSE_POINTS 10
#define BOSS_ATTACK_TRIP        0x01  // +Trip:
#define BOSS_ATTACK_PUSH        0x02  // +Push:
#define BOSS_ATTACK_PROJECTILE  0x04  // +Projectile:
#define BOSS_ATTACK_SPELL_ONLY  0x08  // +Spell Only: (boss_cast_special_dispatch, serpent parts)
#define BOSS_ATTACK_PLUS_SPELL  0x10  // +Plus Spell: (boss_counter_hit_dispatch)
struct boss_attack {
    int offense_point;                // +0x00 atoi of the quoted point name, minus 1
    float range_min;                  // +0x04 $Range: first value
    float range_max;                  // +0x08 $Range: second value (boss_move_target_in_range)
    float cos_max_angle;              // +0x0C cos($Max angle: degrees)
    int enabled;                      // +0x10 1 for every parsed record
    int spell;                        // +0x14 +Spell: (spell_find_by_name), -1 none
    int variation;                    // +0x18 $Variation: (variation_names index; the attack move id)
    unsigned int flags;               // +0x1C BOSS_ATTACK_*
};
LAYOUT_CHECK(boss_attack, sizeof(boss_attack) == 0x20);

// A boss threat record (0x18 bytes, 5 per boss; symbols/types.h boss_threat). The amounts are HP deltas
// (negative for damage; character_add_hp passes its delta), so the list pick subtracts them.
#define BOSS_MAX_THREATS 5
struct boss_threat {
    int handle;                       // +0x00 attacker handle, -1 free
    float threat[4];                  // +0x04 [0], [1] and [3] get every amount, [2] only when the flag is 0;
                                      //       boss_threat_decay scales [0] by 0.1 and [1] by 1/60 per second
    GameTimer timer;                  // +0x14 20 s from the last amount
};
LAYOUT_CHECK(boss_threat, sizeof(boss_threat) == 0x18);

// A candidate of boss_target_list_build (0x10 bytes on the per-boss callers' stacks). Name ours.
struct boss_target {
    Character *character;             // +0x00
    float dist;                       // +0x04 edge distance from the boss
    float score;                      // +0x08 cumulative weight (boss_target_list_pick)
    unsigned int flags;               // +0x0C set by the per-boss filters (boss_target_filter_dispatch)
};
LAYOUT_CHECK(boss_target, sizeof(boss_target) == 0x10);

// One "$OffenseNN" mesh tag of a rider boss (0x1c bytes; serpent_boss_load / serpent_choose_attack and the
// phoenix's copies; decomp54). Names ours.
struct serpent_offense_point {
    int tag;                          // +0x00 character_find_mesh_tag
    vector3 up;                       // +0x04 the tag's up vector this frame
    vector3 pos;                      // +0x10 the tag's position + the boss's centre offset
};

// ---- boss_ai.cpp -----------------------------------------------------------------------------------
extern GameTimer boss_rider_sound_timer;              // 0x245c9e0 (name from static_init_boss_rider_sound_timer)

bool  boss_cast_special_dispatch(Character *c, int anim);                // 0x4a0b40
void  boss_attack_trigger_dispatch(Character *boss, Character *target);  // 0x4a0ca0
bool  boss_move_target_in_range(Character *boss, Character *target, boss_attack *move);   // 0x4a0e60
bool  boss_move_in_range_dispatch(Character *boss, Character *target);  // 0x4a0ea0
bool  boss_attack_stuns_dispatch(int species);         // 0x4a1090
bool  boss_attack_knocks_down_dispatch(int species);   // 0x4a1150
boss_threat *boss_get_threat_table(Character *boss);   // 0x4a1200 NULL for bosses without one
// (boss, amount, attacker handle, flag): sum.symbols has "(boss, attacker, amount, melee)", the body and
// character_add_hp's pushes (delta, source, a) give this order (decomp29).
void  boss_threat_add(Character *boss, float amount, int attacker, int flag);   // 0x4a13b0
bool  species_is_boss(int species);                    // 0x4a14f0
float charinfo_boss_height(char_info *info);           // 0x4a15a0
float charinfo_boss_radius(char_info *info);           // 0x4a16a0
// (filename, stages, table, offense points): the second and fourth arguments are not read.
void  boss_ai_parse_table(const char *filename, int num_stages, boss_attack table[][BOSS_MAX_OFFENSE_POINTS],
                          int num_offense_points);     // 0x4a1790
bool  boss_stage_attack_enabled_dispatch(int species, int index);   // 0x4a1ac0
void  boss_anim_remap_in(Character *c, int *anims);    // 0x4a1b70 (character_boss_anim_remap_in's argument)
void  boss_anim_remap_out(Character *c, int *anims);   // 0x4a1be0
int   charinfo_index_from_species(int species);        // 0x4a1d90 index or -1 (copy of charinfo_find_by_species)
bool  boss_ensure_mount(Character *boss);              // 0x4a1dc0
void  boss_mount_pick_flee_point(Character *mount, vector3 *out);   // 0x4a1ef0
void  boss_release_mount(Character *rider);            // 0x4a1fe0
int   character_target_point_count(Character *c);      // 0x4a21b0
// (attacker, defender) as living_combat_resolve_hit pushes them: the attacker is the boss, and the
// defender gets the move's +Plus Spell: (sum.symbols "(boss, attacker)" and bosses.md "counter hit" read
// it the other way round; decomp29).
void  boss_counter_hit_dispatch(Character *attacker, Character *defender);   // 0x4a2230
void  boss_set_ride_anims(Character *c);               // 0x4a2300
void  boss_script_hook_stub(Character *c);             // 0x4a2360
void  boss_set_foot_anims(Character *c);               // 0x4a23c0
void  boss_dismount_dispatch(Character *c);            // 0x4a2420
bool  boss_species_mounted_stage(int species);         // 0x4a2480
bool  species_is_boss_b(int species);                  // 0x4a24c0
void  boss_ai_do_frame_hook(Character *c);             // 0x4a2560
void  boss_clear_current_move(Character *c);           // 0x4a2590
bool  boss_get_attack_range(Character *c, float *out); // 0x4a2870
bool  serpent_part_on_hit(Character *c);               // 0x4a28b0 (true: the hit is taken by a serpent part)
bool  character_is_serpent_part(Character *c);         // 0x4a28f0
bool  boss_attack_test_dispatch(living_combat *lc);    // 0x4a2920
bool  boss_attack_resolve_dispatch(living_combat *lc); // 0x4a2b80
bool  character_is_not_boss(Character *c);             // 0x4a2d40 type != 9
bool  boss_move_trigger_dispatch(Character *c);        // 0x4a2d50
bool  character_uses_pathfinding(Character *c);        // 0x4a2d70
bool  boss_hook_dispatch_51_c1(Character *boss, Character *target);  // 0x4a2da0
int   boss_threat_find(int handle, boss_threat *table);  // 0x4a2de0 index or -1
// decomp54: the copy of boss_target_list_find after the evil_urath unit (the serpent's calls; unit not
// known): (list, count, handle) -> index or -1.
int   boss_target_list_find_b(boss_target *list, int count, int handle);   // 0x4c4580
int   boss_target_list_find(boss_target *list, int count, int handle);     // 0x4c3c70 (the phoenix's calls)
void  boss_target_filter_dispatch(Character *boss, boss_target *t);  // 0x4a2e00
int   boss_target_list_build(Character *boss, boss_target *list, boss_threat *table);   // 0x4a2f60 count
// mask 0x80000000: the nearest candidate; otherwise a weighted random pick among the candidates whose
// flags share a bit with the mask (mask 0: all). cur_target gets x1.2, or 0 when exclude_cur is set.
int   boss_target_list_pick(boss_target *list, int count, boss_threat *table, int cur_target,
                            bool exclude_cur, unsigned int mask);   // 0x4a31c0 handle or -1
void  rider_unlink_mount(Character *rider);            // 0x4a33b0
int   rand_int_range(int lo, int hi);                  // 0x4a3410 lo + (int)(frand() * (hi - lo))
extern int pyrul_missile_bone;                         // 0x247fd34 (character_perform_spell_cast; decomp37)
int   random_stun_duration();                         // 0x4b9e50 rand_int_range(3000, 7000) (unit not known)
int   rand_weighted_index(float *weights, int n);      // 0x4a3440 index or -1 (n <= 10)
void  boss_rider_sound_timer_reset();                  // 0x4a34d0 (state_game_enter)
void  boss_rider_random_sound(vector3 *pos);           // 0x4a34e0
void  level_preload_effects();                         // 0x4a3570 (level_load_files)

// ---- per-boss units (files not known) ----------------------------------------------------------------
move_info *phoenix_select_ranged_move(Character *c);   // 0x499a90 (phoenix unit, file not known)
void  tiger_restore_ride_anims(Character *rider);      // 0x49c750 (tiger unit, file not known)

// The ghost rider's stage (ghost_update_stage writes 1..3; written by ghost_rider_boss_load too).
// spell_compute_resist gives spell 0x50 (Ghost Paralyzing Fear) a 0.5 hit chance in stage 3. Name ours;
// its .bss belongs to the ghost rider unit.
extern int ghost_stage;                                // 0x2480310

// decomp29: the cdecl per-boss functions boss_ai.cpp calls (the thiscall ones are ai_state members).
// "Folded" ones are empty bodies (or a bare "return false") that identical-code folding merged into
// empty_function (summoner.h) or ls_test_use; their names are ours.
void  luminar_free_bitmap();                           // 0x4c8fd0
void  tiger_unload();                                  // 0x49ae90
void  giant_salamanka_free_bitmap();                   // 0x4c8540
void  serpent_unload();                                // 0x495cf0
void  ghost_unload();                                  // 0x4ba8d0
void  phoenix_unload();                                // 0x4989d0
void  tentacle_beast_unload();                         // 0x4c7050
void  evil_joseph_unload();                            // 0x4c37f0
void  titus_unload();                                  // folded (boss_ai_unload case 0xc1)
void  pyrul_unload();                                  // folded (case 0xc4)
void  murod_oni_unload();                              // folded (case 0xb8)
void  murod_unload();                                  // folded (case 0xb7)
void  evil_urath_unload();                             // folded (case 0xb1)
void  machival_unload();                               // folded (case 0xc5)
void  ghost_revive_minions();                          // 0x4bbc70

// decomp43: the boss calls of the level scripts (jadetemple, liangshan, templeint2, sewers). The set_solo
// functions take the rider (script_object_find_character's result, pushed unchecked): rider_unlink_mount,
// combat flag on (the cmd_*_solo console commands do the same).
void  tiger_set_solo(Character *rider);                // 0x49c7c0
void  serpent_set_solo(Character *rider);              // 0x497a40
void  phoenix_set_solo(Character *rider);              // 0x499dd0
void  ghost_set_solo(Character *rider);                // 0x4bbd80
void  ghost_minions_deactivate();                      // 0x4bbdd0
void  evil_joseph_destroy_effects();                   // 0x4c40d0
void  tentacle_beast_battle_update();                  // 0x4c7640
// decomp46 (level_script_temple_inside.cpp): the Tiger Rider's handshake with the TempleInt script. The
// rider is the last living entity of species 0x57 (char_info +0x10), NULL when none; ls_templeint_frame
// reads its HP through the result unchecked.
Character *tiger_find_rider();                         // 0x49a010
// decomp54 (D3403): tiger_dismount_stage_reached (0x2452a98, set by tiger_check_stage), tiger_stage_trigger
// (0x2452a99, set by the script) and tiger_combat (0x2452a9a) are fields of the tiger's state object,
// macros in engine/ai/unnamed_tiger_0x499e50.h (included at the end of this header).
// One tentacle of the sewer boss (0x110 bytes, 4 at 0x248c328; sum.symbols types the array
// uint8[0x440]). decomp59 (D3903): the full layout from the tentacle unit (game/unnamed_tentacle_beast_
// 0x4c6480.cpp); init_tentacle_records constructs the five slots and the five GameTimers. Type and field
// names ours.
struct tentacle_slot {
    tentacle_slot();                                   // 0x4c64f0 tentacle_slot__ctor (constructs timer)
    int handle;                                        // +0x00 -1 free (a held character)
    int _unk04[4];                                     // +0x04 not used
    GameTimer timer;                                   // +0x14 not used
};
struct tentacle_info {
    tentacle_slot slots[5];                            // +0x000 -1 at load
    int _unk078;                                       // +0x078
    move_info *move;                                   // +0x07c the chosen attack (tentacle_move_1 / _2 / _86), NULL none
    Character *character;                              // +0x080 (ls_sewer_tb_intro_cs_end, 0x4ee2dc)
    unsigned int flags;                                // +0x084 1 submerged, 2 rising, 4 sinking
    int splash_fx;                                     // +0x088 "splash.vfx" of the last dive / surfacing
    int target;                                        // +0x08c the chosen attack's target handle
    int num_hits;                                      // +0x090 tentacle_collect_area_hits
    int hits[4];                                       // +0x094 (a fifth hit would overwrite death_anim)
    int death_anim;                                    // +0x0a4 the death move (6, 0x22), -1 none
    GameTimer think_timer;                             // +0x0a8 2.5-5 s between choices
    vector3 pickup_pos;                                // +0x0ac "$tag-26-pickup" at load (not read)
    int grab_tag;                                      // +0x0b8 "$tag-grab-point"
    int victim;                                        // +0x0bc held victim, -1 none (tentacle_begin_sink)
    GameTimer timer_c0;                                // +0x0c0 constructed, never used
    GameTimer hold_timer;                              // +0x0c4 30 s, then the release
    GameTimer squeeze_timer;                           // +0x0c8 the victim's 3 HP every 6 s
    int thrown;                                        // +0x0cc the released victim in flight, -1 none
    vector3 throw_velocity;                            // +0x0d0
    float throw_gravity;                               // +0x0dc
    float release_hp;                                  // +0x0e0 the victim's HP * 0.75 at the grab
    float reach;                                       // +0x0e4 distance to "$tag-27-swipe" at load
    vector3 swipe_pos;                                 // +0x0e8 "$tag-27-swipe" (world) at load
    vector3 squeeze_pos;                               // +0x0f4 "$tag-35-squeeze" (unrotated) + position at load
    GameTimer sink_timer;                              // +0x100 the end of the rise / dive
    float hold_time;                                   // +0x104
    float up_time;                                     // +0x108 seconds above water
    float hp_mark;                                     // +0x10c HP fraction - 0.3 (tentacle_beast_tentacle_resurface)
};
LAYOUT_CHECK(tentacle_info, sizeof(tentacle_info) == 0x110);
#define NUM_TENTACLES 4
extern tentacle_info tentacle_beast_tentacles[NUM_TENTACLES];   // 0x248c328
void  tentacle_begin_sink(tentacle_info *t, int ms);   // 0x4c6c50
extern bool tentacle_beast_battle_started;             // 0x248c76c
extern int  tentacle_beast_tentacles_up;               // 0x248c798
extern bool evil_joseph_combat_enabled;                // 0x248b9b4 evil_joseph_ai_idle engages when set
extern bool evil_urath_combat_enabled;                 // 0x248bd0c evil_urath_ai_idle engages when set

bool  titus_cast_special(Character *c, int anim);      // 0x4c32e0
bool  pyrul_cast_special(Character *c, int anim);      // 0x4b94a0
bool  phoenix_cast_special(Character *c, int anim);    // 0x499840
bool  ghost_cast_special(Character *c, int anim);      // 0x4bb970
bool  evil_joseph_cast_special(Character *c, int anim);// 0x4c3980
bool  giant_salamanka_move_has_spell(int handle, int anim);   // 0x4c8b60
bool  murod_oni_cast_special(Character *c, int anim);  // folded into ls_test_use (xor al, al)

void  titus_knock_down_party(Character *boss, Character *target);      // 0x4c2ef0
void  pyrul_fire_attack(Character *boss, Character *target);           // 0x4b9670
void  phoenix_special_strike(Character *boss, Character *target);      // 0x499890
void  ghost_fire_attack(Character *boss, Character *target);           // 0x4bb9a0
void  evil_joseph_fire_attack(Character *boss, Character *target);     // 0x4c4020
void  giant_salamanka_fire_attack(Character *boss, Character *target); // 0x4c8b80
void  murod_oni_fire_attack(Character *boss, Character *target);       // folded (trigger case 0xb8)
void  machival_knock_down_party(Character *boss, Character *target);   // 0x4c1a80

int   tiger_move_in_range(Character *boss, Character *target);         // 0x49c690 (decomp54: mov eax, 1 / xor eax; was bool)
bool  serpent_move_in_range(Character *boss, Character *target);       // 0x497a00
int   giant_salamanka_target_in_reach(Character *boss, Character *target);   // 0x4c8c40 (decomp59: mov eax, 1 / xor eax; was bool, D3905)
bool  tentacle_target_in_reach(Character *boss, Character *target);    // 0x4c7910
bool  murod_oni_move_is_first();                       // 0x4c6460
bool  giant_salamanka_move_pushes();                   // 0x4c8c00
bool  giant_salamanka_move_knocks_down();              // 0x4c8c20
bool  giant_salamanka_stage_attack_enabled(int index); // 0x4c8cc0

void  tiger_anim_remap_in(Character *c, int *anims);   // 0x49c130
void  serpent_anim_remap_in(Character *c, int *anims); // 0x495d00
void  phoenix_anim_remap_in(Character *c, int *anims); // 0x499ad0
void  ghost_anim_remap_in(Character *c, int *anims);   // 0x4bb610
void  tiger_anim_remap_out(Character *c, int *anims);  // 0x49c1e0
void  serpent_anim_remap_out(Character *c, int *anims);// 0x495db0
void  phoenix_anim_remap_out(Character *c, int *anims);// 0x499b80
void  ghost_anim_remap_out(Character *c, int *anims);  // 0x4bb6c0

int   evil_joseph_get_attack_move_id();                // 0x4c40b0
int   serpent_part_index(Character *c);                // 0x4975a0 0 = the serpent itself
void  boss_part_deactivate(Character *c);              // 0x4975e0
void  serpent_set_ride_anims(Character *c);            // 0x497750
void  tiger_set_ride_anims(Character *c);              // 0x49a5e0
void  phoenix_set_ride_anims(Character *c);            // 0x499ce0
void  ghost_set_ride_anims(Character *c);              // 0x4bb870
void  serpent_script_hook(Character *c);               // folded (boss_script_hook_stub case 0x59)
void  tiger_script_hook(Character *c);                 // folded (case 0x57)
void  phoenix_script_hook(Character *c);               // folded (case 0x58)
void  ghost_script_hook(Character *c);                 // folded (case 0x5a)
void  serpent_set_foot_anims(Character *c);            // 0x497650
void  tiger_set_foot_anims(Character *c);              // 0x49c4a0
void  phoenix_set_foot_anims(Character *c);            // 0x499be0
void  ghost_set_foot_anims(Character *c);              // 0x4bb720
void  boss_dismount(Character *c);                     // 0x49c610 (tiger, phoenix and serpent)
void  ghost_dismount(Character *c);                    // 0x4bb810
void  tentacle_beast_frame_hook(Character *c);         // 0x4c7960
void  luminar_frame_hook(Character *c);                // 0x4c9000
void  luminar_delay_next_attack(Character *c);         // 0x4ca5f0
void  tiger_move_done_hook(Character *c);              // 0x49c670
float titus_get_attack_reach(Character *c);            // 0x4c2ea0
float pyrul_move_range(Character *c);                  // 0x4b9a00
bool  phoenix_get_attack_range(Character *c, float *out);   // 0x499810
bool  tiger_get_attack_range(Character *c, float *out);     // 0x49c720

bool  serpent_whip_test(Character *c);                 // 0x497890
void  serpent_whip_resolve_hit(Character *c);          // 0x4979b0
int   tentacle_get_attack_kind(Character *c);          // 0x4c7770 (sum.symbols: "(handle)"; the calls pass the character)
bool  tentacle_collect_area_hits(Character *c);        // 0x4c77c0
void  tentacle_apply_area_hits(Character *c);          // 0x4c78b0
int   titus_get_attack_kind(Character *c);             // 0x4c3060
bool  titus_collect_area_hits(Character *c);           // 0x4c3130
void  titus_apply_area_hits(Character *c);             // 0x4c31f0
int   ghost_is_own_move(Character *c);                 // 0x4bba60 (4 = the attack the dispatchers test)
bool  ghost_attack_test(Character *c);                 // 0x4bbae0
void  ghost_resolve_hit(Character *c);                 // 0x4bbc20
int   pyrul_is_own_move(Character *c);                 // 0x4b9a50 (0 and 2 tested)
bool  pyrul_attack_test_class0(Character *c);          // 0x4b9ae0
bool  pyrul_attack_test_class2(Character *c);          // 0x4b9cc0
void  pyrul_resolve_hit(Character *c);                 // 0x4b9e00
int   murod_oni_get_attack_kind(Character *c);         // 0x4c62d0
bool  murod_oni_collect_area_hits(Character *c);       // 0x4c6330
void  murod_oni_apply_area_hits(Character *c);         // 0x4c6410
int   machival_get_attack_kind();                      // 0x4c1b60 (no argument pushed)
bool  machival_attack_test(Character *c);              // 0x4c1bc0
bool  machival_collect_area_hits(Character *c);        // 0x4c1d10
void  machival_apply_area_hits(Character *c);          // 0x4c1dd0
bool  tiger_move_trigger(Character *c);                // 0x49c780
bool  titus_hold_attack(Character *boss, Character *target);     // 0x4c30e0
bool  luminar_hold_attack(Character *boss, Character *target);   // 0x4ca5a0

void  titus_filter_target(boss_target *t);             // 0x4c2210 (only the record is pushed)
void  pyrul_target_filter(Character *boss, boss_target *t);        // 0x4b88e0
void  murod_oni_evaluate_target(Character *boss, boss_target *t);  // 0x4c6190
void  murod_filter_target(Character *boss, boss_target *t);        // 0x4c5b70
void  evil_urath_filter_target(Character *boss, boss_target *t);   // 0x4c4470
void  ghost_target_flags(Character *boss, boss_target *t);         // 0x4bba20
void  evil_joseph_filter_target(Character *boss, boss_target *t);  // 0x4c4080
void  machival_filter_target(Character *boss, boss_target *t);     // 0x4c0d10
void  serpent_evaluate_target(Character *boss, boss_target *t);    // 0x4961b0
void  phoenix_evaluate_target(Character *boss, boss_target *t);    // 0x499e20

// Per-boss data the dispatchers read (the .bss of the per-boss units).
// giant_salamanka_threat_table (0x248c7ec): a field of the Giant Salamanka state object (decomp59, D3904;
// engine/ai/unnamed_giant_salamanka_0x4c8030.h).
// The bosses' "combat enabled" bytes (their console commands set them; level_script_init sets them for
// the multiplayer levels 0x15 / 0x13 / 0x19; decomp41). The last two names are ours.
extern bool titus_turn_lock;                         // 0x248b914 titus_update_turn_lock (decomp56: ai_combat_face_target)
extern bool titus_combat_enabled;                     // 0x248b3b4 titus_ai_idle engages when 1
extern bool pyrul_combat_enabled;                     // 0x247fd38 cmd_pyrul_combat, pyrul_ai_idle
extern bool machival_combat_enabled;                  // 0x248b034 cmd_machival_combat, machival_ai_idle
// The Giant Salamanka's stage handshake with the Khosani lab script (decomp34, D1407): giant_salamanka_combat_enabled
// (0x248c7d0), _stage2_reached / _stage2_ack (0x248c7dc / 0x248c7dd), _stage3_reached / _stage3_ack (0x248c7de /
// 0x248c7df) are fields of the state object since decomp59 (D3904), macros in unnamed_giant_salamanka_0x4c8030.h.
// tiger_threat_table (0x2452ac0): a field of the tiger's state object (D3403).
// serpent_threat_table (0x24522e8): a field of the serpent's state object (D3403, unnamed_serpent_0x495290.h).
// phoenix_threat_table (0x2452938): a field of the phoenix's state object (D3403, unnamed_phoenix_0x497ac0.h).
extern boss_threat ghost_rider_threat_table[BOSS_MAX_THREATS];       // 0x248081c
extern boss_threat titus_threat_table[BOSS_MAX_THREATS];             // 0x248b3b8
extern boss_threat pyrul_threat_table[BOSS_MAX_THREATS];             // 0x247fd3c
extern boss_threat murod_oni_threat_table[BOSS_MAX_THREATS];         // 0x248c008
extern boss_threat murod_threat_table[BOSS_MAX_THREATS];             // 0x248bf3c
extern boss_threat evil_urath_threat_table[BOSS_MAX_THREATS];        // 0x248bd10
extern boss_threat evil_joseph_threat_table[BOSS_MAX_THREATS];       // 0x248b9b8
// tiger_cur_move (0x2452c40): a field of the tiger's state object (D3403).
// phoenix_cur_attack (0x2452920): a field of the phoenix's state object (D3403).
extern boss_attack *ghost_cur_move;                   // 0x2480810 (name ours)
extern boss_attack *titus_current_move;               // 0x248b90c
extern boss_attack *pyrul_cur_move;                   // 0x2480290 (name ours)
extern boss_attack *murod_oni_current_move;           // 0x248c2dc
extern boss_attack *evil_urath_current_move;          // 0x248bec8
extern boss_attack *evil_joseph_current_move;         // 0x248bc8c
extern boss_attack *machival_current_move;            // 0x248b30c
// The serpent and its two copies (index serpent_part_index): the attack each one is playing (sum.symbols
// types it int[3]; the dispatchers read +0x14 / +0x1c through it) and its move id. decomp54 (D3403):
// serpent_part_busy, serpent_part_move, serpent_handle, serpent_timer_attack, serpent_stage and
// serpent_target_stage are fields of the serpent's state object, macros in
// engine/ai/unnamed_serpent_0x495290.h (included at the end of this header).
// tiger_stage (0x2452a90) and tiger_target_stage (0x2452a94): fields of the tiger's state object (D3403).
// phoenix_stage (0x2452420) and phoenix_target_stage (0x2452424): fields of the phoenix's state object
// (D3403, macros in engine/ai/unnamed_phoenix_0x497ac0.h).
// The stage each boss switches to (cmd_*_stage, *_boss_load; cutscene_play_escape sets 3; decomp52).



// "ride_horse_walk" into the serpent rider's locomotion anims (cutscene_start_special_cases, decomp52).
void  serpent_set_cutscene_walk_anim();                // 0x497840

// After boss_ai.cpp (decomp42: gameplay.cpp's calls).
void  level_page_in_characters();                      // 0x4a3580 (level_load_files)
void  frame_sleep_when_inactive(int active, int a);    // 0x4a3870 timer_wait(10) when active == 0
void  obstacles_debug_render();                        // 0x4a3890 (Render_3d_obstacles)
void  shadow_list_clear();                             // 0x4a3eb0

// ---- decomp56: the Pyrul unit (0x4b87c0-0x4b9e62) and the Ghost Rider unit (0x4b9e70-0x4bbe77) ----------
// Files not known (placeholders game/unnamed_pyrul_0x4b87c0.cpp, game/unnamed_ghost_rider_0x4b9e70.cpp).
class console_command;
class matrix;

// One found "$offense NN" mesh tag of a boss (0x1c bytes, 10 per boss after the count; the *_boss_load
// loops pack the found tags, the *_update_attack_points functions refresh dir / pos every think).
// Type and field names ours (bosses.md "offense points").
struct boss_offense_point {
    int tag;                          // +0x00 character_find_mesh_tag(offense_point_name(i))
    vector3 dir;                      // +0x04 second row (uvec) of the bone's world orientation
    vector3 pos;                      // +0x10 bone position + the boss's centre offset
};
LAYOUT_CHECK(boss_offense_point, sizeof(boss_offense_point) == 0x1c);

// Shared helper of another boss unit (Evil Joseph unit 0x4c3470-0x4c40d0, written in decomp59:
// boss_target_list_find 0x4c3c70 is declared above).
void  boss_resurrect_full(Character *c);               // 0x4c3fb0

// Pyrul (species 0xc4). The move pointers are [stage][index] records of pyrul_attack_table; names ours.
extern console_command ccmd_pyrul_stage;              // 0x247fcf0
extern console_command ccmd_pyrul_combat;             // 0x247fd18
void  cmd_pyrul_stage();                               // 0x4b8850
void  cmd_pyrul_combat();                              // 0x4b88c0
extern boss_attack *pyrul_move_2_3;                   // 0x247fcfc &pyrul_attack_table[1][3]
extern boss_attack *pyrul_move_3_3;                   // 0x247fd00 [2][3]
extern boss_attack *pyrul_move_3_2;                   // 0x247fd04 [2][2]
extern boss_attack *pyrul_move_2_2;                   // 0x247fd08 [1][2]
extern boss_attack *pyrul_move_1_0;                   // 0x247fd0c [0][0]
extern boss_attack *pyrul_move_2_0;                   // 0x247fd10 [1][0]
extern boss_attack *pyrul_move_3_0;                   // 0x247fd14 [2][0]
extern int  pyrul_stage;                              // 0x247fd28 1..3
extern int  pyrul_target_stage;                       // 0x247fd2c cmd_pyrul_stage (0 = by HP)
extern int  pyrul_247fd30;                            // 0x247fd30 zeroed when the target is lost, never read
extern int  pyrul_num_offense_points;                 // 0x247fdb4
extern boss_offense_point pyrul_offense_points[10];   // 0x247fdb8
extern boss_attack pyrul_attack_table[3][BOSS_MAX_OFFENSE_POINTS];   // 0x247fed0 "pyrul_boss.tbl"
extern GameTimer pyrul_move_timer;                    // 0x2480294 started when a move is chosen
extern int  pyrul_target;                             // 0x2480298 target handle
extern GameTimer pyrul_target_timer;                  // 0x248029c retarget timer
extern GameTimer pyrul_timer_a;                       // 0x24802a0 stage 2 special (30 s)
extern GameTimer pyrul_timer_b;                       // 0x24802a4 stage 3 special (30 s)
extern int  pyrul_ground_fx;                          // 0x24802a8 last wall of fire effect, never read
extern int  pyrul_24802ac;                            // 0x24802ac -1 at load, never read
extern int  pyrul_24802b0;                            // 0x24802b0 0 at load, never read
extern int  pyrul_24802b4;                            // 0x24802b4 0 at load, never read
extern int  pyrul_num_hits;                           // 0x24802b8
extern int  pyrul_hits[5];                            // 0x24802bc handles the area attack hits
extern boss_attack *pyrul_move_1_2;                   // 0x24802d0 [0][2]
extern boss_attack *pyrul_move_1_1;                   // 0x24802d4 [0][1]
extern boss_attack *pyrul_move_2_1;                   // 0x24802d8 [1][1]
extern boss_attack *pyrul_move_3_1;                   // 0x24802dc [2][1]
void  pyrul_timer_a_reset(Character *c);               // 0x4b89b0 (the argument is not read)
void  pyrul_timer_b_reset(Character *c);               // 0x4b89c0 (the argument is not read)
void  pyrul_stage1_reset(Character *c);                // folded into empty_function 0x4f13e0 (name ours)
void  pyrul_init_move_ptrs();                          // 0x4b8b20
void  pyrul_timer_set_random(int min_ms, int max_ms);  // 0x4b93d0 pyrul_target_timer
void  pyrul_update_attack_points(Character *c);        // 0x4b9400
void  pyrul_get_missile_origin(Character *c, matrix *orient, vector3 *pos);   // 0x4b95f0
bool  pyrul_point_in_attack_cone(Character *c, boss_attack *move);  // 0x4b9c20

// Ghost Rider (species 0x5a). Move pointers [stage][index] of ghost_attack_table; names ours. The minions
// are the bone servants (species 0x43, spell 0x51), bone mages (0x45, spell 0x52) and undead (0x9c,
// spell 0x53), two each (bosses.md "Ghost minions").
extern boss_attack *ghost_move_3_2;                   // 0x24802e0 [2][2]
extern boss_attack *ghost_move_1_2;                   // 0x24802e4 [0][2]
extern boss_attack *ghost_move_2_2;                   // 0x24802e8 [1][2]
extern Character *ghost_mount;                        // 0x24802ec the mount at load (rider sound)
extern boss_attack *ghost_move_3_0;                   // 0x24802f0 [2][0]
extern boss_attack *ghost_move_3_1;                   // 0x24802f4 [2][1]
extern boss_attack *ghost_move_2_1;                   // 0x24802f8 [1][1]
extern console_command ccmd_ghost_combat;             // 0x2480300
extern boss_attack *ghost_move_2_0;                   // 0x248030c [1][0]
extern int  ghost_target_stage;                       // 0x2480314 cmd_ghost_stage (0 = by HP)
extern bool ghost_dismount_requested;                 // 0x2480318 HP below 66 %: waits for the script
extern bool ghost_dismount_done;                      // 0x2480319 set by the Ghost-Dismount cutscene end
extern bool ghost_combat_enabled;                     // 0x248031a cmd_ghost_combat, ghost_ai_idle
extern bool ghost_mounted;                            // 0x248031b
extern int  ghost_remap_count;                        // 0x2480320 ghost_anim_remap_in / _out
extern int  ghost_remap_kind[2];                      // 0x2480324 locomotion_anim_names index
extern float ghost_remap_weight[2];                   // 0x248032c
extern int  ghost_num_offense_points;                 // 0x2480334
extern boss_offense_point ghost_offense_points[10];   // 0x2480338
extern boss_attack ghost_attack_table[3][BOSS_MAX_OFFENSE_POINTS];   // 0x2480450 "ghost_rider_boss.tbl"
extern int  ghost_pending_submode;                    // 0x2480814 0x20 (AIS_SAFETY) when ghost_choose_attack returns 1
extern int  ghost_2480818;                            // 0x2480818 zeroed when the target is lost, never read
extern int  ghost_target;                             // 0x2480894 target handle
extern GameTimer ghost_target_timer;                  // 0x2480898 retarget timer
extern GameTimer ghost_think_timer;                   // 0x248089c wander check (ghost_timer_set_random)
extern GameTimer ghost_wander_timer;                  // 0x24808a0 wander duration (5 s)
extern GameTimer ghost_fear_timer;                    // 0x24808a4 paralysing fear (45 s / 30 s)
extern GameTimer ghost_summon_timer;                  // 0x24808a8 minion summons (20-40 s)
extern int  ghost_saved_trip_anim;                    // 0x24808ac anim_tripped_liedown while mounted
extern int  ghost_bone_servants[2];                   // 0x24808b0 minion handles (species 0x43)
extern int  ghost_bone_mages[2];                      // 0x24808b8 (species 0x45; the servants in jadetemple)
extern int  ghost_undead[2];                          // 0x24808c0 (species 0x9c; the servants in jadetemple)
extern int  ghost_minions_raised;                     // 0x24808c8 summons out (the summon gate is < 2)
extern int  ghost_summon_minion;                      // 0x24808cc handle for the next summon, -1 none
extern vector3 ghost_summon_pos;                      // 0x24808d0
extern int  ghost_num_hits;                           // 0x24808dc
extern int  ghost_hits[12];                           // 0x24808e0
extern console_command ccmd_ghost_unlink;             // 0x2480910
extern boss_attack *ghost_move_1_0;                   // 0x248091c [0][0]
extern boss_attack *ghost_move_1_1;                   // 0x2480920 [0][1]
extern console_command ccmd_ghost_stage;              // 0x2480928
extern console_command ccmd_ghost_solo;               // 0x2480938
extern bool ghost_loaded;                             // 0x2480944 ghost_rider_boss_load ran (ghost_unload clears it)
void  cmd_ghost_stage();                               // 0x4b9ea0
void  cmd_ghost_unlink();                              // 0x4b9f00
void  cmd_ghost_combat();                              // 0x4b9fc0
Character *ghost_find_rider();                         // 0x4b9fe0 the last living entity of species 0x5a
void  cmd_ghost_solo();                                // 0x4ba050
// (unused, layer, pos): pos->y = the highest floor hit between pos->y + 20 and pos->y - 20, if any.
void  ghost_find_floor_below(Character *unused, int layer, vector3 *pos);   // 0x4ba0d0
void  ghost_timer_set_random(int min_ms, int max_ms);  // 0x4ba690 ghost_think_timer
void  ghost_set_stage1_anims(Character *c);            // 0x4ba6c0
void  ghost_setup_anims_unmounted(Character *c);       // 0x4ba780
void  ghost_setup_anims(Character *c);                 // 0x4ba7a0
void  ghost_init_move_ptrs();                          // 0x4ba870
void  ghost_timer2_set_random(int min_ms, int max_ms); // 0x4bb050 ghost_target_timer
void  ghost_update_attack_points(Character *c);        // 0x4bb080
void  ghost_choose_move_target(Character *boss);       // 0x4bb120 summon move, minion and position
bool  ghost_point_in_attack_cone(Character *c, boss_attack *move);  // 0x4bbb80

// ---- decomp59: the boss units 0x4c0c40-0x4ca6ff (files not known) ----------------------------------------
// Placeholders game/unnamed_<boss>_<address>.cpp. The thiscall functions are ai_state members (ai.h).
// Declarations made before decomp59 stay in their places above. Names ours unless sum.symbols has one.
struct navpoint;

// Machival (species 0xc5; game/unnamed_machival_0x4c0c40.cpp). One stage of moves; move 0 the melee.
extern console_command ccmd_machival_combat;          // 0x248b010
void  cmd_machival_combat();                           // 0x4c0cf0
extern boss_attack *machival_move_3;                  // 0x248b01c &machival_attack_table[0][3]
extern boss_attack *machival_move_5;                  // 0x248b020 [0][5]
extern boss_attack *machival_move_4;                  // 0x248b024 [0][4]
extern boss_attack *machival_move_2;                  // 0x248b028 [0][2]
extern boss_attack *machival_move_1;                  // 0x248b02c [0][1]
extern int  machival_248b030;                         // 0x248b030 zeroed when the target is lost, never read
extern bool machival_timers_started;                  // 0x248b035
extern boss_threat machival_threat_table[BOSS_MAX_THREATS];   // 0x248b038
extern int  machival_num_offense_points;              // 0x248b0b0
extern boss_offense_point machival_offense_points[10];        // 0x248b0b4
extern boss_attack machival_attack_table[1][BOSS_MAX_OFFENSE_POINTS];   // 0x248b1cc "machival_boss.tbl"
extern GameTimer machival_248b310;                    // 0x248b310 constructed, never used
extern int  machival_target_handle;                   // 0x248b314
extern GameTimer machival_attack_timer;               // 0x248b318
extern GameTimer machival_teleport_timer;             // 0x248b31c 70 s (2 s after a failed try)
extern GameTimer machival_spell100_timer;             // 0x248b320 spell 0x64
extern GameTimer machival_spell101_timer;             // 0x248b324 spell 0x65
extern GameTimer machival_spell102_timer;             // 0x248b328 spell 0x66
extern int  machival_cast_anim;                       // 0x248b32c
extern int  machival_casting_spell;                   // 0x248b330 -1 none
extern GameTimer machival_party_spell_timer;          // 0x248b334 spell 0x5b
extern int  machival_area_hit_count;                  // 0x248b338
extern int  machival_area_hits[5];                    // 0x248b33c
extern navpoint *machival_teleports[6];               // 0x248b350 "$tele_01".."$tele_06"
extern navpoint *machival_teleport_navpoint;          // 0x248b368 (mp_send_swi_boss)
extern vector3 machival_teleport_pos;                 // 0x248b36c the facing after the teleport (mp_handle_swi_boss)
extern boss_attack *machival_move_melee;              // 0x248b378 [0][0]
bool  machival_offense_point_reaches(Character *c, boss_attack *move);   // 0x4c0dd0
void  machival_reset_timers();                         // 0x4c0e70
void  machival_find_teleports();                       // 0x4c0ee0
void  machival_init_move_tables();                     // 0x4c1030
// (boss, out in move 3's range, out in the melee cone, out in move 1's cone, candidates, count)
void  machival_count_targets_in_reach(Character *boss, int *in_reach, int *in_melee, int *in_move1,
                                      boss_target *list, int count);   // 0x4c11c0
int   machival_pick_random_party_member();             // 0x4c1330 handle or -1
void  machival_set_attack_timer(int min_ms, int max_ms);   // 0x4c17f0
void  machival_update_offense_points(Character *c);    // 0x4c1820

// Titus (species 0xc1; game/unnamed_titus_0x4c20e0.cpp). titus_move_<s><i> = &titus_moves[s][i].
extern console_command ccmd_titus_stage;              // 0x248b968
extern console_command ccmd_titus_combat;             // 0x248b958
void  cmd_titus_stage();                               // 0x4c2180
void  cmd_titus_combat();                              // 0x4c21f0
extern boss_attack *titus_move_0;                     // 0x248b380
extern boss_attack *titus_move_12;                    // 0x248b384
extern boss_attack *titus_move_21;                    // 0x248b388 the rock eruption
extern boss_attack *titus_move_2;                     // 0x248b38c
extern boss_attack *titus_move_1;                     // 0x248b390
extern boss_attack *titus_move_13;                    // 0x248b394
extern boss_attack *titus_move_11;                    // 0x248b398 grab (turn lock)
extern boss_attack *titus_move_23;                    // 0x248b39c grab (turn lock)
extern boss_attack *titus_move_10;                    // 0x248b3a0
extern boss_attack *titus_move_22;                    // 0x248b3a4
extern boss_attack *titus_move_20;                    // 0x248b964
extern int  titus_stage;                              // 0x248b3a8 1..3
extern int  titus_forced_stage;                       // 0x248b3ac cmd_titus_stage (0 = by HP)
extern unsigned int titus_flags;                      // 0x248b3b0 bit 0: the move 10 follow-up is next
extern int  titus_offense_point_count;                // 0x248b430
extern boss_offense_point titus_offense_points[10];   // 0x248b434
extern boss_attack titus_moves[3][BOSS_MAX_OFFENSE_POINTS];   // 0x248b54c "titus_boss.tbl"
extern GameTimer titus_action_timer;                  // 0x248b910
extern int  titus_target_handle;                      // 0x248b918
extern GameTimer titus_attack_timer;                  // 0x248b91c
extern GameTimer titus_move10_timer;                  // 0x248b920
extern GameTimer titus_move13_timer;                  // 0x248b924
extern GameTimer titus_stage_timer_a;                 // 0x248b928 move 20
extern GameTimer titus_stage_timer_b;                 // 0x248b92c move 21
extern int  titus_pending_mode;                       // 0x248b930 0x3c
extern int  titus_area_hit_count;                     // 0x248b934
extern int  titus_area_hits[5];                       // 0x248b938
extern move_info *titus_rock_erupt_move;              // 0x248b94c find_move(9, 1)
extern int  titus_248b950;                            // 0x248b950 never accessed
extern int  titus_rock_erupt_fx_handle;               // 0x248b954
bool  titus_target_list_any_within(boss_target *list, int count, float range);   // 0x4c22a0
void  titus_update_turn_lock(Character *c);            // 0x4c22e0
void  titus_reset_attack_timers(Character *c);         // 0x4c2330 stage 2 entry (argument not read)
void  titus_reset_stage_timers(Character *c);          // 0x4c2360 stage 3 entry (argument not read)
void  titus_stage1_reset(Character *c);                // folded into empty_function 0x4f13e0
void  titus_init_move_tables();                        // 0x4c2450
void  titus_end_attack(Character *c);                  // 0x4c2610 current move NULL, go idle
void  titus_set_attack_timer(int min_ms, int max_ms);  // 0x4c2d90
void  titus_update_offense_points(Character *c);       // 0x4c2dc0
// (effect position, effect owner): fx_frame's call for the eruption effect.
void  titus_rock_erupt_damage(const vector3 *pos, int source);   // 0x4c3340

// Evil Joseph (species 0xc9; game/unnamed_evil_joseph_0x4c3470.cpp). One stage; moves 0-3 summon helper
// 0-3, move 8 the plain attack.
extern console_command ccmd_darkjo_combat;            // 0x248b980
void  cmd_darkjo_combat();                             // 0x4c34e0
extern boss_attack *evil_joseph_move_1;               // 0x248b978 &evil_joseph_moves[0][1]
extern boss_attack *evil_joseph_move_4;               // 0x248b98c
extern boss_attack *evil_joseph_move_2;               // 0x248b990
extern boss_attack *evil_joseph_move_5;               // 0x248b994
extern boss_attack *evil_joseph_move_3;               // 0x248b998
extern boss_attack *evil_joseph_move_6;               // 0x248b99c
extern boss_attack *evil_joseph_move_7;               // 0x248b9a0
extern int  evil_joseph_248b9a4;                      // 0x248b9a4 never accessed
extern int  evil_joseph_248b9a8;                      // 0x248b9a8 1 at load, never read
extern int  evil_joseph_248b9ac;                      // 0x248b9ac 1 at load, never read
extern int  evil_joseph_248b9b0;                      // 0x248b9b0 zeroed when the target is lost
extern int  evil_joseph_mesh_tag_count;               // 0x248ba30
extern boss_offense_point evil_joseph_offense_points[10];   // 0x248ba34
extern boss_attack evil_joseph_moves[1][BOSS_MAX_OFFENSE_POINTS];   // 0x248bb4c "darkjo_boss.tbl"
extern int  evil_joseph_248bc90;                      // 0x248bc90 never accessed
extern int  evil_joseph_target_handle;                // 0x248bc94
extern GameTimer evil_joseph_attack_timer;            // 0x248bc98
extern int  evil_joseph_attack_move_id;               // 0x248bc9c
extern int  evil_joseph_summon_handle;                // 0x248bca0
extern int  evil_joseph_helpers[4];                   // 0x248bca4 species 0xc0, 0xca, 0xcb, 0xb0
extern vector3 evil_joseph_summon_pos;                // 0x248bcb4
extern int  evil_joseph_resurrect_count;              // 0x248bcc0 summons out minus revivals
extern int  evil_joseph_helper_index;                 // 0x248bcc4
extern int  evil_joseph_teleport_index;               // 0x248bcc8 HP band 1..4, -1 none
extern int  evil_joseph_248bccc[3];                   // 0x248bccc never accessed
extern navpoint *evil_joseph_teleports[5];            // 0x248bcd8 "$tele_02", 03, 01, 05, 06
extern int  evil_joseph_248bcec;                      // 0x248bcec -1, never read
extern int  evil_joseph_248bcf0;                      // 0x248bcf0 -1, never read
extern boss_attack *evil_joseph_move_8;               // 0x248bcf4
extern boss_attack *evil_joseph_move_0;               // 0x248bcf8
extern bool evil_joseph_loaded;                       // 0x248bcfc
void  evil_joseph_spawn_helpers(Character *boss);      // 0x4c3500
void  evil_joseph_find_teleports();                    // 0x4c3720
void  evil_joseph_init_move_tables();                  // 0x4c3790
void  evil_joseph_set_attack_timer(int min_ms, int max_ms);   // 0x4c3ca0
void  evil_joseph_pick_helper_summon(Character *boss); // 0x4c3cd0

// Evil Urath (species 0xb1; game/unnamed_evil_urath_0x4c4170.cpp). No table file: spells 0x55-0x5b and
// leaps between the temple floor and the balconies (leap flags 1 floor, 2 balcony, 8 rising, 4 coming down).
extern console_command ccmd_urath_combat;             // 0x248bf28
void  cmd_urath_combat();                              // 0x4c41e0
extern int  evil_urath_248bd00;                       // 0x248bd00 1 at load, never read
extern int  evil_urath_248bd04;                       // 0x248bd04 0 at load, never read
extern unsigned int evil_urath_leap_flags;            // 0x248bd08
extern boss_attack evil_urath_unused_moves[BOSS_MAX_OFFENSE_POINTS];   // 0x248bd88 never accessed
extern int  evil_urath_pending_mode;                  // 0x248becc 0x23
extern int  evil_urath_target_handle;                 // 0x248bed0
extern GameTimer evil_urath_attack_timer;             // 0x248bed4
extern navpoint *evil_urath_leap_target;              // 0x248bed8
extern navpoint *evil_urath_teleports_a[3];           // 0x248bedc "$balc01_0N"
extern navpoint *evil_urath_teleports_b[3];           // 0x248bee8 "$temple_0N"
extern navpoint *evil_urath_teleports_c[3];           // 0x248bef4 "$balc02_0N"
extern int  evil_urath_248bf00[2];                    // 0x248bf00 never accessed
extern float evil_urath_leap_rise_speed;              // 0x248bf08 horizontal speed
extern float evil_urath_leap_fall_speed;              // 0x248bf0c
extern float evil_urath_leap_height;                  // 0x248bf10
extern float evil_urath_leap_start_y;                 // 0x248bf14
extern float evil_urath_leap_peak_y;                  // 0x248bf18
extern int  evil_urath_casting_spell;                 // 0x248bf1c
extern int  evil_urath_cast_anim;                     // 0x248bf20
extern int  evil_urath_248bf24;                       // 0x248bf24 never accessed
// (boss, nav layer, out distance): a nav wall crosses the line to the leap target (out: the last crossing).
bool  evil_urath_leap_check_path(Character *boss, int layer, float *dist);   // 0x4c4200
void  evil_urath_pick_teleport_point();                // 0x4c4380
void  evil_urath_load_hook();                          // folded into empty_frame_hook_40acc0 (name ours)
void  evil_urath_find_teleports();                     // 0x4c4640
void  evil_urath_set_attack_timer(int min_ms, int max_ms);   // 0x4c5110

// Murod (species 0xb7; game/unnamed_murod_0x4c5140.cpp). No table file, no console command.
extern int  murod_cure_statuses[4];                   // 0x59ddb0 blind, silence, curse, poison
extern bool murod_engaged;                            // 0x248bf38
extern int  murod_target_handle;                      // 0x248bfb4
extern int  murod_casting_spell;                      // 0x248bfb8 -1 none
extern int  murod_action22_count;                     // 0x248bfbc spell 0x22 in a row
extern float murod_last_health;                       // 0x248bfc0
extern int  murod_move_anim;                          // 0x248bfc4
extern vector3 murod_leap_dest;                       // 0x248bfc8
extern vector3 murod_leap_dir;                        // 0x248bfd4
extern GameTimer murod_attack_timer;                  // 0x248bfe0
extern GameTimer murod_leap_timer;                    // 0x248bfe4
extern GameTimer murod_heal_timer;                    // 0x248bfe8
extern GameTimer murod_spell_timer;                   // 0x248bfec
void  murod_set_attack_timer(int min_ms, int max_ms);  // 0x4c5a30

// Murod Oni (species 0xb8; game/unnamed_murod_oni_0x4c5c20.cpp). One stage of moves.
extern boss_attack *murod_oni_move_0;                 // 0x248bff0 &murod_oni_moves[0][0]
extern boss_attack *murod_oni_move_3;                 // 0x248bff4 [0][3] (murod_oni_move_is_first)
extern int  murod_oni_248bff8[2];                     // 0x248bff8 never accessed
extern int  murod_oni_248c000;                        // 0x248c000 zeroed when the target is lost
extern bool murod_oni_engaged;                        // 0x248c004
extern int  murod_oni_offense_point_count;            // 0x248c080
extern boss_offense_point murod_oni_offense_points[10];   // 0x248c084
extern boss_attack murod_oni_moves[1][BOSS_MAX_OFFENSE_POINTS];   // 0x248c19c "murod_oni_boss.tbl"
extern int  murod_oni_target_handle;                  // 0x248c2e0
extern int  murod_oni_saved_target_handle;            // 0x248c2e4
extern GameTimer murod_oni_attack_timer;              // 0x248c2e8
extern GameTimer murod_oni_crowd_timer;               // 0x248c2ec
extern GameTimer murod_oni_248c2f0;                   // 0x248c2f0 never used
extern GameTimer murod_oni_timer_b;                   // 0x248c2f4
extern int  murod_oni_area_hit_count;                 // 0x248c2f8
extern int  murod_oni_area_hits[5];                   // 0x248c2fc
extern boss_attack *murod_oni_move_1;                 // 0x248c310
extern boss_attack *murod_oni_move_4;                 // 0x248c314
extern boss_attack *murod_oni_move_2;                 // 0x248c318
void  murod_oni_init_move_tables();                    // 0x4c5d80
void  murod_oni_init_timers(Character *c);             // 0x4c5dc0 (argument not read)
void  murod_oni_update_offense_points(Character *c);   // 0x4c6010
void  murod_oni_set_attack_timer(int min_ms, int max_ms);   // 0x4c60b0
bool  murod_oni_offense_point_reaches(Character *c, boss_attack *move);   // 0x4c6230

// Tentacle beast (species 0x93; game/unnamed_tentacle_beast_0x4c6480.cpp). tentacle_info is declared above.
extern move_info *tentacle_move_2;                    // 0x248c320 find_move(0, 2): the squeeze
extern int  tentacle_248c324;                         // 0x248c324 never accessed
extern GameTimer tentacle_beast_grab_timer;           // 0x248c768
extern int  tentacle_beast_grab_ready;                // 0x248c770
extern int  tentacle_beast_tentacles_lost;            // 0x248c774
extern console_command ccmd_tentacle_test;            // 0x248c778
extern int  tentacle_248c784;                         // 0x248c784 never accessed
extern console_command ccmd_tentacles;                // 0x248c788
extern move_info *tentacle_move_1;                    // 0x248c794 find_move(0, 1): the swipe
extern int  tentacle_last_cycled;                     // 0x248c79c
extern move_info *tentacle_move_87;                   // 0x248c7a0 find_move(0, 0x87): the release
extern move_info *tentacle_move_86;                   // 0x248c7a4 find_move(0, 0x86): the grab
extern tentacle_info *tentacle_beast_active_tentacle; // 0x248c7a8
extern bool tentacle_beast_loaded;                    // 0x248c7ac
extern bool tentacle_test_mode;                       // 0x248c7ad
void  tentacle_begin_rise(tentacle_info *t);           // 0x4c6c20
tentacle_info *tentacle_find(Character *c);            // 0x4c7330 NULL none
void  tentacle_beast_tentacle_submerge(tentacle_info *t);   // 0x4c74f0
void  tentacle_beast_tentacle_resurface(tentacle_info *t);  // 0x4c7570
void  tentacle_beast_cycle_tentacles();                // 0x4c76d0
void  cmd_tentacles();                                 // 0x4c7cc0
void  cmd_tentacle_test();                             // 0x4c7e90

// Luminar (species 0x51; game/unnamed_luminar_0x4c8d10.cpp). luminar_free_bitmap, _frame_hook,
// _delay_next_attack and _hold_attack are declared above.
extern int  luminar_beam_bm;                          // 0x59dfd0 "GlowLuminarSkin02.vbm", -1 none
extern console_command ccmd_stage;                    // 0x248ce30
void  cmd_stage();                                     // 0x4c8d90
extern int  luminar_move_anim;                        // 0x248cdb0
extern int  luminar_248cdb4;                          // 0x248cdb4 never accessed
extern int  luminar_stage;                            // 0x248cdb8 1..3
extern int  luminar_forced_stage;                     // 0x248cdbc cmd_stage (1..3)
extern vector3 luminar_bite_tag_pos;                  // 0x248cdc0 object space
extern vector3 luminar_pickup_tag_pos;                // 0x248cdcc object space
extern float luminar_melee_reach;                     // 0x248cdd8
extern float luminar_pickup_reach;                    // 0x248cddc
extern int  luminar_target_handle;                    // 0x248cde0 the carried character
extern int  luminar_held_handle;                      // 0x248cde4 the thrown character in flight
extern GameTimer luminar_release_timer;               // 0x248cde8
extern GameTimer luminar_teleport_timer;              // 0x248cdec
extern GameTimer luminar_beam_timer;                  // 0x248cdf0
extern vector3 luminar_throw_velocity;                // 0x248cdf4
extern float luminar_grab_release_health;             // 0x248ce00
extern float luminar_throw_gravity;                   // 0x248ce04
extern float luminar_grab_cooldown;                   // 0x248ce08
extern bool luminar_beam_hit_done;                    // 0x248ce0c
extern bool luminar_beam_fx_variant;                  // 0x248ce0d
extern vector3 luminar_teleport_dest;                 // 0x248ce10
extern vector3 luminar_teleport_dir;                  // 0x248ce1c
extern GameTimer luminar_retarget_timer;              // 0x248ce28
extern GameTimer luminar_attack_timer;                // 0x248ce2c
bool  luminar_can_switch_target();                     // 0x4ca5c0 (ai_retaliate_on_hit; every 5 s)

#endif

// decomp54: the per-boss state objects (their fields keep the sum.symbols names as macros). Outside the
// guard so that the unit headers, which include this one first, see its types.
#include "engine/ai/unnamed_serpent_0x495290.h"
#include "engine/ai/unnamed_phoenix_0x497ac0.h"
#include "engine/ai/unnamed_tiger_0x499e50.h"
#include "engine/ai/unnamed_giant_salamanka_0x4c8030.h"
