// combat.cpp - Sum.exe v1.40, code 0x463360-0x468c77 (module "combat.cpp" in sum.symbols / strings.tbl:
// ids 215-217; no path string, so engine/combat/combat.h's directory is guessed).
// Written from the Sum.exe disassembly (tools/asm.mjs fn; the jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other optimised units (no frame pointer, 0x90 padding).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: init_ccmd_bloodspots_thunk (0x463360; the bloodspots and death console commands, tagged
// living_entity.cpp in sum.symbols, moved here by D6100) and living_combat_send_net (0x463540) to
// living_combat_set_40 (0x468c70-0x468c77). The previous function, cmd_Spell_ap (0x4632a0), is
// living_entity.cpp's last; the next, sinit_stringpool_941740
// (0x468c80), is gamesound.cpp's first. Every function is written (decomp75 added
// combat_trigger_special_ability, living_combat_update_boss_attack and blood_splot_place, D5700-D5701;
// decomp73 combat_trigger_reflex_ability, D5506).
//
// Most functions are thiscall members of living_combat (Character +0x558); +0x08 is the other side of
// the exchange. The "skill" of an attack (+0x3c) is a skill id (characterinfo.h SKILL_*).
//
// Data: .data 0x59a16c-0x59a31b: Debug_death, the console commands' literals, then the combat literals
// ("Queueing counter attack" 0x59a1bc first, "BloodSplot-large02.tga" last); "Super_power" / "Toggles
// Super_power" and the console macro texts (pooled, 0x588c34-0x588c50) and "!" (pooled, 0x594438).
// .bss 0x941710-0x94173d: ccmd_death, ccmd_Super_power, ccmd_bloodspots, Bloodspots, Super_power. This
// block was read as two interleaved units; it is one object's (D6100).
//
// Inline expansions (as in objects.cpp, DECISIONS.md D66): this unit expands vector3 / matrix
// assignment inline in places (combat_fire_weapon_missile, combat_get_weapon_tag_position) while
// calling vector3's copy constructor; the code below uses the headers' operators. The dot products are
// evaluated z, y, x (an inline helper, written out here).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <new>
#include "engine/combat/combat.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/items.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/spells/spells.h"
#include "engine/effects/fx.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/gamesound/gamesound.h"
#include "engine/rendering/levelrender.h"
#include "engine/rendering/unnamed_decals_0x4bbe80.h"
#include "vsdk/os/console.h"
#include "vsdk/os/frametime.h"
#include "vsdk/localize/localize.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/skeleton.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/interface.h"

// The multiplayer roles as the code tests them (game_flags bit 1 = multiplayer, mp_player bit 0 = host).
#define MP_HOST   ((game_flags & 2) && mp_local_player && (mp_local_player->flags & 1))
#define MP_CLIENT ((game_flags & 2) && mp_local_player && !(mp_local_player->flags & 1))

// ---- .data (0x59a16c-0x59a31b) -------------------------------------------------------------------
// The object's only initialised global comes first, then the literals of the two console commands
// below, then the combat literals ("Queueing counter attack" 0x59a1bc first). Its address, between
// living_entity.cpp's last literal ("Spell_ap") and the bloodspots literals, is where this object's
// .data starts: VC6 puts an object's initialised globals before its literals (D6100).
bool Debug_death = true;              // 0x59a16c "death"

// ---- .bss ----------------------------------------------------------------------------------------
bool Super_power;                                      // 0x94173d
bool Bloodspots;                                       // 0x94173c (only cmd_bloodspots uses it)

// ---- 0x463360-0x46353f: the bloodspots and death console commands ------------------------------------
// sum.symbols tags them living_entity.cpp, but they belong to this object (D6100): Debug_death above,
// and the .bss of ccmd_death 0x941710, ccmd_bloodspots 0x941730 and Bloodspots 0x94173c interleaves with
// this unit's ccmd_Super_power 0x941720 and Super_power 0x94173d (0x941710-0x94173d is one block, the
// living_entity.cpp object ends with move_play_serial 0x941708, gamesound.cpp starts at 0x941740).

// 0x463360 init_ccmd_bloodspots_thunk: jmp to the initialiser. Confidence: exact.
// 0x463370 init_ccmd_bloodspots: the console_command constructor. Confidence: exact.
console_command ccmd_bloodspots("bloodspots", "Toggles Bloodspots", cmd_bloodspots);   // 0x941730

// 0x463390  Confidence: exact.
void cmd_bloodspots()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Bloodspots = true;
        else if (arg_type & ARG_FALSE)
            Bloodspots = false;
        else if (arg_type & ARG_NONE)
            Bloodspots ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "bloodspots", "Bloodspots");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "bloodspots", Bloodspots ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x463450 init_ccmd_death_thunk: jmp to the initialiser. Confidence: exact.
// 0x463460 init_ccmd_death: the console_command constructor. Confidence: exact.
console_command ccmd_death("death", "Toggles Debug_death", cmd_death);   // 0x941710

// 0x463480  Confidence: exact.
void cmd_death()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Debug_death = true;
        else if (arg_type & ARG_FALSE)
            Debug_death = false;
        else if (arg_type & ARG_NONE)
            Debug_death ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "death", "Debug_death");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "death", Debug_death ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x463540  Confidence: likely (register choice per branch). Sends the exchange to the other machines:
// the attacking side's handles first.
void living_combat::send_net(int state_arg, bool special)
{
    int a, b, variant, skill_id;
    move_info *defence;
    int net_a;

    switch (state) {
    case COMBAT_ATTACKING:
        a = owner->handle;
        b = opponent->owner->handle;
        variant = move_variant;
        defence = opponent->defense_move;
        skill_id = skill;
        break;
    case COMBAT_SPECIAL:
        a = owner->handle;
        b = opponent->owner->handle;
        variant = 0x20;
        defence = opponent->defense_move;
        skill_id = skill;
        break;
    default:
        a = opponent->owner->handle;
        b = owner->handle;
        variant = opponent->move_variant;
        defence = defense_move;
        skill_id = opponent->skill;
        break;
    }
    net_a = mp_handle_to_net(a);
    mp_send_living_combat(net_a, mp_handle_to_net(b), variant, defence, state_arg, skill_id, special);
}

// 0x4635e0  Confidence: exact (store order as in the binary).
void living_combat::reset()
{
    state = COMBAT_IDLE;
    target = -1;
    anim = 0;
    defense_move = NULL;
    opponent = NULL;
    move_variant = 0;
    chain_skill = SKILL_NONE;
    skill = SKILL_NONE;
    cooldown = 0.0f;
    chain_learn = false;
    attack_turn = false;
    chain_count = 0;
    chain_index = 0;
    last_chain_skill = SKILL_NONE;
}

// 0x463620  Confidence: exact.
void living_combat::play_attack(move_info *move)
{
    anim = owner->play_move((int)move, 1.0f, 0, 0, 0);
    if (anim != -1)
        state = COMBAT_ATTACKING;
    else
        state = COMBAT_IDLE;
}

// 0x463660  Confidence: exact.
void living_combat::play_special(move_info *move)
{
    anim = owner->play_move((int)move, 1.0f, 0, 0, 0);
    if (anim != -1)
        state = COMBAT_SPECIAL;
    else
        state = COMBAT_IDLE;
}

// 0x4636a0  Confidence: exact. Knocked-down characters (AI 8 / 0x2e) do not flinch; a party member
// without flag 1 uses the flinch category 5.
void living_combat::play_flinch_secondary(int variant)
{
    int category;
    move_info *m;

    if (owner->ai.is_mode(AI_STATE_DISABLED, 0x2e) == true) {
        state = COMBAT_IDLE;
        return;
    }
    category = 4;
    if (owner->type == 7 && !(owner->flags & 1))
        category = 5;
    m = owner->find_move_alt(category, variant);
    if (!m) {
        state = COMBAT_IDLE;
        return;
    }
    flinch_anim = owner->play_move((int)m, 1.0f, 0, 1, 0);
    if (flinch_anim == -1)
        state = COMBAT_IDLE;
    else
        state = COMBAT_REACTING;
}

// 0x463730  Confidence: exact. Bosses never flinch (the type 9 test before character_is_not_boss makes
// that call always false).
void living_combat::play_flinch(int variant)
{
    int category;
    move_info *m;

    if (owner->ai.is_mode(AI_STATE_DISABLED, 0x2e) == true) {
        state = COMBAT_IDLE;
        return;
    }
    if (owner->type == 9 && !character_is_not_boss(owner))
        return;
    category = 4;
    if (owner->type == 7 && !(owner->flags & 1))
        category = 5;
    m = owner->find_move_alt(category, variant);
    if (!m) {
        state = COMBAT_IDLE;
        return;
    }
    anim = owner->play_move((int)m, 1.0f, 0, 1, 0);
    if (anim == -1)
        state = COMBAT_IDLE;
    else
        state = COMBAT_REACTING;
}

// 0x4637d0  Confidence: exact. A parry move matching the attacker's move variant, else the default one.
void living_combat::pick_parry()
{
    if (opponent && opponent->owner && opponent->owner->cur_move) {
        move_info *m = owner->find_move_alt(2, opponent->owner->cur_move->variant);
        defense_move = m;
        if (!m) {
            m = owner->find_move(2, 0);
            defense_move = m;
        }
        if (m) {
            state = COMBAT_DEFENCE;
            return;
        }
    }
    state = COMBAT_IDLE;
}

// 0x463830  Confidence: exact.
void living_combat::pick_dodge()
{
    defense_move = owner->find_move(3, 0);
    if (!defense_move)
        state = COMBAT_IDLE;
    else
        state = COMBAT_DEFENCE;
}

// 0x463860  Confidence: exact. The message only goes to string_buffer (never printed).
void living_combat::queue_counter_attack()
{
    if (owner->get_skill(SKILL_COUNTER_ATTACK) > 0.0f) {
        if (frand() <= owner->get_skill(SKILL_COUNTER_ATTACK)) {
            chain_skill = SKILL_COUNTER_ATTACK;
            sprintf(string_buffer, "Queueing counter attack for %s\n", owner->info->name);
        }
    }
}

// 0x4638c0  Confidence: exact.
void living_combat::play_defense()
{
    if (owner->ai.is_mode(AI_STATE_DISABLED, 0x2e) != true) {
        anim = owner->play_move((int)defense_move, 1.0f, 0, 0, 0);
        if (anim != -1) {
            state = COMBAT_REACTING;
            return;
        }
    }
    state = COMBAT_IDLE;
}

// 0x463910  Confidence: unsure (the vector temporaries: matrix assignment is expanded inline into three
// vector3 copies, each followed by a copy-constructor call into one temporary, 0x463a60-0x463aa6).
// The weapon's "$missile" tag through the hand bone (char_info +0x168) gives the arrow's start; the
// phoenix (species 0x58) shoots fire arrows, everyone else the generic arrow.
bool living_combat::fire_weapon_missile()
{
    vector3 ofs;
    vector3 tag_pos, bone_pos, p;
    matrix tag_orient, bone_orient, rot, orient;
    int bone, tag;

    ofs.zero();
    if (!owner->equipment[5])
        return false;
    bone = owner->info->_unk168[0];
    tag = vim_find_tag(owner->equipment[5]->type->mesh, "$missile");
    if (tag < 0)
        return false;
    vim_get_tag_transform(owner->equipment[5]->type->mesh, tag, &tag_pos, &tag_orient);
    owner->instance->get_bone_transform(&bone_pos, &bone_orient, bone);
    matrix43 bone_m(bone_pos, bone_orient);
    matrix43 tag_m(tag_pos, tag_orient);
    matrix43 world = tag_m * bone_m;
    world.get_rotation(&rot);
    p.x = world.m[3][0];
    p.y = world.m[3][1];
    p.z = world.m[3][2];
    orient = owner->orient * rot;
    p.unrotate(owner->orient);
    p.x = p.x + owner->pos.x;
    p.y = p.y + owner->pos.y;
    p.z = p.z + owner->pos.z;
    p.x = ofs.x + p.x;
    p.y = ofs.y + p.y;
    p.z = ofs.z + p.z;
    spell_fire_missile(owner->info->species == 0x58 ? SPELL_FIRE_ARROW : SPELL_GENERIC_ARROW, -1, owner,
                       opponent->owner, &p, &orient);
    return true;
}

// 0x463b40  Confidence: exact. Weapon (or unarmed) damage + 3 x level.
float living_combat::attack_rating()
{
    if (owner->equipment[5])
        return (float)(owner->level * 3 + owner->equipment[5]->type->damage);
    return (float)(owner->info->damage + owner->level * 3);
}

// 0x463b90  Confidence: exact. The weapon's skill clamped to [0, 1] (skill 0 = hand to hand).
float living_combat::weapon_skill()
{
    float s = owner->get_skill(owner->equipment[5] ? owner->equipment[5]->type->weapon_skill : 0);

    if (s <= 0.0f)
        return 0.0f;
    if (s >= 1.0f)
        return 1.0f;
    return s;
}

// 0x463be0  Confidence: exact.
bool living_combat::has_weapon_skill()
{
    return weapon_skill() > 0.0f;
}

// 0x463c00  Confidence: exact. This side's share of the attack speed (speed skill, scaled by double
// attack); with the combat option the player's character is 25 % faster and never gets two turns in a
// row against the player.
float living_combat::speed_share(living_combat *other)
{
    float mine = owner->get_skill(SKILL_SPEED);
    float theirs = other->owner->get_skill(SKILL_SPEED);

    if (mine == 0.0f && theirs == 0.0f)
        return 0.5f;
    if (other->owner->skill_allowed(SKILL_DOUBLE_ATTACK))
        theirs = (other->owner->get_skill(SKILL_DOUBLE_ATTACK) + 1.0f) * theirs;
    if (owner->skill_allowed(SKILL_DOUBLE_ATTACK))
        mine = (owner->get_skill(SKILL_DOUBLE_ATTACK) + 1.0f) * mine;
    if (owner == cur_character && config_combat_flag)
        mine *= 1.25f;
    else if (other->owner == cur_character && config_combat_flag)
        theirs *= 1.25f;
    if (attack_turn && other->owner == cur_character && config_combat_flag)
        return 0.0f;
    return mine / (theirs + mine);
}

// 0x463d00  Confidence: likely (an 8168 byte match; differs under 8966, D7807). Not for the player's character; with a 20 % chance a party member uses
// double attack and a monster a random combat special it knows (kick only on its own target, trip not on
// knocked-down or untrippable targets, push not on char_info flag 0x10 targets).
void living_combat::ai_choose_special()
{
    int list[0x24];
    int n, i;

    if (owner == cur_character)
        return;
    if (frand() > 0.2f)
        return;
    n = 0;
    if (owner->type == 7) {
        if (owner->get_skill(SKILL_DOUBLE_ATTACK) <= 0.0f)
            return;
        list[0] = SKILL_DOUBLE_ATTACK;
        n = 1;
    } else {
        for (i = 0; i < 0x24; i++) {
            if (!skill_is_combat_special(i))
                continue;
            if (owner->get_skill(i) <= 0.0f)
                continue;
            if (i == SKILL_KICK) {
                if (opponent->target != owner->handle)
                    continue;
            } else if (i == SKILL_TRIP) {
                if (opponent->owner->ai.is_mode(AI_STATE_DISABLED, 0x2d))
                    continue;
                if (opponent->owner->info->flags & 0x8)
                    continue;
            } else if (i == SKILL_PUSH) {
                if (opponent->owner->info->flags & 0x10)
                    continue;
            }
            list[n++] = i;
        }
        if (!n)
            return;
    }
    chain_skill = list[rand() % n];
}

// 0x463e20  Confidence: likely (the argument is not read). A random attack move (category 0xa, not the
// last one) when the opponent is within about 18 degrees of the facing; in multiplayer by weapon class.
move_info *living_combat::pick_attack_move(int allow_any)
{
    int prev = move_variant;
    vector3 d;
    move_info *moves[10];
    int cls, n;
    move_info *m;

    d.x = opponent->owner->pos.x - owner->pos.x;
    d.y = 0.0f;
    d.z = opponent->owner->pos.z - owner->pos.z;
    d.normalize();
    if (d.z * owner->orient.fvec.z + d.y * owner->orient.fvec.y + d.x * owner->orient.fvec.x < 0.95f)
        return NULL;
    cls = 0;
    if ((game_flags & 2) && owner->equipment[5])
        cls = owner->equipment[5]->type->weapon_class;
    n = charinfo_collect_moves(moves, 0xa, owner->info, 0, move_variant, cls);
    if (!n)
        return NULL;
    m = moves[(int)floor(frand() * n)];
    if (prev == 0 && n == 1)
        move_variant = 0;
    else
        move_variant = m->id;
    return m;
}

// 0x463f30  Confidence: exact.
void living_combat::begin_attack()
{
    move_info *m;

    skill = SKILL_NONE;
    ai_choose_special();
    if (chain_skill != SKILL_NONE) {
        play_skill_attack();
        chain_skill = SKILL_NONE;
        return;
    }
    m = pick_attack_move(1);
    if (m)
        play_attack(m);
}

// 0x463f70  Confidence: exact (a separate function that is a tail jump to begin_attack; the original
// is probably a second entry point with the same body).
void living_combat::begin_attack_thunk()
{
    begin_attack();
}

// 0x463f80  Confidence: exact.
void living_combat::ranged_attack()
{
    move_info *m;

    if (owner->info->species != 0x58)
        m = charinfo_find_move(owner->info, 0xb, 0x20, 1);
    else
        m = phoenix_select_ranged_move(owner);
    if (m)
        play_special(m);
}

// 0x463fc0  Confidence: exact. A free function: (record, opponent) with the opponent's defence rolled in
// simulation against the record.
bool living_combat_test_defense(living_combat *c, living_combat *opp)
{
    living_combat *saved = c->opponent;
    int hits;

    c->opponent = opp;
    opp->roll_defense(c, true);
    hits = c->hits;
    c->opponent = saved;
    return hits > 0;
}

// 0x463ff0  Confidence: exact.
bool living_combat_resolve_hit_vs(living_combat *c, living_combat *opp)
{
    living_combat *saved = c->opponent;
    bool r;

    c->opponent = opp;
    r = c->resolve_hit();
    c->opponent = saved;
    return r;
}

// 0x464010 init_ccmd_Super_power_thunk (jmp). Confidence: exact.
// 0x464020 init_ccmd_Super_power (console_command constructor). Confidence: exact.
console_command ccmd_Super_power("Super_power", "Toggles Super_power", cmd_Super_power);   // 0x941720

// 0x464040  Confidence: exact (the console's bool command macro).
void cmd_Super_power()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Super_power = true;
        else if (arg_type & ARG_FALSE)
            Super_power = false;
        else if (arg_type & ARG_NONE)
            Super_power ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "Super_power", "Super_power");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "Super_power", Super_power ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x464100  Confidence: likely (byte-indexed jump table 0x4645a0 for 0xa..0x55; the shared status block
// is written twice as in the binary). The defender (this) rolls parry (skill 9) and dodge (skill 0xb)
// against the attacker's hit value; simulate = 1 only counts the hits and never parries.
void living_combat::roll_defense(living_combat *att, bool simulate)
{
    float parry, dodge, hit, d, total, parry_chance, dodge_chance, r;

    att->hits = 0;
    if (!simulate)
        move_variant = 0;
    parry = owner->get_skill(SKILL_PARRY);
    if (parry <= 0.0f)
        parry = 0.0f;
    dodge = owner->get_skill(SKILL_DODGE);
    if (dodge <= 0.0f)
        dodge = 0.0f;
    hit = 0.0f;
    switch (att->skill) {
    case SKILL_AXESPIN: case SKILL_EARTHSHAKER: case SKILL_MEND: case SKILL_STEELSKIN:
    case SKILL_OVERPOWER: case SKILL_BLESS: case SKILL_REVITALIZE: case SKILL_PRAYER:
        hit = 1.0f;
        // fall through
    case SKILL_COUNTER_ATTACK:
        parry = 0.0f;
        dodge = 0.0f;
        break;
    case SKILL_DOUBLE_ATTACK: case SKILL_BLINDNESS_STAB: case SKILL_ARM_SLICE: case SKILL_LEG_SWIPE:
    case SKILL_DESPERATION: case SKILL_STAMINA_BURST: case SKILL_ADDED_BLOW: case SKILL_LIFE_LEECH:
    case SKILL_MIND_DRAIN: case SKILL_SACRIFICE: case SKILL_CONFUSION: case SKILL_SILENCE: case SKILL_MUG:
    case SKILL_NONE:
        hit = att->weapon_skill();
        if (att->owner->has_status(STATUS_BLIND))
            hit *= 0.3f;
        if (att->owner->has_status(7))
            hit += 0.2f;
        if (att->opponent->owner->has_status(7))
            hit += 0.1f;
        if (att->opponent->owner->has_status(8))
            hit *= 0.5f;
        if (att->owner->has_status(STATUS_CRIPPLE))
            hit -= 0.2f;
        if (hit < 0.0f)
            hit = 0.0f;
        break;
    case SKILL_ICY_TOUCH:
        hit = att->owner->get_skill(SKILL_ICE);
        break;
    case SKILL_BURNING_HANDS:
        hit = att->owner->get_skill(SKILL_FIRE);
        break;
    case SKILL_SHOCKING_GRASP:
        hit = att->owner->get_skill(SKILL_ENERGY);
        break;
    case SKILL_SERPENT_WHIP: case SKILL_SERPENT_WHIP_TRIP:
        hit = att->owner->get_skill(att->skill);
        if (att->owner->has_status(STATUS_BLIND))
            hit *= 0.3f;
        if (att->owner->has_status(7))
            hit += 0.2f;
        if (att->opponent->owner->has_status(7))
            hit += 0.1f;
        if (att->opponent->owner->has_status(8))
            hit *= 0.5f;
        if (att->owner->has_status(STATUS_CRIPPLE))
            hit -= 0.2f;
        if (hit >= 0.95f)
            hit = 0.95f;
        break;
    case SKILL_PUSH: case SKILL_KICK: case SKILL_TRIP: case SKILL_SERPENT_WHIP_PUSH:
    case SKILL_SERPENT_WHIP_MULTI:
        hit = att->owner->get_skill(att->skill);
        parry = 0.0f;
        break;
    case SKILL_BACKSTAB:
        if (!att->opponent->owner->ai.is_mode(AI_STATE_COMBAT, -1))
            dodge = 0.0f;
        parry = 0.0f;
        hit = att->owner->get_skill(att->skill) * (1500 / owner->level);
        if (hit >= 0.95f)
            hit = 0.95f;
        break;
    case SKILL_AIMED_ATTACK_EYES: case SKILL_AIMED_ATTACK_ARMS: case SKILL_AIMED_ATTACK_LEGS:
        parry = 0.0f;
        dodge = 0.0f;
        hit = att->owner->get_skill(att->skill) * (1500 / owner->level);
        if (hit >= 0.95f)
            hit = 0.95f;
        break;
    }
    if (simulate == true)
        parry = 0.0f;
    if (owner->has_status(STATUS_SLOW) || owner->ai.is_disabled_any()) {
        d = dodge - 0.2f;
        if (d < 0.0f)
            d = 0.0f;
    } else
        d = dodge;
    d *= 0.5f;
    if (owner->loco_blend > 0.5f)
        d = 0.0f;
    total = hit + d + parry;
    if (total > 0.0f) {
        parry_chance = parry / total;
        dodge_chance = d / total;
    } else {
        parry_chance = 0.0f;
        dodge_chance = 0.0f;
    }
    r = frand();
    if (Super_power && att->owner->type == 7)
        r = 1.0f;
    if (!simulate) {
        if (r < parry_chance)
            pick_parry();
        else if (r < dodge_chance + parry_chance)
            pick_dodge();
        else {
            state = COMBAT_HIT;
            att->hits++;
        }
    } else if (att->skill == SKILL_BACKSTAB && r < hit)
        att->hits++;
    else if (r >= dodge_chance)
        att->hits++;
    if (att->skill == SKILL_DOUBLE_ATTACK && frand() >= dodge_chance + parry_chance) {
        if (!simulate) {
            state = COMBAT_HIT;
            defense_move = NULL;
        }
        att->hits++;
    }
}

// 0x4645f0  Confidence: exact (byte-indexed jump table 0x4646b4 for 0xa..0x4f).
void living_combat::play_skill_attack()
{
    move_info *m;

    switch (chain_skill) {
    case SKILL_KICK:
        m = owner->find_move(1, 0x66);
        break;
    case SKILL_TRIP:
        m = owner->find_move(1, 0x67);
        break;
    case SKILL_PUSH:
        m = owner->find_move(1, 0x68);
        break;
    case SKILL_AXESPIN:
        m = owner->find_move(0, 0x77);
        break;
    case SKILL_EARTHSHAKER:
        m = owner->find_move(0, 0x78);
        break;
    case SKILL_MEND: case SKILL_STEELSKIN: case SKILL_OVERPOWER: case SKILL_BLESS: case SKILL_REVITALIZE:
    case SKILL_PRAYER:
        m = owner->find_move(0, 0x79);
        break;
    case SKILL_COUNTER_ATTACK: case SKILL_DOUBLE_ATTACK:
        m = owner->find_move(1, 0x69);
        break;
    case SKILL_AIMED_ATTACK_EYES: case SKILL_AIMED_ATTACK_ARMS: case SKILL_AIMED_ATTACK_LEGS:
    case SKILL_BLINDNESS_STAB: case SKILL_ARM_SLICE: case SKILL_LEG_SWIPE: case SKILL_DESPERATION:
    case SKILL_STAMINA_BURST: case SKILL_ADDED_BLOW: case SKILL_LIFE_LEECH: case SKILL_MIND_DRAIN:
    case SKILL_MEDEVAN_TECHNIQUE: case SKILL_ICY_TOUCH: case SKILL_BURNING_HANDS: case SKILL_SHOCKING_GRASP:
    case SKILL_SACRIFICE: case SKILL_CONFUSION: case SKILL_SILENCE: case SKILL_MUG:
        m = pick_attack_move(0);
        break;
    default:
        m = NULL;
        break;
    }
    if (!m) {
        state = COMBAT_IDLE;
        return;
    }
    skill = chain_skill;
    play_attack(m);
}

// Picks the player's chain attack from the four configured slots (25 % each; slot 3 before slot 2) and
// announces it. Written out twice in the binary (0x46491d, 0x464af7): a macro here.
#define COMBAT_PICK_CHAIN(rec) do { \
        float c_ = frand(); \
        int s_ = character_get_chain_skill(party_leader, c_ < 0.25f ? 0 : c_ < 0.5f ? 1 : c_ < 0.75f ? 3 : 2); \
        char buf_[0x40]; \
        if (s_ == SKILL_NONE) \
            s_ = SKILL_ADDED_BLOW; \
        (rec)->chain_skill = s_; \
        character_learn_chain_skill(party_leader, s_, 0); \
        strcpy(buf_, XSTR("Chain ", 215)); \
        strcat(buf_, localize_string_or_invalid(skill_table[s_].name)); \
        strcat(buf_, "!"); \
        hud_show_message(buf_); \
    } while (0)

// 0x464700  Confidence: likely (the multiplayer filter and the two chain blocks). One round of a melee
// exchange: who attacks is decided by the speed share; the other side rolls its defence. one_sided =
// the opponent does not fight back (living_combat_update_attack, _ranged).
void living_combat::resolve_exchange(bool one_sided)
{
    living_combat *att, *def;
    float r, share;

    if (game_flags & 2) {
        if (mp_local_player && !(mp_local_player->flags & 1)) {
            if (one_sided) {
                if (!character_is_locally_controlled(owner))
                    return;
            } else if (!character_is_locally_controlled(owner) &&
                       !character_is_locally_controlled(opponent->owner))
                return;
        } else if (mp_local_player && (mp_local_player->flags & 1)) {
            if (!(one_sided == true && character_is_locally_controlled(owner)) &&
                (!character_is_locally_controlled(owner) || !character_is_locally_controlled(opponent->owner)))
                return;
        } else
            return;
        printf("Local combat sith %s and %s\n", owner->get_name(), opponent->owner->get_name());
    }
    defense_move = NULL;
    if (!one_sided) {
        opponent->defense_move = NULL;
        r = frand();
        share = speed_share(opponent);
        if (!config_combat_flag) {
            if (chain_learn) {
                chain_learn = false;
                character_learn_chain_skill(party_leader, chain_skill, chain_count);
                share = 1.0f;
            } else if (opponent->chain_learn) {
                opponent->chain_learn = false;
                character_learn_chain_skill(party_leader, opponent->chain_skill, opponent->chain_count);
                share = 0.0f;
            } else {
                last_chain_skill = SKILL_NONE;
                if (owner == cur_character || opponent->owner == cur_character)
                    share = attack_turn ? 0.0f : 1.0f;
            }
        }
        if (chain_skill == SKILL_COUNTER_ATTACK)
            share = 1.0f;
        else if (opponent->chain_skill == SKILL_COUNTER_ATTACK)
            share = 0.0f;
        if (r <= share) {
            att = this;
            def = opponent;
        } else {
            att = opponent;
            def = this;
        }
        if (config_combat_flag && att->owner == cur_character && att->attack_turn)
            COMBAT_PICK_CHAIN(att);
        if (chain_skill != SKILL_COUNTER_ATTACK) {
            att->attack_turn = true;
            def->attack_turn = false;
        } else if (att->owner == cur_character)
            hud_show_message(XSTR("Counterattack!", 216));
        def->last_chain_skill = SKILL_NONE;
        def->chain_count = 0;
        def->chain_index = 0;
        att->begin_attack();
        if (att->state != COMBAT_IDLE)
            def->roll_defense(att, false);
        else {
            def->begin_attack();
            if (def->state != COMBAT_IDLE)
                att->roll_defense(def, false);
        }
    } else {
        if (!config_combat_flag) {
            if (chain_learn)
                chain_learn = false;
            else {
                last_chain_skill = SKILL_NONE;
                chain_count = 0;
                chain_index = 0;
            }
        } else if (owner == cur_character && attack_turn)
            COMBAT_PICK_CHAIN(this);
        if (owner->ai.is_attacking())
            begin_attack_thunk();
        else
            ranged_attack();
        if (state != COMBAT_IDLE)
            opponent->roll_defense(this, true);
        if (opponent->owner->type == 7)
            combat_mode_set(true);
    }
    if (game_flags & 2) {
        if (state == COMBAT_ATTACKING || state == COMBAT_SPECIAL)
            send_net(one_sided, state == COMBAT_SPECIAL);
        else if (opponent && (opponent->state == COMBAT_ATTACKING || opponent->state == COMBAT_SPECIAL))
            send_net(one_sided, opponent->state == COMBAT_SPECIAL);
    }
}

// 0x464cb0  Confidence: exact. The defence move's hit trigger (7) is not later than the attacker's.
bool living_combat::defense_in_time()
{
    float mine = move_get_trigger_time(defense_move, 7);
    float theirs = opponent->owner->move_trigger_time_left(7);

    return theirs < 0.0f || !(mine < theirs);
}

// 0x464d00  Confidence: likely (an 8168 byte match; differs under 8966, D7807). +0.25 from 0.25 m above, -0.25 from below, +0.5 against a knocked-down
// defender, +0.15 from behind.
float combat_position_modifier(Character *attacker, Character *defender)
{
    float m = 0.0f;
    vector3 d;
    float facing;

    if (defender->pos.y + 0.25f < attacker->pos.y)
        m = 0.25f;
    if (defender->pos.y - 0.25f > attacker->pos.y)
        m = m - 0.25f;
    d.x = defender->pos.x - attacker->pos.x;
    d.y = defender->pos.y - attacker->pos.y;
    d.z = defender->pos.z - attacker->pos.z;
    d.normalize();
    facing = d.z * defender->orient.fvec.z + d.y * defender->orient.fvec.y + d.x * defender->orient.fvec.x;
    if (defender->ai.is_mode(AI_STATE_DISABLED, 0x2d))
        return m + 0.5f;
    if (facing >= 0.0f)
        return m + 0.15f;
    return m;
}

// 0x464de0  Confidence: likely (the armour terms are summed in the order level, 6, 8, 7, 4, 3). Without
// armour in slots 3, 4, 6, 7, 8: protection skill + level %; with armour: level % + sum of material
// protection multiplier x item protection. An active bless adds its value.
float living_combat::get_protection()
{
    Character *c = owner;
    item_info *e3 = c->equipment[3];
    item_info *e4 = c->equipment[4];
    item_info *e7 = c->equipment[7];
    item_info *e8 = c->equipment[8];
    item_info *e6 = c->equipment[6];
    float prot;

    if (!e3 && !e4 && !e7 && !e8 && !e6)
        prot = c->get_skill(SKILL_PROTECTION) + c->level * 0.01f;
    else {
        float p3 = e3 ? materials[e3->type->material].protection_mult * e3->type->protection : 0.0f;
        float p4 = e4 ? materials[e4->type->material].protection_mult * e4->type->protection : 0.0f;
        float p7 = e7 ? materials[e7->type->material].protection_mult * e7->type->protection : 0.0f;
        float p8 = e8 ? materials[e8->type->material].protection_mult * e8->type->protection : 0.0f;
        float p6 = c->equipment[6] ? materials[c->equipment[6]->type->material].protection_mult *
                                     c->equipment[6]->type->protection : 0.0f;
        prot = c->level * 0.01f + p6 + p8 + p7 + p4 + p3;
    }
    if (c->has_status(STATUS_BLESS))
        return owner->status_value(STATUS_BLESS) + prot;
    return prot;
}

// The self-cast of a combat skill (mend, steelskin, overpower, bless, revitalize, prayer): through the
// spell request on the multiplayer host, directly otherwise. Written out per skill in the binary (each
// with its own stack temporaries); a macro here. `direct` is the single-player call.
#define COMBAT_SELF_CAST(sp, direct) do { \
        if (MP_HOST) { \
            spell_cast_request req_; \
            req_.caster = owner; \
            req_.spell = (sp); \
            req_.target = owner->handle; \
            req_.target_pos = &owner->pos; \
            req_.layer = owner->layer; \
            req_._unk14 = 0; \
            req_.source_type = 6; \
            req_.params = spell_prepare_cast(owner, (sp), owner->handle, 0, 6, 0); \
            mp_spell_cast_sync(&req_); \
            spell_cast_launch_effects(&req_); \
        } else if (game_flags & 1) \
            direct; \
    } while (0)

// 0x464f50  Confidence: likely (two jump tables: bytes 0x465d40 for 0xa..0x55 and dwords 0x465d8c for
// 0x38..0x4f; the order of the stack temporaries). docs/sum-exe/combat.md "Formula". The argument is not
// read (all callers pass 0).
float living_combat::melee_damage(int unused)
{
    item_info **equip = owner->equipment;
    int damage_type = -1;
    int elemental = -1;
    int n = party_count_active();
    float party = 0.0f;
    float prot, dmg, base, mat, res1, res2, f, result;
    Character *c;
    char buf[0x80];

    if (opponent->owner->has_status(8) && frand() < 0.25f)
        return 0.0f;
    prot = opponent->get_protection() * 0.5f;
    if (prot >= 0.75f)
        prot = 0.75f;
    dmg = 0.0f;
    switch (skill) {
    case SKILL_COUNTER_ATTACK: case SKILL_DOUBLE_ATTACK: case SKILL_BACKSTAB:
    case SKILL_AIMED_ATTACK_EYES: case SKILL_AIMED_ATTACK_ARMS: case SKILL_AIMED_ATTACK_LEGS:
    case SKILL_BLINDNESS_STAB: case SKILL_ARM_SLICE: case SKILL_LEG_SWIPE: case SKILL_DESPERATION:
    case SKILL_STAMINA_BURST: case SKILL_ADDED_BLOW: case SKILL_LIFE_LEECH: case SKILL_MIND_DRAIN:
    case SKILL_ICY_TOUCH: case SKILL_BURNING_HANDS: case SKILL_SHOCKING_GRASP: case SKILL_SACRIFICE:
    case SKILL_CONFUSION: case SKILL_SILENCE: case SKILL_AXESPIN: case SKILL_MUG: case SKILL_NONE:
        if (equip[5] && !(owner->info->flags & 0x200)) {
            base = equip[5]->type->damage * 0.1f;
            mat = materials[equip[5]->type->material].damage_mult;
            damage_type = equip[5]->type->damage_type;
            elemental = equip[5]->type->elemental_damage;
        } else {
            damage_type = 6;
            base = owner->info->damage * 0.1f;
            if (owner->info->species == 0xb0)
                damage_type = 4;
            mat = 0.0f;
        }
        if (owner->has_status(STATUS_POWER) == true)
            base += base;
        dmg = (owner->level * 0.5f + 1.0f) * (mat + base);
        if (n < 3)
            party = 0.0f;
        else if (n == 3)
            party = 0.2f;
        else if (n == 4)
            party = 0.4f;
        else if (n == 5)
            party = 0.6f;
        if (owner->type == 7)
            party = -party;
        dmg = (frand() - 0.5f + party) * dmg * 0.6f + dmg;
        if (owner->info->species == 0x91)
            dmg += dmg;
        if (skill == SKILL_BACKSTAB) {
            if (owner->type == 9 || (owner->flags & 0x40))
                dmg += dmg;
            else {
                dmg = (owner->get_skill(SKILL_BACKSTAB) * 10.0f + 1.2f) * dmg;
                if (backstab_full)
                    dmg *= 2.5f;
                else
                    dmg *= 0.5f;        // the aimed attacks' multiply (shared code)
            }
        } else if (skill >= SKILL_AIMED_ATTACK_EYES && skill <= SKILL_AIMED_ATTACK_LEGS)
            dmg *= 0.5f;
        else if (skill == SKILL_ADDED_BLOW)
            dmg *= 0.7f;
        break;
    case SKILL_KICK:
        damage_type = 6;
        dmg = owner->level * 4.0f;
        break;
    case SKILL_TRIP: case SKILL_EARTHSHAKER: case SKILL_MEND: case SKILL_STEELSKIN: case SKILL_OVERPOWER:
    case SKILL_BLESS: case SKILL_REVITALIZE: case SKILL_PRAYER: case SKILL_SERPENT_WHIP_TRIP:
    case SKILL_SERPENT_WHIP_PUSH:
        dmg = 0.0f;
        break;
    case SKILL_PUSH:
        dmg = 0.0f;
        chain_index += 2;
        break;
    case SKILL_SERPENT_WHIP_MULTI:
        dmg = owner->level * 8.0f;
        break;
    case SKILL_SERPENT_WHIP:
        dmg = owner->level * 10.0f;
        break;
    }
    dmg = (chain_count * 0.1f + 1.0f) * ((1.0f - prot) * dmg);

    switch (skill) {
    case SKILL_ICY_TOUCH:
        elemental = 1;
        break;
    case SKILL_BURNING_HANDS:
        elemental = 0;
        break;
    case SKILL_SHOCKING_GRASP:
        elemental = 2;
        break;
    case SKILL_BLINDNESS_STAB: case SKILL_ARM_SLICE: case SKILL_LEG_SWIPE:
        dmg *= 0.3f;
        break;
    case SKILL_LIFE_LEECH:
        dmg *= 0.5f;
        if (!MP_CLIENT)
            owner->add_hp(dmg, -1, 1, 0);
        break;
    case SKILL_MIND_DRAIN:
        if (!MP_CLIENT) {
            opponent->owner->add_ap(dmg * -0.1f);
            owner->add_ap(dmg * 0.1f);
        }
        dmg = 0.0f;
        break;
    case SKILL_SACRIFICE:
        dmg += dmg;
        if (!MP_CLIENT)
            owner->add_hp(dmg * (-2.0f / 3.0f), -1, 1, 0);
        break;
    case SKILL_SILENCE:
        dmg *= 0.15f;
        break;
    case SKILL_CONFUSION:
        if (!MP_CLIENT)
            opponent->owner->add_ap(dmg * -0.25f);
        dmg = 0.0f;
        break;
    case SKILL_STAMINA_BURST:
        dmg = (float)owner->ap / owner->get_max_stat(1) * dmg;
        break;
    case SKILL_DESPERATION:
        dmg = (1.3f - owner->hp / owner->get_max_stat(0)) * dmg;
        break;
    case SKILL_MUG: {
        int gold;

        dmg *= 0.5f;
        gold = (int)dmg;
        party_gold += gold;
        sprintf(buf, XSTR("Stole %d gold!", 217), gold);
        if ((game_flags & 1) || owner == cur_character)
            hud_show_message(buf);
        break;
    }
    case SKILL_AXESPIN: case SKILL_EARTHSHAKER:
        if (!MP_CLIENT) {
            for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                if (c && c != owner && c->type != 7) {
                    float dx = c->pos.x - owner->pos.x;
                    float dy = c->pos.y - owner->pos.y;
                    float dz = c->pos.z - owner->pos.z;

                    if (dz * dz + dx * dx + dy * dy < 25.0f) {
                        if (skill == SKILL_AXESPIN)
                            c->add_hp(dmg * -0.75f, owner->handle, 1, 0);
                        else if (game_flags & 2) {
                            spell_cast_request req;
                            req.caster = owner;
                            req.spell = SPELL_TRIP;
                            req.target = c->handle;
                            req.target_pos = &c->pos;
                            req.layer = c->layer;
                            req._unk14 = 0;
                            req.source_type = 6;
                            req.params = spell_prepare_cast(owner, SPELL_TRIP, c->handle, 0, 6, 0);
                            mp_spell_cast_sync(&req);
                            spell_cast_launch_effects(&req);
                        } else
                            c->get_hit_by_spell(SPELL_TRIP, owner->handle, 0, 0, 6, 0, 0, 0);
                    }
                }
            }
        }
        dmg = 0.0f;
        break;
    case SKILL_MEND:
        COMBAT_SELF_CAST(SPELL_HEAL, owner->get_hit_by_spell(SPELL_HEAL, owner->handle, 0, 0, 6, 0, 0, 0));
        spell_play_hit_sound(SPELL_HEAL, &owner->pos);
        break;
    case SKILL_STEELSKIN:
        COMBAT_SELF_CAST(SPELL_BLESS, owner->get_hit_by_spell(SPELL_BLESS, owner->handle, 0, 0, 6, 0, 0, 0));
        spell_play_hit_sound(SPELL_BLESS, &owner->pos);
        break;
    case SKILL_OVERPOWER:
        COMBAT_SELF_CAST(SPELL_EMPOWER, owner->get_hit_by_spell(SPELL_EMPOWER, owner->handle, 0, 0, 6, 0, 0, 0));
        spell_play_hit_sound(SPELL_EMPOWER, &owner->pos);
        break;
    case SKILL_BLESS:
        COMBAT_SELF_CAST(SPELL_PROTECT, spell_cast_execute(owner, SPELL_PROTECT, owner->handle, &owner->pos,
                                                           owner->layer, 0, 6, 0));
        break;
    case SKILL_REVITALIZE:
        COMBAT_SELF_CAST(SPELL_VITALIZE, spell_cast_execute(owner, SPELL_VITALIZE, owner->handle, &owner->pos,
                                                            owner->layer, 0, 6, 0));
        break;
    case SKILL_PRAYER:
        COMBAT_SELF_CAST(SPELL_FAITH, spell_cast_execute(owner, SPELL_FAITH, owner->handle, &owner->pos,
                                                         owner->layer, 0, 6, 0));
        break;
    }

    res1 = 1.0f - opponent->owner->get_resistance(damage_type);
    res2 = 1.0f - opponent->owner->get_resistance(elemental);
    if (res1 < 0.0f)
        f = res1;
    else if (res2 < 0.0f)
        f = res2;
    else
        f = res2 * res1;
    dmg = f * dmg;
    pos_mod = combat_position_modifier(owner, opponent->owner);
    dmg = pos_mod * dmg + dmg;
    if (opponent->owner->has_status(STATUS_CURSE))
        dmg = (opponent->owner->status_value(STATUS_CURSE) + 1.0f) * dmg;
    if (owner->skill_allowed(SKILL_CRITICAL_HIT) && frand() < owner->get_skill(SKILL_CRITICAL_HIT))
        result = dmg + dmg;
    else
        result = dmg;
    if (Super_power && owner->type == 7)
        result = 200.0f;
    return (float)(int)floor(result);
}

// 0x465df0  Confidence: likely (byte-indexed jump table 0x46659c for 0xa..0x55; the per-case tails are
// written out as in the binary). On a multiplayer client only the effects run (and only when the hit
// would not kill). The aimed attacks' status chance uses cur_character's aimed attack skill, not the
// attacker's (literal; NOTES.md).
void living_combat::apply_melee_hit(bool flinch)
{
    float dmg;
    int tag;

    if (!hits)
        return;
    if (MP_CLIENT) {
        dmg = melee_damage(0);
        if (opponent->owner->hp - dmg <= 0.0f)
            return;
    } else {
        if (serpent_part_on_hit(opponent->owner))
            return;
        dmg = melee_damage(0);
        if (pos_mod != owner->pos_mod_text) {
            owner->pos_mod_text = pos_mod;
            owner->pos_mod_text_timer = 2.0f;
        }
        trigger_reflex_ability(dmg);
        if (!opponent->owner->add_hp(-dmg, owner->handle, 1, 0) ||
            opponent->owner->ai.is_mode(AI_STATE_DISABLED, -1) == true) {
            spawn_blood(dmg);
            return;
        }
        trigger_special_ability(dmg);
        if (hits > 1 && skill == SKILL_DOUBLE_ATTACK) {
            dmg = melee_damage(0);
            dmg = owner->get_skill(SKILL_DOUBLE_ATTACK) * dmg;
            trigger_reflex_ability(dmg);
            if (!opponent->owner->add_hp(-dmg, owner->handle, 1, 0) ||
                opponent->owner->ai.is_mode(AI_STATE_DISABLED, -1) == true) {
                spawn_blood(dmg);
                return;
            }
            trigger_special_ability(dmg);
        }
    }
    spawn_blood(dmg);
    switch (skill) {
    case SKILL_COUNTER_ATTACK: case SKILL_DOUBLE_ATTACK: case SKILL_ADDED_BLOW: case SKILL_AXESPIN:
    case SKILL_NONE:
        combat_play_weapon_hit_sound(owner, opponent->owner);
        combat_spawn_hit_vfx(owner, opponent->owner);
        if (flinch == true)
            opponent->play_flinch(owner->cur_move->variant);
        break;
    case SKILL_BACKSTAB:
        game_play_sound(7, 1.0f, 0.0f, 0, 0);
        combat_spawn_hit_vfx(owner, opponent->owner);
        combat_spawn_hit_vfx(owner, opponent->owner);
        combat_spawn_hit_vfx(owner, opponent->owner);
        if (flinch == true)
            opponent->play_flinch(owner->cur_move->variant);
        break;
    case SKILL_DESPERATION: case SKILL_LIFE_LEECH: case SKILL_SACRIFICE:
        combat_play_weapon_hit_sound(owner, opponent->owner);
        combat_spawn_hit_vfx(owner, opponent->owner);
        if (flinch == true)
            opponent->play_flinch(owner->cur_move->variant);
        fx_attach_to_character("generichit01.vfx", opponent->owner->handle, -1, -1, 0, 0);
        break;
    case SKILL_STAMINA_BURST: case SKILL_MIND_DRAIN: case SKILL_CONFUSION:
        combat_play_weapon_hit_sound(owner, opponent->owner);
        combat_spawn_hit_vfx(owner, opponent->owner);
        if (flinch == true)
            opponent->play_flinch(owner->cur_move->variant);
        fx_attach_to_character("generichit03.vfx", opponent->owner->handle, -1, -1, 0, 0);
        break;
    case SKILL_ICY_TOUCH:
        combat_play_weapon_hit_sound(owner, opponent->owner);
        combat_spawn_hit_vfx(owner, opponent->owner);
        tag = combat_get_weapon_tag_position(owner, opponent->owner);
        if (tag == -1)
            return;
        fx_attach_to_character("spell-icicle-target.vfx", opponent->owner->handle, tag, -1, 0, 0);
        if (flinch == true)
            opponent->play_flinch(owner->cur_move->variant);
        break;
    case SKILL_BURNING_HANDS:
        combat_play_weapon_hit_sound(owner, opponent->owner);
        combat_spawn_hit_vfx(owner, opponent->owner);
        tag = combat_get_weapon_tag_position(owner, opponent->owner);
        if (tag == -1)
            return;
        fx_attach_to_character("firearrow-target.vfx", opponent->owner->handle, tag, -1, 0, 0);
        if (flinch == true)
            opponent->play_flinch(owner->cur_move->variant);
        break;
    case SKILL_SHOCKING_GRASP:
        combat_play_weapon_hit_sound(owner, opponent->owner);
        combat_spawn_hit_vfx(owner, opponent->owner);
        tag = combat_get_weapon_tag_position(owner, opponent->owner);
        if (tag == -1)
            return;
        fx_attach_to_character("generichit03.vfx", opponent->owner->handle, tag, -1, 0, 0);
        if (flinch == true)
            opponent->play_flinch(owner->cur_move->variant);
        break;
    case SKILL_TRIP: case SKILL_SERPENT_WHIP_TRIP:
        opponent->owner->ai.knock_down(rand_int_range(3000, 7000));
        break;
    case SKILL_PUSH:
        opponent->play_flinch(owner->cur_move->variant);
        break;
    case SKILL_KICK:
        if (flinch == true)
            opponent->owner->ai.disable_2f();
        break;
    case SKILL_SERPENT_WHIP_PUSH:
        opponent->owner->ai.disable_2f();
        break;
    case SKILL_SILENCE:
        combat_play_weapon_hit_sound(owner, opponent->owner);
        combat_spawn_hit_vfx(owner, opponent->owner);
        if (flinch == true)
            opponent->play_flinch(owner->cur_move->variant);
        opponent->owner->status_effect_add(STATUS_SILENCE, 15000, -1, 0, owner->handle, 0);
        fx_attach_to_character("generichit05.vfx", opponent->owner->handle, -1, -1, 0, 0);
        break;
    case SKILL_AIMED_ATTACK_EYES: case SKILL_BLINDNESS_STAB:
        combat_play_weapon_hit_sound(owner, opponent->owner);
        combat_spawn_hit_vfx(owner, opponent->owner);
        if (flinch == true)
            opponent->play_flinch(owner->cur_move->variant);
        if (cur_character->get_skill(SKILL_AIMED_ATTACK) > frand()) {
            opponent->owner->status_effect_add(STATUS_BLIND, 15000, -1, 0, owner->handle, 0);
            fx_attach_to_character("generichit04.vfx", opponent->owner->handle, -1, -1, 0, 0);
        }
        break;
    case SKILL_AIMED_ATTACK_ARMS: case SKILL_ARM_SLICE:
        combat_play_weapon_hit_sound(owner, opponent->owner);
        combat_spawn_hit_vfx(owner, opponent->owner);
        if (flinch == true)
            opponent->play_flinch(owner->cur_move->variant);
        if (cur_character->get_skill(SKILL_AIMED_ATTACK) > frand()) {
            opponent->owner->status_effect_add(STATUS_CRIPPLE, 15000, -1, 0, owner->handle, 0);
            fx_attach_to_character("generichit03.vfx", opponent->owner->handle, -1, -1, 0, 0);
        }
        break;
    case SKILL_AIMED_ATTACK_LEGS: case SKILL_LEG_SWIPE:
        combat_play_weapon_hit_sound(owner, opponent->owner);
        combat_spawn_hit_vfx(owner, opponent->owner);
        if (flinch == true)
            opponent->play_flinch(owner->cur_move->variant);
        if (cur_character->get_skill(SKILL_AIMED_ATTACK) > frand()) {
            opponent->owner->status_effect_add(STATUS_SLOW, 15000, -1, 0, owner->handle, 0);
            fx_attach_to_character("generichit05.vfx", opponent->owner->handle, -1, -1, 0, 0);
        }
        break;
    }
}

// 0x4665f0  Confidence: likely (the matrix assignment of the dead `orient` is expanded inline, partly
// through vector3::operator= calls). The bone of the defender nearest to the hit tag of the attacker's
// current move on the weapon (or to the hand bone when unarmed); -1 when there is no move.
int combat_get_weapon_tag_position(Character *attacker, Character *defender)
{
    move_info *m = (move_info *)attacker->get_current_move();
    vector3 p, tag_pos, bone_pos, bp;
    matrix tag_orient, bone_orient, rot, orient, bo;
    int bone, tag, i, n, best;
    float best_d;

    if (!attacker || !defender || !m)
        return -1;
    if (attacker->equipment[5]) {
        if (!m->hit_tag)
            return -1;
        tag = vim_find_tag(attacker->equipment[5]->type->mesh, m->hit_tag);
        vim_get_tag_transform(attacker->equipment[5]->type->mesh, tag, &tag_pos, &tag_orient);
        bone = attacker->info->_unk168[0];
        attacker->instance->get_bone_transform(&bone_pos, &bone_orient, bone);
        matrix43 bone_m(bone_pos, bone_orient);
        matrix43 tag_m(tag_pos, tag_orient);
        matrix43 world = tag_m * bone_m;
        world.get_rotation(&rot);
        p.x = world.m[3][0];
        p.y = world.m[3][1];
        p.z = world.m[3][2];
        orient = attacker->orient * rot;
    } else {
        bone = attacker->info->_unk168[0];
        attacker->instance->get_bone_transform(&bone_pos, &bone_orient, bone);
        matrix43 world(bone_pos, bone_orient);
        world.get_rotation(&rot);
        p.x = world.m[3][0];
        p.y = world.m[3][1];
        p.z = world.m[3][2];
        orient = attacker->orient * rot;
    }
    p.unrotate(attacker->orient);
    best = -1;
    best_d = 10000000.0f;
    p.x = p.x + attacker->pos.x;
    p.y = p.y + attacker->pos.y;
    p.z = p.z + attacker->pos.z;
    n = defender->instance->character->mesh->skin->num_bones;
    for (i = 0; i < n; i++) {
        float dx, dy, dz, dist;

        character_get_bone_world_matrix(defender, i, &bp, &bo);
        dx = p.x - bp.x;
        dy = p.y - bp.y;
        dz = p.z - bp.z;
        dist = dz * dz + dy * dy + dx * dx;
        if (dist < best_d) {
            best_d = dist;
            best = i;
        }
    }
    return best;
}

// 0x466920  Confidence: exact (the matrix copy is nine inline dword moves).
void character_get_bone_world_matrix(Character *c, int bone, vector3 *out_pos, matrix *out_orient)
{
    c->instance->get_bone_transform(out_pos, out_orient, bone);
    *out_orient = c->orient * *out_orient;
    out_pos->unrotate(c->orient);
    out_pos->x = c->pos.x + out_pos->x;
    out_pos->y = c->pos.y + out_pos->y;
    out_pos->z = c->pos.z + out_pos->z;
}

// 0x4669c0  Confidence: exact (byte-indexed jump table 0x466a64 for skin 1..16). The chosen name is
// not what is spawned: skin 1 gets "blood.vfx", skins 3 and 9 "sparkhit01.vfx", the others (2, 10-16)
// nothing, and "sparkhit.vfx" is never used (literal).
void combat_spawn_hit_vfx(Character *attacker, Character *defender)
{
    int skin = defender->info->skin;
    const char *name = NULL;
    int tag;

    switch (skin) {
    case 1: case 2:
        name = "blood.vfx";
        break;
    case 3: case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16:
        name = "sparkhit.vfx";
        break;
    }
    if (name) {
        tag = combat_get_weapon_tag_position(attacker, defender);
        if (tag == -1)
            return;
        if (skin == 1) {
            fx_attach_to_character("blood.vfx", defender->handle, tag, -1, 0, 0);
            return;
        }
        if (skin == 3 || skin == 9)
            fx_attach_to_character("sparkhit01.vfx", defender->handle, tag, -1, 0, 0);
    }
}

// 0x466a80  Confidence: exact. Weapon material (or the attacker's skin) against the leg armour (slot 7)
// or the defender's skin.
void combat_play_weapon_hit_sound(Character *attacker, Character *defender)
{
    int mat = attacker->equipment[5] ? attacker->equipment[5]->type->material : attacker->info->skin;

    surface_play_sound(mat, defender->equipment[7] ? defender->equipment[7]->type->material : defender->info->skin, &defender->pos);
}

// The base chain window per point of speed skill (.rdata 0x5819ac, read by chain_check_window only): a
// const float of this unit (D7806; name ours). VC6 loads a const float from memory instead of folding it,
// so a literal `* 2.0f` would compile to fadd st, st.
const float chain_window_per_speed = 2.0f;

// 0x466ae0  Confidence: likely (dword jump table 0x466d04: only state 1 does work; the tail after the
// window test is written once, the binary has it twice). The chain-attack timing window of the player's
// input: 0 = accepted (chain started or continued), +100 = not allowed, -100 = no attack, otherwise
// the signed distance to the window.
float living_combat::chain_check_window(int skill_id)
{
    float window, t;
    int i;

    switch (state) {
    case COMBAT_ATTACKING:
        window = owner->get_skill(SKILL_SPEED) * chain_window_per_speed;
        for (i = 0; i < chain_count; i++) {
            if (i < chain_index)
                window *= 0.95f;
            else
                window *= 0.9f;
        }
        if (skill_id == SKILL_PUSH)
            window += window;
        if (owner->get_skill(SKILL_DOUBLE_ATTACK) > 0.0f)
            window = (owner->get_skill(SKILL_DOUBLE_ATTACK) + 1.0f) * window;
        if (skill_id == SKILL_AXESPIN) {
            if (!owner->find_move(0, 0x77))
                return 100.0f;
            if (!owner->equipment[5])
                return 100.0f;
            if (owner->equipment[5]->type->weapon_skill != SKILL_AXE_WEAPONS)
                return 100.0f;
        } else if (skill_id == SKILL_EARTHSHAKER) {
            if (!owner->find_move(0, 0x78))
                return 100.0f;
            if (owner->info->species != 4) {
                if (!owner->equipment[5])
                    return 100.0f;
                if (owner->equipment[5]->type->weapon_skill != 5)
                    return 100.0f;
            }
        }
        if (owner->move_trigger_time_left(6) <= 0.0f)
            break;
        if (owner->move_trigger_time_left(7) <= 0.0f) {
            t = -owner->move_trigger_time_left(7);
            if (t < window) {
                if (skill_id == SKILL_NONE)
                    return 0.0f;
                if (skill_id == last_chain_skill)
                    return 100.0f;
                chain_learn = true;
                if (++chain_count == 1)
                    chain_index = 0;
                chain_skill = skill_id;
                last_chain_skill = skill_id;
                return 0.0f;
            }
            return window - t;
        } else {
            t = owner->move_trigger_time_left(7);
            if (t < window) {
                if (skill_id == SKILL_NONE)
                    return 0.0f;
                if (skill_id == last_chain_skill)
                    return 100.0f;
                chain_learn = true;
                if (++chain_count == 1)
                    chain_index = 0;
                chain_skill = skill_id;
                last_chain_skill = skill_id;
                return 0.0f;
            }
            return -(window - t);
        }
    case COMBAT_IDLE:
    case COMBAT_HIT:
    case COMBAT_DEFENCE:
    case COMBAT_REACTING:
    case COMBAT_SPECIAL:
        return -100.0f;
    }
    return -100.0f;
}

// 0x466d20  Confidence: exact (dword jump table 0x466e70 for states 0..5). Both sides attack each other.
void living_combat::update_exchange()
{
    switch (state) {
    case COMBAT_IDLE:
        if (opponent->state == COMBAT_IDLE)
            resolve_exchange(false);
        return;
    case COMBAT_ATTACKING:
        if (owner->move_trigger_fired(6, 1))
            break;
        if (!owner->move_trigger_fired(7, 0))
            return;
        if (species_is_hero(owner->info->species))
            feedback_play(owner->info->species, 1, &owner->pos);
        if (opponent->state != COMBAT_HIT) {
            if (opponent->defense_move && opponent->defense_move->category == 2) {
                combat_spawn_spark_vfx(owner, opponent->owner);
                trigger_reflex_ability(0.0f);
            }
            return;
        }
        apply_melee_hit(true);
        return;
    case COMBAT_REACTING:
        if (owner->move_trigger_fired(6, 1))
            state = COMBAT_IDLE;
        return;
    case COMBAT_HIT:
        if (opponent->state == COMBAT_IDLE)
            break;
        if (opponent->state == COMBAT_REACTING)
            state = COMBAT_IDLE;
        return;
    case COMBAT_DEFENCE:
        if (opponent->state == COMBAT_IDLE || opponent->state == COMBAT_REACTING)
            break;
        if (!defense_in_time())
            return;
        play_defense();
        if (state == COMBAT_REACTING && defense_move->category == 2)
            queue_counter_attack();
        return;
    case COMBAT_SPECIAL:
        if (!owner->move_trigger_fired(6, 1))
            return;
        break;
    }
    state = COMBAT_IDLE;
}

// 0x466e90  Confidence: exact. Metal weapons (materials 10..16) spark on a parry 80 % of the time.
void combat_spawn_spark_vfx(Character *attacker, Character *defender)
{
    int tag;

    if (frand() <= 0.8f && attacker->equipment[5]) {
        int mat = attacker->equipment[5]->type->material;

        if (mat >= 10 && mat <= 16) {
            tag = combat_get_weapon_tag_position(attacker, defender);
            if (tag != -1)
                fx_attach_to_character("sparkhit01.vfx", defender->handle, tag, -1, 0, 0);
        }
    }
}

// 0x466f00  Confidence: exact (dword jump table 0x467008 for states 0..3). One-sided attack, at most
// every 3 / (10 x speed) seconds. LOGIC FIX (D8706): the table sends HIT (2) and DEFENCE (3) straight to
// `state = COMBAT_IDLE` (0x466fff); only states above 3 wait for move trigger 6 (0x466ff0).
void living_combat::update_attack()
{
    int r;

    if (cooldown > 0.0f)
        cooldown -= reported_frametime;
    switch (state) {
    case COMBAT_IDLE:
        if (chain_learn || cooldown <= 0.0f || opponent->owner->type == 9) {
            cooldown = 1.0f / (owner->get_skill(SKILL_SPEED) * 10.0f) * 3.0f;
            resolve_exchange(true);
        }
        return;
    case COMBAT_ATTACKING:
        if (owner->move_trigger_fired(6, 1))
            break;
        r = owner->move_trigger_fired(7, 1);
        if (r == 1) {
            if (species_is_hero(owner->info->species))
                feedback_play(owner->info->species, 1, &owner->pos);
            if (hits > 0)
                resolve_hit();
            return;
        }
        if (r == -1)
            state = COMBAT_IDLE;
        return;
    case COMBAT_HIT:
    case COMBAT_DEFENCE:
        break;
    default:
        if (!owner->move_trigger_fired(6, 1))
            return;
        break;
    }
    state = COMBAT_IDLE;
}

// 0x467020  Confidence: exact (switch lowered to sub / dec tests). Ranged attack (AI combat sub 0x10).
void living_combat::update_ranged()
{
    switch (state) {
    case COMBAT_IDLE:
        resolve_exchange(true);
        return;
    case COMBAT_SPECIAL:
        if (owner->move_trigger_fired(7, 1) == 1) {
            fire_weapon_missile();
            if (owner->info->species == 0x57)
                tiger_restore_ride_anims(owner);
            state = COMBAT_REACTING;
        }
        return;
    case COMBAT_REACTING:
        if (owner->move_finished(anim)) {
            state = COMBAT_IDLE;
            if (owner->type == 9)
                boss_clear_current_move(owner);
        }
        return;
    default:
        state = COMBAT_IDLE;
        if (owner->type == 9)
            boss_clear_current_move(owner);
        return;
    }
}

// One cast of a triggered spell by `c` (the defender) for combat_trigger_reflex_ability: on the
// multiplayer host as a request (spell_prepare_cast, mp_spell_cast_sync, spell_cast_launch_effects),
// otherwise a hit-only spell hits the target directly and the others go through spell_cast_execute.
// Written out four times in the binary with the stack temporaries of each copy; a macro here (D5506).
#define COMBAT_TRIGGER_CAST(c, sp, tgt, tpos, tlayer, itype, src, hit_char, sp_target) do { \
        if (game_flags & GF_MULTIPLAYER) { \
            spell_cast_request req_(-1); \
            req_.caster = (c); \
            req_.spell = (sp); \
            req_.target = (tgt); \
            req_.target_pos = (tpos); \
            req_.layer = (tlayer); \
            req_.item = (itype); \
            req_.source_type = (src); \
            req_.params = spell_prepare_cast((c), (sp), (tgt), (itype), (src), damage); \
            mp_spell_cast_sync(&req_); \
            spell_cast_launch_effects(&req_); \
        } else if (spells[sp].flags & SPELL_HIT_ONLY) { \
            (hit_char)->get_hit_by_spell((sp), (c)->handle, (tpos), &(hit_char)->orient, (src), (itype), \
                                         damage, false); \
            spell_play_hit_sound((sp), &(hit_char)->pos); \
        } else \
            spell_cast_execute((c), (sp), (sp_target), (tpos), (tlayer), (int)(itype), (src), 0); \
    } while (0)

// 0x4670a0  Confidence: likely (written with combat_trigger_reflex_ability's macro; same size as the original,
// a different register order in the prologue; D5700). The attacker answers its own landed hit:
// with the chance of its char_info special ability it casts that spell (source type 1), then its weapon's
// triggered effect with that effect's chance (source type 3, the weapon type as the item). A beneficial spell
// goes to the attacker itself (at its position + 1.2 up), any other to the defender.
void living_combat::trigger_special_ability(float damage)
{
    vector3 target_pos = opponent->owner->pos;
    target_pos.y += 1.2f;
    vector3 self_pos = owner->pos;
    self_pos.y += 1.2f;
    int spell;
    item_info *w;

    if (owner->info->special_ability != -1 && frand() < owner->info->special_chance) {
        spell = owner->info->special_ability;
        if (spells[spell].flags & SPELL_BENEFICIAL)
            COMBAT_TRIGGER_CAST(owner, spell, owner->handle, &self_pos, owner->layer, (ItemType *)0, 1,
                                owner, owner->handle);
        else
            COMBAT_TRIGGER_CAST(owner, spell, opponent->owner->handle, &target_pos, opponent->owner->layer,
                                (ItemType *)0, 1, opponent->owner, opponent->owner->handle);
    }
    w = owner->equipment[5];
    if (w && w->type->weapon_triggered_effect && frand() < w->type->weapon_triggered_effect->chance) {
        spell = w->type->weapon_triggered_effect->spell;
        if (spells[spell].flags & SPELL_BENEFICIAL)
            COMBAT_TRIGGER_CAST(owner, spell, owner->handle, &self_pos, owner->layer, owner->equipment[5]->type,
                                3, owner, owner->handle);
        else
            COMBAT_TRIGGER_CAST(owner, spell, opponent->owner->handle, &target_pos, opponent->owner->layer,
                                owner->equipment[5]->type, 3, opponent->owner, opponent->owner->handle);
    }
}

// 0x467670  Confidence: likely (VC6 compiles the same flow and stores, 1605 bytes, but keeps opponent->owner in
// callee-saved registers across frand where the original reloads it and holds -1 in edi; D5506).
// The defender (opponent's owner) answers a hit: with the chance of its char_info reflex ability, it
// casts that spell (source type 2); then every worn item whose type has an armour triggered effect
// casts that effect's spell with its chance (source type 4, the item type as the item). A beneficial
// spell goes to the defender itself (at its position + 1.2 up), any other to the attacker (owner). The
// single-player spell_cast_execute call of the item effects passes the defender's own handle as the
// target of an offensive spell (with the attacker's position and layer), unlike every other branch
// (bug K300 candidate, D5506).
void living_combat::trigger_reflex_ability(float damage)
{
    vector3 self_pos = opponent->owner->pos;
    self_pos.y += 1.2f;
    vector3 attacker_pos = owner->pos;
    attacker_pos.y += 1.2f;
    int spell = opponent->owner->info->reflex_ability;
    int i;

    if (spell != -1 && frand() < opponent->owner->info->reflex_chance) {
        if (spells[spell].flags & SPELL_BENEFICIAL)
            COMBAT_TRIGGER_CAST(opponent->owner, spell, opponent->owner->handle, &self_pos,
                                opponent->owner->layer, (ItemType *)0, 2, opponent->owner,
                                opponent->owner->handle);
        else
            COMBAT_TRIGGER_CAST(opponent->owner, spell, owner->handle, &attacker_pos, owner->layer,
                                (ItemType *)0, 2, owner, owner->handle);
    }
    for (i = 0; i < 9; i++) {
        item_info *it = opponent->owner->equipment[i];
        if (it && it->type->kind == 1 && it->type->armour_triggered_effect &&
            frand() < it->type->armour_triggered_effect->chance) {
            spell = it->type->armour_triggered_effect->spell;
            if (spells[spell].flags & SPELL_BENEFICIAL)
                COMBAT_TRIGGER_CAST(opponent->owner, spell, opponent->owner->handle, &self_pos,
                                    opponent->owner->layer, opponent->owner->equipment[i]->type, 4,
                                    opponent->owner, opponent->owner->handle);
            else
                COMBAT_TRIGGER_CAST(opponent->owner, spell, owner->handle, &attacker_pos, owner->layer,
                                    opponent->owner->equipment[i]->type, 4, owner,
                                    opponent->owner->handle);
        }
    }
}

// 0x467cb0  Confidence: exact. The backstab special attack (AI combat sub 0x11): move 0x6c, the full
// bonus when the target is not in combat mode.
void living_combat::update_special()
{
    move_info *m;
    int r;

    switch (state) {
    case COMBAT_IDLE:
        m = owner->find_move(0, 0x6c);
        if (!m)
            return;
        owner->ai.flags |= 2;
        owner->ai.emerge_move = owner->play_move((int)m, 1.0f, 1, 1, 0);
        state = COMBAT_ATTACKING;
        skill = SKILL_BACKSTAB;
        if (opponent->owner->ai.is_mode(AI_STATE_COMBAT, -1))
            backstab_full = false;
        else
            backstab_full = true;
        opponent->roll_defense(this, true);
        if (game_flags & 2)
            send_net(1, false);
        return;
    case COMBAT_ATTACKING:
        if (!owner->move_playing(owner->ai.emerge_move, NULL)) {
            state = COMBAT_IDLE;
            return;
        }
        if (owner->move_trigger_fired(6, 0)) {
            state = COMBAT_IDLE;
            return;
        }
        r = owner->move_trigger_fired(7, 1);
        if (r == -1) {
            state = COMBAT_IDLE;
            return;
        }
        if (r == 1)
            resolve_hit();
        return;
    }
}

// 0x467db0  Confidence: exact.
void character_break_paralysis_on_hit(Character *c)
{
    if (c && c->has_status(STATUS_PARALYZE) == true && (c->flags & 0x80000))
        c->status_effect_cure(STATUS_PARALYZE, 1, 0);
}

// 0x467df0  Confidence: likely. Hit resolution: running targets are slowed, bosses test their move
// range, others need the target in reach (x 2.25 when it moves); then the damage, and the knock-down /
// stun / flinch reactions. Returns false when the hit did not happen or the target died.
bool living_combat::resolve_hit()
{
    bool boss = owner->type == 9;
    bool no_flinch;
    Character *t;

    if (owner->ai.is_combat_sub10() == true)
        opponent->owner->ai.retaliate_on_hit(owner->handle);
    if (!hits)
        return false;
    no_flinch = false;
    if (!opponent)
        return false;
    if (opponent->owner->loco_blend > 0.5f && (game_flags & 1))
        opponent->owner->status_effect_add(STATUS_SLOW, 5000, -1, 0, -1, 0);
    character_break_paralysis_on_hit(opponent->owner);
    boss_counter_hit_dispatch(owner, opponent->owner);
    if (boss) {
        if (!boss_move_in_range_dispatch(owner, opponent->owner))
            return false;
    } else if (owner->ai.is_attacking() == true) {
        float reach = owner->info->attack_radius;
        float dist = owner->distance_between_edges(opponent->owner, true, owner->ai._unk7c);

        if (opponent->owner->is_moving() && !opponent->owner->ai.is_mode(AI_STATE_DISABLED, 0x2f))
            reach *= 2.25f;
        if (dist > reach)
            return false;
        t = opponent->owner;
        if ((t->ai.target == owner->handle && t->ai.is_mode(AI_STATE_COMBAT, -1) && (t->ai.flags & 0x20) &&
             reach * 1.2f > dist) ||
            opponent->owner->ai.is_mode(AI_STATE_COMBAT, 0x11))
            no_flinch = true;
    }
    apply_melee_hit(false);
    t = opponent->owner;
    if (t->hp <= 0.0f)
        return false;
    if (boss) {
        if (t->ai.is_mode(AI_STATE_DISABLED, -1))
            return true;
        if (opponent->owner->find_move(9, 0x8e) && boss_attack_knocks_down_dispatch(owner->info->species)) {
            opponent->owner->ai.knock_down(rand_int_range(3000, 7000));
            combat_play_weapon_hit_sound(owner, opponent->owner);
            return true;
        }
        if (opponent->owner->find_move(9, 0x91) && boss_attack_stuns_dispatch(owner->info->species)) {
            opponent->owner->ai.disable_2f();
            combat_play_weapon_hit_sound(owner, opponent->owner);
            return true;
        }
        if (!opponent->owner->move_playing(opponent->flinch_anim, NULL) ||
            opponent->owner->ai.is_combat_e_or_sneak_attack()) {
            opponent->play_flinch_secondary(owner->cur_move->variant);
            combat_play_weapon_hit_sound(owner, opponent->owner);
            return true;
        }
    }
    t = opponent->owner;
    if (t->type == 9 || t->ai.get_mode() == AI_STATE_DISABLED || opponent->owner->ai.is_attacking())
        return true;
    if (!owner || !owner->cur_move || no_flinch)
        return true;
    opponent->play_flinch(owner->cur_move->variant);
    combat_play_weapon_hit_sound(owner, opponent->owner);
    return true;
}

// 0x468160  Confidence: exact.
void living_combat::flinch_if_idle(int variant)
{
    if (owner->type != 9 && !owner->ai.is_attacking() && !owner->ai.is_combat_sub10() &&
        owner->ai.get_mode() != AI_STATE_DISABLED)
        play_flinch(variant);
}

// 0x4681b0  Confidence: exact as code (a constructor: GameTimer at +0xc, reset, returns this). Written
// as construct() while Character keeps the D1 union around +0x558 (D300); in the original this is
// living_combat::living_combat().
living_combat *living_combat::construct()
{
#ifdef _MSC_VER
    // An explicit constructor call (D7805): a placement new makes VC6 /GX open an EH frame for the
    // matching placement delete, which the binary does not have.
    ((GameTimer *)&timer)->GameTimer::GameTimer();
#else
    new (&timer) GameTimer;
#endif
    reset();
    return this;
}

// 0x4681d0  Confidence: exact.
void living_combat::set_owner(Character *c)
{
    owner = c;
}

// 0x4681e0  Confidence: exact.
Character *living_combat::get_target()
{
    return living_from_handle(target);
}

// 0x4681f0  Confidence: exact (the first argument is not read).
void living_combat::set_target(int unused, Character *c)
{
    reset();
    target = c->handle;
}

// 0x468210  Confidence: exact. Per-frame dispatch; true when the record is idle. A melee exchange needs
// both sides attacking each other and facing (dot of the forward vectors below -0.95).
bool living_combat::update(int move_id)
{
    Character *t = living_from_handle(target);
    float facing;

    if (!t) {
        reset();
        return true;
    }
    opponent = &t->combat;
    opponent = &t->combat;
    facing = owner->orient.fvec.dot(t->orient.fvec);
    if (owner->ai.is_attacking() == true) {
        if (owner->type == 9) {
            update_boss_attack(move_id);
            return state == COMBAT_IDLE ? true : false;
        }
        if (opponent->owner->type == 9) {
            update_attack();
            return state == COMBAT_IDLE ? true : false;
        }
        if (!opponent->owner->ai.is_attacking()) {
            update_attack();
            return state == COMBAT_IDLE ? true : false;
        }
        if (opponent->target != owner->handle || !(facing < -0.95f)) {
            update_attack();
            return state == COMBAT_IDLE ? true : false;
        }
        opponent->opponent = this;
        update_exchange();
        return state == COMBAT_IDLE ? true : false;
    }
    if (owner->ai.is_combat_sub10() == true) {
        update_ranged();
        return state == COMBAT_IDLE ? true : false;
    }
    if (owner->ai.is_combat_sub11() == true) {
        update_special();
        return state == COMBAT_IDLE ? true : false;
    }
    return state == COMBAT_IDLE ? true : false;
}

// 0x468360  Confidence: exact (VC6 byte match; the jump table: 0, 1, 2-3, 6, the rest; the trigger-7 test as
// written). A boss's attack (living_combat::update for type 9): idle starts move_id unless the same move is
// still playing or a boss hook takes over; state 6 is the boss special cast that boss_cast_special_dispatch
// accepts for the move's anim.
void living_combat::update_boss_attack(int move_id)
{
    move_info *m;
    int a, r;
    bool hook;

    switch (state) {
    case COMBAT_IDLE:
        if (move_id == move_variant && !owner->move_finished(anim))
            return;
        if (boss_hook_dispatch_51_c1(owner, opponent->owner))
            return;
        m = owner->find_move(0, move_id);
        a = owner->play_move((int)m, 1.0f, 0, 0, 0);
        if (a == -1)
            return;
        anim = a;
        state = COMBAT_ATTACKING;
        move_variant = m->id;
        if (!boss_attack_test_dispatch(this))
            opponent->roll_defense(this, true);
        if (game_flags & 2)
            send_net(1, false);
        if (owner->type == 9 && boss_cast_special_dispatch(owner, a))
            state = 6;
        return;
    case COMBAT_ATTACKING:
        if (owner->move_trigger_fired(6, 1)) {
            boss_clear_current_move(owner);
            state = COMBAT_IDLE;
            return;
        }
        r = owner->move_trigger_fired(7, 1);
        hook = false;
        if (boss_move_trigger_dispatch(owner))
            hook = true;
        if (r == 1 || (r == 0 && hook)) {
            boss_attack_trigger_dispatch(owner, opponent->owner);
            if (hits > 0) {
                if (owner->info->species == 0x59 && character_is_serpent_part(owner))
                    return;
                if (!boss_attack_resolve_dispatch(this))
                    resolve_hit();
            }
        } else if (r == -1) {
            boss_clear_current_move(owner);
            state = COMBAT_IDLE;
        }
        return;
    case COMBAT_HIT:
    case COMBAT_DEFENCE:
        boss_clear_current_move(owner);
        state = COMBAT_IDLE;
        return;
    case 6:
        if (owner->move_trigger_fired(7, 0))
            boss_attack_trigger_dispatch(owner, opponent->owner);
        if (owner->move_trigger_fired(6, 1)) {
            state = COMBAT_IDLE;
            boss_clear_current_move(owner);
        }
        return;
    default:
        if (owner->move_trigger_fired(6, 1)) {
            state = COMBAT_IDLE;
            boss_clear_current_move(owner);
        }
        return;
    }
}

// 0x4685c0  Confidence: exact. Replays an exchange sent by another machine (living_combat_send_net):
// hit = 0 makes the defender (the target's record) pick its parry / dodge or take the hit.
void living_combat::apply_net_exchange(int hit, int move, int defence, int skill_id, bool special)
{
    Character *t = living_from_handle(target);

    if (!t)
        return;
    opponent = &t->combat;
    switch (skill_id) {
    case SKILL_BACKSTAB: {
        chain_skill = SKILL_BACKSTAB;
        move_info *m = owner->find_move(0, 0x6c);
        owner->ai.emerge_move = owner->play_move((int)m, 1.0f, 1, 1, 0);
        if (MP_HOST) {
            state = COMBAT_ATTACKING;
            skill = SKILL_BACKSTAB;
            if (opponent->owner->ai.is_mode(AI_STATE_COMBAT, -1))
                backstab_full = false;
            else
                backstab_full = true;
            opponent->roll_defense(this, true);
        }
        break;
    }
    case SKILL_NONE:
        skill = SKILL_NONE;
        if (special)
            play_special(owner->find_move(0xb, move));
        else
            play_attack(owner->find_move(0, move));
        break;
    default:
        chain_skill = skill_id;
        play_skill_attack();
        break;
    }
    if (hit) {
        if (special) {
            hits = 1;
            return;
        }
        if (MP_HOST)
            opponent->roll_defense(this, true);
        return;
    }
    opponent->state = COMBAT_HIT;
    if (defence != -1) {
        opponent->opponent = this;
        switch (defence) {
        case 2:
            opponent->pick_parry();
            break;
        case 3:
            opponent->pick_dodge();
            break;
        }
    }
    if (opponent->state == COMBAT_HIT)
        hits = 1;
}

// 0x468740  Confidence: exact.
bool living_combat::is_busy()
{
    return state != COMBAT_IDLE ? true : false;
}

// 0x468750  Confidence: exact.
void living_combat::set_38(int s)
{
    chain_skill = s;
}

// 0x468760  Confidence: likely (VC6 expands the vector operators less in each later case, which this
// source order reproduces only if the original's matches; D5701; u takes the unrounded radius, D9203). A blood splot decal (2 s, lit, depth
// 0.25) under the target: at a random point of its size radius on the ground plane, with a random
// horizontal direction (length at least 0.03 before normalising) and its perpendicular as the half-axes,
// scaled by 0.2 / 0.3 / 0.45 of the radius for size 0 / 1 / 2. Another size places the decal with bitmap
// -1. The rand() results are not used. The grey level goes to decal_place's colour argument bit for bit
// (decal_place declares it int; lights_shade_vertices' last argument, probably a float).
void living_combat::blood_splot_place(int size, float grey)
{
    Character *t = get_target();
    float radius = t->size_radius();
    vector3 pos = t->pos;
    vector3 off;
    vector3 u;
    int bm;

    off.random_unit_vector();
    off.y = 0.0f;
    off *= radius;
    pos += off;
    do {
        u.random_unit_vector();
        u.y = 0.0f;
    } while (u.len() < 0.03f);
    u.normalize_safe();
    vector3 v;
    v.x = 0.0f - u.z;
    v.y = 0.0f;
    v.z = u.x - 0.0f;
    bm = -1;
    switch (size) {
    case 0:
        u = u * (radius *= 0.2f);
        v = v * radius;
        rand();
        bm = bm_load("BloodSplot-small02.tga", -1, -1);
        break;
    case 1:
        u = u * (radius *= 0.3f);
        v = v * radius;
        rand();
        bm = bm_load("BloodSplot-med02.tga", -1, -1);
        break;
    case 2:
        u = u * (radius *= 0.45f);
        v = radius * v;
        rand();
        bm = bm_load("BloodSplot-large02.tga", -1, -1);
        break;
    }
    decal_place(bm, &pos, &u, &v, 2000, true, *(int *)&grey, 0.25f);
}

// 0x468a90  Confidence: exact (an out-of-line vector operator whose COMDAT landed in this unit; 16
// other callers). s * v.
vector3 operator*(float s, const vector3 &v)
{
    return vector3(s * v.x, s * v.y, s * v.z);
}

// 0x468ac0  Confidence: exact (the colour bytes are read as `mov eax, [mem]; and eax, 0xff`, i.e.
// ubyte locals). Blood splots for skins 1-2 (flesh): one sized by the damage for small characters,
// one of each size up to it for large ones. The rand() call's result is not used (literal).
void living_combat::spawn_blood(float damage)
{
    int skin = opponent->owner->info->skin;
    Character *t;
    ubyte r, g, b;
    float grey;

    if (skin <= 0 || skin > 2)
        return;
    t = get_target();
    lighting_sample_stub(t->ground_lobj, t->ground_face, t->ground_hit, &r, &g, &b);
    grey = rgb_to_lightened_grey(r, g, b);
    if (t->info->size <= 1) {
        if (damage > 15.0f) {
            blood_splot_place(2, grey);
            return;
        }
        if (damage > 5.0f) {
            rand();
            blood_splot_place(1, grey);
            return;
        }
        blood_splot_place(0, grey);
        return;
    }
    if (t->info->size >= 2) {
        if (damage > 15.0f)
            blood_splot_place(2, grey);
        if (damage > 5.0f)
            blood_splot_place(1, grey);
        blood_splot_place(0, grey);
    }
}

// 0x468c40  Confidence: exact. Only the serpent bosses (type 9, species 0x59) take a special skill.
void living_combat::set_special_skill_species89(int s)
{
    if (owner->type == 9 && owner->info->species == 0x59)
        skill = s;
}

// 0x468c60  Confidence: exact.
int living_combat::get_special_skill()
{
    return skill;
}

// 0x468c70  Confidence: exact.
void living_combat::set_40(int n)
{
    hits = n;
}
