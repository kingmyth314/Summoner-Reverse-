// summoner_cd.cpp - Sum.exe v1.40, code 0x43e660-0x43eaf8 (finding the game disc)
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: sum.symbols module column (strings.tbl ids 935-937 of the `// summoner_cd.cpp` block). It follows
// WinMain (0x43e5e0, the end of summoner.cpp) and ends before state14_enter (0x43eb00, the multiplayer
// state unit; not included: no string, data or call ties it to this file).
// Data: .data 0x594f2c-0x594f87 ("SUMMONER_2", "SUMMONER_1", "setup.exe", "A:\", the "Using %s" line,
// "%c:\", "rb", "%s\eleh.s3d"); "levelm.vpp" (0x593f30) and "" (0x5bd0d0) are pooled with other units.
// .bss: cd_root (0x60ad7c), after summoner.cpp's .bss (0x60abb0-0x60ad7b).
//
// THE SHIPPED Sum.exe IS PATCHED HERE (a no-CD edit in the file, not Timeslip's in-memory one; D2600). The
// code below is the unpatched reading:
//   0x43e69b  b2 2e 90        mov dl, '.' / nop     was `add dl, 'A'` (80 c2 41): root[0] = 'A' + i
//   0x43e6b0  0f 85 00000000  jne +0                the DRIVE_CDROM test's jump (target lost; read as continue)
//   0x43e6e3  0f 85 00000000  jne +0                the GetVolumeInformationA test's jump (read as continue)
//   0x43e872  75 00           jne +0                the second DRIVE_CDROM test (read as skipping result = i)
//   0x43e914  b0 2e 90        mov al, '.' / nop     was `add eax, 'A'` (83 c0 41)
//   0x594f50  ".\\" + NUL     was "A:\" (the root template)
//   0x594f70  ".\" + NULs     was "%c:\" (cd_set_root's format; the pushed letter is ignored)
// With the patch every drive index finds the files under the current directory, so drive 0 is chosen and
// cd_root is ".\".

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdlib.h>
#include <windows.h>
#include "vsdk/os/os.h"
#include "vsdk/os/timer.h"
#include "vsdk/os/key.h"
#include "vsdk/vfile/file.h"
#include "vsdk/localize/localize.h"
#include "vsdk/gr/gr.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"

#define CD_NUM_DRIVES 26              // 'A'..'Z' (cmp ebx, 0x1a)

// ---- .bss ---------------------------------------------------------------------------------------------
char cd_root[64];                     // 0x60ad7c

// 0x43e660  Confidence: unsure (the three patched branches, see the file comment; the rest likely: the
// `found` flag is stored only on the SUMMONER_1 path, the other matches jump straight into the found
// block, so `found` is dead after it and that block cannot loop). The volume label read by
// GetVolumeInformationA is never compared: `volume` only selects which file proves the disc.
int cd_find_drive(const char *volume)
{
    char cwd[MAX_PATH];
    struct _finddata_t fd;
    char label[256];
    long h;
    int i;
    int result;
    int found;
    int has_setup;
    int has_levelm;

    result = -1;
    found = 0;
    GetCurrentDirectoryA(MAX_PATH, cwd);
    for (i = 0; i < CD_NUM_DRIVES; i++) {
        char root[] = "A:\\";
        char path[512] = "";

        root[0] = (char)('A' + i);
        if (GetDriveTypeA(root) != DRIVE_CDROM)
            continue;
        result = -3;
        if (GetVolumeInformationA(root, label, sizeof(label), NULL, NULL, NULL, NULL, 0) != TRUE)
            continue;

        has_setup = 0;
        has_levelm = 0;
        strcpy(path, root);
        strcat(path, "setup.exe");
        h = _findfirst(path, &fd);
        if (h != -1) {
            has_setup = 1;
            _findclose(h);
        }
        strcpy(path, root);
        strcat(path, "levelm.vpp");
        h = _findfirst(path, &fd);
        if (h != -1) {
            has_levelm = 1;
            _findclose(h);
        }

        if (volume) {
            if (!_stricmp(volume, "SUMMONER_1") && has_setup)
                found = 1;
            if (!_stricmp(volume, "SUMMONER_2") && has_levelm)
                found = 1;
        } else if (has_setup || has_levelm) {
            found = 1;
        }
        if (found) {
            if (!has_levelm || GetDriveTypeA(root) == DRIVE_CDROM)
                result = i;
            break;
        }
    }
    SetCurrentDirectoryA(cwd);
    return result;
}

// 0x43e8a0  Confidence: exact. The message only goes to a stack buffer (the debug print is compiled out).
int cd_select_drive()
{
    char msg[256];
    int ok;

    ok = cd_set_root(cd_find_drive(NULL));
    if (ok)
        sprintf(msg, "Using %s for Summoner CD\n", cd_root);
    return ok;
}

// 0x43e8e0  Confidence: likely (differs under cl 8966, match24; was exact) (unpatched format "%c:\", see the file comment).
int cd_set_root(int drive)
{
    if (drive < 0) {
        strcpy(cd_root, "");
        return 0;
    }
    sprintf(cd_root, "%c:\\", drive + 'A');
    file_set_cd_root(cd_root);
    return 1;
}

// 0x43e940  Confidence: exact. Sprintf onto its own source: path is both the format argument and the
// destination.
int cd_has_level_data()
{
    char path[256];
    FILE *fp;

    file_get_full_dir(path_models_levels_copy, path);
    sprintf(path, "%s\\eleh.s3d", path);
    fp = fopen(path, "rb");
    if (fp) {
        fclose(fp);
        return 1;
    }
    return 0;
}

// 0x43e9b0  Confidence: exact (VC6 byte match, match24; was likely) (the while loop is rotated: the first test before it, `jl` back at its
// end). MB_OKCANCEL | MB_SETFOREGROUND | MB_TOPMOST (0x50001); Cancel (IDCANCEL) exits. Timeslip's
// dinput.dll turns the jge at 0x43e9cb into a jmp (docs/patches/timeslip.md).
void CdCheck()
{
    int drive;

    if (cd_has_level_data())
        return;
    os_poll();
    drive = cd_find_drive(NULL);
    while (drive < 0) {
        if (MessageBoxA(NULL, XSTR("Please insert any Summoner CD into your CD rom drive and press OK to "
                                   "continue or press Cancel to exit.", 935),
                        XSTR("CD Required at Startup", 936),
                        MB_OKCANCEL | MB_SETFOREGROUND | MB_TOPMOST) == IDCANCEL)
            exit(1);
        os_poll();
        drive = cd_find_drive(NULL);
    }
    cd_set_root(drive);
}

// 0x43ea20  Confidence: likely (the box is centred horizontally only: y = height / 2 is its top edge; the
// text is re-read from the string table every pass). Esc exits; 300 ms per poll.
void cd_prompt_disc2()
{
    int drive;
    int w, h;
    int x, y;
    char *text;

    for (;;) {
        text = XSTR("Insert Summoner CD #2 (ESC exits game)", 937);
        drive = cd_find_drive("SUMMONER_2");
        if (drive >= 0)
            break;
        gr_get_string_size(&w, &h, text, -1, -1);
        w += 6;
        h += 6;
        x = (gr_screen_width() - w) / 2;
        y = gr_screen_height() / 2;
        gr_clear();
        ui_draw_text_box(x, y, w, h);
        gr_string(x + 3, y + 2, text, -1, gr_string_mode);
        if (key_inkey() == KEY_ESC)
            exit(1);
        gr_flip();
        os_poll();
        timer_wait(300);
    }
    cd_set_root(drive);
}
