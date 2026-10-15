// unnamed_jpeg_write_0x5565e0.cpp - Sum.exe v1.40, code 0x5565e0-0x5566d9: Volition's JPEG writer for
// gr_screenshot (format != 0). The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): targautils.cpp ends at 0x5565d6 with its .data at 0x5adb88 (".pcx");
// this function's only literal "wb" is the next .data word (0x5adb90), and the String unit's .data
// (TextBuffer's two globals at 0x5adb94 / 0x5adb98, which that file defines before its String literals)
// follows. It uses no .bss. Whether it is a file of its own or the head of the String file cannot be
// decided from the layout; it shares nothing with the String code, so it is kept apart.

#include <stdio.h>
#include "vsdk/bmpman/targautils.h"
#include "vsdk/bmpman/jpeg_min.h"

// 0x5565e0  Confidence: likely (the row table is the 4 KB array at the top of the 0x11fc frame; the
// _chkstk call is the compiler's for a frame above one page).
// (name, rgb, w, h): R, G, B bytes, w * 3 per row, top-down. Default quality, the float DCT.
// The row table holds 1024 rows: a screen taller than 1024 lines overruns it (game bug, NOTES.md).
void jpeg_write_file(const char *name, ubyte *rgb, int w, int h)
{
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    JSAMPROW rows[1024];
    FILE *fp;
    int i;

    fp = fopen(name, "wb");
    if (fp == NULL)
        return;
    for (i = 0; i < h; i++) {
        rows[i] = rgb;
        rgb += w * 3;
    }
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);
    cinfo.image_width = w;
    cinfo.image_height = h;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    cinfo.dct_method = JDCT_FLOAT;
    jpeg_start_compress(&cinfo, 1);
    jpeg_write_scanlines(&cinfo, rows, h);
    jpeg_finish_compress(&cinfo);
    fclose(fp);
    jpeg_destroy_compress(&cinfo);
}
