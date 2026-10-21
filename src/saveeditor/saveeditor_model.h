/* saveeditor_model: the editable state behind SummonerSaveEditor.exe, kept apart from the window code so a
 * test program (app_test.c) can drive it without clicking.
 *
 * A sem_doc holds an opened save, the values read from it (orig) and the values the player has changed
 * (cur). sem_build_ops turns the difference into the library's edit list (se_op, the savedit command-line
 * options); sem_save_new applies that list all or nothing on a copy of the save and writes a NEW file. The
 * opened file and the document are never changed by saving.
 *
 * Strings from the save and the tables are Latin-1 bytes, as in saveedit.h. */
#ifndef SAVEEDITOR_MODEL_H
#define SAVEEDITOR_MODEL_H

#include "saveedit.h"

#define SEM_MAX_MEMBERS 16

/* member fields, in the order of the edit options */
enum { SEM_F_LEVEL, SEM_F_XP, SEM_F_HP, SEM_F_MAXHP, SEM_F_AP, SEM_F_MAXAP, SEM_F_SKILLPOINTS, SEM_NFIELDS };
extern const se_op_kind sem_field_op[SEM_NFIELDS];
extern const char *const sem_field_label[SEM_NFIELDS];

typedef struct {
    char field[SEM_NFIELDS][64];   /* as text, as the edit options take them */
    uint8_t skills[SE_SKILL_COUNT];
} sem_member_vals;

typedef struct { uint32_t hash; int64_t count; } sem_item;

typedef struct {
    char desc[256];
    char gold[64];
    sem_member_vals m[SEM_MAX_MEMBERS];
    int ninv, invcap;
    sem_item *inv;                 /* one row per item type held (counts of repeated entries added up) */
    uint8_t *flags[SE_LEVEL_COUNT];/* one byte per flag, 0 or 1 (only when have_blocks) */
} sem_state;

typedef struct {
    char label[64];                /* "Joseph", or "member 2" for a record without a known name */
    int record, dead, skillpoints_ok;
} sem_member_info;

typedef struct {
    se_save save;
    const se_tables *t;            /* borrowed; may have no items or no events */
    se_header hdr;
    int nmembers;
    sem_member_info mi[SEM_MAX_MEMBERS];
    uint32_t *types;               /* the save's item types (ITEM chunk) */
    int ntypes;
    int have_blocks;               /* story flags readable (the tables reproduce the level-data size) */
    se_level_block blocks[SE_LEVEL_COUNT];
    char flags_why[256];           /* why not, when have_blocks is 0 */
    sem_state orig, cur;
} sem_doc;

/* Opens and reads a save (the file is only read). t may be NULL. On failure *d is left empty. */
int sem_open(sem_doc *d, const wchar_t *path, const se_tables *t, char *err, size_t errlen);
int sem_open_mem(sem_doc *d, const void *data, size_t size, const se_tables *t, char *err, size_t errlen);
void sem_close(sem_doc *d);
void sem_revert(sem_doc *d);       /* cur = orig */

/* Changes (to cur only). Return 0, or -1 with a message. */
int sem_set_desc(sem_doc *d, const char *latin1);
int sem_set_gold(sem_doc *d, const char *text);
int sem_set_field(sem_doc *d, int member, int field, const char *text);
int sem_set_skill(sem_doc *d, int member, int id, int value);
int sem_inv_find(const sem_state *st, uint32_t hash);              /* row or -1 */
int sem_inv_set(sem_doc *d, uint32_t hash, int64_t count);         /* 0 removes the row */
int sem_inv_add(sem_doc *d, uint32_t hash, int64_t count);         /* more of a held type, or a new row */
int sem_set_flag(sem_doc *d, int level, int index, int value);

/* Item and flag names as the library looks them up; NULL when the tables do not give one. */
const char *sem_item_name(const sem_doc *d, uint32_t hash);
int sem_item_editable(const sem_doc *d, uint32_t hash);   /* a name that hashes back to the type, so the edit can name it */
int sem_is_type(const sem_doc *d, uint32_t hash);         /* one of the save's item types */
int sem_flag_count(const sem_doc *d, int level);
const char *sem_flag_name(const sem_doc *d, int level, int index);
int sem_flag_editable(const sem_doc *d, int level, int index);

/* The edit list for cur against orig. Order: description, gold, then per changed member "--member I" and
 * its fields and skills, then inventory counts (rows of the save in their order, then new rows), then story
 * flags (level order, flag order). Values are malloc'd; sem_ops_free. Returns the number of ops or -1. */
typedef struct { se_op *ops; int n, cap; } sem_ops;
int sem_build_ops(const sem_doc *d, sem_ops *out, char *err, size_t errlen);
void sem_ops_free(sem_ops *o);
int sem_change_count(const sem_ops *o);   /* ops other than --member */

/* Applies the edit list to a copy of the save and writes it to path, which must not exist and must not be
 * the opened file (the caller checks the latter; se_write_new refuses an existing file). Nothing is written
 * when an edit is refused. log receives the library's change lines. */
int sem_save_new(const sem_doc *d, const wchar_t *path, char *log, size_t loglen, char *err, size_t errlen);

/* The first summonerN.sav (N = 0, 1, ...) that does not exist in dir (the game's slot names). */
int sem_next_slot(const wchar_t *dir, wchar_t *out, size_t outlen);

/* Latin-1 <-> UTF-16. sem_to_latin1 fails (-1) on a character above U+00FF or when out is too small. */
int sem_to_latin1(const wchar_t *w, char *out, size_t outlen);
void sem_from_latin1(const char *s, wchar_t *out, size_t outlen);
/* The shortest decimal (no exponent) that reads back as f. */
void sem_fmt_float(float f, char *out, size_t outlen);

#endif
