// unnamed_eax_0x521e40.cpp - Sum.exe v1.40, code 0x521e40-0x5225a1: the EAX 2.0 reverb control (module
// '-' in sum.symbols). The file name is a placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (D4010): after the sound object's closing COMDATs (0x521e10 / 0x521e20) and before the
// quaternion unit's static initialisers (0x5225b0). No initialiser and no COMDAT of its own. Built
// without string pooling: eax_save_props and eax_load_props use 28 separate "%d\n" / "%f\n" literals.
// .data 0x5a8960-0x5a8b4b (the 26 environment names, their literals, "wt", the formats, "rt"), .bss
// 0x2cbf488-0x2cbf497.

#include <stdio.h>
#include <string.h>
#include "vsdk/snd/snd.h"
#include "vsdk/math/vector.h"

// ---- .bss --------------------------------------------------------------------------------------------
int eax_reverb_muted;                             // 0x2cbf488
long eax_saved_reverb;                            // 0x2cbf48c
IKsPropertySet *eax_property_set;                 // 0x2cbf490
bool eax_enabled;                                 // 0x2cbf494

// ---- .data -------------------------------------------------------------------------------------------
// The EAX 2.0 environment names in EAX_ENVIRONMENT_* order.
const char *eax_environment_names[EAX_NUM_ENVIRONMENTS] = {   // 0x5a8960
    "generic", "paddedcell", "room", "bathroom", "livingroom", "stoneroom", "auditorium",
    "concerthall", "cave", "arena", "hangar", "carpetedhallway", "hallway", "stonecorridor",
    "alley", "forest", "city", "mountains", "quarry", "plain", "parkinglot", "sewerpipe",
    "underwater", "drugged", "dizzy", "psychotic",
};

// 0x521e40  Confidence: exact (the result as SETGE / DEC).
int eax_set_decay_time(float t)
{
    if (!eax_enabled)
        return -1;
    clamp_float(&t, 0.1f, 20.0f);
    if (FAILED(eax_property_set->Set(DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_DECAYTIME,
                                     NULL, 0, &t, sizeof(t))))
        return -1;
    return 0;
}

// 0x521e90  Confidence: exact (as eax_set_decay_time).
int eax_set_environment(int env)
{
    if (!eax_enabled)
        return -1;
    if (FAILED(eax_property_set->Set(DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ENVIRONMENT,
                                     NULL, 0, &env, sizeof(env))))
        return -1;
    return 0;
}

// 0x521ed0  Confidence: exact (pointer loop to the end of the table).
int eax_set_environment_by_name(const char *name)
{
    int i;

    for (i = 0; i < EAX_NUM_ENVIRONMENTS; i++) {
        if (!_stricmp(eax_environment_names[i], name))
            return eax_set_environment(i);
    }
    return -1;
}

// 0x521f10  Confidence: exact. No caller found.
int eax_stub_ret_m1()
{
    return -1;
}

// 0x521f20  Confidence: exact. Nothing is released.
void eax_close()
{
    if (eax_enabled)
        eax_enabled = false;
}

// 0x521f40  Confidence: likely. Takes DirectSound's listener property set; without get and set
// support for all listener parameters it is released and EAX is switched off. Starts in "cave".
int eax_init()
{
    ULONG support = 0;

    if (eax_enabled != true) {
        eax_property_set = ds_get_eax_property_set();
        if (eax_property_set == NULL)
            return -1;
        if (FAILED(eax_property_set->QuerySupport(DSPROPSETID_EAX20_ListenerProperties,
                                                  DSPROPERTY_EAXLISTENER_ALLPARAMETERS, &support))
            || (support & (KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET))
                   != (KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET)) {
            if (eax_property_set != NULL) {
                eax_property_set->Release();
                eax_property_set = NULL;
            }
            ds_disable_eax();
            eax_enabled = false;
            return -1;
        }
        eax_enabled = true;
        eax_set_environment(EAX_ENVIRONMENT_CAVE);
        eax_reverb_muted = 0;
    }
    return 0;
}

// 0x521fd0  Confidence: exact. The buffer's room level to the minimum. Neither the QueryInterface
// result is checked nor the property set released (a reference leak per played buffer, NOTES.md).
void eax_buffer_init(IUnknown *buffer)
{
    IKsPropertySet *ps;
    long room;

    if (eax_enabled) {
        room = EAX_ROOM_MIN;
        buffer->QueryInterface(IID_IKsPropertySet, (void **)&ps);
        ps->Set(DSPROPSETID_EAX20_BufferProperties, DSPROPERTY_EAXBUFFER_ROOM, NULL, 0, &room, sizeof(room));
    }
}

// 0x522020  Confidence: exact. Saves the room level and sets it to the minimum (muted even when the
// read fails).
void eax_mute_reverb()
{
    ULONG returned;
    long room;

    if (eax_enabled && !eax_reverb_muted) {
        if (SUCCEEDED(eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM,
                                            NULL, 0, &eax_saved_reverb, sizeof(eax_saved_reverb), &returned))) {
            room = EAX_ROOM_MIN;
            eax_property_set->Set(DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM,
                                  NULL, 0, &room, sizeof(room));
        }
        eax_reverb_muted = 1;
    }
}

// 0x522090  Confidence: exact.
void eax_restore_reverb()
{
    if (eax_enabled && eax_reverb_muted) {
        eax_property_set->Set(DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ROOM,
                              NULL, 0, &eax_saved_reverb, sizeof(eax_saved_reverb));
        eax_reverb_muted = 0;
    }
}

// 0x5220d0  Confidence: exact.
bool eax_is_reverb_muted()
{
    if (eax_reverb_muted) return true;
    return false;
}

// 0x5220e0  Confidence: exact (8-aligned frame for the double arguments). One value per line in
// EAXLISTENERPROPERTIES order; 1 = written, 0 = the file did not open.
int eax_save_props(const char *filename)
{
    EAXLISTENERPROPERTIES p;
    FILE *f = fopen(filename, "wt");

    if (f == NULL)
        return 0;
    eax_get_listener_props(&p);
    fprintf(f, "%d\n", p.lRoom);
    fprintf(f, "%d\n", p.lRoomHF);
    fprintf(f, "%f\n", p.flRoomRolloffFactor);
    fprintf(f, "%f\n", p.flDecayTime);
    fprintf(f, "%f\n", p.flDecayHFRatio);
    fprintf(f, "%d\n", p.lReflections);
    fprintf(f, "%f\n", p.flReflectionsDelay);
    fprintf(f, "%d\n", p.lReverb);
    fprintf(f, "%f\n", p.flReverbDelay);
    fprintf(f, "%d\n", p.dwEnvironment);
    fprintf(f, "%f\n", p.flEnvironmentSize);
    fprintf(f, "%f\n", p.flEnvironmentDiffusion);
    fprintf(f, "%f\n", p.flAirAbsorptionHF);
    fprintf(f, "%d\n", p.dwFlags);
    fclose(f);
    return 1;
}

// 0x522240  Confidence: exact (one Get per property; the byte count goes to a reused slot). The last
// Get asks for AIRABSORPTIONHF (14) again instead of FLAGS (15), so dwFlags is saved as the air
// absorption bits (game bug, NOTES.md). Nothing checks the property set or the results.
void eax_get_listener_props(EAXLISTENERPROPERTIES *props)
{
    ULONG returned;

    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 2, NULL, 0, &props->lRoom, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 3, NULL, 0, &props->lRoomHF, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 4, NULL, 0, &props->flRoomRolloffFactor, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 5, NULL, 0, &props->flDecayTime, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 6, NULL, 0, &props->flDecayHFRatio, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 7, NULL, 0, &props->lReflections, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 8, NULL, 0, &props->flReflectionsDelay, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 9, NULL, 0, &props->lReverb, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 10, NULL, 0, &props->flReverbDelay, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 11, NULL, 0, &props->dwEnvironment, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 12, NULL, 0, &props->flEnvironmentSize, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 13, NULL, 0, &props->flEnvironmentDiffusion, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 14, NULL, 0, &props->flAirAbsorptionHF, 4, &returned);
    eax_property_set->Get(DSPROPSETID_EAX20_ListenerProperties, 14, NULL, 0, &props->dwFlags, 4, &returned);
}

// 0x522420  Confidence: exact. 0 when EAX is off or the file does not open, else 1; the values are
// not checked.
int eax_load_props(const char *filename)
{
    EAXLISTENERPROPERTIES p;
    FILE *f;

    if (!eax_enabled)
        return 0;
    f = fopen(filename, "rt");
    if (f == NULL)
        return 0;
    fscanf(f, "%d\n", &p.lRoom);
    fscanf(f, "%d\n", &p.lRoomHF);
    fscanf(f, "%f\n", &p.flRoomRolloffFactor);
    fscanf(f, "%f\n", &p.flDecayTime);
    fscanf(f, "%f\n", &p.flDecayHFRatio);
    fscanf(f, "%d\n", &p.lReflections);
    fscanf(f, "%f\n", &p.flReflectionsDelay);
    fscanf(f, "%d\n", &p.lReverb);
    fscanf(f, "%f\n", &p.flReverbDelay);
    fscanf(f, "%d\n", &p.dwEnvironment);
    fscanf(f, "%f\n", &p.flEnvironmentSize);
    fscanf(f, "%f\n", &p.flEnvironmentDiffusion);
    fscanf(f, "%f\n", &p.flAirAbsorptionHF);
    fscanf(f, "%d\n", &p.dwFlags);
    eax_set_listener_props(&p);
    fclose(f);
    return 1;
}

// 0x522580  Confidence: exact.
void eax_set_listener_props(EAXLISTENERPROPERTIES *props)
{
    if (eax_property_set != NULL)
        eax_property_set->Set(DSPROPSETID_EAX20_ListenerProperties, DSPROPERTY_EAXLISTENER_ALLPARAMETERS,
                              NULL, 0, props, sizeof(*props));
}
