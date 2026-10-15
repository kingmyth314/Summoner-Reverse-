// vsdk/vfile/file.h - the File class and the file helpers (vsdk\vfile in sum.symbols). Two units, both
// without a file name in the binary (decomp61): vsdk/unnamed_vfile_0x5128b0.cpp (paths, open-file slots,
// File open / close / seek, FileReader; 0x5128b0-0x514eb8) and vsdk/unnamed_vfile_rw_0x525720.cpp (the
// typed readers, writers and sync calls; 0x525720-0x52693c).
// Not original source. One merged declaration of the five stub File classes (DECISIONS.md D2).
#ifndef VSDK_VFILE_FILE_H
#define VSDK_VFILE_FILE_H

#include "vsdk/vsdk.h"
#include <time.h>

class vector2;
class vector3;
class matrix;
class matrix43;
class quaternion;
class String;
struct PackfileEntry;

#define PATH_ANY 9999999                // 0x98967f, the "search every path" id
#define PATH_NONE 9999998               // 0x98967e, file_exists_on_disk refuses it as well (name ours)
#define FILE_MAX_PATHS 0x200            // file_path_table entries (file_path_id_valid)
#define FILE_MAX_OPEN 4                 // open-file slots (init_file_slots)
#define FILE_BUFFER_SIZE 0x8000         // read buffer of one slot

// File::open modes (the slot keeps the whole value; check_flags(1) = read mode).
#define FILE_READ        1
#define FILE_WRITE       2
#define FILE_TEXT        0x80000000     // with FILE_WRITE: fopen "wt" instead of "wb"

// Error values in the slot (File::error) and the open / seek results (names ours).
#define FILE_ERR_IO       (-99999)      // 0xfffe7961 fopen / fseek / rename / short read failed
#define FILE_ERR_WRITE    (-2)          // fwrite returned 0
#define FILE_ERR_SHORT    (-1)          // File::read got fewer bytes than asked
#define FILE_ERR_IN_PACK  (-3)          // delete / backup / attributes of a packfile entry
#define FILE_ERR_NOT_FOUND (-4)
#define FILE_ERR_NO_SLOT  (-6)

// The three-register LFSR stream cipher of the open-file slots (lfsr_cipher__* 0x559db0-0x559e10, tagged
// vsdk\vfile, another unit). The constructor is inline: its COMDAT 0x513eb0 (sum.symbols
// lfsr_cipher__lfsr_cipher) was emitted by the vfile unit, whose implicit file_slot constructor calls it.
class lfsr_cipher {
public:
    lfsr_cipher() { regs[0] = 0; regs[1] = 0; regs[2] = 0; }   // 0x513eb0 lfsr_cipher__lfsr_cipher
    void set_key(const unsigned int *key);         // 0x559db0 zero words get default seeds
    void get_key(unsigned int *key);               // 0x559df0
    void crypt(void *buf, int len);                // 0x559e10 in place, both directions
    unsigned int regs[3];
};
LAYOUT_CHECK(lfsr_cipher, sizeof(lfsr_cipher) == 0xc);

class File;

// One open file (0x8040 bytes, file_slots 0x2694068 x4). Field names ours. The implicit constructor
// 0x513e90 (sum.symbols file_slot__file_slot) only constructs the cipher; init_file_slots runs it through
// the vector constructor iterator.
struct file_slot {
    unsigned int mode;                 // +0x0000 the File::open mode, 0 = free
    unsigned int flags;                // +0x0004 bit 0: encrypted (File::set_cipher_key)
    int version;                       // +0x0008 File::check_version / get_version / set_version
    int error;                         // +0x000c 0, or FILE_ERR_*
    int pos;                           // +0x0010 position in the file (File::tell)
    int num_ops;                       // +0x0014 read / write calls
    int num_bytes;                     // +0x0018 bytes read or written
    int size;                          // +0x001c file size (packfile entry size or _filelength)
    int buf_start;                     // +0x0020 file position of buffer[0], -1 = nothing buffered
    int buf_pos;                       // +0x0024 read position in buffer
    int buf_len;                       // +0x0028 bytes in buffer
    ubyte buffer[FILE_BUFFER_SIZE];    // +0x002c
    FILE *fp;                          // +0x802c NULL for a packfile entry
    File *owner;                       // +0x8030
    lfsr_cipher cipher;                // +0x8034
};
LAYOUT_CHECK(file_slot, sizeof(file_slot) == 0x8040);

// One search path (file_path_table 0x2693068). Entry 0 is the install root ("" directory).
struct file_path_entry {
    char *dir;                         // +0x0 lower case, backslashes ("data\ca"), in file_string_pool
    char *exts;                        // +0x4 ".tga .vbm " style list (one blank after each)
};
LAYOUT_CHECK(file_path_entry, sizeof(file_path_entry) == 8);

// File (0x114 bytes; Dash Faction rf::File has the same size). __thiscall members. File has a destructor,
// so a stack File gets an EH frame (gr_font_read_file, effects_load_bases).
class File {
public:
    File();                                              // 0x5140d0
    ~File();                                             // 0x5140f0 closes if open
    bool find(const char *name, int path_id);            // 0x5141c0 (returns AL)
    int  open(const char *name, int mode, int path_id);  // 0x514630 mode 1 read, 2 write; 0 success, < 0 failure
    // Returns the slot's error value (level_load_s3d returns it; D804).
    int  close();                                        // 0x514740
    int  check_flags(int mask);                          // 0x514100 slot mode & mask (1 = read mode)
    bool check_version(int min_ver);                     // 0x514120 slot version >= min_ver
    int  get_version();                                  // 0x514150
    void set_version(int ver);                           // 0x514170
    int  size(const char *name, int path_id);            // 0x514860 (called as size(NULL, PATH_ANY))
    int  seek(int pos, int origin);                      // 0x514910 < 0 failure
    int  tell();                                         // 0x5149f0
    bool eof();                                          // 0x514a10
    int  error();                                        // 0x514a40
    int  Delete(const char *name, int path_id);          // 0x514a80 File__delete ("delete" is a keyword); 0 = removed
    // decomp55 (the script editor). (name, path_id): find(name) first when name is not NULL, then the
    // found file's attributes (bit 2 = read-only); (new_name, name, path_id): find(name) when not NULL,
    // then rename the found file to new_name in its folder: 0, -4 not found, -3 open, -99999 rename failed.
    // sum.symbols calls the second make_backup (the script save renames the old file to "<file>.bak").
    int  get_attributes(const char *name, int path_id);  // 0x514bf0
    int  make_backup(const char *new_name, const char *name, int path_id);   // 0x514ae0

    // decomp61 (unnamed_vfile_0x5128b0.cpp). The find helpers fill full_path, path_id and found.
    bool find_at(const char *path, PackfileEntry **entry, bool use_packfiles);    // 0x513ec0
    bool find_in_path(const char *name, int path_id, PackfileEntry **entry,
                      bool use_packfiles);                                         // 0x513f30
    bool find_on_cd(const char *name, int path_id, bool use_packfiles);           // 0x513f70 (no caller)
    // install_path + the packfile's name into out (at most max - 1 chars) and the entry's byte offset
    // in the .vpp; false when the file is not in a packfile (snd_load).
    bool get_packfile_path(char *out, int *offset, int max);                       // 0x513fd0
    char *get_full_path();                               // 0x514190 full_path
    // thiscall although this is not used (ret 4; File::find calls it with ECX = this): sum.symbols
    // File__is_absolute_path / File__find_free_slot (D4105). "X:..." or a leading slash / backslash.
    bool is_absolute_path(const char *name);             // 0x513fb0 File__is_absolute_path
    int  find_free_slot();                               // 0x5140b0 File__find_free_slot, -1 when all four are open
    char *get_filename();                                // 0x5141a0 after the last backslash (no caller)
    void set_cipher_key(unsigned int key);               // 0x5143e0 (no caller)
    int  open_found(int mode);                           // 0x514460 0, -4 not found, -6 no slot, -99999
    int  get_async_status();                             // 0x5147c0 -1 error, 0 reading, 1 done (1 when not async)
    int  check_stream_error();                           // 0x514810 fflush failure sets -99999 (no caller)
    bool is_in_packfile();                               // 0x5148f0 the packfile is on the CD (no caller)
    int  get_path_id();                                  // 0x514a60 (no caller)
    bool is_open();                                      // 0x514a70 slot >= 0
    // (flags, name, path_id): flags 2 = read-only, 4 = archive (no caller).
    int  set_attributes(int flags, const char *name, int path_id);      // 0x514c60
    int  clear_attributes(int flags, const char *name, int path_id);    // 0x514ce0
    // (crc out, name, path_id): CRC-32 of the whole file; opens and closes it if it was not open (no caller).
    int  calc_crc32(unsigned int *crc, const char *name, int path_id);  // 0x514d60

    // Typed readers: the version check passes (min_ver), else the default comes back. The default is read
    // as a byte / word (mov al / mov ax, [esp+0xc]), so its type is the result type (DECISIONS.md D3).
    char   read_int8(int min_ver, char def);             // 0x525850
    ubyte  read_uint8(int min_ver, ubyte def);           // 0x5258a0
    short  read_int16(int min_ver, short def);           // 0x5258f0
    ushort read_uint16(int min_ver, ushort def);         // 0x525940 (decomp32)
    bool   read_bool(int min_ver, bool def);             // 0x525800 byte != 0 (decomp32; pathfinding passes (0, true))
    int    read_int(int min_ver, int def);               // 0x525990
    unsigned int read_uint(int min_ver, unsigned int def);   // 0x5259e0
    float  read_float(int min_ver, float def);          // 0x525a30 (decomp24)
    // (buf, size, min_ver, default): the .plr readers pass (buf, 0x20 / 0x100, 0, 0). Return unused.
    void   read_string(char *buf, int size, int min_ver, const char *def);   // 0x525ee0 (decomp24)
    int    read_line(char *buf, int size, int min_ver);  // 0x525f60 chars read (0 at end of file)
    int    read(void *buf, int len, int min_ver, int unused);   // 0x525fe0 bytes read
    // decomp28: (out, min_ver, default) readers, ret 0xc.
    void   read_vector(vector3 *v, int min_ver, const vector3 *def);          // 0x525a80
    void   read_quaternion(quaternion *q, int min_ver, const quaternion *def);   // 0x525ad0
    // uint16 length-prefixed string into buf (truncated to max), (buf, max, min_ver, default).
    void   read_string_buf(char *buf, int max, int min_ver, const char *def);   // 0x525e20
    void   read_matrix(matrix *m, int min_ver, const matrix *def);   // 0x525b40 (decomp53)
    // decomp61 (unnamed_vfile_rw_0x525720.cpp).
    void   read_matrix43(matrix43 *m, int min_ver, const matrix43 *def);   // 0x525ba0
    void   read_String(String *s, int min_ver, const char *def);           // 0x525c90 uint16 length
    void   read_String8(String *s, int min_ver, const char *def);          // 0x525ce0 (no caller)
    int    read_line_String(String *s, int min_ver);                       // 0x525d80 (no caller)
    // cdecl: (file, out) the uint16 length and the bytes into the String.
    static void read_String_raw(File *f, String *s);                       // 0x5257a0

    void write_bool(bool v);                             // 0x5261f0 (v == 1) as one byte
    void write_int8(char v);                             // 0x526210
    void write_uint8(ubyte v);                           // 0x526220
    void write_uint16(ushort v);                         // 0x526240
    void write_int16(short v);                           // 0x526230 (decomp24)
    void write_int(int v);                               // 0x526250
    void write_uint(unsigned int v);                     // 0x526260
    void write_float(float v);                           // 0x526270 (decomp24)
    void write_string_z(const char *s);                  // 0x5263e0 with its terminator (decomp24)
    void write_vector(const vector3 *v);                 // 0x526280 (decomp25, vim_write_header)
    // text (strlen when len < 0; NULL writes only the newline) then "\n". decomp61: void (EAX is not set
    // on every path and no caller reads it; D4100).
    void write_line(const char *text, int len);          // 0x5264e0
    void write(const void *buf, int len);                // 0x526410 (void: D4100)
    void write_quaternion(const quaternion *q);          // 0x526290 (decomp28)
    void write_string(const char *s, int len);           // 0x526390 uint16 length (strlen if -1), bytes (decomp28)
    void write_matrix(const matrix *m);                  // 0x5262d0 fvec, rvec, uvec (decomp53)
    void write_matrix43(const matrix43 *m);              // 0x526300 column order, then the position

    // Read or write by the open mode (check_flags(1) = read): (value, min_ver, default), ret 0xc.
    // decomp28; every s3d.cpp call pushes 0 as the scalar default.
    void sync_bool(bool *v, int min_ver, bool def);                    // 0x526580 (no caller)
    void sync_int8(char *v, int min_ver, char def);                    // 0x5265c0 (no caller)
    void sync_uint8(ubyte *v, int min_ver, ubyte def);                 // 0x526600
    void sync_int16(short *v, int min_ver, short def);                 // 0x526640
    void sync_uint16(ushort *v, int min_ver, ushort def);              // 0x526680
    void sync_int(int *v, int min_ver, int def);                       // 0x5266c0
    void sync_uint(unsigned int *v, int min_ver, unsigned int def);    // 0x526700
    void sync_float(float *v, int min_ver, float def);                 // 0x526740
    void sync_vector(vector3 *v, int min_ver, const vector3 *def);     // 0x526780
    void sync_quaternion(quaternion *q, int min_ver, const quaternion *def);   // 0x5267c0
    void sync_matrix(matrix *m, int min_ver, const matrix *def);       // 0x526800
    void sync_matrix43(matrix43 *m, int min_ver, const matrix43 *def); // 0x526840 (no caller)
    void sync_String(String *s, int min_ver, const char *def);         // 0x526880 (no caller)
    // (buf, size, min_ver, default): reads into buf (size bytes), writes buf with size -1 (strlen).
    void sync_string_buf(char *buf, int size, int min_ver, const char *def);   // 0x5268c0
    void sync_bytes(void *buf, int len, int min_ver);                  // 0x526900

    int path_id;                      // +0x000 path id of the found file (File__find_in_path, File__get_path_id)
    bool found;                       // +0x004 set by find_at (name ours; was _unk004)
    char full_path[0x103];            // +0x005 resolved path (File__get_full_path returns this + 5)
    PackfileEntry *packentry;         // +0x108 NULL when the file is on disk (File__is_in_packfile)
    ubyte _unk10c;                    // +0x10c byte, cleared by find and never read
    int slot;                         // +0x110 open-file slot index, -1 closed (File__is_open)
};
LAYOUT_CHECK(File, sizeof(File) == 0x114);

// FileReader (0x14 bytes, vsdk\vfile 0x513a20-0x513e5f; unit file not known): reads from an open File
// or from a memory block with the same calls (skeleton_read reads preloaded .mvf data this way). No
// destructor. Past the end of a memory block the readers return 0. decomp25 (skeleton.cpp).
class FileReader {
public:
    FileReader();                                        // 0x513a20 mode -1
    void attach_memory(void *buf, int size);             // 0x513a30 mode 1
    void attach_file(File *f);                           // 0x513a50 mode 0
    int  tell();                                         // 0x513a70
    int  seek(int offset, int origin);                   // 0x513a90 origin 0 set, 1 current, 2 end; 0 / -1 (decomp61)
    int  eof();                                          // 0x513b00
    int  error();                                        // 0x513b30 always 0 for memory
    int  read(void *buf, int len);                       // 0x513b40 bytes read (decomp61; 0 past the end of memory)
    int    read_int();                                   // 0x513bb0
    float  read_float();                                 // 0x513bf0
    short  read_int16();                                 // 0x513c40
    ushort read_uint16();                                // 0x513c80
    char   read_int8();                                  // 0x513cc0
    ubyte  read_uint8();                                 // 0x513d00
    void read_vector(vector3 *out);                      // 0x513d40 12 bytes
    void read_quaternion(quaternion *out);               // 0x513da0 16 bytes
    void read_string(char *buf, int max);                // 0x513e00 NUL-terminated
    int mode;                          // +0x00 0 File, 1 memory, -1 none
    File *file;                        // +0x04
    ubyte *buffer;                     // +0x08
    int pos;                           // +0x0c
    int size;                          // +0x10
};
LAYOUT_CHECK(FileReader, sizeof(FileReader) == 0x14);


void file_change_ext(char *out, const char *name, const char *ext);   // 0x5130a0
// use_packfiles is tested as a dword (test eax, eax), so int (decomp61, D4104; was bool).
void file_init(const char *install_path, int use_packfiles);         // 0x512d60 (LoadSettings: NULL, true)
void file_shutdown();                                                 // 0x512d00 atexit of file_init
int  file_add_path(const char *dir, const char *exts);               // 0x512e60 new path id
void file_remove_path(int path_id);                                  // 0x513080 (decomp52)
bool file_path_id_valid(int path_id);                                // 0x512cd0 PATH_ANY or a registered id
char *file_get_full_dir(int path_id, char *out);                     // 0x512a70 install path + dir
char *file_get_dir(int path_id, char *out);                          // 0x512af0 the dir alone (no caller)
// String versions (hidden result; no caller).
String file_get_path_string(int path_id);                            // 0x512930
String file_get_dir_string(int path_id);                             // 0x5129e0
char *file_get_ext(const char *name);                                 // 0x5131c0 from the last '.', or "" (decomp53)
void file_strip_ext(char *out, const char *name);                     // 0x513100
int  file_make_path_dirs(int path_id);                                // 0x5131e0 nonzero = failure
int  file_make_dirs(const char *path);                                // 0x513210 recursive _mkdir
// install path + directory of path_id + "\" + name into out; NULL for an invalid id or an install path of
// 256 chars or more (decomp61: returns out, D4102; the pilots ignore it).
char *file_make_path(int path_id, const char *name, char *out);      // 0x512b30
char *file_make_cd_path(int path_id, const char *name, char *out);   // 0x512c00 from cd_root_path
// decomp49 (file_packfile.cpp): the part after the last backslash of path into out (return unused).
void file_get_filename(char *out, const char *path);                  // 0x513160
extern char install_path[];          // 0x26b4168 hard drive root (packfile_load_hdd, packfile_add, packfile_fopen)
extern char cd_root_path[256];       // 0x26b627c CD root (packfile_load_cd, packfile_add, packfile_fopen)
bool file_exists_on_disk(const char *name, int path_id);             // 0x513710 _stat of file_make_path
// true when the file existed and _unlink succeeded (decomp61: bool, D4103; the pilots ignore it).
bool file_delete(const char *name, int path_id);                     // 0x513770
// Deletes every match of pattern in path_id smaller than 10 bytes (failed downloads).
void file_delete_small_files(const char *pattern, int path_id);      // 0x5137e0 (return unused)
int  file_delete_all(int path_id);                                    // 0x513280 files deleted (no caller)
char *file_getcwd(char *buf, int size);                               // 0x5138c0 (no caller)
bool file_chdrive(int drive);                                         // 0x5138e0 1 = A:
int  file_chdir(const char *path);                                    // 0x513900 0 ok, 1 bad drive, 2 failed
bool file_dir_exists(const char *path);                               // 0x5139d0 (no caller)

// file_find_files fills a circular list of 0x38-byte nodes (operator new) that the caller frees. The list
// head is either a bare {next, prev} pair (new_saveload.cpp) or a whole node (new_charselect.cpp), so the
// node derives from the head (DECISIONS.md D15). The type names are ours.
struct file_find_node;
struct file_find_list {
    file_find_node *next;              // +0x00
    file_find_node *prev;              // +0x04
};
struct file_find_node : file_find_list {
    char name[0x28];                   // +0x08 file name (with the extension when flags has bit 2)
    int attributes;                    // +0x30 2 read-only, 4 archive (decomp61, D4101)
    time_t time;                       // +0x34 _finddata_t time_write (VC6 time_t is 32-bit)
};
typedef file_find_node file_find_entry;   // new_charselect.cpp's name
typedef file_find_list file_list;         // new_hud.cpp's names (quicksave: a bare head)
typedef file_find_node file_entry;
// (list, path_id, pattern, flags, callback); flags 5 / 6 and callback 0 in the pilots. Flags: 1 sort by
// name, 2 sort by time (newest first; used when bit 1 is clear), 4 keep the extension. The callback
// gets the node before it is copied, as bool (*)(file_find_node *); false skips the file. The parameter
// stays void * (the script editor passes a cast pointer). Return unused.
void file_find_files(file_find_list *list, int path_id, const char *pattern, int flags,
                     void *callback);                                 // 0x5133b0
// Every file of every path whose directory starts with dir, one find per extension (no caller).
void file_list_files(file_find_list *list, const char *dir);          // 0x513590

// Array helpers (cdecl; vsdk\vfile in sum.symbols, decomp28): read or write by the open mode.
void file_sync_vector_array(File *f, vector3 *v, int count, int min_ver, const vector3 *def);   // 0x544ee0
void file_sync_float_pair_array(File *f, vector2 *v, int count, int min_ver, const vector2 *def);   // 0x544f30
void file_sync_int16_array(File *f, short *v, int count);             // 0x545080
// Defaults set by file_init (decomp28), defined in unnamed_vfile_rw_0x525720.cpp.
extern vector3 file_zero_vector;          // 0x2cbf4d8
extern quaternion file_identity_quaternion;   // 0x2cbf4c8
extern matrix file_default_matrix;        // 0x2cbf518
extern matrix43 file_default_matrix43;    // 0x2cbf4e8 (name ours; never set or read)

// decomp61: the vfile unit's globals.
extern file_path_entry file_path_table[FILE_MAX_PATHS];   // 0x2693068
extern file_slot file_slots[FILE_MAX_OPEN];               // 0x2694068
extern bool file_inited;                                  // 0x26b637c
extern int vfile_access_level;                            // 0x26b6380
extern bool file_root_exts_set;                           // 0x26b6384 file_add_path("" ...) (name ours; never read)
extern int file_last_path_id;                             // 0x5a7814 (-1)
extern char file_last_ext[8];                             // 0x5a7818 ("none"; name ours)

// decomp42 (state_game_enter): the access level of the file system around the level load.
void vfile_raise_access_level();      // 0x513370
void vfile_lower_access_level();      // 0x513390

// decomp47 (summoner_cd.cpp): strcpy into cd_root_path (file_make_cd_path's root).
void file_set_cd_root(const char *path);                              // 0x513e60

#endif
