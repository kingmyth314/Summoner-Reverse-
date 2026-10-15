// boss_ai.cpp - Sum.exe v1.40, code 0x4a04b0-0x4a363b (module "boss_ai.cpp" in sum.symbols; path string
// 0x59cd54 "D:\projects\Summoner\pccode\Engine\ai\boss_ai.cpp", pushed at 0x4a1812, line 0x41f).
// Written from the Sum.exe disassembly (tools/asm.mjs fn; the jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other optimised units (no frame pointer, 0x90 padding).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: static_init_boss_rider_sound_timer (0x4a04b0) to level_page_in_characters (0x4a3580-0x4a363a;
// decomp57: added at the end, D3712).
// The previous function, pf_node_index (0x4a04a0), is pathfinding.cpp's last. sum.symbols also puts
// level_page_in_characters (0x4a3580) in boss_ai.cpp; decomp57 writes it here: no initialiser, no data and
// no other unit's code lies between it and level_preload_effects, and the next unit opens with its
// initialiser at 0x4a3640 (the handle slots, game/unnamed_handleslots_0x4a3640.cpp).
//
// Almost everything is a species switch that forwards to the per-boss units (files not known, module "-";
// docs/sum-exe/bosses.md has their code ranges). The case order below is the order of the case bodies in
// the binary (VC6 lays them out in source order); the byte-indexed jump tables give the case values.
// Where a table's bounds are wider than its real cases (boss_cast_special_dispatch, the stun / knock-down
// tests, boss_stage_attack_enabled_dispatch), the bounding species are written as explicit cases that
// share the default body (NOTES.md).
//
// Data: .data 0x59ccb8-0x59cd87, only this unit's literals: 0.86f (0x59ccb8, boss_turn_rate_override)
// first, then the table keywords "+Plus Spell:" .. "#Boss Info" (reverse order of use) and the path
// string last. "#End" (0x58bd08), "$Variation:" (0x5983cc), "Liangshan" / "Jadetemple" and the float
// constants are pooled. .bss: boss_rider_sound_timer 0x245c9e0 (between pathfinding.cpp's search stamp
// 0x245c9dc and the next unit's handle slots 0x245c9e8).

#include <math.h>
#include <float.h>
#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/spells/spells.h"
#include "engine/effects/fx.h"
#include "engine/gamesound/gamesound.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/vfile/file.h"
#include "vsdk/parse/parse.h"
#include "vsdk/geom/effect_mem.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "engine/rendering/levelrender.h"   // decomp57: lamp_glow_bitmap
#include "engine/s3d/s3d.h"                 // decomp57: level_page_in_textures
#include "vsdk/ca/character_instance.h"
#include "vsdk/gr/gr.h"
#include "vsdk/snd/snd.h"

#define CHAR_TYPE_BOSS 9              // obj +0xc (script_parse_objects: +Boss)

// ---- .bss ----------------------------------------------------------------------------------------
GameTimer boss_rider_sound_timer;     // 0x245c9e0

// 0x4a04b0  Confidence: exact. static_init_boss_rider_sound_timer: "jmp 0x4a04c0" (initialiser entry 290, 0x58848c).
// 0x4a04c0  Confidence: exact. init_boss_rider_sound_timer: "mov ecx, 0x245c9e0 / jmp GameTimer::GameTimer".
// Both compiler-generated from the definition above; GameTimer has no destructor, so no atexit helper.

// 0x4a04d0  Confidence: exact. Per-boss cleanup (ai_state_shutdown). Every case is a tail jump with ECX
// unchanged; the per-boss functions do not read it, so they are written as plain functions.
void ai_state::boss_ai_unload()
{
    switch (owner->info->species) {
    case SPECIES_LUMINAR:         luminar_free_bitmap(); break;
    case SPECIES_TIGER_RIDER:     tiger_unload(); break;
    case SPECIES_GIANT_SALAMANKA: giant_salamanka_free_bitmap(); break;
    case SPECIES_SERPENT_RIDER:   serpent_unload(); break;
    case SPECIES_GHOST_RIDER:     ghost_unload(); break;
    case SPECIES_PHOENIX_RIDER:   phoenix_unload(); break;
    case SPECIES_TENTACLE_BEAST:  tentacle_beast_unload(); break;
    case SPECIES_TITUS:           titus_unload(); break;
    case SPECIES_PYRUL:           pyrul_unload(); break;
    case SPECIES_MUROD_ONI:       murod_oni_unload(); break;
    case SPECIES_MUROD:           murod_unload(); break;
    case SPECIES_EVIL_URATH:      evil_urath_unload(); break;
    case SPECIES_EVIL_JOSEPH:     evil_joseph_unload(); break;
    case SPECIES_MACHIVAL:        machival_unload(); break;
    }
}

// 0x4a0600  Confidence: exact. Per-boss setup (ai_reset, for type 9 and Luminar).
void ai_state::boss_ai_load()
{
    switch (owner->info->species) {
    case SPECIES_LUMINAR:         luminar_boss_load(); break;
    case SPECIES_TIGER_RIDER:     tiger_boss_load(); break;
    case SPECIES_GIANT_SALAMANKA: giant_salamanka_boss_load(); break;
    case SPECIES_SERPENT_RIDER:   serpent_boss_load(); break;
    case SPECIES_PHOENIX_RIDER:   phoenix_boss_load(); break;
    case SPECIES_GHOST_RIDER:     ghost_rider_boss_load(); break;
    case SPECIES_TENTACLE_BEAST:  tentacle_beast_boss_load(); break;
    case SPECIES_TITUS:           titus_boss_load(); break;
    case SPECIES_PYRUL:           pyrul_boss_load(); break;
    case SPECIES_MUROD_ONI:       murod_oni_boss_load(); break;
    case SPECIES_MUROD:           murod_boss_load(); break;
    case SPECIES_EVIL_URATH:      evil_urath_boss_load(); break;
    case SPECIES_EVIL_JOSEPH:     evil_joseph_boss_load(); break;
    case SPECIES_MACHIVAL:        machival_boss_load(); break;
    }
}

// 0x4a0730  Confidence: exact. Every frame for type 9 (ai frame 0x472a90): threat decay, then the stage
// check and the rider seating. No Luminar case (it computes its stage in luminar_ai_select_action).
void ai_state::boss_ai_check_stage()
{
    boss_threat_decay();
    switch (owner->info->species) {
    case SPECIES_TIGER_RIDER:
        tiger_check_stage();
        tiger_seat_rider();
        break;
    case SPECIES_GIANT_SALAMANKA:
        giant_salamanka_check_stage();
        break;
    case SPECIES_SERPENT_RIDER:
        serpent_check_stage();
        serpent_seat_rider();
        serpent_part_check_timeout();
        break;
    case SPECIES_PHOENIX_RIDER:
        phoenix_check_stage();
        phoenix_seat_rider();
        break;
    case SPECIES_GHOST_RIDER:
        ghost_revive_minions();
        ghost_update_stage();
        ghost_seat_rider();
        break;
    case SPECIES_TENTACLE_BEAST:
        tentacle_beast_check_stage();
        break;
    case SPECIES_TITUS:
        titus_check_stage();
        break;
    case SPECIES_PYRUL:
        pyrul_check_stage();
        break;
    case SPECIES_MUROD_ONI:
        murod_oni_check_stage();
        break;
    case SPECIES_MUROD:
        murod_check_stage();
        break;
    case SPECIES_EVIL_URATH:
        evil_urath_check_stage();
        evil_urath_hold_height();
        break;
    case SPECIES_EVIL_JOSEPH:
        evil_joseph_check_stage();
        evil_joseph_check_stage_b();
        break;
    case SPECIES_MACHIVAL:
        machival_check_stage();
        break;
    }
}

// 0x4a08c0  Confidence: exact. Idle mode of a boss (ai frame 0x472a90).
void ai_state::boss_ai_idle_dispatch()
{
    switch (owner->info->species) {
    case SPECIES_LUMINAR:         luminar_ai_idle(); break;
    case SPECIES_TIGER_RIDER:     tiger_ai_idle(); break;
    case SPECIES_GIANT_SALAMANKA: giant_salamanka_ai_idle(); break;
    case SPECIES_SERPENT_RIDER:   serpent_ai_idle(); break;
    case SPECIES_PHOENIX_RIDER:   phoenix_ai_idle(); break;
    case SPECIES_GHOST_RIDER:     ghost_ai_idle(); break;
    case SPECIES_TENTACLE_BEAST:  tentacle_beast_ai_idle(); break;
    case SPECIES_TITUS:           titus_ai_idle(); break;
    case SPECIES_PYRUL:           pyrul_ai_idle(); break;
    case SPECIES_MUROD_ONI:       murod_oni_ai_idle(); break;
    case SPECIES_MUROD:           murod_ai_idle(); break;
    case SPECIES_EVIL_URATH:      evil_urath_ai_idle(); break;
    case SPECIES_EVIL_JOSEPH:     evil_joseph_ai_idle(); break;
    case SPECIES_MACHIVAL:        machival_ai_idle(); break;
    }
}

// 0x4a09f0  Confidence: exact. The boss's combat think function (0x4bff60). A multiplayer client runs
// only Machival's (the host drives the other bosses).
void ai_state::boss_ai_frame()
{
    if (MP_IS_CLIENT() && owner->info->species != SPECIES_MACHIVAL)
        return;
    switch (owner->info->species) {
    case SPECIES_LUMINAR:         luminar_ai_frame(); break;
    case SPECIES_TIGER_RIDER:     tiger_ai_frame(); break;
    case SPECIES_GIANT_SALAMANKA: giant_salamanka_ai_frame(); break;
    case SPECIES_SERPENT_RIDER:   serpent_ai_frame(); break;
    case SPECIES_PHOENIX_RIDER:   phoenix_ai_frame(); break;
    case SPECIES_GHOST_RIDER:     ghost_ai_frame(); break;
    case SPECIES_TENTACLE_BEAST:  tentacle_beast_ai_frame(); break;
    case SPECIES_TITUS:           titus_ai_frame(); break;
    case SPECIES_PYRUL:           pyrul_ai_frame(); break;
    case SPECIES_MUROD_ONI:       murod_oni_ai_frame(); break;
    case SPECIES_MUROD:           murod_ai_frame(); break;
    case SPECIES_EVIL_URATH:      evil_urath_ai_frame(); break;
    case SPECIES_EVIL_JOSEPH:     evil_joseph_ai_frame(); break;
    case SPECIES_MACHIVAL:        machival_ai_frame(); break;
    }
}

// 0x4a0b40  Confidence: likely (the Luminar case only bounds the jump table 0x4a0bf8 / 0x4a0c20 at
// 0x51; it shares the default body). The spell part of a boss move (living_combat_update_boss_attack:
// true sets the combat state 6). A serpent part's move counts when it has +Spell Only:.
bool boss_cast_special_dispatch(Character *c, int anim)
{
    switch (c->info->species) {
    case SPECIES_LUMINAR:
        break;
    case SPECIES_TITUS:
        return titus_cast_special(c, anim);
    case SPECIES_PYRUL:
        return pyrul_cast_special(c, anim);
    case SPECIES_SERPENT_RIDER: {
        boss_attack *m = serpent_part_busy[serpent_part_index(c)];
        if (m && (m->flags & BOSS_ATTACK_SPELL_ONLY))
            return true;
        break;
    }
    case SPECIES_PHOENIX_RIDER:
        return phoenix_cast_special(c, anim);
    case SPECIES_GHOST_RIDER:
        return ghost_cast_special(c, anim);
    case SPECIES_EVIL_JOSEPH:
        return evil_joseph_cast_special(c, anim);
    case SPECIES_GIANT_SALAMANKA:
        return giant_salamanka_move_has_spell(c->handle, anim);
    case SPECIES_MUROD_ONI:
        return murod_oni_cast_special(c, anim);
    }
    return false;
}

// 0x4a0ca0  Confidence: exact. A boss move trigger fired (living_combat_update_boss_attack): missiles,
// spells, party knock-downs. The serpent itself (not a copy) casts its move's spell at the target;
// Nightmare also restarts the serpent's 20 s attack timer.
void boss_attack_trigger_dispatch(Character *boss, Character *target)
{
    switch (boss->info->species) {
    case SPECIES_SERPENT_RIDER: {
        int part = serpent_part_index(boss);
        if (!character_is_serpent_part(boss)) {
            boss_attack *m = serpent_part_busy[part];
            if (m && m->spell != -1) {
                target->get_hit_by_spell(m->spell, boss->handle, 0, 0, 0, 0, 0, 0);
                if (m->spell == SPELL_NIGHTMARE)
                    serpent_timer_attack.SetTimeout(20000);
            }
        }
        break;
    }
    case SPECIES_TITUS:           titus_knock_down_party(boss, target); break;
    case SPECIES_PYRUL:           pyrul_fire_attack(boss, target); break;
    case SPECIES_PHOENIX_RIDER:   phoenix_special_strike(boss, target); break;
    case SPECIES_GHOST_RIDER:     ghost_fire_attack(boss, target); break;
    case SPECIES_EVIL_JOSEPH:     evil_joseph_fire_attack(boss, target); break;
    case SPECIES_GIANT_SALAMANKA: giant_salamanka_fire_attack(boss, target); break;
    case SPECIES_MUROD_ONI:       murod_oni_fire_attack(boss, target); break;
    case SPECIES_MACHIVAL:        machival_knock_down_party(boss, target); break;
    }
}

// 0x4a0e60  Confidence: exact (the range is kept in the move argument's slot). Edge distance below the
// move's second $Range: value, or below the char_info attack radius without a move.
bool boss_move_target_in_range(Character *boss, Character *target, boss_attack *move)
{
    float range = move ? move->range_max : boss->info->attack_radius;
    return boss->distance_between_edges(target, true, -1) < range;
}

// 0x4a0ea0  Confidence: likely (the Luminar / Murod case: the radius is read before the call and compared
// as "radius > distance", fcomp st(1)). Whether a boss's melee hit reaches (living_combat_resolve_hit).
// Evil Urath has no case, so his melee hits never land (NOTES.md).
bool boss_move_in_range_dispatch(Character *boss, Character *target)
{
    switch (boss->info->species) {
    case SPECIES_LUMINAR:
    case SPECIES_MUROD:
        if (boss->info->attack_radius > boss->distance_between_edges(target, true, -1))
            return true;
        break;
    case SPECIES_TIGER_RIDER:
        return tiger_move_in_range(boss, target);
    case SPECIES_SERPENT_RIDER:
        return serpent_move_in_range(boss, target);
    case SPECIES_PHOENIX_RIDER:
        return boss_move_target_in_range(boss, target, phoenix_cur_attack);
    case SPECIES_GHOST_RIDER:
        return boss_move_target_in_range(boss, target, ghost_cur_move);
    case SPECIES_TITUS:
        return boss_move_target_in_range(boss, target, titus_current_move);
    case SPECIES_PYRUL:
        return boss_move_target_in_range(boss, target, pyrul_cur_move);
    case SPECIES_MUROD_ONI:
        return boss_move_target_in_range(boss, target, murod_oni_current_move);
    case SPECIES_GIANT_SALAMANKA:
        return giant_salamanka_target_in_reach(boss, target);
    case SPECIES_TENTACLE_BEAST:
        return tentacle_target_in_reach(boss, target);
    case SPECIES_EVIL_JOSEPH:
        return boss_move_target_in_range(boss, target, evil_joseph_current_move);
    case SPECIES_MACHIVAL:
        return boss_move_target_in_range(boss, target, machival_current_move);
    }
    return false;
}

// 0x4a1090  Confidence: likely (the table 0x4a10b8 / 0x4a10c8 spans 0x51..0xc9: the bounding species are
// written as cases sharing the default). living_combat_resolve_hit then plays the type-9 move 0x91.
bool boss_attack_stuns_dispatch(int species)
{
    switch (species) {
    case SPECIES_LUMINAR:
    case SPECIES_EVIL_JOSEPH:
        break;
    case SPECIES_MUROD_ONI:
        return murod_oni_move_is_first();
    case SPECIES_GIANT_SALAMANKA:
        return giant_salamanka_move_pushes();
    }
    return false;
}

// 0x4a1150  Confidence: likely (table 0x4a1174 / 0x4a1180 over 0x51..0xc9, as above). A knock-down of
// 3-7 s follows in living_combat_resolve_hit.
bool boss_attack_knocks_down_dispatch(int species)
{
    switch (species) {
    case SPECIES_LUMINAR:
    case SPECIES_EVIL_JOSEPH:
        break;
    case SPECIES_GIANT_SALAMANKA:
        return giant_salamanka_move_knocks_down();
    }
    return false;
}

// 0x4a1200  Confidence: exact. Luminar, the Tentacle Beast and Machival keep no threat table.
boss_threat *boss_get_threat_table(Character *boss)
{
    switch (boss->info->species) {
    case SPECIES_GIANT_SALAMANKA: return giant_salamanka_threat_table;
    case SPECIES_TIGER_RIDER:     return tiger_threat_table;
    case SPECIES_SERPENT_RIDER:   return serpent_threat_table;
    case SPECIES_PHOENIX_RIDER:   return phoenix_threat_table;
    case SPECIES_GHOST_RIDER:     return ghost_rider_threat_table;
    case SPECIES_TITUS:           return titus_threat_table;
    case SPECIES_PYRUL:           return pyrul_threat_table;
    case SPECIES_MUROD_ONI:       return murod_oni_threat_table;
    case SPECIES_MUROD:           return murod_threat_table;
    case SPECIES_EVIL_URATH:      return evil_urath_threat_table;
    case SPECIES_EVIL_JOSEPH:     return evil_joseph_threat_table;
    }
    return NULL;
}

// 0x4a1310  Confidence: exact. Threat fades with the boss's frame time (+0x610): the first amount by
// 10 % and the second by 1/60 per second. Records of characters that are gone are freed. Only the
// serpent itself decays its table (its copies share it).
void ai_state::boss_threat_decay()
{
    boss_threat *t = boss_get_threat_table(owner);
    int i;

    if (!t)
        return;
    if (owner->info->species == SPECIES_SERPENT_RIDER && owner->handle != serpent_handle)
        return;
    for (i = 0; i < BOSS_MAX_THREATS; i++, t++) {
        if (t->handle != -1) {
            if (living_from_handle(t->handle)) {
                t->threat[0] -= owner->frametime * t->threat[0] * 0.1f;
                t->threat[1] -= owner->frametime * t->threat[1] * (1.0f / 60.0f);
            } else
                t->handle = -1;
        }
    }
}

// 0x4a13b0  Confidence: likely (the slot loop's shape: the two fill blocks and the placed flag in BL).
// Damage from a party member (type 7) goes to the attacker's record, or to the first free slot (-1 or a
// character that is gone); the remaining stale slots are freed. 20 s timer.
void boss_threat_add(Character *boss, float amount, int attacker, int flag)
{
    Character *a = living_from_handle(attacker);
    boss_threat *table;
    boss_threat *t;
    bool placed;
    int i;

    if (!a)
        return;
    table = boss_get_threat_table(boss);
    if (!table)
        return;
    if (a->type != 7)
        return;
    for (i = 0; i < BOSS_MAX_THREATS; i++) {
        t = &table[i];
        if (t->handle == attacker) {
            t->threat[0] += amount;
            t->threat[1] += amount;
            t->threat[3] += amount;
            if (!flag)
                t->threat[2] += amount;
            t->timer.SetTimeout(20000);
            return;
        }
    }
    placed = false;
    t = table;
    for (i = 0; i < BOSS_MAX_THREATS; i++, t++) {
        if (t->handle == -1) {
            if (!placed) {
                t->handle = attacker;
                t->threat[0] = amount;
                t->threat[1] = amount;
                t->threat[3] = amount;
                if (!flag)
                    t->threat[2] = amount;
                t->timer.SetTimeout(20000);
                placed = true;
            }
        } else if (!living_from_handle(t->handle)) {
            if (!placed) {
                t->handle = attacker;
                t->threat[0] = amount;
                t->threat[1] = amount;
                t->threat[3] = amount;
                if (!flag)
                    t->threat[2] = amount;
                t->timer.SetTimeout(20000);
                placed = true;
            } else
                t->handle = -1;
        }
    }
}

// 0x4a14f0  Confidence: exact. The 14 boss species (characterinfo_parse_table, script_parse_objects).
bool species_is_boss(int species)
{
    switch (species) {
    case SPECIES_LUMINAR:
    case SPECIES_TIGER_RIDER:
    case SPECIES_PHOENIX_RIDER:
    case SPECIES_SERPENT_RIDER:
    case SPECIES_GHOST_RIDER:
    case SPECIES_TENTACLE_BEAST:
    case SPECIES_GIANT_SALAMANKA:
    case SPECIES_EVIL_URATH:
    case SPECIES_MUROD:
    case SPECIES_MUROD_ONI:
    case SPECIES_TITUS:
    case SPECIES_PYRUL:
    case SPECIES_MACHIVAL:
    case SPECIES_EVIL_JOSEPH:
        return true;
    }
    return false;
}

// 0x4a15a0  Confidence: exact. Boss heights; the riders, Murod, Murod Oni and Evil Joseph use the size
// table, other species 1.0. character_size_height (0x4542f0) calls this for type 9 but drops the result
// (fstp st(0)) and returns the size table anyway (NOTES.md, game bug).
float charinfo_boss_height(char_info *info)
{
    switch (info->species) {
    case SPECIES_LUMINAR:
        return 5.5f;
    case SPECIES_GIANT_SALAMANKA:
        return 4.0f;
    case SPECIES_TIGER_RIDER:
    case SPECIES_PHOENIX_RIDER:
    case SPECIES_SERPENT_RIDER:
    case SPECIES_GHOST_RIDER:
    case SPECIES_MUROD:
    case SPECIES_MUROD_ONI:
    case SPECIES_EVIL_JOSEPH:
        return character_size_heights[info->size];
    case SPECIES_TENTACLE_BEAST:
        return 15.0f;
    case SPECIES_TITUS:
    case SPECIES_PYRUL:
        return 3.0f;
    case SPECIES_EVIL_URATH:
        return 8.0f;
    case SPECIES_MACHIVAL:
        return 5.0f;
    }
    return 1.0f;
}

// 0x4a16a0  Confidence: exact. Boss radii (character_size_radius, character_distance_between_edges).
float charinfo_boss_radius(char_info *info)
{
    switch (info->species) {
    case SPECIES_LUMINAR:
        return 1.35f;
    case SPECIES_GIANT_SALAMANKA:
    case SPECIES_MACHIVAL:
        return 2.0f;
    case SPECIES_TIGER_RIDER:
    case SPECIES_PHOENIX_RIDER:
    case SPECIES_SERPENT_RIDER:
    case SPECIES_GHOST_RIDER:
    case SPECIES_EVIL_URATH:
    case SPECIES_MUROD:
    case SPECIES_MUROD_ONI:
    case SPECIES_EVIL_JOSEPH:
        return character_size_radii[info->size];
    case SPECIES_TENTACLE_BEAST:
        return 5.0f;
    case SPECIES_TITUS:
        return 2.5f;
    case SPECIES_PYRUL:
        return 1.8f;
    }
    return 1.0f;
}

// 0x4a1790  Confidence: likely (the unused File: its constructor and destructor bracket the TextFile in
// the EH states 0 / 1, so it is a local here; the local buffer sizes and order from the frame:
// File +0x1bc, debug text +0x17c, message / spell name +0x7c (shared), point name +0x5c, TextFile +0xc).
// A *_boss.tbl: "#Boss Info", "$Num Stages:" (read, not kept), then per "$Stage:" N the offense point
// records into table[N - 1][0..9] until the next "$Stage:" or "#End". The stage count and the offense
// point count arguments are not read (no bounds check on N or on the record count).
void boss_ai_parse_table(const char *filename, int num_stages, boss_attack table[][BOSS_MAX_OFFENSE_POINTS],
                         int num_offense_points)
{
    File f;
    char debug_text[0x40];
    char point_name[0x20];
    TextFile tf;
    int stage;
    boss_attack *a;

    if (tf.Open(filename, PATH_ANY)) {
        char msg[0x100];
        sprintf(msg, "Couldn't open %s for parsing", filename);
        VSDK_ERROR("D:\\projects\\Summoner\\pccode\\Engine\\ai\\boss_ai.cpp", 0x41f, msg);
    }
    tf.Expect("#Boss Info");
    tf.Expect("$Num Stages:");
    tf.ParseInt();
    while (!tf.Accept("#End")) {
        tf.Expect("$Stage:");
        stage = tf.ParseInt();
        if (tf.Accept("$Offense point:")) {
            a = table[stage - 1];
            do {
                tf.ParseString(point_name, 0x20, '"', '"');
                a->offense_point = atoi(point_name) - 1;
                a->enabled = 1;
                tf.Expect("$Range:");
                a->range_min = tf.ParseFloat();
                a->range_max = tf.ParseFloat();
                tf.Expect("$Max angle:");
                a->cos_max_angle = (float)cos(tf.ParseFloat() * 0.017453292f);
                tf.Expect("$Variation:");
                a->variation = tf.ParseTypeString(variation_names, 0xc2);
                if (tf.Accept("+Debug:"))
                    tf.ParseString(debug_text, 0x40, '"', '"');
                a->flags = 0;
                if (tf.Accept("+Trip:"))
                    a->flags |= BOSS_ATTACK_TRIP;
                if (tf.Accept("+Push:"))
                    a->flags |= BOSS_ATTACK_PUSH;
                a->spell = -1;
                if (tf.Accept("+Spell:")) {
                    char spell_name[0x80];
                    tf.ParseString(spell_name, 0x80, '"', '"');
                    a->spell = spell_find_by_name(spell_name);
                    if (tf.Accept("+Projectile:"))
                        a->flags |= BOSS_ATTACK_PROJECTILE;
                    if (tf.Accept("+Spell Only:"))
                        a->flags |= BOSS_ATTACK_SPELL_ONLY;
                    if (tf.Accept("+Plus Spell:"))
                        a->flags |= BOSS_ATTACK_PLUS_SPELL;
                }
                a++;
            } while (!tf.Peek("$Stage:") && !tf.Peek("#End") && tf.Accept("$Offense point:"));
        }
    }
    tf.Close();
}

// 0x4a1ac0  Confidence: likely (table 0x4a1aec / 0x4a1af8 over 0x51..0xc5; the bounding species written
// as cases sharing the default). Per-stage attack enables (ai_approach_target_point).
bool boss_stage_attack_enabled_dispatch(int species, int index)
{
    switch (species) {
    case SPECIES_LUMINAR:
    case SPECIES_MACHIVAL:
        break;
    case SPECIES_GIANT_SALAMANKA:
        return giant_salamanka_stage_attack_enabled(index);
    }
    return true;
}

// 0x4a1b70  Confidence: exact. character_boss_anim_remap_in for the four riders.
void boss_anim_remap_in(Character *c, int *anims)
{
    switch (c->info->species) {
    case SPECIES_TIGER_RIDER:   tiger_anim_remap_in(c, anims); break;
    case SPECIES_SERPENT_RIDER: serpent_anim_remap_in(c, anims); break;
    case SPECIES_PHOENIX_RIDER: phoenix_anim_remap_in(c, anims); break;
    case SPECIES_GHOST_RIDER:   ghost_anim_remap_in(c, anims); break;
    }
}

// 0x4a1be0  Confidence: exact.
void boss_anim_remap_out(Character *c, int *anims)
{
    switch (c->info->species) {
    case SPECIES_TIGER_RIDER:   tiger_anim_remap_out(c, anims); break;
    case SPECIES_SERPENT_RIDER: serpent_anim_remap_out(c, anims); break;
    case SPECIES_PHOENIX_RIDER: phoenix_anim_remap_out(c, anims); break;
    case SPECIES_GHOST_RIDER:   ghost_anim_remap_out(c, anims); break;
    }
}

// 0x4a1c50  Confidence: likely (the result variable: 0.0 is loaded once and kept, fst [esp+8]). ai_turn_towards:
// the top rider of a stack (character_get_rider up the chain) that is a mounted serpent playing move
// 0xbb, or the tiger in stage 2, close to its target (edge distance < 2) that is nearly still
// (get_698 < 0.2) turns at a fixed rate: 0.86 / -0.7. 0 = no override.
float ai_state::boss_turn_rate_override()
{
    ai_state *ai = this;
    Character *r;
    Character *c;
    Character *target;
    float rate;

    for (r = owner->get_rider(); r; r = r->ai.owner->get_rider())
        ai = &r->ai;
    c = ai->owner;
    rate = 0.0f;
    if (c->type == CHAR_TYPE_BOSS) {
        switch (c->info->species) {
        case SPECIES_SERPENT_RIDER:
            if (serpent_stage == 1) {
                boss_attack *m = serpent_part_busy[serpent_part_index(c)];
                if (m && m->variation == 0xbb) {
                    target = living_from_handle(ai->target);
                    if (target && c->distance_between_edges(target, true, ai->_unk7c) < 2.0f &&
                        target->get_698() < 0.2f)
                        return 0.86f;
                }
            }
            break;
        case SPECIES_TIGER_RIDER:
            if (tiger_stage == 2) {
                target = living_from_handle(ai->target);
                if (target && c->distance_between_edges(target, true, ai->_unk7c) < 2.0f &&
                    target->get_698() < 0.2f)
                    return -0.7f;
            }
            break;
        }
    }
    return rate;
}

// 0x4a1d90  Confidence: exact. A second copy of charinfo_find_by_species (0x446bd0).
int charinfo_index_from_species(int species)
{
    int i;

    for (i = 0; i < num_char_infos; i++)
        if (char_infos[i].species == species)
            return i;
    return -1;
}

// 0x4a1dc0  Confidence: likely (the search result variable: the found path tests it again, 0x4a1e63).
// Outside Liangshan and Jadetemple a rider gets a mount: its own, a riderless species-0x55 one within
// 4 m, or a new one at its position (living_entity_create type 4).
bool boss_ensure_mount(Character *boss)
{
    Character *mount;
    Character *c;
    int index;

    if (!_stricmp(level_filename, "Liangshan") || !_stricmp(level_filename, "Jadetemple"))
        return false;
    mount = boss->get_mount();
    if (mount && mount->get_rider() == boss)
        return true;
    mount = NULL;
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->info->species == SPECIES_BOSS_MOUNT && !c->get_rider() &&
            boss->distance_between_edges(c, true, -1) < 4.0f) {
            mount = c;
            break;
        }
    }
    if (!mount) {
        index = charinfo_index_from_species(SPECIES_BOSS_MOUNT);
        if (index < 0)
            return false;
        mount = living_entity_create(4, index, &boss->pos, &boss->orient, 1, boss->layer, 0, 0, 0);
        if (!mount)
            return false;
    }
    mount->mount_link = boss->handle;
    mount->mount_flags |= 2;
    boss->mount_link = mount->handle;
    boss->mount_flags |= 1;
    boss->flags |= 0x20;
    return true;
}

// 0x4a1ef0  Confidence: likely (the out-vector stores: y = 0 between x and z). The point of the mount's
// size / layer list farthest from the mount (squared x / z distance), else the mount's own position.
void boss_mount_pick_flee_point(Character *mount, vector3 *out)
{
    float best = -FLT_MAX;
    int best_i = -1;
    float x = mount->pos.x;
    float z = mount->pos.z;
    pf_point_list *list = &pf_point_lists[mount->info->size][mount->layer];
    int i;

    for (i = 0; i < list->count; i++) {
        vector2 d;
        float len;

        d.x = x - list->points[i].pos->x;
        d.y = z - list->points[i].pos->y;
        len = d.len_sq();
        if (len > best) {
            best = len;
            best_i = i;
        }
    }
    if (best_i != -1) {
        vector2 *p = list->points[best_i].pos;
        out->x = p->x;
        out->y = 0.0f;
        out->z = p->y;
    } else
        *out = mount->pos;
}

// 0x4a1fe0  Confidence: likely (store order of the mount's link fields). The rider steps off: the mount
// is unlinked and runs to the farthest graph point (goal 0xd, 5 s).
void boss_release_mount(Character *rider)
{
    Character *mount = rider->get_mount();
    vector3 dest;

    if (mount) {
        mount->mount_flags = 0;
        mount->flags &= ~0x20;
        mount->mount_link = -1;
        mount->ai.target = -1;
        boss_mount_pick_flee_point(mount, &dest);
        mount->ai.add_move_to_timed(&dest, mount->layer, 30.0f, 0, 5000);
        rider->mount_flags = 0;
        rider->mount_link = -1;
    }
}

// 0x4a2070  Confidence: exact. The attack move id of a type-9 boss's current move (+0x18, the
// $Variation:), for ai_combat_melee_attack / _ranged_attack. The move pointers are not checked.
int ai_state::boss_get_attack_move_id()
{
    if (owner->type == CHAR_TYPE_BOSS) {
        switch (owner->info->species) {
        case SPECIES_TIGER_RIDER:   return tiger_cur_move->variation;
        case SPECIES_PHOENIX_RIDER: return phoenix_cur_attack->variation;
        case SPECIES_GHOST_RIDER:   return ghost_cur_move->variation;
        case SPECIES_TITUS:         return titus_current_move->variation;
        case SPECIES_PYRUL:         return pyrul_cur_move->variation;
        case SPECIES_MUROD_ONI:     return murod_oni_current_move->variation;
        case SPECIES_EVIL_URATH:    return evil_urath_current_move->variation;
        case SPECIES_EVIL_JOSEPH:   return evil_joseph_get_attack_move_id();
        case SPECIES_MACHIVAL:      return machival_current_move->variation;
        case SPECIES_SERPENT_RIDER: return serpent_part_move[serpent_part_index(owner)];
        }
    }
    return 0;
}

// 0x4a21b0  Confidence: exact. The char_info's attack point count; 0 for the riders once they are on
// foot (tiger stage 3, the others stage 2 and 3).
int character_target_point_count(Character *c)
{
    if (c->type == CHAR_TYPE_BOSS && c->info->num_target_points > 0) {
        switch (c->info->species) {
        case SPECIES_TIGER_RIDER:
            if (tiger_stage == 3)
                return 0;
            break;
        case SPECIES_PHOENIX_RIDER:
            if (phoenix_stage > 1)
                return 0;
            break;
        case SPECIES_SERPENT_RIDER:
            if (serpent_stage > 1)
                return 0;
            break;
        case SPECIES_GHOST_RIDER:
            if (ghost_stage > 1)
                return 0;
            break;
        }
    }
    return c->info->num_target_points;
}

// 0x4a2230  Confidence: exact. The boss's melee hit carries its move's spell (+Plus Spell:) to the
// defender: the Ghost Rider's Ghost death as a hit, its other spells as an effect on the defender (1 s);
// a serpent part's spell as a hit.
void boss_counter_hit_dispatch(Character *attacker, Character *defender)
{
    if (attacker->type == CHAR_TYPE_BOSS) {
        switch (attacker->info->species) {
        case SPECIES_GHOST_RIDER: {
            boss_attack *m = ghost_cur_move;
            if (m && (m->flags & BOSS_ATTACK_PLUS_SPELL)) {
                if (m->spell == SPELL_GHOST_DEATH)
                    defender->get_hit_by_spell(SPELL_GHOST_DEATH, attacker->handle, 0, 0, 0, 0, 0, 0);
                else
                    fx_create_spell_effect_at(m->spell, attacker->handle, defender->handle, 1000, NULL, NULL);
            }
            break;
        }
        case SPECIES_SERPENT_RIDER: {
            boss_attack *m = serpent_part_busy[serpent_part_index(attacker)];
            if (m && (m->flags & BOSS_ATTACK_PLUS_SPELL))
                defender->get_hit_by_spell(m->spell, attacker->handle, 0, 0, 0, 0, 0, 0);
            break;
        }
        }
    }
}

// 0x4a2300  Confidence: exact. Mounted locomotion anims (script_entry_start).
void boss_set_ride_anims(Character *c)
{
    if (c->type == CHAR_TYPE_BOSS) {
        switch (c->info->species) {
        case SPECIES_SERPENT_RIDER: serpent_set_ride_anims(c); break;
        case SPECIES_TIGER_RIDER:   tiger_set_ride_anims(c); break;
        case SPECIES_PHOENIX_RIDER: phoenix_set_ride_anims(c); break;
        case SPECIES_GHOST_RIDER:   ghost_set_ride_anims(c); break;
        }
    }
}

// 0x4a2360  Confidence: exact. Four empty per-rider hooks (all folded into empty_function).
void boss_script_hook_stub(Character *c)
{
    if (c->type == CHAR_TYPE_BOSS) {
        switch (c->info->species) {
        case SPECIES_SERPENT_RIDER: serpent_script_hook(c); break;
        case SPECIES_TIGER_RIDER:   tiger_script_hook(c); break;
        case SPECIES_PHOENIX_RIDER: phoenix_script_hook(c); break;
        case SPECIES_GHOST_RIDER:   ghost_script_hook(c); break;
        }
    }
}

// 0x4a23c0  Confidence: exact. On-foot anims (script_entry_start).
void boss_set_foot_anims(Character *c)
{
    if (c->type == CHAR_TYPE_BOSS) {
        switch (c->info->species) {
        case SPECIES_SERPENT_RIDER: serpent_set_foot_anims(c); break;
        case SPECIES_TIGER_RIDER:   tiger_set_foot_anims(c); break;
        case SPECIES_PHOENIX_RIDER: phoenix_set_foot_anims(c); break;
        case SPECIES_GHOST_RIDER:   ghost_set_foot_anims(c); break;
        }
    }
}

// 0x4a2420  Confidence: exact. script_entry_check_done / script_entry_jump_to_sequence.
void boss_dismount_dispatch(Character *c)
{
    if (c->type == CHAR_TYPE_BOSS) {
        switch (c->info->species) {
        case SPECIES_SERPENT_RIDER: boss_dismount(c); break;
        case SPECIES_TIGER_RIDER:   boss_dismount(c); break;
        case SPECIES_PHOENIX_RIDER: boss_dismount(c); break;
        case SPECIES_GHOST_RIDER:   ghost_dismount(c); break;
        }
    }
}

// 0x4a2480  Confidence: exact. Whether a rider species is in a mounted stage (ai_get_combat_style). The
// phoenix case reads serpent_stage (0x2451d98), not phoenix_stage (0x2452420): game bug (NOTES.md).
bool boss_species_mounted_stage(int species)
{
    switch (species) {
    case SPECIES_TIGER_RIDER:
        return tiger_stage == 1 || tiger_stage == 2;
    case SPECIES_PHOENIX_RIDER:
    case SPECIES_SERPENT_RIDER:
        return serpent_stage == 1;
    }
    return false;
}

// 0x4a24c0  Confidence: exact. A second boss test (ai_score_target, ai_retaliate_on_hit,
// ai_alert_guards_of_target) without Luminar, the Serpent Rider and Giant Salamanka.
bool species_is_boss_b(int species)
{
    switch (species) {
    case SPECIES_TIGER_RIDER:
    case SPECIES_PHOENIX_RIDER:
    case SPECIES_GHOST_RIDER:
    case SPECIES_TENTACLE_BEAST:
    case SPECIES_EVIL_URATH:
    case SPECIES_MUROD:
    case SPECIES_MUROD_ONI:
    case SPECIES_TITUS:
    case SPECIES_PYRUL:
    case SPECIES_MACHIVAL:
    case SPECIES_EVIL_JOSEPH:
        return true;
    }
    return false;
}

// 0x4a2560  Confidence: exact. Grab handling (ai_do_frame).
void boss_ai_do_frame_hook(Character *c)
{
    switch (c->info->species) {
    case SPECIES_LUMINAR:        luminar_frame_hook(c); break;
    case SPECIES_TENTACLE_BEAST: tentacle_beast_frame_hook(c); break;
    }
}

// 0x4a2590  Confidence: exact. Ends the boss's current move (living_combat_update_boss_attack /
// _update_ranged).
void boss_clear_current_move(Character *c)
{
    switch (c->info->species) {
    case SPECIES_LUMINAR:
        luminar_delay_next_attack(c);
        break;
    case SPECIES_PHOENIX_RIDER:
        phoenix_cur_attack = NULL;
        break;
    case SPECIES_GHOST_RIDER:
        ghost_cur_move = NULL;
        break;
    case SPECIES_TITUS:
        titus_current_move = NULL;
        break;
    case SPECIES_PYRUL:
        pyrul_cur_move = NULL;
        break;
    case SPECIES_MUROD_ONI:
        murod_oni_current_move = NULL;
        break;
    case SPECIES_EVIL_URATH:
        evil_urath_current_move = NULL;
        break;
    case SPECIES_EVIL_JOSEPH:
        evil_joseph_current_move = NULL;
        break;
    case SPECIES_TIGER_RIDER:
        tiger_move_done_hook(c);
        tiger_cur_move = NULL;
        break;
    case SPECIES_SERPENT_RIDER:
        serpent_part_busy[serpent_part_index(c)] = NULL;
        break;
    case SPECIES_MACHIVAL:
        machival_current_move = NULL;
        break;
    }
}

// 0x4a26f0  Confidence: exact (each case has its own copy of the three stores, *max first). The range
// of the current move (ai_combat_approach / _close_in / _melee_attack); attack_range (+0x34) gets the
// far end. Titus and Pyrul only set attack_range.
void ai_state::boss_get_move_range(float *min, float *max)
{
    boss_attack *m;

    switch (owner->info->species) {
    case SPECIES_EVIL_JOSEPH:
        m = evil_joseph_current_move;
        if (m) {
            *max = m->range_max;
            *min = m->range_min;
            attack_range = *max;
        }
        break;
    case SPECIES_PHOENIX_RIDER:
        m = phoenix_cur_attack;
        if (m) {
            *max = m->range_max;
            *min = m->range_min;
            attack_range = *max;
        }
        break;
    case SPECIES_GHOST_RIDER:
        m = ghost_cur_move;
        if (m) {
            *max = m->range_max;
            *min = m->range_min;
            attack_range = *max;
        }
        break;
    case SPECIES_SERPENT_RIDER:
        m = serpent_part_busy[serpent_part_index(owner)];
        if (m) {
            *max = m->range_max;
            *min = m->range_min;
            attack_range = *max;
        }
        break;
    case SPECIES_TITUS:
        attack_range = titus_get_attack_reach(owner);
        break;
    case SPECIES_PYRUL:
        attack_range = pyrul_move_range(owner);
        break;
    }
}

// 0x4a2870  Confidence: likely (body order: the phoenix call is laid out before the tiger call).
// character_get_attack_range.
bool boss_get_attack_range(Character *c, float *out)
{
    switch (c->info->species) {
    case SPECIES_TIGER_RIDER:
        return tiger_get_attack_range(c, out);
    case SPECIES_PHOENIX_RIDER:
        return phoenix_get_attack_range(c, out);
    }
    return false;
}

// 0x4a28b0  Confidence: exact. A hit serpent copy (not the serpent itself) is switched off.
bool serpent_part_on_hit(Character *c)
{
    if (c->type == CHAR_TYPE_BOSS && c->info->species == SPECIES_SERPENT_RIDER && c->handle != serpent_handle) {
        boss_part_deactivate(c);
        return true;
    }
    return false;
}

// 0x4a28f0  Confidence: exact. One of the serpent's copies (part index != 0).
bool character_is_serpent_part(Character *c)
{
    if (c->info->species == SPECIES_SERPENT_RIDER && serpent_part_index(c))
        return true;
    return false;
}

// 0x4a2920  Confidence: likely (the Machival case: both kinds fall into "return false" even after
// set_40; the kind-7 path has its own copy of the return). Who the current special move reaches; true
// skips the opponent's defence roll in living_combat_update_boss_attack. Machival never returns true,
// so the roll runs after its area test (NOTES.md).
bool boss_attack_test_dispatch(living_combat *lc)
{
    if (lc->owner->type == CHAR_TYPE_BOSS) {
        switch (lc->owner->info->species) {
        case SPECIES_SERPENT_RIDER:
            if (lc->get_special_skill() == SKILL_SERPENT_WHIP_MULTI) {
                if (serpent_whip_test(lc->owner))
                    lc->set_40(1);
                return true;
            }
            break;
        case SPECIES_TENTACLE_BEAST:
            if (tentacle_get_attack_kind(lc->owner) == 0 && tentacle_collect_area_hits(lc->owner)) {
                lc->set_40(1);
                return true;
            }
            break;
        case SPECIES_TITUS:
            if (titus_get_attack_kind(lc->owner) == 3 && titus_collect_area_hits(lc->owner)) {
                lc->set_40(1);
                return true;
            }
            break;
        case SPECIES_GHOST_RIDER:
            if (ghost_is_own_move(lc->owner) == 4 && ghost_attack_test(lc->owner)) {
                lc->set_40(1);
                return true;
            }
            break;
        case SPECIES_PYRUL:
            if (pyrul_is_own_move(lc->owner) == 0) {
                if (pyrul_attack_test_class0(lc->owner)) {
                    lc->set_40(1);
                    return true;
                }
            } else if (pyrul_is_own_move(lc->owner) == 2 && pyrul_attack_test_class2(lc->owner)) {
                lc->set_40(1);
                return true;
            }
            break;
        case SPECIES_MUROD_ONI:
            if (murod_oni_get_attack_kind(lc->owner) == 4 && murod_oni_collect_area_hits(lc->owner)) {
                lc->set_40(1);
                return true;
            }
            break;
        case SPECIES_MACHIVAL:
            if (machival_get_attack_kind() == 7) {
                if (machival_attack_test(lc->owner))
                    lc->set_40(1);
            } else if (machival_get_attack_kind() == 3 && machival_collect_area_hits(lc->owner))
                lc->set_40(1);
            break;
        }
    }
    return false;
}

// 0x4a2b80  Confidence: exact. Applies the hits the test found; true skips the normal
// living_combat_resolve_hit. The Ghost Rider's resolve runs and then the normal one as well.
bool boss_attack_resolve_dispatch(living_combat *lc)
{
    if (lc->owner->type == CHAR_TYPE_BOSS) {
        switch (lc->owner->info->species) {
        case SPECIES_SERPENT_RIDER:
            if (lc->get_special_skill() == SKILL_SERPENT_WHIP_MULTI) {
                serpent_whip_resolve_hit(lc->owner);
                return true;
            }
            break;
        case SPECIES_GHOST_RIDER:
            if (ghost_is_own_move(lc->owner) == 4)
                ghost_resolve_hit(lc->owner);
            break;
        case SPECIES_TENTACLE_BEAST:
            if (tentacle_get_attack_kind(lc->owner) == 0) {
                tentacle_apply_area_hits(lc->owner);
                return true;
            }
            break;
        case SPECIES_TITUS:
            if (titus_get_attack_kind(lc->owner) == 3) {
                titus_apply_area_hits(lc->owner);
                return true;
            }
            break;
        case SPECIES_PYRUL:
            if (pyrul_is_own_move(lc->owner) == 0) {
                pyrul_resolve_hit(lc->owner);
                return true;
            } else if (pyrul_is_own_move(lc->owner) == 2) {
                pyrul_resolve_hit(lc->owner);
                return true;
            }
            break;
        case SPECIES_MUROD_ONI:
            if (murod_oni_get_attack_kind(lc->owner) == 4) {
                murod_oni_apply_area_hits(lc->owner);
                return true;
            }
            break;
        case SPECIES_MACHIVAL:
            if (machival_get_attack_kind() == 7 || machival_get_attack_kind() == 3) {
                machival_apply_area_hits(lc->owner);
                return true;
            }
            break;
        }
    }
    return false;
}

// 0x4a2d40  Confidence: exact.
bool character_is_not_boss(Character *c)
{
    return c->type != CHAR_TYPE_BOSS;
}

// 0x4a2d50  Confidence: exact.
bool boss_move_trigger_dispatch(Character *c)
{
    if (c->info->species == SPECIES_TIGER_RIDER)
        return tiger_move_trigger(c);
    return false;
}

// 0x4a2d70  Confidence: exact. Titus and Pyrul move without path finding (ai_path_find_new).
bool character_uses_pathfinding(Character *c)
{
    if (c->type == CHAR_TYPE_BOSS) {
        switch (c->info->species) {
        case SPECIES_TITUS:
        case SPECIES_PYRUL:
            return false;
        }
    }
    return true;
}

// 0x4a2da0  Confidence: exact. living_combat_update_boss_attack: true holds the attack.
bool boss_hook_dispatch_51_c1(Character *boss, Character *target)
{
    switch (boss->info->species) {
    case SPECIES_LUMINAR: return luminar_hold_attack(boss, target);
    case SPECIES_TITUS:   return titus_hold_attack(boss, target);
    }
    return false;
}

// 0x4a2de0  Confidence: exact.
int boss_threat_find(int handle, boss_threat *table)
{
    int i;

    for (i = 0; i < BOSS_MAX_THREATS; i++, table++)
        if (table->handle == handle)
            return i;
    return -1;
}

// 0x4a2e00  Confidence: exact. Per-boss flags on a candidate (boss_target_list_build).
void boss_target_filter_dispatch(Character *boss, boss_target *t)
{
    switch (boss->info->species) {
    case SPECIES_TITUS:         titus_filter_target(t); break;
    case SPECIES_PYRUL:         pyrul_target_filter(boss, t); break;
    case SPECIES_MUROD_ONI:     murod_oni_evaluate_target(boss, t); break;
    case SPECIES_MUROD:         murod_filter_target(boss, t); break;
    case SPECIES_EVIL_URATH:    evil_urath_filter_target(boss, t); break;
    case SPECIES_GHOST_RIDER:   ghost_target_flags(boss, t); break;
    case SPECIES_EVIL_JOSEPH:   evil_joseph_filter_target(boss, t); break;
    case SPECIES_MACHIVAL:      machival_filter_target(boss, t); break;
    case SPECIES_SERPENT_RIDER: serpent_evaluate_target(boss, t); break;
    case SPECIES_PHOENIX_RIDER: phoenix_evaluate_target(boss, t); break;
    }
}

// 0x4a2f60  Confidence: likely (the candidate block is written out twice, once per loop, as in the
// binary; the order of the query field stores). The boss's candidates: in single player the members of
// cur_character's AI group, in multiplayer every party record's character. A candidate is kept when
// ai_score_target finds it within the fov range, or when it has threat whose 20 s timer runs.
int boss_target_list_build(Character *boss, boss_target *list, boss_threat *table)
{
    int n = 0;

    if (game_flags & GF_SINGLE_PLAYER) {
        ai_group *g = cur_character->ai.group;
        int i;

        if (g) {
            for (i = 0; i < g->count; i++) {
                Character *c = living_from_handle(g->members[i]);
                if (c) {
                    ai_target_query q;
                    bool seen;
                    int t;

                    q._unk08 = 10;
                    q.best = boss->info->fov_range;
                    q.result = 0;
                    q.self = boss;
                    q.candidate = c;
                    ai_score_target(&q, 1);
                    seen = q.best < boss->info->fov_range;
                    t = boss_threat_find(c->handle, table);
                    if ((t != -1 && !table[t].timer.elapsed()) || seen) {
                        list->character = c;
                        list->dist = boss->distance_between_edges(c, true, -1);
                        list->flags = 0;
                        boss_target_filter_dispatch(boss, list);
                        n++;
                        list++;
                    }
                }
            }
        }
    } else {
        Player *p;

        for (p = party_list.next; p != &party_list; p = p->next) {
            Character *c = living_from_handle(p->handle);
            if (c) {
                ai_target_query q;
                bool seen;
                int t;

                q._unk08 = 10;
                q.best = boss->info->fov_range;
                q.result = 0;
                q.self = boss;
                q.candidate = c;
                ai_score_target(&q, 1);
                seen = q.best < boss->info->fov_range;
                t = boss_threat_find(c->handle, table);
                if ((t != -1 && !table[t].timer.elapsed()) || seen) {
                    list->character = c;
                    list->dist = boss->distance_between_edges(c, true, -1);
                    list->flags = 0;
                    boss_target_filter_dispatch(boss, list);
                    n++;
                    list++;
                }
            }
        }
    }
    return n;
}

// 0x4a31c0  Confidence: likely (the threat sum is accumulated as "sum -= t1 + t0"; the weight is
// 2 + 0.1 * sum - (3 * t0 + t1), recomputed per candidate; the found index is tested against -1).
// mask 0x80000000 picks the nearest candidate (start 1000). Otherwise the candidates whose flags share
// a bit with mask (all when mask is 0) get a weight from the threat amounts (negative HP deltas, so more
// damage means more weight); cur_target's weight is x1.2, or 0 with exclude_cur. The cumulative weights
// are stored in score, normalised and sampled with frand.
int boss_target_list_pick(boss_target *list, int count, boss_threat *table, int cur_target,
                          bool exclude_cur, unsigned int mask)
{
    int handle;
    int i;

    if (!count)
        return -1;
    handle = -1;
    if (mask == 0x80000000) {
        float best = 1000.0f;
        for (i = 0; i < count; i++) {
            if (list[i].character && list[i].dist < best) {
                handle = list[i].character->handle;
                best = list[i].dist;
            }
        }
        return handle;
    } else {
        float total = 0.0f;
        float sum = 0.0f;
        int candidates;
        int pick;
        float r;

        for (i = 0; i < count; i++) {
            if (list[i].character) {
                int t = boss_threat_find(list[i].character->handle, table);
                if (t != -1)
                    sum -= table[t].threat[1] + table[t].threat[0];
            }
        }
        candidates = 0;
        for (i = 0; i < count; i++) {
            bool is_cur;
            bool use = true;

            if (list[i].character && list[i].character->handle == cur_target)
                is_cur = true;
            else
                is_cur = false;
            if (mask && !(list[i].flags & mask))
                use = false;
            if (list[i].character && use) {
                float w;
                int t;

                candidates++;
                w = sum * 0.1f + 2.0f;
                t = boss_threat_find(list[i].character->handle, table);
                if (t != -1)
                    w -= table[t].threat[0] * 3.0f + table[t].threat[1];
                if (is_cur) {
                    w *= 1.2f;
                    if (exclude_cur)
                        w = 0.0f;
                }
                total += w;
                list[i].score = total;
            } else
                list[i].score = total;
        }
        if (candidates) {
            for (i = 0; i < count; i++)
                list[i].score /= total;
            r = frand();
            pick = -1;
            for (i = 0; i < count; i++) {
                if (r <= list[i].score) {
                    pick = i;
                    break;
                }
            }
            if (pick != -1)
                return list[pick].character->handle;
        }
    }
    return -1;
}

// 0x4a33b0  Confidence: likely (store order). The *_set_solo console / script path: the mount is
// unlinked, loses its target and is deleted (obj flags: active off, delete on).
void rider_unlink_mount(Character *rider)
{
    Character *mount = rider->get_mount();

    if (mount) {
        mount->flags &= ~0x20;
        mount->mount_flags = 0;
        mount->mount_link = -1;
        mount->ai.target = -1;
        mount->obj_flags = (mount->obj_flags & ~1) | 4;
        rider->mount_flags = 0;
        rider->mount_link = -1;
    }
}

// 0x4a3410  Confidence: exact (hi - lo is stored into hi's slot for fimul).
int rand_int_range(int lo, int hi)
{
    return lo + (int)(frand() * (hi - lo));
}

// 0x4a3440  Confidence: likely (the -1 result variable in EDI). Weighted random index: the running sums
// (at most 10, no check) are normalised and the first one >= frand() wins.
int rand_weighted_index(float *weights, int n)
{
    float cum[10];
    float sum = 0.0f;
    int result;
    float r;
    int i;

    for (i = 0; i < n; i++) {
        sum += weights[i];
        cum[i] = sum;
    }
    for (i = 0; i < n; i++)
        cum[i] /= sum;
    result = -1;
    r = frand();
    for (i = 0; i < n; i++)
        if (r <= cum[i])
            return i;
    return result;
}

// 0x4a34d0  Confidence: exact. state_game_enter.
void boss_rider_sound_timer_reset()
{
    boss_rider_sound_timer.SetTimeout(0);
}

// 0x4a34e0  Confidence: exact. The *_seat_rider functions: when the timer has run out, a random mount
// sound (8, 9 or 10, a quarter of the time none) unless a cutscene runs (game_flags 0x20) or bit 0x200
// is clear; next try in 2-8 s.
void boss_rider_random_sound(vector3 *pos)
{
    if (boss_rider_sound_timer.elapsed()) {
        if (!(game_flags & 0x20) && (game_flags & GF_LEADER_SOUND)) {
            float r = frand();
            if (r < 0.25f)
                sound_play_3d_at(8, pos);
            else if (r < 0.5f)
                sound_play_3d_at(9, pos);
            else if (r < 0.75f)
                sound_play_3d_at(10, pos);
        }
        boss_rider_sound_timer.SetRandomTimeout(2000, 8000);
    }
}

// 0x4a3570  Confidence: exact. level_load_files.
void level_preload_effects()
{
    effects_load_preload_rfx();
}

// 0x4a3580  Confidence: exact (decomp57). level_load_files: the level textures, then per living entity
// its mesh textures, its alternate texture set's bitmaps (when the record has an alternate parent) and
// its sounds; then the lamp glow bitmap and the two first sounds.
void level_page_in_characters()
{
    Character *c;
    char_info *info;
    int i;

    level_page_in_textures();
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        c->instance->page_in_textures();
        info = c->info;
        if (info && info->alt_parent) {
            for (i = 0; i < info->alt_textures->count; i++)
                gr_page_in(info->alt_textures->bitmaps[i], 0);
        }
        charinfo_page_in_sounds(c->info);
    }
    if (lamp_glow_bitmap != -1)
        gr_page_in(lamp_glow_bitmap, 0);
    snd_load(0, 0);
    snd_load(1, 0);
}
