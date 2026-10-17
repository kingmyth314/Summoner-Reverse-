// unnamed_sound_emitter_0x546150.cpp - Sum.exe v1.40, code 0x546150-0x546cf0: the sound emitters (level
// ambients and effect sounds placed in the world: 128 records on a linked list, position updates at an
// interval, voice stealing above 16, repeat delays; module "-" in sum.symbols). The file name is a
// placeholder: no path string, no strings.tbl block.
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range (decomp69 NOTES.md): the polygon object ends with gr_poly_alloc_clip_vertex (0x546130); this
// one opens with its two initialisers (table 0x588a04 / 0x588a08), ends with the implicit constructor of
// the record (0x546c80) and the vector3::operator!= COMDAT (0x546cc0); its data: the message
// literal 0x5aca48 and .bss 0x2f076f0-0x2f0b16c. The noise object follows (0x546d00, .data 0x5aca88).
//
// Like the other vsdk objects this one calls the header inlines out of line (vector3's operator= and
// operator!=, the timers' members), so it was built without inline expansion.
//
// The DirectSound3D path of sound_emitter_play / sound_emitter_update_position is dead:
// sound_emitter_stub_false always returns false, and both test it with `cmp al, 1`.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/math/vector.h"
#include "vsdk/snd/snd.h"
#include "vsdk/snd/ds.h"

// ---- .bss, in address order ------------------------------------------------------------------------------
// 0x546150 init_sound_emitters (jmp 0x546160: `vector constructor iterator` over the 128 records with
// the implicit constructor 0x546c80): compiler-generated. Confidence: likely.
// 0x546180 init_sound_emitter_list_head (jmp 0x546190: the constructor on the head): compiler-generated.
// Confidence: likely. The table order (0x588a04 the array, 0x588a08 the head) is the definition order.
sound_emitter sound_emitters[SOUND_MAX_EMITTERS];       // 0x2f07768
sound_emitter sound_emitter_list_head;                  // 0x2f076f0
int sound_emitters_num_playing;                         // 0x2f0b168
bool sound_emitters_enabled;                            // 0x2f0b16c

// 0x5461a0  Confidence: exact. No caller other than sound_emitters_close.
void sound_emitters_delete_all()
{
    sound_emitter *e, *next;

    for (e = sound_emitter_list_head.next; e != &sound_emitter_list_head; e = next) {
        next = e->next;
        sound_emitter_delete(e - sound_emitters, 0);
    }
}

// 0x5461f0  Confidence: exact.
void sound_emitter_reset(int emitter)
{
    sound_emitter *e = &sound_emitters[emitter];

    e->flags = 0;
    e->sound = -1;
    e->sig = -1;
    e->vel.zero();
    e->pos.zero();
}

// 0x546230  Confidence: exact.
int sound_emitter_find_free()
{
    int i;

    for (i = 0; i < SOUND_MAX_EMITTERS; i++) {
        if (!(sound_emitters[i].flags & SOUND_EMITTER_USED))
            return i;
    }
    return -1;
}

// 0x546250  Confidence: exact. The playing emitter with the lowest volume (below 1000).
int sound_emitter_find_quietest()
{
    sound_emitter *e;
    float min_volume = 1000.0f;
    int best = -1;

    for (e = sound_emitter_list_head.next; e != &sound_emitter_list_head; e = e->next) {
        if (e->sig >= 0 && e->volume < min_volume) {
            min_volume = e->volume;
            best = e - sound_emitters;
        }
    }
    return best;
}

// 0x5462b0  Confidence: exact.
void sound_emitter_stop(int emitter, int fade_ms)
{
    sound_emitter *e = &sound_emitters[emitter];

    e->flags &= ~SOUND_EMITTER_FLAG8;
    if (e->sig >= 0) {
        if (fade_ms > 0)
            snd_stop_fade(e->sig, fade_ms);
        else
            snd_stop(e->sig);
        e->sig = -1;
        sound_emitters_num_playing--;
    }
}

// 0x546320  Confidence: exact (the volume through a float local; snd_play_looping takes five arguments, D8400; the record copy of snd_get_sound_info is only read
// for its volume). Steals the quietest voice when 16 play and it is not louder than this one.
void sound_emitter_play(int emitter)
{
    sound_emitter *e = &sound_emitters[emitter];
    snd_sound info;
    int quietest;

    if (sound_emitters_num_playing >= SOUND_MAX_PLAYING) {
        quietest = sound_emitter_find_quietest();
        if (sound_emitters[quietest].volume > e->volume)
            return;
        sound_emitter_stop(quietest, 0);
    }
    snd_get_sound_info(e->sound, &info);
    if (e->repeat_delay > 0) {
        if (sound_emitter_stub_false() == true)
            e->sig = snd_play_3d(e->sound, &e->pos, false, 0, 0);
        else {
            float vol = e->volume / info.volume;
            e->sig = snd_play(e->sound, vol, 0.0f, 0, 0);
        }
    } else {
        if (sound_emitter_stub_false() == true)
            e->sig = snd_play_3d(e->sound, &e->pos, true, 0, 0);
        else {
            float vol = e->volume / info.volume;
            e->sig = snd_play_looping(e->sound, vol, 0.0f, 0, 0);
        }
    }
    if (e->sig >= 0) {
        e->flags &= ~SOUND_EMITTER_FLAG8;
        sound_emitters_num_playing++;
    }
}

// 0x546430  Confidence: exact.
bool sound_emitter_stub_false()
{
    return false;
}

// 0x546440  Confidence: exact. Stops the emitter at volume 0.1 or below, else sets the volume, or starts
// the sound when it does not play.
void sound_emitter_update_volume(int emitter, float scale)
{
    sound_emitter *e = &sound_emitters[emitter];
    float volume;

    if (e->flags & SOUND_EMITTER_SEGMENT)
        volume = sound_emitter_calc_volume(e->sound, &e->seg_a, &e->seg_b, scale);
    else
        volume = snd_calc_volume_3d(e->sound, &e->pos, scale);
    if (volume <= 0.1f) {
        sound_emitter_stop(emitter, 0);
        return;
    }
    if (e->sig >= 0 && snd_is_playing(e->sig)) {
        snd_set_volume(e->sig, volume);
        e->volume = volume;
        return;
    }
    e->volume = volume;
    if (!(e->flags & SOUND_EMITTER_FLAG8))
        sound_emitter_play(emitter);
}

// 0x546500  Confidence: exact (VC6 byte match, match14; was exact).
void sound_emitter_update_pan(int emitter)
{
    sound_emitter *e = &sound_emitters[emitter];
    float pan;

    if (e->flags & SOUND_EMITTER_SEGMENT)
        pan = sound_emitter_calc_pan(&e->seg_a, &e->seg_b);
    else
        pan = snd_calc_pan(&e->pos);
    if ((float)fabs(e->pan - pan) > 0.1f) {
        snd_set_pan(e->sig, pan);
        e->pan = pan;
    }
}

// 0x546580  Confidence: exact.
void sound_emitter_update_frequency(int emitter)
{
    sound_emitter *e = &sound_emitters[emitter];
    int freq;

    if (e->flags & SOUND_EMITTER_SEGMENT)
        freq = sound_emitter_calc_doppler_segment(e->frequency, &e->seg_a, &e->seg_b, &e->vel);
    else
        freq = sound_emitter_calc_doppler(e->frequency, &e->pos, &e->vel);
    if (abs(freq - e->frequency_set) > 100) {
        e->frequency_set = freq;
        snd_set_frequency(e->sig, freq);
    }
}

// 0x546600  Confidence: exact.
int sound_emitters_init()
{
    int i;

    if (sound_emitters_enabled == true)
        return 0;
    sound_emitter_list_head.prev = sound_emitter_list_head.next = &sound_emitter_list_head;
    for (i = 0; i < SOUND_MAX_EMITTERS; i++)
        sound_emitter_reset(i);
    sound_emitters_num_playing = 0;
    sound_emitters_enabled = true;
    return 0;
}

// 0x546650  Confidence: exact. No caller.
void sound_emitters_close()
{
    if (sound_emitters_enabled) {
        sound_emitters_delete_all();
        sound_emitters_enabled = false;
    }
}

// 0x546670  Confidence: exact (VC6 byte match, match14; was exact). No caller in the game code read so far (sum.symbols lists none).
int sound_emitter_create_segment(int sound, const vector3 *a, const vector3 *b, int update_ms, int delay_ms)
{
    sound_emitter *e;
    int emitter;

    emitter = sound_create_emitter(sound, update_ms, delay_ms);
    if (emitter == -1)
        return -1;
    e = &sound_emitters[emitter];
    e->seg_a = *a;
    e->seg_b = *b;
    e->flags |= SOUND_EMITTER_SEGMENT;
    return emitter;
}

// 0x5466f0  Confidence: exact. The message is formatted but never printed. The new emitter goes to the
// end of the list.
int sound_create_emitter(int sound, int update_ms, int delay_ms)
{
    sound_emitter *e;
    sound_emitter *prev;
    snd_sound *s;
    int emitter;

    if (!sound_emitters_enabled)
        return -1;
    emitter = sound_emitter_find_free();
    if (emitter == -1)
        return -1;
    s = snd_get_sound(sound);
    if (!snd_file_exists(s->name)) {
        sprintf(string_buffer, "Failed to create sound emitter since sound file missing: %s\n", s->name);
        return -1;
    }
    sound_emitter_reset(emitter);
    e = &sound_emitters[emitter];
    e->flags |= SOUND_EMITTER_USED;
    e->sound = sound;
    e->update_ms = update_ms;
    e->update_timer.SetTimeout(0);
    e->repeat_delay = delay_ms;
    e->repeat_timer.invalidate();
    snd_get_format(sound, &e->frequency, NULL, NULL);
    prev = sound_emitter_list_head.prev;
    e->prev = prev;
    e->next = &sound_emitter_list_head;
    prev->next = e;
    sound_emitter_list_head.prev = e;
    return emitter;
}

// 0x5467c0  Confidence: likely (the order of the link stores). No range check of the index.
int sound_emitter_delete(int emitter, int fade)
{
    sound_emitter *prev, *next;

    if (!sound_emitters_enabled)
        return -1;
    if (sound_emitters[emitter].flags & SOUND_EMITTER_USED) {
        sound_emitter_stop(emitter, fade);
        prev = sound_emitters[emitter].prev;
        next = sound_emitters[emitter].next;
        sound_emitters[emitter].next = NULL;
        sound_emitters[emitter].prev = NULL;
        prev->next = next;
        next->prev = prev;
        sound_emitters[emitter].flags &= ~SOUND_EMITTER_FLAG8;
        sound_emitter_reset(emitter);
    }
    return 0;
}

// 0x546840  Confidence: exact (VC6 byte match, match14; was exact). No caller.
void sound_emitter_set_pos_vel(int emitter, const vector3 *a, const vector3 *b, const vector3 *vel, float scale)
{
    sound_emitter *e;

    if (!sound_emitters_enabled || emitter < 0 || emitter >= SOUND_MAX_EMITTERS)
        return;
    e = &sound_emitters[emitter];
    if (a)
        e->seg_a = *a;
    if (b)
        e->seg_b = *b;
    sound_emitter_update_position(emitter, &e->seg_a, vel, scale);
}

// 0x5468c0  Confidence: exact (VC6 byte match, match14; was likely) (the repeat-delay condition is one expression here; the 3D branch is dead).
// Once per update interval: stores the position and velocity; a sound that finished waits for the repeat
// delay, otherwise the volume, pan and Doppler frequency follow the listener.
void sound_emitter_update_position(int emitter, const vector3 *pos, const vector3 *vel, float scale)
{
    sound_emitter *e;
    snd_sound info;
    float volume;
    int channel;

    if (!sound_emitters_enabled || emitter < 0 || emitter >= SOUND_MAX_EMITTERS)
        return;
    e = &sound_emitters[emitter];
    if (!e->update_timer.elapsed())
        return;
    e->update_timer.SetTimeout(e->update_ms);
    e->pos = *pos;
    if (vel)
        e->vel = *vel;
    else
        e->vel.zero();
    if (e->flags & SOUND_EMITTER_PAUSED)
        return;
    if (e->sig >= 0 && e->repeat_delay && !snd_is_playing(e->sig)
        && !(e->repeat_timer.valid() && e->repeat_timer.elapsed())) {
        if (!e->repeat_timer.valid())
            e->repeat_timer.SetTimeout(e->repeat_delay);
    } else {
        e->repeat_timer.invalidate();
        if (sound_emitter_stub_false() == true) {
            if (e->flags & SOUND_EMITTER_SEGMENT)
                volume = sound_emitter_calc_volume(e->sound, &e->seg_a, &e->seg_b, scale);
            else
                volume = snd_calc_volume_3d(e->sound, &e->pos, scale);
            e->volume = volume;
            if (volume <= 0.1f) {
                sound_emitter_stop(emitter, 0);
            } else {
                if (ds_get_channel(e->sig) < 0) {
                    if (!(e->flags & SOUND_EMITTER_FLAG8))
                        sound_emitter_play(emitter);
                    e->volume_scale = -1.0f;
                }
                channel = ds_get_channel(e->sig);
                if (channel >= 0 && snd_get_sound_info(e->sound, &info) == 0) {
                    if (e->volume_scale != scale) {
                        snd_set_volume(e->sig, info.volume * volume);
                        e->volume_scale = scale;
                    }
                    if (e->pos != e->last_pos)
                        ds3d_update_buffer(channel, info.min_range, info.max_range, &e->pos, &e->vel);
                }
            }
        } else {
            sound_emitter_update_volume(emitter, scale);
            if (e->sig < 0)
                return;
            sound_emitter_update_pan(emitter);
            if (vel && !vel->is_zero())
                sound_emitter_update_frequency(emitter);
        }
    }
    e->last_pos = e->pos;
}

// 0x546b20  Confidence: exact.
void sound_emitter_pause(int emitter)
{
    if (!sound_emitters_enabled || emitter < 0 || emitter >= SOUND_MAX_EMITTERS)
        return;
    if (sound_emitters[emitter].flags & SOUND_EMITTER_USED) {
        sound_emitters[emitter].flags |= SOUND_EMITTER_PAUSED;
        sound_emitters[emitter].volume = 0.0f;
        sound_emitter_stop(emitter, 0);
    }
}

// 0x546b80  Confidence: exact.
void sound_emitter_unpause(int emitter)
{
    if (!sound_emitters_enabled || emitter < 0 || emitter >= SOUND_MAX_EMITTERS)
        return;
    if (sound_emitters[emitter].flags & SOUND_EMITTER_USED)
        sound_emitters[emitter].flags &= ~SOUND_EMITTER_PAUSED;
}

// 0x546bc0  Confidence: exact. No caller.
void sound_emitter_set_frequency(int emitter, int freq)
{
    if (emitter >= 0 && emitter < SOUND_MAX_EMITTERS)
        sound_emitters[emitter].frequency = freq;
}

// 0x546bf0  Confidence: exact. No caller.
int sound_emitter_get_frequency(int emitter)
{
    if (emitter >= 0 && emitter < SOUND_MAX_EMITTERS)
        return sound_emitters[emitter].frequency;
    return 11025;
}

// 0x546c20  Confidence: exact.
int sound_emitter_get_sound(int emitter)
{
    if (emitter >= 0 && emitter < SOUND_MAX_EMITTERS)
        return sound_emitters[emitter].sound;
    return -1;
}

// 0x546c50  Confidence: exact.
int sound_emitter_get_repeat_delay(int emitter)
{
    if (emitter >= 0 && emitter < SOUND_MAX_EMITTERS)
        return sound_emitters[emitter].repeat_delay;
    return 0;
}

// Compiler-generated and header inlines at the end of the object (not source here):
// 0x546c80 sound_emitter__sound_emitter: the implicit constructor of the record (vsdk/snd/snd.h).
// Confidence: likely.
// 0x546cc0 vector3__ne: vector3::operator!= (vsdk/math/vector.h). Confidence: exact.
