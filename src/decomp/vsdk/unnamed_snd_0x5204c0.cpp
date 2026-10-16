// unnamed_snd_0x5204c0.cpp - Sum.exe v1.40, code 0x5204c0-0x521e38: the sound library's snd_* layer
// (sounds.tbl, groups, listener, 2D fallback of 3D sounds; module '-' in sum.symbols). The file name is a
// placeholder: no path string, no strings.tbl block.
// decomp66: this copy replaces the repo file; only snd_play_streamed / snd_play_streamed_looping changed
// (int arg -> float pan, D4603).
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (D4010). Four static initialisers open the object (static_initializers entries 592-595:
// cmdline_param_nosound, snd_listener_pos, snd_listener_vel, snd_listener_orient); the last regular
// function is snd_set_eax_environment (0x521e00), followed by the COMDATs Sound::is_used (0x521e10) and
// clamp_uint (0x521e20). The EAX functions after them are a separate object (unnamed_eax_0x521e40.cpp):
// besides the COMDAT boundary, this object pools its strings (both "#Sounds End" uses of
// sounds_parse_table read one literal, 0x5a8954) and the EAX object does not (28 separate "%d\n" /
// "%f\n" literals). .data 0x5a8910-0x5a895f, .bss 0x2cb5410-0x2cbf487 (snd_listener_vel sits right
// after attached_light_slots).
//
// Build flags: string pooling (/Gf), no inline expansion (vector3 / matrix operators, len, dot,
// Sound::is_used are called).

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "vsdk/snd/snd.h"
#include "vsdk/os/cmdline.h"
#include "vsdk/os/console.h"
#include "vsdk/vfile/file.h"
#include "vsdk/parse/parse.h"
#include "vsdk/math/ix.h"

// ---- .bss in initialiser order -----------------------------------------------------------------------
// The help text of the -nosound parameter is a pointer into this unit's .bss (0x2cbf484, after
// snd_disabled): an empty, zero-initialised array (label ours).
static char snd_nosound_help[4];                  // 0x2cbf484
// The initialiser of cmdline_param_nosound and its destructor registration (compiler-generated):
// 0x5204c0 init_cmdline_param_nosound_thunk (call 0x5204d0, jmp 0x5204f0). Confidence: exact.
// 0x5204d0 init_cmdline_param_nosound (CmdLineParam::CmdLineParam). Confidence: exact.
// 0x5204f0 cmdline_param_nosound_register_dtor (atexit 0x520500). Confidence: exact.
// 0x520500 cmdline_param_nosound_dtor (jmp CmdLineParam::~CmdLineParam). Confidence: exact.
CmdLineParam cmdline_param_nosound("-nosound", snd_nosound_help, false);   // 0x2cb5440
// 0x520510 init_snd_listener_pos (jmp 0x520520: vector3()). Confidence: exact.
vector3 snd_listener_pos;                         // 0x2cb5420
// 0x520530 init_snd_listener_vel. Confidence: exact.
vector3 snd_listener_vel;                         // 0x2cb5410
// 0x520550 init_snd_listener_orient (matrix()). Confidence: exact.
matrix snd_listener_orient;                       // 0x2cbf458
// The rest in address order.
float snd_group_volumes[SND_NUM_GROUPS];          // 0x2cb542c
snd_sound sounds[SND_MAX_SOUNDS];                 // 0x2cb5458
int snd_backend;                                  // 0x2cbf47c 1 = DirectSound; every test is a one-case switch (D6003)
bool snd_disabled;                                // 0x2cbf480
static bool snd_sounds_initialized;               // 0x2cbf481 (label ours)

// 0x520570  Confidence: exact (cdecl with the record pushed; sum.symbols names it snd_params_equal).
// Exact compares of the stored parameters.
static bool snd_params_equal(snd_sound *s, float volume, float rolloff, float min_range)
{
    if (s->volume != volume)
        return false;
    if (s->min_range != min_range)
        return false;
    if (s->rolloff != rolloff)
        return false;
    return true;
}

// 0x5205b0  Confidence: exact. No sound with -nosound, the No Sound setting, or Windows NT before 5.
// A failing ds_init returns -1 with snd_disabled still clear (docs/sum-exe/sound.md).
int snd_init(HWND hwnd, int rate, int bits, int channels, bool use_ds3d, bool use_eax, bool no_sound,
             bool dsinit)
{
    OSVERSIONINFO vi;

    if (!cmdline_param_nosound.found() && !no_sound) {
        vi.dwOSVersionInfoSize = sizeof(vi);
        GetVersionEx(&vi);
        if (vi.dwPlatformId != VER_PLATFORM_WIN32_NT || vi.dwMajorVersion >= 5) {
            snd_backend = 1;
            if (ds_init(hwnd, rate, bits, channels, use_ds3d, use_eax, dsinit) != 0)
                return -1;
            snd_init_group_volumes();
            sound_emitters_init();
            snd_init_sounds();
            atexit(snd_close);
            return 0;
        }
    }
    snd_disabled = true;
    return -1;
}

// 0x520680  Confidence: likely (the three bitfield clears are one AND 0x7ffffff).
void snd_init_sounds()
{
    int i;

    if (snd_sounds_initialized == true)
        return;
    for (i = 0; i < SND_MAX_SOUNDS; i++) {
        sounds[i].handle = -1;
        sounds[i].name[0] = '\0';
        sounds[i].loaded = 0;
        sounds[i].level_refs = 0;
        sounds[i].load_failed = 0;
    }
    snd_sounds_initialized = true;
}

// 0x5206c0  Confidence: exact.
void snd_init_group_volumes()
{
    int i;

    for (i = 0; i < 4; i++)
        snd_group_volumes[i] = 1.0f;
}

// 0x5206e0  Confidence: exact (pointer loop over the names).
void snd_close()
{
    int i;

    if (snd_disabled)
        return;
    switch (snd_backend) {
    case 1:
        ds_close();
        break;
    }
    for (i = 0; i < SND_MAX_SOUNDS; i++)
        sounds[i].name[0] = '\0';
}

// 0x520710  Confidence: exact (the matrix copy is the out-of-line COMDAT 0x4a4320).
void snd_change_listener(vector3 *pos, vector3 *vel, matrix *orient)
{
    if (snd_disabled)
        return;
    snd_listener_pos = *pos;
    snd_listener_vel = *vel;
    snd_listener_orient = *orient;
    switch (snd_backend) {
    case 1:
        ds3d_update_listener(pos, vel, orient);
        break;
    }
}

// 0x520770  Confidence: exact (tail jump).
void snd_do_frame()
{
    if (!snd_disabled) {
        switch (snd_backend) {
        case 1:
            ds_do_frame();
            break;
        }
    }
}

// 0x520790  Confidence: exact. Only the second argument is passed on.
void snd_pause_all(int a, int b)
{
    if (!snd_disabled) {
        switch (snd_backend) {
        case 1:
            ds_pause_all(b);
            break;
        }
    }
}

// 0x5207b0  Confidence: exact.
void snd_unpause_all()
{
    if (!snd_disabled) {
        switch (snd_backend) {
        case 1:
            ds_unpause_all();
            break;
        }
    }
}

// 0x5207d0  Confidence: exact. The first record (from slot 0 or 8) with the name and the same three
// parameters, else the first free one after the file is found. The name is copied unchecked into a
// 0x100-byte buffer; the 0x28 limit is tested only for a new record. The parameters are compared
// before min_range <= 0 becomes 1.0, so such a sound never matches its own record and takes a new
// slot on every call (NOTES.md).
int snd_get_handle(const char *name, float min_range, float volume, float rolloff, int group, bool from_start)
{
    char buf[0x100];
    int i, free_slot;
    snd_sound *s;

    strcpy(buf, name);
    free_slot = -1;
    for (i = from_start ? 0 : 8; i < SND_MAX_SOUNDS; i++) {
        s = &sounds[i];
        if (s->is_used()) {
            if (!_stricmp(s->name, buf) && snd_params_equal(s, volume, rolloff, min_range) == true)
                return i;
        } else if (free_slot < 0) {
            free_slot = i;
        }
    }
    if (free_slot == -1)
        return -1;
    if (!snd_file_exists(buf))
        return -1;
    s = &sounds[free_slot];
    s->level_refs = 0;
    if (strlen(buf) >= 0x28) {
        sprintf(string_buffer, "Sound filename %s too long\n", buf);
        return -1;
    }
    strcpy(s->name, buf);
    s->volume = volume;
    s->group = group;
    if (min_range <= 0.0f)
        min_range = 1.0f;
    s->min_range = min_range;
    s->rolloff = rolloff;
    s->max_range = snd_calc_rolloff(min_range, rolloff, volume);
    return free_slot;
}

// 0x5209c0  Confidence: likely (EH frame for the File). Loads a used record once (a failure is kept
// in load_failed): the file is found by its stripped name, then loaded from its packfile's path and
// offset. `loaded` is set even when ds_load_buffer fails.
int snd_load(int handle, int streamed)
{
    snd_sound *s;
    const char *p;
    int offset;

    if (snd_disabled || handle < 0 || handle >= SND_MAX_SOUNDS)
        return -1;
    s = &sounds[handle];
    if (!s->is_used() || s->load_failed)
        return -1;
    if (s->handle < 0) {
        File f;
        char filename[0x100];
        char path[0x200];

        offset = 0;
        strcpy(filename, s->name);
        p = strrchr(s->name, '\\');
        if (p)
            strcpy(filename, p + 1);
        else
            strcpy(filename, s->name);
        if (f.find(filename, 9999999) != true)
            return -1;
        strcpy(path, f.get_full_path());
        offset = 0;
        f.get_packfile_path(path, &offset, 0x80);
        switch (snd_backend) {
        case 1:
            s->handle = ds_load_buffer(s->name, path, offset, s->group, streamed);
            break;
        }
    }
    if (s->handle < 0)
        s->load_failed = 1;
    s->loaded = 1;
    return s->handle;
}

// 0x520bc0  Confidence: exact.
void snd_unload(int handle)
{
    snd_sound *s;

    if (snd_disabled || handle == -1)
        return;
    s = &sounds[handle];
    if (s->handle >= 0) {
        switch (snd_backend) {
        case 1:
            ds_unload_buffer(s->handle);
            break;
        }
        s->handle = -1;
        s->loaded = 0;
    }
}

// 0x520c10  Confidence: exact (the two bitfield clears are one AND 0x0fffffff).
void snd_free(int handle)
{
    if (handle < 0 || handle >= SND_MAX_SOUNDS)
        return;
    snd_unload(handle);
    sounds[handle].name[0] = '\0';
    sounds[handle].level_refs = 0;
    sounds[handle].load_failed = 0;
}

// 0x520c50  Confidence: exact.
void snd_unload_all()
{
    int i;

    for (i = 0; i < SND_MAX_SOUNDS; i++)
        snd_unload(i);
}

// 0x520c70  Confidence: exact (the in-place increment of the 3-bit field). Loads every used sound
// and counts the level in; unused records get level_refs 0.
void snd_level_page_in()
{
    snd_sound *s = sounds;
    int i;

    for (i = 0; i < SND_MAX_SOUNDS; i++, s++) {
        if (s->is_used()) {
            if (!s->loaded)
                snd_load(i, 0);
            s->level_refs++;
        } else {
            s->level_refs = 0;
        }
    }
}

// 0x520ce0  Confidence: likely. Frees the records no level uses any more, counts the others down.
void snd_level_release()
{
    int i;

    for (i = 0; i < SND_MAX_SOUNDS; i++) {
        if (sounds[i].level_refs == 0) {
            sounds[i].name[0] = '\0';
            sounds[i].loaded = 0;
            sounds[i].load_failed = 0;
            if (sounds[i].handle != -1)
                snd_free(i);
            sounds[i].handle = -1;
        } else {
            sounds[i].level_refs--;
        }
    }
}

// 0x520d50  Confidence: exact.
int snd_play(int handle, float volume, float pan, int a, int unused)
{
    float v;

    if (snd_disabled || handle == -1 || handle < 0 || handle >= SND_MAX_SOUNDS)
        return -1;
    if (snd_load(handle, 0) == -1)
        return -1;
    v = snd_get_volume(handle, volume);
    if (v < 0.0f)
        return -1;
    int rval = -1;

    switch (snd_backend) {
    case 1:
        rval = ds_play(sounds[handle].handle, v, pan, 0, a);
        break;
    }
    return rval;
}

// 0x520de0  Confidence: exact.
float snd_get_volume(int handle, float scale)
{
    snd_sound *s = &sounds[handle];

    return snd_group_volumes[s->group] * s->volume * scale;
}

// 0x520e10  Confidence: exact.
int snd_play_looping(int handle, float volume, float pan, int a)
{
    float v;

    if (snd_disabled || handle == -1 || handle < 0 || handle >= SND_MAX_SOUNDS)
        return -1;
    if (snd_load(handle, 0) == -1)
        return -1;
    v = snd_get_volume(handle, volume);
    if (v < 0.0f)
        return -1;
    int rval = -1;

    switch (snd_backend) {
    case 1:
        rval = ds_play(sounds[handle].handle, v, pan, 1, a);
        break;
    }
    return rval;
}

// 0x520ea0  Confidence: exact (the range is tested before snd_disabled here). decomp66 (D4603): the third
// argument is the pan (ds_play_streamed multiplies it by 1000 as a float); every caller passes 0.
int snd_play_streamed(int handle, float volume, float pan)
{
    float v;

    if (handle < 0 || handle >= SND_MAX_SOUNDS || snd_disabled)
        return -1;
    if (snd_load(handle, 1) == -1)
        return -1;
    v = snd_get_volume(handle, volume);
    if (v < 0.0f)
        return -1;
    int rval = -1;

    switch (snd_backend) {
    case 1:
        rval = ds_play_streamed(sounds[handle].handle, v, pan, 0, 1.0f);
        break;
    }
    return rval;
}

// 0x520f20  Confidence: exact.
int snd_play_streamed_looping(int handle, float volume, float pan)
{
    float v;

    if (handle < 0 || handle >= SND_MAX_SOUNDS || snd_disabled)
        return -1;
    if (snd_load(handle, 1) == -1)
        return -1;
    v = snd_get_volume(handle, volume);
    if (v < 0.0f)
        return -1;
    int rval = -1;

    switch (snd_backend) {
    case 1:
        rval = ds_play_streamed(sounds[handle].handle, v, pan, 1, 1.0f);
        break;
    }
    return rval;
}

// 0x520fa0  Confidence: likely (`zero` is a stack local in the binary, loaded when the divisor is 0).
// Full volume (no scale cap, no clamp) at the listener; else the volume (times scale when scale < 1)
// falls off beyond min_range as 1 / ((d / min - 1) * rolloff + 1), 0 beyond max_range, times the group
// volume, at most 1.
float snd_calc_volume_3d(int handle, const vector3 *pos, float scale)
{
    snd_sound *s = &sounds[handle];
    float zero = 0.0f;
    float dist, v, d;

    if (*pos == snd_listener_pos)
        return snd_group_volumes[s->group] * s->volume * scale;
    vector3 dir;
    dist = vector3_normalized_dir(&dir, pos, &snd_listener_pos);
    v = s->volume;
    if (scale < 1.0f)
        v *= scale;
    if (dist > s->max_range)
        return 0.0f;
    if (dist > s->min_range) {
        d = (dist / s->min_range - 1.0f) * s->rolloff + 1.0f;
        if (d != 0.0f)
            v /= d;
        else
            v = zero;
    }
    v *= snd_group_volumes[s->group];
    if (v >= 1.0f)
        v = 1.0f;
    return v;
}

// 0x5210b0  Confidence: likely. As snd_calc_volume_3d for the point of segment a-b nearest to the
// listener; the scale always applies.
float sound_emitter_calc_volume(int handle, const vector3 *a, const vector3 *b, float scale)
{
    snd_sound *s = &sounds[handle];
    float zero = 0.0f;
    float dist, v, d;
    vector3 p;
    vector3 dir;

    ix_point_linesegment(&snd_listener_pos, a, b, &p);
    if (p == snd_listener_pos)
        return snd_group_volumes[s->group] * s->volume * scale;
    dir = p - snd_listener_pos;
    dist = dir.len();
    dir.normalize();
    v = s->volume * scale;
    if (dist > s->max_range)
        return 0.0f;
    if (dist > s->min_range) {
        d = (dist / s->min_range - 1.0f) * s->rolloff + 1.0f;
        if (d != 0.0f)
            v /= d;
        else
            v = zero;
    }
    v *= snd_group_volumes[s->group];
    if (v >= 1.0f)
        v = 1.0f;
    return v;
}

// 0x5211f0  Confidence: likely. The listener's right vector against the direction to the sound,
// 0 within 0.5 units.
float snd_calc_pan(const vector3 *pos)
{
    float pan;

    if (*pos == snd_listener_pos)
        return 0.0f;
    vector3 dir;
    pan = 0.0f;
    if (vector3_normalized_dir(&dir, pos, &snd_listener_pos) > 0.5f)
        return snd_listener_orient.rvec.dot(dir);
    return pan;
}

// 0x521270  Confidence: likely. The direction to the nearest point of the segment is not normalised,
// so the pan exceeds -1..1 beyond one unit (NOTES.md).
float sound_emitter_calc_pan(const vector3 *a, const vector3 *b)
{
    float pan;
    vector3 p;

    ix_point_linesegment(&snd_listener_pos, a, b, &p);
    if (p == snd_listener_pos)
        return 0.0f;
    vector3 d = p - snd_listener_pos;
    pan = 0.0f;
    if (d.len_sq() > 0.25f)
        return snd_listener_orient.rvec.dot(d);
    return pan;
}

// 0x521310  Confidence: exact. freq * (600 + v_listener . to_listener) / (600 - v_source . to_source).
int sound_emitter_calc_doppler(int freq, const vector3 *pos, const vector3 *vel)
{
    vector3 to_source;
    vector3 to_listener;

    if (*pos == snd_listener_pos)
        return freq;
    vector3_normalized_dir(&to_source, pos, &snd_listener_pos);
    vector3_normalized_dir(&to_listener, &snd_listener_pos, pos);
    return (int)((to_source.dot(snd_listener_vel) + 600.0f) / (600.0f - to_listener.dot(*vel)) * freq);
}

// 0x5213b0  Confidence: exact. The same for the nearest point of segment a-b.
int sound_emitter_calc_doppler_segment(int freq, const vector3 *a, const vector3 *b, const vector3 *vel)
{
    vector3 to_source;
    vector3 to_listener;
    vector3 p;

    ix_point_linesegment(&snd_listener_pos, a, b, &p);
    if (p == snd_listener_pos)
        return freq;
    vector3_normalized_dir(&to_source, &p, &snd_listener_pos);
    vector3_normalized_dir(&to_listener, &snd_listener_pos, &p);
    return (int)((to_source.dot(snd_listener_vel) + 600.0f) / (600.0f - to_listener.dot(*vel)) * freq);
}

// 0x521480  Confidence: likely. DirectSound3D plays the sound with its ranges; otherwise the 2D
// volume (at scale 1.75, divided by the sound's own volume, which snd_play applies again) and pan.
int snd_play_3d(int handle, const vector3 *pos, bool looping, int a, int unused)
{
    snd_sound *s;
    float v, pan;

    if (snd_disabled || handle == -1)
        return -1;
    s = &sounds[handle];
    if (snd_load(handle, 0) == -1)
        return -1;
    v = snd_calc_volume_3d(handle, pos, 1.75f);
    if (v < 0.0f)
        return -1;
    if (snd_backend != 1)
        return -1;
    if (snd_is_ds3d_enabled() == true) {
        vector3 vel(0.0f, 0.0f, 0.0f);
        return ds_play_3d(s->handle, pos, &vel, s->min_range, s->max_range,
                          snd_group_volumes[s->group] * s->volume, looping, a);
    }
    pan = snd_calc_pan(pos);
    if (s->volume != 0.0f)
        v /= s->volume;
    if (looping == true)
        return snd_play_looping(handle, v, pan, 0);
    return snd_play(handle, v, pan, a, 0);
}

// 0x5215d0  Confidence: exact.
void snd_stop(int sig)
{
    if (!snd_disabled && sig != -1) {
        switch (snd_backend) {
        case 1:
            ds_stop(sig);
            break;
        }
    }
}

// 0x521600  Confidence: exact.
void snd_stop_all()
{
    if (!snd_disabled) {
        switch (snd_backend) {
        case 1:
            ds_stop_all();
            break;
        }
    }
}

// 0x521620  Confidence: exact.
void snd_stop_fade(int sig, int ms)
{
    if (!snd_disabled && sig != -1) {
        switch (snd_backend) {
        case 1:
            ds_stop_fade(sig, ms);
            break;
        }
    }
}

// 0x521650  Confidence: exact.
bool snd_is_playing(int sig)
{
    if (snd_disabled)
        return false;
    bool rval = false;

    switch (snd_backend) {
    case 1:
        rval = ds_is_playing(sig);
        break;
    }
    return rval;
}

// 0x521680  Confidence: exact (the clamp writes the argument).
void snd_set_volume(int sig, float volume)
{
    if (snd_disabled)
        return;
    if (volume <= 0.0f)
        volume = 0.0f;
    else if (volume >= 1.0f)
        volume = 1.0f;
    switch (snd_backend) {
    case 1:
        ds_set_volume(sig, volume);
        break;
    }
}

// 0x5216e0  Confidence: exact. The playing channel sig of sound handle at its group volume.
void snd_set_volume_in_group(int handle, int sig, float volume)
{
    if (snd_disabled || sig < 0 || handle < 0)
        return;
    if (volume <= 0.0f)
        volume = 0.0f;
    else if (volume >= 1.0f)
        volume = 1.0f;
    volume = snd_get_volume(handle, volume);
    switch (snd_backend) {
    case 1:
        ds_set_volume(sig, volume);
        break;
    }
}

// 0x521760  Confidence: exact.
void snd_set_pan(int sig, float pan)
{
    if (snd_disabled)
        return;
    if (pan <= -1.0f)
        pan = -1.0f;
    else if (pan >= 1.0f)
        pan = 1.0f;
    switch (snd_backend) {
    case 1:
        ds_set_pan(sig, pan);
        break;
    }
}

// 0x5217c0  Confidence: exact.
void snd_set_frequency(int sig, int freq)
{
    if (snd_disabled)
        return;
    clamp_uint((unsigned int *)&freq, 100, 100000);
    switch (snd_backend) {
    case 1:
        ds_set_frequency(sig, freq);
        break;
    }
}

// 0x521800  Confidence: exact.
int snd_get_frequency(int sig, unsigned int *freq)
{
    if (snd_disabled)
        return -1;
    switch (snd_backend) {
    case 1:
        return ds_get_frequency(sig, freq);
    }
    return -1;
}

// 0x521830  Confidence: exact (no return value: EAX holds whatever the last call or test left).
void snd_get_format(int handle, int *a, int *b, int *c)
{
    if (snd_disabled || handle < 0 || handle >= SND_MAX_SOUNDS)
        return;
    if (sounds[handle].handle < 0 && snd_load(handle, 0) == -1)
        return;
    switch (snd_backend) {
    case 1:
        ds_get_format(sounds[handle].handle, a, b, c);
        break;
    }
}

// 0x5218a0  Confidence: exact.
void snd_set_group(int handle, int group)
{
    if (handle >= 0 && handle < SND_MAX_SOUNDS)
        sounds[handle].group = group;
}

// 0x5218e0  Confidence: exact. The table is written also while sound is disabled.
void snd_set_group_volume(int group, float volume)
{
    if (volume <= 0.0f)
        volume = 0.0f;
    else if (volume >= 1.0f)
        volume = 1.0f;
    snd_group_volumes[group] = volume;
    if (!snd_disabled) {
        switch (snd_backend) {
        case 1:
            ds_set_group_volume(group, volume);
            break;
        }
    }
}

// 0x521950  Confidence: exact.
float snd_get_group_volume(int group)
{
    return snd_group_volumes[group];
}

// 0x521960  Confidence: exact (tail jump).
int snd_get_num_playing_channels()
{
    if (!snd_disabled) {
        switch (snd_backend) {
        case 1:
            return ds_get_num_playing_channels();
        }
    }
    return 0;
}

// 0x521980  Confidence: exact.
snd_sound *snd_get_sound(int handle)
{
    if (handle >= 0 && handle < SND_MAX_SOUNDS)
        return &sounds[handle];
    return NULL;
}

// 0x5219a0  Confidence: exact (EH frame for the File).
bool snd_file_exists(const char *name)
{
    File f;
    const char *p = strrchr(name, '\\');

    return f.find(p ? p + 1 : name, 9999999);
}

// 0x521a30  Confidence: exact (the record copy is REP MOVSD of 16 dwords).
int snd_get_sound_info(int handle, snd_sound *out)
{
    if (handle < 0 || handle >= SND_MAX_SOUNDS)
        return -1;
    *out = sounds[handle];
    return 0;
}

// 0x521a60  Confidence: exact (EH frame for the TextFile).
void sounds_parse_table()
{
    TextFile tf;
    int i = 0;

    snd_init_sounds();
    if (tf.Open("sounds.tbl", 9999999) == 0) {
        tf.Expect("#Sounds Start");
        while (!tf.Accept("#Sounds End"))
            sounds_parse_entry(&tf, i++);
        tf.Close();
    }
}

// 0x521b10  Confidence: exact. "name" min_range volume rolloff; the group, the min_range clamp and the
// handle are left alone (snd_init_sounds set the handle).
void sounds_parse_entry(TextFile *tf, int index)
{
    snd_sound *s = &sounds[index];
    float rolloff;

    tf->ParseString(s->name, 0x28, '"', '"');
    s->min_range = tf->ParseFloat();
    s->volume = tf->ParseFloat();
    s->rolloff = rolloff = tf->ParseFloat();
    s->max_range = snd_calc_rolloff(s->min_range, rolloff, s->volume);
}

// 0x521b70  Confidence: exact.
float snd_get_rolloff(int handle)
{
    if (handle >= 0 && handle < SND_MAX_SOUNDS)
        return sounds[handle].max_range;
    return 0.0f;
}

// 0x521b90  Confidence: exact (tail jump).
bool snd_is_ds3d_enabled()
{
    return ds_is_ds3d_enabled();
}

// 0x521ba0  Confidence: exact (tail jump).
bool snd_is_eax_enabled()
{
    return ds_is_eax_enabled();
}

// 0x521bb0  Confidence: exact.
void snd_restore_reverb()
{
    if (ds_is_eax_enabled())
        eax_restore_reverb();
}

// 0x521bc0  Confidence: exact.
void snd_mute_reverb()
{
    if (ds_is_eax_enabled())
        eax_mute_reverb();
}

// 0x521bd0  Confidence: exact (tail jump).
bool snd_is_reverb_muted()
{
    return eax_is_reverb_muted();
}

// 0x521be0  Confidence: exact.
float snd_get_duration(int handle)
{
    if (!snd_disabled && handle >= 0 && handle < SND_MAX_SOUNDS)
        return ds_estimate_duration(sounds[handle].handle);
    return 0.0f;
}

// 0x521c20  Confidence: exact (the first format value lives in the handle's argument slot).
int snd_calc_duration(int handle)
{
    float duration;
    int a, b, c;

    if (handle < 0 || handle >= SND_MAX_SOUNDS || sounds[handle].handle < 0)
        return 0;
    duration = snd_get_duration(handle);
    snd_get_format(handle, &a, &b, &c);
    return (int)(a * b * duration) / 8;
}

// 0x521ca0  Confidence: exact (EH frame for the File, which only the not-loaded branch uses).
int snd_estimate_file_duration(int handle)
{
    float duration;
    int a, b, c;

    if (handle < 0 || handle >= SND_MAX_SOUNDS)
        return 0;
    File f;
    if (sounds[handle].handle >= 0) {
        duration = snd_get_duration(handle);
        snd_get_format(handle, &a, &b, &c);
        return (int)(a * b * duration) / 8;
    }
    return f.size(sounds[handle].name, 9999999);
}

// 0x521dd0  Confidence: exact.
float snd_calc_rolloff(float min_range, float rolloff, float volume)
{
    return min_range * volume / (rolloff * 0.1f) + (1.0f - 1.0f / rolloff) * min_range;
}

// 0x521e00  Confidence: exact.
void snd_set_eax_environment(const char *name)
{
    ds_set_eax_environment(name);
}

// ---- COMDATs at the end of the object (bodies in vsdk/snd/snd.h) -----------------------------------
// 0x521e10 Sound__is_used = snd_sound::is_used. Confidence: exact.
// 0x521e20 clamp_uint. Confidence: exact.
