// engine/characterinfo/characterinfo.h - character info records, skills and chain attacks
// (engine\characterinfo\characterinfo.cpp, path references 0x445374-0x449acf). The experience table
// functions (0x4899d0-0x489bf0) are declared here too; their unit is not known. Not original source.
// Layout: symbols/types.h char_info (with +0xf8 as the resistance set of new_assess.cpp).
#ifndef ENGINE_CHARACTERINFO_CHARACTERINFO_H
#define ENGINE_CHARACTERINFO_CHARACTERINFO_H

#include "vsdk/vsdk.h"
#include "engine/gamesound/gamesound.h"   // sound_ref

class Player;
class Character;

#define MAX_SKILLS 0x54               // 84 skill ids

// char_info::species
#define SPECIES_JOSEPH   0
#define SPECIES_JEKHAR   1
#define SPECIES_FLECE    2
#define SPECIES_ROSALIND 3

// Skill ids (Character.skill index; skill_table 0x8b3f88). 0x55 = no skill (character_get_chain_skill).
enum skill_id {
    SKILL_HAND_TO_HAND = 0x0,
    SKILL_PROTECTION = 0x1,
    SKILL_SPEED = 0x2,
    SKILL_SWORD_WEAPONS = 0x3,
    SKILL_AXE_WEAPONS = 0x4,
    SKILL_BLUNT_WEAPONS = 0x5,
    SKILL_STAFF_WEAPONS = 0x6,
    SKILL_BOW_WEAPONS = 0x7,
    SKILL_HEAVY_ARMS = 0x8,
    SKILL_PARRY = 0x9,
    SKILL_COUNTER_ATTACK = 0xa,
    SKILL_DODGE = 0xb,
    SKILL_CRITICAL_HIT = 0xc,
    SKILL_DOUBLE_ATTACK = 0xd,
    SKILL_PUSH = 0xe,
    SKILL_KICK = 0xf,
    SKILL_TRIP = 0x10,
    SKILL_AIMED_ATTACK = 0x11,
    SKILL_BACKSTAB = 0x12,
    SKILL_ASSESS = 0x13,
    SKILL_APPRAISE = 0x14,
    SKILL_PICK_LOCK = 0x15,
    SKILL_SNEAK = 0x16,
    SKILL_HIDE = 0x17,
    SKILL_FORGE = 0x18,
    SKILL_MINE = 0x19,
    SKILL_ENCHANT = 0x1a,
    SKILL_SUMMON = 0x1b,
    SKILL_HEAL = 0x1c,
    SKILL_DARK = 0x1d,
    SKILL_ENERGY = 0x1e,
    SKILL_HOLY = 0x1f,
    SKILL_FIRE = 0x20,
    SKILL_ICE = 0x21,
    SKILL_DIVINATION = 0x22,
    SKILL_MAGIC_RESIST = 0x23,
    SKILL_SUMMON_GOLEM = 0x24,
    SKILL_SUMMON_RED_MINOTAUR = 0x25,
    SKILL_SUMMON_BLADE_OF_URATH = 0x26,
    SKILL_SUMMON_BLUE_IMP = 0x27,
    SKILL_SUMMON_BLACKFIRE_ELEMENTAL = 0x28,
    SKILL_SUMMON_BLOOD_ELEMENTAL = 0x29,
    SKILL_SUMMON_POISON_ELEMENTAL = 0x2a,
    SKILL_SUMMON_MIST_ELEMENTAL = 0x2b,
    SKILL_SUMMON_WRAITH = 0x2c,
    SKILL_SUMMON_JADE_GOLEM = 0x2d,
    SKILL_SUMMON_LAHARAH = 0x2e,
    SKILL_SUMMON_SAMURAI = 0x2f,
    SKILL_SUMMON_JADE_DRAGON = 0x30,
    SKILL_SUMMON_WATER_DRAGON = 0x31,
    SKILL_SUMMON_WIND_DRAGON = 0x32,
    SKILL_SUMMON_FOREST_DRAGON = 0x33,
    SKILL_BANISH_SUMMON = 0x34,
    SKILL_AIMED_ATTACK_EYES = 0x35,
    SKILL_AIMED_ATTACK_ARMS = 0x36,
    SKILL_AIMED_ATTACK_LEGS = 0x37,
    SKILL_BLINDNESS_STAB = 0x38,
    SKILL_ARM_SLICE = 0x39,
    SKILL_LEG_SWIPE = 0x3a,
    SKILL_DESPERATION = 0x3b,
    SKILL_STAMINA_BURST = 0x3c,
    SKILL_ADDED_BLOW = 0x3d,
    SKILL_LIFE_LEECH = 0x3e,
    SKILL_MIND_DRAIN = 0x3f,
    SKILL_MEDEVAN_TECHNIQUE = 0x40,
    SKILL_ICY_TOUCH = 0x41,
    SKILL_BURNING_HANDS = 0x42,
    SKILL_SHOCKING_GRASP = 0x43,
    SKILL_SACRIFICE = 0x44,
    SKILL_CONFUSION = 0x45,
    SKILL_SILENCE = 0x46,
    SKILL_AXESPIN = 0x47,
    SKILL_EARTHSHAKER = 0x48,
    SKILL_MEND = 0x49,
    SKILL_STEELSKIN = 0x4a,
    SKILL_OVERPOWER = 0x4b,
    SKILL_BLESS = 0x4c,
    SKILL_REVITALIZE = 0x4d,
    SKILL_PRAYER = 0x4e,
    SKILL_MUG = 0x4f,
    SKILL_SERPENT_WHIP = 0x50,
    SKILL_SERPENT_WHIP_TRIP = 0x51,
    SKILL_SERPENT_WHIP_PUSH = 0x52,
    SKILL_SERPENT_WHIP_MULTI = 0x53,
    SKILL_WEAK = 0x54,
    SKILL_NONE = 0x55,
};

// Seven resistance levels (char_info +0xf8, copied whole by assess_format_info): per damage type (fire,
// ice, energy, magic, slashing, piercing, blunt) an index into resistance_level_names, 2 = standard.
struct resistance_set {
    int level[7];
};

// A "$Character:" record (characterinfo_parse_table 0x4452f0; char_infos 0x6d7e50, 0x3d0 bytes each).
struct char_info {
    const char *name;        // +0x000 $Character: name
    const char *portrait;    // +0x004 +Portrait Filename:
    const char *texture_effect; // +0x008 +Texture Effect:
    const char *vfx;         // +0x00C +Vfx:
    int species;             // +0x010 SPECIES_*
    unsigned int checksum;   // +0x014
    int anim_folder;         // +0x018 +Anim Folder: path id
    int anim_class;          // +0x01C
    unsigned int anim_class_mask; // +0x020
    short max_hp;            // +0x024 $Max Hit Points:
    short hp;                // +0x026 $Hit Points:
    short max_ap;            // +0x028 $Max Ability Points:
    short ap;                // +0x02A $Ability Points:
    int xp_gained;           // +0x02C $Experience Gained:
    int level;               // +0x030 $Level:
    float attack_radius;     // +0x034 $Attack Radius:
    ushort damage;           // +0x038 $Damage: (unarmed; every read is zero-extended)
    ubyte _unk3a[0x02];
    short special_ability;   // +0x03C
    short special_arg;       // +0x03E
    float special_chance;    // +0x040
    short reflex_ability;    // +0x044
    short reflex_arg;        // +0x046
    float reflex_chance;     // +0x048
    void *moves[0x10];       // +0x04C +Move: list head per move type
    int team;                // +0x08C $Team:
    int size;                // +0x090 $Size:
    ubyte skill[0x54];       // +0x094 $Skill: by skill id
    ubyte up_skill, down_skill, right_skill, left_skill; // +0x0E8
    sound_ref alert_sound;   // +0x0EC +Alert:
    resistance_set resistance; // +0x0F8 (types.h _unkf8[0x1c])
    short num_default_weapons; // +0x114
    short default_weapons[2];  // +0x116
    ubyte _pad11a[2];
    int default_weapon_materials[2]; // +0x11C
    int prop_tag;            // +0x124 $Default Prop Tag:
    const char *prop_tags[0x10]; // +0x128
    short _unk168[0x10];     // +0x168
    short num_target_points; // +0x188 attack points (character_target_point_count, boss_ai.cpp; D901)
    ubyte _unk18a[0x10];
    short num_footstep_sets; // +0x19A
    ubyte _unk19c[0x04];
    int footstep_sounds_a[3][5]; // +0x1A0
    int footstep_sounds_b[3][5]; // +0x1DC
    int skin;                // +0x218 +Skin:
    ubyte _unk21c[0x04];
    float fov_range;         // +0x220
    float detection_range;   // +0x224
    float speedup_rate;      // +0x228
    float slowdown_rate;     // +0x22C
    float slow_turn_rate;    // +0x230
    float fast_turn_rate;    // +0x234
    float moving_turn_rate;  // +0x238
    float intelligence;      // +0x23C
    ubyte has_light;         // +0x240
    ubyte _pad241[3];
    const char *light_pos;   // +0x244
    float light_color[3];    // +0x248
    float light_atten;       // +0x254
    unsigned int flags;      // +0x258 0x8 cannot be tripped, 0x10 pushed, 0x20 backstabbed; 0x200 assess ignores the weapon
    ubyte _unk25c[0x08];
    float icon_height;       // +0x264
    void *alt_textures;      // +0x268
    int alt_index;           // +0x26C
    char_info *alt_parent;   // +0x270
    int num_drops;           // +0x274
    int drop_items[0x11];    // +0x278
    float drop_chances[0x11];// +0x2BC
    int num_mp_drops;        // +0x300
    int mp_drop_items[0x11]; // +0x304
    float mp_drop_chances[0x11]; // +0x348
    int mp_drop_levels[0x11];    // +0x38C
};
LAYOUT_CHECK(char_info, sizeof(char_info) == 0x3d0);
extern char_info char_infos[256];                     // 0x6d7e50
extern int num_char_infos;                            // 0x714e54 record count (characterinfo_parse_table; decomp29)

int   charinfo_find_by_name(const char *name);       // 0x446b80 index or -1 (decomp30, character.cpp)
int   charinfo_find_by_species(int species);          // 0x446bd0 index or -1
void *charinfo_get_multi_appearance(char_info *info); // 0x449b80
int   species_is_hero(int species);                   // 0x449a20
void  statscreen_unload_anim(Character *c);         // 0x449100
extern const char *variation_names[194];             // 0x595cf8 "", "variation 1", ... (characterinfo.cpp .data; +Variation:)
extern const char *skill_names[36];                   // 0x595b64 (characterinfo.cpp .data; 0x595bf4 ends it)
int   charinfo_find_anim(char_info *info, const char *name);   // 0x448d90, anim index or -1

// A skill record (0x34 bytes, skill_table 0x8b3f88, built by 0x442ba0 from strings.tbl).
// Fields +0x04..+0x20 from new_skill.cpp and docs/sum-exe/summons.md (D102).
struct skill_info {
    char **name;                      // +0x00 localize_strings slot
    int ap_cost;                      // +0x04 (skill_is_available, character_use_skill; was _unk04)
    int recovery;                     // +0x08 seconds (player_start_skill_cooldown multiplies by 1000)
    float hp_share;                   // +0x0c summon skills: share of max HP
    int _unk10;
    int group;                        // +0x14 skills screen page: 0 combat .. 5 spells
    int spell;                        // +0x18 the spell that carries it out (summons, backstab 0x45, assess 0x46)
    char **display_name;              // +0x1c localize_strings slot: summon popup entry name
    unsigned int flags;               // +0x20 1 passive, 2 only while attacking, 4 summon, 8 aimed attack part, 0x10 dragon
    ubyte _unk24[0x2c - 0x24];
    char **description;               // +0x2c localize_strings slot (tooltip)
    char **use_description;           // +0x30 localize_strings slot, preferred for active skills when not empty
};
LAYOUT_CHECK(skill_info, sizeof(skill_info) == 0x34);
extern skill_info skill_table[MAX_SKILLS];            // 0x8b3f88
bool skill_is_summon(int skill);                      // 0x446e70
bool skill_has_flag8(int skill);                      // 0x4473d0 skill_table +0x20 bit 3 (the aimed attack parts)
bool character_can_use_summon(int skill, Character *c);   // 0x446e90 (sum.symbols lists the arguments the other way round)
int  skill_find_by_display_name(const char *name);    // 0x448d40 -1 none
extern bool All_summons;                              // 0x8b50b0

// Chain attacks: the calls pass the Player record (sum.symbols says "char" for character_set_chain_skill).
void character_set_chain_skill(Player *p, int slot, int skill);   // 0x4493e0
int  chain_count_unlocked_skills(Player *p);          // 0x449830
int  chain_get_unlocked_skill(Player *p, int n);      // 0x449890
int  chain_config_find_skill(Player *p, int skill);   // 0x449910: slot 0..3, -1 none
int  chain_skill_get_points(Player *p, int skill);    // 0x449960 (decomp24)
void chain_skill_set_points(Player *p, int skill, int points);   // 0x4499c0 (decomp24; return unused)
void chain_config_reset_defaults();                   // 0x449310 (decomp24)

// A "+Move:" record of a char_info move list (char_info.moves[category], next at +0x40). Partial: the
// fields combat.cpp reads (decomp23, D301). Type name ours (types.h comments call it move_info).
struct move_info {
    ubyte _unk00[0x08];
    int anim;                         // +0x08 animation (types.h Character +0xa8 comment)
    int anim_class;                   // +0x0C +Animation class (charinfo_find_move)
    int category;                     // +0x10 move type (2 parry, 3 dodge, ...: living_combat_update_exchange)
    int id;                           // +0x14 id within the category (charinfo_find_move compares it)
    int variant;                      // +0x18 matched by the opponent's parry / flinch move
    struct { int id; float time; } triggers[3];   // +0x1C {id, float} pairs (move_get_trigger_time)
    ubyte _unk34[0x0c];
    move_info *next;                  // +0x40
    const char *hit_tag;              // +0x44 weapon tag name of the hit (combat_get_weapon_tag_position)
};

move_info *charinfo_find_move(char_info *info, int category, int id, int flag);     // 0x446cb0
// (out, category, info, ?, exclude id, weapon class): the call in living_combat_pick_attack_move pushes them
// in this order (sum.symbols lists info second).
int   charinfo_collect_moves(move_info **out, int category, char_info *info, int a, int exclude, int anim_class);  // 0x446d70 count
float move_get_trigger_time(move_info *move, int trigger);     // 0x446e20
bool  skill_is_combat_special(int skill);                      // 0x4473f0 (0xc..0x11)
int   character_get_chain_skill(Player *p, int slot);          // 0x449440 (SKILL_NONE none)
void  character_cycle_default_chain(Player *p);                // 0x449510 +0x184: 0 -> 3 -> 1 -> 2 -> 0 (decomp35)
void  character_learn_chain_skill(Player *p, int skill, int count);     // 0x449650 (count: chain length or 0)

// experience.tbl (xp_level_table 0x2305420)
int   xp_for_level(int level);                        // 0x489a60
float xp_level_skill_cap(int level);                  // 0x489bf0
void  experience_parse_table();                      // 0x489930 experience.tbl (GameInit)

// The hero character pools (characterinfo.cpp .bss; GameInit makes each current around character_load).
class Mempool;
extern Mempool joseph_char_mempool;                   // 0x7e12c0
extern Mempool joseph_model_mempool;                  // 0x889008
extern Mempool jekhar_char_mempool;                   // 0x65e978
extern Mempool jekhar_model_mempool;                  // 0x8b3c48
extern Mempool flece_char_mempool;                    // 0x7deee8
extern Mempool flece_model_mempool;                   // 0x847fc0
extern Mempool rosalind_char_mempool;                 // 0x714e60
extern Mempool rosalind_model_mempool;                // 0x68e1d8
void characterinfo_load_party(int path_id);           // 0x4450c0 (GameInit passes path_tables_characterinfo)

#endif
