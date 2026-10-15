// vsdk/snd/snd.h - the sound library (snd_* 0x5207d0-0x521ba0, eax_* 0x522020-0x522090; directory and
// file name guessed). Not original source.
#ifndef VSDK_SND_SND_H
#define VSDK_SND_SND_H

#include "vsdk/math/vector.h"
#include <windows.h>                  // HWND (snd_init)

// The three floats of snd_get_handle are the three table parameters of a sound (sounds.tbl); their
// meaning is not settled. group: 0 effects, 1 music. from_start: search from slot 0, not 8.
int  snd_get_handle(const char *name, float p1, float p2, float p3, int group, bool from_start); // 0x5207d0
int  snd_load(int handle, int streamed);                                    // 0x5209c0
int  snd_play(int handle, float volume, float pan, int unused, int group); // 0x520d50
int  snd_play_streamed(int handle, float volume, int arg);                  // 0x520ea0
int  snd_play_streamed_looping(int handle, float volume, int arg);          // 0x520f20
int  snd_play_3d(int handle, const vector3 *pos, float a, float b, int c);  // 0x521480 (trailing types unsure)
void snd_stop(int sig);                                                    // 0x5215d0
void snd_stop_fade(int sig, int ms);                                       // 0x521620
bool snd_is_playing(int sig);                                              // 0x521650
void snd_set_frequency(int sig, int freq);                                 // 0x5217c0 (decomp37)
void snd_get_frequency(int sig, unsigned int *freq);                       // 0x521800 (decomp37)
void snd_set_volume(int sig, float volume);                                // 0x521680
void snd_set_group_volume(int group, float volume);                        // 0x5218e0 (0 = effects)
bool snd_is_eax_enabled();                                                 // 0x521ba0
void eax_mute_reverb();                                                    // 0x522020
void eax_restore_reverb();                                                 // 0x522090
// (hwnd, 44100, 16, 2, directsound3d, eax, no_sound, dsinit) from LoadSettings; atexit(snd_close).
void snd_init(HWND hwnd, int rate, int bits, int channels, bool use_ds3d, bool use_eax, bool no_sound,
              bool dsinit);                                                // 0x5205b0
void snd_close();                                                          // 0x5206e0
void sounds_parse_table();                                                 // 0x521a60 sounds.tbl
// Sound emitters (level ambients; unit not known, decomp27).
void sound_emitter_pause(int emitter);                                     // 0x546b20 (return unused)
void sound_emitter_unpause(int emitter);                                   // 0x546b80 (return unused)
// decomp41 (level_scripts_common.cpp, the level ambients): (sound handle, volume 100, repeat delay ms),
// -1 when emitters are off or the sound is missing; (emitter, fade); (emitter, pos, velocity, 1.75f).
int  sound_create_emitter(int sound, int volume, int delay_ms);            // 0x5466f0
void sound_emitter_delete(int emitter, int fade);                          // 0x5467c0 (return unused)
void sound_emitter_update_position(int emitter, const vector3 *pos, const vector3 *vel, float f);   // 0x5468c0
// The EAX environment and the per-level EAX properties file (<level>.eax in data\internal).
void snd_set_eax_environment(const char *name);                            // 0x521e00 (return unused)
int  eax_save_props(const char *filename);                                 // 0x5220e0 nonzero = saved
void eax_load_props(const char *filename);                                 // 0x522420 (return unused)

// decomp42 (gameplay.cpp).
class matrix;
void snd_change_listener(vector3 *pos, vector3 *vel, matrix *orient);   // 0x520710
void snd_do_frame();                  // 0x520770
void snd_level_page_in();             // 0x520c70
void snd_level_release();             // 0x520ce0
void snd_stop_all();                  // 0x521600

#endif
