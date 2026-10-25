// vsdk/localize/localize.h - the string table (sum.symbols module "localize", code 0x503f60-0x505547,
// .data 0x5a5c30-0x5a6057, .bss 0x25c68b0-0x25d889b; directory and file name guessed: no path string, no
// strings.tbl block, no Summoner 2 counterpart). Not original source. The unit's source is
// src/decomp/vsdk/unnamed_localize_0x503f60.cpp (decomp50).
#ifndef VSDK_LOCALIZE_LOCALIZE_H
#define VSDK_LOCALIZE_LOCALIZE_H

#include "vsdk/vsdk.h"

class TextFile;

extern char *localize_strings[1000];                     // 0x25cd4dc
// The id indexes localize_strings; the English text is the source comment (Volition's XSTR convention).
#define XSTR(text, id) (localize_strings[id])
// The slot instead of the string, for tables that keep char ** (level_display_names; D602). Name ours.
#define XSTR_SLOT(text, id) (&localize_strings[id])

// Languages (localize_set_language, the localize_language_names order).
#define LANGUAGE_ENGLISH 0
#define LANGUAGE_GERMAN  1
#define LANGUAGE_FRENCH  2

// The four name tables: chained hash tables of 253 buckets keyed by the English text's localize_hash.
#define LOCALIZE_TABLE_SIZE 0xfd
struct localize_node {                    // 12 bytes from the current pool (localize_table_add); name ours
    int hash;                             // +0x0 localize_hash of the English text
    char *text;                           // +0x4 translation (localize_strdup)
    localize_node *next;                  // +0x8
};
LAYOUT_CHECK_32(localize_node, sizeof(localize_node) == 0xc);

// One localize_language_names entry (0x42 bytes; struct and field names ours): the section name, then a
// code ("" English, "gr" German, "fr" French; localize_get_language_code_buf).
struct localize_language_info {
    char name[0x21];                      // +0x00 "English" / "German" / "French"
    char code[0x21];                      // +0x21
};
LAYOUT_CHECK(localize_language_info, sizeof(localize_language_info) == 0x42);
extern localize_language_info localize_language_names[3];   // 0x5a5c38

// localize_load_level_table types (the two values its callers pass).
#define LOCALIZE_LEVEL_TOPICS  0          // topic_names.tbl, section <level>_dlg.tbl, {text}
#define LOCALIZE_LEVEL_SCRIPTS 3          // script_names.tbl, section <level>_script.tbl, "text"

extern localize_node *localize_topic_table[LOCALIZE_TABLE_SIZE];   // 0x25c68e4
extern localize_node *localize_script_table[LOCALIZE_TABLE_SIZE];  // 0x25c6cd8
extern localize_node *localize_quest_table[LOCALIZE_TABLE_SIZE];   // 0x25c70cc
extern localize_node *localize_item_table[LOCALIZE_TABLE_SIZE];    // 0x25d847c
extern int localize_language;                            // 0x25d8878
extern int localize_french_punct;                        // 0x25d8870 set for the French string table

// The unit's functions in address order (decomp50).
void  localize_unescape(char *text);                     // 0x503fe0 \n \r \" \xHH, in place
char *localize_strdup(const char *text);                 // 0x504070
void  localize_table_add(const char *text, int english_hash, localize_node **table);   // 0x5040e0
void  localize_set_string_raw(const char *text, int id); // 0x504190 (no caller)
char *localize_table_lookup(const char *english, localize_node **table);              // 0x5041b0
int   localize_hash(const char *text);                   // 0x504200
void  localize_load_strings();                           // 0x504230
char *localize_set_string(char *text, int id);           // 0x504400 NULL when the text buffer is full
void  localize_parse_string_line(TextFile *tf);          // 0x5044d0
void  localize_load_names(int which);                    // 0x504520 1 item_names.tbl, 2 quest_names.tbl
void  localize_load_non_level();                         // 0x5047f0
void  localize_add_if_missing(localize_node **table, int *hashes, const char *english,
                              const char *translation);  // 0x504830
void  localize_load_level_table(const char *name, int type);   // 0x504870
void  localize_clear_level();                            // 0x504ed0
void  localize_level_init(const char *script_name);      // 0x504f10 (level_load; decomp42)
void  localize_init();                                   // 0x504f60 (LoadSettings)
char *localize_topic_name(const char *name);             // 0x505020 (dialogue_line::init, the topic list)
char *localize_item_name(const char *name);              // 0x505040
int   localize_stub_ret0();                              // 0x505060 no reference
char *localize_quest_name(const char *name);             // 0x505070
char *localize_script_name(const char *name);            // 0x505090 (decomp37: character_get_display_name)
void  localize_set_language(int language);               // 0x5050b0
void  localize_get_language_name_buf(char *out);         // 0x505120 (no caller)
void  localize_get_language_code_buf(char *out);         // 0x505160 (no caller)
// decomp50: returns text (every path leaves the argument in EAX; the callers ignore it). Was void.
char *localize_remap_charset(char *text, int to_font);   // 0x5051a0
char **localize_get_string_ptr(int id);                  // 0x5054e0 (no caller)
char *localize_string_or_invalid(char **p);              // 0x5054f0: *p, or "INVALID"
char *localize_get_language_name();                      // 0x505510

#endif
