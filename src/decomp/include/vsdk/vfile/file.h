// vsdk/vfile/file.h - the File class and the file helpers (vsdk\vfile in sum.symbols: 0x5130a0-0x514ae0
// and the typed readers / writers 0x525850-0x526900; the unit file names are not in the binary).
// Not original source. One merged declaration of the five stub File classes (DECISIONS.md D2).
#ifndef VSDK_VFILE_FILE_H
#define VSDK_VFILE_FILE_H

#include "vsdk/vsdk.h"
#include <time.h>

class vector2;
class vector3;
class matrix;
class quaternion;

#define PATH_ANY 9999999                // 0x98967f, the "search every path" id

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
    int  check_flags(int mask);                          // 0x514100 slot flags & mask (1 = read mode)
    bool check_version(int min_ver);                     // 0x514120 slot version >= min_ver
    int  get_version();                                  // 0x514150
    void set_version(int ver);                           // 0x514170
    int  size(const char *name, int path_id);            // 0x514860 (called as size(NULL, PATH_ANY))
    int  seek(int pos, int origin);                      // 0x514910 < 0 failure
    int  tell();                                         // 0x5149f0
    bool eof();                                          // 0x514a10
    int  error();                                        // 0x514a40
    int  Delete(const char *name, int path_id);          // 0x514a80 File__delete ("delete" is a keyword); 0 = removed

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
    int    read_line(char *buf, int size, int unused);   // 0x525f60 (0 at end of file)
    int    read(void *buf, int len, int min_ver, int unused);   // 0x525fe0 bytes read
    // decomp28: (out, min_ver, default) readers, ret 0xc.
    void   read_vector(vector3 *v, int min_ver, const vector3 *def);          // 0x525a80
    void   read_quaternion(quaternion *q, int min_ver, const quaternion *def);   // 0x525ad0
    // uint16 length-prefixed string into buf (truncated to max), (buf, max, min_ver, default).
    void   read_string_buf(char *buf, int max, int min_ver, const char *def);   // 0x525e20

    void write_uint8(ubyte v);                           // 0x526220
    void write_uint16(ushort v);                         // 0x526240
    void write_int16(short v);                           // 0x526230 (decomp24)
    void write_int(int v);                               // 0x526250
    void write_float(float v);                           // 0x526270 (decomp24)
    void write_string_z(const char *s);                  // 0x5263e0 with its terminator (decomp24)
    void write_vector(const vector3 *v);                 // 0x526280 (decomp25, vim_write_header)
    // text (strlen when len < 0; NULL writes only the newline) then "\n"; < 0 failure (decomp22).
    int  write_line(const char *text, int len);          // 0x5264e0
    int  write(const void *buf, int len);                // 0x526410
    void write_quaternion(const quaternion *q);          // 0x526290 (decomp28)
    void write_string(const char *s, int len);           // 0x526390 uint16 length (strlen if -1), bytes (decomp28)

    // Read or write by the open mode (check_flags(1) = read): (value, min_ver, default), ret 0xc.
    // decomp28; every s3d.cpp call pushes 0 as the scalar default.
    void sync_uint8(ubyte *v, int min_ver, ubyte def);                 // 0x526600
    void sync_int16(short *v, int min_ver, short def);                 // 0x526640
    void sync_uint16(ushort *v, int min_ver, ushort def);              // 0x526680
    void sync_int(int *v, int min_ver, int def);                       // 0x5266c0
    void sync_uint(unsigned int *v, int min_ver, unsigned int def);    // 0x526700
    void sync_float(float *v, int min_ver, float def);                 // 0x526740
    void sync_vector(vector3 *v, int min_ver, const vector3 *def);     // 0x526780
    void sync_quaternion(quaternion *q, int min_ver, const quaternion *def);   // 0x5267c0
    void sync_matrix(matrix *m, int min_ver, const matrix *def);       // 0x526800
    // (buf, size, min_ver, default): reads into buf (size bytes), writes buf with size -1 (strlen).
    void sync_string_buf(char *buf, int size, int min_ver, const char *def);   // 0x5268c0
    void sync_bytes(void *buf, int len, int min_ver);                  // 0x526900

    int path_id;                      // +0x000 path id of the found file (File__find_in_path, File__get_path_id)
    ubyte _unk004;                     // +0x004
    char full_path[0x103];             // +0x005 resolved path (File__get_full_path returns this + 5)
    void *packentry;                   // +0x108 (File__is_in_packfile)
    int _unk10c;                       // +0x10c
    int slot;                          // +0x110 open-file slot index, -1 closed (File__is_open)
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
    void seek(int offset, int origin);                   // 0x513a90 origin 0 set, 1 current, 2 end (return unused)
    int  eof();                                          // 0x513b00
    int  error();                                        // 0x513b30 always 0 for memory
    void read(void *buf, int len);                       // 0x513b40 (return unused)
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
void file_init(const char *install_path, bool use_packfiles);        // 0x512d60 (LoadSettings: NULL, true)
int  file_add_path(const char *dir, const char *exts);               // 0x512e60 new path id
char *file_get_full_dir(int path_id, char *out);                     // 0x512a70 install path + dir
void file_strip_ext(char *out, const char *name);                     // 0x513100
int  file_make_path_dirs(int path_id);                                // 0x5131e0 nonzero = failure
// install path + directory of path_id + name into out (return value not used by the pilots).
void file_make_path(int path_id, const char *name, char *out);       // 0x512b30
bool file_exists_on_disk(const char *name, int path_id);             // 0x513710 _stat of file_make_path
void file_delete(const char *name, int path_id);                     // 0x513770 (return unused)
// Deletes every match of pattern in path_id smaller than 10 bytes (failed downloads).
void file_delete_small_files(const char *pattern, int path_id);      // 0x5137e0 (return unused)

// file_find_files fills a circular list of 0x38-byte nodes (operator new) that the caller frees. The list
// head is either a bare {next, prev} pair (new_saveload.cpp) or a whole node (new_charselect.cpp), so the
// node derives from the head (DECISIONS.md D15). The type names are ours.
struct file_find_node;
struct file_find_list {
    file_find_node *next;              // +0x00
    file_find_node *prev;              // +0x04
};
struct file_find_node : file_find_list {
    char name[0x2c];                   // +0x08 file name (with the extension when flags has bit 1)
    time_t time;                       // +0x34 _finddata_t time_write (VC6 time_t is 32-bit)
};
typedef file_find_node file_find_entry;   // new_charselect.cpp's name
typedef file_find_list file_list;         // new_hud.cpp's names (quicksave: a bare head)
typedef file_find_node file_entry;
// (list, path_id, pattern, flags, callback); flags 5 / 6 and callback 0 in the pilots. Return unused.
void file_find_files(file_find_list *list, int path_id, const char *pattern, int flags,
                     void *callback);                                 // 0x5133b0

// Array helpers (cdecl; vsdk\vfile in sum.symbols, decomp28): read or write by the open mode.
void file_sync_vector_array(File *f, vector3 *v, int count, int min_ver, const vector3 *def);   // 0x544ee0
void file_sync_float_pair_array(File *f, vector2 *v, int count, int min_ver, const vector2 *def);   // 0x544f30
void file_sync_int16_array(File *f, short *v, int count);             // 0x545080
// Defaults set by file_init (decomp28).
extern vector3 file_zero_vector;          // 0x2cbf4d8
extern quaternion file_identity_quaternion;   // 0x2cbf4c8
extern matrix file_default_matrix;        // 0x2cbf518


#endif
