/* app_test.c: tests the save editor's "changes -> edit list" logic (saveeditor_model.c) without the window.
 *
 * usage (from the repository folder): app_test --tables FILE.vpp --out DIR save.sav [save.sav ...]
 *
 * For each save, each case changes fields of a document the way the window does (sem_set_*, sem_inv_*), saves
 * it with sem_save_new (edit list -> se_apply -> new file), runs node tools/savedit.mjs on the same input with
 * the equivalent command line written out by hand, and compares the two outputs byte for byte. Refused and
 * no-change cases must write nothing (and node must refuse the same edit). The input save must be unchanged
 * afterwards. Names of items and flags come from each save and the tables at run time. Outputs go to DIR; a
 * case's two outputs are deleted (by their exact paths) when it passed. Exit code 0 when every case passed. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "saveeditor_model.h"

static const wchar_t *g_tables, *g_out;
static se_tables g_t, g_none; /* g_none: no tables at all (tables.vpp missing) */
static int g_pass, g_fail, g_skip;

/* ---- command line for node ------------------------------------------------------------------------- */
typedef struct { wchar_t *a[64]; int n; } args;
static void arg(args *v, const wchar_t *s) { if (v->n < 64) v->a[v->n++] = _wcsdup(s); }
static void argl(args *v, const char *latin1) /* a Latin-1 value as the UTF-16 string node turns back into it */
{
    wchar_t w[1024];
    sem_from_latin1(latin1, w, 1024);
    arg(v, w);
}
static void argf(args *v, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    argl(v, buf);
}
static void args_free(args *v) { int i; for (i = 0; i < v->n; i++) free(v->a[i]); v->n = 0; }

/* CommandLineToArgvW quoting */
static void quote(wchar_t *cl, size_t cap, const wchar_t *s)
{
    size_t n = wcslen(cl), bs = 0;
    const wchar_t *p;
    if (n && n + 1 < cap) cl[n++] = L' ';
    if (n + 1 < cap) cl[n++] = L'"';
    for (p = s; *p && n + 4 < cap; p++) {
        if (*p == L'\\') { bs++; cl[n++] = L'\\'; continue; }
        if (*p == L'"') { while (bs-- && n + 2 < cap) cl[n++] = L'\\'; cl[n++] = L'\\'; }
        bs = 0;
        cl[n++] = *p;
    }
    while (bs-- && n + 2 < cap) cl[n++] = L'\\';
    if (n + 1 < cap) cl[n++] = L'"';
    cl[n] = 0;
}

static int run_node(const wchar_t *in, const wchar_t *out, const args *v)
{
    static wchar_t cl[32768];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    DWORD code = 99;
    int i;
    HANDLE nul;
    SECURITY_ATTRIBUTES sa = {sizeof sa, NULL, TRUE};
    cl[0] = 0;
    quote(cl, 32768, L"node");
    quote(cl, 32768, L"tools\\savedit.mjs");
    quote(cl, 32768, in);
    quote(cl, 32768, out);
    for (i = 0; i < v->n; i++) quote(cl, 32768, v->a[i]);
    quote(cl, 32768, L"--tables");
    quote(cl, 32768, g_tables);
    nul = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, OPEN_EXISTING, 0, NULL);
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = nul;
    si.hStdError = nul;
    if (!CreateProcessW(NULL, cl, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        fwprintf(stderr, L"cannot start node (error %lu)\n", GetLastError());
        CloseHandle(nul);
        return 98;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(nul);
    return (int)code;
}

static int exists(const wchar_t *p) { return GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES; }

static unsigned char *read_all(const wchar_t *p, size_t *n)
{
    FILE *f = _wfopen(p, L"rb");
    unsigned char *b;
    long L;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    L = ftell(f);
    fseek(f, 0, SEEK_SET);
    b = malloc((size_t)L + 1);
    if (b && fread(b, 1, (size_t)L, f) != (size_t)L) { free(b); b = NULL; }
    fclose(f);
    *n = (size_t)L;
    return b;
}

/* ---- one case -------------------------------------------------------------------------------------- */
/* expect: 1 = both write identical files, 0 = both refuse and write nothing, -1 = the app has no changes
 * (nothing to compare with node) */
static void check(const wchar_t *in, const wchar_t *sname, const char *cname, sem_doc *d, args *v, int expect)
{
    wchar_t a[MAX_PATH * 2], b[MAX_PATH * 2];
    char log[16384], err[1024];
    sem_ops o;
    int nops, rc, nrc = 0, ok = 1;
    _snwprintf(a, MAX_PATH * 2, L"%ls\\app-%ls-%hs.sav", g_out, sname, cname);
    _snwprintf(b, MAX_PATH * 2, L"%ls\\node-%ls-%hs.sav", g_out, sname, cname);
    a[MAX_PATH * 2 - 1] = b[MAX_PATH * 2 - 1] = 0;
    if (exists(a) || exists(b)) { wprintf(L"FAIL %ls %hs: an output already exists\n", sname, cname); g_fail++; args_free(v); return; }
    nops = sem_build_ops(d, &o, err, sizeof err);
    if (nops >= 0) sem_ops_free(&o);
    rc = sem_save_new(d, a, log, sizeof log, err, sizeof err);
    if (expect == -1) {
        if (nops != 0 || rc == 0 || exists(a)) { ok = 0; wprintf(L"FAIL %ls %hs: expected no changes, got %d ops (%hs)\n", sname, cname, nops, err); }
    } else {
        nrc = run_node(in, b, v);
        if (expect == 1) {
            size_t na = 0, nb = 0;
            unsigned char *x = rc == 0 ? read_all(a, &na) : NULL, *y = nrc == 0 ? read_all(b, &nb) : NULL;
            if (rc) { ok = 0; wprintf(L"FAIL %ls %hs: app refused: %hs\n", sname, cname, err); }
            if (nrc) { ok = 0; wprintf(L"FAIL %ls %hs: node exit %d\n", sname, cname, nrc); }
            if (x && y && (na != nb || memcmp(x, y, na))) {
                size_t i = 0;
                while (i < na && i < nb && x[i] == y[i]) i++;
                ok = 0;
                wprintf(L"FAIL %ls %hs: outputs differ at 0x%lx\n", sname, cname, (unsigned long)i);
            }
            if (x && na == d->save.size && !memcmp(x, d->save.data, na)) {
                ok = 0;
                wprintf(L"FAIL %ls %hs: the output is the unchanged input\n", sname, cname);
            }
            free(x); free(y);
        } else {
            if (rc == 0 || exists(a)) { ok = 0; wprintf(L"FAIL %ls %hs: app accepted a bad edit\n", sname, cname); }
            if (nrc == 0 || exists(b)) { ok = 0; wprintf(L"FAIL %ls %hs: node accepted it (exit %d)\n", sname, cname, nrc); }
        }
    }
    /* the opened save in memory is the file's bytes still */
    {
        size_t n = 0;
        unsigned char *f = read_all(in, &n);
        if (!f || n != d->save.size || memcmp(f, d->save.data, n)) { ok = 0; wprintf(L"FAIL %ls %hs: input or document changed\n", sname, cname); }
        free(f);
    }
    if (ok) {
        g_pass++;
        if (expect == 1) { DeleteFileW(a); DeleteFileW(b); }
        wprintf(L"pass %ls %hs (%d ops)\n", sname, cname, nops);
    } else g_fail++;
    args_free(v);
}

/* ---- the cases ------------------------------------------------------------------------------------- */
typedef struct {
    int held1, held2, dup;         /* rows of editable held items (-1 = none); dup: a type held in 2+ entries */
    uint32_t new1, new2;           /* editable item types the party does not hold (0 = none) */
    int flagA_level, flagA, flagB_level, flagB;
} picks;

static void pick(const sem_doc *d, picks *p)
{
    int i, k;
    se_inventory inv;
    memset(p, -1, sizeof *p);
    p->new1 = p->new2 = 0;
    for (i = 0; i < d->orig.ninv; i++) {
        if (!sem_item_editable(d, d->orig.inv[i].hash)) continue;
        if (p->held1 < 0) p->held1 = i; else if (p->held2 < 0) p->held2 = i;
    }
    if (se_read_inventory(&d->save, &inv, NULL, 0) == 0) {
        for (i = 0; i < inv.count && p->dup < 0; i++)
            for (k = i + 1; k < inv.count; k++)
                if (inv.entries[k].hash == inv.entries[i].hash && sem_item_editable(d, inv.entries[i].hash)) {
                    p->dup = sem_inv_find(&d->orig, inv.entries[i].hash);
                    break;
                }
        se_inventory_free(&inv);
    }
    for (i = 0; i < d->ntypes; i++) {
        uint32_t h = d->types[i];
        if (sem_inv_find(&d->orig, h) >= 0 || !sem_item_editable(d, h) || h == p->new1) continue;
        if (!p->new1) p->new1 = h; else if (!p->new2) { p->new2 = h; break; }
    }
    if (d->have_blocks) {
        int cur = se_find_level(d->hdr.level);
        for (i = 0; i < SE_LEVEL_COUNT; i++) {
            int L = (cur >= 0 ? cur + i : i) % SE_LEVEL_COUNT;
            for (k = 0; k < sem_flag_count(d, L); k++) {
                if (!sem_flag_editable(d, L, k)) continue;
                if (p->flagA < 0) { p->flagA_level = L; p->flagA = k; }
                else if (L != p->flagA_level && p->flagB < 0) { p->flagB_level = L; p->flagB = k; }
                break;
            }
            if (p->flagB >= 0) break;
        }
    }
}

static void c_general(sem_doc *d, args *v)
{
    char g[32];
    sem_set_desc(d, "App test \xe9");
    snprintf(g, sizeof g, "%lld", atoll(d->orig.gold) + 1234);
    sem_set_gold(d, g);
    arg(v, L"--desc"); argl(v, "App test \xe9");
    arg(v, L"--gold"); argl(v, g);
}
static void c_member0(sem_doc *d, args *v)
{
    char s[32];
    snprintf(s, sizeof s, "%d", atoi(d->orig.m[0].field[SEM_F_LEVEL]) + 1);
    sem_set_field(d, 0, SEM_F_LEVEL, s); arg(v, L"--level"); argl(v, s);
    snprintf(s, sizeof s, "%ld", atol(d->orig.m[0].field[SEM_F_XP]) + 100);
    sem_set_field(d, 0, SEM_F_XP, s); arg(v, L"--xp"); argl(v, s);
    sem_set_field(d, 0, SEM_F_HP, "12.5"); arg(v, L"--hp"); arg(v, L"12.5");
    sem_set_field(d, 0, SEM_F_MAXHP, "250"); arg(v, L"--maxhp"); arg(v, L"250");
}
static void c_member_last(sem_doc *d, args *v)
{
    int m = d->nmembers - 1, s3 = (d->orig.m[m].skills[3] + 5) % 256;
    const char *nm = d->mi[m].label;
    char s[16];
    arg(v, L"--member");
    if (!strcmp(nm, "Joseph") || !strcmp(nm, "Flece") || !strcmp(nm, "Jekhar") || !strcmp(nm, "Rosalind")) argl(v, nm);
    else argf(v, "%d", m);
    sem_set_field(d, m, SEM_F_AP, "7"); arg(v, L"--ap"); arg(v, L"7");
    sem_set_field(d, m, SEM_F_MAXAP, "9"); arg(v, L"--maxap"); arg(v, L"9");
    if (d->mi[m].skillpoints_ok) { sem_set_field(d, m, SEM_F_SKILLPOINTS, "3"); arg(v, L"--skillpoints"); arg(v, L"3"); }
    sem_set_skill(d, m, 3, s3); arg(v, L"--skill"); snprintf(s, sizeof s, "3=%d", s3); argl(v, s);
    sem_set_skill(d, m, 35, 255); arg(v, L"--skill"); arg(v, L"35=255");
}
static int c_inventory(sem_doc *d, args *v, const picks *p)
{
    int any = 0;
    if (p->held1 >= 0) {
        uint32_t h = d->orig.inv[p->held1].hash;
        int64_t n = d->orig.inv[p->held1].count + 2;
        sem_inv_set(d, h, n);
        arg(v, L"--set-item-count"); argf(v, "%s=%lld", sem_item_name(d, h), (long long)n);
        any = 1;
    }
    if (p->held2 >= 0) {
        uint32_t h = d->orig.inv[p->held2].hash;
        sem_inv_set(d, h, 0);
        arg(v, L"--remove-item"); argl(v, sem_item_name(d, h));
        any = 1;
    }
    if (p->new1) {
        sem_inv_add(d, p->new1, 1);
        sem_inv_add(d, p->new1, 2);   /* the window's Add twice: one row of 3 */
        arg(v, L"--add-item"); argf(v, "%s=3", sem_item_name(d, p->new1));
        any = 1;
    }
    if (p->new2) {
        sem_inv_set(d, p->new2, 1);
        arg(v, L"--add-item"); argl(v, sem_item_name(d, p->new2));
        any = 1;
    }
    return any;
}
static int c_flags(sem_doc *d, args *v, const picks *p)
{
    int i, L[2] = {p->flagA_level, p->flagB_level}, F[2] = {p->flagA, p->flagB}, any = 0;
    for (i = 0; i < 2; i++) {
        int nv;
        if (F[i] < 0) continue;
        nv = !d->orig.flags[L[i]][F[i]];
        sem_set_flag(d, L[i], F[i], nv);
        arg(v, L"--flag"); argf(v, "%s:%s=%d", se_level_names[L[i]], sem_flag_name(d, L[i], F[i]), nv);
        any = 1;
    }
    return any;
}

static void run_save(const wchar_t *path)
{
    sem_doc d;
    char err[512];
    picks p;
    args v = {{0}, 0};
    const wchar_t *sname = path, *q;
    wchar_t base[MAX_PATH];
    for (q = path; *q; q++) if (*q == L'\\' || *q == L'/') sname = q + 1;
    _snwprintf(base, MAX_PATH, L"%ls", sname);
    base[MAX_PATH - 1] = 0;
    if (wcsrchr(base, L'.')) *wcsrchr(base, L'.') = 0;
    if (sem_open(&d, path, &g_t, err, sizeof err)) { wprintf(L"FAIL %ls: cannot open: %hs\n", base, err); g_fail++; return; }
    pick(&d, &p);
    wprintf(L"-- %ls: %d members, %d item rows, %d types, flags %hs\n", base, d.nmembers, d.orig.ninv, d.ntypes,
            d.have_blocks ? "readable" : d.flags_why);

    c_general(&d, &v); check(path, base, "general", &d, &v, 1); sem_revert(&d);
    if (d.nmembers > 0) { c_member0(&d, &v); check(path, base, "member0", &d, &v, 1); sem_revert(&d); }
    if (d.nmembers > 1) { c_member_last(&d, &v); check(path, base, "member-last", &d, &v, 1); sem_revert(&d); }
    else g_skip++;
    if (c_inventory(&d, &v, &p)) check(path, base, "inventory", &d, &v, 1); else { args_free(&v); g_skip++; }
    sem_revert(&d);
    if (p.dup >= 0) {
        uint32_t h = d.orig.inv[p.dup].hash;
        int64_t n = d.orig.inv[p.dup].count + 1;
        sem_inv_set(&d, h, n);
        arg(&v, L"--set-item-count"); argf(&v, "%s=%lld", sem_item_name(&d, h), (long long)n);
        check(path, base, "inventory-repeated-entries", &d, &v, 1);
        sem_revert(&d);
    }
    if (c_flags(&d, &v, &p)) check(path, base, "flags", &d, &v, 1); else { args_free(&v); g_skip++; }
    sem_revert(&d);

    /* everything at once: the command line in the app's order of kinds */
    c_general(&d, &v);
    if (d.nmembers > 0) c_member0(&d, &v);
    if (d.nmembers > 1) c_member_last(&d, &v);
    c_inventory(&d, &v, &p);
    c_flags(&d, &v, &p);
    check(path, base, "all", &d, &v, 1);
    sem_revert(&d);

    /* refused: a bad gold value with a good description; nothing written by either */
    sem_set_desc(&d, "refused");
    sem_set_gold(&d, "12x");
    arg(&v, L"--desc"); arg(&v, L"refused"); arg(&v, L"--gold"); arg(&v, L"12x");
    check(path, base, "refused-gold", &d, &v, 0);
    sem_revert(&d);
    if (d.nmembers > 0) {
        sem_set_field(&d, 0, SEM_F_HP, "1e3");
        arg(&v, L"--hp"); arg(&v, L"1e3");
        check(path, base, "refused-hp", &d, &v, 0);
        sem_revert(&d);
    }

    /* changed and changed back: no edits, nothing written */
    sem_set_gold(&d, "1");
    sem_set_gold(&d, d.orig.gold);
    if (d.nmembers > 0) { sem_set_skill(&d, 0, 5, (d.orig.m[0].skills[5] + 1) % 256); sem_set_skill(&d, 0, 5, d.orig.m[0].skills[5]); }
    if (p.held1 >= 0) { sem_inv_set(&d, d.orig.inv[p.held1].hash, 0); sem_inv_set(&d, d.orig.inv[p.held1].hash, d.orig.inv[p.held1].count); }
    if (p.flagA >= 0) { sem_set_flag(&d, p.flagA_level, p.flagA, 1); sem_set_flag(&d, p.flagA_level, p.flagA, d.orig.flags[p.flagA_level][p.flagA]); }
    check(path, base, "changed-back", &d, &v, -1);
    sem_close(&d);

    /* without tables: description, gold and party still edit; no item can be named, no flags */
    if (sem_open(&d, path, &g_none, err, sizeof err)) { wprintf(L"FAIL %ls no-tables: cannot open: %hs\n", base, err); g_fail++; return; }
    {
        int i, bad = d.have_blocks;
        for (i = 0; i < d.ntypes; i++) if (sem_item_editable(&d, d.types[i])) bad = 1;
        if (bad) { wprintf(L"FAIL %ls no-tables: items or flags editable without tables\n", base); g_fail++; }
    }
    c_general(&d, &v);
    if (d.nmembers > 0) c_member0(&d, &v);
    check(path, base, "no-tables", &d, &v, 1);
    sem_close(&d);
}

int wmain(int argc, wchar_t **argv)
{
    int i, first = 0;
    se_table_opts to;
    wchar_t slot[MAX_PATH * 2];
    for (i = 1; i < argc; i++) {
        if (!wcscmp(argv[i], L"--tables") && i + 1 < argc) g_tables = argv[++i];
        else if (!wcscmp(argv[i], L"--out") && i + 1 < argc) g_out = argv[++i];
        else { first = i; break; }
    }
    if (!g_tables || !g_out || !first) { fputs("usage: app_test --tables FILE.vpp --out DIR save.sav [...]\n", stderr); return 2; }
    if (GetFileAttributesW(L"tools\\savedit.mjs") == INVALID_FILE_ATTRIBUTES) { fputs("run this from the repository folder\n", stderr); return 2; }
    memset(&to, 0, sizeof to);
    to.tables_vpp = g_tables;
    se_tables_load(&g_t, &to);
    memset(&to, 0, sizeof to);
    se_tables_load(&g_none, &to);
    for (i = 0; i < g_t.nwarnings; i++) printf("note: %s\n", g_t.warnings[i]);

    /* slot naming: the first free summonerN.sav in the output folder */
    if (sem_next_slot(g_out, slot, MAX_PATH * 2) < 0) { wprintf(L"FAIL next slot\n"); g_fail++; }
    else wprintf(L"next free slot in the output folder: %ls\n", wcsrchr(slot, L'\\') + 1);

    for (i = first; i < argc; i++) run_save(argv[i]);
    se_tables_free(&g_t);
    se_tables_free(&g_none);
    wprintf(L"app_test: %d pass, %d fail, %d skipped (case not applicable)\n", g_pass, g_fail, g_skip);
    return g_fail ? 1 : 0;
}
