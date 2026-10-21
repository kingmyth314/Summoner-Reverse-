/* saveedit: Summoner v1.40 savegame reader and editor (a C port of tools/lib/savedata.mjs and the edit
 * engine of tools/savedit.mjs; format: docs/sum-exe/savegame.md).
 *
 * The library works on a save held in memory (se_save). Loading checks the save; edits are given as a list
 * of (kind, text value) pairs exactly as on the savedit command line and are applied all or nothing: every
 * edit is checked on a working copy first, and the save is changed only when all of them succeed. Writing
 * always creates a NEW file and refuses an existing one.
 *
 * Strings that come from or go into the save or the tables are Latin-1 byte strings (char *), as in the
 * Node tools. File paths are wide strings (wchar_t *): the library is meant for a Windows app.
 *
 * Every function that can fail returns 0 on success and -1 on failure, and fills err (errlen bytes) with a
 * message when err is not NULL.
 */
#ifndef SAVEEDIT_H
#define SAVEEDIT_H

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SE_SAVE_SIZE 270755
#define SE_LEVEL_COUNT 51
#define SE_SECTION_COUNT 13
#define SE_INVT_MAX_ENTRIES 0x209
#define SE_SKILL_COUNT 36
#define SE_MAX_QUESTS 200
#define SE_MAX_WARNINGS 8

/* level_script_table order (index = the save's level index) and the section size table order. */
extern const char *const se_level_names[SE_LEVEL_COUNT];
extern const char *const se_section_names[SE_SECTION_COUNT];

/* name_hash_nocase: Crc32 (table polynomial 0xedb88320, start 0, no final inversion) of the lower-cased
 * name. se_name_hash takes a NUL-terminated Latin-1 string, se_name_hash_n a length. */
uint32_t se_name_hash(const char *s);
uint32_t se_name_hash_n(const char *s, size_t n);

/* ---------------------------------------------------------------------------------------------------- */
/* Tables (items.tbl, events.tbl, <level>.tbl). Sources, first match wins per file: an explicit file
 * (items_tbl, events_tbl), a folder of extracted .tbl files (tables_dir), the packfile (tables_vpp). Any
 * of them may be NULL. lang NULL means "english". */
typedef struct {
    const wchar_t *tables_vpp;
    const wchar_t *tables_dir;
    const wchar_t *items_tbl;
    const wchar_t *events_tbl;
    const char *lang;
} se_table_opts;

typedef struct { uint32_t hash; char *name; } se_item_name;

typedef struct {
    const char *name;              /* = se_level_names[index] */
    int nflags, nints, nfloats;
    char **flags, **ints, **floats; /* names in events.tbl order */
    double layers;                 /* automap layer count (largest $AutomapPC: number; 0 without the table) */
} se_level_def;

typedef struct {
    int index;
    char *name;
    int main;
    char *invalidate;              /* +Invalidate: flag name, or NULL */
} se_quest;

typedef struct {
    char lang[64];
    int have_items;                /* items.tbl parsed */
    int nitems;
    se_item_name *items;           /* hash -> name, first-insertion order, a later duplicate replaces the name */
    int have_events;               /* events.tbl parsed: levels and quests are valid */
    se_level_def levels[SE_LEVEL_COUNT];
    int nquests;
    se_quest *quests;
    char *stopped;                 /* unknown $Level: name that ended the parse, or NULL */
    int nmissing;                  /* level tables not found */
    int nwarnings;
    char warnings[SE_MAX_WARNINGS][256];
} se_tables;

/* Never fails: what cannot be read is left out and reported in warnings (as loadTables does). */
void se_tables_load(se_tables *t, const se_table_opts *opts);
void se_tables_free(se_tables *t);
const char *se_item_name_of(const se_tables *t, uint32_t hash); /* NULL when unknown */

/* ---------------------------------------------------------------------------------------------------- */
/* Saves */
typedef struct {
    unsigned char *data;
    size_t size;
} se_save;

/* Returns NULL when the save looks right (SAVg tag, version 39, size table adding up to the file size),
 * else a message (in msg, msglen bytes). */
const char *se_check(const unsigned char *data, size_t size, char *msg, size_t msglen);

int se_load_mem(se_save *s, const void *data, size_t size, char *err, size_t errlen);  /* copies, checks */
int se_load_file(se_save *s, const wchar_t *path, char *err, size_t errlen);           /* reads, checks */
void se_free(se_save *s);
/* Creates path (refused if it exists: never overwrites) and writes the save. */
int se_write_new(const se_save *s, const wchar_t *path, char *err, size_t errlen);

/* Section offsets in buffer order (index = size-table index, as se_section_names); end = header + all. */
typedef struct { int64_t at[SE_SECTION_COUNT]; int64_t end; } se_sections;
void se_section_offsets(const se_save *s, se_sections *out);
/* Offset of the chunk with that tag (INVT, ITEM, LENT, CMRA, CONT) when the tag is there, else -1. */
int64_t se_chunk_at(const se_save *s, const char *tag);

typedef struct {
    int32_t version;
    char level[0x81];
    char script[0x21];
    float level_time;
    char description[0x101];
    int32_t sizes[SE_SECTION_COUNT];
} se_header;
int se_read_header(const se_save *s, se_header *h);

typedef struct { uint32_t hash; int32_t count; int32_t field; } se_inv_entry;
typedef struct {
    int32_t gold;
    int count;
    se_inv_entry *entries;         /* malloc'd; se_inventory_free */
    int length_ok;                 /* trailing chunk length right (else the game refuses the save) */
} se_inventory;
int se_read_inventory(const se_save *s, se_inventory *inv, char *err, size_t errlen);
void se_inventory_free(se_inventory *inv);

/* ITEM chunk: the item types of the save in table order (index = equipment value). *hashes is malloc'd. */
int se_item_types(const se_save *s, uint32_t **hashes, int *count, char *err, size_t errlen);

/* LENT records */
typedef struct { int index; int64_t at; uint32_t hash; int32_t type; } se_entity;
typedef struct {
    int64_t at;                    /* LENT chunk offset */
    uint32_t party_state;
    int count;
    se_entity *list;               /* malloc'd; se_entities_free */
} se_entities;
int se_read_entities(const se_save *s, se_entities *e, char *err, size_t errlen);
void se_entities_free(se_entities *e);

typedef struct {
    int record;                    /* index in the LENT list */
    int64_t at;                    /* record offset (hash dword); the fields start at at + 4 */
    uint32_t hash;
    const char *name;              /* Joseph, Flece, Jekhar, Rosalind, or NULL */
    int16_t level;
    int32_t xp;
    float hp, maxhp;
    int16_t ap, maxap;
    int32_t skillpoints;
    int skillpoints_ok;            /* 0 when the record ends before the skill-point dword */
    int dead;
    uint8_t skills[SE_SKILL_COUNT];
    int32_t equip[9];              /* item-type index per slot, -1 = empty */
} se_member;
/* Party members (type 7) in record order; writes at most max, returns the number found (or -1). */
int se_party(const se_save *s, se_member *out, int max, char *err, size_t errlen);

/* Per-level data blocks; refused (-1) unless the tables reproduce the save's level-data size exactly. */
typedef struct {
    int index;
    const char *name;
    int64_t offset;                /* flag bits start here */
    int64_t size;
    int64_t ints_at;               /* int32 variables, then floats */
    int32_t visits;
    const se_level_def *level;
} se_level_block;
int se_level_blocks(const se_save *s, const se_tables *t, se_level_block out[SE_LEVEL_COUNT], char *err, size_t errlen);
int se_get_flag(const se_save *s, const se_level_block *b, int i);
int32_t se_level_int(const se_save *s, const se_level_block *b, int i);
float se_level_float(const se_save *s, const se_level_block *b, int i);
int se_quest_bit(const se_save *s, int n);
/* Level index by name (case-insensitive) or decimal index; -1 if none. */
int se_find_level(const char *name);

/* ---------------------------------------------------------------------------------------------------- */
/* Edits: the savedit command-line options, values as text. */
typedef enum {
    SE_OP_DESC, SE_OP_GOLD, SE_OP_ADD_ITEM, SE_OP_REMOVE_ITEM, SE_OP_SET_ITEM_COUNT, SE_OP_FLAG,
    SE_OP_MEMBER, SE_OP_LEVEL, SE_OP_XP, SE_OP_HP, SE_OP_MAXHP, SE_OP_AP, SE_OP_MAXAP, SE_OP_SKILLPOINTS,
    SE_OP_SKILL, SE_OP_COUNT
} se_op_kind;
typedef struct { se_op_kind kind; const char *value; } se_op;

/* "desc", "gold", "add-item", ... (the option name without "--"); -1 if not an edit option. */
int se_op_kind_from_name(const char *name);
const char *se_op_name(se_op_kind k);

/* Applies the edits in order, all or nothing. log (loglen bytes, may be NULL) receives one line per
 * change. Rules as savedit.mjs: --desc and --gold once; the member fields and each --skill id once per
 * member; at least one edit other than --member. */
int se_apply(se_save *s, const se_tables *t, const se_op *ops, int nops,
             char *log, size_t loglen, char *err, size_t errlen);

#ifdef __cplusplus
}
#endif
#endif
