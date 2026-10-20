// vsdk/snd/snd.h - the sound library (snd_* 0x5204c0-0x521e38, eax_* 0x521e40-0x5225a1; directory and
// file name guessed). Not original source.
// decomp60: both units written as src/decomp/vsdk/unnamed_snd_0x5204c0.cpp and unnamed_eax_0x521e40.cpp
// (two objects: the sound object ends with its COMDATs Sound::is_used / clamp_uint, and the two are
// built differently, D4010). The DirectSound layer and the EAX declarations are in vsdk/snd/ds.h.
#ifndef VSDK_SND_SND_H
#define VSDK_SND_SND_H

#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"         // snd_listener_orient
#include "vsdk/snd/ds.h"
#include <windows.h>                  // HWND (snd_init)

class TextFile;

#define SND_MAX_SOUNDS 0x280          // sounds[], 640 records
#define SND_NUM_GROUPS 4              // snd_group_volumes

// The three floats of snd_get_handle are the three table parameters of a sound (sounds.tbl): min range
// (1.0 when <= 0), volume, rolloff. group: 0 effects, 1 music. from_start: search from slot 0, not 8.
int  snd_get_handle(const char *name, float min_range, float volume, float rolloff, int group, bool from_start); // 0x5207d0
int  snd_load(int handle, bool streamed);                                   // 0x5209c0 (D11903) the ds buffer, -1
// (handle, volume, pan, a, unused): a goes to ds_play's last argument (decomp60).
int  snd_play(int handle, float volume, float pan, int a, int unused);      // 0x520d50
int  snd_play_looping(int handle, float volume, float pan, int a, int unused); // 0x520e10 (D8400)
// decomp66 (D4603): the third argument is the pan ds_play_streamed reads as a float (was int arg).
int  snd_play_streamed(int handle, float volume, float pan);                // 0x520ea0
int  snd_play_streamed_looping(int handle, float volume, float pan);        // 0x520f20
// decomp60 (D4009): (handle, pos, looping, a, unused); the callers push (sound, pos, 0 / 1, 0, 0).
int  snd_play_3d(int handle, const vector3 *pos, bool looping, int a, int unused);   // 0x521480
void snd_stop(int sig);                                                    // 0x5215d0
void snd_stop_fade(int sig, int ms);                                       // 0x521620
bool snd_is_playing(int sig);                                              // 0x521650
void snd_set_frequency(int sig, int freq);                                 // 0x5217c0 (decomp37) clamped to 100..100000
int  snd_get_frequency(int sig, unsigned int *freq);                       // 0x521800 (decomp37; decomp60: returns ds_get_frequency's value, -1)
void snd_set_volume(int sig, float volume);                                // 0x521680
void snd_set_group_volume(int group, float volume);                        // 0x5218e0 (0 = effects)
bool snd_is_eax_enabled();                                                 // 0x521ba0
void eax_mute_reverb();                                                    // 0x522020
void eax_restore_reverb();                                                 // 0x522090
// (hwnd, 44100, 16, 2, directsound3d, eax, no_sound, dsinit) from LoadSettings; atexit(snd_close).
// decomp60: returns 0, or -1 (also when ds_init fails, which leaves snd_disabled clear; sound.md).
int  snd_init(HWND hwnd, int rate, int bits, int channels, bool use_ds3d, bool use_eax, bool no_sound,
              bool dsinit);                                                // 0x5205b0
void snd_close();                                                          // 0x5206e0
void sounds_parse_table();                                                 // 0x521a60 sounds.tbl
// Sound emitters (level ambients; decomp27). decomp69: the object 0x546150-0x546cf0, source
// src/decomp/vsdk/unnamed_sound_emitter_0x546150.cpp (module "-", file name a placeholder); the record
// and the rest of the unit are declared at the end of this header. sound_emitters_init returns 0 (both
// paths `xor eax, eax`; D5031).
int  sound_emitters_init();                                                // 0x546600 (snd_init; decomp60)
void sound_emitter_pause(int emitter);                                     // 0x546b20
void sound_emitter_unpause(int emitter);                                   // 0x546b80
// decomp41 (level_scripts_common.cpp, the level ambients): (sound handle, update interval ms, repeat
// delay ms), -1 when emitters are off or the sound is missing; (emitter, fade); (emitter, pos, velocity,
// 1.75f). decomp69 (D5030): the second argument is the position update interval (+0x48, the
// RealtimeTimer timeout of sound_emitter_update_position), not a volume; delete returns -1 when emitters
// are off, else 0.
int  sound_create_emitter(int sound, int update_ms, int delay_ms);         // 0x5466f0
int  sound_emitter_delete(int emitter, int fade);                          // 0x5467c0 (return unused)
void sound_emitter_update_position(int emitter, const vector3 *pos, const vector3 *vel, float f);   // 0x5468c0
// decomp55 (the script editor): the sound and the repeat delay (ms) of an emitter (-1 / 0 out of range).
int  sound_emitter_get_sound(int emitter);                                 // 0x546c20
int  sound_emitter_get_repeat_delay(int emitter);                          // 0x546c50
// One sounds.tbl record (0x40 bytes; sounds 0x2cb5458, 0x280 records, snd_init_sounds clears them).
// snd_get_handle stores p1 at +0x2c (1.0 when <= 0), p2 at +0x28, p3 at +0x34 and the rolloff distance
// computed from them at +0x30 (decomp55). decomp60: +0x38 / +0x3c and is_used (sum.symbols: Sound).
struct snd_sound {
    // Inline: the COMDAT 0x521e10 at the end of the sound object (thiscall).
    bool is_used()                    // 0x521e10 Sound__is_used
    { return name[0] != 0; }
    char name[0x28];                  // +0x00 file name ("Sound filename %s too long" at 0x28)
    float volume;                     // +0x28 snd_get_handle p2
    float min_range;                  // +0x2c snd_get_handle p1
    float max_range;                  // +0x30 snd_calc_rolloff (drawn as the outer sphere)
    float rolloff;                    // +0x34 snd_get_handle p3
    int handle;                       // +0x38 ds buffer, -1 not loaded
    int group : 27;                   // +0x3c snd_group_volumes index (SHL 5 / SAR 5)
    unsigned int loaded : 1;          //       bit 27: snd_load ran (also after a failure)
    int level_refs : 3;               //       bits 28-30: levels using it (page in ++, release --)
    unsigned int load_failed : 1;     //       bit 31: snd_load gives up
};
LAYOUT_CHECK(snd_sound, sizeof(snd_sound) == 0x40);
extern snd_sound sounds[SND_MAX_SOUNDS];   // 0x2cb5458
// The EAX environment and the per-level EAX properties file (<level>.eax in data\internal).
void snd_set_eax_environment(const char *name);                            // 0x521e00 (return unused)
int  eax_save_props(const char *filename);                                 // 0x5220e0 nonzero = saved
int  eax_load_props(const char *filename);                                 // 0x522420 1 = loaded (decomp60; return unused)

// decomp42 (gameplay.cpp).
void snd_change_listener(vector3 *pos, vector3 *vel, matrix *orient);   // 0x520710
void snd_do_frame();                  // 0x520770
// decomp51 (game_pause / game_unpause): two arguments, only the second reaches ds_pause_all (0x5207a1).
void snd_pause_all(int a, int b);     // 0x520790
void snd_unpause_all();               // 0x5207b0
void snd_level_page_in();             // 0x520c70
void snd_level_release();             // 0x520ce0
void snd_stop_all();                  // 0x521600
void snd_free(int handle);            // 0x520c10 snd_unload, then clears the name and level bits (decomp52)

// ---- decomp60: the rest of the sound unit (0x5204c0-0x521e38) ----------------------------------------
void snd_init_sounds();               // 0x520680 once: every record empty
void snd_init_group_volumes();        // 0x5206c0 all 1.0
void snd_unload(int handle);          // 0x520bc0
void snd_unload_all();                // 0x520c50
float snd_get_volume(int handle, float scale);   // 0x520de0 group volume * volume * scale
// (handle, pos, scale): 2D volume of a sound at pos for the listener (0 beyond max_range).
float snd_calc_volume_3d(int handle, const vector3 *pos, float scale);   // 0x520fa0
// (handle, segment a, segment b, scale): the same for the nearest point of a segment.
float sound_emitter_calc_volume(int handle, const vector3 *a, const vector3 *b, float scale);   // 0x5210b0
float snd_calc_pan(const vector3 *pos);                       // 0x5211f0 listener right . direction
float sound_emitter_calc_pan(const vector3 *a, const vector3 *b);   // 0x521270
// (frequency, pos, velocity): Doppler-shifted frequency, speed of sound 600.
int  sound_emitter_calc_doppler(int freq, const vector3 *pos, const vector3 *vel);   // 0x521310
int  sound_emitter_calc_doppler_segment(int freq, const vector3 *a, const vector3 *b, const vector3 *vel);   // 0x5213b0
void snd_set_volume_in_group(int handle, int sig, float volume);   // 0x5216e0
void snd_set_pan(int sig, float pan); // 0x521760 clamped to -1..1
// (handle, a, b, c): loads the sound if needed, then ds_get_format (snd_calc_duration multiplies a * b).
void snd_get_format(int handle, int *a, int *b, int *c);      // 0x521830
void snd_set_group(int handle, int group);                    // 0x5218a0
float snd_get_group_volume(int group);                        // 0x521950
int  snd_get_num_playing_channels();                          // 0x521960
snd_sound *snd_get_sound(int handle);                         // 0x521980 NULL out of range
bool snd_file_exists(const char *name);                       // 0x5219a0 (path stripped)
int  snd_get_sound_info(int handle, snd_sound *out);          // 0x521a30 copies the record; 0 / -1
void sounds_parse_entry(TextFile *tf, int index);             // 0x521b10
float snd_get_rolloff(int handle);                            // 0x521b70 max_range, 0 out of range
bool snd_is_ds3d_enabled();                                   // 0x521b90
void snd_restore_reverb();                                    // 0x521bb0
void snd_mute_reverb();                                       // 0x521bc0
bool snd_is_reverb_muted();                                   // 0x521bd0
float snd_get_duration(int handle);                           // 0x521be0 ds_estimate_duration
int  snd_calc_duration(int handle);                           // 0x521c20 a * b * duration / 8, 0 when not loaded
int  snd_estimate_file_duration(int handle);                  // 0x521ca0 as snd_calc_duration, else the file size
// (min range, rolloff, volume): min * volume / (rolloff * 0.1) + min * (1 - 1 / rolloff).
float snd_calc_rolloff(float min_range, float rolloff, float volume);   // 0x521dd0
// Inline: its only copy is the COMDAT 0x521e20 at the end of the sound object (unsigned compares).
inline void clamp_uint(unsigned int *v, unsigned int lo, unsigned int hi)   // 0x521e20
{
    if (*v < lo)
        *v = lo;
    else if (*v > hi)
        *v = hi;
}
extern vector3 snd_listener_pos;      // 0x2cb5420
extern vector3 snd_listener_vel;      // 0x2cb5410
extern matrix snd_listener_orient;    // 0x2cbf458
extern float snd_group_volumes[SND_NUM_GROUPS];   // 0x2cb542c
extern int snd_backend;               // 0x2cbf47c 1 = DirectSound
extern bool snd_disabled;             // 0x2cbf480

// ---- decomp60: the EAX unit (0x521e40-0x5225a1) -----------------------------------------------------
int  eax_set_decay_time(float t);     // 0x521e40 clamped to 0.1..20; 0 / -1
int  eax_set_environment(int env);    // 0x521e90 0 / -1
int  eax_set_environment_by_name(const char *name);   // 0x521ed0 one of the 26 names, -1 unknown
int  eax_stub_ret_m1();               // 0x521f10 (no caller found)
void eax_close();                     // 0x521f20
int  eax_init();                      // 0x521f40 0 / -1 (ds_init)
void eax_buffer_init(IUnknown *buffer);   // 0x521fd0 the buffer's own reverb off (ds_play)
bool eax_is_reverb_muted();           // 0x5220d0
void eax_get_listener_props(EAXLISTENERPROPERTIES *props);   // 0x522240
void eax_set_listener_props(EAXLISTENERPROPERTIES *props);   // 0x522580
extern const char *eax_environment_names[EAX_NUM_ENVIRONMENTS];   // 0x5a8960
extern int eax_reverb_muted;          // 0x2cbf488
extern long eax_saved_reverb;         // 0x2cbf48c
extern IKsPropertySet *eax_property_set;   // 0x2cbf490
extern bool eax_enabled;              // 0x2cbf494

// ---- decomp69: the sound emitter unit (0x546150-0x546cf0) ---------------------------------------------
#include "vsdk/os/timestamp.h"

#define SOUND_MAX_EMITTERS     0x80
#define SOUND_MAX_PLAYING      16     // sound_emitter_play steals the quietest beyond this
#define SOUND_EMITTER_USED     0x01
#define SOUND_EMITTER_PAUSED   0x02
#define SOUND_EMITTER_SEGMENT  0x04   // the source is the segment seg_a - seg_b (nearest point)
#define SOUND_EMITTER_FLAG8    0x08   // cleared by stop, play and delete, set nowhere (D5032)

// One emitter (0x74 bytes). The constructor is the implicit one (0x546c80, a COMDAT after the unit's
// functions: the member constructors of the five vectors and two timers, nothing else). Field names ours
// unless sum.symbols names them.
struct sound_emitter {
    sound_emitter *next;              // +0x00 list links (sound_emitter_list_head is the sentinel)
    sound_emitter *prev;              // +0x04
    int sound;                        // +0x08 snd handle, -1 free
    int sig;                          // +0x0c playing instance, -1 none
    float volume;                     // +0x10 last computed volume (voice stealing compares it)
    float pan;                        // +0x14 last pan set
    float volume_scale;               // +0x18 scale of the last 3D volume update (-1.0 forces one)
    int frequency_set;                // +0x1c last Doppler frequency set
    int frequency;                    // +0x20 base frequency (snd_get_format; 11025 out of range)
    vector3 pos;                      // +0x24
    vector3 last_pos;                 // +0x30 pos at the previous update (the 3D path)
    vector3 vel;                      // +0x3c
    int update_ms;                    // +0x48 position update interval
    RealtimeTimer update_timer;       // +0x4c
    int flags;                        // +0x50 SOUND_EMITTER_*
    int repeat_delay;                 // +0x54 ms between plays, 0 = looping
    GameTimer repeat_timer;           // +0x58
    vector3 seg_a;                    // +0x5c
    vector3 seg_b;                    // +0x68
};
LAYOUT_CHECK(sound_emitter, sizeof(sound_emitter) == 0x74);

extern sound_emitter sound_emitter_list_head;         // 0x2f076f0
extern sound_emitter sound_emitters[SOUND_MAX_EMITTERS];   // 0x2f07768
extern int sound_emitters_num_playing;                // 0x2f0b168
extern bool sound_emitters_enabled;                   // 0x2f0b16c

void sound_emitters_delete_all();                     // 0x5461a0
void sound_emitter_reset(int emitter);                // 0x5461f0
int  sound_emitter_find_free();                       // 0x546230 -1 when all 128 are used
int  sound_emitter_find_quietest();                   // 0x546250 -1 when none plays
void sound_emitter_stop(int emitter, int fade_ms);    // 0x5462b0
void sound_emitter_play(int emitter);                 // 0x546320
bool sound_emitter_stub_false();                      // 0x546430 the (disabled) DirectSound3D path test
void sound_emitter_update_volume(int emitter, float scale);   // 0x546440
void sound_emitter_update_pan(int emitter);           // 0x546500
void sound_emitter_update_frequency(int emitter);     // 0x546580
void sound_emitters_close();                          // 0x546650 (no caller)
// (sound, a, b, update_ms, delay_ms): an emitter for the segment a - b.
int  sound_emitter_create_segment(int sound, const vector3 *a, const vector3 *b, int update_ms, int delay_ms);   // 0x546670
// (emitter, a, b, vel, scale): sets the segment ends (NULL keeps one), then the position update with a
// (sum.symbols: "(idx, pos, vel)"). No caller.
void sound_emitter_set_pos_vel(int emitter, const vector3 *a, const vector3 *b, const vector3 *vel, float scale);   // 0x546840
void sound_emitter_set_frequency(int emitter, int freq);   // 0x546bc0 (no caller)
int  sound_emitter_get_frequency(int emitter);        // 0x546bf0 (no caller)

#endif
