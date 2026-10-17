// unnamed_evil_joseph_0x4c3470.cpp - Sum.exe v1.40, code 0x4c3470-0x4c4167: the Evil Joseph boss ("dark
// Joseph", species 0xc9) and his four helpers. File name NOT known: a placeholder (src/decomp/README.md). No
// assert path, no strings.tbl block; cut by its static initialiser run (entries 323-324, 0x588510-0x588514)
// and its data.
// Written from the Sum.exe disassembly (tools/asm.mjs; jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other boss units. Confidence per function: exact =
// expect identical code, likely = same semantics, small layout risk, unsure = the source shape is a guess.
//
// Unit range: static_init_evil_joseph_timers (0x4c3470) to evil_joseph_destroy_effects (0x4c40d0-0x4c4167).
// The Evil Urath unit opens with its initialiser 0x4c4170 (entry 325). boss_target_list_find (0x4c3c70),
// which the phoenix and Machival units call too, and boss_resurrect_full (0x4c3fb0), which the Ghost Rider
// calls, are defined here; the Evil Urath unit has its own copy of the list search (0x4c4580).
// Data: .data 0x59dcb8-0x59dd3b (command name and help ("dark josesph" as spelt), the combat message,
// "darkjo_boss.tbl", "Sword of Summoners"; "$tele_0%d" is the Machival unit's string, 0x59dbe4, shared by
// the linker's string pooling); .bss 0x248b978-0x248bcfc (evil_joseph_move_1 first, evil_joseph_loaded
// last; Evil Urath's data starts at 0x248bd00).
// Return codes of evil_joseph_ai_select_action: 0 no target, 1 walk to the teleport point of the HP band
// (sub-mode 0x20), 3 a new move, 5 keep the current move. The frame also accepts 4 (never returned).

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "engine/ai/boss_ai.h"
#include "engine/ai/ai.h"
#include "engine/ai/pathfinding.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/combat/combat.h"
#include "engine/spells/spells.h"
#include "engine/effects/fx.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/script_internal.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/ca/character.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"

#define AIS_CLOSE_COMBAT     0x0f     // as in ai.cpp (sub-mode names 0x59ad94)
#define AIS_GET_CLOSE        0x16
#define AIS_GET_FAR          0x17
#define AIS_FOLLOW_PATH      0x20     // walking to a teleport point (name ours)

#define SPECIES_EJ_HELPER_0  0xc0     // the four helpers (bosses.md "Helper revive")
#define SPECIES_EJ_HELPER_1  0xca
#define SPECIES_EJ_HELPER_2  0xcb
#define SPECIES_EJ_HELPER_3  0xb0

// The helper summon spells: 0x5c, 0x5e, 0x5f and 0x60 (0x5d is not one; NOTES).
#define EJ_IS_SUMMON_SPELL(s) ((s) == 0x5c || ((s) > 0x5d && (s) <= 0x60))

// ---- .bss (address order) -----------------------------------------------------------------------------
boss_attack *evil_joseph_move_1;      // 0x248b978 &evil_joseph_moves[0][1]
// ccmd_darkjo_combat 0x248b980 is defined with its initialiser below.
boss_attack *evil_joseph_move_4;      // 0x248b98c [0][4]
boss_attack *evil_joseph_move_2;      // 0x248b990 [0][2]
boss_attack *evil_joseph_move_5;      // 0x248b994 [0][5]
boss_attack *evil_joseph_move_3;      // 0x248b998 [0][3]
boss_attack *evil_joseph_move_6;      // 0x248b99c [0][6]
boss_attack *evil_joseph_move_7;      // 0x248b9a0 [0][7]
int  evil_joseph_248b9a4;             // 0x248b9a4 never accessed
int  evil_joseph_248b9a8;             // 0x248b9a8 1 at load, never read
int  evil_joseph_248b9ac;             // 0x248b9ac 1 at load, never read
int  evil_joseph_248b9b0;             // 0x248b9b0 zeroed when the target is lost, never read
bool evil_joseph_combat_enabled;      // 0x248b9b4
boss_threat evil_joseph_threat_table[BOSS_MAX_THREATS];   // 0x248b9b8
int  evil_joseph_mesh_tag_count;      // 0x248ba30
boss_offense_point evil_joseph_offense_points[10];        // 0x248ba34 (only the tags are filled)
boss_attack evil_joseph_moves[1][BOSS_MAX_OFFENSE_POINTS];   // 0x248bb4c "darkjo_boss.tbl"
boss_attack *evil_joseph_current_move;   // 0x248bc8c
int  evil_joseph_248bc90;             // 0x248bc90 never accessed
int  evil_joseph_target_handle;       // 0x248bc94
GameTimer evil_joseph_attack_timer;   // 0x248bc98
int  evil_joseph_attack_move_id;      // 0x248bc9c 1..3 for move 8
int  evil_joseph_summon_handle;       // 0x248bca0
int  evil_joseph_helpers[4];          // 0x248bca4
vector3 evil_joseph_summon_pos;       // 0x248bcb4
int  evil_joseph_resurrect_count;     // 0x248bcc0
int  evil_joseph_helper_index;        // 0x248bcc4
int  evil_joseph_teleport_index;      // 0x248bcc8
int  evil_joseph_248bccc[3];          // 0x248bccc never accessed
navpoint *evil_joseph_teleports[5];   // 0x248bcd8
int  evil_joseph_248bcec;             // 0x248bcec -1 at load and combat start, never read
int  evil_joseph_248bcf0;             // 0x248bcf0 -1 at load and combat start, never read
boss_attack *evil_joseph_move_8;      // 0x248bcf4 [0][8] the plain attack (move id 1..3)
boss_attack *evil_joseph_move_0;      // 0x248bcf8 [0][0]
bool evil_joseph_loaded;              // 0x248bcfc

// 0x4c3470  Confidence: exact. static_init_evil_joseph_timers: "jmp 0x4c3480" (initialiser entry 323).
// 0x4c3480  Confidence: likely. init_evil_joseph_timers: the GameTimer constructor for the five threat
// records and evil_joseph_attack_timer.

// 0x4c34b0  Confidence: exact. static_init_ccmd_darkjo_combat: "jmp 0x4c34c0" (entry 324).
// 0x4c34c0  Confidence: exact. init_ccmd_darkjo_combat (console_command constructor).
console_command ccmd_darkjo_combat("darkjo_combat", "Sets dark josesph in combat state", cmd_darkjo_combat);   // 0x248b980

// 0x4c34e0  Confidence: exact.
void cmd_darkjo_combat()
{
    evil_joseph_combat_enabled = true;
    print_to_console("Setting dark joseph's ai state to combat\n", 0);
}

// 0x4c3500  Confidence: likely (the species list is a local array). Creates the four helpers one unit in
// front of the boss, inactive and hidden; the handles are packed (a failed create leaves no gap).
void evil_joseph_spawn_helpers(Character *boss)
{
    int species[4];
    int *handle;
    Character *c;
    int i;

    species[0] = SPECIES_EJ_HELPER_0;
    species[1] = SPECIES_EJ_HELPER_1;
    species[2] = SPECIES_EJ_HELPER_2;
    species[3] = SPECIES_EJ_HELPER_3;
    handle = evil_joseph_helpers;
    for (i = 0; i < 4; i++) {
        int info = charinfo_find_by_species(species[i]);
        vector3 pos = boss->pos + boss->orient.fvec;
        c = living_entity_create(5, info, &pos, &boss->orient, true, boss->layer, 0, 0, false);
        if (c) {
            *handle++ = c->handle;
            c->set_ai_active(0);
            c->flags |= CF_HIDDEN;
        }
    }
}

// 0x4c35e0  Confidence: likely. Runs once per level (evil_joseph_unload clears the flag). The boss
// carries the "Sword of Summoners" as his one attachment, starts hidden (flag 8, plus the ready stance
// flag 1) and creates his helpers.
void ai_state::evil_joseph_boss_load()
{
    int i;
    int tag;

    if (evil_joseph_loaded == true)
        return;
    evil_joseph_248b9a8 = 1;
    evil_joseph_248b9ac = 1;
    evil_joseph_loaded = true;
    evil_joseph_target_handle = -1;
    for (i = 0; i < BOSS_MAX_THREATS; i++)
        evil_joseph_threat_table[i].handle = -1;
    evil_joseph_mesh_tag_count = 0;
    for (i = 0; i < 10; i++) {
        tag = owner->instance->character->find_mesh_tag(offense_point_name(i));
        if (tag != -1) {
            evil_joseph_offense_points[evil_joseph_mesh_tag_count].tag = tag;
            evil_joseph_mesh_tag_count++;
        }
    }
    owner->attachments[0].item_type = item_type_find("Sword of Summoners");
    owner->attachments[0].slot = 0;
    owner->num_attachments = 1;
    evil_joseph_find_teleports();
    evil_joseph_teleport_index = -1;
    evil_joseph_init_move_tables();
    owner->flags |= CF_COMBAT_READY;
    owner->flags |= CF_HIDDEN;
    boss_ai_parse_table("darkjo_boss.tbl", 1, evil_joseph_moves, evil_joseph_mesh_tag_count);
    evil_joseph_resurrect_count = 0;
    evil_joseph_spawn_helpers(owner);
    evil_joseph_helper_index = -1;
    evil_joseph_248bcf0 = -1;
    evil_joseph_248bcec = -1;
}

// 0x4c3720  Confidence: exact (VC6 byte match, match24; was likely) (the numbers are a local array). Teleport points 1..4 (by HP band) are
// "$tele_03", "$tele_01", "$tele_05", "$tele_06"; index 0 ("$tele_02") is never used.
void evil_joseph_find_teleports()
{
    int numbers[5];
    char name[64];
    int i;

    numbers[0] = 2;
    numbers[1] = 3;
    numbers[2] = 1;
    numbers[3] = 5;
    numbers[4] = 6;
    for (i = 0; i < 5; i++) {
        sprintf(name, "$tele_0%d", numbers[i]);
        evil_joseph_teleports[i] = navpoint_find(name);
    }
}

// 0x4c3790  Confidence: exact (store order as in the binary).
void evil_joseph_init_move_tables()
{
    evil_joseph_move_0 = &evil_joseph_moves[0][0];
    evil_joseph_move_1 = &evil_joseph_moves[0][1];
    evil_joseph_move_2 = &evil_joseph_moves[0][2];
    evil_joseph_move_3 = &evil_joseph_moves[0][3];
    evil_joseph_move_4 = &evil_joseph_moves[0][4];
    evil_joseph_move_5 = &evil_joseph_moves[0][5];
    evil_joseph_move_6 = &evil_joseph_moves[0][6];
    evil_joseph_move_7 = &evil_joseph_moves[0][7];
    evil_joseph_move_8 = &evil_joseph_moves[0][8];
}

// 0x4c37f0  Confidence: exact. boss_ai_unload case 0xc9.
void evil_joseph_unload()
{
    evil_joseph_loaded = false;
}

// 0x4c3800  Confidence: exact. boss_ai_idle_dispatch case 0xc9: the combat flag (console, the Urath death
// script) also makes the boss visible.
void ai_state::evil_joseph_ai_idle()
{
    if (owner->get_current_move() == NULL && owner->combat.state != COMBAT_IDLE) {
        owner->combat.state = COMBAT_IDLE;
        return;
    }
    if (evil_joseph_combat_enabled == true) {
        evil_joseph_current_move = NULL;
        evil_joseph_248bcf0 = -1;
        evil_joseph_248bcec = -1;
        owner->flags &= ~CF_HIDDEN;
        enter_combat();
    }
}

// 0x4c3860  Confidence: likely (the sub-mode switch starts at 0xf: no case -1; byte table 0x4c3968).
void ai_state::evil_joseph_ai_frame()
{
    int result;
    Character *target;
    navpoint *nav;

    result = evil_joseph_ai_select_action();
    target = living_from_handle(evil_joseph_target_handle);
    owner->combat.target = evil_joseph_target_handle;
    if (target == NULL || (target->flags & CF_HIDDEN)) {
        target = NULL;
        if (submode != AIS_FOLLOW_PATH) {
            go_idle();
            return;
        }
    }
    if (result == 4)
        return;
    if (result == 3) {
        combat_approach(target);
    } else if (result == 1) {
        nav = evil_joseph_teleports[evil_joseph_teleport_index];
        path_find_new(&owner->pos, owner->layer, &nav->pos, nav->layer, owner->info->size, 0.0f);
        set_mode(AI_STATE_COMBAT, AIS_FOLLOW_PATH);
    }
    switch (submode) {
    case AIS_CLOSE_COMBAT:
        combat_melee_attack(target);
        break;
    case AIS_FOLLOW_PATH:
        if (!follow_path(0))
            go_idle();
        break;
    case AIS_GET_CLOSE:
    case AIS_GET_FAR:
        combat_close_in(target);
        break;
    }
}

// 0x4c3980  Confidence: exact. boss_cast_special_dispatch case 0xc9: the helper summons are cast directly.
bool evil_joseph_cast_special(Character *c, int anim)
{
    int spell;

    if (evil_joseph_current_move == NULL)
        return false;
    spell = evil_joseph_current_move->spell;
    if (!EJ_IS_SUMMON_SPELL(spell))
        return false;
    c->perform_spell_cast(spell, false);
    return true;
}

// 0x4c39b0  Confidence: likely (the HP band ladder shares the "return 1" tails). Below 80 / 60 / 40 / 20 %
// HP the boss walks once to teleport point 1 / 2 / 3 / 4. Otherwise: while no helper summon is out, a
// summon move for the next helper; else move 8 with a random move id 1..3.
int ai_state::evil_joseph_ai_select_action()
{
    boss_target list[BOSS_MAX_THREATS];
    Character *target;
    bool moving;
    bool busy;
    bool timer_up;
    int count;
    int handle;
    int index;

    float frac;

    if (submode == AIS_FOLLOW_PATH)
        return 5;
    moving = owner->get_current_move() != NULL;
    busy = owner->combat.is_busy();
    if (!moving && busy) {
        busy = moving;
        owner->combat.state = COMBAT_IDLE;
    }
    target = living_from_handle(evil_joseph_target_handle);
    if (target && (target->flags & CF_DEAD))
        target = NULL;
    if (target == NULL) {
        evil_joseph_current_move = NULL;
        evil_joseph_248b9b0 = 0;
    }
    if (busy == true)
        return evil_joseph_current_move ? 5 : 0;
    if (evil_joseph_current_move)
        return 5;

    timer_up = evil_joseph_attack_timer.elapsed();
    count = boss_target_list_build(owner, list, evil_joseph_threat_table);
    if (timer_up || !target) {
        evil_joseph_target_handle = boss_target_list_pick(list, count, evil_joseph_threat_table,
                                                          evil_joseph_target_handle, false, 1);
        if (evil_joseph_target_handle == -1)
            return 0;
        living_from_handle(evil_joseph_target_handle);
        evil_joseph_set_attack_timer(20000, 30000);
    }
    frac = owner->hp / (float)owner->get_max_stat(0);
    if (frac < 0.2f) {
        if (evil_joseph_teleport_index != 4) {
            evil_joseph_teleport_index = 4;
            return 1;
        }
    } else if (frac < 0.4f) {
        if (evil_joseph_teleport_index != 3) {
            evil_joseph_teleport_index = 3;
            return 1;
        }
    } else if (frac < 0.6f) {
        if (evil_joseph_teleport_index != 2) {
            evil_joseph_teleport_index = 2;
            return 1;
        }
    } else if (frac < 0.8f) {
        if (evil_joseph_teleport_index != 1) {
            evil_joseph_teleport_index = 1;
            return 1;
        }
    }
    index = boss_target_list_find(list, count, evil_joseph_target_handle);
    if (index != -1 && !(list[index].flags & 1)) {
        handle = boss_target_list_pick(list, count, evil_joseph_threat_table, evil_joseph_target_handle, false, 1);
        if (handle == -1)
            return 0;
        evil_joseph_target_handle = handle;
    }
    if (evil_joseph_resurrect_count == 0) {
        evil_joseph_pick_helper_summon(owner);
        if (evil_joseph_current_move)
            return 3;
    }
    evil_joseph_current_move = evil_joseph_move_8;
    switch (rand() % 3) {
    case 0:
        evil_joseph_attack_move_id = 1;
        break;
    case 1:
        evil_joseph_attack_move_id = 2;
        return 3;
    case 2:
        evil_joseph_attack_move_id = 3;
        return 3;
    }
    return 3;
}

// 0x4c3c70  Confidence: exact. The candidate index of a handle, -1 none (the characters are not
// NULL-checked).
int boss_target_list_find(boss_target *list, int count, int handle)
{
    int i;

    for (i = 0; i < count; i++) {
        if (list[i].character->handle == handle)
            return i;
    }
    return -1;
}

// 0x4c3ca0  Confidence: exact.
void evil_joseph_set_attack_timer(int min_ms, int max_ms)
{
    evil_joseph_attack_timer.SetTimeout(min_ms + (int)(frand() * (max_ms - min_ms)));
}

// 0x4c3cd0  Confidence: likely. The next helper in turn (the first four calls), then a random one that is
// alive, active and not the last one (five tries). The summon point is halfway to the target (or
// cur_character); when that is within 4 of the target, a navigable point 4-8 from the boss in a random
// direction (ten tries). The summon move is the helper's index in the move table.
void evil_joseph_pick_helper_summon(Character *boss)
{
    Character *c;
    Character *t;
    vector3 dir;
    float r;
    int tries;
    int i;

    evil_joseph_summon_handle = -1;
    evil_joseph_current_move = NULL;
    evil_joseph_helper_index++;
    if (evil_joseph_helper_index < 4) {
        evil_joseph_summon_handle = evil_joseph_helpers[evil_joseph_helper_index];
    } else {
        for (tries = 0; tries < 5; tries++) {
            i = rand() % 4;
            c = living_from_handle(evil_joseph_helpers[i]);
            if (c && !(c->obj_flags & 1) && !(c->flags & CF_DEAD) && evil_joseph_summon_handle != evil_joseph_helpers[i]) {
                evil_joseph_summon_handle = evil_joseph_helpers[i];
                break;
            }
        }
    }
    if (evil_joseph_summon_handle == -1)
        return;
    evil_joseph_summon_pos = boss->pos;
    t = living_from_handle(evil_joseph_target_handle);
    if (t == NULL) {
        t = cur_character;
        if (t == NULL)
            return;
    }
    dir.x = boss->pos.x + t->pos.x;
    dir.y = t->pos.y + boss->pos.y;
    dir.z = t->pos.z + boss->pos.z;
    evil_joseph_summon_pos.x = dir.x * 0.5f;
    evil_joseph_summon_pos.y = dir.y * 0.5f;
    evil_joseph_summon_pos.z = dir.z * 0.5f;
    float dx = evil_joseph_summon_pos.x - t->pos.x;
    float dz = evil_joseph_summon_pos.z - t->pos.z;
    if ((float)sqrt(dx * dx + dz * dz) < 4.0f) {
        for (tries = 0; tries < 10; tries++) {
            r = frand_range(4.0f, 8.0f);
            dir.random_direction();
            dir.y = 0.0f;
            dir.normalize_safe();
            dir.x = dir.x * r + boss->pos.x;
            dir.y = dir.y * r + boss->pos.y;
            dir.z = dir.z * r + boss->pos.z;
            if (pf_point_is_navigable(&dir, t->info->size, boss->layer) == true)
                break;
        }
        if (tries == 10)
            return;
        evil_joseph_summon_pos = dir;
    }
    for (i = 0; i < 4; i++) {
        if (evil_joseph_summon_handle == evil_joseph_helpers[i]) {
            evil_joseph_current_move = &evil_joseph_moves[0][i];
            return;
        }
    }
}

// 0x4c3f20  Confidence: exact (VC6 byte match, match24; was likely). boss_ai_check_stage case 0xc9: a dead helper that is not active is revived
// once its death effect has ended (or it has none); each revival lowers the summon count, so the boss
// summons again only when every summoned helper has been revived (bosses.md "Helper revive").
void ai_state::evil_joseph_check_stage()
{
    Character *c;
    effect_object *fx;
    int i;

    for (i = 0; i < 4; i++) {
        c = (Character *)obj_from_handle(evil_joseph_helpers[i]);
        if (c == NULL || !(c->flags & CF_DEAD) || (c->obj_flags & 1))
            continue;
        if (c->get_move_effect() != -1) {
            fx = (effect_object *)obj_from_handle(c->get_move_effect());
            if (fx && fx->instance->get_time_left() > 0.0001f)
                continue;
        }
        evil_joseph_resurrect_count--;
        boss_resurrect_full(c);
    }
}

// folded (0x4f13e0, empty_function)  Confidence: likely. The second call of boss_ai_check_stage case 0xc9.
void ai_state::evil_joseph_check_stage_b()
{
}

// 0x4c3fb0  Confidence: exact. Back to life at full HP with fresh anims, inactive and hidden (the summon
// spell shows it again).
void boss_resurrect_full(Character *c)
{
    c->resurrect();
    c->hp = (float)c->get_max_stat(0);
    c->instance->clear_state_anims();
    c->instance->clear_action_anims();
    c->update_locomotion_anim();
    c->instance->process(0.0f, false, 0);
    c->set_ai_active(0);
    c->flags |= CF_HIDDEN;
}

// 0x4c4020  Confidence: exact. boss_attack_trigger_dispatch case 0xc9: the summon spell on the chosen
// helper at the summon point.
void evil_joseph_fire_attack(Character *boss, Character *target)
{
    if (evil_joseph_current_move == NULL || !EJ_IS_SUMMON_SPELL(evil_joseph_current_move->spell))
        return;
    evil_joseph_resurrect_count++;
    spell_cast_execute(boss, evil_joseph_current_move->spell, evil_joseph_summon_handle, &evil_joseph_summon_pos,
                       boss->layer, 0, 0, 0);
}

// 0x4c4080  Confidence: exact. boss_target_filter_dispatch case 0xc9: flag 1 within 0.9 of the boss's
// char_info +0x220 range.
void evil_joseph_filter_target(Character *boss, boss_target *t)
{
    if (boss->info->fov_range * 0.9f > t->dist)
        t->flags |= 1;
}

// 0x4c40b0  Confidence: exact. The current move is not NULL-checked.
int evil_joseph_get_attack_move_id()
{
    if (evil_joseph_current_move == evil_joseph_move_8)
        return evil_joseph_attack_move_id;
    return evil_joseph_current_move->variation;
}

// 0x4c40d0  Confidence: exact (VC6 byte match, match24; was likely) (the next pointer is read before the test). Hides and deactivates the
// helpers and destroys their kind-8 effects (the level scripts' call).
void evil_joseph_destroy_effects()
{
    Character *c;
    effect_object *fx;
    effect_object *next;
    int i;

    for (i = 0; i < 4; i++) {
        c = living_from_handle(evil_joseph_helpers[i]);
        if (c == NULL)
            continue;
        c->flags |= CF_HIDDEN;
        c->obj_flags &= ~1;
        for (fx = fx_active_list.fx_next, next = fx->fx_next; fx != &fx_active_list; fx = next, next = next->fx_next) {
            if (fx->kind == 8 && fx->owner == c->handle)
                fx_destroy(fx->handle);
        }
    }
}
