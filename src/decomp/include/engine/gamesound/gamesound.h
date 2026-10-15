// gamesound.h - Sum.exe v1.40 game sound layer (Engine\gamesound\gamesound.cpp, 0x468c80-0x4697d6)
// Written from the Sum.exe disassembly. Names from symbols/sum.symbols.
#ifndef GAMESOUND_H
#define GAMESOUND_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"

// A sound reference in a table record (12 bytes): spells_parse_sound (0x47a6c0) and sound_ref_parse
// (0x446790) set the name and handle -1, then store three table values as bytes.
struct sound_ref {
    const char *name;                 // +0x0 sound file name (string pool)
    int handle;                       // +0x4 -1 until loaded
    ubyte params[3];                  // +0x8 volume (0..255), two more table values
    ubyte _pad0b;
};
LAYOUT_CHECK(sound_ref, sizeof(sound_ref) == 0xc);

#define MAX_SURFACES 17                 // surface_sound_table is 17 x 17
#define MAX_SURFACE_TYPES 64            // surface_types (0x200 bytes)
#define MAX_MUSIC_TRACKS 64             // music_tracks (0x200 bytes)
#define MAX_FEEDBACK_SOUNDS 255         // feedback_sounds (0x3fc bytes)
#define MAX_FEEDBACK_PARTY 4            // Joseph 0, Jekhar 1, Flece 2, Rosalind 3
#define MAX_FEEDBACK_EVENTS 16          // "+Affirmative:" .. "+Death:"
#define MAX_FEEDBACK_ALTERNATIVES 4

struct surface_type { int bitmap; int surface; };       // bitmap handle -> surface index
struct music_track { const char *name; const char *filename; };

extern int surface_sound_table[MAX_SURFACES][MAX_SURFACES]; // 0x941754
extern music_track music_tracks[MAX_MUSIC_TRACKS];          // 0x9421d8
extern int num_music_tracks;                                // 0x94389c

void sound_ref_load(sound_ref *ref, bool load);
int game_play_sound(int handle, float volume, float pan, int unused, int group);
int sound_play_2d(int sound, float volume);
int sound_play_3d_at(int sound, const vector3 *pos);
int surface_play_sound(int surface, int kind, const vector3 *pos);
void surfaces_parse_table(const char *level_name);
int lobj_face_get_surface(int lobj_index, int face);
void music_play(int index, bool loop);
bool music_is_playing();
int music_get_current();
void music_stop(int fade_ms);
void music_set_volume(float volume);
void music_feedback_parse_tables();
void surface_sound_table_reset();
int music_find_by_name(const char *name);
void feedback_play(int species, int event, const vector3 *pos);

#endif
