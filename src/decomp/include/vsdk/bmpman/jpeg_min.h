// vsdk/bmpman/jpeg_min.h - the part of the IJG libjpeg 6b compression API (jpeglib.h) that Volition's
// jpeg_write_file (0x5565e0) uses (ours; stands in for <jpeglib.h>, like gr/directx/d3d_min.h). The library
// itself (0x565660-0x566550 and 0x567430-0x56c732) is third-party code compiled from the IJG sources and is
// not rewritten (decomp66, NOTES.md). Field names and order follow jpeglib.h of release 6b
// ("6b 27-Mar-1998" at 0x5aecbc); offsets from jpeg_write_file's stores. Not original source.
#ifndef VSDK_BMPMAN_JPEG_MIN_H
#define VSDK_BMPMAN_JPEG_MIN_H

#include <stdio.h>
#include "vsdk/vsdk.h"

#define JPEG_LIB_VERSION 62               // jpeg_CreateCompress's second argument (push 0x3e)

typedef unsigned char JSAMPLE;
typedef JSAMPLE *JSAMPROW;
typedef JSAMPROW *JSAMPARRAY;
typedef unsigned int JDIMENSION;
typedef int boolean;

typedef enum { JCS_UNKNOWN, JCS_GRAYSCALE, JCS_RGB, JCS_YCbCr, JCS_CMYK, JCS_YCCK } J_COLOR_SPACE;
typedef enum { JDCT_ISLOW, JDCT_IFAST, JDCT_FLOAT } J_DCT_METHOD;

// The error manager (0x84 bytes: jpeg_write_file's frame puts it between the compress object and the row
// table). Only its size matters here.
struct jpeg_error_mgr {
    void *error_exit;                     // +0x00 method pointers, then the message state
    ubyte _unk04[0x80];
};
LAYOUT_CHECK(jpeg_error_mgr, sizeof(jpeg_error_mgr) == 0x84);

// The compression object (0x178 bytes, jpeg_CreateCompress's third argument).
struct jpeg_compress_struct {
    jpeg_error_mgr *err;                  // +0x00
    void *mem;                            // +0x04
    void *progress;                       // +0x08
    void *client_data;                    // +0x0c
    boolean is_decompressor;              // +0x10
    int global_state;                     // +0x14
    void *dest;                           // +0x18
    JDIMENSION image_width;               // +0x1c
    JDIMENSION image_height;              // +0x20
    int input_components;                 // +0x24
    J_COLOR_SPACE in_color_space;         // +0x28
    double input_gamma;                   // +0x30
    int data_precision;                   // +0x38
    int num_components;                   // +0x3c
    J_COLOR_SPACE jpeg_color_space;       // +0x40
    void *comp_info;                      // +0x44
    void *quant_tbl_ptrs[4];              // +0x48
    void *dc_huff_tbl_ptrs[4];            // +0x58
    void *ac_huff_tbl_ptrs[4];            // +0x68
    ubyte arith_dc_L[16];                 // +0x78
    ubyte arith_dc_U[16];                 // +0x88
    ubyte arith_ac_K[16];                 // +0x98
    int num_scans;                        // +0xa8
    const void *scan_info;                // +0xac
    boolean raw_data_in;                  // +0xb0
    boolean arith_code;                   // +0xb4
    boolean optimize_coding;              // +0xb8
    boolean CCIR601_sampling;             // +0xbc
    int smoothing_factor;                 // +0xc0
    J_DCT_METHOD dct_method;              // +0xc4
    ubyte _unkc8[0xb0];                   // +0xc8 the rest of the object
};
LAYOUT_CHECK(jpeg_compress_struct, sizeof(jpeg_compress_struct) == 0x178);

// libjpeg entry points (IJG names; addresses in Sum.exe). The library is C, so the C++ caller wrapped
// the IJG header in extern "C" (the calls go to the plain names).
extern "C" {
jpeg_error_mgr *jpeg_std_error(jpeg_error_mgr *err);                          // 0x5663b0 jerror.c
void jpeg_CreateCompress(jpeg_compress_struct *cinfo, int version, unsigned int structsize);   // 0x565660 jcapimin.c
#define jpeg_create_compress(cinfo) \
    jpeg_CreateCompress((cinfo), JPEG_LIB_VERSION, (unsigned int)sizeof(jpeg_compress_struct))
void jpeg_destroy_compress(jpeg_compress_struct *cinfo);                      // 0x565720 jcapimin.c
void jpeg_finish_compress(jpeg_compress_struct *cinfo);                       // 0x565790 jcapimin.c
void jpeg_start_compress(jpeg_compress_struct *cinfo, boolean write_all_tables);   // 0x5659c0 jcapistd.c
JDIMENSION jpeg_write_scanlines(jpeg_compress_struct *cinfo, JSAMPARRAY scanlines,
                                JDIMENSION num_lines);                        // 0x565a30 jcapistd.c
void jpeg_set_defaults(jpeg_compress_struct *cinfo);                          // 0x565d30 jcparam.c
void jpeg_stdio_dest(jpeg_compress_struct *cinfo, FILE *outfile);             // 0x5662a0 jdatadst.c
}

#endif
