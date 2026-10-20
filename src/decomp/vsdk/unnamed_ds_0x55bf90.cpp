// unnamed_ds_0x55bf90.cpp - Sum.exe v1.40, code 0x55bf90-0x55ddae: the DirectSound layer (ds_*) under the
// sound library: buffers loaded from wave files, 32 channels, streaming, fades, EAX. The file name is a
// placeholder: no path string, no strings.tbl block. The functions and records have the shape of the
// FreeSpace 2 ds.cpp (Dash Faction's rf/sound/sound_ds.h has the same record sizes); names only.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): the static initialiser 0x55bf90 (table entry 649, the channel array)
// opens the object; ds_set_eax_environment (0x55dd80) is the last regular function and ds_channel's
// implicit constructor (0x55dda0) its COMDAT; the ds3d_* functions after it are a second object
// (unnamed_ds3d_0x55ddb0.cpp: their own .data GUID and .bss). .data 0x5add24-0x5ae0b7 (the error texts;
// the sprintf targets are locals, the messages are never shown), .bss 0x2f54720-0x2f8f743.
//
// Every error path formats its message into a local buffer and drops it: the debug output macro of the
// original evidently expanded to the sprintf alone in this build.

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "vsdk/snd/ds.h"
#include "vsdk/snd/snd.h"
#include "vsdk/snd/unnamed_sndstream_0x566560.h"
#include "vsdk/math/vector.h"

// ---- GUIDs (ds.h) ------------------------------------------------------------------------------------
// In Sum.exe these are library .rdata, in the run of GUIDs at 0x5832c8-0x5833cf (with dinput.lib's
// c_dfDIMouse, dxguid.lib's GUID_SysMouse, IID_IDirect3D7 and IID_IDirectDraw7, and the two EAX property
// sets, defined in the EAX unit): an initguid object, not this unit's data. ds.h declares them as C++
// names, so the libraries' C symbols do not resolve them; defined here, values read from Sum.exe.
extern const GUID IID_IKsPropertySet =                   // 0x5832e0 {31efac30-515c-11d0-a9aa-00aa0061be93}
    { 0x31efac30, 0x515c, 0x11d0, { 0xa9, 0xaa, 0x00, 0xaa, 0x00, 0x61, 0xbe, 0x93 } };
extern const GUID IID_IDirectSound3DBuffer =             // 0x5832f0 {279afa86-4981-11ce-a521-0020af0be560}
    { 0x279afa86, 0x4981, 0x11ce, { 0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60 } };
extern const GUID IID_IDirectSound3DListener =           // 0x583300 {279afa84-4981-11ce-a521-0020af0be560}
    { 0x279afa84, 0x4981, 0x11ce, { 0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60 } };

// ---- .bss -------------------------------------------------------------------------------------------
// 0x55bf90  init_ds_channels (jmp 0x55bfa0). Confidence: exact (compiler-generated).
// 0x55bfa0  the initialiser body: `vector constructor iterator` (0x404880) over the 32 channels with
//           ds_channel__ctor (0x55dda0). Confidence: exact (compiler-generated).
ds_channel ds_channels[DS_MAX_CHANNELS];          // 0x2f798b8
ds_buffer ds_buffers[DS_MAX_BUFFERS];             // 0x2f54720
int ds_volume_table[101];                         // 0x2f79720
bool ds_eax_enabled;                              // 0x2f798b4
bool ds_com_initialized;                          // 0x2f798b5
IDirectSoundBuffer *ds_primary_buffer;            // 0x2f79e38
IDirectSound *ds_direct_sound;                    // 0x2f79e3c
ubyte ds_stream_src_buffer[DS_STREAM_SRC_SIZE];   // 0x2f79e40
bool ds3d_enabled;                                // 0x2f8f72c
IKsPropertySet *ds_eax_property_set;              // 0x2f8f730
IDirectSoundBuffer *ds_eax_buffer;                // 0x2f8f734
IDirectSound3DBuffer *ds_eax_buffer3d;            // 0x2f8f738
bool ds_initialized;                              // 0x2f8f73c
int ds_next_signature;                            // 0x2f8f740

// 0x55bfc0  Confidence: likely (differs under cl 8966 in match26).
bool ds_is_3d_buffer(IDirectSoundBuffer *buf)
{
    DSBCAPS caps;

    caps.dwSize = sizeof(caps);
    if (buf->GetCaps(&caps) >= 0)
        return (caps.dwFlags >> 4) & 1;
    return false;
}

// 0x55c000  Confidence: exact.
int ds_duplicate_buffer(IDirectSoundBuffer *src, int channel)
{
    char msg[256];
    HRESULT hr;

    hr = ds_direct_sound->DuplicateSoundBuffer(src, &ds_channels[channel].pdsb);
    if (hr < 0) {
        sprintf(msg, "DuplicateSoundBuffer failed because: %s\n", ds_error_string(hr));
        ds_channels[channel].pdsb = NULL;
        ds_channels[channel].pds3db = NULL;
        return -1;
    }
    return 0;
}

// 0x55c080  Confidence: exact. A small 3D buffer whose property set
// carries the EAX listener settings (22050 Hz, 16-bit mono, 32 bytes).
int ds_eax_init()
{
    WAVEFORMATEX wfx;
    DSBUFFERDESC desc;
    char msg[256];
    HRESULT hr;

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    memset(&desc, 0, sizeof(desc));
    desc.lpwfxFormat = &wfx;
    wfx.nSamplesPerSec = 22050;
    wfx.wBitsPerSample = 16;
    wfx.cbSize = 0;
    wfx.nBlockAlign = 2;
    wfx.nAvgBytesPerSec = 44100;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_MUTE3DATMAXDISTANCE | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRL3D |
                   DSBCAPS_STATIC;
    desc.dwBufferBytes = 32;
    if (ds_direct_sound->CreateSoundBuffer(&desc, &ds_eax_buffer, NULL) < 0) {
        ds_eax_property_set = NULL;
        return -1;
    }
    if (ds_eax_buffer->QueryInterface(IID_IDirectSound3DBuffer, (void **)&ds_eax_buffer3d) < 0) {
        ds_eax_buffer3d = NULL;
        return -1;
    }
    hr = ds_eax_buffer3d->QueryInterface(IID_IKsPropertySet, (void **)&ds_eax_property_set);
    if (hr >= 0 && ds_eax_property_set != NULL)
        return 0;
    sprintf(msg, "Failed to get sound card property set: %s\n", ds_error_string(hr));
    return -1;
}

// 0x55c1a0  Confidence: exact. The retry without 3D after a failed primary
// buffer reads through ds_direct_sound after setting it to NULL (sound.md; a crash on that path).
int ds_init(HWND hwnd, int rate, int bits, int channels, bool use_ds3d, bool use_eax, bool dsinit)
{
    DSBUFFERDESC desc;
    WAVEFORMATEX wfx;
    char msg[256];
    HMODULE dll;
    HRESULT hr;

    if (ds_initialized == true)
        return 0;
    ds_com_initialized = false;
    dll = LoadLibrary("dsound.dll");
    if (dll == NULL)
        return -1;
    FreeLibrary(dll);
    ds3d_enabled = use_ds3d;
    ds_eax_enabled = use_eax;
    if (use_eax)
        ds3d_enabled = true;
    if (!dsinit)
        hr = EAXDirectSoundCreate(NULL, &ds_direct_sound, NULL);
    else
        hr = DirectSoundCreate(NULL, &ds_direct_sound, NULL);
    if (hr < 0) {
        sprintf(msg, "DirectSoundCreate() failed with return value: %s\n", ds_error_string(hr));
        return -1;
    }
    hr = ds_direct_sound->SetCooperativeLevel(hwnd, DSSCL_EXCLUSIVE);
    if (hr < 0) {
        ds_direct_sound = NULL;
        sprintf(msg, "SetCooperativeLevel() failed with return value: %s\n", ds_error_string(hr));
        return -1;
    }
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = ds3d_enabled == true ? DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D : DSBCAPS_PRIMARYBUFFER;
    if (ds_direct_sound->CreateSoundBuffer(&desc, &ds_primary_buffer, NULL) < 0) {
        ds_direct_sound->Release();
        ds_direct_sound = NULL;
        if (ds3d_enabled == true) {
            desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
            ds3d_enabled = false;
            ds_eax_enabled = false;
            if (ds_direct_sound->CreateSoundBuffer(&desc, &ds_primary_buffer, NULL) < 0) {
                ds_direct_sound->Release();
                ds_direct_sound = NULL;
                return -1;
            }
        }
    }
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = (WORD)channels;
    wfx.nSamplesPerSec = rate;
    wfx.wBitsPerSample = (WORD)bits;
    wfx.nBlockAlign = (WORD)(wfx.wBitsPerSample / 8 * wfx.nChannels);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;
    ds_primary_buffer->SetFormat(&wfx);
    ds_init_buffers();
    ds_init_channels();
    snd_stream_init();
    if (ds3d_enabled == true) {
        ds_eax_init();
        if (ds3d_init(true) != 0) {
            ds3d_enabled = false;
            ds_eax_enabled = false;
        }
    }
    if (ds_eax_enabled == true)
        eax_init();
    ds_init_volume_table();
    ds_initialized = true;
    return 0;
}

// 0x55c3f0  Confidence: exact. The format pointer (+0x214) is not cleared.
void ds_init_buffers()
{
    int i;

    for (i = 0; i < DS_MAX_BUFFERS; i++) {
        ds_buffers[i].pdsb = NULL;
        ds_buffers[i].pds3db = NULL;
        ds_buffers[i].pcm_format = NULL;
        ds_buffers[i].hmmio = NULL;
        ds_buffers[i].name[0] = '\0';
        ds_buffers[i].path[0] = '\0';
        ds_buffers[i].hash = -1;
        ds_buffers[i].ref_count = 0;
        ds_buffers[i].offset = 0;
    }
}

// 0x55c430  Confidence: exact.
void ds_init_channels()
{
    int i;

    for (i = 0; i < DS_MAX_CHANNELS; i++)
        ds_channel_init(i);
}

// 0x55c450  Confidence: exact.
void ds_channel_init(int channel)
{
    ds_channel *c = &ds_channels[channel];

    c->pdsb = NULL;
    c->pds3db = NULL;
    c->stream = -1;
    c->buffer = -1;
    c->signature = -1;
    c->flags = 0;
}

// 0x55c480  Confidence: exact (log is the inline FYL2X; log(2.0) is hoisted out of the loop).
void ds_init_volume_table()
{
    int i;

    ds_volume_table[0] = -10000;
    for (i = 1; i <= 100; i++)
        ds_volume_table[i] = (int)(log(i * 0.01f) / log(2.0) * 1000.0);
}

// 0x55c4e0  Confidence: exact.
void ds_close()
{
    if (!ds_initialized)
        return;
    ds_close_channels();
    ds_close_buffers();
    snd_stream_close_all();
    eax_close();
    ds3d_close();
    if (ds_eax_property_set != NULL) {
        ds_eax_property_set->Release();
        ds_eax_property_set = NULL;
    }
    if (ds_eax_buffer != NULL) {
        ds_eax_buffer->Release();
        ds_eax_buffer = NULL;
    }
    if (ds_eax_buffer3d != NULL) {
        ds_eax_buffer3d->Release();
        ds_eax_buffer3d = NULL;
    }
    if (ds_primary_buffer != NULL) {
        ds_primary_buffer->Release();
        ds_primary_buffer = NULL;
    }
    if (ds_direct_sound != NULL) {
        ds_direct_sound->Release();
        ds_direct_sound = NULL;
    }
    acm_close();
    if (ds_com_initialized == true)
        CoUninitialize();
    ds_initialized = false;
}

// 0x55c590  Confidence: exact.
void ds_close_channels()
{
    int i;

    for (i = 0; i < DS_MAX_CHANNELS; i++)
        ds_channel_release(i);
}

// 0x55c5b0  Confidence: exact. The Release retry loops try up to ten times while Release reports an
// error, which a reference count never does. A fading channel is left alone: its fade releases it.
void ds_channel_release(int channel)
{
    char msg[256];
    ds_channel *c = &ds_channels[channel];
    HRESULT hr;
    int tries;

    if (c->flags & DS_CHANNEL_FADING)
        return;
    if (c->stream >= 0) {
        EnterCriticalSection(&snd_stream_lock);
        snd_stream_stop(c->stream);
        LeaveCriticalSection(&snd_stream_lock);
    }
    if (c->pdsb != NULL) {
        hr = c->pdsb->Stop();
        if (hr < 0)
            sprintf(msg, "c->pdsb->Stop() failed with return value %s\n", ds_error_string(hr));
        if (c->pds3db != NULL) {
            tries = 1;
            while (1) {
                hr = c->pds3db->Release();
                if (hr >= 0)
                    break;
                sprintf(msg, "c->pds3db->Release() failed with return value %s\n", ds_error_string(hr));
                if (tries++ >= 10)
                    break;
            }
        }
        tries = 1;
        while (1) {
            hr = c->pdsb->Release();
            if (hr >= 0)
                break;
            sprintf(msg, "c->pdsb->Release() failed with return value %s\n", ds_error_string(hr));
            if (tries++ >= 10)
                break;
        }
    }
    ds_channel_init(channel);
}

// 0x55c6c0  Confidence: exact.
void ds_close_buffers()
{
    int i;

    for (i = 0; i < DS_MAX_BUFFERS; i++)
        ds_sound_release(i);
}

// 0x55c6e0  Confidence: exact.
void ds_sound_release(int buffer)
{
    char msg[256];
    ds_buffer *b;
    HRESULT hr;
    int tries;
    int i;

    if (buffer < 0)
        return;
    b = &ds_buffers[buffer];
    for (i = 0; i < SND_MAX_STREAMS; i++) {
        if ((snd_streams[i].flags & SND_STREAM_USED) && snd_streams[i].buffer == buffer) {
            EnterCriticalSection(&snd_stream_lock);
            snd_stream_stop(i);
            LeaveCriticalSection(&snd_stream_lock);
        }
    }
    if (b->pcm_format != NULL) {
        free(b->pcm_format);
        b->pcm_format = NULL;
    }
    if (b->format != NULL) {
        free(b->format);
        b->format = NULL;
    }
    if (b->hmmio != NULL)
        wav_close(&b->hmmio);
    if (b->pdsb != NULL) {
        hr = b->pdsb->Stop();
        if (hr < 0)
            sprintf(msg, "s->pdsb->Stop() failed with return value %s\n", ds_error_string(hr));
        if (b->pds3db != NULL) {
            tries = 1;
            while (1) {
                hr = b->pds3db->Release();
                if (hr >= 0)
                    break;
                sprintf(msg, "s->pds3db->Release() failed with return value %s\n", ds_error_string(hr));
                if (tries++ >= 10)
                    break;
            }
            b->pds3db = NULL;
        }
        if (b->pdsb != NULL) {
            tries = 1;
            while (1) {
                hr = b->pdsb->Release();
                if (hr >= 0)
                    break;
                sprintf(msg, "s->pdsb->Release() failed with return value %s\n", ds_error_string(hr));
                if (tries++ >= 10)
                    break;
            }

            b->pdsb = NULL;
        }
    }
    b->name[0] = '\0';
    b->path[0] = '\0';
    b->hash = -1;
}

// 0x55c890  Confidence: exact (VC6 8966 byte match, m-1020: streamed is bool, passed on unconverted; the last
// reference count through ds_buffers[i], which VC6 cross-jumps with the first, D11903) (the shared exits). A name already loaded gets one more reference.
int ds_load_buffer(const char *name, const char *path, int offset, int group, bool streamed)
{
    int i;
    ds_buffer *b;

    i = ds_find_buffer(name);
    if (i >= 0 && ds_buffers[i].hash != -1) {
        ds_buffers[i].ref_count++;
        return i;
    }
    if (i == -1)
        return -1;
    b = &ds_buffers[i];
    strcpy(b->name, name);
    strcpy(b->path, path);
    b->offset = offset;
    b->group = group;
    if (ds_load_buffer_data(i, streamed) == -1) {
        b->name[0] = '\0';
        b->path[0] = '\0';
        return -1;
    }
    b->hash = ds_find_free_buffer(name);
    ds_buffers[i].ref_count++;
    return i;
}

// 0x55c990  Confidence: exact. A streamed sound keeps its file open.
int ds_load_buffer_data(int buffer, bool streamed)
{
    ds_buffer *b;
    int r = 0;

    if (ds_wav_open(buffer) != 0)
        return -1;
    if (streamed == true)
        return 0;
    b = &ds_buffers[buffer];
    switch (b->format->wFormatTag) {
    case WAVE_FORMAT_ADPCM:
        if (ds_load_buffer_acm(buffer) != 0)
            goto fail;
        break;
    case WAVE_FORMAT_PCM:
        if (ds_create_buffer(buffer) == 0)
            break;
    default:
    fail:
        r = -1;
        break;
    }
    wav_close(&b->hmmio);
    return r;
}

// 0x55ca10  Confidence: exact (VC6 8966 byte match, m-1020: the store order size, flags, bytes, format found by
// trying all 24, D11902) (the flag selection is the `neg / sbb / and / add` of a ?:).
int ds_create_buffer(int buffer)
{
    ds_buffer *b = &ds_buffers[buffer];
    DSBUFFERDESC desc;
    int r = 0;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = ds3d_enabled == true ? 0x200d2 : 0xca;
    desc.dwBufferBytes = b->data_ck.cksize;
    desc.lpwfxFormat = b->pcm_format;
    if (ds_direct_sound->CreateSoundBuffer(&desc, &b->pdsb, NULL) < 0 || b->pdsb == NULL ||
        ds_fill_buffer(b->hmmio, &b->data_ck, b->pdsb, b->data_ck.cksize) != 0)
        r = -1;
    if (ds3d_enabled == true)
        ds_get_3d_interface(b->pdsb, &b->pds3db);
    return r;
}

// 0x55cae0  Confidence: exact (VC6 8966 byte match, m-1020: the success branch first, got local to it;
// the locals share the argument slots, D11902).
int ds_fill_buffer(HMMIO h, MMCKINFO *ck, IDirectSoundBuffer *buf, unsigned int size)
{
    void *p1;
    DWORD n1;
    int r = 0;

    if (buf->Lock(0, size, &p1, &n1, NULL, NULL, 0) >= 0) {
        UINT got;

        if (wav_read(h, n1, (BYTE *)p1, ck, &got) != 0)
            r = -1;
        buf->Unlock(p1, n1, NULL, 0);
        return r;
    }
    buf->Release();
    return -1;
}

// 0x55cb60  Confidence: exact.
int ds_get_3d_interface(IDirectSoundBuffer *buf, IDirectSound3DBuffer **out)
{
    DSBCAPS caps;
    char msg[256];
    HRESULT hr;

    caps.dwSize = sizeof(caps);
    hr = buf->GetCaps(&caps);
    if (hr >= 0 && (caps.dwFlags & DSBCAPS_CTRL3D)) {
        hr = buf->QueryInterface(IID_IDirectSound3DBuffer, (void **)out);
        if (hr >= 0)
            return 0;
    }
    sprintf(msg, "Could not obtain 3D interface for hardware buffer: %s\n", ds_error_string(hr));
    *out = NULL;
    return -1;
}

// 0x55cbe0  Confidence: likely (the cleanup order). The whole ADPCM file is converted into a static
// buffer; unlike ds_create_buffer it asks for no 3D interface.
int ds_load_buffer_acm(int buffer)
{
    ds_buffer *b = &ds_buffers[buffer];
    HACMSTREAM stream = NULL;
    void *src = NULL;
    WAVEFORMATEX pcm;
    DSBUFFERDESC desc;
    unsigned int size;
    unsigned int dst_size;
    UINT got;
    void *p1;
    DWORD n1;
    DWORD dst_used;
    DWORD src_used;
    int rc = 0;

    size = b->data_ck.cksize;
    src = malloc(size);
    if (src == NULL)
        goto fail;
    if (wav_read(b->hmmio, size, (BYTE *)src, &b->data_ck, &got) != 0)
        goto fail;
    if (acm_stream_open(b->format, &pcm, &stream, 16) != 0)
        goto fail;
    dst_size = acm_stream_size_b(stream, size);
    memset(&desc, 0, sizeof(desc));
    desc.lpwfxFormat = &pcm;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = ds3d_enabled == true ? 0x200d2 : 0xca;
    desc.dwBufferBytes = dst_size;
    if (ds_direct_sound->CreateSoundBuffer(&desc, &b->pdsb, NULL) < 0)
        goto fail;
    if (b->pdsb->Lock(0, dst_size, &p1, &n1, NULL, NULL, 0) < 0 ||
        acm_convert(stream, src, size, p1, dst_size, &dst_used, &src_used) != 0) {
        b->pdsb->Release();
        goto fail;
    }
    b->pdsb->Unlock(p1, n1, NULL, 0);
    acm_stream_close(stream);
    stream = NULL;
    goto done;
fail:
    rc = -1;
done:
    if (src != NULL)
        free(src);
    if (stream != NULL)
        acm_stream_close(stream);
    return rc;
}

// 0x55cd80  Confidence: exact.
int ds_wav_open(int buffer)
{
    ds_buffer *b;

    if (!ds_initialized)
        return -1;
    if (buffer < 0)
        return -1;
    b = &ds_buffers[buffer];
    if (wav_open(b->path, b->offset, &b->hmmio, &b->format, &b->pcm_format, &b->riff_ck) != 0) {
        wav_close(&b->hmmio);
        return -1;
    }
    if (wav_descend_data(&b->hmmio, &b->data_ck, &b->riff_ck) != 0) {
        wav_close(&b->hmmio);
        return -1;
    }
    return 0;
}

// 0x55ce30  Confidence: exact (_rotl is the intrinsic ROL). sum.symbols calls it ds_find_free_buffer;
// it is the name hash ds_find_buffer and ds_load_buffer use.
int ds_find_free_buffer(const char *name)
{
    unsigned int h;

    if (name == NULL)
        return -1;
    h = 0;
    while (*name != '\0')
        h = _rotl(h, 6) ^ *name++;
    return h;
}

// 0x55ce60  Confidence: exact.
int ds_find_buffer(const char *name)
{
    int free_index = -1;
    int hash = ds_find_free_buffer(name);
    int i;

    for (i = 0; i < DS_MAX_BUFFERS; i++) {
        if (ds_buffers[i].hash == hash && _stricmp(ds_buffers[i].name, name) == 0)
            return i;
        if (free_index == -1 && ds_buffers[i].hash == -1)
            free_index = i;
    }
    return free_index;
}

// 0x55ced0  Confidence: exact.
void ds_unload_buffer(int buffer)
{
    if (buffer < 0)
        return;
    if (--ds_buffers[buffer].ref_count == 0)
        ds_sound_release(buffer);
}

// 0x55cf00  Confidence: exact. The failures after ds_duplicate_buffer leave the duplicate in the channel
// (it is released when the channel is next taken).
int ds_play_3d(int buffer, const vector3 *pos, const vector3 *vel, float min_range, float max_range,
               float volume, bool looping, int a)
{
    ds_buffer *b;
    ds_channel *c;
    int ch;

    if (!ds_initialized)
        return -1;
    if (buffer < 0)
        return -1;
    ch = ds_find_free_channel(a);
    if (ch == -1)
        return -1;
    b = &ds_buffers[buffer];
    c = &ds_channels[ch];
    if (ds_duplicate_buffer(b->pdsb, ch) != 0)
        return -1;
    if (!ds_is_3d_buffer(c->pdsb))
        return -1;
    ds_get_3d_interface(c->pdsb, &c->pds3db);
    if (c->pds3db == NULL)
        return -1;
    c->group = b->group;
    c->pdsb->SetVolume(ds_volume_to_attenuation(volume));
    c->pds3db->SetMode(DS3DMODE_NORMAL, DS3D_IMMEDIATE);
    ds3d_update_buffer(ch, min_range, max_range, pos, vel);
    if (looping == true)
        c->flags |= DS_CHANNEL_LOOPING;
    c->volume = volume;
    c->buffer = buffer;
    DWORD play_flags = 0;
    if (looping == true)
        play_flags = DSBPLAY_LOOPING;
    if (c->pdsb->Play(0, 0, play_flags) < 0) {

        ds_channel_release(ch);
        return -1;
    }
    c->signature = ds_next_signature++;
    if (ds_next_signature < 0)
        ds_next_signature = 0;
    return c->signature;
}

// 0x55d070  Confidence: exact.
int ds_volume_to_attenuation(float volume)
{
    int v = (int)(volume * 100.0f);

    if (v > 100)
        v = 100;
    else if (v < 0)
        v = 0;
    return ds_volume_table[v];
}

// 0x55d0a0  Confidence: exact. A channel without a buffer, or a stopped one that is
// neither looping nor streamed (released first). The argument is not read.
int ds_find_free_channel(int priority)
{
    DWORD status;
    int free_channel = -1;
    int i;

    for (i = 0; i < DS_MAX_CHANNELS; i++) {
        if (ds_channels[i].pdsb == NULL) {
            free_channel = i;
            break;
        }
        if (!(ds_channels[i].flags & (DS_CHANNEL_LOOPING | DS_CHANNEL_STREAMED))) {
            if (ds_channels[i].pdsb->GetStatus(&status) < 0)
                return -1;
            if (!(status & DSBSTATUS_PLAYING)) {
                free_channel = i;
                ds_channel_release(i);
                break;
            }
        }
    }
    return free_channel;
}

// 0x55d110  Confidence: likely. A 3D buffer played in 2D has its 3D processing switched off.
int ds_play(int buffer, float volume, float pan, int looping, int a)
{
    ds_buffer *b;
    ds_channel *c;
    int ch;

    if (!ds_initialized)
        return -1;
    if (buffer < 0)
        return -1;
    ch = ds_find_free_channel(a);
    if (ch == -1)
        return -1;
    b = &ds_buffers[buffer];
    c = &ds_channels[ch];
    if (ds_duplicate_buffer(b->pdsb, ch) != 0)
        return -1;
    if (ds_is_3d_buffer(c->pdsb) == true) {
        ds_get_3d_interface(c->pdsb, &c->pds3db);
        if (c->pds3db != NULL)
            c->pds3db->SetMode(DS3DMODE_DISABLE, DS3D_IMMEDIATE);
        if (ds_eax_enabled)
            eax_buffer_init(c->pdsb);
    }
    if ((char)looping == 1)
        c->flags |= DS_CHANNEL_LOOPING;
    c->pdsb->SetVolume(ds_volume_to_attenuation(volume));
    c->volume = volume;
    c->group = b->group;
    c->pdsb->SetPan((int)(pan * 1000.0f));
    c->buffer = buffer;
    if (c->pdsb->Play(0, 0, (char)looping ? DSBPLAY_LOOPING : 0) < 0) {
        ds_channel_release(ch);
        return -1;
    }
    c->signature = ds_next_signature++;
    if (ds_next_signature < 0)
        ds_next_signature = 0;
    return c->signature;
}

// 0x55d2b0  Confidence: exact (VC6 byte match, m-1019 w3; was likely). Always played looping: the stream refills the ring buffer.
int ds_play_streamed(int buffer, float volume, float pan, bool looping, float priority)
{
    ds_channel *c;
    int ch;

    if (!ds_initialized)
        return -1;
    if (buffer < 0)
        return -1;
    ch = ds_find_free_channel(*(int *)&priority);   // the binary pushes the float bits unconverted
    if (ch == -1)
        return -1;
    if (ds_stream_open(buffer, ch) != 0)
        return -1;
    ds_channels[ch].group = ds_buffers[buffer].group;
    ds_channels[ch].flags |= DS_CHANNEL_STREAMED;
    c = &ds_channels[ch];
    c->pdsb->SetVolume(ds_volume_to_attenuation(volume));
    c->volume = volume;
    c->pdsb->SetPan((int)(pan * 1000.0f));
    c->buffer = buffer;
    c->stream = snd_stream_start(buffer, ch, looping);
    if (c->stream < 0) {
        ds_channel_release(ch);
        return -1;
    }
    if (c->pdsb->Play(0, 0, DSBPLAY_LOOPING) < 0) {
        ds_channel_release(ch);
        return -1;
    }
    c->signature = ds_next_signature++;
    if (ds_next_signature < 0)
        ds_next_signature = 0;
    return c->signature;
}

// 0x55d400  Confidence: likely (the order of the descriptor stores).
int ds_stream_open(int buffer, int channel)
{
    ds_buffer *b = &ds_buffers[buffer];
    ds_channel *c = &ds_channels[channel];
    DSBUFFERDESC desc;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_LOCSOFTWARE;
    desc.lpwfxFormat = b->pcm_format;
    desc.dwBufferBytes = b->pcm_format->nAvgBytesPerSec * 2;
    if (ds_direct_sound->CreateSoundBuffer(&desc, &c->pdsb, NULL) < 0 || c->pdsb == NULL)
        return -1;
    wav_descend_data(&b->hmmio, &b->data_ck, &b->riff_ck);
    if (b->format->wFormatTag == WAVE_FORMAT_ADPCM) {
        if (acm_stream_open(b->format, b->pcm_format, &b->acm_stream, b->pcm_format->wBitsPerSample) != 0)
            return -1;
        if (ds_stream_read_chunk(b->hmmio, &b->data_ck, b->acm_stream, c->pdsb, desc.dwBufferBytes) != 0)
            return -1;
    } else if (ds_fill_buffer(b->hmmio, &b->data_ck, c->pdsb, desc.dwBufferBytes) != 0) {
        return -1;
    }
    return 0;
}

// 0x55d530  Confidence: exact (VC6 8966 byte match, m-1020: the locked branch first with the size call hoisted,
// the lock failure an else that sets r, D11902). A failed conversion returns with the buffer still locked.
int ds_stream_read_chunk(HMMIO h, MMCKINFO *ck, HACMSTREAM s, IDirectSoundBuffer *buf, unsigned int size)
{
    void *p1;
    DWORD n1;
    UINT got;
    DWORD dst_used;
    DWORD src_used;
    int r = 0;

    if (buf->Lock(0, size, &p1, &n1, NULL, NULL, 0) >= 0) {
        UINT want = acm_stream_size(s, n1);

        if (wav_read(h, want, ds_stream_src_buffer, ck, &got) != 0)
            r = -1;
        if (got > 0) {
            if (acm_convert(s, ds_stream_src_buffer, got, p1, n1, &dst_used, &src_used) == -1)
                return -1;
            if (src_used < got)
                mmioSeek(h, src_used - got, SEEK_CUR);
        }
        buf->Unlock(p1, n1, NULL, 0);
    } else {
        buf->Release();
        r = -1;
    }
    return r;
}

// 0x55d620  Confidence: exact.
void ds_stop(int sig)
{
    int ch = ds_get_channel(sig);

    if (ch >= 0)
        ds_channel_release(ch);
}

// 0x55d640  Confidence: exact. Below 75 ms the sound stops at once; else the volume drops every 75 ms by
// volume / (ms / 75) (ds_do_frame).
void ds_stop_fade(int sig, int ms)
{
    ds_channel *c;
    int ch;

    if (ms < 75) {
        ds_stop(sig);
        return;
    }
    ch = ds_get_channel(sig);
    if (ch < 0)
        return;
    c = &ds_channels[ch];
    c->flags |= DS_CHANNEL_FADING;
    c->fade_timer.SetTimeout(0);
    c->fade_step = c->volume / (ms / 75);
}

// 0x55d6c0  Confidence: exact.
void ds_do_frame()
{
    int i;

    for (i = 0; i < DS_MAX_CHANNELS; i++) {
        if (ds_channels[i].flags & DS_CHANNEL_FADING)
            ds_channel_update_fade(i);
    }
}

// 0x55d6f0  Confidence: exact. A fading sound that ends by itself keeps DS_CHANNEL_FADING, so
// ds_channel_release never frees the channel and the next sound played on it inherits the fade
// (game bug, NOTES.md).
void ds_channel_update_fade(int channel)
{
    ds_channel *c;
    float v;

    if (!ds_channel_is_playing(channel))
        return;
    c = &ds_channels[channel];
    if (c->fade_timer.elapsed() == true) {
        c->fade_timer.SetTimeout(75);
        v = c->volume - c->fade_step;
        c->volume = v;
        if (v <= 0.0f) {
            c->flags &= ~DS_CHANNEL_FADING;
            ds_channel_release(channel);
            return;
        }
        c->pdsb->SetVolume(ds_volume_to_attenuation(v));
    }
}

// 0x55d780  Confidence: exact.
int ds_channel_is_playing(int channel)
{
    IDirectSoundBuffer *buf = ds_channels[channel].pdsb;
    DWORD status;

    if (buf == NULL)
        return 0;
    if (buf->GetStatus(&status) < 0)
        return 0;
    return status & DSBSTATUS_PLAYING;
}

// 0x55d7b0  Confidence: exact (no symbol and no caller; name ours).
void ds_pause(int sig)
{
    int ch = ds_get_channel(sig);

    if (ch >= 0 && ds_channel_is_playing(ch)) {
        ds_channels[ch].pdsb->Stop();
        ds_channels[ch].flags |= DS_CHANNEL_PAUSED;
    }
}

// 0x55d800  Confidence: exact (the argument is tested as a byte, a bool in the original).
void ds_pause_all(int all)
{
    int i;

    for (i = 0; i < DS_MAX_CHANNELS; i++) {
        if (ds_channel_is_playing(i)) {
            if ((char)all || !(ds_channels[i].flags & DS_CHANNEL_STREAMED)) {
                ds_channels[i].pdsb->Stop();
                ds_channels[i].flags |= DS_CHANNEL_PAUSED;
            }
        }
    }
}

// 0x55d850  Confidence: exact.
void ds_unpause(int sig)
{
    int ch = ds_get_channel(sig);

    if (ch >= 0 && (ds_channels[ch].flags & DS_CHANNEL_PAUSED))
        ds_channel_unpause(ch);
}

// 0x55d880  Confidence: exact.
void ds_channel_unpause(int channel)
{
    DWORD play_flags = 0;

    if (ds_channels[channel].flags & (DS_CHANNEL_LOOPING | DS_CHANNEL_STREAMED))
        play_flags = DSBPLAY_LOOPING;
    ds_channels[channel].pdsb->Play(0, 0, play_flags);
    ds_channels[channel].flags &= ~DS_CHANNEL_PAUSED;
}

// 0x55d8c0  Confidence: exact.
void ds_unpause_all()
{
    int i;

    for (i = 0; i < DS_MAX_CHANNELS; i++) {
        if (ds_channels[i].flags & DS_CHANNEL_PAUSED)
            ds_channel_unpause(i);
    }
}

// 0x55d8f0  Confidence: exact.
void ds_stop_all()
{
    int i;

    for (i = 0; i < DS_MAX_CHANNELS; i++)
        ds_channel_release(i);
}

// 0x55d910  Confidence: exact.
void ds_set_volume(int sig, float volume)
{
    int ch = ds_get_channel(sig);

    if (ch >= 0 && ds_channels[ch].pdsb != NULL) {
        ds_channels[ch].pdsb->SetVolume(ds_volume_to_attenuation(volume));
        ds_channels[ch].volume = volume;
    }
}

// 0x55d960  Confidence: exact.
void ds_set_pan(int sig, float pan)
{
    int ch = ds_get_channel(sig);

    if (ch >= 0 && ds_channels[ch].pdsb != NULL)
        ds_channels[ch].pdsb->SetPan((int)(pan * 1000.0f));
}

// 0x55d9a0  Confidence: exact. The channel index is not checked: for a sound that is no longer playing
// it is -1 and the buffer pointer is read from before ds_channels (inside ds_volume_table; NOTES.md).
void ds_set_frequency(int sig, unsigned int freq)
{
    char msg[256];
    DWORD status;
    int ch = ds_get_channel(sig);
    HRESULT hr;

    hr = ds_channels[ch].pdsb->GetStatus(&status);
    if (hr < 0) {
        sprintf(msg, "GetStatus failed with return value %s\n", ds_error_string(hr));
        return;
    }
    if (status & DSBSTATUS_PLAYING)
        ds_channels[ch].pdsb->SetFrequency(freq);
}

// 0x55da20  Confidence: exact.
bool ds_is_playing(int sig)
{
    int ch = ds_get_channel(sig);

    if (ch < 0)
        return false;
    if (ds_channel_is_playing(ch))
        return true;
    return false;
}

// 0x55da50  Confidence: exact.
void ds_get_format(int buffer, int *rate, int *bits, int *channels)
{
    if (rate != NULL)
        *rate = ds_buffers[buffer].pcm_format->nSamplesPerSec;
    if (bits != NULL)
        *bits = ds_buffers[buffer].pcm_format->wBitsPerSample;
    if (channels != NULL)
        *channels = ds_buffers[buffer].pcm_format->nChannels;
}

// 0x55dac0  Confidence: exact.
int ds_get_frequency(int sig, unsigned int *freq)
{
    int ch = ds_get_channel(sig);

    if (ch < 0)
        return -1;
    return ds_channels[ch].pdsb->GetFrequency((DWORD *)freq) >= 0 ? 0 : -1;
}

// 0x55db00  Confidence: exact.
int ds_get_channel(int sig)
{
    int i;

    if (sig < 0)
        return -1;
    for (i = 0; i < DS_MAX_CHANNELS; i++) {
        if (ds_channels[i].signature == sig)
            break;
    }
    if (i == DS_MAX_CHANNELS)
        return -1;
    return i;
}

// 0x55db30  Confidence: exact (no symbol; name from ds.h, decomp60).
int ds_get_num_playing_channels()
{
    int n = 0;
    int i;

    for (i = 0; i < DS_MAX_CHANNELS; i++) {
        if (ds_channel_is_playing(i) > 0)
            n++;
    }
    return n;
}

// 0x55db50  Confidence: exact.
IDirectSoundBuffer *ds_get_primary_buffer()
{
    return ds_primary_buffer;
}

// 0x55db60  Confidence: exact.
IDirectSound *ds_get_direct_sound()
{
    return ds_direct_sound;
}

// 0x55db70  Confidence: exact.
IKsPropertySet *ds_get_eax_property_set()
{
    return ds_eax_property_set;
}

// 0x55db80  Confidence: exact (VC6's compare tree). DSERR_INVALIDPARAM has no break: it gets
// "DSERR_NOAGGREGATION", and its own literal (0x5ae00c) is left unreferenced.
const char *ds_error_string(HRESULT hr)
{
    const char *s;

    switch (hr) {
    case DS_OK:
        s = "DS_OK";
        break;
    case (HRESULT)0x8878000a:
        s = "DSERR_ALLOCATED";
        break;
    case (HRESULT)0x88780082:
        s = "DSERR_ALREADYINITIALIZED";
        break;
    case (HRESULT)0x88780064:
        s = "DSERR_BADFORMAT";
        break;
    case (HRESULT)0x88780096:
        s = "DSERR_BUFFERLOST";
        break;
    case (HRESULT)0x8878001e:
        s = "DSERR_CONTROLUNAVAIL";
        break;
    case (HRESULT)0x80004005:
        s = "DSERR_GENERIC";
        break;
    case (HRESULT)0x88780032:
        s = "DSERR_INVALIDCALL";
        break;
    case (HRESULT)0x80070057:
        s = "DSERR_INVALIDPARAM";
    case (HRESULT)0x80040110:
        s = "DSERR_NOAGGREGATION";
        break;
    case (HRESULT)0x88780078:
        s = "DSERR_NODRIVER";
        break;
    case (HRESULT)0x8007000e:
        s = "DSERR_OUTOFMEMORY";
        break;
    case (HRESULT)0x887800a0:
        s = "DSERR_OTHERAPPHASPRIO";
        break;
    case (HRESULT)0x88780046:
        s = "DSERR_PRIOLEVELNEEDED";
        break;
    case (HRESULT)0x887800aa:
        s = "DSERR_UNINITIALIZED";
        break;
    case (HRESULT)0x80004001:
        s = "DSERR_UNSUPPORTED";
        break;
    default:
        s = "unknown";
        break;
    }
    return s;
}

// 0x55dc70  Confidence: exact.
bool ds_is_ds3d_enabled()
{
    return ds3d_enabled;
}

// 0x55dc80  Confidence: exact.
bool ds_is_eax_enabled()
{
    return ds_eax_enabled;
}

// 0x55dc90  Confidence: exact.
void ds_disable_3d_eax()
{
    ds3d_enabled = false;
    ds_eax_enabled = false;
}

// 0x55dca0  Confidence: exact.
void ds_disable_eax()
{
    ds_eax_enabled = false;
}

// 0x55dcb0  Confidence: exact. Every channel of the group gets the group volume itself (the sounds'
// own volumes are lost).
void ds_set_group_volume(int group, float volume)
{
    int i;

    clamp_float(&volume, 0.0f, 1.0f);
    for (i = 0; i < DS_MAX_CHANNELS; i++) {
        if (ds_channels[i].pdsb != NULL && (ds_channels[i].group == group || group == -1)) {
            ds_channels[i].pdsb->SetVolume(ds_volume_to_attenuation(volume));
            ds_channels[i].volume = volume;
        }
    }
}

// 0x55dd10  Confidence: exact. The RIFF chunk size over bytes per sample and the rate: the channel
// count is left out (stereo files read twice as long).
float ds_estimate_duration(int buffer)
{
    ds_buffer *b;

    if (buffer < 0 || buffer >= DS_MAX_BUFFERS)
        return 0.0f;
    b = &ds_buffers[buffer];
    return b->riff_ck.cksize / (b->pcm_format->wBitsPerSample * 0.125f) / b->pcm_format->nSamplesPerSec;
}

// 0x55dd80  Confidence: exact.
void ds_set_eax_environment(const char *name)
{
    if (ds_eax_enabled)
        eax_set_environment_by_name(name);
}

// 0x55dda0 ds_channel__ctor: ds_channel's implicit constructor (the RealtimeTimer member, 0x5019e0), a
// COMDAT closing the object. Confidence: exact.
