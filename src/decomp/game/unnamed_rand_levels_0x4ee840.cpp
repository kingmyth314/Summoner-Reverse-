// unnamed_rand_levels_0x4ee840.cpp - Sum.exe v1.40, code 0x4ee840-0x4eee32. Written from the Sum.exe
// disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1 /GX. Confidence per function: exact
// (control flow, calls, constants and globals match), likely (one detail of statement order or expression form
// is a guess), unsure (the shape itself is a guess).
//
// The level scripts of the random-encounter maps of the world map: level_script_table records 0x28-0x2f
// ("Rand-HillsNite01", "Rand-Iceland01" / "Rand-IcelandNite01" (one shared init), "Rand-Grassland01",
// "Rand-Orenia01", "Rand-OreniaNite01", "Rand-DesertNite01", "Rand-GrasslandNite01").
// FILE NAME NOT KNOWN (D5300): no strings.tbl block, no path string, and no single level name to follow the
// level_script_<level>.cpp convention with; whether the seven records are one file or several (or the tail of
// the lenele2d file before them) cannot be decided (no .bss, the literals 0x5a1938-0x5a1af3 follow lenele2d's
// without a gap). One placeholder unit.
// Rand-Grassland01 is also the US Open demo's start level (US_open_demo: the party is built here).
//
// Header (D1400): the level script units have no header of their own; the hooks are declared in
// levelscripts/level_scripts.h.

#include <string.h>
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/gamesound/gamesound.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/player/player.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/savegame.h"
#include "levelscripts/conversation.h"

// Fixed level indices (D2407): level_script_table records.
#define LEVEL_LENELE1D   7                           // "unhide_zane" (0x4eecc8)
#define LEVEL_LENELE1AA  0x23                        // "unhide_korel" (0x4eec34)

// 0x4ee840  Confidence: exact. level_script_table init of Rand-Iceland01 and Rand-IcelandNite01.
void ls_rand_iceland01_init()
{
    level_music = music_find_by_name("rand-iceland01");
}

// 0x4ee860  Confidence: exact. The night map plays the day map's music.
void ls_rand_grasslandnite01_init()
{
    level_music = music_find_by_name("rand-grassland01");
}

// 0x4ee880  Confidence: exact.
void ls_rand_desertnite01_init()
{
    level_music = music_find_by_name("Rand-Desert");
}

// 0x4ee8a0  Confidence: exact. On the _v3 script of the map Death (the cat quest's) is gone once the quest is
// done.
void ls_rand_orenianite01_init()
{
    level_music = music_find_by_name("rand-orenia01");
    if (!_stricmp(level_script_name, "rand-orenianite01_v3")
        && savegame_state.get_flag(cur_level_index, "cat_quest_done"))
        script_npc_set_disabled("Death", true);
}

// 0x4ee8f0  Confidence: exact. The same for Ahlang on the day map.
void ls_rand_orenia01_init()
{
    level_music = music_find_by_name("rand-orenia01");
    if (!_stricmp(level_script_name, "rand-orenia01_v3")
        && savegame_state.get_flag(cur_level_index, "ahlang_quest_done"))
        script_npc_set_disabled("Ahlang", true);
}

// 0x4ee940  Confidence: likely (the order of the two old-item checks after each equip and of the skill
// stores is from the code; the if / else-if chain of the _v3 part could be three ifs with returns, the same
// code). The US Open demo starts here with a level 15 party: Rosalind and Jekhar join (equipped, heal / ice
// resp. 80), and a Joseph hero gets studded armour and heal / fire 80. On the _v3 script one of three
// encounters is left: Lord Korel's (lenele1aa's unhide_korel, until no_more_korel), Zane's bandits (lenele1d's
// unhide_zane, until no_more_zane), or the travelling merchant alone.
void ls_rand_grassland01_init()
{
    item_info *old1;
    item_info *old2;
    Character *c;

    level_music = music_find_by_name("rand-grassland01");
    game_stage_set(0);
    if (US_open_demo) {
        party_set_level(15);
        c = party_member_join("Rosalind", 15, false, true, -1);
        character_equip_item(&old1, &old2, c, 7, item_create_by_name("Burnt Robe"));
        if (old1)
            party_inventory_add_item(old1, 1, 1);
        if (old2)
            party_inventory_add_item(old2, 1, 1);
        character_equip_item(&old1, &old2, c, 5, item_create_by_name("Nagitana"));
        if (old1)
            party_inventory_add_item(old1, 1, 1);
        c->skill[SKILL_HEAL] = 80;
        c->skill[SKILL_ICE] = 80;
        c = party_member_join("Jekhar", 15, false, true, -1);
        character_equip_item(&old1, &old2, c, 7, item_create_by_name("Bougu-Dou"));
        if (old1)
            party_inventory_add_item(old1, 1, 1);
        if (old2)
            party_inventory_add_item(old2, 1, 1);
        character_equip_item(&old1, &old2, c, 5, item_create_by_name("Spineslicer"));
        if (old1)
            party_inventory_add_item(old1, 1, 1);
        party_inventory_add_item(item_create_by_name("Ghork Smashor"), 1, 1);
        party_inventory_add_item(item_create_by_name("Battleaxe"), 1, 1);
        party_inventory_add_item(item_create_by_name("Quilted HOly Robe"), 1, 1);
        if (cur_character->info->species == SPECIES_JOSEPH) {
            character_equip_item(&old1, &old2, cur_character, 7, item_create_by_name("Studded Leather"));
            if (old1)
                party_inventory_add_item(old1, 1, 1);
            if (old2)
                party_inventory_add_item(old2, 1, 1);
            character_equip_item(&old1, &old2, cur_character, 8, item_create_by_name("Studded Pants"));
            if (old1)
                party_inventory_add_item(old1, 1, 1);
            if (old2)
                party_inventory_add_item(old2, 1, 1);
            character_equip_item(&old1, &old2, cur_character, 3, item_create_by_name("Leather Gauntlets"));
            if (old1)
                party_inventory_add_item(old1, 1, 1);
            if (old2)
                party_inventory_add_item(old2, 1, 1);
            cur_character->skill[SKILL_FIRE] = 80;
            cur_character->skill[SKILL_HEAL] = 80;
        }
    }
    if (_stricmp(level_script_name, "rand-grassland01_v3"))
        return;
    if (savegame_state.get_flag(LEVEL_LENELE1AA, "unhide_korel")
        && !savegame_state.get_flag(cur_level_index, "no_more_korel")) {
        script_npc_set_disabled("Traveling Merchant", true);
        script_npc_set_disabled("Zane", true);
        script_npc_set_disabled("Bandit#1", true);
        script_npc_set_disabled("Bandit#2", true);
        script_npc_set_disabled("Bandit#3", true);
        script_npc_set_disabled("Bandit#4", true);
        script_npc_set_disabled("Bandit#5", true);
        script_npc_set_disabled("Bandit#6", true);
    } else if (savegame_state.get_flag(LEVEL_LENELE1D, "unhide_zane")
               && !savegame_state.get_flag(cur_level_index, "no_more_zane")) {
        script_npc_set_disabled("Traveling Merchant", true);
        script_npc_set_disabled("Lord Korel", true);
    } else {
        script_npc_set_disabled("Zane", true);
        script_npc_set_disabled("Bandit#1", true);
        script_npc_set_disabled("Bandit#2", true);
        script_npc_set_disabled("Bandit#3", true);
        script_npc_set_disabled("Bandit#4", true);
        script_npc_set_disabled("Bandit#5", true);
        script_npc_set_disabled("Bandit#6", true);
        script_npc_set_disabled("Lord Korel", true);
    }
}

// 0x4eed80  Confidence: exact. level_script_table action: Korel or Zane and his six bandits turn hostile
// (label 1).
void ls_rand_grassland01_action(dialogue_action *action)
{
    if (!_stricmp(action->name, "KorelHostile"))
        script_npc_goto_label("Lord Korel", 1);
    if (!_stricmp(action->name, "ZaneHostile")) {
        script_npc_goto_label("Zane", 1);
        script_npc_goto_label("Bandit#1", 1);
        script_npc_goto_label("Bandit#2", 1);
        script_npc_goto_label("Bandit#3", 1);
        script_npc_goto_label("Bandit#4", 1);
        script_npc_goto_label("Bandit#5", 1);
        script_npc_goto_label("Bandit#6", 1);
    }
}

// 0x4eee20  Confidence: exact. Rand-HillsNite01 plays the day map's music.
void ls_rand_hillsnite01_init()
{
    level_music = music_find_by_name("rand-hills01");
}
