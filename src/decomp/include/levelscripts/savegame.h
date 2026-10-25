// levelscripts/savegame.h - the savegame object (savegame_* 0x4ceec0-0x4d4a50 in levelscripts\level_scripts.cpp:
// the module column of sum.symbols and the strings.tbl ids 469-476 put them in that unit; decomp27).
// Not original source. One merged class of the three stub copies (DECISIONS.md D14); the layout is the
// decomp27 one (D700), docs/sum-exe/savegame.md has the file format.
#ifndef LEVELSCRIPTS_SAVEGAME_H
#define LEVELSCRIPTS_SAVEGAME_H

#include "vsdk/vsdk.h"

struct conversation_speaker;         // levelscripts/conversation.h
struct automap_label;                // summoner/automap.h
struct lobj;                         // engine/rendering/levelrender.h
class Character;

#define SAVEGAME_VERSION      0x27
#define SAVEGAME_BUFFER_SIZE  0x4b000
#define SAVEGAME_NUM_LEVELS   51     // level_script_table records
#define SAVEGAME_NUM_SLOTS    7      // conversation slots per level (conversation_save_table)
#define SAVEGAME_MAX_LIVING   175    // LENT records (0xaf)
#define SAVEGAME_MAX_LEVEL_ITEMS 0x200
#define SAVEGAME_MAX_TWO_STAGE 0x79  // two-stage entries per level
#define SAVEGAME_MAX_LABELS   0x80   // automap labels
#define SAVEGAME_MAX_GOLD_PILES 0x20 // CONT gold piles

// One section of the buffer: start and capacity (layout functions 0x4d1800..0x4d1c10).
struct savegame_section {
    ubyte *data;                     // +0x0
    int size;                        // +0x4
};

// The per-level block (savegame_data_offset 0x4cfbd0; "data_pointers" in savegame_set_bit's message).
// The block holds num_flags flag bits, num_bits automap reveal bits (layers x 4096), padding to a byte,
// then the int and the float variables. The two bit counts are unsigned: savegame_layout_level_data
// divides their sum with shr, savegame_get_bit compares with jae.
struct savegame_level_data {
    ubyte *data;                     // +0x00
    unsigned int num_flags;          // +0x04 level_script_info.num_flags
    unsigned int num_bits;           // +0x08 automap_layers << 12
    int num_ints;                    // +0x0C
    int num_floats;                  // +0x10
    int size;                        // +0x14 (num_flags + num_bits + 7) / 8 + 4 * (num_ints + num_floats)
};
LAYOUT_CHECK_32(savegame_level_data, sizeof(savegame_level_data) == 0x18);

// The LENT party state (0x24 bytes; savegame_write_party_state / savegame_read_party_state). Field
// names ours, bit positions from the masks in both functions.
struct savegame_party_state {
    unsigned int members : 4;        // bits 0-3 party members present, by species
    unsigned int combat_mode : 1;    // bit 4 combat_mode
    unsigned int solo : 1;           // bit 5 game_flags & GF_SOLO_MODE
    unsigned int summon_controlled : 1; // bit 6 summon_player was set
    int selected : 3;                // bits 7-9 species of the selected member, -1 = the summon
    int summon : 22;                 // bits 10-31 species of cur_summon, -1 = none
    char summon_name[0x20];          // +0x04
};
LAYOUT_CHECK(savegame_party_state, sizeof(savegame_party_state) == 0x24);

// One hot button (6 bytes, unaligned id; 5 members x 6 buttons at the hot button section).
#pragma pack(push, 1)
struct savegame_hot_button {
    bool used;                       // +0
    bool kind;                       // +1 savegame_hot_button_set stores `kind != 0` (setne)
    int id;                          // +2
};
#pragma pack(pop)
LAYOUT_CHECK(savegame_hot_button, sizeof(savegame_hot_button) == 6);

// One automap label record of the section (0x34 bytes; the section starts with an int count).
struct savegame_automap_label {
    int level;                       // +0x00 level_script_table index
    int script;                      // +0x04 conversation slot (level_script_find_index)
    float x;                         // +0x08 automap_label.x
    float z;                         // +0x0C automap_label.z
    int layer;                       // +0x10 automap_label.layer
    char text[0x20];                 // +0x14
};
LAYOUT_CHECK(savegame_automap_label, sizeof(savegame_automap_label) == 0x34);

// The two-stage section (0x673f bytes): counts, hashes, then two bits per (level, entry) pair. The struct pads to
// 0x6740; the game's section is the unpadded byte count, SAVEGAME_TWO_STAGE_BYTES (D5400).
struct savegame_two_stage {
    int counts[SAVEGAME_NUM_LEVELS];                                  // +0x0000
    unsigned int hashes[SAVEGAME_NUM_LEVELS][SAVEGAME_MAX_TWO_STAGE]; // +0x00CC
    ubyte bits[0x607];                                                // +0x6138 (51 * 242 bits)
};
#define SAVEGAME_TWO_STAGE_BITS_BYTES ((SAVEGAME_NUM_LEVELS * SAVEGAME_MAX_TWO_STAGE * 2 + 7) / 8) // 0x607
#define SAVEGAME_TWO_STAGE_BYTES (sizeof(int) * SAVEGAME_NUM_LEVELS + sizeof(unsigned int) * SAVEGAME_NUM_LEVELS * SAVEGAME_MAX_TWO_STAGE + SAVEGAME_TWO_STAGE_BITS_BYTES)
LAYOUT_CHECK(savegame_two_stage_bytes, SAVEGAME_TWO_STAGE_BYTES == 0x673f);

class savegame {                     // savegame_state at 0x2493a38 (0x10e8 bytes)
public:
    savegame();                                                      // 0x4cfb80 savegame_state__clear
    ~savegame() {}                                                   // atexit 0x4ceef0: empty_function
    // Size reports and checks (the header's size table).
    void report_sizes(int *off);                                     // 0x4ceff0
    int  report_read_sizes(int *off);                                // 0x4cf260 0 = a size differs
    // LENT helpers.
    void write_party_state(int *off);                                // 0x4cf500
    savegame_party_state read_party_state(int *off);                 // 0x4cf680 (returned by value)
    int  write_character(Character *c, int *off);                    // 0x4cf6c0 0 = record too long
    int  read_character(int *off);                                   // 0x4cf790 0 = no such entity
    void write_groups(int *off);                                     // 0x4cf8d0
    int  read_groups(int *off);                                      // 0x4cfa00 (return unused)
    void init_buffer(ubyte *buffer, int size);                       // 0x4cfbb0
    int  data_offset(int level, int type, int index);                // 0x4cfbd0 bit offset; type 0 flag,
                                                                     //   1 automap bit, 2 int, 3 float
    int  get_visit_count(int level);                                 // 0x4cfc40
    void set_visit_count(int level, int count);                      // 0x4cfc60
    void inc_visit_count(int level);                                 // 0x4cfc80 (state_game_enter)
    int  get_flag(int level, const char *name);                      // 0x4cfca0 savegame_get_flag
    void set_flag(int level, const char *name, bool value);          // 0x4cfd30 (bool: D701)
    int  get_bit(int level, int index);                              // 0x4cff20 automap reveal bit
    void set_bit(int level, int index, bool value);                  // 0x4cff80
    int  get_int(int level, const char *name);                       // 0x4d0030 savegame_get_int
    void set_int(int level, const char *name, int value);            // 0x4d00b0
    float get_float(int level, const char *name);                    // 0x4d0130
    void set_float(int level, const char *name, float value);        // 0x4d01c0
    int  find_flag_level(unsigned int hash);                         // 0x4d0240 the flag's value in the
                                                                     //   first level that has it, 0 none
    // The conversation blocks (decomp20; argument lists from conversation.cpp's calls). level is the
    // level_script_table index, slot the conversation slot of the level (level_script_find_index).
    void init_conversation_slot(int level, int slot, conversation_speaker *speakers);    // 0x4d02d0
    bool has_conversation_data(int level, int slot);                 // 0x4d0590
    int  get_speaker_bit(int level, int slot, const char *speaker, int kind);           // 0x4d05f0
    void set_speaker_bit(int level, int slot, const char *speaker, int kind, bool value,
                         bool propagate);                            // 0x4d06e0
    int  get_topic_bit(int level, int slot, const char *speaker, const char *topic, int kind); // 0x4d0870
    void set_topic_bit(int level, int slot, const char *speaker, const char *topic, int kind, bool value,
                       bool propagate);                              // 0x4d09e0 kind 0..3
    int  get_quest_bit(int quest);                                   // 0x4d0c00 0 or 1 (+0x103c bit n)
    void set_quest_bit(int quest, bool value);                       // 0x4d0c40
    void quest_evaluate(unsigned int flag_hash);                     // 0x4d0c90 (sum.symbols quest_evaluate)
    int  level_item_add(const char *name);                           // 0x4d0d00 0 = list full
    int  level_item_remove(const char *name);                        // 0x4d0d60 always 1
    int  level_item_has(const char *name);                           // 0x4d0dd0
    // Two-stage objects (doors): two state bits per level object, open and locked (D100).
    void two_stage_set(bool open, bool locked, lobj *obj);           // 0x4d0e20 (return unused)
    void two_stage_get(bool *open, bool *locked, lobj *obj);         // 0x4d1110 (return unused)
    // Map labels (new_automap.cpp). The second argument is a level_script_find_index slot (sum.symbols
    // calls it "layer"); the return values are unused.
    void automap_label_add(int level, int script, automap_label *label);      // 0x4d12b0
    void automap_label_remove(int level, int script, automap_label *label);   // 0x4d1390
    void automap_labels_reset_level(int level, int script);                   // 0x4d14f0
    int  automap_label_count();                                      // 0x4d1620 (max 0x80)
    // Hot buttons (new_hud.cpp): 5 members x 6 buttons of 6 bytes {used, kind, id} at +0x105c; the member
    // is the portrait species (0..3, 4 for the rest). Argument order from the callee (sum.symbols
    // CORRECTION notes): get stores *kind = byte +1, *id = dword +2 and returns `setne al` of byte +0.
    void hot_buttons_clear_member(int member);                       // 0x4d1630 all 6 (decomp24)
    void hot_button_clear(int member, int slot);                     // 0x4d1660 savegame_hot_button_clear
    void hot_button_set(int kind, int id, int member, int slot);     // 0x4d16a0 savegame_hot_button_set
    bool hot_button_get(int *kind, int *id, int member, int slot);   // 0x4d1770 savegame_hot_button_get
    void clear_buffer();                                             // 0x4d17e0
    // The layout, in savegame_build_layout's order (the buffer order).
    void layout_header();                                            // 0x4d1800
    void layout_thumbnail();                                         // 0x4d1820
    void layout_level_data();                                        // 0x4d1860
    void layout_conversation();                                      // 0x4d18e0
    void layout_items();                                             // 0x4d19e0
    void layout_inventory();                                         // 0x4d1a50
    void layout_living();                                            // 0x4d1a80
    void layout_camera();                                            // 0x4d1ae0
    void layout_item_objects();                                      // 0x4d1b10
    void layout_quest_flags();                                       // 0x4d1b40
    void layout_level_items();                                       // 0x4d1b80
    void layout_two_stage();                                         // 0x4d1bb0
    void layout_automap_labels();                                    // 0x4d1be0
    void layout_hot_buttons();                                       // 0x4d1c10
    // unk: the callers' second argument, never read (D702).
    int  write(const char *filename, int unk, const char *description); // 0x4d1c40, nonzero = written
    int  write_header(const char *description);                      // 0x4d1df0
    int  write_thumbnail();                                          // 0x4d1f80 always 1
    int  write_items();                                              // 0x4d1fb0 always 1
    int  write_inventory();                                          // 0x4d2090
    int  write_living();                                             // 0x4d2150
    int  write_camera();                                             // 0x4d2320
    int  write_item_objects();                                       // 0x4d2370
    int  chunk_stub();                                               // 0x4d25a0 return 1
    int  read(const char *filename, int unk);                        // 0x4d25b0 1 read, 0 / -1 failed
    // Header only. version may be 0. Nonzero = the header is valid.
    int  read_info(const char *filename, char *level_name, char *script_name, int *unk_a8,
                   float *level_time, char *description, int *version); // 0x4d2800
    int  read_thumbnail_file(const char *filename, int unk);         // 0x4d2910 (return unused)
    int  read_header(char *level_name, char *script_name, int *unk_a8, float *level_time,
                     char *description, int *version);               // 0x4d29f0 1, 0 sizes, -1 bad
    int  read_items();                                               // 0x4d2b70
    int  read_inventory();                                           // 0x4d2c90
    int  read_living();                                              // 0x4d2dc0
    int  read_camera();                                              // 0x4d3100
    int  read_item_objects();                                        // 0x4d31e0
    int  read_level_items();                                         // 0x4d33f0 always 1
    int  read_two_stage();                                           // 0x4d34e0
    int  read_automap_labels();                                      // 0x4d3740 always 1
    void set_pending_load(const char *filename, int slot);           // 0x4d3870
    int  pending_load_length();                                      // 0x4d38b0 nonzero = load pending
    int  load_pending();                                             // 0x4d38d0 savegame_read's result

    ubyte *buffer;                   // +0x0000 savegame_buffer
    int buffer_size;                 // +0x0004 0x4b000
    int used;                        // +0x0008 bytes laid out = the file size
    savegame_section header;         // +0x000C 0x1f8
    savegame_section thumbnail;      // +0x0014 0xed1e, start aligned to 16
    int *visit_counts;               // +0x001C 51 dwords, start of the level data
    savegame_level_data levels[SAVEGAME_NUM_LEVELS];    // +0x0020
    ubyte *conversation_data;        // +0x04E8 start of the conversation blocks
    savegame_section conversations[SAVEGAME_NUM_LEVELS][SAVEGAME_NUM_SLOTS];   // +0x04EC
    savegame_section items;          // +0x1014 ITEM
    savegame_section inventory;      // +0x101C INVT
    savegame_section living;         // +0x1024 LENT
    savegame_section camera;         // +0x102C CMRA
    savegame_section item_objects;   // +0x1034 CONT
    savegame_section quest_flags;    // +0x103C 200 bits
    savegame_section level_item_hashes; // +0x1044 int count + 0x200 hashes (member name ours)
    savegame_section two_stage;      // +0x104C savegame_two_stage
    savegame_section map_labels;     // +0x1054 int count + 0x80 savegame_automap_label (member name ours)
    savegame_section hot_buttons;    // +0x105C 5 x 6 savegame_hot_button
    char pending_load[0x80];         // +0x1064 savegame_set_pending_load
    int pending_slot;                // +0x10E4
};
LAYOUT_CHECK_32(savegame, sizeof(savegame) == 0x10e8);
extern savegame savegame_state;                                      // 0x2493a38
extern ubyte savegame_buffer[SAVEGAME_BUFFER_SIZE];                  // 0x2494b2c
// The pilots' wrapper spellings (new_automap.cpp, new_hud.cpp), now the object itself (D46).
#define SAVEGAME_AUTOMAP (savegame_state)
#define SAVEGAME_HM      (&savegame_state)
extern int savegame_thumbnail_bm;                                    // 0x59e794 (201x151, 32-bit)
extern int savegame_file_version;                                    // 0x24e3b3c version of the last header
void savegame_alloc();               // 0x4d3920 (GameInit)
// After a flag change (cdecl, no this: savegame_set_flag calls them with ECX not set).
void savegame_refresh_object_flags();    // 0x4cfe00 level items bound to a flag: obj_flags bit 0
void savegame_refresh_character_flags(); // 0x4cfea0 script-object characters bound to a flag: AI on / off
void savegame_build_layout();        // 0x4d4160
int savegame_location_allowed();     // 0x4d4a50 (0 allowed, 1 combat, 2 level, 3 summon)

#endif
