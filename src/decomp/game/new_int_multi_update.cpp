// new_int_multi_update.cpp - Sum.exe v1.40, code 0x410cf0-0x411260 (the online version check of the PXO
// lobby)
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Unit: strings.tbl ids 932 / 933 (section new_int_multi_update.cpp). It follows new_icam_load (0x410788 is
// its last reference to the shared "") and ends before 0x411270.
// .data 0x58bd34-0x58bda2: three ints, then the literals "version.nfo" (used twice, one copy), the URL,
// "SkipVerify", "%i %i %i", "rt". Within each function the literals are laid out in reverse order of use
// (the URL follows "version.nfo"; "rt" follows "%i %i %i"); "version.nfo" once only means this object was
// built with string pooling, unlike the vsdk units. .bss 0x5bd0d4-0x5bd2db. The "" of the two `= ""`
// initialisers is 0x5bd0d0, a byte shared with other units of the interface block (not this unit's).
//
// version_check_run is polled every frame by pxochat_version_check_frame until it returns 1, 2 or 3; the
// download goes to multicache\version.nfo through InetGetFile, and the last non-comment line of the file is
// "major minor build". A "SkipVerify" DWORD under the application key turns the check off.

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "vsdk/localize/localize.h"
#include "vsdk/os/registry.h"
#include "vsdk/vfile/file.h"
#include "vsdk/inetfile/inetgetfile.h"
#include "summoner/summoner.h"
#include "summoner/interface/new_int_multi_update.h"

#define VERSION_URL  "http://www.pxo.net/files/summoner/northamerica/version.nfo"
#define VERSION_FILE "version.nfo"

// ---- .data ----------------------------------------------------------------------------------
int version_check_need_registry = 1;    // 0x58bd34 (name ours)
int version_check_need_fetch = 1;       // 0x58bd38 (name ours)
int version_check_result = 999;         // 0x58bd3c

// ---- .bss ------------------------------------------------------------------------------------
char version_check_status_text[512];    // 0x5bd0d4
InetGetFile *version_check_download;    // 0x5bd2d4
int version_check_done;                 // 0x5bd2d8

// 0x410cf0  Confidence: likely (the EH frame is the one `new` with a constructor needs; the two buffers are
// cleared in this order). Downloads into multicache. The NULL test only catches a failed allocation:
// a getter that cannot connect is reported later by version_check_poll_download.
int version_check_fetch()
{
    char url[512] = "";
    char path[512] = "";

    strcpy(url, VERSION_URL);
    file_make_path(path_multicache, VERSION_FILE, path);
    version_check_download = new InetGetFile(url, path);
    if (version_check_download == NULL) {
        strcpy(version_check_status_text, XSTR("Could not get data from website\n", 932));
        return -1;
    }
    return 0;
}

// 0x410e20  Confidence: likely (the connecting / receiving tests decide nothing: both ways return 0).
int version_check_poll_download()
{
    if (version_check_download == NULL) {
        strcpy(version_check_status_text, XSTR("Could not get data from website\n", 932));
        return -1;
    }
    if (version_check_download->IsFileError()) {
        delete version_check_download;
        version_check_download = NULL;
        strcpy(version_check_status_text, XSTR("Could not get data from website\n", 932));
        return -1;
    }
    if (version_check_download->IsFileReceived()) {
        delete version_check_download;
        version_check_download = NULL;
        return 1;
    }
    if (version_check_download->IsConnecting() || version_check_download->IsReceiving())
        return 0;
    return 0;
}

// 0x410f10  Confidence: exact.
void version_check_set_error_text()
{
    strcpy(version_check_status_text,
           XSTR("There was an error verifying your version of Summoner, if you continue, you will not "
                "necessarily be up to date\n", 933));
}

// 0x410f40  Confidence: likely (switch with cases -1, 0, 1, 2 through the table at 0x4110a0, the error
// case being the default too; the "up to date" tail is shared with the SkipVerify path; two error tails
// store version_check_done differently, so they are two statements). A download that fails after the
// first frame and a version.nfo that cannot be read both give 3; a newer version gives 2.
int version_check_run()
{
    DWORD skip;
    int status;

    if (version_check_done)
        return version_check_result;

    if (version_check_need_registry) {
        version_check_need_registry = 0;
        os_registry_read_dword(NULL, "SkipVerify", &skip, 0);
        if (skip) {
            version_check_done = 1;
            version_check_result = 1;
            return 1;
        }
    }
    if (version_check_need_fetch) {
        version_check_need_fetch = 0;
        if (version_check_fetch() == -1) {
            version_check_done = 1;
            version_check_result = 3;
            return 3;
        }
    }

    status = version_check_poll_download();
    if (status == -1) {
        version_check_done = 1;
        version_check_result = 3;
        return 3;
    }
    if (status == 0)
        return 4;
    if (status == 1) {
        char path[512] = "";
        file_make_path(path_multicache, VERSION_FILE, path);
        switch (version_read_nfo_file(path)) {
        case 1:
        case 2:
            version_check_done = 1;
            version_check_result = 1;
            return 1;
        case 0:
            version_check_done = 1;
            version_check_result = 2;
            return 2;
        case -1:
        default:
            version_check_set_error_text();
            version_check_result = 3;
            version_check_done = 1;
            return 3;
        }
    }
    version_check_done = 1;
    version_check_result = 3;
    return 3;
}

// 0x4110b0  Confidence: likely (frame 0x414: the two 512-byte lines, the three numbers and 8 bytes not
// accounted for; strlen is recomputed for every test; the end test is the feof macro on _flag). The last
// line that is not empty and does not start with ';' is parsed. The comparison with 14000 is unsigned.
int version_read_nfo_file(const char *path)
{
    unsigned int build, major, minor;
    char line[512];
    char last[512];

    FILE *fp = fopen(path, "rt");
    if (fp == NULL)
        return -1;

    strcpy(last, "");
    strcpy(line, "");
    while (!feof(fp)) {
        fgets(line, 512, fp);
        if (strlen(line) && line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = '\0';
        if (strlen(line) && line[0] != ';')
            strcpy(last, line);
    }
    fclose(fp);

    if (!strlen(last))
        return -1;
    major = 0;
    minor = 0;
    build = 0;
    if (sscanf(last, "%i %i %i", &major, &minor, &build) != 3)
        return -1;
    return (major * 100 + minor) * 100 + build <= VERSION_CHECK_OWN_VERSION;
}
