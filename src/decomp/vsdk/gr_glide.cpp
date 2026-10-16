// gr_glide.cpp - Sum.exe v1.40, code 0x547390-0x54bb27 (vsdk\gr\glide\gr_glide.cpp), the 3dfx Glide back
// end (renderer 0x67, command line -glide). Glide 2: the 112 entry points are fetched from glide2x.dll at
// run time (glide_load_dll), so every API call is through a glide_gr* / glide_gu* pointer (D2500).
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Like the other vsdk objects this unit calls the header inlines out of line (gr::Mode's accessors and
// constructors, D1300), so it was built without inline expansion.
//
// Layout of the unit: the DLL loader, the texture cache (0x547990-0x549730), the TMU memory manager
// (glide_record, 0x549780-0x549cbb), then the device and the drawing code. The 16 Direct3D texture cache
// functions after 0x54bb60 that sum.symbols also tags gr_glide.cpp are vsdk\gr\directx (source-map.md).
//
// Data. .data: the literals of glide_load_dll / gr_glide_tcache_init / gr_glide_create_mipmapped_texture
// (0x5aca8c-0x5ad4fd), then the globals 0x5ad500-0x5ad54f, the literal "whee" (0x5ad550), the literals of
// gr_glide_init (0x5ad558-0x5ad79b; the XSTR texts are among them although the code reads
// localize_strings). .bss 0x2f0e9a8-0x2f132d3 in address order below; "" of ccmd_whee is the .bss literal
// 0x2f132d0. Three static initialisers (table 0x588a0c-0x588a14): glide_tmus, gr_glide_current_mode,
// ccmd_whee; they are defined where their initialiser code lies.
//
// Not source of this unit: tcache_slot (gr.h) is called, its members are in the Direct3D texture cache.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "vsdk/os/debug.h"
#include "vsdk/os/console.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/os.h"
#include "vsdk/math/vector.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/localize/localize.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/glide/glide_min.h"
#include "vsdk/gr/glide/gr_glide.h"

// Original __FILE__ of this unit (0x5ad58c ... 0x5ad718; string pooling is off).
#define GR_GLIDE_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\gr\\glide\\gr_glide.cpp"

// MessageBox flags of the fatal message (0x12000).
#define GLIDE_MB_FLAGS (MB_TASKMODAL | MB_SETFOREGROUND)

// 3 << 18: added to the screen x / y of every vertex so that the float keeps 4 bits of fraction, the
// usual Glide snap (0x581ad0 = 786432.0f).
#define GLIDE_SNAP 786432.0f

// 16.16 steps of the section converters (a whole texel: they copy 1:1).
#define GLIDE_STEP_ONE 65536

// A texture side rounded up to a Glide size, 4 .. 256 (the compare chain of 0x547c00 and 0x548539).
#define GLIDE_TEXTURE_SIZE(n) ((n) <= 4 ? 4 : (n) <= 8 ? 8 : (n) <= 16 ? 16 : (n) <= 32 ? 32 : \
                               (n) <= 64 ? 64 : (n) <= 128 ? 128 : 256)

// The draw calls do nothing while the Glide window is closed (glide_set_active).
#define GLIDE_WINDOW_CLOSED (glide_close_on_deactivate == 1 && glide_pending_deactivations == -1)

// ---- .bss (address order) -----------------------------------------------------------------------------
grCheckForRoom_t glide_grCheckForRoom;                                 // 0x2f0e9a8
guTexGetMipMapInfo_t glide_guTexGetMipMapInfo;                         // 0x2f0e9ac
HMODULE glide_dll_handle;                                              // 0x2f0e9b0
grDrawLine_t glide_grDrawLine;                                         // 0x2f0e9b4
grDrawPlanarPolygon_t glide_grDrawPlanarPolygon;                       // 0x2f0e9b8
grDrawPlanarPolygonVertexList_t glide_grDrawPlanarPolygonVertexList;   // 0x2f0e9bc
grDrawPoint_t glide_grDrawPoint;                                       // 0x2f0e9c0
grDrawPolygon_t glide_grDrawPolygon;                                   // 0x2f0e9c4
grDrawPolygonVertexList_t glide_grDrawPolygonVertexList;               // 0x2f0e9c8
grDrawTriangle_t glide_grDrawTriangle;                                 // 0x2f0e9cc
guDrawTriangleWithClip_t glide_guDrawTriangleWithClip;                 // 0x2f0e9d0
grBufferClear_t glide_grBufferClear;                                   // 0x2f0e9d4
grBufferNumPending_t glide_grBufferNumPending;                         // 0x2f0e9d8
grBufferSwap_t glide_grBufferSwap;                                     // 0x2f0e9dc
grRenderBuffer_t glide_grRenderBuffer;                                 // 0x2f0e9e0
grErrorSetCallback_t glide_grErrorSetCallback;                         // 0x2f0e9e4
grSstIdle_t glide_grSstIdle;                                           // 0x2f0e9e8
grSstVideoLine_t glide_grSstVideoLine;                                 // 0x2f0e9ec
grSstVRetraceOn_t glide_grSstVRetraceOn;                               // 0x2f0e9f0
grSstIsBusy_t glide_grSstIsBusy;                                       // 0x2f0e9f4
grSstWinOpen_t glide_grSstWinOpen;                                     // 0x2f0e9f8
grSstWinClose_t glide_grSstWinClose;                                   // 0x2f0e9fc
grSstControl_t glide_grSstControl;                                     // 0x2f0ea00
grSstQueryHardware_t glide_grSstQueryHardware;                         // 0x2f0ea04
grSstQueryBoards_t glide_grSstQueryBoards;                             // 0x2f0ea08
grSstOrigin_t glide_grSstOrigin;                                       // 0x2f0ea0c
grSstSelect_t glide_grSstSelect;                                       // 0x2f0ea10
grSstScreenHeight_t glide_grSstScreenHeight;                           // 0x2f0ea14
grSstScreenWidth_t glide_grSstScreenWidth;                             // 0x2f0ea18
grSstStatus_t glide_grSstStatus;                                       // 0x2f0ea1c
grSstPerfStats_t glide_grSstPerfStats;                                 // 0x2f0ea20
grSstResetPerfStats_t glide_grSstResetPerfStats;                       // 0x2f0ea24
grResetTriStats_t glide_grResetTriStats;                               // 0x2f0ea28
grTriStats_t glide_grTriStats;                                         // 0x2f0ea2c
grAlphaBlendFunction_t glide_grAlphaBlendFunction;                     // 0x2f0ea30
grAlphaCombine_t glide_grAlphaCombine;                                 // 0x2f0ea34
grAlphaControlsITRGBLighting_t glide_grAlphaControlsITRGBLighting;     // 0x2f0ea38
grAlphaTestFunction_t glide_grAlphaTestFunction;                       // 0x2f0ea3c
grAlphaTestReferenceValue_t glide_grAlphaTestReferenceValue;           // 0x2f0ea40
grChromakeyMode_t glide_grChromakeyMode;                               // 0x2f0ea44
grChromakeyValue_t glide_grChromakeyValue;                             // 0x2f0ea48
grClipWindow_t glide_grClipWindow;                                     // 0x2f0ea4c
grColorCombine_t glide_grColorCombine;                                 // 0x2f0ea50
grColorMask_t glide_grColorMask;                                       // 0x2f0ea54
grCullMode_t glide_grCullMode;                                         // 0x2f0ea58
grConstantColorValue_t glide_grConstantColorValue;                     // 0x2f0ea5c
grConstantColorValue4_t glide_grConstantColorValue4;                   // 0x2f0ea60
grDepthBiasLevel_t glide_grDepthBiasLevel;                             // 0x2f0ea64
grDepthBufferFunction_t glide_grDepthBufferFunction;                   // 0x2f0ea68
grDepthBufferMode_t glide_grDepthBufferMode;                           // 0x2f0ea6c
grDepthMask_t glide_grDepthMask;                                       // 0x2f0ea70
grDisableAllEffects_t glide_grDisableAllEffects;                       // 0x2f0ea74
grDitherMode_t glide_grDitherMode;                                     // 0x2f0ea78
grFogColorValue_t glide_grFogColorValue;                               // 0x2f0ea7c
grFogMode_t glide_grFogMode;                                           // 0x2f0ea80
grFogTable_t glide_grFogTable;                                         // 0x2f0ea84
grGammaCorrectionValue_t glide_grGammaCorrectionValue;                 // 0x2f0ea88
grSplash_t glide_grSplash;                                             // 0x2f0ea8c
grTexCalcMemRequired_t glide_grTexCalcMemRequired;                     // 0x2f0ea90
grTexTextureMemRequired_t glide_grTexTextureMemRequired;               // 0x2f0ea94
grTexMinAddress_t glide_grTexMinAddress;                               // 0x2f0ea98
grTexMaxAddress_t glide_grTexMaxAddress;                               // 0x2f0ea9c
grTexNCCTable_t glide_grTexNCCTable;                                   // 0x2f0eaa0
grTexSource_t glide_grTexSource;                                       // 0x2f0eaa4
grTexClampMode_t glide_grTexClampMode;                                 // 0x2f0eaa8
grTexCombine_t glide_grTexCombine;                                     // 0x2f0eaac
grTexCombineFunction_t glide_grTexCombineFunction;                     // 0x2f0eab0
grTexDetailControl_t glide_grTexDetailControl;                         // 0x2f0eab4
grTexFilterMode_t glide_grTexFilterMode;                               // 0x2f0eab8
grTexLodBiasValue_t glide_grTexLodBiasValue;                           // 0x2f0eabc
grTexDownloadMipMap_t glide_grTexDownloadMipMap;                       // 0x2f0eac0
grTexDownloadMipMapLevel_t glide_grTexDownloadMipMapLevel;             // 0x2f0eac4
grTexDownloadMipMapLevelPartial_t glide_grTexDownloadMipMapLevelPartial;   // 0x2f0eac8
ConvertAndDownloadRle_t glide_ConvertAndDownloadRle;                   // 0x2f0eacc
grTexDownloadTable_t glide_grTexDownloadTable;                         // 0x2f0ead0
grTexDownloadTablePartial_t glide_grTexDownloadTablePartial;           // 0x2f0ead4
grTexMipMapMode_t glide_grTexMipMapMode;                               // 0x2f0ead8
grTexMultibase_t glide_grTexMultibase;                                 // 0x2f0eadc
grTexMultibaseAddress_t glide_grTexMultibaseAddress;                   // 0x2f0eae0
guTexAllocateMemory_t glide_guTexAllocateMemory;                       // 0x2f0eae4
guTexChangeAttributes_t glide_guTexChangeAttributes;                   // 0x2f0eae8
guTexCombineFunction_t glide_guTexCombineFunction;                     // 0x2f0eaec
guTexGetCurrentMipMap_t glide_guTexGetCurrentMipMap;                   // 0x2f0eaf0
guTexMemQueryAvail_t glide_guTexMemQueryAvail;                         // 0x2f0eaf4
guTexMemReset_t glide_guTexMemReset;                                   // 0x2f0eaf8
guTexDownloadMipMap_t glide_guTexDownloadMipMap;                       // 0x2f0eafc
guTexDownloadMipMapLevel_t glide_guTexDownloadMipMapLevel;             // 0x2f0eb00
guTexSource_t glide_guTexSource;                                       // 0x2f0eb04
guColorCombineFunction_t glide_guColorCombineFunction;                 // 0x2f0eb08
grLfbLock_t glide_grLfbLock;                                           // 0x2f0eb0c
grLfbUnlock_t glide_grLfbUnlock;                                       // 0x2f0eb10
grLfbConstantAlpha_t glide_grLfbConstantAlpha;                         // 0x2f0eb14
grLfbConstantDepth_t glide_grLfbConstantDepth;                         // 0x2f0eb18
grLfbWriteColorSwizzle_t glide_grLfbWriteColorSwizzle;                 // 0x2f0eb1c
grLfbWriteColorFormat_t glide_grLfbWriteColorFormat;                   // 0x2f0eb20
grLfbWriteRegion_t glide_grLfbWriteRegion;                             // 0x2f0eb24
grLfbReadRegion_t glide_grLfbReadRegion;                               // 0x2f0eb28
grAADrawLine_t glide_grAADrawLine;                                     // 0x2f0eb2c
grAADrawPoint_t glide_grAADrawPoint;                                   // 0x2f0eb30
grAADrawPolygon_t glide_grAADrawPolygon;                               // 0x2f0eb34
grAADrawPolygonVertexList_t glide_grAADrawPolygonVertexList;           // 0x2f0eb38
grAADrawTriangle_t glide_grAADrawTriangle;                             // 0x2f0eb3c
grGlideInit_t glide_grGlideInit;                                       // 0x2f0eb40
grGlideShutdown_t glide_grGlideShutdown;                               // 0x2f0eb44
grGlideGetVersion_t glide_grGlideGetVersion;                           // 0x2f0eb48
grGlideGetState_t glide_grGlideGetState;                               // 0x2f0eb4c
grGlideSetState_t glide_grGlideSetState;                               // 0x2f0eb50
grGlideShamelessPlug_t glide_grGlideShamelessPlug;                     // 0x2f0eb54
grHints_t glide_grHints;                                               // 0x2f0eb58
guFogTableIndexToW_t glide_guFogTableIndexToW;                         // 0x2f0eb5c
guFogGenerateExp_t glide_guFogGenerateExp;                             // 0x2f0eb60
guFogGenerateExp2_t glide_guFogGenerateExp2;                           // 0x2f0eb64
guFogGenerateLinear_t glide_guFogGenerateLinear;                       // 0x2f0eb68
// glide_tmus (0x2f0eb70) is defined below with its initialiser.
glide_tcache_entry *glide_tcache_entries;             // 0x2f0ec00
int glide_texture_mem_used;                           // 0x2f0ec04
GrVertex glide_poly_vertices[GLIDE_MAX_POLY_VERTS];   // 0x2f0ec08 (D2501)
// gr_glide_current_mode (0x2f0f1e4) and ccmd_whee (0x2f0f1e8, 0xc bytes) are defined below with their
// initialisers; 0x2f0f1f4 has no reference.
RECT glide_cursor_clip_rect;                          // 0x2f0f1f8
int gr_glide_video_memory_mb;                         // 0x2f0f208
// (0x2f0f20c has no reference.)
GrHwConfiguration glide_hwconfig;                     // 0x2f0f210 (0x94 bytes)
ubyte glide_unused_2f0f2a4[0x4000];                   // 0x2f0f2a4 no reference (16 KB up to 0x2f132a3; name ours, D2504)
int glide_initialized;                                // 0x2f132a4
int glide_init_done;                                  // 0x2f132a8 (name ours)
volatile int glide_pending_activations;               // 0x2f132ac (volatile, D6001)
volatile int glide_pending_deactivations;             // 0x2f132b0 (name ours; volatile, D6001)
int glide_swap_enabled;                               // 0x2f132b4 (name ours)
bool glide_close_on_deactivate;                       // 0x2f132b8 (name ours)
float gr_glide_texture_scale_u;                       // 0x2f132bc
float gr_glide_texture_scale_v;                       // 0x2f132c0
float glide_texture_scale2_u;                         // 0x2f132c4 (name ours)
float glide_texture_scale2_v;                         // 0x2f132c8 (name ours)
// (0x2f132cc has no reference.)
bool glide_single_tmu;                                // 0x2f132cd

// ---- the DLL ------------------------------------------------------------------------------------------

// 0x547390  Confidence: exact.
void glide_unload_dll()
{
    if (glide_dll_handle) {
        FreeLibrary(glide_dll_handle);
        glide_dll_handle = NULL;
    }
}

// 0x5473b0  Confidence: exact. The order of the GetProcAddress calls is the order of the stores; the
// addresses of grCheckForRoom (0x2f0e9a8) and guTexGetMipMapInfo (0x2f0e9ac) are out of line with it.
int glide_load_dll()
{
    glide_dll_handle = LoadLibrary("glide2x.dll");
    if (!glide_dll_handle)
        return 0;
    glide_grDrawLine = (grDrawLine_t)GetProcAddress(glide_dll_handle, "_grDrawLine@8");
    glide_grDrawPlanarPolygon = (grDrawPlanarPolygon_t)GetProcAddress(glide_dll_handle, "_grDrawPlanarPolygon@12");
    glide_grDrawPlanarPolygonVertexList =
        (grDrawPlanarPolygonVertexList_t)GetProcAddress(glide_dll_handle, "_grDrawPlanarPolygonVertexList@8");
    glide_grDrawPoint = (grDrawPoint_t)GetProcAddress(glide_dll_handle, "_grDrawPoint@4");
    glide_grDrawPolygon = (grDrawPolygon_t)GetProcAddress(glide_dll_handle, "_grDrawPolygon@12");
    glide_grDrawPolygonVertexList =
        (grDrawPolygonVertexList_t)GetProcAddress(glide_dll_handle, "_grDrawPolygonVertexList@8");
    glide_grDrawTriangle = (grDrawTriangle_t)GetProcAddress(glide_dll_handle, "_grDrawTriangle@12");
    glide_guDrawTriangleWithClip =
        (guDrawTriangleWithClip_t)GetProcAddress(glide_dll_handle, "_guDrawTriangleWithClip@12");
    glide_grBufferClear = (grBufferClear_t)GetProcAddress(glide_dll_handle, "_grBufferClear@12");
    glide_grBufferNumPending = (grBufferNumPending_t)GetProcAddress(glide_dll_handle, "_grBufferNumPending@0");
    glide_grBufferSwap = (grBufferSwap_t)GetProcAddress(glide_dll_handle, "_grBufferSwap@4");
    glide_grRenderBuffer = (grRenderBuffer_t)GetProcAddress(glide_dll_handle, "_grRenderBuffer@4");
    glide_grErrorSetCallback = (grErrorSetCallback_t)GetProcAddress(glide_dll_handle, "_grErrorSetCallback@4");
    glide_grSstIdle = (grSstIdle_t)GetProcAddress(glide_dll_handle, "_grSstIdle@0");
    glide_grSstVideoLine = (grSstVideoLine_t)GetProcAddress(glide_dll_handle, "_grSstVideoLine@0");
    glide_grSstVRetraceOn = (grSstVRetraceOn_t)GetProcAddress(glide_dll_handle, "_grSstVRetraceOn@0");
    glide_grSstIsBusy = (grSstIsBusy_t)GetProcAddress(glide_dll_handle, "_grSstIsBusy@0");
    glide_grSstWinOpen = (grSstWinOpen_t)GetProcAddress(glide_dll_handle, "_grSstWinOpen@28");
    glide_grSstWinClose = (grSstWinClose_t)GetProcAddress(glide_dll_handle, "_grSstWinClose@0");
    glide_grSstControl = (grSstControl_t)GetProcAddress(glide_dll_handle, "_grSstControl@4");
    glide_grSstQueryHardware = (grSstQueryHardware_t)GetProcAddress(glide_dll_handle, "_grSstQueryHardware@4");
    glide_grSstQueryBoards = (grSstQueryBoards_t)GetProcAddress(glide_dll_handle, "_grSstQueryBoards@4");
    glide_grSstOrigin = (grSstOrigin_t)GetProcAddress(glide_dll_handle, "_grSstOrigin@4");
    glide_grSstSelect = (grSstSelect_t)GetProcAddress(glide_dll_handle, "_grSstSelect@4");
    glide_grSstScreenHeight = (grSstScreenHeight_t)GetProcAddress(glide_dll_handle, "_grSstScreenHeight@0");
    glide_grSstScreenWidth = (grSstScreenWidth_t)GetProcAddress(glide_dll_handle, "_grSstScreenWidth@0");
    glide_grSstStatus = (grSstStatus_t)GetProcAddress(glide_dll_handle, "_grSstStatus@0");
    glide_grSstPerfStats = (grSstPerfStats_t)GetProcAddress(glide_dll_handle, "_grSstPerfStats@4");
    glide_grSstResetPerfStats = (grSstResetPerfStats_t)GetProcAddress(glide_dll_handle, "_grSstResetPerfStats@0");
    glide_grResetTriStats = (grResetTriStats_t)GetProcAddress(glide_dll_handle, "_grResetTriStats@0");
    glide_grTriStats = (grTriStats_t)GetProcAddress(glide_dll_handle, "_grTriStats@8");
    glide_grAlphaBlendFunction = (grAlphaBlendFunction_t)GetProcAddress(glide_dll_handle, "_grAlphaBlendFunction@16");
    glide_grAlphaCombine = (grAlphaCombine_t)GetProcAddress(glide_dll_handle, "_grAlphaCombine@20");
    glide_grAlphaControlsITRGBLighting =
        (grAlphaControlsITRGBLighting_t)GetProcAddress(glide_dll_handle, "_grAlphaControlsITRGBLighting@4");
    glide_grAlphaTestFunction = (grAlphaTestFunction_t)GetProcAddress(glide_dll_handle, "_grAlphaTestFunction@4");
    glide_grAlphaTestReferenceValue =
        (grAlphaTestReferenceValue_t)GetProcAddress(glide_dll_handle, "_grAlphaTestReferenceValue@4");
    glide_grChromakeyMode = (grChromakeyMode_t)GetProcAddress(glide_dll_handle, "_grChromakeyMode@4");
    glide_grChromakeyValue = (grChromakeyValue_t)GetProcAddress(glide_dll_handle, "_grChromakeyValue@4");
    glide_grClipWindow = (grClipWindow_t)GetProcAddress(glide_dll_handle, "_grClipWindow@16");
    glide_grColorCombine = (grColorCombine_t)GetProcAddress(glide_dll_handle, "_grColorCombine@20");
    glide_grColorMask = (grColorMask_t)GetProcAddress(glide_dll_handle, "_grColorMask@8");
    glide_grCullMode = (grCullMode_t)GetProcAddress(glide_dll_handle, "_grCullMode@4");
    glide_grConstantColorValue = (grConstantColorValue_t)GetProcAddress(glide_dll_handle, "_grConstantColorValue@4");
    glide_grConstantColorValue4 =
        (grConstantColorValue4_t)GetProcAddress(glide_dll_handle, "_grConstantColorValue4@16");
    glide_grDepthBiasLevel = (grDepthBiasLevel_t)GetProcAddress(glide_dll_handle, "_grDepthBiasLevel@4");
    glide_grDepthBufferFunction =
        (grDepthBufferFunction_t)GetProcAddress(glide_dll_handle, "_grDepthBufferFunction@4");
    glide_grDepthBufferMode = (grDepthBufferMode_t)GetProcAddress(glide_dll_handle, "_grDepthBufferMode@4");
    glide_grDepthMask = (grDepthMask_t)GetProcAddress(glide_dll_handle, "_grDepthMask@4");
    glide_grDisableAllEffects = (grDisableAllEffects_t)GetProcAddress(glide_dll_handle, "_grDisableAllEffects@0");
    glide_grDitherMode = (grDitherMode_t)GetProcAddress(glide_dll_handle, "_grDitherMode@4");
    glide_grFogColorValue = (grFogColorValue_t)GetProcAddress(glide_dll_handle, "_grFogColorValue@4");
    glide_grFogMode = (grFogMode_t)GetProcAddress(glide_dll_handle, "_grFogMode@4");
    glide_grFogTable = (grFogTable_t)GetProcAddress(glide_dll_handle, "_grFogTable@4");
    glide_grGammaCorrectionValue =
        (grGammaCorrectionValue_t)GetProcAddress(glide_dll_handle, "_grGammaCorrectionValue@4");
    glide_grSplash = (grSplash_t)GetProcAddress(glide_dll_handle, "_grSplash@20");
    glide_grTexCalcMemRequired = (grTexCalcMemRequired_t)GetProcAddress(glide_dll_handle, "_grTexCalcMemRequired@16");
    glide_grTexTextureMemRequired =
        (grTexTextureMemRequired_t)GetProcAddress(glide_dll_handle, "_grTexTextureMemRequired@8");
    glide_grTexMinAddress = (grTexMinAddress_t)GetProcAddress(glide_dll_handle, "_grTexMinAddress@4");
    glide_grTexMaxAddress = (grTexMaxAddress_t)GetProcAddress(glide_dll_handle, "_grTexMaxAddress@4");
    glide_grTexNCCTable = (grTexNCCTable_t)GetProcAddress(glide_dll_handle, "_grTexNCCTable@8");
    glide_grTexSource = (grTexSource_t)GetProcAddress(glide_dll_handle, "_grTexSource@16");
    glide_grTexClampMode = (grTexClampMode_t)GetProcAddress(glide_dll_handle, "_grTexClampMode@12");
    glide_grTexCombine = (grTexCombine_t)GetProcAddress(glide_dll_handle, "_grTexCombine@28");
    glide_grTexCombineFunction = (grTexCombineFunction_t)GetProcAddress(glide_dll_handle, "_grTexCombineFunction@8");
    glide_grTexDetailControl = (grTexDetailControl_t)GetProcAddress(glide_dll_handle, "_grTexDetailControl@16");
    glide_grTexFilterMode = (grTexFilterMode_t)GetProcAddress(glide_dll_handle, "_grTexFilterMode@12");
    glide_grTexLodBiasValue = (grTexLodBiasValue_t)GetProcAddress(glide_dll_handle, "_grTexLodBiasValue@8");
    glide_grTexDownloadMipMap = (grTexDownloadMipMap_t)GetProcAddress(glide_dll_handle, "_grTexDownloadMipMap@16");
    glide_grTexDownloadMipMapLevel =
        (grTexDownloadMipMapLevel_t)GetProcAddress(glide_dll_handle, "_grTexDownloadMipMapLevel@32");
    glide_grTexDownloadMipMapLevelPartial =
        (grTexDownloadMipMapLevelPartial_t)GetProcAddress(glide_dll_handle, "_grTexDownloadMipMapLevelPartial@40");
    glide_ConvertAndDownloadRle =
        (ConvertAndDownloadRle_t)GetProcAddress(glide_dll_handle, "_ConvertAndDownloadRle@64");
    glide_grCheckForRoom = (grCheckForRoom_t)GetProcAddress(glide_dll_handle, "_grCheckForRoom@4");
    glide_grTexDownloadTable = (grTexDownloadTable_t)GetProcAddress(glide_dll_handle, "_grTexDownloadTable@12");
    glide_grTexDownloadTablePartial =
        (grTexDownloadTablePartial_t)GetProcAddress(glide_dll_handle, "_grTexDownloadTablePartial@20");
    glide_grTexMipMapMode = (grTexMipMapMode_t)GetProcAddress(glide_dll_handle, "_grTexMipMapMode@12");
    glide_grTexMultibase = (grTexMultibase_t)GetProcAddress(glide_dll_handle, "_grTexMultibase@8");
    glide_grTexMultibaseAddress =
        (grTexMultibaseAddress_t)GetProcAddress(glide_dll_handle, "_grTexMultibaseAddress@20");
    glide_guTexAllocateMemory = (guTexAllocateMemory_t)GetProcAddress(glide_dll_handle, "_guTexAllocateMemory@60");
    glide_guTexChangeAttributes =
        (guTexChangeAttributes_t)GetProcAddress(glide_dll_handle, "_guTexChangeAttributes@48");
    glide_guTexCombineFunction = (guTexCombineFunction_t)GetProcAddress(glide_dll_handle, "_guTexCombineFunction@8");
    glide_guTexGetCurrentMipMap =
        (guTexGetCurrentMipMap_t)GetProcAddress(glide_dll_handle, "_guTexGetCurrentMipMap@4");
    glide_guTexGetMipMapInfo = (guTexGetMipMapInfo_t)GetProcAddress(glide_dll_handle, "_guTexGetMipMapInfo@4");
    glide_guTexMemQueryAvail = (guTexMemQueryAvail_t)GetProcAddress(glide_dll_handle, "_guTexMemQueryAvail@4");
    glide_guTexMemReset = (guTexMemReset_t)GetProcAddress(glide_dll_handle, "_guTexMemReset@0");
    glide_guTexDownloadMipMap = (guTexDownloadMipMap_t)GetProcAddress(glide_dll_handle, "_guTexDownloadMipMap@12");
    glide_guTexDownloadMipMapLevel =
        (guTexDownloadMipMapLevel_t)GetProcAddress(glide_dll_handle, "_guTexDownloadMipMapLevel@12");
    glide_guTexSource = (guTexSource_t)GetProcAddress(glide_dll_handle, "_guTexSource@4");
    glide_guColorCombineFunction =
        (guColorCombineFunction_t)GetProcAddress(glide_dll_handle, "_guColorCombineFunction@4");
    glide_grLfbLock = (grLfbLock_t)GetProcAddress(glide_dll_handle, "_grLfbLock@24");
    glide_grLfbUnlock = (grLfbUnlock_t)GetProcAddress(glide_dll_handle, "_grLfbUnlock@8");
    glide_grLfbConstantAlpha = (grLfbConstantAlpha_t)GetProcAddress(glide_dll_handle, "_grLfbConstantAlpha@4");
    glide_grLfbConstantDepth = (grLfbConstantDepth_t)GetProcAddress(glide_dll_handle, "_grLfbConstantDepth@4");
    glide_grLfbWriteColorSwizzle =
        (grLfbWriteColorSwizzle_t)GetProcAddress(glide_dll_handle, "_grLfbWriteColorSwizzle@8");
    glide_grLfbWriteColorFormat =
        (grLfbWriteColorFormat_t)GetProcAddress(glide_dll_handle, "_grLfbWriteColorFormat@4");
    glide_grLfbWriteRegion = (grLfbWriteRegion_t)GetProcAddress(glide_dll_handle, "_grLfbWriteRegion@32");
    glide_grLfbReadRegion = (grLfbReadRegion_t)GetProcAddress(glide_dll_handle, "_grLfbReadRegion@28");
    glide_grAADrawLine = (grAADrawLine_t)GetProcAddress(glide_dll_handle, "_grAADrawLine@8");
    glide_grAADrawPoint = (grAADrawPoint_t)GetProcAddress(glide_dll_handle, "_grAADrawPoint@4");
    glide_grAADrawPolygon = (grAADrawPolygon_t)GetProcAddress(glide_dll_handle, "_grAADrawPolygon@12");
    glide_grAADrawPolygonVertexList =
        (grAADrawPolygonVertexList_t)GetProcAddress(glide_dll_handle, "_grAADrawPolygonVertexList@8");
    glide_grAADrawTriangle = (grAADrawTriangle_t)GetProcAddress(glide_dll_handle, "_grAADrawTriangle@24");
    glide_grGlideInit = (grGlideInit_t)GetProcAddress(glide_dll_handle, "_grGlideInit@0");
    glide_grGlideShutdown = (grGlideShutdown_t)GetProcAddress(glide_dll_handle, "_grGlideShutdown@0");
    glide_grGlideGetVersion = (grGlideGetVersion_t)GetProcAddress(glide_dll_handle, "_grGlideGetVersion@4");
    glide_grGlideGetState = (grGlideGetState_t)GetProcAddress(glide_dll_handle, "_grGlideGetState@4");
    glide_grGlideSetState = (grGlideSetState_t)GetProcAddress(glide_dll_handle, "_grGlideSetState@4");
    glide_grGlideShamelessPlug =
        (grGlideShamelessPlug_t)GetProcAddress(glide_dll_handle, "_grGlideShamelessPlug@4");
    glide_grHints = (grHints_t)GetProcAddress(glide_dll_handle, "_grHints@8");
    glide_guFogTableIndexToW = (guFogTableIndexToW_t)GetProcAddress(glide_dll_handle, "_guFogTableIndexToW@4");
    glide_guFogGenerateExp = (guFogGenerateExp_t)GetProcAddress(glide_dll_handle, "_guFogGenerateExp@8");
    glide_guFogGenerateExp2 = (guFogGenerateExp2_t)GetProcAddress(glide_dll_handle, "_guFogGenerateExp2@8");
    glide_guFogGenerateLinear =
        (guFogGenerateLinear_t)GetProcAddress(glide_dll_handle, "_guFogGenerateLinear@12");
    return 1;
}

// ---- the texture cache --------------------------------------------------------------------------------

// 0x547990 init_glide_records_2f0eb70 (thunk `jmp 0x5479a0`, initialiser table 0x588a0c): compiler-generated.
// 0x5479a0 (body): `vector constructor iterator`(glide_tmus, 0x48, 2, glide_record::glide_record). Confidence: exact.
glide_record glide_tmus[2];                           // 0x2f0eb70

// 0x5479c0  Confidence: likely (the KB / MB value is kept on the FPU stack: a local float or the
// expression twice).
void gr_glide_tcache_init()
{
    int i;
    glide_tcache_entries = new glide_tcache_entry[MAX_BITMAPS];
    int mem = glide_tmus[0].tmu_init(GR_TMU0, true);
    mem += glide_tmus[1].tmu_init(GR_TMU1, false);
    if (mem < 999) {
        sprintf(string_buffer, "Total texture memory on 3dfx card = %d bytes\n", mem);
    } else {
        float kb = mem / 1024.0f;
        if (kb < 999.99f)
            sprintf(string_buffer, "Total texture memory on 3dfx card = %.2f KB\n", kb);
        else
            sprintf(string_buffer, "Total texture memory on 3dfx card = %.2f MB\n", kb / 1024.0f);
    }
    for (i = 0; i < MAX_BITMAPS; i++) {
        glide_tcache_entries[i].bitmap = -1;
        glide_tcache_entries[i].num_sections = 0;
        glide_tcache_entries[i].sections = NULL;
    }
    glide_texture_mem_used = 0;
}

// 0x547a90  Confidence: likely (the empty-entry branch at the top; the four per-TMU stores written out).
void gr_glide_tcache_free_entry(glide_tcache_entry *entry, bool release_tmus, bool free_sections)
{
    int i;
    if (entry->num_sections == 0) {
        entry->num_sections = 0;
        entry->sections = NULL;
        return;
    }
    if (release_tmus) {
        for (i = 0; i < 2; i++)
            glide_tmus[i].free_texture_thunk(entry - glide_tcache_entries);
    }
    if (free_sections) {
        for (i = 0; i < entry->num_sections; i++) {
            if (entry->sections[i].data) {
                delete entry->sections[i].data;
                entry->sections[i].data = NULL;
                glide_texture_mem_used -= entry->sections[i].size;
                entry->sections[i].size = 0;
            }
        }
        entry->num_sections = 0;
        delete entry->sections;
        entry->sections = NULL;
    } else {
        for (i = 0; i < entry->num_sections; i++) {
            entry->sections[i].tmu_addr[0] = 0;
            entry->sections[i].tmu_size[0] = 0;
            entry->sections[i].tmu_addr[1] = 0;
            entry->sections[i].tmu_size[1] = 0;
        }
    }
}

// 0x547b60  Confidence: exact.
void gr_glide_tcache_close()
{
    int i;
    gr_glide_tcache_flush();
    for (i = 0; i < 2; i++)
        glide_tmus[i].close_thunk();
    delete glide_tcache_entries;
    glide_tcache_entries = NULL;
}

// 0x547ba0  Confidence: exact.
void gr_glide_tcache_flush()
{
    int i;
    if (glide_grSstIsBusy())
        glide_grSstIdle();
    for (i = 0; i < MAX_BITMAPS; i++)
        gr_glide_tcache_free_entry(&glide_tcache_entries[i], false, false);
    for (i = 0; i < 2; i++)
        glide_tmus[i].reset_blocks_thunk();
}

// 0x547bf0  Confidence: exact. Empty (name ours, like gr_d3d_tcache_frame); gr_glide_flip jumps to it.
void gr_glide_tcache_frame()
{
}

// 0x547c00  Confidence: exact. Glide's sizes are 1 .. 256, but this accepts only 4 .. 256.
bool gr_glide_round_texture_size(int w, int h)
{
    int tw = GLIDE_TEXTURE_SIZE(w);
    int th = GLIDE_TEXTURE_SIZE(h);
    if (tw == w && th == h)
        return true;
    return false;
}

// 0x547cc0  Confidence: likely (if-else chain of the formats; the LOD and aspect switches are inline in
// the binary, two switches with shared tails). A mipmapped bitmap is one section with all its levels.
int gr_glide_create_mipmapped_texture(int bm, glide_tcache_entry *entry)
{
    int num_pixels;
    ubyte *buf;
    int size;
    ubyte *data;
    GrTextureFormat_t format;                         // left unset for an unknown bitmap format (as in the binary)
    int w, h;
    ubyte *palette;
    int num_levels;
    ushort pal16[256];
    int i;

    bm_get_mipmap_info(bm, &w, &h, &num_pixels, &num_levels);
    if (!gr_glide_round_texture_size(w, h)) {
        sprintf(string_buffer, "Glide: Not drawing %dx%d bitmap '%s'\n", w, h, bm_get_filename(bm));
        return 0;
    }
    int bm_format = bm_lock(bm, &data, &palette);
    bm_bits_per_pixel(bm_format);                     // result unused
    buf = NULL;
    size = 0;
    if (bm_format == BM_FORMAT_8_ALPHA) {
        format = GR_TEXFMT_ALPHA_8;
        size = num_pixels;
        buf = new ubyte[size];
        for (i = 0; i < num_pixels; i++)
            buf[i] = (ubyte)gr_gamma_ramp[data[i]];
    } else if (bm_format == BM_FORMAT_8_PALETTED) {
        format = GR_TEXFMT_ARGB_1555;
        size = num_pixels * 2;
        buf = new ubyte[size];
        ubyte *p = palette;
        for (i = 0; i < 256; i++) {
            int r = p[0];
            int g = p[1];
            int b = p[2];
            int a = 1;
            if (r == 0 && g == 255 && b == 0) {      // pure green is transparent
                r = 0;
                g = 0;
                b = 0;
                a = 0;
            }
            pal16[i] = (ushort)((((((a << 5) | (r / 8)) << 5) | (g / 8)) << 5) | (b / 8));
            p += 3;
        }
        ushort *d = (ushort *)buf;
        for (i = 0; i < num_pixels; i++)
            d[i] = pal16[data[i]];
    } else if (bm_format == BM_FORMAT_888) {
        format = GR_TEXFMT_ARGB_1555;
        size = num_pixels * 2;
        buf = new ubyte[size];
        ushort *d = (ushort *)buf;
        for (i = 0; i < num_pixels; i++) {
            ubyte *s = &data[i * 3];
            int r, g, b, a;
            if (s[0] == 0 && s[1] == 0xff && s[2] == 0) {
                r = 0;
                g = 0;
                b = 0;
                a = 0;
            } else {
                r = s[2] >> 3;
                g = s[1] >> 3;
                b = s[0] >> 3;
                a = 1;
            }
            *d++ = (ushort)((((((a << 5) | r) << 5) | g) << 5) | b);
        }
    } else if (bm_format == BM_FORMAT_1555) {
        format = GR_TEXFMT_ARGB_1555;
        size = num_pixels * 2;
        buf = new ubyte[size];
        for (i = 0; i < num_pixels; i++)
            ((ushort *)buf)[i] = ((ushort *)data)[i] ^ 0x8000;   // the bitmaps' top bit means transparent
    } else if (bm_format == BM_FORMAT_565) {
        format = GR_TEXFMT_RGB_565;
        size = num_pixels * 2;
        buf = new ubyte[size];
        for (i = 0; i < num_pixels; i++)
            ((ushort *)buf)[i] = ((ushort *)data)[i];
    } else if (bm_format == BM_FORMAT_4444) {
        format = GR_TEXFMT_ARGB_4444;
        size = num_pixels * 2;
        buf = new ubyte[size];
        for (i = 0; i < num_pixels; i++)
            ((ushort *)buf)[i] = ((ushort *)data)[i];
    } else if (bm_format == BM_FORMAT_8888) {
        format = GR_TEXFMT_ARGB_4444;
        size = num_pixels * 2;
        buf = new ubyte[size];
        ushort *d = (ushort *)buf;
        for (i = 0; i < num_pixels; i++) {
            ubyte *s = &data[i * 4];
            *d++ = (ushort)((((((s[3] >> 4) << 4) | (s[2] >> 4)) << 4 | (s[1] >> 4)) << 4) | (s[0] >> 4));
        }
    }
    bm_unlock(bm);

    GrLOD_t large_lod = GR_LOD_16, small_lod = GR_LOD_16;
    int max_dim = imax(w, h);
    switch (max_dim) {
    case 256: large_lod = GR_LOD_256; break;
    case 128: large_lod = GR_LOD_128; break;
    case 64:  large_lod = GR_LOD_64; break;
    case 32:  large_lod = GR_LOD_32; break;
    case 16:  large_lod = GR_LOD_16; break;
    case 8:   large_lod = GR_LOD_8; break;
    case 4:   large_lod = GR_LOD_4; break;
    case 2:   large_lod = GR_LOD_2; break;
    case 1:   large_lod = GR_LOD_1; break;
    }
    switch (max_dim >> (num_levels - 1)) {
    case 256: small_lod = GR_LOD_256; break;
    case 128: small_lod = GR_LOD_128; break;
    case 64:  small_lod = GR_LOD_64; break;
    case 32:  small_lod = GR_LOD_32; break;
    case 16:  small_lod = GR_LOD_16; break;
    case 8:   small_lod = GR_LOD_8; break;
    case 4:   small_lod = GR_LOD_4; break;
    case 2:   small_lod = GR_LOD_2; break;
    case 1:   small_lod = GR_LOD_1; break;
    }
    GrAspectRatio_t aspect = GR_ASPECT_1x1;
    if (w < h) {
        switch (h / w) {
        case 1: aspect = GR_ASPECT_1x1; break;
        case 2: aspect = GR_ASPECT_1x2; break;
        case 4: aspect = GR_ASPECT_1x4; break;
        case 8: aspect = GR_ASPECT_1x8; break;
        }
    } else {
        switch (w / h) {
        case 1: aspect = GR_ASPECT_1x1; break;
        case 2: aspect = GR_ASPECT_2x1; break;
        case 4: aspect = GR_ASPECT_4x1; break;
        case 8: aspect = GR_ASPECT_8x1; break;
        }
    }

    glide_section *sec = entry->sections;
    sec->format = format;
    sec->large_lod = large_lod;
    sec->small_lod = small_lod;
    sec->aspect = aspect;
    sec->data = buf;
    sec->size = size;
    glide_texture_mem_used += size;
    sec->u_scale = (float)w * 256.0f / (float)max_dim;
    sec->v_scale = (float)h * 256.0f / (float)max_dim;
    for (i = 0; i < GLIDE_NUM_TMU; i++) {
        sec->tmu_addr[i] = 0;
        sec->tmu_size[i] = 0;
    }
    return 1;
}

// ---- .data (0x5ad500-0x5ad54f; between the literals of gr_glide_create_mipmapped_texture and
// gr_glide_init, so defined in this part of the source) ---------------------------------------------------
// The fog table starts as 59 zeros and 3, 0x42, 0x81, 0xc0, 0xff (an initialiser in the source, D2504);
// gr_glide_set_fog overwrites it before the first use.
GrFog_t glide_fog_table[GR_FOG_TABLE_SIZE] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0x42, 0x81, 0xc0, 0xff
};                                                    // 0x5ad500
int glide_app_active = 1;                             // 0x5ad540
float glide_whee = 0.1f;                              // 0x5ad544 console "whee"
float glide_fog_near = -1.0f;                         // 0x5ad548
float glide_fog_far = -1.0f;                          // 0x5ad54c

// 0x5484f0  Confidence: likely (the order of the per-format blocks follows the binary; the section's
// fields are read again after each conversion: entry->sections[section] each time, D2505).
// One <= 256 x 256 tile of a non-mipmapped bitmap into a Glide texture of power-of-two sides (aspect at
// most 8:1); the texels outside the tile are zero.
int gr_glide_create_texture_section(int bm, glide_tcache_entry *entry, int section, ubyte *data,
                                    ubyte *palette, int bm_w, int bm_h, int format)
{
    ushort pal16[256];
    int x, y;
    int u, v, u0;

    bm_bits_per_pixel(format);                        // result unused
    int sec_w = entry->sections[section].w;
    int sec_h = entry->sections[section].h;
    int tex_w = GLIDE_TEXTURE_SIZE(sec_w);
    int tex_h = GLIDE_TEXTURE_SIZE(sec_h);
    if (tex_h / tex_w > 8)
        tex_w = tex_h / 8;
    else if (tex_w / tex_h > 8)
        tex_h = tex_w / 8;
    int max_dim = max(tex_w, tex_h);
    float u_scale = (float)sec_w * 256.0f / (float)max_dim;
    float v_scale = (float)sec_h * 256.0f / (float)max_dim;
    int size = 0;
    ubyte *buf = NULL;
    GrTextureFormat_t gformat;                        // left unset for an unknown bitmap format (as in the binary)

    if (format == BM_FORMAT_8_ALPHA) {
        // The tile's position is not used: the bitmap's top left corner is copied (D2505).
        gformat = GR_TEXFMT_ALPHA_8;
        size = tex_h * tex_w;
        buf = new ubyte[size];
        ubyte *src = data;
        ubyte *row = buf;
        for (y = 0; y < tex_h; y++) {
            ubyte *d = row;
            for (x = 0; x < tex_w; x++) {
                if (y < bm_h && x < bm_w)
                    *d++ = (ubyte)gr_gamma_ramp[src[x]];
                else
                    *d++ = 0;
            }
            row += tex_w;
            src += bm_w;
        }
    } else if (format == BM_FORMAT_8_PALETTED) {
        gformat = GR_TEXFMT_ARGB_1555;
        size = tex_h * tex_w * 2;
        buf = new ubyte[size];
        ushort *pd = pal16;
        ubyte *p = palette;
        for (int i = 0; i < 256; i++) {
            int r = p[0];
            int g = p[1];
            int b = p[2];
            int a = 1;
            if (r == 0 && g == 255 && b == 0) {
                r = 0;
                g = 0;
                b = 0;
                a = 0;
            }
            *pd++ = (ushort)((((((a << 5) | (r / 8)) << 5) | (g / 8)) << 5) | (b / 8));
            p += 3;
        }
        sec_h = entry->sections[section].h;
        u0 = entry->sections[section].x << 16;
        v = entry->sections[section].y << 16;
        sec_w = entry->sections[section].w;
        ushort *row = (ushort *)buf;
        for (y = 0; y < sec_h; y++) {
            ushort *d = row;
            u = u0;
            for (x = 0; x < sec_w; x++) {
                *d++ = pal16[data[(v / GLIDE_STEP_ONE) * bm_w + u / GLIDE_STEP_ONE]];
                u += GLIDE_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += GLIDE_STEP_ONE;
            row += tex_w;
        }
        for (y = sec_h; y < tex_h; y++) {
            ushort *d = (ushort *)buf + y * tex_w;
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    } else if (format == BM_FORMAT_888) {
        gformat = GR_TEXFMT_ARGB_1555;
        size = tex_h * tex_w * 2;
        buf = new ubyte[size];
        u0 = entry->sections[section].x << 16;
        v = entry->sections[section].y << 16;
        sec_h = entry->sections[section].h;
        sec_w = entry->sections[section].w;
        ushort *row = (ushort *)buf;
        for (y = 0; y < sec_h; y++) {
            ushort *d = row;
            u = u0;
            for (x = 0; x < sec_w; x++) {
                ubyte *s = &data[((v / GLIDE_STEP_ONE) * bm_w + u / GLIDE_STEP_ONE) * 3];
                int r, g, b, a;
                if (s[0] == 0 && s[1] == 0xff && s[2] == 0) {
                    r = 0;
                    g = 0;
                    b = 0;
                    a = 0;
                } else {
                    r = s[2] >> 3;
                    g = s[1] >> 3;
                    b = s[0] >> 3;
                    a = 1;
                }
                *d++ = (ushort)((((((a << 5) | r) << 5) | g) << 5) | b);
                u += GLIDE_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += GLIDE_STEP_ONE;
            row += tex_w;
        }
        for (y = sec_h; y < tex_h; y++) {
            ushort *d = (ushort *)buf + y * tex_w;
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    } else if (format == BM_FORMAT_1555) {
        gformat = GR_TEXFMT_ARGB_1555;
        size = tex_h * tex_w * 2;
        buf = new ubyte[size];
        u0 = entry->sections[section].x << 16;
        v = entry->sections[section].y << 16;
        sec_h = entry->sections[section].h;
        sec_w = entry->sections[section].w;
        ushort *row = (ushort *)buf;
        for (y = 0; y < sec_h; y++) {
            ushort *d = row;
            u = u0;
            for (x = 0; x < sec_w; x++) {
                *d++ = ((ushort *)data)[(v / GLIDE_STEP_ONE) * bm_w + u / GLIDE_STEP_ONE] ^ 0x8000;
                u += GLIDE_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += GLIDE_STEP_ONE;
            row += tex_w;
        }
        for (y = sec_h; y < tex_h; y++) {
            ushort *d = (ushort *)buf + y * tex_w;
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    } else if (format == BM_FORMAT_565) {
        gformat = GR_TEXFMT_RGB_565;
        size = tex_h * tex_w * 2;
        buf = new ubyte[size];
        u0 = entry->sections[section].x << 16;
        v = entry->sections[section].y << 16;
        sec_h = entry->sections[section].h;
        sec_w = entry->sections[section].w;
        ushort *row = (ushort *)buf;
        for (y = 0; y < sec_h; y++) {
            ushort *d = row;
            u = u0;
            for (x = 0; x < sec_w; x++) {
                *d++ = ((ushort *)data)[(v / GLIDE_STEP_ONE) * bm_w + u / GLIDE_STEP_ONE];
                u += GLIDE_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += GLIDE_STEP_ONE;
            row += tex_w;
        }
        for (y = sec_h; y < tex_h; y++) {
            ushort *d = (ushort *)buf + y * tex_w;
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    } else if (format == BM_FORMAT_4444) {
        gformat = GR_TEXFMT_ARGB_4444;
        size = tex_h * tex_w * 2;
        buf = new ubyte[size];
        u0 = entry->sections[section].x << 16;
        v = entry->sections[section].y << 16;
        sec_h = entry->sections[section].h;
        sec_w = entry->sections[section].w;
        ushort *row = (ushort *)buf;
        for (y = 0; y < sec_h; y++) {
            ushort *d = row;
            u = u0;
            for (x = 0; x < sec_w; x++) {
                *d++ = ((ushort *)data)[(v / GLIDE_STEP_ONE) * bm_w + u / GLIDE_STEP_ONE];
                u += GLIDE_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += GLIDE_STEP_ONE;
            row += tex_w;
        }
        for (y = sec_h; y < tex_h; y++) {
            ushort *d = (ushort *)buf + y * tex_w;
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    } else if (format == BM_FORMAT_8888) {
        gformat = GR_TEXFMT_ARGB_4444;
        size = tex_h * tex_w * 2;
        buf = new ubyte[size];
        sec_h = entry->sections[section].h;
        u0 = entry->sections[section].x << 16;
        v = entry->sections[section].y << 16;
        sec_w = entry->sections[section].w;
        ushort *row = (ushort *)buf;
        for (y = 0; y < sec_h; y++) {
            ushort *d = row;
            u = u0;
            for (x = 0; x < sec_w; x++) {
                ubyte *s = &data[((v / GLIDE_STEP_ONE) * bm_w + u / GLIDE_STEP_ONE) * 4];
                *d++ = (ushort)((((((s[3] >> 4) << 4) | (s[2] >> 4)) << 4 | (s[1] >> 4)) << 4) | (s[0] >> 4));
                u += GLIDE_STEP_ONE;
            }
            for (; x < tex_w; x++)
                *d++ = 0;
            v += GLIDE_STEP_ONE;
            row += tex_w;
        }
        for (y = sec_h; y < tex_h; y++) {
            ushort *d = (ushort *)buf + y * tex_w;
            for (x = 0; x < tex_w; x++)
                *d++ = 0;
        }
    }

    GrLOD_t lod = GR_LOD_4;
    switch (max_dim) {
    case 4:   lod = GR_LOD_4; break;
    case 8:   lod = GR_LOD_8; break;
    case 16:  lod = GR_LOD_16; break;
    case 32:  lod = GR_LOD_32; break;
    case 64:  lod = GR_LOD_64; break;
    case 128: lod = GR_LOD_128; break;
    case 256: lod = GR_LOD_256; break;
    }
    GrAspectRatio_t aspect = GR_ASPECT_1x1;
    if (tex_w < tex_h) {
        switch (tex_h / tex_w) {
        case 1: aspect = GR_ASPECT_1x1; break;
        case 2: aspect = GR_ASPECT_1x2; break;
        case 4: aspect = GR_ASPECT_1x4; break;
        case 8: aspect = GR_ASPECT_1x8; break;
        }
    } else {
        switch (tex_w / tex_h) {
        case 1: aspect = GR_ASPECT_1x1; break;
        case 2: aspect = GR_ASPECT_2x1; break;
        case 4: aspect = GR_ASPECT_4x1; break;
        case 8: aspect = GR_ASPECT_8x1; break;
        }
    }

    glide_section *sec = &entry->sections[section];
    sec->format = gformat;
    sec->large_lod = lod;
    sec->small_lod = lod;
    sec->aspect = aspect;
    sec->data = buf;
    sec->size = size;
    glide_texture_mem_used += size;
    sec->u_scale = u_scale;
    sec->v_scale = v_scale;
    for (int i = 0; i < GLIDE_NUM_TMU; i++) {
        sec->tmu_addr[i] = 0;
        sec->tmu_size[i] = 0;
    }
    return 1;
}

// 0x5492f0  Confidence: likely (the 256 clamps as min(); the dirty flags cleared by one word store).
// Splits the bitmap into 256 x 256 sections (one section with all levels for a mipmapped bitmap), in
// the system memory pool.
int gr_glide_create_texture(int bm, glide_tcache_entry *entry)
{
    int w, h, num_pixels, num_levels;
    ubyte *data;
    ubyte *palette;

    Mempool *old_pool = get_current_mempool();
    set_current_mempool(NULL);
    gr_glide_tcache_free_entry(entry, true, true);
    bm_get_mipmap_info(bm, &w, &h, &num_pixels, &num_levels);
    entry->bitmap = bm;
    int nx = (w + 255) / 256;
    int ny = (h + 255) / 256;
    entry->num_sections = ny * nx;
    entry->sections = new glide_section[entry->num_sections];
    *(ushort *)entry->dirty = 0;
    if (num_levels > 1) {
        int ret = gr_glide_create_mipmapped_texture(bm, entry);
        set_current_mempool(old_pool);
        return ret;
    }
    int format = bm_lock(bm, &data, &palette);
    int section = 0;
    int y = 0;
    for (int row = 0; row < ny; row++) {
        int sh = min(256, h - y);
        int x = 0;
        for (int col = 0; col < nx; col++) {
            glide_section *sec = &entry->sections[section];
            sec->x = (ushort)x;
            sec->y = (ushort)y;
            sec->w = (ushort)min(256, w - x);
            sec->h = (ushort)sh;
            x += sec->w;
            gr_glide_create_texture_section(bm, entry, section, data, palette, w, h, format);
            section++;
        }
        y += sh;
    }
    bm_unlock(bm);
    set_current_mempool(old_pool);
    return 1;
}

// 0x5494e0  Confidence: exact.
int gr_glide_get_section_count(int bm)
{
    return glide_tcache_entries[bm_get_current_frame(bm)].num_sections;
}

// 0x549500  Confidence: exact.
void gr_glide_get_section_rect(int bm, int section, int *x, int *y, int *w, int *h)
{
    glide_section *sec = &glide_tcache_entries[bm_get_current_frame(bm)].sections[section];
    *x = sec->x;
    *y = sec->y;
    *w = sec->w;
    *h = sec->h;
}

// 0x549560  Confidence: likely (the reload switch: case order from the binary). Builds the texture when
// the entry does not hold this bitmap, copies the bitmap again when the TMU's dirty flag is set (never:
// nothing in the unit sets it, D2505) and, with load, downloads and selects the section on the TMU.
int gr_glide_tcache_set(int tmu, int bm, int section, float *u_scale, float *v_scale, bool load)
{
    ubyte *data;
    ubyte *palette;
    if (bm >= 0) {
        int frame = bm_get_current_frame(bm);
        glide_record *slot = &glide_tmus[tmu];
        if (!slot->is_bound(frame, section, u_scale, v_scale)) {
            glide_tcache_entry *entry = &glide_tcache_entries[frame];
            if (entry->num_sections < 1 || entry->bitmap != bm) {
                if (!gr_glide_create_texture(bm, entry))
                    return 0;
                entry->dirty[tmu] = 0;
            }
            if (entry->dirty[tmu]) {
                bm_lock(bm, &data, &palette);
                for (int i = 0; i < entry->num_sections; i++) {
                    glide_section *sec = &entry->sections[i];
                    switch (sec->format) {
                    case GR_TEXFMT_ARGB_1555: {
                        ushort *d = (ushort *)sec->data;
                        ushort *s = (ushort *)data;
                        for (int n = sec->size / 2; n > 0; n--)
                            *d++ = *s++ ^ 0x8000;
                        break;
                    }
                    case GR_TEXFMT_ALPHA_8:
                    case GR_TEXFMT_RGB_565:
                    case GR_TEXFMT_ARGB_4444:
                        memcpy(sec->data, data, sec->size);
                        break;
                    }
                }
                bm_unlock(bm);
            }
            if (load)
                slot->load_section(entry, section, u_scale, v_scale);
        }
    }
    return 1;
}

// 0x5496e0  Confidence: exact. Note: gr_glide_create_texture gets the frame index, not the handle.
void gr_glide_mark_texture_dirty(int bm)
{
    int frame = bm_get_current_frame(bm);
    glide_tcache_entry *entry = &glide_tcache_entries[frame];
    for (int i = 0; i < 2; i++)
        glide_tmus[i].invalidate_bitmap(frame);
    gr_glide_create_texture(frame, entry);
}

// 0x549730  Confidence: exact.
void gr_glide_tcache_free(int bm)
{
    int frame = bm_get_current_frame(bm);
    glide_tcache_entry *entry = &glide_tcache_entries[frame];
    for (int i = 0; i < 2; i++)
        glide_tmus[i].invalidate_bitmap(frame);
    gr_glide_tcache_free_entry(entry, true, true);
    entry->bitmap = -1;
}

// ---- glide_record: the TMU texture memory ---------------------------------------------------------------

// 0x549780  Confidence: exact. The tcache_slot fields are set here, no base constructor is called (D2502).
glide_record::glide_record()
{
    initialized = false;
    bitmap = -1;
    section = -1;
    u_scale = 0.0f;
    v_scale = 0.0f;
    blocks = NULL;
    num_blocks = 0;
    mem_used = 0;
}

// 0x5497a0  Confidence: exact (the two grTexLodBiasValue calls share their call in the binary).
// sum.symbols glide_tmu_init. Returns the TMU's free texture memory.
int glide_record::tmu_init(int tmu, bool mipmap)
{
    initialized = true;
    stage = tmu;
    mem_avail = glide_guTexMemQueryAvail(tmu);
    min_addr = glide_grTexMinAddress(stage);
    max_addr = glide_grTexMaxAddress(stage);
    next_addr = min_addr;
    if (mipmap) {
        glide_grTexMipMapMode(stage, GR_MIPMAP_NEAREST, FXFALSE);
        glide_grTexLodBiasValue(stage, 0.5f);
    } else {
        glide_grTexMipMapMode(stage, GR_MIPMAP_DISABLE, FXFALSE);
        glide_grTexLodBiasValue(stage, 0.5f);
    }
    init_blocks(min_addr, max_addr, MAX_BITMAPS);
    mem_used = 0;
    bitmap = -1;
    section = -1;
    return mem_avail;
}

// 0x549840  Confidence: exact.
void glide_record::init_blocks(FxU32 start, FxU32 end, int num)
{
    num_blocks = num;
    start_addr = start;
    end_addr = end;
    blocks = new glide_block[num];
    reset_blocks();
}

// 0x549880  Confidence: exact.
void glide_record::reset_blocks()
{
    int i;
    memset(blocks, 0, num_blocks * sizeof(glide_block));
    for (i = 0; i < num_blocks - 1; i++)
        blocks[i].next = i + 1;
    blocks[num_blocks - 1].next = -1;
    used_head = -1;
    used_tail = -1;
    free_head = 0;
    for (i = 0; i < num_blocks; i++)
        blocks[i].texture = -1;
    bitmap = -1;
    section = -1;
    mem_used = 0;
}

// 0x549900  Confidence: exact (an out-of-line wrapper in source order, not a COMDAT).
void glide_record::free_texture_thunk(int texture)
{
    free_texture(texture);
}

// 0x549910  Confidence: exact.
void glide_record::free_texture(int texture)
{
    for (int i = 0; i < num_blocks; i++) {
        if (blocks[i].texture == texture)
            evict_block(i);
    }
}

// 0x549950  Confidence: exact. The owner's section loses this TMU's copy; the block stays in its list.
void glide_record::evict_block(int index)
{
    glide_block *b = &blocks[index];
    if (b->texture > -1) {
        glide_tcache_entries[b->texture].sections[b->section].tmu_addr[stage] = 0;
        glide_tcache_entries[b->texture].sections[b->section].tmu_size[stage] = 0;
        blocks[index].texture = -1;
        blocks[index].section = -1;
        mem_used -= blocks[index].size;
    }
}

// 0x5499d0  Confidence: exact (`jmp close`: a wrapper in source order, tail call).
void glide_record::close_thunk()
{
    close();
}

// 0x5499e0  Confidence: exact.
void glide_record::close()
{
    if (blocks) {
        delete blocks;
        blocks = NULL;
    }
    num_blocks = 0;
}

// 0x549a10  Confidence: exact (`jmp reset_blocks`).
void glide_record::reset_blocks_thunk()
{
    reset_blocks();
}

// 0x549a20  Confidence: likely (the order of the stores to the slot at the end).
// Downloads the section to this TMU when it has no copy there (or the entry is dirty) and selects it.
void glide_record::load_section(glide_tcache_entry *entry, int section, float *u_scale, float *v_scale)
{
    glide_section *sec = &entry->sections[section];
    GrTexInfo info;
    info.smallLod = sec->small_lod;
    info.largeLod = sec->large_lod;
    info.aspectRatio = sec->aspect;
    info.format = sec->format;
    info.data = sec->data;
    if (sec->tmu_addr[stage] == 0) {
        int size = glide_grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &info);
        sec->tmu_addr[stage] = alloc(size, entry - glide_tcache_entries, section);
        sec->tmu_size[stage] = size;
        mem_used += size;
        glide_grTexDownloadMipMap(stage, sec->tmu_addr[stage], GR_MIPMAPLEVELMASK_BOTH, &info);
        entry->dirty[stage] = 0;
    } else if (entry->dirty[stage]) {
        glide_grTexDownloadMipMap(stage, sec->tmu_addr[stage], GR_MIPMAPLEVELMASK_BOTH, &info);
        entry->dirty[stage] = 0;
    }
    glide_grTexSource(stage, sec->tmu_addr[stage], GR_MIPMAPLEVELMASK_BOTH, &info);
    this->section = section;
    *u_scale = sec->u_scale;
    *v_scale = sec->v_scale;
    this->u_scale = sec->u_scale;
    bitmap = entry - glide_tcache_entries;
    this->v_scale = sec->v_scale;
}

// 0x549b50  Confidence: likely (the casts of the 2 MB bank compare: shr then a signed compare).
// Places a block after the last used one; a texture does not cross a 2 MB boundary. When the memory or
// the blocks run out it starts again at the bottom and evicts the oldest blocks it overlaps.
FxU32 glide_record::alloc(int size, int texture, int section)
{
    int index;
    glide_block *b;
    int head = used_head;
    if (head < 0) {
        index = new_block();
        b = &blocks[index];
        b->addr = start_addr;
    } else {
        FxU32 addr = blocks[used_tail].addr + blocks[used_tail].size;
        FxU32 end = addr + size;
        if (end < end_addr && (int)(end >> 21) > (int)(addr >> 21))
            addr = (end >> 21) << 21;
        if (addr + size <= end_addr && blocks[free_head].next >= 0) {
            index = new_block();
        } else {
            evict_block(head);
            index = used_head;
            addr = start_addr;
            used_tail = index;
        }
        int next = blocks[used_tail].next;
        while (next >= 0 && addr + size > blocks[next].addr) {
            int after = blocks[next].next;
            release_block(next);
            next = after;
        }
        b = &blocks[index];
        b->next = next;
        b->addr = addr;
    }
    b->size = size;
    b->texture = texture;
    b->section = section;
    return b->addr;
}

// 0x549c50  Confidence: exact.
void glide_record::release_block(int index)
{
    int next;

    evict_block(index);
    next = free_head;
    free_head = index;
    blocks[index].next = next;
}

// 0x549c80  Confidence: exact. Takes a free block and links it after the tail of the used list.
int glide_record::new_block()
{
    int index = free_head;
    glide_block *b = &blocks[index];
    free_head = b->next;
    if (used_tail < 0) {
        b->next = -1;
        used_head = index;
        used_tail = index;
    } else {
        glide_block *tail = &blocks[used_tail];
        b->next = tail->next;
        tail->next = index;
        used_tail = index;
    }
    return index;
}

// ---- the device ---------------------------------------------------------------------------------------

// 0x549cc0  Confidence: exact.
void gr_glide_close()
{
    if (glide_initialized) {
        glide_clip_cursor(0);
        gr_glide_tcache_close();
        glide_grGlideShutdown();
        glide_unload_dll();
        glide_initialized = 0;
    }
}

// 0x549cf0  Confidence: exact.
void glide_clip_cursor(int on)
{
    if (on)
        ClipCursor(&glide_cursor_clip_rect);
    else
        ClipCursor(NULL);
}

// 0x549d10  Confidence: likely (the switch over the window mode; the XSTR texts are the .data literals).
// Line numbers of the errors: 758, 762, 785, 813, 843.
void gr_glide_init(HWND hwnd)
{
    char version[80];
    switch (gr_screen.window_mode) {
    case 200:
    case 202:
        VSDK_ERROR(GR_GLIDE_CPP, 0x2f6, "Glide can only be used in fullscreen mode for now.\n");
    case 201:
        break;
    default:
        VSDK_ERROR(GR_GLIDE_CPP, 0x2fa, "Invalid window_mode passed to gr_direct3d_init\n");
    }
    glide_init_done = 0;
    SetEnvironmentVariable("FX_GLIDE_NO_SPLASH", "1");
    glide_initialized = 1;
    if (!glide_load_dll())
        VSDK_ERROR(GR_GLIDE_CPP, 0x311, XSTR("Glide DLL not found!\n", 945));
    Sleep(2000);
    if (glide_close_on_deactivate == 1)
        glide_clip_cursor(1);
    glide_grGlideGetVersion(version);
    sprintf(string_buffer, "Glide version: %s\n", version);
    glide_grGlideInit();
    if (!glide_grSstQueryHardware(&glide_hwconfig))
        VSDK_ERROR(GR_GLIDE_CPP, 0x32d, XSTR("No Glide hardware found!\n", 946));
    glide_grSstSelect(0);
    GrScreenResolution_t res = gr_screen.max_w == 800 ? GR_RESOLUTION_800x600
                             : (gr_screen.max_w == 1024 ? GR_RESOLUTION_1024x768 : GR_RESOLUTION_640x480);
    if (!glide_grSstWinOpen((FxU32)hwnd, res, GR_REFRESH_60Hz, GR_COLORFORMAT_ABGR, GR_ORIGIN_UPPER_LEFT, 2, 1))
        VSDK_ERROR(GR_GLIDE_CPP, 0x34b, XSTR("Glide SstrWinOpen failed!\n", 947));
    int mem = glide_hwconfig.SSTs[0].sstBoard.VoodooConfig.fbRam;
    if (mem < 4) {
        MessageBox(NULL, XSTR("Summoner needs at least 8 mb of video memory to run.\n", 944),
                   XSTR("Missing Features", 942), GLIDE_MB_FLAGS);
        exit(1);
    }
    int num_tmus = glide_hwconfig.SSTs[0].sstBoard.VoodooConfig.nTexelfx;
    if (num_tmus < 2)
        glide_single_tmu = true;
    if (num_tmus > 1 && mem >= 12)
        glide_close_on_deactivate = true;
    gr_glide_video_memory_mb = mem;
    gr_screen.max_bitmap_w = 256;
    gr_screen.max_bitmap_h = 256;
    gr_glide_tcache_init();
    glide_grGammaCorrectionValue(gr_gamma);
    os_add_msg_handler(glide_msg_handler);
    glide_cursor_clip_rect.left = 0;
    glide_cursor_clip_rect.top = 0;
    glide_cursor_clip_rect.right = gr_screen.max_w - 1;
    glide_cursor_clip_rect.bottom = gr_screen.max_h - 1;
    glide_init_done = 1;
    glide_swap_enabled = 1;
    glide_grHints(GR_HINT_STWHINT, GR_STWHINT_ST_DIFF_TMU1);
    glide_grAlphaTestFunction(GR_CMP_GREATER);
    glide_grAlphaTestReferenceValue(0);
}

// 0x549f50  Confidence: likely (the old value is read before the new one is stored).
// WM_ACTIVATE / WM_ACTIVATEAPP: counts the focus changes for gr_glide_flip and, on a Voodoo2, closes or
// reopens the Glide window.
void glide_msg_handler(UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_ACTIVATE || msg == WM_ACTIVATEAPP) {
        int old_active = glide_app_active;
        int active = wparam;
        if (msg == WM_ACTIVATE)
            active = (LOWORD(wparam) == WA_ACTIVE || LOWORD(wparam) == WA_CLICKACTIVE) ? 1 : 0;
        glide_app_active = active;
        if (old_active != active) {
            if (active)
                glide_pending_activations++;
            else
                glide_pending_deactivations++;
            if (glide_close_on_deactivate)
                glide_set_active(active);
        }
    }
}

// 0x549fc0  Confidence: exact.
void glide_set_active(int active)
{
    HWND hwnd = os_get_main_wnd();
    if (active == 1) {
        if (glide_pending_deactivations) {
            glide_pending_deactivations = 0;
            if (hwnd) {
                GrScreenResolution_t res = gr_screen.max_w == 800 ? GR_RESOLUTION_800x600
                    : (gr_screen.max_w == 1024 ? GR_RESOLUTION_1024x768 : GR_RESOLUTION_640x480);
                SetActiveWindow(hwnd);
                SetForegroundWindow(hwnd);
                glide_grSstWinOpen((FxU32)hwnd, res, GR_REFRESH_60Hz, GR_COLORFORMAT_ABGR, GR_ORIGIN_UPPER_LEFT, 2, 1);
                ShowWindow(hwnd, SW_MAXIMIZE);
                glide_clip_cursor(1);
                gr_glide_tcache_flush();
                glide_grGammaCorrectionValue(gr_gamma);
                glide_swap_enabled = 1;
            }
        }
    } else {
        if (glide_pending_deactivations != -1) {
            glide_pending_deactivations = -1;
            if (hwnd) {
                glide_clip_cursor(0);
                ShowWindow(hwnd, SW_MINIMIZE);
                glide_grSstWinClose();
                glide_swap_enabled = 0;
            }
        }
    }
}

// 0x54a0b0  Confidence: exact.
bool gr_glide_is_mode_supported(gr::Mode *mode)
{
    if (glide_single_tmu)
        return !mode->is_multitexture();
    return true;
}

// 0x54a0d0  Confidence: exact.
int gr_glide_get_video_memory_mb()
{
    return gr_glide_video_memory_mb;
}

// 0x54a0e0 init_gr_glide_current_mode (thunk `jmp 0x54a0f0`, initialiser table 0x588a10): compiler-generated.
// 0x54a0f0 (body): `mov ecx, 0x2f0f1e4; jmp gr::Mode::Mode`. Confidence: exact.
gr::Mode gr_glide_current_mode;                       // 0x2f0f1e4

// 0x54a100  Confidence: exact (case order of the texture switch from the binary: 0, 2, 3, 1, 4, 6; the
// fog switch has the store and return duplicated into each case).
// Applies the six fields of a gr::Mode that differ from the mode set last.
void gr_glide_set_mode(gr::Mode mode)
{
    if (GLIDE_WINDOW_CLOSED)
        return;
    if (mode == gr_glide_current_mode)
        return;
    int texture_source = mode.get_texture_source();
    int color_source = mode.get_color_source();
    int alpha_source = mode.get_alpha_source();
    int alpha_blend = mode.get_alpha_blend();
    int zbuffer_type = mode.get_zbuffer_type();
    int fog_type = mode.get_fog_type();

    if (gr_glide_current_mode.get_texture_source() != texture_source) {
        switch (texture_source) {
        case 0:                                       // none
            glide_grTexCombineFunction(GR_TMU0, GR_TEXTURECOMBINE_ONE);
            break;
        case 2:                                       // bilinear, clamped
            glide_grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
            glide_grTexCombineFunction(GR_TMU0, GR_TEXTURECOMBINE_DECAL);
            glide_grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);
            break;
        case 3:                                       // point sampled, clamped
            glide_grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_POINT_SAMPLED, GR_TEXTUREFILTER_POINT_SAMPLED);
            glide_grTexCombineFunction(GR_TMU0, GR_TEXTURECOMBINE_DECAL);
            glide_grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);
            break;
        case 1:                                       // bilinear, wrapped
            glide_grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
            glide_grTexCombineFunction(GR_TMU0, GR_TEXTURECOMBINE_DECAL);
            glide_grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
            break;
        case 4:                                       // TMU0 wrapped times TMU1 clamped (lightmaps)
            glide_grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
            glide_grTexCombineFunction(GR_TMU0, GR_TEXTURECOMBINE_MULTIPLY);
            glide_grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
            glide_grTexFilterMode(GR_TMU1, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
            glide_grTexCombineFunction(GR_TMU1, GR_TEXTURECOMBINE_DECAL);
            glide_grTexClampMode(GR_TMU1, GR_TEXTURECLAMP_CLAMP, GR_TEXTURECLAMP_CLAMP);
            break;
        case 5:
            break;
        case 6:                                       // TMU0 plus TMU1, both wrapped
            glide_grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
            glide_grTexCombineFunction(GR_TMU0, GR_TEXTURECOMBINE_ADD);
            glide_grTexClampMode(GR_TMU0, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
            glide_grTexFilterMode(GR_TMU1, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
            glide_grTexCombineFunction(GR_TMU1, GR_TEXTURECOMBINE_DECAL);
            glide_grTexClampMode(GR_TMU1, GR_TEXTURECLAMP_WRAP, GR_TEXTURECLAMP_WRAP);
            break;
        }
    }

    if (gr_glide_current_mode.get_color_source() != color_source) {
        switch (color_source) {
        case 0:                                       // vertex colour
            glide_grColorCombine(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ZERO, GR_COMBINE_LOCAL_ITERATED,
                                 GR_COMBINE_OTHER_CONSTANT, FXFALSE);
            break;
        case 1:                                       // texture
            glide_grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE, GR_COMBINE_LOCAL_CONSTANT,
                                 GR_COMBINE_OTHER_TEXTURE, FXFALSE);
            break;
        case 2:                                       // texture times vertex colour
            glide_grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED,
                                 GR_COMBINE_OTHER_TEXTURE, FXFALSE);
            break;
        case 3:                                       // texture plus vertex colour
            glide_grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL, GR_COMBINE_FACTOR_ONE,
                                 GR_COMBINE_LOCAL_ITERATED, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
            break;
        }
    }

    if (gr_glide_current_mode.get_alpha_source() != alpha_source) {
        switch (alpha_source) {
        case 0:
            glide_grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ZERO, GR_COMBINE_LOCAL_ITERATED,
                                 GR_COMBINE_OTHER_CONSTANT, FXFALSE);
            glide_grAlphaControlsITRGBLighting(FXFALSE);
            break;
        case 1:
            glide_grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ZERO, GR_COMBINE_LOCAL_ITERATED,
                                 GR_COMBINE_OTHER_CONSTANT, FXFALSE);
            glide_grAlphaControlsITRGBLighting(FXTRUE);
            glide_grConstantColorValue(0xffffffff);
            break;
        case 2:
            glide_grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_ONE, GR_COMBINE_LOCAL_CONSTANT,
                                 GR_COMBINE_OTHER_TEXTURE, FXFALSE);
            glide_grAlphaControlsITRGBLighting(FXFALSE);
            break;
        case 3:
            glide_grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED,
                                 GR_COMBINE_OTHER_TEXTURE, FXFALSE);
            glide_grAlphaControlsITRGBLighting(FXFALSE);
            break;
        }
    }

    if (gr_glide_current_mode.get_alpha_blend() != alpha_blend) {
        switch (alpha_blend) {
        case 0:                                       // opaque
            glide_grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ZERO, GR_BLEND_ZERO);
            break;
        case 1:                                       // additive
            glide_grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ZERO);
            break;
        case 2:                                       // alpha additive
            glide_grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE, GR_BLEND_ZERO, GR_BLEND_ZERO);
            break;
        case 3:                                       // alpha
            glide_grAlphaBlendFunction(GR_BLEND_SRC_ALPHA, GR_BLEND_ONE_MINUS_SRC_ALPHA, GR_BLEND_ZERO,
                                       GR_BLEND_ZERO);
            break;
        case 4:
            break;
        case 5:                                       // multiply (the pre-fog colour when fogged)
            if (gr_fog_enabled && fog_type != 3)
                glide_grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_PREFOG_COLOR, GR_BLEND_ONE, GR_BLEND_ZERO);
            else
                glide_grAlphaBlendFunction(GR_BLEND_DST_COLOR, GR_BLEND_ZERO, GR_BLEND_ONE, GR_BLEND_ZERO);
            break;
        }
    }

    if (gr_glide_current_mode.get_zbuffer_type() != zbuffer_type) {
        switch (zbuffer_type) {
        case 0:
            glide_grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
            glide_grDepthMask(FXFALSE);
            break;
        case 1:
            glide_grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
            glide_grDepthBufferFunction(GR_CMP_LEQUAL);
            glide_grDepthMask(FXFALSE);
            break;
        case 2:
            glide_grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
            glide_grDepthBufferFunction(GR_CMP_EQUAL);
            glide_grDepthMask(FXFALSE);
            break;
        case 3:
            glide_grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
            glide_grDepthBufferFunction(GR_CMP_ALWAYS);
            glide_grDepthMask(FXTRUE);
            break;
        case 4:
            glide_grDepthBufferMode(GR_DEPTHBUFFER_WBUFFER);
            glide_grDepthBufferFunction(GR_CMP_LEQUAL);
            glide_grDepthMask(FXTRUE);
            break;
        }
    }

    if (gr_glide_current_mode.get_fog_type() != fog_type) {
        switch (fog_type) {
        case 0:
            if (gr_fog_enabled)
                glide_grFogMode(GR_FOG_WITH_TABLE);
            else
                glide_grFogMode(GR_FOG_DISABLE);
            break;
        case 1:
            if (gr_fog_enabled)
                glide_grFogMode(GR_FOG_WITH_TABLE | GR_FOG_ADD2);
            else
                glide_grFogMode(GR_FOG_DISABLE);
            break;
        case 2:
            if (gr_fog_enabled)
                glide_grFogMode(GR_FOG_WITH_TABLE | GR_FOG_MULT2);
            else
                glide_grFogMode(GR_FOG_DISABLE);
            break;
        case 3:
            glide_grFogMode(GR_FOG_DISABLE);
            break;
        }
    }
    gr_glide_current_mode = mode;
}

// 0x54a540  Confidence: exact. Grey (r + g + b, not the average) times the tint, clamped to 0..255.
void gr_glide_apply_tint(GrVertex *v)
{
    float sum = v->r + v->g + v->b;
    v->r = gr_tint_r * sum;
    clamp_float(&v->r, 0.0f, 255.0f);
    v->g = gr_tint_g * sum;
    clamp_float(&v->g, 0.0f, 255.0f);
    v->b = gr_tint_b * sum;
    clamp_float(&v->b, 0.0f, 255.0f);
}

// 0x54a5b0  Confidence: exact.
void gr_glide_set_viewport()
{
    if (GLIDE_WINDOW_CLOSED)
        return;
    glide_grClipWindow(gr_screen.offset_x + gr_screen.clip_left, gr_screen.offset_y + gr_screen.clip_top,
                       gr_screen.clip_right + gr_screen.offset_x + 1,
                       gr_screen.clip_bottom + gr_screen.offset_y + 1);
}

// 0x54a600  Confidence: exact.
void gr_glide_clear()
{
    if (GLIDE_WINDOW_CLOSED)
        return;
    gr_glide_set_mode(gr::Mode(0, 0, 0, 0, 0, 0));
    glide_grBufferClear(0, 0, GR_WDEPTHVALUE_FARTHEST);
}

// 0x54a650  Confidence: exact.
void gr_glide_zbuffer_clear()
{
    if (GLIDE_WINDOW_CLOSED)
        return;
    gr_glide_set_mode(gr::Mode(0, 0, 0, 0, 4, 0));
    glide_grColorMask(FXFALSE, FXFALSE);
    glide_grBufferClear(0, 0, GR_WDEPTHVALUE_FARTHEST);
    glide_grColorMask(FXTRUE, FXFALSE);
}

// 0x54a6b0  Confidence: exact.
void gr_glide_deactivate()
{
    if (!gr_screen.dual_monitors) {
        glide_grSstControl(GR_CONTROL_DEACTIVATE);
        glide_pending_activations++;
    }
}

// 0x54a6d0  Confidence: exact (tail jump through the pointer).
void gr_glide_finish()
{
    glide_grSstIdle();
}

// 0x54a6e0  Confidence: exact (the counters are read twice: `n = count; count -= n`).
// Without the Voodoo2 window handling the focus changes are passed to grSstControl here, once a frame.
void gr_glide_flip()
{
    if (GLIDE_WINDOW_CLOSED)
        return;
    if (!glide_close_on_deactivate) {
        int n = glide_pending_activations;
        if (n) {
            glide_pending_activations -= n;
            if (!gr_screen.dual_monitors)
                glide_grSstControl(GR_CONTROL_ACTIVATE);
            glide_swap_enabled++;
        }
        n = glide_pending_deactivations;
        if (n) {
            glide_pending_deactivations -= n;
            if (!gr_screen.dual_monitors)
                glide_grSstControl(GR_CONTROL_DEACTIVATE);
            glide_swap_enabled--;
        }
    }
    if (glide_swap_enabled > 0) {
        if (gr_vsync)
            glide_grBufferSwap(1);
        else
            glide_grBufferSwap(0);
    }
    gr_glide_tcache_frame();
}

// 0x54a780  Confidence: exact. A non-zero flags value loads the bitmap into TMU 1.
void gr_glide_page_in(int bm, int flags)
{
    if (bm > -1)
        gr_glide_tcache_set(flags != 0, bm, 0, &gr_glide_texture_scale_u, &gr_glide_texture_scale_v, true);
}

// 0x54a7b0  Confidence: exact. No reference.
int gr_glide_stub_ret1()
{
    return 1;
}

// 0x54a7c0  Confidence: exact.
void gr_glide_set_bitmap(gr::Mode mode, int bm)
{
    gr_glide_set_mode(mode);
    if (mode.get_texture_source() != 0 && bm > -1)
        gr_glide_tcache_set(GR_TMU0, bm, 0, &gr_glide_texture_scale_u, &gr_glide_texture_scale_v, true);
}

// ---- drawing ------------------------------------------------------------------------------------------

// 0x54a800  Confidence: likely (the vertex fields are stored in a scheduled order; one quad for a
// one-section bitmap, one quad per section otherwise, with a half-texel inset when scaled).
void gr_glide_bitmap(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh, gr::Mode mode)
{
    GrVertex verts[4];
    int bw, bh;
    int sec_x, sec_y, sec_w, sec_h;
    int unscaled;

    if (GLIDE_WINDOW_CLOSED)
        return;
    unscaled = (w == sw && h == sh);
    if (w < 1 || h < 1)
        return;
    gr_glide_set_bitmap(mode, bm);
    float r = (float)gr_color.r;
    float g = (float)gr_color.g;
    float b = (float)gr_color.b;
    float a = (float)gr_color.a;
    int num_sections = gr_glide_get_section_count(bm);
    if (num_sections == 1) {
        bm_get_dimensions(bm, &bw, &bh);
        float u0 = (float)sx * gr_glide_texture_scale_u / (float)bw;
        float fbh = (float)bh;
        float v0 = (float)sy * gr_glide_texture_scale_v / fbh;
        float u1 = (float)(sx + sw) * gr_glide_texture_scale_u / (float)bw;
        float v1 = (float)(sy + sh) * gr_glide_texture_scale_v / fbh;
        int x0 = gr_screen.offset_x + x;
        int y0 = gr_screen.offset_y + y;
        float fx0 = (float)x0;
        float fy0 = (float)y0;
        float fx1 = (float)(x0 + w);
        float fy1 = (float)(y0 + h);

        verts[0].x = fx0;
        verts[0].y = fy0;
        verts[0].r = r;
        verts[0].g = g;
        verts[0].b = b;
        verts[0].a = a;
        verts[0].oow = 1.0f;
        verts[0].tmuvtx[0].sow = u0;
        verts[0].tmuvtx[0].tow = v0;
        if (gr_tint_enabled)
            gr_glide_apply_tint(&verts[0]);
        verts[1].x = fx1;
        verts[1].y = fy0;
        verts[1].r = r;
        verts[1].g = g;
        verts[1].b = b;
        verts[1].a = a;
        verts[1].oow = 1.0f;
        verts[1].tmuvtx[0].sow = u1;
        verts[1].tmuvtx[0].tow = v0;
        if (gr_tint_enabled)
            gr_glide_apply_tint(&verts[1]);
        verts[2].x = fx1;
        verts[2].y = fy1;
        verts[2].r = r;
        verts[2].g = g;
        verts[2].b = b;
        verts[2].a = a;
        verts[2].oow = 1.0f;
        verts[2].tmuvtx[0].sow = u1;
        verts[2].tmuvtx[0].tow = v1;
        if (gr_tint_enabled)
            gr_glide_apply_tint(&verts[2]);
        verts[3].x = fx0;
        verts[3].y = fy1;
        verts[3].r = r;
        verts[3].g = g;
        verts[3].b = b;
        verts[3].a = a;
        verts[3].oow = 1.0f;
        verts[3].tmuvtx[0].sow = u0;
        verts[3].tmuvtx[0].tow = v1;
        if (gr_tint_enabled)
            gr_glide_apply_tint(&verts[3]);
        glide_grDrawPolygonVertexList(4, verts);
        return;
    }

    float x_scale = (float)w / (float)sw;
    float y_scale = (float)h / (float)sh;
    for (int i = 0; i < num_sections; i++) {
        gr_glide_tcache_set(GR_TMU0, bm, i, &gr_glide_texture_scale_u, &gr_glide_texture_scale_v, true);
        gr_glide_get_section_rect(bm, i, &sec_x, &sec_y, &sec_w, &sec_h);
        float u0, v0;
        if (unscaled) {
            u0 = 0.0f;
            v0 = 0.0f;
        } else {
            u0 = 0.5f;
            v0 = 0.5f;
        }
        float u1 = gr_glide_texture_scale_u - u0;
        float v1 = gr_glide_texture_scale_v - v0;
        float ox = (float)(gr_screen.offset_x + x);
        float oy = (float)(gr_screen.offset_y + y);
        float fx0 = (float)(sec_x - sx) * x_scale + ox;
        float fy0 = (float)(sec_y - sy) * y_scale + oy;
        float fx1 = (float)(sec_x - sx + sec_w) * x_scale + ox;
        float fy1 = (float)(sec_y - sy + sec_h) * y_scale + oy;

        verts[0].x = fx0;
        verts[0].y = fy0;
        verts[0].r = r;
        verts[0].g = g;
        verts[0].b = b;
        verts[0].a = a;
        verts[0].oow = 1.0f;
        verts[0].tmuvtx[0].sow = u0;
        verts[0].tmuvtx[0].tow = v0;
        if (gr_tint_enabled)
            gr_glide_apply_tint(&verts[0]);
        verts[1].x = fx1;
        verts[1].y = fy0;
        verts[1].r = r;
        verts[1].g = g;
        verts[1].b = b;
        verts[1].a = a;
        verts[1].oow = 1.0f;
        verts[1].tmuvtx[0].sow = u1;
        verts[1].tmuvtx[0].tow = v0;
        if (gr_tint_enabled)
            gr_glide_apply_tint(&verts[1]);
        verts[2].x = fx1;
        verts[2].y = fy1;
        verts[2].r = r;
        verts[2].g = g;
        verts[2].b = b;
        verts[2].a = a;
        verts[2].oow = 1.0f;
        verts[2].tmuvtx[0].sow = u1;
        verts[2].tmuvtx[0].tow = v1;
        if (gr_tint_enabled)
            gr_glide_apply_tint(&verts[2]);
        verts[3].x = fx0;
        verts[3].y = fy1;
        verts[3].r = r;
        verts[3].g = g;
        verts[3].b = b;
        verts[3].a = a;
        verts[3].oow = 1.0f;
        verts[3].tmuvtx[0].sow = u0;
        verts[3].tmuvtx[0].tow = v1;
        if (gr_tint_enabled)
            gr_glide_apply_tint(&verts[3]);
        glide_grDrawPolygonVertexList(4, verts);
    }
}

// 0x54af10  Confidence: exact. The end points are truncated to pixels; the smaller coordinate of each
// axis gets +0.5, the larger +1.
void gr_glide_line(float x1, float y1, float x2, float y2, gr::Mode mode)
{
    GrVertex v1, v2;
    if (GLIDE_WINDOW_CLOSED)
        return;
    gr_glide_set_bitmap(mode, -1);
    float ox = (float)gr_screen.offset_x;
    v1.x = (float)(int)x1 + ox;
    float oy = (float)gr_screen.offset_y;
    v1.y = (float)(int)y1 + oy;
    float r = (float)gr_color.r;
    v1.r = r;
    float g = (float)gr_color.g;
    v1.g = g;
    float b = (float)gr_color.b;
    v1.b = b;
    float a = (float)gr_color.a;
    v1.a = a;
    v1.oow = 1.0f;
    v2.x = (float)(int)x2 + ox;
    v2.y = (float)(int)y2 + oy;
    v2.r = r;
    v2.g = g;
    v2.b = b;
    v2.a = a;
    v2.oow = 1.0f;
    if (gr_tint_enabled) {
        gr_glide_apply_tint(&v1);
        gr_glide_apply_tint(&v2);
    }
    if (v1.x < v2.x) {
        v1.x += 0.5f;
        v2.x += 1.0f;
    } else {
        v2.x += 0.5f;
        v1.x += 1.0f;
    }
    if (v1.y < v2.y) {
        v1.y += 0.5f;
        v2.y += 1.0f;
    } else {
        v2.y += 0.5f;
        v1.y += 1.0f;
    }
    glide_grDrawLine(&v1, &v2);
}

// 0x54b110  Confidence: exact. Projected vertices: screen x / y snapped, 1/w; no depth.
void gr_glide_line_3d(gr_vertex *v0, gr_vertex *v1, gr::Mode mode)
{
    GrVertex a, b;
    if (GLIDE_WINDOW_CLOSED)
        return;
    gr_glide_set_bitmap(mode, -1);
    a.x = v0->sx + GLIDE_SNAP;
    a.oow = v0->sw;
    a.y = v0->sy + GLIDE_SNAP;
    float r = (float)gr_color.r;
    a.r = r;
    float g = (float)gr_color.g;
    a.g = g;
    float bl = (float)gr_color.b;
    a.b = bl;
    float al = (float)gr_color.a;
    a.a = al;
    b.x = v1->sx + GLIDE_SNAP;
    b.oow = v1->sw;
    b.y = v1->sy + GLIDE_SNAP;
    b.r = r;
    b.g = g;
    b.b = bl;
    b.a = al;
    if (gr_tint_enabled) {
        gr_glide_apply_tint(&a);
        gr_glide_apply_tint(&b);
    }
    glide_grDrawLine(&a, &b);
}

// 0x54b240  Confidence: likely (the order of the uv products). The vertex flags (+0x1a): 8 own alpha,
// 4 own colour (through gr_color_gamma_lut), 1 uv. No check of nv against the 25 vertices.
void gr_glide_tmapper(int nv, gr_vertex **verts, gr::Mode mode)
{
    if (GLIDE_WINDOW_CLOSED)
        return;
    gr_glide_set_bitmap(mode, gr_texture);
    for (int i = 0; i < nv; i++) {
        GrVertex *gv = &glide_poly_vertices[i];
        ubyte flags = verts[i]->_unk1a;
        if (flags & 8)
            gv->a = (float)verts[i]->a;
        else
            gv->a = (float)gr_color.a;
        if (flags & 4) {
            gv->r = gr_color_gamma_lut[verts[i]->r];
            gv->g = gr_color_gamma_lut[verts[i]->g];
            gv->b = gr_color_gamma_lut[verts[i]->b];
        } else {
            gv->r = (float)gr_color.r;
            gv->g = (float)gr_color.g;
            gv->b = (float)gr_color.b;
        }
        if (gr_tint_enabled)
            gr_glide_apply_tint(gv);
        gv->x = verts[i]->sx + GLIDE_SNAP;
        gv->y = verts[i]->sy + GLIDE_SNAP;
        gv->oow = verts[i]->sw;
        if (verts[i]->_unk1a & 1) {
            gv->tmuvtx[0].sow = gv->oow * verts[i]->u * gr_glide_texture_scale_u;
            gv->tmuvtx[0].tow = gr_glide_texture_scale_v * verts[i]->v * gv->oow;
        }
    }
    glide_grDrawPolygonVertexList(nv, glide_poly_vertices);
}

// 0x54b3a0  Confidence: likely (the loops over the vertices strength-reduced to running offsets;
// vlist is filled and never read). One polygon per pass; a multitexture pass takes the next pass as the
// TMU 1 texture and skips it. Sectioned bitmaps use clamping instead of wrapping (texture source 1 -> 2).
void gr_glide_tmapper_multitexture(int nv, gr_multipass_vertex **verts, int npasses, gr_tmapper_pass *passes)
{
    gr_vertex *vlist[32];
    float u_offset, v_offset, u_scale, v_scale;
    int i, k;

    if (GLIDE_WINDOW_CLOSED)
        return;
    for (i = 0; i < nv; i++) {
        vlist[i] = verts[i]->v;
        glide_poly_vertices[i].x = vlist[i]->sx + GLIDE_SNAP;
        glide_poly_vertices[i].y = vlist[i]->sy + GLIDE_SNAP;
        glide_poly_vertices[i].oow = vlist[i]->sw;
    }
    gr_tmapper_pass *pass = passes;
    for (k = 0; k < npasses; k++, pass++) {
        gr_tmapper_pass *next = pass->mode.is_multitexture() ? &passes[k + 1] : NULL;

        if (!pass->vertex_alpha) {
            float a = (float)pass->a;
            for (i = 0; i < nv; i++)
                glide_poly_vertices[i].a = a;
        } else {
            for (i = 0; i < nv; i++)
                glide_poly_vertices[i].a = (float)verts[i]->pass[k].a;
        }
        if (!pass->vertex_color) {
            float r = (float)pass->r;
            float g = (float)pass->g;
            float b = (float)pass->b;
            for (i = 0; i < nv; i++) {
                glide_poly_vertices[i].r = r;
                glide_poly_vertices[i].g = g;
                glide_poly_vertices[i].b = b;
            }
        } else {
            for (i = 0; i < nv; i++) {
                glide_poly_vertices[i].r = gr_color_gamma_lut[verts[i]->pass[k].r];
                glide_poly_vertices[i].g = gr_color_gamma_lut[verts[i]->pass[k].g];
                glide_poly_vertices[i].b = gr_color_gamma_lut[verts[i]->pass[k].b];
            }
        }

        if (pass->bitmap > -1 && pass->enabled) {
            if (pass->sectioned) {
                gr::Mode mode = pass->mode;
                if (mode.get_texture_source() == 1)
                    mode = gr::Mode(2, mode.get_color_source(), mode.get_alpha_source(), mode.get_alpha_blend(),
                                    mode.get_zbuffer_type(), mode.get_fog_type());
                gr_glide_set_mode(mode);
                int section = gr_poly_get_section(nv, verts, k, pass->bitmap, &u_offset, &v_offset, &u_scale,
                                                  &v_scale);
                gr_glide_tcache_set(GR_TMU0, pass->bitmap, section, &gr_glide_texture_scale_u,
                                    &gr_glide_texture_scale_v, true);
                u_scale = gr_glide_texture_scale_u * u_scale;
                v_scale = gr_glide_texture_scale_v * v_scale;
                for (i = 0; i < nv; i++) {
                    glide_poly_vertices[i].tmuvtx[0].sow =
                        (verts[i]->pass[k].u - u_offset) * u_scale * glide_poly_vertices[i].oow;
                    glide_poly_vertices[i].tmuvtx[0].tow =
                        (verts[i]->pass[k].v - v_offset) * v_scale * glide_poly_vertices[i].oow;
                }
            } else {
                gr_glide_set_mode(pass->mode);
                gr_glide_tcache_set(pass->stage, pass->bitmap, 0, &gr_glide_texture_scale_u,
                                    &gr_glide_texture_scale_v, true);
                for (i = 0; i < nv; i++) {
                    glide_poly_vertices[i].tmuvtx[pass->stage].sow =
                        verts[i]->pass[k].u * glide_poly_vertices[i].oow * gr_glide_texture_scale_u;
                    glide_poly_vertices[i].tmuvtx[pass->stage].tow =
                        verts[i]->pass[k].v * glide_poly_vertices[i].oow * gr_glide_texture_scale_v;
                }
                if (next) {
                    gr_glide_tcache_set(next->stage, next->bitmap, 0, &glide_texture_scale2_u,
                                        &glide_texture_scale2_v, true);
                    for (i = 0; i < nv; i++) {
                        glide_poly_vertices[i].tmuvtx[next->stage].sow =
                            verts[i]->pass[k + 1].u * glide_poly_vertices[i].oow * glide_texture_scale2_u;
                        glide_poly_vertices[i].tmuvtx[next->stage].tow =
                            verts[i]->pass[k + 1].v * glide_poly_vertices[i].oow * glide_texture_scale2_v;
                    }
                }
            }
        } else {
            gr_glide_set_mode(pass->mode);
        }
        glide_grDrawPolygonVertexList(nv, glide_poly_vertices);
        if (next) {
            k++;
            pass++;
        }
    }
}

// 0x54b900  Confidence: exact. Hidden function (not in sum.symbols), no reference: the Glide
// counterpart of gr_d3d_set_zbias (name ours).
void gr_glide_set_zbias(int bias)
{
    if (GLIDE_WINDOW_CLOSED)
        return;
    glide_grDepthBiasLevel((FxI16)-bias);
}

// ---- console command "whee" ---------------------------------------------------------------------------
// 0x54b930 init_ccmd_whee_thunk (jmp, initialiser table 0x588a14): compiler-generated.  Confidence: exact.
// 0x54b940 init_ccmd_whee: the dynamic initialiser of this definition.  Confidence: exact.
console_command ccmd_whee("whee", "", cmd_whee);      // 0x2f0f1e8 (name ours)

// 0x54b960  Confidence: exact. Sets glide_whee, which nothing reads.
void cmd_whee()
{
    process_command(ARG_FLOAT);
    glide_whee = command_argument_float;
}

// 0x54b980  Confidence: exact. Returns the pixel format 3 (565).
int gr_glide_read_backbuffer(int x, int y, int w, int h, void *buf)
{
    glide_grLfbReadRegion(GR_BUFFER_BACKBUFFER, x, y, w, h, w * 2, buf);
    return 3;
}

// 0x54b9b0  Confidence: exact (the order of the two stores of the new range).
// Forces the next gr_glide_set_mode to set the fog mode again; rebuilds the fog table when the range
// changed.
void gr_glide_set_fog(int set_range)
{
    if (GLIDE_WINDOW_CLOSED)
        return;
    gr_glide_current_mode.invalidate();
    if (!gr_fog_enabled)
        return;
    if (set_range) {
        float fog_near = gr_scale_by_far_clip(gr_fog_near);
        float fog_far = gr_scale_by_far_clip(gr_fog_far);
        if (fog_near != glide_fog_near || fog_far != glide_fog_far) {
            glide_fog_near = fog_near;
            glide_fog_far = fog_far;
            glide_guFogGenerateLinear(glide_fog_table, fog_near, fog_far);
        }
        glide_grFogTable(glide_fog_table);
    }
    glide_grFogColorValue((((gr_fog_color.b << 8) | gr_fog_color.g) << 8) | gr_fog_color.r);
}

// 0x54ba90  Confidence: exact. Empty.
void gr_glide_set_antialias(bool on)
{
}

// 0x54baa0  Confidence: exact.
void gr_glide_set_alpha_test(bool on)
{
    if (on)
        glide_grAlphaTestFunction(GR_CMP_GREATER);
    else
        glide_grAlphaTestFunction(GR_CMP_ALWAYS);
}

// 0x54bac0  Confidence: exact.
void gr_glide_set_gamma()
{
    glide_grGammaCorrectionValue(gr_gamma);
}

// 0x54bad0  Confidence: exact. Copies the front buffer into the locked back buffer.
void gr_glide_flush_pipeline()
{
    GrLfbInfo_t info;
    info.size = sizeof(info);
    if (glide_grLfbLock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER, GR_LFBWRITEMODE_ANY, GR_ORIGIN_UPPER_LEFT, FXFALSE,
                        &info)) {
        glide_grLfbReadRegion(GR_BUFFER_FRONTBUFFER, 0, 0, gr_screen.max_w, gr_screen.max_h, info.strideInBytes,
                              info.lfbPtr);
        glide_grLfbUnlock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER);
    }
}
