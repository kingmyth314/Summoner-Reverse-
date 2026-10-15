// gamesound.cpp - Sum.exe v1.40, code 0x468c80-0x4697d6 (Engine\gamesound\gamesound.cpp)
// Written from the Sum.exe disassembly (build/sum.asm). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: symbols/sum.symbols tags only the last nine functions (0x469080-0x469720) with
// gamesound.cpp. The unit starts at the static initialiser 0x468c80 instead: its .bss
// (0x941740-0x9438a0) interleaves the surface tables (surface_types 0x941bd8, num_surface_types
// 0x9421d4) with the music/feedback tables, surface_sound_table_reset fills
// surface_sound_table, and docs/sum-exe/sound.md already lists game_play_sound, sound_play_2d,
// sound_play_3d_at and surface_play_sound as gamesound.cpp. So this file also holds
// 0x468c80-0x469072 (11 more functions). The previous function (0x468c70) is combat code.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "engine/gamesound/gamesound.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/console.h"
#include "vsdk/os/vstring.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/parse/parse.h"
#include "vsdk/vfile/file.h"
#include "vsdk/snd/snd.h"
#include "vsdk/bmpman/bmpman.h"
#include "engine/rendering/levelrender.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/objects/living_entity.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"

// Original __FILE__ of this unit (literal 0x59a504).
#define GAMESOUND_CPP "D:\\projects\\Summoner\\pccode\\Engine\\gamesound\\gamesound.cpp"

// ---- .data, in address order (0x59a31c-0x59a383, strings follow) ---------------------------
// Labels of the two pointer tables are ours. "wood" and "stone" are pooled with an earlier unit's
// literals (0x5991e8, 0x5991f0), so the build used string pooling (/Gf).
static const char *surface_type_names[5] = {      // 0x59a31c  _surfaces.tbl type strings
    "wood", "stone", "grass", "sand", "swamp",
};
int surface_default = 1;                          // 0x59a330  (surfaces_parse_table sets 2)
int music_play_signature = -1;                    // 0x59a334
int music_sound_handle = -1;                      // 0x59a338
int music_index = -1;                             // 0x59a33c
bool music_looping = true;                        // 0x59a340  (dword slot 0x00000001)
static const char *feedback_event_names[MAX_FEEDBACK_EVENTS] = { // 0x59a344
    "+Affirmative:", "+Attack:", "+NoEffect:", "+Locked:", "+Injured:", "+InvalidAction:",
    "+InvalidLocation:", "+InvalidTarget:", "+Success:", "+Failed:", "+Poison:", "+HideSuccess:",
    "+SneakSuccess:", "+Unlocked:", "+LevelUp:", "+Death:",
};

// ---- .bss, in address order ----------------------------------------------------------------
// The pool's buffer sits after it (0x9423d8) but must be named in the constructor, so it is
// declared here and defined in its address slot below. Whether VC6 keeps .bss in definition
// order is the same open question as frametime.cpp's saved limits.
extern char feedback_string_buffer[0x1400];
// 0x468c80 sinit_stringpool_941740 (call init, jmp register), 0x468c90 init_stringpool_941740,
// 0x468cb0 register_dtor_stringpool_941740 (atexit(0x468cc0)), 0x468cc0 dtor_stringpool_941740
// (mov ecx, this; jmp StringPool::~StringPool): the compiler-generated dynamic initialiser and
// atexit destructor of this definition. Confidence: exact (allocated = false is the push 0).
StringPool feedback_string_pool(feedback_string_buffer, 0x1400, false); // 0x941740
int surface_sound_table[MAX_SURFACES][MAX_SURFACES];  // 0x941754
static surface_type surface_types[MAX_SURFACE_TYPES]; // 0x941bd8
static int feedback_sounds[MAX_FEEDBACK_SOUNDS];      // 0x941dd8
static int num_surface_types;                         // 0x9421d4
music_track music_tracks[MAX_MUSIC_TRACKS];           // 0x9421d8
char feedback_string_buffer[0x1400];                  // 0x9423d8
static ubyte feedback_first_sound[MAX_FEEDBACK_PARTY][MAX_FEEDBACK_EVENTS]; // 0x9437d8
static ubyte feedback_sound_count[MAX_FEEDBACK_PARTY][MAX_FEEDBACK_EVENTS]; // 0x943818
static ubyte feedback_chance[MAX_FEEDBACK_PARTY][MAX_FEEDBACK_EVENTS];      // 0x943858
// Unsigned: the bound check is CMP ECX, 0xFF / JAE.
static unsigned int num_feedback_sounds;              // 0x943898
int num_music_tracks;                                 // 0x94389c

// 0x468cd0  Confidence: likely. The three floats are computed in params[0], [1], [2] order but
// passed as (p[1], p[0] / 255, p[2]), so the source used locals (or the call had them in this
// order and VC6 evaluated left to right; it normally goes right to left). 0x3b808081 = 1/255.
// The failure text goes to string_buffer and is never shown.
void sound_ref_load(sound_ref *ref, bool load)
{
    if (ref->name != NULL && ref->handle < 0) {
        float volume = ref->params[0] * (1.0f / 255.0f);
        float p1 = ref->params[1];
        float p2 = ref->params[2];
        int handle = snd_get_handle(ref->name, p1, volume, p2, 0, false);

        if (handle == -1) {
            sprintf(string_buffer, "FAILED TO CREATE SOUND BUFFER FOR FILE %s\n", ref->name);
            return;
        }
        ref->handle = handle;
        if (load) {
            if (snd_load(handle, 0) == -1)
                ref->handle = -1;
        }
    }
}

// 0x468d90  Confidence: exact. Always returns 1.
int game_play_sound(int handle, float volume, float pan, int unused, int group)
{
    if (handle > 0)
        snd_play(handle, volume, pan, unused, group);
    return 1;
}

// 0x468dc0  Confidence: exact.
int sound_play_2d(int sound, float volume)
{
    return snd_play(sound, volume, 0.0f, 0, 1);
}

// 0x468de0  Confidence: likely (the -1 is loaded before the test: an rval local or VC6
// hoisting the else value).
int sound_play_3d_at(int sound, const vector3 *pos)
{
    int rval = -1;

    if (sound > 0)
        rval = snd_play_3d(sound, pos, 0.0f, 0.0f, 0);
    return rval;
}

// 0x468e00  Confidence: exact. Index = surface * 17 + kind (shl 4 + add).
int surface_play_sound(int surface, int kind, const vector3 *pos)
{
    int sound = surface_sound_table[surface][kind];

    if (sound == -1)
        return -1;
    return sound_play_3d_at(sound, pos);
}

// 0x468e30  Confidence: likely (local order; EH states: File is state 0, TextFile state 1).
// "<level>_surfaces.tbl": a default surface type, then (bitmap name, surface type) pairs.
// Known defect: an unknown bitmap re-parses the type token with ParseString and size 0x100 into
// the 64-byte name buffer.
void surfaces_parse_table(const char *level_name)
{
    File file;
    TextFile tf;
    char filename[64];
    char name[64];

    num_surface_types = 0;
    surface_default = 2;
    file_strip_ext(filename, level_name);
    strcat(filename, "_surfaces.tbl");
    if (file.find(filename, PATH_ANY)) {
        tf.Open(filename, PATH_ANY);
        tf.Expect("$Default:");
        surface_default = tf.ParseTypeString(surface_type_names, 5);
        while (!tf.Accept("#End")) {
            tf.ParseString(name, 64, '"', '"');
            int bm = bm_find(name);
            if (bm == -1) {
                tf.ParseString(name, 256, '"', '"');
            } else {
                surface_types[num_surface_types].bitmap = bm;
                surface_types[num_surface_types].surface = tf.ParseTypeString(surface_type_names, 5);
                num_surface_types++;
            }
        }
        tf.Close();
    }
}

// 0x469010  Confidence: exact. Face texture index is 1-based (level_texture_bms[tex - 1]).
int lobj_face_get_surface(int lobj_index, int face)
{
    if (lobj_index >= 0 && face >= 0) {
        int bm = level_texture_bms[lobjs[lobj_index].faces[face].texture - 1];
        for (int i = 0; i < num_surface_types; i++) {
            if (bm == surface_types[i].bitmap)
                return surface_types[i].surface;
        }
    }
    return surface_default;
}

// 0x469080  Confidence: exact. 200 ms fade (0xc8). The loop test uses the argument, not the
// global it was just stored in.
void music_play(int index, bool loop)
{
    if (index < 0)
        return;

    if (music_is_playing()) {
        if (index == music_index)
            return;
        music_stop(200);
    } else if (music_play_signature >= 0) {
        snd_stop(music_play_signature);
        music_play_signature = -1;
    }

    music_index = index;
    music_looping = loop;
    music_sound_handle = snd_get_handle(music_tracks[index].filename, 1.0f, 1.0f, 1.0f, 1, true);
    if (music_sound_handle == -1)
        return;

    if (loop)
        music_play_signature = snd_play_streamed_looping(music_sound_handle, 1.0f, 0);
    else
        music_play_signature = snd_play_streamed(music_sound_handle, 1.0f, 0);
}

// 0x469140  Confidence: exact.
bool music_is_playing()
{
    if (music_play_signature == -1)
        return false;
    return snd_is_playing(music_play_signature);
}

// 0x469160  Confidence: exact.
int music_get_current()
{
    if (music_is_playing())
        return music_index;
    return -1;
}

// 0x469180  Confidence: exact.
void music_stop(int fade_ms)
{
    if (music_is_playing()) {
        snd_stop_fade(music_play_signature, fade_ms);
        music_play_signature = -1;
        music_index = -1;
    }
}

// 0x4691b0  Confidence: exact. Group 1 = music.
void music_set_volume(float volume)
{
    snd_set_group_volume(1, volume);
    if (music_play_signature >= 0)
        snd_set_volume(music_play_signature, volume);
}

// 0x4691e0  Confidence: likely (local order; the event loop is compiled as a pointer walk over
// feedback_event_names with a separate index, which an index loop gives under /O2).
// music.tbl: $Name: / $Filename: pairs into music_tracks (strings in feedback_string_pool).
// feedback.tbl: per hero "$Name:", then for each of the 16 events "<event>" chance and up to 4
// quoted file names; each file becomes a sound handle in feedback_sounds.
// The music.tbl file is closed explicitly, feedback.tbl only by the TextFile destructor.
// Error lines: 661 (0x295), 680 (0x2a8), 695 (0x2b7).
void music_feedback_parse_tables()
{
    TextFile tf;
    char buf[256];
    char filename[256];
    char names[MAX_FEEDBACK_ALTERNATIVES][256];

    num_music_tracks = 0;
    feedback_string_pool.Reset();
    tf.Open("music.tbl", path_tables);
    while (!tf.Accept("#End")) {
        tf.Expect("$Name:");
        tf.ParseString(buf, 256, '"', '"');
        music_tracks[num_music_tracks].name = feedback_string_pool.AddUnique(buf);
        tf.Expect("$Filename:");
        tf.ParseString(buf, 256, '"', '"');
        music_tracks[num_music_tracks].filename = feedback_string_pool.AddUnique(buf);
        num_music_tracks++;
    }
    tf.Close();

    surface_sound_table_reset();
    memset(feedback_sound_count, 0, sizeof(feedback_sound_count));

    tf.Open("feedback.tbl", path_tables);
    while (!tf.Accept("#End")) {
        int party;

        tf.Expect("$Name:");
        tf.ParseString(buf, 256, '"', '"');
        if (_stricmp(buf, "Rosalind") == 0)
            party = 3;
        else if (_stricmp(buf, "Flece") == 0)
            party = 2;
        else if (_stricmp(buf, "Joseph") == 0)
            party = 0;
        else if (_stricmp(buf, "Jekhar") == 0)
            party = 1;
        else
            for (;;)
                DebugError(GAMESOUND_CPP, 661, "Bad feedback party name.\n");

        for (int event = 0; event < MAX_FEEDBACK_EVENTS; event++) {
            unsigned int count = 0;

            tf.Expect(feedback_event_names[event]);
            feedback_chance[party][event] = (ubyte)tf.ParseInt();
            while (tf.Peek("\"")) {
                if (count >= MAX_FEEDBACK_ALTERNATIVES)
                    for (;;)
                        DebugError(GAMESOUND_CPP, 680, "Too many feedback sounds for this action!\n");
                tf.ParseString(names[count], 256, '"', '"');
                count++;
            }

            feedback_sound_count[party][event] = (ubyte)count;
            feedback_first_sound[party][event] = (ubyte)num_feedback_sounds;
            for (unsigned int i = 0; i < count; i++) {
                if (num_feedback_sounds >= MAX_FEEDBACK_SOUNDS)
                    for (;;)
                        DebugError(GAMESOUND_CPP, 695, "Too many feedback sounds!  c'mon guys ..\n");
                sprintf(filename, "feedback\\%s", names[i]);
                feedback_sounds[num_feedback_sounds] = snd_get_handle(filename, 5.0f, 1.0f, 1.0f, 0, false);
                num_feedback_sounds++;
            }
        }
    }
}

// Sets both [a][b] and [b][a] of the surface sound table (a pair of surfaces gives one sound).
#define SET_SURFACE_SOUND(a, b, sound) \
    (surface_sound_table[a][b] = (sound), surface_sound_table[b][a] = (sound))

// 0x469580  Confidence: unsure (statement order). All 289 entries become -1, then a fixed
// symmetric table of sound ids 0x0e-0x22 for surfaces 1-5 and 10-16 (the binary's store order is
// the compiler's: it groups equal constants in EAX). The row loop for 10-16 compares the row
// pointer with JLE against row 16 (0x941ba8), hence <= 16.
// Despite the name (symbols/sum.symbols), it resets the surface table, not music or feedback.
void surface_sound_table_reset()
{
    memset(surface_sound_table, 0xff, sizeof(surface_sound_table));

    SET_SURFACE_SOUND(1, 1, 0x22);
    SET_SURFACE_SOUND(2, 1, 0x20);
    SET_SURFACE_SOUND(2, 2, 0x21);
    SET_SURFACE_SOUND(3, 1, 0x1d);
    SET_SURFACE_SOUND(3, 2, 0x1e);
    SET_SURFACE_SOUND(3, 3, 0x1f);
    SET_SURFACE_SOUND(4, 1, 0x19);
    SET_SURFACE_SOUND(4, 2, 0x1a);
    SET_SURFACE_SOUND(4, 3, 0x1b);
    SET_SURFACE_SOUND(4, 4, 0x1c);
    SET_SURFACE_SOUND(5, 1, 0x14);
    SET_SURFACE_SOUND(5, 2, 0x16);
    SET_SURFACE_SOUND(5, 3, 0x17);
    SET_SURFACE_SOUND(5, 4, 0x18);
    SET_SURFACE_SOUND(5, 5, 0x15);

    for (int i = 10; i <= 16; i++) {
        SET_SURFACE_SOUND(i, 1, 0x0e);
        SET_SURFACE_SOUND(i, 5, 0x0f);
        SET_SURFACE_SOUND(i, 2, 0x10);
        SET_SURFACE_SOUND(i, 3, 0x11);
        SET_SURFACE_SOUND(i, 4, 0x12);
        surface_sound_table[i][i] = 0x13;
    }
}

// 0x4696d0  Confidence: exact. The bound is reloaded every pass (num_music_tracks is a global
// and _stricmp could change it).
int music_find_by_name(const char *name)
{
    for (int i = 0; i < num_music_tracks; i++) {
        if (_stricmp(name, music_tracks[i].name) == 0)
            return i;
    }
    return -1;
}

// 0x469720  Confidence: likely (the byte compare of the roll: CMP AL, [chance] / JA implies an
// unsigned byte on both sides; DIV implies an unsigned count). Events 1 (Attack) and 4 (Injured)
// are positional; the rest play 2D. Switch shape from DEC EAX / SUB EAX, 3.
void feedback_play(int species, int event, const vector3 *pos)
{
    if (!species_is_hero(species))
        return;
    if (!(game_flags & 0x200) || (game_flags & 0x20))
        return;

    unsigned int count = feedback_sound_count[species][event];
    int first = feedback_first_sound[species][event];
    if (count == 0)
        return;

    ubyte roll = (ubyte)(int)(frand() * 100.0f);
    if (roll > feedback_chance[species][event])
        return;

    int index = first + rand() % count;
    switch (event) {
    case 1:
    case 4:
        sound_play_3d_at(feedback_sounds[index], pos);
        break;
    default:
        game_play_sound(feedback_sounds[index], 1.0f, 0.0f, 0, 0);
        break;
    }
}
