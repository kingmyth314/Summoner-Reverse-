/* Link support for the 64-bit build (tools/link-build.mjs --arch x64; docs/future/modern-port.md, rung L3).
   EAX (eax.dll) has no 64-bit DLL; Glide and Direct3D 7 are reached at run time (LoadLibrary / QueryInterface) and
   need nothing here; the Bink calls are src/decomp/modern/bink.cpp (FFmpeg). This stand-in takes the path the game
   already has for a missing library: plain DirectSound without EAX. */
#include <windows.h>
#include <dsound.h>

/* EAX.DLL ordinal 5: EAX's DirectSound with its property sets. Plain DirectSound instead; the game's EAX
   queries (ds_get_eax_property_set) then find no property set and leave EAX off. */
HRESULT WINAPI EAXDirectSoundCreate(const GUID *guid, LPDIRECTSOUND *out, IUnknown *outer)
{
    return DirectSoundCreate(guid, out, outer);
}
