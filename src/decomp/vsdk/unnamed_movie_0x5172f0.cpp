// unnamed_movie_0x5172f0.cpp - Sum.exe v1.40, code 0x5172f0-0x517583: Bink movie playback (module "-" in
// sum.symbols). Written from the Sum.exe disassembly (tools/asm.mjs; the Bink entry points from the import
// table, binkw32.dll 0x57e488-0x57e4a4). Target: VC++ 6.0 (cl 12.00.8966), /O2 /GX. Confidence per function:
// exact = expect identical code, likely = same semantics, small layout risk, unsure = the source shape is a
// guess.
//
// File name not known (no path string, no strings.tbl block), so a placeholder; the header is
// vsdk/os/movie.h (directory guessed by an earlier pilot). The unit lies between parse.cpp (its last RET
// 0x5172eb) and the network code (0x517590); its one global, movie_bink_handle 0x26b63e4, follows parse.cpp's
// .bss. movie_play shows vlogo.bik / eaxlogo.bik at start-up and the credits movies.
//
// Bink (RAD Game Tools, bink.h of 1999-2000): only the five frame fields the code reads are declared; the
// entry points are __stdcall (_BinkOpen@8 ...).

#include <stddef.h>
#include <string.h>
#include "vsdk/os/os.h"
#include "vsdk/os/key.h"
#include "vsdk/os/movie.h"
#include "vsdk/vfile/file.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/directx/d3d_min.h"
#include "vsdk/gr/directx/gr_direct3d.h"
#include "vsdk/gr/glide/glide_min.h"
#include "vsdk/gr/glide/gr_glide.h"
#include "summoner/summoner.h"

// ---- the Bink API (binkw32.dll) -------------------------------------------------------------------------
struct BINK {
    unsigned int Width;                  // +0x00
    unsigned int Height;                 // +0x04
    unsigned int Frames;                 // +0x08
    unsigned int FrameNum;               // +0x0c the current frame, 1-based
};
typedef BINK *HBINK;
typedef void *(__stdcall *BINKSNDSYSOPEN)(unsigned int param);

#define BINKSURFACE565   0x0000000a
#define BINKCOPYALL      0x80000000

extern "C" {
__declspec(dllimport) HBINK __stdcall BinkOpen(const char *name, unsigned int flags);                     // 0x57e488
__declspec(dllimport) void __stdcall BinkClose(HBINK bink);                                                // 0x57e48c
__declspec(dllimport) int __stdcall BinkCopyToBuffer(HBINK bink, void *dest, int destpitch, unsigned int destheight,
                               unsigned int destx, unsigned int desty, unsigned int flags);   // 0x57e490
__declspec(dllimport) void __stdcall BinkNextFrame(HBINK bink);                                            // 0x57e494
__declspec(dllimport) void *__stdcall BinkOpenDirectSound(unsigned int param);                             // 0x57e498
__declspec(dllimport) int __stdcall BinkSetSoundSystem(BINKSNDSYSOPEN open, unsigned int param);           // 0x57e49c
__declspec(dllimport) int __stdcall BinkWait(HBINK bink);                                                  // 0x57e4a0
__declspec(dllimport) int __stdcall BinkDoFrame(HBINK bink);                                               // 0x57e4a4
}

#define GR_LFBWRITEMODE_565   0                      // Glide 2 GrLfbWriteMode_t

int movie_blit_frame(HBINK bink);

// ---- .bss ----------------------------------------------------------------------------------------------
HBINK movie_bink_handle;                             // 0x26b63e4

// 0x5172f0  Confidence: exact (VC6 8966 byte match, w1 / D11001: the missing window is an early return and the
// path buffer is declared after it, which puts the File at [esp] as in the original; D5607). The File
// object is constructed and destroyed but never used: the EH frame is for it. Plays movies\<filename> until it ends or Escape is pressed, on a cleared screen,
// with Bink's DirectSound output. Nothing happens without a main window.
void movie_play(const char *filename)
{
    File unused;
    int done;

    if (os_get_main_wnd() == NULL)
        return;
    char path[MAX_PATH];
    file_make_path(path_movies, filename, path);
    BinkSetSoundSystem(BinkOpenDirectSound, 0);
    movie_bink_handle = BinkOpen(path, 0);
    if (movie_bink_handle != NULL) {
        gr::reset_clip();
        gr_set_color(0, 0, 0, 255);
        gr_clear();
        gr_flip();
        gr_clear();
        gr_flip();
        done = 0;
        while (!done) {
            os_poll();
            if (key_inkey() == KEY_ESC)
                break;
            if (!BinkWait(movie_bink_handle))
                done = movie_blit_frame(movie_bink_handle);
        }
        if (movie_bink_handle) {
            BinkClose(movie_bink_handle);
            movie_bink_handle = NULL;
        }
    }
}

// 0x517430  Confidence: exact (VC6 byte match; the Direct3D lock retry as a while loop; the shared failure
// path). Decodes
// the frame and copies it, centred as 640x480 RGB565, into the locked back buffer (Glide or Direct3D; the
// OpenGL renderer shows nothing and the movie ends at once), then flips. Returns 1 after the last frame (or
// under OpenGL), else 0 after advancing to the next frame. A failed lock or copy skips the flip.
int movie_blit_frame(HBINK bink)
{
    GrLfbInfo_t info;
    DDSURFACEDESC2 desc;
    HRESULT hr;
    void *dest;
    int pitch;
    int skipped;

    BinkDoFrame(bink);
    switch (gr_screen.mode) {
    case GR_GLIDE:
        info.size = sizeof(info);
        if (!glide_grLfbLock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER, GR_LFBWRITEMODE_565, GR_ORIGIN_UPPER_LEFT,
                             FXFALSE, &info))
            goto next;
        pitch = info.strideInBytes;
        dest = info.lfbPtr;
        break;
    case GR_DIRECT3D:
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = IDirectDrawSurface7_Lock(d3d_back_surface, NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);
        while (hr == DDERR_SURFACELOST) {
            IDirectDrawSurface7_Restore(d3d_back_surface);
            hr = IDirectDrawSurface7_Lock(d3d_back_surface, NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);
        }
        if (hr != DD_OK)
            goto next;
        dest = desc.lpSurface;
        pitch = desc.lPitch;
        break;
    default:
        return 1;
    }
    if (dest == NULL)
        goto next;
    skipped = BinkCopyToBuffer(bink, dest, pitch, 480, (gr_screen_width() - 640) / 2,
                               (gr_screen_height() - 480) / 2, BINKSURFACE565 | BINKCOPYALL);
    switch (gr_screen.mode) {
    case GR_GLIDE:
        glide_grLfbUnlock(GR_LFB_WRITE_ONLY, GR_BUFFER_BACKBUFFER);
        break;
    case GR_DIRECT3D:
        IDirectDrawSurface7_Unlock(d3d_back_surface, NULL);
        break;
    }
    if (!skipped)
        gr_flip();
next:
    if (bink->FrameNum != bink->Frames) {
        BinkNextFrame(bink);
        return 0;
    }
    return 1;
}
