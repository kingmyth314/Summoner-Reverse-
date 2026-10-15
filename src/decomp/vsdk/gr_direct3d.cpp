// gr_direct3d.cpp - Sum.exe v1.40, the Direct3D 7 back end (renderer 0x66, the retail default):
// code 0x534a20-0x536944 and 0x538650-0x53ac18, both tagged vsdk\gr\directx\gr_direct3d.cpp in sum.symbols.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// TWO OBJECTS (DECISIONS.md D1601). The vfx object (0x536950-0x538630, vsdk\geom: its initialisers are
// table entries 0x5889d0-0x5889d8, its .bss 0x2d90ee8-0x2dbfd1b) lies between the two blocks, and the first
// block ends with its own out-of-line header inlines (gr::Mode 0x5368a0-0x536930, d3d_f2dw 0x536940), as
// every vsdk object here does. So 0x534a20-0x536944 is a separate file of vsdk\gr\directx (state and
// drawing; its name is not known) and only 0x538650-0x53ac18 is gr_direct3d.cpp (its __FILE__ is at
// 0x53956c). Both are written here, part 1 first; they split at the "PART 2" line.
//
// Like the other vsdk objects these call the header inlines out of line (gr::Mode, D1300), so they were
// built without inline expansion. The DirectX calls are the SDK's C macros (d3d_min.h, D1600).
//
// Data. Part 1: .data 0x5a9cf8-0x5a9cf9, .bss 0x2d90ed4-0x2d90ee7, one static initialiser (table
// 0x5889cc). Part 2: .data 0x5a9d24-0x5a9d3f, then the string literals in function order (0x5a9d40-
// 0x5abdf3); .bss 0x2dbfd20-0x2dc6cbb (address order below; the order of VC6's .bss is the open question
// of frametime.cpp); three static initialisers (table 0x5889dc-0x5889e4), the command-line parameters.
//
// Not here: init_gr_d3d_vertex_buf (0x536950, sum.symbols tags it gr_direct3d.cpp) and gr_vertex_pool
// (0x2d90f10) belong to the vfx object (D1601). The texture cache (0x54bb60-0x54d5a0) is another unit.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "vsdk/os/debug.h"
#include "vsdk/os/registry.h"
#include "vsdk/os/timer.h"
#include "vsdk/os/os.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/os/cmdline.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/localize/localize.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/directx/d3d_min.h"
#include "vsdk/gr/directx/gr_direct3d.h"
#include "engine/rendering/levelrender.h"

// Original __FILE__ of gr_direct3d.cpp (0x5abadc, 0x5abb7c, 0x5abbd4; string pooling is off).
#define GR_DIRECT3D_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\gr\\directx\\gr_direct3d.cpp"

// MessageBox flags of the fatal messages (0x12000).
#define D3D_MB_FLAGS (MB_TASKMODAL | MB_SETFOREGROUND)

// =====================================================================================================
// PART 1 - 0x534a20-0x536944, the state and drawing object (file name not known, D1601)
// =====================================================================================================

// ---- .data (0x5a9cf8) ---------------------------------------------------------------------------------
bool gr_d3d_vertex_color = true;                      // 0x5a9cf8
bool gr_d3d_vertex_alpha = true;                      // 0x5a9cf9

// ---- .bss (0x2d90ed4-0x2d90ee7) -----------------------------------------------------------------------
float gr_d3d_texture_scale_v[2];                      // 0x2d90ed4 (sum.symbols had the pair swapped, D1602)
float gr_d3d_texture_scale_u[2];                      // 0x2d90edc

// 0x534a20 init_gr_d3d_current_mode (thunk `jmp 0x534a30`, initialiser table 0x5889cc; the body at
// 0x534a30 is `mov ecx, 0x2d90ee4; jmp gr::Mode::Mode`): compiler-generated.  Confidence: exact.
gr::Mode gr_d3d_current_mode;                         // 0x2d90ee4

// 0x534a40  Confidence: likely (the case bodies share tails in the binary: tail merging).
// Applies a gr::Mode as texture stage states and render states, unless it is the mode already set.
void gr_d3d_set_state(gr::Mode mode)
{
    if (mode == gr_d3d_current_mode)
        return;
    gr_d3d_current_mode = mode;

    int texture_source = mode.get_texture_source();
    int color_source = mode.get_color_source();
    int alpha_source = mode.get_alpha_source();
    int alpha_blend = mode.get_alpha_blend();
    int zbuffer_type = mode.get_zbuffer_type();
    int fog_type = mode.get_fog_type();

    switch (texture_source) {
    case 0:     // no texture
        gr_d3d_tcache_set(0, -1, 0, NULL, NULL, 0);
        gr_d3d_tcache_set(1, -1, 0, NULL, NULL, 0);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        break;
    case 1:     // wrap
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
        if (color_source == 3)
            IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLOROP, D3DTOP_ADD);
        else
            IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        break;
    case 2:     // clamp
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
        if (color_source == 3)
            IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLOROP, D3DTOP_ADD);
        else
            IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        break;
    case 3:     // clamp, no filtering
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MINFILTER, D3DTFN_POINT);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MAGFILTER, D3DTFG_POINT);
        if (color_source == 3)
            IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLOROP, D3DTOP_ADD);
        else
            IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        break;
    case 4:     // stage 1 clamped times stage 0 wrapped (the lightmap mode)
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_TEXCOORDINDEX, 0);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_MINFILTER, D3DTFN_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
        if (color_source == 3)
            IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLOROP, D3DTOP_ADD);
        else
            IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_TEXCOORDINDEX, 1);
        break;
    case 5:     // bump map on stage 0 (gr_d3d_set_bump_env)
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ADDRESS, D3DTADDRESS_MIRROR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_TEXCOORDINDEX, 0);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLOROP, D3DTOP_BUMPENVMAP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG2, D3DTA_CURRENT);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_MINFILTER, D3DTFN_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ADDRESS, D3DTADDRESS_MIRROR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_TEXCOORDINDEX, 1);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        break;
    case 6:     // stage 1 blended over stage 0 by its alpha
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_TEXCOORDINDEX, 0);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_MINFILTER, D3DTFN_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLOROP, D3DTOP_BLENDTEXTUREALPHA);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
        IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_TEXCOORDINDEX, 1);
        break;
    }

    // The back ends read these to pick white / opaque vertices (jump tables 0x5353d0, 0x5353e0: the
    // false case is laid out first).
    switch (color_source) {
    case 1:     // texture only
        gr_d3d_vertex_color = false;
        break;
    case 0:
    case 2:
    case 3:
        gr_d3d_vertex_color = true;
        break;
    }
    switch (alpha_source) {
    case 2:     // texture only
        gr_d3d_vertex_alpha = false;
        break;
    case 0:
    case 1:
    case 3:
        gr_d3d_vertex_alpha = true;
        break;
    }

    switch (alpha_blend) {
    case 0:     // none
        gr_d3d_set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
        gr_d3d_set_render_state(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
        break;
    case 1:     // additive
        gr_d3d_set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        gr_d3d_set_render_state(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
        gr_d3d_set_render_state(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
        gr_d3d_set_render_state(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
        break;
    case 2:     // alpha additive
    case 4:
        gr_d3d_set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        gr_d3d_set_render_state(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        gr_d3d_set_render_state(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
        gr_d3d_set_render_state(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
        break;
    case 3:     // alpha
        gr_d3d_set_render_state(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
        if (d3d_device_desc->dpcTriCaps.dwSrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) {
            gr_d3d_set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
            gr_d3d_set_render_state(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
            gr_d3d_set_render_state(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
        } else {
            gr_d3d_set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
            gr_d3d_set_render_state(D3DRENDERSTATE_SRCBLEND, D3DBLEND_BOTHSRCALPHA);
        }
        break;
    case 5:     // multiply (lightmaps)
        gr_d3d_set_render_state(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
        if (d3d_device_desc->dpcTriCaps.dwSrcBlendCaps & D3DPBLENDCAPS_DESTCOLOR) {
            gr_d3d_set_render_state(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
            gr_d3d_set_render_state(D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
            gr_d3d_set_render_state(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);
        }
        break;
    }

    switch (zbuffer_type) {
    case 0:     // none
        gr_d3d_set_render_state(D3DRENDERSTATE_ZENABLE, FALSE);
        gr_d3d_set_render_state(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
        break;
    case 1:     // read
        gr_d3d_set_render_state(D3DRENDERSTATE_ZENABLE, TRUE);
        gr_d3d_set_render_state(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
        gr_d3d_set_render_state(D3DRENDERSTATE_ZFUNC, d3d_zfunc);
        break;
    case 2:     // read, equal
        gr_d3d_set_render_state(D3DRENDERSTATE_ZENABLE, TRUE);
        gr_d3d_set_render_state(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
        gr_d3d_set_render_state(D3DRENDERSTATE_ZFUNC, D3DCMP_EQUAL);
        break;
    case 3:     // write only
        gr_d3d_set_render_state(D3DRENDERSTATE_ZENABLE, FALSE);
        gr_d3d_set_render_state(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
        break;
    case 4:     // full
        gr_d3d_set_render_state(D3DRENDERSTATE_ZENABLE, TRUE);
        gr_d3d_set_render_state(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
        gr_d3d_set_render_state(D3DRENDERSTATE_ZFUNC, d3d_zfunc);
        break;
    }

    switch (fog_type) {
    case 0:
    case 1:
    case 2:
        if (gr_fog_enabled)
            gr_d3d_set_render_state(D3DRENDERSTATE_FOGENABLE, TRUE);
        else
            gr_d3d_set_render_state(D3DRENDERSTATE_FOGENABLE, FALSE);
        break;
    case 3:     // not allowed
        gr_d3d_set_render_state(D3DRENDERSTATE_FOGENABLE, FALSE);
        break;
    }
}

// 0x535420  Confidence: exact.
// S3 cards only (d3d_s3_depth): draw with the depth test reversed (game_render_frame).
void gr_d3d_invert_depth_test()
{
    if (d3d_s3_depth) {
        d3d_zclear_value = 0.0f;
        d3d_zfunc = D3DCMP_GREATEREQUAL;
        gr_d3d_current_mode.invalidate();
    }
}

// 0x535450  Confidence: exact.
void gr_d3d_restore_depth_test()
{
    if (d3d_s3_depth) {
        d3d_zclear_value = 1.0f;
        d3d_zfunc = D3DCMP_LESSEQUAL;
        gr_d3d_current_mode.invalidate();
    }
}

// 0x535480  Confidence: exact.
// Greys the colour to its sum and scales it by the gr tint.
void gr_d3d_apply_tint(int *r, int *g, int *b)
{
    float lum = (float)(*r + *g + *b);
    *r = (int)(gr_tint_r * lum);
    clamp_int(r, 0, 255);
    *g = (int)(gr_tint_g * lum);
    clamp_int(g, 0, 255);
    *b = (int)(gr_tint_b * lum);
    clamp_int(b, 0, 255);
}

// 0x535510  Confidence: exact.
void gr_d3d_set_zbias(int bias)
{
    gr_d3d_set_render_state(D3DRENDERSTATE_ZBIAS, bias);
}

// 0x535520  Confidence: exact.
void gr_d3d_page_in(int bm)
{
    if (bm > -1)
        gr_d3d_tcache_set(0, bm, 0, &gr_d3d_texture_scale_u[0], &gr_d3d_texture_scale_v[0], 0);
}

// 0x535550  Confidence: exact.
// Clears the clip rectangle to black (gr_clear).
void gr_d3d_clear()
{
    D3DRECT rect;
    gr_d3d_set_state(gr::Mode(0, 0, 0, 0, 0, 0));
    rect.x1 = gr_screen.offset_x + gr_screen.clip_left;
    rect.y1 = gr_screen.offset_y + gr_screen.clip_top;
    rect.x2 = gr_screen.clip_right + gr_screen.offset_x + 1;
    rect.y2 = gr_screen.clip_bottom + gr_screen.offset_y + 1;
    IDirect3DDevice7_Clear(d3d_device, 1, &rect, D3DCLEAR_TARGET, 0, 0.0f, 0);
}

// 0x5355e0  Confidence: exact.
// The same rectangle without the + 1, so the last column and row of the clip area keep their depth.
void gr_d3d_zbuffer_clear()
{
    D3DRECT rect;
    gr_d3d_set_state(gr::Mode(0, 0, 0, 0, 4, 0));
    rect.x1 = gr_screen.offset_x + gr_screen.clip_left;
    rect.y1 = gr_screen.offset_y + gr_screen.clip_top;
    rect.x2 = gr_screen.clip_right + gr_screen.offset_x;
    rect.y2 = gr_screen.clip_bottom + gr_screen.offset_y;
    IDirect3DDevice7_Clear(d3d_device, 1, &rect, D3DCLEAR_ZBUFFER, 0, d3d_zclear_value, 0);
}

// 0x535670  Confidence: exact.
void gr_d3d_set_viewport()
{
    D3DVIEWPORT7 vp;
    vp.dwX = gr_screen.offset_x + gr_screen.clip_left;
    vp.dwY = gr_screen.offset_y + gr_screen.clip_top;
    vp.dwWidth = gr_screen.clip_width;
    vp.dwHeight = gr_screen.clip_height;
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 0.0f;
    IDirect3DDevice7_SetViewport(d3d_device, &vp);
}

// 0x5356d0  Confidence: likely (x87 scheduling of the corner values; vertex order from the stores).
// Draws (sx, sy, sw, sh) of bm at (x, y, w, h) as one fan, or a bitmap split in texture sections as one
// fan per section (the sections ignore the source rectangle and the size). Without MODULATEALPHA the
// colour is premultiplied by the alpha and sent opaque.
void gr_d3d_bitmap(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh, gr::Mode mode)
{
    if (w < 1 || h < 1)
        return;

    gr_d3d_set_bitmap(mode, bm, -1);
    int bw, bh;
    bm_get_dimensions(bm, &bw, &bh);
    int num_sections = gr_d3d_get_section_count(bm);

    int r, g, b, a;
    if (gr_d3d_vertex_color) {
        r = gr_color.r;
        g = gr_color.g;
        b = gr_color.b;
    } else {
        r = g = b = 255;
    }
    if (gr_d3d_vertex_alpha)
        a = gr_color.a;
    else
        a = 255;

    D3DCOLOR color;
    if (d3d_device_desc->dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATEALPHA) {
        if (gr_tint_enabled)
            gr_d3d_apply_tint(&r, &g, &b);
        color = RGBA_MAKE(r, g, b, a);
    } else {
        r = a * r / 255;
        g = a * g / 255;
        b = a * b / 255;
        if (gr_tint_enabled)
            gr_d3d_apply_tint(&r, &g, &b);
        color = RGBA_MAKE(r, g, b, 255);
    }

    d3d_tlvertex v[4];
    if (num_sections == 1) {
        float fw = (float)bw;
        float fh = (float)bh;
        float u0 = ((float)sx + d3d_bitmap_u_offset) * gr_d3d_texture_scale_u[0] / fw;
        float v0 = ((float)sy + d3d_bitmap_u_offset) * gr_d3d_texture_scale_v[0] / fh;
        float u1 = ((float)(sx + sw) + d3d_bitmap_v_offset) * gr_d3d_texture_scale_u[0] / fw;
        float v1 = ((float)(sy + sh) + d3d_bitmap_v_offset) * gr_d3d_texture_scale_v[0] / fh;
        float x0 = (float)(gr_screen.offset_x + x) - 0.5f;
        float y0 = (float)(gr_screen.offset_y + y) - 0.5f;
        float x1 = (float)(gr_screen.offset_x + x + w) - 0.5f;
        float y1 = (float)(gr_screen.offset_y + y + h) - 0.5f;

        v[0].sx = x0;  v[0].sy = y0;  v[0].sz = 0.99f;  v[0].rhw = 1.0f;  v[0].color = color;
        v[0].tu = u0;  v[0].tv = v0;
        v[1].sx = x1;  v[1].sy = y0;  v[1].sz = 0.99f;  v[1].rhw = 1.0f;  v[1].color = color;
        v[1].tu = u1;  v[1].tv = v0;
        v[2].sx = x1;  v[2].sy = y1;  v[2].sz = 0.99f;  v[2].rhw = 1.0f;  v[2].color = color;
        v[2].tu = u1;  v[2].tv = v1;
        v[3].sx = x0;  v[3].sy = y1;  v[3].sz = 0.99f;  v[3].rhw = 1.0f;  v[3].color = color;
        v[3].tu = u0;  v[3].tv = v1;
        gr_d3d_draw_primitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX2, v, 4, 0);
        return;
    }

    for (int i = 0; i < num_sections; i++) {
        int sec_x, sec_y, sec_w, sec_h;
        gr_d3d_tcache_set(0, bm, i, &gr_d3d_texture_scale_u[0], &gr_d3d_texture_scale_v[0], 1);
        gr_d3d_get_section_rect(bm, i, &sec_x, &sec_y, &sec_w, &sec_h);
        float fw = (float)sec_w;
        float fh = (float)sec_h;
        float u0 = d3d_bitmap_u_offset / fw;
        float v0 = d3d_bitmap_u_offset / fh;
        float u1 = 1.0f - d3d_bitmap_v_offset / fw;
        float v1 = 1.0f - d3d_bitmap_v_offset / fh;
        float x0 = (float)(gr_screen.offset_x + sec_x + x) - 0.5f;
        float y0 = (float)(gr_screen.offset_y + sec_y + y) - 0.5f;
        float x1 = (float)(gr_screen.offset_x + sec_x + sec_w + x) - 0.5f;
        float y1 = (float)(gr_screen.offset_y + sec_y + sec_h + y) - 0.5f;

        v[0].sx = x0;  v[0].sy = y0;  v[0].sz = 0.99f;  v[0].rhw = 1.0f;  v[0].color = color;
        v[0].tu = u0;  v[0].tv = v0;
        v[1].sx = x1;  v[1].sy = y0;  v[1].sz = 0.99f;  v[1].rhw = 1.0f;  v[1].color = color;
        v[1].tu = u1;  v[1].tv = v0;
        v[2].sx = x1;  v[2].sy = y1;  v[2].sz = 0.99f;  v[2].rhw = 1.0f;  v[2].color = color;
        v[2].tu = u1;  v[2].tv = v1;
        v[3].sx = x0;  v[3].sy = y1;  v[3].sz = 0.99f;  v[3].rhw = 1.0f;  v[3].color = color;
        v[3].tu = u0;  v[3].tv = v1;
        gr_d3d_draw_primitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX2, v, 4, 0);
    }
}

// 0x535ce0  Confidence: exact.
// The stage argument is not used (the callers pass -1).
void gr_d3d_set_bitmap(gr::Mode mode, int bm, int stage)
{
    if (bm > -1)
        gr_d3d_tcache_set(0, bm, 0, &gr_d3d_texture_scale_u[0], &gr_d3d_texture_scale_v[0], 1);
    gr_d3d_set_state(mode);
}

// 0x535d10  Confidence: exact.
void gr_d3d_set_color()
{
}

// 0x535d20  Confidence: likely (where the early `*out = 0` stands).
// Vertex fog as a specular alpha (linear between gr_fog_near and gr_fog_far). No caller.
void gr_d3d_fog_alpha(float z, D3DCOLOR *out)
{
    float f = (gr_fog_far - z) / (gr_fog_far - gr_fog_near);
    *out = 0;
    if (f < 0.0f)
        f = 0.0f;
    else if (f > 1.0f)
        f = 1.0f;
    *out = (int)(f * 255.0f) << 24;
}

// 0x535da0  Confidence: likely.
void gr_d3d_line(float x1, float y1, float x2, float y2, gr::Mode mode)
{
    d3d_tlvertex v[2];
    int r, g, b, a;

    gr_d3d_set_bitmap(mode, -1, -1);
    if (gr_d3d_vertex_color) {
        r = gr_color.r;
        g = gr_color.g;
        b = gr_color.b;
    } else {
        r = g = b = 255;
    }
    a = 255;
    if (gr_d3d_vertex_alpha)
        a = gr_color.a;
    if (gr_tint_enabled)
        gr_d3d_apply_tint(&r, &g, &b);
    D3DCOLOR color = RGBA_MAKE(r, g, b, a);

    gr_d3d_make_line_verts(&v[0], &v[1], (int)x1, (int)y1, (int)x2, (int)y2);
    v[0].color = color;
    v[1].color = color;
    gr_d3d_draw_primitive(D3DPT_LINELIST, D3DFVF_TLVERTEX2, v, 2, 0);
}

// 0x535eb0  Confidence: likely.
// Screen-space line ends; an axis-aligned line gets half a pixel more at its far end.
void gr_d3d_make_line_verts(d3d_tlvertex *v0, d3d_tlvertex *v1, int x0, int y0, int x1, int y1)
{
    v0->rhw = v1->rhw = 1.0f;
    v0->sz = v1->sz = 0.99f;
    v0->sx = (float)(gr_screen.offset_x + x0) + d3d_line_pixel_offset;
    v0->sy = (float)(gr_screen.offset_y + y0) + d3d_line_pixel_offset;
    v1->sx = (float)(gr_screen.offset_x + x1) + d3d_line_pixel_offset;
    v1->sy = (float)(gr_screen.offset_y + y1) + d3d_line_pixel_offset;
    if (x0 == x1) {
        if (v0->sy < v1->sy)
            v1->sy = v1->sy + 0.5f;
        else
            v0->sy = v0->sy + 0.5f;
    } else if (y0 == y1) {
        if (v0->sx < v1->sx)
            v1->sx = v1->sx + 0.5f;
        else
            v0->sx = v0->sx + 0.5f;
    }
}

// 0x535fd0  Confidence: likely.
void gr_d3d_line_3d(gr_vertex *v0, gr_vertex *v1, gr::Mode mode)
{
    d3d_tlvertex v[2];
    int r, g, b, a;

    gr_d3d_set_bitmap(mode, -1, -1);
    if (gr_d3d_vertex_color) {
        r = gr_color.r;
        g = gr_color.g;
        b = gr_color.b;
    } else {
        r = g = b = 255;
    }
    if (gr_d3d_vertex_alpha)
        a = gr_color.a;
    else
        a = 255;

    D3DCOLOR color;
    if (d3d_device_desc->dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATEALPHA) {
        if (gr_tint_enabled)
            gr_d3d_apply_tint(&r, &g, &b);
        color = RGBA_MAKE(r, g, b, a);
    } else {
        r = a * r / 255;
        g = a * g / 255;
        b = a * b / 255;
        if (gr_tint_enabled)
            gr_d3d_apply_tint(&r, &g, &b);
        color = RGBA_MAKE(r, g, b, 255);
    }

    v[0].sz = gr_calc_depth(v0->sw);
    v[1].sz = gr_calc_depth(v1->sw);
    v[0].rhw = v0->sw;
    v[1].rhw = v1->sw;
    v[0].sx = v0->sx;
    v[0].sy = v0->sy;
    v[1].sx = v1->sx;
    v[1].sy = v1->sy;
    v[0].color = color;
    v[1].color = color;
    gr_d3d_draw_primitive(D3DPT_LINELIST, D3DFVF_TLVERTEX2, v, 2, 0);
}

// 0x5361a0  Confidence: likely (the vertex pointer is reloaded from verts[] for the flag tests).
// One textured fan of projected vertices (at most 32). Vertex flags (+0x1a): 1 uv, 4 colour, 8 alpha.
void gr_d3d_tmapper(int nv, gr_vertex **verts, gr::Mode mode)
{
    d3d_tlvertex dv[32];
    int r, g, b, a;

    gr_d3d_set_bitmap(mode, gr_texture, -1);
    for (int i = 0; i < nv; i++) {
        gr_vertex *v = verts[i];
        d3d_tlvertex *d = &dv[i];
        d->rhw = v->sw;
        d->sz = gr_calc_depth(v->sw);
        d->specular_bytes[3] = (ubyte)gr_get_fog_alpha(v->z);

        if (gr_d3d_vertex_alpha) {
            if (verts[i]->_unk1a & 8)
                a = verts[i]->a;
            else
                a = gr_color.a;
        } else {
            a = 255;
        }
        if (gr_d3d_vertex_color) {
            if (verts[i]->_unk1a & 4) {
                r = gr_gamma_ramp[verts[i]->r];
                g = gr_gamma_ramp[verts[i]->g];
                b = gr_gamma_ramp[verts[i]->b];
            } else {
                r = gr_color.r;
                g = gr_color.g;
                b = gr_color.b;
            }
        } else {
            r = g = b = 255;
        }
        if (gr_tint_enabled)
            gr_d3d_apply_tint(&r, &g, &b);
        d->color = RGBA_MAKE(r, g, b, a);
        d->sx = v->sx;
        d->sy = v->sy;
        if (verts[i]->_unk1a & 1) {
            d->tu = gr_d3d_texture_scale_u[0] * v->u;
            d->tv = gr_d3d_texture_scale_v[0] * v->v;
        }
    }
    gr_d3d_draw_primitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX2, dv, nv, 0);
}

// 0x536360  Confidence: unsure (loop shapes; the pass record and the vertex record are read from this
// function only).
// gr_tmapper_multipass: one fan per pass; a multitexture pass also takes the next pass's bitmap on its
// stage, and that pass is skipped. Colours go through the gamma ramp; no tint.
void gr_d3d_tmapper_multipass(int nv, gr_multipass_vertex **verts, int npasses, gr_tmapper_pass *passes)
{
    gr_vertex *vlist[32];
    d3d_tlvertex dv[32];
    int i, j;

    for (j = 0; j < nv; j++) {
        gr_vertex *v = verts[j]->v;
        vlist[j] = v;
        dv[j].sx = v->sx;
        dv[j].sy = v->sy;
        dv[j].rhw = v->sw;
        dv[j].sz = gr_calc_depth(v->sw);
        dv[j].specular_bytes[3] = (ubyte)gr_get_fog_alpha(v->z);
    }

    for (i = 0; i < npasses; i++) {
        gr_tmapper_pass *pass = &passes[i];
        gr_tmapper_pass *pass2;
        if (!pass->mode.is_multitexture())
            pass2 = NULL;
        else
            pass2 = &passes[i + 1];

        if (pass->bitmap > -1 && pass->enabled) {
            if (pass->sectioned) {
                gr::Mode m = pass->mode;
                if (m.get_texture_source() == 1)
                    m = gr::Mode(2, m.get_color_source(), m.get_alpha_source(), m.get_alpha_blend(),
                                 m.get_zbuffer_type(), m.get_fog_type());
                gr_d3d_set_state(m);
                float u_off, v_off, u_scale, v_scale;
                int section = gr_poly_get_section(nv, verts, i, pass->bitmap, &u_off, &v_off, &u_scale,
                                                  &v_scale);
                gr_d3d_tcache_set(0, pass->bitmap, section, &gr_d3d_texture_scale_u[0],
                                  &gr_d3d_texture_scale_v[0], 1);
                u_scale = gr_d3d_texture_scale_u[0] * u_scale;
                v_scale = gr_d3d_texture_scale_v[0] * v_scale;
                for (j = 0; j < nv; j++) {
                    dv[j].tu = (verts[j]->pass[i].u - u_off) * u_scale;
                    dv[j].tv = (verts[j]->pass[i].v - v_off) * v_scale;
                }
            } else {
                gr_d3d_set_state(pass->mode);
                gr_d3d_tcache_set(pass->stage, pass->bitmap, 0, &gr_d3d_texture_scale_u[0],
                                  &gr_d3d_texture_scale_v[0], 1);
                for (j = 0; j < nv; j++) {
                    float *uv = &dv[j].tu + 2 * pass->stage;
                    uv[0] = gr_d3d_texture_scale_u[0] * verts[j]->pass[i].u;
                    uv[1] = gr_d3d_texture_scale_v[0] * verts[j]->pass[i].v;
                }
                if (pass2) {
                    gr_d3d_tcache_set(pass2->stage, pass2->bitmap, 0, &gr_d3d_texture_scale_u[1],
                                      &gr_d3d_texture_scale_v[1], 1);
                    for (j = 0; j < nv; j++) {
                        float *uv = &dv[j].tu + 2 * pass2->stage;
                        uv[0] = gr_d3d_texture_scale_u[1] * verts[j]->pass[i + 1].u;
                        uv[1] = gr_d3d_texture_scale_v[1] * verts[j]->pass[i + 1].v;
                    }
                }
            }
        } else {
            gr_d3d_set_state(pass->mode);
        }

        for (j = 0; j < nv; j++) {
            gr_multipass_vertex *mv = verts[j];
            int r, g, b, a;
            if (gr_d3d_vertex_alpha) {
                if (pass->vertex_alpha)
                    a = mv->pass[i].a;
                else
                    a = pass->a;
            } else {
                a = 255;
            }
            if (gr_d3d_vertex_color) {
                if (pass->vertex_color) {
                    r = gr_gamma_ramp[mv->pass[i].r];
                    g = gr_gamma_ramp[mv->pass[i].g];
                    b = gr_gamma_ramp[mv->pass[i].b];
                } else {
                    r = gr_gamma_ramp[pass->r];
                    g = gr_gamma_ramp[pass->g];
                    b = gr_gamma_ramp[pass->b];
                }
            } else {
                r = g = b = 255;
            }
            dv[j].color = RGBA_MAKE(r, g, b, a);
        }
        gr_d3d_draw_primitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX2, dv, nv, 0);
        if (pass2)
            i++;
    }
}

// 0x536790  Confidence: exact.
bool gr_d3d_has_bump_format()
{
    return d3d_texture_format_c_found;
}

// 0x5367a0  Confidence: exact.
// Stage 0 bump environment (luminance scale / offset and the 2x2 matrix), when a du/dv format exists.
bool gr_d3d_set_bump_env(float lscale, float loffset, float m00, float m01, float m10, float m11)
{
    if (!d3d_texture_format_c_found)
        return false;
    IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_BUMPENVLSCALE, d3d_f2dw(lscale));
    IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_BUMPENVLOFFSET, d3d_f2dw(loffset));
    IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_BUMPENVMAT00, d3d_f2dw(m00));
    IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_BUMPENVMAT01, d3d_f2dw(m01));
    IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_BUMPENVMAT10, d3d_f2dw(m10));
    IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_BUMPENVMAT11, d3d_f2dw(m11));
    return true;
}

// Out-of-line header inlines at the end of the object (not source here): gr::Mode 0x5368a0-0x536930
// (gr.h, D1300) and
// 0x536940 d3d_f2dw (gr_direct3d.h: the DirectX SDK's F2DW).  Confidence: exact.

// =====================================================================================================
// PART 2 - 0x538650-0x53ac18, gr_direct3d.cpp
// =====================================================================================================

// ---- .data (0x5a9d24-0x5a9d3f) ------------------------------------------------------------------------
DWORD gr_d3d_lock_discard = DDLOCK_DISCARDCONTENTS | DDLOCK_NOSYSLOCK | DDLOCK_WRITEONLY;  // 0x5a9d24 0x2820
DWORD gr_d3d_lock_nooverwrite = DDLOCK_NOOVERWRITE | DDLOCK_NOSYSLOCK | DDLOCK_WRITEONLY;  // 0x5a9d28 0x1820
int d3d_unused_5a9d2c = 1;                            // 0x5a9d2c no reference (name ours)
int d3d_unused_5a9d30 = 1;                            // 0x5a9d30 no reference (name ours)
bool d3d_windowed = true;                             // 0x5a9d34
float d3d_last_fog_start = -1.0f;                     // 0x5a9d38
float d3d_last_fog_end = -1.0f;                       // 0x5a9d3c

// ---- .bss, in address order (0x2dbfd20-0x2dc6cbb) -----------------------------------------------------
// The command-line parameters are defined below with their initialisers.
D3DDEVICEDESC7 d3d_device_caps;                       // 0x2dbfd20
DDGAMMARAMP d3d_saved_gamma_ramp;                     // 0x2dbfe10
DDCAPS d3d_hel_caps;                                  // 0x2dc0410
float d3d_fog_start;                                  // 0x2dc058c
DDPIXELFORMAT d3d_screen_pixel_format;                // 0x2dc0590
RECT d3d_window_rect;                                 // 0x2dc05b0
int d3d_zfunc;                                        // 0x2dc05c0
d3d_color_channel d3d_green_channel;                  // 0x2dc05c8
float d3d_fog_end;                                    // 0x2dc05d8
int d3d_unused_2dc05dc;                               // 0x2dc05dc cleared by gr_d3d_init_device, never read (name ours)
//  cmdline_param_vertexfog                              0x2dc05e0
d3d_color_channel d3d_blue_channel;                   // 0x2dc05f8
DDCAPS d3d_driver_caps;                               // 0x2dc0608
DDPIXELFORMAT d3d_texture_format_a;                   // 0x2dc0798
d3d_color_channel d3d_red_channel;                    // 0x2dc07b8
d3d_device_record d3d_ddraw_devices[D3D_MAX_DDRAW_DEVICES];   // 0x2dc07c8
//  cmdline_param_rz                                     0x2dc2908
DDPIXELFORMAT d3d_texture_format_b;                   // 0x2dc2920
DDPIXELFORMAT d3d_texture_format_c;                   // 0x2dc2940
int d3d_flag_2dc2960;                                 // 0x2dc2960
RECT d3d_screen_rect;                                 // 0x2dc2968
float d3d_zclear_value;                               // 0x2dc2988
d3d_device_record d3d_3d_devices[D3D_MAX_3D_DEVICES]; // 0x2dc2990
//  cmdline_param_fb2                                    0x2dc6c10
IDirect3DVertexBuffer7 *d3d_vertex_buffer;            // 0x2dc6c28
int d3d_vertex_buffer_pos;                            // 0x2dc6c2c
bool d3d_s3_depth;                                    // 0x2dc6c30
bool d3d_use_vertex_buffer;                           // 0x2dc6c31
bool d3d_ati_5144;                                    // 0x2dc6c32
LPDIRECTDRAWCREATEEX d3d_ddraw_enumerate_ex;          // 0x2dc6c34
IDirectDraw7 *d3d_ddraw;                              // 0x2dc6c38
void *d3d_direct3d;                                   // 0x2dc6c3c IDirect3D7 * (D1604)
IDirect3DDevice7 *d3d_device;                         // 0x2dc6c40
IDirectDrawSurface7 *d3d_back_surface;             // 0x2dc6c44 the render target (D1603)
IDirectDrawSurface7 *d3d_primary_surface;                // 0x2dc6c48 the primary surface (D1603)
IDirectDrawSurface7 *d3d_zbuffer_surface;             // 0x2dc6c4c
IDirectDrawGammaControl *d3d_gamma_control;           // 0x2dc6c50
int d3d_texture_format_b_bits;                        // 0x2dc6c54
int d3d_texture_format_a_bits;                        // 0x2dc6c58
bool d3d_texture_format_c_found;                      // 0x2dc6c5c
D3DDEVICEDESC7 *d3d_device_desc;                      // 0x2dc6c60
int d3d_scene_depth;                                  // 0x2dc6c64
int d3d_device_ready;                                 // 0x2dc6c68
float d3d_bitmap_u_offset;                            // 0x2dc6c6c
float d3d_bitmap_v_offset;                            // 0x2dc6c70
float d3d_line_pixel_offset;                          // 0x2dc6c74
int d3d_min_texture_w;                                // 0x2dc6c78
int d3d_max_texture_w;                                // 0x2dc6c7c
int d3d_min_texture_h;                                // 0x2dc6c80
int d3d_max_texture_h;                                // 0x2dc6c84
int d3d_square_textures_only;                         // 0x2dc6c88
int d3d_pow2_textures_only;                           // 0x2dc6c8c
bool d3d_multitexture_supported;                      // 0x2dc6c90
HWND d3d_hwnd;                                        // 0x2dc6c94
int gr_d3d_video_memory_mb;                           // 0x2dc6c98
int d3d_fog_mode;                                     // 0x2dc6c9c
int d3d_init_done;                                    // 0x2dc6ca0
volatile int d3d_flush_request;                       // 0x2dc6ca4
volatile int d3d_flush_request_b;                     // 0x2dc6ca8
int d3d_num_ddraw_devices;                            // 0x2dc6cac
int d3d_num_3d_devices;                               // 0x2dc6cb0
// The parameters' help texts are .bss addresses, so not literals: zero-filled arrays (labels ours).
static char d3d_fb2_help[4];                          // 0x2dc6cb4
static char d3d_vertexfog_help[4];                    // 0x2dc6cb8

// ---- command-line parameters ------------------------------------------------------------------------
// 0x538650 init_cmdline_param_fb2_thunk (initialiser table 0x5889dc: calls the initialiser, then jumps
// to the atexit registration): compiler-generated.  Confidence: exact.
// 0x538660 init_cmdline_param_fb2: the dynamic initialiser of this definition.  Confidence: exact.
// 0x538680 cmdline_param_fb2_register_dtor (atexit): compiler-generated.  Confidence: exact.
// 0x538690 cmdline_param_fb2_dtor (atexit handler, CmdLineParam::~CmdLineParam): compiler-generated.
// Confidence: exact.
CmdLineParam cmdline_param_fb2("-fb2", d3d_fb2_help, false);              // 0x2dc6c10
// 0x5386a0 init_cmdline_param_vertexfog_thunk (table 0x5889e0): compiler-generated.  Confidence: exact.
// 0x5386b0 init_cmdline_param_vertexfog: the dynamic initialiser.  Confidence: exact.
// 0x5386d0 cmdline_param_vertexfog_register_dtor: compiler-generated.  Confidence: exact.
// 0x5386e0 cmdline_param_vertexfog_dtor: compiler-generated.  Confidence: exact.
CmdLineParam cmdline_param_vertexfog("-vertexfog", d3d_vertexfog_help, false);   // 0x2dc05e0
// 0x5386f0 init_cmdline_param_rz_thunk (table 0x5889e4): compiler-generated.  Confidence: exact.
// 0x538700 init_cmdline_param_rz: the dynamic initialiser (no help text, takes an argument).
// Confidence: exact.
// 0x538720 cmdline_param_rz_register_dtor: compiler-generated.  Confidence: exact.
// 0x538730 cmdline_param_rz_dtor: compiler-generated.  Confidence: exact.
CmdLineParam cmdline_param_rz("-rz", NULL, true);                         // 0x2dc2908

// 0x538740  Confidence: exact.
void gr_d3d_set_render_state(DWORD state, DWORD value)
{
    IDirect3DDevice7_SetRenderState(d3d_device, state, value);
}

// 0x538760  Confidence: likely (the lock pointer lives in the count argument's slot).
// With the vertex buffer: appends the vertices (starting over at 1000) and draws from the buffer.
void gr_d3d_draw_primitive(DWORD type, DWORD fvf, void *verts, DWORD count, DWORD flags)
{
    if (d3d_use_vertex_buffer) {
        DWORD lock_flags;
        if (d3d_vertex_buffer_pos + count >= 1000) {
            lock_flags = gr_d3d_lock_discard;
            d3d_vertex_buffer_pos = 0;
        } else if (d3d_vertex_buffer_pos == 0) {
            lock_flags = gr_d3d_lock_discard;
        } else {
            lock_flags = gr_d3d_lock_nooverwrite;
        }
        void *data = NULL;
        IDirect3DVertexBuffer7_Lock(d3d_vertex_buffer, lock_flags, &data, NULL);
        memcpy((ubyte *)data + d3d_vertex_buffer_pos * sizeof(d3d_tlvertex), verts,
               count * sizeof(d3d_tlvertex));
        IDirect3DVertexBuffer7_Unlock(d3d_vertex_buffer);
        IDirect3DDevice7_DrawPrimitiveVB(d3d_device, type, d3d_vertex_buffer, d3d_vertex_buffer_pos, count,
                                         flags);
        d3d_vertex_buffer_pos += count;
    } else {
        IDirect3DDevice7_DrawPrimitive(d3d_device, type, fvf, verts, count, flags);
    }
}

// 0x538850  Confidence: likely.
// Lists the DirectDraw drivers, then the hardware (non-T&L) Direct3D devices of each; returns the last
// one found.
d3d_device_record *d3d_enumerate_ddraw_devices()
{
    d3d_num_ddraw_devices = 0;
    d3d_num_3d_devices = 0;
    if (DirectDrawEnumerateA(d3d_ddraw_enum_callback, NULL) != DD_OK)
        return NULL;
    for (int i = 0; i < d3d_num_ddraw_devices; i++) {
        d3d_device_record *rec = &d3d_ddraw_devices[i];
        if (d3d_ddraw_enumerate_ex(rec->ddraw_guid_ptr, (void **)&d3d_ddraw, &IID_IDirectDraw7, NULL) != DD_OK)
            return NULL;
        if (IDirectDraw7_QueryInterface(d3d_ddraw, &IID_IDirect3D7, &d3d_direct3d) != DD_OK)
            return NULL;
        IDirect3D7_EnumDevices(D3D_DIRECT3D, d3d_enum_devices_callback, rec);
        IDirect3D7_Release(D3D_DIRECT3D);
        d3d_direct3d = NULL;
        IDirectDraw7_Release(d3d_ddraw);
        d3d_ddraw = NULL;
    }
    if (d3d_num_3d_devices > 0)
        return &d3d_3d_devices[d3d_num_3d_devices - 1];
    return NULL;
}

// 0x538920  Confidence: likely (memmove: the 16-byte copies are calls, not inline).
// EnumDevices callback: records every device without hardware transform and lighting.
HRESULT CALLBACK d3d_enum_devices_callback(LPSTR description, LPSTR name, D3DDEVICEDESC7 *desc,
                                           LPVOID context)
{
    if (!desc || (desc->dwDevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT))
        return D3DENUMRET_OK;
    d3d_device_record *ddraw_rec = (d3d_device_record *)context;
    d3d_device_record *rec = &d3d_3d_devices[d3d_num_3d_devices++];
    memmove(&rec->d3d_guid, &desc->deviceGUID, sizeof(GUID));
    rec->d3d_guid_ptr = &rec->d3d_guid;
    memmove(&rec->ddraw_guid, &ddraw_rec->ddraw_guid, sizeof(GUID));
    if (ddraw_rec->ddraw_guid_ptr)
        rec->ddraw_guid_ptr = &rec->ddraw_guid;
    else
        rec->ddraw_guid_ptr = NULL;
    strcpy(rec->name, "Direct 3D - ");
    strcat(rec->name, ddraw_rec->name);
    return D3DENUMRET_OK;
}

// 0x5389f0  Confidence: likely.
// DirectDrawEnumerate callback. It returns DDENUMRET_CANCEL (`xor eax, eax` before the ret at
// 0x538a58), so only the first driver, the primary display, is ever listed.
BOOL WINAPI d3d_ddraw_enum_callback(GUID *guid, LPSTR description, LPSTR name, LPVOID context)
{
    d3d_device_record *rec = &d3d_ddraw_devices[d3d_num_ddraw_devices++];
    if (guid) {
        memmove(&rec->ddraw_guid, guid, sizeof(GUID));
        rec->ddraw_guid_ptr = &rec->ddraw_guid;
    } else {
        memset(&rec->ddraw_guid, 0, sizeof(GUID));
        rec->ddraw_guid_ptr = NULL;
    }
    strcpy(rec->name, description);
    return DDENUMRET_CANCEL;
}

// 0x538a70  Confidence: likely.
// Identity world and view; a projection with _11 = _22 = 2, _33 = zfar / (zfar - 1), _34 = 1,
// _43 = -_33 (gr_d3d_init_device passes 250).
HRESULT gr_d3d_set_default_transforms(IDirect3DDevice7 *dev, float zfar)
{
    D3DMATRIX world, view, proj;
    HRESULT hr;
    memset(&world, 0, sizeof(world));
    memset(&view, 0, sizeof(view));
    memset(&proj, 0, sizeof(proj));
    world._11 = world._22 = world._33 = world._44 = 1.0f;
    view._11 = view._22 = view._33 = view._44 = 1.0f;
    hr = IDirect3DDevice7_SetTransform(dev, D3DTRANSFORMSTATE_WORLD, &world);
    if (hr != DD_OK)
        return hr;
    hr = IDirect3DDevice7_SetTransform(dev, D3DTRANSFORMSTATE_VIEW, &view);
    if (hr != DD_OK)
        return hr;
    float q = zfar / (zfar - view._44);
    proj._11 = 2.0f;
    proj._22 = 2.0f;
    proj._33 = q;
    proj._34 = 1.0f;
    proj._43 = -q;
    proj._44 = 0.0f;
    return IDirect3DDevice7_SetTransform(dev, D3DTRANSFORMSTATE_PROJECTION, &proj);
}

// 0x538b70  Confidence: likely.
// Windowed blit rectangles from the client area (clamped to the screen size); a new window gets a clipper.
void gr_d3d_set_window(HWND hwnd)
{
    RECT rect;
    bool changed = false;
    if (hwnd != d3d_hwnd)
        changed = true;
    d3d_hwnd = hwnd;
    GetClientRect(hwnd, &rect);
    ClientToScreen(hwnd, (POINT *)&rect.left);
    ClientToScreen(hwnd, (POINT *)&rect.right);
    int w = abs(rect.right - rect.left + 1);
    int h = abs(rect.bottom - rect.top + 1);
    w = min(w, gr_screen.max_w);
    h = min(h, gr_screen.max_h);
    d3d_screen_rect.left = rect.left;
    d3d_screen_rect.right = rect.left + w - 1;
    d3d_screen_rect.top = rect.top;
    d3d_screen_rect.bottom = rect.top + h - 1;
    d3d_window_rect.left = 0;
    d3d_window_rect.top = 0;
    d3d_window_rect.right = w - 1;
    d3d_window_rect.bottom = h - 1;
    if (changed) {
        IDirectDrawClipper *clipper;
        IDirectDraw7_CreateClipper(d3d_ddraw, 0, &clipper, NULL);
        IDirectDrawClipper_SetHWnd(clipper, 0, d3d_hwnd);
        IDirectDrawSurface7_SetClipper(d3d_primary_surface, clipper);
        IDirectDrawClipper_Release(clipper);
    }
}

// 0x538c70  Confidence: likely (the compare tree is VC6's; case order = order of the bodies).
// The DirectX SDK samples' error text table (117 codes). Written with the values; the source surely used
// the DDERR_ / D3DERR_ names.
const char *ddraw_error_string(HRESULT hr)
{
    switch (hr) {
    case (HRESULT)0x8007000e:
        return "DirectDraw does not have enough memory to perform the operation.";
    case (HRESULT)0x80004005:
        return "Generic failure.";
    case (HRESULT)0x80004001:
        return "Action not supported.";
    case (HRESULT)0x80070057:
        return "One or more of the parameters passed to the function are incorrect.";
    case (HRESULT)0x88760014:
        return "This surface can not be detached from the requested surface.";
    case (HRESULT)0x8876000a:
        return "This surface can not be attached to the requested surface.";
    case (HRESULT)0x88760005:
        return "This object is already initialized.";
    case (HRESULT)0x88760028:
        return "Support is currently not available.";
    case (HRESULT)0x88760037:
        return "An exception was encountered while performing the requested operation.";
    case (HRESULT)0x8876005a:
        return "Height of rectangle provided is not a multiple of reqd alignment.";
    case (HRESULT)0x8876005f:
        return "Unable to match primary surface creation request with existing primary surface.";
    case (HRESULT)0x88760064:
        return "One or more of the caps bits passed to the callback are incorrect.";
    case (HRESULT)0x8876006e:
        return "DirectDraw does not support the provided cliplist.";
    case (HRESULT)0x88760078:
        return "DirectDraw does not support the requested mode.";
    case (HRESULT)0x88760082:
        return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.";
    case (HRESULT)0x88760091:
        return "The pixel format was invalid as specified.";
    case (HRESULT)0x88760096:
        return "Rectangle provided was invalid.";
    case (HRESULT)0x887600a0:
        return "Operation could not be carried out because one or more surfaces are locked.";
    case (HRESULT)0x887600aa:
        return "There is no 3D present.";
    case (HRESULT)0x887600b4:
        return "Operation could not be carried out because there is no alpha accleration hardware present or available.";
    case (HRESULT)0x887600cd:
        return "No cliplist available.";
    case (HRESULT)0x887600d2:
        return "Operation could not be carried out because there is no color conversion hardware present or available.";
    case (HRESULT)0x887600d7:
        return "Surface doesn't currently have a color key";
    case (HRESULT)0x887600dc:
        return "Operation could not be carried out because there is no hardware support of the destination color key.";
    case (HRESULT)0x887600d4:
        return "Create function called without DirectDraw object method SetCooperativeLevel being called.";
    case (HRESULT)0x887600e1:
        return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.";
    case (HRESULT)0x887600e6:
        return "Flipping visible surfaces is not supported.";
    case (HRESULT)0x887600f0:
        return "There is no GDI present.";
    case (HRESULT)0x887600fa:
        return "Operation could not be carried out because there is no hardware present or available.";
    case (HRESULT)0x887600ff:
        return "Requested item was not found.";
    case (HRESULT)0x88760104:
        return "Operation could not be carried out because there is no overlay hardware present or available.";
    case (HRESULT)0x88760118:
        return "Operation could not be carried out because there is no appropriate raster op hardware present or available.";
    case (HRESULT)0x88760122:
        return "Operation could not be carried out because there is no rotation hardware present or available.";
    case (HRESULT)0x88760136:
        return "Operation could not be carried out because there is no hardware support for stretching.";
    case (HRESULT)0x8876013c:
        return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.";
    case (HRESULT)0x8876013d:
        return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.";
    case (HRESULT)0x88760140:
        return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.";
    case (HRESULT)0x8876014a:
        return "Operation could not be carried out because there is no texture mapping hardware present or available.";
    case (HRESULT)0x8876014f:
        return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.";
    case (HRESULT)0x88760154:
        return "Operation could not be carried out because there is no hardware support for zbuffer blitting.";
    case (HRESULT)0x8876015e:
        return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.";
    case (HRESULT)0x88760168:
        return "The hardware needed for the requested operation has already been allocated.";
    case (HRESULT)0x8876017c:
        return "DirectDraw does not have enough memory to perform the operation.";
    case (HRESULT)0x8876017e:
        return "The hardware does not support clipped overlays.";
    case (HRESULT)0x88760180:
        return "Can only have ony color key active at one time for overlays.";
    case (HRESULT)0x88760190:
        return "No src color key specified for this operation.";
    case (HRESULT)0x88760183:
        return "Access to this palette is being refused because the palette is already locked by another thread.";
    case (HRESULT)0x8876019a:
        return "This surface is already attached to the surface it is being attached to.";
    case (HRESULT)0x887601a4:
        return "This surface is already a dependency of the surface it is being made a dependency of.";
    case (HRESULT)0x887601ae:
        return "Access to this surface is being refused because the surface is already locked by another thread.";
    case (HRESULT)0x887601b8:
        return "Access to surface refused because the surface is obscured.";
    case (HRESULT)0x887601c2:
        return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.";
    case (HRESULT)0x887601cc:
        return "The requested surface is not attached.";
    case (HRESULT)0x887601d6:
        return "Height requested by DirectDraw is too large.";
    case (HRESULT)0x887601e0:
        return "Size requested by DirectDraw is too large, but the individual height and width are OK.";
    case (HRESULT)0x887601ea:
        return "Width requested by DirectDraw is too large.";
    case (HRESULT)0x887601fe:
        return "FOURCC format requested is unsupported by DirectDraw.";
    case (HRESULT)0x88760208:
        return "Bitmask in the pixel format requested is unsupported by DirectDraw.";
    case (HRESULT)0x88760219:
        return "Vertical blank is in progress.";
    case (HRESULT)0x8876023e:
        return "Return if a clipper object is attached to the source surface passed into a BltFast call.";
    case (HRESULT)0x88760249:
        return "Windows can not create any more DCs.";
    case (HRESULT)0x88760247:
        return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.";
    case (HRESULT)0x88760237:
        return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.";
    case (HRESULT)0x88760232:
        return "A DirectDraw object representing this driver has already been created for this process.";
    case (HRESULT)0x88760245:
        return "An attempt was made to set the cooperative level when it was already set to exclusive.";
    case (HRESULT)0x8876023b:
        return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.";
    case (HRESULT)0x8876023a:
        return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.";
    case (HRESULT)0x88760231:
        return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.";
    case (HRESULT)0x88760243:
        return "Returned when the position of the overlay on the destination is no longer legal for that destination.";
    case (HRESULT)0x8876023f:
        return "No blitter hardware present.";
    case (HRESULT)0x88760238:
        return "No clipper object attached to surface object.";
    case (HRESULT)0x8876024a:
        return "No DC was ever created for this surface.";
    case (HRESULT)0x88760240:
        return "No DirectDraw ROP hardware.";
    case (HRESULT)0x88760233:
        return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.";
    case (HRESULT)0x88760235:
        return "Software emulation not available.";
    case (HRESULT)0x88760239:
        return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.";
    case (HRESULT)0x88760242:
        return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.";
    case (HRESULT)0x8876023c:
        return "No palette object attached to this surface.";
    case (HRESULT)0x8876023d:
        return "No hardware support for 16 or 256 color palettes.";
    case (HRESULT)0x88760244:
        return "Returned when an overlay member is called for a non-overlay surface.";
    case (HRESULT)0x88760246:
        return "An attempt has been made to flip a surface that is not flippable.";
    case (HRESULT)0x88760248:
        return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.";
    case (HRESULT)0x88760241:
        return "Returned when GetOverlayPosition is called on a hidden overlay.";
    case (HRESULT)0x88760234:
        return "This process already has created a primary surface.";
    case (HRESULT)0x88760236:
        return "Region passed to Clipper::GetClipList is too small.";
    case (HRESULT)0x8876021c:
        return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.";
    case (HRESULT)0x8876024b:
        return "This surface can not be restored because it was created in a different mode.";
    case (HRESULT)0x88760230:
        return "Rectangle provided was not horizontally aligned on required boundary.";
    case (HRESULT)0x8876024c:
        return "This surface can not be restored because it is an implicitly created surface.";
    case (HRESULT)0x8876024d:
        return "The surface being used is not a palette-based surface.";
    case (HRESULT)0x887602bc:
        return "D3DERR_BADMAJORVERSION";
    case (HRESULT)0x887602bd:
        return "D3DERR_BADMINORVERSION";
    case (HRESULT)0x887602ca:
        return "D3DERR_EXECUTE_LOCKED";
    case (HRESULT)0x887602cb:
        return "D3DERR_EXECUTE_NOT_LOCKED";
    case (HRESULT)0x887602c6:
        return "D3DERR_EXECUTE_CREATE_FAILED";
    case (HRESULT)0x887602c7:
        return "D3DERR_EXECUTE_DESTROY_FAILED";
    case (HRESULT)0x887602c8:
        return "D3DERR_EXECUTE_LOCK_FAILED";
    case (HRESULT)0x887602c9:
        return "D3DERR_EXECUTE_UNLOCK_FAILED";
    case (HRESULT)0x887602cc:
        return "D3DERR_EXECUTE_FAILED";
    case (HRESULT)0x887602cd:
        return "D3DERR_EXECUTE_CLIPPED_FAILED";
    case (HRESULT)0x887602d0:
        return "D3DERR_TEXTURE_NO_SUPPORT";
    case (HRESULT)0x887602d1:
        return "D3DERR_TEXTURE_CREATE_FAILED";
    case (HRESULT)0x887602d2:
        return "D3DERR_TEXTURE_DESTROY_FAILED";
    case (HRESULT)0x887602d3:
        return "D3DERR_TEXTURE_LOCK_FAILED";
    case (HRESULT)0x887602d8:
        return "D3DERR_TEXTURE_NOT_LOCKED";
    case (HRESULT)0x887602d7:
        return "D3DERR_TEXTURELOCKED";
    case (HRESULT)0x887602d4:
        return "D3DERR_TEXTURE_UNLOCK_FAILED";
    case (HRESULT)0x887602d5:
        return "D3DERR_TEXTURE_LOAD_FAILED";
    case (HRESULT)0x887602da:
        return "D3DERR_MATRIX_CREATE_FAILED";
    case (HRESULT)0x887602db:
        return "D3DERR_MATRIX_DESTROY_FAILED";
    case (HRESULT)0x887602dc:
        return "D3DERR_MATRIX_SETDATA_FAILED";
    case (HRESULT)0x887602de:
        return "D3DERR_SETVIEWPORTDATA_FAILED";
    case (HRESULT)0x887602e4:
        return "D3DERR_MATERIAL_CREATE_FAILED";
    case (HRESULT)0x887602e5:
        return "D3DERR_MATERIAL_DESTROY_FAILED";
    case (HRESULT)0x887602e6:
        return "D3DERR_MATERIAL_SETDATA_FAILED";
    case (HRESULT)0x887602ee:
        return "D3DERR_LIGHT_SET_FAILED";
    case DD_OK:
        return "No error.";
    }
    return "Unrecognized error value.";
}

// 0x5394e0  Confidence: likely.
void gr_d3d_init(HWND hwnd)
{
    D3DVERTEXBUFFERDESC desc;
    d3d_hwnd = hwnd;
    d3d_init_done = 0;
    gr_screen.bits_per_pixel = 16;
    gr_screen.bytes_per_pixel = 2;
    gr_d3d_init_device();
    gr_d3d_tcache_init();
    gr_d3d_begin_scene();
    desc.dwSize = sizeof(desc);
    desc.dwCaps = d3d_use_vertex_buffer ? (D3DVBCAPS_WRITEONLY | D3DVBCAPS_SYSTEMMEMORY | D3DVBCAPS_DONOTCLIP)
                                        : (D3DVBCAPS_WRITEONLY | D3DVBCAPS_SYSTEMMEMORY);
    desc.dwFVF = D3DFVF_TLVERTEX2;
    desc.dwNumVertices = 1000;
    if (IDirect3D7_CreateVertexBuffer(D3D_DIRECT3D, &desc, &d3d_vertex_buffer, 0) < 0)
        VSDK_ERROR(GR_DIRECT3D_CPP, 0x8aa, "Unable to create character vertex buffer for D3d!");
    d3d_init_done = 1;
}

// 0x539590  Confidence: likely (an empty case 1: the compare tree tests depth 1 separately).
void gr_d3d_begin_scene()
{
    if (d3d_device_ready) {
        switch (d3d_scene_depth) {
        case 0:
            d3d_scene_depth++;
            IDirect3DDevice7_BeginScene(d3d_device);
            break;
        case 1:
            break;
        }
    }
}

// 0x5395c0  Confidence: unsure (long; block layout, the shared failure exit and the Voodoo message that
// the binary places after the epilogue, 0x53a125).
// Loads ddraw.dll, picks the last hardware device of the primary driver, creates the surfaces (windowed:
// primary + offscreen render target with a clipper; fullscreen: a flipping chain at 16 bpp), the z-buffer
// and the device, checks the caps the game needs and sets the default states. Any DirectX failure:
// gr_d3d_close and exit(0).
void gr_d3d_init_device()
{
    IDirectDrawClipper *clipper;
    DDSCAPS2 caps;
    DWORD optimizations;
    D3DVIEWPORT7 vp;
    DDSURFACEDESC2 desc;
    DDSURFACEDESC2 zdesc;
    DDPIXELFORMAT zformat;
    DDDEVICEIDENTIFIER2 id;
    char error[256];
    char msg[0x20000];
    HMODULE ddraw_dll;
    d3d_device_record *dev;
    HWND hwnd;
    DWORD raster;
    int missing;
    DWORD total_mem, free_mem;
    float fog_start, fog_end;

    ddraw_dll = LoadLibraryA("ddraw.dll");
    if (!ddraw_dll)
        return;
    os_registry_read_dword(NULL, "D3D Optimizations", &optimizations, 0);
    d3d_use_vertex_buffer = optimizations != 0;
    d3d_unused_2dc05dc = 0;
    d3d_ddraw_enumerate_ex = (LPDIRECTDRAWCREATEEX)GetProcAddress(ddraw_dll, "DirectDrawCreateEx");
    if (!d3d_ddraw_enumerate_ex)
        return;

    switch (gr_screen.window_mode) {
    case 200:
    case 202:
        d3d_windowed = true;
        break;
    case 201:
        d3d_windowed = false;
        break;
    default:
        VSDK_ERROR(GR_DIRECT3D_CPP, 0x41d, "Invalid window_mode passed to gr_direct3d_init\n");
    }

    dev = d3d_enumerate_ddraw_devices();
    if (!dev)
        VSDK_ERROR(GR_DIRECT3D_CPP, 0x425, "No Direct3D devices found!\n");

    hwnd = d3d_hwnd;
    if (d3d_ddraw_enumerate_ex(dev->ddraw_guid_ptr, (void **)&d3d_ddraw, &IID_IDirectDraw7, NULL) != DD_OK)
        goto failed;
    if (IDirectDraw7_QueryInterface(d3d_ddraw, &IID_IDirect3D7, &d3d_direct3d) != DD_OK)
        goto failed;
    if (IDirectDraw7_GetDeviceIdentifier(d3d_ddraw, &id, 0) != DD_OK)
        goto failed;

    if (d3d_windowed) {
        GetClientRect(hwnd, &d3d_screen_rect);
        GetClientRect(hwnd, &d3d_window_rect);
        ClientToScreen(hwnd, (POINT *)&d3d_screen_rect.left);
        ClientToScreen(hwnd, (POINT *)&d3d_screen_rect.right);
        if (IDirectDraw7_SetCooperativeLevel(d3d_ddraw, hwnd, DDSCL_NORMAL) != DD_OK)
            goto failed;
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        desc.dwFlags = DDSD_CAPS;
        desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        if (IDirectDraw7_CreateSurface(d3d_ddraw, &desc, &d3d_primary_surface, NULL) != DD_OK)
            goto failed;
        memset(&desc, 0, sizeof(desc));
        desc.dwWidth = gr_screen.max_w;
        desc.dwHeight = gr_screen.max_h;
        desc.dwSize = sizeof(desc);
        desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
        if (IDirectDraw7_CreateSurface(d3d_ddraw, &desc, &d3d_back_surface, NULL) != DD_OK)
            goto failed;
        if (IDirectDraw7_CreateClipper(d3d_ddraw, 0, &clipper, NULL) != DD_OK)
            goto failed;
        IDirectDrawClipper_SetHWnd(clipper, 0, hwnd);
        IDirectDrawSurface7_SetClipper(d3d_primary_surface, clipper);
        IDirectDrawClipper_Release(clipper);
    } else {
        if (IDirectDraw7_SetCooperativeLevel(d3d_ddraw, hwnd,
                                             DDSCL_EXCLUSIVE | DDSCL_NOWINDOWCHANGES | DDSCL_FULLSCREEN) != DD_OK)
            goto failed;
        if (IDirectDraw7_SetDisplayMode(d3d_ddraw, gr_screen.max_w, gr_screen.max_h, 16, 0, 0) != DD_OK)
            goto failed;
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
        desc.dwBackBufferCount = 1;
        if (IDirectDraw7_CreateSurface(d3d_ddraw, &desc, &d3d_primary_surface, NULL) != DD_OK)
            goto failed;
        memset(&caps, 0, sizeof(caps));
        caps.dwCaps = DDSCAPS_BACKBUFFER;
        if (IDirectDrawSurface7_GetAttachedSurface(d3d_primary_surface, &caps, &d3d_back_surface) != DD_OK)
            goto failed;
        gr_d3d_noop_hook(1);
    }

    // The z-buffer: the device's first plain z format, in video memory for the HAL device.
    IDirect3D7_EnumZBufferFormats(D3D_DIRECT3D, &dev->d3d_guid, d3d_enum_zbuffer_callback, &zformat);
    memset(&zdesc, 0, sizeof(zdesc));
    zdesc.dwHeight = gr_screen.max_h;
    zdesc.dwWidth = gr_screen.max_w;
    zdesc.dwSize = sizeof(zdesc);
    zdesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    zdesc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    zdesc.ddpfPixelFormat = zformat;
    if (IsEqualGUID(dev->d3d_guid, IID_IDirect3DHALDevice))    // 0x53ac00 guid_equal
        zdesc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
    else
        zdesc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    if (IDirectDraw7_CreateSurface(d3d_ddraw, &zdesc, &d3d_zbuffer_surface, NULL) != DD_OK)
        goto failed;
    if (IDirectDrawSurface7_AddAttachedSurface(d3d_back_surface, d3d_zbuffer_surface) != DD_OK)
        goto failed;
    if (IDirect3D7_CreateDevice(D3D_DIRECT3D, &dev->d3d_guid, d3d_back_surface, &d3d_device) != DD_OK)
        goto failed;

    vp.dwX = 0;
    vp.dwY = 0;
    vp.dwWidth = gr_screen.max_w;
    vp.dwHeight = gr_screen.max_h;
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 0.0f;
    gr_d3d_set_default_transforms(d3d_device, 250.0f);
    if (IDirect3DDevice7_SetViewport(d3d_device, &vp) != DD_OK)
        goto failed;

    memset(&zdesc, 0, sizeof(zdesc));
    zdesc.dwSize = sizeof(zdesc);
    IDirectDrawSurface7_GetSurfaceDesc(d3d_back_surface, &zdesc);
    d3d_compute_pixel_format_shifts(&zdesc);
    d3d_screen_pixel_format = zdesc.ddpfPixelFormat;

    d3d_texture_format_b_bits = 0;
    d3d_texture_format_a_bits = 0;
    if (IDirect3DDevice7_EnumTextureFormats(d3d_device, d3d_enum_texture_format_callback, NULL) != DD_OK)
        goto failed;
    if (!d3d_texture_format_b_bits) {
        gr_d3d_close();
        MessageBox(NULL, "Alpha channel textures", "Missing Features", D3D_MB_FLAGS);
        exit(1);
    }
    if (!d3d_texture_format_a_bits) {
        gr_d3d_close();
        MessageBox(NULL, "16-bpp RGB textures", "Missing Features", D3D_MB_FLAGS);
        exit(1);
    }
    d3d_pixel_format_masks_valid(&d3d_texture_format_b);
    d3d_pixel_format_masks_valid(&d3d_texture_format_a);
    d3d_pixel_format_masks_valid(&d3d_screen_pixel_format);

    memset(&d3d_device_caps, 0, sizeof(d3d_device_caps));
    if (IDirect3DDevice7_GetCaps(d3d_device, &d3d_device_caps) != DD_OK) {
    failed:
        gr_d3d_close();
        exit(0);
    }
    d3d_device_desc = &d3d_device_caps;
    if (!(d3d_device_caps.dwDevCaps & D3DDEVCAPS_DRAWPRIMITIVES2EX))
        MessageBox(NULL,
                   "These drivers are not compliant with DirectX 7.  Please get the latest drivers from your "
                   "hardware vendor's web site.",
                   "Warning", D3D_MB_FLAGS);

    if (d3d_device_desc->wMaxTextureBlendStages >= 2 && d3d_device_desc->wMaxSimultaneousTextures >= 2)
        d3d_multitexture_supported = true;
    else
        d3d_multitexture_supported = false;

    d3d_min_texture_w = d3d_device_desc->dwMinTextureWidth;
    d3d_max_texture_w = d3d_device_desc->dwMaxTextureWidth;
    d3d_min_texture_h = d3d_device_desc->dwMinTextureHeight;
    d3d_max_texture_h = d3d_device_desc->dwMaxTextureHeight;
    if (d3d_min_texture_w < 16)
        d3d_min_texture_w = 16;
    if (d3d_min_texture_h < 16)
        d3d_min_texture_h = 16;
    if (d3d_max_texture_w < 16)
        d3d_max_texture_w = 256;
    if (d3d_max_texture_h < 16)
        d3d_max_texture_h = 256;
    gr_screen.max_bitmap_h = d3d_max_texture_h;
    d3d_pow2_textures_only = (d3d_device_desc->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) ? 1 : 0;
    d3d_square_textures_only = (d3d_device_desc->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) ? 1 : 0;
    d3d_driver_caps.dwSize = d3d_hel_caps.dwSize = sizeof(DDCAPS);
    gr_screen.max_bitmap_w = d3d_max_texture_w;
    IDirectDraw7_GetCaps(d3d_ddraw, &d3d_driver_caps, &d3d_hel_caps);

    // The blend modes the game cannot do without.
    strcpy(msg, XSTR("Your video card is missing the following features required by Summoner:\r\n\r\n", 938));
    missing = 0;
    if (!(d3d_device_desc->dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATE)) {
        strcat(msg, XSTR("Texture blending mode = Modulate\r\n", 939));
        missing++;
    }
    if (!(d3d_device_desc->dpcTriCaps.dwSrcBlendCaps & (D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_BOTHSRCALPHA))) {
        strcat(msg, XSTR("Source blending mode = SRCALPHA or BOTHSRCALPHA\r\n", 940));
        missing++;
    }
    if (!(d3d_device_desc->dpcTriCaps.dwDestBlendCaps & (D3DPBLENDCAPS_INVSRCALPHA | D3DPBLENDCAPS_BOTHINVSRCALPHA))) {
        strcat(msg, XSTR("Destination blending mode = INVSRCALPHA or BOTHINVSRCALPHA\r\n", 941));
        missing++;
    }
    if (d3d_texture_format_b_bits < 4)
        d3d_device_desc->dpcTriCaps.dwTextureBlendCaps &= ~D3DPTBLENDCAPS_MODULATEALPHA;
    if (missing) {
        gr_d3d_close();
        MessageBox(NULL, msg, XSTR("Missing Features", 942), D3D_MB_FLAGS);
        exit(1);
    }

    if (d3d_driver_caps.dwCaps2 & DDCAPS2_PRIMARYGAMMA) {
        if (IDirectDrawSurface7_QueryInterface(d3d_primary_surface, &IID_IDirectDrawGammaControl,
                                               (void **)&d3d_gamma_control) < 0)
            d3d_gamma_control = NULL;
        else
            IDirectDrawGammaControl_GetGammaRamp(d3d_gamma_control, 0, &d3d_saved_gamma_ramp);
    }

    raster = d3d_device_desc->dpcTriCaps.dwRasterCaps;
    if (raster & D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT)
        gr_antialias_enabled = true;
    d3d_fog_mode = 0;
    if (raster & D3DPRASTERCAPS_FOGTABLE) {
        if ((raster & D3DPRASTERCAPS_WFOG) && id.dwVendorId != 0x102b)     // not Matrox
            d3d_fog_mode = 2;
        else
            d3d_fog_mode = 1;
    } else {
        fog_start = 1.0f;
        fog_end = 1.0f;
        if (raster & D3DPRASTERCAPS_FOGVERTEX) {
            d3d_fog_mode = 3;
        } else {
            gr_d3d_set_render_state(D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_NONE);
            gr_d3d_set_render_state(D3DRENDERSTATE_FOGENABLE, TRUE);
            gr_d3d_set_render_state(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);
            gr_d3d_set_render_state(D3DRENDERSTATE_FOGSTART, *(DWORD *)&fog_start);
            gr_d3d_set_render_state(D3DRENDERSTATE_FOGEND, *(DWORD *)&fog_end);
            gr_d3d_set_render_state(D3DRENDERSTATE_FOGENABLE, FALSE);
        }
    }

    if (stristr(id.szDriver, "3dfx") || stristr(id.szDescription, "Voodoo")
        || stristr(id.szDescription, "Banshee")) {
        gr_d3d_close();
        sprintf(error, XSTR("Error -- %s -- This Voodoo card cannot be run in Direct3D mode.  Use Glide instead.", 943),
                id.szDescription);
        MessageBox(NULL, error, XSTR("Error", 715), D3D_MB_FLAGS);
        exit(1);
    }

    // Per-vendor depth and fog settings: everything but S3 draws with the reversed depth test.
    if (id.dwVendorId == 0x1002) {        // ATI
        d3d_fog_mode = 3;
        if (id.dwDeviceId == 0x5246)       // Rage 128
            gr_byte_25d921d = true;
        else if (id.dwDeviceId == 0x5144)  // Radeon
            d3d_ati_5144 = true;
    }
    if (id.dwVendorId == 0x5333) {        // S3
        Gr_far_min *= 0.1f;
        d3d_fog_mode = 5;
        d3d_s3_depth = true;
        d3d_zclear_value = 1.0f;
        d3d_zfunc = D3DCMP_LESSEQUAL;
        Gr_far_max *= 0.1f;
    } else {
        d3d_zclear_value = 0.0f;
        d3d_zfunc = D3DCMP_GREATEREQUAL;
    }
    if (cmdline_param_rz.found())
        d3d_zfunc = (d3d_zfunc != D3DCMP_LESSEQUAL) ? D3DCMP_LESSEQUAL : D3DCMP_GREATEREQUAL;
    if (cmdline_param_fb2.found())
        d3d_fog_mode = 5;
    if (cmdline_param_vertexfog.found())
        d3d_fog_mode = 3;

    memset(&caps, 0, sizeof(caps));
    caps.dwCaps = DDSCAPS_TEXTURE;
    d3d_flag_2dc2960 = 1;
    if (IDirectDraw7_GetAvailableVidMem(d3d_ddraw, &caps, &total_mem, &free_mem) != DD_OK)
        free_mem = 0;
    if (free_mem < 0x500000) {
        gr_d3d_close();
        MessageBox(NULL, XSTR("At least 8 MB of available video memory required.", 944),
                   XSTR("Missing Features", 942), D3D_MB_FLAGS);
        exit(1);
    }
    gr_d3d_video_memory_mb = free_mem;

    gr_d3d_set_render_state(D3DRENDERSTATE_DITHERENABLE, FALSE);
    gr_d3d_set_render_state(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    gr_d3d_set_render_state(D3DRENDERSTATE_TEXTUREMIN, D3DFILTER_LINEAR);
    gr_d3d_set_render_state(D3DRENDERSTATE_TEXTUREMAG, D3DFILTER_LINEAR);
    gr_d3d_set_render_state(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    gr_d3d_set_render_state(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
    gr_d3d_set_render_state(D3DRENDERSTATE_SPECULARENABLE, FALSE);
    gr_d3d_set_render_state(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATER);
    if (d3d_use_vertex_buffer) {
        gr_d3d_set_render_state(D3DRENDERSTATE_CLIPPING, FALSE);
        gr_d3d_set_render_state(D3DRENDERSTATE_LIGHTING, FALSE);
        gr_d3d_lock_discard = DDLOCK_DISCARDCONTENTS | DDLOCK_WRITEONLY;
        gr_d3d_lock_nooverwrite = DDLOCK_NOOVERWRITE | DDLOCK_WRITEONLY;
    }
    d3d_vertex_buffer_pos = 0;
    levelrender_d3d_vertex_buffer_pos = 0;
    gr_d3d_set_render_state(D3DRENDERSTATE_ALPHAREF, d3d_use_vertex_buffer ? 0x18 : 0);
    IDirect3DDevice7_SetTextureStageState(d3d_device, 0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
    IDirect3DDevice7_SetTextureStageState(d3d_device, 1, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
    d3d_device_ready = 1;
}

// 0x53a170  Confidence: likely (the counters are dead, the loops stay).
// Walks the R, G, B (and alpha) masks to their first and past their last bit; nothing is kept. A zero
// mask would never end the first loop.
void d3d_pixel_format_masks_valid(DDPIXELFORMAT *pf)
{
    DWORD m;
    for (m = pf->dwRBitMask; !(m & 1); m >>= 1)
        ;
    for (; m & 1; m >>= 1)
        ;
    for (m = pf->dwGBitMask; !(m & 1); m >>= 1)
        ;
    for (; m & 1; m >>= 1)
        ;
    for (m = pf->dwBBitMask; !(m & 1); m >>= 1)
        ;
    for (; m & 1; m >>= 1)
        ;
    if (pf->dwFlags & DDPF_ALPHAPIXELS) {
        for (m = pf->dwRGBAlphaBitMask; !(m & 1); m >>= 1)
            ;
        for (; m & 1; m >>= 1)
            ;
    }
}

// Bits set in a contiguous mask (0 for 0). A macro (name ours): the binary repeats the two loops for
// every mask, and nothing in these objects is inlined, so it is not a function.
#define D3D_MASK_BITS(mask, bits) \
    do { DWORD m_ = (mask); (bits) = 0; \
         if (m_) { while (!(m_ & 1)) m_ >>= 1; while (m_ & 1) { m_ >>= 1; (bits)++; } } } while (0)

// 0x53a1d0  Confidence: unsure (the bit counting is a macro of ours, D3D_MASK_BITS).
// EnumTextureFormats callback. Keeps a 16-bit du/dv format with 8 + 8 bits (bump mapping), the 16-bit RGB
// format with the most colour bits (a) and the format of at most 16 bits with the most alpha bits under
// 8 (b).
HRESULT CALLBACK d3d_enum_texture_format_callback(DDPIXELFORMAT *pf, LPVOID context)
{
    DWORD flags = pf->dwFlags;
    if (flags & DDPF_BUMPDUDV) {
        int du, dv;
        D3D_MASK_BITS(pf->dwRBitMask, du);
        D3D_MASK_BITS(pf->dwGBitMask, dv);
        if (pf->dwRGBBitCount == 16 && du == 8 && dv == du) {
            d3d_texture_format_c = *pf;
            d3d_texture_format_c_found = true;
            return D3DENUMRET_OK;
        }
    } else if (flags & DDPF_RGB) {
        int r, g, b, a;
        D3D_MASK_BITS(pf->dwRBitMask, r);
        D3D_MASK_BITS(pf->dwGBitMask, g);
        D3D_MASK_BITS(pf->dwBBitMask, b);
        if (flags & DDPF_ALPHAPIXELS)
            D3D_MASK_BITS(pf->dwRGBAlphaBitMask, a);
        else
            a = 0;
        if (r > 0 && g > 0 && b > 0 && pf->dwRGBBitCount == 16) {
            int total = r + g + b;
            if (total > d3d_texture_format_a_bits) {
                d3d_texture_format_a_bits = total;
                d3d_texture_format_a = *pf;
            }
        }
        if (a > 0 && a < 8 && pf->dwRGBBitCount <= 16 && a > d3d_texture_format_b_bits) {
            d3d_texture_format_b_bits = a;
            d3d_texture_format_b = *pf;
        }
    }
    return D3DENUMRET_OK;
}

// 0x53a360  Confidence: exact.
// Empty; called with 1 after the fullscreen surfaces are made and on each texture flush, 0 at close.
void gr_d3d_noop_hook(int arg)
{
}

// 0x53a370  Confidence: likely (the stores are grouped at the end in the binary).
// Shift, width and 255 / max of the render target's R, G, B masks (the alpha mask is walked, not kept).
void d3d_compute_pixel_format_shifts(DDSURFACEDESC2 *desc)
{
    DWORD r_mask = desc->ddpfPixelFormat.dwRBitMask;
    DWORD g_mask = desc->ddpfPixelFormat.dwGBitMask;
    DWORD b_mask = desc->ddpfPixelFormat.dwBBitMask;
    DWORD m;
    int r_shift = 0, r_bits = 0, g_shift = 0, g_bits = 0, b_shift = 0, b_bits = 0;

    for (m = r_mask; !(m & 1); m >>= 1)
        r_shift++;
    for (; m & 1; m >>= 1)
        r_bits++;
    int r_scale = 255 / (r_mask >> r_shift);
    for (m = g_mask; !(m & 1); m >>= 1)
        g_shift++;
    for (; m & 1; m >>= 1)
        g_bits++;
    int g_scale = 255 / (g_mask >> g_shift);
    for (m = b_mask; !(m & 1); m >>= 1)
        b_shift++;
    for (; m & 1; m >>= 1)
        b_bits++;
    int b_scale = 255 / (b_mask >> b_shift);
    if (desc->ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) {
        for (m = desc->ddpfPixelFormat.dwRGBAlphaBitMask; !(m & 1); m >>= 1)
            ;
        for (; m & 1; m >>= 1)
            ;
    }

    d3d_red_channel.bits = r_bits;
    d3d_red_channel.shift = r_shift;
    d3d_red_channel.scale = r_scale;
    d3d_red_channel.mask = r_mask;
    d3d_green_channel.bits = g_bits;
    d3d_green_channel.shift = g_shift;
    d3d_green_channel.scale = g_scale;
    d3d_green_channel.mask = g_mask;
    d3d_blue_channel.bits = b_bits;
    d3d_blue_channel.shift = b_shift;
    d3d_blue_channel.scale = b_scale;
    d3d_blue_channel.mask = b_mask;
}

// 0x53a4c0  Confidence: exact.
// EnumZBufferFormats callback: the first plain z format (no stencil) ends the enumeration.
HRESULT CALLBACK d3d_enum_zbuffer_callback(DDPIXELFORMAT *pf, LPVOID context)
{
    if (pf->dwFlags == DDPF_ZBUFFER) {
        *(DDPIXELFORMAT *)context = *pf;
        return D3DENUMRET_CANCEL;
    }
    return D3DENUMRET_OK;
}

// 0x53a4f0  Confidence: likely.
// Clears and shows a black frame, frees the texture cache, puts the gamma ramp back and releases the
// objects. d3d_vertex_buffer is not released.
void gr_d3d_close()
{
    if (d3d_device_ready) {
        gr::reset_clip();
        gr_clear();
        gr_d3d_flip_surface();
        gr_d3d_tcache_close();
        if (d3d_gamma_control) {
            IDirectDrawGammaControl_SetGammaRamp(d3d_gamma_control, 0, &d3d_saved_gamma_ramp);
            IDirectDrawGammaControl_Release(d3d_gamma_control);
            d3d_gamma_control = NULL;
        }
        if (d3d_device) {
            IDirect3DDevice7_Release(d3d_device);
            d3d_device = NULL;
        }
        if (d3d_zbuffer_surface) {
            IDirectDrawSurface7_Release(d3d_zbuffer_surface);
            d3d_zbuffer_surface = NULL;
        }
        if (d3d_back_surface) {
            IDirectDrawSurface7_Release(d3d_back_surface);
            d3d_back_surface = NULL;
        }
        if (d3d_primary_surface) {
            IDirectDrawSurface7_Release(d3d_primary_surface);
            d3d_primary_surface = NULL;
        }
        if (d3d_direct3d) {
            IDirect3D7_Release(D3D_DIRECT3D);
            d3d_direct3d = NULL;
        }
        if (d3d_ddraw) {
            HWND hwnd = os_get_main_wnd();
            IDirectDraw7_RestoreDisplayMode(d3d_ddraw);
            IDirectDraw7_SetCooperativeLevel(d3d_ddraw, hwnd, DDSCL_NORMAL);
            gr_d3d_noop_hook(0);
            IDirectDraw7_Release(d3d_ddraw);
            d3d_ddraw = NULL;
        }
        d3d_device_ready = 0;
    }
}

// 0x53a5f0  Confidence: exact.
void gr_d3d_flip_surface()
{
    gr_d3d_end_scene();
    IDirectDrawSurface7_Flip(d3d_primary_surface, NULL, DDFLIP_WAIT);
}

// 0x53a610  Confidence: likely (empty case 0, as in gr_d3d_begin_scene).
void gr_d3d_end_scene()
{
    if (d3d_device_ready) {
        switch (d3d_scene_depth) {
        case 0:
            break;
        case 1:
            d3d_scene_depth--;
            IDirect3DDevice7_EndScene(d3d_device);
            break;
        }
    }
}

// 0x53a640  Confidence: likely (a tail jump; why the wrapper exists is not known).
void gr_d3d_close_thunk()
{
    gr_d3d_close();
}

// 0x53a650  Confidence: likely.
void gr_d3d_flip_thunk()
{
    gr_d3d_flip();
}

// 0x53a660  Confidence: likely.
// Windowed: blit the render target into the window and wait. Fullscreen: restore lost surfaces and flip
// (vsync unless gr_vsync is off), retrying every second while the surfaces stay lost.
void gr_d3d_flip()
{
    gr::reset_clip();
    gr_d3d_end_scene();
    if (d3d_windowed) {
        IDirectDrawSurface7_Blt(d3d_primary_surface, &d3d_screen_rect, d3d_back_surface, &d3d_window_rect,
                                DDBLT_WAIT, NULL);
        while (IDirectDrawSurface7_GetBltStatus(d3d_primary_surface, DDGBS_ISBLTDONE) != DD_OK)
            ;
    } else {
        for (;;) {
            if (IDirectDrawSurface7_IsLost(d3d_primary_surface) == DDERR_SURFACELOST)
                IDirectDrawSurface7_Restore(d3d_primary_surface);
            if (IDirectDrawSurface7_IsLost(d3d_back_surface) == DDERR_SURFACELOST)
                IDirectDrawSurface7_Restore(d3d_back_surface);
            if (IDirectDrawSurface7_IsLost(d3d_zbuffer_surface) == DDERR_SURFACELOST)
                IDirectDrawSurface7_Restore(d3d_zbuffer_surface);
            HRESULT hr = IDirectDrawSurface7_Flip(d3d_primary_surface, NULL,
                                                  gr_vsync ? DDFLIP_WAIT : DDFLIP_WAIT | DDFLIP_NOVSYNC);
            if (hr == DDERR_SURFACELOST) {
                timer_wait(1000);
                continue;
            }
            if (hr == DD_OK) {
                while (IDirectDrawSurface7_GetFlipStatus(d3d_primary_surface, DDGFS_ISFLIPDONE) != DD_OK)
                    ;
            }
            break;
        }
    }
    gr_d3d_tcache_frame();
    int n = d3d_flush_request;
    if (n) {
        d3d_flush_request -= n;
        gr_d3d_tcache_flush();
        gr_d3d_noop_hook(1);
    }
    n = d3d_flush_request_b;
    if (n) {
        d3d_flush_request_b -= n;
        gr_d3d_noop_hook(0);
    }
    if (d3d_use_vertex_buffer) {
        d3d_vertex_buffer_pos = 0;
        levelrender_d3d_vertex_buffer_pos = 0;
    }
    gr_d3d_begin_scene();
}

// 0x53a7f0  Confidence: likely.
// Copies a rectangle of the render target, assumed 16 bpp (returns 3, the 565 format), 0 if it cannot
// be locked.
int gr_d3d_read_backbuffer(int x, int y, int w, int h, void *buf)
{
    DDSURFACEDESC2 desc;
    gr_d3d_end_scene();
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    if (IDirectDrawSurface7_Lock(d3d_back_surface, NULL, &desc, DDLOCK_WAIT | DDLOCK_READONLY, NULL) != DD_OK)
        return 0;
    ubyte *dst = (ubyte *)buf;
    ubyte *src = (ubyte *)desc.lpSurface + desc.lPitch * y + x * 2;
    for (int i = 0; i < h; i++) {
        memcpy(dst, src, w * 2);
        dst += w * 2;
        src += desc.lPitch;
    }
    IDirectDrawSurface7_Unlock(d3d_back_surface, NULL);
    gr_d3d_begin_scene();
    return 3;
}

// 0x53a8c0  Confidence: likely.
// gr_set_fog's back end (it passes 1): table or vertex fog, the range in the units of the fog mode, the
// colour. FOGSTART / FOGEND are sent only when they change.
void gr_d3d_set_fog(int set_range)
{
    if (gr_fog_enabled && d3d_fog_mode) {
        gr_d3d_set_render_state(D3DRENDERSTATE_FOGENABLE, TRUE);
        if (d3d_fog_mode == 3) {
            gr_d3d_set_render_state(D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_LINEAR);
        } else {
            gr_d3d_set_render_state(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);
            if (set_range) {
                if (d3d_fog_mode == 5) {
                    d3d_fog_start = gr_fog_near * 100.0f;
                    d3d_fog_end = gr_fog_far * 100.0f;
                } else if (d3d_fog_mode == 4) {
                    d3d_fog_start = 1.0f;
                    d3d_fog_end = 1.0f;
                } else if (d3d_fog_mode == 2) {
                    d3d_fog_start = gr_scale_by_far_clip(gr_fog_near);
                    d3d_fog_end = gr_scale_by_far_clip(gr_fog_far);
                } else {
                    d3d_fog_start = gr_calc_depth(1.0f / gr_scale_by_far_clip(gr_fog_near));
                    d3d_fog_end = gr_calc_depth(1.0f / gr_scale_by_far_clip(gr_fog_far));
                }
                if (d3d_last_fog_start != d3d_fog_start) {
                    d3d_last_fog_start = d3d_fog_start;
                    gr_d3d_set_render_state(D3DRENDERSTATE_FOGSTART, *(DWORD *)&d3d_fog_start);
                }
                if (d3d_last_fog_end != d3d_fog_end) {
                    d3d_last_fog_end = d3d_fog_end;
                    gr_d3d_set_render_state(D3DRENDERSTATE_FOGEND, *(DWORD *)&d3d_fog_end);
                }
            }
        }
        gr_d3d_set_render_state(D3DRENDERSTATE_FOGCOLOR,
                                D3DRGB(gr_fog_color.r / 255.0f, gr_fog_color.g / 255.0f, gr_fog_color.b / 255.0f));
    } else {
        gr_d3d_set_render_state(D3DRENDERSTATE_FOGENABLE, FALSE);
        if (d3d_fog_mode == 3)
            gr_d3d_set_render_state(D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_NONE);
        else
            gr_d3d_set_render_state(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);
    }
}

// 0x53aad0  Confidence: exact.
bool gr_d3d_is_mode_supported(gr::Mode *mode)
{
    if (d3d_multitexture_supported)
        return true;
    return !mode->is_multitexture();
}

// 0x53aaf0  Confidence: exact.
int gr_d3d_get_video_memory_mb()
{
    return gr_d3d_video_memory_mb;
}

// 0x53ab00  Confidence: exact.
void gr_d3d_set_antialias(bool on)
{
    gr_d3d_set_render_state(D3DRENDERSTATE_ANTIALIAS, on ? D3DANTIALIAS_SORTINDEPENDENT : 0);
}

// 0x53ab20  Confidence: exact.
// gr_gamma_ramp (0..255) as the 16-bit ramp of all three channels.
void gr_d3d_set_gamma()
{
    DDGAMMARAMP ramp;
    if (d3d_gamma_control) {
        for (int i = 0; i < 256; i++)
            ramp.red[i] = ramp.green[i] = ramp.blue[i] = (WORD)(gr_gamma_ramp[i] << 8);
        IDirectDrawGammaControl_SetGammaRamp(d3d_gamma_control, 0, &ramp);
    }
}

// 0x53ab80  Confidence: exact.
void gr_d3d_set_alpha_test(bool on)
{
    if (on)
        gr_d3d_set_render_state(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATER);
    else
        gr_d3d_set_render_state(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_ALWAYS);
}

// 0x53abb0  Confidence: exact.
// Fullscreen: copies the front surface into the render target and waits.
void gr_d3d_flush_pipeline()
{
    if (!d3d_windowed) {
        IDirectDrawSurface7_Blt(d3d_back_surface, NULL, d3d_primary_surface, NULL, DDBLT_WAIT, NULL);
        while (IDirectDrawSurface7_GetBltStatus(d3d_back_surface, DDGBS_ISBLTDONE) != DD_OK)
            ;
    }
}

// 0x53ac00 guid_equal: IsEqualGUID (objbase.h inline; called in gr_d3d_init_device), out of line at the
// end of the object, not source here.  Confidence: exact.
