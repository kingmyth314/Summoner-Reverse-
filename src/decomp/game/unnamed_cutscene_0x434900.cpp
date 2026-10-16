// unnamed_cutscene_0x434900.cpp - Sum.exe v1.40, code 0x434900-0x437659: the cutscene player (cutscene.tbl,
// the camera files, actors, effects, sounds, fog and visibility keys, shot timing, the restore, the escape
// cutscenes and the three cutscene console commands).
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range (decomp52, D3201): the source map cut the file at 0x4349b0 and gave the optimised static
// initialisers 0x434900-0x4349a5 to the input unit before it. Their objects (the cutscene object 0x601e68,
// cutscene_entries 0x6034c8, cutscene_alloc_pool 0x5fccc0) lie inside this file's .bss, interleaved with
// variables only this file's unoptimised code uses (cutscene_saved_actors 0x5fcce8 between the pool and
// the object, ccmd_play_cutscene 0x6034b8 between the object and the entries), so they are defined here.
// One object cannot hold two optimisation levels except through #pragma optimize: the three objects come
// first, then the rest of the file is compiled without optimisation (ebp frames, 0xcc padding, every
// statement reloads its operands; this file's own ccmd initialisers 0x4374a0.. open ebp frames too).
// /Oi stays on (inline strlen / strcpy / strcmp), as in objects.cpp and living_entity.cpp.
// Local variables: VC6 12.00.8168 lays out /Od locals by a hash of their names (DECISIONS.md D6101; block
// locals after the function's own): names and declaration order reproduce the frame offsets noted per function.
//
// Data: .data 0x59399c-0x593c31 (cutscene_saved_music, cutscene_stream_sound, then the literals),
// .bss 0x5fccc0-0x609f3b. The float literals 1000.0 (0x57ef28), 1.0 (0x57e990), 0.0 (0x57eb84) are shared
// __real@ constants of earlier units.
//
// GAME BUG G3 (docs/sum-exe/timing-bugs.md, "Cutscene timer sign bug"): the shot clock is
// ((float)timeGetTime() - sequence_start_time) / 1000. sequence_start_time is unsigned; VC6 builds the
// zero-extended 64-bit copy of it but subtracts with FISUB dword, a SIGNED 32-bit load, so after ~24.86
// days of Windows uptime the start time reads as about -2^31 ms and every shot is overdue. Kept as in the
// binary in cutscene_sequence_update and cutscene_events_frame (the camera sampler 0x494310 has it too).

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>
#include "vsdk/os/console.h"
#include "vsdk/os/frametime.h"
#include "vsdk/os/norlsallocpool.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/parse/parse.h"
#include "vsdk/vfile/file.h"
#include "vsdk/gr/gr.h"
#include "vsdk/snd/snd.h"
#include "vsdk/geom/effect_mem.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/interface/interface.h"
#include "engine/camera/camera.h"
#include "engine/camera/camera_sequence.h"
#include "engine/objects/objects.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/effects/fx.h"
#include "engine/ai/ai.h"
#include "engine/ai/boss_ai.h"
#include "engine/gamesound/gamesound.h"
#include "engine/rendering/levelrender.h"
#include "levelscripts/level_scripts.h"
#include "summoner/unnamed_cutscene_0x434900.h"

// ---- .data (0x59399c-0x5939a3; the literals follow from 0x5939a4) --------------------------------------
int cutscene_saved_music = -1;        // 0x59399c
int cutscene_stream_sound = -1;       // 0x5939a0

// ---- the three objects with optimised initialisers (static_initializers entries 111-113) -------------
// Compiler-generated:
// 0x434900 sinit_cutscene_601e68 (call init, jmp register; entry 111). Confidence: exact.
// 0x434910 init_cutscene_601e68 (cutscene::cutscene on 0x601e68, tail jump). Confidence: exact.
// 0x434920 register_dtor_cutscene_601e68 (atexit(0x434930)). Confidence: exact.
// 0x434930 dtor_cutscene_601e68 (cutscene::~cutscene, tail jump). Confidence: exact.
cutscene the_cutscene;                                // 0x601e68
// Compiler-generated (no destructor, so no atexit):
// 0x434940 init_cutscene_entries_6034c8_thunk (jmp, entry 112). Confidence: exact.
// 0x434950 init_cutscene_entries_6034c8 (85 x cutscene_entry::cutscene_entry, stride 0x140). Confidence: exact.
cutscene_entry cutscene_entries[MAX_CUTSCENES];       // 0x6034c8
// Compiler-generated:
// 0x434970 sinit_norlsallocpool_5fccc0 (entry 113). Confidence: exact.
// 0x434980 init_norlsallocpool_5fccc0 (NoRlsAllocPool::NoRlsAllocPool, tail jump). Confidence: exact.
// 0x434990 register_dtor_norlsallocpool_5fccc0 (atexit(0x4349a0)). Confidence: exact.
// 0x4349a0 dtor_norlsallocpool_5fccc0 (NoRlsAllocPool::~NoRlsAllocPool, tail jump). Confidence: exact.
NoRlsAllocPool cutscene_alloc_pool;                   // 0x5fccc0

#ifdef _MSC_VER
#pragma optimize("", off)
#endif

// ---- .bss (0x5fcce8-0x609f3b) -------------------------------------------------------------------------
cutscene_saved_actor cutscene_saved_actors[MAX_CUTSCENE_SAVED_ACTORS];   // 0x5fcce8
char cutscene_pool_buffer[CUTSCENE_POOL_SIZE];        // 0x5fd260 (name ours)
float cutscene_saved_fog_multiplier;                  // 0x601e60
int cutscene_num_saved_actors;                        // 0x6034b0
float cutscene_saved_fog_near;                         // 0x609f08 the near distance (sum.symbols name; D3204)
float cutscene_saved_fog_far;                        // 0x609f1c the far distance (sum.symbols name; D3204)
float cutscene_last_time;                             // 0x609f2c
int cutscene_num_entries;                             // 0x609f30
bool cutscene_playing;                                // 0x609f34
char *cutscene_pending_name;                          // 0x609f38

// 0x4349b0  Confidence: exact. i at ebp-4. flags (+4) and script (+0xc) are left alone.
cutscene_entry::cutscene_entry()
{
    int i;

    name = NULL;
    level = NULL;
    num_cameras = 0;
    for (i = 0; i < MAX_CUTSCENE_CAMERAS; i++)
        cameras[i] = NULL;
    num_effects = 0;
    for (i = 0; i < MAX_CUTSCENE_EFFECTS; i++)
        effects[i] = NULL;
    num_shot_effects = 0;
    for (i = 0; i < MAX_CUTSCENE_SHOT_EFFECTS; i++)
        shot_effects[i] = NULL;
    soundtrack = NULL;
    num_minitracks = 0;
    for (i = 0; i < MAX_CUTSCENE_MINITRACKS; i++)
        minitracks[i] = NULL;
    num_fogs = 0;
    for (i = 0; i < MAX_CUTSCENE_FOGS; i++)
        fogs[i] = NULL;
    num_vissets = 0;
    for (i = 0; i < MAX_CUTSCENE_VISSETS; i++)
        vissets[i] = NULL;
    num_fades = 0;
    for (i = 0; i < MAX_CUTSCENE_FADES; i++)
        fades[i] = NULL;
}

// 0x434b40  Confidence: exact (exact flow; the frame order puts the File and the first float of each
// file in the loop block). Each $Camera file: magic "CSCR" (0x52435343), version >= 0x20000, two floats
// that are not used, the shot count, then the shots; each shot starts where the previous one ended.
// The path test reads this->path_id, File::open the object's field by its address (cutscene_path_id).
bool cutscene::load_files(cutscene_entry *e)
{
    int ix;                            // ebp-0x10 (EH frame above)
    float first = 0;                  // ebp-0x14
    int version;                      // ebp-0x20
    float pad;                   // ebp-0x1c
    int ident;                        // ebp-0x18
    int j;                            // ebp-0x24
    int num;                        // ebp-0x28

    if (path_id < 0)
        return false;
    num_sequences = 0;
    cur_sequence = -1;
    cur_fade_event = 0;
    cutscene_camera_tracks_clear();
    for (ix = 0; ix < e->num_cameras; ix++) {
        File f;                       // ebp-0x13c
        float reserved;               // ebp-0x140

        if (f.open(e->cameras[ix], 1, cutscene_path_id) < 0)
            return false;
        ident = f.read_int(0, 0);
        if (ident != 0x52435343) {
            f.close();
            return false;
        }
        version = f.read_int(0, 0);
        if (version < 0x20000) {
            f.close();
            return false;
        }
        f.set_version(version);
        reserved = f.read_float(0, 0);
        pad = f.read_float(0, 0);
        if (ix == 0)
            time_offset = first;
        num = f.read_int(0, 0);
        if (num <= 0) {
            f.close();
            return false;
        }
        for (j = 0; j < num; j++) {
            sequences[num_sequences + j].read_track(&f, first);
            first = sequences[num_sequences + j].end;
        }
        if (ix == e->num_cameras - 1)
            end_time = first;
        num_sequences = num_sequences + num;
        f.close();
    }
    return true;
}

// 0x434e20  Confidence: exact. i ebp-4, ok ebp-8, actor ebp-0xc (this ebp-0x10, then the strcmp temporaries).
// The effects without a shot: on the named actor, else (no such actor, or no name) in the world.
bool cutscene::bind_actors(cutscene_entry *e)
{
    int i;
    bool ok;
    Character *actor;

    ok = true;
    if (path_id < 0)
        return false;
    for (i = 0; i < e->num_effects; i++) {
        if (e->effects[i]->character != NULL) {
            for (actor = living_list.live_next; actor != &living_list; actor = actor->live_next) {
                if (strcmp(actor->get_name(), e->effects[i]->character) == 0)
                    break;
            }
            if (actor != &living_list)
                effect_handles[i] = fx_attach_to_character(e->effects[i]->name, actor->handle, -1, -1, 1, 0);
            else
                effect_handles[i] = fx_add_effect(e->effects[i]->name, &vector3_zero, &identity_matrix, 0);
        } else {
            effect_handles[i] = fx_add_effect(e->effects[i]->name, &vector3_zero, &identity_matrix, 0);
        }
        if (effect_handles[i] == -1)
            ok = false;
    }
    for (i = 0; i < e->num_shot_effects; i++)
        vfx_preload(e->shot_effects[i]->name, NULL);
    return ok;
}

// 0x435000  Confidence: exact. cutscene_active_count goes up before any test (cutscene_cleanup takes it
// down; load_data ignores this result and every earlier failure returns before this call, so it does not
// leak). While it is nonzero, state_game_frame and ai_frame step the world with cutscene_frametime.
// GAME BUG (latent, NOTES.md B3): the minitrack handles are only ever set to -1, so a +Minitrack plays sound -1
// (no retail entry has one).
bool cutscene::load_sounds(cutscene_entry *e)
{
    int i;

    cutscene_active_count++;
    snd_level_page_in();
    if (path_id < 0)
        return false;
    if (e->soundtrack != NULL) {
        soundtrack_sound = snd_get_handle(e->soundtrack, 1.0f, 1.0f, 1.0f, 0, true);
        if (soundtrack_sound < 0)
            return false;
    }
    for (i = 0; i < e->num_minitracks; i++)
        minitrack_sounds[i] = -1;
    return true;
}

// 0x4350b0  Confidence: exact. The eh vector constructor of sequences[] (cutscene_camera_sequence ctor /
// dtor, 50 x 0x70) is compiler-generated; then the body. i ebp-4. cur_fade_event is not set.
cutscene::cutscene()
{
    int i;

    vis_index = -1;
    data = NULL;
    path_id = -1;
    num_sequences = -1;
    cur_sequence = -1;
    sequence_started = false;
    for (i = 0; i < MAX_CUTSCENE_EFFECTS; i++)
        effect_handles[i] = -1;
    for (i = 0; i < MAX_CUTSCENE_SHOT_EFFECTS; i++)
        fx_handles[i] = -1;
    soundtrack_sound = -1;
    for (i = 0; i < MAX_CUTSCENE_MINITRACKS; i++)
        minitrack_sounds[i] = -1;
    time_offset = -1.0f;
    end_time = -1.0f;
}

// 0x4351c0  Confidence: exact. Only the compiler's eh vector destructor of sequences[].
cutscene::~cutscene()
{
}

// 0x4351e0  Confidence: exact. i ebp-4.
// GAME BUG (live, effect inferred; NOTES.md B4): soundtrack_sound is freed but not reset to -1, and
// load_sounds sets it only for an entry with a $Soundtrack, so a later cutscene without one (the four
// retail dragon summons) streams the freed handle at shot 0 and frees it again at its end.
void cutscene::unload()
{
    int i;

    vis_index = -1;
    data = NULL;
    num_sequences = -1;
    cur_sequence = -1;
    sequence_started = false;
    for (i = 0; i < MAX_CUTSCENE_EFFECTS; i++) {
        if (effect_handles[i] == -1)
            continue;
        fx_destroy(effect_handles[i]);
        effect_handles[i] = -1;
    }
    for (i = 0; i < MAX_CUTSCENE_SHOT_EFFECTS; i++) {
        if (fx_handles[i] == -1)
            continue;
        fx_destroy(fx_handles[i]);
        fx_handles[i] = -1;
    }
    if (soundtrack_sound >= 0) {
        snd_stop(soundtrack_sound);
        snd_free(soundtrack_sound);
    }
    time_offset = -1.0f;
    end_time = -1.0f;
    clear_data_path();
}

// 0x435320  Confidence: exact. p ebp-4, path ebp-0x104 (this ebp-0x108). The clear goes through the
// object's address (mov ecx, 0x601e68), not this.
int cutscene::set_data_path(const char *name)
{
    char *p;
    char path[256];

    if (name == NULL)
        return -1;
    the_cutscene.clear_data_path();
    sprintf(path, "data\\cutscenes\\%s", name);
    p = strstr(path, "_cscript.tbl");
    if (p != NULL)
        *p = 0;
    path_id = file_add_path(path, ".csc .mvf .vfx");
    return path_id;
}

// 0x4353b0  Confidence: exact.
void cutscene::clear_data_path()
{
    if (path_id >= 0) {
        file_remove_path(path_id);
        path_id = -1;
    }
}

// 0x4353e0  Confidence: exact. ok ebp-4. Each failure stores false and jumps to its own stub after the
// final return (0x43548b / 0x43548d / 0x43548f, innermost first), and each stub jumps back to the common
// tail 0x435460, which the success path reaches by falling through: the tail sits inside the innermost
// else and the stubs are gotos after each if / else (the reconstruction of that layout; the original
// spelling is not known). The return values of bind_actors and load_sounds are not used, so a missing
// soundtrack does not fail the load.
bool cutscene::load_data(cutscene_entry *e)
{
    bool ok;

    ok = true;
    if (e == NULL) {
        ok = false;
    } else {
        cutscene_draw_loading();
        if (set_data_path(e->name) < 0) {
            ok = false;
        } else {
            cutscene_draw_loading();
            if (!load_files(e)) {
                ok = false;
            } else {
                cutscene_draw_loading();
                bind_actors(e);
                cutscene_draw_loading();
                load_sounds(e);
                cutscene_draw_loading();
            done:
                if (!ok)
                    the_cutscene.unload();
                else
                    data = e;
                cutscene_loading_finish();
                return ok;
            }
            goto done;
        }
        goto done;
    }
    goto done;
}

// 0x4354a0  Confidence: exact. i ebp-4. The last $Fog key whose shot has been reached.
// GAME BUG (latent, NOTES.md B5): with no key reached yet (no $Fog line, or the first key's shot is later than
// the current shot) i is 0 and fogs[-1] is the num_fogs word read as a pointer: NULL (a crash) for an
// entry without $Fog. cutscene_events_frame calls this every cutscene frame.
void cutscene::get_fog_distances(float *near_out, float *far_out)
{
    int i;

    for (i = 0; i < data->num_fogs; i++) {
        if (cur_sequence < data->fogs[i]->shot)
            break;
    }
    *near_out = data->fogs[i - 1]->near_dist;
    *far_out = data->fogs[i - 1]->far_dist;
}

// 0x435520  Confidence: exact. nexti ebp-4, j ebp-8 (unsigned: jae), vs_cur ebp-0xc. The next $VisSet once
// its shot is the current one.
void cutscene::apply_vis_overrides()
{
    unsigned int j;
    int nexti;
    cutscene_visset *vs_cur;

    if (data->num_vissets == 0)
        return;
    nexti = vis_index + 1;
    if (nexti >= data->num_vissets)
        return;
    if (cur_sequence == data->vissets[nexti]->shot) {
        level_vis_force_reset();
        vs_cur = data->vissets[nexti];
        for (j = 0; j < vs_cur->count; j++)
            level_vis_force_add(vs_cur->names[j]);
        vis_index = nexti;
    }
}

// 0x4355e0  Confidence: likely (the two short temporaries of the length expression: the record's length is
// written through ebp-0x292 / ebp-0x294). No bound is checked on any list except $VisSet, and the
// $Camera / +Minitrack counts are not reset per entry (the constructor zeroes them once).
// `length` is a short read with "%d" (sscanf writes four bytes into its four-byte slot; harmless).
void cutscene_parse_table()
{
    TextFile tf;                      // ebp-0x5c
    char buf[0x100];                  // ebp-0x15c
    cutscene_entry *e;                // ebp-0x160
    char line[0x100];                 // ebp-0x260
    char *character;                  // ebp-0x264
    int n;                            // ebp-0x268
    int shot;                         // ebp-0x26c
    short length;                     // ebp-0x270
    char *effect;                     // ebp-0x274
    char *track_name;                 // ebp-0x278
    int track_shot;                   // ebp-0x27c
    cutscene_fog *fog;                // ebp-0x280
    int fade_shot;                    // ebp-0x284
    int fade_out;                     // ebp-0x288
    float fade_length;                // ebp-0x28c
    float fade_time;                  // ebp-0x290

    cutscene_alloc_pool.Init(cutscene_pool_buffer, CUTSCENE_POOL_SIZE);
    cutscene_num_entries = 0;
    tf.Open("cutscene.tbl", path_tables);
    tf.Expect("#Cutscene Table");
    while (tf.Accept("$Cutscene:")) {
        e = &cutscene_entries[cutscene_num_entries];
        tf.ParseString(buf, 0x100, '"', '"');
        e->name = (char *)cutscene_alloc_pool.Alloc(strlen(buf) + 1);
        strcpy(e->name, buf);
        e->flags = 0;
        while (1) {
            if (tf.Accept("+NoAbort"))
                e->flags |= CUTSCENE_FLAG_NOABORT;
            else if (tf.Accept("+HideUnscripted"))
                e->flags |= CUTSCENE_FLAG_HIDEUNSCRIPTED;
            else
                break;
        }
        tf.Expect("$Level:");
        tf.ParseString(buf, 0x100, '"', '"');
        e->level = (char *)cutscene_alloc_pool.Alloc(strlen(buf) + 1);
        strcpy(e->level, buf);
        if (tf.Accept("$Script:")) {
            tf.ParseString(buf, 0x100, '"', '"');
            e->script = (char *)cutscene_alloc_pool.Alloc(strlen(buf) + 1);
            strcpy(e->script, buf);
        } else {
            e->script = e->level;
        }
        e->num_cameras = 0;
        while (tf.Accept("$Camera:")) {
            tf.ParseString(buf, 0x100, '"', '"');
            e->cameras[e->num_cameras] = (char *)cutscene_alloc_pool.Alloc(strlen(buf) + 1);
            strcpy(e->cameras[e->num_cameras], buf);
            e->num_cameras++;
        }
        while (1) {
            if (tf.Accept("$Animation:")) {
                character = NULL;
            } else if (tf.Accept("$Character Animation:")) {
                tf.ParseString(buf, 0x100, '"', '"');
                character = (char *)cutscene_alloc_pool.Alloc(strlen(buf) + 1);
                strcpy(character, buf);
            } else {
                break;
            }
            tf.ParseString(buf, 0x100, '"', '"');
            effect = (char *)cutscene_alloc_pool.Alloc(strlen(buf) + 1);
            strcpy(effect, buf);
            tf.ReadLine(line, 0x100);
            n = sscanf(line, "%d %d", &shot, &length);
            if (n > 0) {
                e->shot_effects[e->num_shot_effects] = (cutscene_shot_effect *)cutscene_alloc_pool.Alloc(0x10);
                e->shot_effects[e->num_shot_effects]->name = effect;
                e->shot_effects[e->num_shot_effects]->shot = shot;
                e->shot_effects[e->num_shot_effects]->character = character;
                e->shot_effects[e->num_shot_effects]->length = n > 1 ? (length > 1 ? length : 1) : 0;
                e->num_shot_effects++;
            } else {
                e->effects[e->num_effects] = (cutscene_effect *)cutscene_alloc_pool.Alloc(8);
                e->effects[e->num_effects]->name = effect;
                e->effects[e->num_effects]->character = character;
                e->num_effects++;
            }
        }
        if (tf.Accept("$Soundtrack:")) {
            tf.ParseString(buf, 0x100, '"', '"');
            e->soundtrack = (char *)cutscene_alloc_pool.Alloc(strlen(buf) + 1);
            strcpy(e->soundtrack, buf);
        }
        while (tf.Accept("+Minitrack:")) {
            tf.ParseString(buf, 0x100, '"', '"');
            track_name = (char *)cutscene_alloc_pool.Alloc(strlen(buf) + 1);
            strcpy(track_name, buf);
            track_shot = tf.ParseInt();
            e->minitracks[e->num_minitracks] = (cutscene_minitrack *)cutscene_alloc_pool.Alloc(8);
            e->minitracks[e->num_minitracks]->name = track_name;
            e->minitracks[e->num_minitracks]->shot = track_shot;
            e->num_minitracks++;
        }
        e->num_fogs = 0;
        while (tf.Accept("$Fog:")) {
            fog = (cutscene_fog *)cutscene_alloc_pool.Alloc(0xc);
            e->fogs[e->num_fogs] = fog;
            fog->near_dist = tf.ParseFloat();
            fog->far_dist = tf.ParseFloat();
            fog->shot = tf.ParseInt();
            e->num_fogs++;
        }
        e->num_vissets = 0;
        while (1) {
            if (cutscene_parse_visset(e, &tf))
                break;
        }
        e->num_fades = 0;
        while (1) {
            if (tf.Accept("$Fade Out:"))
                fade_out = 1;
            else if (tf.Accept("$Fade In:"))
                fade_out = 0;
            else
                break;
            fade_time = tf.ParseFloat();
            fade_length = tf.ParseFloat();
            fade_shot = tf.ParseInt();
            e->fades[e->num_fades] = (cutscene_fade *)cutscene_alloc_pool.Alloc(0x10);
            e->fades[e->num_fades]->fade_out = fade_out;
            e->fades[e->num_fades]->time = fade_time;
            e->fades[e->num_fades]->length = fade_length;
            e->fades[e->num_fades]->shot = fade_shot;
            e->num_fades++;
        }
        cutscene_num_entries++;
    }
    tf.Expect("#End");
    tf.Close();
}

// 0x435ff0  Confidence: exact. idx ebp-4, ix ebp-8, vs ebp-0xc, buf ebp-0x10c, s ebp-0x110. 0 after one
// "$VisSet: shot "$vis.." .." line, -1 when there is none (the normal end of the list) or it is bad
// (17th set, shots not growing, more than 16 names, a name not starting "$vis", pool full).
int cutscene_parse_visset(cutscene_entry *e, TextFile *tf)
{
    int idx;
    char buf[0x100];
    cutscene_visset *vs;
    int ix;
    char *s;

    idx = e->num_vissets;
    if (idx >= MAX_CUTSCENE_VISSETS)
        return -1;
    if (!tf->Accept("$VisSet:"))
        return -1;
    vs = (cutscene_visset *)cutscene_alloc_pool.Alloc(0x44);
    for (ix = 0; ix < 16; ix++)
        vs->names[ix] = NULL;
    e->vissets[idx] = vs;
    vs->shot = tf->ParseInt();
    if (idx > 0 && e->vissets[idx - 1]->shot >= vs->shot)
        return -1;
    vs->count = 0;
    while (tf->Peek("\"")) {
        if (vs->count >= 16)
            return -1;
        tf->ParseString(buf, 0x100, '"', '"');
        if (_strnicmp(buf, "$vis", 4) != 0)
            return -1;
        s = (char *)cutscene_alloc_pool.Alloc(strlen(buf) + 1);
        if (s == NULL)
            return -1;
        strcpy(s, buf);
        vs->names[vs->count] = s;
        vs->count++;
    }
    e->num_vissets++;
    return 0;
}

// 0x4361d0  Confidence: exact. i ebp-4, ch ebp-8. The first entry with the name on this level or on "any",
// or ANY entry whose level is "Placeholder" whatever its name (none in retail).
// GAME BUG (latent; docs/sum-exe/cutscenes.md): a failed cutscene_load_data returns here with the input
// locked, the summon stashed and the music stopped, and cutscene_saved_actors (175 slots) has no bound.
void cutscene_play(const char *name)
{
    int i;
    Character *ch;

    for (i = 0; i < cutscene_num_entries; i++) {
        if ((_stricmp(cutscene_entries[i].name, name) == 0 && _stricmp(cutscene_entries[i].level, level_filename) == 0)
            || (_stricmp(cutscene_entries[i].name, name) == 0 && _stricmp(cutscene_entries[i].level, "any") == 0)
            || _stricmp(cutscene_entries[i].level, "Placeholder") == 0) {
            if (game_flags & GF_CUTSCENE)
                cutscene_cleanup();
            else
                cutscene_begin_loading(6);
            summon_stash_for_cutscene();
            fx_destroy_spell_effects();
            fx_pause_for_cutscene();
            cutscene_start_special_cases(name);
            ui_input_lock();
            cutscene_saved_fog_far = fog_get_far_current();      // the names are swapped (D3204)
            cutscene_saved_fog_near = fog_get_near_current();
            cutscene_saved_fog_multiplier = Gr_level_fog_multiplier;
            Gr_level_fog_multiplier = 1.0f;
            level_ambient_pause_all();
            cutscene_saved_music = music_get_current();
            music_stop(200);
            snd_stop_all();
            if (!the_cutscene.load_data(&cutscene_entries[i]))
                return;
            game_camera.set_sequence_transform(NULL, NULL);
            if (cutscene_data->num_vissets != 0)
                level_vis_force_begin();
            cutscene_script_load(name);
            cutscene_num_saved_actors = 0;
            for (ch = living_list.live_next; ch != &living_list; ch = ch->live_next) {
                ch->ai.goals_mark_keep();
                ch->clear_all_effects();
                if (ch->cutscene_object >= 0) {
                    ch->prop_tag = ch->default_prop_tag;
                    if (ch->anim_stand != ch->anim_ready)
                        ch->flags &= ~CF_COMBAT_READY;
                }
                if ((ch->flags & 0x400) || ch->ai.is_mode(9, -1))
                    ch->ai.go_idle();
                ch->flags &= ~0x400;
                cutscene_saved_actors[cutscene_num_saved_actors].name_hash = name_hash_nocase(ch->get_name());
                cutscene_saved_actors[cutscene_num_saved_actors].summonlike = (ch->flags & CF_SUMMONLIKE) != 0;
                cutscene_saved_actors[cutscene_num_saved_actors].flag_20 = (ch->flags & 0x20) != 0;
                cutscene_saved_actors[cutscene_num_saved_actors].alt_texture = (ch->flags & 0x2000000) != 0;
                cutscene_saved_actors[cutscene_num_saved_actors].active = (ch->obj_flags & 1) != 0;
                cutscene_num_saved_actors++;
            }
            hover_tint_set_object(NULL, 0, 0, 0);
            hover_tint_set_character(NULL, 0, 0, 0);
            gr_set_vsync(true);
            frame_show_loading_icon = true;
            game_flags |= GF_CUTSCENE;
            game_flags |= 0x8000;
            if (_stricmp(cutscene_data->name, "Placeholder") == 0)
                cutscene_playing = true;
            return;
        }
    }
}

// 0x4365e0  Confidence: exact.
void cutscene_start_special_cases(const char *name)
{
    if (_stricmp(name, "serpent-dismount") == 0)
        serpent_set_cutscene_walk_anim();
    else if (_stricmp(name, "Nar-LeneleAct4") == 0)
        Snow_enabled = false;
}

// 0x436620  Confidence: exact. i ebp-4, ch ebp-8, hash ebp-0xc, next ebp-0x10, cur ebp-0x14. The first saved
// record with the character's name hash wins.
void cutscene_cleanup()
{
    int i;
    Character *ch;
    effect_object *cur;
    effect_object *next;
    int hash;

    if (!(game_flags & GF_CUTSCENE) || cutscene_data == NULL)
        return;
    frame_show_loading_icon = false;
    cutscene_active_count--;
    cutscene_frametime = 0;
    snd_level_release();
    if (cutscene_stream_sound >= 0) {
        snd_stop_fade(cutscene_stream_sound, 200);
        cutscene_stream_sound = -1;
    }
    game_flags &= ~GF_CUTSCENE;
    if (cutscene_saved_music >= 0) {
        music_play(cutscene_saved_music, true);
        cutscene_saved_music = -1;
    }
    game_flags |= 0x8000;
    cutscene_playing = false;
    gr_set_vsync(false);
    cutscene_end_special_cases(cutscene_data->name);
    cutscene_script_unload();
    for (ch = living_list.live_next; ch != &living_list; ch = ch->live_next) {
        hash = name_hash_nocase(ch->get_name());
        ch->ai.goals_mark_keep();
        ch->set_speed_scale(ch->run_speed, false);
        if (ch->flags & 0x10) {
            ch->set_ai_active(1);
            ch->flags &= ~0x10;
        }
        for (i = 0; i < cutscene_num_saved_actors; i++) {
            if (cutscene_saved_actors[i].name_hash == hash) {
                if (ch->type != 7 && cutscene_saved_actors[i].summonlike)
                    ch->flags |= CF_SUMMONLIKE;
                else
                    ch->flags &= ~CF_SUMMONLIKE;
                if (cutscene_saved_actors[i].flag_20)
                    ch->flags |= 0x20;
                else
                    ch->flags &= ~0x20;
                if (ch->info->species == 0) {
                    if (cutscene_saved_actors[i].alt_texture)
                        ch->set_armour_alt_texture(1, 0);
                    else
                        ch->set_armour_alt_texture(0, 0);
                }
                if (cutscene_saved_actors[i].active)
                    ch->obj_flags |= 1;
                else
                    ch->obj_flags &= ~1;
                break;
            }
        }
        ch->refresh_item_status_effects();
    }
    level_ambient_unpause_all();
    game_camera.end_sequence(true);
    fog_set_distances(cutscene_saved_fog_near, cutscene_saved_fog_far);   // (near, far): names swapped (D3204)
    Gr_level_fog_multiplier = cutscene_saved_fog_multiplier;
    if (cutscene_data->num_vissets != 0)
        level_vis_force_end();
    if (cur_summon != NULL && (cur_summon->flags & 0x800)) {
        for (cur = fx_active_list.fx_next; next = cur->fx_next, cur != &fx_active_list; cur = next) {
            if (cur->owner == cur_summon->handle)
                fx_destroy(cur->handle);
        }
    }
    printf("killing off %d effects\n", cutscene_data->num_shot_effects);
    for (i = 0; i < cutscene_data->num_shot_effects; i++) {
        if (cutscene_fx_handles[i] == -1)
            continue;
        printf("killing effect %d\n", i);
        fx_destroy(cutscene_fx_handles[i]);
        cutscene_fx_handles[i] = -1;
    }
    the_cutscene.unload();
    effects_release_cutscene_memory();
    frametime_restore_limits();
    if (!(game_flags & 8))
        ui_input_unlock();
    fx_resume_after_cutscene();
    summon_restore_after_cutscene();
    gr_fade_in(0);
    levelrender_fade_reset();
    if (cur_character->flags & CF_COMBAT_READY)
        combat_mode_set(true);
    else
        combat_mode_set(false);
}

// 0x436a60  Confidence: exact.
void cutscene_end_special_cases(const char *name)
{
    if (_stricmp(name, "Nar-LeneleAct4") == 0)
        Snow_enabled = true;
}

// 0x436a90  Confidence: exact (the `fst / fmul 1.0 / fst` chain is written as nested assignments to t; the
// 1.0 is perhaps a time-scale macro). rem ebp-4, t ebp-8; ebp-0x10 / -0xc and -0x18 / -0x14 are the
// compiler's 64-bit conversion temporaries of the two unsigned operands (G3, see the file comment).
void cutscene_sequence_update()
{
    float rem;
    float t;

    if (!(game_flags & GF_CUTSCENE) || cutscene_data == NULL || game_is_paused())
        return;
    cutscene_sequence_started = false;
    if (cutscene_cur_sequence < 0) {
        rem = 0;
    } else {
        // G3: FISUB dword of the unsigned start time (signed 32-bit load).
        cutscene_frametime = (t = (t = ((float)timeGetTime() - sequence_start_time) / 1000.0f) * 1.0f)
                             - cutscene_last_time;
        cutscene_last_time = t;
        rem = cutscene_sequences[cutscene_cur_sequence].end - cutscene_time_offset - t;
    }
    if (rem <= 0.0f) {
        cutscene_cur_sequence++;
        if (cutscene_cur_sequence >= cutscene_num_sequences) {
            cutscene_cleanup();
            if (game_flags & 8)
                gameseq_set_state(1);
        } else {
            if (cutscene_cur_sequence == 0) {
                sequence_start_time = timeGetTime();
                cutscene_stream_sound = snd_play_streamed(the_cutscene.soundtrack_sound, 1.0f, 0);
                cutscene_last_time = 0;
            }
            cutscene_sequence_started = true;
            game_camera.start_sequence(&cutscene_sequences[cutscene_cur_sequence]);
        }
    }
}

// 0x436c00  Confidence: exact (the fade time as in cutscene_sequence_update; the switch has case 1 before
// case 0: the case bodies follow the source order, the compare chain is sorted). Locals: fog_far ebp-4,
// ix ebp-8, fog_near ebp-0xc, c ebp-0x10, eff ebp-0x14, po ebp-0x18, fade ebp-0x1c, sequence ebp-0x20, now ebp-0x24
// (then the strcmp temporaries, the 64-bit temporaries and the switch value ebp-0x4c). The shot range is
// tested twice in a row (in the binary).
void cutscene_events_frame()
{
    float fog_far;
    int ix;
    Character *c;
    float fog_near;
    obj *po;
    cutscene_shot_effect *eff;
    cutscene_fade *fade;
    cutscene_camera_sequence *sequence;
    float now;

    if (!(game_flags & GF_CUTSCENE) || cutscene_data == NULL || game_is_paused())
        return;
    if (cutscene_cur_sequence < 0 || cutscene_cur_sequence >= cutscene_num_sequences)
        return;
    if (cutscene_cur_sequence < 0 || cutscene_cur_sequence >= cutscene_num_sequences)
        return;
    frame_show_loading_icon = false;
    if (cutscene_sequence_started) {
        if (cutscene_cur_sequence == 0) {
            if (cutscene_data->num_fades > 0 && cutscene_data->fades[0]->fade_out == 0)
                gr_fade_out(0);
            else
                gr_fade_in(0);
        }
        for (ix = 0; ix < cutscene_data->num_shot_effects; ix++) {
            if (cutscene_data->shot_effects[ix]->shot == cutscene_cur_sequence) {
                if (cutscene_data->shot_effects[ix]->character != NULL) {
                    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                        if (strcmp(c->get_name(), cutscene_data->shot_effects[ix]->character) == 0)
                            break;
                    }
                    if (c != &living_list)
                        cutscene_fx_handles[ix] = fx_attach_to_character(cutscene_data->shot_effects[ix]->name,
                                                                        c->handle, -1, -1, 0, 0);
                    else
                        cutscene_fx_handles[ix] = fx_create_world(cutscene_data->shot_effects[ix]->name, true);
                } else if (stristr(cutscene_data->shot_effects[ix]->name, "camera")) {
                    cutscene_fx_handles[ix] = fx_create_at_camera(cutscene_data->shot_effects[ix]->name, true);
                } else {
                    cutscene_fx_handles[ix] = fx_create_world(cutscene_data->shot_effects[ix]->name, true);
                }
            } else if (cutscene_data->shot_effects[ix]->shot > cutscene_cur_sequence) {
                break;
            }
        }
        for (ix = 0; ix < cutscene_data->num_shot_effects; ix++) {
            if (cutscene_fx_handles[ix] == -1)
                continue;
            eff = cutscene_data->shot_effects[ix];
            if (eff->length > 0 && cutscene_cur_sequence >= eff->shot + eff->length) {
                po = obj_from_handle_b(cutscene_fx_handles[ix]);
                if (po != NULL) {
                    printf("Stopping render of effect\n");
                    po->obj_flags &= ~1;
                }
            }
        }
        for (ix = 0; ix < cutscene_data->num_minitracks; ix++) {
            if (cutscene_data->minitracks[ix]->shot == cutscene_cur_sequence)
                snd_play(the_cutscene.minitrack_sounds[ix], 1.0f, 0, 0, 0);
            else if (cutscene_data->minitracks[ix]->shot > cutscene_cur_sequence)
                break;
        }
    }
    if (cutscene_cur_fade_event < cutscene_data->num_fades) {
        fade = cutscene_data->fades[cutscene_cur_fade_event];
        if (cutscene_cur_sequence >= fade->shot) {
            sequence = &cutscene_sequences[cutscene_cur_sequence];
            // G3: FISUB dword of the unsigned start time (signed 32-bit load).
            now = (now = ((float)timeGetTime() - sequence_start_time) / 1000.0f) * 1.0f;
            if (cutscene_time_offset + now - sequence->start >= fade->time) {
                switch (fade->fade_out) {
                case 1:
                    gr_fade_out((int)(fade->length * 1000.0f));
                    break;
                case 0:
                    gr_fade_in((int)(fade->length * 1000.0f));
                    break;
                }
                cutscene_cur_fade_event++;
            }
        }
    }
    the_cutscene.get_fog_distances(&fog_near, &fog_far);
    fog_set_distances(fog_near, fog_far);
    the_cutscene.apply_vis_overrides();
    cutscene_script_entries_frame();
}

// 0x437120  Confidence: exact. Empty (game_cleanup).
void cutscene_shutdown_stub()
{
}

// 0x437130  Confidence: exact (the block locals take ebp-4 .. ebp-0x20 in declaration order). The pending
// console cutscene: the three escape scenes first set the boss stage to 3 and take the rider off its
// mount. The tiger loop keeps the LAST rider found, the phoenix loop the first.
void cutscene_play_escape()
{
    game_flags &= ~GF_ESCAPE_CUTSCENE;
    if (cutscene_pending_name == NULL)
        return;
    ui_input_lock();
    if (_stricmp(cutscene_pending_name, "Tiger-Escape") == 0) {
        tiger_target_stage = 3;
        Character *rider = NULL;      // ebp-4
        Character *ch;                 // ebp-8
        for (ch = living_list.live_next; ch != &living_list; ch = ch->live_next) {
            if (ch->info->species == SPECIES_TIGER_RIDER)
                rider = ch;
        }
        if (rider != NULL) {
            Character *mount = rider->get_mount();   // ebp-0xc
            if (mount != NULL) {
                mount->mount_flags = 0;
                mount->mount_link = -1;
                mount->ai.target = -1;
                mount->flags &= ~0x20;
                rider->mount_flags = 0;
                rider->mount_link = -1;
                rider->flags |= CF_COMBAT_READY;
            }
        }
    } else if (_stricmp(cutscene_pending_name, "Serpent-Escape") == 0) {
        serpent_target_stage = 3;
        Character *rider = living_from_handle(serpent_handle);   // ebp-0x10
        if (rider != NULL) {
            Character *mount = rider->get_mount();   // ebp-0x14
            if (mount != NULL) {
                mount->mount_flags = 0;
                mount->mount_link = -1;
                mount->ai.target = -1;
                mount->flags &= ~0x20;
                rider->mount_flags = 0;
                rider->mount_link = -1;
                rider->flags |= CF_COMBAT_READY;
            }
        }
    } else if (_stricmp(cutscene_pending_name, "PHNX-Escape") == 0) {
        phoenix_target_stage = 3;
        Character *rider = NULL;      // ebp-0x18
        Character *ch;                 // ebp-0x1c
        for (ch = living_list.live_next; ch != &living_list; ch = ch->live_next) {
            if (ch->info->species == SPECIES_PHOENIX_RIDER) {
                rider = ch;
                break;
            }
        }
        if (rider != NULL) {
            Character *mount = rider->get_mount();   // ebp-0x20
            if (mount != NULL) {
                mount->mount_flags = 0;
                mount->mount_link = -1;
                mount->ai.target = -1;
                mount->flags &= ~0x20;
                rider->mount_flags = 0;
                rider->mount_link = -1;
                rider->flags |= CF_COMBAT_READY;
            }
        }
    }
    cutscene_play(cutscene_pending_name);
    cutscene_pending_name = NULL;
}

// 0x437400  Confidence: exact. i ebp-4.
void cutscene_change_level_for(const char *name)
{
    int i;

    for (i = 0; i < cutscene_num_entries; i++) {
        if (_stricmp(name, cutscene_entries[i].name) == 0) {
            game_change_level(cutscene_entries[i].level, cutscene_entries[i].script);
            game_flags |= GF_ESCAPE_CUTSCENE | 8;
            cutscene_pending_name = cutscene_entries[i].name;
            return;
        }
    }
}

// Compiler-generated:
// 0x4374a0 init_ccmd_play_cutscene_thunk (ebp frame, call; static_initializers entry 114). Confidence: exact.
// 0x4374b0 init_ccmd_play_cutscene (console_command constructor). Confidence: exact.
console_command ccmd_play_cutscene("play_cutscene", "Play a cutscene", cmd_play_cutscene);   // 0x6034b8

// 0x4374d0  Confidence: exact. f (ebp-0x120) is constructed and destroyed and never used.
void cmd_play_cutscene()
{
    if (calling_a_command_function) {
        File f;

        process_command(ARG_STRING);
        cutscene_play(next_arg);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: play_cutscene <cutscene name>", 0);
}

// Compiler-generated:
// 0x437560 init_ccmd_play_and_load_cutscene_thunk (entry 115). Confidence: exact.
// 0x437570 init_ccmd_play_and_load_cutscene. Confidence: exact.
console_command ccmd_play_and_load_cutscene("play_and_load_cutscene",
                                            "Play a cutscene and loads the level associated with it",
                                            cmd_play_and_load_cutscene);   // 0x609f10

// 0x437590  Confidence: exact. The help text is play_cutscene's (the same literal 0x593b90).
void cmd_play_and_load_cutscene()
{
    if (calling_a_command_function) {
        File f;

        process_command(ARG_STRING);
        cutscene_change_level_for(next_arg);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: play_cutscene <cutscene name>", 0);
}

// Compiler-generated:
// 0x437620 init_ccmd_stop_cutscene_thunk (entry 116). Confidence: exact.
// 0x437630 init_ccmd_stop_cutscene. Confidence: exact.
console_command ccmd_stop_cutscene("stop_cutscene", "Stop the current cutscene", cmd_stop_cutscene);   // 0x609f20

// 0x437650  Confidence: exact.
void cmd_stop_cutscene()
{
    cutscene_cleanup();
}

#ifdef _MSC_VER
#pragma optimize("", on)
#endif
