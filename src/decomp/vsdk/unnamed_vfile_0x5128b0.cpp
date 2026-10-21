// unnamed_vfile_0x5128b0.cpp - Sum.exe v1.40, code 0x5128b0-0x514eb8 (vsdk\vfile; the unit's file name
// is not in the binary, so the name is a placeholder). Written from the Sum.exe disassembly
// (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2. Confidence per function: exact = expect
// identical code, likely = same semantics, small layout or scheduling risk, unsure = the source shape is
// a guess.
//
// The file system of the vsdk: the search paths (file_add_path and the path helpers), the four open-file
// slots with their 32 KB read buffers, File's find / open / close / seek / tell and the attribute and
// rename helpers, directory scans (file_find_files) and FileReader, which reads a File or a memory block
// with the same calls. The typed readers and writers (File::read_int ...) are a separate unit
// (unnamed_vfile_rw_0x525720.cpp).
//
// Unit bounds (NOTES.md): after memory.cpp's last function (deallocate_from_commonblock 0x512880) and
// before plainstring.cpp (is_blank 0x514ec0). Initialiser table entries 0x5888f8 (the path string pool)
// and 0x5888fc (the file slots). .bss 0x2693068-0x26b638f, .data 0x5a77a4-0x5a783b. The EH funclets of
// file_get_path_string, file_get_dir_string, file_delete_all and file_delete_small_files are one block
// (0x57d0b0-0x57d12b, int3 padding on both sides).
//
// String literals: in .data in first-use order (0x5a77a4 "\\" ... 0x5a7834 "%s\\%s"); the two all-zero ""
// literals are in .bss (0x26b6388 file_get_dir_string, 0x26b638c file_get_ext).

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#include "vsdk/vfile/file.h"
#include "vsdk/vfile/packfile/file_packfile.h"
#include "vsdk/os/vstring.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/os/console.h"
#include "vsdk/os/crc32.h"
#include "vsdk/math/matrix.h"
#include "vsdk/bmpman/bmpman.h"
#include "summoner/summoner.h"

// ---- .bss ------------------------------------------------------------------------------------------
file_path_entry file_path_table[FILE_MAX_PATHS];   // 0x2693068
// 0x512900  init_file_slots: the dynamic initialiser (jmp to 0x512910: `vector constructor iterator`
// (0x404880) of file_slot::file_slot over the four slots). Confidence: exact (compiler-generated).
// 0x513e90  file_slot::file_slot, implicit (constructs cipher, returns this). Confidence: exact
// (compiler-generated).
// 0x513eb0  lfsr_cipher::lfsr_cipher, the inline constructor's COMDAT (vsdk/vfile/file.h; sum.symbols
// lfsr_cipher__lfsr_cipher). Confidence: exact (compiler-generated).
file_slot file_slots[FILE_MAX_OPEN];               // 0x2694068
char install_path[0x100];                          // 0x26b4168
// The pool that holds the path directories and extension lists (names ours).
// 0x5128b0  init_stringpool_26b427c_thunk (call 0x5128c0, jmp 0x5128e0). Confidence: exact (compiler-generated).
// 0x5128c0  init_stringpool_26b427c: StringPool(buffer, 0x2000, false). Confidence: exact (compiler-generated).
// 0x5128e0  stringpool_26b4268_register_dtor: atexit(0x5128f0). Confidence: exact (compiler-generated).
// 0x5128f0  stringpool_26b4268_dtor: ~StringPool (0x544e20). Confidence: exact (compiler-generated).
static char file_string_pool_buffer[0x2000];       // 0x26b427c
StringPool file_string_pool(file_string_pool_buffer, 0x2000, false);   // 0x26b4268
char cd_root_path[256];                            // 0x26b627c
bool file_inited;                                  // 0x26b637c
int vfile_access_level;                            // 0x26b6380
bool file_root_exts_set;                           // 0x26b6384

// 0x512930  Confidence: exact (the EH state numbering). install_path + the path's directory, or for the
// root the install path without its trailing backslash (substr to size - 2). No caller.
String file_get_path_string(int path_id)
{
    String s(install_path);

    if (path_id)
        return s + file_path_table[path_id].dir;
    return s.substr(0, s.size() - 2);
}

// 0x5129e0  Confidence: exact. No caller.
String file_get_dir_string(int path_id)
{
    String s("");

    if (path_id)
        s = file_path_table[path_id].dir;
    return s;
}

// 0x512a70  Confidence: exact.
char *file_get_full_dir(int path_id, char *out)
{
    strcpy(out, install_path);
    if (path_id)
        strcat(out, file_path_table[path_id].dir);
    else
        out[strlen(out) - 1] = '\0';
    return out;
}

// 0x512af0  Confidence: exact. No caller.
char *file_get_dir(int path_id, char *out)
{
    out[0] = '\0';
    if (path_id)
        strcpy(out, file_path_table[path_id].dir);
    return out;
}

// 0x512b30  Confidence: exact. PATH_ANY passes file_path_id_valid and then indexes file_path_table with
// 9999999 (no caller passes it; NOTES.md).
char *file_make_path(int path_id, const char *name, char *out)
{
    if (!file_path_id_valid(path_id))
        return NULL;
    if (strlen(install_path) >= 0x100)
        return NULL;
    strcpy(out, install_path);
    if (path_id) {
        strcat(out, file_path_table[path_id].dir);
        strcat(out, "\\");
    }
    if (name)
        strcat(out, name);
    return out;
}

// 0x512c00  Confidence: exact. As file_make_path from the CD root (only caller File::find_on_cd).
char *file_make_cd_path(int path_id, const char *name, char *out)
{
    if (!file_path_id_valid(path_id))
        return NULL;
    if (strlen(cd_root_path) >= 0x100)
        return NULL;
    strcpy(out, cd_root_path);
    if (path_id) {
        strcat(out, file_path_table[path_id].dir);
        strcat(out, "\\");
    }
    if (name)
        strcat(out, name);
    return out;
}

// 0x512cd0  Confidence: exact (the id <= 0 test is a jle after the range check).
bool file_path_id_valid(int path_id)
{
    if (path_id == PATH_ANY)
        return true;
    if (path_id >= 0 && path_id < FILE_MAX_PATHS && (path_id <= 0 || file_path_table[path_id].dir))
        return true;
    return false;
}

// 0x512d00  Confidence: exact. The atexit handler of file_init.
void file_shutdown()
{
    int i;

    for (i = 0; i < FILE_MAX_OPEN; i++) {
        if (file_slots[i].mode)
            file_slots[i].owner->close();
    }
    for (i = 0; i < FILE_MAX_PATHS; i++) {
        file_path_table[i].dir = NULL;
        file_path_table[i].exts = NULL;
    }
    packfile_release_string_pool();
    install_path[0] = '\0';
    cd_root_path[0] = '\0';
    file_inited = false;
}

// 0x512d60  Confidence: exact (GetModuleFileNameA's buffer is a 256-byte local copied with strcpy).
void file_init(const char *path, int use_packfiles)
{
    char exe_path[256];

    install_path[0] = '\0';
    if (path) {
        strcpy(install_path, path);
        int len = strlen(install_path);
        if (install_path[len - 1] != '\\') {
            install_path[len] = '\\';
            install_path[len + 1] = '\0';
        }
    } else {
        GetModuleFileNameA(NULL, exe_path, 255);
        strcpy(install_path, exe_path);
        strrchr(install_path, '\\')[1] = '\0';
    }
#ifdef FIXED_BUILD
    // FIXED_BUILD: C310 LongInstallPath. The game joins install_path and file names in 128-byte stack buffers
    // (packfile_add, packfile_fopen ...), so a path of 64 characters or more is replaced by its 8.3 short form
    // (the same folder, with the trailing backslash). A volume without 8.3 names keeps the long path.
    {
        int path_len = strlen(install_path);
        if (path_len >= 64) {
            char short_path[MAX_PATH];
            DWORD n = GetShortPathNameA(install_path, short_path, sizeof short_path);
            if (n != 0 && n < sizeof short_path - 1 && n < (DWORD)path_len) {
                if (short_path[n - 1] != '\\') {
                    short_path[n++] = '\\';
                    short_path[n] = '\0';
                }
                memcpy(install_path, short_path, n + 1);
            }
        }
    }
#endif
    file_zero_vector.zero();
    file_identity_quaternion.make_identity();
    file_default_matrix.make_identity();
    file_inited = true;
    if (use_packfiles)
        packfile_init();
    atexit(file_shutdown);
}

// 0x512e60  Confidence: exact (an indexed signed loop over the table: VC6 strength-reduces it and keeps `jl`).
// Registers a search path (lower case, '/' turned into '\') with its extension list (" " appended) and
// returns its id; the empty directory is the root (id 0, only its extensions are stored). A directory
// that is already registered returns its id; the loop over the new extensions looks each one up in the
// old list with stristr and drops the result, so the old list is never extended (NOTES.md). -1 when all
// 511 ids are used.
int file_add_path(const char *dir, const char *exts)
{
    char ext[8];
    char dir_buf[256];
    char exts_buf[256];
    int free_id = -1;
    int i;

    strcpy(dir_buf, dir);
    strcpy(exts_buf, exts);
    strlwr(dir_buf);
    strlwr(exts_buf);
    if (strlen(exts_buf))
        strcat(exts_buf, " ");
    int len = strlen(dir_buf);
    for (i = 0; i < len; i++) {
        if (dir_buf[i] == '/' || dir_buf[i] == '\\')
            dir_buf[i] = '\\';
    }
    if (strlen(dir_buf) == 0) {
        file_path_table[0].exts = file_string_pool.Add(exts_buf);
        file_root_exts_set = true;
        return 0;
    }

    for (i = 1; i < FILE_MAX_PATHS; i++) {
        if (file_path_table[i].dir == NULL && free_id < 0)
            free_id = i;
        if (file_path_table[i].dir != NULL && _stricmp(file_path_table[i].dir, dir_buf) == 0) {
            const char *s = exts;
            while (s) {
                const char *next = strstr(s, " .");
                if (next) {
                    strncpy(ext, s, next - s);
                    ext[next - s] = '\0';
                    s = next + 1;
                } else {
                    strcpy(ext, s);
                    s = NULL;
                }
                stristr(file_path_table[i].exts, ext);
            }
            return i;
        }
    }
    if (free_id < 0)
        return -1;
    file_path_table[free_id].dir = (char *)file_string_pool.AddUnique(dir_buf);
    file_path_table[free_id].exts = (char *)file_string_pool.AddUnique(exts_buf);
    return free_id;
}

// 0x513080  Confidence: exact.
void file_remove_path(int path_id)
{
    file_path_table[path_id].dir = NULL;
    file_path_table[path_id].exts = NULL;
}

// 0x5130a0  Confidence: exact.
void file_change_ext(char *out, const char *name, const char *ext)
{
    file_strip_ext(out, name);
    if (*ext != '.')
        strcat(out, ".");
    strcat(out, ext);
}

// 0x513100  Confidence: exact (memcpy is the intrinsic rep movsd / movsb).
void file_strip_ext(char *out, const char *name)
{
    const char *dot = strrchr(name, '.');

    if (dot == NULL) {
        strcpy(out, name);
        return;
    }
    memcpy(out, name, dot - name);
    out[dot - name] = '\0';
}

// 0x513160  Confidence: exact.
void file_get_filename(char *out, const char *path)
{
    const char *p = strrchr(path, '\\');

    if (p == NULL)
        strcpy(out, path);
    else
        strcpy(out, p + 1);
}

// 0x5131c0  Confidence: exact.
char *file_get_ext(const char *name)
{
    char *dot = (char *)strrchr(name, '.');

    if (dot == NULL)
        dot = (char *)"";
    return dot;
}

// 0x5131e0  Confidence: exact.
int file_make_path_dirs(int path_id)
{
    char dir[256];

    return file_make_dirs(file_get_full_dir(path_id, dir));
}

// 0x513210  Confidence: exact. Creates the parent directories first (recursion on the path without its
// last component), then the directory itself; the _mkdir result is ignored, so the function always
// returns 0.
int file_make_dirs(const char *path)
{
    char buf[256];

    strcpy(buf, path);
    char *p = strrchr(buf, '\\');
    if (p) {
        *p = '\0';
        int r = file_make_dirs(buf);
        if (r)
            return r;
    }
    _mkdir(path);
    return 0;
}

// 0x513280  Confidence: exact (File is constructed after file_make_path). Deletes every plain file of
// the path that is not read-only; returns the count. No caller.
int file_delete_all(int path_id)
{
    int count = 0;
    char path[256];

    file_make_path(path_id, "*", path);
    File f;
    struct _finddata_t fd;
    long handle = _findfirst(path, &fd);
    if (handle != -1) {
        do {
            if (!(fd.attrib & (_A_SUBDIR | _A_RDONLY))) {
                f.find(fd.name, path_id);
                f.Delete(NULL, PATH_ANY);
                count++;
            }
        } while (_findnext(handle, &fd) == 0);
        _findclose(handle);
    }
    return count;
}

// 0x513370  Confidence: exact.
void vfile_raise_access_level()
{
    sprintf(string_buffer, "Group file access raised to level %d.\n", ++vfile_access_level);
}

// 0x513390  Confidence: exact.
void vfile_lower_access_level()
{
    sprintf(string_buffer, "Group file access lowered to level %d.\n", --vfile_access_level);
}

// 0x5133b0  Confidence: exact (each sort loop starts again from list->next; the splices go through a saved neighbour).
// Fills list with the plain files of path_id matching pattern. Flags: 4 keeps the extension, 1 sorts
// by name (_stricmp), else 2 sorts by time, newest first; neither appends in find order. The callback
// sees the stack copy and can reject the file.
void file_find_files(file_find_list *list, int path_id, const char *pattern, int flags, void *callback)
{
    long handle;
    file_find_node entry;
    char base[80];
    struct _finddata_t fd;
    char path[256];

    list->next = (file_find_node *)list;
    list->prev = (file_find_node *)list;
    file_make_path(path_id, pattern, path);
    handle = _findfirst(path, &fd);
    if (handle == -1)
        return;
    do {
        if (fd.attrib & _A_SUBDIR)
            continue;
        if (flags & 4) {
            strcpy(entry.name, fd.name);
        } else {
            file_strip_ext(base, fd.name);
            strcpy(entry.name, base);
        }
        entry.time = fd.time_write;
        entry.attributes = 0;
        if (fd.attrib & _A_RDONLY)
            entry.attributes = 2;
        if (fd.attrib & _A_ARCH)
            entry.attributes |= 4;
        if (callback && !((bool (*)(file_find_node *))callback)(&entry))
            continue;

        file_find_node *node = new file_find_node;
        *node = entry;
        file_find_node *p = list->next;
        if (p != (file_find_node *)list && (flags & 3)) {
            if (flags & 1) {
                for (p = list->next; p != (file_find_node *)list; p = p->next) {
                    if (_stricmp(p->name, node->name) >= 0)
                        break;
                }
            } else {
                for (p = list->next; p != (file_find_node *)list; p = p->next) {
                    if (p->time < node->time)
                        break;
                }
            }
            if (p != (file_find_node *)list) {
                file_find_node *prev = p->prev;
                p->prev = node;
                node->prev = prev;
                prev->next = node;
                node->next = p;
                continue;
            }
        }
        file_find_node *last = list->prev;
        node->next = (file_find_node *)list;
        node->prev = last;
        list->prev = node;
        last->next = node;
    } while (_findnext(handle, &fd) == 0);
    _findclose(handle);
}

// 0x513590  Confidence: exact (D18605: ext is a 5-byte array declared last, which puts it at the bottom
// of the frame below p and i, as the binary). For every path id
// from 0 up to the first unregistered one whose full directory starts with dir, one file_find_files per
// extension of its list (". xxx " steps of 5), the results spliced onto list. A find that matches
// nothing splices its stack head into list (NOTES.md). No caller.
void file_list_files(file_find_list *list, const char *dir)
{
    file_find_node found;
    char full_dir[256];
    int i;
    char ext[5];

    list->next = (file_find_node *)list;
    list->prev = (file_find_node *)list;
    for (i = 0; i < FILE_MAX_PATHS; i++) {
        file_path_entry *p = &file_path_table[i];
        if (!file_path_id_valid(i))
            return;
        if (_strnicmp(file_get_full_dir(i, full_dir), dir, strlen(dir)) != 0)
            continue;
        int offset = 0;
        strncpy(ext, p->exts, 4);
        ext[4] = '\0';
        while (strlen(ext)) {
            char pattern[8] = "*";      // word "*", then a dword and a word of zeroes
            strcat(pattern, ext);
            file_find_files(&found, i, pattern, 4, NULL);
            list->prev->next = found.next;
            found.next->prev = list->prev;
            list->prev = found.prev;
            found.prev->next = (file_find_node *)list;
            offset += 5;
            if (offset >= (int)strlen(p->exts))
                break;
            strncpy(ext, p->exts + offset, 4);
            ext[4] = '\0';
        }
    }
}

// 0x513710  Confidence: exact.
bool file_exists_on_disk(const char *name, int path_id)
{
    struct _stat st;
    char path[512];

    if (path_id == PATH_ANY || path_id == PATH_NONE)
        return false;
    file_make_path(path_id, name, path);
    if (_stat(path, &st) == 0)
        return true;
    return false;
}

// 0x513770  Confidence: exact.
bool file_delete(const char *name, int path_id)
{
    char path[512];

    if (!file_exists_on_disk(name, path_id))
        return false;
    file_make_path(path_id, name, path);
    SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL);
    return _unlink(path) == 0;
}

// 0x5137e0  Confidence: exact. The File local is constructed and destroyed but never used.
void file_delete_small_files(const char *pattern, int path_id)
{
    struct _finddata_t fd;
    File f;
    char path[512];

    file_make_path(path_id, pattern, path);
    long handle = _findfirst(path, &fd);
    if (handle != -1) {
        do {
            if (!(fd.attrib & _A_SUBDIR) && fd.size < 10)
                file_delete(fd.name, path_id);
        } while (_findnext(handle, &fd) == 0);
        _findclose(handle);
    }
}

// 0x5138c0  Confidence: exact. No caller.
char *file_getcwd(char *buf, int size)
{
    return _getcwd(buf, size);
}

// 0x5138e0  Confidence: exact.
bool file_chdrive(int drive)
{
    if (_chdrive(drive) == 0)
        return true;
    return false;
}

// 0x513900  Confidence: exact.
// A "x:" prefix switches the drive first; an empty rest changes to the root of that drive.
int file_chdir(const char *path)
{
    char buf[256];

    strcpy(buf, path);
    _strlwr(buf);
    int drive = _getdrive();
    char *colon = strchr(buf, ':');
    char *p;
    if (colon) {
        if (!file_chdrive(colon[-1] - 'a' + 1))
            return 1;
        p = colon + 1;
    } else {
        p = buf;
    }
    char root[] = "\\.";
    if (*p == '\0')
        p = root;
    if (_chdir(p)) {
        file_chdrive(drive);
        return 2;
    }
    return 0;
}

// 0x5139d0  Confidence: exact. Changes to path and back; true when path could be entered. The drive is
// not restored when path was on another drive (file_chdir(cwd) switches it back only if cwd has one, and
// _getcwd gives one, so it is). No caller.
bool file_dir_exists(const char *path)
{
    char cwd[256];

    _getcwd(cwd, 255);
    if (file_chdir(path) == 0) {
        file_chdir(cwd);
        return true;
    }
    return false;
}

// ---- FileReader ------------------------------------------------------------------------------------

// 0x513a20  Confidence: exact.
FileReader::FileReader()
{
    mode = -1;
    buffer = NULL;
}

// 0x513a30  Confidence: exact.
void FileReader::attach_memory(void *buf, int size)
{
    mode = 1;
    buffer = (ubyte *)buf;
    this->size = size;
    pos = 0;
}

// 0x513a50  Confidence: exact.
void FileReader::attach_file(File *f)
{
    mode = 0;
    file = f;
    buffer = NULL;
    size = -1;
    pos = 0;
}

// 0x513a70  Confidence: exact.
int FileReader::tell()
{
    if (mode != 0)
        return pos;
    return file->tell();
}

// 0x513a90  Confidence: exact (case 0 shares case 1's bound check). Memory blocks: only positive
// offsets are accepted (seek(0, 0) fails), and origin 2 counts back from the current position, not from
// the end (NOTES.md). 0 or -1.
int FileReader::seek(int offset, int origin)
{
    if (mode != 0) {
        switch (origin) {
        case 0:
            if (offset > 0 && offset < size) {
                pos = offset;
                return 0;
            }
            break;
        case 1:
            if (offset > 0 && offset + pos < size) {
                pos = offset + pos;
                return 0;
            }
            break;
        case 2:
            if (offset > 0 && pos - offset >= 0) {
                pos = pos - offset;
                return 0;
            }
            break;
        }
        return -1;
    }
    return file->seek(offset, origin);
}

// 0x513b00  Confidence: exact.
int FileReader::eof()
{
    if (mode != 0)
        return pos >= size;
    return file->eof();
}

// 0x513b30  Confidence: exact.
int FileReader::error()
{
    if (mode != 0)
        return 0;
    return file->error();
}

// 0x513b40  Confidence: exact (VC6 byte match, w6 pass; was likely). In memory a read that would reach the last byte fails (pos + len must
// stay below size; the typed readers below have the same bound).
int FileReader::read(void *buf, int len)
{
    if (mode != 0) {
        if (pos + len >= size)
            return 0;
        memcpy(buf, buffer + pos, len);
        pos += len;
        return len;
    }
    return file->read(buf, len, 0, 0);
}

// 0x513bb0  Confidence: exact (VC6 byte match, w6 pass; was likely) (differs under cl 8966 in match26).
int FileReader::read_int()
{
    if (mode != 0) {
        if (pos + 4 >= size)
            return 0;
        int v = *(int *)(buffer + pos);
        pos += 4;
        return v;
    }
    return file->read_int(0, 0);
}

// 0x513bf0  Confidence: exact (VC6 byte match, w6 pass; was likely) (differs under cl 8966 in match26; 0.0f is the constant 0x57eb84).
float FileReader::read_float()
{
    if (mode != 0) {
        if (pos + 4 >= size)
            return 0.0f;
        float v;
        memcpy(&v, buffer + pos, 4);
        pos += 4;
        return v;
    }
    return file->read_float(0, 0.0f);
}

// 0x513c40  Confidence: exact (VC6 byte match, w6 pass; was likely) (differs under cl 8966 in match26).
short FileReader::read_int16()
{
    if (mode != 0) {
        if (pos + 2 >= size)
            return 0;
        short v = *(short *)(buffer + pos);
        pos += 2;
        return v;
    }
    return file->read_int16(0, 0);
}

// 0x513c80  Confidence: exact (VC6 byte match, w6 pass; was likely) (differs under cl 8966 in match26).
ushort FileReader::read_uint16()
{
    if (mode != 0) {
        if (pos + 2 >= size)
            return 0;
        ushort v = *(ushort *)(buffer + pos);
        pos += 2;
        return v;
    }
    return file->read_uint16(0, 0);
}

// 0x513cc0  Confidence: exact (VC6 byte match, w6 pass; was likely) (differs under cl 8966 in match26).
char FileReader::read_int8()
{
    if (mode != 0) {
        if (pos + 1 >= size)
            return 0;
        char v = *(char *)(buffer + pos);
        pos += 1;
        return v;
    }
    return file->read_int8(0, 0);
}

// 0x513d00  Confidence: exact (VC6 byte match, w6 pass; was likely) (differs under cl 8966 in match26).
ubyte FileReader::read_uint8()
{
    if (mode != 0) {
        if (pos + 1 >= size)
            return 0;
        ubyte v = buffer[pos];
        pos += 1;
        return v;
    }
    return file->read_uint8(0, 0);
}

// 0x513d40  Confidence: exact (VC6 byte match, w6 pass; was likely) (the three dword copies). Past the end the vector is left as it is.
void FileReader::read_vector(vector3 *out)
{
    if (mode != 0) {
        if (pos + 12 >= size)
            return;
        out->x = *(float *)(buffer + pos);
        pos += 4;
        out->y = *(float *)(buffer + pos);
        pos += 4;
        out->z = *(float *)(buffer + pos);
        pos += 4;
        return;
    }
    file->read_vector(out, 0, &file_zero_vector);
}

// 0x513da0  Confidence: exact (VC6 byte match, w6 pass; was likely) (as read_vector).
void FileReader::read_quaternion(quaternion *out)
{
    if (mode != 0) {
        if (pos + 16 >= size)
            return;
        out->x = *(float *)(buffer + pos);
        pos += 4;
        out->y = *(float *)(buffer + pos);
        pos += 4;
        out->z = *(float *)(buffer + pos);
        pos += 4;
        out->w = *(float *)(buffer + pos);
        pos += 4;
        return;
    }
    file->read_quaternion(out, 0, &file_identity_quaternion);
}

// 0x513e00  Confidence: exact (`while (1)` with the end test first: VC6 keeps the test at the top). Copies up to max chars and then terminates at buf[max] (one byte past a
// max-sized buffer, as File::read_string).
void FileReader::read_string(char *buf, int max)
{
    if (mode != 0) {
        int i = 0;
        while (1) {
            if (pos >= size)
                break;
            int c = buffer[pos++];
            if (c == 0)
                break;
            if (i < max)
                buf[i++] = c;
        }
        buf[i] = '\0';
        return;
    }
    file->read_string(buf, max, 0, 0);
}

// 0x513e60  Confidence: exact.
void file_set_cd_root(const char *path)
{
    if (path)
        strcpy(cd_root_path, path);
}

// ---- File ------------------------------------------------------------------------------------------

// 0x513ec0  Confidence: exact (found = true, then the byte is returned).
bool File::find_at(const char *path, PackfileEntry **entry, bool use_packfiles)
{
    struct _stat st;

    *entry = NULL;
    if (_stat(path, &st) == 0)
        return found = true;
    if (use_packfiles) {
        *entry = packfile_find(path);
        if (*entry)
            return found = true;
    }
    return false;
}

// 0x513f30  Confidence: exact.
bool File::find_in_path(const char *name, int path_id, PackfileEntry **entry, bool use_packfiles)
{
    file_make_path(path_id, name, full_path);
    this->path_id = path_id;
    return find_at(full_path, entry, use_packfiles);
}

// 0x513f70  Confidence: exact. Passes the address of its own name parameter as the packfile entry out
// pointer (lea edx, [esp + 0x10] is the first argument's slot), so a packfile hit overwrites the local
// name instead of packentry (NOTES.md). No caller.
bool File::find_on_cd(const char *name, int path_id, bool use_packfiles)
{
    file_make_cd_path(path_id, name, full_path);
    this->path_id = path_id;
    return find_at(full_path, (PackfileEntry **)&name, use_packfiles);
}

// 0x513fb0  Confidence: exact.
bool File::is_absolute_path(const char *name)
{
    if (name[1] == ':' || name[0] == '/' || name[0] == '\\')
        return true;
    return false;
}

// 0x513fd0  Confidence: exact. offset = the entry's first sector * 2048.
bool File::get_packfile_path(char *out, int *offset, int max)
{
    char buf[512];
    PackfileEntry *e = packentry;

    if (e == NULL)
        return false;
    strcpy(buf, install_path);
    strcat(buf, e->packfile->name);
    if (strlen(buf) > (unsigned int)(max - 1))
        return false;
    strcpy(out, buf);
    *offset = e->sector << 11;
    return true;
}

// 0x5140b0  Confidence: exact.
int File::find_free_slot()
{
    int i;

    for (i = 0; i < FILE_MAX_OPEN; i++) {
        if (file_slots[i].mode == 0)
            return i;
    }
    return -1;
}

// 0x5140d0  Confidence: exact (store order).
File::File()
{
    full_path[0] = '\0';
    packentry = NULL;
    slot = -1;
    found = false;
}

// 0x5140f0  Confidence: exact.
File::~File()
{
    if (slot >= 0)
        close();
}

// 0x514100  Confidence: exact.
int File::check_flags(int mask)
{
    return file_slots[slot].mode & mask;
}

// 0x514120  Confidence: exact.
bool File::check_version(int min_ver)
{
    return file_slots[slot].version >= min_ver;
}

// 0x514150  Confidence: exact.
int File::get_version()
{
    return file_slots[slot].version;
}

// 0x514170  Confidence: exact.
void File::set_version(int ver)
{
    file_slots[slot].version = ver;
}

// 0x514190  Confidence: exact.
char *File::get_full_path()
{
    return full_path;
}

// 0x5141a0  Confidence: exact. No caller.
char *File::get_filename()
{
    char *p = strrchr(full_path, '\\');

    if (p == NULL)
        return full_path;
    return p + 1;
}

// .data 0x5a7814-0x5a781f, between file_chdir's literal and find's (names: file_last_path_id from
// sum.symbols, file_last_ext ours). The path and the extension of the last file that open found with
// PATH_ANY; strcpy into the 8-byte buffer overflows for longer extensions.
int file_last_path_id = -1;           // 0x5a7814
char file_last_ext[8] = "none";       // 0x5a7818

// 0x5141c0  Confidence: exact (indexed path loops; the path_root flag as two calls; `? true : false` on find_at;
// the fall-through of the last loop goes straight to path id 0, written as a goto; ext is set only on the PATH_ANY path).
// An absolute name is looked up as it is (disk only). An explicit path id is tried (with the packfiles
// only for path_root), then the root with the packfiles. PATH_ANY tries every path whose extension list
// contains the name's extension, then the root. On failure full_path is the name in the first path that
// takes the extension (the root when none does): where a write would create it. With an explicit path id
// that fails, ext was never set and the second loop reads it uninitialised (NOTES.md).
bool File::find(const char *name, int path_id)
{
    char ext[16];
    int i;

    if (name == NULL || *name == '\0')
        return false;
    found = false;
    _unk10c = 0;
    if (is_absolute_path(name)) {
        strcpy(full_path, name);
        this->path_id = -1;
        return find_at(full_path, &packentry, false) ? true : false;
    }
    if (path_id != PATH_ANY) {
        this->path_id = path_id;
        if (path_id == path_root) {
            if (find_in_path(name, path_id, &packentry, true))
                return true;
        } else if (find_in_path(name, path_id, &packentry, false))
            return true;
    } else {
        strcpy(full_path, name);
        _strlwr(full_path);
        ext[0] = '\0';
        char *dot = strrchr(full_path, '.');
        if (dot) {
            strcpy(ext, dot);
            strcat(ext, " ");
        }
        for (i = 0; i < FILE_MAX_PATHS; i++) {
            if (file_path_id_valid(i) && stristr(file_path_table[i].exts, ext) &&
                find_in_path(name, i, &packentry, false))
                return true;
        }
    }
    if (find_in_path(name, 0, &packentry, true))
        return true;

    for (i = 0; i < FILE_MAX_PATHS; i++) {
        if (file_path_id_valid(i) && stristr(file_path_table[i].exts, ext))
            goto have;
    }
    this->path_id = 0;
    goto make;
have:
    this->path_id = i;
    if (i >= FILE_MAX_PATHS)
        this->path_id = 0;
make:
    file_make_path(this->path_id, name, full_path);
    _unk10c = 0;
    return false;
}

// 0x5143e0  Confidence: exact (the key array on the stack; D18604: the slot index local gives the binary's
// per-branch slot address; D21901: bit 0 cleared through a local one-bit bit-field (the binary's load / and /
// store) and the flag set before the key stores, which puts the key in EDX and stores the zeros as immediates).
// 0 turns the encryption off. No caller.
void File::set_cipher_key(unsigned int key)
{
    struct cipher_bits { unsigned encrypted : 1; };
    unsigned int k[3];
    int i = slot;

    if (key == 0) {
        ((cipher_bits *)&file_slots[i].flags)->encrypted = 0;
        return;
    }
    file_slots[i].flags |= 1;
    k[0] = key;
    k[2] = 0;
    k[1] = 0;
    file_slots[i].cipher.set_key(k);
}

// 0x514460  Confidence: exact. Opens the file find found: write mode
// creates the path's directories and fopens "wt" (FILE_TEXT) or "wb"; read mode needs found and opens the
// packfile entry or fopens "rb" unbuffered. 0, -4 not found, -6 no free slot, -99999 open failed.
int File::open_found(int mode)
{
    int s;
    file_slot *fs;

    if (mode & FILE_WRITE) {
        s = find_free_slot();
        if (s < 0)
            return FILE_ERR_NO_SLOT;
        fs = &file_slots[s];
        const char *fmode = "wt";
        if (!(mode & FILE_TEXT))
            fmode = "wb";
        if (path_id > 0)
            file_make_path_dirs(path_id);
        fs->fp = fopen(full_path, fmode);
        if (fs->fp == NULL)
            return FILE_ERR_IO;
        slot = s;
        fs->mode = mode;
        fs->flags = 0;
        fs->error = 0;
        fs->version = 0;
        fs->pos = 0;
        fs->owner = this;
        fs->num_bytes = 0;
        fs->num_ops = 0;
        found = true;
        fs->size = 0;
        return 0;
    }

    if (!found)
        return FILE_ERR_NOT_FOUND;
    s = find_free_slot();
    if (s < 0)
        return FILE_ERR_NO_SLOT;
    fs = &file_slots[s];
    fs->fp = NULL;
    if (packentry) {
        if (!packfile_fopen(packentry))
            return FILE_ERR_IO;
        fs->size = packentry->size;
    } else {
        fs->fp = fopen(full_path, "rb");
        if (fs->fp == NULL)
            return FILE_ERR_IO;
        fs->size = _filelength(_fileno(fs->fp));
    }
    if (fs->fp)
        setvbuf(fs->fp, NULL, _IONBF, 0);
    fs->mode = mode;
    fs->flags = 0;
    fs->error = 0;
    fs->version = 0;
    fs->pos = 0;
    fs->owner = this;
    fs->num_bytes = 0;
    fs->num_ops = 0;
    fs->buf_len = 0;
    fs->buf_pos = 0;
    slot = s;
    fs->buf_start = -1;
    progress_update(1);
    return 0;
}

// 0x514630  Confidence: exact. An explicit path id is tried first; then the path of the last PATH_ANY
// open when the extension is the same (not for path_root); then a PATH_ANY search, whose path id and
// extension are remembered on success.
int File::open(const char *name, int mode, int path_id)
{
    char *ext = file_get_ext(name);
    int r;

    if (path_id != PATH_ANY && find(name, path_id)) {
        strcpy(file_last_ext, ext);
        return open_found(mode);
    }
    if (file_last_path_id != -1 && file_path_id_valid(file_last_path_id) &&
        _stricmp(ext, file_last_ext) == 0 && file_last_path_id != path_root &&
        find(name, file_last_path_id))
        return open_found(mode);
    find(name, PATH_ANY);
    r = open_found(mode);
    if (r == 0) {
        file_last_path_id = this->path_id;
        strcpy(file_last_ext, ext);
    }
    return r;
}

// 0x514740  Confidence: exact. A packfile close counts as success (ebx = 0); a failed fclose sets
// -99999 unless an error is already set. Returns the slot's error.
int File::close()
{
    file_slot *s = &file_slots[slot];
    int r;

    if (packentry) {
        r = 0;
        packfile_fclose(packentry);
    } else {
        r = fclose(s->fp);
    }
    s->fp = NULL;
    packentry = NULL;
    if (r && s->error == 0)
        s->error = FILE_ERR_IO;
    s->mode = 0;
    int err = s->error;
    slot = -1;
    return err;
}

// 0x5147c0  Confidence: exact (D18600: a slot pointer with s->owner->packentry written out three times; 8966
// keeps the slot address as a common subexpression, the binary's unfolded `add eax, file_slots`). Asks the
// owner's packfile entry, not this File's (the same object).
int File::get_async_status()
{
    if (slot >= 0) {
        file_slot *s = &file_slots[slot];
        if (s->owner->packentry && (s->owner->packentry->packfile->flags & PACKFILE_ON_CD))
            return packfile_fstatus(s->owner->packentry);
    }
    return 1;
}

// 0x514810  Confidence: exact (sum.symbols calls the CRT function ferror; asm.mjs names it fflush). No caller.
int File::check_stream_error()
{
    file_slot *s = &file_slots[slot];

    if (s->fp && fflush(s->fp) && s->error == 0)
        s->error = FILE_ERR_IO;
    return s->error;
}

// 0x514860  Confidence: exact. The size of name (find first when it is given) or of the found file.
int File::size(const char *name, int path_id)
{
    if (name && !find(name, path_id))
        return FILE_ERR_NOT_FOUND;
    if (packentry)
        return packentry->size;
    if (slot < 0) {
        FILE *fp = fopen(full_path, "rb");
        if (fp == NULL)
            return FILE_ERR_NOT_FOUND;
        int n = _filelength(_fileno(fp));
        fclose(fp);
        return n;
    }
    return file_slots[slot].size;
}

// 0x5148f0  Confidence: exact. No caller.
bool File::is_in_packfile()
{
    return packentry && (packentry->packfile->flags & PACKFILE_ON_CD);
}

// 0x514910  Confidence: exact (the origin is added to the pos argument itself). A target inside the read buffer only moves the buffer position;
// otherwise the buffer is dropped and the stream is seeked.
int File::seek(int pos, int origin)
{
    file_slot *s = &file_slots[slot];
    int target;

    if (origin == 1)
        pos += s->pos;
    else if (origin == 2)
        pos += size(NULL, PATH_ANY);
    target = pos;
    if (check_flags(FILE_READ) && s->buf_start != -1 && target >= s->buf_start &&
        target < s->buf_start + s->buf_len) {
        s->pos = target;
        s->buf_pos = target - s->buf_start;
        return 0;
    }
    PackfileEntry *e = packentry;
    s->pos = target;
    s->buf_pos = 0;
    s->buf_len = 0;
    s->buf_start = -1;
    if (e) {
        if (!packfile_fseek(e, target, SEEK_SET))
            return FILE_ERR_IO;
    } else if (fseek(s->fp, target, SEEK_SET)) {
        return FILE_ERR_IO;
    }
    return 0;
}

// 0x5149f0  Confidence: exact.
int File::tell()
{
    return file_slots[slot].pos;
}

// 0x514a10  Confidence: exact.
bool File::eof()
{
    return file_slots[slot].pos >= file_slots[slot].size;
}

// 0x514a40  Confidence: exact.
int File::error()
{
    if (slot < 0)
        return 0;
    return file_slots[slot].error;
}

// 0x514a60  Confidence: exact. No caller.
int File::get_path_id()
{
    return path_id;
}

// 0x514a70  Confidence: exact.
bool File::is_open()
{
    return slot >= 0;
}

// 0x514a80  Confidence: exact.
int File::Delete(const char *name, int path_id)
{
    if (name && !find(name, path_id))
        return FILE_ERR_NOT_FOUND;
    if (packentry)
        return FILE_ERR_IN_PACK;
    int r = _unlink(full_path);
    found = false;
    return r ? FILE_ERR_IO : 0;
}

// 0x514ae0  Confidence: exact. Renames the found file to new_name in the same folder and keeps the new
// path in full_path.
int File::make_backup(const char *new_name, const char *name, int path_id)
{
    char dir[256];
    char path[256];

    if (name && !find(name, path_id))
        return FILE_ERR_NOT_FOUND;
    if (packentry)
        return FILE_ERR_IN_PACK;
    strcpy(dir, full_path);
    *strrchr(dir, '\\') = '\0';
    sprintf(path, "%s\\%s", dir, new_name);
    if (rename(full_path, path))
        return FILE_ERR_IO;
    strcpy(full_path, path);
    return 0;
}

// 0x514bf0  Confidence: exact. 3 for a packfile entry (read-only and archive).
int File::get_attributes(const char *name, int path_id)
{
    int attributes = 0;

    if (name && !find(name, path_id))
        return FILE_ERR_NOT_FOUND;
    if (packentry)
        return 3;
    DWORD a = GetFileAttributesA(full_path);
    if (a == 0xffffffff)
        return FILE_ERR_IO;
    if (a & FILE_ATTRIBUTE_READONLY)
        attributes = 2;
    if (a & FILE_ATTRIBUTE_ARCHIVE)
        attributes |= 4;
    return attributes;
}

// 0x514c60  Confidence: exact. No caller.
int File::set_attributes(int flags, const char *name, int path_id)
{
    if (name && !find(name, path_id))
        return FILE_ERR_NOT_FOUND;
    if (packentry)
        return FILE_ERR_IN_PACK;
    DWORD a = GetFileAttributesA(full_path);
    if (a == 0xffffffff)
        return FILE_ERR_IO;
    if (flags & 2)
        a |= FILE_ATTRIBUTE_READONLY;
    if (flags & 4)
        a |= FILE_ATTRIBUTE_ARCHIVE;
    return SetFileAttributesA(full_path, a) ? 0 : FILE_ERR_IO;
}

// 0x514ce0  Confidence: exact. No caller.
int File::clear_attributes(int flags, const char *name, int path_id)
{
    if (name && !find(name, path_id))
        return FILE_ERR_NOT_FOUND;
    if (packentry)
        return FILE_ERR_IN_PACK;
    DWORD a = GetFileAttributesA(full_path);
    if (a == 0xffffffff)
        return FILE_ERR_IO;
    if (flags & 2)
        a &= ~FILE_ATTRIBUTE_READONLY;
    if (flags & 4)
        a &= ~FILE_ATTRIBUTE_ARCHIVE;
    return SetFileAttributesA(full_path, a) ? 0 : FILE_ERR_IO;
}

// 0x514d60  Confidence: exact (unit compiled /Ob0: Crc32's constructor is not
// expanded: its COMDAT 0x514eb0 is called). CRC-32 of the whole file in 1 KB blocks. A file that was
// not open is opened and closed again (the result is close()'s); an open one must be in read mode and
// gets its position back (the result is error()). No caller.
// 0x514eb0  Crc32::Crc32, the inline constructor's COMDAT (vsdk/os/crc32.h; sum.symbols Crc32__Crc32_b).
// Confidence: exact (compiler-generated).
int File::calc_crc32(unsigned int *crc, const char *name, int path_id)
{
    bool opened = false;
    int pos = 0;
    Crc32 c;
    char buf[0x400];
    int done = 0;
    int r;

    if (name && !find(name, path_id))
        return FILE_ERR_NOT_FOUND;
    if (slot < 0) {
        opened = true;
        r = open_found(FILE_READ);
        if (r < 0)
            return r;
    } else {
        if (!check_flags(FILE_READ))
            return -1;
        pos = tell();
        if (pos < 0)
            return pos;
        r = seek(0, 0);
        if (r != 0)
            return r;
    }
    int total = size(NULL, PATH_ANY);
    do {
        int n;
        if (done + 0x400 <= total)
            n = 0x400;
        else
            n = total - done;
        read(buf, n, 0, 0);
        if (error() < 0)
            break;
        done += n;
        *crc = c.update(buf, n);
    } while (done < total);
    if (opened)
        return close();
    seek(pos, 0);
    return error();
}
