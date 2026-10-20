// vsdk/snd/ds.h - the DirectSound layer (ds_* 0x55bf90-0x55ddae and ds3d_* 0x55ddb0-0x55e03f) and the EAX
// 2.0 property-set declarations the sound and EAX units use (ours; stands in for <dsound.h> / <eax.h>,
// like gr/directx/d3d_min.h, D4008). decomp60 declared the prototypes from the call sites; decomp66 wrote
// both units (src/decomp/vsdk/unnamed_ds_0x55bf90.cpp, unnamed_ds3d_0x55ddb0.cpp; file names placeholders)
// and added the DirectSound interfaces, the buffer and channel records and the rest of the functions.
// Names from sum.symbols except ds_get_num_playing_channels (0x55db30) and ds_pause (0x55d7b0), which have
// no symbol. Not original source.
#ifndef VSDK_SND_DS_H
#define VSDK_SND_DS_H

#include "vsdk/vsdk.h"
#include <windows.h>                  // HWND, GUID, HRESULT, IUnknown
#include <mmsystem.h>                 // HMMIO, MMCKINFO, WAVEFORMATEX
#include <mmreg.h>
#include <msacm.h>                    // HACMSTREAM
#include "vsdk/os/timestamp.h"        // RealtimeTimer (the channel fade)

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
// The GUIDs are shared .rdata (0x5832e0 / 0x5832f0 / 0x583300 / 0x5833b0 / 0x5833c0, also read by the
// Direct3D / DirectInput code): an initguid object, not this unit's data.
extern const GUID IID_IKsPropertySet;                    // 0x5832e0 {31efac30-515c-11d0-a9aa-00aa0061be93}
extern const GUID IID_IDirectSound3DBuffer;              // 0x5832f0 (decomp66)
extern const GUID IID_IDirectSound3DListener;            // 0x583300 (decomp66)
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

// ---- DirectSound (DirectX 6/7 <dsound.h> subset, decomp66) ------------------------------------------
#define DS_OK                       0
#define DSSCL_EXCLUSIVE             3
#define DSBCAPS_PRIMARYBUFFER       0x00000001
#define DSBCAPS_STATIC              0x00000002
#define DSBCAPS_LOCSOFTWARE         0x00000008
#define DSBCAPS_CTRL3D              0x00000010
#define DSBCAPS_CTRLPAN             0x00000040
#define DSBCAPS_CTRLVOLUME          0x00000080
#define DSBCAPS_MUTE3DATMAXDISTANCE 0x00020000
#define DSBPLAY_LOOPING             0x00000001
#define DSBSTATUS_PLAYING           0x00000001
#define DS3DMODE_NORMAL             0
#define DS3DMODE_DISABLE            2
#define DS3D_IMMEDIATE              0
#define DS3D_DEFERRED               1

struct DSBUFFERDESC {                 // 0x24 (DirectX 7, with guid3DAlgorithm)
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwBufferBytes;
    DWORD dwReserved;
    WAVEFORMATEX *lpwfxFormat;
    GUID guid3DAlgorithm;
};
LAYOUT_CHECK(DSBUFFERDESC, sizeof(DSBUFFERDESC) == 0x24);
struct DSBCAPS {                      // 0x14
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwBufferBytes;
    DWORD dwUnlockTransferRate;
    DWORD dwPlayCpuOverhead;
};
LAYOUT_CHECK(DSBCAPS, sizeof(DSBCAPS) == 0x14);

struct IDirectSoundBuffer : public IUnknown {          // vtable +0x0c ..
    virtual HRESULT __stdcall GetCaps(DSBCAPS *caps) = 0;                          // +0x0c
    virtual HRESULT __stdcall GetCurrentPosition(DWORD *play, DWORD *write) = 0;   // +0x10
    virtual HRESULT __stdcall GetFormat(WAVEFORMATEX *fmt, DWORD size, DWORD *written) = 0;
    virtual HRESULT __stdcall GetVolume(LONG *vol) = 0;
    virtual HRESULT __stdcall GetPan(LONG *pan) = 0;
    virtual HRESULT __stdcall GetFrequency(DWORD *freq) = 0;                       // +0x20
    virtual HRESULT __stdcall GetStatus(DWORD *status) = 0;                        // +0x24
    virtual HRESULT __stdcall Initialize(void *ds, const DSBUFFERDESC *desc) = 0;
    virtual HRESULT __stdcall Lock(DWORD offset, DWORD bytes, void **p1, DWORD *n1, void **p2, DWORD *n2,
                                   DWORD flags) = 0;                               // +0x2c
    virtual HRESULT __stdcall Play(DWORD reserved1, DWORD priority, DWORD flags) = 0;   // +0x30
    virtual HRESULT __stdcall SetCurrentPosition(DWORD pos) = 0;
    virtual HRESULT __stdcall SetFormat(const WAVEFORMATEX *fmt) = 0;              // +0x38
    virtual HRESULT __stdcall SetVolume(LONG vol) = 0;                             // +0x3c
    virtual HRESULT __stdcall SetPan(LONG pan) = 0;                                // +0x40
    virtual HRESULT __stdcall SetFrequency(DWORD freq) = 0;                        // +0x44
    virtual HRESULT __stdcall Stop() = 0;                                          // +0x48
    virtual HRESULT __stdcall Unlock(void *p1, DWORD n1, void *p2, DWORD n2) = 0;  // +0x4c
    virtual HRESULT __stdcall Restore() = 0;
};
struct IDirectSound : public IUnknown {
    virtual HRESULT __stdcall CreateSoundBuffer(const DSBUFFERDESC *desc, IDirectSoundBuffer **out,
                                                IUnknown *outer) = 0;               // +0x0c
    virtual HRESULT __stdcall GetCaps(void *caps) = 0;
    virtual HRESULT __stdcall DuplicateSoundBuffer(IDirectSoundBuffer *src, IDirectSoundBuffer **out) = 0;   // +0x14
    virtual HRESULT __stdcall SetCooperativeLevel(HWND hwnd, DWORD level) = 0;     // +0x18
    virtual HRESULT __stdcall Compact() = 0;
    virtual HRESULT __stdcall GetSpeakerConfig(DWORD *config) = 0;
    virtual HRESULT __stdcall SetSpeakerConfig(DWORD config) = 0;
    virtual HRESULT __stdcall Initialize(const GUID *guid) = 0;
};
struct IDirectSound3DBuffer : public IUnknown {
    virtual HRESULT __stdcall GetAllParameters(void *p) = 0;                        // +0x0c
    virtual HRESULT __stdcall GetConeAngles(DWORD *inside, DWORD *outside) = 0;
    virtual HRESULT __stdcall GetConeOrientation(void *v) = 0;
    virtual HRESULT __stdcall GetConeOutsideVolume(LONG *vol) = 0;
    virtual HRESULT __stdcall GetMaxDistance(float *d) = 0;
    virtual HRESULT __stdcall GetMinDistance(float *d) = 0;
    virtual HRESULT __stdcall GetMode(DWORD *mode) = 0;
    virtual HRESULT __stdcall GetPosition(void *v) = 0;
    virtual HRESULT __stdcall GetVelocity(void *v) = 0;
    virtual HRESULT __stdcall SetAllParameters(const void *p, DWORD apply) = 0;
    virtual HRESULT __stdcall SetConeAngles(DWORD inside, DWORD outside, DWORD apply) = 0;
    virtual HRESULT __stdcall SetConeOrientation(float x, float y, float z, DWORD apply) = 0;
    virtual HRESULT __stdcall SetConeOutsideVolume(LONG vol, DWORD apply) = 0;
    virtual HRESULT __stdcall SetMaxDistance(float d, DWORD apply) = 0;             // +0x40
    virtual HRESULT __stdcall SetMinDistance(float d, DWORD apply) = 0;             // +0x44
    virtual HRESULT __stdcall SetMode(DWORD mode, DWORD apply) = 0;                 // +0x48
    virtual HRESULT __stdcall SetPosition(float x, float y, float z, DWORD apply) = 0;   // +0x4c
    virtual HRESULT __stdcall SetVelocity(float x, float y, float z, DWORD apply) = 0;   // +0x50
};
struct IDirectSound3DListener : public IUnknown {
    virtual HRESULT __stdcall GetAllParameters(void *p) = 0;                        // +0x0c
    virtual HRESULT __stdcall GetDistanceFactor(float *f) = 0;
    virtual HRESULT __stdcall GetDopplerFactor(float *f) = 0;
    virtual HRESULT __stdcall GetOrientation(void *front, void *top) = 0;
    virtual HRESULT __stdcall GetPosition(void *v) = 0;
    virtual HRESULT __stdcall GetRolloffFactor(float *f) = 0;
    virtual HRESULT __stdcall GetVelocity(void *v) = 0;
    virtual HRESULT __stdcall SetAllParameters(const void *p, DWORD apply) = 0;
    virtual HRESULT __stdcall SetDistanceFactor(float f, DWORD apply) = 0;
    virtual HRESULT __stdcall SetDopplerFactor(float f, DWORD apply) = 0;           // +0x30
    virtual HRESULT __stdcall SetOrientation(float fx, float fy, float fz, float tx, float ty, float tz,
                                             DWORD apply) = 0;                       // +0x34
    virtual HRESULT __stdcall SetPosition(float x, float y, float z, DWORD apply) = 0;   // +0x38
    virtual HRESULT __stdcall SetRolloffFactor(float f, DWORD apply) = 0;           // +0x3c
    virtual HRESULT __stdcall SetVelocity(float x, float y, float z, DWORD apply) = 0;   // +0x40
    virtual HRESULT __stdcall CommitDeferredSettings() = 0;                          // +0x44
};
// The two creation calls (DSOUND.dll ordinal 1 through the thunk 0x57b56a; EAX.DLL ordinal 5 imported
// directly, so the call goes through the IAT).
extern "C" HRESULT __stdcall DirectSoundCreate(const GUID *guid, IDirectSound **out, IUnknown *outer);   // 0x57b56a
extern "C" __declspec(dllimport) HRESULT __stdcall EAXDirectSoundCreate(const GUID *guid, IDirectSound **out,
                                                                         IUnknown *outer);   // [0x57e03c]

// ---- the buffer and channel records -------------------------------------------------------------------
// One loaded sound file (0x250 bytes; ds_buffers 0x2f54720, 256 records).
struct ds_buffer {
    char name[256];                   // +0x000
    char path[256];                   // +0x100
    int offset;                       // +0x200 in the packfile
    int hash;                         // +0x204 ds_find_free_buffer(name), -1 = free record
    int group;                        // +0x208 sound group (snd_group_volumes)
    IDirectSoundBuffer *pdsb;         // +0x20c
    IDirectSound3DBuffer *pds3db;     // +0x210
    WAVEFORMATEX *format;             // +0x214 the file's format (PCM 1 or MS-ADPCM 2)
    WAVEFORMATEX *pcm_format;         // +0x218 what the buffer plays (always PCM)
    HACMSTREAM acm_stream;            // +0x21c streamed ADPCM
    HMMIO hmmio;                      // +0x220 open while loading or streaming
    MMCKINFO data_ck;                 // +0x224 "data"
    MMCKINFO riff_ck;                 // +0x238 "RIFF" / "WAVE"
    int ref_count;                    // +0x24c
};
LAYOUT_CHECK(ds_buffer, sizeof(ds_buffer) == 0x250);
#define DS_MAX_BUFFERS 256

// ds_channel::flags
#define DS_CHANNEL_LOOPING   0x1
#define DS_CHANNEL_PAUSED    0x2
#define DS_CHANNEL_STREAMED  0x4
#define DS_CHANNEL_FADING    0x8
// One playing voice (0x2c bytes; ds_channels 0x2f798b8, 32 records). The RealtimeTimer member gives it the
// implicit constructor ds_channel__ctor (0x55dda0, the array's static initialiser passes it).
struct ds_channel {
    IDirectSoundBuffer *pdsb;         // +0x00 a duplicate of the buffer's
    IDirectSound3DBuffer *pds3db;     // +0x04
    int buffer;                       // +0x08 ds_buffers index, -1
    int stream;                       // +0x0c snd_streams index, -1
    int signature;                    // +0x10 what ds_play* returned, -1
    float volume;                     // +0x14 0..1
    int group;                        // +0x18
    int _unk1c;                       // +0x1c (not used)
    float fade_step;                  // +0x20 volume taken off every 75 ms
    RealtimeTimer fade_timer;         // +0x24
    unsigned int flags;               // +0x28 DS_CHANNEL_*
};
LAYOUT_CHECK(ds_channel, sizeof(ds_channel) == 0x2c);
#define DS_MAX_CHANNELS 32

// ---- the ds unit's .bss (0x2f54720-0x2f8f743) ----------------------------------------------------------
extern ds_buffer ds_buffers[DS_MAX_BUFFERS];      // 0x2f54720
extern int ds_volume_table[101];                  // 0x2f79720 attenuation per volume percent
extern bool ds_eax_enabled;                       // 0x2f798b4
extern bool ds_com_initialized;                   // 0x2f798b5 (never set)
extern ds_channel ds_channels[DS_MAX_CHANNELS];   // 0x2f798b8
extern IDirectSoundBuffer *ds_primary_buffer;     // 0x2f79e38
extern IDirectSound *ds_direct_sound;             // 0x2f79e3c
#define DS_STREAM_SRC_SIZE (44100 * 2 + 100)      // 88300 bytes up to 0x2f8f72c (name ours)
extern ubyte ds_stream_src_buffer[DS_STREAM_SRC_SIZE];   // 0x2f79e40 compressed bytes for one ACM chunk
extern bool ds3d_enabled;                         // 0x2f8f72c
extern IKsPropertySet *ds_eax_property_set;       // 0x2f8f730
extern IDirectSoundBuffer *ds_eax_buffer;         // 0x2f8f734
extern IDirectSound3DBuffer *ds_eax_buffer3d;     // 0x2f8f738
extern bool ds_initialized;                       // 0x2f8f73c
extern int ds_next_signature;                     // 0x2f8f740
// the ds3d unit's .bss (0x2f8f744-0x2f8f749)
extern IDirectSound3DListener *ds3d_listener;     // 0x2f8f744
extern bool ds3d_initialized;                     // 0x2f8f748

// ---- ds_* (argument meaning from the sound unit's calls) -----------------------------------------------
bool ds_is_3d_buffer(IDirectSoundBuffer *buf);    // 0x55bfc0 DSBCAPS_CTRL3D
int  ds_duplicate_buffer(IDirectSoundBuffer *src, int channel);   // 0x55c000 0 / -1
int  ds_eax_init();                               // 0x55c080 the EAX helper buffer and its property set
// (hwnd, rate, bits, channels, ds3d, eax, dsinit) 0 = ok; dsinit: DirectSoundCreate, else EAXDirectSoundCreate.
int  ds_init(HWND hwnd, int rate, int bits, int channels, bool use_ds3d, bool use_eax, bool dsinit);   // 0x55c1a0
void ds_init_buffers();                           // 0x55c3f0
void ds_init_channels();                          // 0x55c430
void ds_channel_init(int channel);                // 0x55c450
void ds_init_volume_table();                      // 0x55c480 1000 log2(percent / 100), [0] -10000
void ds_close();                                  // 0x55c4e0
void ds_close_channels();                         // 0x55c590
void ds_channel_release(int channel);             // 0x55c5b0 nothing while fading
void ds_close_buffers();                          // 0x55c6c0
void ds_sound_release(int buffer);                // 0x55c6e0
// (sound name, file path, offset in the packfile, group, streamed) -> buffer, < 0 failed.
int  ds_load_buffer(const char *name, const char *path, int offset, int group, bool streamed);  // 0x55c890 (D11903)
int  ds_load_buffer_data(int buffer, bool streamed);   // 0x55c990
int  ds_create_buffer(int buffer);                // 0x55ca10 PCM data into a static buffer
int  ds_fill_buffer(HMMIO h, MMCKINFO *ck, IDirectSoundBuffer *buf, unsigned int size);   // 0x55cae0
int  ds_get_3d_interface(IDirectSoundBuffer *buf, IDirectSound3DBuffer **out);   // 0x55cb60
int  ds_load_buffer_acm(int buffer);              // 0x55cbe0 MS-ADPCM converted whole (no 3D interface)
int  ds_wav_open(int buffer);                     // 0x55cd80
// sum.symbols' name; the body is the name hash of the buffers (rol 6, xor), -1 for NULL (decomp66).
int  ds_find_free_buffer(const char *name);       // 0x55ce30
int  ds_find_buffer(const char *name);            // 0x55ce60 the loaded one, else the first free, -1
void ds_unload_buffer(int buffer);                // 0x55ced0
// (buffer, pos, velocity, min range, max range, volume, looping, priority) -> signature.
int  ds_play_3d(int buffer, const vector3 *pos, const vector3 *vel, float min_range, float max_range,
                float volume, bool looping, int a);       // 0x55cf00
int  ds_volume_to_attenuation(float volume);      // 0x55d070
int  ds_find_free_channel(int priority);          // 0x55d0a0 (the argument is not read)
int  ds_play(int buffer, float volume, float pan, int looping, int a);   // 0x55d110
// decomp66 (D4603): the third argument is the pan (it was int a); the fifth goes to
// ds_find_free_channel (the sound unit passes 1.0f).
int  ds_play_streamed(int buffer, float volume, float pan, bool looping, float priority);   // 0x55d2b0
int  ds_stream_open(int buffer, int channel);     // 0x55d400 a 2-second ring buffer, the first part filled
int  ds_stream_read_chunk(HMMIO h, MMCKINFO *ck, HACMSTREAM s, IDirectSoundBuffer *buf, unsigned int size);   // 0x55d530
void ds_stop(int sig);                            // 0x55d620
void ds_stop_fade(int sig, int ms);               // 0x55d640
void ds_do_frame();                               // 0x55d6c0
void ds_channel_update_fade(int channel);         // 0x55d6f0
int  ds_channel_is_playing(int channel);          // 0x55d780 DSBSTATUS_PLAYING
void ds_pause(int sig);                           // 0x55d7b0 (no symbol, name ours; no caller)
void ds_pause_all(int all);                       // 0x55d800 streams too when all
void ds_unpause(int sig);                         // 0x55d850
void ds_channel_unpause(int channel);             // 0x55d880
void ds_unpause_all();                            // 0x55d8c0
void ds_stop_all();                               // 0x55d8f0
void ds_set_volume(int sig, float volume);        // 0x55d910
void ds_set_pan(int sig, float pan);              // 0x55d960
void ds_set_frequency(int sig, unsigned int freq);   // 0x55d9a0 (no check of the channel index)
bool ds_is_playing(int sig);                      // 0x55da20
void ds_get_format(int buffer, int *rate, int *bits, int *channels);   // 0x55da50
int  ds_get_frequency(int sig, unsigned int *freq);   // 0x55dac0
int  ds_get_channel(int sig);                     // 0x55db00
int  ds_get_num_playing_channels();               // 0x55db30 (name ours; no symbol)
IDirectSoundBuffer *ds_get_primary_buffer();      // 0x55db50
IDirectSound *ds_get_direct_sound();              // 0x55db60
IKsPropertySet *ds_get_eax_property_set();        // 0x55db70
const char *ds_error_string(HRESULT hr);          // 0x55db80
bool ds_is_ds3d_enabled();                        // 0x55dc70
bool ds_is_eax_enabled();                         // 0x55dc80
void ds_disable_3d_eax();                         // 0x55dc90
void ds_disable_eax();                            // 0x55dca0
void ds_set_group_volume(int group, float volume);   // 0x55dcb0 every channel of the group (-1: all)
float ds_estimate_duration(int buffer);           // 0x55dd10
void ds_set_eax_environment(const char *name);    // 0x55dd80
// ---- ds3d_* (the second object, 0x55ddb0-0x55e03f) ---------------------------------------------------
int  ds3d_update_buffer(int channel, float min_range, float max_range, const vector3 *pos, const vector3 *vel);   // 0x55ddb0
int  ds3d_update_listener(const vector3 *pos, const vector3 *vel, const matrix *orient);   // 0x55de30
void ds3d_close();                                // 0x55def0
void ds3d_release_listener();                     // 0x55df10
int  ds3d_init(bool eax);                         // 0x55df30
int  ds3d_init_listener();                        // 0x55dfe0

#endif
