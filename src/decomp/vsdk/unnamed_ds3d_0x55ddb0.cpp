// unnamed_ds3d_0x55ddb0.cpp - Sum.exe v1.40, code 0x55ddb0-0x55e03f: the DirectSound3D listener and buffer
// updates (ds3d_*). The file name is a placeholder: no path string, no strings.tbl block (FreeSpace 2
// keeps the same functions in a ds3d.cpp of its own; names only).
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp66 NOTES.md): after the ds object's closing COMDAT (ds_channel__ctor 0x55dda0); the UI
// bevel code follows at 0x55e040. .data 0x5ae0b8-0x5ae103 (the EAX 1.0 reverb property-set GUID, an
// initialised global, then the one message), .bss 0x2f8f744-0x2f8f749 (after the ds object's
// ds_next_signature).

#include <windows.h>
#include <stdio.h>
#include "vsdk/snd/ds.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"

// ---- .data ------------------------------------------------------------------------------------------
// {62a69bae-df9d-11d1-99a6-00c04fc99d46}, EAX 1.0's DSPROPSETID_EAX_ReverbProperties (in .data, so not
// const).
GUID DSPROPSETID_EAX_ReverbProperties = { 0x62a69bae, 0xdf9d, 0x11d1,
                                          { 0x99, 0xa6, 0x00, 0xc0, 0x4f, 0xc9, 0x9d, 0x46 } };   // 0x5ae0b8

// ---- .bss -------------------------------------------------------------------------------------------
IDirectSound3DListener *ds3d_listener;            // 0x2f8f744
bool ds3d_initialized;                            // 0x2f8f748

// 0x55ddb0  Confidence: exact.
int ds3d_update_buffer(int channel, float min_range, float max_range, const vector3 *pos, const vector3 *vel)
{
    IDirectSound3DBuffer *b;

    if (channel == -1)
        return 0;
    b = ds_channels[channel].pds3db;
    b->SetPosition(pos->x, pos->y, pos->z, DS3D_DEFERRED);
    b->SetVelocity(vel->x, vel->y, vel->z, DS3D_DEFERRED);
    b->SetMinDistance(min_range, DS3D_DEFERRED);
    b->SetMaxDistance(max_range, DS3D_DEFERRED);
    ds3d_listener->CommitDeferredSettings();
    return 0;
}

// 0x55de30  Confidence: exact. Roll-off factor 2, Doppler factor 1.
int ds3d_update_listener(const vector3 *pos, const vector3 *vel, const matrix *orient)
{
    if (!ds3d_initialized)
        return -1;
    if (ds3d_listener == NULL)
        return -1;
    ds3d_listener->SetPosition(pos->x, pos->y, pos->z, DS3D_DEFERRED);
    ds3d_listener->SetVelocity(vel->x, vel->y, vel->z, DS3D_DEFERRED);
    ds3d_listener->SetOrientation(orient->fvec.x, orient->fvec.y, orient->fvec.z, orient->uvec.x,
                                  orient->uvec.y, orient->uvec.z, DS3D_DEFERRED);
    ds3d_listener->SetRolloffFactor(2.0f, DS3D_DEFERRED);
    ds3d_listener->SetDopplerFactor(1.0f, DS3D_DEFERRED);
    return ds3d_listener->CommitDeferredSettings() >= 0 ? 0 : -1;
}

// 0x55def0  Confidence: exact.
void ds3d_close()
{
    if (ds3d_initialized) {
        ds3d_release_listener();
        ds3d_initialized = false;
    }
}

// 0x55df10  Confidence: exact.
void ds3d_release_listener()
{
    if (ds3d_listener != NULL) {
        ds3d_listener->Release();
        ds3d_listener = NULL;
    }
}

// 0x55df30  Confidence: exact (the support test). With EAX the card must support the EAX 1.0 reverb
// set: the test is `(support & KSPROPERTY_SUPPORT_SET | KSPROPERTY_SUPPORT_GET) != 3` (the OR binds
// last), so only the SET bit is checked; then property 0 is set to 1.
int ds3d_init(bool eax)
{
    IKsPropertySet *ps;
    ULONG support;
    ULONG value;

    if (ds3d_initialized == true)
        return 0;
    if (eax == true) {
        ps = ds_get_eax_property_set();
        if (ps == NULL)
            return -1;
        support = 0;
        if (ps->QuerySupport(DSPROPSETID_EAX_ReverbProperties, 0, &support) < 0)
            return -1;
        if ((support & KSPROPERTY_SUPPORT_SET | KSPROPERTY_SUPPORT_GET) != 3)
            return -1;
        value = 1;
        if (ps->Set(DSPROPSETID_EAX_ReverbProperties, 0, NULL, 0, &value, sizeof(value)) < 0)
            return -1;
    }
    if (ds3d_init_listener() != 0)
        return -1;
    ds3d_initialized = true;
    return 0;
}

// 0x55dfe0  Confidence: exact.
int ds3d_init_listener()
{
    char msg[256];
    IDirectSoundBuffer *pb;
    HRESULT hr;

    if (ds3d_listener == NULL) {
        pb = ds_get_primary_buffer();
        hr = pb->QueryInterface(IID_IDirectSound3DListener, (void **)&ds3d_listener);
        if (hr < 0) {
            sprintf(msg, "Fatal error calling pPrimaryBuffer->QueryInterface(): %s\n", ds_error_string(hr));
            ds3d_listener = NULL;
            return -1;
        }
    }
    return 0;
}
