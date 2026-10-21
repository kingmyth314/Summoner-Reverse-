// characterinfo.cpp - Sum.exe v1.40, code 0x4425b0-0x449d88 (Engine\characterinfo\characterinfo.cpp)
// Written from the Sum.exe disassembly (tools/asm.mjs; llvm-objdump). Target: VC++ 6.0 (cl 12.00.8966),
// /O2 like the other optimised game units (no frame pointer, 0x90 padding).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: sum.symbols tags 0x4425b0 (init_chain_skill_names_thunk) to 0x449c00 (cmd_levelchars) with
// characterinfo.cpp; 0x442350-0x442380 (disp_rate) belong to the net unit before it (sum.symbols comment)
// and items.cpp starts at 0x449d90.
//
// Data: .data 0x595b24-0x596173 (the name tables below), then this unit's literals (0x596174-0x598f4c,
// in reverse order of first use as VC6 emits them); .bss 0x65e968-0x8b50b2. Literals pooled with earlier
// units (/Gf): "masad" 0x5930c0, "masad_v2"-style shared strings of player.cpp / gameplay ("$Character:"
// 0x593d54, "$Level:" 0x593a58, "$Animation:" 0x593a34, "Ring of Fire" 0x593e78, "Ring of Darkness"
// 0x593e64, "fire_ring_xp" 0x593e54, "got_ring_of_water" 0x5917d0, "Liangshan" 0x592cf4, ".mvf" 0x594e18,
// ".svf" 0x594dec, "Tiger Rider" 0x596d44 (pooled later in this unit), "#End" 0x58bd08, "%d" 0x589820,
// the console strings 0x588c34-0x588c50), "" 0x5bd0d0.
//
// Static initialisers (static_initializers entries 159-177 and three after the functions), in source
// order: chain_skill_table, skill_table, resistance_level_names (localize_strings slots: dynamic because
// they store &localize_strings[id]), 15 empty initialisers (the 15 Mempools, inline empty constructor,
// D1801), ccmd_All_summons, then the two StringPools (constructor + atexit destructor). ccmd_Quick_combo,
// ccmd_charinfo and ccmd_levelchars are defined further down, next to their functions.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "engine/characterinfo/characterinfo.h"
#include "engine/objects/living_entity.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/effects/fx.h"
#include "engine/gamesound/gamesound.h"
#include "engine/spells/spells.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/os/console.h"
#include "vsdk/parse/parse.h"
#include "vsdk/vfile/file.h"
#include "vsdk/localize/localize.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/snd/snd.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/geom/effect_mem.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"

// Original __FILE__ of this unit (literal 0x5987bc).
#define CHARACTERINFO_CPP "D:\\projects\\Summoner\\pccode\\Engine\\characterinfo\\characterinfo.cpp"

// ---- .data (0x595b24-0x596173) -------------------------------------------------------------------------
const char *summon_creature_names[16] = {             // 0x595b24 (skills 0x24..0x33)
    "Golem", "Red Minotaur", "Blade of Urath", "Blue Imp", "Blackfire Elemental", "Blood Elemental",
    "Poison Elemental", "Mist Elemental", "Wraith", "Jade Golem", "Spirit of Laharah", "Celestial Samurai",
    "Jade Dragon", "Water Dragon", "Four Winds Dragon", "Forest Dragon",
};
const char *skill_names[MAX_SKILLS] = {                // 0x595b64
    "hand to hand", "protection", "speed", "sword weapons", "axe weapons", "blunt weapons",
    "staff weapons", "bow weapons", "heavy arms", "parry", "counter attack", "dodge", "critical hit",
    "double attack", "push", "kick", "trip", "aimed attack", "backstab", "assess", "appraise",
    "pick lock", "sneak", "hide", "forge", "mine", "enchant", "summon", "heal", "dark", "energy", "holy",
    "fire", "ice", "divination", "magic resist", "summon golem", "summon red minotaur",
    "summon Blade of Urath", "summon blue imp", "summon blackfire elemental", "summon blood elemental",
    "summon poison elemental", "summon mist elemental", "summon wraith", "summon jade golem",
    "summon laharah", "summon samurai", "summon jade dragon", "summon water dragon",
    "summon wind dragon", "summon forest dragon", "banish summon", "aimed attack eyes",
    "aimed attack arms", "aimed attack legs", "blindness stab", "arm slice", "leg swipe", "Desperation",
    "stamina burst", "added blow", "life leech", "mind drain", "medevan technique", "icy touch",
    "burning hands", "shocking grasp", "sacrifice", "confusion", "silence", "axespin", "earthshaker",
    "mend", "steelskin", "overpower", "bless", "revitalize", "prayer", "mug", "serpent whip",
    "serpent whip trip", "serpent whip push", "serpent whip multi",
};
int num_skill_names = sizeof(skill_names) / sizeof(skill_names[0]);   // 0x595cb4 (no reader)
const char *resistance_level_type_names[6] = {         // 0x595cb8 (+Resistances: values)
    "weak", "vulnerable", "standard", "strong", "invulnerable", "strengthen",
};
const char *team_names[4] = { "friendly", "neutral", "hostile", "evil" };   // 0x595cd0
const char *skill_group_names[6] = {                   // 0x595ce0 (no reader in this unit)
    "Combat", "Stealth", "Creation", "Examination", "Weapons", "Spells",
};
const char *variation_names[194] = {          // 0x595cf8 (+Variation:; also spells, boss_ai)
    "", "variation 1", "variation 2", "variation 3", "variation 4", "variation 5", "variation 6",
    "variation 7", "variation 8", "variation 9", "variation 10", "variation 11", "variation 12",
    "variation 13", "variation 14", "variation 15", "variation 16", "variation 17", "variation 18",
    "variation 19", "variation 20", "variation 21", "variation 22", "variation 23", "variation 24",
    "variation 25", "variation 26", "variation 27", "variation 28", "variation 29", "variation 30",
    "ranged_attack low", "ranged_attack medium", "ranged_attack high", "backward", "stand_to_ready",
    "ready_to_stand", "stand_to_ready_hurt", "ready_to_stand_hurt", "ready_to_cloaked", "cloaked_to_ready",
    "heal", "cure", "regenerate", "restore", "resurrect", "vitalize", "revitalize", "revive", "blind",
    "silence", "black cloud", "echo chamber", "recall", "sleep", "invisibility", "death", "light", "lbolt",
    "gravity", "chain lightning", "empower", "haste", "thunderstorm", "power leech", "turn undead",
    "destroy undead", "bless", "wrath of god", "control undead", "protect", "faith", "curse", "fire arrow",
    "fireball", "burning hands", "meteor storm", "wall of fire", "ring of fire", "fireshield", "inferno",
    "icicle", "icy sleep", "freeze", "blizzard", "slow", "wall of ice", "iceshield", "ice coffin",
    "detect monsters", "detect magic", "free view", "scrye", "glyph", "control summon", "banish summon",
    "predict", "dark wall", "poison", "blue imp sleep", "life drain", "jade beam", "kick", "trip", "push",
    "double attack", "inspect stats", "appraise", "backstab", "set trap", "disarm trap", "hide", "steal",
    "pick lock", "forge", "aimed attack", "mine", "enchant", "medevan technique", "axespin", "earthshaker",
    "attackcast", "summon golem", "summon red minotaur", "summon blur", "summon blue imp",
    "summon blackfire elemental", "summon water", "summon forest", "status", "area", "Luminar bite",
    "Luminar near hit", "Luminar far hit", "Luminar grab", "Luminar throw", "Luminar holding hit",
    "Luminar beam attack", "Luminar teleport out", "Luminar teleport in", "flail", "pick up ready",
    "tripped fall", "tripped lie down", "tripped get up", "pushed", "stand", "ready", "stand hurt",
    "ready hurt", "walk", "run", "norm walk", "norm run", "walk hurt", "run hurt", "norm walk hurt",
    "norm run hurt", "statscreen light", "statscreen heavy", "hide_ready", "cloaked_ready", "cloaked_sneak",
    "walk up stairs", "walk down stairs", "turn left", "turn right", "gsal swipe 1", "gsal fireball 1",
    "gsal kick left 1", "gsal kick right 1", "gsal tail 1", "gsal swipe 2", "gsal fireball 2",
    "gsal kick right 2", "gsal bite 3", "gsal fireball 3", "gsal kick right 3", "tgr spear 1",
    "tgr spear 2", "tgr spear 3", "tgr spear 4", "tgr throw spear", "tgr transition axe", "tgr charge axe",
    "tgr swipe axe", "serp1 push", "serp1 sickle", "serp1 nightmare", "phoenix wall of fire1",
    "phoenix inferno1", "phoenix wall of fire23", "phoenix inferno23", "phoenix burning hands",
};
const char *hurt_variation_names[10] = {      // 0x596000 (dropped for non-bosses)
    "stand_to_ready_hurt", "ready_to_stand_hurt", "stand hurt", "ready hurt", "walk hurt", "run hurt",
    "norm walk hurt", "norm run hurt", "turn left", "turn right",
};
int num_hurt_variation_names = sizeof(hurt_variation_names) / sizeof(hurt_variation_names[0]);   // 0x596028

// .rdata 0x580e60-0x5818d3 (const), after summoner.cpp's SEH table (0x580e50) and before living_entity.cpp's
// size heights (0x5818d8). Defined here in source order: its names are this unit's literals 0x5971ec-0x5965c0,
// emitted after hurt_variation_names' and before default_footstep_sounds'. {name, species id (not the index),
// load}. The dword after it (0x5818d4) holds 223; the code uses that address only as the end bound of its loops.
const species_info species_table[NUM_SPECIES_TABLE] = {   // 0x580e60
    { "Joseph", 0, 1 },
    { "Jekhar", 1, 1 },
    { "Flece", 2, 1 },
    { "Rosalind", 3, 1 },
    { "Slave Foreman", 55, 0 },
    { "Slave Driver", 56, 0 },
    { "Slave Female", 54, 0 },
    { "Slave Male", 53, 0 },
    { "Khosani PriestessA", 47, 0 },
    { "Khosani Keeper", 46, 0 },
    { "Khosani PriestessB", 48, 0 },
    { "Khosani PriestessC", 49, 0 },
    { "Khosani ElderB", 50, 0 },
    { "Khosani ElderC", 51, 0 },
    { "Khosani ElderD", 52, 0 },
    { "Kothudin", 45, 0 },
    { "Shrine Keeper", 44, 0 },
    { "Wounded Orenian Soldier", 41, 0 },
    { "Medevan Scribe1", 38, 0 },
    { "Medevan Scribe2", 39, 0 },
    { "Armacelsus", 40, 0 },
    { "Medevan Servant1", 26, 0 },
    { "Medevan Servant2", 27, 0 },
    { "Palace Guard", 34, 0 },
    { "Medevan Lord1", 28, 0 },
    { "Medevan Lord2", 29, 0 },
    { "Medevan Page1", 32, 0 },
    { "Medevan Page2", 33, 0 },
    { "Medevan Squire1", 30, 0 },
    { "Medevan Squire2", 31, 0 },
    { "Medevan Noblewoman", 25, 0 },
    { "Medevan Knight3", 37, 0 },
    { "Medevan Knight2", 36, 0 },
    { "Medevan Knight1", 35, 0 },
    { "Medevan Priest Prelate", 24, 0 },
    { "Medevan Priest Old", 23, 0 },
    { "Medevan Priest Initiate", 22, 0 },
    { "Medevan Priest2", 21, 0 },
    { "Mummy", 20, 1 },
    { "Medevan Glassblower", 17, 0 },
    { "Medevan Miller", 18, 0 },
    { "Medevan Archer", 19, 1 },
    { "Khosani Warrior3", 16, 0 },
    { "Khosani Warrior2", 15, 0 },
    { "Khosani General", 14, 0 },
    { "Sir Ammund", 12, 0 },
    { "Temple Guard", 13, 0 },
    { "Golem", 4, 1 },
    { "Brass Golem", 5, 1 },
    { "Broken Brass Golem", 7, 1 },
    { "Onyx Golem", 8, 1 },
    { "Jade Golem", 9, 1 },
    { "Steel Golem", 10, 1 },
    { "Iron Golem", 11, 1 },
    { "Grey Stone Golem", 6, 1 },
    { "Generic Fighter", 57, 1 },
    { "Onif", 58, 1 },
    { "Oni Male", 59, 1 },
    { "Oni Mage", 60, 1 },
    { "OG mage", 61, 1 },
    { "Iona", 62, 1 },
    { "Generic Mage", 65, 1 },
    { "Barbarian Fighter", 66, 1 },
    { "Bone Servant", 67, 1 },
    { "Bone Servant2", 68, 1 },
    { "Bone Mage", 69, 1 },
    { "Village Woman", 70, 1 },
    { "Village Woman2", 71, 1 },
    { "Village Man", 72, 1 },
    { "Village Man2", 73, 1 },
    { "Village Man3", 74, 1 },
    { "Village Man4", 75, 1 },
    { "Village Man4 Crawling", 76, 1 },
    { "Village Man1 Bum", 77, 1 },
    { "Village Man1 Flogged", 78, 1 },
    { "Village Man2 Drunk", 79, 1 },
    { "Village Man3 Sneaking", 80, 1 },
    { "Luminar", 81, 1 },
    { "Cabbit", 83, 1 },
    { "Horse1", 84, 1 },
    { "WarHorse1", 85, 1 },
    { "Rat", 86, 1 },
    { "Tiger Rider", 87, 1 },
    { "Phoenix Rider", 88, 1 },
    { "Serpent Rider", 89, 1 },
    { "Ghost Rider", 90, 1 },
    { "Crow", 91, 1 },
    { "Orenian Soldier1", 94, 1 },
    { "Orenian Soldier2", 96, 1 },
    { "Orenian Scout", 95, 1 },
    { "Orenian Archer", 99, 1 },
    { "Green Bacite", 92, 1 },
    { "Brown Bacite", 93, 1 },
    { "Medevan Boy1", 100, 1 },
    { "Medevan Boy Sweeping", 101, 1 },
    { "Medevan Girl1", 102, 1 },
    { "Medevan Girl Stirring", 103, 1 },
    { "Medevan Soldier1", 104, 1 },
    { "Medevan Apprentice", 109, 0 },
    { "Medevan Elite Guard", 112, 0 },
    { "Medevan Sailor", 113, 0 },
    { "Medevan Thief", 114, 0 },
    { "Medevan Beggar", 115, 0 },
    { "medevan Gypsy", 116, 0 },
    { "Yago", 121, 1 },
    { "Aoqi", 117, 1 },
    { "Orenian Shopkeep1", 97, 1 },
    { "Orenian Shopkeep2", 98, 1 },
    { "Medevan Priest1", 106, 1 },
    { "Medevan Shop Woman1", 107, 1 },
    { "Medevan Shopkeep2", 108, 1 },
    { "Bellias", 118, 1 },
    { "Lenele Courtier1", 119, 1 },
    { "Lenele Herald", 120, 0 },
    { "Abbot Laurent", 122, 1 },
    { "Khosani Warrior1", 123, 1 },
    { "Iona Monk1", 124, 1 },
    { "Iona Monk Sitting1", 125, 1 },
    { "Iona Monk Meditate", 126, 1 },
    { "Iona Monk Chamberlain", 127, 1 },
    { "Iona Monkette", 128, 1 },
    { "Iona Monkette Meditate", 130, 1 },
    { "Iona Monkette Sitting1", 129, 1 },
    { "KungFu Monk", 131, 1 },
    { "KungFu Monkette", 132, 1 },
    { "Med High Priest", 143, 1 },
    { "Red Minotaur", 144, 1 },
    { "Sornehan", 145, 1 },
    { "Sornehan Knight", 146, 1 },
    { "Tentacle Beast", 147, 1 },
    { "Alligator Beast", 148, 1 },
    { "Carados", 149, 1 },
    { "Cave Guardian", 150, 1 },
    { "Chicken", 151, 1 },
    { "Galienne", 152, 1 },
    { "Gorgonne", 153, 1 },
    { "Greater Gorgonne", 154, 1 },
    { "Medevan Male Inkeeper", 110, 0 },
    { "Medevan Female Inkeeper", 111, 0 },
    { "Tancred", 155, 1 },
    { "Aspirant Girl", 140, 0 },
    { "Aspirant Girl Stirring", 141, 0 },
    { "Aspirant Boy", 142, 0 },
    { "Novice Male", 133, 0 },
    { "Novice Female", 134, 0 },
    { "Keykeeper", 135, 0 },
    { "Cellarer", 136, 0 },
    { "Doorkeeper", 137, 0 },
    { "Master of Arms", 138, 0 },
    { "Prior", 139, 0 },
    { "Bone Knight1", 156, 1 },
    { "Frozen Bone Knight", 157, 1 },
    { "Bone King", 158, 1 },
    { "Ice Witch", 159, 1 },
    { "Aspirant Male", 160, 0 },
    { "Khosani High Priestess", 161, 1 },
    { "Gargoyle", 162, 1 },
    { "Fire Imp", 163, 1 },
    { "Blue Imp", 164, 1 },
    { "Black Imp", 165, 1 },
    { "Bacite Shaman", 166, 1 },
    { "Baby Bacite", 167, 1 },
    { "Kungfu Monk Idle", 168, 1 },
    { "Kungfu Monkette Idle", 169, 1 },
    { "Crystal Rokhul", 170, 1 },
    { "Normal Rokhul", 63, 1 },
    { "Sand Rokhul", 64, 1 },
    { "Bone Knight2", 171, 1 },
    { "Giant Salamanka", 172, 1 },
    { "Orenian Samurai", 173, 1 },
    { "Fire Salamanka", 174, 1 },
    { "Onyx Gargoyle", 175, 1 },
    { "Blade of Urath", 176, 1 },
    { "Evil Urath", 177, 1 },
    { "Pijian", 178, 1 },
    { "Elite Samurai", 179, 1 },
    { "Boat", 180, 1 },
    { "Water Dragon", 181, 1 },
    { "Four Winds Dragon", 182, 1 },
    { "Murod", 183, 1 },
    { "Murod Oni", 184, 1 },
    { "Wraith", 185, 1 },
    { "Stone Statue", 187, 1 },
    { "Ikaemos Priest King", 186, 1 },
    { "Qifeng", 188, 1 },
    { "Lich", 189, 1 },
    { "Archlich", 190, 1 },
    { "Khosani Elder", 42, 1 },
    { "Khosani Miner", 43, 1 },
    { "Blackfire Elemental", 192, 1 },
    { "Titus", 193, 1 },
    { "Jade Dragon", 194, 1 },
    { "Forest Dragon", 195, 1 },
    { "Pyrul", 196, 1 },
    { "Machival", 197, 1 },
    { "Dog1", 198, 1 },
    { "Cat", 199, 1 },
    { "TrashMaster", 200, 1 },
    { "Evil Joseph", 201, 1 },
    { "Poison Elemental", 202, 1 },
    { "Blood Elemental", 203, 1 },
    { "Mist Elemental", 204, 1 },
    { "Spirit of Laharah", 205, 1 },
    { "Celestial Samurai", 206, 1 },
    { "Black Knight", 207, 1 },
    { "Blue Bacite", 208, 1 },
    { "Blue Minotaur", 209, 1 },
    { "Blue Oni Male", 210, 1 },
    { "Burning Bone Knight", 211, 1 },
    { "Forest Hag", 212, 1 },
    { "Green Imp", 213, 1 },
    { "Red Oni Female", 214, 1 },
    { "Red Bacite", 215, 1 },
    { "Red Rokhul", 216, 1 },
    { "Sand Minotaur", 217, 1 },
    { "Standin Man1", 220, 1 },
    { "Standin Man2", 221, 1 },
    { "Standin Kid1", 222, 1 },
    { "Standin Kid2", 223, 1 },
    { "Standin Gal1", 224, 1 },
    { "Standin Gal2", 225, 1 },
    { "Jade Temple Priest", 218, 0 },
    { "Jade Temple Soldier", 219, 0 },
};
// The per-size heights (character_size_height, charinfo_boss_height) follow at 0x5818d8, before this unit's
// float constants (0x5818e8-0x581903: characterinfo_parse_table's 0.01s, charinfo_load_move_anim's ...), so
// they are this unit's .rdata, not living_entity.cpp's (whose .rdata begins at 0x581904).
const float character_size_heights[4] = { 0.9f, 1.8f, 2.5f, 99.9f };   // 0x5818d8

const char *default_footstep_sounds_left[5] = {      // 0x59602c
    "footsteps\\left_wood_d.wav", "footsteps\\left_stone_h.wav", "footsteps\\left_grass_a.wav",
    "footsteps\\left_sand_a.wav", "footsteps\\left_wet_a.wav"
};
const char *default_footstep_sounds_right[5] = {     // 0x596040
    "footsteps\\right_wood_d.wav", "footsteps\\right_stone_h.wav", "footsteps\\right_grass_a.wav",
    "footsteps\\right_sand_a.wav", "footsteps\\right_wet_a.wav"
};
const char *size_names[4] = {                 // 0x596054 ($Size:)
    "small", "medium", "large", "huge",
};
const char *move_type_names[NUM_MOVE_TYPES] = { // 0x596064 (+Move:; [15] NULL)
    "attack", "special attack", "parry", "dodge", "flinch", "non-combat flinch", "death", "cast",
    "movement", "misc", "transition", "ranged attack", "emerge", "banish", "state",
};
const char *location_names[4] = {             // 0x5960a4 ($Location:)
    "", "low", "medium", "high",
};
const char *anim_event_names[14] = {          // 0x5960b4 ($Trigger:)
    "", "generic", "generic 1", "generic 2", "generic 3", "generic 4", "critical end", "impact start",
    "impact end", "no interrupt", "swap prop", "start skin anim", "left-foot", "right-foot",
};
const char *prop_tag_names[16] = {            // 0x5960ec ($Prop Tag:)
    "primary hand", "secondary hand", "luggage swords", "luggage maces", "luggage large swords",
    "luggage staves", "luggage pole arms", "luggage bows", "luggage crossbows", "luggage daggers",
    "luggage 1", "tiger spear01", "tiger spear02", "tiger spear03", "tiger spear04", "shield",
};
const char *attack_point_names[8] = {         // 0x59612c (living_entity_setup_head)
    "$Attack01", "$Attack02", "$Attack03", "$Attack04", "$Attack05", "$Attack06", "$Attack07", "$Attack08",
};
const char *offense_point_names[10] = {       // 0x59614c (offense_point_name)
    "$offense01", "$offense02", "$offense03", "$offense04", "$offense05", "$offense06", "$offense07",
    "$offense08", "$offense09", "$offense10",
};

// ---- .bss (0x65e968-0x8b50b2), in address order --------------------------------------------------------
// The objects with dynamic initialisers are defined further down in their source order (the initialiser
// order); their addresses are listed here as comments. Buffer and table labels are ours unless sum.symbols
// names them. ccmd_charinfo 0x65e968, ccmd_Quick_combo 0x65e998, charinfo_string_pool 0x68e1a8,
// charinfo_script_anim_pool 0x68e1c0, ccmd_All_summons 0x68e200, the Mempools, ccmd_levelchars 0x8b3c38,
// chain_skill_table 0x8b3c68, skill_table 0x8b3f88, resistance_level_names 0x8b5098.
char rosalind_char_buffer[0x2f800];           // 0x65e9a4
void *joseph_anim_buffer;                     // 0x68e1a4
// Between charinfo_string_pool (0x68e1a8, 0x14 bytes) and charinfo_script_anim_pool (0x68e1c0): an int of
// this unit's .bss that only mp_send_jn_rqst reads (the join request's +0x40); no instruction writes it by address (name ours).
int mp_join_unk_68e1bc;                       // 0x68e1bc
int joseph_anim_buffer_size;                  // 0x68e1d4
int charinfo_cast_spells_used;            // 0x68e1f4 entries of charinfo_cast_spells in use
void *statscreen_anim_buffer;                 // 0x68e1f8
int charinfo_anim_mem_total;                  // 0x68e1fc the four hero anim blocks (written only)
char jekhar_model_buffer[0x49c00];            // 0x68e20c
int summon_anim_buffer_size;                  // 0x6d7e4c
char_info char_infos[MAX_CHARACTER_INFO];     // 0x6d7e50
void *summon_anim_buffer;                     // 0x714e50
int num_char_infos;                           // 0x714e54
void *jekhar_anim_buffer;                     // 0x714e58
char flece_char_buffer[0x2f800];              // 0x714e7c
char charinfo_string_pool_buffer[0x14c00];    // 0x74469c
char joseph_model_buffer[0x38400];            // 0x7592dc
move_info *move_info_free_list;               // 0x7916dc
int flece_anim_buffer_size;                   // 0x7916e0
char flece_model_buffer[0x4d800];             // 0x7916e4
multi_charinfo_slot_t multi_charinfo_slots[4];// 0x7def08
char charinfo_script_anim_pool_buffer[0x1400];// 0x7dfeb8
int rosalind_anim_buffer_size;                // 0x7e12b8
void *flece_anim_buffer;                      // 0x7e12dc
Player *stashed_summon_player;                // 0x7e12e0
move_info move_infos[MAX_TOTAL_MOVES];        // 0x7e12e8
char joseph_char_buffer[0x2a800];             // 0x81d7c0
int statscreen_anim_buffer_size;              // 0x847fdc 0x7800 (written only)
int jekhar_anim_buffer_size;                  // 0x847fe0
char player_instance_buffer[0xa000];          // 0x848004
char rosalind_model_buffer[0x37000];          // 0x852004
unsigned int summon_stash_level_offset;       // 0x889044
void *rosalind_anim_buffer;                   // 0x889048
int charinfo_cast_spells[250];                // 0x88904c (up to 0x889434)
char jekhar_char_buffer[0x2a800];             // 0x889434
Character *stashed_summon;                    // 0x8b3c34
int charinfo_level_mem_used;                  // 0x8b3c44
bool All_summons;                             // 0x8b50b0
bool charinfo_party_loaded;                   // 0x8b50b1 characterinfo_load_party ran
bool Quick_combo;                             // 0x8b50b2

// ---- static initialisers, in source order --------------------------------------------------------------

// 0x4425b0 init_chain_skill_names_thunk (jmp). Confidence: exact.
// 0x4425c0 init_chain_skill_names: the stores of the table below (registers hold the repeated values).
// Confidence: likely (one initialiser per table; the store order is the scheduler's).
chain_skill chain_skill_table[NUM_CHAIN_SKILLS] = {   // 0x8b3c68
    { XSTR_SLOT("Chain Added Blow", 7), SPECIES_JOSEPH, 0x3d, 0, 0 },
    { XSTR_SLOT("Chain Added Blow", 7), SPECIES_FLECE, 0x3d, 0, 0 },
    { XSTR_SLOT("Chain Added Blow", 7), SPECIES_JEKHAR, 0x3d, 0, 0 },
    { XSTR_SLOT("Chain Added Blow", 7), SPECIES_ROSALIND, 0x3d, 0, 0 },
    { XSTR_SLOT("Chain Kick", 8), SPECIES_JEKHAR, 0xf, 1, 0 },
    { XSTR_SLOT("Chain Axespin", 9), SPECIES_JEKHAR, 0x47, 1, 10 },
    { XSTR_SLOT("Chain Earthshaker", 10), SPECIES_JEKHAR, 0x48, 1, 30 },
    { XSTR_SLOT("Chain Mend", 11), SPECIES_JEKHAR, 0x49, 2, 0 },
    { XSTR_SLOT("Chain Steelskin", 12), SPECIES_JEKHAR, 0x4a, 2, 10 },
    { XSTR_SLOT("Chain Overpower", 13), SPECIES_JEKHAR, 0x4b, 2, 30 },
    { XSTR_SLOT("Chain Desperation", 14), SPECIES_JEKHAR, 0x3b, 3, 0 },
    { XSTR_SLOT("Chain Sacrifice", 15), SPECIES_JEKHAR, 0x44, 3, 10 },
    { XSTR_SLOT("Chain Life Leech", 16), SPECIES_JEKHAR, 0x3e, 3, 30 },
    { XSTR_SLOT("Chain Kick", 8), SPECIES_FLECE, 0xf, 1, 0 },
    { XSTR_SLOT("Chain Mug", 17), SPECIES_FLECE, 0x4f, 1, 10 },
    { XSTR_SLOT("Chain Trip", 18), SPECIES_FLECE, 0x10, 1, 30 },
    { XSTR_SLOT("Chain Leg Swipe", 19), SPECIES_FLECE, 0x3a, 2, 0 },
    { XSTR_SLOT("Chain Arm Slice", 20), SPECIES_FLECE, 0x39, 2, 10 },
    { XSTR_SLOT("Chain Blind Stab", 21), SPECIES_FLECE, 0x38, 2, 30 },
    { XSTR_SLOT("Chain Desperation", 14), SPECIES_FLECE, 0x3b, 3, 0 },
    { XSTR_SLOT("Chain Stamina Burst", 22), SPECIES_FLECE, 0x3c, 3, 10 },
    { XSTR_SLOT("Chain Silence", 23), SPECIES_FLECE, 0x46, 3, 30 },
    { XSTR_SLOT("Chain Shock Grsp", 24), SPECIES_ROSALIND, 0x43, 1, 0 },
    { XSTR_SLOT("Chain Burn Hands", 25), SPECIES_ROSALIND, 0x42, 1, 10 },
    { XSTR_SLOT("Chain Icy Touch", 26), SPECIES_ROSALIND, 0x41, 1, 30 },
    { XSTR_SLOT("Chain Bless", 27), SPECIES_ROSALIND, 0x4c, 2, 0 },
    { XSTR_SLOT("Chain Revitalize", 28), SPECIES_ROSALIND, 0x4d, 2, 10 },
    { XSTR_SLOT("Chain Prayer", 29), SPECIES_ROSALIND, 0x4e, 2, 30 },
    { XSTR_SLOT("Chain Confusion", 30), SPECIES_ROSALIND, 0x45, 3, 0 },
    { XSTR_SLOT("Chain Stamina Burst", 22), SPECIES_ROSALIND, 0x3c, 3, 10 },
    { XSTR_SLOT("Chain Mind Drain", 31), SPECIES_ROSALIND, 0x3f, 3, 30 },
    { XSTR_SLOT("Chain Desperation", 14), SPECIES_JOSEPH, 0x3b, 1, 0 },
    { XSTR_SLOT("Chain Stamina Burst", 22), SPECIES_JOSEPH, 0x3c, 1, 10 },
    { XSTR_SLOT("Chain Silence", 23), SPECIES_JOSEPH, 0x46, 1, 30 },
    { XSTR_SLOT("Chain Confusion", 30), SPECIES_JOSEPH, 0x45, 2, 0 },
    { XSTR_SLOT("Chain Revitalize", 28), SPECIES_JOSEPH, 0x4d, 2, 10 },
    { XSTR_SLOT("Chain Burn Hands", 25), SPECIES_JOSEPH, 0x42, 2, 30 },
    { XSTR_SLOT("Chain Push", 32), SPECIES_JOSEPH, 0xe, 3, 0 },
    { XSTR_SLOT("Chain Mind Drain", 31), SPECIES_JOSEPH, 0x3f, 3, 10 },
    { XSTR_SLOT("Chain Life Leech", 16), SPECIES_JOSEPH, 0x3e, 3, 30 },
};

// 0x442b90 init_skill_table_thunk (jmp). Confidence: exact.
// 0x442ba0 init_skill_table: the stores of the table below. Confidence: likely (as above). Fields:
// {name, ap_cost, recovery, hp_share, _unk10, group, spell, display_name, flags, _unk24, _unk28,
//  description, use_description}; L34 is the empty string.
skill_info skill_table[MAX_SKILLS] = {                // 0x8b3f88
    { XSTR_SLOT("hand to hand", 33), 0, 0, 0.0f, -1, 4, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Inc. % to hit in hand to hand combat.", 35),
      XSTR_SLOT("", 34) },   // 0x0
    { XSTR_SLOT("protection", 36), 0, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Take less damage from physical attacks.", 37),
      XSTR_SLOT("", 34) },   // 0x1
    { XSTR_SLOT("speed", 38), 0, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("The speed at which you attack.", 39),
      XSTR_SLOT("", 34) },   // 0x2
    { XSTR_SLOT("sword weapons", 40), 0, 0, 0.0f, -1, 4, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Inc. % to hit with swords and daggers.", 41),
      XSTR_SLOT("", 34) },   // 0x3
    { XSTR_SLOT("axe weapons", 42), 0, 0, 0.0f, -1, 4, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Inc. % to hit with axes and polearms.", 43),
      XSTR_SLOT("", 34) },   // 0x4
    { XSTR_SLOT("blunt weapons", 44), 0, 0, 0.0f, -1, 4, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Inc. % to hit with maces and hammers.", 45),
      XSTR_SLOT("", 34) },   // 0x5
    { XSTR_SLOT("staff weapons", 46), 0, 0, 0.0f, -1, 4, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Inc. % to hit with staves.", 47),
      XSTR_SLOT("", 34) },   // 0x6
    { XSTR_SLOT("bow weapons", 48), 0, 0, 0.0f, -1, 4, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Inc. % to hit with bows and crossbows.", 49),
      XSTR_SLOT("", 34) },   // 0x7
    { XSTR_SLOT("heavy arms", 50), 0, 0, 0.0f, -1, 4, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Can equip heavier weapons and armor.", 51),
      XSTR_SLOT("", 34) },   // 0x8
    { XSTR_SLOT("parry", 52), 0, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Inc. chance to deflect incoming blows.", 53),
      XSTR_SLOT("", 34) },   // 0x9
    { XSTR_SLOT("counter attack", 54), 0, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Inc. chance to get free hit after parry.", 55),
      XSTR_SLOT("", 34) },   // 0xa
    { XSTR_SLOT("dodge", 56), 0, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Inc. chance to dodge attacks from all sides.", 57),
      XSTR_SLOT("", 34) },   // 0xb
    { XSTR_SLOT("critical hit", 58), 0, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Inc. chance of doing double damage.", 59),
      XSTR_SLOT("", 34) },   // 0xc
    { XSTR_SLOT("double attack", 60), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Inc. chance of getting chain attacks.", 61),
      XSTR_SLOT("", 34) },   // 0xd
    { XSTR_SLOT("push", 62), 1, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("(Active) Inc. chance chain attack for next attack.", 63),
      XSTR_SLOT("Knocks target off balance.  Easier to get chains.", 64) },   // 0xe
    { XSTR_SLOT("kick", 65), 2, 0, 0.0f, 6, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("(Active) Inc. % to hit with blunt melee attack.", 66),
      XSTR_SLOT("High damage blunt attack.", 67) },   // 0xf
    { XSTR_SLOT("trip", 68), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("(Active) Inc. % to knock enemy to ground.", 69),
      XSTR_SLOT("Knocks enemy to the ground for short duration.", 70) },   // 0x10
    { XSTR_SLOT("aimed attack", 71), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("(Active) Inc. % chance causing status effect.", 72),
      XSTR_SLOT("Causes status effects (Blind, Cripple, or Slow)", 73) },   // 0x11
    { XSTR_SLOT("backstab", 74), 5, 0, 0.0f, -1, 0, 0x45,
      XSTR_SLOT("", 34), 0, -1, -1,
      XSTR_SLOT("(Active) Inc. & to hit and damage of backstab.", 75),
      XSTR_SLOT("Deals massive damage to target from behind.", 76) },   // 0x12
    { XSTR_SLOT("assess", 77), 5, 0, 0.0f, -1, 3, 0x46,
      XSTR_SLOT("", 34), 0, -1, -1,
      XSTR_SLOT("(Active) Inc. ability to assess stronger foes.", 78),
      XSTR_SLOT("Gives information about target.", 79) },   // 0x13
    { XSTR_SLOT("appraise", 80), 5, 0, 0.0f, -1, 3, -1,
      XSTR_SLOT("", 34), 0, -1, -1,
      XSTR_SLOT("(Active) Inc. ability to identify better items.", 81),
      XSTR_SLOT("Identifies items.", 82) },   // 0x14
    { XSTR_SLOT("pick lock", 83), 5, 0, 0.0f, -1, 1, -1,
      XSTR_SLOT("", 34), 0, -1, -1,
      XSTR_SLOT("(Active) Pick locks of higher difficulty.", 84),
      XSTR_SLOT("Pick locked doors and chests.", 85) },   // 0x15
    { XSTR_SLOT("sneak", 86), 5, 10, 0.0f, -1, 1, -1,
      XSTR_SLOT("", 34), 0, -1, -1,
      XSTR_SLOT("(Active) Inc. duration of sneak.", 87),
      XSTR_SLOT("Move around without being heard.", 88) },   // 0x16
    { XSTR_SLOT("hide", 89), 3, 30, 0.0f, -1, 1, -1,
      XSTR_SLOT("", 34), 0, -1, -1,
      XSTR_SLOT("(Active) Dec. recycle time on hide.", 90),
      XSTR_SLOT("Prevent all detection by enemies.", 91) },   // 0x17
    { XSTR_SLOT("forge", 92), 5, 0, 0.0f, -1, 2, -1,
      XSTR_SLOT("", 34), 0, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x18
    { XSTR_SLOT("mine", 93), 5, 0, 0.0f, -1, 2, -1,
      XSTR_SLOT("", 34), 0, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x19
    { XSTR_SLOT("enchant", 94), 5, 0, 0.0f, -1, 2, -1,
      XSTR_SLOT("", 34), 0, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x1a
    { XSTR_SLOT("summon", 95), 0, 30, 0.0f, -1, 2, -1,
      XSTR_SLOT("", 34), 0, -1, -1,
      XSTR_SLOT("Speed at which summons gain power.", 96),
      XSTR_SLOT("Summons a powerful monster to aid you.", 97) },   // 0x1b
    { XSTR_SLOT("heal", 98), 0, 0, 0.0f, -1, 5, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Gain access to higher level heal spells.", 99),
      XSTR_SLOT("", 34) },   // 0x1c
    { XSTR_SLOT("dark", 100), 0, 0, 0.0f, -1, 5, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Gain access to higher level dark spells.", 101),
      XSTR_SLOT("", 34) },   // 0x1d
    { XSTR_SLOT("energy", 102), 0, 0, 0.0f, -1, 5, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Gain access to higher level energy spells.", 103),
      XSTR_SLOT("", 34) },   // 0x1e
    { XSTR_SLOT("holy", 104), 0, 0, 0.0f, -1, 5, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Gain access to higher level holy spells.", 105),
      XSTR_SLOT("", 34) },   // 0x1f
    { XSTR_SLOT("fire", 106), 0, 0, 0.0f, -1, 5, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Gain access to higher level fire spells.", 107),
      XSTR_SLOT("", 34) },   // 0x20
    { XSTR_SLOT("ice", 108), 0, 0, 0.0f, -1, 5, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Gain access to higher level ice spells.", 109),
      XSTR_SLOT("", 34) },   // 0x21
    { XSTR_SLOT("divination", 110), 0, 0, 0.0f, -1, 5, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x22
    { XSTR_SLOT("magic resist", 111), 0, 0, 0.0f, -1, 5, -1,
      XSTR_SLOT("", 34), 1, -1, -1,
      XSTR_SLOT("Inc. resistance to all types of magic.", 112),
      XSTR_SLOT("", 34) },   // 0x23
    { XSTR_SLOT("summon golem", 113), 7, 0, 0.1f, -1, 2, 0x26,
      XSTR_SLOT("Golem", 114), 4, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x24
    { XSTR_SLOT("summon red minotaur", 115), 15, 0, 0.2f, -1, 2, 0x27,
      XSTR_SLOT("Red Minotaur", 116), 4, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x25
    { XSTR_SLOT("summon Blade of Urath", 117), 25, 0, 0.5f, -1, 2, 0x28,
      XSTR_SLOT("Blade of Urath", 118), 4, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x26
    { XSTR_SLOT("summon blue imp", 119), 15, 0, 0.3f, -1, 2, 0x29,
      XSTR_SLOT("Blue Imp", 120), 4, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x27
    { XSTR_SLOT("summon blackfire elemental", 121), 10, 0, 0.2f, -1, 2, 0x2a,
      XSTR_SLOT("Blackfire Elemental", 122), 4, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x28
    { XSTR_SLOT("summon blood elemental", 123), 30, 0, 0.4f, -1, 2, 0x2b,
      XSTR_SLOT("Blood Elemental", 124), 4, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x29
    { XSTR_SLOT("summon poison elemental", 125), 20, 0, 0.3f, -1, 2, 0x2c,
      XSTR_SLOT("Poison Elemental", 126), 4, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x2a
    { XSTR_SLOT("summon mist elemental", 127), 10, 0, 0.3f, -1, 2, 0x2d,
      XSTR_SLOT("Mist Elemental", 128), 4, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x2b
    { XSTR_SLOT("summon wraith", 129), 30, 0, 0.5f, -1, 2, 0x2e,
      XSTR_SLOT("Wraith", 130), 4, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x2c
    { XSTR_SLOT("summon jade golem", 131), 14, 0, 0.6f, -1, 2, 0x2f,
      XSTR_SLOT("Jade Golem", 132), 4, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x2d
    { XSTR_SLOT("summon laharah", 133), 50, 0, 0.8f, -1, 2, 0x30,
      XSTR_SLOT("Spirit of Laharah", 134), 4, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x2e
    { XSTR_SLOT("summon samurai", 135), 30, 0, 0.5f, -1, 2, 0x31,
      XSTR_SLOT("Celestial Samurai", 136), 4, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x2f
    { XSTR_SLOT("summon jade dragon", 137), 30, 0, 0.8f, -1, 2, 0x32,
      XSTR_SLOT("Jade Dragon", 138), 20, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x30
    { XSTR_SLOT("summon water dragon", 139), 50, 0, 0.0f, -1, 2, 0x33,
      XSTR_SLOT("Water Dragon", 140), 20, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x31
    { XSTR_SLOT("summon wind dragon", 141), 30, 0, 0.5f, -1, 2, 0x34,
      XSTR_SLOT("Four Winds Dragon", 142), 20, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x32
    { XSTR_SLOT("summon forest dragon", 143), 30, 0, 0.5f, -1, 2, 0x35,
      XSTR_SLOT("Forest Dragon", 144), 20, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x33
    { XSTR_SLOT("banish summon", 145), 0, 0, 0.0f, -1, 2, -1,
      XSTR_SLOT("", 34), 0, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("Banishes creature under your control.", 146) },   // 0x34
    { XSTR_SLOT("aimed attack eyes", 147), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("Eyes", 148), 8, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x35
    { XSTR_SLOT("aimed attack arms", 149), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("Arms", 150), 8, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x36
    { XSTR_SLOT("aimed attack legs", 151), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("Legs", 152), 8, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x37
    { XSTR_SLOT("blindness stab", 153), 3, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("Eyes", 148), 2, -1, -1,
      XSTR_SLOT("Blinds the target.  Low damage.", 154),
      XSTR_SLOT("", 34) },   // 0x38
    { XSTR_SLOT("arm slice", 155), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("Arms", 150), 2, -1, -1,
      XSTR_SLOT("Cripples target.  Medium damage.", 156),
      XSTR_SLOT("", 34) },   // 0x39
    { XSTR_SLOT("leg swipe", 157), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("Legs", 152), 2, -1, -1,
      XSTR_SLOT("Slows target.  Medium damage.", 158),
      XSTR_SLOT("", 34) },   // 0x3a
    { XSTR_SLOT("Desperation", 159), 1, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("+Damage with amount of damage taken.", 160),
      XSTR_SLOT("", 34) },   // 0x3b
    { XSTR_SLOT("stamina burst", 161), 3, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("+Damage with the amount of AP left.", 162),
      XSTR_SLOT("", 34) },   // 0x3c
    { XSTR_SLOT("added blow", 163), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Standard secondary attack.", 164),
      XSTR_SLOT("", 34) },   // 0x3d
    { XSTR_SLOT("life leech", 165), 3, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Does damage, recovers hit points.", 166),
      XSTR_SLOT("", 34) },   // 0x3e
    { XSTR_SLOT("mind drain", 167), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Target loses APs, recovers APs.", 168),
      XSTR_SLOT("", 34) },   // 0x3f
    { XSTR_SLOT("medevan technique", 169), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x40
    { XSTR_SLOT("icy touch", 170), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Does ice based damage.", 171),
      XSTR_SLOT("", 34) },   // 0x41
    { XSTR_SLOT("burning hands", 172), 3, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Does fire based damage.", 173),
      XSTR_SLOT("", 34) },   // 0x42
    { XSTR_SLOT("shocking grasp", 174), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Does energy based damage.", 175),
      XSTR_SLOT("", 34) },   // 0x43
    { XSTR_SLOT("sacrifice", 176), 1, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Double damage, lose own HP.", 177),
      XSTR_SLOT("", 34) },   // 0x44
    { XSTR_SLOT("confusion", 178), 1, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Removes APs from target.", 179),
      XSTR_SLOT("", 34) },   // 0x45
    { XSTR_SLOT("silence", 180), 3, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Target cannot cast spells.", 181),
      XSTR_SLOT("", 34) },   // 0x46
    { XSTR_SLOT("axespin", 182), 3, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Damages nearby targets. Axe only.", 183),
      XSTR_SLOT("", 34) },   // 0x47
    { XSTR_SLOT("earthshaker", 184), 8, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Knockdown nearby targets.  Blunt only.", 185),
      XSTR_SLOT("", 34) },   // 0x48
    { XSTR_SLOT("mend", 186), 4, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("User recovers some hitpoints.", 187),
      XSTR_SLOT("", 34) },   // 0x49
    { XSTR_SLOT("steelskin", 188), 5, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("User takes less damage.", 189),
      XSTR_SLOT("", 34) },   // 0x4a
    { XSTR_SLOT("overpower", 190), 6, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("User does extra damage.", 191),
      XSTR_SLOT("", 34) },   // 0x4b
    { XSTR_SLOT("bless", 192), 4, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Party takes less physical damage.", 193),
      XSTR_SLOT("", 34) },   // 0x4c
    { XSTR_SLOT("revitalize", 194), 6, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Party regains HP.", 195),
      XSTR_SLOT("", 34) },   // 0x4d
    { XSTR_SLOT("prayer", 196), 4, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Party takes less magical damage.", 197),
      XSTR_SLOT("", 34) },   // 0x4e
    { XSTR_SLOT("mug", 198), 2, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("Steals gold. Does medium damage.", 199),
      XSTR_SLOT("", 34) },   // 0x4f
    { XSTR_SLOT("serpent whip", 200), 0, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x50
    { XSTR_SLOT("serpent whip trip", 201), 0, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x51
    { XSTR_SLOT("serpent whip push", 202), 0, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x52
    { XSTR_SLOT("serpent whip multi", 203), 0, 0, 0.0f, -1, 0, -1,
      XSTR_SLOT("", 34), 2, -1, -1,
      XSTR_SLOT("", 34),
      XSTR_SLOT("", 34) },   // 0x53
};

// 0x4447f0 init_strptrs_8b5098_thunk (jmp). Confidence: exact.
// 0x444800 init_strptrs_8b5098: six slot stores. Confidence: exact.
char **resistance_level_names[6] = {                  // 0x8b5098 (assess_format_info)
    XSTR_SLOT("weak", 204), XSTR_SLOT("vulnerable", 205), XSTR_SLOT("standard", 206),
    XSTR_SLOT("strong", 207), XSTR_SLOT("invulnerable", 208), XSTR_SLOT("strengthen", 209),
};

// The 15 pools of the unit: Mempool has an inline empty default constructor (D1801), so each definition
// leaves an empty dynamic initialiser (thunk + bare ret). Which initialiser belongs to which pool is not
// visible; they are listed in address order. Confidence: unsure (the order) for each pair.
// 0x444840 init_empty_444850_thunk (jmp). Confidence: exact.
// 0x444850 init_empty_444850 (bare ret). Confidence: unsure (which pool).
Mempool jekhar_char_mempool;                  // 0x65e978
// 0x444860 init_empty_444870_thunk (jmp). Confidence: exact.
// 0x444870 init_empty_444870 (bare ret). Confidence: unsure (which pool).
Mempool rosalind_model_mempool;               // 0x68e1d8
// 0x444880 init_empty_444890_thunk (jmp). Confidence: exact.
// 0x444890 init_empty_444890 (bare ret). Confidence: unsure (which pool).
Mempool rosalind_anim_mempool;                // 0x6d7e10
// 0x4448a0 init_empty_4448b0_thunk (jmp). Confidence: exact.
// 0x4448b0 init_empty_4448b0 (bare ret). Confidence: unsure (which pool).
Mempool flece_anim_mempool;                   // 0x6d7e30
// 0x4448c0 init_empty_4448d0_thunk (jmp). Confidence: exact.
// 0x4448d0 init_empty_4448d0 (bare ret). Confidence: unsure (which pool).
Mempool rosalind_char_mempool;                // 0x714e60
// 0x4448e0 init_empty_4448f0_thunk (jmp). Confidence: exact.
// 0x4448f0 init_empty_4448f0 (bare ret). Confidence: unsure (which pool).
Mempool player_instance_mempool;              // 0x744680
// 0x444900 init_empty_444910_thunk (jmp). Confidence: exact.
// 0x444910 init_empty_444910 (bare ret). Confidence: unsure (which pool).
Mempool statscreen_mempool;                   // 0x7592a0
// 0x444920 init_empty_444930_thunk (jmp). Confidence: exact.
// 0x444930 init_empty_444930 (bare ret). Confidence: unsure (which pool).
Mempool summon_mempool;                       // 0x7592c0
// 0x444940 init_empty_444950_thunk (jmp). Confidence: exact.
// 0x444950 init_empty_444950 (bare ret). Confidence: unsure (which pool).
Mempool flece_char_mempool;                   // 0x7deee8
// 0x444960 init_empty_444970_thunk (jmp). Confidence: exact.
// 0x444970 init_empty_444970 (bare ret). Confidence: unsure (which pool).
Mempool joseph_char_mempool;                  // 0x7e12c0
// 0x444980 init_empty_444990_thunk (jmp). Confidence: exact.
// 0x444990 init_empty_444990 (bare ret). Confidence: unsure (which pool).
Mempool flece_model_mempool;                  // 0x847fc0
// 0x4449a0 init_empty_4449b0_thunk (jmp). Confidence: exact.
// 0x4449b0 init_empty_4449b0 (bare ret). Confidence: unsure (which pool).
Mempool jekhar_anim_mempool;                  // 0x847fe8
// 0x4449c0 init_empty_4449d0_thunk (jmp). Confidence: exact.
// 0x4449d0 init_empty_4449d0 (bare ret). Confidence: unsure (which pool).
Mempool joseph_model_mempool;                 // 0x889008
// 0x4449e0 init_empty_4449f0_thunk (jmp). Confidence: exact.
// 0x4449f0 init_empty_4449f0 (bare ret). Confidence: unsure (which pool).
Mempool joseph_anim_mempool;                  // 0x889028
// 0x444a00 init_empty_444a10_thunk (jmp). Confidence: exact.
// 0x444a10 init_empty_444a10 (bare ret). Confidence: unsure (which pool).
Mempool jekhar_model_mempool;                 // 0x8b3c48

// 0x444a20 init_ccmd_All_summons_thunk (jmp). Confidence: exact.
// 0x444a30 init_ccmd_All_summons (console_command constructor). Confidence: exact.
console_command ccmd_All_summons("All_summons", "Toggles All_summons", cmd_All_summons);   // 0x68e200

// 0x444a50  Confidence: exact (the console's bool command macro, as cmd_Super_power).
void cmd_All_summons()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            All_summons = true;
        else if (arg_type & ARG_FALSE)
            All_summons = false;
        else if (arg_type & ARG_NONE)
            All_summons ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "All_summons", "All_summons");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "All_summons", All_summons ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x444b10 sinit_stringpool_68e1a8 (call init, jmp register). Confidence: exact.
// 0x444b20 init_stringpool_68e1a8: StringPool(buffer, 0x14c00, false). Confidence: exact.
// 0x444b40 register_dtor_stringpool_68e1a8: atexit. Confidence: exact.
// 0x444b50 dtor_stringpool_68e1a8: jmp ~StringPool. Confidence: exact.
StringPool charinfo_string_pool(charinfo_string_pool_buffer, sizeof(charinfo_string_pool_buffer), false);   // 0x68e1a8
// 0x444b60 sinit_stringpool_68e1c0. Confidence: exact.
// 0x444b70 init_stringpool_68e1c0: StringPool(buffer, 0x1400, false). Confidence: exact.
// 0x444b90 register_dtor_stringpool_68e1c0. Confidence: exact.
// 0x444ba0 dtor_stringpool_68e1c0. Confidence: exact.
StringPool charinfo_script_anim_pool(charinfo_script_anim_pool_buffer, sizeof(charinfo_script_anim_pool_buffer), false);   // 0x68e1c0

// ---- functions -----------------------------------------------------------------------------------------

// 0x444bb0  Confidence: exact (VC6 8966 byte match). 1 = the move need not be loaded here: the parry / flinch low moves, the
// non-combat flinch moves, the multiplayer hero cut moves, in towns (and templeint unless full_set) every
// combat list, and on masad / Liangshan / JadeTemple the boss rider moves. The town test is written out
// twice (once per level, as in the binary); `m->category == 4 && m->id == 0x82` repeats the category
// test (the `cmp eax, eax` at 0x444bff). Side effect: on masad (not masad_v2), Liangshan and JadeTemple
// m->anim is set to 1 before the name tests.
int move_skip_on_level(char_info *info, move_info *m, bool full_set)
{
    int c;

    if ((m->category == 4 || m->category == 2) && m->variant == 1)
        return 1;
    if (m->category == 4 && m->variant == 2)
        return 1;
    if (m->category == 2 && m->variant == 3)
        return 1;
    if (m->category == 4 && m->id == 0x81)
        return 1;
    if (m->category == 4 && m->id == 0x82)
        return 1;
    if (m->category == 5)
        return 1;
    if ((game_flags & 2) && species_is_hero(info->species)) {
        if (m->category == 0xe && m->id == 0x92)
            return 1;
        if (m->id == 0x94)
            return 1;
        if (m->id == 0x9c)
            return 1;
        if (m->id == 0x9d)
            return 1;
        if (m->category == 0xa && m->id == 0x23)
            return 1;
        if (m->id == 0x24)
            return 1;
        if (m->id == 0x25 || m->id == 0x26)
            return 1;
        return 0;
    }
    if (level_is_town) {
        c = m->category;
        if (c == 0 || c == 1 || c == 2 || c == 3 || c == 4 || c == 6 || c == 7 || c == 0xa || c == 0xb
            || c == 0xc || c == 0xd || c == 9)
            return 1;
        if (c == 0xe) {
            c = m->id;
            if (c == 0x6f || c == 0x95 || c == 0xa0 || c == 0x27 || c == 0x28 || c == 0xa1 || c == 0xa2)
                return 1;
        }
    }
    if (!_stricmp(level_filename, "masad") && _stricmp(level_script_name, "masad_v2")) {
        m->anim = 1;
        if (!_strnicmp(m->anim_file, "serp_stage1", 11)) {
            if (m->category != 0xe)
                return 1;
        } else if (!_strnicmp(m->anim_file, "serp", 4)) {
            return 1;
        } else if (!_strnicmp(m->anim_file, "ghrd_stage1", 11)) {
            if (m->category != 0xe)
                return 1;
        } else if (!_strnicmp(m->anim_file, "ghrd", 4)) {
            return 1;
        } else if (!_strnicmp(m->anim_file, "tigr_horse_", 11)) {
            if (m->category != 0xe)
                return 1;
        } else if (!_strnicmp(m->anim_file, "tigr", 4)) {
            return 1;
        } else if (!_strnicmp(m->anim_file, "tigr_stage3", 11)) {
            return 1;
        } else if (!_strnicmp(m->anim_file, "phnx_stage1", 11)) {
            if (m->category != 0xe)
                return 1;
        } else if (!_strnicmp(m->anim_file, "phnx", 4)) {
            return 1;
        }
    }
    if (!_stricmp(level_filename, "templeint") && !full_set) {
        c = m->category;
        if (c == 0 || c == 1 || c == 2 || c == 3 || c == 4 || c == 6 || c == 7 || c == 0xa || c == 0xb
            || c == 0xc || c == 0xd || c == 9)
            return 1;
        if (c == 0xe) {
            c = m->id;
            if (c == 0x6f || c == 0x95 || c == 0xa0 || c == 0x27 || c == 0x28 || c == 0xa1 || c == 0xa2)
                return 1;
        }
    }
    if (!_stricmp(level_filename, "Liangshan") || !_stricmp(level_filename, "JadeTemple")) {
        m->anim = 1;
        if (!_strnicmp(m->anim_file, "serp_stage1", 11))
            return 1;
        if (!_strnicmp(m->anim_file, "serp_stage2", 11))
            return 1;
        if (!_strnicmp(m->anim_file, "ghrd_stage1", 11))
            return 1;
        if (!_strnicmp(m->anim_file, "tigr_horse_", 11))
            return 1;
        if (!_strnicmp(m->anim_file, "tigr_stage1", 11))
            return 1;
        if (!_strnicmp(m->anim_file, "tigr_stage2", 11))
            return 1;
        if (!_strnicmp(m->anim_file, "phnx_stage1", 11))
            return 1;
        if (!_strnicmp(m->anim_file, "phnx_stage2", 11))
            return 1;
    }
    return 0;
}

// 0x4450c0  Confidence: exact (index loops: pointer loops compare unsigned, the binary signed; the species
// read through the array after the two stores, D13102). Copies the hero pools onto their static buffers, threads the move free
// list, parses "<species>.tbl" for every species_table entry marked for loading, then collects, for every
// record that is neither a hero nor a boss, the spells of its "cast" moves into charinfo_cast_spells.
// The argument is not read.
void characterinfo_load_party(int path_id)
{
    char filename[0x100];
    const species_info *s;
    move_info *m, *prev;
    char_info *info;
    char *p;
    int i, spell;

    if (charinfo_party_loaded)
        return;
    joseph_char_mempool.Copy(joseph_char_buffer, sizeof(joseph_char_buffer), "Joseph char");
    jekhar_char_mempool.Copy(jekhar_char_buffer, sizeof(jekhar_char_buffer), "Jekhar char");
    flece_char_mempool.Copy(flece_char_buffer, sizeof(flece_char_buffer), "Flece char");
    rosalind_char_mempool.Copy(rosalind_char_buffer, sizeof(rosalind_char_buffer), "Rosalind char");
    joseph_model_mempool.Copy(joseph_model_buffer, sizeof(joseph_model_buffer), "Joseph model");
    jekhar_model_mempool.Copy(jekhar_model_buffer, sizeof(jekhar_model_buffer), "Jekhar model");
    flece_model_mempool.Copy(flece_model_buffer, sizeof(flece_model_buffer), "Flece model");
    rosalind_model_mempool.Copy(rosalind_model_buffer, sizeof(rosalind_model_buffer), "Rosalind model");
    player_instance_mempool.Copy(player_instance_buffer, sizeof(player_instance_buffer), "Player instance");
    num_char_infos = 0;
    prev = NULL;
    for (i = 0; i < MAX_TOTAL_MOVES; i++) {
        move_infos[i].next = prev;
        prev = &move_infos[i];
    }
    move_info_free_list = prev;
    charinfo_cast_spells_used = 0;
    for (i = 0; i < NUM_SPECIES_TABLE; i++) {
        s = &species_table[i];
        if (s->load == 1) {
            sprintf(filename, "%s.tbl", s->name);
            for (p = filename; *p; p++)
                *p = tolower(*p);
            characterinfo_parse_table(filename);
        }
    }
    for (i = 0; i < num_char_infos; i++) {
        info = &char_infos[i];
        info->cast_spells = &charinfo_cast_spells[charinfo_cast_spells_used];
        info->num_cast_spells = 0;
        int sp = char_infos[i].species;
        if (species_is_hero(sp) || species_is_boss(sp))
            continue;
        for (m = (move_info *)info->moves[MOVE_TYPE_CAST]; m; m = m->next) {
            spell = spell_find_by_cast_id(m->id);
            if (spell != -1) {
                info->cast_spells[info->num_cast_spells] = spell;
                info->num_cast_spells++;
                charinfo_cast_spells_used++;
            }
        }
    }
    charinfo_party_loaded = true;
}

// 0x4452f0  Confidence: exact (m-1020c w4, D14305: default_weapons as one dword store, the move link read
// before the category store, info->moves cleared by a loop, the two footstep arrays, 1/30 as 0x3d088888)
// (control flow and stores checked block by block; the stack buffers are
// separate block-scope arrays as in the frame). Notes:
// - One record pointer is taken before the loop: every "$Character:" of the file is parsed into it (the
//   species tables hold one each); "+Alternate character:" copies get their own records. Returns it.
// - The flag lines loop until none matches; each matched flag is followed by an optional ':'.
// - $Aggressiveness:, $Teamwork:, $Conservation: are read and dropped.
// - $Default Weapon: has no bound on the two-entry arrays; a move's +Animation class: bits are ORed
//   into the recycled record without clearing it first; $Trigger: has no bound on the three triggers.
// - Moves with a hurt / turn variation (hurt_variation_names) are unlinked and freed for every record
//   that is not a boss (heroes included); the loop keeps reading m->id after the free.
// - The two 0.01 constants are 0x5818ec (the ability chances) and 0x5818f0 (+Intelligence:, the drops).
char_info *characterinfo_parse_table(const char *filename)
{
    File f;
    TextFile tf;
    char sound_folder_buf[0x104];
    const char *sound_folder;
    char_info *info, *alt;
    move_info *m;
    int i, n, type;

    if (tf.Open(filename, PATH_ANY)) {
        char msg[0x100];
        sprintf(msg, "Couldn't open %s for parsing", filename);
        VSDK_ERROR(CHARACTERINFO_CPP, 0x661, msg);
    }
    tf.Expect("#Character Info");
    info = &char_infos[num_char_infos++];
    info->checksum = tf.Checksum();
    while (!tf.Accept("#End")) {
        tf.Expect("$Character:");
        characterinfo_parse_name(&tf, info);
        info->flags = 0;
        info->anim_class = 0;
        info->portrait = NULL;
        if (tf.Accept("+Portrait Filename:"))
            info->portrait = charinfo_string_pool.AddUniqueQuoted(&tf);
        info->texture_effect = NULL;
        if (tf.Accept("+Texture Effect:"))
            info->texture_effect = charinfo_string_pool.AddUniqueQuoted(&tf);
        for (;;) {
            if (tf.Accept("+No Shadow"))
                info->flags |= CIF_NO_SHADOW;
            else if (tf.Accept("+No draw"))
                info->flags |= CIF_NO_DRAW;
            else if (tf.Accept("+Edged"))
                info->flags |= CIF_EDGED;
            else if (tf.Accept("+Elongated"))
                info->flags |= CIF_ELONGATED;
            else if (tf.Accept("+Notrip"))
                info->flags |= CIF_NO_TRIP;
            else if (tf.Accept("+Nopush"))
                info->flags |= CIF_NO_PUSH;
            else if (tf.Accept("+Nobackstab"))
                info->flags |= CIF_NO_BACKSTAB;
            else if (tf.Accept("+Noaimedattack"))
                info->flags |= CIF_NO_AIMED_ATTACK;
            else if (tf.Accept("+Noblind"))
                info->flags |= CIF_NO_BLIND;
            else if (tf.Accept("+Nosilence"))
                info->flags |= CIF_NO_SILENCE;
            else if (tf.Accept("+UseMonsterDamage"))
                info->flags |= CIF_USE_MONSTER_DAMAGE;
            else if (tf.Accept("+Flying"))
                info->flags |= CIF_FLYING;
            else if (tf.Accept("+ShortDialogueCamera"))
                info->flags |= CIF_SHORT_DIALOGUE_CAMERA;
            else if (tf.Accept("+ReallyShortDialogueCamera"))
                info->flags |= CIF_REALLY_SHORT_DIALOGUE_CAMERA;
            else if (tf.Accept("+TallDialogueCamera"))
                info->flags |= CIF_TALL_DIALOGUE_CAMERA;
            else
                break;
            tf.Accept(":");
        }
        info->vfx = NULL;
        if (tf.Accept("+Vfx:"))
            info->vfx = charinfo_string_pool.AddUniqueQuoted(&tf);
        tf.Expect("$Size:");
        info->size = tf.ParseTypeString(size_names, 4);
        tf.Expect("$Team:");
        info->team = tf.ParseTypeString(team_names, 4);
        tf.Expect("$Max Hit Points:");
        info->max_hp = (short)tf.ParseInt();
        tf.Expect("$Hit Points:");
        info->hp = (short)tf.ParseInt();
        tf.Expect("$Max Ability Points:");
        info->max_ap = (short)tf.ParseInt();
        tf.Expect("$Ability Points:");
        info->ap = (short)tf.ParseInt();
        tf.Expect("$Aggressiveness:");
        tf.ParseFloat();
        tf.Expect("$Teamwork:");
        tf.ParseFloat();
        tf.Expect("$Conservation:");
        tf.ParseFloat();
        info->intelligence = 0.5f;
        if (tf.Accept("+Intelligence:"))
            info->intelligence = tf.ParseFloat() * 0.01f;
        info->xp_gained = 0;
        if (tf.Accept("$Experience Gained:"))
            info->xp_gained = tf.ParseInt();
        info->level = 1;
        if (tf.Accept("$Level:"))
            info->level = tf.ParseInt();
        tf.Expect("$Attack Radius:");
        info->attack_radius = tf.ParseFloat();

        info->num_default_weapons = 0;
        *(int *)info->default_weapons = -1;   // both slots, one dword store (as the binary, 0x445dxx)
        while (tf.Accept("$Default Weapon:")) {
            char name[0x100];
            tf.ParseString(name, 0x100, '"', '"');
            info->default_weapons[info->num_default_weapons] = (short)item_type_find(name);
            if (info->default_weapons[info->num_default_weapons] == -1) {
                char msg[0x100];
                sprintf(msg, "Unable to locate default weapon %s for character %s", name, info->name);
                VSDK_ERROR(CHARACTERINFO_CPP, 0x703, msg);
            }
            if (tf.Accept("made of"))
                info->default_weapon_materials[info->num_default_weapons] = tf.ParseTypeString(material_names, MAX_MATERIALS);
            else
                info->default_weapon_materials[info->num_default_weapons] = 0;
            info->num_default_weapons++;
        }
        if (tf.Accept("$Damage:"))
            info->damage = (ushort)tf.ParseInt();
        if (tf.Accept("+Special ability:")) {
            char buf[0x40];
            int chance;
            tf.ParseString(buf, 0x40, '"', '"');
            info->special_ability = (short)spell_find_by_name(buf);
            chance = tf.ParseInt();
            info->special_arg = 0;
            info->special_chance = chance * 0.01f;
            tf.ReadLine(buf, 0x40);
            if (!is_blank(buf))
                info->special_arg = (short)atoi(buf);
        } else {
            info->special_ability = -1;
        }
        if (tf.Accept("+Reflex ability:")) {
            char buf[0x40];
            int chance;
            tf.ParseString(buf, 0x40, '"', '"');
            info->reflex_ability = (short)spell_find_by_name(buf);
            chance = tf.ParseInt();
            info->reflex_arg = 0;
            info->reflex_chance = chance * 0.01f;
            tf.ReadLine(buf, 0x40);
            if (!is_blank(buf))
                info->reflex_arg = (short)atoi(buf);
        } else {
            info->reflex_ability = -1;
        }
        tf.Expect("$Field of view range:");
        info->fov_range = tf.ParseFloat();
        tf.Expect("$Detection range:");
        info->detection_range = tf.ParseFloat();
        tf.Expect("$Speedup rate:");
        info->speedup_rate = tf.ParseFloat() * 0.5f;
        tf.Expect("$Slowdown rate:");
        info->slowdown_rate = tf.ParseFloat() * 0.5f;
        tf.Expect("$Slow turn rate:");
        info->slow_turn_rate = tf.ParseFloat();
        tf.Expect("$Fast turn rate:");
        info->fast_turn_rate = tf.ParseFloat();
        tf.Expect("$Moving turn rate:");
        info->moving_turn_rate = tf.ParseFloat();
        info->icon_height = 2.0f;
        if (tf.Accept("$Icon Height:"))
            info->icon_height = tf.ParseFloat();

        for (i = 0; i < 0x10; i++) {
            info->_unk168[i] = -1;
            info->prop_tags[i] = NULL;
        }
        info->prop_tag = 0x11;
        for (;;) {
            bool is_default = false;
            int tag;
            if (tf.Accept("$Default Prop Tag:"))
                is_default = true;
            else if (!tf.Accept("$Prop Tag:"))
                break;
            tag = tf.ParseTypeString(prop_tag_names, 0x10);
            tf.Accept("is");
            info->prop_tags[tag] = charinfo_string_pool.AddUniqueQuoted(&tf);
            if (is_default)
                info->prop_tag = tag;
        }
        characterinfo_parse_resistances(&tf, info);

        memset(info->skill, 0, sizeof(info->skill));
        info->skill[SKILL_SPEED] = 10;
        if (tf.Accept("$Up Skill:")) {
            char buf[0x40];
            tf.ParseString(buf, 0x40, '"', '"');
            for (i = 0; i < MAX_SKILLS; i++)
                if (!_stricmp(skill_names[i], buf))
                    break;
            info->up_skill = (ubyte)i;
        } else {
            info->up_skill = SKILL_NONE;
        }
        if (tf.Accept("$Down Skill:")) {
            char buf[0x40];
            tf.ParseString(buf, 0x40, '"', '"');
            for (i = 0; i < MAX_SKILLS; i++)
                if (!_stricmp(skill_names[i], buf))
                    break;
            info->down_skill = (ubyte)i;
        } else {
            info->down_skill = SKILL_NONE;
        }
        if (tf.Accept("$Left Skill:")) {
            char buf[0x40];
            tf.ParseString(buf, 0x40, '"', '"');
            for (i = 0; i < MAX_SKILLS; i++)
                if (!_stricmp(skill_names[i], buf))
                    break;
            info->left_skill = (ubyte)i;
        } else {
            info->left_skill = SKILL_NONE;
        }
        if (tf.Accept("$Right Skill:")) {
            char buf[0x40];
            tf.ParseString(buf, 0x40, '"', '"');
            for (i = 0; i < MAX_SKILLS; i++)
                if (!_stricmp(skill_names[i], buf))
                    break;
            info->right_skill = (ubyte)i;
        } else {
            info->right_skill = SKILL_NONE;
        }
        while (tf.Accept("$Skill:")) {
            char buf[0x40];
            tf.ParseString(buf, 0x40, '"', '"');
            for (n = 0; n < MAX_SKILLS; n++)
                if (!_stricmp(skill_names[n], buf))
                    break;
            info->skill[n] = (ubyte)tf.ParseInt();
        }

        if (tf.Peek("+Footstep sounds:")) {
            info->num_footstep_sets = 0;
            while (tf.Accept("+Footstep sounds:")) {
                char buf[0x100];
                float p1, p2, p3;
                p1 = tf.ParseFloat();
                p2 = tf.ParseFloat();
                p3 = tf.ParseFloat();
                for (i = 0; i < 5; i++) {
                    tf.ParseString(buf, 0x100, '"', '"');
                    info->footstep_sounds_a[info->num_footstep_sets][i] = snd_get_handle(buf, p1, p2, p3, 0, false);
                }
                for (i = 0; i < 5; i++) {
                    tf.ParseString(buf, 0x100, '"', '"');
                    info->footstep_sounds_b[info->num_footstep_sets][i] = snd_get_handle(buf, p1, p2, p3, 0, false);
                }
                info->num_footstep_sets++;
            }
        } else {
            info->num_footstep_sets = 1;
            for (i = 0; i < 5; i++) {
                info->footstep_sounds_a[0][i] = snd_get_handle(default_footstep_sounds_left[i], 5.0f, 0.9f, 1.0f, 0, false);
                info->footstep_sounds_b[0][i] = snd_get_handle(default_footstep_sounds_right[i], 5.0f, 0.9f, 1.0f, 0, false);
            }
        }
        if (tf.Accept("+Skin:"))
            info->skin = tf.ParseTypeString(material_names, MAX_MATERIALS);
        else
            info->skin = 1;

        for (i = 0; i < NUM_MOVE_TYPES; i++)   // a loop: VC6 turns it into the rep stosd, with the lea first
            info->moves[i] = NULL;
        if (tf.Accept("+Allowed weapon classes:")) {
            char line[0x100];
            char *tok;
            tf.ReadLine(line, 0x100);
            for (tok = strtok(line, "\t\""); tok; tok = strtok(NULL, "\t\"")) {
                if (is_blank(tok))
                    continue;
                for (i = 0; i < 9; i++)
                    if (!_stricmp(tok, anim_class_names[i]))
                        break;
                info->anim_class_mask |= 1 << i;
            }
        } else {
            info->anim_class_mask = 0xffffffff;
        }
        {
            char path[0x104];
            if (tf.Accept("+Anim Folder:")) {
                char folder[0x104];
                tf.ParseString(folder, 0x104, '"', '"');
                sprintf(path, "data%cca%c%s", '\\', '\\', folder);
            } else {
                sprintf(path, "data%cca%c%s", '\\', '\\', info->name);
            }
            info->anim_folder = file_add_path(path, ".mvf");
        }
        sound_folder = NULL;
        if (tf.Accept("+Sound Folder:")) {
            tf.ParseString(sound_folder_buf, 0x104, '"', '"');
            sound_folder = sound_folder_buf;
        }

        while (tf.Accept("+Move:")) {
            char buf[0x100];
            m = characterinfo_add_move();
            type = tf.ParseTypeString(move_type_names, NUM_MOVE_TYPES);
            m->next = (move_info *)info->moves[type];
            m->category = type;
            info->moves[type] = m;
            tf.Expect("$Animation:");
            tf.ParseString(buf, 0x100, '"', '"');
            m->anim_file = charinfo_string_pool.AddUnique(buf);
            tf.Accept("+Turning");
            if (tf.Accept("+Animation class:")) {
                do {
                    m->anim_class |= 1 << tf.ParseTypeString(anim_class_names, 9);
                } while (!tf.Peek("$Action:"));
            } else {
                m->anim_class = 1;
            }
            tf.Expect("$Action:");
            m->action = charinfo_string_pool.AddUniqueQuoted(&tf);
            m->anim = -1;
            sound_ref_parse(&tf, info, &m->sound, sound_folder);
            m->effect = NULL;
            if (tf.Accept("$Effect:"))
                m->effect = charinfo_string_pool.AddUniqueQuoted(&tf);
            m->id = 0;
            m->variant = 0;
            for (;;) {
                if (tf.Accept("$Location:"))
                    m->variant = tf.ParseTypeString(location_names, 4);
                else if (tf.Accept("$Variation:"))
                    m->id = tf.ParseTypeString(variation_names, 0xc2);
                else
                    break;
            }
            m->hit_tag = NULL;
            if (tf.Accept("$Contact tag:")) {
                m->hit_tag = charinfo_string_pool.AddUniqueQuoted(&tf);
                if (!tf.Accept("on primary prop"))
                    tf.Accept("on secondary prop");
            }
            for (i = 0; i < 3; i++)
                m->triggers[i].id = 0;
            if (tf.Accept("$Trigger:")) {
                n = 0;
                do {
                    m->triggers[n].id = tf.ParseTypeString(anim_event_names, 0xe);
                    m->triggers[n].time = tf.ParseFloat() * 0.03333333f;   // 0x3d088888, one ulp below 1 / 30
                    n++;
                } while (tf.Accept("$Trigger:"));
            }
            if (!species_is_boss(info->species)) {
                for (i = 0; i < num_hurt_variation_names; i++) {
                    if (!_stricmp(variation_names[m->id], hurt_variation_names[i])) {
                        info->moves[m->category] = m->next;
                        move_info_free(m);
                    }
                }
            }
        }
        if (!charinfo_find_move(info, 0xe, 0x8f, false))
            info->flags |= CIF_NO_TRIP;
        if (!charinfo_find_move(info, 9, 0x91, false))
            info->flags |= CIF_NO_PUSH;

        info->num_drops = 0;
        while (tf.Accept("+Drop:")) {
            char buf[0x100];
            int chance;
            tf.ParseString(buf, 0xff, '"', '"');
            chance = tf.ParseInt();
            if (info->num_drops < 5) {
                info->drop_items[info->num_drops] = item_type_find(buf);
                info->drop_chances[info->num_drops] = chance * 0.01f;
                info->num_drops++;
            }
        }
        info->num_mp_drops = 0;
        while (tf.Accept("+Multiplayer Drop:")) {
            char buf[0x100];
            int chance, level;
            tf.ParseString(buf, 0xff, '"', '"');
            chance = tf.ParseInt();
            level = tf.ParseInt();
            if (info->num_mp_drops < 0x11) {
                info->mp_drop_items[info->num_mp_drops] = item_type_find(buf);
                info->mp_drop_chances[info->num_mp_drops] = chance * 0.01f;
                info->mp_drop_levels[info->num_mp_drops] = level;
                info->num_mp_drops++;
            }
        }
        if (tf.Accept("+Alert:"))
            sound_ref_parse(&tf, info, &info->alert_sound, sound_folder);
        if (tf.Accept("+CharacterLightPos:")) {
            info->has_light = 1;
            info->light_pos = charinfo_string_pool.AddUniqueQuoted(&tf);
            tf.Expect("+LightColor:");
            tf.ParseFloatList(info->light_color, 3);
            tf.Expect("+LightAtten:");
            info->light_atten = tf.ParseFloat();
        }
        info->alt_textures = (alt_texture_set *)alt_textures_find(info->name, &info->alt_index);
        info->alt_parent = NULL;
        while (tf.Accept("+Alternate character:")) {
            if (num_char_infos == MAX_CHARACTER_INFO)
                VSDK_ERROR(CHARACTERINFO_CPP, 0x923, "Character_info array is full.  Need to increase MAX_CHARACTER_INFO.");
            alt = &char_infos[num_char_infos++];
            *alt = *info;
            characterinfo_parse_name(&tf, alt);
            characterinfo_parse_resistances(&tf, alt);
            alt->alt_index = 1;
            alt->alt_textures = (alt_texture_set *)alt_textures_find_entry(info->name, alt->name);
            alt->alt_parent = info;
        }
    }
    tf.Close();
    return info;
}

// 0x446740  Confidence: exact.
move_info *characterinfo_add_move()
{
    move_info *m;

    if (!move_info_free_list)
        VSDK_ERROR(CHARACTERINFO_CPP, 0x57e, "Move_info array is full.  Need to increase MAX_TOTAL_MOVES.");
    m = move_info_free_list;
    move_info_free_list = m->next;
    return m;
}

// 0x446770  Confidence: exact.
void move_info_free(move_info *m)
{
    m->next = move_info_free_list;
    move_info_free_list = m;
}

// 0x446790  Confidence: exact. "$Sound:" "name" vol p2 p3: a name without a backslash is looked for in
// the sound folder (or a folder named after the character); the volume is scaled to 0..255.
void sound_ref_parse(TextFile *tf, char_info *info, sound_ref *ref, const char *folder)
{
    char name[0x80];
    char path[0x100];

    ref->name = NULL;
    ref->handle = -1;
    if (!tf->Accept("$Sound:"))
        return;
    tf->ParseString(name, 0x80, '"', '"');
    if (!strchr(name, '\\')) {
        if (!folder)
            sprintf(path, "%s%c%s", info->name, '\\', name);
        else
            sprintf(path, "%s%c%s", folder, '\\', name);
        ref->name = charinfo_string_pool.AddUnique(path);
    } else {
        ref->name = charinfo_string_pool.AddUnique(name);
    }
    ref->params[0] = (ubyte)(tf->ParseFloat() * 255.0f);
    ref->params[1] = (ubyte)tf->ParseFloat();
    ref->params[2] = (ubyte)tf->ParseFloat();
}

// 0x446890  Confidence: exact.
void characterinfo_parse_name(TextFile *tf, char_info *info)
{
    int i;

    info->name = charinfo_string_pool.AddUniqueQuoted(tf);
    for (i = 0; i < NUM_SPECIES_TABLE; i++) {
        if (!_stricmp(info->name, species_table[i].name)) {
            info->species = species_table[i].species;
            break;
        }
    }
    if (i == NUM_SPECIES_TABLE) {
        char msg[0x100];
        sprintf(msg, "Character type %s doesn't have a species type. Please add one to CharacterInfo.cpp.\n", info->name);
        VSDK_ERROR(CHARACTERINFO_CPP, 0x60b, msg);
    }
}

// 0x446930  Confidence: exact.
void characterinfo_parse_resistances(TextFile *tf, char_info *info)
{
    int type, i;

    for (i = 0; i < 7; i++)
        info->resistance.level[i] = 2;
    if (!tf->Accept("+Resistances:"))
        return;
    while (!tf->Peek("#Resistances End")) {
        type = tf->ParseTypeString(damage_type_names, 7);
        info->resistance.level[type] = tf->ParseTypeString(resistance_level_type_names, 6);
    }
    tf->Expect("#Resistances End");
}

// 0x4469c0  Confidence: likely (the order of the identity-matrix stores; the length as vector3::len,
// whose operand order differs: z*z + y*y + x*x here). Once per record: the prop tag bones, the attack
// point tags and, for +Elongated records, the height of the $head bone.
void living_entity_setup_head(char_info *info, character_instance *ci)
{
    ca_character *ca = ci->character;
    int i, tag;

    if (info->flags & CIF_HEAD_SET_UP)
        return;
    for (i = 0; i < 0x10; i++)
        if (info->_unk168[i] == -1 && info->prop_tags[i])
            info->_unk168[i] = (short)ca->find_mesh_tag(info->prop_tags[i]);
    info->num_target_points = 0;
    for (i = 0; i < 8; i++) {
        tag = ca->find_mesh_tag(attack_point_names[i]);
        if (tag != -1) {
            info->target_points[info->num_target_points] = (short)tag;
            info->num_target_points++;
        }
    }
    if (info->flags & CIF_ELONGATED) {
        info->head_height = 0.0f;
        tag = ca->find_mesh_tag("$head");
        info->head_bone = tag;
        if (tag != -1) {
            vector3 pos(0.0f, 0.0f, 0.0f);
            matrix orient(vector3(1.0f, 0.0f, 0.0f), vector3(0.0f, 1.0f, 0.0f), vector3(0.0f, 0.0f, 1.0f));
            matrix out_orient;
            vector3 out_pos;
            ci->get_bone_world_transform(tag, &orient, &pos, &out_orient, &out_pos);
            info->head_height = out_pos.len() + 0.1f;
        }
    }
    info->flags |= CIF_HEAD_SET_UP;
}

// 0x446b80  Confidence: exact.
int charinfo_find_by_name(const char *name)
{
    int i;

    for (i = 0; i < num_char_infos; i++)
        if (!_stricmp(name, char_infos[i].name))
            return i;
    return -1;
}

// 0x446bd0  Confidence: exact.
int charinfo_find_by_species(int species)
{
    int i;

    for (i = 0; i < num_char_infos; i++)
        if (species == char_infos[i].species)
            return i;
    return -1;
}

// 0x446c00  Confidence: exact.
int summon_find_charinfo(int spell)
{
    int i;

    for (i = 0; i < MAX_SKILLS; i++)
        if ((skill_table[i].flags & 4) && skill_table[i].spell == spell)
            return charinfo_find_by_name(summon_skill_creature_name(i));
    return -1;
}

// 0x446c40  Confidence: exact.
const char *summon_skill_creature_name(int skill)
{
    if (skill >= SKILL_SUMMON_GOLEM && skill <= SKILL_SUMMON_FOREST_DRAGON)
        return summon_creature_names[skill - SKILL_SUMMON_GOLEM];
    return "";
}

// 0x446c60  Confidence: exact. The move of any list that plays the anim, not counting the low parry /
// flinch moves.
move_info *charinfo_find_move_any(char_info *info, int anim, bool loaded_only)
{
    move_info *m;
    int i;

    for (i = 0; i < NUM_MOVE_TYPES; i++) {
        for (m = (move_info *)info->moves[i]; m; m = m->next) {
            if ((m->category == 2 || m->category == 4) && m->variant == 1)
                continue;
            if (m->anim == anim)
                return m;
        }
    }
    return NULL;
}

// 0x446cb0  Confidence: exact.
move_info *charinfo_find_move(char_info *info, int category, int id, bool loaded_only)
{
    move_info *m;

    for (m = (move_info *)info->moves[category]; m; m = m->next) {
        if (m->id != id)
            continue;
        if (!species_is_hero(info->species))
            return m;
        if (m->anim != -1 && (m->anim_class & info->anim_class))
            return m;
        if (!loaded_only)
            return m;
    }
    return NULL;
}

// 0x446d10  Confidence: exact (charinfo_find_move on +0x18).
move_info *charinfo_find_move_alt(char_info *info, int category, int variant, bool loaded_only)
{
    move_info *m;

    for (m = (move_info *)info->moves[category]; m; m = m->next) {
        if (m->variant != variant)
            continue;
        if (!species_is_hero(info->species))
            return m;
        if (m->anim != -1 && (m->anim_class & info->anim_class))
            return m;
        if (!loaded_only)
            return m;
    }
    return NULL;
}

// 0x446d70  Confidence: exact. Moves with an id 1..5 other than `exclude` usable with the weapon class
// (multiplayer slot copies use the class given, the others the loaded classes).
int charinfo_collect_moves(move_info **out, int max, char_info *info, int category, int exclude, int anim_class)
{
    move_info *m;
    unsigned int mask;
    int n;

    if ((game_flags & 2) && multi_charinfo_slot(info))
        mask = 1 << anim_class;
    else
        mask = info->anim_class;
    for (m = (move_info *)info->moves[category], n = 0; m; m = m->next) {
        if (m->id != exclude && m->id >= 1 && m->id <= 5 && (m->anim_class & mask)) {
            *out++ = m;
            n++;
        }
    }
    return n;
}

// 0x446de0  Confidence: exact.
multi_charinfo_slot_t *multi_charinfo_slot(char_info *info)
{
    int i;

    for (i = 0; i < 4; i++)
        if (&multi_charinfo_slots[i].info == info)
            return &multi_charinfo_slots[i];
    return NULL;
}

// 0x446e20  Confidence: exact.
float move_get_trigger_time(move_info *m, int trigger)
{
    int i;

    if (!m)
        return 0.0f;
    for (i = 0; i < 3; i++)
        if (m->triggers[i].id == trigger)
            return m->triggers[i].time;
    VSDK_ERROR(CHARACTERINFO_CPP, 0xd61, "Trigger not found.");
}

// 0x446e70  Confidence: exact (VC6 byte match) (shr / and of the flags byte: a shift form, or a bit field).
bool skill_is_summon(int skill)
{
    if ((skill_table[skill].flags >> 2) & 1)
        return true;
    return false;
}

// 0x446e90  Confidence: exact (m-1020c w4, D14304: the ring checks are one else-if chain ending in a
// single `return false`, so a failed ring test jumps there instead of 8966's setne) (switch on the summon skill, jump table 0x44739c; the case bodies are laid
// out in level order, so the cases are written in that order). In multiplayer only Joseph summons, by
// level; in single player by the party's rings (and the ring's savegame xp for the second summons).
bool character_can_use_summon(int skill, Character *c)
{
    if (All_summons)
        return true;
    if (game_flags & 2) {
        if (c->info->species != SPECIES_JOSEPH)
            return false;
        switch (skill) {
        case SKILL_SUMMON_BLACKFIRE_ELEMENTAL: return c->level >= 1;
        case SKILL_SUMMON_MIST_ELEMENTAL:      return c->level >= 4;
        case SKILL_SUMMON_BLUE_IMP:            return c->level >= 8;
        case SKILL_SUMMON_RED_MINOTAUR:        return c->level >= 12;
        case SKILL_SUMMON_GOLEM:               return c->level >= 16;
        case SKILL_SUMMON_SAMURAI:             return c->level >= 20;
        case SKILL_SUMMON_POISON_ELEMENTAL:    return c->level >= 24;
        case SKILL_SUMMON_JADE_GOLEM:          return c->level >= 28;
        case SKILL_SUMMON_BLADE_OF_URATH:      return c->level >= 32;
        case SKILL_SUMMON_BLOOD_ELEMENTAL:     return c->level >= 36;
        case SKILL_SUMMON_WRAITH:              return c->level >= 40;
        case SKILL_SUMMON_LAHARAH:             return c->level >= 44;
        }
        return false;
    }
    if (!_stricmp(level_filename, "templeint2")) {
        if (!_stricmp(summon_skill_creature_name(skill), "Blackfire Elemental"))
            return false;
        if (!_stricmp(summon_skill_creature_name(skill), "Poison Elemental"))
            return false;
        if (!_stricmp(summon_skill_creature_name(skill), "Blade of Urath"))
            return false;
        if (!_stricmp(summon_skill_creature_name(skill), "Blood Elemental"))
            return false;
    }
    if (!_stricmp(summon_skill_creature_name(skill), "Blackfire Elemental")) {
        if (party_count_item_by_name("Ring of Darkness"))
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Mist Elemental")) {
        if (party_count_item_by_name("Ring of Light"))
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Red Minotaur")) {
        if (party_count_item_by_name("Ring of Fire"))
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Golem")) {
        if (party_count_item_by_name("Ring of Stone"))
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Blue Imp")) {
        if (savegame_state.get_flag(0x10, "got_ring_of_water"))
            return true;
        if (party_count_item_by_name("Ring of Water"))
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Poison Elemental")) {
        if (savegame_state.get_flag(0x14, "got_ring_of_forest"))
            return true;
        if (party_count_item_by_name("Ring of Forest"))
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Jade Golem")) {
        if (savegame_state.get_flag(0x22, "got_ring_of_jade"))
            return true;
        if (party_count_item_by_name("Ring of Jade"))
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Celestial Samurai")) {
        if (savegame_state.get_flag(0x1c, "got_ring_of_wind"))
            return true;
        if (party_count_item_by_name("Ring of Four Winds"))
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Spirit of Laharah")) {
        if (savegame_state.get_flag(0x22, "got_laharah_spirit"))
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Wraith")) {
        if (party_count_item_by_name("Ring of Light") && savegame_state.get_int(4, "light_ring_xp") > 60000)
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Blood Elemental")) {
        if (party_count_item_by_name("Ring of Fire") && savegame_state.get_int(4, "fire_ring_xp") > 60000)
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Blade of Urath")) {
        if (party_count_item_by_name("Ring of Stone") && savegame_state.get_int(4, "stone_ring_xp") > 60000)
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Water Dragon")) {
        if (party_count_item_by_name("Ring of Water") && savegame_state.get_int(4, "water_ring_xp") > 60000)
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Four Winds Dragon")) {
        if (party_count_item_by_name("Ring of Four Winds") && savegame_state.get_int(4, "wind_ring_xp") > 60000)
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Forest Dragon")) {
        if (party_count_item_by_name("Ring of Forest") && savegame_state.get_int(4, "forest_ring_xp") > 60000)
            return true;
    } else if (!_stricmp(summon_skill_creature_name(skill), "Jade Dragon")) {
        if (party_count_item_by_name("Ring of Jade") && savegame_state.get_int(4, "jade_ring_xp") > 60000)
            return true;
    }
    return false;
}

// 0x4473d0  Confidence: exact (VC6 byte match) (as skill_is_summon).
bool skill_has_flag8(int skill)
{
    if ((skill_table[skill].flags >> 3) & 1)
        return true;
    return false;
}

// 0x4473f0  Confidence: exact.
bool skill_is_combat_special(int skill)
{
    if (skill >= SKILL_CRITICAL_HIT && skill <= SKILL_AIMED_ATTACK)
        return true;
    return false;
}

// 0x447410  Confidence: exact (VC6 byte match) (the two species switches, jump tables 0x44775c / 0x44776c, are written
// with the cases in the order of their bodies: Joseph, Jekhar, Rosalind, Flece). Loads the anims of a
// record for one more weapon class (anim_class bit): non-heroes go through level_load_preload, heroes
// into their "<hero> Anims" pool, then every character using the record is re-initialised.
void characterinfo_load_party_anims(char_info *info, int anim_class, bool load_sounds, int summon)
{
    ca_character *ca;
    unsigned int mask;
    bool full_set;
    Mempool *pool;
    move_info *m;
    Character *c;
    Player *p;
    int i;

    mask = 1 << anim_class;
    full_set = false;
    if (!info->alt_parent)
        ca = ca_find_character(info->name);
    else
        ca = ca_find_character(info->alt_parent->name);
    if (mask & info->anim_class)
        return;
    charinfo_unload_anims(info);
    if (!species_is_hero(info->species)) {
        level_load_preload(info, load_sounds, summon, NULL);
        return;
    }
    if (!(game_flags & 2) && !_stricmp(level_filename, "masad") && _stricmp(level_script_name, "masad_v2")
        && info->species != SPECIES_JOSEPH) {
        printf("Trying to load animations for a non joseph character in masad!\n");
        return;
    }
    switch (info->species) {
    case SPECIES_JOSEPH:
        pool = &joseph_anim_mempool;
        pool->Copy((char *)joseph_anim_buffer, joseph_anim_buffer_size, "Joseph Anims");
        full_set = true;
        break;
    case SPECIES_JEKHAR:
        pool = &jekhar_anim_mempool;
        pool->Copy((char *)jekhar_anim_buffer, jekhar_anim_buffer_size, "Jekhar Anims");
        full_set = false;
        break;
    case SPECIES_ROSALIND:
        pool = &rosalind_anim_mempool;
        pool->Copy((char *)rosalind_anim_buffer, rosalind_anim_buffer_size, "Rosalind Anims");
        full_set = false;
        break;
    case SPECIES_FLECE:
        pool = &flece_anim_mempool;
        pool->Copy((char *)flece_anim_buffer, flece_anim_buffer_size, "Flece Anims");
        full_set = true;
        break;
    default:
        printf("What the heehaw -- we have %s in the level\n", info->name);
        pool = &level_mempool;
        break;
    }
    set_current_mempool(pool);
    if (game_flags & 2) {
        full_set = true;
        level_load_preload(info, load_sounds, summon, pool);
    } else {
        mask |= 1;
        for (i = 0; i < NUM_MOVE_TYPES; i++)
            for (m = (move_info *)info->moves[i]; m; m = m->next)
                if (m->id != 0x9e && m->id != 0x9f && (m->anim_class & mask))
                    charinfo_load_move_anim(info, ca, m, load_sounds, info->anim_folder, full_set);
    }
    set_current_mempool(&player_instance_mempool);
    switch (info->species) {
    case SPECIES_JOSEPH:
        sprintf(string_buffer, "Usage in Joseph's pool is %d\n", joseph_anim_mempool.offset);
        break;
    case SPECIES_JEKHAR:
        sprintf(string_buffer, "Usage in Jekhar's pool is %d\n", jekhar_anim_mempool.offset);
        break;
    case SPECIES_ROSALIND:
        sprintf(string_buffer, "Usage in Rosalind's pool is %d\n", rosalind_anim_mempool.offset);
        break;
    case SPECIES_FLECE:
        sprintf(string_buffer, "Usage in Flece's pool is %d\n", flece_anim_mempool.offset);
        break;
    }
    info->anim_class = mask;
    charinfo_remap_town_moves(info, full_set);
    for (c = living_list.live_next; c != &living_list; c = c->live_next)
        if (c->info == info)
            character_reinit_anims(c, anim_class);
    for (c = living_list_inactive.live_next; c != &living_list_inactive; c = c->live_next)
        if (c->info == info)
            character_reinit_anims(c, anim_class);
    for (p = player_list_other.next; p != &player_list_other; p = p->next) {
        c = living_from_handle(p->handle);
        if (c && c->info == info)
            character_reinit_anims(c, anim_class);
    }
    set_current_mempool(NULL);
}

// 0x447780  Confidence: exact. Adds the move's skeleton (state skeletons for "state" moves) unless
// move_skip_on_level says skip, loads its sound (not for the ids 0x92-0x95) and the triggers of state
// moves (frames: the time * 30.000002, constant 0x5818f4).
void charinfo_load_move_anim(char_info *info, ca_character *ca, move_info *m, bool load_sounds, int anim_folder,
                             bool full_set)
{
    int i;

    if (move_skip_on_level(info, m, full_set))
        return;
    m->anim = ca->add_skeleton(m->anim_file, m->action, m->category == MOVE_TYPE_STATE, anim_folder, true, false);
    if (load_sounds && m->id != 0x92 && m->id != 0x93 && m->id != 0x94 && m->id != 0x95)
        sound_ref_load(&m->sound, false);
    if (m->category == MOVE_TYPE_STATE) {
        for (i = 0; i < 3; i++)
            if (m->triggers[i].id)
                ca->add_skeleton_trigger(m->anim, anim_event_names[m->triggers[i].id], m->triggers[i].time * 30.000002f);
    }
}

// 0x447850  Confidence: exact (VC6 byte match, m-1020 w9; was likely) (the town branch walks the lists in the binary's order 0-7, 10-13, 9,
// one loop each, so they are written out; NOTES). In towns (and templeint without full_set) every combat
// move plays the "movement" anim of id 1 and the listed state moves the anim of state move 0x92 (not
// checked for NULL); elsewhere the low flinch / parry moves borrow the anims and sounds of the medium ones.
void charinfo_remap_town_moves(char_info *info, bool full_set)
{
    move_info *src, *src2, *m;

    if (level_is_town || (!_stricmp(level_filename, "templeint") && !full_set)) {
        src = charinfo_find_move(info, 8, 1, true);
        if (!src)
            return;
#define REMAP_LIST(n) for (m = (move_info *)info->moves[n]; m; m = m->next) m->anim = src->anim
        REMAP_LIST(0);
        REMAP_LIST(1);
        REMAP_LIST(2);
        REMAP_LIST(3);
        REMAP_LIST(4);
        REMAP_LIST(5);
        REMAP_LIST(6);
        REMAP_LIST(7);
        REMAP_LIST(10);
        REMAP_LIST(11);
        REMAP_LIST(12);
        REMAP_LIST(13);
        REMAP_LIST(9);
#undef REMAP_LIST
        m = (move_info *)info->moves[MOVE_TYPE_STATE];
        src = charinfo_find_move(info, MOVE_TYPE_STATE, 0x92, true);
        for (; m; m = m->next) {
            if (m->id == 0x6f || m->id == 0x95 || m->id == 0xa0 || m->id == 0x27 || m->id == 0x28
                || m->id == 0xa1 || m->id == 0xa2)
                m->anim = src->anim;
        }
        return;
    }
    src = charinfo_find_move_alt(info, 4, 3, true);
    src2 = charinfo_find_move_alt(info, 2, 2, true);
    if (src) {
        for (m = (move_info *)info->moves[4]; m; m = m->next) {
            if ((m->variant == 1 || m->variant == 2 || m->id == 0x81 || m->id == 0x82)
                && (m->anim_class & info->anim_class)) {
                m->anim = src->anim;
                m->sound = src->sound;
            }
        }
        m = (move_info *)info->moves[5];
        while (m) {
            m->anim = src->anim;
            m->sound = src->sound;
            m = m->next;
        }
    }
    if (src2) {
        for (m = (move_info *)info->moves[2]; m; m = m->next) {
            if ((m->variant == 1 || m->variant == 3) && (m->anim_class & info->anim_class)) {
                m->anim = src2->anim;
                m->sound = src2->sound;
            }
        }
    }
}

// 0x447b10  Confidence: exact (VC6 byte match, m-1019 w1, D10000). The name is the first four letters of
// the skeleton set and the last four digits of its name hash. Loads a non-hero record's anims into the given
// pool (default: the level pool, or the summon pool for summons) through its .svf preload index, and the
// effects of a few boss records.
void level_load_preload(char_info *info, bool load_sounds, int summon, Mempool *pool)
{
    Mempool *mp;
    char name[0x20];
    char hash[0x20];
    bool full_set;
    ca_character *ca;
    unsigned int start;
    move_info *m;
    Character *c;
    int i;

    full_set = false;
    if (!pool) {
        if (!summon) {
            mp = &level_mempool;
        } else {
            mp = &summon_mempool;
            full_set = true;
        }
    } else
        mp = pool;
    if (!_stricmp(info->name, "Tiger Rider"))
        full_set = true;
    if (game_flags & 2)
        full_set = true;
    set_current_mempool(mp);
    start = mp->offset;
    if (!info->alt_parent)
        ca = ca_find_character(info->name);
    else
        ca = ca_find_character(info->alt_parent->name);
    ca_preload_loaded = 0;
    strncpy(name, ca->name, 4);
    name[4] = 0;
    sprintf(hash, "%d", str_hash_nocase_rol6(ca->name));
    strcat(name, hash + strlen(hash) - 4);
    strcat(name, ".svf");
    ca_load_preload_index(name);
    for (i = 0; i < NUM_MOVE_TYPES; i++)
        for (m = (move_info *)info->moves[i]; m; m = m->next)
            charinfo_load_move_anim(info, ca, m, load_sounds, info->anim_folder, full_set);
    ca_preload_loaded = 0;
    if (info->alt_parent)
        charinfo_mark_anims_loaded(info->alt_parent);
    else
        charinfo_mark_anims_loaded(info);
    charinfo_remap_town_moves(info, full_set);
    for (c = living_list.live_next; c != &living_list; c = c->live_next)
        if (c->info == info)
            c->instance->reset_anims(true);
    set_current_mempool(NULL);
    charinfo_level_mem_used += mp->offset - start;
    set_current_mempool(&level_mempool);
    if (info->species == 0xc4 || info->species == 0xc1 || info->species == 0xb7 || info->species == 0x5a) {
        for (i = 0; i < NUM_MOVE_TYPES; i++)
            for (m = (move_info *)info->moves[i]; m; m = m->next)
                if (m->effect)
                    vfx_preload(m->effect, NULL);
    }
    if (!_stricmp(level_filename, "templeint2")) {
        if (info->species == 0xc5) {
            vfx_preload("Mach-MeteorStorm.vfx", NULL);
            vfx_preload("Mach-DarkVoid-Source.vfx", NULL);
            vfx_preload("mach-scream-source.vfx", NULL);
            vfx_preload("mach-teleport-in.vfx", NULL);
            vfx_preload("mach-teleport-out.vfx", NULL);
            vfx_preload("mach-quake.vfx", NULL);
        } else if (info->species == 0xb1) {
            vfx_preload("urath-Blind-source.vfx", NULL);
            vfx_preload("urath-silence-source.vfx", NULL);
            vfx_preload("urath-death-source.vfx", NULL);
            vfx_preload("urath-curse-source.vfx", NULL);
            vfx_preload("urath-poison-source.vfx", NULL);
            vfx_preload("urath-paralyze-source.vfx", NULL);
            vfx_preload("urath-darkspear-source.vfx", NULL);
        }
        vfx_preload("spell-sleep-target.vfx", NULL);
        vfx_preload("spell-silence-target.vfx", NULL);
        vfx_preload("spell-blind-target.vfx", NULL);
    }
    set_current_mempool(NULL);
}

// 0x447e80  Confidence: exact (VC6 byte match, m-1020 w9; was likely; the string walked through a local pointer, D11802).
unsigned int str_hash_nocase_rol6(const char *s)
{
    unsigned int h;
    const char *p;

    if (!s)
        return 0xffffffff;
    h = 0;
    for (p = s; *p; p++)
        h = _rotl(h, 6) ^ tolower(*p);
    return h;
}

// 0x447ec0  Confidence: exact.
void charinfo_mark_anims_loaded(char_info *info)
{
    int i;

    for (i = 0; i < num_char_infos; i++)
        if (char_infos[i].alt_parent == info)
            char_infos[i].anim_class = 0xffffffff;
    if (game_flags & 2) {
        for (i = 0; i < 4; i++)
            if (multi_charinfo_slots[i].info.species == info->species)
                multi_charinfo_slots[i].info.anim_class = 0xffffffff;
    }
    info->anim_class = 0xffffffff;
}

// 0x447f20  Confidence: exact (jump table 0x447fd8). The default prop tag follows the weapon class:
// none -> "luggage 1", swords -> "luggage swords", ...
void character_reinit_anims(Character *c, int anim_class)
{
    c->instance->reset_anims(true);
    c->fill_locomotion_anims();
    c->instance->clear_state_anims();
    c->instance->clear_action_anims();
    c->cur_anim = -1;
    switch (anim_class) {
    case 0: c->default_prop_tag = 10; break;
    case 1: c->default_prop_tag = 2; break;
    case 2: c->default_prop_tag = 4; break;
    case 3: c->default_prop_tag = 3; break;
    case 4: c->default_prop_tag = 5; break;
    case 5: c->default_prop_tag = 6; break;
    case 6: c->default_prop_tag = 7; break;
    case 7: c->default_prop_tag = 8; break;
    case 8: c->default_prop_tag = 9; break;
    }
}

// 0x448000  Confidence: exact (VC6 byte match, m-1019 w1, D10000). Before a cutscene: finishes the summon's emerge (it fast-forwards the
// emerge anim and moves the move's trigger times, a permanent edit of the shared move record), frees its
// anims and skeleton set, takes it out of the party or the object lists and remembers it; with no summon,
// deletes the inactive summons. The level pool offset is saved for summon_restore_after_cutscene.
void summon_stash_for_cutscene()
{
    Character *s = cur_summon;
    Character *c, *next;
    effect_object *fx;
    move_info *mv;
    int i;

    stashed_summon = NULL;
    stashed_summon_player = NULL;
    summon_stash_level_offset = level_mempool.offset;
    if (s) {
        if (s->flags & 0x800)
            return;
    if (!(s->obj_flags & 1)) {
        if (!s->ai.is_mode(6, -1)) {
            cur_summon->ai.begin_emerge();
            mv = cur_summon->cur_move;
#ifdef FIXED_BUILD
            // FIXED_BUILD: C317 SummonStashNoMove. A summon without an emerge move (the Mist Elemental) has no
            // current move here; skip the fast-forward that needs one (the original reads mv->anim through NULL).
            if (mv) {
                float t = cur_summon->instance->get_anim_length(mv->anim);
                cur_summon->instance->process(t - 0.01f, false, 0);
                for (i = 0; i < 3; i++)
                    mv->triggers[i].time -= cur_summon->move_trigger_time_left(mv->triggers[i].id) + 0.1f;
            }
#else
            float t = cur_summon->instance->get_anim_length(mv->anim);
            cur_summon->instance->process(t - 0.01f, false, 0);
            for (i = 0; i < 3; i++)
                mv->triggers[i].time -= cur_summon->move_trigger_time_left(mv->triggers[i].id) + 0.1f;
#endif
            cur_summon->ai.summon_emerge_update();
            for (fx = fx_active_list.fx_next; fx != &fx_active_list; fx = fx->fx_next) {
                if (fx->kind == 8 && fx->owner == cur_summon->handle) {
                    fx_destroy(fx->handle);
                    break;
                }
            }
        }
    }
    charinfo_unload_anims(cur_summon->info);
    character_release(cur_summon->instance->character);
    stashed_summon = cur_summon;
    stashed_summon_player = summon_player;
    if (summon_player) {
        party_remove_member(cur_summon->get_name());
    } else {
        cur_summon->live_prev->live_next = cur_summon->live_next;
        cur_summon->live_next->live_prev = cur_summon->live_prev;
        cur_summon->live_next = NULL;
        cur_summon->live_prev = NULL;
        cur_summon->prev->next = cur_summon->next;
        cur_summon->next->prev = cur_summon->prev;
        cur_summon->next = NULL;
        cur_summon->prev = NULL;
    }
    cur_summon->flags |= 8;
    cur_summon->instance->clear_action_anims();
    cur_summon->instance->clear_state_anims();
    fx_end_owned(cur_summon->handle, true);
    summon_mempool.offset = 0;
    } else {
        for (c = living_list_inactive.live_next, next = c->live_next; c != &living_list_inactive;
             c = next, next = next->live_next) {
            if ((c->flags & 0x80) && (c->flags & CF_SUMMON) && !(c->flags & 0x40000)) {
                fx_end_owned(c->handle, true);
                c->deinit();
            }
        }
        summon_mempool.offset = 0;
    }
}

// 0x4482a0  Confidence: exact (VC6 byte match, m-1020 w9; was likely). A script anim: a move of the script list (15) with the anim file as its
// name, added to the record's skeleton set (in the summon pool when the file fits, else the level pool;
// multiplayer 0x80 skips the pool choice). Returns the skeleton index, -1 during a summon cutscene.
int charinfo_load_script_anim(char_info *info, const char *name, int path_id)
{
    ca_character *ca;
    move_info *m;
    Character *c;
    character_instance *ci;
    int i;

    if (cur_summon && (cur_summon->flags & 0x800))
        return -1;
    if (!(game_flags & 0x80)) {
        File f;
        int need = (int)(f.size(name, path_id) * 1.2f);
        if (summon_mempool.size - summon_mempool.offset > (unsigned int)need)
            set_current_mempool(&summon_mempool);
        else
            set_current_mempool(&level_mempool);
    }
    if (!info->alt_parent)
        ca = ca_find_character(info->name);
    else
        ca = ca_find_character(info->alt_parent->name);
    ca->lock_skeleton_count();
    m = characterinfo_add_move();
    m->action = charinfo_script_anim_pool.Add(name);
    m->anim_file = charinfo_script_anim_pool.Add(name);
    m->anim = ca->add_skeleton(name, name, false, path_id, true, true);
    m->anim_class = 1;
    m->category = MOVE_TYPE_SCRIPT;
    m->id = 0;
    m->variant = 0;
    for (i = 0; i < 3; i++)
        m->triggers[i].id = 0;
    m->sound.name = NULL;
    m->sound.handle = -1;
    m->effect = NULL;
    m->next = (move_info *)info->moves[MOVE_TYPE_SCRIPT];
    info->moves[MOVE_TYPE_SCRIPT] = m;
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        ci = c->instance;
        if (ci->character == ca) {
            ci->reset_anims(false);
            ci->weights[m->anim] = 0.0f;
            ci->times[m->anim] = ci->character->skeletons[m->anim]->start;
            c->update_locomotion_anim();
        }
    }
    set_current_mempool(NULL);
    return m->anim;
}

// 0x4484a0  Confidence: exact.
void charinfo_free_moves(char_info *info)
{
    move_info *m, *next;
    ca_character *ca;

    for (m = (move_info *)info->moves[MOVE_TYPE_SCRIPT]; m; m = next) {
        next = m->next;
        move_info_free(m);
    }
    info->moves[MOVE_TYPE_SCRIPT] = NULL;
    if (!(game_flags & 0x80)) {
        if (!info->alt_parent)
            ca = ca_find_character(info->name);
        else
            ca = ca_find_character(info->alt_parent->name);
        if (ca)
            ca->unlock_skeleton_count();
    }
}

// 0x448500  Confidence: exact (VC6 byte match, m-1019 w1, D10000). After the cutscene: drops every script anim, puts the stashed summon back
// (party or object lists), rebuilds its instance and anims in the summon pool and restores the level pool
// offset.
void summon_restore_after_cutscene()
{
    int i;

    for (i = 0; i < num_char_infos; i++)
        charinfo_free_moves(&char_infos[i]);
    charinfo_script_anim_pool.Reset();
    if (stashed_summon) {
    cur_summon = stashed_summon;
    summon_player = stashed_summon_player;
    if (summon_player) {
        party_add_member(cur_summon->get_name());
    } else {
        cur_summon->prev = obj_active_list.prev;
        cur_summon->next = &obj_active_list;
        obj_active_list.prev->next = cur_summon;
        obj_active_list.prev = cur_summon;
        cur_summon->live_prev = living_list.live_prev;
        cur_summon->live_next = &living_list;
        living_list.live_prev->live_next = cur_summon;
        living_list.live_prev = cur_summon;
    }
    cur_summon->flags &= ~8;
    stashed_summon = NULL;
    stashed_summon_player = NULL;
    summon_mempool.offset = 0;
    set_current_mempool(&summon_mempool);
    cur_summon->instance = charinfo_create_character_instance(cur_summon->info, true);
    characterinfo_load_party_anims(cur_summon->info,
                                   cur_summon->equipment[5] ? cur_summon->equipment[5]->type->weapon_class : 0,
                                   true, true);
    if (cur_summon->info->vfx)
        fx_attach_to_character(cur_summon->info->vfx, cur_summon->handle, -1, -1, 1, 0);
    set_current_mempool(NULL);
    level_mempool.offset = summon_stash_level_offset;
        return;
    }
    level_mempool.offset = summon_stash_level_offset;
}

// 0x4486b0  Confidence: exact.
void charinfo_mark_anims_unloaded(char_info *info)
{
    int i;

    for (i = 0; i < num_char_infos; i++)
        if (char_infos[i].alt_parent == info)
            char_infos[i].anim_class = 0;
    if (game_flags & 2) {
        for (i = 0; i < 4; i++)
            if (multi_charinfo_slots[i].info.species == info->species)
                multi_charinfo_slots[i].info.anim_class = 0;
    }
    info->anim_class = 0;
}

// 0x448710  Confidence: exact (VC6 byte match) (the two tail calls: an if / else in the source).
void charinfo_unload_anims(char_info *info)
{
    ca_character *ca;
    move_info *m;
    int i;

    if (!info->alt_parent)

        ca = ca_find_character(info->name);

    else

        ca = ca_find_character(info->alt_parent->name);
    if (ca && ca->num_anims > 0)
        ca->free_skeletons();
    for (i = 0; i < NUM_MOVE_TYPES; i++) {
        for (m = (move_info *)info->moves[i]; m; m = m->next) {
            m->anim = -1;
            m->sound.handle = -1;
        }
    }
    if (info->alt_parent)
        charinfo_mark_anims_unloaded(info->alt_parent);
    else
        charinfo_mark_anims_unloaded(info);
}

// 0x448790  Confidence: exact.
void charinfo_page_in_sounds(char_info *info)
{
    int set, i;

    for (set = 0; set < info->num_footstep_sets; set++) {
        for (i = 0; i < 5; i++) {
            if (info->footstep_sounds_a[set][i] >= 0)
                snd_load(info->footstep_sounds_a[set][i], 0);
            if (info->footstep_sounds_b[set][i] >= 0)
                snd_load(info->footstep_sounds_b[set][i], 0);
        }
    }
}

// 0x4487f0  Confidence: exact.
const char *offense_point_name(int i)
{
    return offense_point_names[i];
}

// 0x448800  Confidence: likely (the town and multiplayer branches share the summon size store). The
// per-level anim blocks: sizes by level kind, the summon pool, the stat screen block, then every party
// member's (and in single player the other players') instance and anims rebuilt.
void characterinfo_alloc_level_buffers()
{
    Player *p;
    Character *c;

    set_current_mempool(&level_mempool);
    charinfo_level_mem_used = 0;
    charinfo_anim_mem_total = 0;
    if (!(game_flags & 2)) {
        if (level_is_town) {
            rosalind_anim_buffer_size = 0x25800;
            joseph_anim_buffer_size = 0x19000;
            jekhar_anim_buffer_size = 0x19000;
            flece_anim_buffer_size = 0x19000;
            summon_anim_buffer_size = 0xfa000;
        } else {
            if (!_stricmp(level_filename, "templeint")) {
                jekhar_anim_buffer_size = 0x19000;
                rosalind_anim_buffer_size = 0x25800;
            } else {
                jekhar_anim_buffer_size = 0x46c00;
                rosalind_anim_buffer_size = 0x70800;
            }
            joseph_anim_buffer_size = 0x4f800;
            flece_anim_buffer_size = 0x57800;
            summon_anim_buffer_size = 0xfa000;
            if (!_stricmp(level_filename, "masad") && _stricmp(level_script_name, "masad_v2")) {
                joseph_anim_buffer_size = 0x4f800;
                flece_anim_buffer_size = 300;
                jekhar_anim_buffer_size = 300;
                rosalind_anim_buffer_size = 300;
                summon_anim_buffer_size = 0xfa000;
            }
        }
    } else {
        joseph_anim_buffer_size = 0x7e400;
        flece_anim_buffer_size = 0x8ac00;
        jekhar_anim_buffer_size = 0x69c00;
        rosalind_anim_buffer_size = 0xa1800;
        summon_anim_buffer_size = 0xfa000;
    }
    joseph_anim_buffer = operator new(joseph_anim_buffer_size);
    jekhar_anim_buffer = operator new(jekhar_anim_buffer_size);
    rosalind_anim_buffer = operator new(rosalind_anim_buffer_size);
    flece_anim_buffer = operator new(flece_anim_buffer_size);
    summon_anim_buffer = operator new(summon_anim_buffer_size);
    summon_mempool.Copy((char *)summon_anim_buffer, summon_anim_buffer_size, "Summon");
    statscreen_anim_buffer_size = 0x7800;
    charinfo_anim_mem_total = joseph_anim_buffer_size + jekhar_anim_buffer_size + flece_anim_buffer_size
                              + rosalind_anim_buffer_size;
    statscreen_anim_buffer = operator new(0x7800);
    set_current_mempool(NULL);
    player_instance_mempool.Copy(player_instance_buffer, sizeof(player_instance_buffer), "Player instance");
    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (c)
            charinfo_unload_anims(c->info);
    }
    for (p = party_list.next; p != &party_list; p = p->next) {
        c = living_from_handle(p->handle);
        if (!c)
            continue;
        if (!(game_flags & 2) && !_stricmp(level_filename, "masad") && _stricmp(level_script_name, "masad_v2")
            && c->info->species != SPECIES_JOSEPH) {
            printf("Cannot load any non joseph characters in masad!!\n");
            continue;
        }
        c->instance = charinfo_create_character_instance(c->info, false);
        if (game_flags & 2)
            if (c->equipment[7] && c->equipment[7]->type->slot == 8)
                c->apply_hero_model(1);
            else
                c->apply_hero_model(0);
        characterinfo_load_party_anims(c->info, c->equipment[5] ? c->equipment[5]->type->weapon_class : 0, true, false);
    }
    if (game_flags & 1) {
        for (p = player_list_other.next; p != &player_list_other; p = p->next) {
            c = living_from_handle(p->handle);
            if (!c)
                continue;
            if (!_stricmp(level_filename, "masad") && _stricmp(level_script_name, "masad_v2")
                && c->info->species != SPECIES_JOSEPH) {
                printf("Cannot load any non joseph characters in masad!!\n");
                continue;
            }
            charinfo_unload_anims(c->info);
            c->instance = charinfo_create_character_instance(c->info, false);
            characterinfo_load_party_anims(c->info, c->equipment[5] ? c->equipment[5]->type->weapon_class : 0, true, false);
        }
    }
}

// 0x448c10  Confidence: exact.
void characterinfo_level_unload(bool reset_player_pool)
{
    int i;

    character_release_all();
    for (i = 0; i < num_char_infos; i++)
        char_infos[i].anim_class = 0;
    if (game_flags & 2) {
        for (i = 0; i < 4; i++)
            multi_charinfo_slots[i].info.anim_class = 0;
    }
    if (reset_player_pool)
        player_instance_mempool.Copy(player_instance_buffer, sizeof(player_instance_buffer), "Player instance");
}

// 0x448c80  Confidence: exact (VC6 byte match) (jump table 0x448ce0; every case keeps its own `1.0f - x`, so the
// subtraction is written once after the switch and the compiler copied it into the cases). Returns
// 1 - the damage multiplier of a resistance level (weak 2.0, vulnerable 1.5, standard 1.0, strong 0.5,
// invulnerable 0, strengthen -0.5).
float resistance_level_damage_mult(int level)
{
    float mult;

    switch (level) {
    case 0: mult = 2.0f; break;
    case 1: mult = 1.5f; break;
    case 3: mult = 0.5f; break;
    case 4: mult = 0.0f; break;
    case 5: mult = -0.5f; break;
    default: mult = 1.0f; break;
    }
    return 1.0f - mult;
}

// 0x448d00  Confidence: exact.
int summon_skill_from_creature_name(const char *name)
{
    int i;

    for (i = 0; i < MAX_SKILLS; i++)
        if ((skill_table[i].flags & 4) && !_stricmp(name, summon_skill_creature_name(i)))
            return i;
    return -1;
}

// 0x448d40  Confidence: exact.
int skill_find_by_display_name(const char *name)
{
    int i;

    for (i = 0; i < MAX_SKILLS; i++)
        if ((skill_table[i].flags & 4) && !_stricmp(name, localize_string_or_invalid(skill_table[i].display_name)))
            return i;
    return -1;
}

// 0x448d90  Confidence: exact. The skeleton of a loaded state move by its action name.
int charinfo_find_anim(char_info *info, const char *name)
{
    move_info *m;

    for (m = (move_info *)info->moves[MOVE_TYPE_STATE]; m; m = m->next)
        if (!_stricmp(m->action, name) && (info->anim_class & m->anim_class))
            return m->anim;
    return -1;
}

// 0x448de0  Confidence: exact (VC6 byte match, m-1020 w9; was likely). Loads the skeleton set and mesh (the parent's for +Alternate character:
// copies, with the copy's texture entry) into the record's pool and wraps it in a new instance; the
// level pool use of non-hero, non-summon records is counted; multiplayer slot copies remember their
// reloaded mesh textures.
character_instance *charinfo_create_character_instance(char_info *info, int summon)
{
    Mempool *pool;
    unsigned int start;
    ca_character *ca;
    character_instance *ci;
    multi_charinfo_slot_t *slot;
    vim *mesh;
    int i;

    pool = charinfo_select_mempool(info->species, summon);
    set_current_mempool(pool);
    start = pool->offset;
    if (info->alt_parent)
        ca = character_load((char **)&info->alt_parent->name, 1, false, (int)info->alt_textures, NULL, NULL);
    else
        ca = character_load((char **)&info->name, 1, false, 0, NULL, NULL);
    ci = new character_instance(ca);
    set_current_mempool(NULL);
    unsigned int end = pool->offset;
    if (!summon && !species_is_hero(info->species))
        charinfo_level_mem_used += end - start;
    if ((game_flags & 2) && (slot = multi_charinfo_slot(info)) != NULL) {
        for (i = 0; i < ca->mesh->vim->num_textures; i++) {
            mesh = ca->mesh->vim;
            slot->textures[i] = bm_reload(mesh->textures[i]);
        }
    }
    return ci;
}

// 0x448f20  Confidence: exact (the hero range is tested inline, not through species_is_hero).
Mempool *charinfo_select_mempool(int species, int summon)
{
    if (species >= SPECIES_JOSEPH && species <= SPECIES_ROSALIND)
        return &player_instance_mempool;
    if (summon) {
        summon_mempool.offset = 0;
        return &summon_mempool;
    }
    return &level_mempool;
}

// 0x448f60  Confidence: exact (VC6 byte match, m-1019 w1, D10000). The stat screen idle anim (state move 0x9e light / 0x9f heavy armour):
// in multiplayer it is already loaded; else it goes into the stat screen pool. The party member using
// the record gets it (m is NULL when the record has no such move: not checked).
void statscreen_load_anim(Character *c, int heavy)
{
    ca_character *ca;
    move_info *m;
    Player *p;
    Character *lc;

    if (!species_is_hero(c->info->species))
        return;
    if (game_flags & 2) {
        if (!heavy)
            m = charinfo_find_move(c->info, MOVE_TYPE_STATE, 0x9e, false);
        else
            m = charinfo_find_move(c->info, MOVE_TYPE_STATE, 0x9f, false);
        int anim = m->anim;
        c->anim_statscreen = anim;
        c->instance->rewind_anim(anim);
        return;
    }
    statscreen_mempool.Copy((char *)statscreen_anim_buffer, 0x7800, "statscreen");
    set_current_mempool(&statscreen_mempool);
    ca = ca_find_character(c->info->name);
    for (m = (move_info *)c->info->moves[MOVE_TYPE_STATE]; m; m = m->next) {
        if (!heavy) {
            if (m->id == 0x9e) {
                printf("Loading light armor version for %s\n", c->info->name);
                charinfo_load_move_anim(c->info, ca, m, true, c->info->anim_folder, false);
                break;
            }
        } else if (m->id == 0x9f) {
            printf("Loading heavy armor version for %s\n", c->info->name);
            charinfo_load_move_anim(c->info, ca, m, true, c->info->anim_folder, false);
            break;
        }
    }
    set_current_mempool(NULL);
    for (p = party_list.next; p != &party_list; p = p->next) {
        if (p->char_info == c->info - char_infos && (lc = living_from_handle(p->handle)) != NULL) {
            int anim = m->anim;
            lc->anim_statscreen = anim;
            lc->instance->rewind_anim(anim);
            return;
        }
    }
}

// 0x449100  Confidence: exact (VC6 byte match).
void statscreen_unload_anim(Character *c)
{
    move_info *m;
    int anim;
    Player *p;
    Character *lc;

    if (!species_is_hero(c->info->species))
        return;
    if (game_flags & 2) {
        c->anim_statscreen = -1;
        return;
    }
    printf("Unloading stat screen animation for %s\n", c->info->name);
    m = charinfo_find_move(c->info, MOVE_TYPE_STATE, 0x9e, true);
    if (!m)
        m = charinfo_find_move(c->info, MOVE_TYPE_STATE, 0x9f, true);
    anim = m->anim;
    ca_character *ch = ca_find_character(c->info->name);
    ch->free_skeleton(anim);
    for (p = party_list.next; p != &party_list; p = p->next) {
        if (p->char_info == c->info - char_infos && (lc = living_from_handle(p->handle)) != NULL) {
            lc->anim_statscreen = -1;
            return;
        }
    }
}

// 0x449200  Confidence: exact.
void chain_config_set(int species, int slot, int skill)
{
    char name[0x14];

    sprintf(name, "chainconfig%d%d", species, slot);
    savegame_state.set_int(4, name, skill);
}

// 0x449240  Confidence: exact.
int chain_config_get(int species, int slot)
{
    char name[0x14];
    int skill;

    sprintf(name, "chainconfig%d%d", species, slot);
    skill = savegame_state.get_int(4, name);
    if (!skill) {
        chain_config_reset_defaults();
        skill = savegame_state.get_int(4, name);
    }
    return skill;
}

// 0x449290  Confidence: exact.
void chain_points_set(int species, int slot, int points)
{
    char name[0x14];

    sprintf(name, "chainpts%d%d", species, slot);
    savegame_state.set_int(4, name, points);
}

// 0x4492d0  Confidence: exact.
int chain_points_get(int species, int slot)
{
    char name[0x14];

    sprintf(name, "chainpts%d%d", species, slot);
    return savegame_state.get_int(4, name);
}

// 0x449310  Confidence: exact. Slots 0..3 = the record's up, down, left, right skills.
void chain_config_reset_defaults()
{
    int species;

    for (species = 0; species < 4; species++) {
        chain_config_set(species, 0, char_infos[charinfo_find_by_species(species)].up_skill);
        chain_config_set(species, 1, char_infos[charinfo_find_by_species(species)].down_skill);
        chain_config_set(species, 2, char_infos[charinfo_find_by_species(species)].left_skill);
        chain_config_set(species, 3, char_infos[charinfo_find_by_species(species)].right_skill);
        chain_points_set(species, 0, 0);
        chain_points_set(species, 1, 0);
        chain_points_set(species, 2, 0);
        chain_points_set(species, 3, 0);
    }
}

// 0x4493e0  Confidence: exact.
void character_set_chain_skill(Player *p, int slot, int skill)
{
    int species = char_infos[p->char_info].species;
    int i;

    if (species >= 4)
        return;
    for (i = 0; i < 4; i++)
        if (chain_config_get(species, i) == skill)
            chain_config_set(species, i, SKILL_NONE);
    chain_config_set(species, slot, skill);
}

// 0x449440  Confidence: exact (VC6 byte match; an if / else chain on the slot; a switch would compile the same way
// at four cases). Non-heroes use their record's skills; no AP means no chain.
int character_get_chain_skill(Player *p, int slot)
{
    Character *c;
    int skill;

    if (char_infos[p->char_info].species >= 4) {
        if (slot == 0)
            skill = char_infos[p->char_info].up_skill;
        else if (slot == 1)
            skill = char_infos[p->char_info].down_skill;
        else if (slot == 2)
            skill = char_infos[p->char_info].left_skill;
        else if (slot == 3)
            skill = char_infos[p->char_info].right_skill;
        else {
            hud_show_message(XSTR("No chain skill configured!", 210));
            skill = SKILL_NONE;
        }
    } else {
        skill = chain_config_get(char_infos[p->char_info].species, slot);
    }
    c = living_from_handle(p->handle);
    if (c && c->ap >= 1)
        return skill;
    hud_show_message(XSTR("No AP!", 211));
    return SKILL_NONE;
}

// 0x449510  Confidence: exact.
void character_cycle_default_chain(Player *p)
{
    if (p->default_chain == 0)
        p->default_chain = 3;
    else if (p->default_chain == 3)
        p->default_chain = 1;
    else if (p->default_chain == 1)
        p->default_chain = 2;
    else if (p->default_chain == 2)
        p->default_chain = 0;
}

// 0x449560 init_ccmd_Quick_combo_thunk (jmp). Confidence: exact.
// 0x449570 init_ccmd_Quick_combo (console_command constructor). Confidence: exact.
console_command ccmd_Quick_combo("Quick_combo", "Toggles Quick_combo", cmd_Quick_combo);   // 0x65e998

// 0x449590  Confidence: exact (the console's bool command macro).
void cmd_Quick_combo()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Quick_combo = true;
        else if (arg_type & ARG_FALSE)
            Quick_combo = false;
        else if (arg_type & ARG_NONE)
            Quick_combo ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it\n",
                "Quick_combo", "Quick_combo");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "Quick_combo", Quick_combo ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x449650  Confidence: likely (the order of the float compare operands). A landed chain attack: with
// chance ap_cost * 0.23 it costs floor(count * ap_cost * 0.2) + 1 AP; then, for a hero, with chance 0.13
// (always with Quick_combo) the skill's chain slot gains a point (10 with Quick_combo), up to 100, and
// every chain skill of the slot whose threshold is crossed is announced in a message box.
void character_learn_chain_skill(Player *p, int skill, int count)
{
    char who[0x40];
    char msg[0x50];
    char *ok[1];
    int species = char_infos[p->char_info].species;
    Character *c;
    int *slot;
    int i, points, new_points;
    float cost;

    c = living_from_handle(p->handle);
    if (c) {
        cost = (float)skill_table[skill].ap_cost;
        if (frand() < cost * 0.23f)
        {
            int ap = (int)floor(count * cost * 0.2f) + 1;
            c->add_ap((float)-ap);
        }
    }
    if (species >= 4)
        return;
    slot = NULL;
    for (i = 0; i < NUM_CHAIN_SKILLS; i++)
        if (chain_skill_table[i].species == species && chain_skill_table[i].skill == skill)
            slot = &chain_skill_table[i].slot;
    if (!slot)
        return;
    if (!Quick_combo && !(frand() <= 0.13f))
        return;
    points = chain_points_get(species, *slot);
    if (points >= 100)
        return;
    new_points = Quick_combo ? points + 10 : points + 1;
    chain_points_set(species, *slot, new_points);
    for (i = 0; i < NUM_CHAIN_SKILLS; i++) {
        if (chain_skill_table[i].species == species && chain_skill_table[i].slot == *slot
            && points < chain_skill_table[i].points && new_points >= chain_skill_table[i].points) {
            c->get_display_name(who);
            sprintf(msg, XSTR("%s learned %s!", 212), who, localize_string_or_invalid(chain_skill_table[i].name));
            ok[0] = XSTR("Ok", 213);
            ui_message_box_show(XSTR("Chain skill", 214), msg, -1, -1, 1, ok, NULL, 1);
        }
    }
}

// 0x449830  Confidence: exact.
int chain_count_unlocked_skills(Player *p)
{
    int species = char_infos[p->char_info].species;
    int i, n;

    if (species >= 4)
        return 0;
    n = 0;
    for (i = 0; i < NUM_CHAIN_SKILLS; i++)
        if (chain_skill_table[i].species == species
            && chain_points_get(species, chain_skill_table[i].slot) >= chain_skill_table[i].points)
            n++;
    return n;
}

// 0x449890  Confidence: exact.
int chain_get_unlocked_skill(Player *p, int n)
{
    int species = char_infos[p->char_info].species;
    int i, k;

    if (species < 4) {
        k = 0;
        for (i = 0; i < NUM_CHAIN_SKILLS; i++) {
            if (chain_skill_table[i].species == species
                && chain_points_get(species, chain_skill_table[i].slot) >= chain_skill_table[i].points) {
                if (k == n)
                    return chain_skill_table[i].skill;
                k++;
            }
        }
    }
    return SKILL_NONE;
}

// 0x449910  Confidence: exact.
int chain_config_find_skill(Player *p, int skill)
{
    int species = char_infos[p->char_info].species;
    int i;

    if (species < 4) {
        for (i = 0; i < 4; i++)
            if (chain_config_get(species, i) == skill)
                return i;
    }
    return -1;
}

// 0x449960  Confidence: exact (no hero test: other species read "chainpts<species>..").
int chain_skill_get_points(Player *p, int skill)
{
    int species = char_infos[p->char_info].species;
    int i;

    for (i = 0; i < NUM_CHAIN_SKILLS; i++)
        if (chain_skill_table[i].species == species && chain_skill_table[i].skill == skill)
            break;
    if (i >= NUM_CHAIN_SKILLS)
        return 0;
    return chain_points_get(species, chain_skill_table[i].slot);
}

// 0x4499c0  Confidence: exact.
void chain_skill_set_points(Player *p, int skill, int points)
{
    int species = char_infos[p->char_info].species;
    int i;

    for (i = 0; i < NUM_CHAIN_SKILLS; i++)
        if (chain_skill_table[i].species == species && chain_skill_table[i].skill == skill)
            break;
    if (i < NUM_CHAIN_SKILLS)
        chain_points_set(species, chain_skill_table[i].slot, points);
}

// 0x449a20  Confidence: exact (no lower bound).
int species_is_hero(int species)
{
    return species <= SPECIES_ROSALIND;
}

// 0x449a30  Confidence: exact (VC6 byte match, match27; was likely: one return of the result local, set to
// the record itself in an else; D7604: with no free slot the error repeats for ever, as in the binary). In
// multiplayer a hero record is copied into a free slot (so each player's hero can load its own weapon anims
// and textures); everything else uses the record itself.
char_info *characterinfo_find_multi_slot(int index)
{
    char_info *info = NULL;
    int i;

    if ((game_flags & 2) && species_is_hero(char_infos[index].species)) {
        for (i = 0; i < 4; i++) {
            if (!multi_charinfo_slots[i].used) {
                info = &multi_charinfo_slots[i].info;
                *info = char_infos[index];
                multi_charinfo_slots[i].used = 1;
                multi_charinfo_slots[i].info.flags |= CIF_MULTI_SLOT;
                break;
            }
        }
        while (i == 4)      // no free slot: the error repeats for ever (0x449adc jumps back)
            VSDK_ERROR(CHARACTERINFO_CPP, 0x14be, "Unable to find multiplayer character info slot");
    } else
        info = &char_infos[index];
    return info;
}

// 0x449b00  Confidence: exact.
void multi_charinfo_slot_release(char_info *info)
{
    int i;

    if (game_flags & 1)
        return;
    if (!species_is_hero(info->species))
        return;
    for (i = 0; i < 4; i++) {
        if (multi_charinfo_slots[i].used && &multi_charinfo_slots[i].info == info) {
            multi_charinfo_slots[i].used = 0;
            return;
        }
    }
}

// 0x449b60  Confidence: exact.
void multi_charinfo_slots_clear()
{
    int i;

    for (i = 0; i < 4; i++)
        multi_charinfo_slots[i].used = 0;
}

// 0x449b80  Confidence: exact.
void *charinfo_get_multi_appearance(char_info *info)
{
    multi_charinfo_slot_t *slot = multi_charinfo_slot(info);

    if (!slot)
        return NULL;
    return slot->textures;
}

// 0x449ba0 init_ccmd_charinfo_thunk (jmp). Confidence: exact.
// 0x449bb0 init_ccmd_charinfo. Confidence: likely (the command's function is the shared
// empty_function: its body was empty or compiled out).
console_command ccmd_charinfo("charinfo", "dump info on all characters in the game", empty_function);   // 0x65e968

// 0x449bd0 init_ccmd_levelchars_thunk (jmp). Confidence: exact.
// 0x449be0 init_ccmd_levelchars (console_command constructor). Confidence: exact.
console_command ccmd_levelchars("levelchars", "dump info on all characters in the level", cmd_levelchars);   // 0x8b3c38

// 0x449c00  Confidence: exact (VC6 byte match under 8966, match27 survey; was likely: the shared `push 0` of
// print_to_console / time). Appends a count of the living characters per species to levelchars.txt in the
// install directory.
void cmd_levelchars()
{
    char path[0x100];
    int counts[0xe2];
    time_t t;
    struct tm *tm;
    FILE *fp;
    int i, total;
    Character *c;

    file_get_full_dir(path_root, path);
    strcat(path, "\\levelchars.txt");
    fp = fopen(path, "a+");
    if (!fp) {
        print_to_console("Unable to open levelchars.txt for writing", 0);
        return;
    }
    t = time(NULL);
    tm = localtime(&t);
    fprintf(fp, "==============================\n");
    fprintf(fp, "\tCharacter count for level %s(%s)\n", level_filename, level_script_name);
    fprintf(fp, "\tDate: %s", asctime(tm));
    fprintf(fp, "==============================\n");
    memset(counts, 0, sizeof(counts));
    total = 0;
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        counts[c->info->species]++;
        total++;
    }
    for (i = 0; i < NUM_SPECIES_TABLE; i++)
        if (counts[species_table[i].species] > 0)
            fprintf(fp, "%s\t%d\n", species_table[i].name, counts[species_table[i].species]);
    fprintf(fp, "Total number of chars:\t%d\n", total);
    fprintf(fp, "\n\n");
    fclose(fp);
    print_to_console("done dumping character information to levelchars.txt", 0);
}
