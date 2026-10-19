// living_entity.cpp - Sum.exe v1.40, code 0x44d9f0-0x46335f (Engine\objects\living_entity.cpp)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: sum.symbols tags 166 functions from 0x44d9f0 (sinit_living_entity_8eca60) to 0x463480
// (cmd_death) with living_entity.cpp; items.cpp (optimised) ends at 0x44d9ee. The last unit function is
// cmd_Spell_ap (0x4632a0): the bloodspots and death commands (0x463360-0x46353f) open combat.cpp's
// object, which is where they are written now (D6100).
//
// Build (D6100): one object compiled with /O2, `#pragma optimize("", off)` from the top and
// `#pragma optimize("", on)` again from character_say (0x461dc0) on, as in the cutscene file (D3201).
// The unoptimised part has ebp frames, 0xcc padding and reloads every operand (docs/sum-exe/source-map.md
// "Version 2"), like objects.cpp; the optimised tail has no frame pointer and 0x90 padding. /O2 keeps
// /Oi (inline strcpy / strlen) and string pooling on in the unoptimised part. Inline functions WERE expanded
// (as in objects.cpp, D66): vector3::operator= (field moves through a pointer local, then the returned
// copy), operator+= / -, len_sq, matrix::operator= (three row copies) and lobj_ray's constructor
// appear as moves, while calls nested inside an expansion stay out of line (the vector3 copy
// constructor 0x410cd0, operator= 0x413190 inside matrix::operator=; the vector3 constructor 0x43b580 and the
// COMDATs vector3::operator+ 0x45b680 / operator- 0x45b6b0, emitted in this unit's range, inside
// character_clip_move_to_navmesh). The source below is written with the inline operators; their
// temporaries are not declared.
// XSTR: the unoptimised code tests the constant 0 of Volition's `(0 ? text : localize_strings[id])`
// before every lookup (`xor ecx, ecx; test ecx, ecx; je`, status_effect_add 0x454c79). localize.h's
// XSTR is an lvalue the pilots take the address of, so this unit redefines it (D1701).
// Local variables: VC6 12.00.8168 lays out /Od locals by a hash of their names (DECISIONS.md D6101): the
// names and declaration order are chosen so that it reproduces the frame offsets the notes give.

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "engine/objects/living_entity.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/ai/ai.h"
#include "engine/combat/combat.h"
#include "engine/spells/spells.h"
#include "engine/effects/fx.h"
#include "engine/camera/camera.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "engine/ai/pathfinding.h"
#include "engine/ai/boss_ai.h"
#include "engine/gamesound/gamesound.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/profile.h"
#include "vsdk/os/console.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/geom/vfx.h"
#include "vsdk/geom/effect_mem.h"
#include "engine/objects/item_paging.h"
#include "vsdk/localize/localize.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/gr/gr.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/snd/snd.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/interface.h"
#include "summoner/interface/new_hud.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

#undef XSTR
#define XSTR(text, id) (0 ? (char *)(text) : localize_strings[id])

// D7800: in the unoptimised code a clamp of a variable into itself (`x = max(x, 0)`) is two direct stores
// in Sum.exe, one per branch; cl 12.00.8966 builds a ?: temporary and copies it. Written as if / else,
// which gives the binary's code. A reproduction device (as D6101's renames): the original source may well
// have used min / max. Other conditionals (`f = c ? 1 : 0`, `(short)min(v, 0x7fff)`, XSTR) do go through
// a temporary in the binary and stay as they are.
#define ASSIGN_MAX(x, a, b) if ((a) > (b)) (x) = (a); else (x) = (b)
#define ASSIGN_MIN(x, a, b) if ((a) < (b)) (x) = (a); else (x) = (b)

// D7802: the unoptimised calls of character_find_mesh_tag on instance->character store the model pointer
// (and a non-constant name) in compiler temporaries before the push: the code of an inline wrapper
// expanded at /Od (a simple local such as update_head_look's ch is used directly, as the binary does).
// The wrapper is the original's in some form (ca_character member or free function); name ours.
inline int mesh_tag(ca_character *ch, const char *name)
{
    return ch->find_mesh_tag(name);
}

// Original __FILE__ of this unit (literal 0x599b50, used by character_update_layer and
// living_entity_create; "Objects" with a capital O as in objects.cpp's).
#define LIVING_ENTITY_CPP "D:\\projects\\Summoner\\pccode\\Engine\\Objects\\living_entity.cpp"

// Everything up to living_entity_create, the static initialisers of the living lists and the two
// unoptimised console commands included, is compiled without optimisation (D6100).
#ifdef _MSC_VER
#pragma optimize("", off)
#endif

// ---- .rdata --------------------------------------------------------------------------------------
// team_enemy_masks first (0x581904-0x581913, right after characterinfo.cpp's float constants; read by
// character_set_team). At 0x581914 living_entity__vtable (one slot, Character::deinit 0x44edf0), then this unit's float
// constants 0x581918-0x58197c (1.8, 0.98, pi/2, -pi/2, 2^31, 400, 0.0001, 999999, 0.001, 0.0005,
// -0.01, 0.55, 0.9, -8, -10, -20, -7, 20, 0.04, 0.15, 0.77, 0.3, 20, 0.025, -999999). The other
// constants are shared __real@ constants of earlier units (0.5 0x57e6b0, 100 0x57e6b4, 1.0 0x57e990,
// 10 0x57eb7c, 0 0x57eb84, ...).
const unsigned int team_enemy_masks[4] = { 0xc, 0, 0xb, 7 };   // 0x581904

// ---- .data (0x599644-0x5996b3, the literals follow from 0x5996b4) --------------------------------
int Character_los_enabled = 1;        // 0x599644 (label ours; only character_has_line_of_sight_to_point reads it)
char *locomotion_anim_names[NUM_LOCO_ANIMS] = {    // 0x599648
    "stand", "norm_walk", "norm_run", "ready", "walk", "run", "stand_hurt", "norm_walk_hurt",
    "norm_run_hurt", "ready_hurt", "walk_hurt", "run_hurt", "sneak_ready", "hide_ready",
    "tripped_liedown", "cloaked_ready", "cloaked_sneak", "statscreen",
};
float Chance_to_drop_gold = 0.25f;    // 0x599690 "chance_gold"
bool Use_spell_ap = true;             // 0x599694 "Spell_ap"
int character_speech_times[3] = { 250, 312, 400 };   // 0x599698 (label ours; character_say)
int hero_model_prefix_table[4] = { 6, 6, 2, 2 };     // 0x5996a4 (label ours; character_hero_model_prefix)

// ---- .bss (0x8eca30-0x94173c, interleaved with combat.cpp's; see the console commands) ----------
// The compiler-generated static initialisers of the unit's objects (frame-pointer wrappers, as in
// objects.cpp), one comment line per function:
// 0x44d9f0 sinit_living_entity_8eca60: call init, call register. Confidence: exact.
// 0x44da00 init_living_entity_8eca60: the constructor on 0x8eca60. Confidence: exact.
// 0x44da10 register_dtor_living_entity_8eca60: atexit(0x44da30). Confidence: exact.
// 0x44da30 dtor_living_entity_8eca60: the destructor. Confidence: exact.
Character living_list;                // 0x8eca60
// 0x44da40 sinit_living_entity_8ed1e8. Confidence: exact.
// 0x44da50 init_living_entity_8ed1e8. Confidence: exact.
// 0x44da60 register_dtor_living_entity_8ed1e8: atexit(0x44da80). Confidence: exact.
// 0x44da80 dtor_living_entity_8ed1e8. Confidence: exact.
Character living_list_inactive;       // 0x8ed1e8
// 0x44db50 sinit_living_entity_pool_8ed980. Confidence: exact.
// 0x44db60 init_living_entity_pool_8ed980: eh vector constructor iterator (0xaf x 0x788, constructor,
//          destructor). Confidence: exact.
// 0x44db90 register_dtor_living_entity_pool_8ed980: atexit(0x44dbb0). Confidence: exact.
// 0x44dbb0 dtor_living_entity_pool_8ed980: eh vector destructor iterator. Confidence: exact.
Character living_entity_pool[MAX_LIVING_ENTITIES];   // 0x8ed980
// 0x44da90 init_gametimer_array_93ff80_thunk: call init. Confidence: exact.
// 0x44daa0 init_gametimer_array_93ff80: the compiler's construction loop for the GameTimer at +8 of
//          each record (count 100, stride 0xc; no destructor, so no atexit). Confidence: exact.
spell_cooldown spell_cooldowns[MAX_SPELL_COOLDOWNS];  // 0x93ff78
character_speech_record character_speech[MAX_CHARACTER_SPEECH];   // 0x940438
layer_lobj_list *level_layer_ground[20];   // 0x9414b8 (filled by level_load_layers)
// 0x44daf0 init_gametimer_array_941514_thunk. Confidence: exact.
// 0x44db00 init_gametimer_array_941514: count 0x6c, stride 4. Confidence: exact.
GameTimer spell_global_timers[MAX_SPELLS];             // 0x941514
move_fade move_fades[MAX_MOVE_FADES];  // 0x9416c8
bool Players_immortal;                // 0x941704
bool No_resist;                       // 0x941705
int move_play_serial;                 // 0x941708
// Bloodspots (0x94173c) and Debug_death (.data 0x59a16c) are combat.cpp's (D6100).

// 0x44dbd0  Confidence: exact. i at ebp-4.
void spell_cooldown_start(int spell, Character *c, int ms)
{
    int i;

    for (i = 0; i < MAX_SPELL_COOLDOWNS; i++) {
        if (spell_cooldowns[i].handle == -1)
            break;
    }
    if (i == MAX_SPELL_COOLDOWNS)
        return;
    spell_cooldowns[i].handle = c->handle;
    spell_cooldowns[i].spell = spell;
    spell_cooldowns[i].timer.SetTimeout(ms * 1000);
    spell_global_timers[spell].SetTimeout(ms * 1000);
}

// 0x44dc70  Confidence: exact (the summon-type test is `if (type == 5) { switch ... }` with the
// default case storing into the argument; the three spells share the cooldown of spell 0x1c).
// this at ebp-4, the switch value at ebp-8.
void Character::start_spell_recovery(int spell, int extra, int ms)
{
    if (type == 5) {
        switch (spell) {
        case 0x11:
        case 0x1c:
        case 0x1e:
            spell_cooldown_start(0x1c, this, spells[0x1c].ai_recovery_time);
            spell_cooldown_start(0x1e, this, spells[0x1e].ai_recovery_time);
            spell_cooldown_start(0x11, this, spells[0x11].ai_recovery_time);
            return;
        default:
            ms = spells[spell].ai_recovery_time;
            break;
        }
    }
    if (ms < 0)
        return;
    if (type == 7 && Max_skills == 1)
        return;
    spell_cooldown_start(spell, this, extra + ms);
}

// 0x44dd30  Confidence: exact. i at ebp-4, this at ebp-8.
int Character::spell_on_cooldown(int spell)
{
    int i;

    for (i = 0; i < MAX_SPELL_COOLDOWNS; i++) {
        if (spell_cooldowns[i].handle == handle && spell_cooldowns[i].spell == spell) {
            if (spell_cooldowns[i].timer.elapsed())
                return 0;
            return 1;
        }
    }
    return 0;
}

// Rounding to a step. An inline function of the original (name ours): the step is multiplied by 0.5 at
// run time (an inline argument is not folded with the constant; D7801), before x is computed.
inline float round_to_step(float x, float step)
{
    return (float)(int)((x + step * 0.5f) / step) * step;
}

// 0x44ddb0  Confidence: likely (the min / max ?: temporaries at ebp-0x18 / -0x1c; the skill copy is a
// struct assignment of 0x54 bytes, rep movsd 0x15). Frame: ap_mult -4, v -8, hp_mult -0xc, i -0x10,
// sk -0x14, this -0x20.
void Character::scale_stats_to_level()
{
    float ap_mult;
    float hp_mult;
    int v;
    int i;
    float sk;

    if (type == 7 && summon_skill_from_creature_name(info->name) == -1) {
        hp_mult = 1.0f;
        ap_mult = 1.0f;
    } else {
        hp_mult = (float)level / (float)info->level;
        if (hp_mult < 1.0f)
            ap_mult = hp_mult;
        else
            ap_mult = (float)sqrt(hp_mult) * hp_mult;
    }
    base_max_hp = round_to_step(info->max_hp * hp_mult, 1.0f);
    hp = round_to_step(info->hp * hp_mult, 1.0f);
    v = (int)(info->max_ap * ap_mult + 0.5f);
    base_max_ap = (short)min(v, 0x7fff);
    v = (int)(info->ap * ap_mult + 0.5f);
    ap = (short)min(v, 0x7fff);
    if ((game_flags & GF_MULTIPLAYER) && type != 7) {
        base_max_hp = mp_netgame_num_players * base_max_hp;
        hp = base_max_hp;
    }
    *(skill_levels *)skill = *(skill_levels *)info->skill;
    for (i = 0; i < MAX_SKILLS; i++) {
        if (i == SKILL_SPEED)
            continue;
        if (i == SKILL_PROTECTION)
            continue;
        sk = skill[i] * hp_mult;
        ASSIGN_MAX(sk, 0.0f, sk);
        ASSIGN_MIN(sk, 100.0f, sk);
        skill[i] = (ubyte)(int)(sk + 0.5f);
    }
}

// 0x44e060  Confidence: likely. The members are built in declaration order: the 18 status slots (the
// compiler's loop over the two GameTimers at +4 / +0x14 of each, count at ebp-0x18, stride 0x20),
// timer2ec, ai (state 1 of the EH frame: ai_state has a destructor), combat, timer6d4,
// net_update_timer; then the vtable store and the body. combat is still the D1 union with `protection`
// (new_assess.cpp), so its constructor is called in the body here instead of between ai and timer6d4
// (D1702).
Character::Character()
{
    combat.construct();
    type = 0;
}

// 0x44e150  Confidence: exact (vtable store, EH state 0, the body, ~ai_state = empty_function
// at 0x4f13e0, obj::~obj).
Character::~Character()
{
    type = 0;
}

// 0x44e1c0 living_entity_init  Confidence: likely. Frame: summon -4, species -8, style -0xc, centre
// -0x18, weapon_type -0x1c, i -0x20, t -0x24 / r -0x28, j -0x2c, the equip outputs -0x30 / -0x34, slot
// -0x38, item -0x3c, held -0x40, light orient -0x64, bone -0x68, light pos -0x74, the ?: temporary
// -0x78, this -0xe0; the rest are inline-expansion temporaries. The fifth argument (a bool, D6105) is not read.
void Character::init(int type, int info_index, const vector3 *pos, const matrix *orient, bool unused,
                     int level)
{
    int summon;
    int species;
    int style;
    vector3 centre;
    int weapon_type;
    int i;
    float t;
    float r;
    int j;
    item_info *old_b;
    item_info *old_a;
    int slot;
    item_info *item;
    item_info *held;
    matrix light_orient;
    int bone;
    vector3 light_pos;

    live_list_append(&living_list, this);
    link_new(type);
    ai.init();
    light = -1;
    hit_by_party = 0;
    light_level = -1.0f;
    target_speed = 0.0f;
    cutscene_object = -1;
    script_object = -1;
    _unk710 = -1;
    ai.owner = this;
    this->pos = *pos;
    this->orient = *orient;
    this->info = characterinfo_find_multi_slot(info_index);
    if (type == 7 && summon_skill_from_creature_name(this->info->name) != -1)
        summon = 1;
    else
        summon = 0;
    instance = charinfo_create_character_instance(this->info, summon);
    if (this->info->species >= SPECIES_JOSEPH && this->info->species <= SPECIES_ROSALIND && type == 7)
        apply_hero_model(0);
    weapon_type = 0;
    if (this->info->default_weapons[0] != -1)
        weapon_type = item_types[this->info->default_weapons[0]].weapon_class;
    characterinfo_load_party_anims(this->info, weapon_type, 1, summon);
    equipment_clear(equipment);
    if (this->info->alt_parent != NULL) {
        for (j = 0; j < this->info->alt_textures->count; j++)
            this->info->alt_textures->bitmaps[j] = bm_load(this->info->alt_textures->names[j], -1, -1);
    }
    flags = 0;
    loco_blend = 0.0f;
    frametime = 0.0f;
    slowdown_rate = 0.0f;
    ground_lobj = -1;
    ground_face = -1;
    layer = 0;
    cur_move = NULL;
    cur_anim = -1;
    stance_blend = 1.0f;
    equipment_clear(equipment);
    move_sound = 0;
    name[0] = 0;
    _vec6c0.zero();
    mount_link = -1;
    mount_flags = 0;
    if (type == 7)
        timer2ec.SetTimeout(7500);
    else
        timer2ec.invalidate();
    net_update_timer.invalidate();
    instance->get_bounding_sphere(&centre, &bound_radius);
    fill_locomotion_anims();
    living_entity_setup_head(this->info, instance);
    prop_tag = this->info->prop_tag;
    default_prop_tag = prop_tag;
    // (cmp [+0x5ac], [+0x5b8]; je: set when equal, D9201 found by the hybrid game)
    if (anim_stand == anim_ready || (game_flags & GF_MULTIPLAYER)) {
        flags |= CF_COMBAT_READY;
        prop_tag = 0;
    }
    combat.set_owner(this);
    slowdown_rate = this->info->slowdown_rate;
    if (level <= 0)
        level = 1;
    this->level = (short)level;
    xp = xp_for_level(level);
    scale_stats_to_level();
    set_team(this->info->team);
    clear_all_effects();
    for (i = 0; i < this->info->num_default_weapons; i++) {
        item = item_create(this->info->default_weapons[i]);
        if (item == NULL)
            continue;
        if (item->type->kind == 1)
            slot = 6;
        else if (i == 0)
            slot = 5;
        else
            slot = i + 8;
        character_equip_item(&old_a, &old_b, this, slot, item);
        if (old_a != NULL)
            item_free(old_a);
        if (old_b != NULL)
            item_free(old_b);
    }
    num_attachments = 0;
    if (this->info->species == 0xce) {
        held = equipment[9];
        item_page_in(held->type - item_types, 9, 0);
        attachments[0].item_type = held->type - item_types;
        attachments[0].slot = 4;
        num_attachments = 1;
        held = character_unequip_slot(this, 9, 1);
        if (held != NULL)
            item_free(held);
    }
    update_locomotion_anim();
    speed_scale = run_speed;
    hp_text_timer = 0.0f;
    float_text_scale = 0.0f;
    float_text[0] = 0;
    pos_mod_text = 0.0f;
    pos_mod_text_timer = 0.0f;
    _unk690 = 0;
    move_fx = -1;
    obj_flags |= 1;
    forward_speed = 0.0f;
    velocity.zero();
    species = this->info->species;
    style = 0;
    if (species == SPECIES_JOSEPH)
        style = 0;
    else if (species == SPECIES_JEKHAR)
        style = 0;
    else if (species == SPECIES_ROSALIND)
        style = 5;
    else if (species == SPECIES_FLECE)
        style = 1;
    ai.set_18(style);
    t = 10.0f;
    r = frand() * t;
    instance->process(r, false, 1);
    if (this->info->has_light) {
        bone = mesh_tag(instance->character, this->info->light_pos);
        instance->get_bone_transform(&light_pos, &light_orient, bone);
        light_pos.unrotate(*orient);
        light_pos += this->pos;
        light = attached_light_create(&light_pos, 1.0f, 0, this->info->light_atten,
                                      this->info->light_color[0], this->info->light_color[1],
                                      this->info->light_color[2]);
    }
    if (this->info->vfx != NULL) {
        set_current_mempool(summon ? &summon_mempool : &level_mempool);
        fx_attach_to_character(this->info->vfx, handle, -1, -1, 1, 0);
        set_current_mempool(NULL);
    }
    find_foot_bones();
}

// 0x44ed90  Confidence: exact.
char *Character::get_name()
{
    return name;
}

// 0x44edb0  Confidence: exact (strcpy expanded as repne scasb / rep movs: VC6 /Od keeps strcpy
// intrinsic).
void Character::set_name(const char *name)
{
    strcpy(this->name, name);
}

// 0x44edf0 living_entity__vfunc0 (vtable slot 0)  Confidence: exact. p at ebp-4, c at ebp-8, this
// at ebp-0xc. The model is released when no other living entity uses the same ca_character.
void Character::deinit()
{
    Player *p;
    Character *c;

    ai.shutdown();
    equipment_free(equipment);
    equipment_clear(equipment);
    if (light > -1)
        attached_light_free(light);
    if ((flags & CF_SUMMON) && team == 0) {
        hp = 0.0f;
        p = player_from_handle(handle);
        if (p != NULL)
            party_remove_player(p, false);
    }
    if (this == cur_summon)
        cur_summon = NULL;
    if ((flags & 0x800) || ((flags & CF_SUMMON) && !(flags & 0x40000))) {
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (c != this && c->instance->character == instance->character)
                break;
        }
        if (c == &living_list) {
            charinfo_unload_anims(info);
            character_release(instance->character);
        }
    }
    if (type == 7) {
        player_clear_handle_of(handle);
        if (game_flags & GF_MULTIPLAYER)
            multi_charinfo_slot_release(info);
    }
    obj::deinit();
    if (live_next != NULL && live_prev != NULL)
        live_list_remove(this);
    type = 0;
}

// 0x44f020  Confidence: exact. Frame: centre -0xc, r -0x10, far -0x14, d -0x20, this -0x38;
// the rest inline temporaries. Type 0x79 (in a cutscene, game_flags 0x20) adds the centre offset;
// species 0x58 culls with a 1.5 x larger sphere.
bool Character::is_in_view(float radius_scale)
{
    vector3 centre;
    float far_dist;
    float r;

    instance->get_bounding_sphere(&centre, &r);
    centre.unrotate(orient);
    centre += pos;
    if ((game_flags & GF_CUTSCENE) && info->species == 0x79)
        centre += centre_offset;
    far_dist = char_fog_get_far();
    if (far_dist > 0.0f) {
        vector3 d = camera_view_pos - centre;
        if (d.len_sq() - r * r * radius_scale > far_dist * far_dist)
            return true;
    }
    if (info != NULL && info->species == 0x58)
        return gr_cull_sphere(&centre, 1.5f * r * radius_scale, 0);
    return gr_cull_sphere(&centre, r * radius_scale, 0);
}

// 0x44f1a0  Confidence: exact. t at ebp-4, this at ebp-0xa0 (the rest inline temporaries). Slots 0-3
// are the level's player starts, later ones the formation slot behind cur_character. After the store the
// start is read back through start_navpoint (+0x34), not the table (D6103).
void Character::place_at_party_start(int slot)
{
    float t;

    if (slot < 4) {
        start_navpoint = level_player_starts[slot];
        pos = ((navpoint *)start_navpoint)->pos;
        orient.set_from_angles(0.0f, 0.0f, ((navpoint *)start_navpoint)->heading);
        layer = ((navpoint *)start_navpoint)->layer;
    } else {
        ai.formation_slot_position(&pos, &cur_character->pos, &cur_character->orient.fvec, slot);
        orient = cur_character->orient;
    }
    pos.y += 1.0f;
    find_ground();
    last_pos = pos;
    last_orient = orient;
    if (flags & CF_DEAD) {
        death_move = find_move(6, death_move_sub);
        instance->start_anim(death_move->anim, 1.0f, true, 1.0f);
        instance->process(0.0f, false, 0);
        t = instance->get_anim_time_left(death_move->anim);
        instance->process(t, false, 0);
        hp = 0.0f;
    }
}

// 0x44f5f0  Confidence: exact (as place_at_party_start, from a navpoint). t at ebp-4, this at ebp-0x6c.
void Character::place_at_marker(navpoint *marker)
{
    float t;

    pos = marker->pos;
    orient.set_from_angles(0.0f, 0.0f, marker->heading);
    layer = marker->layer;
    pos.y += 1.0f;
    find_ground();
    last_pos = pos;
    last_orient = orient;
    if (flags & CF_DEAD) {
        death_move = find_move(6, death_move_sub);
        instance->start_anim(death_move->anim, 1.0f, true, 1.0f);
        instance->process(0.0f, false, 0);
        t = instance->get_anim_time_left(death_move->anim);
        instance->process(t, false, 0);
        hp = 0.0f;
    }
}

// 0x44f880  Confidence: exact. Frame: hit -0x2c, hidden -0x30, eye_pt -0x3c, ray -0x88 (its inline
// constructor's store at -0x58 is the first instruction), li -0x8c, this -0xd0.
bool Character::has_line_of_sight_to_point(vector3 *point)
{
    lobj_hit hit;
    bool hidden;
    vector3 eye_pt;
    lobj_ray ray;
    int li;

    if (Character_los_enabled == 0)
        return true;
    li = 0;
    hidden = false;
    eye_pt = pos;
    eye_pt.y += size_height();
    ray.start = eye_pt;
    ray.dir = *point - eye_pt;
    ray._unk18 = 0.3f;
    ray.flags = 0;
    ray.require_flags = 0;
    ray.exclude_flags = 0x440;
    lobj_raycast_draw_lists(&ray, &hit, &li, &hidden, 0);
    if (hit.num_hits != 0)
        return false;
    return true;
}

// 0x44fa20  Confidence: exact (p at ebp-0xc, the operator= copy at ebp-0x18, this at ebp-0x20). A
// living target is aimed at its head height.
bool Character::has_line_of_sight_to(Character *target)
{
    vector3 p;

    if (living_from_handle(target->handle) != NULL) {
        p = target->pos;
        p.y += target->size_height();
        return has_line_of_sight_to_point(&p);
    }
    return has_line_of_sight_to_point(&target->pos);
}

// 0x44fab0  Confidence: exact. Frame: lvl_up -4 (bool), max_level -8, ring_xp -0xc, points -0x10, cap
// -0x14, msg -0x64, p -0x68, i -0x6c, pl -0x70, k -0x74, sk -0x78, this -0x7c. With `quiet` the level
// feedback sound is skipped and a party member spends its skill points at once, round-robin over its
// progression table: the loop never ends when no entry is below 100 at the character's level while
// points are left (NOTES.md, game bugs).
void Character::add_xp(int xp, bool quiet, bool from_net)
{
    bool lvl_up;
    int max_level;
    int ring_xp;
    int cap;
    int points;
    char msg[80];

    max_level = xp_max_level();
    if (level >= max_level)
        return;
    if ((game_flags & GF_MULTIPLAYER) && mp_local_player && !(mp_local_player->flags & 1) && !from_net)
        return;
    if ((game_flags & GF_MULTIPLAYER) && mp_local_player && (mp_local_player->flags & 1))
        mp_broadcast_xp(this, xp);
    this->xp += xp;
    if ((game_flags & GF_SINGLE_PLAYER) && get_skill(SKILL_SUMMON) > 0.0f) {
        ring_xp = (int)((float)(xp * 2) * get_skill(SKILL_SUMMON));
        if (equipment[1] != NULL)
            ring_add_xp(equipment[1]->type->name, ring_xp, this->xp * 2);
        if (equipment[2] != NULL)
            ring_add_xp(equipment[2]->type->name, ring_xp, this->xp * 2);
        rings_add_xp_all(ring_xp / 8, this->xp * 2);
    }
    lvl_up = xp_level_reached(level, this->xp);
    if (lvl_up) {
        while (lvl_up) {
            xp_level_up_gains(info->species, &level, &base_max_hp, &base_max_ap, &points, &cap);
            hp = (float)get_max_stat(0);
            ap = (short)get_max_stat(1);
            if (type == 7) {
                if ((game_flags & GF_SINGLE_PLAYER) || character_is_locally_controlled(this)) {
                    Player *p;
                    int i;

                    p = player_from_handle(handle);
                    p->skill_points += points;
                    p->skill_cap = cap;
                    for (i = 0; i < p->num_progression; i++) {
                        if (level == p->progression[i].level)
                            skill[p->progression[i].skill] = (ubyte)p->progression[i].value;
                    }
                }
                sprintf(msg, XSTR("%s gained level %d!", 321), name, level);
                hud_show_message(msg);
            }
            set_floating_text(XSTR("+Level!", 322), 2.0f, 0x46, 0xc8, 0x46);
            if (level == max_level)
                break;
            lvl_up = xp_level_reached(level, this->xp);
        }
        if (!quiet && species_is_hero(info->species) && character_is_locally_controlled(this))
            feedback_play(info->species, 0xe, &pos);
    }
    if (type == 7 && quiet && !Max_skills) {
        Player *pl;
        int k;
        int sk;

        k = 0;
        pl = player_from_handle(handle);
        while (pl->skill_points > 0) {
            if (level >= pl->progression[k].level) {
                sk = pl->progression[k].skill;
                if (skill[sk] < 100) {
                    skill[sk] += 10;
                    pl->skill_points--;
                }
            }
            k = (k + 1) % pl->num_progression;
        }
    }
}

// 0x44ffc0  Confidence: exact (key at ebp-0x1c, total -0x20, old -0x24; strcpy expanded). The ring's
// experience is kept as a level-4 savegame integer, capped at `cap`.
void ring_add_xp(const char *ring, int xp, int cap)
{
    char key[28];
    int total;
    int old;

    if (_stricmp(ring, "Ring of Darkness") == 0)
        strcpy(key, "dark_ring_xp");
    else if (_stricmp(ring, "Ring of Light") == 0)
        strcpy(key, "light_ring_xp");
    else if (_stricmp(ring, "Ring of Fire") == 0)
        strcpy(key, "fire_ring_xp");
    else if (_stricmp(ring, "Ring of Stone") == 0)
        strcpy(key, "stone_ring_xp");
    else if (_stricmp(ring, "Ring of Water") == 0)
        strcpy(key, "water_ring_xp");
    else if (_stricmp(ring, "Ring of Four Winds") == 0)
        strcpy(key, "wind_ring_xp");
    else if (_stricmp(ring, "Ring of Forest") == 0)
        strcpy(key, "forest_ring_xp");
    else if (_stricmp(ring, "Ring of Jade") == 0)
        strcpy(key, "jade_ring_xp");
    else
        return;
    old = savegame_state.get_int(4, key);
    total = old + xp;
    ASSIGN_MIN(total, total, cap);
    savegame_state.set_int(4, key, total);
    ring_check_summon_unlock(ring, old, total);
}

// 0x450220  Confidence: exact (the ?: temporaries of the seven XSTRs at ebp-4 .. -0x1c). A ring
// crossing 60000 experience unlocks its summon; the four dragon rings must also be in the party
// inventory. The Ring of Darkness has no message, and a ring that lands on exactly 60000 shows none
// (`new > 60000`).
void ring_check_summon_unlock(const char *ring, int old_xp, int new_xp)
{
    if (old_xp < 60000 && new_xp > 60000) {
        if (_stricmp(ring, "Ring of Light") == 0)
            hud_show_message(XSTR("Joseph acquired Summon Wraith!", 314));
        if (_stricmp(ring, "Ring of Fire") == 0)
            hud_show_message(XSTR("Joseph acquired Summon Blood Elemental!", 315));
        if (_stricmp(ring, "Ring of Stone") == 0)
            hud_show_message(XSTR("Joseph acquired Summon Blade of Urath!", 316));
        if (_stricmp(ring, "Ring of Water") == 0 && party_count_item_by_name("Ring of Water"))
            hud_show_message(XSTR("Joseph acquired Summon Dragon of Water!", 317));
        if (_stricmp(ring, "Ring of Four Winds") == 0 && party_count_item_by_name("Ring of Four Winds"))
            hud_show_message(XSTR("Joseph acquired Summon Dragon of Four Winds!", 318));
        if (_stricmp(ring, "Ring of Forest") == 0 && party_count_item_by_name("Ring of Forest"))
            hud_show_message(XSTR("Joseph acquired Summon Dragon of Forest!", 319));
        if (_stricmp(ring, "Ring of Jade") == 0 && party_count_item_by_name("Ring of Jade"))
            hud_show_message(XSTR("Joseph acquired Summon Jade Dragon!", 320));
    }
}

// Shared body of rings_add_xp_all's eight blocks (a macro of ours: the code repeats it with the old
// value kept in `old`).
#define RING_XP_ADD(name)                                    \
    strcpy(key, name);                                       \
    total = savegame_state.get_int(4, key);                  \
    oldv = total;                                            \
    total = total + xp;                                      \
    ASSIGN_MIN(total, total, cap);                                 \
    savegame_state.set_int(4, key, total)

// 0x450420  Confidence: exact (oldv -4, key -0x20, total -0x24). Every ring gets the share, worn or
// not; the darkness ring has no unlock check.
void rings_add_xp_all(int xp, int cap)
{
    int oldv;
    char key[28];
    int total;

    RING_XP_ADD("dark_ring_xp");
    RING_XP_ADD("light_ring_xp");
    ring_check_summon_unlock("Ring of Light", oldv, total);
    RING_XP_ADD("fire_ring_xp");
    ring_check_summon_unlock("Ring of Fire", oldv, total);
    RING_XP_ADD("stone_ring_xp");
    ring_check_summon_unlock("Ring of Stone", oldv, total);
    RING_XP_ADD("water_ring_xp");
    ring_check_summon_unlock("Ring of Water", oldv, total);
    RING_XP_ADD("wind_ring_xp");
    ring_check_summon_unlock("Ring of Four Winds", oldv, total);
    RING_XP_ADD("forest_ring_xp");
    ring_check_summon_unlock("Ring of Forest", oldv, total);
    RING_XP_ADD("jade_ring_xp");
    ring_check_summon_unlock("Ring of Jade", oldv, total);
}

// 0x450850  Confidence: exact. Frame: c -4, v -8, can_apply -0xc (bool), max_hp -0x10, this -0x14, the
// ?: temporaries -0x18 / -0x1c. Returns false when the character is (now) dead. Multiplayer hosts
// scale damage dealt by bosses / 0x40-flagged attackers by 4 and by other non-party attackers by 2.
// The +0x6e8 / +0x6f0 pair is the floating HP number: a hit within 0.2 s of the last one adds to it.
// The first test stores the amount only when it is zero (`delta == 0`), as the code reads.
bool Character::add_hp(float delta, int source, int a, bool from_net)
{
    Character *c;
    float v;
    bool can_apply;
    int max_hp;

    if (flags & CF_DEAD)
        return false;
    if (type == 4)
        return true;
    c = living_from_handle(source);
    hit_by_party = (c != NULL && c->type == 7) ? 1 : 0;
    if ((game_flags & GF_MULTIPLAYER) && !from_net && delta < 0.0f && c != NULL) {
        if ((c->flags & 0x40) || c->type == 9)
            delta *= 4.0f;
        else if (c->type != 7)
            delta *= 2.0f;
    }
    if (hp_text_timer <= 0.0f && delta == 0.0f) {
        hp_overflow = delta;
        hp_text_timer = 2.0f;
    }
    if (delta != 0.0f) {
        if (hp_text_timer >= 1.8f)
            hp_overflow += delta;
        else
            hp_overflow = delta;
        hp_text_timer = 2.0f;
    }
    if (delta < 0.0f)
        flags |= CF_HURT;
    if (delta == 0.0f)
        return true;
    if (type == 9)
        boss_threat_add(this, delta, source, a);
    if (has_status(STATUS_SLEEP) == 1 && delta < 0.0f)
        status_effect_cure(STATUS_SLEEP, 1, 0);
    if (delta < 0.0f && ai.is_mode(AI_STATE_DISABLED, 0x31) == 1) {
        clear_flag_2_4();
        ai.go_idle();
    }
    if ((flags & 0x400) && delta < 0.0f)
        ai.add_sneak();
    if (has_status(0x10)) {
        if (delta < 0.0f && c != NULL) {
            v = status_value(0x10);
            v += delta;
            if (v < 0.0f)
                v = 0.0f;
            status_set_1c(0x10, v);
        }
    } else {
        if (MP_IS_CLIENT())
            can_apply = true;
        else if (delta < 0.0f)
            can_apply = (Debug_death && (!Players_immortal || type != 7)) ? 1 : 0;
        else
            can_apply = true;
        if (can_apply) {
            max_hp = get_max_stat(0);
            hp += delta;
            if ((float)max_hp < hp) {
                hp_overflow = (float)max_hp - (hp - delta);
                hp = (float)max_hp;
            }
            if (MP_IS_HOST())
                mp_broadcast_hp(this, source);
        }
    }
    if (hp <= 0.0f)
        return !kill(0x22);
    return true;
}

// 0x450c70  Confidence: exact. Frame: buf -0x10, new_ap -0x14, max_ap -0x18, this -0x1c, the XSTR
// temporary -0x20.
void Character::add_ap(float delta)
{
    int new_ap;
    char buf[16];
    int max_ap;

    if (type == 4)
        return;
    if (delta == 0.0f)
        return;
    new_ap = ap + (int)delta;
    ASSIGN_MAX(new_ap, new_ap, 0);
    max_ap = get_max_stat(1);
    ASSIGN_MIN(new_ap, new_ap, max_ap);
    sprintf(buf, XSTR("%d AP", 323), (int)delta);
    set_floating_text(buf, 2.0f, 100, 100, 0xb4);
    if (MP_IS_HOST())
        mp_broadcast_ap(this, (short)new_ap);
    ap = (short)new_ap;
}

// 0x450d90  Confidence: exact (the one condition with the client test first). Returns true when the
// character starts dying (CF_HIDDEN set; character_on_death finishes it). Party members (type 7) and,
// in single player, bosses never die here; Debug_death 0 ("death") makes everything immortal except
// on a multiplayer client.
bool Character::kill(int death_sub)
{
    death_move_sub = death_sub;
    hp = 0.0f;
    clear_all_effects();
    if (!MP_IS_CLIENT() && (!Debug_death || (type == 9 && (game_flags & GF_SINGLE_PLAYER))))
        return false;
    if (type == 7)
        return false;
    flags |= CF_HIDDEN;
    return true;
}

// Scatters a drop position in the character's ground plane: pos, then +/- 0.5 along the right and
// forward axes. Every drop of monster_drop_loot repeats these three statements (expanded inline:
// operator=, operator* and operator+= with their temporaries); a macro of ours.
#define LOOT_SCATTER(p)                                    \
    p = pos;                                               \
    p += orient.rvec * (0.5f - frand());                   \
    p += orient.fvec * (0.5f - frand())

// One named-character drop: the unique item of a quest character, never locked.
#define LOOT_UNIQUE(item_name)                                                         \
    unlocked = true;                                                                   \
    LOOT_SCATTER(drop_pos);                                                            \
    drops[n] = item_obj_spawn(item_create_by_name(item_name), &drop_pos, &orient, 1, -1, -1); \
    if (drops[n] != NULL)                                                              \
        n++

// 0x450e30  Confidence: likely (statement order and frame; the many inline-expansion temporaries
// -0xc0..-0x470 are not declared). Frame: j -4, lock -8, box -0xc, bone -0x10, drops -0x50 (16),
// unlocked -0x54, bone_orient -0x78, i -0x7c, n -0x80, highlight -0x84, drop_pos -0x90, count -0x94,
// chances -0x98, num -0x9c, items -0xa0, levels -0xa4, t -0xa8, roll -0xac, weapon -0xb0, shield
// -0xb4, lock_chance -0xb8, first -0xbc, this -0x478.
// Gold (Chance_to_drop_gold, 1..level*10, times the players in multiplayer), then the char_info drop
// table (single player: +0x274; multiplayer: +0x300 with per-entry minimum levels), then in single
// player the wielded weapon and shield, then the quest items of named characters. Several drops (or one
// that is not gold) go into a container that is locked with chance level / 100 unless a drop was rare
// (chance > 0.98), unique, or the game is multiplayer. Medevan Knights (species 0x23-0x25) skip the
// table. The roll is frand() mirrored when (int)(frand() * 100) is odd, minus 0.01.
void Character::monster_drop_loot()
{
    int j;
    float lock;
    container *box;
    int bone;
    item_obj *drops[16];
    bool unlocked;
    matrix bone_orient;
    int i;
    int n;
    int highlight;
    vector3 drop_pos;
    int count;
    float *chances;
    int num;
    int *items;
    int *levels;
    float t;
    float roll;
    item_info *weapon;
    item_info *shield;
    float lock_chance;
    item_obj *first;

    n = 0;
    unlocked = (game_flags & GF_MULTIPLAYER) != 0;
    lock = 0.0f;
    if (frand() < Chance_to_drop_gold) {
        drop_pos = pos;
        drop_pos += orient.rvec * 0.5f;
        count = (int)(frand() * level * 10.0f) + 1;
        if (game_flags & GF_MULTIPLAYER)
            count *= mp_netgame_num_players;
        drops[n] = item_obj_spawn(item_create_misc_by_name("Gold"), &drop_pos, &orient, count, -1, -1);
        if (drops[n] != NULL)
            n++;
    }
    if (info->species == 0x23 || info->species == 0x24 || info->species == 0x25) {
        printf("No drop for Medevan Knights\n");
    } else {
        if (game_flags & GF_SINGLE_PLAYER) {
            num = info->num_drops;
            items = info->drop_items;
            chances = info->drop_chances;
            levels = NULL;
        } else {
            num = info->num_mp_drops;
            items = info->mp_drop_items;
            chances = info->mp_drop_chances;
            levels = info->mp_drop_levels;
        }
        for (i = 0; i < num; i++) {
            if (levels != NULL && levels[i] > level)
                continue;
            if ((item_types[items[i]].flags & ITF_DIFFICULTY_EASY) && mp_game_difficulty != 0)
                continue;
            if ((item_types[items[i]].flags & ITF_DIFFICULTY_NORMAL) && mp_game_difficulty != 1)
                continue;
            if ((item_types[items[i]].flags & ITF_DIFFICULTY_HARD) && mp_game_difficulty != 2)
                continue;
            roll = frand();
            t = frand() * 100.0f;
            if ((int)t % 2)
                roll = 1.0f - roll;
            roll -= 0.01f;
            printf("Rolling %d %d\n", (int)(roll * 100.0f), (int)(chances[i] * 100.0f));
            if (roll <= chances[i]) {
                printf("Dropping\n");
                if (chances[i] > 0.98f)
                    unlocked = true;
                LOOT_SCATTER(drop_pos);
                drops[n] = item_obj_spawn(item_create(items[i]), &drop_pos, &orient, 1, -1, -1);
                if (drops[n] != NULL)
                    n++;
            }
        }
    }
    if (game_flags & GF_SINGLE_PLAYER) {
        if (equipment[5] != NULL && equipment[5]->type->kind == 0) {
            bone = info->prop_tag_bones[prop_tag];
            if (bone != -1) {
                instance->get_bone_world_transform(bone, &orient, &pos, &bone_orient, &drop_pos);
            } else {
                drop_pos = pos;
                drop_pos += orient.rvec + 0.5f;
            }
            if (!(equipment[5]->type->weapon_flags & 2)) {
                weapon = character_unequip_slot(this, 5, 1);
                drops[n] = item_obj_spawn(weapon, &drop_pos, &orient, 1, -1, -1);
                if (drops[n] != NULL)
                    n++;
            }
        }
        if (equipment[9] != NULL && equipment[9]->type->kind == 0) {
            bone = info->prop_tag_bones[1];
            if (bone != -1) {
                instance->get_bone_world_transform(bone, &orient, &pos, &bone_orient, &drop_pos);
            } else {
                drop_pos = pos;
                drop_pos -= orient.rvec + 0.5f;
            }
            if (!(equipment[9]->type->weapon_flags & 2)) {
                shield = character_unequip_slot(this, 9, 1);
                drops[n] = item_obj_spawn(shield, &drop_pos, &orient, 1, -1, -1);
                if (drops[n] != NULL)
                    n++;
            }
        }
    }
    if (_stricmp(name, "Slave Driver#$key") == 0) {
        LOOT_UNIQUE("Slave Pit Key");
    } else if (_stricmp(name, "Mortankas") == 0) {
        LOOT_UNIQUE("Parchment#Tome Page 8");
    } else if (_stricmp(name, "Azha the Archlich") == 0) {
        LOOT_UNIQUE("Azha's Femur");
    } else if (_stricmp(name, "Lord Korel") == 0) {
        LOOT_UNIQUE("Jarl's Sword");
    } else if (_stricmp(name, "Zane") == 0) {
        LOOT_UNIQUE("Zane's Bow");
    } else if (_stricmp(name, "Cerval") == 0) {
        LOOT_UNIQUE("Silk Garments");
    } else if (_stricmp(name, "Haenul") == 0) {
        LOOT_UNIQUE("Eraekor Gemstone");
    } else if (_stricmp(name, "Guardian of the Shrine") == 0) {
        LOOT_UNIQUE("Ring of Forest");
    } else if (cur_level_index == 3 && _stricmp(level_script_name, "Masad") == 0
               && _stricmp(name, "Orenian Scout#$npc002") == 0) {
        for (i = 0; i < n; i++) {
            if (drops[i]->item->type - item_types == item_type_find("Worn Katana"))
                break;
        }
        if (i >= n) {
            LOOT_UNIQUE("Worn Katana");
        }
    }
    if (n == 0)
        return;
    for (i = 0; i < n; i++) {
        if (drops[i]->item->type->kind == 0) {
            sound_play_3d_at(0xd, &pos);
            break;
        }
    }
    if (i >= n)
        sound_play_3d_at(0xc, &pos);
    lock = 0.0f;
    highlight = -1;
    if (n > 1 || !drops[0]->is_gold()) {
        if (unlocked) {
            lock = 0.0f;
        } else {
            lock_chance = level / 100.0f;
            if (frand() < lock_chance)
                lock = frand();
            else
                lock = 0.0f;
        }
        box = container_obj_create(&drop_pos, lock);
        if (box != NULL) {
            highlight = box->handle;
            for (j = 0; j < n; j++)
                box->add_item(drops[j]);
            do {
                first = box->get_item(0);
                if (first != NULL) {
                    box->pos = first->pos;
                    box->pos.y += 0.2f;
                }
            } while (0);
            printf("CONTAINER POS : (%f %f %f) (%f %f %f) (%f %f %f)\n", box->pos.x, box->pos.y,
                   box->pos.z, box->get_item(0)->pos.x, box->get_item(0)->pos.y, box->get_item(0)->pos.z,
                   cur_character->pos.x, cur_character->pos.y, cur_character->pos.z);
        }
    } else if (n == 1) {
        highlight = drops[0]->handle;
    }
    hud_set_timed_highlight(highlight);
    if (MP_IS_HOST() && n > 0)
        container_spawn_loot(n, drops, highlight, lock, &drop_pos);
}

// 0x4530d0  Confidence: likely. Frame: ray -0x4c (its inline constructor's store at -0x1c comes
// first), foot -0x5c, angle -0x60, dy -0x64, d -0x70, fwd -0x74, ch -0x78, hit -0xb0, head_orient
// -0x104, len -0x108, head_pos -0x114, foot_orient -0x138, foot_pos -0x144, alt -0x148, sign -0x14c,
// head -0x15c, this -0x1bc. Once per character (flag 0x4000000): the lean of the corpse, from the
// head and left-foot bones of the last frame of the death move dropped onto the floor; species 0xce,
// species 0x9f, or no death move, get none.
void Character::update_head_look()
{
    lobj_ray ray;
    int foot;
    float angle;
    float dy;
    vector3 d;
    float fwd;
    ca_character *ch;
    lobj_hit hit;
    matrix head_orient;
    float len;
    vector3 head_pos;
    matrix foot_orient;
    vector3 foot_pos;
    int alt;
    float sign;
    int head;

    if (flags & CF_HEAD_LOOK_DONE)
        return;
    flags |= CF_HEAD_LOOK_DONE;
    if (info->species == 0xce || info->species == 0x9f) {
        move_turn_angle = 0.0f;
        return;
    }
    if (death_move == NULL) {
        move_turn_angle = 0.0f;
        return;
    }
    ch = instance->character;
    head = foot = -1;
    head = ch->find_mesh_tag("BDBN-Head");
    if (head == -1)
        head = ch->find_mesh_tag("BDBN-Head01");
    foot = ch->find_mesh_tag("BDBN-Foot-L");
    if (foot != -1) {
        alt = ch->find_mesh_tag("BDBN-Foot-L01");
        if (alt != -1)
            foot = alt;
    } else {
        foot = ch->find_mesh_tag("BDBN-Foot-L02");
    }
    if (head == -1 || foot == -1) {
        last_orient = orient;
        return;
    }
    len = instance->get_anim_length(death_move->anim);
    instance->process(len - 0.01f, false, 0);
    instance->process(0.0f, false, 0);
    instance->get_bone_world_transform(head, &orient, &pos, &head_orient, &head_pos);
    instance->get_bone_world_transform(foot, &orient, &pos, &foot_orient, &foot_pos);
    ray.start = head_pos;
    ray.start.y += 2.0f;
    ray.dir.x = 0.0f;
    ray.dir.y = -10.0f;
    ray.dir.z = 0.0f;
    ray._unk18 = 0.0f;
    ray.flags = 0;
    ray.require_flags = 0;
    ray.exclude_flags = 0;
    ray._unk28 = 0;
    lobj_raycast_all(&ray, &hit);
    if (hit.num_hits > 0)
        head_pos.y += ray.dir.y * hit.t;
    ray.start = foot_pos;
    ray.start.y += 2.0f;
    ray.dir.x = 0.0f;
    ray.dir.y = -10.0f;
    ray.dir.z = 0.0f;
    ray._unk18 = 0.0f;
    ray.flags = 0;
    ray.require_flags = 0;
    ray.exclude_flags = 0;
    ray._unk28 = 0;
    lobj_raycast_all(&ray, &hit);
    if (hit.num_hits > 0)
        foot_pos.y += ray.dir.y * hit.t;
    play_move((int)death_move, 1.0f, 1, 1, 0);
    instance->process(0.0f, false, 0);
    d = foot_pos - head_pos;
    dy = d.y;
    fwd = d.x * orient.fvec.x + d.y * orient.fvec.y + d.z * orient.fvec.z;
    sign = -1.0f;
    if (dy < 0.0f) {
        sign *= -1.0f;
        dy = -dy;
    }
    if (fwd < 0.0f) {
        sign *= -1.0f;
        fwd = -fwd;
    }
    angle = atan2_fast(dy, fwd) * sign;
    if (angle > -1.5707964f && angle < 1.5707964f)
        move_turn_angle = angle;
    else
        move_turn_angle = 0.0f;
}

// 0x4536c0  Confidence: exact (p at ebp-4, c -8, this -0xc; `this` is not read). Gives Joseph
// (species 0) back the max HP lent to the summon; in single player also searched in the other list.
void Character::summon_return_lent_max_hp()
{
    Player *p;
    Character *c;

    for (p = party_list.next; p != &party_list; p = p->next) {
        if (char_infos[p->char_info].species == SPECIES_JOSEPH) {
            c = living_from_handle(p->handle);
            if (c != NULL) {
                c->base_max_hp += p->lent_max_hp;
                p->lent_max_hp = 0.0f;
            }
            if (game_flags & GF_SINGLE_PLAYER)
                break;
        }
    }
    if ((game_flags & GF_SINGLE_PLAYER) && p == &party_list) {
        for (p = player_list_other.next; p != &player_list_other; p = p->next) {
            if (char_infos[p->char_info].species == SPECIES_JOSEPH) {
                c = living_from_handle(p->handle);
                if (c != NULL) {
                    c->base_max_hp += p->lent_max_hp;
                    p->lent_max_hp = 0.0f;
                }
                break;
            }
        }
    }
    savegame_state.hot_buttons_clear_member(4);
}

// 0x4537f0  Confidence: likely. Frame: p -4, q -8, scale -0xc, xp_total -0x10, ratio -0x14, killers
// -0x24 (4), n -0x28, dist -0x2c, c -0x30, share -0x34, text -0xb4, i -0xb8, pl -0xbc, c2 -0xc0, this
// -0xe4, the switch value -0x100. Experience goes to the living, visible, non-summon party members
// within 20 m (dist^2 < 400) when a party member hit the victim: char_info xp scaled by
// (level / info level) (sqrt(sqrt(r)) * r above 1), times the players in multiplayer, split evenly.
// When Joseph dies his summon turns hostile (team 3, type 5). The character then moves to the
// inactive list; monsters (not summons) drop their loot unless this is a multiplayer client.
void Character::on_death()
{
    Player *p;
    Player *q;
    float scale;
    float xp_total;
    float ratio;
    Character *killers[4];
    int n;
    float dist;
    Character *c;
    float share;
    char text[128];
    int i;
    Player *pl;
    Character *c2;

    if (flags & CF_DEAD)
        return;
    if (!(hp <= 0.0f) || (type == 9 && (game_flags & GF_SINGLE_PLAYER)))
        return;
    ((GameTimer *)&combat.timer)->SetTimeout(0);
    flags |= CF_HIDDEN;
    if (cur_anim >= 0) {
        if (move_has_trigger(9) && move_trigger_time_left(9) > 0.0001f)
            return;
        fx_remove_missiles_from(cur_anim);
    }
    flags &= ~CF_SUMMONLIKE;
    flags &= ~0x400;
    fx_end_owned(handle, 0);
    clear_all_effects();
    flags |= CF_DEAD;
    ai_group_remove_member(this);
    instance->clear_action_anims();
    instance->clear_state_anims();
    update_locomotion_anim();
    death_move = find_move(6, death_move_sub);
    play_move((int)death_move, 1.0f, 1, 1, 0);
    instance->process(0.0f, false, 0);
    if ((flags & CF_SUMMON) && type == 7) {
        summon_return_lent_max_hp();
        p = player_from_handle(handle);
        if (p != NULL)
            party_remove_player(p, false);
    } else if (hit_by_party) {
        n = 0;
        for (q = party_list.next; q != &party_list; q = q->next) {
            c = living_from_handle(q->handle);
            if (c == NULL)
                continue;
            if ((c->flags & CF_DEAD) || (c->flags & CF_HIDDEN) || (c->flags & CF_SUMMON))
                continue;
            dist = (pos - c->pos).len_sq();
            if (dist < 400.0f)
                killers[n++] = c;
        }
        ratio = (float)level / info->level;
        if (ratio < 1.0f)
            scale = ratio;
        else
            scale = (float)sqrt((float)sqrt(ratio)) * ratio;
        xp_total = info->xp_gained * scale;
        if (game_flags & GF_MULTIPLAYER)
            xp_total = mp_netgame_num_players * xp_total;
        if (n > 0) {
            share = xp_total / n;
            ASSIGN_MIN(share, share, 2147483648.0f);
            sprintf(text, XSTR("+%.f xp", 324), (double)(int)(n * share + 0.5f));
            set_floating_text(text, 2.0f, 0x32, 0xb4, 0x32);
            if (!MP_IS_CLIENT()) {
                for (i = 0; i < n; i++)
                    killers[i]->add_xp((int)share, false, false);
            }
        }
    }
    if (species_is_hero(info->species))
        feedback_play(info->species, 0xf, &pos);
    if (info->species == SPECIES_JOSEPH && cur_summon != NULL && cur_summon->type == 7) {
        pl = player_from_handle(handle);
        if ((game_flags & GF_SINGLE_PLAYER) || (pl != NULL && summon_player->mp_owner == pl->mp_owner)) {
            party_remove_player(summon_player, false);
            cur_summon->set_team(3);
            cur_summon->type = 5;
            cur_summon->ai.go_idle();
            if (interface_get_screen() == 0)
                hud_show_message(XSTR("Summoned creature has turned against you!", 325));
        }
    }
    if (this == cur_summon) {
        for (p = party_list.next; p != &party_list; p = p->next) {
            c2 = living_from_handle(p->handle);
            if (c2->info->species == SPECIES_JOSEPH) {
                player_start_skill_cooldown(c2, SKILL_SUMMON);
                break;
            }
        }
        if (game_flags & GF_MULTIPLAYER)
            mp_summon_claim_release();
        cur_summon = NULL;
    }
    if (type != 7 || (flags & CF_SUMMON)) {
        if (!(flags & CF_SUMMON) && !MP_IS_CLIENT())
            monster_drop_loot();
        switch (type) {
        case 7:
            type = 8;
            break;
        case 5:
            type = 6;
            break;
        case 9:
            type = 10;
            break;
        }
    }
    live_list_remove(this);
    live_list_append(&living_list_inactive, this);
}

// 0x454070  Confidence: exact (switch bodies in the order 8, 6, 10: the compare chain tests 6, 8, 10).
// p at ebp-4, this -8, the switch value -0xc. Joseph gets back the max HP he lent to the summon.
void Character::resurrect()
{
    Player *p;

    switch (type) {
    case 8:
        type = 7;
        break;
    case 6:
        type = 5;
        break;
    case 10:
        type = 9;
        break;
    }
    hp = (float)get_max_stat(0);
    flags &= ~(CF_HIDDEN | CF_DEAD);
    obj_flags |= 1;
    ai.go_idle();
    instance->clear_state_anims();
    instance->clear_action_anims();
    update_locomotion_anim();
    instance->process(0.0f, false, 0);
    if (type == 7) {
        if (game_flags & GF_SINGLE_PLAYER) {
            if (cur_character->ai.group == NULL)
                ai_group_create(this);
            else
                ai_group_add_member(cur_character->ai.group, this);
        } else {
            ai_group_create(this);
        }
        if (info->species == SPECIES_JOSEPH) {
            p = player_from_handle(handle);
            if (p != NULL) {
                base_max_hp += p->lent_max_hp;
                hp = (float)get_max_stat(0);
                p->lent_max_hp = 0.0f;
            }
        }
    }
    if (live_next != NULL && live_prev != NULL)
        live_list_remove(this);
    live_list_append(&living_list, this);
}

// 0x4542b0  Confidence: exact.
float Character::size_radius()
{
    if (type == 9)
        return charinfo_boss_radius(info);
    return character_size_radii[info->size];
}

// 0x4542f0  Confidence: exact. The boss height is computed and thrown away (`fstp st(0)`): the
// statement has no `return`, so every boss gets the size-table height (NOTES.md, game bugs).
float Character::size_height()
{
    if (type == 9)
        charinfo_boss_height(info);
    return character_size_heights[info->size];
}

// 0x454330  Confidence: exact (the radius ?: temporaries at ebp-0xa4 / -0xa8; the NULL test is made
// twice, as in the code). Frame: result -4, a -0x10, b -0x1c, m -0x40, this -0xa0. Centre to centre, minus
// both radii; with a target point (char_info +0x18a bone) the distance to that bone, radii not taken
// off. `flat` drops the heights.
float Character::distance_between_edges(Character *other, bool flat, int point)
{
    float result;
    vector3 a;
    vector3 b;
    matrix m;

    if (other == NULL)
        return 999999.0f;
    if (other == NULL)
        return 999999.0f;
    a = pos + centre_offset;
    b = other->pos + other->centre_offset;
    if (point == -1) {
        if (flat == 1)
            a.y = b.y = 0.0f;
        result = distance(&a, &b);
        result -= (type == 9) ? charinfo_boss_radius(info) : character_size_radii[info->size];
        result -= (other->type == 9) ? charinfo_boss_radius(other->info)
                                : character_size_radii[other->info->size];
    } else {
        other->instance->get_bone_world_transform(other->info->target_points[point],
                                                  &other->orient, &other->pos, &m, &b);
        b += other->centre_offset;
        if (flat == 1)
            a.y = b.y = 0.0f;
        result = distance(&a, &b);
    }
    return result;
}

// 0x454610  Confidence: exact.
void Character::set_team(int team)
{
    set_team_mask(team, team_enemy_masks[team]);
}

// 0x454640  Confidence: exact.
void Character::set_team_mask(int team, unsigned int mask)
{
    this->team = team;
    enemy_team_mask = mask;
}

// 0x454670  Confidence: exact.
void Character::set_flag_2_4(int arg)
{
    flags |= CF_SUMMONLIKE;
    if (arg >= 0)
        flags |= 0x4;
}

// 0x4546b0  Confidence: exact.
void Character::clear_flag_2_4()
{
    if (flags & 0x4)
        flags &= ~0x4;
    flags &= ~CF_SUMMONLIKE;
}

// 0x454700  Confidence: exact. Frame: time -4, caster -8, remain_ms -0xc, fobj -0x10, this -0x14, the three
// switch values -0x18 / -0x28 / -0x2c / -0x30, kind -0x1c, the fabs temporaries -0x20 / -0x24, the XSTR
// temporaries -0x34 .. -0x60. A status is not stacked: the longer of the remaining and the new time is
// kept (kind 0 timed, 2 `permanent` until cured with force); an item-held status (kind 1) is never
// replaced. Immunities: char_info flags 0x80 blind, 0x100 silence; bosses (type 9) and 0x40-flagged
// characters ignore the disabling statuses. Nothing applies on the world map (level 4) or without a
// level. The value with the larger magnitude wins.
void Character::status_effect_add(int id, int ms, int spell, float value, int source, bool permanent)
{
    int time;
    Character *caster;
    int remain_ms;
    obj *fobj;

    remain_ms = 0;
    if (id == -1)
        return;
    if (cur_level_index == -1 || cur_level_index == 4)
        return;
    switch (id) {
    case STATUS_BLIND:
        if (info->flags & 0x80)
            return;
        break;
    case STATUS_SILENCE:
        if ((info->flags & 0x100) || type == 9 || (flags & 0x40))
            return;
        break;
    case STATUS_CURSE:
    case STATUS_SLEEP:
    case STATUS_PARALYZE:
    case STATUS_SLOW:
    case STATUS_ICE_COFFIN:
        if (type == 9 || (flags & 0x40))
            return;
        // fall through
    case STATUS_POISON:
        if (type == 9)
            return;
        break;
    }
    if (id == STATUS_SLOW && get_mount() != NULL)
        return;
    switch (status[id].kind) {
    case 2:
        time = ms;
        break;
    case -1:
    case 0:
        if (status[id].timer.valid()) {
            remain_ms = status[id].timer.GetRemainingTime();
            if (remain_ms > ms)
                time = remain_ms;
            else
                time = ms;
        } else {
            time = ms;
        }
        break;
    default:
        return;
    }
    if (MP_IS_HOST())
        mp_broadcast_status_add(this, id, ms, spell, value, source);
    if (id == STATUS_PARALYZE) {
        if (spell == 0x49) {
            if (ms > remain_ms)
                flags |= 0x80000;
        } else {
            flags &= ~0x80000;
        }
    }
    if (permanent)
        status[id].kind = 2;
    else
        status[id].kind = 0;
    status[id].spell = spell;
    status[id].timer.SetTimeout(time);
    status[id].source = source;
    if ((float)fabs(value) > (float)fabs(status[id].value))
        status[id].value = value;
    caster = living_from_handle(source);
    if (caster == NULL || !(caster->flags & 0x800)) {
        switch (id) {
        case STATUS_LIGHT:
            status[id].fx = fx_create_status_effect(handle);
            break;
        case STATUS_INVISIBLE:
            status[id].fx = status_effect_update_fx(handle, 5);
            break;
        case STATUS_FIREWALL:
        case STATUS_BLIZZARD:
        case STATUS_DARK_WALL:
            status[id].fx = -1;
            break;
        default:
            if (spell != -1 && spells[spell].hit_effect != NULL) {
                fx_stop_status_effects(handle);
                status[id].fx = fx_attach_to_character(spells[spell].hit_effect, handle, -1, -1, 0, 1);
                if (status[id].fx != -1) {
                    fobj = obj_from_handle_b(status[id].fx);
                    if (fobj != NULL)
                        ((effect_object *)fobj)->spell = spell;
                } else {
                    printf("BOGUS STATUS EFFECT HANDLE\n");
                }
            }
            break;
        }
    }
    status[id].tick_ms = status_effect_duration(id);
    if (!status[id].tick_timer.valid() && status[id].tick_ms > 0)
        status[id].tick_timer.SetTimeout(status[id].tick_ms);
    switch (id) {
    case STATUS_SLEEP:
    case STATUS_PARALYZE:
    case STATUS_ICE_COFFIN:
        if (MP_IS_CLIENT())
            ai.go_idle();
        ai.go_inactive();
        instance->clear_action_anims();
        instance->clear_state_anims();
        update_locomotion_anim();
        instance->process(0.0f, false, 0);
        flags |= CF_SUMMONLIKE;
        break;
    case STATUS_SLOW:
        set_speed_scale(walk_speed, true);
        break;
    }
    switch (id) {
    case STATUS_SILENCE:
        set_floating_text(XSTR("+Silence", 326), 2.0f, 0xb4, 0xb4, 0xb4);
        break;
    case STATUS_BLIND:
        set_floating_text(XSTR("+Blind", 327), 2.0f, 0xb4, 0xb4, 0xb4);
        break;
    case STATUS_FAITH:
        set_floating_text(XSTR("+Faith", 328), 2.0f, 0xb4, 0xb4, 0xb4);
        break;
    case STATUS_CURSE:
        set_floating_text(XSTR("+Curse", 329), 2.0f, 0xb4, 0xb4, 0xb4);
        break;
    case STATUS_BLESS:
        set_floating_text(XSTR("+Bless", 330), 2.0f, 0xb4, 0xb4, 0xb4);
        break;
    case STATUS_REGEN:
        set_floating_text(XSTR("+Regen", 331), 2.0f, 0xb4, 0xb4, 0xb4);
        break;
    case STATUS_POWER:
        set_floating_text(XSTR("+Power", 332), 2.0f, 0xb4, 0xb4, 0xb4);
        break;
    case STATUS_SLEEP:
        set_floating_text(XSTR("+Sleep", 333), 2.0f, 0xb4, 0xb4, 0xb4);
        break;
    case STATUS_PARALYZE:
        set_floating_text(XSTR("+Paralyze", 334), 2.0f, 0xb4, 0xb4, 0xb4);
        break;
    case STATUS_POISON:
        set_floating_text(XSTR("+Poison", 335), 2.0f, 0xb4, 0xb4, 0xb4);
        break;
    case STATUS_CRIPPLE:
        set_floating_text(XSTR("+Cripple", 336), 2.0f, 0xb4, 0xb4, 0xb4);
        break;
    case STATUS_SLOW:
        set_floating_text(XSTR("+Slow", 337), 2.0f, 0xb4, 0xb4, 0xb4);
        break;
    }
}

// 0x454ff0  Confidence: exact (each immunity test is made twice, as the code has it). this at ebp-4,
// switch values -8 / -0xc / -0x18 / -0x1c, the fabs temporaries -0x10 / -0x14. An item-held status
// (kind 1): no duration (the timer is set to 1,000,000 ms), the slot's spell field holds the item.
void Character::status_effect_add_from_item(int id, item_info *item, float value)
{
    if (id == -1)
        return;
    if (cur_level_index == -1 || cur_level_index == 4)
        return;
    switch (id) {
    case STATUS_BLIND:
        if (info->flags & 0x80)
            return;
        if (info->flags & 0x80)
            return;
        break;
    case STATUS_SILENCE:
        if (info->flags & 0x100)
            return;
        if (info->flags & 0x100)
            return;
        break;
    case STATUS_ICE_COFFIN:
        if (type == 9)
            return;
        if (type == 9)
            return;
        break;
    }
    if (id == STATUS_SLOW && get_mount() != NULL)
        return;
    if (item == NULL)
        return;
    switch (status[id].kind) {
    case 0:
        status_effect_cure(id, true, false);
        // fall through
    case -1:
        status[id].kind = 1;
        status[id].timer.invalidate();
        status[id].item = item;
        status[id].value = value;
        break;
    case 1:
        if ((float)fabs(value) > (float)fabs(status[id].value)) {
            status[id].item = item;
            status[id].value = value;
        } else {
            return;
        }
        break;
    default:
        return;
    }
    status[id].fx = -1;
    status[id].timer.SetTimeout(1000000);
    switch (id) {
    case STATUS_LIGHT:
        status[id].fx = fx_create_status_effect(handle);
        break;
    case STATUS_INVISIBLE:
        status[id].fx = status_effect_update_fx(handle, 5);
        break;
    default:
        status[id].fx = -1;
        break;
    }
    status[id].tick_ms = status_effect_duration(id);
    if (!status[id].tick_timer.valid() && status[id].tick_ms > 0)
        status[id].tick_timer.SetTimeout(status[id].tick_ms);
    switch (id) {
    case STATUS_SLEEP:
    case STATUS_PARALYZE:
    case STATUS_ICE_COFFIN:
        ai.go_inactive();
        instance->clear_action_anims();
        instance->clear_state_anims();
        update_locomotion_anim();
        instance->process(0.0f, false, 0);
        flags |= CF_SUMMONLIKE;
        break;
    case STATUS_SLOW:
        set_speed_scale(walk_speed, true);
        break;
    }
}

// 0x4553a0  Confidence: exact. Frame: had -4, this -8, XSTR temporaries -0xc / -0x18 .. -0x44, the
// spell switch value -0x10, the message switch value -0x14. Without `force` a permanent (kind 2) status
// shows "Cant cure!". `restore_fx` re-attaches the spell's hit effect (or restarts the status 8 effect)
// when the slot was set by a spell other than 0x50.
void Character::status_effect_cure(int id, bool force, bool restore_fx)
{
    bool had;

    if (id == -1)
        return;
    if (!force && status[id].kind == 2) {
        set_floating_text(XSTR("Cant cure!", 338), 2.0f, 0xb4, 0xb4, 0xb4);
        return;
    }
    switch (status[id].spell) {
    case 0x22:
    case 0x24:
        if (status[id].fx != -1)
            fx_stop(status[id].fx);
        break;
    }
    if (restore_fx && status[id].spell != -1 && spells[status[id].spell].hit_effect != NULL
        && status[id].spell != 0x50) {
        if (id == STATUS_INVISIBLE)
            status_effect_update_fx(handle, 6);
        else if (status[id].spell >= 0 && status[id].spell < MAX_SPELLS)
            fx_attach_to_character(spells[status[id].spell].hit_effect, handle, -1, -1, 0, 1);
    }
    had = has_status(id);
    status[id].kind = -1;
    status[id].timer.invalidate();
    status[id].fx = -1;
    status[id].tick_timer.invalidate();
    status[id].tick_ms = 0;
    status[id].value = 0.0f;
    if (had && (id == STATUS_PARALYZE || id == STATUS_SLEEP || id == STATUS_ICE_COFFIN)) {
        flags &= ~CF_SUMMONLIKE;
        ai.go_idle();
    } else if (had && id == STATUS_SLOW) {
        set_speed_scale(run_speed, true);
    }
    if (had) {
        switch (id) {
        case STATUS_SILENCE:
            set_floating_text(XSTR("-Silence", 339), 2.0f, 0xb4, 0x32, 0x32);
            break;
        case STATUS_BLIND:
            set_floating_text(XSTR("-Blind", 340), 2.0f, 0xb4, 0x32, 0x32);
            break;
        case STATUS_FAITH:
            set_floating_text(XSTR("-Faith", 341), 2.0f, 0xb4, 0x32, 0x32);
            break;
        case STATUS_CURSE:
            set_floating_text(XSTR("-Curse", 342), 2.0f, 0xb4, 0x32, 0x32);
            break;
        case STATUS_BLESS:
            set_floating_text(XSTR("-Bless", 343), 2.0f, 0xb4, 0x32, 0x32);
            break;
        case STATUS_REGEN:
            set_floating_text(XSTR("-Regen", 344), 2.0f, 0xb4, 0x32, 0x32);
            break;
        case STATUS_POWER:
            set_floating_text(XSTR("-Power", 345), 2.0f, 0xb4, 0x32, 0x32);
            break;
        case STATUS_SLEEP:
            set_floating_text(XSTR("-Sleep", 346), 2.0f, 0xb4, 0x32, 0x32);
            break;
        case STATUS_PARALYZE:
            set_floating_text(XSTR("-Paralyze", 347), 2.0f, 0xb4, 0x32, 0x32);
            break;
        case STATUS_POISON:
            set_floating_text(XSTR("-Poison", 348), 2.0f, 0xb4, 0x32, 0x32);
            break;
        case STATUS_CRIPPLE:
            set_floating_text(XSTR("-Cripple", 349), 2.0f, 0xb4, 0x32, 0x32);
            break;
        case STATUS_SLOW:
            set_floating_text(XSTR("-Slow", 350), 2.0f, 0xb4, 0x32, 0x32);
            break;
        }
        if (MP_IS_HOST())
            mp_broadcast_status_cure(this, id, restore_fx);
    }
}

// 0x455950  Confidence: exact (`== 1` on the returned byte).
bool Character::has_status(int id)
{
    if (status[id].timer.valid() == 1)
        return true;
    return false;
}

// 0x455990  Confidence: exact. Active and not a permanent (kind 2) status.
bool Character::status_active(int id)
{
    if (status[id].timer.valid() == 1 && status[id].kind != 2)
        return true;
    return false;
}

// 0x4559e0  Confidence: exact.
int Character::status_time_left(int id)
{
    if (status[id].timer.valid() == 1)
        return status[id].timer.GetRemainingTime();
    return 0;
}

// 0x455a30  Confidence: exact.
float Character::status_value(int id)
{
    return status[id].value;
}

// 0x455a50  Confidence: exact.
void Character::status_set_1c(int id, float value)
{
    status[id].value = value;
}

// 0x455a70  Confidence: exact.
void Character::status_set_8(int id, int fx)
{
    status[id].fx = fx;
}

// 0x455a90  Confidence: exact. i at ebp-4, this at ebp-8.
void Character::clear_all_effects()
{
    int i;

    for (i = 0; i < NUM_STATUS_EFFECTS; i++)
        status_effect_cure(i, true, false);
    fx_stop_status_effects(handle);
    fx_stop_status_aura(handle);
    instance->set_tint(0xff, 0xff, 0xff);
    float_text_scale = 0.0f;
}

// 0x455b20  Confidence: exact. Frame: s -4, e -8, ilvl -0xc, pick -0x10, best_level -0x14, l2 -0x18,
// fill_item -0x1c, bl2 -0x20, l3 -0x24, l4 -0x28, mod -0x2c, this -0x30. Statuses 0-8 from the
// equipment: an item-held status follows its item (or the best other item giving it, by +Effect: level,
// 0 = the character's level), and a free slot takes the best item effect (not on the world map).
void Character::refresh_item_status_effects()
{
    int s;
    int e;
    int ilvl;
    item_info *pick;
    int l2;
    int best_level;
    int l3;
    int bl2;
    item_info *fill_item;
    float mod;
    int l4;

    for (s = 0; s < 9; s++) {
        if (status[s].kind == 1) {
            for (e = 0; e < 9; e++) {
                if (status[s].item == equipment[e])
                    break;
            }
            if (e < 9) {
                ilvl = equipment[e]->type->status_effects[s];
                if (ilvl == 0)
                    ilvl = level;
                status[s].value = status_effect_modifier(s, ilvl);
            } else {
                pick = NULL;
                best_level = -1;
                for (e = 0; e < 9; e++) {
                    if (equipment[e] != NULL) {
                        l2 = equipment[e]->type->status_effects[s];
                        if (l2 < 0)
                            continue;
                        if (l2 == 0)
                            l2 = level;
                        if (l2 > best_level) {
                            pick = equipment[e];
                            best_level = l2;
                        }
                    }
                }
                if (pick != NULL) {
                    status[s].item = pick;
                    status[s].value = status_effect_modifier(s, best_level);
                } else {
                    status_effect_cure(s, true, false);
                }
            }
        } else if (status[s].kind == -1 && cur_level_index != -1 && cur_level_index != 4) {
            fill_item = NULL;
            bl2 = -1;
            for (e = 0; e < 9; e++) {
                if (equipment[e] != NULL) {
                    l3 = equipment[e]->type->status_effects[s];
                    if (l3 < 0)
                        continue;
                    if (l3 == 0)
                        l3 = level;
                    if (l3 > bl2) {
                        fill_item = equipment[e];
                        bl2 = l3;
                    }
                }
            }
            if (fill_item != NULL) {
                l4 = fill_item->type->status_effects[s];
                if (l4 >= 0) {
                    if (l4 == 0)
                        mod = status_effect_modifier(s, level);
                    else
                        mod = status_effect_modifier(s, l4);
                    status_effect_add_from_item(s, fill_item, mod);
                }
            }
        }
    }
}

// 0x455e00  Confidence: likely (the offset is passed by value; the sum is built in the argument slot
// through the inline copy constructor). this at ebp-0x1c.
void Character::ai_travel_offset(vector3 offset)
{
    ai.travel_to_xyz(offset + pos);
}

// 0x455e70  Confidence: exact. Frame: dirn -0xc, to_target -0x18, this -0x40 (both vectors built in place
// from the inline operator- under 8966; to_target named for its hash bucket, D6101 / D7803). The normalised direction is
// computed and not used: the test is on the raw difference, so a travel order is skipped only when that
// dot product is exactly 1.
void Character::walk_to_point(vector3 p, int priority)
{
    vector3 dirn = p - pos;
    dirn.normalize_safe();
    vector3 to_target = p - pos;
    if (to_target.dot(orient.fvec) != 1.0f)
        ai.travel_to_point(&p, priority);
}

// 0x455f50  Confidence: exact.
void Character::ai_add_goal9(vector3 *pos, int layer, int priority, bool a)
{
    if (ai.is_traveling_sub0() == 1)
        ai.go_idle();
    ai.add_travel(pos, layer, priority, a);
}

// 0x455fb0  Confidence: likely for the logic, unsure for the frame. Three profile labels (the EH unwind
// map 0x583a80 destroys ebp-0x90 in state 0, ebp-0xb0 in state 1 and ebp-0xb8 in state 2; their text is
// not in the binary, D66) and two locals the function initialises without using (ebp-0x5c, the
// _unk30 store of an inline lobj_ray constructor, and ebp-0x3c). Frame: list -0x10, best_y -0x40, i
// -0x94, skip -0x98, probe -0xa4, count -0xa8, hit -0xac, idx -0xb4, j -0xbc, listed -0xc0, this
// -0x104. The floor is searched first in the remembered object, then in the objects of the
// character's layer (level_layer_ground, or every object on that layer), then in the floor objects
// (lobj flag 0x40) outside the layer list, then everywhere; the probe starts 10 m up (2 m once on the
// ground, CF 0x100000).
void Character::find_ground()
{
    layer_lobj_list *list = NULL;
    int unused3c;
    float best_y;
    lobj_ray unused_ray;
    profile_label label("character_find_ground");
    int i;
    int skip;
    vector3 probe;
    int count;
    bool hit;

    unused3c = 0;
    if (flags & CF_NO_GROUND)
        return;
    if (flags & 0x1000)
        return;
    if (!(obj_flags & 2))
        flags &= ~CF_ON_GROUND;
    best_y = -10000.0f;
    skip = -1;
    if (ground_face != -1) {
        probe = pos;
        if (!(flags & CF_ON_GROUND))
            probe.y += 10.0f;
        else
            probe.y += 2.0f;
        hit = character_probe_lobj_ground(flags, ground_lobj, &probe, &best_y, &ground_face, &ground_lobj,
                                          (vector3 *)ground_hit, true);
        if (hit)
            skip = ground_lobj;
    }
    if (skip == -1)
        ground_face = -1;
    if (layer >= 0) {
        profile_label label1("character_find_ground layer");

        list = level_layer_ground[layer];
        if (list != NULL)
            count = list->count;
        else
            count = num_lobjs;
        for (i = 0; i < count; i++) {
            int idx;

            if (list != NULL) {
                idx = list->indices[i];
            } else {
                idx = i;
                if (lobjs[idx].get_layer_n(0) != layer && lobjs[idx].get_layer_n(1) != layer)
                    continue;
            }
            if (idx == skip)
                continue;
            probe = pos;
            if (!(flags & CF_ON_GROUND))
                probe.y += 10.0f;
            else
                probe.y += 2.0f;
            character_probe_lobj_ground(flags, idx, &probe, &best_y, &ground_face, &ground_lobj, (vector3 *)ground_hit,
                                        false);
        }
    }
    if (ground_face == -1 && layer >= 0) {
        profile_label label2("character_find_ground floors");

        best_y = -10000.0f;
        for (i = 0; i < num_lobjs; i++) {
            if (!(lobjs[i].flags & 0x40))
                continue;
            if (list != NULL) {
                bool listed;
                int j;

                listed = false;
                for (j = 0; j < list->count; j++) {
                    if (list->indices[j] == i) {
                        listed = true;
                        break;
                    }
                }
                if (listed)
                    continue;
            }
            if (i == skip)
                continue;
            probe = pos;
            if (!(flags & CF_ON_GROUND))
                probe.y += 10.0f;
            else
                probe.y += 2.0f;
            character_probe_lobj_ground(flags, i, &probe, &best_y, &ground_face, &ground_lobj, (vector3 *)ground_hit,
                                        false);
        }
    }
    if (ground_face == -1) {
        best_y = -10000.0f;
        for (i = 0; i < num_lobjs; i++) {
            probe = pos;
            if (!(flags & CF_ON_GROUND))
                probe.y += 10.0f;
            else
                probe.y += 2.0f;
            if (i == skip)
                continue;
            character_probe_lobj_ground(flags, i, &probe, &best_y, &ground_face, &ground_lobj, (vector3 *)ground_hit,
                                        false);
        }
        if (ground_face == -1)
            return;
    }
    pos.y = best_y;
    flags |= CF_ON_GROUND;
}

// 0x456750  Confidence: exact. cdecl (no `this`). Frame: hit -0x2c, ray -0x78 (constructor store
// -0x48 first), hy -0x7c. A ray 20 m down (4 m once on the ground) into one level object; with
// `use_face` only the remembered face is tested (ray flag 2). The highest hit above *best_y wins.
// Returns true when the object was hit at all.
bool character_probe_lobj_ground(unsigned int flags, int lobj_index, vector3 *pos, float *best_y,
                                 short *face, short *lobj_out, vector3 *hit_pos, bool use_face)
{
    lobj_hit hit;
    lobj_ray ray;
    float hy;

    ray.start = *pos;
    if (!(flags & CF_ON_GROUND)) {
        ray.dir.x = 0.0f;
        ray.dir.y = -20.0f;
        ray.dir.z = 0.0f;
    } else {
        ray.dir.x = 0.0f;
        ray.dir.y = -4.0f;
        ray.dir.z = 0.0f;
    }
    ray._unk18 = 0.0f;
    ray.require_flags = 0;
    ray.exclude_flags = 0;
    if (use_face == 1) {
        ray.flags = 2;
        ray._unk2c = (int)&lobjs[lobj_index].mesh->faces[*face];
    } else {
        ray.flags = 0;
    }
    lobj_raycast(lobj_index, &ray, &hit, true);
    if (hit.num_hits > 0) {
        hy = ray.dir.y * hit.t + pos->y;
        if (hy > *best_y) {
            *best_y = hy;
            *lobj_out = (short)hit.face;
            *face = (short)(hit.face_ptr - lobjs[*lobj_out].mesh->faces);
            *hit_pos = hit.pos;
        }
        return true;
    }
    return false;
}

// 0x4568e0  Confidence: exact (text at ebp-0x100, this at -0x104; the error is the endless
// DebugError loop, line 3850).
void Character::update_layer()
{
    char text[256];

    if (ground_face != -1) {
        if (ground_lobj == -1)
            layer = -1;
        else
            layer = lobj_get_layer(ground_lobj);
        if (layer == -1) {
            layer = 0;
            sprintf(text, "Invalid layer for mesh %s.  It is probably not in a $layer selection set.\n"
                          "This problem should be fixed immediately.  Problem could also be a hole in "
                          "the world.", lobjs[ground_lobj].name);
            VSDK_ERROR(LIVING_ENTITY_CPP, 3850, text);
        }
    }
}

// 0x4569c0  Confidence: likely. One profile label (EH state 0); every return goes through a byte
// temporary (ebp-0x58, -0x5c, -0x84, -0xac, -0xb0, -0xb4) and the EH state -1 store. Frame: p -0x18, c
// -0x1c, r -0x20, rc -0x24, r_self -0x2c, px -0x30, pz -0x34, py -0x38, top -0x3c, dz -0x40, rsum -0x44,
// dx -0x48, push -0x54, this -0xec. Characters of the same team overlap at 3/4 of their radii. With
// `relative` (`move` is a step from pos) the step is replaced by the way round the first character hit
// and the result is false; otherwise true when something is in the way.
bool Character::find_overlap(vector3 *move, int unused, bool relative)
{
    profile_label label("character_find_overlap");
    vector3 p;
    Character *c;
    float r;
    float rc;
    float r_self;
    float px;
    float pz;
    float py;
    float top;
    float dz;
    float rsum;
    float dx;
    vector3 push;

    if ((flags & 0x20) || (flags & CF_SUMMONLIKE) || (flags & 0x400))
        return false;
    if (get_698() < 0.001f)
        return false;
    p = *move;
    if (relative)
        p += pos;
    r_self = size_radius();
    px = p.x;
    pz = p.z;
    py = p.y;
    top = size_height() + p.y;
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->flags & 0x20)
            continue;
        if (c == this)
            continue;
        rc = c->size_radius();
        if (team == c->team) {
            r = r_self * 0.75f;
            rc = rc * 0.75f;
        } else {
            r = r_self;
        }
        if (px - r < rc + c->pos.x && px + r > c->pos.x - rc && pz - r < rc + c->pos.z
            && pz + r > c->pos.z - rc && c->size_height() + c->pos.y > py && top > c->pos.y) {
            dx = px - c->pos.x;
            dz = pz - c->pos.z;
            rsum = r + rc;
            if (dx * dx + dz * dz < rsum * rsum) {
                if (relative) {
                    push = p - c->pos;
                    push.y = 0.0f;
                    if (push.is_zero())
                        return false;
                    push.normalize();
                    push *= rsum;
                    *move = (c->pos + push) - pos;
                    return false;
                }
                return true;
            }
        }
    }
    return false;
}

// 0x456ed0  Confidence: likely (speed -4, old -8, the returned vector -0x14, its pointer -0x18, the
// sqrt cast temporary -0x20, this -0x1c). The root speed of the locomotion blend at `blend`, the
// current blend restored afterwards.
float Character::anim_speed_at_blend(float blend)
{
    float speed;
    float old;

    old = loco_blend;
    if (blend != old) {
        loco_blend = blend;
        update_locomotion_anim();
    }
    speed = instance->sum_root_velocity().len();
    if (blend != old) {
        loco_blend = old;
        update_locomotion_anim();
    }
    return speed;
}

// 0x456f80  Confidence: exact.
float Character::get_698()
{
    if (ai_suspended())
        return 0.0f;
    return forward_speed;
}

// 0x456fb0  Confidence: exact (an output pointer, not a returned vector3: EAX is not set).
void Character::get_forward_velocity(vector3 *out)
{
    if (ai_suspended()) {
        out->zero();
        return;
    }
    *out = orient.fvec * forward_speed;
}

// 0x457060  Confidence: likely (the two fst temporaries at ebp-0x10 / -8).
float Character::get_speed()
{
    if (ai_suspended())
        return 0.0f;
    return velocity.len();
}

// 0x4570d0  Confidence: exact.
void Character::set_6ac(float v)
{
    target_speed = v;
}

// 0x4570f0  Confidence: exact.
void Character::boss_anim_remap_in(int *anims)
{
    if (type == 9)
        ::boss_anim_remap_in(this, anims);
}

// 0x457120  Confidence: exact.
void Character::boss_anim_remap_out(int *anims)
{
    if (type == 9)
        ::boss_anim_remap_out(this, anims);
}

// The LENT record macros (names ours). Every value goes through two nested do-while (0) levels with a
// block-local copy (save) or straight into its destination (load); a vector is a third level around
// three floats, a bool a third level around its byte. Floats are copied with integer moves, as VC6
// does for a plain assignment.
#define LE_PUT(T, v)                                                    \
    do {                                                                \
        T le_tmp = (v);                                                 \
        do {                                                            \
            *(T *)(out + ofs) = le_tmp;                                 \
            ofs += sizeof(T);                                           \
        } while (0);                                                    \
    } while (0)
#define LE_PUT_VECTOR(v) do { LE_PUT(float, (v).x); LE_PUT(float, (v).y); LE_PUT(float, (v).z); } while (0)
#define LE_PUT_BOOL(v) do { bool le_b = (v); LE_PUT(bool, le_b); } while (0)
#define LE_GET(T, dst)                                                  \
    do {                                                                \
        do {                                                            \
            (dst) = *(const T *)(in + ofs);                             \
            ofs += sizeof(T);                                           \
        } while (0);                                                    \
    } while (0)
#define LE_GET_VECTOR(v) do { LE_GET(float, (v).x); LE_GET(float, (v).y); LE_GET(float, (v).z); } while (0)
#define LE_GET_BOOL(dst) do { ubyte le_b; LE_GET(ubyte, le_b); (dst) = (le_b != 0); } while (0)

// The status part of a record: up to four active timed or permanent statuses, in the order of the
// table, padded to four with empty records (kind 0xff). 19 bytes each.
#define LE_PUT_STATUSES(n, id)                                          \
    n = 0;                                                              \
    for (i = 0; i < NUM_STATUS_EFFECTS; i++) {                          \
        if (n >= 4)                                                     \
            break;                                                      \
        id = sort_order[i];                                             \
        switch (status[id].kind) {                                      \
        case 0:                                                         \
        case 2:                                                         \
            LE_PUT(ubyte, (ubyte)status[id].kind);                      \
            LE_PUT(ubyte, (ubyte)id);                                   \
            LE_PUT(ubyte, (ubyte)status[id].spell);                     \
            LE_PUT(int, status[id].timer.GetRemainingTime());           \
            LE_PUT(int, status[id].tick_timer.GetRemainingTime());      \
            LE_PUT(int, status[id].tick_ms);                            \
            LE_PUT(float, status[id].value);                            \
            n++;                                                        \
        }                                                               \
    }                                                                   \
    for (i = n; i < 4; i++) {                                           \
        LE_PUT(ubyte, 0xff);                                            \
        LE_PUT(ubyte, 0);                                               \
        LE_PUT(ubyte, 0);                                               \
        LE_PUT(int, 0);                                                 \
        LE_PUT(int, 0);                                                 \
        LE_PUT(int, 0);                                                 \
        LE_PUT(int, 0);                                                 \
    }

// The script object part (types 4, 5, 9): its timer, action and suspended flag, or a blank.
#define LE_PUT_SCRIPT()                                                                 \
    if (script_object >= 0) {                                                           \
        do {                                                                            \
            LE_PUT(int, script_objects[script_object].timer.GetRemainingTime());        \
            LE_PUT(int, script_objects[script_object].cur_action);                      \
            LE_PUT_BOOL(script_objects[script_object].suspended == 1);                  \
        } while (0);                                                                    \
    } else {                                                                            \
        do {                                                                            \
            LE_PUT(int, 0);                                                             \
            LE_PUT(int, -1);                                                            \
            LE_PUT_BOOL(false);                                                         \
        } while (0);                                                                    \
    }

// 0x457150  Confidence: likely (statement order and widths are the code's; the macro shapes are ours,
// see above). Frame: p -4, b -8, sort_order -0x50 (18), ofs -0x54, h -0x58, i -0x5c, the per-put copies
// -0x60 .. -0x1e8, this -0x1ec, the switch values -0x1f0 .. -0x1fc. Writes the LENT record of one living
// entity (savegame_write_living) and returns its length: a 32-byte header (type, pos, heading, layer,
// object flags, flags), then by type: 4 / 5 / 9 statuses, level, HP, base max HP, AP, base max AP, prop
// tag, the AI state and the script object; 7 (party member) statuses, the item types of equipment slots
// 0-8 (slot 9 is not saved), 36 of the 84 skill cooldowns, the first 36 skill levels, level, xp, HP,
// base max HP, AP, base max AP, prop tag and the AI state; types 6 and 8 nothing. Then the death
// animation byte (0 when alive). Types 5 and 9 share the case-4 code (jump table 0x4587d7).
int Character::save_state(ubyte *out)
{
    float p;
    float b;
    int ofs = 0;
    int sort_order[NUM_STATUS_EFFECTS] = { 0x10, 0xa, 9, 8, 1, 7, 5, 0xb, 2, 0, 3, 4, 6, 0xc, 0xd, 0xe,
                                           0xf, 0x11 };
    float h;
    int i;

    LE_PUT(int, type);
    LE_PUT_VECTOR(pos);
    orient.extract_angles(&p, &b, &h);
    LE_PUT(float, h);
    LE_PUT(int, layer);
    LE_PUT(unsigned int, obj_flags);
    LE_PUT(unsigned int, flags);
    switch (type) {
    case 5:
    case 9: {
        int id;
        int n;

        LE_PUT_STATUSES(n, id);
        LE_PUT(short, level);
        LE_PUT(float, hp);
        LE_PUT(float, base_max_hp);
        LE_PUT(short, ap);
        LE_PUT(short, base_max_ap);
        LE_PUT(int, prop_tag);
        ofs += ai.save(out + ofs);
        LE_PUT_SCRIPT();
        break;
    }
    case 4: {
        int n;
        int id;

        LE_PUT_STATUSES(n, id);
        LE_PUT(short, level);
        LE_PUT(float, hp);
        LE_PUT(float, base_max_hp);
        LE_PUT(short, ap);
        LE_PUT(short, base_max_ap);
        LE_PUT(int, prop_tag);
        ofs += ai.save(out + ofs);
        LE_PUT_SCRIPT();
        break;
    }
    case 7: {
        int n;
        Player *pl;
        int id;

        LE_PUT_STATUSES(n, id);
        for (i = 0; i < 9; i++) {
            if (equipment[i] != NULL)
                LE_PUT(int, equipment[i]->type - item_types);
            else
                LE_PUT(int, -1);
        }
        pl = player_from_handle(handle);
        for (i = 0; i < 36; i++)
            LE_PUT(int, pl->skill_cooldown[i].GetRemainingTime());
        for (i = 0; i < 36; i++) {
            ubyte s = skill[i];

            LE_PUT(ubyte, s);
        }
        LE_PUT(short, level);
        LE_PUT(int, xp);
        LE_PUT(float, hp);
        LE_PUT(float, base_max_hp);
        LE_PUT(short, ap);
        LE_PUT(short, base_max_ap);
        LE_PUT(int, prop_tag);
        ofs += ai.save(out + ofs);
        break;
    }
    }
    if (flags & CF_DEAD)
        LE_PUT(ubyte, (ubyte)death_move_sub);
    else
        LE_PUT(ubyte, 0);
    return ofs;
}

// 0x4587f0  Confidence: exact.
int character_save_state_size()
{
    return 0x17c;
}

// One status record read back (load_state). A remaining time of MAX_TIME (an invalid timer's) leaves
// the timer invalid; the ice coffin (16) gets its effect re-attached (spells[0x24].misc_effect).
#define LE_GET_STATUSES(id, fx)                                                         \
    for (i = 0; i < 4; i++) {                                                           \
        LE_GET(ubyte, b);                                                               \
        if (b == 0 || b == 2) {                                                         \
            LE_GET(ubyte, id);                                                          \
            status[id].kind = b;                                                        \
            LE_GET(ubyte, b);                                                           \
            status[id].spell = b;                                                       \
            LE_GET(int, v);                                                             \
            ASSIGN_MAX(v, v, 0);                                                              \
            if (v == MAX_TIME)                                                          \
                status[id].timer.invalidate();                                          \
            else                                                                        \
                status[id].timer.SetTimeout(v);                                         \
            LE_GET(int, v);                                                             \
            ASSIGN_MAX(v, v, 0);                                                              \
            if (v == MAX_TIME)                                                          \
                status[id].tick_timer.invalidate();                                     \
            else                                                                        \
                status[id].tick_timer.SetTimeout(v);                                    \
            LE_GET(int, status[id].tick_ms);                                            \
            LE_GET(float, status[id].value);                                            \
            if (id == STATUS_ICE_COFFIN) {                                              \
                fx = fx_attach_to_character(spells[0x24].misc_effect, handle, -1, -1, 1, 1); \
                status_set_8(STATUS_ICE_COFFIN, fx);                                    \
            }                                                                           \
        } else {                                                                        \
            LE_GET(ubyte, b);                                                           \
            LE_GET(ubyte, b);                                                           \
            LE_GET(int, v);                                                             \
            LE_GET(int, v);                                                             \
            LE_GET(int, v);                                                             \
            LE_GET(float, h);                                                           \
        }                                                                               \
    }

#define LE_GET_SCRIPT(dummy)                                                            \
    if (script_object >= 0) {                                                           \
        do {                                                                            \
            LE_GET(int, v);                                                             \
            if (v == MAX_TIME)                                                          \
                script_objects[script_object].timer.invalidate();                       \
            else                                                                        \
                script_objects[script_object].timer.SetTimeout(v);                      \
            LE_GET(int, script_objects[script_object].cur_action);                      \
            LE_GET_BOOL(script_objects[script_object].suspended);                       \
        } while (0);                                                                    \
    } else {                                                                            \
        do {                                                                            \
            LE_GET(int, v);                                                             \
            LE_GET(int, v);                                                             \
            LE_GET_BOOL(dummy);                                                         \
        } while (0);                                                                    \
    }

// 0x458800  Confidence: likely (as save_state). Frame: dummy -4 (bool), b -8, h -0xc, ofs -0x10, i -0x14,
// v -0x18, this -0xa8, the switch value -0xac. The reverse of save_state; type 4 also recomputes xp
// from the level (types 5 / 9 do not). A party member's empty slots are emptied (except the weapon slot
// 5), the stats are rescaled before HP / AP are read (scale_stats_to_level), species 0xce sets
// attachments[0].slot = 1 (init stores num_attachments = 1 and slot 4: likely meant as the count;
// NOTES.md). Dead characters get their death pose (type 4: death move category 8 first, then 6, then
// move 0x22), lose their equipment unless they are party members, and go to the inactive list.
int Character::load_state(const ubyte *in)
{
    bool dummy;
    ubyte b;
    float h;
    int ofs;
    int i;
    int v;

    ofs = 0;
    LE_GET(int, v);
    type = v;
    LE_GET_VECTOR(pos);
    LE_GET(float, h);
    orient.set_from_angles(0.0f, 0.0f, h);
    last_pos = pos;
    last_orient = orient;
    LE_GET(int, layer);
    LE_GET(int, v);
    obj_flags = v;
    LE_GET(int, v);
    flags = v;
    switch (type) {
    case 5:
    case 9: {
        ubyte id;
        int fx;

        LE_GET_STATUSES(id, fx);
        LE_GET(short, level);
        scale_stats_to_level();
        LE_GET(float, hp);
        LE_GET(float, base_max_hp);
        LE_GET(short, ap);
        LE_GET(short, base_max_ap);
        LE_GET(int, prop_tag);
        ofs += ai.load(in + ofs);
        LE_GET_SCRIPT(dummy);
        break;
    }
    case 4: {
        ubyte id;
        int fx;

        LE_GET_STATUSES(id, fx);
        LE_GET(short, level);
        xp = xp_for_level(level);
        scale_stats_to_level();
        LE_GET(float, hp);
        LE_GET(float, base_max_hp);
        LE_GET(short, ap);
        LE_GET(short, base_max_ap);
        LE_GET(int, prop_tag);
        ofs += ai.load(in + ofs);
        LE_GET_SCRIPT(dummy);
        break;
    }
    case 7: {
        Player *pl;
        ubyte id;
        int fx;
        item_info *old_b;
        item_info *old_a;

        LE_GET_STATUSES(id, fx);
        for (i = 0; i < 9; i++) {
            LE_GET(int, v);
            printf("%d %s\n", i, v >= 0 ? item_types[v].name : "nothing");
            if (v >= 0) {
                character_equip_item(&old_a, &old_b, this, i, item_create(v));
                if (old_a != NULL)
                    item_free(old_a);
                if (old_b != NULL)
                    item_free(old_b);
            } else if (i != 5) {
                old_a = character_unequip_slot(this, i, 1);
                if (old_a != NULL)
                    item_free(old_a);
            }
        }
        pl = player_from_handle(handle);
        for (i = 0; i < 36; i++) {
            LE_GET(int, v);
            if (v == MAX_TIME)
                pl->skill_cooldown[i].invalidate();
            else
                pl->skill_cooldown[i].SetTimeout(v);
        }
        for (i = 0; i < 36; i++) {
            LE_GET(ubyte, b);
            skill[i] = b;
        }
        LE_GET(short, level);
        LE_GET(int, xp);
        LE_GET(float, hp);
        LE_GET(float, base_max_hp);
        LE_GET(short, ap);
        LE_GET(short, base_max_ap);
        LE_GET(int, prop_tag);
        ofs += ai.load(in + ofs);
        if (info->species == 0xce)
            attachments[0].slot = 1;
        break;
    }
    }
    LE_GET(ubyte, b);
    if (flags & CF_DEAD) {
        float len;
        item_info *it;

        death_move_sub = b;
        if (type == 4) {
            death_move = find_move(8, death_move_sub);
            if (death_move == NULL)
                death_move = find_move(6, death_move_sub);
        } else {
            death_move = find_move(6, death_move_sub);
        }
        if (death_move == NULL)
            death_move = find_move(6, 0x22);
        if (death_move == NULL)
            obj_flags &= ~1;
        if (death_move != NULL) {
            len = instance->get_anim_length(death_move->anim);
            instance->start_anim(death_move->anim, 1.0f, true, 1.0f);
            instance->process(len, false, 1);
        }
        if (type != 7) {
            for (i = 0; i < 9; i++) {
                it = character_unequip_slot(this, i, 1);
                if (it != NULL)
                    item_free(it);
            }
        }
    }
    if ((flags & 0x2000000) && info->species == SPECIES_JOSEPH)
        set_armour_alt_texture(1, 1);
    if (flags & CF_DEAD) {
        live_list_remove(this);
        live_list_append(&living_list_inactive, this);
    }
    return ofs;
}

// 0x45a120  Confidence: likely. One profile label (EH state 0). Frame: i -0x10, walk -0x14, set -0x28
// (count, two anims, two weights: the record boss_anim_remap_in / _out rewrite), ready -0x30, rider
// -0x34, n -0x38, idle -0x3c, stand -0x40, run -0x44, hide -0x48, two_stances -0x4c, this -0x50. Picks
// the stance animations (cloaked, badly hurt, combat ready CF 0x1 or relaxed) and blends idle / walk /
// run by loco_blend (0..0.5 idle-walk, 0.5..1 walk-run; walk alone when there is no run animation).
// Tripped (AI disabled 0x2d / 0x2e) shows the lie-down animation; hiding blends hide_ready and ready
// by stance_blend.
void Character::update_locomotion_anim()
{
    profile_label label("character_update_locomotion_anim");
    int i;
    int walk;
    loco_blend_set set;
    int ready;
    Character *rider;
    int n;
    int idle;
    int stand;
    int run;
    int hide;
    bool two_stances;

    if (flags & CF_HIDDEN)
        return;
    instance->clear_state_anims();
    n = 0;
    if (ai.is_mode(AI_STATE_DISABLED, 0x2d) == 1 || ai.is_mode(AI_STATE_DISABLED, 0x2e)) {
        if (anim_tripped_liedown != -1)
            instance->set_state_weight(anim_tripped_liedown, 1.0f);
        return;
    }
    if (flags & 0x400) {
        ready = anim_cloaked_ready;
        walk = anim_cloaked_sneak;
        stand = anim_ready;
        idle = ready;
        run = -1;
    } else if (is_badly_hurt()) {
        if (flags & CF_COMBAT_READY) {
            stand = anim_stand_hurt;
            ready = anim_ready_hurt;
            walk = anim_walk_hurt;
            run = anim_run_hurt;
            idle = ready;
        } else {
            stand = anim_stand_hurt;
            ready = anim_ready_hurt;
            walk = anim_norm_walk_hurt;
            run = anim_norm_run_hurt;
            idle = stand;
        }
    } else if (flags & CF_COMBAT_READY) {
        stand = anim_stand;
        ready = anim_ready;
        walk = anim_walk;
        run = anim_run;
        idle = ready;
    } else {
        stand = anim_stand;
        ready = anim_ready;
        walk = anim_norm_walk;
        run = anim_norm_run;
        idle = stand;
    }
    if (ai.is_mode(AI_STATE_HIDING, -1) == 1) {
        hide = anim_hide_ready;
        if (hide != -1) {
            instance->set_state_weight(hide, 1.0f - stance_blend);
            instance->set_state_weight(ready, stance_blend);
        }
        return;
    }
    walk_speed = instance->get_anim_root_speed(walk);
    if (run < 0)
        run_speed = walk_speed;
    else
        run_speed = instance->get_anim_root_speed(run);
    if (loco_blend < 0.0005f) {
        two_stances = false;
        if (ai.is_combat_e_or_sneak_attack() && stand != ready)
            two_stances = true;
        if (two_stances) {
            set.anim[n] = stand;
            set.weight[n] = stance_blend;
            n++;
            set.anim[n] = ready;
            set.weight[n] = 1.0f - stance_blend;
            n++;
        } else {
            set.anim[n] = idle;
            set.weight[n] = 1.0f;
            n++;
        }
    } else if (run < 0) {
        if (loco_blend > 1.0f)
            printf("%s %f\n", name, loco_blend);
        set.anim[n] = idle;
        set.weight[n] = 1.0f - loco_blend;
        n++;
        set.anim[n] = walk;
        set.weight[n] = loco_blend;
        n++;
    } else if (loco_blend < 0.5f) {
        set.anim[n] = idle;
        set.weight[n] = 1.0f - loco_blend * 2.0f;
        n++;
        set.anim[n] = walk;
        set.weight[n] = loco_blend * 2.0f;
        n++;
    } else if (loco_blend < 1.0f) {
        set.anim[n] = walk;
        set.weight[n] = 1.0f - (loco_blend - 0.5f) * 2.0f;
        n++;
        set.anim[n] = run;
        set.weight[n] = (loco_blend - 0.5f) * 2.0f;
        n++;
    } else {
        set.anim[n] = run;
        set.weight[n] = 1.0f;
        n++;
    }
    set.count = n;
    rider = get_rider();
    if (rider != NULL)
        rider->boss_anim_remap_in((int *)&set);
    boss_anim_remap_out((int *)&set);
    for (i = 0; i < set.count; i++)
        instance->set_state_weight(set.anim[i], set.weight[i]);
}

// 0x45a6e0  Confidence: exact (d at ebp-4, 16 unused bytes, the block's diff at -0x18, this -0x1c; the
// first case returns, D8707). Maps a speed to
// loco_blend: up to the walk speed 0..0.5 (0..1 when walk and run speeds are within 0.01), above it
// 0.5..1 between walk and run speed.
void Character::set_locomotion_speed(float speed)
{
    float d;
    float spare[4];     // unused: the binary's frame is 0x1c (D8707)

    if (run_speed - 0.01f <= speed)
        speed = run_speed;
    if (speed <= walk_speed) {
        float diff;

        diff = walk_speed - run_speed;
        if (diff < 0.01f && diff > -0.01f) {
            if (walk_speed <= 0.0f)
                loco_blend = 0.0f;
            else
                loco_blend = speed / walk_speed;
            return;
        }
        if (walk_speed <= 0.0f)
            loco_blend = 0.0f;
        else
            loco_blend = speed / walk_speed / 2.0f;
    } else {
        speed -= walk_speed;
        d = run_speed - walk_speed;
        if (d <= 0.0f)
            loco_blend = 0.0f;
        else
            loco_blend = speed / d / 2.0f + 0.5f;
    }
}

// 0x45a840  Confidence: likely (rate -4, len -8, vec -0x14, dst -0x18, the returned reference
// -0x34, this -0x38). v^2 / (2 * deceleration) at the current root speed; the root velocity is
// copied to out_vel when given.
float Character::stopping_distance(vector3 *out_vel)
{
    float rate;
    vector3 vec;
    float dst;
    float len;

    if (speed_scale == 0.0f)
        return 0.0f;
    rate = slowdown_rate * speed_scale;
    vec = instance->get_root_velocity();
    len = vec.len();
    dst = len * len / (rate * 2.0f);
    if (out_vel != NULL)
        *out_vel = vec;
    return dst;
}

// 0x45a930  Confidence: likely (a -4, want_speed -8, t -0xc, vmag -0x10, vec -0x1c, this -0x34). The
// distance covered while the root speed changes from now to the speed at `blend` (speed-up rate of
// the char_info, or the slow-down rate).
float Character::distance_to_reach_blend(float blend)
{
    float a;
    float want_speed;
    float t;
    float vmag;
    vector3 vec;

    vec = instance->get_root_velocity();
    vmag = vec.len();
    want_speed = anim_speed_at_blend(blend);
    if (want_speed > vmag)
        a = info->speedup_rate * speed_scale;
    else
        a = -slowdown_rate * speed_scale;
    t = (want_speed - vmag) / a;
    return vmag * t + a * t * t / 2.0f;
}

// 0x45aa20  Confidence: likely (as distance_to_reach_blend with the target speed given).
float Character::distance_to_reach_speed(float speed)
{
    float a;
    float t;
    float vmag;
    vector3 vec;

    vec = instance->get_root_velocity();
    vmag = vec.len();
    if (speed > vmag)
        a = info->speedup_rate * speed_scale;
    else
        a = -slowdown_rate * speed_scale;
    t = (speed - vmag) / a;
    return vmag * t + a * t * t / 2.0f;
}

// 0x45ab00  Confidence: likely for the flow, unsure for the temporaries (the inline budget runs out
// in this function: the later vector3 constructors and finally operator+ / operator- are called out of
// line, the last two through the COMDATs 0x45b680 / 0x45b6b0 emitted here). One profile label (EH
// state 0). Frame: target -0x18, moved -0x1c, slid -0x28, ok -0x2c, ok2 -0x30, ahead -0x40, bone_ok
// -0x44, bone_target -0x50, bone_pos -0x5c, bone_orient -0x80, step -0x8c, bone_slid -0x98, dest -0xa4,
// fix -0xb0, fixed -0xbc, slid3 -0xc8, check -0xd4, this -0x2b0.
// Clips a move to the navigation mesh. Characters with char_info flag 4 and a navigation bone
// (+0x25c) also keep that bone on the mesh: the body slides from pos towards pos + 2 * move (target +
// move, as the code reads), then the bone's slide correction is added when the corrected position is
// navigable. Returns true when the move was changed.
bool Character::clip_move_to_navmesh(vector3 *move)
{
    profile_label label("character_clip_move_to_navmesh");
    vector3 target;
    bool moved;
    vector3 slid;
    bool ok;
    bool ok2;
    vector3 ahead;
    bool bone_ok;
    vector3 bone_target;
    vector3 bone_pos;
    matrix bone_orient;
    vector3 step;
    vector3 bone_slid;
    vector3 dest;

    moved = false;
    if (flags & 0x1000)
        return false;
    target = pos + *move;
    if ((info->flags & 4) && info->head_bone != -1) {
        instance->get_bone_world_transform(info->head_bone, &last_orient, &pos, &bone_orient, &bone_pos);
        bone_ok = pf_point_is_navigable(&bone_pos, info->size, layer);
        ahead = target + *move;
        ok = pf_slide_move(&pos, &ahead, &slid, info->size, layer);
        if (ok) {
            *move = slid - pos;
            moved = true;
            if (!pf_point_is_navigable(&slid, info->size, layer)) {
                pf_find_nearest_navigable_point(&slid, info->size, layer);
                *move = slid - pos;
            }
        }
        if (bone_ok) {
            step = pos + *move;
            instance->get_bone_world_transform(info->head_bone, &orient, &step, &bone_orient, &bone_target);
            ok2 = pf_slide_move(&bone_pos, &bone_target, &bone_slid, info->size, layer);
            if (ok2) {
                vector3 slid3;

                moved = true;
                vector3 fix(bone_slid.x - bone_target.x, bone_slid.y - bone_target.y,
                            bone_slid.z - bone_target.z);
                vector3 fixed(fix.x + move->x, fix.y + move->y, fix.z + move->z);
                dest = pos + fixed;
                ok = pf_slide_move(&pos, &dest, &slid3, info->size, layer);
                if (!ok) {
                    vector3 check;

                    check = fixed + pos;
                    if (pf_point_is_navigable(&check, info->size, layer))
                        *move = fixed;
                }
            }
        }
    } else {
        vector3 to;

        to = pos + *move;
        ok = pf_slide_move(&pos, &to, &slid, info->size, layer);
        if (ok) {
            vector3 d = slid - pos;
            vector3 end;

            moved = true;
            end = d + pos;
            if (!pf_point_is_navigable(&end, info->size, layer)) {
                vector3 near_pt = d + pos;

                pf_find_nearest_navigable_point(&near_pt, info->size, layer);
                *move = near_pt - pos;
            } else {
                *move = d;
            }
        } else {
            vector3 p = pos + *move;

            if (pf_find_nearest_navigable_point(&p, info->size, layer) == 1) {
                moved = true;
                *move = p - pos;
            }
        }
    }
    return moved;
}

// 0x45b680 vector3::operator+(const vector3 &) const (vsdk/math/vector.h): the COMDAT of the inline
// operator, emitted after its first out-of-line use (character_clip_move_to_navmesh, above). The code is
// optimised (no ebp frame, z first) although this part of the unit is built /Od (D5505). Confidence:
// exact (compiler-generated from the header).

// 0x45b6b0 vector3::operator-(const vector3 &) const: the COMDAT as above. Confidence: exact
// (compiler-generated from the header).

// 0x45b6e0  Confidence: exact (D8707: locals named for their D6101 buckets, the expiry code inside the
// `case 0: case 2:` of the kind switch, the magic clamp in one expression). Frame: invis -4, n_active
// -8, some -0xc, s -0x10, petrified -0x14, v -0x18, res -0x1c, res_magic -0x20, src -0x24, aura -0x28,
// me -0x2c, this -0x30, the switch values -0x34 .. -0x40. Per frame: the item statuses (not on a
// multiplayer client), then per active status the tint, the effect handle, the periodic effect (regen /
// poison add the value as HP, firewall / blizzard deal
// it resisted by the spell's damage type and by magic, the dark wall casts spells 7, 8 and 0x17 on the
// character: through spell_cast_execute in multiplayer), and the expiry. The ice coffin (16) ends when
// its value (the coffin's HP) is used up. Invisibility (8) fades the character (instance +0xb4 = 20)
// unless an effect of kind 5 / 6 is attached; paralysis (10) shows the stone effect.
void Character::update_status_effects()
{
    int n_active;
    bool invis;
    bool some;
    int s;
    float res;
    float v;
    bool petrified;
    float res_magic;
    Character *src;
    effect_object *aura;
    Character *me;

    some = false;
    if (!MP_IS_CLIENT())
        refresh_item_status_effects();
    if (game_flags & GF_CUTSCENE)
        return;
    n_active = 0;
    for (s = 0; s < NUM_STATUS_EFFECTS; s++) {
        if (status[s].kind == -1)
            continue;
        some = true;
        switch (s) {
        case STATUS_BLIND:
            instance->set_tint(0, 0, 0);
            break;
        case STATUS_SILENCE:
            instance->set_tint(0x80, 0, 0x80);
            break;
        case STATUS_FAITH:
            instance->set_tint(0xff, 0xff, 0xff);
            break;
        case STATUS_CURSE:
            instance->set_tint(0, 0, 0xff);
            break;
        case STATUS_BLESS:
            instance->set_tint(0xff, 0xff, 0xff);
            break;
        case STATUS_REGEN:
            instance->set_tint(0xff, 0xff, 0);
            break;
        case STATUS_POWER:
            instance->set_tint(0xff, 0, 0);
            break;
        case STATUS_SLEEP:
            instance->set_tint(0, 0xc8, 0xc8);
            break;
        case STATUS_POISON:
            instance->set_tint(0, 0xff, 0);
            break;
        case STATUS_CRIPPLE:
            instance->set_tint(0x80, 0x37, 0);
            break;
        case STATUS_SLOW:
            instance->set_tint(0x80, 0x37, 0);
            break;
        }
        if (status[s].fx != -1 && fx_is_finished(status[s].fx, 0))
            status[s].fx = -1;
        if (status[s].kind == 1 || status[s].timer.valid()) {
            if (!MP_IS_CLIENT() && status[s].tick_timer.elapsed() && status[s].tick_ms > 0) {
                switch (s) {
                case STATUS_REGEN:
                case STATUS_POISON:
                    add_hp(status[s].value, status[s].source, 0, false);
                    break;
                case STATUS_FIREWALL:
                case STATUS_BLIZZARD:
                    v = status[s].value;
                    if (!No_resist) {
                        if (v < 0.0f) {
                            res = get_resistance(spells[status[s].spell].damage_type);
                            v = (1.0f - res) * v;
                        }
                        if (v < 0.0f) {
                            res_magic = get_resistance(3);
                            if ((v = (1.0f - res_magic) * v) < 0.0f)
                                v = v;
                            else
                                v = 0.0f;
                        }
                    }
                    add_hp(v, status[s].source, 0, false);
                    break;
                case STATUS_DARK_WALL:
                    if (game_flags & GF_MULTIPLAYER) {
                        src = living_from_handle(status[s].source);
                        spell_cast_execute(src, 7, handle, NULL, -1, 0, 0, 0);
                        spell_cast_execute(src, 8, handle, NULL, -1, 0, 0, 0);
                        spell_cast_execute(src, 0x17, handle, NULL, -1, 0, 0, 0);
                    } else {
                        get_hit_by_spell(7, status[s].source, 0, 0, 8, 0, 0, 0);
                        get_hit_by_spell(8, status[s].source, 0, 0, 8, 0, 0, 0);
                        get_hit_by_spell(0x17, status[s].source, 0, 0, 8, 0, 0, 0);
                    }
                    break;
                }
                status[s].tick_timer.SetTimeout(status[s].tick_ms);
            }
            switch (status[s].kind) {
            case 0:
            case 2:
                if (s == STATUS_ICE_COFFIN) {
                    if (!MP_IS_CLIENT() && status[s].timer.valid() && status[s].value <= 0.0f)
                        status_effect_cure(s, true, true);
                    else
                        n_active++;
                } else if (!MP_IS_CLIENT() && status[s].timer.elapsed()) {
                    status_effect_cure(s, true, true);
                    if (s == STATUS_SILENCE)
                        ai.flags &= ~0x80;
                } else {
                    switch (s) {
                    case STATUS_INVISIBLE:
                    case STATUS_PARALYZE:
                    case STATUS_FIREWALL:
                    case STATUS_BLIZZARD:
                        break;
                    default:
                        n_active++;
                        break;
                    }
                }
                break;
            }
        }
    }
    petrified = status[STATUS_PARALYZE].kind != -1;
    invis = status[STATUS_INVISIBLE].kind != -1;
    if (!petrified && invis) {
        for (aura = fx_active_list.fx_next; aura != &fx_active_list; aura = aura->fx_next) {
            if ((aura->kind == 6 || aura->kind == 5) && aura->owner == handle)
                break;
        }
        if (aura == &fx_active_list)
            instance->_unkb4 = 0x14;
    } else if (!(flags & 0x400) && !ai.is_mode(AI_STATE_HIDING, -1)) {
        instance->_unkb4 = 0xff;
    }
    if (petrified) {
        instance->set_tint(0xff, 0xff, 0xff);
        me = living_from_handle(handle);
        if (me == NULL || !(me->flags & 0x80000))
            status_stone_fx(handle);
    } else {
        fx_stop_status_aura(handle);
    }
    if (n_active <= 0)
        fx_stop_status_effects(handle);
    else if (!petrified && !invis)
        fx_get_status_holder(handle);
}

// The AP clamp of character_update_regen_and_status evaluates its arguments right to left into two
// short temporaries (ebp-0x2c, -0x28) and a third for the result (-0x30): an inline function, not the
// min macro (which would call get_max_stat twice). Name ours.
inline short short_min(short a, short b)
{
    return a < b ? a : b;
}

// 0x45bf70  Confidence: likely. One profile label (EH state 0). Frame: dt -0x10, regen -0x18, b -0x1c,
// r -0x20, g -0x24, mult -0x38, this -0x34. Sleeping, paralyzed or frozen characters do not animate and
// stay inactive. AP regenerates every 7.5 s: max AP * AP regen multiplier * 5%, at least 1, doubled in
// multiplayer. The light level is the floor's light map sample, averaged, / 255 * 0.55.
void Character::update_regen_and_status()
{
    profile_label label("character_update_regen_and_status");
    float dt;
    int regen;
    ubyte b;
    ubyte r;
    ubyte g;
    float mult;

    if (!has_status(STATUS_PARALYZE) && !has_status(STATUS_SLEEP) && !has_status(STATUS_ICE_COFFIN)) {
        process_anim(frametime);
    } else {
        process_anim(0.0f);
        flags |= CF_SUMMONLIKE;
        ai.go_inactive();
    }
    if (frametime > 0.0f)
        dt = frametime;
    else
        dt = reported_frametime;
    hp_text_timer -= dt;
    if (hp_text_timer < 0.0f)
        hp_text_timer = 0.0f;
    pos_mod_text_timer -= dt;
    if (pos_mod_text_timer < 0.0f)
        pos_mod_text_timer = 0.0f;
    float_text_scale -= dt;
    if (float_text_scale < 0.0f)
        float_text_scale = 0.0f;
    if (cur_anim == -1 || !move_playing(cur_anim, NULL)) {
        if (flags & 0x4)
            clear_flag_2_4();
        cur_anim = -1;
    }
    if (timer2ec.valid() && timer2ec.elapsed()) {
        mult = ap_regen_mult() * 0.05f;
        regen = (int)(get_max_stat(1) * mult);
        ASSIGN_MAX(regen, 1, regen);
        if (game_flags & GF_MULTIPLAYER)
            regen *= 2;
        ap += regen;
        ap = short_min(ap, (short)get_max_stat(1));
        timer2ec.SetTimeout(7500);
    }
    update_status_effects();
    if (!(flags & CF_DEAD) && ground_lobj >= 0 && ground_lobj < num_lobjs && ground_face >= 0) {
        lighting_sample_stub(ground_lobj, ground_face, ground_hit, &r, &g, &b);
        light_level = (r + g + b) / 3.0f;
        light_level = light_level / 255.0f;
        light_level = light_level * 0.55f;
    }
}

// 0x45c340  Confidence: likely. One profile label (EH state 0). Frame: h -0x48, result -0x44, snap -0x3c,
// b -0x38, speed -0x34, move -0x30, turn -0x24, len -0x14, p -0x10, old_pos -0x60, v -0x54, seen -0x6c,
// t -0x78, this -0x1ac. One AI-driven step: the root speed eases towards target_speed (speed-up /
// slow-down rates), the locomotion blend follows, the animation's root motion and turn are applied, a
// plane normal (when given) projects the step, other characters push it aside (result -3), and the
// navigation mesh clips it. -4 when disabled (CF 0x2). The floor is searched again after a move of more
// than 1 cm by a non-local or AI-flag-8 character, or when standing with object flag 2 and not 8.
int Character::ai_move_frame(int unused, vector3 *plane_normal)
{
    profile_label label("character_ai_move_frame");
    float h;
    int result;
    bool snap;
    float b;
    float speed;
    vector3 move;
    quaternion turn;
    float len;
    float p;
    vector3 v;
    vector3 seen;
    vector3 t;

    seen = v = instance->sum_root_velocity();
    speed = v.len();
    if (speed < target_speed) {
        speed += info->speedup_rate * speed_scale * frametime;
        if (speed > target_speed)
            speed = target_speed;
    }
    if (speed > target_speed) {
        speed -= slowdown_rate * speed_scale * frametime;
        if (speed < target_speed)
            speed = target_speed;
    }
    slowdown_rate = info->slowdown_rate;
    set_locomotion_speed(speed);
    update_locomotion_anim();
    move = instance->get_motion_delta(frametime);
    v = move / frametime;
    forward_speed = speed;
    move.unrotate(orient);
    turn = instance->take_turn_delta(frametime);
    orient.extract_angles(&p, &b, &h);
    p = 0.0f;
    b = 0.0f;
    orient.set_from_angles(p, b, h + turn.w);
    centre_offset = instance->get_root_offset(frametime);
    centre_offset.unrotate(orient);
    centre_offset.y = 0.0f;
    if (plane_normal != NULL) {
        t = move + centre_offset;
        t = *plane_normal * (t.x * plane_normal->x + t.y * plane_normal->y + t.z * plane_normal->z);
        move = t - centre_offset;
    }
    result = 0;
    if (find_overlap(&move, unused, true))
        result = -3;
    vector3 old_pos = pos;
    if (result == 0 && (!move.is_zero() || (info->flags & 4))) {
        clip_move_to_navmesh(&move);
        layer = pf_trace_edge_crossings(&pos, layer, move + pos);
        pos += move;
    }
    len = 0.0f;
    if (!move.is_zero()) {
        velocity = move / frametime;
        len = move.len();
    } else {
        velocity.zero();
    }
    if (flags & CF_SUMMONLIKE)
        result = -4;
    snap = false;
    if (len > 0.01f) {
        if ((character_is_locally_controlled(this) && (obj_flags & 2)) || (ai.flags & 8))
            snap = true;
    } else if (!(obj_flags & 8) && (obj_flags & 2)) {
        snap = true;
    }
    if (snap == 1)
        find_ground();
    update_regen_and_status();
    frametime = 0.0f;
    return result;
}

// 0x45ccb0  Confidence: exact. cdecl (the character is the argument). Characters flagged 0x40000 or
// placed at a navpoint stop rendering (object flag 1 cleared); the others get object flag 4.
void character_update_render_flag(Character *c)
{
    if ((c->flags & 0x40000) || c->start_navpoint != NULL)
        c->obj_flags &= ~1;
    else
        c->obj_flags |= 4;
}

// 0x45cd00 living_entity_move_frame  Confidence: likely. Frame: move -0xc, left -0x1c, frac -0x20, m
// -0x44, da -0x48, da2 -0x4c, m2 -0x70, fx -0x74, len -0x160, this -0x164. The frame of a dying or dead
// character: the corpse leans towards update_head_look's angle over the death animation; once the
// animation is over (and CF 0x8000), its effect or the render flag decides whether it stays visible;
// otherwise the death animation's root motion moves it on the navigation mesh.
void Character::move_frame()
{
    vector3 move;
    float left;
    float frac;
    matrix m;
    float da;
    float da2;
    matrix m2;
    obj *fx;
    float len;

    left = 0.0f;
    if (death_move != NULL)
        left = instance->get_anim_time_left(death_move->anim);
    update_head_look();
    process_anim(reported_frametime);
    if (instance != NULL && death_move != NULL) {
        if (left > 0.0001f) {
            frac = (instance->get_anim_length(death_move->anim) - left) * move_turn_angle
                 / instance->get_anim_length(death_move->anim);
            da = frac - move_turn_applied;
            move_turn_applied = frac;
            m.set_from_axis_angle(orient.rvec, da);
            orient = m * orient;
        } else if (move_turn_applied != move_turn_angle) {
            da2 = move_turn_angle - move_turn_applied;
            move_turn_applied = move_turn_angle;
            m2.set_from_axis_angle(orient.rvec, da2);
            orient = m2 * orient;
        }
    }
    if ((flags & 0x8000) && left < 0.0001f) {
        if (move_fx != -1) {
            fx = obj_from_handle(move_fx);
            if (fx == NULL || ((effect_object *)fx)->instance->get_time_left() < 0.0001f) {
                character_update_render_flag(this);
            } else {
                obj_flags &= ~1;
                instance->clear_action_anims();
                instance->clear_state_anims();
            }
        } else {
            character_update_render_flag(this);
        }
        hp_text_timer -= reported_frametime;
        if (hp_text_timer < 0.0f)
            hp_text_timer = 0.0f;
        pos_mod_text_timer -= reported_frametime;
        if (pos_mod_text_timer < 0.0f)
            pos_mod_text_timer = 0.0f;
        float_text_scale -= reported_frametime;
        if (float_text_scale < 0.0f)
            float_text_scale = 0.0f;
        return;
    }
    update_regen_and_status();
    move = instance->get_motion_delta(reported_frametime);
    move.unrotate(orient);
    if (left > 0.0001f) {
        clip_move_to_navmesh(&move);
        layer = pf_trace_edge_crossings(&pos, layer, move + pos);
    } else {
        if (flags & 0x40000) {
            switch (info->species) {
            case 0xbd:
            case 0xbe:
            case 0xcc:
                obj_flags &= ~1;
                break;
            }
        }
        move.zero();
    }
    pos += move;
    len = move.len();
    if (len > 0.1f)
        find_ground();
}

// 0x45d4c0  Confidence: exact (left_tag -4, right_tag -8, the ca_character copies -0xc / -0x10 are the
// returns of an inline accessor, written here as the member read; this -0x14).
void Character::find_foot_bones()
{
    int left_tag;
    int right_tag;

    left_tag = 0;
    right_tag = 0;
    left_tag = mesh_tag(instance->character, "BDBN-Foot-L");
    right_tag = mesh_tag(instance->character, "BDBN-Foot-R");
    foot_tag_l = (short)left_tag;
    foot_tag_r = (short)right_tag;
}

// 0x45d540  Confidence: exact. Frame: sample -4, sounds -8, surface -0xc, fmax -0x10, h -0x14, fmin -0x18, frequency
// -0x1c, this -0x20, the ?: temporary -0x24. The left / right footstep table of the char_info, by the
// floor surface (one set, or a random one of several), played at a random 90-100% of its frequency.
void Character::play_footstep(vector3 *at, bool left_foot)
{
    int (*sounds)[5];
    int sample;
    int surface;
    float fmax;
    int h;
    float fmin;
    unsigned int frequency;

    surface = lobj_face_get_surface(ground_lobj, ground_face);
    sounds = left_foot ? info->footstep_sounds_a : info->footstep_sounds_b;
    sample = -1;
    if (info->num_footstep_sets == 1)
        sample = sounds[0][surface];
    else if (info->num_footstep_sets > 1)
        sample = sounds[rand() % info->num_footstep_sets][surface];
    if (sample >= 0) {
        h = sound_play_3d_at(sample, at);
        if (h >= 0) {
            snd_get_frequency(h, &frequency);
            fmin = (float)frequency * 0.9f;
            fmax = (float)frequency;
            snd_set_frequency(h, (int)frand_range(fmin, fmax));
        }
    }
}

// 0x45d690  Confidence: exact (p -0xc, tag_value -0x1c (named for its D6101 bucket, D8707), w -0x18,
// this -0x70; the rest inline temporaries). A foot's animation trigger plays its footstep at the foot
// tag; the left foot first.
void Character::update_footsteps()
{
    vector3 p;
    vector3 w;
    int tag_value;

    if (foot_tag_l != -1) {
        instance->get_tag_pos(&p, &tag_value, foot_tag_l);
        w = p + pos;
        if (instance->anim_trigger_hit("left-foot")) {
            play_footstep(&w, true);
            return;
        }
    }
    if (foot_tag_r != -1) {
        instance->get_tag_pos(&p, &tag_value, foot_tag_r);
        w = p + pos;
        if (instance->anim_trigger_hit("right-foot"))
            play_footstep(&w, false);
    }
}

// 0x45d830  Confidence: exact (ratio -4, style_id -8, wpn -0xc, this -0x10). Spell-casting combat styles
// (4, 5) keep 8 m while they have AP (party members above 25%, others above 5%) and may cast; a
// ranged weapon gives its attack radius. Bosses use their own table.
bool Character::get_attack_range(float *out)
{
    float ratio;
    int style_id;
    item_info *wpn;

    if (type == 9)
        return boss_get_attack_range(this, out);
    style_id = ai.get_combat_style();
    if (!(type == 7 && (ai.flags & AI_FLAG_PLAYER_CONTROL)) && !(ai.flags & 0x80)
        && !has_status(STATUS_SILENCE) && (style_id == 4 || style_id == 5)) {
        ratio = (float)ap / get_max_stat(1);
        if ((type == 7 && ratio > 0.25f) || (type != 7 && ratio > 0.05f)) {
            *out = 8.0f;
            return true;
        }
    }
    wpn = equipment[5];
    if (wpn != NULL && wpn->type->kind == 0 && weapon_is_ranged(&wpn->type->weapon) == 1) {
        *out = wpn->type->attack_radius;
        return true;
    }
    return false;
}

// 0x45d980  Confidence: exact (group -4, this -8, the fild temporary -0xc, the result -0x10).
bool Character::can_cast_spell(int spell)
{
    int group;

    group = spell_get_group(spell);
    if (group == -1)
        return false;
    if (has_status(STATUS_SILENCE) == 1)
        return false;
    if (!find_move(7, spells[spell].variation))
        return false;
    if (spells[spell].cost > ap)
        return false;
    if (spell_on_cooldown(spell))
        return false;
    return get_skill(spell_group_skill[group]) >= spells[spell].skill;
}

// 0x45da60  Confidence: exact (locals named for their D6101 buckets, D8707). Frame: spell_fx -4, fx_id -8,
// mi -0xc, anim -0x10, recovery -0x14, fx_base -0x18, fx_obj -0x1c, vi -0x20, this -0x24, the switch
// value -0x28. Plays the cast move (bosses keep
// their current animation), attaches the cast effect (Pyrul, species 0xc4, at its missile bone),
// launches the missile spells, starts the recovery (the effect's length / 15 plus the +Recovery time:
// field) and takes the AP: the summon spells 0x26-0x35 cost their skill's AP, the others the spell
// cost. Returns the animation or -1.
int Character::perform_spell_cast(int spell, bool arg)
{
    int fx_id;
    const char *spell_fx;
    int anim;
    move_info *mi;
    int recovery;
    effect_object *fx_obj;
    vfx_base *fx_base;
    vfx_instance *vi;

    if (type != 9) {
        mi = find_move(7, spells[spell].variation);
        if (mi == NULL)
            return -1;
        anim = play_move((int)mi, 1.0f, 0, 0, arg);
        if (anim == -1)
            return -1;
    } else {
        anim = cur_anim;
    }
    if (type == 7 && (spells[spell].flags & SPELL_FADE_PROPS))
        character_start_move_fade(this, anim);
    spell_fx = spell_get_cast_effect(spell);
    if (spell_fx != NULL) {
        if (info->species == 0xc4)
            fx_id = fx_attach_to_character(spell_fx, handle, pyrul_missile_bone, anim, 0, 0);
        else
            fx_id = fx_attach_to_character(spell_fx, handle, -1, anim, 0, 0);
        if (fx_id == -1)
            return -1;
        spell_play_cast_sound(spell, this);
        switch (spell) {
        case 0x18:
        case 0x19:
        case 0x1a:
        case 0x20:
        case 0x4d:
        case 0x4e:
        case 0x5b:
            spell_launch_missile(this, fx_id);
            break;
        }
        fx_obj = (effect_object *)obj_from_handle(fx_id);
        if (fx_obj == NULL)
            return -1;
        fx_obj->spell = spell;
        fx_obj->fx_flags |= 0x100;
        vi = fx_obj->instance;
        fx_base = effect_get_base((vi->id_bits >> 9) & 0x7fffff);   // the 23-bit id field, read unoptimised
        if (fx_base == NULL)
            return -1;
        recovery = (int)(fx_base->num_frames / 15.0f);
        start_spell_recovery(spell, recovery, spells[spell].recovery_time);
        cast_effect_track_add(spell, fx_id);
    }
    if (Use_spell_ap && !MP_IS_CLIENT()) {
        if (spell >= 0x26 && spell < 0x36)
            add_ap((float)-skill_table[spell - 2].ap_cost);
        else
            add_ap(-spells[spell].cost);
    }
    return anim;
}

// 0x45dd80  Confidence: exact. cdecl. i at ebp-4, cur -8, dur -0xc. The first free fade record gets
// the character, its move and a rate of half a fade per animation length.
void character_start_move_fade(Character *c, int move)
{
    int i;
    float dur;
    move_info *cur;

    for (i = 0; i < MAX_MOVE_FADES; i++) {
        if (move_fades[i].handle == -1) {
            move_fades[i].handle = c->handle;
            move_fades[i].move = move;
            cur = c->get_current_move();
            dur = c->instance->get_anim_length(cur->anim);
            move_fades[i].rate = 0.5f / dur;
            break;
        }
    }
}

// 0x45de20  Confidence: exact (move -4, this -8). Bosses do not flinch.
void Character::play_flinch(int variant)
{
    move_info *move;

    if (type == 9)
        return;
    move = find_move(4, variant);
    if (move != NULL && play_move((int)move, 1.0f, 1, 1, 0) != -1) {
        set_flag_2_4(-1);
        flags |= 0x4;
        combat.state = COMBAT_IDLE;
    }
}

// The hit effect of a spell on its target: the preloaded effect name, or spell_get_hit_effect's
// (resolved at run time). Every use repeats the two calls; a macro of ours.
#define ATTACH_SPELL_HIT_FX(spell, preloaded)                                                         \
    if (preloaded)                                                                                    \
        fx_attach_to_character(spells[spell].hit_effect, handle, -1, -1, 0, 1);                      \
    else                                                                                              \
        fx_attach_to_character(spell_get_hit_effect(spell), handle, -1, -1, 0, 1)

// 0x45dea0 living_entity_get_hit_by_spell  Confidence: likely for the flow (byte-indexed jump table
// at 0x460115 over the 0x6c spells, bodies in the code's order), unsure for the frame of the many case
// locals. Frame: die -4, flinch -8, chance -0xc, dist -0x10, count -0x14, src -0x18, p -0x1c, magic
// -0x20, amount -0x24, retaliate -0x28, ms -0x2c, show_fx -0x30, status -0x34, use_resist -0x38, level
// -0x3c, resist -0x40, c -0x44, this -0x1a0.
// The spell's level comes from the source: the caster's level (0, 8), its special / reflex ability
// argument (1, 2), the item's triggered / charge effect value (3-5), a combat self-cast (6: a third,
// quarter or half of the level, times 1 + 0.15 per chain hit) or 100 (7). A target resists with chance
// 1 - (1 - (0.5 + magic resistance - 0.04 * level)) * hit chance. The cases set count and die for
// spell_roll_amount; several also store an `amount` that the roll overwrites afterwards (0x3e, 0x40-0x42,
// and 0x66, 0x68, 0x6a, 0x6b, 0x69 on non-party targets, 0xc / 0x54 / 0x57 / 0x65: dead stores, NOTES.md).
// Nothing happens to the dead (except resurrection) or once the whole party is dead.
void Character::get_hit_by_spell(int spell, int source, vector3 *pos, matrix *orient, int source_type,
                                 ItemType *item, float power, bool preloaded)
{
    int die;
    int flinch;
    float chance;
    float dist;
    int count;
    Character *src;
    Player *p;
    float magic;
    float amount;
    bool retaliate;
    int ms;
    bool show_fx;
    int status;
    bool use_resist;
    int level;
    float resist;
    Character *c;

    flinch = 0;
    if (((flags & CF_DEAD) || hp <= 0.0f) && spell != 5 && spell != 3 && spell != 0x69)
        return;
    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (c != NULL && !(c->flags & CF_DEAD))
            break;
    }
    if (p == &party_list)
        return;
    if (serpent_part_on_hit(this))
        return;
    if (type == 9)
        boss_threat_add(this, 0.0f, source, 0);
    if (spell == SPELL_NIGHTMARE) {
        get_hit_by_spell(7, source, pos, orient, source_type, item, power, false);
        get_hit_by_spell(8, source, pos, orient, source_type, item, power, false);
        get_hit_by_spell(0x43, source, pos, orient, source_type, item, power, false);
        if (has_status(STATUS_BLIND) || has_status(STATUS_SILENCE) || has_status(STATUS_SLEEP))
            fx_attach_to_character(spells[7].hit_effect, handle, -1, -1, 0, 1);
        return;
    }
    show_fx = true;
    retaliate = false;
    use_resist = true;
    src = living_from_handle(source);
    if (src == NULL)
        return;
    if ((spells[spell].flags & SPELL_OFFENSIVE) && type == 7 && src->get_team() != 0)
        combat_mode_set(1);
    if (spells[spell].distance > 0.0f) {
        dist = distance(&this->pos, &src->pos);
        if (dist > spells[spell].distance)
            return;
    }
    switch (source_type) {
    case 0:
    case 8:
        level = src->level;
        break;
    case 6:
        if (spell == SPELL_HEAL)
            level = (int)(src->level / 3.0f);
        else if (spell == SPELL_VITALIZE)
            level = (int)(src->level / 4.0f);
        else
            level = (int)(src->level / 2.0f);
        level = (int)(level * (src->combat.chain_count * 0.15f + 1.0f));
        break;
    case 7:
        level = 100;
        break;
    case 1:
        level = src->info->special_arg;
        if (level <= 0)
            level = src->level;
        break;
    case 2:
        level = src->info->reflex_arg;
        if (level == 0)
            level = src->level;
        break;
    case 3:
        printf("Item special\n");
        if (item != NULL && item->weapon_triggered_effect != NULL) {
            printf("Item special item found!\n");
            level = item->weapon_triggered_effect->value;
            if (level == 0) {
                printf("Item special failure 1!\n");
                level = src->level;
            }
        } else {
            printf("Item special failure 2!\n");
            return;
        }
        break;
    case 4:
        if (item != NULL && item->armour_triggered_effect != NULL) {
            level = item->armour_triggered_effect->value;
            if (level == 0)
                level = src->level;
        } else {
            return;
        }
        printf("Reflex abil\n");
        break;
    case 5:
        if (item != NULL && item->charge_effect != NULL) {
            level = item->charge_effect->_unk0c;
            if (level <= 0)
                level = src->level;
        } else {
            return;
        }
        break;
    default:
        return;
    }
    status = damage_type_to_status(spell);
    if (src->info->species == 0xb6)
        chance = src->level * 2.0f / 100.0f;
    else if (spell == 0x50 && ghost_stage == 3)
        chance = 0.5f;
    else if (source_type == 8)
        chance = 0.3f;
    else
        chance = spells[spell].hit_chance;
    magic = get_resistance(3);
    resist = (0.5f + magic) - level * 0.04f;
    resist = 1.0f - (1.0f - resist) * chance;
    count = 0;
    die = 0;
    amount = 0.0f;
    switch (spell) {
    case 0xf:
        retaliate = true;
        count = level;
        if (source_type == 0)
            count = (int)(count * (src->get_skill(SKILL_ENERGY) + 1.0f));
        die = -spells[spell].die;
        flinch = 0x82;
        break;
    case 0:
    case 4:
        count = level;
        die = spells[spell].die;
        break;
    case 1:
        if (level >= 100) {
            status_effect_cure(STATUS_BLIND, true, false);
            status_effect_cure(STATUS_SILENCE, true, false);
            status_effect_cure(STATUS_CURSE, true, false);
            status_effect_cure(STATUS_SLEEP, true, false);
            status_effect_cure(STATUS_PARALYZE, true, false);
            status_effect_cure(STATUS_POISON, true, false);
            status_effect_cure(STATUS_CRIPPLE, true, false);
            status_effect_cure(STATUS_SLOW, true, false);
            status_effect_cure(STATUS_ICE_COFFIN, true, false);
        } else {
            status_effect_cure(STATUS_BLIND, false, false);
            status_effect_cure(STATUS_SILENCE, false, false);
            status_effect_cure(STATUS_CURSE, false, false);
            status_effect_cure(STATUS_SLEEP, false, false);
            status_effect_cure(STATUS_PARALYZE, false, false);
            status_effect_cure(STATUS_POISON, false, false);
            status_effect_cure(STATUS_CRIPPLE, false, false);
            status_effect_cure(STATUS_SLOW, false, false);
            status_effect_cure(STATUS_ICE_COFFIN, false, false);
        }
        break;
    case 0x49:
        if (src != NULL) {
            int paralyze_ms;

            paralyze_ms = 0;
            if (src->info->species == 0x59)
                paralyze_ms = 3000;
            else if (src->info->species == 0x57)
                paralyze_ms = 8000;
            else
                paralyze_ms = 5000;
            status_effect_add(STATUS_PARALYZE, paralyze_ms, spell, 0.0f, src->handle, false);
        }
        return;
    case 2: case 6: case 7: case 8: case 0x10: case 0x13: case 0x15: case 0x16: case 0x17: case 0x22:
    case 0x24: case 0x25: case 0x3c: case 0x3d: case 0x43: case 0x50: case 0x55: case 0x56: case 0x58:
    case 0x59: case 0x5a: {
        float roll;
        float mod;
        int duration;

        show_fx = false;
        if (spell == 0x17 && !(src->flags & 0x800)) {
            vector3 dir;
            matrix m;
            int fx;
            effect_object *fx_obj;

            dir = src->pos - this->pos;
            dir.normalize();
            m.set_from_forward(dir);
            if (preloaded) {
                fx = fx_attach_to_character(spells[0x17].hit_effect, handle, -1, -1, 0, 1);
            } else {
                printf("Preloaded curse\n");
                fx = fx_attach_to_character(spell_get_hit_effect(spell), handle, -1, -1, 0, 1);
            }
            fx_obj = (effect_object *)obj_from_handle_b(fx);
            if (fx_obj != NULL)
                fx_obj->orient = m;
        }
        if (!No_resist && (spells[spell].flags & SPELL_OFFENSIVE)) {
            roll = frand();
            if (roll <= resist) {
                if (spells[spell].hit_effect != NULL && spell != 0x17 && spell != 0x24 && !(src->flags & 0x800)) {
                    ATTACH_SPELL_HIT_FX(spell, preloaded);
                }
                break;
            }
        }
        if (src->info->species == 0xc3)
            duration = (int)(src->level * 3.0f * 1000.0f);
        else if (src->info->species == 0xb6)
            duration = (int)(src->level * 4.0f * 1000.0f);
        else
            duration = spell_get_duration_ms(spell, level);
        mod = status_effect_modifier(status, level);
        if (spell_target_value(this, spell) > 0.01f) {
            if (source_type == 0 && src->ai.mode_target != spell)
                status_effect_add(status, duration, -1, mod, src->handle, source_type == 7);
            else if (preloaded)
                status_effect_add(status, duration, spell, mod, src->handle, source_type == 7);
            else
                status_effect_add(status, duration, -1, mod, src->handle, source_type == 7);
            if ((spells[spell].flags & SPELL_OFFENSIVE) && spell != 0x24)
                flinch = 0x81;
        }
        break;
    }
    case 0x67: {
        int i;
        bool any;
        float roll;
        float mod;
        int sub_spells[5] = { 0x3d, 7, 0x3c, 8, 0x17 };
        int sub_spell;
        int duration;
        int statuses[5] = { STATUS_PARALYZE, STATUS_BLIND, STATUS_POISON, STATUS_SILENCE, STATUS_CURSE };

        show_fx = false;
        any = false;
        for (i = 0; i < 5; i++) {
            sub_spell = sub_spells[i];
            if (!No_resist && (spells[spell].flags & SPELL_OFFENSIVE)) {
                roll = frand();
                if (roll < resist)
                    continue;
            }
            duration = spell_get_duration_ms(sub_spell, level);
            status = statuses[i];
            mod = status_effect_modifier(status, level);
            if (spell_target_value(this, sub_spell) > 0.01f) {
                any = true;
                if (preloaded)
                    status_effect_add(status, duration, sub_spell, mod, src->handle, false);
                else
                    status_effect_add(status, duration, -1, mod, src->handle, false);
            }
        }
        if (any == 1)
            flinch = 0x81;
        break;
    }
    case 0x21:
        die = -spells[spell].die;
        count = level;
        break;
    case 0xb:
        ms = spell_get_duration_ms(0xb, level);
        show_fx = false;
        if (preloaded) {
            if ((game_flags & GF_SINGLE_PLAYER) && level >= 50)
                status_effect_add(status, ms, spell, 1.0f, src->handle, false);
            else
                status_effect_add(status, ms, spell, 0.0f, src->handle, false);
        } else {
            if ((game_flags & GF_SINGLE_PLAYER) && level >= 50)
                status_effect_add(status, ms, -1, 1.0f, src->handle, false);
            else
                status_effect_add(status, ms, -1, 0.0f, src->handle, false);
        }
        break;
    case 0xe:
        ms = spell_get_duration_ms(0xe, level);
        show_fx = false;
        if (preloaded)
            status_effect_add(status, ms, spell, 0.0f, src->handle, false);
        else
            status_effect_add(status, ms, -1, 0.0f, src->handle, false);
        break;
    case 0x19:
    case 0x1a:
    case 0x20:
    case 0x4d:
    case 0x5b: {
        int fx;

        retaliate = true;
        show_fx = false;
        count = level;
        if (source_type == 0) {
            if (spell == SPELL_FIRE_ARROW)
                count = (int)(count * (src->get_skill(SKILL_FIRE) + 1.0f));
            else if (spell == 0x20)
                count = (int)(count * (src->get_skill(SKILL_ICE) + 1.0f));
        }
        die = -spells[spell].die;
        combat.flinch_if_idle(2);
        if (preloaded) {
            fx = fx_add_effect(spells[spell].hit_effect, pos, orient, 1);
            if (spell == SPELL_FIREBALL && fx != -1)
                spell_effect_obj_set_timer(src->handle, fx, pos, layer, &this->orient);
        } else {
            printf("Trying to get_hit_by_spell by an invalid method!!\n");
            fx_add_effect(spell_get_hit_effect(spell), pos, orient, 1);
        }
        break;
    }
    case 0x18:
        if (type == 9 || (flags & 0x40)) {
            int fx;

            retaliate = true;
            show_fx = false;
            count = level;
            die = -spells[spell].die;
            combat.flinch_if_idle(2);
            if (preloaded) {
                fx = fx_add_effect(spells[spell].hit_effect, pos, orient, 1);
                if (spell == SPELL_FIREBALL && fx != -1)
                    spell_effect_obj_set_timer(src->handle, fx, pos, layer, &this->orient);
            } else {
                printf("Trying to get_hit_by_spell by an invalid method!!\n");
                fx_add_effect(spell_get_hit_effect(spell), pos, orient, 1);
            }
        } else {
            float dmg;
            int fx;

            retaliate = true;
            show_fx = false;
            use_resist = false;
            combat.flinch_if_idle(2);
            if (preloaded)
                fx = fx_add_effect(spells[spell].hit_effect, pos, orient, 1);
            else
                fx = fx_add_effect(spell_get_hit_effect(spell), pos, orient, 1);
            dmg = hp * 0.5f;
            printf("jade dmg: %d\n", (double)dmg);
            add_hp(-dmg, source, 1, false);
        }
        break;
    case 0x4e: {
        int fx;

        retaliate = true;
        show_fx = false;
        count = 3;
        die = -spells[spell].die;
        combat.flinch_if_idle(2);
        if (preloaded) {
            fx = fx_add_effect(spells[spell].hit_effect, pos, orient, 1);
            if (fx != -1) {
                spell_effect_obj_set_timer(src->handle, fx, pos, layer, &this->orient);
                ai.knock_down(random_stun_duration());
            }
        } else {
            fx_add_effect(spell_get_hit_effect(spell), pos, orient, 1);
            ai.knock_down(random_stun_duration());
        }
        break;
    }
    case 0x38: {
        int fx;

        retaliate = true;
        show_fx = false;
        count = 3;
        die = -spells[spell].die;
        combat.flinch_if_idle(2);
        if (preloaded) {
            fx = fx_add_effect(spells[spell].hit_effect, pos, orient, 1);
            if (fx != -1)
                spell_effect_obj_set_timer(src->handle, fx, pos, layer, &this->orient);
        } else {
            fx_add_effect(spell_get_hit_effect(spell), pos, orient, 1);
        }
        break;
    }
    case 0x39:
        retaliate = true;
        show_fx = false;
        count = 3;
        die = -spells[spell].die;
        combat.flinch_if_idle(2);
        break;
    case 0x40:
    case 0x41:
    case 0x42:
        amount = (float)-level;
        count = level;
        die = 0;
        show_fx = false;
        break;
    case 0x3e: {
        float roll;

        show_fx = false;
        roll = frand();
        if (!No_resist && roll < resist)
            break;
        amount = power * 0.05f * level;
        break;
    }
    case 0x3f: {
        float roll;

        show_fx = false;
        roll = frand();
        if (!No_resist && roll < resist)
            break;
        add_ap(-power * 0.05f * level);
        break;
    }
    case 0x44: {
        float roll;

        show_fx = false;
        roll = frand();
        if (!No_resist && roll < resist)
            break;
        add_ap((float)-level);
        break;
    }
    case 0x3a: {
        float roll;

        show_fx = false;
        roll = frand();
        if (!No_resist && roll < resist)
            break;
        ai.disable_2f();
        break;
    }
    case 0x3b: {
        float roll;

        show_fx = false;
        roll = frand();
        if (!No_resist && roll < resist)
            break;
        ai.knock_down(rand_int_range(3000, 7000));
        break;
    }
    case 0x4c: {
        float roll;

        show_fx = false;
        roll = frand();
        if (!No_resist && roll < resist)
            break;
        if (preloaded)
            status_effect_add(STATUS_PARALYZE, 180000, 0x4c, 0.0f, -1, false);
        else
            status_effect_add(STATUS_PARALYZE, 180000, -1, 0.0f, -1, false);
        break;
    }
    case 0x1c:
    case 0x1e:
    case 0x4f:
    case 0x64:
        count = level;
        if (src->info->species == 0x58)
            die = -10;
        else
            die = -spells[spell].die;
        flinch = 0x82;
        show_fx = false;
        retaliate = true;
        break;
    case 0x11: {
        float roll;
        float gain;
        int drained;

        show_fx = false;
        retaliate = true;
        roll = frand();
        if (!No_resist && roll < resist)
            break;
        roll = frand();
        if (roll < 0.05f) {
            drained = ap;
            ASSIGN_MAX(drained, drained, 0);
        } else {
            drained = (int)(ap * (level * 0.01f) + 0.5f);
            ASSIGN_MAX(drained, drained, 0);
        }
        add_ap((float)-drained);
        gain = (float)drained;
        ASSIGN_MIN(gain, gain, 20.0f);
        src->add_ap(gain);
        play_flinch(0x82);
        break;
    }
    case 3:
    case 5:
    case 0x69:
        if (spell != 0x69 || type == 7) {
            float heal;
            float cur;
            float max_hp;

            show_fx = false;
            resurrect();
            if (spell == 5) {
                hp = (float)get_max_stat(0);
            } else if (spell == 0x69) {
                heal = src->level * 2.0f;
                cur = hp;
                ASSIGN_MAX(heal, heal, cur);
                max_hp = (float)get_max_stat(0);
                ASSIGN_MIN(heal, heal, max_hp);
                hp = heal;
            } else {
                hp = 1.0f;
            }
        } else {
            amount = src->level * -7.0f;
            count = src->level * 7;
            die = 0;
            flinch = 0x82;
        }
        break;
    case 0x68:
        retaliate = true;
        show_fx = false;
        amount = src->level * -20.0f;
        count = src->level * 20;
        die = 0;
        flinch = 0x82;
        fx_add_effect(spells[spell].hit_effect, pos, orient, 1);
        break;
    case 0x6a:
        get_hit_by_spell(0x17, source, pos, orient, source_type, item, power, false);
        get_hit_by_spell(7, source, pos, orient, source_type, item, power, false);
        get_hit_by_spell(8, source, pos, orient, source_type, item, power, false);
        get_hit_by_spell(0x3c, source, pos, orient, source_type, item, power, false);
        amount = src->level * -10.0f;
        count = src->level * 10;
        die = 0;
        flinch = 0x82;
        show_fx = false;
        if (has_status(STATUS_CURSE) || has_status(STATUS_BLIND) || has_status(STATUS_SILENCE)
            || has_status(STATUS_POISON))
            show_fx = true;
        break;
    case 0x6b:
        if (type == 7) {
            get_hit_by_spell(0x16, source, pos, orient, source_type, item, power, false);
            get_hit_by_spell(0x13, source, pos, orient, source_type, item, power, false);
            get_hit_by_spell(2, source, pos, orient, source_type, item, power, false);
            get_hit_by_spell(0x10, source, pos, orient, source_type, item, power, false);
            show_fx = false;
            if (has_status(STATUS_FAITH) || has_status(STATUS_BLESS) || has_status(STATUS_REGEN)
                || has_status(STATUS_POWER))
                show_fx = true;
        } else {
            amount = src->level * -8.0f;
            count = src->level * 8;
            die = 0;
            show_fx = false;
            flinch = 0x82;
        }
        break;
    case 0x66: {
        float roll;

        retaliate = true;
        show_fx = false;
        roll = frand();
        if (!No_resist && roll < resist)
            break;
        if (src != NULL && distance(&this->pos, &src->pos) < 10.0f)
            amount = (float)-level;
        count = level;
        die = -spells[spell].die;
        break;
    }
    case 0xc:
    case 0x54:
    case 0x57:
    case 0x65: {
        float roll;
        int i;
        item_info *it;

        retaliate = true;
        show_fx = false;
        if (spell == 0x65)
            show_fx = true;
        if (type == 9 || (flags & 0x40))
            break;
        roll = frand();
        if (!No_resist && roll < resist) {
            if (spell == 0x65)
                get_hit_by_spell(0x67, source, NULL, NULL, 0, NULL, 0.0f, false);
            break;
        }
        if (spell == 0x54)
            show_fx = true;
        amount = -hp;
        count = (int)ceil(hp);
        die = 0;
        use_resist = false;
        if (type != 7) {
            for (i = 0; i < 9; i++) {
                if (equipment[i] != NULL) {
                    it = character_unequip_slot(this, i, 1);
                    item_free(it);
                }
            }
        }
        flinch = 0x82;
        break;
    }
    case 0xd: {
        float roll;

        roll = frand();
        if (!No_resist && roll < resist)
            break;
        if (level > 1) {
            this->level -= 1;
            scale_stats_to_level();
        }
        break;
    }
    }
    amount = spell_roll_amount(this, spell, count, die, use_resist ? magic : 0.0f);
    add_hp(amount, source, 0, false);
    if (amount < 0.0f && flinch != 0)
        play_flinch(flinch);
    if (show_fx == 1) {
        if (preloaded) {
            if (spells[spell].hit_effect != NULL)
                fx_attach_to_character(spells[spell].hit_effect, handle, -1, -1, 0, 1);
        } else if (spells[spell].hit_effect != NULL) {
            fx_attach_to_character(spell_get_hit_effect(spell), handle, -1, -1, 0, 1);
        }
    }
    if (spell == 0x21) {
        float roll;

        roll = frand();
        if (No_resist || !(roll <= magic)) {
            ms = spell_get_duration_ms(0x21, level);
            show_fx = false;
            if (preloaded)
                status_effect_add(status, ms, spell, 0.0f, src->handle, false);
            else
                status_effect_add(status, ms, -1, 0.0f, src->handle, false);
            play_flinch(0x81);
        } else {
            combat.flinch_if_idle(2);
        }
    }
    if (retaliate)
        ai.retaliate_on_hit(source);
}

// 0x460190  Confidence: likely (two jump tables: by spell, byte-indexed 0x460e90 / 0x460e58, and by the
// result kind 0x460ef8; case bodies in the code's order). Frame: has_pos -4, src -8, play_sound -0xc,
// snd_pos -0x18, show_fx -0x1c, flinch -0x20, status -0x24, force -0x28, i -0x2c, this -0x88, the
// switch values -0x8c / -0x9c / -0xa0. Applies a hit that spell_cast_launch_effects / fx_frame /
// fx_missile_hit resolved (h->result: spell_prepare_cast's result): statuses, cures, resurrection, the
// hit sound and effect, then the HP / AP change by kind. Only the multiplayer host (or single player
// for none of them: every change is behind MP_IS_HOST) changes HP, AP and statuses here; the flinches
// and effects play everywhere.
void Character::spell_apply_hit_effects(spell_hit *h)
{
    bool has_pos;
    Character *src;
    bool play_sound;
    vector3 snd_pos;
    bool show_fx;
    bool flinch;
    int status;
    bool force;
    int i;

    if (((flags & CF_DEAD) || hp <= 0.0f) && h->spell != 5 && h->spell != 3 && h->spell != 0x69)
        return;
    play_sound = true;
    show_fx = true;
    has_pos = false;
    flinch = false;
    src = living_from_handle(h->source);
    snd_pos = pos;
    switch (h->spell) {
    case 0: case 0xc: case 0xd: case 0xf: case 0x11: case 0x3e: case 0x3f: case 0x40: case 0x41:
    case 0x42: case 0x66:
        break;
    case 2: case 6: case 7: case 8: case 0xb: case 0xe: case 0x10: case 0x13: case 0x15: case 0x16:
    case 0x22: case 0x24: case 0x25: case 0x3c: case 0x3d: case 0x4c:
        if (h->hit_mask & 1) {
            status = damage_type_to_status(h->spell);
            if (MP_IS_HOST())
                status_effect_add(status, h->ms, h->spell, h->value, h->source, false);
            if ((spells[h->spell].flags & SPELL_OFFENSIVE) && h->spell != 0x24)
                flinch = true;
        } else if (h->spell == 0x24) {
            show_fx = false;
        }
        break;
    case 1:
        if (MP_IS_HOST() && (h->hit_mask & 1)) {
            force = (h->value != 0.0f) ? 1 : 0;
            status_effect_cure(STATUS_BLIND, force, false);
            status_effect_cure(STATUS_SILENCE, force, false);
            status_effect_cure(STATUS_CURSE, force, false);
            status_effect_cure(STATUS_SLEEP, force, false);
            status_effect_cure(STATUS_PARALYZE, force, false);
            status_effect_cure(STATUS_POISON, force, false);
            status_effect_cure(STATUS_CRIPPLE, force, false);
            status_effect_cure(STATUS_SLOW, force, false);
            status_effect_cure(STATUS_ICE_COFFIN, force, false);
        }
        break;
    case 0x17:
        for (i = 0; i < 6; i++) {
            if (h->handles[i] == handle) {
                if (h->hit_mask & (1 << i)) {
                    if (MP_IS_HOST())
                        status_effect_add(STATUS_CURSE, h->curse_ms, h->spell, h->curse_value, h->source, false);
                    flinch = true;
                } else {
                    show_fx = false;
                }
                break;
            }
        }
        play_sound = false;
        break;
    case 0x18: case 0x19: case 0x1a: case 0x20: case 0x4d: case 0x4e: case 0x5b:
        if (h->pos != NULL && h->orient != NULL)
            has_pos = true;
        break;
    case 0x21:
        if (h->hit_mask & 1)
            flinch = (h->amount > 0.0f || h->duration > 0) ? 1 : 0;
        break;
    case 0x1c: case 0x1e: case 0x4f: case 0x64:
        show_fx = false;
        play_sound = false;
        break;
    case 0x3a:
        if (h->hit_mask & 1)
            ai.disable_2f();
        show_fx = false;
        play_sound = false;
        break;
    case 3:
    case 5:
        if (h->hit_mask & 1) {
            resurrect();
            if (h->spell == 5)
                hp = (float)get_max_stat(0);
            else
                hp = 1.0f;
            if (this == cur_character)
                hud_mode_reset();
        }
        show_fx = false;
        play_sound = false;
        break;
    case 0x3b:
        if (h->hit_mask & 1)
            ai.knock_down(rand_int_range(3000, 7000));
        show_fx = false;
        play_sound = false;
        break;
    case 4:
        snd_pos = src->pos;
        play_sound = (this == src);
        break;
    case 0x65:
        break;
    case 0x67: {
        int sub_spells[5] = { 0x3d, 7, 0x3c, 8, 0x17 };
        int j;
        int statuses[5] = { STATUS_PARALYZE, STATUS_BLIND, STATUS_POISON, STATUS_SILENCE, STATUS_CURSE };

        for (j = 0; j < 5; j++) {
            if (h->durations[j] != 0)
                status_effect_add(statuses[j], h->durations[j], sub_spells[j], h->values[j], src->handle, false);
        }
        break;
    }
    default:
        return;
    }
    if (play_sound)
        spell_play_hit_sound(h->spell, &snd_pos);
    if (show_fx && spells[h->spell].hit_effect != NULL) {
        switch (h->spell) {
        case 0x17:
        case 0x1e:
            if (src != NULL)
                fx_create_spell_effect_facing(h->spell, src->handle, handle);
            break;
        case 0x1a:
            if (has_pos) {
                int fx;

                fx = fx_add_effect(spells[h->spell].hit_effect, h->pos, h->orient, 1);
                if (MP_IS_HOST() && fx != -1)
                    spell_effect_obj_set_timer(h->source, fx, h->pos, layer, &orient);
            }
            break;
        default:
            if (has_pos)
                fx_add_effect(spells[h->spell].hit_effect, h->pos, h->orient, 1);
            else
                fx_attach_to_character(spells[h->spell].hit_effect, handle, -1, -1, 0, 1);
            break;
        }
    }
    switch (h->kind) {
    case 1:
        if (MP_IS_HOST())
            add_hp(h->amount, h->source, 0, false);
        if (h->amount < 0.0f && h->flinch)
            combat.flinch_if_idle(h->flinch);
        break;
    case 2:
        if (MP_IS_HOST())
            add_ap(h->amount);
        if (h->amount < 0.0f && h->flinch)
            combat.flinch_if_idle(h->flinch);
        break;
    case 3:
    case 5:
        if (flinch && h->flinch)
            combat.flinch_if_idle(h->flinch);
        break;
    case 4: {
        int k;

        for (k = 0; k < 6; k++) {
            if (handle == h->handles[k] && (h->hit_mask & (1 << k))) {
                if (MP_IS_HOST())
                    add_hp(h->amounts[k], h->source, 0, false);
                if (h->amounts[k] < 0.0f && h->flinch)
                    combat.flinch_if_idle(h->flinch);
                break;
            }
        }
        break;
    }
    case 6:
        if (h->hit_mask & 1) {
            if (MP_IS_HOST()) {
                add_ap(-h->amount);
                if ((float)h->duration > 0.0f)
                    status_effect_add(STATUS_SLEEP, h->duration, 0x21, 0.0f, h->source, false);
            }
            if (flinch && h->flinch)
                combat.flinch_if_idle(h->flinch);
        }
        break;
    case 7:
        if (MP_IS_HOST()) {
            add_hp(h->amount, h->source, 0, false);
            src->add_hp(h->value, -1, 0, false);
        }
        if (h->amount < 0.0f && h->flinch)
            combat.flinch_if_idle(h->flinch);
        break;
    case 8:
        if (level > 1) {
            level -= 1;
            scale_stats_to_level();
        }
        break;
    case 9:
        if (h->hit_mask & 1) {
            if (MP_IS_HOST()) {
                float gain;

                add_ap((float)-h->iamount);
                gain = (float)h->iamount;
                ASSIGN_MIN(gain, gain, 20.0f);
                src->add_ap(gain);
            }
            if (h->flinch)
                combat.flinch_if_idle(h->flinch);
        }
        break;
    }
    if (MP_IS_HOST() && (spells[h->spell].flags & SPELL_OFFENSIVE))
        ai.retaliate_on_hit(h->source);
}

// 0x460f20  Confidence: exact (the move record arrives as an int: the declaration keeps D303's text;
// ii -4, mi -8, fx -0xc, pt -0x18, this -0x2c, the switch value -0x30). Starts a move: refused while a
// boss emerges (AI +0x194) in combat or, unless `keep`, once the current move's trigger 9 time is
// (nearly) reached (time left <= 0.0001, D6104); the move gets a new serial (cur_anim, returned), its
// latches are reset, death moves (category 6)
// hold their last frame (and flag 0x8000 for the species that stay as a corpse effect), its sound
// replaces the last one and its effect is attached (species 0x57-0x5a: dropped at pos + 0.77 m).
int Character::play_move(int m, float speed, bool keep, bool clear_actions, bool held)
{
    move_info *mi;
    int ii;
    int fx;
    vector3 pt;

    if (m == 0)
        return -1;
    if (((move_info *)m)->anim == -1)
        return -1;
    if (ai.is_combat_e_or_sneak_attack() && ai.emerge_move != -1)
        return -1;
    mi = get_current_move();
    if (!keep) {
        if (mi != NULL && move_has_trigger(9) && move_trigger_time_left(9) <= 0.0001f)
            return -1;
        fx_remove_missiles_from(cur_anim);
    }
    if (clear_actions)
        instance->clear_action_anims();
    cur_move = (move_info *)m;
    move_start_time = level_session_time;
    move_play_serial++;
    if (move_play_serial < 0)
        move_play_serial = 0;
    cur_anim = move_play_serial;
    for (ii = 0; ii < 3; ii++)
        trigger_latch[ii] = 0;
    if (cur_move->category == 6) {
        switch (info->species) {
        case 0x4: case 0x8: case 0x9: case 0x45: case 0x57: case 0x58: case 0x59: case 0x5a:
        case 0x90: case 0x95: case 0x9f: case 0xa4: case 0xb0: case 0xb9: case 0xba: case 0xc0:
        case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce:
            flags |= 0x8000;
            held = true;
            break;
        default:
            held = true;
            break;
        }
    }
    instance->start_anim(((move_info *)m)->anim, speed, held, 1.0f);
    instance->process(0.0f, false, 0);
    if (((move_info *)m)->sound.handle != -1 && !(game_flags & GF_CUTSCENE)) {
        if (snd_is_playing(move_sound))
            snd_stop_fade(move_sound, 100);
        move_sound = sound_play_3d_at(((move_info *)m)->sound.handle, &pos);
    }
    if (((move_info *)m)->effect != NULL) {
        if (cur_move->category == 6
            && (info->species == 0x5a || info->species == 0x57 || info->species == 0x59
                || info->species == 0x58)) {
            pt = pos;
            pt.y += 0.77f;
            fx = fx_add_effect(((move_info *)m)->effect, &pt, &orient, 1);
        } else {
            fx = fx_attach_to_character(((move_info *)m)->effect, handle, -1, -1, 0, 0);
        }
        if (fx != -1 && cur_move->category == 6 && (move_fx == -1 || fx_is_finished(move_fx, 0)))
            move_fx = fx;
    }
    return cur_anim;
}

// 0x461370  Confidence: exact. Bosses have no flinch moves (category 4).
move_info *Character::find_move(int category, int id)
{
    if (category == 4 && type == 9)
        return NULL;
    return charinfo_find_move(info, category, id, 1);
}

// 0x4613b0  Confidence: exact.
move_info *Character::find_move_alt(int category, int id)
{
    if (category == 4 && type == 9)
        return NULL;
    return charinfo_find_move_alt(info, category, id, 1);
}

// 0x4613f0  Confidence: exact (t -4, len -8: the fst keeps the length for the division). 0..1 of the
// move, 1 when it is not playing.
float Character::move_progress(int anim)
{
    float t;
    float len;

    if (!move_playing(anim, &t))
        return 1.0f;
    len = instance->get_anim_length(cur_move->anim);
    return (len - t) / len;
}

// 0x461450  Confidence: exact (rem -4, tmp -8, this -0xc). The move serial `anim` is still the current
// one and its animation has time left; an ended move clears cur_anim.
bool Character::move_playing(int anim, float *t)
{
    float rem;
    float tmp;

    if (anim == -1)
        return false;
    if (anim != cur_anim)
        return false;
    if (t == NULL)
        t = &tmp;
    rem = instance->get_anim_time_left(cur_move->anim);
    *t = rem;
    if (rem < 0.0001f) {
        cur_anim = -1;
        return false;
    }
    return true;
}

// 0x4614d0  Confidence: exact.
bool Character::move_finished(int anim)
{
    return !move_playing(anim, NULL);
}

// 0x461500  Confidence: exact.
move_info *Character::get_current_move()
{
    if (move_playing(cur_anim, NULL))
        return cur_move;
    return NULL;
}

// 0x461540  Confidence: exact (mv -4, first -8, pick -0xc, n -0x10). A random "talk*" move of the
// char_info's category 8 list; returned as an int (the play_move argument, D303).
int Character::pick_talk_move()
{
    move_info *mv;
    move_info *first;
    int pick;
    int n;

    first = NULL;
    n = 0;
    for (mv = (move_info *)info->moves[8]; mv != NULL; mv = mv->next) {
        if (_strnicmp(mv->action, "talk", 4) == 0) {
            if (first == NULL)
                first = mv;
            n++;
        }
    }
    if (n == 0)
        return 0;
    if (n == 1)
        return (int)first;
    pick = rand() % n;
    for (mv = first; mv != NULL; mv = mv->next) {
        if (_strnicmp(mv->action, "talk", 4) == 0) {
            if (pick == 0)
                return (int)mv;
            pick--;
        }
    }
    return 0;
}

// 0x461620  Confidence: exact. The current move record is not checked for NULL.
bool Character::move_has_trigger(int trigger)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (cur_move->triggers[i].id == trigger)
            return true;
    }
    return false;
}

// 0x461670  Confidence: exact. Seconds until the trigger's time from the move's start (negative once
// passed); 0 without a move or trigger. It reads cur_move even after the move ended.
float Character::move_trigger_time_left(int trigger)
{
    int i;

    if (cur_move != NULL) {
        for (i = 0; i < 3; i++) {
            if (cur_move->triggers[i].id == trigger)
                return cur_move->triggers[i].time - (level_session_time - move_start_time);
        }
    }
    return 0.0f;
}

// 0x4616f0  Confidence: exact.
void Character::move_trigger_mark(int trigger)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (cur_move->triggers[i].id != trigger)
            continue;
        trigger_latch[i] = 1;
        break;
    }
}

// 0x461740  Confidence: exact. 1 once when the trigger time has passed (latched), 0 before or when
// already reported, -1 without a move or trigger. `latch` is set for trigger 9 and never read.
int Character::move_trigger_fired(int trigger, bool latch)
{
    int i;

    if (trigger == 9)
        latch = true;
    for (i = 0; i < 3; i++) {
        if (cur_move == NULL)
            return -1;
        if (cur_move->triggers[i].id != trigger)
            continue;
        if (trigger_latch[i])
            return 0;
        if (move_trigger_time_left(trigger) <= 0.0001f) {
            trigger_latch[i] = 1;
            return 1;
        }
        return 0;
    }
    return -1;
}

// 0x4617f0  Confidence: exact. Below 30% of max HP, or slowed.
bool Character::is_badly_hurt()
{
    if (get_max_stat(0) * 0.3f > hp)
        return true;
    if (has_status(STATUS_SLOW))
        return true;
    return false;
}

// 0x461840  Confidence: exact.
int Character::get_move_effect()
{
    return move_fx;
}

// 0x461860 init_ccmd_immortal_thunk: calls the initialiser (frame-pointer wrapper). Confidence: exact.
// 0x461870 init_ccmd_immortal: the console_command constructor. Confidence: exact.
console_command ccmd_immortal("immortal", "Toggles Players_immortal", cmd_immortal);   // 0x8ed970

// 0x461890  Confidence: exact (the console's bool command macro; the "TRUE" / "FALSE" ?: temporary at
// ebp-4).
void cmd_immortal()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Players_immortal = true;
        else if (arg_type & ARG_FALSE)
            Players_immortal = false;
        else if (arg_type & ARG_NONE)
            Players_immortal ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "immortal", "Players_immortal");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "immortal", Players_immortal ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x461980 init_ccmd_health_thunk: calls the initialiser (frame-pointer wrapper). Confidence: exact.
// 0x461990 init_ccmd_health: the console_command constructor. Confidence: exact.
console_command ccmd_health("health", "Sets the percentage health for the debug object", cmd_health);   // 0x941508

// 0x4619b0  Confidence: exact (c at ebp-4, v -8, the fild temporary -0xc). Sets the HP of the
// character under the mouse to a fraction (0..1) of its char_info max HP; 0 kills it.
void cmd_health()
{
    Character *c;
    float v;

    c = living_from_handle(world_mouse_target_handle);
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        v = command_argument_float;
        if (v >= 0.0f && v <= 1.0f) {
            if (c != NULL) {
                c->hp = c->info->max_hp * v;
                if (v == 0.0f)
                    c->kill(0x22);
            }
        } else {
            doing_help_for_comand = 1;
            checking_status_for_command = 0;
        }
    }
    if (checking_status_for_command) {
        if (c != NULL) {
            sprintf(string_buffer, "Health: %.3f\n", c->hp);
            print_to_console(string_buffer, 0);
        } else {
            print_to_console("No study object selected.\n", 0);
        }
    }
    if (doing_help_for_comand)
        print_to_console("Usage: health <percentage> -- where percentage is the percent to set the debug\n"
                         "characters health to (i.e. between 0.0 and 1.0).", 0);
}

// 0x461ad0  Confidence: exact. Types 4 and 1 and the dead are never enemies; otherwise the team mask,
// and party members (team 0) treat another team-0 character in the ice coffin (status 16) as one.
bool Character::is_enemy(Character *c)
{
    if (c->type == 4 || c->type == 1 || c->is_dead_type())
        return false;
    if (enemy_team_mask & (1 << c->team))
        return true;
    if (team == 0 && c->team == 0 && c->has_status(STATUS_ICE_COFFIN))
        return true;
    return false;
}

// 0x461b60  Confidence: exact.
int Character::get_team()
{
    return team;
}

// 0x461b80  Confidence: exact. Frame: c -4, used -8, count -0xc, where -0x4c, i -0x50, dup -0x54, name
// -0x94, o -0x98. Takes the first free pool slot (the error loop, line 8424, when there is none),
// initialises it, names it "<char_info name>#<n>" with the first unused n, and unless it is a party
// member or `placed`, moves it onto the navigation mesh and the floor below (the position text of a
// failed floor search is formatted and not used).
Character *living_entity_create(int type, int info_index, const vector3 *pos, const matrix *orient, bool a,
                                int layer, int level, int unused, bool placed)
{
    Character *c;
    bool used;
    int count;
    char where[64];
    int i;
    bool dup;
    char name[64];
    Character *o;

    for (i = 0; i < MAX_LIVING_ENTITIES; i++) {
        if (living_entity_pool[i].type == 0)
            break;
    }
    if (i == MAX_LIVING_ENTITIES)
        VSDK_ERROR(LIVING_ENTITY_CPP, 8424,
                   "No more space for living entities.  Please increase MAX_LIVING_ENTIITES in living_entity.cpp");
    c = &living_entity_pool[i];
    c->init(type, info_index, pos, orient, a, level);
    c->layer = layer;
    c->ai.reset();
    if (c->type == 5)
        c->ai.flags |= 0x40;
    if (c->type == 4)
        c->ai.flags |= 0x40;
    used = false;
    count = 1;
    while (!used) {
        dup = false;
        sprintf(name, "%s#%d", c->info->name, count);
        for (o = living_list.live_next; o != &living_list; o = o->live_next) {
            if (_stricmp(o->name, name) == 0) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            used = true;
            c->set_name(name);
        }
        count++;
    }
    if (c->type == 7 || placed)
        return c;
    pf_find_nearest_navigable_point(&c->pos, c->info->size, c->layer);
    c->pos.y += 1.0f;
    c->find_ground();
    if (c->ground_face == -1)
        vector3_to_string_buf(where, 0x40, pos, 1);
    return c;
}

// ---- 0x461dc0-0x46335f: the optimised tail ---------------------------------------------------------
// From character_say on, the code is optimised (no frame pointer, 0x90 padding). It is the same object
// as the unoptimised part, not another one (D6100): the tail's own tables (character_speech_times
// 0x599698, hero_model_prefix_table 0x5996a4) lie among the unit's other initialised globals, before
// the literals of the unoptimised part (VC6 puts an object's initialised globals before its literals,
// tested with VC6's linker), and the tail's console command objects (0x8eca30-0x8eca50, 0x940428) lie
// among the living lists and the pool in the one .bss block. strings.tbl ids 314-352 are used on both
// sides (character_get_display_name at 0x462070 reads 351 / 352).
#ifdef _MSC_VER
#pragma optimize("", on)
#endif

// 0x461dc0  Confidence: likely (the frame is 0x134 bytes, so the quoted text is at most 240 bytes:
// unsure). One speech bubble per character: its own record, else a free one, else the one with the
// least time left (all are below 100 s). The text is quoted, split to the line width of the resolution
// (5 lines at most) and rejoined with newlines up to 255 characters.
void Character::say(const char *text, float seconds)
{
    int lens[5];
    int offsets[5];
    char quoted[256];
    character_speech_record *r;
    float best;
    int total;
    int slot;
    int n;
    int i;

    best = 100.0f;
    slot = -1;
    for (i = 0; i < MAX_CHARACTER_SPEECH; i++) {
        if (character_speech[i].speaker == handle) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        for (i = 0; i < MAX_CHARACTER_SPEECH; i++) {
            if (character_speech[i].speaker == -1) {
                slot = i;
                break;
            }
            if (best > character_speech[i].time_left) {
                best = character_speech[i].time_left;
                slot = i;
            }
        }
        if (slot == -1)
            return;
    }
    character_speech[slot].time_left = seconds;
    memset(character_speech[slot].text, 0, 0xff);
    character_speech[slot].speaker = handle;
    r = &character_speech[slot];
    strcpy(quoted, "\"");
    strcat(quoted, text);
    strcat(quoted, "\"");
    n = gr_split_str(lens, offsets, quoted, character_speech_times[resolution_index], 5, 0, -1);
    strncpy(r->text, quoted + offsets[0], lens[0]);
    total = lens[0];
    for (i = 1; i < n; i++) {
        if (lens[i] + total + 1 > 0xff)
            break;
        strcat(r->text, "\n");
        strncat(r->text, quoted + offsets[i], lens[i]);
        total += lens[i] + 1;
    }
}

// 0x461fe0  Confidence: exact. Up to 16 characters of floating text, its time and colour; the length
// test is signed (D6106).
void Character::set_floating_text(const char *text, float seconds, ubyte r, ubyte g, ubyte b)
{
    if ((int)strlen(text) > 16) {
        strncpy(float_text, text, 16);
        float_text_nul = 0;
    } else {
        strcpy(float_text, text);
    }
    float_text_scale = seconds;
    float_text_rgb[0] = r;
    float_text_rgb[1] = g;
    float_text_rgb[2] = b;
}

// 0x462070  Confidence: exact (the last two branches are the same code: two calls that may have been
// different functions before identical-code folding, unsure). The name up to its '#' / digit suffix,
// localised; "onif" is the Oni Female; multiplayer party members keep their player names.
void Character::get_display_name(char *buf)
{
    size_t n;

    if (strlen(name) != 0) {
        if ((game_flags & GF_MULTIPLAYER) && type == 7 && !(flags & CF_SUMMON)) {
            strcpy(buf, name);
            return;
        }
        n = strcspn(name, "#$01234567890");
        if (n != 0) {
            strncpy(buf, name, n);
            buf[n] = 0;
        } else {
            strcpy(buf, name);
        }
        if (_stricmp(buf, "onif") == 0) {
            strcpy(buf, XSTR("Oni Female", 351));
            return;
        }
        if (((flags & CF_SUMMON) || type == 7 || type == 8) && !(flags & 0x40000))
            strcpy(buf, localize_script_name(buf));
        else
            strcpy(buf, localize_script_name(buf));
    } else {
        strcpy(buf, XSTR("Unknown", 352));
    }
}

// 0x462200  Confidence: exact.
void Character::set_ai_active(int on)
{
    if (on) {
        obj_flags |= 1;
        flags &= ~0x20;
        ai.go_idle();
    } else {
        obj_flags &= ~1;
        flags |= 0x20;
        ai.go_inactive();
    }
}

// 0x462260  Confidence: exact. A negative speed resets it to the run speed; flag 0x200 (slow-walk).
void Character::set_speed_scale(float speed, bool walk)
{
    speed_scale = speed;
    if (speed < 0.0f)
        speed_scale = run_speed;
    if (walk)
        flags |= 0x200;
    else
        flags &= ~0x200;
}

// 0x4622b0  Confidence: exact. The alpha of the props while a move fade runs: fading out over the
// first `rate` of the move, back in over the last; a finished move frees its record.
ubyte Character::get_move_fade_alpha()
{
    ubyte alpha;
    float t;
    float rate;
    float end;
    int i;

    alpha = instance->_unkb4;
    for (i = 0; i < MAX_MOVE_FADES; i++) {
        if (move_fades[i].handle == handle) {
            t = move_progress(move_fades[i].move);
            if (t >= 1.0f) {
                move_fades[i].handle = -1;
                return alpha;
            }
            rate = move_fades[i].rate;
            end = 1.0f - rate;
            if (t < rate)
                alpha = (ubyte)(int)((rate - t) * 255.0f / rate);
            else if (t > end)
                alpha = (ubyte)(int)((t - end) * 255.0f / rate);
            else
                alpha = 0;
        }
    }
    return alpha;
}

// 0x462380  Confidence: exact.
int Character::is_main_hero()
{
    if (_stricmp(info->name, "Joseph") == 0 || _stricmp(info->name, "Rosalind") == 0
        || _stricmp(info->name, "Flece") == 0 || _stricmp(info->name, "Jekhar") == 0)
        return 1;
    return 0;
}

// 0x462400  Confidence: exact. A party member's skill must be unlocked; everyone else has all skills.
bool Character::skill_allowed(int skill)
{
    Player *p;

    p = player_from_handle(handle);
    if (p != NULL && !(flags & CF_SUMMON))
        return p->skill_unlocked(skill);
    return true;
}

// 0x462440  Confidence: exact. skill level * 0.01 plus the equipment: speed is multiplied by the
// equipment factor, banish is always 1, a summon skill counts as the summon skill; the weapon and combat
// skills (3..0x23) are rounded to steps of 0.05 (at least 0.05). -999999 when not allowed.
float Character::get_skill(int skill)
{
    float v;
    float r;

    if (!skill_allowed(skill))
        return -999999.0f;
    v = this->skill[skill] * 0.01f;
    if (skill == SKILL_SPEED)
        return skill_equip_mod(skill) * v;
    if (skill == SKILL_BANISH_SUMMON)
        return 1.0f;
    if (skill_is_summon(skill)) {
        v = this->skill[SKILL_SUMMON] * 0.01f;
        return skill_equip_mod(SKILL_SUMMON) + v;
    }
    r = skill_equip_mod(skill) + v;
    if (r > 0.0f && skill >= SKILL_SWORD_WEAPONS && skill < SKILL_SUMMON_GOLEM) {
        r = (int)((r + 0.025f) * 20.0f) * 0.05f;
        if (r <= 0.05f)
            r = 0.05f;
    }
    return r;
}

// 0x462540  Confidence: exact. The +Skill Mod: sum of the ten equipment slots (a party member must have
// the skill at its level); in multiplayer appraise (0x14) counts as aimed attack (0x11) and pick lock
// (0x15) as dark (0x1d). For speed it is (1 + sum) times 1 / each weapon's or armour's speed.
float Character::skill_equip_mod(int skill)
{
    float sum;
    float mult;
    int i;
    int j;
    Player *p;

    sum = 0.0f;
    mult = 1.0f;
    for (i = 0; i < 10; i++) {
        if (equipment[i] != NULL) {
            ItemType *t = equipment[i]->type;

            for (j = 0; j < 4; j++) {
                if (t->skill_mods[j] != NULL) {
                    int id = t->skill_mods[j]->id;

                    if (game_flags & GF_MULTIPLAYER) {
                        switch (id) {
                        case SKILL_PICK_LOCK:
                            id = SKILL_DARK;
                            break;
                        case SKILL_APPRAISE:
                            id = SKILL_AIMED_ATTACK;
                            break;
                        }
                    }
                    if (id == skill) {
                        p = player_from_handle(handle);
                        if (p == NULL || p->skill_unlocked_at_level(skill))
                            sum += t->skill_mods[j]->value;
                    }
                }
            }
            if (skill == SKILL_SPEED) {
                switch (t->kind) {
                case 0:
                    mult /= t->speed;
                    break;
                case 1:
                    mult /= t->armour_speed;
                    break;
                }
            }
        }
    }
    if (skill == SKILL_SPEED)
        return (sum + 1.0f) * mult;
    return sum;
}

// 0x462660  Confidence: exact.
float Character::resist_equip_bonus(int type)
{
    float r;
    int i;
    int j;

    r = 0.0f;
    for (i = 0; i < 10; i++) {
        if (equipment[i] != NULL) {
            for (j = 0; j < 4; j++) {
                item_mod *m = equipment[i]->type->resist_mods[j];

                if (m != NULL && m->id == type)
                    r += m->value;
            }
        }
    }
    return r;
}

// 0x4626b0  Confidence: likely. Magic (3): half the magic resist skill, the equipment, minus a curse,
// plus 2% per level, at least the faith value. The others: the char_info resistance level's factor plus
// the equipment. The curse is tested with the type's register (3 = STATUS_CURSE).
float Character::get_resistance(int type)
{
    float skill_part;
    float equip;
    float curse;
    float r;
    float faith;

    if (type == -1)
        return 0.0f;
    if (type == 3) {
        skill_part = get_skill(SKILL_MAGIC_RESIST) * 0.5f;
        equip = resist_equip_bonus(type);
        curse = 0.0f;
        if (has_status(STATUS_CURSE) == 1) {
            curse = status_value(STATUS_CURSE);
            if (curse <= 0.0f)
                curse = 0.0f;
        }
        r = skill_part + equip - curse + level * 0.02f;
        if (has_status(STATUS_FAITH) == 1) {
            faith = status_value(STATUS_FAITH);
            if (!(r > faith))
                r = faith;
        }
        return r;
    }
    return resistance_level_damage_mult(info->resistance.level[type]) + resist_equip_bonus(type);
}

// 0x4627c0  Confidence: exact (8966: the item type through a local, D8700).
int Character::equip_bonus(int stat)
{
    int sum;
    int i;
    int j;

    sum = 0;
    for (i = 0; i < 10; i++) {
        if (equipment[i] != NULL) {
            ItemType *t = equipment[i]->type;

            for (j = 0; j < 2; j++) {
                item_stat_mod *m = t->stat_mods[j];

                if (m != NULL && m->stat == stat)
                    sum += m->value;
            }
        }
    }
    return sum;
}

// 0x462810  Confidence: exact.
float Character::ap_regen_mult()
{
    float m;
    int i;

    m = 1.0f;
    for (i = 0; i < 10; i++) {
        if (equipment[i] != NULL)
            m *= equipment[i]->type->ap_regen;
    }
    return m;
}

// 0x462840  Confidence: exact. Max HP at least 1, max AP at least 0.
int Character::get_max_stat(int stat)
{
    int v;

    switch (stat) {
    case 0:
        v = (int)(equip_bonus(0) + base_max_hp);
        return max(v, 1);
    case 1:
        v = equip_bonus(1) + base_max_ap;
        return max(v, 0);
    }
    return 0;
}

// 0x4628a0  Confidence: exact. Reloads a hero's mesh (Joseph, Jekhar and Flece have a heavy model 1,
// Rosalind none) and sets the six underwear / armour textures: helmet (equipment 7: head, body, arms; a
// heavy helmet also its leg texture), cuirass (4), cloak (3, alternate texture with CF 0x2000000) and
// leggings (8).
void Character::apply_hero_model(int heavy)
{
    char *tex[6];
    int i;

    if (heavy < 0)
        return;
    switch (info->species) {
    case SPECIES_JOSEPH:
        if (heavy >= 2)
            return;
        break;
    case SPECIES_JEKHAR:
        if (heavy >= 2)
            return;
        break;
    case SPECIES_FLECE:
        if (heavy >= 2)
            return;
        break;
    case SPECIES_ROSALIND:
        if (heavy >= 1)
            return;
        break;
    default:
        return;
    }
    memset(tex, 0, sizeof(tex));
    instance->character->reload_mesh(heavy);
    if (equipment[4] != NULL)
        tex[4] = equipment[4]->type->armour_textures[heavy][info->species][0];
    if (equipment[3] != NULL) {
        ItemType *t = equipment[3]->type;

        if (flags & 0x2000000)
            tex[5] = t->armour_textures[heavy][info->species][1];
        else
            tex[5] = t->armour_textures[heavy][info->species][0];
    }
    if (equipment[7] != NULL) {
        ItemType *t = equipment[7]->type;

        tex[0] = t->armour_textures[heavy][info->species][2];
        tex[1] = t->armour_textures[heavy][info->species][0];
        tex[2] = t->armour_textures[heavy][info->species][1];
        if (t->slot == 8)
            tex[3] = t->armour_textures[heavy][info->species][3];
        else if (equipment[8] != NULL)
            tex[3] = equipment[8]->type->armour_textures[heavy][info->species][0];
    } else if (equipment[8] != NULL) {
        tex[3] = equipment[8]->type->armour_textures[heavy][info->species][0];
    }
    for (i = 0; i < 6; i++)
        set_underwear_textures(i, tex[i]);
}

// 0x462a20  Confidence: exact (both tables are locals built at every call; defaults declared first,
// D8700). Puts a part's texture (or the underwear default; Joseph's arms with the alternate cloak flag
// get "Arms_Underwear1A") on the hero model's texture slot; in multiplayer on the appearance bitmap
// instead. Joseph and Jekhar have no cape.
void Character::set_underwear_textures(int part, const char *texture)
{
    const char *defaults[4][6] = {
        { "Joseph05.tga", "Torso_Uwear_Josp.tga", NULL, "Thigh_Uwear_Josp.tga", "Leg_Uwear_Josp.tga",
          "Arms_Uwear_Josp.tga" },
        { "Jekhar02.tga", "Torso_Uwear_Jekr.tga", NULL, "Thigh_Uwear_Jekr.tga", "Leg_Uwear_Jekr.tga",
          "Arms_Uwear_Jekr.tga" },
        { "Flece06.tga", "Torso_Uwear_Flec.tga", "Cape_Uwear_Flec.tga", "Thigh_Uwear_Flec.tga",
          "Leg_Uwear_Flec.tga", "Arms_Uwear_Flec.tga" },
        { "Rosal06.tga", "Torso_Uwear_Ros.tga", "Cape_Uwear_Ros.tga", "Thigh_Uwear_Ros.tga",
          "Leg_Uwear_Ros.tga", "Arms_Uwear_Ros.tga" },
    };
    int slots[4][6] = {
        { 4, 0, -1, 1, 2, 3 },
        { 1, 0, -1, 3, 4, 2 },
        { 5, 2, 0, 1, 4, 3 },
        { 5, 1, 0, 3, 4, 2 },
    };
    int species;
    int *app;

    species = info->species;
    if (species < SPECIES_JOSEPH || species > SPECIES_ROSALIND)
        return;
    if ((species == SPECIES_JOSEPH || species == SPECIES_JEKHAR) && part == 2)
        return;
    if (texture == NULL) {
        if (species == SPECIES_JOSEPH && (flags & 0x2000000) && part == 5)
            texture = "Arms_Underwear1A_Josp.tga";
        else
            texture = defaults[species][part];
    }
    if (!(game_flags & GF_MULTIPLAYER)) {
        instance->character->replace_texture(slots[species][part], texture);
        return;
    }
    app = (int *)charinfo_get_multi_appearance(info);
    if (app != NULL)
        bm_replace_file(app[slots[info->species][part]], texture, -1);
}

// 0x462c80  Confidence: exact (one else-if chain shares the force test, D8700). Joseph's alternate cloak
// texture (CF 0x2000000); `force` re-applies it unchanged.
void Character::set_armour_alt_texture(bool on, bool force)
{
    if (info->species != SPECIES_JOSEPH)
        return;
    if (on && !(flags & 0x2000000))
        flags |= 0x2000000;
    else if (!on && (flags & 0x2000000))
        flags &= ~0x2000000;
    else if (!force)
        return;
    if (equipment[3] != NULL) {
        char **t = equipment[3]->type->armour_textures[0][info->species];

        if (flags & 0x2000000)
            set_underwear_textures(5, t[1]);
        else
            set_underwear_textures(5, t[0]);
    } else {
        set_underwear_textures(5, NULL);
    }
}

// 0x462d20  Confidence: exact.
Character *Character::get_mount()
{
    Character *c;

    c = living_from_handle(mount_link);
    if (c != NULL && (c->mount_flags & 2))
        return c;
    return NULL;
}

// 0x462d40  Confidence: exact.
Character *Character::get_rider()
{
    Character *c;

    c = living_from_handle(mount_link);
    if (c != NULL && (c->mount_flags & 1))
        return c;
    return NULL;
}

// 0x462d60  Confidence: exact. The binary tests the low byte of the flags loaded as a dword (`shr al`,
// D8700: a register copy narrowed to ubyte).
bool Character::is_mount()
{
    unsigned int f = mount_flags;

    return ((ubyte)f & 2) ? true : false;
}

// 0x462d70  Confidence: exact (as is_mount, D8700).
bool Character::is_rider()
{
    unsigned int f = mount_flags;

    return ((ubyte)f & 1) ? true : false;
}

// 0x462d80  Confidence: exact.
void living_entity_reset_tables()
{
    int i;

    for (i = 0; i < MAX_MOVE_FADES; i++)
        move_fades[i].handle = -1;
    for (i = 0; i < MAX_CHARACTER_SPEECH; i++)
        character_speech[i].speaker = -1;
    for (i = 0; i < MAX_SPELL_COOLDOWNS; i++)
        spell_cooldowns[i].handle = -1;
    for (i = 0; i < MAX_SPELLS; i++)
        spell_global_timers[i].SetTimeout(0);
}

// 0x462de0  Confidence: exact. With pause_ai every AI character is suspended; while a conversation is
// not idle, characters in combat (except combat sub-mode 0xe) and type-5 characters are.
bool Character::ai_suspended()
{
    if (Ai_paused && !(ai.flags & AI_FLAG_PLAYER_CONTROL))
        return true;
    if (!conversation_active.is_idle()) {
        if ((ai.is_mode(AI_STATE_COMBAT, -1) && !ai.is_mode(AI_STATE_COMBAT, 0xe)) || type == 5)
            return true;
    }
    return false;
}

// 0x462e40  Confidence: exact. Travelling, or a travel goal (9) pending, or moving faster than
// sqrt(0.1).
int Character::is_moving()
{
    if (ai.is_traveling_sub0() || ai.is_traveling_sub1() || ai.goal_find(1, 9, 0, 0, -1, 0, 0, 0x6c) != -1)
        return 1;
    if (velocity.len_sq() < 0.1f)
        return 0;
    return 1;
}

// 0x462ec0  Confidence: exact. Not a prefix: the hero model render mask (the declaration keeps its
// const char * for new_status.cpp / new_inventory.cpp, D1711). Without a helmet the default mask per
// hero {6, 6, 2, 2}; with one, the helmet's +RenderMask: byte for "[heavy ]<hero>".
const char *Character::hero_model_prefix()
{
    char model[0x40];
    int index;

    if (info->species != SPECIES_JOSEPH && info->species != SPECIES_JEKHAR && info->species != SPECIES_FLECE
        && info->species != SPECIES_ROSALIND)
        return 0;
    if (equipment[7] == NULL) {
        switch (info->species) {
        case SPECIES_JOSEPH:
            return (const char *)hero_model_prefix_table[0];
        case SPECIES_JEKHAR:
            return (const char *)hero_model_prefix_table[1];
        case SPECIES_FLECE:
            return (const char *)hero_model_prefix_table[2];
        case SPECIES_ROSALIND:
            return (const char *)hero_model_prefix_table[3];
        }
    }
    strcpy(model, "");
    if (equipment[7]->type->slot == 8)
        strcat(model, "heavy ");
    switch (info->species) {
    case SPECIES_JOSEPH:
        strcat(model, "joseph");
        break;
    case SPECIES_JEKHAR:
        strcat(model, "jekhar");
        break;
    case SPECIES_FLECE:
        strcat(model, "flece");
        break;
    case SPECIES_ROSALIND:
        strcat(model, "rosalind");
        break;
    }
    index = hero_model_index(model);
    if (index < 0)
        return 0;
    return (const char *)(int)equipment[7]->type->render_mask[index];
}

// 0x463050 init_ccmd_chance_gold_thunk: jmp to the initialiser. Confidence: exact.
// 0x463060 init_ccmd_chance_gold: the console_command constructor. Confidence: exact.
console_command ccmd_chance_gold("chance_gold", "Sets Chance_to_drop_gold", cmd_chance_gold);   // 0x8eca30

// 0x463080  Confidence: exact (the console's float command macro).
void cmd_chance_gold()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Chance_to_drop_gold = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "chance_gold",
                "Chance_to_drop_gold");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "chance_gold", Chance_to_drop_gold);
        print_to_console(string_buffer, 0);
    }
}

// 0x463110 init_ccmd_kill_summon_thunk: jmp to the initialiser. Confidence: exact.
// 0x463120 init_ccmd_kill_summon: the console_command constructor. Confidence: exact.
console_command ccmd_kill_summon("kill_summon", "kills off friendly summoned creature", cmd_kill_summon);   // 0x8eca50

// 0x463140  Confidence: exact. Removes (and deletes) every summon record of the party but the leader.
// The next link is read before each loop test (the original's safe-iteration shape, D6106).
void cmd_kill_summon()
{
    Player *p;
    Player *next;

    for (p = party_list.next, next = p->next; p != &party_list; p = next, next = p->next) {
        if (p != party_leader && (p->flags & PF_SUMMON))
            party_remove_player(p, true);
    }
}

// 0x463180 init_ccmd_no_resist_thunk: jmp to the initialiser. Confidence: exact.
// 0x463190 init_ccmd_no_resist: the console_command constructor. Confidence: exact.
console_command ccmd_no_resist("no_resist", "Toggles No_resist", cmd_no_resist);   // 0x940428

// 0x4631b0  Confidence: exact (the console's bool command macro).
void cmd_no_resist()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            No_resist = true;
        else if (arg_type & ARG_FALSE)
            No_resist = false;
        else if (arg_type & ARG_NONE)
            No_resist ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "no_resist", "No_resist");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "no_resist", No_resist ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x463270 init_ccmd_Spell_ap_thunk: jmp to the initialiser. Confidence: exact.
// 0x463280 init_ccmd_Spell_ap: the console_command constructor. Confidence: exact.
console_command ccmd_Spell_ap("Spell_ap", "Toggles Use_spell_ap", cmd_Spell_ap);   // 0x8eca40

// 0x4632a0  Confidence: exact.
void cmd_Spell_ap()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Use_spell_ap = true;
        else if (arg_type & ARG_FALSE)
            Use_spell_ap = false;
        else if (arg_type & ARG_NONE)
            Use_spell_ap ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "Spell_ap", "Use_spell_ap");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "Spell_ap", Use_spell_ap ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}
