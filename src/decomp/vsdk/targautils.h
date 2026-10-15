// targautils.h - Sum.exe v1.40 image file helpers (vsdk\bmpman\targautils.cpp, 0x5549c0-0x5565d6):
// TGA, PCX and m2v (one MPEG-2 intra picture) readers and writers used by bmpman.cpp and gr_screenshot.
// Written from the Sum.exe disassembly. Names from symbols/sum.symbols.
#ifndef TARGAUTILS_H
#define TARGAUTILS_H

#include "stubs_container_targa.h"

// Return codes of the PCX functions (strings: bm_image_error_string): 0 no error, 1 error opening
// file, 2 couldn't read PCX header, 3 unsupported PCX version, 4 error reading data,
// 5 couldn't find palette information (unused here), 6 error writing data.
// The TGA readers return 0 on success and 1 on any failure; the writers 0 / -1.

extern int m2v_quantizer_scale;        // 0x2f53aec (.bss)
extern int tga_last_path_id;           // 0x5ada64 (.data, -1)

// MPEG-2 helpers on a bit reader (no callers in Sum.exe v1.40).
int  m2v_skip_extension_and_user_data(m2v_bitstream *bs);               // 0x5549c0
void m2v_skip_extra_bit_information(m2v_bitstream *bs);                 // 0x554ac0
void m2v_read_slice_quantizer_scale(m2v_bitstream *bs);                 // 0x554af0
int  m2v_skip_macroblock_stuffing(m2v_bitstream *bs);                   // 0x554b10
int  m2v_read_header(const char *name, int *w, int *h);                 // 0x554b60 file size, 0 = failure
int  m2v_read_bitmap(const char *name, ubyte **data);                   // 0x554d00 file size, 0 = failure

// TGA
int  tga_rle_encode_row(ubyte *dst, ubyte *src, int dst_bpp, int src_bpp, int row_bytes);  // 0x554ea0
int  tga_convert_pixels(ubyte *dst, ubyte *src, int count, int src_bpp, int dst_bpp);     // 0x555020
int  bytes_equal(ubyte *a, ubyte *b, int n);                                              // 0x5550d0
int  tga_read_header(const char *name, int *w, int *h, int *bpp, void *unused1, void *unused2,
                     int path_id);                                                        // 0x555110
int  tga_rle_decode(ubyte *dst, ubyte *src, int num_pixels, int pixel_size);              // 0x555330
void copy_pixels_advance(int n, ubyte **dst, ubyte **src, int size);                      // 0x555410
int  tga_read_bitmap(const char *name, ubyte *data, ubyte *palette, int unused,
                     int path_id);                                                        // 0x555450
int  tga_write_rle(const char *name, ubyte *data, int unused, int w, int h, int bpp,
                   int path_id);                                                          // 0x555970
int  tga_write(const char *name, ubyte *data, int unused, int w, int h, int bpp,
               int path_id);                                                              // 0x555b50

// PCX
int  pcx_read_header(const char *name, int *w, int *h, int *bpp, ubyte *palette,
                     int path_id);                                                        // 0x555cf0
int  pcx_read_bitmap(const char *name, ubyte *data, ubyte *palette, int path_id);         // 0x555f60
int  pcx_write(const char *name, int w, int h, ubyte **rows, ubyte *palette);             // 0x5562a0
int  pcx_encode_line(ubyte *src, int width, File *f);                                     // 0x556470
int  pcx_encode_byte(ubyte byte, ubyte count, File *f);                                   // 0x556550

const char *bm_image_error_string(int n);                                                 // 0x5565a0

#endif
