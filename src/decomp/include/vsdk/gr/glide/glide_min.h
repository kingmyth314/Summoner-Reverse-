// vsdk/gr/glide/glide_min.h - stands in for 3dfx's <glide.h> / <glideutl.h> (ours, decomp45, D2500).
// Sum.exe does not link Glide: glide_load_dll (0x5473b0) loads "glide2x.dll" and fetches 112 entry points
// with GetProcAddress by their decorated __stdcall names ("_grDrawLine@8" ...), so this is the Glide 2.x
// API (not Glide 3). Only the types, constants and pointer types that the unit needs are here; the
// prototypes follow the decorated names (argument bytes) and the call sites. Not original source.
#ifndef VSDK_GR_GLIDE_GLIDE_MIN_H
#define VSDK_GR_GLIDE_GLIDE_MIN_H

#include "vsdk/vsdk.h"

#define FX_CALL __stdcall

typedef unsigned char  FxU8;
typedef unsigned short FxU16;
typedef short          FxI16;
typedef unsigned int   FxU32;
typedef int            FxI32;
typedef int            FxBool;
#define FXFALSE 0
#define FXTRUE  1

typedef FxU32 GrColor_t;                 // 0x00BBGGRR with GR_COLORFORMAT_ABGR
typedef FxU8  GrAlpha_t;
typedef FxI32 GrChipID_t;
typedef FxI32 GrLOD_t;
typedef FxI32 GrAspectRatio_t;
typedef FxI32 GrTextureFormat_t;
typedef FxI32 GrCombineFunction_t;
typedef FxI32 GrCombineFactor_t;
typedef FxI32 GrCombineLocal_t;
typedef FxI32 GrCombineOther_t;
typedef FxI32 GrAlphaBlendFnc_t;
typedef FxI32 GrCmpFnc_t;
typedef FxI32 GrDepthBufferMode_t;
typedef FxI32 GrFogMode_t;
typedef FxI32 GrTextureCombineFnc_t;
typedef FxI32 GrTextureFilterMode_t;
typedef FxI32 GrTextureClampMode_t;
typedef FxI32 GrMipMapMode_t;
typedef FxI32 GrBuffer_t;
typedef FxI32 GrLock_t;
typedef FxI32 GrLfbWriteMode_t;
typedef FxI32 GrLfbSrcFmt_t;
typedef FxI32 GrOriginLocation_t;
typedef FxI32 GrScreenResolution_t;
typedef FxI32 GrScreenRefresh_t;
typedef FxI32 GrColorFormat_t;
typedef FxI32 GrSstType;
typedef FxI32 GrHint_t;
typedef FxI32 GrChromakeyMode_t;
typedef FxI32 GrCullMode_t;
typedef FxI32 GrDitherMode_t;
typedef FxI32 GrTexTable_t;
typedef FxI32 GrTexBaseRange_t;
typedef FxI32 GrNCCTable_t;
typedef FxI32 GrColorCombineFnc_t;
typedef FxI32 GrMipMapId_t;
typedef FxU8  GrFog_t;

// ---- vertices -----------------------------------------------------------------------------------------
#define GLIDE_NUM_TMU 2
typedef struct {
    float sow;                           // s / w
    float tow;                           // t / w
    float oow;                           // 1 / w
} GrTmuVertex;
typedef struct {
    float x, y, z;                       // +0x00 screen x, y (snapped), z
    float r, g, b;                       // +0x0c 0..255
    float ooz;                           // +0x18
    float a;                             // +0x1c 0..255
    float oow;                           // +0x20
    GrTmuVertex tmuvtx[GLIDE_NUM_TMU];   // +0x24
} GrVertex;
LAYOUT_CHECK(GrVertex, sizeof(GrVertex) == 0x3c);

// ---- textures -----------------------------------------------------------------------------------------
typedef struct {
    GrLOD_t smallLod;                    // +0x00
    GrLOD_t largeLod;                    // +0x04
    GrAspectRatio_t aspectRatio;         // +0x08
    GrTextureFormat_t format;            // +0x0c
    void *data;                          // +0x10
} GrTexInfo;
LAYOUT_CHECK(GrTexInfo, sizeof(GrTexInfo) == 0x14);

// ---- linear frame buffer ------------------------------------------------------------------------------
typedef struct {
    int size;                            // +0x00 sizeof(GrLfbInfo_t), set by the caller
    void *lfbPtr;                        // +0x04
    FxU32 strideInBytes;                 // +0x08
    GrLfbWriteMode_t writeMode;          // +0x0c
    GrOriginLocation_t origin;           // +0x10
} GrLfbInfo_t;
LAYOUT_CHECK(GrLfbInfo_t, sizeof(GrLfbInfo_t) == 0x14);

// ---- hardware query (grSstQueryHardware) --------------------------------------------------------------
#define MAX_NUM_SST 4
typedef struct {
    int tmuRev;
    int tmuRam;
} GrTMUConfig_t;
typedef struct {
    int fbRam;                           // +0x00 MB (gr_glide_init reads SSTs[0] +8 from the record start)
    int fbiRev;                          // +0x04
    int nTexelfx;                        // +0x08 number of TMUs
    FxBool sliDetect;                    // +0x0c
    GrTMUConfig_t tmuConfig[GLIDE_NUM_TMU];   // +0x10
} GrVoodooConfig_t;
typedef struct {
    int fbRam;
    int nTexelfx;
    GrTMUConfig_t tmuConfig;
} GrSst96Config_t;
typedef struct {
    int rev;
} GrAT3DConfig_t;
typedef GrVoodooConfig_t GrVoodoo2Config_t;
typedef struct {
    int num_sst;                         // +0x00
    struct {
        GrSstType type;                  // +0x04
        union {
            GrVoodooConfig_t VoodooConfig;   // +0x08
            GrSst96Config_t SST96Config;
            GrAT3DConfig_t AT3DConfig;
            GrVoodoo2Config_t Voodoo2Config;
        } sstBoard;
    } SSTs[MAX_NUM_SST];
} GrHwConfiguration;
LAYOUT_CHECK(GrHwConfiguration, sizeof(GrHwConfiguration) == 0x94);

// Opaque records of entry points the game fetches but never calls.
struct GrState;
struct GrSstPerfStats_t;
struct GrMipMapInfo;
struct GuNccTable;

// ---- constants (the values the call sites push) ------------------------------------------------------
#define GR_TMU0 0
#define GR_TMU1 1

#define GR_LOD_256 0
#define GR_LOD_128 1
#define GR_LOD_64  2
#define GR_LOD_32  3
#define GR_LOD_16  4
#define GR_LOD_8   5
#define GR_LOD_4   6
#define GR_LOD_2   7
#define GR_LOD_1   8

#define GR_ASPECT_8x1 0
#define GR_ASPECT_4x1 1
#define GR_ASPECT_2x1 2
#define GR_ASPECT_1x1 3
#define GR_ASPECT_1x2 4
#define GR_ASPECT_1x4 5
#define GR_ASPECT_1x8 6

#define GR_TEXFMT_ALPHA_8   0x2
#define GR_TEXFMT_RGB_565   0xa
#define GR_TEXFMT_ARGB_1555 0xb
#define GR_TEXFMT_ARGB_4444 0xc

#define GR_MIPMAPLEVELMASK_BOTH 3

#define GR_MIPMAP_DISABLE 0
#define GR_MIPMAP_NEAREST 1

#define GR_TEXTUREFILTER_POINT_SAMPLED 0
#define GR_TEXTUREFILTER_BILINEAR      1
#define GR_TEXTURECLAMP_WRAP  0
#define GR_TEXTURECLAMP_CLAMP 1

#define GR_TEXTURECOMBINE_DECAL    1
#define GR_TEXTURECOMBINE_ADD      3
#define GR_TEXTURECOMBINE_MULTIPLY 4
#define GR_TEXTURECOMBINE_ONE      0xa

#define GR_COMBINE_FUNCTION_LOCAL                 1
#define GR_COMBINE_FUNCTION_SCALE_OTHER           3
#define GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL 4
#define GR_COMBINE_FACTOR_ZERO  0
#define GR_COMBINE_FACTOR_LOCAL 1
#define GR_COMBINE_FACTOR_ONE   8
#define GR_COMBINE_LOCAL_ITERATED 0
#define GR_COMBINE_LOCAL_CONSTANT 1
#define GR_COMBINE_OTHER_ITERATED 0
#define GR_COMBINE_OTHER_TEXTURE  1
#define GR_COMBINE_OTHER_CONSTANT 2

#define GR_BLEND_ZERO                0x0
#define GR_BLEND_SRC_ALPHA           0x1
#define GR_BLEND_DST_COLOR           0x2
#define GR_BLEND_ONE                 0x4
#define GR_BLEND_ONE_MINUS_SRC_ALPHA 0x5
#define GR_BLEND_PREFOG_COLOR        0xf

#define GR_CMP_EQUAL   2
#define GR_CMP_LEQUAL  3
#define GR_CMP_GREATER 4
#define GR_CMP_ALWAYS  7

#define GR_DEPTHBUFFER_DISABLE 0
#define GR_DEPTHBUFFER_WBUFFER 2

#define GR_FOG_DISABLE    0
#define GR_FOG_WITH_TABLE 2
#define GR_FOG_MULT2      0x100
#define GR_FOG_ADD2       0x200
#define GR_FOG_TABLE_SIZE 64

#define GR_BUFFER_FRONTBUFFER 0
#define GR_BUFFER_BACKBUFFER  1
#define GR_LFB_WRITE_ONLY     1
#define GR_LFBWRITEMODE_ANY   0xff
#define GR_ORIGIN_UPPER_LEFT  0
#define GR_WDEPTHVALUE_FARTHEST 0xffff

#define GR_RESOLUTION_640x480  7
#define GR_RESOLUTION_800x600  8
#define GR_RESOLUTION_1024x768 0xc
#define GR_REFRESH_60Hz        0
#define GR_COLORFORMAT_ABGR    1

#define GR_CONTROL_ACTIVATE   1
#define GR_CONTROL_DEACTIVATE 2

#define GR_HINT_STWHINT          0
#define GR_STWHINT_ST_DIFF_TMU1  0x10

// ---- entry point types (__stdcall; the decorated size in the comment) ---------------------------------
typedef void (FX_CALL *grDrawLine_t)(const GrVertex *v1, const GrVertex *v2);                      // @8
typedef void (FX_CALL *grDrawPlanarPolygon_t)(int nverts, const int ilist[], const GrVertex vlist[]);  // @12
typedef void (FX_CALL *grDrawPlanarPolygonVertexList_t)(int nverts, const GrVertex vlist[]);       // @8
typedef void (FX_CALL *grDrawPoint_t)(const GrVertex *pt);                                         // @4
typedef void (FX_CALL *grDrawPolygon_t)(int nverts, const int ilist[], const GrVertex vlist[]);    // @12
typedef void (FX_CALL *grDrawPolygonVertexList_t)(int nverts, const GrVertex vlist[]);             // @8
typedef void (FX_CALL *grDrawTriangle_t)(const GrVertex *a, const GrVertex *b, const GrVertex *c); // @12
typedef void (FX_CALL *guDrawTriangleWithClip_t)(const GrVertex *a, const GrVertex *b, const GrVertex *c);  // @12
typedef void (FX_CALL *grBufferClear_t)(GrColor_t color, GrAlpha_t alpha, FxU16 depth);            // @12
typedef int (FX_CALL *grBufferNumPending_t)();                                                     // @0
typedef void (FX_CALL *grBufferSwap_t)(int swap_interval);                                         // @4
typedef void (FX_CALL *grRenderBuffer_t)(GrBuffer_t buffer);                                       // @4
typedef void (*GrErrorCallbackFnc_t)(const char *string, FxBool fatal);
typedef void (FX_CALL *grErrorSetCallback_t)(GrErrorCallbackFnc_t fnc);                            // @4
typedef void (FX_CALL *grSstIdle_t)();                                                             // @0
typedef FxU32 (FX_CALL *grSstVideoLine_t)();                                                       // @0
typedef FxBool (FX_CALL *grSstVRetraceOn_t)();                                                     // @0
typedef FxBool (FX_CALL *grSstIsBusy_t)();                                                         // @0
typedef FxBool (FX_CALL *grSstWinOpen_t)(FxU32 hWnd, GrScreenResolution_t res, GrScreenRefresh_t ref,
                                         GrColorFormat_t cformat, GrOriginLocation_t org,
                                         int nColBuffers, int nAuxBuffers);                        // @28
typedef void (FX_CALL *grSstWinClose_t)();                                                         // @0
typedef FxBool (FX_CALL *grSstControl_t)(FxU32 code);                                              // @4
typedef FxBool (FX_CALL *grSstQueryHardware_t)(GrHwConfiguration *hwconfig);                       // @4
typedef FxBool (FX_CALL *grSstQueryBoards_t)(GrHwConfiguration *hwconfig);                         // @4
typedef void (FX_CALL *grSstOrigin_t)(GrOriginLocation_t origin);                                  // @4
typedef void (FX_CALL *grSstSelect_t)(int which_sst);                                              // @4
typedef FxU32 (FX_CALL *grSstScreenHeight_t)();                                                    // @0
typedef FxU32 (FX_CALL *grSstScreenWidth_t)();                                                     // @0
typedef FxU32 (FX_CALL *grSstStatus_t)();                                                          // @0
typedef void (FX_CALL *grSstPerfStats_t)(GrSstPerfStats_t *stats);                                 // @4
typedef void (FX_CALL *grSstResetPerfStats_t)();                                                   // @0
typedef void (FX_CALL *grResetTriStats_t)();                                                       // @0
typedef void (FX_CALL *grTriStats_t)(FxU32 *trisProcessed, FxU32 *trisDrawn);                      // @8
typedef void (FX_CALL *grAlphaBlendFunction_t)(GrAlphaBlendFnc_t rgb_sf, GrAlphaBlendFnc_t rgb_df,
                                               GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df);  // @16
typedef void (FX_CALL *grAlphaCombine_t)(GrCombineFunction_t function, GrCombineFactor_t factor,
                                         GrCombineLocal_t local, GrCombineOther_t other, FxBool invert);  // @20
typedef void (FX_CALL *grAlphaControlsITRGBLighting_t)(FxBool enable);                             // @4
typedef void (FX_CALL *grAlphaTestFunction_t)(GrCmpFnc_t function);                                // @4
typedef void (FX_CALL *grAlphaTestReferenceValue_t)(GrAlpha_t value);                              // @4
typedef void (FX_CALL *grChromakeyMode_t)(GrChromakeyMode_t mode);                                 // @4
typedef void (FX_CALL *grChromakeyValue_t)(GrColor_t value);                                       // @4
typedef void (FX_CALL *grClipWindow_t)(FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy);            // @16
typedef void (FX_CALL *grColorCombine_t)(GrCombineFunction_t function, GrCombineFactor_t factor,
                                         GrCombineLocal_t local, GrCombineOther_t other, FxBool invert);  // @20
typedef void (FX_CALL *grColorMask_t)(FxBool rgb, FxBool a);                                       // @8
typedef void (FX_CALL *grCullMode_t)(GrCullMode_t mode);                                           // @4
typedef void (FX_CALL *grConstantColorValue_t)(GrColor_t value);                                   // @4
typedef void (FX_CALL *grConstantColorValue4_t)(float a, float r, float g, float b);               // @16
typedef void (FX_CALL *grDepthBiasLevel_t)(FxI16 level);                                           // @4
typedef void (FX_CALL *grDepthBufferFunction_t)(GrCmpFnc_t function);                              // @4
typedef void (FX_CALL *grDepthBufferMode_t)(GrDepthBufferMode_t mode);                             // @4
typedef void (FX_CALL *grDepthMask_t)(FxBool mask);                                                // @4
typedef void (FX_CALL *grDisableAllEffects_t)();                                                   // @0
typedef void (FX_CALL *grDitherMode_t)(GrDitherMode_t mode);                                       // @4
typedef void (FX_CALL *grFogColorValue_t)(GrColor_t fogcolor);                                     // @4
typedef void (FX_CALL *grFogMode_t)(GrFogMode_t mode);                                             // @4
typedef void (FX_CALL *grFogTable_t)(const GrFog_t ft[GR_FOG_TABLE_SIZE]);                         // @4
typedef void (FX_CALL *grGammaCorrectionValue_t)(float value);                                     // @4
typedef void (FX_CALL *grSplash_t)(float x, float y, float width, float height, FxU32 frame);      // @20
typedef FxU32 (FX_CALL *grTexCalcMemRequired_t)(GrLOD_t smallLod, GrLOD_t largeLod, GrAspectRatio_t aspect,
                                                GrTextureFormat_t format);                         // @16
typedef FxU32 (FX_CALL *grTexTextureMemRequired_t)(FxU32 evenOdd, GrTexInfo *info);                // @8
typedef FxU32 (FX_CALL *grTexMinAddress_t)(GrChipID_t tmu);                                        // @4
typedef FxU32 (FX_CALL *grTexMaxAddress_t)(GrChipID_t tmu);                                        // @4
typedef void (FX_CALL *grTexNCCTable_t)(GrChipID_t tmu, GrNCCTable_t table);                       // @8
typedef void (FX_CALL *grTexSource_t)(GrChipID_t tmu, FxU32 startAddress, FxU32 evenOdd, GrTexInfo *info);  // @16
typedef void (FX_CALL *grTexClampMode_t)(GrChipID_t tmu, GrTextureClampMode_t s, GrTextureClampMode_t t);  // @12
typedef void (FX_CALL *grTexCombine_t)(GrChipID_t tmu, GrCombineFunction_t rgb_function,
                                       GrCombineFactor_t rgb_factor, GrCombineFunction_t alpha_function,
                                       GrCombineFactor_t alpha_factor, FxBool rgb_invert,
                                       FxBool alpha_invert);                                       // @28
typedef void (FX_CALL *grTexCombineFunction_t)(GrChipID_t tmu, GrTextureCombineFnc_t fnc);         // @8
typedef void (FX_CALL *grTexDetailControl_t)(GrChipID_t tmu, int lod_bias, FxU8 detail_scale,
                                             float detail_max);                                    // @16
typedef void (FX_CALL *grTexFilterMode_t)(GrChipID_t tmu, GrTextureFilterMode_t minfilter,
                                          GrTextureFilterMode_t magfilter);                        // @12
typedef void (FX_CALL *grTexLodBiasValue_t)(GrChipID_t tmu, float bias);                           // @8
typedef void (FX_CALL *grTexDownloadMipMap_t)(GrChipID_t tmu, FxU32 startAddress, FxU32 evenOdd,
                                              GrTexInfo *info);                                    // @16
typedef void (FX_CALL *grTexDownloadMipMapLevel_t)(GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLod,
                                                   GrLOD_t largeLod, GrAspectRatio_t aspectRatio,
                                                   GrTextureFormat_t format, FxU32 evenOdd, void *data);  // @32
typedef void (FX_CALL *grTexDownloadMipMapLevelPartial_t)(GrChipID_t tmu, FxU32 startAddress,
                                                          GrLOD_t thisLod, GrLOD_t largeLod,
                                                          GrAspectRatio_t aspectRatio, GrTextureFormat_t format,
                                                          FxU32 evenOdd, void *data, int start, int end);  // @40
typedef void (FX_CALL *ConvertAndDownloadRle_t)(GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLod,
                                                GrLOD_t largeLod, GrAspectRatio_t aspectRatio,
                                                GrTextureFormat_t format, FxU32 evenOdd, FxU8 *bm_data,
                                                long bm_h, FxU32 u0, FxU32 v0, FxU32 width, FxU32 height,
                                                FxU32 dest_width, FxU32 dest_height, FxU16 *tlut);  // @64
typedef void (FX_CALL *grCheckForRoom_t)(FxI32 n);                                                 // @4
typedef void (FX_CALL *grTexDownloadTable_t)(GrChipID_t tmu, GrTexTable_t type, void *data);       // @12
typedef void (FX_CALL *grTexDownloadTablePartial_t)(GrChipID_t tmu, GrTexTable_t type, void *data,
                                                    int start, int end);                           // @20
typedef void (FX_CALL *grTexMipMapMode_t)(GrChipID_t tmu, GrMipMapMode_t mode, FxBool lodBlend);   // @12
typedef void (FX_CALL *grTexMultibase_t)(GrChipID_t tmu, FxBool enable);                           // @8
typedef void (FX_CALL *grTexMultibaseAddress_t)(GrChipID_t tmu, GrTexBaseRange_t range, FxU32 startAddress,
                                                FxU32 evenOdd, GrTexInfo *info);                   // @20
typedef GrMipMapId_t (FX_CALL *guTexAllocateMemory_t)(GrChipID_t tmu, FxU8 odd_even_mask, int width,
                                                      int height, GrTextureFormat_t fmt,
                                                      GrMipMapMode_t mm_mode, GrLOD_t smallest_lod,
                                                      GrLOD_t largest_lod, GrAspectRatio_t aspect,
                                                      GrTextureClampMode_t s_clamp_mode,
                                                      GrTextureClampMode_t t_clamp_mode,
                                                      GrTextureFilterMode_t minfilter_mode,
                                                      GrTextureFilterMode_t magfilter_mode,
                                                      float lod_bias, FxBool trilinear);           // @60
typedef FxBool (FX_CALL *guTexChangeAttributes_t)(GrMipMapId_t mmid, int width, int height,
                                                  GrTextureFormat_t fmt, GrMipMapMode_t mm_mode,
                                                  GrLOD_t smallest_lod, GrLOD_t largest_lod,
                                                  GrAspectRatio_t aspect, GrTextureClampMode_t s_clamp_mode,
                                                  GrTextureClampMode_t t_clamp_mode,
                                                  GrTextureFilterMode_t minFilterMode,
                                                  GrTextureFilterMode_t magFilterMode);            // @48
typedef void (FX_CALL *guTexCombineFunction_t)(GrChipID_t tmu, GrTextureCombineFnc_t fnc);         // @8
typedef GrMipMapId_t (FX_CALL *guTexGetCurrentMipMap_t)(GrChipID_t tmu);                           // @4
typedef GrMipMapInfo *(FX_CALL *guTexGetMipMapInfo_t)(GrMipMapId_t mmid);                          // @4
typedef FxU32 (FX_CALL *guTexMemQueryAvail_t)(GrChipID_t tmu);                                     // @4
typedef void (FX_CALL *guTexMemReset_t)();                                                         // @0
typedef void (FX_CALL *guTexDownloadMipMap_t)(GrMipMapId_t mmid, const void *src, const GuNccTable *table);  // @12
typedef void (FX_CALL *guTexDownloadMipMapLevel_t)(GrMipMapId_t mmid, GrLOD_t lod, const void **src);  // @12
typedef void (FX_CALL *guTexSource_t)(GrMipMapId_t id);                                            // @4
typedef void (FX_CALL *guColorCombineFunction_t)(GrColorCombineFnc_t fnc);                         // @4
typedef FxBool (FX_CALL *grLfbLock_t)(GrLock_t type, GrBuffer_t buffer, GrLfbWriteMode_t writeMode,
                                      GrOriginLocation_t origin, FxBool pixelPipeline, GrLfbInfo_t *info);  // @24
typedef FxBool (FX_CALL *grLfbUnlock_t)(GrLock_t type, GrBuffer_t buffer);                         // @8
typedef void (FX_CALL *grLfbConstantAlpha_t)(GrAlpha_t alpha);                                     // @4
typedef void (FX_CALL *grLfbConstantDepth_t)(FxU16 depth);                                         // @4
typedef void (FX_CALL *grLfbWriteColorSwizzle_t)(FxBool swizzleBytes, FxBool swapWords);           // @8
typedef void (FX_CALL *grLfbWriteColorFormat_t)(GrColorFormat_t colorFormat);                      // @4
typedef FxBool (FX_CALL *grLfbWriteRegion_t)(GrBuffer_t dst_buffer, FxU32 dst_x, FxU32 dst_y,
                                             GrLfbSrcFmt_t src_format, FxU32 src_width, FxU32 src_height,
                                             FxI32 src_stride, void *src_data);                    // @32
typedef FxBool (FX_CALL *grLfbReadRegion_t)(GrBuffer_t src_buffer, FxU32 src_x, FxU32 src_y,
                                            FxU32 src_width, FxU32 src_height, FxU32 dst_stride,
                                            void *dst_data);                                       // @28
typedef void (FX_CALL *grAADrawLine_t)(const GrVertex *v1, const GrVertex *v2);                    // @8
typedef void (FX_CALL *grAADrawPoint_t)(const GrVertex *pt);                                       // @4
typedef void (FX_CALL *grAADrawPolygon_t)(int nverts, const int ilist[], const GrVertex vlist[]);  // @12
typedef void (FX_CALL *grAADrawPolygonVertexList_t)(int nverts, const GrVertex vlist[]);           // @8
typedef void (FX_CALL *grAADrawTriangle_t)(const GrVertex *a, const GrVertex *b, const GrVertex *c,
                                           FxBool ab_antialias, FxBool bc_antialias, FxBool ca_antialias);  // @24
typedef void (FX_CALL *grGlideInit_t)();                                                           // @0
typedef void (FX_CALL *grGlideShutdown_t)();                                                       // @0
typedef void (FX_CALL *grGlideGetVersion_t)(char version[80]);                                     // @4
typedef void (FX_CALL *grGlideGetState_t)(GrState *state);                                         // @4
typedef void (FX_CALL *grGlideSetState_t)(const GrState *state);                                   // @4
typedef void (FX_CALL *grGlideShamelessPlug_t)(const FxBool on);                                   // @4
typedef void (FX_CALL *grHints_t)(GrHint_t hintType, FxU32 hintMask);                              // @8
typedef float (FX_CALL *guFogTableIndexToW_t)(int i);                                              // @4
typedef void (FX_CALL *guFogGenerateExp_t)(GrFog_t fogtable[GR_FOG_TABLE_SIZE], float density);    // @8
typedef void (FX_CALL *guFogGenerateExp2_t)(GrFog_t fogtable[GR_FOG_TABLE_SIZE], float density);   // @8
typedef void (FX_CALL *guFogGenerateLinear_t)(GrFog_t fogtable[GR_FOG_TABLE_SIZE], float nearZ,
                                              float farZ);                                         // @12

#endif
