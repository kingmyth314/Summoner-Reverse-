/* Link support for the 64-bit build (tools/link-build.mjs --arch x64; docs/future/modern-port.md, rung L3).
   Bink (binkw32.dll) and EAX (eax.dll) have no 64-bit DLLs; Glide and Direct3D 7 are reached at run time
   (LoadLibrary / QueryInterface) and need nothing here. These stand-ins take the paths the game already has
   for a missing library: no movie (BinkOpen NULL), plain DirectSound without EAX. */
#include <windows.h>
#include <dsound.h>

/* EAX.DLL ordinal 5: EAX's DirectSound with its property sets. Plain DirectSound instead; the game's EAX
   queries (ds_get_eax_property_set) then find no property set and leave EAX off. */
HRESULT WINAPI EAXDirectSoundCreate(const GUID *guid, LPDIRECTSOUND *out, IUnknown *outer)
{
    return DirectSoundCreate(guid, out, outer);
}

/* binkw32.dll: BinkOpen fails, so movie_play returns at once (FFmpeg's Bink decoder replaces this later). */
void *WINAPI BinkOpen(const char *name, unsigned int flags) { (void)name; (void)flags; return NULL; }
void WINAPI BinkClose(void *bink) { (void)bink; }
int WINAPI BinkCopyToBuffer(void *bink, void *dest, int destpitch, unsigned int destheight, unsigned int destx,
                            unsigned int desty, unsigned int flags)
{
    (void)bink; (void)dest; (void)destpitch; (void)destheight; (void)destx; (void)desty; (void)flags;
    return 0;
}
void WINAPI BinkNextFrame(void *bink) { (void)bink; }
void *WINAPI BinkOpenDirectSound(unsigned int param) { (void)param; return NULL; }
int WINAPI BinkSetSoundSystem(void *open, unsigned int param) { (void)open; (void)param; return 0; }
int WINAPI BinkWait(void *bink) { (void)bink; return 0; }
int WINAPI BinkDoFrame(void *bink) { (void)bink; return 0; }
