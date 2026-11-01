// [Experimental] LevelReverb: each level's own EAX reverb (docs/experimental-build.md). Not part of Sum.exe: compiled
// only for link-build --config experimental.
//
// The game ships 49 <level>.eax files in tables.vpp, but level_parse_table hands eax_load_props a path in
// data\internal, which it opens with the C runtime's fopen: packfiles are invisible to it and the retail install has
// no such folder, so every level keeps the "generic" environment (game-bugs K261). Here, when the loose file is
// missing, the same file is read through the game's File class (disk, then packfiles) and its 14 values parsed as
// eax_load_props parses them. The files were written with K261's bug: their flags line holds the bits of the air
// absorption (-5.0, 0xC0A00000), which would clear the six scale and clip flags and set reserved ones, so flags with
// bits outside the six are replaced by EAX 2's default (all six set).
//
// It only sounds with EAX (Soundcard=EAX on an EAX-capable DirectSound, such as a DirectSound-with-EAX wrapper); the
// values are parsed and logged either way.
#include <stdio.h>
#include <string.h>
#include "experimental/experimental.h"

#ifdef EXPERIMENTAL_BUILD

#include "vsdk/vsdk.h"
#include "vsdk/vfile/file.h"
#include "vsdk/snd/snd.h"
#include "summoner/summoner.h"

#define EAX_LISTENER_FLAGS_ALL 0x3f    // EAXLISTENERFLAGS_DECAYTIMESCALE .. _DECAYHFLIMIT, EAX 2's default

// level_parse_table, after eax_load_props(path) found no loose file: name is "<level>.eax".
void exp_level_reverb(const char *name)
{
    static int enabled = -1;
    if (enabled < 0)
        enabled = exp_ini_int("Experimental", "LevelReverb", 0);
    if (!enabled)
        return;

    File f;
    char text[512];
    if (f.open(name, 1, path_internal_copy) < 0) {
        exp_log("LevelReverb: %s not found", name);
        return;
    }
    int n = f.read(text, sizeof text - 1, 0, 0);
    f.close();
    text[n > 0 ? n : 0] = 0;

    EAXLISTENERPROPERTIES p;
    int got = sscanf(text, "%d %d %f %f %f %d %f %d %f %d %f %f %f %d", &p.lRoom, &p.lRoomHF,
                     &p.flRoomRolloffFactor, &p.flDecayTime, &p.flDecayHFRatio, &p.lReflections,
                     &p.flReflectionsDelay, &p.lReverb, &p.flReverbDelay, (int *)&p.dwEnvironment,
                     &p.flEnvironmentSize, &p.flEnvironmentDiffusion, &p.flAirAbsorptionHF, (int *)&p.dwFlags);
    if (got != 14) {
        exp_log("LevelReverb: %s: %d of 14 values", name, got);
        return;
    }
    unsigned file_flags = p.dwFlags;
    if (file_flags & ~EAX_LISTENER_FLAGS_ALL)
        p.dwFlags = EAX_LISTENER_FLAGS_ALL;
    bool on = snd_is_eax_enabled();
    exp_log("LevelReverb: %s: room %d, decay %.2f s, environment %u, size %.1f, flags 0x%x (file 0x%x)%s", name,
            p.lRoom, p.flDecayTime, p.dwEnvironment, p.flEnvironmentSize, p.dwFlags, file_flags,
            on ? ", set" : ", EAX off: not set");
    if (on)
        eax_set_listener_props(&p);
}

#endif // EXPERIMENTAL_BUILD
