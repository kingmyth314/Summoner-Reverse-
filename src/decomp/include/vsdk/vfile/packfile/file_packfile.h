// vsdk/vfile/packfile/file_packfile.h - packfile (.vpp) access (vsdk\vfile\packfile\file_packfile.cpp, assert
// path 0x5a9be0; code 0x533db0-0x534a1b). Not original source. decomp22 declared the two functions parse.cpp
// calls; decomp49 wrote the unit (vsdk/file_packfile.cpp) and declared the rest.
#ifndef VSDK_VFILE_PACKFILE_FILE_PACKFILE_H
#define VSDK_VFILE_PACKFILE_FILE_PACKFILE_H

#include "vsdk/vsdk.h"
#include "vsdk/os/stringpool.h"

#define PACKFILE_SIGNATURE      0x51890ace
#define MAX_PACKFILES           16         // packfile_add: Num_packfiles >= 0x10 fails
#define MAX_PACKFILE_FILES      0x34bc     // 13500 directory entries for all packfiles (packfile_alloc_directory)
#define PACKFILE_LOOKUP_SIZE    0x50e9     // 20713 hash slots
#define PACKFILE_SECTOR_SIZE    0x800
#define PACKFILE_DIR_PER_SECTOR 32         // 64-byte directory records per 2048-byte sector
#define PACKFILE_ON_CD          1          // Packfile::flags

struct Packfile;

// One file of a packfile (0x1c bytes, packfile_entries 0x2d0bfb0). symbols/types.h PackfileEntry.
struct PackfileEntry {
    int hash;                         // +0x00 packfile_hash(name), non-negative (signed idiv by the table size)
    char *name;                       // +0x04 in packfile_string_pool
    int sector;                       // +0x08 first sector, counted from the start of the .vpp
    unsigned int size;                // +0x0c bytes (packfile_assign_offsets rounds it up with shr)
    Packfile *packfile;               // +0x10
    FILE *hdd_file;                   // +0x14 open stream when the packfile is on the hard drive
    FILE *cd_file;                    // +0x18 open stream when it is on the CD
};
LAYOUT_CHECK(PackfileEntry, sizeof(PackfileEntry) == 0x1c);

// One loaded .vpp (0x34 bytes, Packfiles 0x2d68440). symbols/types.h has a byte flags + 7 pad bytes and calls
// +0x30 directory_size; packfile_add stores +0x20 and +0x24 as dwords, and +0x30 is the header's total file
// size (packfile_read_whole_file reads that many bytes of the .vpp; D2901).
struct Packfile {
    char name[0x20];                  // +0x00 e.g. "summoner.vpp" (strncpy 31)
    int flags;                        // +0x20 PACKFILE_ON_CD when found under cd_root_path; tested as a byte
    int _unk24;                       // +0x24 cleared by packfile_add, never read in this unit
    int num_files;                    // +0x28
    PackfileEntry *entries;           // +0x2c run of packfile_entries (packfile_alloc_directory)
    int file_size;                    // +0x30 header dword 3: size of the whole .vpp
};
LAYOUT_CHECK(Packfile, sizeof(Packfile) == 0x34);

// {hash, entry}; hash -1 = empty slot (Packfile_lookup 0x2d68780).
struct PackfileLookup {
    int hash;
    PackfileEntry *entry;
};
LAYOUT_CHECK(PackfileLookup, sizeof(PackfileLookup) == 8);

// The first 16 bytes of sector 0 of a .vpp (the rest of the sector is padding). Type and field names ours.
struct packfile_header {
    unsigned int signature;           // +0x00 PACKFILE_SIGNATURE
    unsigned int version;             // +0x04 >= 1 (jae)
    int num_files;                    // +0x08
    int file_size;                    // +0x0c
};

extern StringPool packfile_string_pool;                          // 0x2d0bf98
extern PackfileEntry packfile_entries[MAX_PACKFILE_FILES];       // 0x2d0bfb0 (name ours)
extern Packfile Packfiles[MAX_PACKFILES];                        // 0x2d68440
extern PackfileLookup Packfile_lookup[PACKFILE_LOOKUP_SIZE];     // 0x2d68780
extern int packfile_num_entries;                                 // 0x2d90ec8
extern int Num_packfiles;                                        // 0x2d90ecc
extern int packfile_initialized;                                 // 0x2d90ed0

void packfile_init();                                            // 0x533df0 (file_init)
void packfile_release_string_pool();                             // 0x533ef0 (file_shutdown)
void packfile_lookup_insert(PackfileEntry *entry);               // 0x533f10
// The file count (0 = not a packfile); copies the count and the size into pf first.
int  packfile_check_header(Packfile *pf, packfile_header *header);   // 0x533f60
// num records of one directory sector into pf->entries[*index ...]; always 1.
int  packfile_read_directory(Packfile *pf, ubyte *sector, int num, int *index);   // 0x533f90
int  packfile_hash(const char *name);                            // 0x534090 -1 for NULL
void packfile_assign_offsets(Packfile *pf, int first_sector);    // 0x5340d0
int  packfile_alloc_directory(Packfile *pf);                     // 0x534110 1 (DebugError when full)
int  packfile_load_cd(Packfile *pf);                             // 0x5341a0 1 = loaded
int  packfile_load_hdd(Packfile *pf);                            // 0x5342e0 1 = loaded
// 1 = loaded or already loaded, 0 = not found or no room (decomp49; was void, D2900).
int  packfile_add(const char *name);                             // 0x534420 (level_load: levelm / levele / levelt.vpp)
Packfile *packfile_find_packfile(const char *name);              // 0x5345a0 _stricmp, NULL when not loaded
PackfileEntry *packfile_find(const char *name);                  // 0x5345f0 by the file part of name
int  packfile_fopen(PackfileEntry *entry);                       // 0x534690 1 = opened (and seeked to the entry)
void packfile_fclose(PackfileEntry *entry);                      // 0x5347e0
int  packfile_fseek(PackfileEntry *entry, int offset, int origin);   // 0x534830 1 = fseek returned 0
int  packfile_fread(void *buf, int size, int count, PackfileEntry *entry, int unused);   // 0x5348a0 fread's result; the fifth argument is pushed by File::read and never read (D4106)
int  packfile_fstatus(PackfileEntry *entry);                     // 0x5348f0 always 1

// Reads the whole packfile `name` into buf (size bytes); 0 = failure (it printf's the error).
int packfile_read_whole_file(char *buf, int size, const char *name);                  // 0x534900
// Entry `name` of packfile `vpp`: its size and its offset in the packfile; 0 = not found. Argument order
// from TextFile::Open, which stores the third into TextFile::size and adds the fourth to the buffer
// (the sum.symbols comment has &offset, &size; decomp22 D206; confirmed by the stores at 0x534a08 / 0x534a14).
int packfile_find_in_packfile(const char *vpp, const char *name, int *size, int *offset);   // 0x5349a0

#endif
