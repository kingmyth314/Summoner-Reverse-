// vsdk/snd/ds.h - the DirectSound layer (ds_* 0x55c000-0x55dfe0, not written) as the sound and EAX units
// call it, and the EAX 2.0 property-set declarations they use (ours; stands in for <dsound.h> / <eax.h>,
// like gr/directx/d3d_min.h, D4008). Prototypes from the call sites in 0x5204c0-0x5225a1 (decomp60);
// names from sum.symbols except ds_get_num_playing_channels (0x55db30 has no symbol). Not original source.
#ifndef VSDK_SND_DS_H
#define VSDK_SND_DS_H

#include "vsdk/vsdk.h"
#include <windows.h>                  // HWND, GUID, HRESULT, IUnknown

class vector3;
class matrix;

// ---- EAX 2.0 (the values the game uses; names from the EAX 2.0 SDK headers) ------------------------
// IKsPropertySet: the property set interface of DirectSound buffers (vtable: QueryInterface, AddRef,
// Release +0x08, Get +0x0c, Set +0x10, QuerySupport +0x14).
struct IKsPropertySet {
    virtual HRESULT __stdcall QueryInterface(const GUID &riid, void **out) = 0;
    virtual ULONG   __stdcall AddRef() = 0;
    virtual ULONG   __stdcall Release() = 0;
    virtual HRESULT __stdcall Get(const GUID &set, ULONG id, void *instance, ULONG instance_len,
                                  void *data, ULONG data_len, ULONG *returned) = 0;
    virtual HRESULT __stdcall Set(const GUID &set, ULONG id, void *instance, ULONG instance_len,
                                  void *data, ULONG data_len) = 0;
    virtual HRESULT __stdcall QuerySupport(const GUID &set, ULONG id, ULONG *support) = 0;
};
// The GUIDs are shared .rdata (0x5832e0 / 0x5833b0 / 0x5833c0, also read by ds_eax_init and the
// Direct3D / DirectInput code): an initguid object, not this unit's data.
extern const GUID IID_IKsPropertySet;                    // 0x5832e0 {31efac30-515c-11d0-a9aa-00aa0061be93}
extern const GUID DSPROPSETID_EAX20_ListenerProperties;  // 0x5833b0 {0306a6a8-b224-11d2-99e5-0000e8d8c722}
extern const GUID DSPROPSETID_EAX20_BufferProperties;    // 0x5833c0 {0306a6a7-b224-11d2-99e5-0000e8d8c722}
#define KSPROPERTY_SUPPORT_GET 1
#define KSPROPERTY_SUPPORT_SET 2
// listener property ids
#define DSPROPERTY_EAXLISTENER_ALLPARAMETERS        1
#define DSPROPERTY_EAXLISTENER_ROOM                 2
#define DSPROPERTY_EAXLISTENER_DECAYTIME            5
#define DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF      14
#define DSPROPERTY_EAXLISTENER_ENVIRONMENT          11
// buffer property ids
#define DSPROPERTY_EAXBUFFER_ROOM                   4
#define EAX_ENVIRONMENT_CAVE                        8
#define EAX_ROOM_MIN                                (-10000)
#define EAX_NUM_ENVIRONMENTS                        26

// The 14 listener properties in their ALLPARAMETERS order (0x38 bytes; the <level>.eax file holds
// them one per line; LONG / DWORD in the SDK, int here: the same type for VC6, and fprintf %d fits).
struct EAXLISTENERPROPERTIES {
    int lRoom;                        // +0x00
    int lRoomHF;                      // +0x04
    float flRoomRolloffFactor;        // +0x08
    float flDecayTime;                // +0x0c
    float flDecayHFRatio;             // +0x10
    int lReflections;                 // +0x14
    float flReflectionsDelay;         // +0x18
    int lReverb;                      // +0x1c
    float flReverbDelay;              // +0x20
    unsigned int dwEnvironment;       // +0x24
    float flEnvironmentSize;          // +0x28
    float flEnvironmentDiffusion;     // +0x2c
    float flAirAbsorptionHF;          // +0x30
    unsigned int dwFlags;             // +0x34
};
LAYOUT_CHECK(EAXLISTENERPROPERTIES, sizeof(EAXLISTENERPROPERTIES) == 0x38);

// ---- ds_* (another vsdk object; argument meaning from the sound unit's calls) -----------------------
// (hwnd, rate, bits, channels, ds3d, eax, dsinit) 0 = ok.
int  ds_init(HWND hwnd, int rate, int bits, int channels, bool use_ds3d, bool use_eax, bool dsinit);   // 0x55c1a0
void ds_close();                                          // 0x55c4e0
// (sound name, file path, offset in the packfile, group, streamed) -> buffer, < 0 failed.
int  ds_load_buffer(const char *name, const char *path, int offset, int group, int streamed);   // 0x55c890
void ds_unload_buffer(int buffer);                        // 0x55ced0
// (buffer, pos, velocity, min range, max range, volume, looping, a) -> signature.
int  ds_play_3d(int buffer, const vector3 *pos, const vector3 *vel, float min_range, float max_range,
                float volume, bool looping, int a);       // 0x55cf00
int  ds_play(int buffer, float volume, float pan, int looping, int a);   // 0x55d110
int  ds_play_streamed(int buffer, float volume, int a, int looping, float f);   // 0x55d2b0
void ds_stop(int sig);                                    // 0x55d620
void ds_stop_fade(int sig, int ms);                       // 0x55d640
void ds_do_frame();                                       // 0x55d6c0
void ds_pause_all(int a);                                 // 0x55d800
void ds_unpause_all();                                    // 0x55d8c0
void ds_stop_all();                                       // 0x55d8f0
void ds_set_volume(int sig, float volume);                // 0x55d910
void ds_set_pan(int sig, float pan);                      // 0x55d960
void ds_set_frequency(int sig, unsigned int freq);        // 0x55d9a0
bool ds_is_playing(int sig);                              // 0x55da20
void ds_get_format(int buffer, int *a, int *b, int *c);   // 0x55da50 (snd_calc_duration multiplies a * b)
int  ds_get_frequency(int sig, unsigned int *freq);       // 0x55dac0
int  ds_get_num_playing_channels();                       // 0x55db30 (name ours; no symbol)
IKsPropertySet *ds_get_eax_property_set();                // 0x55db70
bool ds_is_ds3d_enabled();                                // 0x55dc70
bool ds_is_eax_enabled();                                 // 0x55dc80
void ds_disable_eax();                                    // 0x55dca0
void ds_set_group_volume(int group, float volume);        // 0x55dcb0
float ds_estimate_duration(int buffer);                   // 0x55dd10
void ds_set_eax_environment(const char *name);            // 0x55dd80
void ds3d_update_listener(const vector3 *pos, const vector3 *vel, const matrix *orient);   // 0x55de30

#endif
