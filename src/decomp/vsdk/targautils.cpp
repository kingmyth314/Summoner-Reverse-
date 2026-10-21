// targautils.cpp - Sum.exe v1.40, code 0x5549c0-0x5565d6 (vsdk\bmpman\targautils.cpp)
// Written from the Sum.exe disassembly (build/sum.asm). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Data of this unit: .data 0x5ada54-0x5adb8c (string literals in function order, plus tga_last_path_id at
// 0x5ada64 and the error string table at 0x5adad8, whose positions between the literals give their place
// in the file), .bss 0x2f53aec (m2v_quantizer_scale). No static initialisers.
//
// Despite the symbol names there is no MPEG-2 decoder here: the four bit-reader helpers are unreferenced
// leftovers, and m2v_read_bitmap only loads the whole file (bm_lock_m2v decodes it elsewhere).

#include <stdio.h>
#include <string.h>
#include "vsdk/os/debug.h"
#include "vsdk/bmpman/targautils.h"

// Original __FILE__ of this unit (pushed by tga_read_bitmap, 0x5ada90).
#define TARGAUTILS_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\bmpman\\targautils.cpp"

// The 18-byte TGA header, read field by field into a naturally aligned struct (0x14 bytes on the stack
// of tga_read_header and tga_read_bitmap: width at +0xe, bpp at +0x12, descriptor at +0x13).
struct tga_header {
    ubyte id_length;          // +0x00
    ubyte colormap_type;      // +0x01
    ubyte image_type;         // +0x02 1/2/3 raw (mapped/true colour/grey), 9/10/11 RLE
    short colormap_first;     // +0x04
    short colormap_length;    // +0x06
    ubyte colormap_depth;     // +0x08
    short x_origin;           // +0x0A
    short y_origin;           // +0x0C
    short width;              // +0x0E
    short height;             // +0x10
    ubyte bits;               // +0x12
    ubyte descriptor;         // +0x13 bit 5 = top-left origin, bits 0-3 = alpha bits
};

// The 128-byte PCX header, read and written whole.
struct pcx_header {
    char  manufacturer;       // +0x00 10
    char  version;            // +0x01 5
    char  encoding;           // +0x02 1 = RLE
    char  bits_per_pixel;     // +0x03 8
    short xmin, ymin;         // +0x04
    short xmax, ymax;         // +0x08
    short hdpi, vdpi;         // +0x0C
    ubyte colormap[48];       // +0x10
    char  reserved;           // +0x40
    char  nplanes;            // +0x41 1
    short bytes_per_line;     // +0x42
    short palette_info;       // +0x44
    short hscreen, vscreen;   // +0x46
    char  filler[54];         // +0x4A
};

// ---- .bss ----------------------------------------------------------------------------------------
int m2v_quantizer_scale;      // 0x2f53aec  (name ours; written by m2v_read_slice_quantizer_scale only)

// 0x5549c0  Confidence: likely (each branch has its own next_start_code call except the picture coding
// extension without composite display, which shares the sequence extension's: tail merging).
// MPEG-2 extension_and_user_data: skips extension (0x1b5) and user data (0x1b2) start codes; 1 when
// the next start code is something else, 0 for an extension id other than 1, 2 or 8.
int m2v_skip_extension_and_user_data(m2v_bitstream *bs)
{
    int code;

    bs->next_start_code();
    for (;;) {
        code = bs->show_bits(32);
        if (code != 0x1b5) {
            if (code != 0x1b2)
                return 1;
            bs->get_bits(32);
            bs->next_start_code();
        } else {
            bs->get_bits(32);
            switch (bs->get_bits(4)) {
            case 1:                             // sequence extension (44 more bits)
                bs->get_bits(32);
                bs->get_bits(12);
                bs->next_start_code();
                break;
            case 2:                             // sequence display extension
                bs->get_bits(3);
                if (bs->get_bits(1))
                    bs->get_bits(24);
                bs->get_bits(29);
                bs->next_start_code();
                break;
            case 8:                             // picture coding extension
                bs->get_bits(16);
                bs->get_bits(2);
                bs->get_bits(12);
                if (bs->get_bits(1))
                    bs->get_bits(20);
                bs->next_start_code();
                break;
            default:
                return 0;
            }
        }
    }
}

// 0x554ac0  Confidence: exact. extra_bit_information / extra_information bytes.
void m2v_skip_extra_bit_information(m2v_bitstream *bs)
{
    while (bs->get_bits(1))
        bs->get_bits(8);
}

// 0x554af0  Confidence: exact. Slice header: quantiser_scale_code, then extra_bit_slice.
void m2v_read_slice_quantizer_scale(m2v_bitstream *bs)
{
    m2v_quantizer_scale = bs->get_bits(5);
    bs->get_bits(1);
}

// 0x554b10  Confidence: exact (loop shape). Skips macroblock_stuffing (0x00f) and escape (0x008) codes;
// returns 1 and eats the bit when the address increment is 1 (code '1'), else 0.
int m2v_skip_macroblock_stuffing(m2v_bitstream *bs)
{
    int code = bs->show_bits(11);

    while (code < 0x18) {
        if (code != 0xf && code != 0x8)
            return 0;
        bs->get_bits(11);
        code = bs->show_bits(11);
    }
    if (code >= 0x400) {
        bs->get_bits(1);
        return 1;
    }
    return 0;
}

// 0x554b60  Confidence: exact (the byte swap: a 16-bit half swap then a byte swap inside each half
// gives exactly the register use of 0x554c72-0x554c9c). Reads the sequence header code and the
// size word (big endian: 12 bits width, 12 bits height, aspect and frame rate). Returns the file size.
int m2v_read_header(const char *name, int *w, int *h, int unused)
{
    unsigned int header[2];
    File f;
    char filename[256];

    if (f.open(name, 1, PATH_ANY) < 0) {
        file_change_ext(filename, name, ".m2v");
        if (f.open(filename, 1, PATH_ANY) < 0)
            return 0;
    }

    int size = f.size(0, PATH_ANY);
    f.read(header, 8, 0, 0);
    if (f.error()) {
        f.close();
        return 0;
    }

    unsigned short *half = (unsigned short *)&header[1];
    unsigned short t = half[0];
    half[0] = half[1];
    half[1] = t;
    unsigned char *b = (unsigned char *)&header[1];
    unsigned char c = b[0];
    b[0] = b[1];
    b[1] = c;
    c = b[2];
    b[2] = b[3];
    b[3] = c;

    *w = header[1] >> 20;
    *h = (header[1] >> 8) & 0xfff;
    f.close();
    return size;
}

// 0x554d00  Confidence: exact. Loads the whole file into a new buffer; returns its size, 0 on failure.
// *data keeps the freed pointer after a read error.
int m2v_read_bitmap(const char *name, ubyte **data, int unused)
{
    File f;
    char filename[256];

    if (f.open(name, 1, PATH_ANY) < 0) {
        file_change_ext(filename, name, ".m2v");
        if (f.open(filename, 1, PATH_ANY) < 0)
            return 0;
    }

    int size = f.size(0, PATH_ANY);
    *data = new ubyte[size];
    if (*data == 0)
        return 0;

    f.read(*data, size, 0, 0);
    if (f.error()) {
        f.close();
        delete[] *data;
        return 0;
    }
    f.close();
    return size;
}

// 0x554ea0  Confidence: likely (the 0x81 and end-of-row packet headers may have been written with
// `|= 0x80`; the raw-packet header uses min_run where dup would be equal). TGA RLE packets for one
// row (gr_screenshot) or a whole image (tga_write_rle). count is the packet's pixel count plus one;
// a run starts once min_run equal pixels follow (2 for 8-bit output). Pixels are compared with
// dst_bpp bytes although they are src_bpp apart. Returns the bytes written.
int tga_rle_encode_row(ubyte *dst, ubyte *src, int dst_bpp, int src_bpp, int row_bytes)
{
    ubyte *out = dst;
    ubyte *start = 0;
    ubyte *prev = 0;
    int min_run = (dst_bpp == 1) ? 2 : 1;
    ubyte *end = src + row_bytes;
    int count = 1;
    int dup = 0;

    do {
        if (count == 0x81) {
            *out = 0x7f;
            if (dup >= min_run) {
                *out = 0xff;
                count = 2;
            }
            out++;
            out += tga_convert_pixels(out, start, count - 1, src_bpp, dst_bpp);
            count = 1;
        } else if (count == 1) {
            start = src;
            prev = src;
            dup = 0;
            count = 2;
            src += src_bpp;
        } else {
            if (bytes_equal(src, prev, dst_bpp)) {
                dup++;
                if (dup == min_run && count > dup + 1) {
                    *out++ = (ubyte)(count - min_run - 2);
                    count = count - dup - 1;
                    out += tga_convert_pixels(out, start, count, src_bpp, dst_bpp);
                    count = dup + 1;
                    start = src;
                }
            } else if (dup >= min_run) {
                *out++ = (ubyte)(dup | 0x80);
                out += tga_convert_pixels(out, start, 1, src_bpp, dst_bpp);
                count = 1;
                continue;
            } else {
                dup = 0;
                prev = src;
            }
            count++;
            src += src_bpp;
        }
    } while (src < end);

    count--;
    if (count >= 1) {
        *out = (ubyte)(count - 1);
        if (dup >= min_run) {
            *out = (ubyte)((count - 1) | 0x80);
            count = 1;
        }
        out++;
        out += tga_convert_pixels(out, start, count, src_bpp, dst_bpp);
    }
    return out - dst;
}

// 0x555020  Confidence: exact (the 565 channels scaled with `* 8` / `* 4`: the binary keeps the masks,
// D6800). 16-bit to 16-bit toggles bit 15 of the SOURCE pixels in place and
// writes nothing to dst (the caller still advances by the returned size); 16 (565) to 24 expands to
// B, G, R bytes; anything else is a plain copy. Returns the bytes for dst.
int tga_convert_pixels(ubyte *dst, ubyte *src, int count, int src_bpp, int dst_bpp)
{
    int i;

    if (src_bpp == 2) {
        if (dst_bpp == 2) {
            ushort *p = (ushort *)src;
            for (i = 0; i < count; i++)
                *p++ ^= 0x8000;
            return count * 2;
        }
        if (dst_bpp == 3) {
            ushort *p = (ushort *)src;
            for (i = 0; i < count; i++) {
                ushort c = *p++;
                *dst++ = (c & 0x1f) * 8;
                *dst++ = ((c >> 5) & 0x3f) * 4;
                *dst++ = ((c >> 11) & 0x1f) * 8;
            }
            return count * 3;
        }
    }

    memcpy(dst, src, count * src_bpp);
    return count * dst_bpp;
}

// 0x5550d0  Confidence: exact. 1 when the first n bytes are equal (the first byte is compared even
// for n <= 0).
int bytes_equal(ubyte *a, ubyte *b, int n)
{
    do {
        if (*a++ != *b++)
            return 0;
    } while (--n > 0);
    return 1;
}

// ---- .data ---------------------------------------------------------------------------------------
// 0x5ada64, between m2v_read_bitmap's ".m2v" and tga_read_header's ".tga": defined here. Set from the
// File's path id by both TGA readers and never read (name ours).
int tga_last_path_id = -1;

// 0x555110  Confidence: exact (VC6 byte match, m-1020, D11101: an `opened` flag, so both failures share one
// out-of-line `return 1`; the order of the inline strcpy/strcat; the second open tests == 0, the
// first < 0, as in the binary). Without an extension ".tga" is appended; a name with a '.' fails at
// once. Returns 0 on success, 1 on failure.
int tga_read_header(const char *name, int *w, int *h, int *bpp, void *unused1, void *unused2,
                    int path_id)
{
    tga_header header;
    File f;
    char filename[512];

    if (f.open(name, 1, path_id) < 0) {
        bool opened = false;
        if (strchr(name, '.') == NULL) {
            strcpy(filename, name);
            strcat(filename, ".tga");
            if (f.open(filename, 1, path_id) == 0)
                opened = true;
        }
        if (!opened)
            return 1;
    }

    tga_last_path_id = f.path_id;
    header.id_length = f.read_int8(0, 0);
    header.colormap_type = f.read_int8(0, 0);
    header.image_type = f.read_int8(0, 0);
    header.colormap_first = f.read_int16(0, 0);
    header.colormap_length = f.read_int16(0, 0);
    header.colormap_depth = f.read_int8(0, 0);
    header.x_origin = f.read_int16(0, 0);
    header.y_origin = f.read_int16(0, 0);
    header.width = f.read_int16(0, 0);
    header.height = f.read_int16(0, 0);
    header.bits = f.read_int8(0, 0);
    header.descriptor = f.read_int8(0, 0);

    if (f.error()) {
        f.close();
        return 1;
    }

    *w = header.width;
    *h = header.height;
    *bpp = header.bits;
    f.close();
    return 0;
}

// 0x555330  Confidence: exact (`c &= ~0x80` gives the binary's byte `and bl, 0x7f`, D10800; the run loop is a do-while on the packet's count field; the header byte
// as an int: the source cursor then lives in dst's argument slot as in the binary, D6803). Decodes
// num_pixels pixels of TGA RLE packets; returns the source bytes used.
int tga_rle_decode(ubyte *dst, ubyte *src, int num_pixels, int pixel_size)
{
    ubyte *d = dst;
    ubyte *s = src;
    ubyte pixel[4];
    ubyte *pp;
    int i = 0;

    while (i < num_pixels) {
        int c = *s++;
        if (c & 0x80) {
            c &= ~0x80;
            i += c + 1;
            pp = pixel;
            copy_pixels_advance(1, &pp, &s, pixel_size);
            do {
                memcpy(d, pixel, pixel_size);
                d += pixel_size;
            } while (c--);
        } else {
            i += c + 1;
            copy_pixels_advance(c + 1, &d, &s, pixel_size);
        }
    }
    return s - src;
}

// 0x555410  Confidence: exact (both pointers read before the stores, D6800). Copies n pixels and
// advances both pointers. (The symbol comment's argument order "(n, size, &dst, &src)" is wrong: size
// is the last argument.)
void copy_pixels_advance(int n, ubyte **dst, ubyte **src, int size)
{
    int bytes = n * size;

    memcpy(*dst, *src, bytes);
    ubyte *d = *dst;
    ubyte *s = *src;
    *dst = d + bytes;
    *src = s + bytes;
}

// 0x555450  Confidence: exact (the row loops are written with y and strength-reduced by the compiler
// into a top and a bottom pointer, which the binary has; the colour map fill order is the chained
// assignment's right-to-left order). Error line 598 (0x256). Reads the image into data (rows bottom-up
// unless the descriptor's top-left bit is set) and the colour map into palette as R, G, B, padding it to
// 256 entries. Grey (3) and RLE grey (11) images are accepted but not decoded. Note the seek over the
// image id goes to the absolute offset id_length (origin 0), not past the header.
int tga_read_bitmap(const char *name, ubyte *data, ubyte *palette, int unused, int path_id)
{
    tga_header header;
    File f;
    char filename[256];
    char error[256];
    int i, y;

    if (f.open(name, 1, path_id) < 0) {
        file_change_ext(filename, name, ".tga");
        if (f.open(filename, 1, path_id) < 0) {
            if (f.open(filename, 1, PATH_ANY) < 0) {
                sprintf(error, "Could not open file: %s", filename);
                for (;;)
                    DebugError(TARGAUTILS_CPP, 598, error);
            }
        }
    }

    tga_last_path_id = f.path_id;
    header.id_length = f.read_int8(0, 0);
    header.colormap_type = f.read_int8(0, 0);
    header.image_type = f.read_int8(0, 0);
    header.colormap_first = f.read_int16(0, 0);
    header.colormap_length = f.read_int16(0, 0);
    header.colormap_depth = f.read_int8(0, 0);
    header.x_origin = f.read_int16(0, 0);
    header.y_origin = f.read_int16(0, 0);
    header.width = f.read_int16(0, 0);
    header.height = f.read_int16(0, 0);
    header.bits = f.read_int8(0, 0);
    header.descriptor = f.read_int8(0, 0);

    if (f.error()) {
        f.close();
        return 1;
    }
    if (header.bits != 8 && header.bits != 24 && header.bits != 32 && header.bits != 16) {
        f.close();
        return 1;
    }
    if ((header.descriptor & 0xf) != 8 && (header.descriptor & 0xf) != 0 &&
        (header.descriptor & 0xf) != 1) {
        f.close();
        return 1;
    }
    if (header.image_type != 1 && header.image_type != 2 && header.image_type != 3 &&
        header.image_type != 9 && header.image_type != 10 && header.image_type != 11) {
        f.close();
        return 1;
    }
    if (header.id_length > 0) {
        if (f.seek(header.id_length, 0) < 0) {
            f.close();
            return 1;
        }
    }

    if (header.colormap_type) {
        for (i = 0; i < header.colormap_length; i++) {
            palette[i * 3 + 2] = f.read_uint8(0, 0);
            palette[i * 3 + 1] = f.read_uint8(0, 0);
            palette[i * 3 + 0] = f.read_uint8(0, 0);
        }
        for (; i < 256; i++)
            palette[i * 3] = palette[i * 3 + 1] = palette[i * 3 + 2] = 0;
    }

    int size = f.size(0, PATH_ANY) - f.tell();
    ubyte *buf = new ubyte[size];
    ubyte *src = buf;
    f.read(buf, size, 0, 0);

    int pixel_size = header.bits >> 3;
    int width = header.width;
    int row_bytes = width * pixel_size;

    if (header.image_type == 1 || header.image_type == 2) {
        for (y = 0; y < header.height; y++) {
            ubyte *row = (header.descriptor & 0x20) ? data + y * row_bytes
                                                    : data + (header.height - 1 - y) * row_bytes;
            copy_pixels_advance(width, &row, &src, pixel_size);
        }
    } else if (header.image_type == 9 || header.image_type == 10) {
        for (y = 0; y < header.height; y++) {
            ubyte *row = (header.descriptor & 0x20) ? data + y * row_bytes
                                                    : data + (header.height - 1 - y) * row_bytes;
            src += tga_rle_decode(row, src, width, pixel_size);
        }
    }

    delete[] buf;
    f.close();
    return 0;
}

// 0x555970  Confidence: exact. Unreferenced. Writes an RLE (type 10) .tga, top-left origin; the
// whole image is encoded as one row into a buffer twice the image size. 0 on success, -1 on failure.
int tga_write_rle(const char *name, ubyte *data, int unused, int w, int h, int bpp, int path_id)
{
    File f;
    char filename[256];
    int bytes = (bpp + 7) / 8;

    file_change_ext(filename, name, ".tga");
    if (f.open(filename, 2, path_id) < 0)
        return -1;

    f.write_uint8(0);           // id length
    f.write_uint8(0);           // colour map type
    f.write_uint8(10);          // image type: RLE true colour
    f.write_uint16(0);          // colour map first entry
    f.write_uint16(0);          // colour map length
    f.write_uint8(0);           // colour map depth
    f.write_uint16(0);          // x origin
    f.write_uint16(0);          // y origin
    f.write_uint16(w);
    f.write_uint16(h);
    f.write_uint8(bpp);
    f.write_uint8(0x20);        // top-left origin

    int size = bytes * w * h;
    ubyte *buf = new ubyte[size * 2];
    int len = tga_rle_encode_row(buf, data, bytes, bytes, size);
    if (len < 0) {
        delete[] buf;
        f.close();
        return -1;
    }
    f.write(buf, len);
    f.close();
    delete[] buf;
    return 0;
}

// 0x555b50  Confidence: exact. Unreferenced. Writes an uncompressed (type 2) .tga, top-left origin.
int tga_write(const char *name, ubyte *data, int unused, int w, int h, int bpp, int path_id)
{
    File f;
    char filename[256];
    int bytes = (bpp + 7) / 8;

    file_change_ext(filename, name, ".tga");
    if (f.open(filename, 2, path_id) < 0)
        return -1;

    f.write_uint8(0);
    f.write_uint8(0);
    f.write_uint8(2);           // image type: true colour
    f.write_uint16(0);
    f.write_uint16(0);
    f.write_uint8(0);
    f.write_uint16(0);
    f.write_uint16(0);
    f.write_uint16(w);
    f.write_uint16(h);
    f.write_uint8(bpp);
    f.write_uint8(0x20);
    f.write(data, bytes * w * h);
    f.close();
    return 0;
}

// 0x5adad8: the messages walked by bm_image_error_string, one literal with embedded NULs. It lies between
// tga_write's ".tga" and pcx_read_header's ".pcx" in .data, so it is defined here (name ours).
static char bm_image_error_strings[] =
    "No error.\0"
    "Error opening file.\0"
    "Couldn't read PCX header.\0"
    "Unsupported PCX version.\0"
    "Error reading data.\0"
    "Couldn't find palette information.\0"
    "Error writing data.";

// 0x555cf0  Confidence: exact. Accepts only 8-bit single-plane RLE version 5 files. The 0x300-byte
// palette is the last 768 bytes of the file.
int pcx_read_header(const char *name, int *w, int *h, int *bpp, ubyte *palette, int path_id)
{
    pcx_header header;
    File f;
    char filename[256];

    if (f.open(name, 1, path_id) < 0) {
        file_change_ext(filename, name, ".pcx");
        if (f.open(filename, 1, path_id) < 0)
            return 1;
    }

    f.read(&header, sizeof(header), 0, 0);
    if (f.error()) {
        f.close();
        return 2;
    }
    if (header.manufacturer != 10 || header.encoding != 1 || header.nplanes != 1 ||
        header.bits_per_pixel != 8 || header.version != 5) {
        f.close();
        return 3;
    }

    if (w)
        *w = header.xmax - header.xmin + 1;
    if (h)
        *h = header.ymax - header.ymin + 1;
    if (bpp)
        *bpp = 8;
    if (palette) {
        f.seek(f.size(0, PATH_ANY) - 0x300, 0);
        f.read(palette, 0x300, 0, 0);
    }
    f.close();
    return 0;
}

// 0x555f60  Confidence: likely (the decode loop; c is initialised at the top and count = 0 (D15301): then
// every read before the loop pushes the zero in EBP as the binary does; with `int pos = 0;` after the last
// read (D18701) that read also pushes EBP; left: the binary clears pos (ESI) just before that call, 8966 after).
// Decodes through a 1 KB read buffer; bytes past the image width in
// each line are dropped.
int pcx_read_bitmap(const char *name, ubyte *data, ubyte *palette, int path_id)
{
    pcx_header header;
    File f;
    char filename[256];
    ubyte buffer[0x400];
    int count;
    ubyte c = 0;
    int x, y;

    if (f.open(name, 1, path_id) < 0) {
        file_change_ext(filename, name, ".pcx");
        if (f.open(filename, 1, path_id) < 0)
            return 1;
    }

    f.read(&header, sizeof(header), 0, 0);
    if (f.error()) {
        f.close();
        return 2;
    }
    if (header.manufacturer != 10 || header.encoding != 1 || header.nplanes != 1 ||
        header.bits_per_pixel != 8 || header.version != 5) {
        f.close();
        return 3;
    }

    int width = header.xmax - header.xmin + 1;
    int height = header.ymax - header.ymin + 1;

    f.seek(f.size(0, PATH_ANY) - 0x300, 0);
    f.read(palette, 0x300, 0, 0);
    f.seek(0x80, 0);

    count = 0;
    f.read(buffer, sizeof(buffer), 0, 0);
    int pos = 0;
    if (f.error()) {
        f.close();
        return 4;
    }

    for (y = 0; y < height; y++) {
        ubyte *p = data;
        for (x = 0; x < header.bytes_per_line; x++) {
            if (count == 0) {
                c = buffer[pos++];
                if (pos == 0x400) {
                    f.read(buffer, sizeof(buffer), 0, 0);
                    pos = 0;
                }
                if ((c & 0xc0) == 0xc0) {
                    count = c & 0x3f;
                    c = buffer[pos++];
                    if (pos == 0x400) {
                        f.read(buffer, sizeof(buffer), 0, 0);
                        pos = 0;
                    }
                } else {
                    count = 1;
                }
            }
            if (x < width)
                *p++ = c;
            count--;
        }
        data += width;
    }

    f.close();
    return 0;
}

// 0x5562a0  Confidence: exact (filename is 512 bytes: the frame is 0x394, D6800). Unreferenced. Writes
// an 8-bit RLE .pcx from an array of row pointers, then the 0x0c marker and the 768-byte palette.
int pcx_write(const char *name, int w, int h, ubyte **rows, ubyte *palette)
{
    pcx_header header;
    File f;
    char filename[512];
    int i;

    file_change_ext(filename, name, ".pcx");
    memset(&header, 0, sizeof(header));
    header.manufacturer = 10;
    header.encoding = 1;
    header.nplanes = 1;
    header.bits_per_pixel = 8;
    header.version = 5;
    header.xmax = w - 1;
    header.ymax = h - 1;
    header.ymin = 0;
    header.xmin = 0;
    header.bytes_per_line = w;

    if (f.open(filename, 2, PATH_ANY) < 0)
        return 1;

    f.write(&header, sizeof(header));
    for (i = 0; i < h; i++) {
        if (!pcx_encode_line(rows[i], w, &f)) {
            f.close();
            return 6;
        }
    }
    f.write_uint8(0x0c);
    f.write(palette, 0x300);
    f.close();
    if (f.error())
        return 6;
    return 0;
}

// 0x556470  Confidence: exact (pointer increment form; total declared before count, D6800). RLE-encodes
// one line; returns the bytes written, 0 on a write failure. Runs are cut at 63.
int pcx_encode_line(ubyte *src, int width, File *f)
{
    ubyte last = *src;
    ubyte cur;
    int total = 0;
    ubyte count = 1;
    int i, n;

    for (i = 1; i < width; i++) {
        cur = *++src;
        if (cur == last) {
            count++;
            if (count == 63) {
                n = pcx_encode_byte(last, count, f);
                if (n == 0)
                    return 0;
                total += n;
                count = 0;
            }
        } else {
            if (count) {
                n = pcx_encode_byte(last, count, f);
                if (n == 0)
                    return 0;
                total += n;
            }
            last = cur;
            count = 1;
        }
    }

    if (count) {
        n = pcx_encode_byte(last, count, f);
        if (n == 0)
            return 0;
        return total + n;
    }
    return total;
}

// 0x556550  Confidence: exact. Returns the bytes written (0, 1 or 2).
int pcx_encode_byte(ubyte byte, ubyte count, File *f)
{
    if (count) {
        if (count == 1 && (byte & 0xc0) != 0xc0) {
            f->write_uint8(byte);
            return 1;
        }
        f->write_uint8(0xc0 | count);
        f->write_uint8(byte);
        return 2;
    }
    return 0;
}

// 0x5565a0  Confidence: exact (the NULL test inside the loop is in the binary). The n-th message of
// bm_image_error_strings (the PCX return codes).
const char *bm_image_error_string(int n)
{
    const char *s = bm_image_error_strings;

    while (n--) {
        if (s == NULL)
            return NULL;
        s += strlen(s) + 1;
    }
    return s;
}
