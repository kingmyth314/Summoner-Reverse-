// unnamed_localize_0x503f60.cpp - Sum.exe v1.40, code 0x503f60-0x505547 (sum.symbols module "localize"):
// strings.tbl, the item / quest / topic / script name tables, the language switch and the charset remap.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob0 (the
// static initialisers call the out-of-line empty Mempool() at 0x505530 and localize_clear_level calls
// Mempool::ResetOffset at 0x505540, both COMDATs left at the end of this object; intrinsic strcpy / strcat /
// strlen / memset). No string pooling (two "INVALID", three "#end", ... literals).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// File name not known (no path string, no strings.tbl block, no Summoner 2 file with these functions), so a
// placeholder; the header is vsdk/localize/localize.h (directory guessed).
// Data: .data 0x5a5c30-0x5a6057 (localize_pool_used, the language table, the "Farewell." table, the English
// flag, then the literals in order of use), .bss 0x25c68b0-0x25d889b (address order below; VC6 does not keep
// the definition order).

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "vsdk/localize/localize.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/parse/parse.h"
#include "vsdk/vfile/file.h"

// The /O2 code still emits the English literal of every XSTR it expands (an unreferenced copy right before
// the used one, e.g. "Golem" 0x5a5eac before 0x5a5eb4): the unit's XSTR is the conditional form of D1701.
#undef XSTR
#define XSTR(text, id) (0 ? (char *)(text) : localize_strings[id])

#define LOCALIZE_TEXT_SIZE 0xa000         // the strings.tbl text buffer (0x25ce47c-0x25d847b)

// ---- .data (0x5a5c30-0x5a5d3f) ----------------------------------------------------------------------
static char localize_text_buffer[LOCALIZE_TEXT_SIZE];    // 0x25ce47c (name ours; defined here for the initialiser)
// sum.symbols: data:int "bytes used". It is the write pointer into localize_text_buffer (initialised
// 0x25ce47c, stored into localize_strings[id]; D3001).
char *localize_pool_used = localize_text_buffer;         // 0x5a5c30
// 0x5a5c34: 4 zero bytes, no reference.
localize_language_info localize_language_names[3] = {    // 0x5a5c38
    { "English", "" },
    { "German", "gr" },
    { "French", "fr" },
};
// The fixed topic pair of localize_load_level_table: [0] is the English key, [language] the translation.
static char localize_farewell[3][20] = {                 // 0x5a5d00 (name ours)
    "\"Farewell.\"",
    "\"Leb wohl.\"",
    "\"Au revoir.\"",
};
int localize_is_english = 1;                             // 0x5a5d3c (name ours) set by localize_set_language, never read

// ---- .bss (0x25c68b0-0x25d889b, address order) ------------------------------------------------------
// The three static objects in initialiser order (table 0x5887e0, 0x5887e4, 0x5887e8).
// 0x503f60 init_localize_pool_non_level (jmp thunk). Confidence: exact.
// 0x503f70 its body: localize_pool_non_level.Mempool() (jmp to the folded empty constructor). Confidence: exact.
Mempool localize_pool_non_level;                         // 0x25c68b0 "loc: non-level", buffer 0x25c9cdc
// 0x503f80 init_localize_pool_level (jmp thunk). Confidence: exact.
// 0x503f90 its body: localize_pool_level.Mempool(). Confidence: exact.
Mempool localize_pool_level;                             // 0x25c74c0 "loc: level", buffer 0x25c74dc
// 0x503fa0 init_localize_string_pool: 0x503fb0, then jmp 0x503fc0. Confidence: exact.
// 0x503fb0 localize_init_string_pool: StringPool::StringPool(0xa800). Confidence: exact.
// 0x503fc0 localize_string_pool_register_dtor: atexit(0x503fd0). Confidence: exact.
// 0x503fd0 localize_string_pool_dtor: localize_string_pool.~StringPool(). Confidence: exact.
StringPool localize_string_pool(0xa800);                 // 0x25c68d0 the non-level pool's strings
localize_node *localize_topic_table[LOCALIZE_TABLE_SIZE];   // 0x25c68e4
localize_node *localize_script_table[LOCALIZE_TABLE_SIZE];  // 0x25c6cd8
localize_node *localize_quest_table[LOCALIZE_TABLE_SIZE];   // 0x25c70cc
static char localize_level_buffer[0x2800];               // 0x25c74dc (name ours)
static char localize_non_level_buffer[0x3800];           // 0x25c9cdc (name ours)
char *localize_strings[1000];                            // 0x25cd4dc
localize_node *localize_item_table[LOCALIZE_TABLE_SIZE];    // 0x25d847c
int localize_french_punct;                               // 0x25d8870 (module - in sum.symbols; gr_font reads it)
int localize_is_german;                                  // 0x25d8874 (name ours) never read
int localize_language;                                   // 0x25d8878
static int localize_pools_created;                       // 0x25d887c (name ours) localize_init
int localize_strings_loaded;                             // 0x25d8880
int localize_level_loading;                              // 0x25d8884
int localize_names1_loaded;                              // 0x25d8888
int localize_names2_loaded;                              // 0x25d888c
int localize_level_loaded;                               // 0x25d8890
int localize_max_string_id;                              // 0x25d8894
Mempool *localize_current_pool;                          // 0x25d8898

// 0x503fe0  Confidence: exact (m-1020d w9, D16301: indexed with i, not a walking pointer, so the empty-string
// test reads text[0] apart and 8966 shrink-wraps ESI around the loop; the digits as text[i++] / text[i]; if/else
// chain in the compare order n, r, ", x; the hex buffer is "0x" + two digits, the 'x' stored from the register
// that holds it).
// \" keeps the quote, any other escaped character stays itself; \xHH reads exactly two digits.
void localize_unescape(char *text)
{
    int out = 0;
    int i;

    for (i = 0; text[i]; i++) {
        char c = text[i];
        if (c == '\\') {
            c = text[++i];
            if (c == 'n') {
                c = '\n';
            } else if (c == 'r') {
                c = '\r';
            } else if (c == '"') {
                c = '"';
            } else if (c == 'x') {
                char hex[8];
                i++;
                hex[0] = '0';
                hex[1] = c;
                hex[2] = text[i++];
                hex[3] = text[i];
                hex[4] = '\0';
                c = (char)strtol(hex, NULL, 16);
            }
        }
        text[out++] = c;
    }
    text[out] = '\0';
}

// 0x504070  Confidence: exact. No NULL check of the pool allocation.
char *localize_strdup(const char *text)
{
    if (localize_current_pool == NULL)
        return "INVALID";
    if (localize_current_pool == &localize_pool_non_level)
        return localize_string_pool.Add(text);
    char *p = (char *)localize_current_pool->allocate_in_mempool(strlen(text) + 1);
    strcpy(p, text);
    return p;
}

// 0x5040e0  Confidence: exact (the walk: the first node's test is duplicated by VC6's loop rotation).
// The node and the copy are allocated before the duplicate test, so a duplicate leaks both in the pool.
void localize_table_add(const char *text, int english_hash, localize_node **table)
{
    if (localize_current_pool == NULL)
        return;
    localize_node *node = (localize_node *)localize_current_pool->allocate_in_mempool(sizeof(localize_node));
    if (node == NULL)
        return;
    unsigned int bucket = (unsigned int)english_hash % LOCALIZE_TABLE_SIZE;
    node->text = localize_strdup(text);
    node->hash = english_hash;
    node->next = NULL;

    localize_node *p = table[bucket];
    if (p == NULL) {
        table[bucket] = node;
        return;
    }
    if (p->hash == english_hash && _stricmp(p->text, text) == 0)
        return;
    while (p->next != NULL) {
        p = p->next;
        if (p->hash == english_hash && _stricmp(p->text, text) == 0)
            return;
    }
    p->next = node;
}

// 0x504190  Confidence: exact.
void localize_set_string_raw(const char *text, int id)
{
    localize_strings[id] = localize_strdup(text);
}

// 0x5041b0  Confidence: exact.
char *localize_table_lookup(const char *english, localize_node **table)
{
    if (english == NULL)
        return "INVALID";
    int hash = localize_hash(english);
    localize_node *p;
    for (p = table[(unsigned int)hash % LOCALIZE_TABLE_SIZE]; p != NULL; p = p->next) {
        if (p->hash == hash)
            break;
    }
    if (p == NULL)
        return (char *)english;
    return p->text;
}

// 0x504200  Confidence: exact (rol 6 is the _rotl intrinsic; the character is sign-extended).
int localize_hash(const char *text)
{
    if (text == NULL)
        return -1;
    unsigned int hash = 0;
    for (; *text; text++)
        hash = _rotl(hash, 6) ^ *text;
    return (int)hash;
}

// 0x504230  Confidence: exact (VC6 byte match, match22; index loop for the lower-casing; was likely) (local order; the "#<language>" section name is lowered with a pointer loop).
// The unused ids after the last one all point at the "NULL Format String" entry.
void localize_load_strings()
{
    char section[64];
    char filename[128];

    strcpy(section, "#");
    strcat(section, localize_language_names[localize_language].name);
    for (int k = 0; section[k]; k++)
        section[k] = tolower(section[k]);
    strcpy(filename, "strings.tbl");

    TextFile tf;
    tf.Open(filename, PATH_ANY);
    tf.Rewind();
    tf.SkipTo(section, NULL);
    tf.Expect(section);
    while (!tf.Accept("#end"))
        localize_parse_string_line(&tf);
    tf.Close();

    localize_set_string("NULL Format String", localize_max_string_id + 1);
    for (int i = localize_max_string_id + 1; i < 1000; i++)
        localize_strings[i] = localize_strings[localize_max_string_id];
}

// 0x504400  Confidence: likely (the room test: the binary compares len with buffer end + 1 - pointer).
// The slot is set and localize_max_string_id stored before the room test, so a full buffer leaves the id
// pointing at unwritten text. Ids 935, 936, 942 and 944 skip the font remap.
char *localize_set_string(char *text, int id)
{
    char *dest = localize_pool_used;
    if (id >= 0)
        localize_strings[id] = dest;
    int len = strlen(text);
    localize_max_string_id = id;
    if (len >= localize_text_buffer + LOCALIZE_TEXT_SIZE + 1 - dest)
        return NULL;

    localize_unescape(text);
    switch (id) {
    case 935:
    case 936:
    case 942:
    case 944:
        break;
    default:
        localize_remap_charset(text, 1);
        break;
    }
    strcpy(dest, text);
    localize_pool_used += len + 1;
    return dest;
}

// 0x5044d0  Confidence: exact. "<id>, {text}"
void localize_parse_string_line(TextFile *tf)
{
    char text[0x400];
    int id = tf->ParseInt();
    tf->Expect(",");
    tf->ParseString(text, sizeof(text), '{', '}');
    localize_set_string(text, id);
}

// 0x504520  Confidence: likely (the switch with the default return before the TextFile is built; locals).
// "#english": <id>, "English" lines give the hash per id; the "#<language>" section then gives
// <id>, "translation". Neither part checks the id against the 1000 slots of the stack table.
void localize_load_names(int which)
{
    char section[64];
    char text[0x200];
    char filename[256];
    int hashes[1000];
    localize_node **table;
    char *p;

    switch (which) {
    case 1:
        table = localize_item_table;
        strcpy(filename, "item_names.tbl");
        break;
    case 2:
        table = localize_quest_table;
        strcpy(filename, "quest_names.tbl");
        break;
    default:
        return;
    }
    memset(hashes, 0xff, sizeof(hashes));

    TextFile tf;
    tf.Open(filename, PATH_ANY);
    tf.Rewind();
    tf.Expect("#english");
    while (!tf.Accept("#End")) {
        int id = tf.ParseInt();
        tf.Accept(",");
        tf.ParseString(text, sizeof(text), '"', '"');
        localize_unescape(text);
        localize_remap_charset(text, 1);
        hashes[id] = localize_hash(text);
    }

    strcpy(section, "#");
    strcat(section, localize_language_names[localize_language].name);
    for (p = section; *p; p++)
        *p = tolower(*p);
    tf.Rewind();
    tf.SkipTo(section, NULL);
    tf.Expect(section);
    while (!tf.Accept("#end")) {
        int id = tf.ParseInt();
        tf.Accept(",");
        tf.ParseString(text, sizeof(text), '"', '"');
        localize_unescape(text);
        localize_remap_charset(text, 1);
        localize_table_add(text, hashes[id], table);
    }
    tf.Close();
}

// 0x5047f0  Confidence: exact.
void localize_load_non_level()
{
    localize_current_pool = &localize_pool_non_level;
    localize_load_strings();
    localize_strings_loaded = 1;
    localize_load_names(1);
    localize_names1_loaded = 1;
    localize_load_names(2);
    localize_names2_loaded = 1;
}

// 0x504830  Confidence: exact. hashes is -1 terminated (localize_load_level_table's table is filled with -1).
void localize_add_if_missing(localize_node **table, int *hashes, const char *english, const char *translation)
{
    int hash = localize_hash(english);
    for (; *hashes != -1; hashes++) {
        if (*hashes == hash)
            return;
    }
    localize_table_add(translation, hash, table);
}

// 0x504870  Confidence: exact (m-1020d w9, D16302: the language name and both hash passes indexed with i, which
// 8966 strength-reduces after the first loop test as the binary has it; braces = false after the strcat in the
// script branch; if/else for the two ParseString forms, merged into one call by VC6).
// The section is "<level>_dlg.tbl" / "<level>_script.tbl" inside topic_names.tbl / script_names.tbl. English
// lines are taken in order (the id is parsed and ignored), so the n-th translation pairs with the n-th
// English line. Topics drop everything from a '#'. A missing section only prints.
// Defect: the section name is lowered with `i < section[i]` (the character, not the length), which stops
// early once the index reaches the character value (harmless for real level names).
void localize_load_level_table(const char *name, int type)
{
    char section[64];
    char lang[64];
    char filename[128];
    char text[0x200];
    int hashes[500];
    localize_node **table;
    bool braces;
    int i;

    if (type == LOCALIZE_LEVEL_TOPICS) {
        strcpy(filename, "topic_names.tbl");
        braces = true;
        strcpy(section, name);
        table = localize_topic_table;
        strcat(section, "_dlg.tbl");
    } else if (type == LOCALIZE_LEVEL_SCRIPTS) {
        strcpy(filename, "script_names.tbl");
        table = localize_script_table;
        strcpy(section, name);
        strcat(section, "_script.tbl");
        braces = false;
    } else {
        return;
    }
    for (i = 0; i < section[i]; i++)
        section[i] = tolower(section[i]);
    memset(hashes, 0xff, sizeof(hashes));

    TextFile tf;
    tf.Open(filename, PATH_ANY);
    tf.Rewind();
    tf.Expect("#english");
    if (tf.SkipTo(section, NULL)) {
        tf.SkipToNextLine();
        i = 0;
        while (!tf.Accept("#Stop File")) {
            tf.ParseInt();
            tf.Accept(",");
            if (braces)
                tf.ParseString(text, sizeof(text), '{', '}');
            else
                tf.ParseString(text, sizeof(text), '"', '"');
            localize_unescape(text);
            localize_remap_charset(text, 1);
            hashes[i++] = localize_hash(text);
        }

        strcpy(lang, "#");
        strcat(lang, localize_language_names[localize_language].name);
        for (i = 0; lang[i]; i++)
            lang[i] = tolower(lang[i]);
        tf.Rewind();
        tf.SkipTo(lang, NULL);
        tf.Expect(lang);
        tf.SkipTo(section, NULL);
        tf.SkipToNextLine();
        i = 0;
        while (!tf.Accept("#Stop File")) {
            tf.ParseInt();
            tf.Accept(",");
            if (braces)
                tf.ParseString(text, sizeof(text), '{', '}');
            else
                tf.ParseString(text, sizeof(text), '"', '"');
            if (type == LOCALIZE_LEVEL_TOPICS) {
                char *comment = strstr(text, "#");
                if (comment)
                    *comment = '\0';
            }
            localize_unescape(text);
            localize_remap_charset(text, 1);
            localize_table_add(text, hashes[i++], table);
        }
    } else {
        printf("!!!Not found!!! section: %s in file: %s\n", section, filename);
    }
    tf.Close();

    if (type == LOCALIZE_LEVEL_TOPICS) {
        localize_add_if_missing(table, hashes, localize_farewell[0], localize_farewell[localize_language]);
    } else if (type == LOCALIZE_LEVEL_SCRIPTS) {
        // The party and the summons keep their names when a level's table does not list them.
        localize_add_if_missing(table, hashes, "Joseph", "Joseph");
        localize_add_if_missing(table, hashes, "Jekhar", "Jekhar");
        localize_add_if_missing(table, hashes, "Flece", "Flece");
        localize_add_if_missing(table, hashes, "Rosalind", "Rosalind");
        localize_add_if_missing(table, hashes, "Golem", XSTR("Golem", 114));
        localize_add_if_missing(table, hashes, "Red Minotaur", XSTR("Red Minotaur", 116));
        localize_add_if_missing(table, hashes, "Blade of Urath", XSTR("Blade of Urath", 118));
        localize_add_if_missing(table, hashes, "Blue Imp", XSTR("Blue Imp", 120));
        localize_add_if_missing(table, hashes, "Blackfire Elemental", XSTR("Blackfire Elemental", 122));
        localize_add_if_missing(table, hashes, "Blood Elemental", XSTR("Blood Elemental", 124));
        localize_add_if_missing(table, hashes, "Poison Elemental", XSTR("Poison Elemental", 126));
        localize_add_if_missing(table, hashes, "Mist Elemental", XSTR("Mist Elemental", 128));
        localize_add_if_missing(table, hashes, "Wraith", XSTR("Wraith", 130));
        localize_add_if_missing(table, hashes, "Jade Golem", XSTR("Jade Golem", 132));
        // The key is "Spirit Of Laharah" (capital O), the string-table text "Spirit of Laharah".
        localize_add_if_missing(table, hashes, "Spirit Of Laharah", XSTR("Spirit of Laharah", 134));
        localize_add_if_missing(table, hashes, "Celestial Samurai", XSTR("Celestial Samurai", 136));
    }
}

// 0x504ed0  Confidence: exact.
void localize_clear_level()
{
    localize_level_loading = 0;
    localize_level_loaded = 0;
    localize_pool_level.ResetOffset();
    memset(localize_topic_table, 0, sizeof(localize_topic_table));
    memset(localize_script_table, 0, sizeof(localize_script_table));
}

// 0x504f10  Confidence: exact (localize_level_loading is set between the two loads; a store cannot move
// across the calls).
void localize_level_init(const char *script_name)
{
    localize_clear_level();
    localize_current_pool = &localize_pool_level;
    localize_load_level_table(script_name, LOCALIZE_LEVEL_TOPICS);
    localize_level_loading = 1;
    localize_load_level_table(script_name, LOCALIZE_LEVEL_SCRIPTS);
    localize_level_loaded = 1;
    localize_current_pool = NULL;
}

// 0x504f60  Confidence: exact (localize_strings_loaded is stored before the first memset).
void localize_init()
{
    localize_set_language(LANGUAGE_ENGLISH);
    if (!localize_pools_created) {
        localize_pool_non_level.Copy(localize_non_level_buffer, sizeof(localize_non_level_buffer), "loc: non-level");
        localize_pool_level.Copy(localize_level_buffer, sizeof(localize_level_buffer), "loc: level");
        localize_pools_created = 1;
    }
    localize_strings_loaded = 0;
    memset(localize_strings, 0, sizeof(localize_strings));
    memset(localize_topic_table, 0, sizeof(localize_topic_table));
    memset(localize_item_table, 0, sizeof(localize_item_table));
    memset(localize_quest_table, 0, sizeof(localize_quest_table));
    memset(localize_script_table, 0, sizeof(localize_script_table));
    localize_level_loading = 0;
    localize_level_loaded = 0;
    localize_names1_loaded = 0;
    localize_names2_loaded = 0;
    localize_load_non_level();
}

// 0x505020  Confidence: exact.
char *localize_topic_name(const char *name)
{
    return localize_table_lookup(name, localize_topic_table);
}

// 0x505040  Confidence: exact.
char *localize_item_name(const char *name)
{
    return localize_table_lookup(name, localize_item_table);
}

// 0x505060  Confidence: exact (xor eax, eax / ret; no reference anywhere, so the signature is unknown).
int localize_stub_ret0()
{
    return 0;
}

// 0x505070  Confidence: exact.
char *localize_quest_name(const char *name)
{
    return localize_table_lookup(name, localize_quest_table);
}

// 0x505090  Confidence: exact.
char *localize_script_name(const char *name)
{
    return localize_table_lookup(name, localize_script_table);
}

// 0x5050b0  Confidence: exact.
void localize_set_language(int language)
{
    localize_language = language;
    printf("Setting language to %s\n", localize_language_names[language].name);
    localize_french_punct = 0;
    localize_is_german = 0;
    localize_is_english = 0;
    switch (language) {
    case LANGUAGE_ENGLISH:
        localize_is_english = 1;
        break;
    case LANGUAGE_GERMAN:
        localize_is_german = 1;
        break;
    case LANGUAGE_FRENCH:
        localize_french_punct = 1;
        break;
    }
}

// 0x505120  Confidence: exact.
void localize_get_language_name_buf(char *out)
{
    strcpy(out, localize_language_names[localize_language].name);
}

// 0x505160  Confidence: exact.
void localize_get_language_code_buf(char *out)
{
    strcpy(out, localize_language_names[localize_language].code);
}

// 0x5051a0  Confidence: exact (m-1020d w9, D16301: the three loops indexed with i, so the first test reads
// text[0] before the walking register is set up; case order inside each switch follows the jump targets).
// German: to_font != 0 maps Windows-1252 to the font's CP437 positions, 0 maps back (ß goes to 0xbc but
// comes back from 0xe1). French has one direction only (to_font is ignored), so French text read back from
// the font is not converted; capitals with accents become the lower-case glyphs, several go to 0xff, and Ä
// (0xc4) becomes 0x90 (É in CP437).
char *localize_remap_charset(char *text, int to_font)
{
    int i;

    switch (localize_language) {
    case LANGUAGE_FRENCH:
        for (i = 0; text[i] != 0; i++) {
            switch (text[i]) {
            case '\xe1': text[i] = '\xa0'; break;
            case '\xe0': text[i] = '\x85'; break;
            case '\xe2': text[i] = '\x83'; break;
            case '\xe5': text[i] = '\x86'; break;
            case '\xc5': text[i] = '\x8f'; break;
            case '\xeb': text[i] = '\x89'; break;
            case '\xea': text[i] = '\x88'; break;
            case '\xc9':
            case '\xe9': text[i] = '\x82'; break;
            case '\xc8':
            case '\xe8': text[i] = '\x8a'; break;
            case '\xc4': text[i] = '\x90'; break;
            case '\xef': text[i] = '\x8b'; break;
            case '\xee': text[i] = '\x8c'; break;
            case '\xf2': text[i] = '\x95'; break;
            case '\xf3': text[i] = '\xa2'; break;
            case '\xf4': text[i] = '\x93'; break;
            case '\xfa': text[i] = '\xa3'; break;
            case '\xf9': text[i] = '\x97'; break;
            case '\xfb': text[i] = '\x96'; break;
            case '\xc0':
            case '\xc1':
            case '\xc2':
            case '\xca':
            case '\xce':
            case '\xcf':
            case '\xd9':
            case '\xda':
            case '\xdb': text[i] = '\xff'; break;
            case '\xe7': text[i] = '\x87'; break;
            case '\xc7': text[i] = '\x80'; break;
            case '\xe6': text[i] = '\x91'; break;
            case '\xc6': text[i] = '\x92'; break;
            case '\x9c': text[i] = '\x9b'; break;
            case '\x8c': text[i] = '\x9c'; break;
            }
        }
        return text;
    case LANGUAGE_GERMAN:
        if (!to_font) {
            for (i = 0; text[i] != 0; i++) {
                switch (text[i]) {
                case '\x81': text[i] = '\xfc'; break;
                case '\x84': text[i] = '\xe4'; break;
                case '\x94': text[i] = '\xf6'; break;
                case '\x9a': text[i] = '\xdc'; break;
                case '\x8e': text[i] = '\xc4'; break;
                case '\x99': text[i] = '\xd6'; break;
                case '\xe1': text[i] = '\xdf'; break;
                }
            }
        } else {
            for (i = 0; text[i] != 0; i++) {
                switch (text[i]) {
                case '\xfc': text[i] = '\x81'; break;
                case '\xe4': text[i] = '\x84'; break;
                case '\xf6': text[i] = '\x94'; break;
                case '\xdc': text[i] = '\x9a'; break;
                case '\xc4': text[i] = '\x8e'; break;
                case '\xd6': text[i] = '\x99'; break;
                case '\xdf': text[i] = '\xbc'; break;
                }
            }
        }
        return text;
    }
    return text;
}

// 0x5054e0  Confidence: exact.
char **localize_get_string_ptr(int id)
{
    return &localize_strings[id];
}

// 0x5054f0  Confidence: exact.
char *localize_string_or_invalid(char **p)
{
    if (p != NULL && *p != NULL)
        return *p;
    return "INVALID";
}

// 0x505510  Confidence: exact.
char *localize_get_language_name()
{
    return localize_language_names[localize_language].name;
}

// 0x505530 vector3__vector3_default: the empty Mempool() COMDAT of this /Ob0 object (the two pool
// initialisers jump to it), folded by the linker with every other empty constructor. Defined inline in
// vsdk/os/memory.h (D1801); no code here.
// 0x505540 Mempool__ResetOffset: the COMDAT of the inline in vsdk/os/memory.h that localize_clear_level
// calls (sum.symbols module vsdk\os). No code here.
