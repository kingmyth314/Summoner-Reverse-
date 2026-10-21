/* saveeditor_model.c: see saveeditor_model.h. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "saveeditor_model.h"

const se_op_kind sem_field_op[SEM_NFIELDS] = {
    SE_OP_LEVEL, SE_OP_XP, SE_OP_HP, SE_OP_MAXHP, SE_OP_AP, SE_OP_MAXAP, SE_OP_SKILLPOINTS};
const char *const sem_field_label[SEM_NFIELDS] = {
    "Level", "XP", "HP", "Max HP", "AP", "Max AP", "Skill points"};

static void seterr(char *err, size_t n, const char *fmt, ...)
{
    va_list ap;
    if (!err || !n) return;
    va_start(ap, fmt);
    vsnprintf(err, n, fmt, ap);
    va_end(ap);
}

/* JavaScript's trim() white space within Latin-1, and the line ends the option patterns reject */
static int js_ws(unsigned char c) { return c == ' ' || (c >= 0x09 && c <= 0x0d) || c == 0xa0; }
static int clean_name(const char *s)
{
    size_t n = s ? strlen(s) : 0, i;
    if (!n || js_ws((unsigned char)s[0]) || js_ws((unsigned char)s[n - 1])) return 0;
    for (i = 0; i < n; i++) if (s[i] == '\n' || s[i] == '\r') return 0;
    return 1;
}

void sem_fmt_float(float f, char *out, size_t outlen)
{
    int p;
    if (f != f) { snprintf(out, outlen, "nan"); return; }
    if ((double)f > 3.5e38 || (double)f < -3.5e38) { snprintf(out, outlen, f > 0 ? "inf" : "-inf"); return; }
    for (p = 0; p <= 12; p++) {
        snprintf(out, outlen, "%.*f", p, (double)f);
        if ((float)strtod(out, NULL) == f) return;
    }
}

int sem_to_latin1(const wchar_t *w, char *out, size_t outlen)
{
    size_t i, n = wcslen(w);
    if (n + 1 > outlen) return -1;
    for (i = 0; i < n; i++) {
        if (w[i] > 0xff) return -1;
        out[i] = (char)(unsigned char)w[i];
    }
    out[n] = 0;
    return 0;
}
void sem_from_latin1(const char *s, wchar_t *out, size_t outlen)
{
    size_t i = 0;
    if (!outlen) return;
    for (; s[i] && i + 1 < outlen; i++) out[i] = (wchar_t)(unsigned char)s[i];
    out[i] = 0;
}

/* ---------------------------------------------------------------------------------------------------- */
static void state_free(sem_state *st)
{
    int i;
    free(st->inv);
    for (i = 0; i < SE_LEVEL_COUNT; i++) free(st->flags[i]);
    memset(st, 0, sizeof *st);
}
static int state_copy(sem_state *dst, const sem_state *src, const sem_doc *d)
{
    int i;
    memset(dst, 0, sizeof *dst);
    memcpy(dst->desc, src->desc, sizeof dst->desc);
    memcpy(dst->gold, src->gold, sizeof dst->gold);
    memcpy(dst->m, src->m, sizeof dst->m);
    dst->invcap = src->ninv + 16;
    dst->inv = malloc((size_t)dst->invcap * sizeof(sem_item));
    if (!dst->inv) return -1;
    dst->ninv = src->ninv;
    if (src->ninv) memcpy(dst->inv, src->inv, (size_t)src->ninv * sizeof(sem_item));
    for (i = 0; i < SE_LEVEL_COUNT; i++) {
        int n = d->have_blocks ? d->blocks[i].level->nflags : 0;
        if (!src->flags[i] || n <= 0) continue;
        dst->flags[i] = malloc((size_t)n);
        if (!dst->flags[i]) return -1;
        memcpy(dst->flags[i], src->flags[i], (size_t)n);
    }
    return 0;
}

void sem_close(sem_doc *d)
{
    state_free(&d->orig);
    state_free(&d->cur);
    free(d->types);
    se_free(&d->save);
    memset(d, 0, sizeof *d);
}

static int read_doc(sem_doc *d, char *err, size_t errlen)
{
    se_member pm[SEM_MAX_MEMBERS];
    se_inventory inv;
    int i, n, k;
    char why[256];
    sem_state *o = &d->orig;

    if (se_read_header(&d->save, &d->hdr)) { seterr(err, errlen, "cannot read the save header"); return -1; }
    memcpy(o->desc, d->hdr.description, 255);
    o->desc[255] = 0;

    if (se_read_inventory(&d->save, &inv, why, sizeof why)) { seterr(err, errlen, "inventory: %s", why); return -1; }
    snprintf(o->gold, sizeof o->gold, "%ld", (long)inv.gold);
    o->invcap = inv.count + 16;
    o->inv = malloc((size_t)o->invcap * sizeof(sem_item));
    if (!o->inv) { se_inventory_free(&inv); seterr(err, errlen, "out of memory"); return -1; }
    for (i = 0; i < inv.count; i++) {
        int r = sem_inv_find(o, inv.entries[i].hash);
        if (r >= 0) o->inv[r].count += inv.entries[i].count;
        else { o->inv[o->ninv].hash = inv.entries[i].hash; o->inv[o->ninv].count = inv.entries[i].count; o->ninv++; }
    }
    se_inventory_free(&inv);

    if (se_item_types(&d->save, &d->types, &d->ntypes, why, sizeof why)) { seterr(err, errlen, "item types: %s", why); return -1; }

    n = se_party(&d->save, pm, SEM_MAX_MEMBERS, why, sizeof why);
    if (n < 0) { seterr(err, errlen, "party: %s", why); return -1; }
    if (n > SEM_MAX_MEMBERS) n = SEM_MAX_MEMBERS;
    d->nmembers = n;
    for (i = 0; i < n; i++) {
        sem_member_info *mi = &d->mi[i];
        sem_member_vals *mv = &o->m[i];
        if (pm[i].name) snprintf(mi->label, sizeof mi->label, "%s", pm[i].name);
        else snprintf(mi->label, sizeof mi->label, "member %d (0x%08lx)", i, (unsigned long)pm[i].hash);
        mi->record = pm[i].record;
        mi->dead = pm[i].dead;
        mi->skillpoints_ok = pm[i].skillpoints_ok;
        snprintf(mv->field[SEM_F_LEVEL], 64, "%d", (int)pm[i].level);
        snprintf(mv->field[SEM_F_XP], 64, "%ld", (long)pm[i].xp);
        sem_fmt_float(pm[i].hp, mv->field[SEM_F_HP], 64);
        sem_fmt_float(pm[i].maxhp, mv->field[SEM_F_MAXHP], 64);
        snprintf(mv->field[SEM_F_AP], 64, "%d", (int)pm[i].ap);
        snprintf(mv->field[SEM_F_MAXAP], 64, "%d", (int)pm[i].maxap);
        snprintf(mv->field[SEM_F_SKILLPOINTS], 64, "%ld", (long)pm[i].skillpoints);
        memcpy(mv->skills, pm[i].skills, SE_SKILL_COUNT);
    }

    d->have_blocks = 0;
    if (!d->t || !d->t->have_events) snprintf(d->flags_why, sizeof d->flags_why, "events.tbl is not loaded (tables.vpp missing?)");
    else if (se_level_blocks(&d->save, d->t, d->blocks, why, sizeof why)) snprintf(d->flags_why, sizeof d->flags_why, "%s", why);
    else d->have_blocks = 1;
    if (d->have_blocks)
        for (i = 0; i < SE_LEVEL_COUNT; i++) {
            int nf = d->blocks[i].level->nflags;
            if (nf <= 0) continue;
            o->flags[i] = malloc((size_t)nf);
            if (!o->flags[i]) { seterr(err, errlen, "out of memory"); return -1; }
            for (k = 0; k < nf; k++) o->flags[i][k] = (uint8_t)(se_get_flag(&d->save, &d->blocks[i], k) ? 1 : 0);
        }
    if (state_copy(&d->cur, o, d)) { seterr(err, errlen, "out of memory"); return -1; }
    return 0;
}

int sem_open_mem(sem_doc *d, const void *data, size_t size, const se_tables *t, char *err, size_t errlen)
{
    memset(d, 0, sizeof *d);
    if (se_load_mem(&d->save, data, size, err, errlen)) return -1;
    d->t = t;
    if (read_doc(d, err, errlen)) { sem_close(d); return -1; }
    return 0;
}

int sem_open(sem_doc *d, const wchar_t *path, const se_tables *t, char *err, size_t errlen)
{
    memset(d, 0, sizeof *d);
    if (se_load_file(&d->save, path, err, errlen)) return -1;
    d->t = t;
    if (read_doc(d, err, errlen)) { sem_close(d); return -1; }
    return 0;
}

void sem_revert(sem_doc *d)
{
    sem_state fresh;
    if (state_copy(&fresh, &d->orig, d)) { state_free(&fresh); return; }
    state_free(&d->cur);
    d->cur = fresh;
}

/* ---------------------------------------------------------------------------------------------------- */
int sem_set_desc(sem_doc *d, const char *s)
{
    if (strlen(s) > 255) return -1;
    snprintf(d->cur.desc, sizeof d->cur.desc, "%s", s);
    return 0;
}
int sem_set_gold(sem_doc *d, const char *text)
{
    if (strlen(text) >= sizeof d->cur.gold) return -1;
    snprintf(d->cur.gold, sizeof d->cur.gold, "%s", text);
    return 0;
}
int sem_set_field(sem_doc *d, int member, int field, const char *text)
{
    if (member < 0 || member >= d->nmembers || field < 0 || field >= SEM_NFIELDS || strlen(text) >= 64) return -1;
    snprintf(d->cur.m[member].field[field], 64, "%s", text);
    return 0;
}
int sem_set_skill(sem_doc *d, int member, int id, int value)
{
    if (member < 0 || member >= d->nmembers || id < 0 || id >= SE_SKILL_COUNT || value < 0 || value > 255) return -1;
    d->cur.m[member].skills[id] = (uint8_t)value;
    return 0;
}

int sem_inv_find(const sem_state *st, uint32_t hash)
{
    int i;
    for (i = 0; i < st->ninv; i++) if (st->inv[i].hash == hash) return i;
    return -1;
}
int sem_is_type(const sem_doc *d, uint32_t hash)
{
    int i;
    for (i = 0; i < d->ntypes; i++) if (d->types[i] == hash) return 1;
    return 0;
}
int sem_inv_set(sem_doc *d, uint32_t hash, int64_t count)
{
    sem_state *st = &d->cur;
    int r = sem_inv_find(st, hash);
    if (count < 0 || count > 0x7fffffff) return -1;
    if (r >= 0) {
        if (count) st->inv[r].count = count;
        else { memmove(st->inv + r, st->inv + r + 1, (size_t)(st->ninv - r - 1) * sizeof(sem_item)); st->ninv--; }
        return 0;
    }
    if (!count) return 0;
    if (!sem_is_type(d, hash)) return -1;
    if (st->ninv == st->invcap) {
        sem_item *p = realloc(st->inv, (size_t)(st->invcap * 2 + 16) * sizeof(sem_item));
        if (!p) return -1;
        st->inv = p;
        st->invcap = st->invcap * 2 + 16;
    }
    st->inv[st->ninv].hash = hash;
    st->inv[st->ninv].count = count;
    st->ninv++;
    return 0;
}
int sem_inv_add(sem_doc *d, uint32_t hash, int64_t count)
{
    int r = sem_inv_find(&d->cur, hash);
    if (count <= 0) return -1;
    return sem_inv_set(d, hash, (r >= 0 ? d->cur.inv[r].count : 0) + count);
}

int sem_flag_count(const sem_doc *d, int level)
{
    if (!d->have_blocks || level < 0 || level >= SE_LEVEL_COUNT) return 0;
    return d->blocks[level].level->nflags;
}
const char *sem_flag_name(const sem_doc *d, int level, int index)
{
    if (index < 0 || index >= sem_flag_count(d, level)) return NULL;
    return d->blocks[level].level->flags[index];
}
int sem_flag_editable(const sem_doc *d, int level, int index)
{
    const char *nm = sem_flag_name(d, level, index);
    uint32_t h;
    int i;
    if (!clean_name(nm)) return 0;
    h = se_name_hash(nm);
    /* the library finds a flag by the hash of its name: only the first flag with that hash can be named */
    for (i = 0; i < index; i++) if (se_name_hash(d->blocks[level].level->flags[i]) == h) return 0;
    return 1;
}
int sem_set_flag(sem_doc *d, int level, int index, int value)
{
    if (index < 0 || index >= sem_flag_count(d, level) || !d->cur.flags[level]) return -1;
    d->cur.flags[level][index] = (uint8_t)(value ? 1 : 0);
    return 0;
}

const char *sem_item_name(const sem_doc *d, uint32_t hash)
{
    return d->t ? se_item_name_of(d->t, hash) : NULL;
}
int sem_item_editable(const sem_doc *d, uint32_t hash)
{
    const char *nm = sem_item_name(d, hash);
    return clean_name(nm) && se_name_hash(nm) == hash && sem_is_type(d, hash);
}

/* ---------------------------------------------------------------------------------------------------- */
void sem_ops_free(sem_ops *o)
{
    int i;
    for (i = 0; i < o->n; i++) free((char *)o->ops[i].value);
    free(o->ops);
    memset(o, 0, sizeof *o);
}
static int push(sem_ops *o, se_op_kind k, const char *fmt, ...)
{
    va_list ap;
    int n;
    char *s;
    if (o->n == o->cap) {
        se_op *p = realloc(o->ops, (size_t)(o->cap * 2 + 16) * sizeof(se_op));
        if (!p) return -1;
        o->ops = p;
        o->cap = o->cap * 2 + 16;
    }
    va_start(ap, fmt);
    n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0 || !(s = malloc((size_t)n + 1))) return -1;
    va_start(ap, fmt);
    vsnprintf(s, (size_t)n + 1, fmt, ap);
    va_end(ap);
    o->ops[o->n].kind = k;
    o->ops[o->n].value = s;
    o->n++;
    return 0;
}
int sem_change_count(const sem_ops *o)
{
    int i, c = 0;
    for (i = 0; i < o->n; i++) if (o->ops[i].kind != SE_OP_MEMBER) c++;
    return c;
}

static int64_t held(const sem_state *st, uint32_t h)
{
    int r = sem_inv_find(st, h);
    return r >= 0 ? st->inv[r].count : 0;
}

int sem_build_ops(const sem_doc *d, sem_ops *o, char *err, size_t errlen)
{
    const sem_state *a = &d->orig, *b = &d->cur;
    int i, k;
    memset(o, 0, sizeof *o);
#define PUSH(...) do { if (push(o, __VA_ARGS__)) { sem_ops_free(o); seterr(err, errlen, "out of memory"); return -1; } } while (0)
    if (strcmp(a->desc, b->desc)) PUSH(SE_OP_DESC, "%s", b->desc);
    if (strcmp(a->gold, b->gold)) PUSH(SE_OP_GOLD, "%s", b->gold);
    for (i = 0; i < d->nmembers; i++) {
        int marked = 0;
        for (k = 0; k < SEM_NFIELDS; k++) {
            if (!strcmp(a->m[i].field[k], b->m[i].field[k])) continue;
            if (!marked) { PUSH(SE_OP_MEMBER, "%d", i); marked = 1; }
            PUSH(sem_field_op[k], "%s", b->m[i].field[k]);
        }
        for (k = 0; k < SE_SKILL_COUNT; k++) {
            if (a->m[i].skills[k] == b->m[i].skills[k]) continue;
            if (!marked) { PUSH(SE_OP_MEMBER, "%d", i); marked = 1; }
            PUSH(SE_OP_SKILL, "%d=%d", k, (int)b->m[i].skills[k]);
        }
    }
    /* inventory: a changed total of a held type, then the new types; each as --set-item-count Name=N */
    for (k = 0; k < 2; k++) {
        const sem_state *rows = k == 0 ? a : b;
        for (i = 0; i < rows->ninv; i++) {
            uint32_t h = rows->inv[i].hash;
            int64_t was = held(a, h), now = held(b, h);
            if (k == 1 && sem_inv_find(a, h) >= 0) continue;
            if (was == now) continue;
            if (!sem_item_editable(d, h)) {
                sem_ops_free(o);
                seterr(err, errlen, "item 0x%08lx has no usable name in the tables, so its count cannot be changed", (unsigned long)h);
                return -1;
            }
            PUSH(SE_OP_SET_ITEM_COUNT, "%s=%lld", sem_item_name(d, h), (long long)now);
        }
    }
    if (d->have_blocks)
        for (i = 0; i < SE_LEVEL_COUNT; i++) {
            int nf = sem_flag_count(d, i);
            if (!a->flags[i] || !b->flags[i]) continue;
            for (k = 0; k < nf; k++) {
                if (a->flags[i][k] == b->flags[i][k]) continue;
                if (!sem_flag_editable(d, i, k)) {
                    sem_ops_free(o);
                    seterr(err, errlen, "flag %d of level %s cannot be named for the editor (empty or repeated name)", k, se_level_names[i]);
                    return -1;
                }
                PUSH(SE_OP_FLAG, "%d:%s=%d", i, sem_flag_name(d, i, k), (int)b->flags[i][k]);
            }
        }
#undef PUSH
    return o->n;
}

int sem_save_new(const sem_doc *d, const wchar_t *path, char *log, size_t loglen, char *err, size_t errlen)
{
    sem_ops o;
    se_save copy;
    int rc;
    if (log && loglen) log[0] = 0;
    if (sem_build_ops(d, &o, err, errlen) < 0) return -1;
    if (!sem_change_count(&o)) { sem_ops_free(&o); seterr(err, errlen, "there are no changes to save"); return -1; }
    if (se_load_mem(&copy, d->save.data, d->save.size, err, errlen)) { sem_ops_free(&o); return -1; }
    rc = se_apply(&copy, d->t, o.ops, o.n, log, loglen, err, errlen);
    if (rc == 0) rc = se_write_new(&copy, path, err, errlen);
    se_free(&copy);
    sem_ops_free(&o);
    return rc;
}

int sem_next_slot(const wchar_t *dir, wchar_t *out, size_t outlen)
{
    int n;
    for (n = 0; n < 100000; n++) {
        int w = _snwprintf(out, outlen, L"%ls%lssummoner%d.sav", dir, (dir[0] && dir[wcslen(dir) - 1] != L'\\') ? L"\\" : L"", n);
        if (w < 0 || (size_t)w >= outlen) return -1;
        if (GetFileAttributesW(out) == INVALID_FILE_ATTRIBUTES) return GetLastError() == ERROR_FILE_NOT_FOUND ? n : -1;
    }
    return -1;
}
