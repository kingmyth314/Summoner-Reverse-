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

// char_info::flags (+0x258), set by the flag lines after +Texture Effect: (characterinfo_parse_table;
// decomp38). 8 / 0x10 are also set when the record has no trip / push flinch move; 0x400 marks the
// head / attack point lookup done (living_entity_setup_head); 0x10000 a multiplayer slot copy.
#define CIF_NO_SHADOW          0x0001   // +No Shadow
#define CIF_EDGED              0x0002   // +Edged
#define CIF_ELONGATED          0x0004   // +Elongated (living_entity_setup_head measures the $head bone)
#define CIF_NO_TRIP            0x0008   // +Notrip
#define CIF_NO_PUSH            0x0010   // +Nopush
#define CIF_NO_BACKSTAB        0x0020   // +Nobackstab
#define CIF_NO_AIMED_ATTACK    0x0040   // +Noaimedattack
#define CIF_NO_BLIND           0x0080   // +Noblind
#define CIF_NO_SILENCE         0x0100   // +Nosilence
#define CIF_USE_MONSTER_DAMAGE 0x0200   // +UseMonsterDamage (assess ignores the weapon)
#define CIF_HEAD_SET_UP        0x0400   // living_entity_setup_head ran
#define CIF_FLYING             0x0800   // +Flying
#define CIF_SHORT_DIALOGUE_CAMERA        0x1000   // +ShortDialogueCamera
#define CIF_REALLY_SHORT_DIALOGUE_CAMERA 0x2000   // +ReallyShortDialogueCamera
#define CIF_TALL_DIALOGUE_CAMERA         0x4000   // +TallDialogueCamera
#define CIF_NO_DRAW            0x8000   // +No draw
#define CIF_MULTI_SLOT         0x10000  // characterinfo_find_multi_slot copy

#define MAX_CHARACTER_INFO     256      // "Need to increase MAX_CHARACTER_INFO."
#define MAX_TOTAL_MOVES        3250     // move_infos ("Need to increase MAX_TOTAL_MOVES."; decomp38)
#define NUM_MOVE_TYPES         16       // char_info::moves, move_type_names
#define MOVE_TYPE_CAST         7        // "cast" moves (characterinfo_load_party collects their spells)
#define MOVE_TYPE_STATE        14       // "state" moves (named anims: charinfo_find_anim)
#define MOVE_TYPE_SCRIPT       15       // script anims (charinfo_load_script_anim, no table name)

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
    short target_points[8];  // +0x18A mesh tags of "$Attack01".."$Attack08" (living_entity_setup_head; decomp38)
    short num_footstep_sets; // +0x19A
    short num_cast_spells;   // +0x19C spells of the "cast" moves (characterinfo_load_party; decomp38)
    ubyte _pad19e[2];
    int footstep_sounds_a[3][5]; // +0x1A0
    int footstep_sounds_b[3][5]; // +0x1DC
    int skin;                // +0x218 +Skin:
    int *cast_spells;        // +0x21C into charinfo_cast_spells (characterinfo_load_party; decomp38)
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
    unsigned int flags;      // +0x258 CIF_*
    int head_bone;           // +0x25C "$head" mesh tag, -1 none (CIF_ELONGATED records; decomp38)
    float head_height;       // +0x260 distance of the head bone from the origin + 0.1 (decomp38)
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
// 84 names, one per skill id, then the count 84 (0x595cb4); items_parse_table only matches the first 36
// (D1800).
extern const char *skill_names[MAX_SKILLS];           // 0x595b64 (characterinfo.cpp .data)
extern int num_skill_names;                           // 0x595cb4 84 (no reader; label ours)
int   charinfo_find_anim(char_info *info, const char *name);   // 0x448d90, anim index or -1

// A skill record (0x34 bytes, skill_table 0x8b3f88, built by 0x442ba0 from strings.tbl).
// Fields +0x04..+0x20 from new_skill.cpp and docs/sum-exe/summons.md (D102).
struct skill_info {
    char **name;                      // +0x00 localize_strings slot
    int ap_cost;                      // +0x04 (skill_is_available, character_use_skill; was _unk04)
    int recovery;                     // +0x08 seconds (player_start_skill_cooldown multiplies by 1000)
    float hp_share;                   // +0x0c summon skills: share of max HP
    int _unk10;                       // +0x10 -1 (6 for kick; no reader in characterinfo.cpp)
    int group;                       // +0x14 skills screen page: 0 combat .. 5 spells
    int spell;                        // +0x18 the spell that carries it out (summons, backstab 0x45, assess 0x46)
    char **display_name;              // +0x1c localize_strings slot: summon popup entry name
    unsigned int flags;               // +0x20 1 passive, 2 only while attacking, 4 summon, 8 aimed attack part, 0x10 dragon
    int _unk24;                       // +0x24 -1 in every record (init_skill_table; decomp38)
    int _unk28;                       // +0x28 -1 in every record
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

// A "+Move:" record of a char_info move list (char_info.moves[category], next at +0x40), 0x4c bytes
// (move_infos, MAX_TOTAL_MOVES). Completed from characterinfo_parse_table (decomp38); combat.cpp's names
// kept (decomp23, D301). Type name ours (types.h comments call it move_info).
struct move_info {
    const char *action;               // +0x00 $Action: (the skeleton name; charinfo_find_anim matches it)
    const char *anim_file;            // +0x04 $Animation: file
    int anim;                         // +0x08 skeleton index, -1 not loaded (types.h Character +0xa8 comment)
    int anim_class;                   // +0x0C +Animation class: bit per anim_class_names entry (default 1)
    int category;                     // +0x10 move type (move_type_names; 2 parry, 3 dodge, ...)
    int id;                           // +0x14 $Variation: (variation_names index; charinfo_find_move)
    int variant;                      // +0x18 $Location: (location_names: 1 low, 2 medium, 3 high)
    struct { int id; float time; } triggers[3];   // +0x1C $Trigger: anim_event_names index, frame / 30
    sound_ref sound;                  // +0x34 $Sound: (sound_ref_parse)
    move_info *next;                  // +0x40
    const char *hit_tag;              // +0x44 $Contact tag: weapon tag name of the hit
    const char *effect;               // +0x48 $Effect: (vfx_preload for some bosses)
};
LAYOUT_CHECK(move_info, sizeof(move_info) == 0x4c);

// Flag: for the heroes, skip a move whose anim is loaded for none of the record's weapon classes.
move_info *charinfo_find_move(char_info *info, int category, int id, bool loaded_only);   // 0x446cb0
move_info *charinfo_find_move_alt(char_info *info, int category, int variant, bool loaded_only); // 0x446d10 (by +0x18)
move_info *charinfo_find_move_any(char_info *info, int anim);  // 0x446c60 any list, by anim (skips parry/flinch low)
// (out, max, info, category, exclude id, weapon class): the call in living_combat_pick_attack_move pushes them
// in this order (sum.symbols lists info second). max (10 from combat.cpp) is not checked (decomp38: the
// category is the fourth argument, D1803).
int   charinfo_collect_moves(move_info **out, int max, char_info *info, int category, int exclude, int anim_class);  // 0x446d70 count
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
// The argument is not read (decomp38).
void characterinfo_load_party(int path_id);           // 0x4450c0 (GameInit passes path_tables_characterinfo)

// ---- decomp38: the rest of characterinfo.cpp (0x4425b0-0x449d88) ----------------------------------------

// One entry of species_table (0x580e60, 223 entries, .rdata of an unknown unit): $Character: names map to
// species ids through it (characterinfo_parse_name); characterinfo_load_party parses "<name>.tbl" for every
// entry with load set.
struct species_info {
    const char *name;                 // +0x0
    int species;                      // +0x4
    ubyte load;                       // +0x8 parsed at startup (characterinfo_load_party)
    ubyte _pad09[3];
};
LAYOUT_CHECK(species_info, sizeof(species_info) == 0xc);
#define NUM_SPECIES_TABLE 223
extern species_info species_table[NUM_SPECIES_TABLE];  // 0x580e60

// A chain attack entry (0x14 bytes, chain_skill_table 0x8b3c68, 40 entries, init_chain_skill_names).
// Per hero, the chain slot (0 added blow, 1..3 the up / down / left-right chains of chainpts) and the
// chain points at which the skill is learned (0, 10, 30).
struct chain_skill {
    char **name;                      // +0x00 localize_strings slot ("Chain Kick", ...)
    int species;                      // +0x04 SPECIES_*
    int skill;                        // +0x08 skill id
    int slot;                         // +0x0C chain points counter ("chainpts<species><slot>")
    ubyte points;                     // +0x10 points needed (character_learn_chain_skill)
    ubyte _pad11[3];
};
LAYOUT_CHECK(chain_skill, sizeof(chain_skill) == 0x14);
#define NUM_CHAIN_SKILLS 40
extern chain_skill chain_skill_table[NUM_CHAIN_SKILLS];   // 0x8b3c68

// A multiplayer char_info copy (0x3ec bytes, multi_charinfo_slots 0x7def08, 4 slots).
struct multi_charinfo_slot_t {
    int used;                         // +0x000
    char_info info;                   // +0x004
    int textures[6];                  // +0x3D4 bm_reload of the mesh textures (charinfo_get_multi_appearance)
};
LAYOUT_CHECK(multi_charinfo_slot_t, sizeof(multi_charinfo_slot_t) == 0x3ec);
extern multi_charinfo_slot_t multi_charinfo_slots[4]; // 0x7def08

extern const char *summon_creature_names[16];         // 0x595b24 skills 0x24..0x33 (label ours)
extern const char *resistance_level_type_names[6];    // 0x595cb8 "weak" .. "strengthen" (+Resistances:; label ours)
extern const char *team_names[4];                     // 0x595cd0
extern const char *skill_group_names[6];              // 0x595ce0 "Combat" .. "Spells" (no reader here; label ours)
extern const char *hurt_variation_names[10];          // 0x596000 moves dropped for non-bosses (label ours)
extern int num_hurt_variation_names;                  // 0x596028 10 (label ours)
extern const char *default_footstep_sounds[2][5];     // 0x59602c left / right (label ours)
extern const char *size_names[4];                     // 0x596054 $Size: (label ours)
extern const char *move_type_names[NUM_MOVE_TYPES];   // 0x596064 (15 names; [15] NULL)
extern const char *location_names[4];                 // 0x5960a4 $Location: "", low, medium, high (label ours)
extern const char *anim_event_names[14];              // 0x5960b4
extern const char *prop_tag_names[16];                // 0x5960ec $Prop Tag: (label ours)
extern const char *attack_point_names[8];             // 0x59612c "$Attack01".. (label ours)
extern const char *offense_point_names[10];           // 0x59614c "$offense01"..
extern bool Quick_combo;                              // 0x8b50b2

class StringPool;
class TextFile;
class console_command;
class character_instance;
extern StringPool charinfo_string_pool;               // 0x68e1a8
extern StringPool charinfo_script_anim_pool;          // 0x68e1c0
extern move_info move_infos[MAX_TOTAL_MOVES];         // 0x7e12e8 (label ours)
extern move_info *move_info_free_list;                // 0x7916dc
extern int characterinfo_anim_slot_count;             // 0x68e1f4
extern int charinfo_cast_spells[250];                 // 0x88904c (label ours)
extern Mempool player_instance_mempool;               // 0x744680
extern Mempool statscreen_mempool;                    // 0x7592a0 (label ours)
extern Mempool joseph_anim_mempool;                   // 0x889028 (label ours)
extern Mempool jekhar_anim_mempool;                   // 0x847fe8 (label ours)
extern Mempool flece_anim_mempool;                    // 0x6d7e30 (label ours)
extern Mempool rosalind_anim_mempool;                 // 0x6d7e10 (label ours)
extern Character *stashed_summon;                     // 0x8b3c34
extern Player *stashed_summon_player;                 // 0x7e12e0
extern int charinfo_level_mem_used;                   // 0x8b3c44
extern console_command ccmd_All_summons;              // 0x68e200
extern console_command ccmd_Quick_combo;              // 0x65e998
extern console_command ccmd_charinfo;                 // 0x65e968
extern console_command ccmd_levelchars;               // 0x8b3c38

void cmd_All_summons();                               // 0x444a50
void cmd_Quick_combo();                               // 0x449590
void cmd_levelchars();                                // 0x449c00
// 1 when the move need not be loaded on this level (towns, masad, the boss levels; decomp38).
int   move_used_on_level(char_info *info, move_info *move, bool full_set);   // 0x444bb0
char_info *characterinfo_parse_table(const char *filename);   // 0x4452f0 the last record of the file
move_info *characterinfo_add_move();                  // 0x446740 from move_info_free_list
void  move_info_free(move_info *move);                // 0x446770
void  sound_ref_parse(TextFile *tf, char_info *info, sound_ref *ref, const char *folder);  // 0x446790
void  characterinfo_parse_name(TextFile *tf, char_info *info);          // 0x446890 name + species
void  characterinfo_parse_resistances(TextFile *tf, char_info *info);   // 0x446930
void  living_entity_setup_head(char_info *info, character_instance *ci); // 0x4469c0 prop tags, attack points, head
int   summon_find_charinfo(int spell);                // 0x446c00 char_infos index of the summon cast by the spell, -1
const char *summon_skill_creature_name(int skill);    // 0x446c40 "" for other skills
multi_charinfo_slot_t *multi_charinfo_slot(char_info *info);   // 0x446de0 the slot holding the copy, NULL
void  characterinfo_load_party_anims(char_info *info, int anim_class, bool load_sounds, bool summon);   // 0x447410
void  charinfo_load_move_anim(char_info *info, struct ca_character *ca, move_info *move, bool load_sounds,
                              int anim_folder, bool full_set);  // 0x447780
void  charinfo_remap_town_moves(char_info *info, bool full_set); // 0x447850
void  level_load_preload(char_info *info, bool load_sounds, bool summon, Mempool *pool);  // 0x447b10
unsigned int str_hash_nocase_rol6(const char *s);     // 0x447e80 -1 for NULL
void  charinfo_mark_anims_loaded(char_info *info);    // 0x447ec0 anim_class = -1 (and the copies)
void  character_reinit_anims(Character *c, int anim_class);     // 0x447f20
void  summon_stash_for_cutscene();                    // 0x448000
int   charinfo_load_script_anim(char_info *info, const char *name, int path_id);  // 0x4482a0 anim, -1
void  charinfo_free_moves(char_info *info);           // 0x4484a0 the script anims
void  summon_restore_after_cutscene();                // 0x448500
void  charinfo_mark_anims_unloaded(char_info *info);  // 0x4486b0 anim_class = 0 (and the copies)
void  charinfo_unload_anims(char_info *info);         // 0x448710
void  charinfo_page_in_sounds(char_info *info);       // 0x448790 footsteps
const char *offense_point_name(int i);                // 0x4487f0
void  characterinfo_alloc_level_buffers();            // 0x448800
void  characterinfo_level_unload(bool reset_player_pool);   // 0x448c10
float resistance_level_damage_mult(int level);        // 0x448c80 1 - damage multiplier (weak -1 .. strengthen 1.5)
int   summon_skill_from_creature_name(const char *name);    // 0x448d00 -1
character_instance *charinfo_create_character_instance(char_info *info, bool summon);   // 0x448de0
Mempool *charinfo_select_mempool(int species, bool summon); // 0x448f20
void  statscreen_load_anim(Character *c, bool heavy); // 0x448f60
void  chain_config_set(int species, int slot, int skill);   // 0x449200 savegame "chainconfig%d%d"
int   chain_config_get(int species, int slot);        // 0x449240 (resets the defaults when 0)
void  chain_points_set(int species, int slot, int points);  // 0x449290 savegame "chainpts%d%d"
int   chain_points_get(int species, int slot);        // 0x4492d0
char_info *characterinfo_find_multi_slot(int index);  // 0x449a30 multiplayer heroes get a slot copy
void  multi_charinfo_slot_release(char_info *info);   // 0x449b00
void  multi_charinfo_slots_clear();                   // 0x449b60

#endif
