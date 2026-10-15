// file_packfile.cpp - Sum.exe v1.40, code 0x533db0-0x534a1b (vsdk\vfile\packfile\file_packfile.cpp, assert
// path 0x5a9be0 in packfile_alloc_directory)
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// .data 0x5a9aac-0x5a9cf7: string literals only, in function order (the previous unit's last literal ends at
// 0x5a9aa8; gr_direct3d.cpp's data starts at 0x5a9cf8). Built without string pooling: "%s%s" four times, "rb"
// six times. .bss 0x2d0bf98-0x2d90ed3: the string pool, the entry pool, Packfiles, the lookup table and three
// ints. No "" literal.
//
// The PC storage layer: a packfile is looked for first under install_path, then under cd_root_path; the
// directory is read with stdio, and each entry later opens its own FILE (packfile_fopen).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vsdk/vfile/packfile/file_packfile.h"
#include "vsdk/vfile/file.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/os/timer.h"
#include "vsdk/os/debug.h"

// Original __FILE__ of this unit (the DebugError call pushes this literal, 0x5a9be0).
#define FILE_PACKFILE_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\vfile\\packfile\\file_packfile.cpp"

// One 64-byte directory record: a 60-byte NUL-padded name and the size (names ours).
#define PACKFILE_DIR_NAME_LEN 60

// ---- .bss, in address order ----------------------------------------------------------------
// 0x533db0 init_packfile_string_pool (initialiser table entry 0x5889c8: `call ctor; jmp register`),
// 0x533dc0 packfile_string_pool_ctor (`mov ecx, 0x2d0bf98; jmp StringPool::StringPool`),
// 0x533dd0 packfile_string_pool_register_dtor (atexit(0x533de0)),
// 0x533de0 packfile_string_pool_dtor (`mov ecx, 0x2d0bf98; jmp StringPool::~StringPool`):
// compiler-generated for this definition.  Confidence: exact.
StringPool packfile_string_pool;                        // 0x2d0bf98
PackfileEntry packfile_entries[MAX_PACKFILE_FILES];     // 0x2d0bfb0 (0x5c490 bytes; name ours)
Packfile Packfiles[MAX_PACKFILES];                      // 0x2d68440
PackfileLookup Packfile_lookup[PACKFILE_LOOKUP_SIZE];   // 0x2d68780
int packfile_num_entries;                               // 0x2d90ec8
int Num_packfiles;                                      // 0x2d90ecc
int packfile_initialized;                               // 0x2d90ed0

// 0x533df0  Confidence: likely (the load time is an int local converted with fild; the two timer_get
// arguments are left on the stack and the double is stored over them, which is the compiler's).
void packfile_init()
{
    int i;

    Num_packfiles = 0;
    packfile_num_entries = 0;
    for (i = 0; i < PACKFILE_LOOKUP_SIZE; i++) {
        Packfile_lookup[i].hash = -1;
        Packfile_lookup[i].entry = NULL;
    }
    packfile_string_pool.Allocate(0x40000);

    int start = timer_get(1000);
    packfile_add("tables.vpp");
    packfile_add("summoner.vpp");
    packfile_add("chars.vpp");
    packfile_add("items.vpp");
    packfile_add("effects.vpp");
    packfile_add("music.vpp");
    packfile_add("sounds.vpp");
    packfile_add("cutscene.vpp");
    int elapsed = timer_get(1000) - start;
    printf("TOTAL PACKFILE LOAD TIME : %.2f\n", (float)elapsed * 0.001f);
    printf("Packfile static alloc : %d\n", sizeof(packfile_entries));
    printf("TOTAL PACKFILE FILES: %d\n", packfile_num_entries);
    packfile_initialized = 1;
}

// 0x533ef0  Confidence: exact (tail jump to StringPool::Release).
void packfile_release_string_pool()
{
    if (packfile_initialized)
        packfile_string_pool.Release();
}

// 0x533f10  Confidence: exact. Linear probing; a full table drops the entry silently.
void packfile_lookup_insert(PackfileEntry *entry)
{
    int slot = entry->hash % PACKFILE_LOOKUP_SIZE;
    int i;

    for (i = 0; i < PACKFILE_LOOKUP_SIZE; i++) {
        if (Packfile_lookup[slot].hash == -1) {
            Packfile_lookup[slot].hash = entry->hash;
            Packfile_lookup[slot].entry = entry;
            return;
        }
        slot++;
        if (slot >= PACKFILE_LOOKUP_SIZE)
            slot = 0;
    }
}

// 0x533f60  Confidence: likely (the success value is the file count still in EAX; written as a return of
// pf->num_files). A packfile with no files fails like a bad header.
int packfile_check_header(Packfile *pf, packfile_header *header)
{
    pf->num_files = header->num_files;
    pf->file_size = header->file_size;
    if (header->signature != PACKFILE_SIGNATURE || header->version < 1)
        return 0;
    return pf->num_files;
}

// 0x533f90  Confidence: likely (the name is copied with a 15-dword rep movsd: memcpy of 60 bytes; the loop
// counts down in the argument slot). cd_file and the sector are set later by packfile_assign_offsets.
int packfile_read_directory(Packfile *pf, ubyte *sector, int num, int *index)
{
    char name[PACKFILE_DIR_NAME_LEN];
    unsigned int size;
    int i;

    for (i = 0; i < num; i++) {
        memcpy(name, sector, PACKFILE_DIR_NAME_LEN);
        sector += PACKFILE_DIR_NAME_LEN;
        size = *(unsigned int *)sector;
        sector += 4;
        pf->entries[*index].hash = packfile_hash(name);
        pf->entries[*index].name = packfile_string_pool.Add(name);
        pf->entries[*index].size = size;
        pf->entries[*index].packfile = pf;
        pf->entries[*index].hdd_file = NULL;
        packfile_lookup_insert(&pf->entries[*index]);
        (*index)++;
    }
    return 1;
}

// 0x534090  Confidence: likely (rol through _rotl; the character is tolower's result as a char; abs is the
// intrinsic cdq / xor / sub).
int packfile_hash(const char *name)
{
    unsigned int h;

    if (name == NULL)
        return -1;
    h = 0;
    while (*name) {
        char c = (char)tolower(*name);
        h = _rotl(h, 5) ^ c;
        name++;
    }
    return abs((int)h);
}

// 0x5340d0  Confidence: exact. Each file starts on the sector after the previous one ends.
void packfile_assign_offsets(Packfile *pf, int first_sector)
{
    int i;

    for (i = 0; i < pf->num_files; i++) {
        pf->entries[i].sector = first_sector;
        pf->entries[i].cd_file = NULL;
        first_sector += (pf->entries[i].size + PACKFILE_SECTOR_SIZE - 1) >> 11;
    }
}

// 0x534110  Confidence: likely (the count is stored before the pointer is formed). Error line 423 (0x1a7).
// The result tests the pointer, which is never NULL.
int packfile_alloc_directory(Packfile *pf)
{
    if (pf->num_files + packfile_num_entries > MAX_PACKFILE_FILES) {
        printf("SIZEOF FILE ENTRY : %d\n", sizeof(PackfileEntry));
        printf("MAX_PACKFILE_FILES : %d\n", MAX_PACKFILE_FILES);
        printf("TOTAL SIZE FOR ALL PACKFILES : %d\n", sizeof(packfile_entries));
        VSDK_ERROR(FILE_PACKFILE_CPP, 423, "Out of packfile space!\n");
    }
    pf->entries = &packfile_entries[packfile_num_entries];
    packfile_num_entries += pf->num_files;
    return pf->entries != NULL;
}

// 0x5341a0  Confidence: likely (frame: path 0x80, sector 0x800, index and the sector count as locals;
// the count of the last batch is taken before the fread). Sector 0 is the header, then one sector per 32
// directory records; the data starts after the directory.
int packfile_load_cd(Packfile *pf)
{
    int sectors;
    int index;
    char path[128];
    ubyte sector[PACKFILE_SECTOR_SIZE];
    int remaining, n;

    index = 0;
    sprintf(path, "%s%s", cd_root_path, pf->name);
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
        return 0;
    fread(sector, PACKFILE_SECTOR_SIZE, 1, fp);
    if (!packfile_check_header(pf, (packfile_header *)sector)) {
        fclose(fp);
        return 0;
    }
    if (!packfile_alloc_directory(pf)) {
        fclose(fp);
        return 0;
    }
    remaining = pf->num_files;
    sectors = 1;
    while (remaining) {
        n = remaining > PACKFILE_DIR_PER_SECTOR ? PACKFILE_DIR_PER_SECTOR : remaining;
        remaining -= n;
        fread(sector, PACKFILE_SECTOR_SIZE, 1, fp);
        if (!packfile_read_directory(pf, sector, n, &index)) {
            fclose(fp);
            return 0;
        }
        sectors++;
    }
    packfile_assign_offsets(pf, sectors);
    fclose(fp);
    return 1;
}

// 0x5342e0  Confidence: likely (as packfile_load_cd, with install_path).
int packfile_load_hdd(Packfile *pf)
{
    int sectors;
    int index;
    char path[128];
    ubyte sector[PACKFILE_SECTOR_SIZE];
    int remaining, n;

    index = 0;
    sprintf(path, "%s%s", install_path, pf->name);
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
        return 0;
    fread(sector, PACKFILE_SECTOR_SIZE, 1, fp);
    if (!packfile_check_header(pf, (packfile_header *)sector)) {
        fclose(fp);
        return 0;
    }
    if (!packfile_alloc_directory(pf)) {
        fclose(fp);
        return 0;
    }
    remaining = pf->num_files;
    sectors = 1;
    while (remaining) {
        n = remaining > PACKFILE_DIR_PER_SECTOR ? PACKFILE_DIR_PER_SECTOR : remaining;
        remaining -= n;
        fread(sector, PACKFILE_SECTOR_SIZE, 1, fp);
        if (!packfile_read_directory(pf, sector, n, &index)) {
            fclose(fp);
            return 0;
        }
        sectors++;
    }
    packfile_assign_offsets(pf, sectors);
    fclose(fp);
    return 1;
}

// 0x534420  Confidence: likely (the open test is shared by both fopen calls: where is set on each path and
// one fclose follows). The hard drive wins over the CD. The NULL test of name comes after name was already
// used by _stricmp, sprintf and fopen, and a name of 32 characters or more is refused only after the file
// was found. Packfiles[].name is not terminated by strncpy (31 characters fit; the .bss is zero).
int packfile_add(const char *name)
{
    char path[128];
    int i;
    int where;
    FILE *fp;

    for (i = 0; i < Num_packfiles; i++) {
        if (_stricmp(name, Packfiles[i].name) == 0) {
            printf("Ignoring packfile (%s) which was already loaded!\n", name);
            return 1;
        }
    }

    sprintf(path, "%s%s", install_path, name);
    fp = fopen(path, "rb");
    if (fp != NULL) {
        where = 1;
    } else {
        sprintf(path, "%s%s", cd_root_path, name);
        fp = fopen(path, "rb");
        if (fp == NULL)
            return 0;
        where = 2;
    }
    fclose(fp);

    if (Num_packfiles >= MAX_PACKFILES)
        return 0;
    if (name == NULL)
        return 0;
    if (strlen(name) > 31)
        return 0;

    Packfile *pf = &Packfiles[Num_packfiles++];
    strncpy(pf->name, name, 31);
    pf->flags = (where == 2);
    pf->_unk24 = 0;
    pf->num_files = 0;
    pf->entries = NULL;
    if (where == 2) {
        printf("Adding CD PACKFILE : %s\n", name);
        return packfile_load_cd(pf);
    }
    printf("Adding HARD DRIVE PACKFILE : %s\n", name);
    return packfile_load_hdd(pf);
}

// 0x5345a0  Confidence: exact.
Packfile *packfile_find_packfile(const char *name)
{
    int i;

    for (i = 0; i < Num_packfiles; i++) {
        if (_stricmp(name, Packfiles[i].name) == 0)
            return &Packfiles[i];
    }
    return NULL;
}

// 0x5345f0  Confidence: likely (the NULL-name test returns instead of probing on). Only the file part of
// the name is looked up, so two packfiles with the same file name shadow each other (the first one added
// wins: insertion stops at the first free slot and the search returns the first match).
PackfileEntry *packfile_find(const char *name)
{
    char filename[256];
    int hash, slot, i;

    file_get_filename(filename, name);
    hash = packfile_hash(filename);
    slot = hash % PACKFILE_LOOKUP_SIZE;
    for (i = 0; i < PACKFILE_LOOKUP_SIZE; i++) {
        if (Packfile_lookup[slot].hash == -1)
            return NULL;
        if (Packfile_lookup[slot].hash == hash) {
            if (Packfile_lookup[slot].entry->name == NULL)
                return NULL;
            if (_stricmp(Packfile_lookup[slot].entry->name, filename) == 0)
                return Packfile_lookup[slot].entry;
        }
        slot++;
        if (slot >= PACKFILE_LOOKUP_SIZE)
            slot = 0;
    }
    return NULL;
}

// 0x534690  Confidence: likely (the two fseek calls are merged into one tail by the compiler). Fails when
// the entry is open already.
int packfile_fopen(PackfileEntry *entry)
{
    char path[128];

    if (entry == NULL || entry->packfile == NULL || entry->hdd_file != NULL || entry->cd_file != NULL)
        return 0;

    if (entry->packfile->flags & PACKFILE_ON_CD) {
        strcpy(path, cd_root_path);
        strcat(path, entry->packfile->name);
        entry->cd_file = fopen(path, "rb");
        if (entry->cd_file == NULL)
            return 0;
        fseek(entry->cd_file, entry->sector * PACKFILE_SECTOR_SIZE, SEEK_SET);
    } else {
        strcpy(path, install_path);
        strcat(path, entry->packfile->name);
        entry->hdd_file = fopen(path, "rb");
        if (entry->hdd_file == NULL)
            return 0;
        fseek(entry->hdd_file, entry->sector * PACKFILE_SECTOR_SIZE, SEEK_SET);
    }
    return 1;
}

// 0x5347e0  Confidence: exact. The CD stream is closed without a NULL test.
void packfile_fclose(PackfileEntry *entry)
{
    if (entry == NULL || entry->packfile == NULL)
        return;
    if (entry->packfile->flags & PACKFILE_ON_CD) {
        fclose(entry->cd_file);
        entry->cd_file = NULL;
    } else if (entry->hdd_file != NULL) {
        fclose(entry->hdd_file);
        entry->hdd_file = NULL;
    }
}

// 0x534830  Confidence: exact. The offset is relative to the entry for every origin, so SEEK_END seeks
// from the end of the .vpp, not of the entry (File__seek is the only caller).
int packfile_fseek(PackfileEntry *entry, int offset, int origin)
{
    if (entry == NULL || entry->packfile == NULL)
        return 0;
    if (entry->packfile->flags & PACKFILE_ON_CD)
        return fseek(entry->cd_file, offset + entry->sector * PACKFILE_SECTOR_SIZE, origin) == 0;
    if (entry->hdd_file == NULL)
        return 0;
    return fseek(entry->hdd_file, offset + entry->sector * PACKFILE_SECTOR_SIZE, origin) == 0;
}

// 0x5348a0  Confidence: exact. "FREAD BAIL" is printed for a NULL entry or packfile, not for a short read;
// a hard drive entry that is not open returns 0 silently.
int packfile_fread(void *buf, int size, int count, PackfileEntry *entry)
{
    FILE *fp;

    if (entry == NULL || entry->packfile == NULL) {
        printf("FREAD BAIL\n");
        return 0;
    }
    if (entry->packfile->flags & PACKFILE_ON_CD) {
        fp = entry->cd_file;
    } else {
        fp = entry->hdd_file;
        if (fp == NULL)
            return 0;
    }
    return fread(buf, size, count, fp);
}

// 0x5348f0  Confidence: exact. File__get_async_status calls it only for CD entries.
int packfile_fstatus(PackfileEntry *entry)
{
    return 1;
}

// 0x534900  Confidence: exact. Called by file_blast_into_buffer (parse.cpp). Defects: the .vpp is opened by
// its bare name (relative to the current directory, not install_path), and a packfile on the CD reports
// success without reading anything into buf.
int packfile_read_whole_file(char *buf, int size, const char *name)
{
    Packfile *pf = packfile_find_packfile(name);
    if (pf == NULL) {
        printf("A\n");
        return 0;
    }
    if (size < pf->file_size) {
        printf("B : (%d %d)\n", size, pf->file_size);
        return 0;
    }
    if (!(pf->flags & PACKFILE_ON_CD)) {
        FILE *fp = fopen(pf->name, "rb");
        if (fp == NULL) {
            printf("C\n");
            return 0;
        }
        fread(buf, pf->file_size, 1, fp);
        fclose(fp);
    }
    return 1;
}

// 0x5349a0  Confidence: exact. A linear _stricmp search of one packfile's directory (TextFile::Open).
int packfile_find_in_packfile(const char *vpp, const char *name, int *size, int *offset)
{
    Packfile *pf = packfile_find_packfile(vpp);
    int i;

    if (pf == NULL)
        return 0;
    for (i = 0; i < pf->num_files; i++) {
        if (_stricmp(name, pf->entries[i].name) == 0) {
            *size = pf->entries[i].size;
            *offset = pf->entries[i].sector * PACKFILE_SECTOR_SIZE;
            return 1;
        }
    }
    return 0;
}
