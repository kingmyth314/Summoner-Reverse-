// player.cpp - Sum.exe v1.40, code 0x433340-0x43439f (Summoner\player\player.cpp)
// Written from the Sum.exe disassembly (build/sum.asm). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: symbols/sum.symbols tags 22 functions from 0x433400 to 0x434350. The unit starts at the
// static initialiser thunk 0x433340 instead: the initialisers 0x433340-0x4333f6 construct the GameTimers
// of party_list, player_free_list, player_list_other and Players[5], all player.cpp .bss. The three
// untagged functions inside the range (player_input_reset 0x433730, player_skill_unlocked_at_level
// 0x433b10, player_init_from_character 0x433b70) are Player members like their neighbours and are
// written here too. The previous unit ends with the "gr_info" console command's RET at 0x433331; the
// next one starts with init_ccmd_slew_thunk at 0x4343a0.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "summoner/player/player.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/console.h"
#include "vsdk/localize/localize.h"
#include "summoner/summoner.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/interface.h"
#include "engine/camera/camera.h"
#include "engine/rendering/levelrender.h"
#include "engine/gamesound/gamesound.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

// Original __FILE__ of this unit (literal 0x5938bc).
#define PLAYER_CPP "D:\\projects\\Summoner\\pccode\\Summoner\\player\\player.cpp"

// ---- .data, in address order (0x5934d0-0x5937eb, the literals follow) --------------------------
// Fourteen model names; GameInit passes &[0], &[2], &[4], &[6] to character_load (two names for the
// first three heroes, one for Rosalind). The "#" half is not referenced directly. Label ours.
char *hero_model_names[14] = {        // 0x5934d0
    "Joseph", "Heavy Joseph", "Jekhar", "Heavy Jekhar", "Flece", "Heavy Flece", "Rosalind",
    "#Joseph", "#Heavy Joseph", "#Jekhar", "#Heavy Jekhar", "#Flece", "#Heavy Flece", "#Rosalind",
};

// {skill, level, value}. The counts are separate ints (mp_player_file_create and player_create read
// them from memory; player_init_from_character uses the array sizes).
skill_progression joseph_skill_progression[17] = {      // 0x593508
    { 3, 1, 10 }, { 0x1b, 1, 10 }, { 0xe, 1, 10 }, { 0xb, 1, 10 }, { 0x1c, 1, 10 },
    { 6, 5, 10 }, { 9, 6, 10 }, { 0x1f, 7, 10 }, { 8, 8, 10 }, { 0x20, 9, 10 },
    { 7, 10, 10 }, { 5, 12, 10 }, { 0x23, 13, 10 }, { 0xd, 14, 10 }, { 4, 15, 10 },
    { 0x1d, 16, 10 }, { 0xa, 17, 10 },
};
int joseph_skill_progression_count = sizeof(joseph_skill_progression) / sizeof(skill_progression); // 0x5935d4
skill_progression jekhar_skill_progression[13] = {      // 0x5935d8
    { 4, 1, 10 }, { 3, 1, 10 }, { 0xb, 1, 10 }, { 8, 1, 10 }, { 9, 1, 10 },
    { 0xe, 1, 10 }, { 5, 5, 10 }, { 0xa, 6, 10 }, { 0xf, 8, 10 }, { 0xc, 10, 10 },
    { 0xd, 12, 10 }, { 0x23, 14, 10 }, { 0x1c, 18, 10 },
};
int jekhar_skill_progression_count = sizeof(jekhar_skill_progression) / sizeof(skill_progression); // 0x593674
skill_progression flece_skill_progression[16] = {       // 0x593678
    { 7, 1, 10 }, { 0x15, 1, 10 }, { 0xb, 1, 10 }, { 0x16, 1, 10 }, { 3, 1, 10 },
    { 0x11, 1, 10 }, { 0xf, 4, 10 }, { 0x12, 5, 10 }, { 8, 6, 10 }, { 0x14, 7, 10 },
    { 0x17, 8, 10 }, { 9, 10, 10 }, { 0x10, 11, 10 }, { 0x1d, 14, 10 }, { 0x23, 16, 10 },
    { 0xa, 18, 10 },
};
int flece_skill_progression_count = sizeof(flece_skill_progression) / sizeof(skill_progression); // 0x593738
skill_progression rosalind_skill_progression[14] = {    // 0x593740 (8-aligned: pad at 0x59373c)
    { 6, 1, 10 }, { 0x1c, 1, 10 }, { 0x1e, 1, 10 }, { 0xb, 1, 10 }, { 0x13, 1, 10 },
    { 0x23, 5, 10 }, { 0x1d, 6, 10 }, { 0x20, 7, 10 }, { 0x1f, 8, 10 }, { 7, 9, 10 },
    { 0x21, 10, 10 }, { 0x10, 12, 10 }, { 9, 14, 10 }, { 0xc, 16, 10 },
};
int rosalind_skill_progression_count = sizeof(rosalind_skill_progression) / sizeof(skill_progression); // 0x5937e8

// ---- .bss ------------------------------------------------------------------------------------------
// Written in initialiser order, which is definition order. The addresses are not in that order
// (player_list_other 0x5fc030 < ccmd_max_skills 0x5fc1b8 < player_free_list 0x5fc1c8 < party_list
// 0x5fc350 < Players 0x5fc4d8): the same open .bss question as frametime.cpp.
//
// ONE initialiser constructs party_list (84 GameTimers at +0x28 = 0x5fc378) and then player_free_list
// (0x5fc1f0), so both are written as one declaration (one dynamic initialiser per declaration statement
// is assumed; no atexit, Player has no destructor).
// 0x433340 init_gametimer_arrays_5fc378_thunk (jmp). Confidence: exact.
// 0x433350 init_gametimer_arrays_5fc378. Confidence: likely (the one declaration).
Player party_list, player_free_list;  // 0x5fc350, 0x5fc1c8
// 0x433390 init_gametimer_array_5fc058_thunk (jmp). Confidence: exact.
// 0x4333a0 init_gametimer_array_5fc058. Confidence: exact.
Player player_list_other;             // 0x5fc030
// An explicit loop over the five records with the inlined implicit constructor, which builds its member
// array with `vector constructor iterator`.
// 0x4333c0 init_gametimer_arrays_5fc500_thunk (jmp). Confidence: exact.
// 0x4333d0 init_gametimer_arrays_5fc500. Confidence: likely (compiler-generated; label ours).
Player Players[MAX_PLAYERS];          // 0x5fc4d8

// The bool console command: the dynamic initialiser of the object. "%s is %s\n", "TRUE", "FALSE" and the
// usage text are pooled with an earlier unit (0x588c34..), so /Gf.
// 0x433400 init_ccmd_max_skills_thunk (jmp, static_initializers entry 109). Confidence: exact.
// 0x433410 init_ccmd_max_skills (console_command constructor). Confidence: exact.
console_command ccmd_max_skills("max_skills", "Toggles Max_skills", cmd_max_skills); // 0x5fc1b8

Player *party_leader;                 // 0x5fcc80
Player *summon_player;                // 0x5fcc84
Character *cur_character;             // 0x5fcc88
Character *cur_summon;                // 0x5fcc8c
bool party_lists_initialized;         // 0x5fcc90 set by party_lists_init, never read (label ours)
bool Max_skills;                      // 0x5fcc91
// Tagged gameplay.cpp (its users), but it sits in this unit's .bss before ccmd_slew (0x5fcca8, next
// unit) and player_input_reset is a Player member here, so it is defined here.
unsigned int player_input[3];         // 0x5fcc98

// 0x433430  Confidence: exact. Expansion of the console's bool command macro.
void cmd_max_skills()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Max_skills = true;
        else if (arg_type & ARG_FALSE)
            Max_skills = false;
        else if (arg_type & ARG_NONE)
            Max_skills ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "max_skills", "Max_skills");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "max_skills", Max_skills ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x4334f0  Confidence: exact (mp_owner is cleared before the input_reset call, the other fields
// after it; the source order of the field stores is read from the code).
Player *player_alloc_slot()
{
    Player *p = player_free_list.next;

    if (p == &player_free_list)
        VSDK_ERROR(PLAYER_CPP, 421, "No more player slots availeble -- something went horribly wrong -- find allender");

    list_remove(p);
    p->mp_owner = NULL;
    p->input_reset();
    p->handle = -1;
    p->flags = 0;
    p->num_progression = 0;
    p->skill_points = 0;
    p->skill_cap = 0;
    p->progression = NULL;
    p->lent_max_hp = 0.0f;
    list_append(&party_list, p);
    return p;
}

// 0x433580  Confidence: exact.
void player_clear_handle_of(int handle)
{
    Player *p = player_from_handle(handle);

    if (p != NULL)
        p->handle = -1;
}

// 0x4335a0  Confidence: exact. A new leader is picked whether or not p led: in single player the
// first living other member (then stop), in multiplayer every other member owned by this machine
// (no break, so the last one wins). The deleting call is the one virtual of the object, not a
// destructor (no flag is pushed). `delete_obj == true` because the test is CMP byte, 1.
void party_remove_player(Player *p, bool delete_obj)
{
    Player *q;
    Character *c;
    obj *o;

    list_remove(p);
    list_append(&player_free_list, p);

    for (q = party_list.next; q != &party_list; q = q->next) {
        if (q == p)
            continue;
        c = living_from_handle(q->handle);
        if ((game_flags & GF_SINGLE_PLAYER) && !(c->flags & CF_DEAD)) {
            party_set_leader(q);
            break;
        }
        if ((game_flags & GF_MULTIPLAYER) && q->mp_owner == mp_local_player)
            party_set_leader(q);
    }

    if (p->handle != -1) {
        o = living_from_handle(p->handle);
        if (o != NULL)
            ai_group_remove_member((Character *)o);
        else
            o = obj_from_handle(p->handle);
        if (delete_obj == true) {
            o->deinit();
            if (p == summon_player)
                cur_summon = NULL;
        }
    }
    p->handle = -1;
    if (p == summon_player)
        summon_player = NULL;
}

// 0x4336b0  Confidence: exact (the "next" pointer is read before the end test, as a safe-iterate
// macro would; the mp loop leaves 0 in ECX, which is then reused for party_leader).
void party_remove_all()
{
    Player *p, *next;
    mp_player *mp;

    for (p = party_list.next, next = p->next; p != &party_list; p = next, next = p->next)
        party_remove_player(p, true);
    for (p = player_list_other.next, next = p->next; p != &player_list_other; p = next, next = p->next)
        party_remove_player(p, true);

    if (game_flags & GF_MULTIPLAYER) {
        for (mp = mp_player_list.next; mp != &mp_player_list; mp = mp->next)
            mp->object = NULL;
    }
    party_leader = NULL;
}

// 0x433730  Confidence: exact. thiscall although `this` is not used (both callers load ECX).
void Player::input_reset()
{
    player_input[0] = 0;
    player_input[1] = 0;
    player_input[2] = 0;
}

// 0x433750  Confidence: exact.
void Player::set_ai_control(bool on)
{
    Character *c;

    if (handle == -1)
        return;
    c = living_from_handle(handle);
    if (on)
        c->ai.flags &= ~AI_FLAG_PLAYER_CONTROL;
    else
        c->ai.flags |= AI_FLAG_PLAYER_CONTROL;
}

// 0x433790  Confidence: exact. Species 0..3 through a jump table (0x433950). With Max_skills every
// progression skill is set to 100 and its level rewritten to 1. The two `leader == true` tests are
// CMP byte, 1; the cur_character one is TEST AL, AL.
Character *Player::create(const vector3 *pos, const matrix *orient, const char *name, bool leader)
{
    Character *c;
    int i;

    c = living_entity_create(7, char_info, pos, orient, 1, 0, 0, 0, 0);
    if (c == NULL)
        return NULL;
    handle = c->handle;
    if (leader)
        cur_character = c;

    switch (char_infos[char_info].species) {
    case SPECIES_JOSEPH:
        progression = joseph_skill_progression;
        num_progression = joseph_skill_progression_count;
        break;
    case SPECIES_JEKHAR:
        progression = jekhar_skill_progression;
        num_progression = jekhar_skill_progression_count;
        break;
    case SPECIES_FLECE:
        progression = flece_skill_progression;
        num_progression = flece_skill_progression_count;
        break;
    case SPECIES_ROSALIND:
        progression = rosalind_skill_progression;
        num_progression = rosalind_skill_progression_count;
        break;
    default:
        progression = NULL;
        num_progression = 0;
        break;
    }

    for (i = 0; i < MAX_SKILLS; i++) {
        if (i > 2)
            c->skill[i] = 0;
    }
    for (i = 0; i < num_progression; i++) {
        if (!Max_skills) {
            if (c->level >= progression[i].level)
                c->skill[progression[i].skill] = (ubyte)progression[i].value;
        } else {
            c->skill[progression[i].skill] = 100;
            progression[i].level = 1;
        }
    }
    default_chain = 0;
    for (i = 0; i < MAX_SKILLS; i++)
        skill_cooldown[i].SetTimeout(0);

    c->set_name(name);
    if (!(game_flags & GF_MULTIPLAYER) && leader != true)
        set_ai_control(true);
    else
        set_ai_control(false);
    c->ai.reset();
    c->set_team(0);
    // Nested, with ai_group_create in both elses (D12400): the binary pushes c before the leader test.
    if (game_flags & GF_SINGLE_PLAYER) {
        if (leader != true)
            ai_group_add_member(cur_character->ai.group, c);
        else
            ai_group_create(c);
    } else
        ai_group_create(c);
    return c;
}

// 0x433960  Confidence: exact.
bool Player::make_leader()
{
    if (flags & PF_LEADER)
        return false;
    if (obj_from_handle(handle) == NULL)
        return false;
    flags = (flags & ~PF_HOLD) | PF_LEADER;
    set_ai_control(false);
    return true;
}

// 0x4339a0  Confidence: exact (two set_ai_control call sites: VC6 tail duplication or two source
// branches each ending in the call).
bool Player::drop_leader()
{
    if (flags & PF_LEADER) {
        flags &= ~PF_LEADER;
        if (game_flags & GF_SOLO_MODE)
            flags |= PF_HOLD;
        else
            flags &= ~PF_HOLD;
        set_ai_control(true);
        return true;
    }
    return false;
}

// 0x4339e0  Confidence: exact. The if-form bool (D12400) gives the dword load and SHR AL, 3 / AND AL, 1.
bool Player::is_leader()
{
    if (flags & PF_LEADER)
        return true;
    return false;
}

// 0x4339f0  Confidence: exact. Skill 0x34 and the summon skills are Joseph's only. skill_is_summon
// is called a second time in the ring check although it is known false there, so the source tested
// it in both conditions. Levels: 0x18, 0x11, 0x10 are level_script_table indices.
bool Player::skill_unlocked(int skill)
{
    int i;

    if (skill >= 0 && skill <= 2)
        return true;
    if (skill != 0x34 && !skill_is_summon(skill)) {
        if ((!All_summons && (skill == 0x1b || skill_is_summon(skill)) && (game_flags & GF_SINGLE_PLAYER)
             && (!savegame_state.get_flag(0x18, "got_ring_of_darkness")
                 || (savegame_state.get_flag(0x11, "act2_done") && !savegame_state.get_flag(0x10, "got_ring_of_water"))))
            || (!Max_skills && (game_flags & GF_SINGLE_PLAYER) && (skill == 0x16 || skill == 0x17)
                && !savegame_state.get_flag(0x18, "lpalaceint_yago_found")))
            return false;

        for (i = 0; i < num_progression; i++) {
            if (skill == progression[i].skill)
                return true;
        }
        return false;
    }
    return char_infos[char_info].species == SPECIES_JOSEPH;
}

// 0x433b10  Confidence: exact (MOV EAX, 1: int, not bool).
int Player::skill_unlocked_at_level(int skill)
{
    Character *c = living_from_handle(handle);
    int i;

    if (c != NULL) {
        if (skill == 2)
            return 1;
        for (i = 0; i < num_progression; i++) {
            if (skill == progression[i].skill && c->level >= progression[i].level)
                return 1;
        }
    }
    return 0;
}

// 0x433b70  Confidence: exact. Unlike Player::create the progression counts are constants here
// (0x11, 0xd, 0x10, 0xe), so the array sizes were used, not the count variables.
void Player::init_from_character(Character *c)
{
    int i;

    char_info = c->info - char_infos;
    handle = c->handle;
    switch (c->info->species) {
    case SPECIES_JOSEPH:
        progression = joseph_skill_progression;
        num_progression = sizeof(joseph_skill_progression) / sizeof(skill_progression);
        break;
    case SPECIES_JEKHAR:
        progression = jekhar_skill_progression;
        num_progression = sizeof(jekhar_skill_progression) / sizeof(skill_progression);
        break;
    case SPECIES_FLECE:
        progression = flece_skill_progression;
        num_progression = sizeof(flece_skill_progression) / sizeof(skill_progression);
        break;
    case SPECIES_ROSALIND:
        progression = rosalind_skill_progression;
        num_progression = sizeof(rosalind_skill_progression) / sizeof(skill_progression);
        break;
    default:
        progression = NULL;
        num_progression = 0;
        break;
    }
    for (i = 0; i < MAX_SKILLS; i++)
        skill_cooldown[i].SetTimeout(0);
    lent_max_hp = 0.0f;
    skill_points = 0;
    skill_cap = (int)xp_level_skill_cap(c->level);
    set_ai_control(true);
    c->ai.reset();
    c->set_team(0);
    if (c != cur_character && cur_character != NULL && cur_character->ai.group != NULL)
        ai_group_add_member(cur_character->ai.group, c);
}

static void party_lists_atexit_cleanup();

// 0x433ca0  Confidence: exact (party_list.next / player_free_list.next stay in registers through
// the loop; the five records are inserted at the head, so the free list runs Players[4]..[0]).
void party_lists_init()
{
    int i;

    list_init(&party_list);
    list_init(&player_free_list);
    for (i = 0; i < MAX_PLAYERS; i++) {
        Players[i].next = player_free_list.next;
        player_free_list.next->prev = &Players[i];
        player_free_list.next = &Players[i];
        Players[i].prev = &player_free_list;
    }
    list_init(&player_list_other);
    party_lists_initialized = true;
    party_leader = NULL;
    cur_character = NULL;
    atexit(party_lists_atexit_cleanup);
}

// 0x433d10  Confidence: exact. Same loops as the start of party_remove_all.
static void party_lists_atexit_cleanup()
{
    Player *p, *next;

    for (p = party_list.next, next = p->next; p != &party_list; p = next, next = p->next)
        party_remove_player(p, true);
    for (p = player_list_other.next, next = p->next; p != &player_list_other; p = next, next = p->next)
        party_remove_player(p, true);
}

// 0x433d60  Confidence: exact (when p already leads, ECX is the reloaded party_leader, the same
// record; hud_target_mode is stored between the push and the camera call).
void party_set_leader(Player *p)
{
    Character *c;
    Player *q;

    if (obj_from_handle(p->handle) == NULL)
        return;
    if (p == party_leader) {
        party_leader->make_leader();
        return;
    }
    c = living_from_handle(p->handle);
    if (c->ai.flags & AI_FLAG_PLAYER_CONTROL)
        return;
    if (!p->is_leader() && party_leader != NULL)
        party_leader->drop_leader();

    party_leader = p;
    cur_character = c;
    p->make_leader();
    interface_refresh_cur_screen();
    for (q = party_list.next; q != &party_list; q = q->next)
        q->set_ai_control(q != party_leader);
    if (!(cur_character->flags & CF_DEAD))
        cur_character->ai.group->leader = cur_character->handle;

    c->ai.goals_remove_type1();
    if (!c->ai.is_disabled_any() && !c->ai.is_casting())
        c->ai.go_idle();
    party_leader->input_reset();
    ui_clear_pick_overrides();
    world_pick_clear_filter();
    hud_target_mode = -1;
    game_camera.set_target(cur_character);
    levelrender_clear_draw_lists();
    if (game_flags & GF_LEADER_SOUND)
        game_play_sound(0x28, 1.0f, 0.0f, 0, 0);
}

// 0x433eb0  Confidence: exact.
Player *player_from_handle(int handle)
{
    Player *p;

    for (p = party_list.next; p != &party_list; p = p->next) {
        if (p->handle == handle)
            return p;
    }
    for (p = player_list_other.next; p != &player_list_other; p = p->next) {
        if (p->handle == handle)
            return p;
    }
    return NULL;
}

// 0x433ef0  Confidence: exact (CMP byte, 1 for the Max_skills test).
void player_start_skill_cooldown(Character *c, int skill)
{
    Player *p;

    if (Max_skills == true)
        return;
    p = player_from_handle(c->handle);
    if (p != NULL)
        p->skill_cooldown[skill].SetTimeout(skill_table[skill].recovery * 1000);
}

// 0x433f40  Confidence: exact. Unlinks the character from the active object and living lists by
// hand (the obj_link_new pair is not called). The first member that is neither dead nor gone becomes
// leader, whoever led before.
bool party_remove_member(const char *name)
{
    char buf[256];
    Player *p;
    Character *c;
    int species;

    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (!_stricmp(c->get_name(), name))
            break;
    }
    if (p == &party_list)
        return false;

    list_remove(p);
    list_append(&player_list_other, p);
    ai_group_remove_member(c);
    list_remove(c);
    do {
        c->live_prev->live_next = c->live_next;
        c->live_next->live_prev = c->live_prev;
        c->live_next = NULL;
        c->live_prev = NULL;
    } while (0);
    c->flags |= CF_LEFT_PARTY;

    species = c->info->species;
    if (species >= SPECIES_JOSEPH && species <= SPECIES_ROSALIND) {
        char *fmt = XSTR("%s has left the party!\n", 934);
        sprintf(buf, fmt, c->get_name());
        hud_show_message(buf);
    }

    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (!(c->flags & CF_DEAD) && !(c->flags & CF_LEFT_PARTY)) {
            party_set_leader(p);
            break;
        }
    }
    return true;
}

// 0x4340d0  Confidence: exact (the HP store goes through an int temporary: FILD of the
// get_max_stat result). Relinks the character into obj_active_list and, unless dead, into living_list (prev
// first, as list_append; D7108).
Player *party_add_member(const char *name)
{
    char buf[256];
    Player *p;
    Character *c;
    int species;

    for (p = player_list_other.next; p != &player_list_other; p = p->next) {
        c = living_from_handle(p->handle);
        if (!_stricmp(c->get_name(), name))
            break;
    }
    if (p == &player_list_other)
        return NULL;

    list_remove(p);
    list_append(&party_list, p);
    ai_group_add_member(cur_character->ai.group, c);

    c->prev = obj_active_list.prev;
    c->next = &obj_active_list;
    obj_active_list.prev->next = c;
    obj_active_list.prev = c;

    c->flags &= ~(CF_LEFT_PARTY | CF_HIDDEN | CF_SUMMONLIKE);
    if (c->flags & CF_DEAD) {
        c->resurrect();
    } else {
        c->live_prev = living_list.live_prev;
        c->live_next = &living_list;
        living_list.live_prev->live_next = c;
        living_list.live_prev = c;
    }
    c->hp = (float)c->get_max_stat(0);
    c->ap = (short)c->get_max_stat(1);
    c->clear_all_effects();
    p->set_ai_control(true);
    party_sort_by_species();

    species = c->info->species;
    if (species >= SPECIES_JOSEPH && species <= SPECIES_ROSALIND) {
        char *fmt = XSTR("%s has joined the party!\n", 622);
        sprintf(buf, fmt, c->get_name());
        hud_show_message(buf);
    }
    return p;
}

// 0x434270  Confidence: exact (switch through the jump table 0x434338; the zeroed array may be
// five assignments or an initialiser list).
void party_sort_by_species()
{
    Player *sorted[MAX_PLAYERS] = { NULL, NULL, NULL, NULL, NULL };
    Player *p;
    Character *c;
    int i;

    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (c == NULL)
            continue;
        // The order is Joseph, Flece, Rosalind, Jekhar (jump table 0x434338: species 1 -> slot 3, 2 -> 1, 3 -> 2;
        // found by the strict relocation check, D9100).
        switch (c->info->species) {
        case SPECIES_JOSEPH:   i = 0; break;
        case SPECIES_FLECE:    i = 1; break;
        case SPECIES_ROSALIND: i = 2; break;
        case SPECIES_JEKHAR:   i = 3; break;
        default:               i = 4; break;
        }
        sorted[i] = p;
    }
    for (i = 0; i < MAX_PLAYERS; i++) {
        if (sorted[i] != NULL) {
            list_remove(sorted[i]);
            list_append(&party_list, sorted[i]);
        }
    }
}

// 0x434350  Confidence: exact.
int party_count_active()
{
    Player *p;
    Character *c;
    int count = 0;

    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (c != NULL && (cur_level_index == 4 || !(c->flags & CF_HIDDEN)) && !(c->flags & CF_LEFT_PARTY))
            count++;
    }
    return count;
}
