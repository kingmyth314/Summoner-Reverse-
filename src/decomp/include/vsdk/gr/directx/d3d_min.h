// vsdk/gr/directx/d3d_min.h - the DirectDraw 7 / Direct3D 7 declarations gr_direct3d.cpp uses (ours; it
// stands in for <ddraw.h> / <d3d.h> / <d3dtypes.h>, DECISIONS.md D1600). Not original source.
//
// The interfaces are C-style COM objects (a pointer to a table of __stdcall function pointers, `this`
// passed first): gr_d3d_set_bump_env loads d3d_device's vtable before calling d3d_f2dw and loads the
// global d3d_device again for the `this` argument (0x5367ac-0x5367cd), which is the expansion of the
// C macros `IDirect3DDevice7_SetTextureStageState(p, ...)` = `(p)->lpVtbl->SetTextureStageState(p, ...)`,
// not a C++ virtual call (that evaluates the object once). Slot numbers are from the calls in
// Sum.exe; slots the game never calls keep their SDK names as `void *` placeholders.
//
// Do not include this header together with the system DirectX headers (levelrender.cpp includes
// <d3d.h>): the type names are the SDK's.
#ifndef VSDK_GR_DIRECTX_D3D_MIN_H
#define VSDK_GR_DIRECTX_D3D_MIN_H

#include <windows.h>

// ---- basic types --------------------------------------------------------------------------------------
typedef float D3DVALUE;
typedef DWORD D3DCOLOR;

// ---- DirectDraw structures ----------------------------------------------------------------------------
struct DDPIXELFORMAT {                // 0x20
    DWORD dwSize;                     // +0x00
    DWORD dwFlags;                    // +0x04 DDPF_*
    DWORD dwFourCC;                   // +0x08
    DWORD dwRGBBitCount;              // +0x0C also dwZBufferBitDepth, dwBumpBitCount
    DWORD dwRBitMask;                 // +0x10 also dwBumpDuBitMask
    DWORD dwGBitMask;                 // +0x14 also dwBumpDvBitMask
    DWORD dwBBitMask;                 // +0x18 also dwBumpLuminanceBitMask
    DWORD dwRGBAlphaBitMask;          // +0x1C
};
struct DDSCAPS2 {                     // 0x10
    DWORD dwCaps;
    DWORD dwCaps2;
    DWORD dwCaps3;
    DWORD dwCaps4;
};
struct DDCOLORKEY {
    DWORD dwColorSpaceLowValue;
    DWORD dwColorSpaceHighValue;
};
struct DDSURFACEDESC2 {               // 0x7c
    DWORD dwSize;                     // +0x00
    DWORD dwFlags;                    // +0x04 DDSD_*
    DWORD dwHeight;                   // +0x08
    DWORD dwWidth;                    // +0x0C
    LONG  lPitch;                     // +0x10
    DWORD dwBackBufferCount;          // +0x14
    DWORD dwMipMapCount;              // +0x18
    DWORD dwAlphaBitDepth;            // +0x1C
    DWORD dwReserved;                 // +0x20
    void *lpSurface;                  // +0x24
    DDCOLORKEY ddckCKDestOverlay;     // +0x28
    DDCOLORKEY ddckCKDestBlt;         // +0x30
    DDCOLORKEY ddckCKSrcOverlay;      // +0x38
    DDCOLORKEY ddckCKSrcBlt;          // +0x40
    DDPIXELFORMAT ddpfPixelFormat;    // +0x48
    DDSCAPS2 ddsCaps;                 // +0x68
    DWORD dwTextureStage;             // +0x78
};
// DirectX 7 DDCAPS (dwSize 0x17c in gr_d3d_init_device); only the first fields are read.
struct DDCAPS {
    DWORD dwSize;                     // +0x00
    DWORD dwCaps;                     // +0x04
    DWORD dwCaps2;                    // +0x08 DDCAPS2_PRIMARYGAMMA
    BYTE  _rest[0x17c - 0xc];
};
struct DDGAMMARAMP {                  // 0x600
    WORD red[256];
    WORD green[256];
    WORD blue[256];
};
struct DDDEVICEIDENTIFIER2 {          // 0x42c
    char szDriver[512];               // +0x000
    char szDescription[512];          // +0x200
    LARGE_INTEGER liDriverVersion;    // +0x400
    DWORD dwVendorId;                 // +0x408
    DWORD dwDeviceId;                 // +0x40C
    DWORD dwSubSysId;                 // +0x410
    DWORD dwRevision;                 // +0x414
    GUID  guidDeviceIdentifier;       // +0x418
    DWORD dwWHQLLevel;                // +0x428
};

// ---- Direct3D structures ------------------------------------------------------------------------------
struct D3DPRIMCAPS {                  // 0x38
    DWORD dwSize;                     // +0x00
    DWORD dwMiscCaps;                 // +0x04
    DWORD dwRasterCaps;               // +0x08 D3DPRASTERCAPS_*
    DWORD dwZCmpCaps;                 // +0x0C
    DWORD dwSrcBlendCaps;             // +0x10 D3DPBLENDCAPS_*
    DWORD dwDestBlendCaps;            // +0x14
    DWORD dwAlphaCmpCaps;             // +0x18
    DWORD dwShadeCaps;                // +0x1C
    DWORD dwTextureCaps;              // +0x20 D3DPTEXTURECAPS_*
    DWORD dwTextureFilterCaps;        // +0x24
    DWORD dwTextureBlendCaps;         // +0x28 D3DPTBLENDCAPS_*
    DWORD dwTextureAddressCaps;       // +0x2C
    DWORD dwStippleWidth;             // +0x30
    DWORD dwStippleHeight;            // +0x34
};
struct D3DDEVICEDESC7 {               // 0xec (d3d_device_caps, zeroed with 0x3b dwords)
    DWORD dwDevCaps;                  // +0x00 D3DDEVCAPS_*
    D3DPRIMCAPS dpcLineCaps;          // +0x04
    D3DPRIMCAPS dpcTriCaps;           // +0x3C
    DWORD dwDeviceRenderBitDepth;     // +0x74
    DWORD dwDeviceZBufferBitDepth;    // +0x78
    DWORD dwMinTextureWidth;          // +0x7C
    DWORD dwMinTextureHeight;         // +0x80
    DWORD dwMaxTextureWidth;          // +0x84
    DWORD dwMaxTextureHeight;         // +0x88
    DWORD dwMaxTextureRepeat;         // +0x8C
    DWORD dwMaxTextureAspectRatio;    // +0x90
    DWORD dwMaxAnisotropy;            // +0x94
    D3DVALUE dvGuardBandLeft;         // +0x98
    D3DVALUE dvGuardBandTop;          // +0x9C
    D3DVALUE dvGuardBandRight;        // +0xA0
    D3DVALUE dvGuardBandBottom;       // +0xA4
    D3DVALUE dvExtentsAdjust;         // +0xA8
    DWORD dwStencilCaps;              // +0xAC
    DWORD dwFVFCaps;                  // +0xB0
    DWORD dwTextureOpCaps;            // +0xB4
    WORD  wMaxTextureBlendStages;     // +0xB8
    WORD  wMaxSimultaneousTextures;   // +0xBA
    DWORD dwMaxActiveLights;          // +0xBC
    D3DVALUE dvMaxVertexW;            // +0xC0
    GUID  deviceGUID;                 // +0xC4
    WORD  wMaxUserClipPlanes;         // +0xD4
    WORD  wMaxVertexBlendMatrices;    // +0xD6
    DWORD dwVertexProcessingCaps;     // +0xD8
    DWORD dwReserved1;                // +0xDC
    DWORD dwReserved2;                // +0xE0
    DWORD dwReserved3;                // +0xE4
    DWORD dwReserved4;                // +0xE8
};
struct D3DVIEWPORT7 {                 // 0x18
    DWORD dwX, dwY, dwWidth, dwHeight;
    D3DVALUE dvMinZ, dvMaxZ;
};
struct D3DMATRIX {                    // 0x40
    D3DVALUE _11, _12, _13, _14;
    D3DVALUE _21, _22, _23, _24;
    D3DVALUE _31, _32, _33, _34;
    D3DVALUE _41, _42, _43, _44;
};
struct D3DRECT {
    LONG x1, y1, x2, y2;
};
struct D3DVERTEXBUFFERDESC {          // 0x10
    DWORD dwSize;
    DWORD dwCaps;                     // D3DVBCAPS_*
    DWORD dwFVF;
    DWORD dwNumVertices;
};

// ---- callbacks ------------------------------------------------------------------------------------------
typedef BOOL (WINAPI *LPDDENUMCALLBACKA)(GUID *guid, LPSTR description, LPSTR name, LPVOID context);
typedef HRESULT (CALLBACK *LPD3DENUMDEVICESCALLBACK7)(LPSTR description, LPSTR name, D3DDEVICEDESC7 *desc,
                                                      LPVOID context);
typedef HRESULT (CALLBACK *LPD3DENUMPIXELFORMATSCALLBACK)(DDPIXELFORMAT *pf, LPVOID context);

// ---- interfaces ---------------------------------------------------------------------------------------
struct IDirectDraw7;
struct IDirectDrawSurface7;
struct IDirectDrawClipper;
struct IDirectDrawGammaControl;
struct IDirect3D7;
struct IDirect3DDevice7;
struct IDirect3DVertexBuffer7;

struct IDirectDrawClipperVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDirectDrawClipper *This, const GUID *riid, void **out);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IDirectDrawClipper *This);
    ULONG   (STDMETHODCALLTYPE *Release)(IDirectDrawClipper *This);                         // +0x08
    void *GetClipList;                                                                      // +0x0C
    void *GetHWnd;                                                                          // +0x10
    void *Initialize;                                                                       // +0x14
    void *IsClipListChanged;                                                                // +0x18
    void *SetClipList;                                                                      // +0x1C
    HRESULT (STDMETHODCALLTYPE *SetHWnd)(IDirectDrawClipper *This, DWORD flags, HWND hwnd); // +0x20
};
struct IDirectDrawClipper { IDirectDrawClipperVtbl *lpVtbl; };

struct IDirectDrawGammaControlVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDirectDrawGammaControl *This, const GUID *riid, void **out);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IDirectDrawGammaControl *This);
    ULONG   (STDMETHODCALLTYPE *Release)(IDirectDrawGammaControl *This);                    // +0x08
    HRESULT (STDMETHODCALLTYPE *GetGammaRamp)(IDirectDrawGammaControl *This, DWORD flags,
                                              DDGAMMARAMP *ramp);                           // +0x0C
    HRESULT (STDMETHODCALLTYPE *SetGammaRamp)(IDirectDrawGammaControl *This, DWORD flags,
                                              DDGAMMARAMP *ramp);                           // +0x10
};
struct IDirectDrawGammaControl { IDirectDrawGammaControlVtbl *lpVtbl; };

struct IDirectDrawSurface7Vtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDirectDrawSurface7 *This, const GUID *riid, void **out); // +0x00
    ULONG   (STDMETHODCALLTYPE *AddRef)(IDirectDrawSurface7 *This);                         // +0x04
    ULONG   (STDMETHODCALLTYPE *Release)(IDirectDrawSurface7 *This);                        // +0x08
    HRESULT (STDMETHODCALLTYPE *AddAttachedSurface)(IDirectDrawSurface7 *This,
                                                    IDirectDrawSurface7 *attach);           // +0x0C
    void *AddOverlayDirtyRect;                                                              // +0x10
    HRESULT (STDMETHODCALLTYPE *Blt)(IDirectDrawSurface7 *This, RECT *dst, IDirectDrawSurface7 *src,
                                     RECT *src_rect, DWORD flags, void *fx);                // +0x14
    void *BltBatch;                                                                         // +0x18
    void *BltFast;                                                                          // +0x1C
    void *DeleteAttachedSurface;                                                            // +0x20
    void *EnumAttachedSurfaces;                                                             // +0x24
    void *EnumOverlayZOrders;                                                               // +0x28
    HRESULT (STDMETHODCALLTYPE *Flip)(IDirectDrawSurface7 *This, IDirectDrawSurface7 *target,
                                      DWORD flags);                                         // +0x2C
    HRESULT (STDMETHODCALLTYPE *GetAttachedSurface)(IDirectDrawSurface7 *This, DDSCAPS2 *caps,
                                                    IDirectDrawSurface7 **out);             // +0x30
    HRESULT (STDMETHODCALLTYPE *GetBltStatus)(IDirectDrawSurface7 *This, DWORD flags);      // +0x34
    void *GetCaps;                                                                          // +0x38
    void *GetClipper;                                                                       // +0x3C
    void *GetColorKey;                                                                      // +0x40
    void *GetDC;                                                                            // +0x44
    HRESULT (STDMETHODCALLTYPE *GetFlipStatus)(IDirectDrawSurface7 *This, DWORD flags);     // +0x48
    void *GetOverlayPosition;                                                               // +0x4C
    void *GetPalette;                                                                       // +0x50
    void *GetPixelFormat;                                                                   // +0x54
    HRESULT (STDMETHODCALLTYPE *GetSurfaceDesc)(IDirectDrawSurface7 *This, DDSURFACEDESC2 *desc); // +0x58
    void *Initialize;                                                                       // +0x5C
    HRESULT (STDMETHODCALLTYPE *IsLost)(IDirectDrawSurface7 *This);                         // +0x60
    HRESULT (STDMETHODCALLTYPE *Lock)(IDirectDrawSurface7 *This, RECT *rect, DDSURFACEDESC2 *desc,
                                      DWORD flags, HANDLE event);                           // +0x64
    void *ReleaseDC;                                                                        // +0x68
    HRESULT (STDMETHODCALLTYPE *Restore)(IDirectDrawSurface7 *This);                        // +0x6C
    HRESULT (STDMETHODCALLTYPE *SetClipper)(IDirectDrawSurface7 *This, IDirectDrawClipper *clipper); // +0x70
    void *SetColorKey;                                                                      // +0x74
    void *SetOverlayPosition;                                                               // +0x78
    void *SetPalette;                                                                       // +0x7C
    HRESULT (STDMETHODCALLTYPE *Unlock)(IDirectDrawSurface7 *This, RECT *rect);             // +0x80
};
struct IDirectDrawSurface7 { IDirectDrawSurface7Vtbl *lpVtbl; };

struct IDirectDraw7Vtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDirectDraw7 *This, const GUID *riid, void **out); // +0x00
    ULONG   (STDMETHODCALLTYPE *AddRef)(IDirectDraw7 *This);                                // +0x04
    ULONG   (STDMETHODCALLTYPE *Release)(IDirectDraw7 *This);                               // +0x08
    void *Compact;                                                                          // +0x0C
    HRESULT (STDMETHODCALLTYPE *CreateClipper)(IDirectDraw7 *This, DWORD flags, IDirectDrawClipper **out,
                                               IUnknown *outer);                            // +0x10
    void *CreatePalette;                                                                    // +0x14
    HRESULT (STDMETHODCALLTYPE *CreateSurface)(IDirectDraw7 *This, DDSURFACEDESC2 *desc,
                                               IDirectDrawSurface7 **out, IUnknown *outer); // +0x18
    void *DuplicateSurface;                                                                 // +0x1C
    void *EnumDisplayModes;                                                                 // +0x20
    void *EnumSurfaces;                                                                     // +0x24
    void *FlipToGDISurface;                                                                 // +0x28
    HRESULT (STDMETHODCALLTYPE *GetCaps)(IDirectDraw7 *This, DDCAPS *driver, DDCAPS *hel);  // +0x2C
    void *GetDisplayMode;                                                                   // +0x30
    void *GetFourCCCodes;                                                                   // +0x34
    void *GetGDISurface;                                                                    // +0x38
    void *GetMonitorFrequency;                                                              // +0x3C
    void *GetScanLine;                                                                      // +0x40
    void *GetVerticalBlankStatus;                                                           // +0x44
    void *Initialize;                                                                       // +0x48
    HRESULT (STDMETHODCALLTYPE *RestoreDisplayMode)(IDirectDraw7 *This);                    // +0x4C
    HRESULT (STDMETHODCALLTYPE *SetCooperativeLevel)(IDirectDraw7 *This, HWND hwnd, DWORD flags); // +0x50
    HRESULT (STDMETHODCALLTYPE *SetDisplayMode)(IDirectDraw7 *This, DWORD w, DWORD h, DWORD bpp,
                                                DWORD refresh, DWORD flags);                // +0x54
    void *WaitForVerticalBlank;                                                             // +0x58
    HRESULT (STDMETHODCALLTYPE *GetAvailableVidMem)(IDirectDraw7 *This, DDSCAPS2 *caps, DWORD *total,
                                                    DWORD *free_mem);                       // +0x5C
    void *GetSurfaceFromDC;                                                                 // +0x60
    void *RestoreAllSurfaces;                                                               // +0x64
    void *TestCooperativeLevel;                                                             // +0x68
    HRESULT (STDMETHODCALLTYPE *GetDeviceIdentifier)(IDirectDraw7 *This, DDDEVICEIDENTIFIER2 *id,
                                                     DWORD flags);                          // +0x6C
};
struct IDirectDraw7 { IDirectDraw7Vtbl *lpVtbl; };

struct IDirect3D7Vtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDirect3D7 *This, const GUID *riid, void **out);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IDirect3D7 *This);
    ULONG   (STDMETHODCALLTYPE *Release)(IDirect3D7 *This);                                 // +0x08
    HRESULT (STDMETHODCALLTYPE *EnumDevices)(IDirect3D7 *This, LPD3DENUMDEVICESCALLBACK7 cb,
                                             void *context);                                // +0x0C
    HRESULT (STDMETHODCALLTYPE *CreateDevice)(IDirect3D7 *This, const GUID *rclsid,
                                              IDirectDrawSurface7 *target,
                                              IDirect3DDevice7 **out);                      // +0x10
    HRESULT (STDMETHODCALLTYPE *CreateVertexBuffer)(IDirect3D7 *This, D3DVERTEXBUFFERDESC *desc,
                                                    IDirect3DVertexBuffer7 **out, DWORD flags); // +0x14
    HRESULT (STDMETHODCALLTYPE *EnumZBufferFormats)(IDirect3D7 *This, const GUID *rclsid,
                                                    LPD3DENUMPIXELFORMATSCALLBACK cb,
                                                    void *context);                         // +0x18
    HRESULT (STDMETHODCALLTYPE *EvictManagedTextures)(IDirect3D7 *This);                    // +0x1C
};
struct IDirect3D7 { IDirect3D7Vtbl *lpVtbl; };

struct IDirect3DVertexBuffer7Vtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDirect3DVertexBuffer7 *This, const GUID *riid, void **out);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IDirect3DVertexBuffer7 *This);
    ULONG   (STDMETHODCALLTYPE *Release)(IDirect3DVertexBuffer7 *This);                     // +0x08
    HRESULT (STDMETHODCALLTYPE *Lock)(IDirect3DVertexBuffer7 *This, DWORD flags, void **data,
                                      DWORD *size);                                         // +0x0C
    HRESULT (STDMETHODCALLTYPE *Unlock)(IDirect3DVertexBuffer7 *This);                      // +0x10
};
struct IDirect3DVertexBuffer7 { IDirect3DVertexBuffer7Vtbl *lpVtbl; };

struct IDirect3DDevice7Vtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IDirect3DDevice7 *This, const GUID *riid, void **out); // +0x00
    ULONG   (STDMETHODCALLTYPE *AddRef)(IDirect3DDevice7 *This);                            // +0x04
    ULONG   (STDMETHODCALLTYPE *Release)(IDirect3DDevice7 *This);                           // +0x08
    HRESULT (STDMETHODCALLTYPE *GetCaps)(IDirect3DDevice7 *This, D3DDEVICEDESC7 *desc);     // +0x0C
    HRESULT (STDMETHODCALLTYPE *EnumTextureFormats)(IDirect3DDevice7 *This,
                                                    LPD3DENUMPIXELFORMATSCALLBACK cb,
                                                    void *context);                         // +0x10
    HRESULT (STDMETHODCALLTYPE *BeginScene)(IDirect3DDevice7 *This);                        // +0x14
    HRESULT (STDMETHODCALLTYPE *EndScene)(IDirect3DDevice7 *This);                          // +0x18
    void *GetDirect3D;                                                                      // +0x1C
    void *SetRenderTarget;                                                                  // +0x20
    void *GetRenderTarget;                                                                  // +0x24
    HRESULT (STDMETHODCALLTYPE *Clear)(IDirect3DDevice7 *This, DWORD count, D3DRECT *rects, DWORD flags,
                                       D3DCOLOR color, D3DVALUE z, DWORD stencil);          // +0x28
    HRESULT (STDMETHODCALLTYPE *SetTransform)(IDirect3DDevice7 *This, DWORD state, D3DMATRIX *m); // +0x2C
    void *GetTransform;                                                                     // +0x30
    HRESULT (STDMETHODCALLTYPE *SetViewport)(IDirect3DDevice7 *This, D3DVIEWPORT7 *vp);     // +0x34
    void *MultiplyTransform;                                                                // +0x38
    void *GetViewport;                                                                      // +0x3C
    void *SetMaterial;                                                                      // +0x40
    void *GetMaterial;                                                                      // +0x44
    void *SetLight;                                                                         // +0x48
    void *GetLight;                                                                         // +0x4C
    HRESULT (STDMETHODCALLTYPE *SetRenderState)(IDirect3DDevice7 *This, DWORD state, DWORD value); // +0x50
    void *GetRenderState;                                                                   // +0x54
    void *BeginStateBlock;                                                                  // +0x58
    void *EndStateBlock;                                                                    // +0x5C
    void *PreLoad;                                                                          // +0x60
    HRESULT (STDMETHODCALLTYPE *DrawPrimitive)(IDirect3DDevice7 *This, DWORD type, DWORD fvf, void *verts,
                                               DWORD count, DWORD flags);                   // +0x64
    void *DrawIndexedPrimitive;                                                             // +0x68
    void *SetClipStatus;                                                                    // +0x6C
    void *GetClipStatus;                                                                    // +0x70
    void *DrawPrimitiveStrided;                                                             // +0x74
    void *DrawIndexedPrimitiveStrided;                                                      // +0x78
    HRESULT (STDMETHODCALLTYPE *DrawPrimitiveVB)(IDirect3DDevice7 *This, DWORD type,
                                                 IDirect3DVertexBuffer7 *vb, DWORD start,
                                                 DWORD count, DWORD flags);                 // +0x7C
    HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitiveVB)(IDirect3DDevice7 *This, DWORD type,
                                                        IDirect3DVertexBuffer7 *vb, DWORD start,
                                                        DWORD num_verts, WORD *indices,
                                                        DWORD num_indices, DWORD flags);   // +0x80 (decomp65)
    void *ComputeSphereVisibility;                                                          // +0x84
    void *GetTexture;                                                                       // +0x88
    HRESULT (STDMETHODCALLTYPE *SetTexture)(IDirect3DDevice7 *This, DWORD stage,
                                            IDirectDrawSurface7 *tex);                      // +0x8C
    void *GetTextureStageState;                                                             // +0x90
    HRESULT (STDMETHODCALLTYPE *SetTextureStageState)(IDirect3DDevice7 *This, DWORD stage, DWORD type,
                                                      DWORD value);                         // +0x94
};
struct IDirect3DDevice7 { IDirect3DDevice7Vtbl *lpVtbl; };

typedef HRESULT (WINAPI *LPDIRECTDRAWCREATEEX)(GUID *guid, void **out, const GUID *iid, IUnknown *outer);

// ---- ddraw.dll / dxguid.lib -----------------------------------------------------------------------------
// DirectDrawEnumerateA is reached through the import-library thunk (`call 0x57b564`), not through an
// import pointer, so it is declared without __declspec(dllimport). DirectDrawCreateEx is looked up with
// GetProcAddress (d3d_ddraw_enumerate_ex).
extern "C" HRESULT WINAPI DirectDrawEnumerateA(LPDDENUMCALLBACKA cb, LPVOID context);
extern "C" const GUID IID_IDirect3DHALDevice;         // 0x583350 {84e63de0-46aa-11cf-816f-0000c020156e}
extern "C" const GUID IID_IDirect3D7;                 // 0x583360 {f5049e77-4861-11d2-a407-00a0c90629a8}
extern "C" const GUID IID_IDirectDrawGammaControl;    // 0x583370 {69c11c3e-b46b-11d1-ad7a-00c04fc29b4e}
extern "C" const GUID IID_IDirectDraw7;               // 0x583380 {15e65ec0-3b9c-11d2-b92f-00609797ea5b}

// ---- constants (SDK values) -----------------------------------------------------------------------------
#define DD_OK                           0
#define DDENUMRET_CANCEL                0
#define DDENUMRET_OK                    1
#define D3DENUMRET_CANCEL               0
#define D3DENUMRET_OK                   1
#define DDERR_SURFACELOST               ((HRESULT)0x887601c2L)

#define DDSCL_FULLSCREEN                0x00000001
#define DDSCL_NOWINDOWCHANGES           0x00000004
#define DDSCL_NORMAL                    0x00000008
#define DDSCL_EXCLUSIVE                 0x00000010
#define DDSD_CAPS                       0x00000001
#define DDSD_HEIGHT                     0x00000002
#define DDSD_WIDTH                      0x00000004
#define DDSD_BACKBUFFERCOUNT            0x00000020
#define DDSD_PIXELFORMAT                0x00001000
#define DDSCAPS_BACKBUFFER              0x00000004
#define DDSCAPS_COMPLEX                 0x00000008
#define DDSCAPS_FLIP                    0x00000010
#define DDSCAPS_OFFSCREENPLAIN          0x00000040
#define DDSCAPS_PRIMARYSURFACE          0x00000200
#define DDSCAPS_SYSTEMMEMORY            0x00000800
#define DDSCAPS_TEXTURE                 0x00001000
#define DDSCAPS_3DDEVICE                0x00002000
#define DDSCAPS_VIDEOMEMORY             0x00004000
#define DDSCAPS_ZBUFFER                 0x00020000
#define DDCAPS2_PRIMARYGAMMA            0x00020000
#define DDPF_ALPHAPIXELS                0x00000001
#define DDPF_RGB                        0x00000040
#define DDPF_ZBUFFER                    0x00000400
#define DDPF_BUMPDUDV                   0x00080000
#define DDLOCK_WAIT                     0x00000001
#define DDLOCK_READONLY                 0x00000010
#define DDLOCK_WRITEONLY                0x00000020
#define DDLOCK_NOSYSLOCK                0x00000800
#define DDLOCK_NOOVERWRITE              0x00001000
#define DDLOCK_DISCARDCONTENTS          0x00002000
#define DDFLIP_WAIT                     0x00000001
#define DDFLIP_NOVSYNC                  0x00000008
#define DDBLT_WAIT                      0x01000000
#define DDGBS_ISBLTDONE                 0x00000002
#define DDGFS_ISFLIPDONE                0x00000002

#define D3DDEVCAPS_DRAWPRIMITIVES2EX    0x00008000
#define D3DDEVCAPS_HWTRANSFORMANDLIGHT  0x00010000
#define D3DPRASTERCAPS_FOGVERTEX        0x00000080
#define D3DPRASTERCAPS_FOGTABLE         0x00000100
#define D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT 0x00000800
#define D3DPRASTERCAPS_WFOG             0x00100000
#define D3DPBLENDCAPS_SRCALPHA          0x00000010
#define D3DPBLENDCAPS_INVSRCALPHA       0x00000020
#define D3DPBLENDCAPS_DESTCOLOR         0x00000100
#define D3DPBLENDCAPS_BOTHSRCALPHA      0x00000800
#define D3DPBLENDCAPS_BOTHINVSRCALPHA   0x00001000
#define D3DPTBLENDCAPS_MODULATE         0x00000002
#define D3DPTBLENDCAPS_MODULATEALPHA    0x00000008
#define D3DPTEXTURECAPS_POW2            0x00000002
#define D3DPTEXTURECAPS_SQUAREONLY      0x00000020
#define D3DVBCAPS_WRITEONLY             0x00010000
#define D3DVBCAPS_DONOTCLIP             0x00000001
#define D3DVBCAPS_SYSTEMMEMORY          0x00000800

#define D3DCLEAR_TARGET                 0x00000001
#define D3DCLEAR_ZBUFFER                0x00000002
#define D3DTRANSFORMSTATE_WORLD         1
#define D3DTRANSFORMSTATE_VIEW          2
#define D3DTRANSFORMSTATE_PROJECTION    3
#define D3DPT_LINELIST                  2
#define D3DPT_TRIANGLELIST              4   // decomp65 (the character meshes)
#define D3DPT_TRIANGLEFAN               6
#define D3DFVF_XYZRHW                   0x004
#define D3DFVF_DIFFUSE                  0x040
#define D3DFVF_SPECULAR                 0x080
#define D3DFVF_TEX2                     0x200
#define D3DFVF_TLVERTEX2 (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2)   // 0x2c4 (name ours)

// D3DRENDERSTATETYPE
#define D3DRENDERSTATE_ANTIALIAS        2
#define D3DRENDERSTATE_TEXTUREPERSPECTIVE 4
#define D3DRENDERSTATE_ZENABLE          7
#define D3DRENDERSTATE_SHADEMODE        9
#define D3DRENDERSTATE_ZWRITEENABLE     14
#define D3DRENDERSTATE_ALPHATESTENABLE  15
#define D3DRENDERSTATE_TEXTUREMAG       17
#define D3DRENDERSTATE_TEXTUREMIN       18
#define D3DRENDERSTATE_SRCBLEND         19
#define D3DRENDERSTATE_DESTBLEND        20
#define D3DRENDERSTATE_CULLMODE         22
#define D3DRENDERSTATE_ZFUNC            23
#define D3DRENDERSTATE_ALPHAREF         24
#define D3DRENDERSTATE_ALPHAFUNC        25
#define D3DRENDERSTATE_DITHERENABLE     26
#define D3DRENDERSTATE_ALPHABLENDENABLE 27
#define D3DRENDERSTATE_FOGENABLE        28
#define D3DRENDERSTATE_SPECULARENABLE   29
#define D3DRENDERSTATE_FOGCOLOR         34
#define D3DRENDERSTATE_FOGTABLEMODE     35
#define D3DRENDERSTATE_FOGSTART         36
#define D3DRENDERSTATE_FOGEND           37
#define D3DRENDERSTATE_ZBIAS            47
#define D3DRENDERSTATE_CLIPPING         136
#define D3DRENDERSTATE_LIGHTING         137
#define D3DRENDERSTATE_FOGVERTEXMODE    140
// D3DTEXTURESTAGESTATETYPE
#define D3DTSS_COLOROP                  1
#define D3DTSS_COLORARG1                2
#define D3DTSS_COLORARG2                3
#define D3DTSS_ALPHAOP                  4
#define D3DTSS_ALPHAARG1                5
#define D3DTSS_ALPHAARG2                6
#define D3DTSS_BUMPENVMAT00             7
#define D3DTSS_BUMPENVMAT01             8
#define D3DTSS_BUMPENVMAT10             9
#define D3DTSS_BUMPENVMAT11             10
#define D3DTSS_TEXCOORDINDEX            11
#define D3DTSS_ADDRESS                  12
#define D3DTSS_ADDRESSU                 13
#define D3DTSS_ADDRESSV                 14
#define D3DTSS_MAGFILTER                16
#define D3DTSS_MINFILTER                17
#define D3DTSS_MIPFILTER                18
#define D3DTSS_BUMPENVLSCALE            22
#define D3DTSS_BUMPENVLOFFSET           23
// D3DTEXTUREOP
#define D3DTOP_DISABLE                  1
#define D3DTOP_SELECTARG1               2
#define D3DTOP_MODULATE                 4
#define D3DTOP_ADD                      7
#define D3DTOP_BLENDTEXTUREALPHA        13
#define D3DTOP_BUMPENVMAP               22
// D3DTA_*
#define D3DTA_DIFFUSE                   0
#define D3DTA_CURRENT                   1
#define D3DTA_TEXTURE                   2
// D3DTEXTUREADDRESS
#define D3DTADDRESS_WRAP                1
#define D3DTADDRESS_MIRROR              2
#define D3DTADDRESS_CLAMP               3
// D3DTEXTUREMAGFILTER / MINFILTER / MIPFILTER
#define D3DTFG_POINT                    1
#define D3DTFG_LINEAR                   2
#define D3DTFN_POINT                    1
#define D3DTFN_LINEAR                   2
#define D3DTFP_LINEAR                   3
// D3DTEXTUREFILTER (render states 17 / 18)
#define D3DFILTER_LINEAR                2
// D3DBLEND
#define D3DBLEND_ZERO                   1
#define D3DBLEND_ONE                    2
#define D3DBLEND_SRCALPHA               5
#define D3DBLEND_INVSRCALPHA            6
#define D3DBLEND_DESTCOLOR              9
#define D3DBLEND_BOTHSRCALPHA           12
// D3DCMPFUNC
#define D3DCMP_EQUAL                    3
#define D3DCMP_LESSEQUAL                4
#define D3DCMP_GREATER                  5
#define D3DCMP_GREATEREQUAL             7
#define D3DCMP_ALWAYS                   8
#define D3DFOG_LINEAR                   3
#define D3DCULL_NONE                    1
#define D3DSHADE_GOURAUD                2
#define D3DANTIALIAS_SORTINDEPENDENT    2

// d3d.h's colour macro: alpha in the top byte. VC6 folds the constant 0xff alpha into
// `or reg, 0xffffff00` before the shifts (gr_d3d_set_fog 0x53aa47; the opaque paths of gr_d3d_bitmap).
#define RGBA_MAKE(r, g, b, a) ((D3DCOLOR)(((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define D3DRGB(r, g, b) \
    (0xff000000L | (((long)((r) * 255)) << 16) | (((long)((g) * 255)) << 8) | (long)((b) * 255))
#define D3DFOG_NONE                     0

// ---- the SDK's C call macros (only the ones gr_direct3d.cpp uses) -------------------------------------
// They name the object twice, as in ddraw.h / d3d.h without C++ (see the top of this file).
#define IDirectDraw7_QueryInterface(p, a, b)            (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDraw7_Release(p)                         (p)->lpVtbl->Release(p)
#define IDirectDraw7_CreateClipper(p, a, b, c)          (p)->lpVtbl->CreateClipper(p, a, b, c)
#define IDirectDraw7_CreateSurface(p, a, b, c)          (p)->lpVtbl->CreateSurface(p, a, b, c)
#define IDirectDraw7_GetCaps(p, a, b)                   (p)->lpVtbl->GetCaps(p, a, b)
#define IDirectDraw7_RestoreDisplayMode(p)              (p)->lpVtbl->RestoreDisplayMode(p)
#define IDirectDraw7_SetCooperativeLevel(p, a, b)       (p)->lpVtbl->SetCooperativeLevel(p, a, b)
#define IDirectDraw7_SetDisplayMode(p, a, b, c, d, e)   (p)->lpVtbl->SetDisplayMode(p, a, b, c, d, e)
#define IDirectDraw7_GetAvailableVidMem(p, a, b, c)     (p)->lpVtbl->GetAvailableVidMem(p, a, b, c)
#define IDirectDraw7_GetDeviceIdentifier(p, a, b)       (p)->lpVtbl->GetDeviceIdentifier(p, a, b)
#define IDirectDrawSurface7_QueryInterface(p, a, b)     (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDrawSurface7_Release(p)                  (p)->lpVtbl->Release(p)
#define IDirectDrawSurface7_AddAttachedSurface(p, a)    (p)->lpVtbl->AddAttachedSurface(p, a)
#define IDirectDrawSurface7_Blt(p, a, b, c, d, e)       (p)->lpVtbl->Blt(p, a, b, c, d, e)
#define IDirectDrawSurface7_Flip(p, a, b)               (p)->lpVtbl->Flip(p, a, b)
#define IDirectDrawSurface7_GetAttachedSurface(p, a, b) (p)->lpVtbl->GetAttachedSurface(p, a, b)
#define IDirectDrawSurface7_GetBltStatus(p, a)          (p)->lpVtbl->GetBltStatus(p, a)
#define IDirectDrawSurface7_GetFlipStatus(p, a)         (p)->lpVtbl->GetFlipStatus(p, a)
#define IDirectDrawSurface7_GetSurfaceDesc(p, a)        (p)->lpVtbl->GetSurfaceDesc(p, a)
#define IDirectDrawSurface7_IsLost(p)                   (p)->lpVtbl->IsLost(p)
#define IDirectDrawSurface7_Lock(p, a, b, c, d)         (p)->lpVtbl->Lock(p, a, b, c, d)
#define IDirectDrawSurface7_Restore(p)                  (p)->lpVtbl->Restore(p)
#define IDirectDrawSurface7_SetClipper(p, a)            (p)->lpVtbl->SetClipper(p, a)
#define IDirectDrawSurface7_Unlock(p, a)                (p)->lpVtbl->Unlock(p, a)
#define IDirectDrawClipper_SetHWnd(p, a, b)             (p)->lpVtbl->SetHWnd(p, a, b)
#define IDirectDrawClipper_Release(p)                   (p)->lpVtbl->Release(p)
#define IDirectDrawGammaControl_GetGammaRamp(p, a, b)   (p)->lpVtbl->GetGammaRamp(p, a, b)
#define IDirectDrawGammaControl_SetGammaRamp(p, a, b)   (p)->lpVtbl->SetGammaRamp(p, a, b)
#define IDirectDrawGammaControl_Release(p)              (p)->lpVtbl->Release(p)
#define IDirect3D7_Release(p)                           (p)->lpVtbl->Release(p)
#define IDirect3D7_EnumDevices(p, a, b)                 (p)->lpVtbl->EnumDevices(p, a, b)
#define IDirect3D7_CreateDevice(p, a, b, c)             (p)->lpVtbl->CreateDevice(p, a, b, c)
#define IDirect3D7_CreateVertexBuffer(p, a, b, c)       (p)->lpVtbl->CreateVertexBuffer(p, a, b, c)
#define IDirect3D7_EnumZBufferFormats(p, a, b, c)       (p)->lpVtbl->EnumZBufferFormats(p, a, b, c)
#define IDirect3DVertexBuffer7_Lock(p, a, b, c)         (p)->lpVtbl->Lock(p, a, b, c)
#define IDirect3DVertexBuffer7_Unlock(p)                (p)->lpVtbl->Unlock(p)
#define IDirect3DDevice7_Release(p)                     (p)->lpVtbl->Release(p)
#define IDirect3DDevice7_GetCaps(p, a)                  (p)->lpVtbl->GetCaps(p, a)
#define IDirect3DDevice7_EnumTextureFormats(p, a, b)    (p)->lpVtbl->EnumTextureFormats(p, a, b)
#define IDirect3DDevice7_BeginScene(p)                  (p)->lpVtbl->BeginScene(p)
#define IDirect3DDevice7_EndScene(p)                    (p)->lpVtbl->EndScene(p)
#define IDirect3DDevice7_Clear(p, a, b, c, d, e, f)     (p)->lpVtbl->Clear(p, a, b, c, d, e, f)
#define IDirect3DDevice7_SetTransform(p, a, b)          (p)->lpVtbl->SetTransform(p, a, b)
#define IDirect3DDevice7_SetViewport(p, a)              (p)->lpVtbl->SetViewport(p, a)
#define IDirect3DDevice7_SetRenderState(p, a, b)        (p)->lpVtbl->SetRenderState(p, a, b)
#define IDirect3DDevice7_DrawPrimitive(p, a, b, c, d, e) (p)->lpVtbl->DrawPrimitive(p, a, b, c, d, e)
#define IDirect3DDevice7_DrawPrimitiveVB(p, a, b, c, d, e) (p)->lpVtbl->DrawPrimitiveVB(p, a, b, c, d, e)
#define IDirect3DDevice7_DrawIndexedPrimitiveVB(p, a, b, c, d, e, f, g) (p)->lpVtbl->DrawIndexedPrimitiveVB(p, a, b, c, d, e, f, g)
#define IDirect3DDevice7_SetTextureStageState(p, a, b, c) (p)->lpVtbl->SetTextureStageState(p, a, b, c)

#endif
