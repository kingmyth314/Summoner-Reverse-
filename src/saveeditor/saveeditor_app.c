/* SummonerSaveEditor - a savegame editor for players, on top of the saveedit library (saveedit.h) through the
 * document model of saveeditor_model.h.
 *
 * It sits in the game folder next to Sum.exe: it lists the saves of the savegame folder there and reads the
 * names of items and story flags from tables.vpp in the same folder (without it, description, gold and party
 * can still be edited), and the skill names from Sum.exe (without it, skills are shown by id). Every change becomes one edit of the library's edit list; saving applies the whole
 * list all or nothing and writes a NEW file (the next free summonerN.sav, or a name picked with Save As).
 * The opened save is never written.
 *
 * Command line (all optional): --game-dir DIR (use DIR instead of the program's folder), --tables FILE.vpp
 * (instead of tables.vpp in the game folder), --open FILE.SAV */
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include "saveeditor_model.h"

#define APP_TITLE L"Summoner Save Editor"
#define NOTICE L"Changes are written to a new save; your saves are never overwritten."
#define PATHLEN (MAX_PATH * 4)

enum {
    PAGE_SAVES, PAGE_GENERAL, PAGE_PARTY, PAGE_INVENTORY, PAGE_FLAGS, PAGE_COUNT, PAGE_ALL = 99
};
enum {
    ID_TAB = 100, ID_SAVEDIR, ID_SAVELIST, ID_OPEN, ID_OPENOTHER, ID_REFRESH, ID_TABLESINFO,
    ID_FILEINFO, ID_DESC, ID_GOLD,
    ID_MEMBERS, ID_FIELD0, ID_SKILLS = ID_FIELD0 + SEM_NFIELDS, ID_SKILLVAL, ID_SKILLSET, ID_MEMBERINFO,
    ID_INV, ID_INVCOUNT, ID_INVSET, ID_INVREMOVE, ID_ADDITEM, ID_ADDCOUNT, ID_ADD, ID_INVINFO,
    ID_LEVELS, ID_FLAGS, ID_FLAGINFO,
    ID_NOTICE, ID_STATUS, ID_SAVESLOT, ID_SAVEAS, ID_UNDO, ID_LEVELINFO
};

static HINSTANCE g_inst;
static HWND g_wnd, g_tab;
static HFONT g_font;
static int g_page;
static int g_filling;              /* controls are being filled from the document: ignore their notifications */

#define MAX_CTLS 96
static struct { HWND h; int page; } g_ctl[MAX_CTLS];
static int g_nctl;

static wchar_t g_gameDir[PATHLEN], g_saveDir[PATHLEN], g_tablesPath[PATHLEN];
static int g_tablesOk;
static char g_skillNames[SE_SKILL_COUNT][SEM_SKILL_NAME_LEN]; /* from the game's Sum.exe; empty without it */
static int g_haveSkillNames;
static se_tables g_tables;

static sem_doc g_doc;
static int g_docOpen;
static wchar_t g_docPath[PATHLEN];
static int g_member = -1, g_level = -1;

typedef struct { wchar_t path[PATHLEN]; wchar_t name[MAX_PATH]; char desc[256]; char level[0x81]; FILETIME time; int ok; } save_row;
static save_row *g_saves;
static int g_nsaves;

static HWND H(int id) { return GetDlgItem(g_wnd, id); }

static HWND ctl(int page, DWORD ex, const wchar_t *cls, const wchar_t *text, DWORD style, int x, int y, int w, int h, int id)
{
    HWND c = CreateWindowExW(ex, cls, text, WS_CHILD | style, x, y, w, h, g_wnd, (HMENU)(INT_PTR)id, g_inst, NULL);
    SendMessageW(c, WM_SETFONT, (WPARAM)g_font, TRUE);
    if (g_nctl < MAX_CTLS) { g_ctl[g_nctl].h = c; g_ctl[g_nctl].page = page; g_nctl++; }
    if (page == PAGE_ALL || page == g_page) ShowWindow(c, SW_SHOW);
    return c;
}
static HWND label(int page, const wchar_t *text, int x, int y, int w, int h, int id)
{
    return ctl(page, 0, L"STATIC", text, 0, x, y, w, h, id);
}
static HWND button(int page, const wchar_t *text, int x, int y, int w, int id)
{
    return ctl(page, 0, L"BUTTON", text, WS_TABSTOP | BS_PUSHBUTTON, x, y, w, 26, id);
}
static HWND edit(int page, int x, int y, int w, int id, DWORD extra)
{
    return ctl(page, WS_EX_CLIENTEDGE, L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL | extra, x, y, w, 23, id);
}
static HWND listview(int page, int x, int y, int w, int h, int id, DWORD exstyle)
{
    HWND lv = ctl(page, WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, x, y, w, h, id);
    ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT | exstyle);
    return lv;
}
static void column(HWND lv, int i, const wchar_t *text, int width)
{
    LVCOLUMNW c;
    memset(&c, 0, sizeof c);
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    c.pszText = (wchar_t *)text;
    c.cx = width;
    ListView_InsertColumn(lv, i, &c);
}
static int lv_add(HWND lv, const wchar_t *text, LPARAM data)
{
    LVITEMW it;
    memset(&it, 0, sizeof it);
    it.mask = LVIF_TEXT | LVIF_PARAM;
    it.iItem = ListView_GetItemCount(lv);
    it.pszText = (wchar_t *)text;
    it.lParam = data;
    return ListView_InsertItem(lv, &it);
}
static void lv_set(HWND lv, int row, int col, const wchar_t *text) { ListView_SetItemText(lv, row, col, (wchar_t *)text); }
static LPARAM lv_data(HWND lv, int row)
{
    LVITEMW it;
    memset(&it, 0, sizeof it);
    it.mask = LVIF_PARAM;
    it.iItem = row;
    if (!ListView_GetItem(lv, &it)) return -1;
    return it.lParam;
}
static int lv_sel(HWND lv) { return ListView_GetNextItem(lv, -1, LVNI_SELECTED); }

static void show_page(int page)
{
    int i;
    g_page = page;
    for (i = 0; i < g_nctl; i++)
        if (g_ctl[i].page != PAGE_ALL) ShowWindow(g_ctl[i].h, g_ctl[i].page == page ? SW_SHOW : SW_HIDE);
    RedrawWindow(g_wnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

static void status(const wchar_t *fmt, ...)
{
    wchar_t buf[1024];
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf(buf, 1024, fmt, ap);
    va_end(ap);
    buf[1023] = 0;
    SetWindowTextW(H(ID_STATUS), buf);
}
static void msg(UINT icon, const wchar_t *fmt, ...)
{
    static wchar_t buf[70000];
    va_list ap;
    va_start(ap, fmt);
    _vsnwprintf(buf, 70000, fmt, ap);
    va_end(ap);
    buf[69999] = 0;
    MessageBoxW(g_wnd, buf, APP_TITLE, MB_OK | icon);
}
static void w_of(const char *latin1, wchar_t *out, size_t n) { sem_from_latin1(latin1, out, n); }

static int pending(void)
{
    sem_ops o;
    char err[256];
    int n;
    if (!g_docOpen) return 0;
    if (sem_build_ops(&g_doc, &o, err, sizeof err) < 0) return 1;
    n = sem_change_count(&o);
    sem_ops_free(&o);
    return n;
}
static const wchar_t *file_part(const wchar_t *p) { const wchar_t *s = wcsrchr(p, L'\\'); return s ? s + 1 : p; }
static void folder_of(const wchar_t *p, wchar_t *out, size_t n)
{
    wchar_t *s;
    _snwprintf(out, n, L"%ls", p);
    out[n - 1] = 0;
    s = wcsrchr(out, L'\\');
    if (s) *s = 0; else _snwprintf(out, n, L".");
}

static void update_slot_button(void)
{
    wchar_t dir[PATHLEN], slot[PATHLEN], text[PATHLEN];
    if (!g_docOpen) { SetWindowTextW(H(ID_SAVESLOT), L"Save as new slot"); return; }
    folder_of(g_docPath, dir, PATHLEN);
    if (sem_next_slot(dir, slot, PATHLEN) < 0) _snwprintf(text, PATHLEN, L"Save as new slot (no free name)");
    else _snwprintf(text, PATHLEN, L"Save as %ls", file_part(slot));
    text[PATHLEN - 1] = 0;
    SetWindowTextW(H(ID_SAVESLOT), text);
}

static void update_status(void)
{
    sem_ops o;
    char err[512];
    wchar_t w[512];
    int n;
    EnableWindow(H(ID_SAVESLOT), g_docOpen);
    EnableWindow(H(ID_SAVEAS), g_docOpen);
    EnableWindow(H(ID_UNDO), g_docOpen);
    if (!g_docOpen) { status(L"Open a save on the Saves page."); return; }
    if (sem_build_ops(&g_doc, &o, err, sizeof err) < 0) { w_of(err, w, 512); status(L"%ls", w); return; }
    n = sem_change_count(&o);
    sem_ops_free(&o);
    if (n) status(L"%ls: %d change%ls not saved yet.", file_part(g_docPath), n, n == 1 ? L"" : L"s");
    else status(L"%ls: no changes.", file_part(g_docPath));
}

/* ---- Saves page -------------------------------------------------------------------------------------- */
static int cmp_time(const void *a, const void *b)
{
    return -CompareFileTime(&((const save_row *)a)->time, &((const save_row *)b)->time);
}
static void refresh_saves(void)
{
    wchar_t pat[PATHLEN], w[512];
    WIN32_FIND_DATAW fd;
    HANDLE fh;
    HWND lv = H(ID_SAVELIST);
    int i, cap = 0;
    free(g_saves);
    g_saves = NULL;
    g_nsaves = 0;
    ListView_DeleteAllItems(lv);
    _snwprintf(pat, PATHLEN, L"%ls\\*.sav", g_saveDir);
    pat[PATHLEN - 1] = 0;
    fh = FindFirstFileW(pat, &fd);
    if (fh != INVALID_HANDLE_VALUE) {
        do {
            save_row *r;
            se_save s;
            se_header h;
            char err[256];
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            if (g_nsaves == cap) {
                save_row *p = realloc(g_saves, (size_t)(cap * 2 + 32) * sizeof(save_row));
                if (!p) break;
                g_saves = p;
                cap = cap * 2 + 32;
            }
            r = &g_saves[g_nsaves++];
            memset(r, 0, sizeof *r);
            _snwprintf(r->path, PATHLEN, L"%ls\\%ls", g_saveDir, fd.cFileName);
            r->path[PATHLEN - 1] = 0;
            _snwprintf(r->name, MAX_PATH, L"%ls", fd.cFileName);
            r->name[MAX_PATH - 1] = 0;
            r->time = fd.ftLastWriteTime;
            if (se_load_file(&s, r->path, err, sizeof err) == 0) {
                if (se_read_header(&s, &h) == 0) {
                    memcpy(r->desc, h.description, 255);
                    memcpy(r->level, h.level, 0x80);
                    r->ok = 1;
                }
                se_free(&s);
            } else snprintf(r->desc, sizeof r->desc, "(cannot be read: %s)", err);
        } while (FindNextFileW(fh, &fd));
        FindClose(fh);
    }
    if (g_nsaves) qsort(g_saves, (size_t)g_nsaves, sizeof(save_row), cmp_time);
    for (i = 0; i < g_nsaves; i++) {
        FILETIME lt;
        SYSTEMTIME st;
        int row = lv_add(lv, g_saves[i].name, i);
        w_of(g_saves[i].desc, w, 512); lv_set(lv, row, 1, w);
        w_of(g_saves[i].level, w, 512); lv_set(lv, row, 2, w);
        FileTimeToLocalFileTime(&g_saves[i].time, &lt);
        FileTimeToSystemTime(&lt, &st);
        _snwprintf(w, 512, L"%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
        lv_set(lv, row, 3, w);
    }
    {
        wchar_t t[PATHLEN + 64];
        _snwprintf(t, PATHLEN + 64, L"Saves in %ls (%d), newest first. Double-click one to open it.", g_saveDir, g_nsaves);
        t[PATHLEN + 63] = 0;
        SetWindowTextW(H(ID_SAVEDIR), t);
    }
}

/* ---- filling the pages from the document ---------------------------------------------------------- */
static void fill_member(int m)
{
    HWND lv = H(ID_SKILLS);
    wchar_t w[128];
    int k;
    g_member = m;
    g_filling++;
    for (k = 0; k < SEM_NFIELDS; k++) {
        if (m >= 0) w_of(g_doc.cur.m[m].field[k], w, 128); else w[0] = 0;
        SetWindowTextW(H(ID_FIELD0 + k), w);
        EnableWindow(H(ID_FIELD0 + k), m >= 0 && (k != SEM_F_SKILLPOINTS || g_doc.mi[m].skillpoints_ok));
    }
    ListView_DeleteAllItems(lv);
    if (m >= 0)
        for (k = 0; k < SE_SKILL_COUNT; k++) {
            int row, v = g_doc.cur.m[m].skills[k];
            if (g_haveSkillNames) {
                _snwprintf(w, 128, L"%hs", g_skillNames[k]);
                w[0] = towupper(w[0]);                 /* "sword weapons" -> "Sword weapons" */
            } else _snwprintf(w, 128, L"Skill %d", k);
            row = lv_add(lv, w, k);
            _snwprintf(w, 128, L"%d", v); lv_set(lv, row, 1, w);
            /* as the Character screen shows it: hundredths, level = value / 10; speed (2) as a percentage */
            if (k == 2) _snwprintf(w, 128, L"%d%%", v * 10);
            else _snwprintf(w, 128, L"%d.%d", v / 10, v % 10);
            lv_set(lv, row, 2, w);
            lv_set(lv, row, 3, v != g_doc.orig.m[m].skills[k] ? L"changed" : L"");
        }
    SetWindowTextW(H(ID_SKILLVAL), L"");
    if (m >= 0) {
        wchar_t lab[96];
        w_of(g_doc.mi[m].label, lab, 96);
        _snwprintf(w, 128, L"%ls%ls, party record %d", lab, g_doc.mi[m].dead ? L" (dead)" : L"", g_doc.mi[m].record);
        SetWindowTextW(H(ID_MEMBERINFO), w);
    } else SetWindowTextW(H(ID_MEMBERINFO), L"No party members in this save.");
    g_filling--;
}

static void item_label(uint32_t h, wchar_t *out, size_t n)
{
    const char *nm = sem_item_name(&g_doc, h);
    if (nm) {
        wchar_t w[256];
        w_of(nm, w, 256);
        _snwprintf(out, n, L"%ls", w);
    } else _snwprintf(out, n, L"item 0x%08lx (no name: cannot be changed)", (unsigned long)h);
    out[n - 1] = 0;
}

static void fill_inventory(uint32_t select)
{
    HWND lv = H(ID_INV);
    wchar_t w[300];
    int i, sel = -1;
    g_filling++;
    ListView_DeleteAllItems(lv);
    for (i = 0; i < g_doc.cur.ninv; i++) {
        const sem_item *it = &g_doc.cur.inv[i];
        int r = sem_inv_find(&g_doc.orig, it->hash), row;
        item_label(it->hash, w, 300);
        row = lv_add(lv, w, (LPARAM)it->hash);
        _snwprintf(w, 300, L"%lld", (long long)it->count); lv_set(lv, row, 1, w);
        lv_set(lv, row, 2, r < 0 ? L"added" : g_doc.orig.inv[r].count != it->count ? L"changed" : L"");
        if (it->hash == select) sel = row;
    }
    /* rows the save had that are gone now */
    for (i = 0; i < g_doc.orig.ninv; i++) {
        int row;
        if (sem_inv_find(&g_doc.cur, g_doc.orig.inv[i].hash) >= 0) continue;
        item_label(g_doc.orig.inv[i].hash, w, 300);
        row = lv_add(lv, w, (LPARAM)g_doc.orig.inv[i].hash);
        lv_set(lv, row, 1, L"0");
        lv_set(lv, row, 2, L"removed");
        if (g_doc.orig.inv[i].hash == select) sel = row;
    }
    if (sel >= 0) {
        ListView_SetItemState(lv, sel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(lv, sel, FALSE);
    }
    g_filling--;
}

static void fill_add_list(void)
{
    HWND cb = H(ID_ADDITEM);
    int i;
    SendMessageW(cb, CB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_doc.ntypes; i++) {
        wchar_t w[256];
        LRESULT at;
        uint32_t h = g_doc.types[i];
        if (!sem_item_editable(&g_doc, h)) continue;
        w_of(sem_item_name(&g_doc, h), w, 256);
        if (SendMessageW(cb, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)w) != CB_ERR) continue;
        at = SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)w);
        if (at >= 0) SendMessageW(cb, CB_SETITEMDATA, (WPARAM)at, (LPARAM)h);
    }
}

static void fill_flags(int level)
{
    HWND lv = H(ID_FLAGS);
    wchar_t w[300];
    int k, n = sem_flag_count(&g_doc, level);
    g_level = level;
    g_filling++;
    ListView_DeleteAllItems(lv);
    for (k = 0; k < n; k++) {
        const char *nm = sem_flag_name(&g_doc, level, k);
        int row;
        wchar_t wn[256];
        if (nm && nm[0]) { w_of(nm, wn, 256); _snwprintf(w, 300, L"%ls", wn); }
        else _snwprintf(w, 300, L"flag %d", k);
        w[299] = 0;
        row = lv_add(lv, w, k);
        ListView_SetCheckState(lv, row, g_doc.cur.flags[level][k] ? TRUE : FALSE);
        _snwprintf(w, 300, L"%d", k); lv_set(lv, row, 1, w);
        lv_set(lv, row, 2, !sem_flag_editable(&g_doc, level, k) ? L"cannot be changed (name)" :
                           g_doc.cur.flags[level][k] != g_doc.orig.flags[level][k] ? L"changed" : L"");
    }
    g_filling--;
}

static void fill_levels(void)
{
    HWND lb = H(ID_LEVELS);
    int i, first = -1;
    SendMessageW(lb, LB_RESETCONTENT, 0, 0);
    ListView_DeleteAllItems(H(ID_FLAGS));
    g_level = -1;
    if (!g_doc.have_blocks) {
        wchar_t w[300], why[256];
        w_of(g_doc.flags_why, why, 256);
        _snwprintf(w, 300, L"Story flags are not available for this save: %ls", why);
        w[299] = 0;
        SetWindowTextW(H(ID_FLAGINFO), w);
        return;
    }
    SetWindowTextW(H(ID_FLAGINFO), L"Tick or clear a flag. Setting a flag that ends a quest also removes that quest from the journal, as the game does.");
    for (i = 0; i < SE_LEVEL_COUNT; i++) {
        wchar_t w[160], wn[128];
        LRESULT at;
        int n = sem_flag_count(&g_doc, i);
        if (n <= 0) continue;
        w_of(se_level_names[i], wn, 128);
        _snwprintf(w, 160, L"%2d  %ls (%d)", i, wn, n);
        at = SendMessageW(lb, LB_ADDSTRING, 0, (LPARAM)w);
        SendMessageW(lb, LB_SETITEMDATA, (WPARAM)at, i);
        if (first < 0 || i == se_find_level(g_doc.hdr.level)) first = (int)at;
    }
    if (first >= 0) {
        SendMessageW(lb, LB_SETCURSEL, (WPARAM)first, 0);
        fill_flags((int)SendMessageW(lb, LB_GETITEMDATA, (WPARAM)first, 0));
    }
}

static void fill_all(void)
{
    wchar_t w[PATHLEN + 400], a[300], b[300];
    int i;
    g_filling++;
    w_of(g_doc.hdr.level, a, 300);
    w_of(g_doc.hdr.script, b, 300);
    _snwprintf(w, PATHLEN + 400, L"Opened: %ls", g_docPath);
    w[PATHLEN + 399] = 0;
    SetWindowTextW(H(ID_FILEINFO), w);
    _snwprintf(w, PATHLEN + 400, L"Level: %ls, script %ls%ls", a, b,
               g_tablesOk ? L"" : L"\r\ntables.vpp was not found: item names and story flags are not available.");
    w[PATHLEN + 399] = 0;
    SetWindowTextW(H(ID_LEVELINFO), w);
    w_of(g_doc.cur.desc, a, 300);
    SetWindowTextW(H(ID_DESC), a);
    w_of(g_doc.cur.gold, a, 300);
    SetWindowTextW(H(ID_GOLD), a);
    EnableWindow(H(ID_DESC), g_docOpen);
    EnableWindow(H(ID_GOLD), g_docOpen);

    SendMessageW(H(ID_MEMBERS), LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_doc.nmembers; i++) {
        w_of(g_doc.mi[i].label, a, 300);
        SendMessageW(H(ID_MEMBERS), LB_ADDSTRING, 0, (LPARAM)a);
    }
    if (g_doc.nmembers) SendMessageW(H(ID_MEMBERS), LB_SETCURSEL, 0, 0);
    g_filling--;
    fill_member(g_doc.nmembers ? 0 : -1);
    fill_inventory(0);
    fill_add_list();
    SetWindowTextW(H(ID_ADDCOUNT), L"1");
    SetWindowTextW(H(ID_INVCOUNT), L"");
    fill_levels();
    update_slot_button();
    update_status();
}

static int confirm_discard(void)
{
    int n = pending();
    if (!n) return 1;
    return MessageBoxW(g_wnd, L"The changes to this save have not been saved. Discard them?", APP_TITLE,
                       MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES;
}

static int open_save(const wchar_t *path, int ask)
{
    sem_doc d;
    char err[512];
    wchar_t w[512];
    if (ask && !confirm_discard()) return 0;
    if (sem_open(&d, path, &g_tables, err, sizeof err)) {
        w_of(err, w, 512);
        msg(MB_ICONERROR, L"Cannot open %ls:\n\n%ls", path, w);
        return 0;
    }
    if (g_docOpen) sem_close(&g_doc);
    g_doc = d;
    g_docOpen = 1;
    if (!GetFullPathNameW(path, PATHLEN, g_docPath, NULL)) _snwprintf(g_docPath, PATHLEN, L"%ls", path);
    g_docPath[PATHLEN - 1] = 0;
    fill_all();
    TabCtrl_SetCurSel(g_tab, PAGE_GENERAL);
    show_page(PAGE_GENERAL);
    return 1;
}

/* ---- saving ------------------------------------------------------------------------------------------ */
static void save_to(const wchar_t *target)
{
    static char log[65536];
    char err[1024];
    static wchar_t wlog[65536];
    wchar_t full[PATHLEN], werr[1024];
    sem_ops o;
    int n;
    if (!g_docOpen) return;
    if (sem_build_ops(&g_doc, &o, err, sizeof err) < 0) { w_of(err, werr, 1024); msg(MB_ICONERROR, L"%ls\n\nNothing was written.", werr); return; }
    n = sem_change_count(&o);
    sem_ops_free(&o);
    if (!n) { msg(MB_ICONINFORMATION, L"There are no changes to save."); return; }
    if (!GetFullPathNameW(target, PATHLEN, full, NULL)) _snwprintf(full, PATHLEN, L"%ls", target);
    full[PATHLEN - 1] = 0;
    if (CompareStringOrdinal(full, -1, g_docPath, -1, TRUE) == CSTR_EQUAL) {
        msg(MB_ICONWARNING, L"That is the opened save. The editor never overwrites a save: pick another name.");
        return;
    }
    if (GetFileAttributesW(full) != INVALID_FILE_ATTRIBUTES) {
        msg(MB_ICONWARNING, L"%ls already exists. The editor never overwrites a file: pick another name.", full);
        return;
    }
    if (sem_save_new(&g_doc, full, log, sizeof log, err, sizeof err)) {
        w_of(err, werr, 1024);
        msg(MB_ICONERROR, L"The changes were refused:\n\n%ls\n\nNothing was written.", werr);
        return;
    }
    w_of(log, wlog, 65536);
    {
        wchar_t *p, *q;
        static wchar_t crlf[70000];
        for (p = wlog, q = crlf; *p && q < crlf + 69990; p++) { if (*p == L'\n') *q++ = L'\r'; *q++ = *p; }
        *q = 0;
        msg(MB_ICONINFORMATION, L"Saved to\n%ls\n\n%ls\nThe opened save was not changed. The editor now shows the new save.", full, crlf);
    }
    status(L"Saved to %ls", full);
    refresh_saves();
    open_save(full, 0);
    status(L"Saved to %ls", full);
}

static void save_slot(void)
{
    wchar_t dir[PATHLEN], slot[PATHLEN];
    folder_of(g_docPath, dir, PATHLEN);
    if (sem_next_slot(dir, slot, PATHLEN) < 0) { msg(MB_ICONWARNING, L"No free summonerN.sav name in %ls. Use Save As.", dir); return; }
    save_to(slot);
}

static void save_as(void)
{
    OPENFILENAMEW of;
    wchar_t file[PATHLEN], dir[PATHLEN], slot[PATHLEN];
    folder_of(g_docPath, dir, PATHLEN);
    file[0] = 0;
    if (sem_next_slot(dir, slot, PATHLEN) >= 0) _snwprintf(file, PATHLEN, L"%ls", file_part(slot));
    file[PATHLEN - 1] = 0;
    memset(&of, 0, sizeof of);
    of.lStructSize = sizeof of;
    of.hwndOwner = g_wnd;
    of.lpstrFilter = L"Summoner saves (*.sav)\0*.sav\0All files\0*.*\0";
    of.lpstrFile = file;
    of.nMaxFile = PATHLEN;
    of.lpstrInitialDir = dir;
    of.lpstrDefExt = L"sav";
    of.lpstrTitle = L"Save the changes as a new file";
    of.Flags = OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN | OFN_HIDEREADONLY;
    if (GetSaveFileNameW(&of)) save_to(file);
}

static void open_other(void)
{
    OPENFILENAMEW of;
    wchar_t file[PATHLEN];
    file[0] = 0;
    memset(&of, 0, sizeof of);
    of.lStructSize = sizeof of;
    of.hwndOwner = g_wnd;
    of.lpstrFilter = L"Summoner saves (*.sav)\0*.sav\0All files\0*.*\0";
    of.lpstrFile = file;
    of.nMaxFile = PATHLEN;
    of.lpstrInitialDir = g_saveDir;
    of.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&of)) open_save(file, 1);
}

/* ---- edits from the controls ------------------------------------------------------------------------- */
static int get_latin1(int id, char *out, size_t n, const wchar_t *what)
{
    wchar_t w[1024];
    GetWindowTextW(H(id), w, 1024);
    if (sem_to_latin1(w, out, n)) {
        status(L"%ls: only Western European (Latin-1) characters can be saved; the last change was not taken.", what);
        return -1;
    }
    return 0;
}

static int parse_count(int id, long long lo, long long *out)
{
    wchar_t w[64], *end;
    long long v;
    GetWindowTextW(H(id), w, 64);
    v = wcstoll(w, &end, 10);
    while (*end == L' ') end++;
    if (end == w || *end || v < lo || v > 0x7fffffff) return -1;
    *out = v;
    return 0;
}

static void on_edit_change(int id)
{
    char s[300];
    if (g_filling || !g_docOpen) return;
    if (id == ID_DESC) { if (!get_latin1(id, s, 256, L"Description")) sem_set_desc(&g_doc, s); }
    else if (id == ID_GOLD) { if (!get_latin1(id, s, 64, L"Gold")) sem_set_gold(&g_doc, s); }
    else if (id >= ID_FIELD0 && id < ID_FIELD0 + SEM_NFIELDS && g_member >= 0) {
        if (!get_latin1(id, s, 64, L"Party member")) sem_set_field(&g_doc, g_member, id - ID_FIELD0, s);
    }
    update_status();
}

static void on_skill_set(void)
{
    int row = lv_sel(H(ID_SKILLS));
    long long v;
    if (g_member < 0 || row < 0) { status(L"Pick a skill in the list first."); return; }
    if (parse_count(ID_SKILLVAL, 0, &v) || v > 255) { status(L"A skill value is a whole number from 0 to 255."); return; }
    sem_set_skill(&g_doc, g_member, (int)lv_data(H(ID_SKILLS), row), (int)v);
    fill_member(g_member);
    ListView_SetItemState(H(ID_SKILLS), row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(H(ID_SKILLS), row, FALSE);
    update_status();
}

static int selected_item(uint32_t *h)
{
    int row = lv_sel(H(ID_INV));
    if (row < 0) { status(L"Pick an item in the list first."); return -1; }
    *h = (uint32_t)lv_data(H(ID_INV), row);
    if (!sem_item_editable(&g_doc, *h)) { status(L"This item has no name in tables.vpp, so the editor cannot change it."); return -1; }
    return 0;
}

static void on_inv_set(int remove)
{
    uint32_t h;
    long long v = 0;
    if (!g_docOpen || selected_item(&h)) return;
    if (!remove && parse_count(ID_INVCOUNT, 0, &v)) { status(L"A count is a whole number from 0 to 2147483647."); return; }
    if (sem_inv_set(&g_doc, h, v)) { status(L"That count cannot be set."); return; }
    fill_inventory(h);
    update_status();
}

static void on_add(void)
{
    HWND cb = H(ID_ADDITEM);
    wchar_t w[256];
    LRESULT at;
    long long v;
    uint32_t h;
    if (!g_docOpen) return;
    GetWindowTextW(cb, w, 256);
    at = SendMessageW(cb, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)w);
    if (at == CB_ERR) { status(L"\"%ls\" is not an item type of this save. Pick one from the list.", w); return; }
    if (parse_count(ID_ADDCOUNT, 1, &v)) { status(L"The number to add is a whole number from 1 to 2147483647."); return; }
    h = (uint32_t)SendMessageW(cb, CB_GETITEMDATA, (WPARAM)at, 0);
    if (sem_inv_add(&g_doc, h, v)) { status(L"Cannot add that many (a count is at most 2147483647)."); return; }
    fill_inventory(h);
    update_status();
}

static void on_flag_toggled(int row, int checked)
{
    int k = (int)lv_data(H(ID_FLAGS), row);
    if (g_level < 0 || k < 0) return;
    if (!sem_flag_editable(&g_doc, g_level, k)) {
        g_filling++;
        ListView_SetCheckState(H(ID_FLAGS), row, g_doc.cur.flags[g_level][k] ? TRUE : FALSE);
        g_filling--;
        status(L"This flag's name is empty or repeated in its level, so the editor cannot change it.");
        return;
    }
    sem_set_flag(&g_doc, g_level, k, checked);
    lv_set(H(ID_FLAGS), row, 2, g_doc.cur.flags[g_level][k] != g_doc.orig.flags[g_level][k] ? L"changed" : L"");
    update_status();
}

/* ---- window ------------------------------------------------------------------------------------------ */
static void create_controls(void)
{
    static const wchar_t *const tabs[PAGE_COUNT] = {L"Saves", L"Description and gold", L"Party", L"Inventory", L"Story flags"};
    TCITEMW ti;
    int i, y;
    HWND lv;
    g_tab = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP, 8, 8, 864, 520,
                            g_wnd, (HMENU)ID_TAB, g_inst, NULL);
    SendMessageW(g_tab, WM_SETFONT, (WPARAM)g_font, TRUE);
    memset(&ti, 0, sizeof ti);
    ti.mask = TCIF_TEXT;
    for (i = 0; i < PAGE_COUNT; i++) { ti.pszText = (wchar_t *)tabs[i]; TabCtrl_InsertItem(g_tab, i, &ti); }

    /* Saves */
    ctl(PAGE_SAVES, 0, L"STATIC", L"", SS_PATHELLIPSIS | SS_NOPREFIX, 20, 42, 840, 20, ID_SAVEDIR);
    lv = listview(PAGE_SAVES, 20, 64, 840, 380, ID_SAVELIST, 0);
    column(lv, 0, L"File", 130); column(lv, 1, L"Description", 380); column(lv, 2, L"Level", 160); column(lv, 3, L"Date", 140);
    button(PAGE_SAVES, L"Open", 20, 452, 100, ID_OPEN);
    button(PAGE_SAVES, L"Open other file...", 128, 452, 150, ID_OPENOTHER);
    button(PAGE_SAVES, L"Refresh", 286, 452, 100, ID_REFRESH);
    label(PAGE_SAVES, L"", 20, 486, 840, 36, ID_TABLESINFO);

    /* Description and gold */
    ctl(PAGE_GENERAL, 0, L"STATIC", L"", SS_PATHELLIPSIS | SS_NOPREFIX, 20, 44, 840, 20, ID_FILEINFO);
    label(PAGE_GENERAL, L"", 20, 66, 840, 40, ID_LEVELINFO);
    label(PAGE_GENERAL, L"Description (shown on the Save/Load screens, up to 255 characters):", 20, 112, 600, 20, 0);
    edit(PAGE_GENERAL, 20, 134, 700, ID_DESC, 0);
    SendMessageW(H(ID_DESC), EM_SETLIMITTEXT, 255, 0);
    label(PAGE_GENERAL, L"Party gold:", 20, 176, 120, 20, 0);
    edit(PAGE_GENERAL, 20, 198, 160, ID_GOLD, 0);

    /* Party */
    label(PAGE_PARTY, L"Party members:", 20, 44, 180, 20, 0);
    ctl(PAGE_PARTY, WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY, 20, 66, 180, 140, ID_MEMBERS);
    label(PAGE_PARTY, L"", 220, 44, 300, 20, ID_MEMBERINFO);
    for (i = 0, y = 70; i < SEM_NFIELDS; i++, y += 32) {
        wchar_t w[64];
        _snwprintf(w, 64, L"%hs:", sem_field_label[i]);
        label(PAGE_PARTY, w, 220, y + 3, 100, 20, 0);
        edit(PAGE_PARTY, 320, y, 150, ID_FIELD0 + i, 0);
    }
    label(PAGE_PARTY, L"HP and max HP may have decimals (12.5). Raising the level does not give the HP, AP or skill points of the skipped levels.",
          20, 300, 450, 60, 0);
    label(PAGE_PARTY, g_haveSkillNames ? L"Saved skills (the value is hundredths: 10 = level 1.0):"
                                       : L"Saved skills (no Sum.exe in this folder, so no names; 10 = level 1.0):", 490, 44, 370, 20, 0);
    lv = listview(PAGE_PARTY, 490, 66, 370, 380, ID_SKILLS, 0);
    column(lv, 0, L"Skill", 150); column(lv, 1, L"Value", 55); column(lv, 2, L"In game", 65); column(lv, 3, L"", 60);
    label(PAGE_PARTY, L"Value (0-255):", 490, 459, 100, 20, 0);
    edit(PAGE_PARTY, 590, 456, 80, ID_SKILLVAL, ES_NUMBER);
    button(PAGE_PARTY, L"Set", 680, 455, 80, ID_SKILLSET);

    /* Inventory */
    lv = listview(PAGE_INVENTORY, 20, 44, 520, 440, ID_INV, 0);
    column(lv, 0, L"Item", 330); column(lv, 1, L"Count", 80); column(lv, 2, L"", 80);
    label(PAGE_INVENTORY, L"Selected item:", 560, 44, 290, 20, 0);
    label(PAGE_INVENTORY, L"Count:", 560, 71, 60, 20, 0);
    edit(PAGE_INVENTORY, 620, 68, 100, ID_INVCOUNT, ES_NUMBER);
    button(PAGE_INVENTORY, L"Set count", 730, 67, 110, ID_INVSET);
    button(PAGE_INVENTORY, L"Remove", 730, 100, 110, ID_INVREMOVE);
    label(PAGE_INVENTORY, L"Add an item (the item types of this save):", 560, 160, 290, 20, 0);
    ctl(PAGE_INVENTORY, 0, L"COMBOBOX", L"", WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN | CBS_SORT | CBS_AUTOHSCROLL, 560, 184, 290, 300, ID_ADDITEM);
    label(PAGE_INVENTORY, L"Number:", 560, 221, 60, 20, 0);
    edit(PAGE_INVENTORY, 620, 218, 100, ID_ADDCOUNT, ES_NUMBER);
    button(PAGE_INVENTORY, L"Add", 730, 217, 110, ID_ADD);
    label(PAGE_INVENTORY, L"Only items with a name in tables.vpp can be changed. A new item type cannot be added: the save lists the types it can hold.",
          560, 270, 290, 80, ID_INVINFO);

    /* Story flags */
    label(PAGE_FLAGS, L"Levels (number of flags):", 20, 44, 220, 20, 0);
    ctl(PAGE_FLAGS, WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY, 20, 66, 220, 400, ID_LEVELS);
    lv = listview(PAGE_FLAGS, 250, 66, 610, 400, ID_FLAGS, LVS_EX_CHECKBOXES);
    column(lv, 0, L"Flag", 340); column(lv, 1, L"Index", 60); column(lv, 2, L"", 180);
    label(PAGE_FLAGS, L"", 20, 474, 840, 40, ID_FLAGINFO);

    /* every page */
    label(PAGE_ALL, NOTICE, 12, 536, 860, 20, ID_NOTICE);
    label(PAGE_ALL, L"", 12, 558, 860, 20, ID_STATUS);
    button(PAGE_ALL, L"Save as new slot", 12, 584, 220, ID_SAVESLOT);
    button(PAGE_ALL, L"Save As...", 240, 584, 120, ID_SAVEAS);
    button(PAGE_ALL, L"Undo all changes", 368, 584, 150, ID_UNDO);
    SetWindowPos(g_tab, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    show_page(PAGE_SAVES);
}

static LRESULT CALLBACK wndproc(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
    switch (m) {
    case WM_CREATE:
        g_wnd = w;
        create_controls();
        return 0;
    case WM_COMMAND: {
        int id = LOWORD(wp), code = HIWORD(wp);
        if (code == EN_CHANGE) { on_edit_change(id); return 0; }
        if (id == ID_MEMBERS && code == LBN_SELCHANGE) {
            int s = (int)SendMessageW(H(ID_MEMBERS), LB_GETCURSEL, 0, 0);
            if (s >= 0) fill_member(s);
            return 0;
        }
        if (id == ID_LEVELS && code == LBN_SELCHANGE) {
            int s = (int)SendMessageW(H(ID_LEVELS), LB_GETCURSEL, 0, 0);
            if (s >= 0) fill_flags((int)SendMessageW(H(ID_LEVELS), LB_GETITEMDATA, (WPARAM)s, 0));
            return 0;
        }
        if (code != BN_CLICKED) break;
        switch (id) {
        case ID_OPEN: {
            int row = lv_sel(H(ID_SAVELIST));
            if (row < 0) status(L"Pick a save in the list first.");
            else open_save(g_saves[lv_data(H(ID_SAVELIST), row)].path, 1);
            return 0;
        }
        case ID_OPENOTHER: open_other(); return 0;
        case ID_REFRESH: refresh_saves(); return 0;
        case ID_SKILLSET: on_skill_set(); return 0;
        case ID_INVSET: on_inv_set(0); return 0;
        case ID_INVREMOVE: on_inv_set(1); return 0;
        case ID_ADD: on_add(); return 0;
        case ID_SAVESLOT: save_slot(); return 0;
        case ID_SAVEAS: save_as(); return 0;
        case ID_UNDO:
            if (g_docOpen && pending() && MessageBoxW(w, L"Undo all changes to this save?", APP_TITLE, MB_YESNO | MB_ICONQUESTION) == IDYES) {
                sem_revert(&g_doc);
                fill_all();
            }
            return 0;
        }
        break;
    }
    case WM_NOTIFY: {
        NMHDR *nh = (NMHDR *)lp;
        if (nh->idFrom == ID_TAB && nh->code == TCN_SELCHANGE) { show_page(TabCtrl_GetCurSel(g_tab)); return 0; }
        if (nh->idFrom == ID_SAVELIST && nh->code == NM_DBLCLK) {
            NMITEMACTIVATE *ia = (NMITEMACTIVATE *)lp;
            if (ia->iItem >= 0) open_save(g_saves[lv_data(H(ID_SAVELIST), ia->iItem)].path, 1);
            return 0;
        }
        if (nh->code == LVN_ITEMCHANGED && !g_filling && g_docOpen) {
            NMLISTVIEW *nl = (NMLISTVIEW *)lp;
            if (!(nl->uChanged & LVIF_STATE) || nl->iItem < 0) return 0;
            if (nh->idFrom == ID_FLAGS && ((nl->uNewState ^ nl->uOldState) & LVIS_STATEIMAGEMASK) && (nl->uOldState & LVIS_STATEIMAGEMASK))
                on_flag_toggled(nl->iItem, ((nl->uNewState & LVIS_STATEIMAGEMASK) >> 12) == 2);
            else if (nh->idFrom == ID_SKILLS && (nl->uNewState & LVIS_SELECTED) && g_member >= 0) {
                wchar_t v[16];
                _snwprintf(v, 16, L"%d", g_doc.cur.m[g_member].skills[lv_data(H(ID_SKILLS), nl->iItem)]);
                SetWindowTextW(H(ID_SKILLVAL), v);
            } else if (nh->idFrom == ID_INV && (nl->uNewState & LVIS_SELECTED)) {
                uint32_t h = (uint32_t)lv_data(H(ID_INV), nl->iItem);
                int r = sem_inv_find(&g_doc.cur, h);
                wchar_t v[32];
                _snwprintf(v, 32, L"%lld", r >= 0 ? (long long)g_doc.cur.inv[r].count : 0LL);
                SetWindowTextW(H(ID_INVCOUNT), v);
            }
            return 0;
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
        if ((HWND)lp == H(ID_NOTICE)) {
            SetTextColor((HDC)wp, RGB(0, 0x50, 0));
            SetBkMode((HDC)wp, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }
        break;
    case WM_CLOSE:
        if (!confirm_discard()) return 0;
        DestroyWindow(w);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(w, m, wp, lp);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, LPWSTR cmd, int show)
{
    INITCOMMONCONTROLSEX icc = {sizeof icc, ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_STANDARD_CLASSES};
    NONCLIENTMETRICSW ncm;
    WNDCLASSEXW wc;
    RECT r = {0, 0, 880, 620};
    MSG mm;
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    wchar_t **argv, openPath[PATHLEN] = L"", tablesArg[PATHLEN] = L"";
    int argc, i;
    se_table_opts to;
    (void)prev; (void)cmd;
    g_inst = inst;
    InitCommonControlsEx(&icc);
    memset(&ncm, 0, sizeof ncm);
    ncm.cbSize = sizeof ncm;
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof ncm, &ncm, 0)) g_font = CreateFontIndirectW(&ncm.lfMessageFont);
    if (!g_font) g_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    /* the game folder: the program's own, or --game-dir */
    if (!GetModuleFileNameW(NULL, g_gameDir, PATHLEN)) g_gameDir[0] = 0;
    if (wcsrchr(g_gameDir, L'\\')) *wcsrchr(g_gameDir, L'\\') = 0;
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (i = 1; argv && i < argc; i++) {
        if (!wcscmp(argv[i], L"--game-dir") && i + 1 < argc) {
            if (!GetFullPathNameW(argv[++i], PATHLEN, g_gameDir, NULL)) _snwprintf(g_gameDir, PATHLEN, L"%ls", argv[i]);
            if (wcslen(g_gameDir) > 3 && g_gameDir[wcslen(g_gameDir) - 1] == L'\\') g_gameDir[wcslen(g_gameDir) - 1] = 0;
        } else if (!wcscmp(argv[i], L"--tables") && i + 1 < argc) {
            if (!GetFullPathNameW(argv[++i], PATHLEN, tablesArg, NULL)) _snwprintf(tablesArg, PATHLEN, L"%ls", argv[i]);
        } else if (!wcscmp(argv[i], L"--open") && i + 1 < argc) _snwprintf(openPath, PATHLEN, L"%ls", argv[++i]);
    }
    if (argv) LocalFree(argv);
    g_gameDir[PATHLEN - 1] = openPath[PATHLEN - 1] = 0;
    _snwprintf(g_saveDir, PATHLEN, L"%ls\\savegame", g_gameDir);
    if (tablesArg[0]) _snwprintf(g_tablesPath, PATHLEN, L"%ls", tablesArg);
    else _snwprintf(g_tablesPath, PATHLEN, L"%ls\\tables.vpp", g_gameDir);
    g_saveDir[PATHLEN - 1] = g_tablesPath[PATHLEN - 1] = 0;
    g_tablesOk = GetFileAttributesW(g_tablesPath) != INVALID_FILE_ATTRIBUTES;
    {
        wchar_t exe[PATHLEN];
        /* Sum.exe in the game folder; else Sum-orig.exe or any other Sum*.exe there whose skill table checks
           out (a test folder keeps the original under another name) */
        WIN32_FIND_DATAW fd;
        HANDLE h;
        _snwprintf(exe, PATHLEN, L"%ls\\Sum.exe", g_gameDir);
        exe[PATHLEN - 1] = 0;
        g_haveSkillNames = sem_load_skill_names(exe, g_skillNames) > 0;
        if (!g_haveSkillNames) {
            _snwprintf(exe, PATHLEN, L"%ls\\Sum-orig.exe", g_gameDir);
            exe[PATHLEN - 1] = 0;
            g_haveSkillNames = sem_load_skill_names(exe, g_skillNames) > 0;
        }
        _snwprintf(exe, PATHLEN, L"%ls\\Sum*.exe", g_gameDir);
        exe[PATHLEN - 1] = 0;
        if (!g_haveSkillNames && (h = FindFirstFileW(exe, &fd)) != INVALID_HANDLE_VALUE) {
            do {
                _snwprintf(exe, PATHLEN, L"%ls\\%ls", g_gameDir, fd.cFileName);
                exe[PATHLEN - 1] = 0;
                g_haveSkillNames = sem_load_skill_names(exe, g_skillNames) > 0;
            } while (!g_haveSkillNames && FindNextFileW(h, &fd));
            FindClose(h);
        }
    }
    memset(&to, 0, sizeof to);
    to.tables_vpp = g_tablesOk ? g_tablesPath : NULL;
    se_tables_load(&g_tables, &to);

    memset(&wc, 0, sizeof wc);
    wc.cbSize = sizeof wc;
    wc.lpfnWndProc = wndproc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName = L"SummonerSaveEditor";
    RegisterClassExW(&wc);
    AdjustWindowRect(&r, style, FALSE);
    CreateWindowExW(0, wc.lpszClassName, APP_TITLE, style, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
                    NULL, NULL, inst, NULL);
    if (!g_wnd) return 1;

    refresh_saves();
    {
        wchar_t t[PATHLEN + 300];
        if (g_tablesOk) {
            wchar_t w0[256] = L"";
            if (g_tables.nwarnings) w_of(g_tables.warnings[0], w0, 256);
            _snwprintf(t, PATHLEN + 300, L"Names of items and story flags: %ls%ls%ls", g_tablesPath, w0[0] ? L"\r\nNote: " : L"", w0);
        } else
            _snwprintf(t, PATHLEN + 300, L"tables.vpp was not found in %ls. Description, gold and the party can still be edited; "
                       L"item names and story flags need it (put this program in the game folder, next to Sum.exe).", g_gameDir);
        t[PATHLEN + 299] = 0;
        SetWindowTextW(H(ID_TABLESINFO), t);
    }
    update_status();
    if (!g_tablesOk) status(L"tables.vpp not found next to the editor: items and story flags are not available.");
    ShowWindow(g_wnd, show);
    UpdateWindow(g_wnd);
    if (openPath[0]) open_save(openPath, 0);

    while (GetMessageW(&mm, NULL, 0, 0) > 0) {
        if (IsDialogMessageW(g_wnd, &mm)) continue;
        TranslateMessage(&mm);
        DispatchMessageW(&mm);
    }
    if (g_docOpen) sem_close(&g_doc);
    se_tables_free(&g_tables);
    free(g_saves);
    return 0;
}
