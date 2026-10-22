// spells.cpp - Sum.exe v1.40, code 0x479b60-0x47f857 (module "spells.cpp" in sum.symbols / strings.tbl;
// no path string, so the directory of engine/spells/spells.h is guessed).
// Written from the Sum.exe disassembly (tools/asm.mjs fn; the jump tables decoded from the raw bytes).
// Target: VC++ 6.0 (cl 12.00.8966), /O2 like the other optimised units (no frame pointer, 0x90 padding).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: the first function is init_strptrs_22e92a0_thunk (0x479b60), the last cmd_summon
// (0x47f730-0x47f857). Every function is written: decomp75 added spell_spawn_creature and summon_create
// (D5702-D5703); decomp73 the rest of the casting half: spell_prepare_cast, spell_cast_execute
// and spell_cast_launch_effects (D5507-D5509).
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
#include "vsdk/ca/skeleton.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/geom/vfx.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "summoner/interface/new_hud.h"
#include "summoner/multi/multispell.h"
#include "summoner/unnamed_cutscene_0x434900.h"
#include "engine/camera/camera.h"
#include "engine/rendering/unnamed_render_scene_0x481680.h"
#include "engine/objects/objects.h"
#include "engine/objects/item_paging.h"
#include "levelscripts/savegame.h"

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
// 0x47a130 sinit_spell_string_pool (call 0x47a140, jmp 0x47a150; entry 234). Confidence: exact.
// 0x47a140 init_spell_string_pool (StringPool(0x3000)). Confidence: exact.
// 0x47a150 register_dtor_spell_string_pool (atexit(0x47a160)). Confidence: exact.
// 0x47a160 dtor_spell_string_pool (jmp StringPool::~StringPool). Confidence: exact.
StringPool spell_string_pool(0x3000);                  // 0x22e9228
// ccmd_summon 0x22e9240 is defined with its command below (its initialiser runs there).
int spell_sound_path;                                  // 0x22e925c
spell_group spell_groups[NUM_SPELL_GROUPS];            // 0x22e9260
// localize_strings slot tables (dynamic initialisers: &localize_strings[id] is stored; one initialiser
// per table, element order = store order); these two are the unit's first static initialisers, so they
// are defined first in the original (before the string pool), although their .bss comes last.
// 0x479b60 init_strptrs_22e92a0_thunk (jmp, static_initializers entry 232). Confidence: exact.
// 0x479b70 init_strptrs_22e92a0: VC6 loads "Unused." (361) into EAX once and stores it five times.
//          Confidence: likely.
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
// 0x479ce0 init_strptrs_22e9338_thunk (jmp, entry 233). Confidence: exact.
// 0x479cf0 init_strptrs_22e9338 (59 slot stores). Confidence: likely.
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

// A dynamic initialiser with an empty body (an object with an inline empty constructor, defined after
// spell_string_pool). Not reproduced.
// 0x47a170 init_empty_47a180_thunk (jmp, entry 235). Confidence: exact.
// 0x47a180 init_empty_47a180 (a bare ret). Confidence: unsure (which object).

// 0x47a190  Confidence: likely (D14404, closer: the record indexed as spells[i] and prev_class set after
// the pool reset; left: EBX / EDI swapped for cls and the record pointer). One record per $Spell:, in table order. The effect block repeats until
// no effect or +Variation: line follows; +Misc effect: and +Banish effect: both take their $Sound: into
// +0x34. The +Recovery time:, +AI Recovery time: and +Loop Time: values are parsed but the record index
// is stored (`mov word ptr [esi-0x12], bp` with EBP = i, likewise -0x10 and -0xe): kept literally.
// Each class's records are consecutive; spell_groups gets (count, first) per class.
void spells_parse_table()
{
    TextFile tf;
    int i;
    int prev_class;

    spell_string_pool.Reset();
    prev_class = NUM_SPELL_GROUPS;
    spell_sound_path = file_add_path("data\\sounds\\spells", ".wav");
    tf.Open("spells.tbl", path_tables);
    tf.Expect("#Spells");
    for (i = 0; i < MAX_SPELLS; i++) {
        int cls;
        sound_ref *sound;

        spells[i].index = i;
        tf.Expect("$Spell:");
        tf.Expect("+Name:");
        spells[i].name = spell_string_pool.AddQuoted(&tf);
        tf.Expect("+Class:");
        cls = tf.ParseTypeString(spell_class_parse_names, NUM_SPELL_CLASSES);
        spells[i].spell_class = cls;
        spells[i].damage_type = -1;
        if (tf.Accept("+Damage type:"))
            spells[i].damage_type = tf.ParseTypeString(damage_type_names, 7);
        spells[i].hit_chance = 1.0f;
        if (tf.Accept("+Hit Chance:"))
            spells[i].hit_chance = tf.ParseInt() * 0.01f;
        spells[i].cast_effect = NULL;
        spells[i].hit_effect = NULL;
        spells[i].preloaded_hit_effect = NULL;
        spells[i].misc_effect = NULL;
        spells[i].banish_effect = NULL;
        spells[i].variation = 0;
        for (;;) {
            if (tf.Accept("+Cast effect:")) {
                spells[i].cast_effect = spell_string_pool.AddQuoted(&tf);
                sound = &spells[i].cast_sound;
            } else if (tf.Accept("+Hit effect:")) {
                spells[i].hit_effect = spell_string_pool.AddQuoted(&tf);
                sound = &spells[i].hit_sound;
            } else if (tf.Accept("+Preloaded Hit effect:")) {
                spells[i].preloaded_hit_effect = spell_string_pool.AddQuoted(&tf);
                sound = &spells[i].hit_sound;
            } else if (tf.Accept("+Misc effect:")) {
                spells[i].misc_effect = spell_string_pool.AddQuoted(&tf);
                sound = &spells[i].misc_sound;
            } else if (tf.Accept("+Banish effect:")) {
                spells[i].banish_effect = spell_string_pool.AddQuoted(&tf);
                sound = &spells[i].misc_sound;
            } else if (tf.Accept("+Variation:")) {
                spells[i].variation = tf.ParseTypeString(variation_names, 0xc2);
                continue;
            } else
                break;
            spells_parse_sound(&tf, &spells[i], sound);
        }
        tf.Expect("+Skill:");
        spells[i].skill = tf.ParseFloat() * 0.01f;
        tf.Expect("+Cost:");
        spells[i].cost = tf.ParseFloat();
        tf.Expect("+Distance:");
        spells[i].distance = tf.ParseFloat();
        spells[i].die = 0;
        if (tf.Accept("+Die:"))
            spells[i].die = tf.ParseInt();
        spells[i].duration = -1;
        if (tf.Accept("+Duration:"))
            spells[i].duration = (short)tf.ParseInt();
        spells[i].recovery_time = spells[i].duration;
        if (tf.Accept("+Recovery time:")) {
#ifdef FIXED_BUILD
            // FIXED_BUILD: G12 SpellRecoveryTimes. Store the parsed value, not the record index (here and
            // for +AI Recovery time: and +Loop Time: below).
            spells[i].recovery_time = (short)tf.ParseInt();
#else
            tf.ParseInt();
            spells[i].recovery_time = (short)i;
#endif
        }
        spells[i].ai_recovery_time = -1;
        if (tf.Accept("+AI Recovery time:")) {
#ifdef FIXED_BUILD
            // part of FIXED_BUILD G12
            spells[i].ai_recovery_time = (short)tf.ParseInt();
#else
            tf.ParseInt();
            spells[i].ai_recovery_time = (short)i;
#endif
        }
        spells[i].emerge_time = 0;
        if (tf.Accept("+Emerge time:"))
            spells[i].emerge_time = tf.ParseInt();
        spells[i].loop_time = 0;
        if (tf.Accept("+Loop Time:")) {
#ifdef FIXED_BUILD
            // part of FIXED_BUILD G12
            spells[i].loop_time = (short)tf.ParseInt();
#else
            tf.ParseInt();
            spells[i].loop_time = (short)i;
#endif
        }
        spells[i].target_radius = 0.0f;
        if (tf.Accept("+Target radius:"))
            spells[i].target_radius = tf.ParseFloat();
        spells[i].flags = 0;
        for (;;) {
            if (tf.Accept("+Offensive"))
                spells[i].flags |= SPELL_OFFENSIVE;
            else if (tf.Accept("+Beneficial"))
                spells[i].flags |= SPELL_BENEFICIAL;
            else if (tf.Accept("+Target character"))
                spells[i].flags |= SPELL_TARGET_CHARACTER;
            else if (tf.Accept("+Target location"))
                spells[i].flags |= SPELL_TARGET_LOCATION;
            else if (tf.Accept("+Server authorize"))
                spells[i].flags |= SPELL_SERVER_AUTHORIZE;
            else if (tf.Accept("+Fade Props"))
                spells[i].flags |= SPELL_FADE_PROPS;
            else if (tf.Accept("+Status change"))
                spells[i].flags |= SPELL_STATUS_CHANGE;
            else {
                if (tf.Accept("+Hit only"))
                    spells[i].flags |= SPELL_HIT_ONLY;
                break;
            }
        }
        if (cls != prev_class) {
            spell_groups[cls].count = 0;
            spell_groups[cls].first = &spells[i];
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
    case 0x08: case 0x56: return STATUS_SILENCE;          // silence, Urath silence
    case 0x07: case 0x55: return STATUS_BLIND;            // blind, Urath blind
    case 0x13: case 0x15: return STATUS_BLESS;            // bless, protect
    case 0x17: case 0x58: return STATUS_CURSE;            // curse, Urath curse
    case 0x16: return STATUS_FAITH;                       // faith
    case 0x10: return STATUS_POWER;                       // empower
    case 0x22: case 0x3d: case 0x49: case 0x4c: case 0x50: case 0x5a:
        return STATUS_PARALYZE;     // freeze, paralyze, Rider stun, Titus gaze, Ghost fear, Urath paralyze
    case 0x21: case 0x25: case 0x43: return STATUS_SLEEP; // icy sleep, Blue Imp sleep, sleep
    case 0x06: case 0x3c: case 0x59: return STATUS_POISON;   // Poison, poisoning, Urath poison
    case 0x24: return 0x10;                               // ice coffin
    case 0x0e: return 7;                                  // light
    case 0x0b: return 8;                                  // invisibility
    }
    return -1;
}

// 0x47a9c0  Confidence: exact (dword jump table 0x47aa2c for 0..0x11: every status has a case; the bodies in
// the binary's order, case 8 last; `(level * 4.0f) * 0.01f` keeps both multiplies).
float status_effect_modifier(int status, int level)
{
    switch (status) {
    case STATUS_FAITH: case STATUS_CURSE: case STATUS_BLESS:
        return (level * 4.0f) * 0.01f;
    case STATUS_REGEN:
        return level * 2.0f;
    case STATUS_POISON:
        return -(level * 2.0f);
    case 0xe:
        return -(level * 8.0f);
    case 0xf:
        return -(level * 4.0f);
    case 0x10:
        return level * 10.0f;
    case 8:
        if (level >= 50)
            return 1.0f;
        break;
    case 0: case 1: case 6: case 7: case 9: case 0xa: case 0xc: case 0xd: case 0x11:
        return 0.0f;
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
    case 0x50:
        return 20000;
    case 0x49:
        return 3000;
    case 0x22:
        return 30000;
    case 0x0b:
        return level * 3000;
    case 0x4c:
        return 180000;
    }
    return 0;
}

// 0x47ab60  Confidence: exact (byte-indexed jump table 0x47abc0 for 0..0x11: every status has a case). Status 8 (invisible)
// lasts invisibility's +Loop Time: seconds, which spells_parse_table sets to the record index (11).
int status_effect_duration(int status)
{
    switch (status) {
    case STATUS_REGEN: return 20000;
    case STATUS_POISON: return 10000;
    case 0xe: case 0xf: return 2000;
    case 0x11:         return 5000;
    case 8:            return spells[0x0b].loop_time * 1000;
    case 0: case 1: case 2: case 3: case 4: case 6: case 7: case 9: case 0xa: case 0xc: case 0xd: case 0x10:
        return 0;
    }
    return 0;
}

// 0x47abe0  Confidence: likely (two jump tables: 0x47bc40 by source type 0..8, bytes 0x47bcbc for spells
// 0..0x67 into the dwords 0x47bc64; the bit-field stores of the first word are merged by VC6 in ways that
// do not show the statement order). Fills the cast parameters of `spell` from `caster` at the character
// `target` (a handle): the power level by source type (6 a combat skill: chain count and level, 7 a
// fixed 100, 1 / 2 the char_info special / reflex level, 3 / 4 / 5 the item's weapon / armour triggered
// effect value or charge effect level, else the caster's level), then per spell the target mode, the
// hit mask (value), the flinch kind and the amounts. Spell 0x65 recurses with 0x67 when resisted.
// Returns mode -1 when the cast is impossible (no caster, no target for a character spell, out of
// range). Field use per mode: spells.h spell_cast_params / spell_hit. No_resist makes spell 0x65 do
// nothing (it skips the whole case), and the curse (0x17, source types 1-4) and drain (0x18) cases read
// the target without a NULL test (D5507).
spell_cast_params spell_prepare_cast(Character *caster, int spell, int target, ItemType *item,
                                     int source_type, float power)
{
    spell_cast_params p;
    Character *t;
    item_triggered_effect *e;
    float resist, magic, r;
    int level, status, die, n, i;
    Character *c;

    p.mode = -1;
    p.value = 0;
    p.kind = 0;
    if (caster == NULL)
        return p;
    t = living_from_handle(target);
    if ((spells[spell].flags & SPELL_TARGET_CHARACTER) && t == NULL)
        return p;
    if (spells[spell].distance > 0.0f && t != NULL &&
        distance(&t->pos, &caster->pos) > spells[spell].distance)
        return p;

    switch (source_type) {
    case 6:
        if (spell == SPELL_HEAL) {
            float bonus = caster->combat.chain_count * 0.15f + 1.0f;
            level = (int)(bonus * (int)(caster->level * 0.33333334f));
        } else {
            float bonus = caster->combat.chain_count * 0.15f + 1.0f;
            if (spell == SPELL_VITALIZE)
                level = (int)(bonus * (int)(caster->level * 0.25f));
            else
                level = (int)(bonus * (int)(caster->level * 0.5f));
        }
        break;
    case 7:
        level = 100;
        break;
    case 1:
        level = caster->info->special_arg;
        if (level <= 0)
            level = caster->level;
        break;
    case 2:
        level = caster->info->reflex_arg;
        if (level == 0)
            level = caster->level;
        break;
    case 3:
        if (item == NULL)
            return p;
        e = item->weapon_triggered_effect;
        if (e == NULL)
            return p;
        level = e->value;
        if (level == 0)
            level = caster->level;
        break;
    case 4:
        if (item == NULL)
            return p;
        e = item->armour_triggered_effect;
        if (e == NULL)
            return p;
        level = e->value;
        if (level == 0)
            level = caster->level;
        break;
    case 5:
        if (item == NULL || item->charge_effect == NULL)
            return p;
        level = item->charge_effect->_unk0c;   // the power level of the charge effect
        if (level <= 0)
            level = caster->level;
        break;
    case 0:
    case 8:
        level = caster->level;
        break;
    default:
        return p;
    }

    switch (spell) {
    case 0x02: case 0x06: case 0x07: case 0x08: case 0x0b: case 0x0e: case 0x10: case 0x13:
    case 0x15: case 0x16: case 0x22: case 0x24: case 0x25: case 0x3c: case 0x3d: case 0x4c:
        // a status effect on the target: duration, value
        if ((spells[spell].flags & SPELL_TARGET_CHARACTER) && t == NULL)
            break;
        p.mode = 3;
        status = damage_type_to_status(spell);
        if (caster->info->species == 0xc3)
            p.target[0] = (int)(caster->level * 3000.0f);
        else if (caster->info->species == 0xb6)
            p.target[0] = (int)(caster->level * 4000.0f);
        else
            p.target[0] = spell_get_duration_ms(spell, level);
        p.ftarget[1] = status_effect_modifier(status, level);
        if (spell_target_value(t, spell) <= 0.01f)
            break;
        spell_compute_resist(&magic, &resist, spell, caster, t, level, source_type);
        r = frand();
        if (No_resist || !(spells[spell].flags & SPELL_OFFENSIVE) || r >= resist)
            p.value = 1;
        if ((spells[spell].flags & SPELL_OFFENSIVE) && spell != 0x24)
            p.kind = 2;
        break;

    case 0x3f:
        if (t == NULL)
            break;
        spell_compute_resist(&magic, &resist, 0x3f, caster, t, level, source_type);
        if (frand() < resist)
            break;
        p.mode = 2;
        p.value = 1;
        p.ftarget[0] = level * power * 0.05f;
        break;

    case 0x01:                                                  // cure
        p.mode = 3;
        p.target[0] = 0;
        p.ftarget[1] = 1.0f;
        if (level < 100)
            p.ftarget[1] = 0.0f;
        if (spell_target_value(t, spell) > 0.01f)
            p.value = 1;
        break;

    case 0x17:                                                  // curse: one target or an area
        for (i = 0; i < 6; i++)
            p.target[i] = -1;
        if (caster->info->species == 0xc3)
            p.target[6] = (int)(caster->level * 3000.0f);
        else if (caster->info->species == 0xb6)
            p.target[6] = (int)(caster->level * 4000.0f);
        else
            p.target[6] = spell_get_duration_ms(0x17, level);
        p.ftarget[7] = status_effect_modifier(3, level);
        if (source_type > 0 && source_type <= 4) {
            p.target[0] = t->handle;
            if (spell_target_value(t, 0x17) != 0.0f) {
                spell_compute_resist(&magic, &resist, 0x17, caster, t, level, source_type);
                r = frand();
                if (No_resist || r >= resist)
                    p.value |= 1;
            }
        } else {
            n = 0;
            for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                if (caster->is_enemy(c)) {
                    if (distance(&caster->pos, &c->pos) < 5.003f) {
                        p.target[n] = c->handle;
                        if (spell_target_value(c, 0x17) != 0.0f) {
                            spell_compute_resist(&magic, &resist, 0x17, caster, c, level, source_type);
                            r = frand();
                            if (No_resist || r >= resist)
                                p.value |= 1 << n;
                        }
                        n++;
                    }
                    if (n == 6)
                        break;
                }
            }
        }
        p.mode = 5;
        p.kind = 2;
        break;

    case 0x0c:
        if (t == NULL)
            break;
        p.mode = 1;
        if (spell_target_value(t, spell) == 0.0f)
            break;
        spell_compute_resist(&magic, &resist, spell, caster, t, level, source_type);
        r = frand();
        if (!No_resist && r < resist)
            break;
        p.value = 1;
        p.ftarget[0] = -(float)t->get_max_stat(0);
        break;

    case 0x40: case 0x41: case 0x42:
        if (t == NULL)
            break;
        spell_compute_resist(&magic, &resist, spell, caster, t, level, source_type);
        p.mode = 1;
        p.value = 1;
        p.kind = 2;
        p.ftarget[0] = spell_roll_amount(t, spell, level, 0, magic);
        break;

    case 0x65:
        if (t == NULL || No_resist)
            break;
        if (spell_target_value(t, 0x65) == 0.0f)
            break;
        spell_compute_resist(&magic, &resist, 0x65, caster, t, level, source_type);
        if (frand() >= resist) {
            p.mode = 1;
            p.value = 1;
            p.ftarget[0] = -(float)t->get_max_stat(0);
        } else
            p = spell_prepare_cast(caster, 0x67, target, item, source_type, 0.0f);
        break;

    case 0x67: {                                                // five statuses at once
        int status_spells[5] = { 0x3d, 7, 0x3c, 8, 0x17 };
        int statuses[5] = { 0xa, 0, 0xb, 1, 3 };

        p.mode = 10;
        for (i = 0; i < 5; i++) {
            p.target[i] = 0;
            p.target[5 + i] = 0;
            if (spell_target_value(t, status_spells[i]) <= 0.01f)
                continue;
            spell_compute_resist(&magic, &resist, status_spells[i], caster, t, level, source_type);
            r = frand();
            if (!No_resist && (spells[spell].flags & SPELL_OFFENSIVE) && r < resist)
                continue;
            p.target[i] = spell_get_duration_ms(status_spells[i], level);
            p.ftarget[5 + i] = status_effect_modifier(statuses[i], level);
        }
        break;
    }

    case 0x0f: case 0x19: case 0x20: case 0x4d: case 0x5b:     // damage, scaled by the skill for a cast
        if (t == NULL)
            break;
        spell_compute_resist(&magic, &resist, spell, caster, t, level, source_type);
        n = level;
        if (source_type == 0)
            n = (int)(level * (caster->get_skill(spell_group_skill[spell_get_group(spell)]) + 1.0f));
        p.mode = 1;
        p.value = 1;
        p.kind = 2;
        p.ftarget[0] = spell_roll_amount(t, spell, n, -spells[spell].die, magic);
        break;

    case 0x1a: case 0x4e:
        if (t == NULL)
            break;
        spell_compute_resist(&magic, &resist, spell, caster, t, level, source_type);
        p.mode = 1;
        p.value = 1;
        p.kind = 2;
        p.ftarget[0] = spell_roll_amount(t, spell, level, -spells[spell].die, magic);
        break;

    case 0x00: case 0x04:                                       // heal, vitalize
        if ((spells[spell].flags & SPELL_TARGET_CHARACTER) && t == NULL)
            break;
        p.mode = 1;
        p.value = 1;
        p.ftarget[0] = spell_roll_amount(t, spell, level, spells[spell].die, 0.0f);
        break;

    case 0x21:
        if (spell_target_value(t, 0x21) == 0.0f)
            break;
        spell_compute_resist(&magic, &resist, 0x21, caster, t, level, source_type);
        p.ftarget[0] = spell_roll_amount(t, 0x21, level, -spells[spell].die, magic);
        r = frand();
        if (!No_resist && r < resist)
            p.target[1] = 0;
        else
            p.target[1] = spell_get_duration_ms(0x21, level);
        p.mode = 6;
        p.value = 1;
        p.kind = 2;
        break;

    case 0x1c: case 0x1e: case 0x4f: case 0x64:                 // chains: up to 6 handles and amounts
        if (caster->info->species == 0x58)
            die = -10;
        else
            die = -spells[spell].die;
        for (i = 0; i < 6; i++) {
            p.target[i] = -1;
            p.target[6 + i] = 0;
        }
        if (spell == 0x1e) {
            n = 0;
            for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                if (caster->is_enemy(c)) {
                    if (distance(&caster->pos, &c->pos) < 5.003f) {
                        p.target[n] = c->handle;
                        if (spell_target_value(c, 0x1e) != 0.0f) {
                            spell_compute_resist(&magic, &resist, 0x1e, caster, c, level, source_type);
                            p.value |= 1 << n;
                            p.ftarget[6 + n] = spell_roll_amount(c, 0x1e, level, die, magic);
                        }
                        n++;
                    }
                    if (n == 6)
                        break;
                }
            }
        } else {
            Character *created[4];

            spell_spawn_creature(created, caster, spell);
            for (i = 0; i < 4; i++) {
                spell_compute_resist(&magic, &resist, spell, caster, living_from_handle(created[i]->handle),
                                     level, source_type);
                p.target[i] = created[i]->handle;
                p.ftarget[6 + i] = spell_roll_amount(created[i], spell, level, die, magic);
                p.value |= 1 << i;
            }
        }
        p.mode = 4;
        p.kind = 2;
        break;

    case 0x66:                                                  // every party member in 10 m
        for (i = 0; i < 6; i++) {
            p.target[i] = -1;
            p.target[6 + i] = 0;
        }
        die = -spells[spell].die;
        n = 0;
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (caster->is_enemy(c) && c->type == 7) {
                if (distance(&caster->pos, &c->pos) < 10.003f) {
                    p.target[n] = c->handle;
                    if (spell_target_value(c, 0x66) != 0.0f) {
                        spell_compute_resist(&magic, &resist, 0x66, caster, c, level, source_type);
                        p.value |= 1 << n;
                        p.ftarget[6 + n] = spell_roll_amount(c, 0x66, level, die, magic);
                    }
                    n++;
                }
                if (n == 6)
                    break;
            }
        }
        p.mode = 4;
        p.kind = 2;
        break;

    case 0x18:                                                  // drain
        p.mode = 7;
        p.value = 1;
        p.kind = 2;
        if (t->type != 9 && !(t->flags & 0x40))
            p.ftarget[0] = -(t->hp * 0.5f);
        else {
            spell_compute_resist(&magic, &resist, 0x18, caster, t, level, source_type);
            p.ftarget[0] = spell_roll_amount(t, 0x18, level, -spells[spell].die, magic);
        }
        p.ftarget[1] = -(caster->hp * 0.5f);
        break;

    case 0x3e:
        if (t == NULL)
            break;
        spell_compute_resist(&magic, &resist, 0x3e, caster, t, level, source_type);
        if (frand() < resist)
            break;
        p.mode = 1;
        p.value = 1;
        p.ftarget[0] = level * power * 0.05f;
        break;

    case 0x0d:
        p.mode = 8;
        if (spell_target_value(t, spell) == 0.0f)
            break;
        spell_compute_resist(&magic, &resist, spell, caster, t, level, source_type);
        r = frand();
        if (!No_resist && r < resist)
            break;
        p.value = 1;
        break;

    case 0x11:                                                  // AP drain
        spell_compute_resist(&magic, &resist, spell, caster, t, level, source_type);
        r = frand();
        if (!No_resist && r < resist)
            break;
        if (frand() < 0.05f)
            n = t->ap;
        else
            n = (int)(t->ap * level * 0.01f + 0.5f);
        p.target[0] = n > 0 ? n : 0;
        p.mode = 9;
        p.value = 1;
        p.kind = 2;
        break;

    case 0x3a: case 0x3b:                                       // trip
        spell_compute_resist(&magic, &resist, spell, caster, t, level, source_type);
        r = frand();
        if (!No_resist && r < resist)
            break;
        p.mode = 0;
        p.value = 1;
        break;

    case 0x03: case 0x05:
        p.mode = 0;
        p.value = 1;
        break;
    }
    return p;
}

// 0x47bd30  Confidence: exact (VC6 byte match, m-1020 w11, D12006: each count cleared after its list head, and
// n_others decremented after the append as n_friends is; the list walks and the two-step pick as the binary does them; the EH states
// of the four local Character list heads; D5702). Spells 0x1c, 0x4f and 0x64 (the creature spells whose effect
// strikes four times): every living entity within the spell's +Distance: of the caster that it can see (not
// type 4, not team 1) is moved to a local list of friends (team 0 when the caster is team 0, and the caster
// itself) or others. With no friend the others go back and `out` is left as it was; otherwise each of the
// four strikes picks, with chance 7/8 while any other is listed or picked, an other (a new one unless one
// was picked already and rand() % (2 x remaining) is 0, then a picked one again), else a friend the same
// way. Every list goes back to the end of living_list afterwards (the living-list order changes). The
// "picked again" walk stores the list head itself when the random index runs past the end; it cannot,
// since the index is below the count (latent).
void spell_spawn_creature(Character **out, Character *caster, int spell)
{
    Character friends;
    Character others;

    if (spell == 0x1c || spell == 0x4f || spell == 0x64) {
        Character *c, *next;
        int n_friends, n_others, team, t;
        float range_sq;
        bool friendly;

        friends.live_next = &friends;
        friends.live_prev = &friends;
        n_friends = 0;
        others.live_next = &others;
        others.live_prev = &others;
        n_others = 0;
        team = caster->get_team();
        range_sq = spells[spell].distance * spells[spell].distance;
        for (c = living_list.live_next, next = c->live_next; c != &living_list; c = next, next = c->live_next) {
            t = c->get_team();
            if (c->type == 4 || t == 1)
                continue;
            friendly = false;
            if (team == 0 && t == 0)
                friendly = true;
            if (caster == c)
                friendly = true;
            if ((c->pos - caster->pos).len_sq() <= range_sq && caster->has_line_of_sight_to(c)) {
                if (friendly) {
                    live_list_remove(c);
                    live_list_append(&friends, c);
                    n_friends++;
                } else {
                    live_list_remove(c);
                    live_list_append(&others, c);
                    n_others++;
                }
            }
        }
        if (friends.live_next == &friends) {
            for (c = others.live_next, next = c->live_next; c != &others; c = next, next = c->live_next) {
                live_list_remove(c);
                live_list_append(&living_list, c);
            }
        } else {
            Character picked_friends;
            Character picked_others;
            int got_friends = 0, got_others = 0;
            int i, r;

            picked_friends.live_next = &picked_friends;
            picked_friends.live_prev = &picked_friends;
            picked_others.live_next = &picked_others;
            picked_others.live_prev = &picked_others;
            for (i = 0; i < 4; i++) {
                if (got_others + n_others > 0 && rand() % 8) {
                    if (got_others == 0 || (n_others > 0 && rand() % (n_others * 2))) {
                        r = rand() % n_others;
                        for (c = others.live_next; c != &others && --r >= 0; c = c->live_next)
                            ;
                        got_others++;
                        live_list_remove(c);
                        live_list_append(&picked_others, c);
                        n_others--;
                    } else {
                        r = rand() % got_others;
                        for (c = picked_others.live_next; c != &picked_others && --r >= 0; c = c->live_next)
                            ;
                    }
                } else {
                    if (got_friends == 0 || (n_friends > 0 && rand() % (n_friends * 2))) {
                        r = rand() % n_friends;
                        for (c = friends.live_next; c != &friends && --r >= 0; c = c->live_next)
                            ;
                        got_friends++;
                        live_list_remove(c);
                        live_list_append(&picked_friends, c);
                        n_friends--;
                    } else {
                        r = rand() % got_friends;
                        for (c = picked_friends.live_next; c != &picked_friends && --r >= 0; c = c->live_next)
                            ;
                    }
                }
                out[i] = c;
            }
            for (c = friends.live_next, next = c->live_next; c != &friends; c = next, next = c->live_next) {
                live_list_remove(c);
                live_list_append(&living_list, c);
            }
            for (c = others.live_next, next = c->live_next; c != &others; c = next, next = c->live_next) {
                live_list_remove(c);
                live_list_append(&living_list, c);
            }
            for (c = picked_friends.live_next, next = c->live_next; c != &picked_friends;
                 c = next, next = c->live_next) {
                live_list_remove(c);
                live_list_append(&living_list, c);
            }
            for (c = picked_others.live_next, next = c->live_next; c != &picked_others;
                 c = next, next = c->live_next) {
                live_list_remove(c);
                live_list_append(&living_list, c);
            }
        }
    }
}

// 0x47c510  Confidence: exact (VC6 byte match) (the hit chance reuses the caster's argument slot). Hit chance: the
// species 0xb6 caster uses level x 2 %, Ghost Paralyzing Fear in ghost stage 3 0.5, source type 8 0.3,
// otherwise the table's +Hit Chance:.
void spell_compute_resist(float *out_magic_resist, float *out_resist, int spell, Character *caster,
                          Character *target, int power, int source_type)
{
    float hit;
    float magic;

    if (caster->info->species == 0xb6)
    {
        float pct = caster->level * 2.0f;
        hit = pct * 0.01f;
    }
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

// The type of spell_cast_execute's `where`: a vector with an empty destructor (the EH frame of 0x47c5d0
// whose state goes to 0 at the start of the single-player part and is never unwound visibly), as
// camera_frame's cam_vec (D3805). Name ours (D5508).
class spell_vec : public vector3 {
public:
    spell_vec() {}
    ~spell_vec() {}
    void operator=(const vector3 &v) { x = v.x; y = v.y; z = v.z; }
};

// 0x47c5d0  Confidence: likely (byte-indexed jump table 0x47d3a8 for spells 0..0x6b into the dwords
// 0x47d348; the EH frame (handler 0x57c598, state 0 from the start of the single-player part) is the
// empty destructor of `where`'s type, written as spell_vec, D5508). Carries out a cast. On the
// multiplayer host the cast becomes a request (spell_prepare_cast, then spell_cast_launch_effects and
// mp_spell_cast_sync, in that order). Otherwise: the source type passed on is 5 for an item cast unless
// the item came from a weapon / armour trigger (3 / 4), a combat skill (6) or a fixed level (7); a
// summoning spell of a summon skill creates the summon; then per spell the hit, the effect or the
// missile, and the hit sound at `where`. Several cases (0x17 / 0x58, the summoned creatures 0x1c / 0x4f /
// 0x64, 0x36, 0x4c, the spells without a case) play the sound at `where` without setting it: the
// position is whatever the stack holds (bug K301 candidate, D5508). `b` is not read.
void spell_cast_execute(Character *caster, int spell, int target, vector3 *pos, int layer, int a,
                        int source_type, int b)
{
    Character *t;
    Character *c;
    Player *pl;
    int st, i;

    if (game_flags & GF_MULTIPLAYER) {
        spell_cast_request req;

        req.caster = caster;
        req.spell = spell;
        req.target = target;
        req.target_pos = pos;
        req.layer = layer;
        req._unk14 = a;
        req.source_type = source_type;
        req.params = spell_prepare_cast(caster, spell, target, (ItemType *)a, source_type, 0.0f);
        switch (spell) {
        case 0x1d:
        case 0x1f:
            req._unk1c[0] = *(int *)&caster->pos.x;
            req._unk1c[1] = *(int *)&caster->pos.y;
            req._unk1c[2] = *(int *)&caster->pos.z;
            break;
        case 0x65:
            if (req.params.mode == 10)
                req.spell = 0x67;
            break;
        }
        spell_cast_launch_effects(&req);
        mp_spell_cast_sync(&req);
        return;
    }

    spell_vec where;
#ifdef FIXED_BUILD
    // FIXED_BUILD: K300 SpellHitSoundPosition. `where` (the hit sound's position) starts as the target
    // character's position, else the target point (an unset AI point is 0, 0, 0), else the caster's; the
    // cases below that set it still do.
    {
        Character *hit = target != -1 ? living_from_handle(target) : NULL;
        if (hit)
            where = hit->pos;
        else if (pos && (pos->x != 0.0f || pos->y != 0.0f || pos->z != 0.0f))
            where = *pos;
        else if (caster)
            where = caster->pos;
    }
#endif
    st = 5;
    if (a == 0)
        st = source_type;
    if (source_type == 4)
        st = source_type;
    else if (source_type == 3)
        st = source_type;
    else if (source_type == 6)
        st = source_type;
    else if (source_type == 7)
        st = source_type;

    for (i = 0; i < MAX_SKILLS; i++) {
        if (skill_table[i].spell == spell && skill_is_summon(i)) {
            summon_create(caster, pos, layer, spell, -1);
            where = *pos;
            break;
        }
    }
    if (spell == 0x68 || spell == 0x69 || spell == 0x6a || spell == 0x6b) {
        for (pl = party_list.next; pl != &party_list; pl = pl->next) {
            c = living_from_handle(pl->handle);
            if (c && c->info->species == 0) {
                where = c->ai.cast_point;
                break;
            }
        }
    }

    switch (spell) {
    case 0x04: case 0x15: case 0x16:                            // the whole side: vitalize, protect, faith
        if (caster->type == 7) {
            for (pl = party_list.next; pl != &party_list; pl = pl->next) {
                c = living_from_handle(pl->handle);
                if (c && !(c->flags & 8))
                    c->get_hit_by_spell(spell, caster->handle, NULL, NULL, st, (ItemType *)a, 0.0f, true);
            }
        } else {
            int found;

            if (cur_character == NULL || cur_character->ai.group == NULL)
                break;
            found = 0;
            for (i = 0; i < cur_character->ai.group->num_enemies; i++) {
                Character *e = living_from_handle(cur_character->ai.group->enemies[i]);
                if (e) {
                    if (e == caster)
                        found = 1;
                    if (e->get_team() == caster->get_team() && !(e->flags & 8))
                        e->get_hit_by_spell(spell, caster->handle, NULL, NULL, st, (ItemType *)a, 0.0f, true);
                }
            }
            if (!found)
                caster->get_hit_by_spell(spell, caster->handle, NULL, NULL, st, (ItemType *)a, 0.0f, true);
        }
        where = caster->pos;
        break;

    case 0x00: case 0x01: case 0x02: case 0x06: case 0x07: case 0x08: case 0x0b: case 0x0f: case 0x10:
    case 0x13: case 0x21: case 0x22: case 0x24: case 0x25: case 0x50: case 0x55: case 0x56: case 0x59:
    case 0x5a:
        t = living_from_handle(target);
        if (t == NULL)
            return;
        t->get_hit_by_spell(spell, caster->handle, NULL, NULL, st, (ItemType *)a, 0.0f, true);
        where = t->pos;
        break;

    case 0x17: case 0x58:                                       // areas around the caster
        if (a) {
            const char *eff = spell_get_cast_effect(spell);
            if (eff)
                fx_attach_to_character(eff, caster->handle, -1, -1, false, false);
        }
        fx_set_target(fx_create_spell_area(spell, caster->handle, &caster->pos, NULL), a);
        break;

    case 0x18: case 0x19: case 0x1a: case 0x20: case 0x4e: case 0x5b: { // missiles
        vector3 bone_pos;
        matrix bone_orient;

        t = living_from_handle(target);
        if (t == NULL)
            return;
        if (st == 3 || st == 4) {
            // a weapon / armour trigger: the hit at once, at the target's centre raised by its root bone
            t->instance->get_bone_transform(&bone_pos, &bone_orient,
                                            t->instance->character->mesh->skin->root_bone);
            vector3 hit_pos = t->pos + t->centre_offset;
            hit_pos.y += bone_pos.y;
            matrix hit_orient;
            hit_orient = t->orient;
            t->get_hit_by_spell(spell, caster->handle, &hit_pos, &hit_orient, st, (ItemType *)a, 0.0f, true);
        } else if (st == 5) {
            // an item: a missile from the caster's centre (raised by its root bone) at the target
            caster->instance->get_bone_transform(&bone_pos, &bone_orient,
                                                 caster->instance->character->mesh->skin->root_bone);
            vector3 start = caster->pos + caster->centre_offset;
            start.y += bone_pos.y;
            vector3 dir = (t->centre_offset + t->pos) - (caster->centre_offset + caster->pos);
            dir.normalize_safe();
            vector3 up(0.0f, 1.0f, 0.0f);
            vector3 right = dir.cross(up);
            matrix orient(right, up, dir);
            fx_set_target(spell_fire_missile(spell, -1, caster, t, &start, &orient), a);
        } else {
            // the missile the caster's current move launched
            effect_object *fx;

            for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next)
                if (fx->owner == caster->handle && fx->source == caster->cur_anim)
                    break;
            if (fx != &fx_active_list)
                fx_set_target(spell_fire_missile(spell, fx->handle, caster, t, NULL, NULL), a);
        }
        where = t->pos;
        break;
    }

    case 0x1d: case 0x1f: {                                     // ground effects at pos
        matrix m;
        vector3 dir = *pos - caster->pos;
        dir.normalize();
        m.set_from_forward_up(dir, caster->orient.uvec);
        fx_set_target(spell_place_ground_effect(spell, caster->handle, pos, layer, &m), a);
        where = *pos;
        break;
    }

    case 0x23:                                                  // blizzard
        fx_set_target(spell_spawn_hit_fx(caster->handle, pos, layer, &caster->orient), a);
        where = *pos;
        break;

    case 0x0e:
        caster->get_hit_by_spell(0xe, caster->handle, NULL, NULL, st, (ItemType *)a, 0.0f, true);
        where = caster->pos;
        break;

    case 0x36:
        summon_dismiss(target);
        break;

    case 0x48:
        t = living_from_handle(target);
        if (t == NULL)
            return;
        t->get_hit_by_spell(7, caster->handle, NULL, NULL, st, (ItemType *)a, 0.0f, true);
        t->get_hit_by_spell(8, caster->handle, NULL, NULL, st, (ItemType *)a, 0.0f, true);
        t->get_hit_by_spell(0x43, caster->handle, NULL, NULL, st, (ItemType *)a, 0.0f, true);
        where = t->pos;
        break;

    case 0x1c: case 0x4f: case 0x64: {                          // creatures: four effects 0.5 s apart
        Character *created[4];
        int delay;

        spell_spawn_creature(created, caster, spell);
        for (delay = 0, i = 0; delay < 2000; delay += 500, i++)
            fx_set_target(fx_create_spell_effect_near(spell, caster->handle, created[i]->handle, delay), a);
        break;
    }

    case 0x11:
        t = living_from_handle(target);
        if (t == NULL)
            return;
        fx_create_spell_effect_facing(0x11, caster->handle, t->handle);
        t->get_hit_by_spell(0x11, caster->handle, NULL, NULL, st, (ItemType *)a, 0.0f, true);
        where = t->pos;
        break;

    case 0x03: case 0x05: {
        matrix spawn;

        t = living_from_handle(target);
        if (t == NULL)
            return;
        fx_find_spawn_point(&spawn, t, 4.5f);
        fx_set_target(fx_create_spell_effect_at(spell, caster->handle, t->handle, 6250, &t->pos, &spawn), a);
        where = t->pos;
        break;
    }

    case 0x0c: case 0x54: case 0x57:
        t = living_from_handle(target);
        if (t == NULL)
            return;
        fx_set_target(fx_create_spell_effect_at(spell, caster->handle, t->handle, 1000, NULL, NULL), a);
        where = t->pos;
        break;

    case 0x1e:
        if (a) {
            const char *eff = spell_get_cast_effect(0x1e);
            if (eff)
                fx_attach_to_character(eff, caster->handle, -1, -1, false, false);
        }
        fx_set_target(fx_create_spell_area(0x1e, caster->handle, &caster->pos, NULL), a);
        where = caster->pos;
        break;

    case 0x51:
        summon_create(caster, pos, layer, 0x51, target);
        where = *pos;
        break;

    case 0x52:
        summon_create(caster, pos, layer, 0x52, target);
        where = *pos;
        break;

    case 0x4c:
        t = living_from_handle(target);
        if (t == NULL)
            return;
        t->get_hit_by_spell(0x4c, caster->handle, NULL, NULL, 0, NULL, 0.0f, true);
        break;

    case 0x5c: case 0x5d: case 0x5e: case 0x5f: case 0x60: case 0x61: case 0x62: case 0x63:
        summon_create(caster, pos, layer, spell, target);
        where = *pos;
        break;

    case 0x65:
        t = living_from_handle(target);
        if (t == NULL)
            return;
        fx_set_target(fx_create_spell_effect_at(0x65, caster->handle, t->handle, 1000, NULL, NULL), a);
        where = caster->pos;
        break;

    case 0x66:
        fx_set_target(fx_create_spell_effect_at(0x66, caster->handle, -1, 3350, &caster->pos, &caster->orient), a);
        where = caster->pos;
        break;

    case 0x0d:
        t = living_from_handle(target);
        if (t == NULL || t->type == 7)
            return;
        t->get_hit_by_spell(0xd, caster->handle, NULL, NULL, 0, NULL, 0.0f, true);
        where = caster->pos;
        break;

    case 0x68:                                                  // monsters within 10 of `where`
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if ((c->type == 5 || c->type == 9) && distance_squared(&c->pos, &where) < 100.0f) {
                vector3 hit_pos = c->pos;
                matrix hit_orient = c->orient;
                hit_pos.y = 0.77f;
                c->get_hit_by_spell(0x68, caster->handle, &hit_pos, &hit_orient, st, (ItemType *)a, 0.0f, true);
            }
        }
        return;

    case 0x69: case 0x6a: case 0x6b:
        if (spell == 0x6b || spell == 0x69) {
            for (pl = party_list.next; pl != &party_list; pl = pl->next) {
                c = living_from_handle(pl->handle);
                if (c != cur_summon)
                    c->get_hit_by_spell(spell, caster->handle, NULL, NULL, st, (ItemType *)a, 0.0f, true);
            }
        }
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if ((c->type == 5 || c->type == 9) && distance_squared(&c->pos, &where) < 100.0f)
                c->get_hit_by_spell(spell, caster->handle, NULL, NULL, st, (ItemType *)a, 0.0f, true);
        }
        return;
    }
    spell_play_hit_sound(spell, &where);
}

// Records a spell effect for the effect object `fx` (the request's parameters), as five cases of
// spell_cast_launch_effects do after fx_set_target. Written out each time in the binary; a macro here.
#define SPELL_FX_RECORD(fx, req) do { \
        effect_object *o_ = (effect_object *)obj_from_handle(fx); \
        o_->spell_effect = spell_effect_add((spell_effect *)&(req)->params); \
    } while (0)

// 0x47d420  Confidence: likely (two switches: bytes 0x47deec for spells 3..0x67 into the dwords 0x47dec8,
// bytes 0x47df68 for 0..0x4c into 0x47df54). The effects of a prepared cast (spell_cast_execute's host
// branch, the self-casts of combat.cpp and the triggered abilities, the multiplayer spell packets): the
// first switch creates the visible effects, records the cast in spell_effects for the effect objects
// (spell_effect_add) and remembers where to play the hit sound; the second applies the hits that need no
// effect object (spell_apply_hit_effects) and handles summons. A weapon / armour triggered missile hits
// at once; its matrix copy of the target's orientation is computed and never used (the hit passes the
// target's own orientation). The creature effects of 0x4f and 0x64 use spell 0x1c's effect, and spell
// 0x0c / 0x65 / 0x67 read the target's position without a NULL test (D5509).
void spell_cast_launch_effects(spell_cast_request *req)
{
    bool played;
    vector3 where;
    Character *t;
    Character *c;
    Player *pl;
    int fx, i;

    if (req->caster == NULL)
        return;
    played = false;
    where = req->caster->pos;
    switch (req->spell) {
    case 0x23:                                                  // blizzard
        fx = spell_spawn_hit_fx(req->caster->handle, req->target_pos, req->layer, &req->caster->orient);
        if (fx != -1) {
            fx_set_target(fx, req->_unk14);
            played = true;
            where = *req->target_pos;
        }
        break;

    case 0x17: case 0x1e:                                       // areas around the caster
        if (req->_unk14) {
            const char *eff = spell_get_cast_effect(req->spell);
            if (eff)
                fx_attach_to_character(eff, req->caster->handle, -1, -1, false, false);
        }
        if (spell_effect_pool_full()) {
            fx = fx_create_spell_area(req->spell, req->caster->handle, &req->caster->pos, &req->params);
            if (fx != -1) {
                fx_set_target(fx, req->_unk14);
                SPELL_FX_RECORD(fx, req);
                played = true;
            }
        }
        break;

    case 0x18: case 0x19: case 0x1a: case 0x20: case 0x4d: case 0x4e: case 0x5b:   // missiles
        t = living_from_handle(req->target);
        if (t == NULL)
            return;
        if (req->source_type == 3 || req->source_type == 4) {
            // a weapon / armour trigger: the hit at once, at the target's centre raised by its root bone
            spell_hit h;
            vector3 bone_pos;
            matrix bone_orient;
            matrix m;

            t->instance->get_bone_transform(&bone_pos, &bone_orient,
                                            t->instance->character->mesh->skin->root_bone);
            vector3 hit_pos = t->pos + t->centre_offset;
            hit_pos.y += bone_pos.y;
            m = t->orient;
            h.spell = req->spell;
            h.source = req->caster->handle;
            h.pos = &hit_pos;
            h.orient = &t->orient;
            h.source_type = req->source_type;
            h.item = req->item;
            h.power = 0.0f;
            *(spell_cast_params *)((char *)&h + 0x1c) = req->params;
            t->spell_apply_hit_effects(&h);
            break;
        }
        if (!spell_effect_pool_full())
            break;
        if (req->source_type == 5) {
            // an item: a missile from the caster's centre (raised by its root bone) at the target
            vector3 bone_pos;
            matrix bone_orient;

            req->caster->instance->get_bone_transform(&bone_pos, &bone_orient,
                                                      req->caster->instance->character->mesh->skin->root_bone);
            vector3 start = req->caster->centre_offset + req->caster->pos;
            start.y += bone_pos.y;
            vector3 dir = (t->pos + t->centre_offset) - (req->caster->pos + req->caster->centre_offset);
            dir.normalize_safe();
            vector3 up(0.0f, 1.0f, 0.0f);
            vector3 right = dir.cross(up);
            matrix orient(right, up, dir);
            fx = spell_fire_missile(req->spell, -1, req->caster, t, &start, &orient);
        } else {
            // the missile the caster's current move launched
            effect_object *o;

            for (o = fx_active_list.fx_next; o != &fx_active_list; o = o->fx_next)
                if (o->owner == req->caster->handle && o->source == req->caster->cur_anim)
                    break;
            if (o == &fx_active_list)
                break;
            fx = spell_fire_missile(req->spell, o->handle, req->caster, t, NULL, NULL);
        }
        if (fx != -1) {
            fx_set_target(fx, req->_unk14);
            SPELL_FX_RECORD(fx, req);
            where = t->pos;
            played = true;
        }
        break;

    case 0x0c: case 0x65: case 0x67:
        if (spell_effect_pool_full()) {
            fx = fx_create_spell_effect_at(req->spell, req->caster->handle, req->target, 1000, NULL, NULL);
            if (fx != -1) {
                fx_set_target(fx, req->_unk14);
                SPELL_FX_RECORD(fx, req);
                played = true;
                where = living_from_handle(req->target)->pos;
            }
        }
        break;

    case 0x66:
        if (spell_effect_pool_full()) {
            fx = fx_create_spell_effect_at(req->spell, req->caster->handle, -1, 3350, &req->caster->pos,
                                           &req->caster->orient);
            if (fx != -1) {
                fx_set_target(fx, req->_unk14);
                SPELL_FX_RECORD(fx, req);
                played = true;
                where = req->caster->pos;
            }
        }
        break;

    case 0x1c: case 0x4f: case 0x64: {                          // creatures: four effects 0.5 s apart
        int *handle = &req->params.target[0];
        int delay;

        for (i = 0, delay = 0; delay < 2000; delay += 500, i++, handle++) {
            spell_effect *e;

            if (!spell_effect_pool_full())
                break;
            fx = fx_create_spell_effect_near(0x1c, req->caster->handle, *handle, delay);
            if (fx == -1)
                continue;
            fx_set_target(fx, req->_unk14);
            effect_object *o_ = (effect_object *)obj_from_handle(fx);
            o_->spell_effect = e = spell_effect_add((spell_effect *)&req->params);
            if (e)
                ((spell_cast_params *)e)->value &= 1 << i;
            played = true;
            where = req->caster->pos;
        }
        break;
    }

    case 0x03: case 0x05: {
        matrix spawn;

        if (!spell_effect_pool_full())
            return;
        t = living_from_handle(req->target);
        if (t == NULL)
            return;
        fx_find_spawn_point(&spawn, t, 4.5f);
        fx = fx_create_spell_effect_at(req->spell, req->caster->handle, req->target, 6250, &t->pos, &spawn);
        if (fx != -1) {
            fx_set_target(fx, req->_unk14);
            SPELL_FX_RECORD(fx, req);
            where = t->pos;
            played = true;
        }
        if (t == cur_character)
            hud_dead_clear_popups();
        break;
    }

    case 0x1d: case 0x1f: {                                     // ground effects at the target point
        matrix m;
        vector3 from = *(vector3 *)req->_unk1c;
        vector3 dir = *req->target_pos - from;

        if (req->caster->info->species == 0xc4)
            dir = dir.cross(req->caster->orient.uvec);
        dir.normalize_safe();
        m.set_from_forward_up(dir, req->caster->orient.uvec);
        fx = spell_place_ground_effect(req->spell, req->caster->handle, req->target_pos, req->layer, &m);
        if (fx != -1) {
            fx_set_target(fx, req->_unk14);
            played = true;
            where = *req->target_pos;
        }
        break;
    }
    }

    switch (req->spell) {
    case 0x00: case 0x01: case 0x02: case 0x06: case 0x07: case 0x08: case 0x0b: case 0x0d: case 0x0e:
    case 0x0f: case 0x10: case 0x11: case 0x13: case 0x21: case 0x22: case 0x24: case 0x25: case 0x3a:
    case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f: case 0x40: case 0x41: case 0x42: case 0x4c: {
        spell_hit h;

        if (spells[req->spell].flags & SPELL_TARGET_CHARACTER) {
            t = living_from_handle(req->target);
            if (t == NULL)
                return;
            if (req->spell == 0xd && t->type == 7)
                return;
        } else
            t = req->caster;
        h.spell = req->spell;
        h.source = req->caster->handle;
        h.pos = NULL;
        h.orient = NULL;
        h.source_type = req->source_type;
        h.item = req->item;
        h.power = 0.0f;
        *(spell_cast_params *)((char *)&h + 0x1c) = req->params;
        t->spell_apply_hit_effects(&h);
        break;
    }

    case 0x04: case 0x15: case 0x16: {                          // the whole side
        spell_hit h;

        h.spell = req->spell;
        h.source = req->caster->handle;
        h.pos = NULL;
        h.orient = NULL;
        h.source_type = req->source_type;
        h.item = req->item;
        h.power = 0.0f;
        *(spell_cast_params *)((char *)&h + 0x1c) = req->params;
        if (req->caster->type == 7) {
            for (pl = party_list.next; pl != &party_list; pl = pl->next) {
                c = living_from_handle(pl->handle);
                if (c && !(c->flags & 8))
                    c->spell_apply_hit_effects(&h);
            }
        } else {
            int found;

            if (cur_character == NULL || cur_character->ai.group == NULL)
                break;
            found = 0;
            for (i = 0; i < cur_character->ai.group->num_enemies; i++) {
                Character *e = living_from_handle(cur_character->ai.group->enemies[i]);
                if (e) {
                    if (e == req->caster)
                        found = 1;
                    if (e->get_team() == req->caster->get_team() && !(e->flags & 8))
                        e->spell_apply_hit_effects(&h);
                }
            }
            if (!found)
                req->caster->spell_apply_hit_effects(&h);
        }
        break;
    }

    case 0x26: case 0x27: case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e:
    case 0x2f: case 0x30: case 0x31:                            // summons: always the sound
        summon_create(req->caster, req->target_pos, req->layer, req->spell, -1);
        where = *req->target_pos;
        played = true;
        break;

    case 0x36:
        summon_dismiss(req->target);
        break;
    }
    if (played)
        spell_play_hit_sound(req->spell, &where);
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

// The x / z pair of an avoidance description, through float arguments: the binary copies the six floats
// with fld / fstp (an inline with float parameters; plain member copies are integer moves). Name ours.
static inline void set_xz(float *d, float x, float z)
{
    d[0] = x;
    d[1] = z;
}

// 0x47e100  Confidence: exact (VC6 byte match, m-1020 w11, D12004: the avoidance record is filled in the
// binary's store order, cx cz ax bx shape extent_a _unk00 az bz ...). The effect gets an avoidance box from the middle of ground points 7..1 and 8..13
// along their direction.
int spell_place_ground_effect(int spell, int caster, const vector3 *pos, int owner, const matrix *orient)
{
    int h;
    effect_object *fx;
    vector3 p1, p2;
    vector2 centre, dir;
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
    vector2 a(p1.x, p1.z);
    vector2 b(p2.x, p2.z);

    centre = 0.5f * (a + b);
    dir = b - centre;
    len = dir.normalize() + 1.5f;
    vector2 perp(-dir.y, dir.x);
    aa.cx = centre.x;
    aa.cz = centre.y;
    aa.ax = dir.x;
    aa.bx = perp.x;
    aa.shape = 0;
    aa.extent_a = len;
    aa._unk00 = 0;
    aa.az = dir.y;
    aa.bz = perp.y;
    aa.extent_b = 1.5f;
    aa._unk28 = 0.5f;
    aa.owner = owner;
    fx->avoid_area = avoid_area_add(&aa);
    return h;
}

// 0x47e280  Confidence: exact (VC6 byte match) (do-while: no entry test of first != last). Walks the "$groundNN" tags
// from first towards last; once one tag fails the floor probe, it and every later tag get
// get_tag_transform(tag, 1, 1) (result not used).
void spell_find_ground_points(vector3 *out, int first, int last, effect_object *fx)
{
    int step = (first < last) ? 1 : -1;
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

// 0x47e330  Confidence: exact (D13502: `v += fx->pos` on the unrotated result, the ray declared where it is
// set up). Moves the node's world
// position onto the floor below it (ray from 1 m above the point, 2 m down) and writes the node back.
bool spell_probe_ground_point(effect_object *fx, int node, vector3 *point)
{
    vector3 v;
    vector3 pos;
    lobj_hit hit;
    matrix orient;

    fx->instance->get_node_transform(&pos, &orient, node);
    v = pos.unrotated(fx->orient);
    v += fx->pos;
    point->x = v.x;
    point->z = v.z;
    point->y = point->y + 1.0f;
    lobj_ray ray;
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

// 0x47e4d0  Confidence: exact (VC6 byte match, match27; marked exact before without one: the pairs through
// set_xz). Marks the object as a fireball (spell 0x1a) ground effect with an avoidance box of 2 x 2 m.
void spell_effect_obj_set_timer(int caster, int handle, const vector3 *pos, int owner, const matrix *orient)
{
    effect_object *fx = (effect_object *)obj_from_handle_b(handle);
    avoid_area_desc aa;

    if (fx) {
        fx->kind = 7;
        fx->timer.SetTimeout(0);
        fx->caster = caster;
        fx->spell = SPELL_FIREBALL;
        set_xz(&aa.cx, pos->x, pos->z);
        set_xz(&aa.ax, orient->rvec.x, orient->rvec.z);
        set_xz(&aa.bx, orient->fvec.x, orient->fvec.z);
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
    aa._unk00 = 0;
    aa.shape = 1;
    aa.cx = pos->x;
    aa.cz = pos->z;
    aa.ax = orient->rvec.x;
    aa.az = orient->rvec.z;
    aa.bx = orient->fvec.x;
    aa.bz = orient->fvec.z;
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

// 0x47e6c0  Confidence: exact (VC6 byte match). Records the "$missile" node of the effect object (in the caster's
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

// 0x47e830  Confidence: exact (D13501: the early return when there is no missile slot). An explicit start / orientation wins; otherwise a recorded missile slot
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
    if (missile == -1)
        return h;
    i = spell_missile_remove(missile);
    if (i != -1 && target && !start) {
        h = fx_create_missile(spell, &spell_missiles[i].pos, &spell_missiles[i].orient, caster->handle,
                              target->handle);
        spell_play_sound_34(spell, &spell_missiles[i].pos);
    }
    return h;
}

// A second empty dynamic initialiser, so an object with an inline empty constructor is defined here
// (between spell_fire_missile and summon_create). Not reproduced.
// 0x47e8e0 init_empty_47e8f0_thunk (jmp). Confidence: exact.
// 0x47e8f0 init_empty_47e8f0 (a bare ret). Confidence: unsure (which object).

// 0x47e900  Confidence: likely (D5703: void, not Character *; the ring chain is an if / else-if chain in the
// binary's order, whose identical get_int calls VC6 tail-merges where the original repeats them,
// 2195 against 2415 bytes). Creates the summon of `spell` facing the caster at `pos` (or, with `existing`
// a handle, moves that living entity there): its level is (skill 0x1b x 5 + caster level + 0.5) in
// multiplayer, the level the matching ring's xp ("dark_ring_xp" ...; three summons take 0.7 of it) reaches
// for the party (type 7) without All_summons, else (skill 0x1b + 0.5) x caster level + 0.5, at least 1. Four
// species (summon_species_values) stand that far back along the facing; their creation passes placed = 1.
// The party's summon gets a Player record (summon_player) and joins the caster's AI group; a dragon skill
// (flag 0x10) destroys the caster's emerge effect, plays the skill's cutscene, freezes every living entity
// that is not one of cur_character's group's enemies, and takes (1 - hp share) of the caster's base max HP
// as its HP cap; the others lend the hp share of the caster's base max HP to the Player record. Then the
// emerge effect, the +Misc effect: and the multiplayer handle.
void summon_create(Character *caster, vector3 *pos, int layer, int spell, int existing)
{
    vector3 dir = caster->pos - *pos;
    matrix orient;
    Character *s;
    int ci, level, xp, i;

    orient.set_from_forward(dir);
    if (existing == -1) {
        int species;
        vector3 p;
        bool placed;

        ci = summon_find_charinfo(spell);
        if (ci == -1)
            return;
        if (game_flags & 2) {
            level = (int)(caster->get_skill(0x1b) * 5.0f + caster->level + 0.5f);
        } else if (caster->type == 7 && !All_summons) {
            species = char_infos[ci].species;
            if (species == 0xc0 || species == 0xcd)
                xp = savegame_state.get_int(4, "dark_ring_xp");
            else if (species == 0xcc)
                xp = savegame_state.get_int(4, "light_ring_xp");
            else if (species == 0xb9)
                xp = (int)(savegame_state.get_int(4, "light_ring_xp") * 0.7f);
            else if (species == 0x90)
                xp = savegame_state.get_int(4, "fire_ring_xp");
            else if (species == 0xcb)
                xp = (int)(savegame_state.get_int(4, "fire_ring_xp") * 0.7f);
            else if (species == 4)
                xp = savegame_state.get_int(4, "stone_ring_xp");
            else if (species == 0xb0)
                xp = (int)(savegame_state.get_int(4, "stone_ring_xp") * 0.7f);
            else if (species == 0xa4)
                xp = savegame_state.get_int(4, "water_ring_xp");
            else if (species == 0xb5)
                xp = savegame_state.get_int(4, "water_ring_xp");
            else if (species == 0xce)
                xp = savegame_state.get_int(4, "wind_ring_xp");
            else if (species == 0xb6)
                xp = savegame_state.get_int(4, "wind_ring_xp");
            else if (species == 0xca)
                xp = savegame_state.get_int(4, "forest_ring_xp");
            else if (species == 0xc3)
                xp = savegame_state.get_int(4, "forest_ring_xp");
            else if (species == 9)
                xp = savegame_state.get_int(4, "jade_ring_xp");
            else if (species == 0xc2)
                xp = savegame_state.get_int(4, "jade_ring_xp");
            else
                xp = 0;
            for (level = 2; level < 30; level++)
                if (xp < xp_for_level(level))
                    break;
            level--;
        } else {
            level = (int)((caster->get_skill(0x1b) + 0.5f) * caster->level + 0.5f);
        }
        p = *pos;
        placed = false;
        for (i = 0; i < 4; i++) {
            if (summon_species_values[i].species == char_infos[ci].species) {
                p -= orient.fvec * summon_species_values[i].value;
                placed = true;
                break;
            }
        }
        if (level < 1)
            level = 1;
        s = living_entity_create(caster->type, ci, &p, &orient, true, layer, level, 1, placed);
    } else {
        s = living_from_handle(existing);
        s->orient = orient;
        s->pos = *pos;
        s->layer = layer;
        s->flags &= ~8;
        ci = charinfo_find_by_species(s->info->species);
    }
    if (!s)
        return;
    s->flags |= 0x103;
    s->obj_flags &= ~1;
    s->set_team(caster->get_team());
    if (spell == 0x27)
        item_free(character_unequip_slot(s, 5, true));
    else if (s->type == 7 && s->equipment[5])
        item_page_in(s->equipment[5]->type - item_types, 8, 0);
    if (spell == 0x2e)
        s->instance->_unkb4 = 0x80;
    if (s->type == 7) {
        summon_player = player_alloc_slot();
        summon_player->char_info = ci;
        summon_player->handle = s->handle;
        summon_player->flags |= PF_SUMMON;
        if (game_flags & 0x1000)
            summon_player->flags |= PF_HOLD;
        ai_group_add_member(caster->ai.group, s);
        if (game_flags & 2)
            summon_player->mp_owner = player_from_handle(caster->handle)->mp_owner;
        cur_summon = s;
        for (i = 0; i < MAX_SKILLS; i++) {
            if (skill_table[i].spell == spell) {
                if (skill_table[i].flags & 0x10) {
                    effect_object *fx;
                    Character *c;
                    ai_group *g;
                    vector3 a, b;
                    matrix m;
                    float angle;
                    int j;

                    for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
                        if (fx->source == caster->ai.emerge_move && fx->owner == caster->handle) {
                            fx_destroy(fx->handle);
                            break;
                        }
                    }
                    cur_summon->flags |= 0x1820;
                    cutscene_play(skill_names[i]);
                    levelrender_fade_out_start();
                    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                        if (c->type == 7)
                            continue;
                        g = cur_character->ai.group;
                        for (j = 0; j < g->num_enemies; j++)
                            if (c->handle == g->enemies[j])
                                break;
                        if (j == g->num_enemies) {
                            c->flags |= 0x10;
                            c->set_ai_active(0);
                        }
                    }
                    a.x = 0.0f;
                    a.y = 0.0f;
                    a.z = 1.0f;
                    b.x = 0.0f;
                    b.y = 1.0f;
                    b.z = 0.0f;
                    angle = vector3_angle_and_axis(&a, &cur_summon->orient.fvec, &b);
                    m.set_from_axis_angle(b, angle);
                    game_camera.set_sequence_transform(&cur_summon->pos, &m);
                    caster->hp = float_min(caster->hp, (1.0f - skill_table[i].hp_share) * caster->base_max_hp);
                } else {
                    Player *pl = player_from_handle(caster->handle);
                    pl->lent_max_hp = skill_table[i].hp_share * caster->base_max_hp;
                    caster->base_max_hp -= pl->lent_max_hp;
                    if (caster->get_max_stat(0) < caster->hp)
                        caster->hp = (float)caster->get_max_stat(0);
                }
                break;
            }
        }
    } else if (caster->info->species == 0x5a || caster->info->species == 0xc9) {
        s->flags |= 0x40000;
    }
    {
        int fx = summon_spawn_effect(spell_get_effect_1c(spell), &s->pos, &s->orient, s, spells[spell].emerge_time,
                                     spells[spell].target_radius, layer);
        const char *misc;

        s->ai.disable_timer.invalidate();
        if (fx == -1 && !(s->flags & 0x800)) {
            printf("no room for summon target effect!!  make summon emerge through AI code\n");
            s->ai.disable_timer.SetTimeout(spells[spell].emerge_time);
            s->ai.flags |= 0x200;
        }
        if (cur_summon && (cur_summon->flags & 0x800))
            cur_summon->move_fx = fx;
        misc = spells[spell].misc_effect;
        if (misc) {
            if (s->flags & 0x800)
                fx_attach_to_character(misc, s->handle, -1, -1, false, false);
            else
                fx_summon_hook_stub(fx_add_effect(misc, &s->pos, &s->orient, true), (int)s);
        }
    }
    if ((game_flags & 2) && mp_local_player && (mp_local_player->flags & 1))
        mp_broadcast_summon_handle(s->handle);
    else if ((game_flags & 2) && mp_local_player && !(mp_local_player->flags & 1))
        mp_summon_map_handle(s->handle);
}

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
        // fall through
    case 0x0c: case 0x17: case 0x22: case 0x24: case 0x25: case 0x3d: case 0x43:
    not_boss:
        if (target->type == 9 || (target->flags & 0x40))
            return 0.0f;
        return 1.0f;
    case 0x0d:                                            // Life Drain
        if (target->level > 1)
            goto not_boss;
        return 0.0f;
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
    for (i = 0; i < count; i++) // D25900: closer (6 -> 3; the binary's trip count is dec / lea +1, 8966 gives mov)
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

// 0x47f690  Confidence: exact (VC6 byte match, match27: the found record breaks out to one `return e`; was likely).
spell_effect *spell_effect_alloc()
{
    int i;
    spell_effect *e = NULL;

    for (i = 0; i < MAX_SPELL_EFFECTS; i++)
        if (!spell_effects[i].used) {
            e = &spell_effects[i];
            e->used = 1;
            e->slot = 0x3f;
            break;
        }
    return e;
}

// 0x47f6d0  Confidence: exact (signed pointer difference by 0x34; no lower bound check).
void spell_effect_free(spell_effect *e)
{
    if (e - spell_effects < MAX_SPELL_EFFECTS)
        e->used = 0;
}

// 0x47f700 init_ccmd_summon_thunk (jmp). Confidence: exact.
// 0x47f710 init_ccmd_summon (console_command constructor). Confidence: exact.
console_command ccmd_summon("summon", "Summon a creature", cmd_summon);   // 0x22e9240

// 0x47f730  Confidence: exact. Only "golem" and "blackfire" are known; there is no
// calling_a_command_function test, so the help and status passes of the console also summon.
void cmd_summon()
{
    int spell = -1;
    Character *c;
    vector3 pos;

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
    pos = c->pos;
    pos += c->orient.fvec * 4.0f;
    summon_create(c, &pos, 0, spell, -1);
}
