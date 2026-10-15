// vsdk/gr/glide/gr_glide.h - the 3dfx Glide back end (vsdk\gr\glide\gr_glide.cpp, code 0x547390-0x54bb27;
// renderer 0x67). decomp40 declared the entry points gr.cpp dispatches to (signatures from gr.cpp's
// pushes); decomp45 wrote the unit and added the rest: the Glide 2 entry point pointers, the TMU memory
// records, the texture cache records and the calls the not-yet-written Glide paths of other units make
// (levelrender_draw_list_glide 0x48e6c0, gr_glide_render_character_mesh 0x50b4c0). Not original source.
#ifndef VSDK_GR_GLIDE_GR_GLIDE_H
#define VSDK_GR_GLIDE_GR_GLIDE_H

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/glide/glide_min.h"

// ---- texture cache records (D2505) -------------------------------------------------------------------
// One section of a bitmap: Glide textures are at most 256 x 256, so gr_glide_create_texture splits a
// bitmap into 256-texel tiles (num_sections = ceil(w / 256) * ceil(h / 256)). 0x38 bytes, layout from
// gr_glide_create_texture_section / glide_record::load_section / gr_glide_get_section_rect; names ours.
struct glide_section {
    GrLOD_t large_lod;                   // +0x00 (GrTexInfo.largeLod)
    GrLOD_t small_lod;                   // +0x04 (GrTexInfo.smallLod)
    GrAspectRatio_t aspect;              // +0x08
    GrTextureFormat_t format;            // +0x0c GR_TEXFMT_*
    FxU32 tmu_addr[GLIDE_NUM_TMU];       // +0x10 texture memory address per TMU, 0 = not downloaded
    int tmu_size[GLIDE_NUM_TMU];         // +0x18 bytes downloaded per TMU
    ubyte *data;                         // +0x20 converted texels (operator new)
    int size;                            // +0x24 bytes of data (counted in glide_texture_mem_used)
    float u_scale;                       // +0x28 section width * 256 / the larger texture side
    float v_scale;                       // +0x2c
    ushort x, y, w, h;                   // +0x30 the section's rectangle in the bitmap
};
LAYOUT_CHECK(glide_section, sizeof(glide_section) == 0x38);

// One entry per bitmap frame (MAX_BITMAPS of them in glide_tcache_entries, indexed by
// bm_get_current_frame). 0x10 bytes; names ours.
struct glide_tcache_entry {
    int bitmap;                          // +0x00 the bitmap handle it was made from, -1 free
    int num_sections;                    // +0x04
    ubyte dirty[GLIDE_NUM_TMU];          // +0x08 per TMU (gr_glide_create_texture clears both with one word store)
    ubyte _pad0a[2];
    glide_section *sections;             // +0x0c
};
LAYOUT_CHECK(glide_tcache_entry, sizeof(glide_tcache_entry) == 0x10);

// A block of TMU texture memory (0x14 bytes; glide_record's blocks, MAX_BITMAPS per TMU). The used
// blocks form a list in address order (used_head .. used_tail), the rest the free list. Names ours.
struct glide_block {
    int next;                            // +0x00 next block index, -1 end
    FxU32 addr;                          // +0x04 start address in the TMU
    int size;                            // +0x08 bytes
    int texture;                         // +0x0c owning glide_tcache_entry index, -1 none
    int section;                         // +0x10 owning section, -1 none
};
LAYOUT_CHECK(glide_block, sizeof(glide_block) == 0x14);

// The per-TMU texture memory manager (0x48 bytes; glide_tmus[2] at 0x2f0eb70, D2502, D2503). The
// sum.symbols glide_record__* functions are its thiscall members; glide_tmu_init (0x5497a0) is one too.
class glide_record : public tcache_slot {
public:
    glide_record();                                   // 0x549780 glide_record__ctor
    int  tmu_init(int tmu, bool mipmap);              // 0x5497a0 glide_tmu_init: returns the TMU's texture memory
    void init_blocks(FxU32 start, FxU32 end, int num);   // 0x549840 glide_record__init_blocks
    void reset_blocks();                              // 0x549880 glide_record__reset_blocks
    void free_texture_thunk(int texture);             // 0x549900 glide_record__free_texture_thunk
    void free_texture(int texture);                   // 0x549910 glide_record__free_texture
    void evict_block(int index);                      // 0x549950 glide_record__evict_block
    void close_thunk();                               // 0x5499d0 glide_record__close_thunk
    void close();                                     // 0x5499e0 glide_record__close
    void reset_blocks_thunk();                        // 0x549a10 glide_record__reset_blocks_thunk
    void load_section(glide_tcache_entry *entry, int section, float *u_scale, float *v_scale);   // 0x549a20
    FxU32 alloc(int size, int texture, int section);  // 0x549b50 glide_record__alloc: the address
    void release_block(int index);                    // 0x549c50 glide_record__release_block
    int  new_block();                                 // 0x549c80 glide_record__new_block
    glide_block *blocks;                              // +0x18
    int num_blocks;                                   // +0x1c
    int free_head;                                    // +0x20
    int used_head;                                    // +0x24
    int used_tail;                                    // +0x28
    FxU32 start_addr;                                 // +0x2c
    FxU32 end_addr;                                   // +0x30
    FxU32 mem_avail;                                  // +0x34 guTexMemQueryAvail
    FxU32 min_addr;                                   // +0x38 grTexMinAddress
    FxU32 max_addr;                                   // +0x3c grTexMaxAddress
    FxU32 next_addr;                                  // +0x40 = min_addr, no reader
    int mem_used;                                     // +0x44 bytes in blocks
};
LAYOUT_CHECK(glide_record, sizeof(glide_record) == 0x48);

// ---- .bss --------------------------------------------------------------------------------------------
// The entry points (glide_load_dll; sum.symbols names, address order).
extern grCheckForRoom_t glide_grCheckForRoom;                          // 0x2f0e9a8
extern guTexGetMipMapInfo_t glide_guTexGetMipMapInfo;                  // 0x2f0e9ac
extern HMODULE glide_dll_handle;                                       // 0x2f0e9b0
extern grDrawLine_t glide_grDrawLine;                                  // 0x2f0e9b4
extern grDrawPlanarPolygon_t glide_grDrawPlanarPolygon;                // 0x2f0e9b8
extern grDrawPlanarPolygonVertexList_t glide_grDrawPlanarPolygonVertexList;   // 0x2f0e9bc
extern grDrawPoint_t glide_grDrawPoint;                                // 0x2f0e9c0
extern grDrawPolygon_t glide_grDrawPolygon;                            // 0x2f0e9c4
extern grDrawPolygonVertexList_t glide_grDrawPolygonVertexList;        // 0x2f0e9c8
extern grDrawTriangle_t glide_grDrawTriangle;                          // 0x2f0e9cc
extern guDrawTriangleWithClip_t glide_guDrawTriangleWithClip;          // 0x2f0e9d0
extern grBufferClear_t glide_grBufferClear;                            // 0x2f0e9d4
extern grBufferNumPending_t glide_grBufferNumPending;                  // 0x2f0e9d8
extern grBufferSwap_t glide_grBufferSwap;                              // 0x2f0e9dc
extern grRenderBuffer_t glide_grRenderBuffer;                          // 0x2f0e9e0
extern grErrorSetCallback_t glide_grErrorSetCallback;                  // 0x2f0e9e4
extern grSstIdle_t glide_grSstIdle;                                    // 0x2f0e9e8
extern grSstVideoLine_t glide_grSstVideoLine;                          // 0x2f0e9ec
extern grSstVRetraceOn_t glide_grSstVRetraceOn;                        // 0x2f0e9f0
extern grSstIsBusy_t glide_grSstIsBusy;                                // 0x2f0e9f4
extern grSstWinOpen_t glide_grSstWinOpen;                              // 0x2f0e9f8
extern grSstWinClose_t glide_grSstWinClose;                            // 0x2f0e9fc
extern grSstControl_t glide_grSstControl;                              // 0x2f0ea00
extern grSstQueryHardware_t glide_grSstQueryHardware;                  // 0x2f0ea04
extern grSstQueryBoards_t glide_grSstQueryBoards;                      // 0x2f0ea08
extern grSstOrigin_t glide_grSstOrigin;                                // 0x2f0ea0c
extern grSstSelect_t glide_grSstSelect;                                // 0x2f0ea10
extern grSstScreenHeight_t glide_grSstScreenHeight;                    // 0x2f0ea14
extern grSstScreenWidth_t glide_grSstScreenWidth;                      // 0x2f0ea18
extern grSstStatus_t glide_grSstStatus;                                // 0x2f0ea1c
extern grSstPerfStats_t glide_grSstPerfStats;                          // 0x2f0ea20
extern grSstResetPerfStats_t glide_grSstResetPerfStats;                // 0x2f0ea24
extern grResetTriStats_t glide_grResetTriStats;                        // 0x2f0ea28
extern grTriStats_t glide_grTriStats;                                  // 0x2f0ea2c
extern grAlphaBlendFunction_t glide_grAlphaBlendFunction;              // 0x2f0ea30
extern grAlphaCombine_t glide_grAlphaCombine;                          // 0x2f0ea34
extern grAlphaControlsITRGBLighting_t glide_grAlphaControlsITRGBLighting;   // 0x2f0ea38
extern grAlphaTestFunction_t glide_grAlphaTestFunction;                // 0x2f0ea3c
extern grAlphaTestReferenceValue_t glide_grAlphaTestReferenceValue;    // 0x2f0ea40
extern grChromakeyMode_t glide_grChromakeyMode;                        // 0x2f0ea44
extern grChromakeyValue_t glide_grChromakeyValue;                      // 0x2f0ea48
extern grClipWindow_t glide_grClipWindow;                              // 0x2f0ea4c
extern grColorCombine_t glide_grColorCombine;                          // 0x2f0ea50
extern grColorMask_t glide_grColorMask;                                // 0x2f0ea54
extern grCullMode_t glide_grCullMode;                                  // 0x2f0ea58
extern grConstantColorValue_t glide_grConstantColorValue;              // 0x2f0ea5c
extern grConstantColorValue4_t glide_grConstantColorValue4;            // 0x2f0ea60
extern grDepthBiasLevel_t glide_grDepthBiasLevel;                      // 0x2f0ea64
extern grDepthBufferFunction_t glide_grDepthBufferFunction;            // 0x2f0ea68
extern grDepthBufferMode_t glide_grDepthBufferMode;                    // 0x2f0ea6c
extern grDepthMask_t glide_grDepthMask;                                // 0x2f0ea70
extern grDisableAllEffects_t glide_grDisableAllEffects;                // 0x2f0ea74
extern grDitherMode_t glide_grDitherMode;                              // 0x2f0ea78
extern grFogColorValue_t glide_grFogColorValue;                        // 0x2f0ea7c
extern grFogMode_t glide_grFogMode;                                    // 0x2f0ea80
extern grFogTable_t glide_grFogTable;                                  // 0x2f0ea84
extern grGammaCorrectionValue_t glide_grGammaCorrectionValue;          // 0x2f0ea88
extern grSplash_t glide_grSplash;                                      // 0x2f0ea8c
extern grTexCalcMemRequired_t glide_grTexCalcMemRequired;              // 0x2f0ea90
extern grTexTextureMemRequired_t glide_grTexTextureMemRequired;        // 0x2f0ea94
extern grTexMinAddress_t glide_grTexMinAddress;                        // 0x2f0ea98
extern grTexMaxAddress_t glide_grTexMaxAddress;                        // 0x2f0ea9c
extern grTexNCCTable_t glide_grTexNCCTable;                            // 0x2f0eaa0
extern grTexSource_t glide_grTexSource;                                // 0x2f0eaa4
extern grTexClampMode_t glide_grTexClampMode;                          // 0x2f0eaa8
extern grTexCombine_t glide_grTexCombine;                              // 0x2f0eaac
extern grTexCombineFunction_t glide_grTexCombineFunction;              // 0x2f0eab0
extern grTexDetailControl_t glide_grTexDetailControl;                  // 0x2f0eab4
extern grTexFilterMode_t glide_grTexFilterMode;                        // 0x2f0eab8
extern grTexLodBiasValue_t glide_grTexLodBiasValue;                    // 0x2f0eabc
extern grTexDownloadMipMap_t glide_grTexDownloadMipMap;                // 0x2f0eac0
extern grTexDownloadMipMapLevel_t glide_grTexDownloadMipMapLevel;      // 0x2f0eac4
extern grTexDownloadMipMapLevelPartial_t glide_grTexDownloadMipMapLevelPartial;   // 0x2f0eac8
extern ConvertAndDownloadRle_t glide_ConvertAndDownloadRle;            // 0x2f0eacc
extern grTexDownloadTable_t glide_grTexDownloadTable;                  // 0x2f0ead0
extern grTexDownloadTablePartial_t glide_grTexDownloadTablePartial;    // 0x2f0ead4
extern grTexMipMapMode_t glide_grTexMipMapMode;                        // 0x2f0ead8
extern grTexMultibase_t glide_grTexMultibase;                          // 0x2f0eadc
extern grTexMultibaseAddress_t glide_grTexMultibaseAddress;            // 0x2f0eae0
extern guTexAllocateMemory_t glide_guTexAllocateMemory;                // 0x2f0eae4
extern guTexChangeAttributes_t glide_guTexChangeAttributes;            // 0x2f0eae8
extern guTexCombineFunction_t glide_guTexCombineFunction;              // 0x2f0eaec
extern guTexGetCurrentMipMap_t glide_guTexGetCurrentMipMap;            // 0x2f0eaf0
extern guTexMemQueryAvail_t glide_guTexMemQueryAvail;                  // 0x2f0eaf4
extern guTexMemReset_t glide_guTexMemReset;                            // 0x2f0eaf8
extern guTexDownloadMipMap_t glide_guTexDownloadMipMap;                // 0x2f0eafc
extern guTexDownloadMipMapLevel_t glide_guTexDownloadMipMapLevel;      // 0x2f0eb00
extern guTexSource_t glide_guTexSource;                                // 0x2f0eb04
extern guColorCombineFunction_t glide_guColorCombineFunction;          // 0x2f0eb08
extern grLfbLock_t glide_grLfbLock;                                    // 0x2f0eb0c
extern grLfbUnlock_t glide_grLfbUnlock;                                // 0x2f0eb10
extern grLfbConstantAlpha_t glide_grLfbConstantAlpha;                  // 0x2f0eb14
extern grLfbConstantDepth_t glide_grLfbConstantDepth;                  // 0x2f0eb18
extern grLfbWriteColorSwizzle_t glide_grLfbWriteColorSwizzle;          // 0x2f0eb1c
extern grLfbWriteColorFormat_t glide_grLfbWriteColorFormat;            // 0x2f0eb20
extern grLfbWriteRegion_t glide_grLfbWriteRegion;                      // 0x2f0eb24
extern grLfbReadRegion_t glide_grLfbReadRegion;                        // 0x2f0eb28
extern grAADrawLine_t glide_grAADrawLine;                              // 0x2f0eb2c
extern grAADrawPoint_t glide_grAADrawPoint;                            // 0x2f0eb30
extern grAADrawPolygon_t glide_grAADrawPolygon;                        // 0x2f0eb34
extern grAADrawPolygonVertexList_t glide_grAADrawPolygonVertexList;    // 0x2f0eb38
extern grAADrawTriangle_t glide_grAADrawTriangle;                      // 0x2f0eb3c
extern grGlideInit_t glide_grGlideInit;                                // 0x2f0eb40
extern grGlideShutdown_t glide_grGlideShutdown;                        // 0x2f0eb44
extern grGlideGetVersion_t glide_grGlideGetVersion;                    // 0x2f0eb48
extern grGlideGetState_t glide_grGlideGetState;                        // 0x2f0eb4c
extern grGlideSetState_t glide_grGlideSetState;                        // 0x2f0eb50
extern grGlideShamelessPlug_t glide_grGlideShamelessPlug;              // 0x2f0eb54
extern grHints_t glide_grHints;                                        // 0x2f0eb58
extern guFogTableIndexToW_t glide_guFogTableIndexToW;                  // 0x2f0eb5c
extern guFogGenerateExp_t glide_guFogGenerateExp;                      // 0x2f0eb60
extern guFogGenerateExp2_t glide_guFogGenerateExp2;                    // 0x2f0eb64
extern guFogGenerateLinear_t glide_guFogGenerateLinear;                // 0x2f0eb68

extern glide_record glide_tmus[2];                    // 0x2f0eb70 (sum.symbols: uint8[0x90], D2503)
extern glide_tcache_entry *glide_tcache_entries;      // 0x2f0ec00 MAX_BITMAPS entries
extern int glide_texture_mem_used;                    // 0x2f0ec04 bytes of converted texel buffers
// The vertex buffer of gr_glide_tmapper / gr_glide_tmapper_multitexture (D2501: GrVertex[25], not the
// void*[32] of sum.symbols; the name is kept).
#define GLIDE_MAX_POLY_VERTS 25
extern GrVertex glide_poly_vertex_ptrs[GLIDE_MAX_POLY_VERTS];   // 0x2f0ec08
extern gr::Mode gr_glide_current_mode;                // 0x2f0f1e4 the mode last applied (invalid at start)
extern RECT glide_cursor_clip_rect;                   // 0x2f0f1f8 {0, 0, w - 1, h - 1}
extern int gr_glide_video_memory_mb;                  // 0x2f0f208 frame buffer MB of SST 0 (fbRam)
extern GrHwConfiguration glide_hwconfig;              // 0x2f0f210 (name ours)
extern int glide_initialized;                         // 0x2f132a4
extern int glide_init_done;                           // 0x2f132a8 (name ours) 1 at the end of gr_glide_init, no reader
// Activations still to pass to grSstControl by gr_glide_flip (the msg handler and gr_glide_deactivate add
// one). sum.symbols' name says the opposite; kept.
extern int glide_deactivation_count;                  // 0x2f132ac
// Deactivations still to pass to grSstControl; with glide_close_on_deactivate, -1 = the Glide window is
// closed (glide_set_active) and every draw call returns at once. Name ours.
extern int glide_pending_deactivations;               // 0x2f132b0
extern int glide_swap_enabled;                        // 0x2f132b4 (name ours) gr_glide_flip swaps while > 0
// Set by gr_glide_init for 2 TMUs and 12 MB or more (Voodoo2): the Glide window is closed when the game
// loses the focus and opened again when it gets it back. Name ours.
extern bool glide_close_on_deactivate;                // 0x2f132b8
extern float gr_glide_texture_scale_u;                // 0x2f132bc the uv scales of the bitmap bound last
extern float gr_glide_texture_scale_v;                // 0x2f132c0
extern float glide_texture_scale2_u;                  // 0x2f132c4 (name ours) the same for the second pass of a multitexture pair
extern float glide_texture_scale2_v;                  // 0x2f132c8
// 1 when there are fewer than 2 TMUs (gr_glide_init 0x549e8d): multitexture modes are then unsupported.
extern bool glide_single_tmu;                         // 0x2f132cd

// ---- .data -------------------------------------------------------------------------------------------
extern GrFog_t glide_fog_table[GR_FOG_TABLE_SIZE];    // 0x5ad500 (name ours) guFogGenerateLinear / grFogTable
extern int glide_app_active;                          // 0x5ad540 (name ours) 1, last WM_ACTIVATE(APP) state
extern float glide_whee;                              // 0x5ad544 (name ours) 0.1, console "whee", no reader
extern float glide_fog_near;                          // 0x5ad548 (name ours) -1, the last table's range
extern float glide_fog_far;                           // 0x5ad54c

// ---- functions ---------------------------------------------------------------------------------------
void glide_unload_dll();                              // 0x547390
int  glide_load_dll();                                // 0x5473b0 0 when glide2x.dll is missing
void gr_glide_tcache_init();                          // 0x5479c0
// (entry, release_tmus, free_sections): frees the entry's TMU blocks and / or its section buffers.
void gr_glide_tcache_free_entry(glide_tcache_entry *entry, bool release_tmus, bool free_sections);   // 0x547a90
void gr_glide_tcache_close();                         // 0x547b60
void gr_glide_tcache_flush();                         // 0x547ba0
void gr_glide_tcache_frame();                         // 0x547bf0 empty (name ours; gr_glide_flip tail-calls it)
bool gr_glide_round_texture_size(int w, int h);       // 0x547c00 true when both are already 4..256 powers of two
int  gr_glide_create_mipmapped_texture(int bm, glide_tcache_entry *entry);   // 0x547cc0 one section, all levels
// (bm, entry, section, data, palette, bm_w, bm_h, format): converts one section into a texture buffer.
int  gr_glide_create_texture_section(int bm, glide_tcache_entry *entry, int section, ubyte *data,
                                     ubyte *palette, int bm_w, int bm_h, int format);   // 0x5484f0
int  gr_glide_create_texture(int bm, glide_tcache_entry *entry);   // 0x5492f0
int  gr_glide_get_section_count(int bm);              // 0x5494e0
void gr_glide_get_section_rect(int bm, int section, int *x, int *y, int *w, int *h);   // 0x549500
// (tmu, bm, section, &u_scale, &v_scale, load): makes the texture and, with load, binds it to the TMU.
int  gr_glide_tcache_set(int tmu, int bm, int section, float *u_scale, float *v_scale, bool load);   // 0x549560
void gr_glide_mark_texture_dirty(int bm);             // 0x5496e0
void gr_glide_tcache_free(int bm);                    // 0x549730
void gr_glide_close();                                // 0x549cc0
void glide_clip_cursor(int on);                       // 0x549cf0
void gr_glide_init(HWND hwnd);                        // 0x549d10 (return not used by gr_init)
void glide_msg_handler(UINT msg, WPARAM wparam, LPARAM lparam);   // 0x549f50
void glide_set_active(int active);                    // 0x549fc0
bool gr_glide_is_mode_supported(gr::Mode *mode);      // 0x54a0b0 (gr_is_mode_supported passes the pointer on)
int  gr_glide_get_video_memory_mb();                  // 0x54a0d0
void gr_glide_set_mode(gr::Mode mode);                // 0x54a100
void gr_glide_apply_tint(GrVertex *v);                // 0x54a540
void gr_glide_set_viewport();                         // 0x54a5b0
void gr_glide_clear();                                // 0x54a600
void gr_glide_zbuffer_clear();                        // 0x54a650
void gr_glide_deactivate();                           // 0x54a6b0
void gr_glide_finish();                               // 0x54a6d0
void gr_glide_flip();                                 // 0x54a6e0
void gr_glide_page_in(int bm, int flags);             // 0x54a780 (gr_page_in passes both arguments; flags != 0 = TMU 1)
int  gr_glide_stub_ret1();                            // 0x54a7b0 no reference
void gr_glide_set_bitmap(gr::Mode mode, int bm);      // 0x54a7c0
void gr_glide_bitmap(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh,
                     gr::Mode mode);                  // 0x54a800
void gr_glide_line(float x1, float y1, float x2, float y2, gr::Mode mode);   // 0x54af10
void gr_glide_line_3d(gr_vertex *v0, gr_vertex *v1, gr::Mode mode);       // 0x54b110 (gr_line_3d 0x53e5e7)
void gr_glide_tmapper(int nv, gr_vertex **verts, gr::Mode mode);              // 0x54b240
void gr_glide_tmapper_multitexture(int nv, gr_multipass_vertex **verts, int npasses,
                                   gr_tmapper_pass *passes);                  // 0x54b3a0
void gr_glide_set_zbias(int bias);                    // 0x54b900 (name ours) no reference
void cmd_whee();                                      // 0x54b960
int  gr_glide_read_backbuffer(int x, int y, int w, int h, void *buf);         // 0x54b980 returns 3 (565)
void gr_glide_set_fog(int set_range);                 // 0x54b9b0 gr_set_fog passes 1
void gr_glide_set_antialias(bool on);                 // 0x54ba90 empty
void gr_glide_set_alpha_test(bool on);                // 0x54baa0
void gr_glide_set_gamma();                            // 0x54bac0
void gr_glide_flush_pipeline();                       // 0x54bad0

#endif
