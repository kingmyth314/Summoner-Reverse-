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
// uint8[0x440]). Only the fields the level scripts and tentacle_begin_sink touch are named. Type name ours.
struct tentacle_info {
    ubyte _unk000[0x80];
    Character *character;                              // +0x080 (ls_sewer_tb_intro_cs_end, 0x4ee2dc)
    ubyte _unk084[0x38];
    int victim;                                        // +0x0bc held victim, -1 none (tentacle_begin_sink)
    ubyte _unk0c0[0x50];
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
bool  giant_salamanka_target_in_reach(Character *boss, Character *target);   // 0x4c8c40
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
extern boss_threat giant_salamanka_threat_table[BOSS_MAX_THREATS];   // 0x248c7ec
// The bosses' "combat enabled" bytes (their console commands set them; level_script_init sets them for
// the multiplayer levels 0x15 / 0x13 / 0x19; decomp41). The last two names are ours.
extern bool titus_combat_enabled;                     // 0x248b3b4 titus_ai_idle engages when 1
extern bool pyrul_combat_enabled;                     // 0x247fd38 cmd_pyrul_combat, pyrul_ai_idle
extern bool machival_combat_enabled;                  // 0x248b034 cmd_machival_combat, machival_ai_idle
// The Giant Salamanka's stage handshake with the Khosani lab script (decomp34, D1407).
extern bool giant_salamanka_combat_enabled;           // 0x248c7d0 gates giant_salamanka_ai_frame
extern bool giant_salamanka_stage2_reached;           // 0x248c7dc set by giant_salamanka_check_stage
extern bool giant_salamanka_stage2_ack;               // 0x248c7dd set by the level script (cutscene end)
extern bool giant_salamanka_stage3_reached;           // 0x248c7de
extern bool giant_salamanka_stage3_ack;               // 0x248c7df
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

#endif

// decomp54: the per-boss state objects (their fields keep the sum.symbols names as macros). Outside the
// guard so that the unit headers, which include this one first, see its types.
#include "engine/ai/unnamed_serpent_0x495290.h"
#include "engine/ai/unnamed_phoenix_0x497ac0.h"
#include "engine/ai/unnamed_tiger_0x499e50.h"
