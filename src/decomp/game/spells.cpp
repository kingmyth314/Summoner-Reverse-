// spells.cpp - Sum.exe v1.40, code 0x479b60-0x47f857 (module "spells.cpp" in sum.symbols / strings.tbl;
// no path string, so the directory of engine/spells/spells.h is guessed).
// Written from the Sum.exe disassembly (tools/asm.mjs fn; the jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other optimised units (no frame pointer, 0x90 padding).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: the first function is init_strptrs_22e92a0_thunk (0x479b60), the last cmd_summon
// (0x47f730-0x47f857). Not written yet (see NOTES.md): spell_prepare_cast 0x47abe0, spell_spawn_creature
// 0x47bd30, spell_cast_execute 0x47c5d0, spell_cast_launch_effects 0x47d420, summon_create 0x47e900, the
// casting half of the unit (about 4,400 instructions).
//
// Data: .data 0x59b988-0x59bcab (the tables below, then this unit's literals, "+Hit only" first);
// literals pooled with earlier units (/Gf): ".wav", "$Sound:", "#End", "$missile", "Summon", "Misc" and
// the lower-case class names. .bss 0x22e4b18-0x22e94e7 in address order below; 0x22e923c (4 bytes) and
// 0x22e924c-0x22e925b are not referenced by this unit's code (probably the objects of the two empty
// initialisers 0x47a180 / 0x47e8f0, which are not reproduced: NOTES.md).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "engine/spells/spells.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/items.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/effects/fx.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/gamesound/gamesound.h"
#include "engine/rendering/levelrender.h"
#include "vsdk/os/console.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/parse/parse.h"
#include "vsdk/vfile/file.h"
#include "vsdk/localize/localize.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/geom/vfx.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"

// ---- .data (0x59b988-0x59ba0f; the literals follow) ----------------------------------------------
// Skill per spell group (spell_get_group result): heal, dark, energy, holy, fire, ice magic, then summon.
const int spell_group_skill[7] = {                     // 0x59b988
    SKILL_HEAL, SKILL_DARK, SKILL_ENERGY, SKILL_HOLY, SKILL_FIRE, SKILL_ICE, SKILL_SUMMON,
};
int spells_unk_59b9a4 = -1;                            // 0x59b9a4 not referenced (label ours)
// Capitalised class names: not referenced by any code in the binary (label ours). "Summon" and "Misc"
// are pooled with earlier units (0x598d0c, 0x599474).
const char *spell_class_names[NUM_SPELL_CLASSES] = {   // 0x59b9a8
    "Heal", "Dark", "Energy", "Holy", "Fire", "Ice", "Summon", "Misc",
};
const char *spell_class_parse_names[NUM_SPELL_CLASSES] = {     // 0x59b9c8 (+Class:; all pooled)
    "heal", "dark", "energy", "holy", "fire", "ice", "summon", "misc",
};
const char *spell_default_hit_effect = "GenericHit01.vfx";     // 0x59b9e8
int spells_unk_59b9ec = 0;                             // 0x59b9ec not referenced (label ours)
// {species, value} pairs read by summon_create (0x47ec4b: base 0x59b9f0, float at +4). Meaning not
// decoded; type and name ours.
struct summon_species_value { int species; float value; };
summon_species_value summon_species_values[4] = {     // 0x59b9f0
    { 0xc2, 7.0f }, { 0xb5, 13.0f }, { 0xb6, 35.0f }, { 0xc3, 14.0f },
};

// ---- .bss (0x22e4b18-0x22e94e7), in address order ------------------------------------------------
spell_missile spell_missiles[MAX_SPELL_MISSILES];      // 0x22e4b18
spell_info spells[MAX_SPELLS];                         // 0x22e4f28
spell_effect spell_effects[MAX_SPELL_EFFECTS];         // 0x22e8528
// 0x47a130 sinit_spell_string_pool (call 0x47a140, jmp 0x47a150), 0x47a140 init (StringPool(0x3000)),
// 0x47a150 register (atexit(0x47a160)), 0x47a160 dtor (jmp StringPool::~StringPool). Confidence: exact.
StringPool spell_string_pool(0x3000);                  // 0x22e9228
// ccmd_summon 0x22e9240 is defined with its command below (its initialiser runs there).
int spell_sound_path;                                  // 0x22e925c
spell_group spell_groups[NUM_SPELL_GROUPS];            // 0x22e9260
// localize_strings slot tables (dynamic initialisers: &localize_strings[id] is stored). 0x479b60 /
// 0x479b70 init_strptrs_22e92a0: VC6 loads "Unused." (361) into EAX once and stores it five times.
// 0x479ce0 / 0x479cf0 init_strptrs_22e9338. Confidence: likely (one initialiser per table, element
// order = store order); these two are the unit's first static initialisers, so they are defined first
// in the original (before the string pool), although their .bss comes last.
char **spell_descriptions[38] = {                      // 0x22e92a0
    &XSTR("Restore HPs to target.", 353), &XSTR("Remove target's status effects.", 354),
    &XSTR("Target heals HP over time.", 355), &XSTR("Target is restored to life.", 356),
    &XSTR("Restore HPs to party.", 357), &XSTR("Target is fully restored.", 358),
    &XSTR("Target loses HPs over time.", 359), &XSTR("Target cannot see. Penalty to hit.", 360),
    &XSTR("Target cannot cast spells.", 181), &XSTR("Unused.", 361), &XSTR("Unused.", 361),
    &XSTR("Target cannot be seen. Bonus to dodge", 362), &XSTR("Target is killed.", 363),
    &XSTR("Target loses a level.", 364), &XSTR("Caster gains bonus to hit", 365),
    &XSTR("Does energy damage to target.", 366), &XSTR("Target deals extra damage.", 367),
    &XSTR("Target loses AP, Caster gains AP.", 368), &XSTR("Unused.", 361),
    &XSTR("Target takes less physical damage.", 369), &XSTR("Unused.", 361),
    &XSTR("Party takes less physical damage.", 193), &XSTR("Party is resistant to magic.", 370),
    &XSTR("Nearby targets take extra damage.", 371), &XSTR("Does non-elemental damage to target.", 372),
    &XSTR("Does fire damage to target.", 373), &XSTR("Does persistent fire damage to target.", 374),
    &XSTR("Unused.", 361), &XSTR("Does fire damage to random targets.", 375),
    &XSTR("Creates a barrier of fire.", 376), &XSTR("Does fire damage to nearby targets.", 377),
    &XSTR("Creates a barrier of dark effects.", 378), &XSTR("Does ice damage to target.", 379),
    &XSTR("Sleeps target + damage.", 380), &XSTR("Freezes target + damage.", 381),
    &XSTR("Area effect ice damage.", 382), &XSTR("Freezes target in coffin.", 383),
    &XSTR("Sleeps target.", 384),
};
char **spell_display_names[MAX_SPELLS] = {             // 0x22e9338
    &XSTR("heal", 98), &XSTR("cure", 385), &XSTR("regenerate", 386), &XSTR("resurrect", 387),
    &XSTR("vitalize", 388), &XSTR("revive", 389), &XSTR("Poison", 390), &XSTR("blind", 391),
    &XSTR("silence", 180), &XSTR("black cloud", 392), &XSTR("recall", 393), &XSTR("invisibility", 394),
    &XSTR("death", 395), &XSTR("Life Drain", 396), &XSTR("light", 397), &XSTR("lightning", 398),
    &XSTR("empower", 399), &XSTR("power leech", 400), &XSTR("destroy undead", 401), &XSTR("bless", 192),
    &XSTR("wrath of god", 402), &XSTR("protect", 403), &XSTR("faith", 404), &XSTR("curse", 405),
    &XSTR("Jade Beam", 406), &XSTR("fire arrow", 407), &XSTR("fireball", 408), &XSTR("burning hands", 172),
    &XSTR("meteor storm", 409), &XSTR("wall of fire", 410), &XSTR("inferno", 411), &XSTR("dark wall", 412),
    &XSTR("icicle", 413), &XSTR("icy Sleep", 414), &XSTR("freeze", 415), &XSTR("blizzard", 416),
    &XSTR("ice coffin", 417), &XSTR("Blue Imp Sleep", 418), &XSTR("summon golem", 113),
    &XSTR("summon red minotaur", 115), &XSTR("summon blur", 419), &XSTR("summon blue imp", 119),
    &XSTR("summon blackfire elemental", 121), &XSTR("summon blood elemental", 123),
    &XSTR("summon poison elemental", 125), &XSTR("summon mist elemental", 127), &XSTR("summon wraith", 129),
    &XSTR("summon jade golem", 131), &XSTR("summon laharah", 133), &XSTR("summon samurai", 135),
    &XSTR("summon jade dragon", 137), &XSTR("summon water dragon", 139), &XSTR("summon wind dragon", 141),
    &XSTR("summon forest dragon", 143), &XSTR("banish summon", 145), &XSTR("generic arrow", 420),
    &XSTR("gsal fireball", 421), &XSTR("gsal bite", 422), &XSTR("pushback", 423), &XSTR("trip", 68),
    &XSTR("poisoning", 424), &XSTR("paralyze", 425), &XSTR("leech", 426), &XSTR("conversion", 427),
    &XSTR("iceshield", 428), &XSTR("fireshield", 429), &XSTR("energyshield", 430), &XSTR("sleep", 431),
    &XSTR("apdrain", 432), &XSTR("backstab", 74), &XSTR("Assess", 433), &XSTR("Aimed Attack", 434),
    &XSTR("Nightmare", 435), &XSTR("Rider Stun", 436), &XSTR("Phoenix burning hands", 437),
    &XSTR("Phoenix Inferno", 438), &XSTR("Titus stone gaze", 439), &XSTR("Pyrul fire arrow", 440),
    &XSTR("Pyrul fireball", 441), &XSTR("Pyrul meteor storm", 442), &XSTR("Ghost Paralyzing Fear", 443),
    &XSTR("Ghost Summon Bone Servant", 444), &XSTR("Ghost Summon Bone Mage", 445),
    &XSTR("Ghost Summon Undead", 446), &XSTR("Ghost death", 447), &XSTR("Urath blind", 448),
    &XSTR("Urath silence", 449), &XSTR("Urath death", 450), &XSTR("Urath curse", 451),
    &XSTR("Urath poison", 452), &XSTR("Urath paralyze", 453), &XSTR("Urath dark spear", 454),
    &XSTR("Dark Joseph Summon Blood", 455), &XSTR("Dark Joseph Summon Mist", 456),
    &XSTR("Dark Joseph Summon Poison", 457), &XSTR("Dark Joseph Summon Blackfire", 458),
    &XSTR("Dark Joseph Summon Blade", 459), &XSTR("Dark Joseph Summon Lich", 460),
    &XSTR("Dark Joseph Summon Brownstone Golem", 461), &XSTR("Dark Joseph Summon Jade Golem", 462),
    &XSTR("Mach meteor storm", 463), &XSTR("Dark void", 464), &XSTR("Laharah's scream", 465),
    &XSTR("Failed Dark void", 466), &XSTR("Jade Dragon", 138), &XSTR("Water Dragon", 140),
    &XSTR("Four Winds Dragon", 142), &XSTR("Forest Dragon", 144),
};

// 0x47a170 init_empty_47a180_thunk / 0x47a180 init_empty_47a180: a dynamic initialiser with an empty
// body (an object with an inline empty constructor, defined after spell_string_pool). Not reproduced.

// 0x47a190  Confidence: likely. One record per $Spell:, in table order. The effect block repeats until
// no effect or +Variation: line follows; +Misc effect: and +Banish effect: both take their $Sound: into
// +0x34. The +Recovery time:, +AI Recovery time: and +Loop Time: values are parsed but the record index
// is stored (`mov word ptr [esi-0x12], bp` with EBP = i, likewise -0x10 and -0xe): kept literally.
// Each class's records are consecutive; spell_groups gets (count, first) per class.
void spells_parse_table()
{
    TextFile tf;
    int i;
    int prev_class = NUM_SPELL_GROUPS;

    spell_string_pool.Reset();
    spell_sound_path = file_add_path("data\\sounds\\spells", ".wav");
    tf.Open("spells.tbl", path_tables);
    tf.Expect("#Spells");
    for (i = 0; i < MAX_SPELLS; i++) {
        spell_info *s = &spells[i];
        int cls;
        sound_ref *sound;

        s->index = i;
        tf.Expect("$Spell:");
        tf.Expect("+Name:");
        s->name = spell_string_pool.AddQuoted(&tf);
        tf.Expect("+Class:");
        cls = tf.ParseTypeString(spell_class_parse_names, NUM_SPELL_CLASSES);
        s->spell_class = cls;
        s->damage_type = -1;
        if (tf.Accept("+Damage type:"))
            s->damage_type = tf.ParseTypeString(damage_type_names, 7);
        s->hit_chance = 1.0f;
        if (tf.Accept("+Hit Chance:"))
            s->hit_chance = tf.ParseInt() * 0.01f;
        s->cast_effect = NULL;
        s->hit_effect = NULL;
        s->preloaded_hit_effect = NULL;
        s->misc_effect = NULL;
        s->banish_effect = NULL;
        s->variation = 0;
        for (;;) {
            if (tf.Accept("+Cast effect:")) {
                s->cast_effect = spell_string_pool.AddQuoted(&tf);
                sound = &s->cast_sound;
            } else if (tf.Accept("+Hit effect:")) {
                s->hit_effect = spell_string_pool.AddQuoted(&tf);
                sound = &s->hit_sound;
            } else if (tf.Accept("+Preloaded Hit effect:")) {
                s->preloaded_hit_effect = spell_string_pool.AddQuoted(&tf);
                sound = &s->hit_sound;
            } else if (tf.Accept("+Misc effect:")) {
                s->misc_effect = spell_string_pool.AddQuoted(&tf);
                sound = &s->misc_sound;
            } else if (tf.Accept("+Banish effect:")) {
                s->banish_effect = spell_string_pool.AddQuoted(&tf);
                sound = &s->misc_sound;
            } else if (tf.Accept("+Variation:")) {
                s->variation = tf.ParseTypeString(variation_names, 0xc2);
                continue;
            } else
                break;
            spells_parse_sound(&tf, s, sound);
        }
        tf.Expect("+Skill:");
        s->skill = tf.ParseFloat() * 0.01f;
        tf.Expect("+Cost:");
        s->cost = tf.ParseFloat();
        tf.Expect("+Distance:");
        s->distance = tf.ParseFloat();
        s->die = 0;
        if (tf.Accept("+Die:"))
            s->die = tf.ParseInt();
        s->duration = -1;
        if (tf.Accept("+Duration:"))
            s->duration = (short)tf.ParseInt();
        s->recovery_time = s->duration;
        if (tf.Accept("+Recovery time:")) {
            tf.ParseInt();
            s->recovery_time = (short)i;
        }
        s->ai_recovery_time = -1;
        if (tf.Accept("+AI Recovery time:")) {
            tf.ParseInt();
            s->ai_recovery_time = (short)i;
        }
        s->emerge_time = 0;
        if (tf.Accept("+Emerge time:"))
            s->emerge_time = tf.ParseInt();
        s->loop_time = 0;
        if (tf.Accept("+Loop Time:")) {
            tf.ParseInt();
            s->loop_time = (short)i;
        }
        s->target_radius = 0.0f;
        if (tf.Accept("+Target radius:"))
            s->target_radius = tf.ParseFloat();
        s->flags = 0;
        for (;;) {
            if (tf.Accept("+Offensive"))
                s->flags |= SPELL_OFFENSIVE;
            else if (tf.Accept("+Beneficial"))
                s->flags |= SPELL_BENEFICIAL;
            else if (tf.Accept("+Target character"))
                s->flags |= SPELL_TARGET_CHARACTER;
            else if (tf.Accept("+Target location"))
                s->flags |= SPELL_TARGET_LOCATION;
            else if (tf.Accept("+Server authorize"))
                s->flags |= SPELL_SERVER_AUTHORIZE;
            else if (tf.Accept("+Fade Props"))
                s->flags |= SPELL_FADE_PROPS;
            else if (tf.Accept("+Status change"))
                s->flags |= SPELL_STATUS_CHANGE;
            else {
                if (tf.Accept("+Hit only"))
                    s->flags |= SPELL_HIT_ONLY;
                break;
            }
        }
        if (cls != prev_class) {
            spell_groups[cls].count = 0;
            spell_groups[cls].first = s;
        }
        spell_groups[cls].count++;
        prev_class = cls;
    }
    tf.Expect("#End");
    tf.Close();
    for (i = 0; i < MAX_SPELL_EFFECTS; i++) {
        spell_effects[i].used = 0;
        spell_effects[i].slot = 0x3f;
    }
}

// 0x47a6c0  Confidence: exact. The spell record argument is not read.
void spells_parse_sound(TextFile *tf, spell_info *spell, sound_ref *ref)
{
    char name[0x80];

    ref->name = NULL;
    ref->handle = -1;
    if (tf->Accept("$Sound:")) {
        tf->ParseString(name, sizeof(name), '"', '"');
        ref->name = spell_string_pool.Add(name);
        ref->params[0] = (ubyte)(tf->ParseFloat() * 255.0f);
        ref->params[1] = (ubyte)tf->ParseFloat();
        ref->params[2] = (ubyte)tf->ParseFloat();
    }
}

// 0x47a760  Confidence: exact (tail jump).
void spells_free_strings()
{
    spell_string_pool.Release();
}

// 0x47a770  Confidence: exact.
void spells_reset_runtime()
{
    int i;

    for (i = 0; i < MAX_SPELL_MISSILES; i++)
        spell_missiles[i].handle = -1;
    for (i = 0; i < MAX_SPELLS; i++) {
        spells[i].cast_sound.handle = -1;
        spells[i].hit_sound.handle = -1;
        spells[i].misc_sound.handle = -1;
        spells[i].banish_sound.handle = -1;
    }
    for (i = 0; i < MAX_SPELL_EFFECTS; i++) {
        spell_effects[i].used = 0;
        spell_effects[i].slot = 0x3f;
    }
}

// 0x47a7c0  Confidence: exact.
int spell_find_by_name(const char *name)
{
    int i;

    for (i = 0; i < MAX_SPELLS; i++)
        if (!_stricmp(name, spells[i].name))
            return i;
    return -1;
}

// 0x47a800  Confidence: exact.
int spell_find_by_cast_id(int id)
{
    int i;

    for (i = 0; i < MAX_SPELLS; i++)
        if (spells[i].variation == id)
            return i;
    return -1;
}

// 0x47a830  Confidence: exact.
int spell_get_group(int spell)
{
    int g, i;

    for (g = 0; g < NUM_SPELL_GROUPS; g++)
        for (i = 0; i < spell_groups[g].count; i++)
            if (spell_groups[g].first[i].index == spell)
                return g;
    return -1;
}

// 0x47a870  Confidence: exact.
const char *spell_get_cast_effect(int spell)
{
    return spells[spell].cast_effect;
}

// 0x47a880  Confidence: exact.
const char *spell_get_effect_1c(int spell)
{
    return spells[spell].hit_effect;
}

// 0x47a890  Confidence: exact (neg / sbb / and).
const char *spell_get_hit_effect(int spell)
{
    if (spells[spell].preloaded_hit_effect)
        return spells[spell].preloaded_hit_effect;
    return spells[spell].hit_effect ? spell_default_hit_effect : NULL;
}

// 0x47a8c0  Confidence: exact (byte-indexed jump table 0x47a964 for 2..0x5a).
int damage_type_to_status(int spell)
{
    switch (spell) {
    case 0x02: return STATUS_REGEN;                       // regenerate
    case 0x06: case 0x3c: case 0x59: return STATUS_POISON;   // Poison, poisoning, Urath poison
    case 0x07: case 0x55: return STATUS_BLIND;            // blind, Urath blind
    case 0x08: case 0x56: return STATUS_SILENCE;          // silence, Urath silence
    case 0x0b: return 8;                                  // invisibility
    case 0x0e: return 7;                                  // light
    case 0x10: return STATUS_POWER;                       // empower
    case 0x13: case 0x15: return STATUS_BLESS;            // bless, protect
    case 0x16: return STATUS_FAITH;                       // faith
    case 0x17: case 0x58: return STATUS_CURSE;            // curse, Urath curse
    case 0x21: case 0x25: case 0x43: return STATUS_SLEEP; // icy sleep, Blue Imp sleep, sleep
    case 0x22: case 0x3d: case 0x49: case 0x4c: case 0x50: case 0x5a:
        return STATUS_PARALYZE;     // freeze, paralyze, Rider stun, Titus gaze, Ghost fear, Urath paralyze
    case 0x24: return 0x10;                               // ice coffin
    }
    return -1;
}

// 0x47a9c0  Confidence: exact (dword jump table 0x47aa2c for 0..0x11).
float status_effect_modifier(int status, int level)
{
    switch (status) {
    case STATUS_FAITH: case STATUS_CURSE: case STATUS_BLESS:
        return level * 4.0f * 0.01f;
    case STATUS_REGEN:
        return level * 2.0f;
    case 8:
        if (level >= 50)
            return 1.0f;
        break;
    case STATUS_POISON:
        return -(level * 2.0f);
    case 0xe:
        return -(level * 8.0f);
    case 0xf:
        return -(level * 4.0f);
    case 0x10:
        return level * 10.0f;
    }
    return 0.0f;
}

// 0x47aa80  Confidence: exact (byte-indexed jump table 0x47aaf8 for 2..0x5a).
int spell_get_duration_ms(int spell, int level)
{
    switch (spell) {
    case 0x02: case 0x06: case 0x13: case 0x15: case 0x16: case 0x3c: case 0x59:
        return 120000;
    case 0x07: case 0x08: case 0x0e: case 0x10: case 0x17: case 0x21: case 0x25: case 0x3d: case 0x43:
    case 0x55: case 0x56: case 0x58: case 0x5a:
        return 60000;
    case 0x0b:
        return level * 3000;
    case 0x22:
        return 30000;
    case 0x49:
        return 3000;
    case 0x4c:
        return 180000;
    case 0x50:
        return 20000;
    }
    return 0;
}

// 0x47ab60  Confidence: exact (byte-indexed jump table 0x47abc0 for 0..0x11). Status 8 (invisible)
// lasts invisibility's +Loop Time: seconds, which spells_parse_table sets to the record index (11).
int status_effect_duration(int status)
{
    switch (status) {
    case STATUS_REGEN: return 20000;
    case 8:            return spells[0x0b].loop_time * 1000;
    case STATUS_POISON: return 10000;
    case 0xe: case 0xf: return 2000;
    case 0x11:         return 5000;
    }
    return 0;
}

// 0x47c510  Confidence: likely (the hit chance reuses the caster's argument slot). Hit chance: the
// species 0xb6 caster uses level x 2 %, Ghost Paralyzing Fear in ghost stage 3 0.5, source type 8 0.3,
// otherwise the table's +Hit Chance:.
void spell_compute_resist(float *out_magic_resist, float *out_resist, int spell, Character *caster,
                          Character *target, int power, int source_type)
{
    float hit;
    float magic;

    if (caster->info->species == 0xb6)
        hit = caster->level * 2.0f * 0.01f;
    else if (spell == SPELL_GHOST_PARALYZING_FEAR && ghost_stage == 3)
        hit = 0.5f;
    else if (source_type == 8)
        hit = 0.3f;
    else
        hit = spells[spell].hit_chance;
    magic = target ? target->get_resistance(3) : 0.0f;
    *out_magic_resist = magic;
    *out_resist = 1.0f - (1.0f - (magic + 0.5f - power * 0.04f)) * hit;
}

// 0x47dfc0  Confidence: exact. True when a record is free (the name in sum.symbols says the opposite;
// spell_cast_launch_effects only adds a record when this is true).
bool spell_effect_pool_full()
{
    int i;

    for (i = 0; i < MAX_SPELL_EFFECTS; i++)
        if (!spell_effects[i].used)
            return true;
    return false;
}

// 0x47dfe0  Confidence: exact.
void spell_play_cast_sound(int spell, Character *c)
{
    int h;

    sound_ref_load(&spells[spell].cast_sound, false);
    h = spells[spell].cast_sound.handle;
    if (h != -1)
        sound_play_3d_at(h, &c->pos);
}

// 0x47e020  Confidence: exact. The summon spells 0x32..0x35 (water, wind, forest dragon, banish) play
// once as a 2D sound and drop the handle.
void spell_play_hit_sound(int spell, const vector3 *pos)
{
    int h;

    sound_ref_load(&spells[spell].hit_sound, false);
    h = spells[spell].hit_sound.handle;
    if (h != -1) {
        if (spell >= 0x32 && spell <= 0x35) {
            game_play_sound(h, 1.0f, 0.0f, 0, 0);
            spells[spell].hit_sound.handle = -1;
            return;
        }
        sound_play_3d_at(h, pos);
    }
}

// 0x47e090  Confidence: exact (same body on +0x34).
void spell_play_sound_34(int spell, const vector3 *pos)
{
    int h;

    sound_ref_load(&spells[spell].misc_sound, false);
    h = spells[spell].misc_sound.handle;
    if (h != -1) {
        if (spell >= 0x32 && spell <= 0x35) {
            game_play_sound(h, 1.0f, 0.0f, 0, 0);
            spells[spell].misc_sound.handle = -1;
            return;
        }
        sound_play_3d_at(h, pos);
    }
}

// 0x47e100  Confidence: likely (the two ground points are copied into x / z pairs first: the stores
// 0x47e17f-0x47e18f). The effect gets an avoidance box from the middle of ground points 7..1 and 8..13
// along their direction.
int spell_place_ground_effect(int spell, int caster, const vector3 *pos, int owner, const matrix *orient)
{
    int h;
    effect_object *fx;
    vector3 p1, p2;
    vector2 centre, end, dir;
    float len;
    avoid_area_desc aa;

    h = fx_create_ground_effect(spell, caster, pos, orient);
    if (h == -1)
        return h;
    fx = (effect_object *)obj_from_handle_b(h);
    if (!fx)
        return -1;
    fx->instance->process(0, &fx->pos, &fx->orient);
    spell_find_ground_points(&p1, 7, 0, fx);
    spell_find_ground_points(&p2, 8, 0xe, fx);
    centre.x = p1.x;
    centre.y = p1.z;
    end.x = p2.x;
    end.y = p2.z;
    centre.x = (centre.x + end.x) * 0.5f;
    centre.y = (centre.y + end.y) * 0.5f;
    dir.x = end.x - centre.x;
    dir.y = end.y - centre.y;
    len = dir.normalize() + 1.5f;
    aa.cx = centre.x;
    aa.cz = centre.y;
    aa.ax = dir.x;
    aa.az = dir.y;
    aa.bx = -dir.y;
    aa.bz = dir.x;
    aa.shape = 0;
    aa.extent_a = len;
    aa._unk00 = 0;
    aa.extent_b = 1.5f;
    aa._unk28 = 0.5f;
    aa.owner = owner;
    fx->avoid_area = avoid_area_add(&aa);
    return h;
}

// 0x47e280  Confidence: likely (do-while: no entry test of first != last). Walks the "$groundNN" tags
// from first towards last; once one tag fails the floor probe, it and every later tag get
// get_tag_transform(tag, 1, 1) (result not used).
void spell_find_ground_points(vector3 *out, int first, int last, effect_object *fx)
{
    int step = (first >= last) ? -1 : 1;
    bool failed = false;
    int i = first;
    char name[0x40];

    out->x = fx->pos.x;
    out->y = fx->pos.y;
    out->z = fx->pos.z;
    do {
        int tag;

        sprintf(name, "$ground%02d", i);
        tag = fx->instance->find_tag(name);
        if (!failed) {
            if (!spell_probe_ground_point(fx, tag, out)) {
                fx->instance->get_tag_transform(tag, 1, 1);
                failed = true;
            }
        } else
            fx->instance->get_tag_transform(tag, 1, 1);
        i += step;
    } while (i != last);
}

// 0x47e330  Confidence: likely (the vector temporaries; vector3's out-of-line operator= / copy
// constructor are not called for the result copies, which are inline here). Moves the node's world
// position onto the floor below it (ray from 1 m above the point, 2 m down) and writes the node back.
bool spell_probe_ground_point(effect_object *fx, int node, vector3 *point)
{
    vector3 v;
    vector3 pos;
    lobj_ray ray;
    lobj_hit hit;
    matrix orient;
    vector3 r;

    fx->instance->get_node_transform(&pos, &orient, node);
    r = pos.unrotated(fx->orient);
    v.x = r.x + fx->pos.x;
    v.y = r.y + fx->pos.y;
    v.z = r.z + fx->pos.z;
    point->x = v.x;
    point->z = v.z;
    point->y = point->y + 1.0f;
    ray.start.x = v.x;
    ray.start.y = point->y;
    ray.start.z = v.z;
    ray.dir.x = 0.0f;
    ray.dir.y = -2.0f;
    ray.dir.z = 0.0f;
    ray._unk18 = 0.01f;
    ray.flags = 0;
    ray.require_flags = 0;
    ray.exclude_flags = 0;
    lobj_raycast_all(&ray, &hit);
    if (hit.num_hits > 0) {
        point->y = hit.t * ray.dir.y + point->y - ray._unk18;
        v.x = v.x - fx->pos.x;
        v.y = point->y - fx->pos.y;
        v.z = v.z - fx->pos.z;
        pos = v.rotated(fx->orient);
        fx->instance->get_node_world_transform(node, pos, orient, 0, 0, 1);
        return true;
    }
    return false;
}

// 0x47e4d0  Confidence: exact. Marks the object as a fireball (spell 0x1a) ground effect with an
// avoidance box of 2 x 2 m.
void spell_effect_obj_set_timer(int caster, int handle, const vector3 *pos, int owner, const matrix *orient)
{
    effect_object *fx = (effect_object *)obj_from_handle_b(handle);
    avoid_area_desc aa;

    if (fx) {
        fx->kind = 7;
        fx->timer.SetTimeout(0);
        fx->caster = caster;
        fx->spell = SPELL_FIREBALL;
        aa.cx = pos->x;
        aa.cz = pos->z;
        aa.ax = orient->rvec.x;
        aa.az = orient->rvec.z;
        aa.bx = orient->fvec.x;
        aa.bz = orient->fvec.z;
        aa.shape = 0;
        aa._unk00 = 0;
        aa.extent_a = 2.0f;
        aa.extent_b = 2.0f;
        aa._unk28 = 0.5f;
        aa.owner = owner;
        fx->avoid_area = avoid_area_add(&aa);
    }
}

// 0x47e590  Confidence: exact. Blizzard's hit effect (spells[0x23] +0x1c) with an avoidance shape 1 of
// 3.3 m.
int spell_spawn_hit_fx(int caster, const vector3 *pos, int owner, const matrix *orient)
{
    int h;
    effect_object *fx;
    avoid_area_desc aa;

    h = fx_add_effect(spells[SPELL_BLIZZARD].hit_effect, pos, orient, 1);
    if (h == -1)
        return h;
    fx = (effect_object *)obj_from_handle_b(h);
    if (!fx)
        return -1;
    fx->caster = caster;
    fx->spell = SPELL_BLIZZARD;
    fx->instance->process(0, &fx->pos, &fx->orient);
    aa.cx = pos->x;
    aa.cz = pos->z;
    aa.ax = orient->rvec.x;
    aa.az = orient->rvec.z;
    aa.bx = orient->fvec.x;
    aa.bz = orient->fvec.z;
    aa._unk00 = 0;
    aa.shape = 1;
    aa._unk28 = 0.5f;
    aa.extent_b = 3.3f;
    aa.extent_a = 3.3f;
    aa.owner = owner;
    fx->avoid_area = avoid_area_add(&aa);
    return h;
}

// 0x47e680  Confidence: exact.
int spell_missile_remove(int handle)
{
    int i;

    for (i = 0; i < MAX_SPELL_MISSILES; i++)
        if (spell_missiles[i].handle == handle) {
            spell_missiles[i].handle = -1;
            break;
        }
    if (i == MAX_SPELL_MISSILES)
        return -1;
    return i;
}

// 0x47e6c0  Confidence: likely. Records the "$missile" node of the effect object (in the caster's
// frame) for a later spell_fire_missile. When all 20 slots are taken, the slots whose effect is gone are
// freed, but this missile is not recorded (both exits of the second search return).
void spell_launch_missile(Character *caster, int handle)
{
    effect_object *fx;
    int tag, i;
    vector3 pos;
    matrix orient;

    if (handle == -1)
        return;
    fx = (effect_object *)obj_from_handle_b(handle);
    if (!fx)
        return;
    tag = fx->instance->find_tag("$missile");
    if (tag < 0)
        return;
    for (i = 0; i < MAX_SPELL_MISSILES; i++)
        if (spell_missiles[i].handle == -1)
            break;
    if (i >= MAX_SPELL_MISSILES) {
        for (i = 0; i < MAX_SPELL_MISSILES; i++)
            if (spell_missiles[i].handle != -1 && !obj_from_handle_b(spell_missiles[i].handle))
                spell_missiles[i].handle = -1;
        for (i = 0; i < MAX_SPELL_MISSILES; i++)
            if (spell_missiles[i].handle == -1)
                return;
        return;
    }
    fx->instance->get_node_transform(&pos, &orient, tag);
    matrix43_compose(&caster->orient, &caster->pos, &orient, &pos);
    spell_missiles[i].handle = handle;
    spell_missiles[i].pos.x = pos.x;
    spell_missiles[i].pos.y = pos.y;
    spell_missiles[i].pos.z = pos.z;
    spell_missiles[i].orient.rvec.x = orient.rvec.x;
    spell_missiles[i].orient.rvec.y = orient.rvec.y;
    spell_missiles[i].orient.rvec.z = orient.rvec.z;
    spell_missiles[i].orient.uvec.x = orient.uvec.x;
    spell_missiles[i].orient.uvec.y = orient.uvec.y;
    spell_missiles[i].orient.uvec.z = orient.uvec.z;
    spell_missiles[i].orient.fvec.x = orient.fvec.x;
    spell_missiles[i].orient.fvec.y = orient.fvec.y;
    spell_missiles[i].orient.fvec.z = orient.fvec.z;
}

// 0x47e830  Confidence: exact. An explicit start / orientation wins; otherwise a recorded missile slot
// is used (and freed) when there is a target.
int spell_fire_missile(int spell, int missile, Character *caster, Character *target,
                       const vector3 *start, const matrix *orient)
{
    int h = -1;
    int i;

    if (start && orient && target) {
        h = fx_create_missile(spell, start, orient, caster->handle, target->handle);
        spell_play_sound_34(spell, start);
    }
    if (missile != -1) {
        i = spell_missile_remove(missile);
        if (i != -1 && target && !start) {
            h = fx_create_missile(spell, &spell_missiles[i].pos, &spell_missiles[i].orient, caster->handle,
                                  target->handle);
            spell_play_sound_34(spell, &spell_missiles[i].pos);
        }
    }
    return h;
}

// 0x47e8e0 init_empty_47e8f0_thunk / 0x47e8f0 init_empty_47e8f0: a second empty dynamic initialiser,
// so an object with an inline empty constructor is defined here (between spell_fire_missile and
// summon_create). Not reproduced.

// 0x47e900 summon_create: not written yet.

// 0x47f270  Confidence: exact (tail jump to mp_summon_claim_release).
void summon_dismiss(int handle)
{
    if (cur_summon) {
        cur_summon->ai.class_die();
        if (game_flags & 2)
            mp_summon_claim_release();
    }
}

// 0x47f2a0  Confidence: likely (byte-indexed jump table 0x47f480 for 0..0x43; the shared tails are the
// compiler's). How useful a spell is on the target: 1.0 or 0.0 for most, 1 - resistance for the damage
// spells. Cure tests status 0 twice (literal).
float spell_target_value(Character *target, int spell)
{
    switch (spell) {
    case 0x00: case 0x04:                                 // heal, vitalize
        if (target->get_max_stat(0) > target->hp)
            return 1.0f;
        return 0.0f;
    case 0x01:                                            // cure
        if (target->has_status(STATUS_BLIND) || target->has_status(STATUS_BLIND) ||
            target->has_status(STATUS_SILENCE) || target->has_status(STATUS_CURSE) ||
            target->has_status(STATUS_SLEEP) || target->has_status(STATUS_PARALYZE) ||
            target->has_status(STATUS_POISON) || target->has_status(STATUS_CRIPPLE) ||
            target->has_status(STATUS_SLOW))
            return 1.0f;
        return 0.0f;
    case 0x03: case 0x05:                                 // resurrect, revive
        if (target->flags & CF_DEAD)
            return 0.0f;
        return 1.0f;
    case 0x07: case 0x09:                                 // blind, black cloud
        if (target->info->flags & 0x80)
            return 0.0f;
        return 1.0f;
    case 0x08:                                            // silence
        if (target->info->flags & 0x100)
            return 0.0f;
        goto not_boss;
    case 0x0d:                                            // Life Drain
        if (target->level > 1)
            goto not_boss;
        return 0.0f;
    case 0x0c: case 0x17: case 0x22: case 0x24: case 0x25: case 0x3d: case 0x43:
    not_boss:
        if (target->type == 9 || (target->flags & 0x40))
            return 0.0f;
        return 1.0f;
    case 0x0f:                                            // lightning
        return 1.0f - target->get_resistance(2);
    case 0x11:                                            // power leech
        if (target->ap <= 0)
            return 0.0f;
        return 1.0f;
    case 0x19: case 0x1a: case 0x1c: case 0x1d: case 0x1e:   // fire
        return 1.0f - target->get_resistance(0);
    case 0x20: case 0x21: case 0x23:                      // ice
        return 1.0f - target->get_resistance(1);
    }
    return 1.0f;
}

// 0x47f4d0  Confidence: likely (EBP frame with an 8-aligned stack: VC6 does this for a double; the
// floor argument is the only one). count dice of size die; damage (negative) is scaled by the target's
// resistance to the spell's damage type and by 1 - resist, and may not turn into healing. Party members
// take 15/30/45 % more spell damage in a party of 3/4/5, other targets as much less.
float spell_roll_amount(Character *target, int spell, int count, int die, float resist)
{
    float total = 0.0f;
    float party;
    int i, n;

    if (count > 0 && die == 0)
        return (float)-count;
    if (count == 0 || die == 0)
        return 0.0f;
    for (i = 0; i < count; i++)
        total += (int)floor(frand() * die + 1.0f);
    if (!No_resist && total < 0.0f) {
        if (target)
            total = (1.0f - target->get_resistance(spells[spell].damage_type)) * total;
        if (total < 0.0f && resist != 0.0f) {
            total = (1.0f - resist) * total;
            if (total >= 0.0f)
                total = 0.0f;
        }
    }
    n = party_count_active();
    party = 0.0f;
    if (n < 3)
        party = 0.0f;
    else if (n == 3)
        party = 0.15f;
    else if (n == 4)
        party = 0.3f;
    else if (n == 5)
        party = 0.45f;
    if (target && target->type != 7)
        party = -party;
    if (total > 0.0f)
        party = 0.0f;
    return (party + 1.0f) * total;
}

// 0x47f660  Confidence: exact.
spell_effect *spell_effect_add(const spell_effect *rec)
{
    spell_effect *e = spell_effect_alloc();

    if (e) {
        *e = *rec;
        e->used = 1;
    }
    return e;
}

// 0x47f690  Confidence: likely (the two bit-field stores are one `or 0x7f`).
spell_effect *spell_effect_alloc()
{
    int i;

    for (i = 0; i < MAX_SPELL_EFFECTS; i++)
        if (!spell_effects[i].used) {
            spell_effects[i].used = 1;
            spell_effects[i].slot = 0x3f;
            return &spell_effects[i];
        }
    return NULL;
}

// 0x47f6d0  Confidence: exact (signed pointer difference by 0x34; no lower bound check).
void spell_effect_free(spell_effect *e)
{
    if (e - spell_effects < MAX_SPELL_EFFECTS)
        e->used = 0;
}

// 0x47f700 init_ccmd_summon_thunk / 0x47f710 init_ccmd_summon. Confidence: exact.
console_command ccmd_summon("summon", "Summon a creature", cmd_summon);   // 0x22e9240

// 0x47f730  Confidence: likely. Only "golem" and "blackfire" are known; there is no
// calling_a_command_function test, so the help and status passes of the console also summon.
void cmd_summon()
{
    int spell = -1;
    Character *c;
    vector3 pos, ofs;

    process_command(ARG_STRING);
    if (!strcmp(next_arg, "golem"))
        spell = SPELL_SUMMON_GOLEM;
    if (!strcmp(next_arg, "blackfire"))
        spell = SPELL_SUMMON_BLACKFIRE;
    else if (spell == -1) {
        console_printf("Unknown summon type : \"%s\"", next_arg);
        return;
    }
    c = cur_character;
    pos.x = c->pos.x;
    pos.y = c->pos.y;
    pos.z = c->pos.z;
    ofs.x = c->orient.fvec.x * 4.0f;
    ofs.y = c->orient.fvec.y * 4.0f;
    ofs.z = c->orient.fvec.z * 4.0f;
    pos.x += ofs.x;
    pos.y += ofs.y;
    pos.z += ofs.z;
    summon_create(c, &pos, 0, spell, -1);
}
