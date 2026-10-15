// levelscripts/level_scripts.h - level script hooks, the level tables, shopkeepers and quests
// (levelscripts\scripts\level_scripts_common.cpp path references 0x4caff8-0x4cbe8d; level_scripts.cpp
// 0x4d5990-0x4d6040 and 0x4f0990 by string ids; events_parse_table 0x4d39c0 fills the quest table).
// One header for these units; file name from strings.tbl. Not original source.
// Layouts: symbols/types.h level_script_info, shop_item, shopkeeper, quest_info.
#ifndef LEVELSCRIPTS_LEVEL_SCRIPTS_H
#define LEVELSCRIPTS_LEVEL_SCRIPTS_H

#include "vsdk/vsdk.h"

// One level's script hooks in level_script_table (0x59f128, 51 records).
struct level_script_info {
    const char *name;                    // +0x00 level file name ("masad", "sewer", ...)
    void (*init)();                      // +0x04
    void (*frame)();                     // +0x08
    void (*leave)();                     // +0x0C
    char (*use)(const char *name);       // +0x10
    void *unused14;                      // +0x14
    void *action;                        // +0x18 script/dialogue action handler
    int load_steps;                      // +0x1C loading-screen progress total
    const char *music;                   // +0x20
    int num_flags;                       // +0x24 savegame flag bits
    const unsigned int *flag_hashes;     // +0x28
    int automap_layers;                  // +0x2C
    int num_ints;                        // +0x30
    const unsigned int *int_hashes;      // +0x34
    int num_floats;                      // +0x38
    const unsigned int *float_hashes;    // +0x3C
};
LAYOUT_CHECK(level_script_info, sizeof(level_script_info) == 0x40);
extern level_script_info level_script_table[51];     // 0x59f128
int name_hash_nocase(const char *name);               // 0x4d5f50 (savegame name hashes)
extern bool level_is_town;                           // 0x60a40c set by level_script_parse_table ("#Town")
extern int cur_level_index;                          // 0x59f120 (4 = world map)
int  level_script_find(const char *level_name);      // 0x4d6300, -1 = none
int  level_script_find_index(int level, const char *script_name);   // 0x4d6340, -1 = none
void level_format_display_name(char *out, int level);// 0x4d63b0

// The level table (0x10 bytes, level_table 0x59fde8).
struct level_table_entry {
    int id;                                          // +0x0
    char **display_name;                             // +0x4 localize_strings slot
    unsigned int mp_flags;                           // +0x8 bit 0: listed on the level select screen
    int unk0c;                                       // +0xc
};
extern level_table_entry level_table[26];            // 0x59fde8
// The level_table index of a level id, or -1 (the "endgame" string after the table is only its end).
int  level_table_find_id(int level);                 // 0x4d6390 (formerly level_is_endgame)
char *level_table_display_name(int level);           // 0x4d64c0 NULL when unknown

// {level file name, localized display name slot} (type name ours).
struct level_display_name {
    const char *name;
    char **text;
};
extern level_display_name level_display_names[52];   // 0x594440 (51 used, cmp 0x33)

// ---- shops (level_parse_shopkeeper 0x4cc4d0) ---------------------------------------------------------
struct shop_item {                   // 8 bytes, level_shop_items 0x2491ac0
    int item_type;                   // +0x0 item_types index
    int price;                       // +0x4
};
struct shopkeeper {                  // 0x14 bytes, level_shopkeepers 0x2490ca0
    const char *name;                // +0x00 the character's name
    int num_sell;                    // +0x04 +Sell List: entries
    shop_item *sell;                 // +0x08
    int num_buy;                     // +0x0C +Buy List: entries
    shop_item *buy;                  // +0x10
};
shopkeeper *level_shopkeeper_find(const char *name); // 0x4ce950

// ---- quests (events_parse_table 0x4d39c0) --------------------------------------------------------------
struct quest_info {                  // 0x6c bytes at 0x253ef40
    const char *name;                // +0x00 $Quest: name
    ubyte side_quest;                // +0x04 1 = side quest
    ubyte _pad05[3];
    struct {                         // +0x08 one per +Stage: / +Invalidate: line
        const char *text;            //   +Stage: text (0 for an +Invalidate: entry)
        unsigned int flag_hash;      //   +Flag: / +Invalidate: name hash, 0 none
    } steps[12];
    ubyte num_steps;                 // +0x68
    signed char invalidate_step;     // +0x69 -1 none
    ubyte _pad6a[2];
};
LAYOUT_CHECK(quest_info, sizeof(quest_info) == 0x6c);
extern quest_info quest_table[200];                  // 0x253ef40
extern int quest_count;                              // 0x25443a4
int quest_get_current_step(int quest);               // 0x4d6080, -1 = not active

// ---- level containers and exits (level_scripts_common.cpp .bss 0x248ce40-0x2493a08) ------------------
// lc_entries: 50 records of 0x10c bytes (lc_entry_add). sum.symbols tags them new_hud.cpp, but the
// address is in the level scripts' .bss (see game/NOTES-hud.md). Type name ours.
struct lobj;
struct lc_entry {
    lobj *object;                    // +0x000
    char name[0x100];                // +0x004
    unsigned int flags;              // +0x104 bit 0 gold container
    int gold;                        // +0x108
};
LAYOUT_CHECK(lc_entry, sizeof(lc_entry) == 0x10c);
extern lc_entry lc_entries[50];                      // 0x248ce40
extern int lc_entry_count;                           // 0x2491a68
// level_check_triggers' state. sum.symbols types level_exit_trigger void* (the trigger record); new_hud.cpp
// passes it to level_script_find, so it is the record's leading name here.
extern char *level_exit_trigger;                     // 0x24902c4
extern unsigned int level_exit_block_flags;          // 0x2491a50 (1 party too far, 2 hostiles near)
extern bool game_frozen;                             // 0x2491a5c (GameTick zeroes reported_frametime)
class vector3;
extern vector3 level_exit_return_pos;                // 0x2493a08

void goto_credits();                                 // 0x4ef6a0 (level_script_templeint2.cpp range)

#endif
