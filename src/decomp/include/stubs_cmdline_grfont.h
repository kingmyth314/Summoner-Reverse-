// engine_stubs.h - declarations of the vsdk functions and classes that cmdline.cpp and gr_font.cpp
// call but do not define. Names from symbols/sum.symbols; signatures inferred from the call sites in
// Sum.exe v1.40 (argument counts, pushes, how the result is tested), not from any reference source.
#ifndef STUBS_CMDLINE_GRFONT_H
#define STUBS_CMDLINE_GRFONT_H

// ---------------------------------------------------------------- vsdk\os\debug (0x531cb0)
// PC version shows a message box and calls exit(1), but every call site in Sum.exe re-pushes the
// arguments and calls it again in an endless loop (jmp back to the call), so the vsdk error macro
// must be a loop around the call. __FILE__ / __LINE__ in the original; the literal path and the
// line number are written out here so the strings and pushes match.
void DebugError(const char *file, int line, const char *msg);
#define VSDK_ERROR(file, line, msg) for (;;) DebugError(file, line, msg)

// ---------------------------------------------------------------- vsdk\os\memory.cpp
// operator new (0x512550) -> operator_new_dbg(size, 0, 0); operator delete (0x512570) -> free.
// Both are the global operators, so plain new/delete in the source reach them.

// ---------------------------------------------------------------- vsdk\os\registry.cpp
// 0x512280 (subkey, name, out, default): copies the value or the default into out.
void os_registry_read_string(const char *subkey, const char *name, char *out, const char *default_value);

// ---------------------------------------------------------------- vsdk\vfile (File, 0x114 bytes)
// __thiscall members; File has a destructor, so a stack File gets an EH frame (gr_font_read_file).
class File {
public:
    File();                                            // 0x5140d0
    ~File();                                           // 0x5140f0 closes if open
    int open(const char *name, int mode, int path_id); // 0x514630 < 0 on failure
    void close();                                      // 0x514740
    char read_int8(int min_ver, int default_value);    // 0x525850
    short read_int16(int min_ver, int default_value);  // 0x5258f0
    int read_int(int min_ver, int default_value);      // 0x525990
    unsigned int read_uint(int min_ver, int default_value); // 0x5259e0
    int read(void *buf, int len, int min_ver, int unused);  // 0x525fe0
private:
    char data[0x114];
};

// ---------------------------------------------------------------- String (0x5567f0..., 8 bytes)
// __thiscall members; returns by value through a hidden pointer.
class String {
public:
    String();                                   // 0x5567f0
    String(const char *s);                      // 0x556810
    String(const String &other);                // 0x556850
    ~String();                                  // 0x5568b0
    const char *c_str() const;                  // 0x5568c0
    int size() const;                           // 0x5568d0
    String substr(int begin, int end) const;    // 0x5569d0
    String left(int n) const;                   // 0x556a80
    String &operator=(const String &other);     // 0x556e40
    String &operator=(const char *s);           // 0x556ea0
    char &operator[](int n);                    // 0x557ab0 (String__char_at, bounds-checked)
private:
    int max_len;
    char *buf;
};

// ---------------------------------------------------------------- vsdk\bmpman (0x503cf0)
// (format, w, h, data, palette, a6); gr_font_create passes (4, w, h, bits, 0, -1).
int bm_create(int format, int w, int h, void *data, void *palette, int a6);

// ---------------------------------------------------------------- vsdk\gr\gr.cpp
// gr_bitmap_ex (0x506cb0): (bm, x, y, w, h, sx, sy, mode), clipped. The mode is a 4-byte
// gr::Mode (gr__Mode__Mode packs six 5-bit fields) passed by value; an int pushes the same dword.
void gr_bitmap_ex(int bm, int x, int y, int w, int h, int sx, int sy, int mode);
extern int gr_gamma_ramp[256];                  // 0x25d8e0c

// ---------------------------------------------------------------- localize (0x25d8870)
extern int localize_french_punct;               // set for the French string table

#endif
