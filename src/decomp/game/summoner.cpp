// summoner.cpp - Sum.exe v1.40, code 0x43cd70-0x43e654 (the game's main module: static data, console
// commands, MainLoop, LoadSettings, GameInit, cleanup, summoner.cfg, the updater launch, WinMain).
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966),
// /O2 /Ob1. Confidence per function: exact = expect identical code, likely = same semantics, small
// layout risk, unsure = the source shape is a guess.
//
// Unit range: sum.symbols tags 0x43cd70 (init_level_display_names_thunk) to 0x43e4a0 (launch_updater).
// WinMain (0x43e5e0, module "-") is written here too: its only literal "Summoner Main Thread" (0x594f14)
// is the last string of this unit's .data, right before summoner_cd.cpp's "SUMMONER_2" (0x594f2c), and
// its SEH scope table (0x580e50) follows resolution_table in .rdata. The previous unit (gametarget.cpp)
// ends with hud_set_timed_highlight (0x43cd50); summoner_cd.cpp starts at cd_find_drive (0x43e660).
//
// Data: .rdata 0x580e38-0x580e4f (resolution_table); .data 0x594440-0x594f2b (level_display_names,
// then the literals); .bss 0x60abb0-0x60ad7b (path ids, fonts, command-line objects, console
// commands, Summoner_options, flags, the console sound). The .bss addresses are not in definition order
// (the same open question as frametime.cpp / player.cpp); the definitions below follow the
// initialiser order where there is one and are otherwise grouped by use.
//
// Literals pooled with earlier units (/Gf): "" (0x5bd0d0), "masad" (0x5930c0), "catacombs" (0x593194),
// "Eleh" (0x592d08), "Liangshan" (0x592cf4), ".plr" (0x58a0e0), ".sav" (0x58bc98).

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "vsdk/os/os.h"
#include "vsdk/os/console.h"
#include "vsdk/os/cmdline.h"
#include "vsdk/os/registry.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/timer.h"
#include "vsdk/os/key.h"
#include "vsdk/os/mouse.h"
#include "vsdk/os/exceptionhandler.h"
#include "vsdk/vfile/file.h"
#include "vsdk/localize/localize.h"
#include "vsdk/net/net.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/gr/gr.h"
#include "vsdk/snd/snd.h"
#include "vsdk/ca/character_instance.h"
#include "vsdk/geom/effect_mem.h"
#include "vsdk/ui/ui.h"
#include "engine/effects/effect_stream.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/spells/spells.h"
#include "engine/ai/ai.h"
#include "engine/gamesound/gamesound.h"
#include "engine/rendering/levelrender.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/conversation.h"
#include "levelscripts/savegame.h"
#include "summoner/summoner.h"
#include "summoner/automap.h"
#include "summoner/player/player.h"
#include "summoner/multi/multi.h"
#include "summoner/multi/multisave.h"     // pxo_cfg_flags
#include "summoner/interface/interface.h"
#include "summoner/interface/new_pxochat.h"   // pxo_run_updater_on_exit
#include "experimental/experimental.h"   // EXPERIMENTAL_BUILD only (the renderer's window mode)

void console_sound_stop();            // 0x43d710 (defined after its first caller)

// ---- .rdata ------------------------------------------------------------------------------------------
// 0x580e38 {w, h} per resolution_index. Unsigned: LoadSettings halves the entries with SHR for
// mouse_move_to (0x43db6f, D606).
#ifdef FIXED_BUILD
uint resolution_table[3][2] = {     // part of FIXED_BUILD CustomResolution: LoadSettings writes slot 2
#else
const uint resolution_table[3][2] = {
#endif
    { 640, 480 },
    { 800, 600 },
    { 1024, 768 },
};

// ---- .data -------------------------------------------------------------------------------------------
// 0x594440 {level file name, display name slot}, 51 entries (D603). The slot addresses are not constant to
// the compiler, so only entry 0's name is in the image ("test", 0x5945d8) and the initialiser writes the
// other 101 dwords (D602).
// 0x43cd70 init_level_display_names_thunk (jmp, static_initializers entry 132). Confidence: exact.
// 0x43cd80 init_level_display_names. Confidence: likely (how XSTR_SLOT was spelled).
level_display_name level_display_names[51] = {
    { "test", XSTR_SLOT("test level", 571) },
    { "masad", XSTR_SLOT("Masad Village", 572) },
    { "lenele1d", XSTR_SLOT("Lenele Outskirts", 573) },
    { "lenele1aa", XSTR_SLOT("Lenele Marketplace", 574) },
    { "lenele1ab", XSTR_SLOT("Lenele Crown District", 575) },
    { "lenele1b", XSTR_SLOT("Lenele Old City", 576) },
    { "lenele1c", XSTR_SLOT("Lenele Temple Plaza", 577) },
    { "TempleInt", XSTR_SLOT("The Temple of Urath", 578) },
    { "sewer", XSTR_SLOT("Lenele Sewers", 579) },
    { "sewerboss", XSTR_SLOT("Lenele Palace Aqueduct", 580) },
    { "LPalaceInt", XSTR_SLOT("Lenele Palace", 581) },
    { "lenele1e", XSTR_SLOT("Lenele Palace Plaza", 582) },
    { "Tancredhouse", XSTR_SLOT("Tancred's Haven", 583) },
    { "IonaExT", XSTR_SLOT("Iona Monastery", 584) },
    { "catacombs", XSTR_SLOT("Iona Catacombs", 585) },
    { "IkaemosExt", XSTR_SLOT("Ikaemos Swamp", 586) },
    { "IkaemosTopInt", XSTR_SLOT("Ikaemos Palace Upper Level", 587) },
    { "IkaemosBottomInt", XSTR_SLOT("Ikaemos Palace Lower Chambers", 588) },
    { "KhosaniStrng", XSTR_SLOT("City of the Khosani", 589) },
    { "KhosaniLab", XSTR_SLOT("Labyrinth of the Khosani", 590) },
    { "lenele2d", XSTR_SLOT("Lenele Outskirts#Act2", 591) },
    { "lenele2aa", XSTR_SLOT("Lenele Marketplace#Act 2", 592) },
    { "lenele2ab", XSTR_SLOT("Lenele Crown District#Act 2", 593) },
    { "Eleh", XSTR_SLOT("Tower of Eleh", 594) },
    { "wolong", XSTR_SLOT("Wolong#Night", 595) },
    { "WolongCaverns", XSTR_SLOT("Caverns of Wolong", 596) },
    { "wolong2", XSTR_SLOT("Wolong#Day", 597) },
    { "Liangshan", XSTR_SLOT("Liangshan Forest", 598) },
    { "Jadetemple", XSTR_SLOT("The Jade Temple", 599) },
    { "IonaExt02", XSTR_SLOT("Iona Monastery#Act 4", 600) },
    { "lenele3d", XSTR_SLOT("Lenele Outskirts#Act 4", 601) },
    { "lenele3a", XSTR_SLOT("Lenele City#Act 4", 602) },
    { "LPalaceInt02", XSTR_SLOT("Lenele Palace#Act 4", 603) },
    { "KhosaniLab2", XSTR_SLOT("Labyrinth of the Khosani#Act4", 604) },
    { "IkaemosExt2", XSTR_SLOT("Ikaemos Swamp#Act 4", 605) },
    { "IkaemosBottomInt2", XSTR_SLOT("Ikaemos Palace#Act4", 606) },
    { "TempleInt2", XSTR_SLOT("The Temple of Urath#Act 4", 607) },
    { "Rand-Desert", XSTR_SLOT("Random Enc. -- Desert", 608) },
    { "Rand-DesertNite01", XSTR_SLOT("Random Enc. -- Desert Night", 609) },
    { "Rand-Forest01", XSTR_SLOT("Random Enc. -- Forest Day", 610) },
    { "Rand-ForestNite1", XSTR_SLOT("Random Enc. -- Forest Night", 611) },
    { "Rand-Hills01", XSTR_SLOT("Random Enc. -- Hills Day", 612) },
    { "Rand-HillsNite01", XSTR_SLOT("Random Enc. -- Hills Night", 613) },
    { "Rand-Iceland01", XSTR_SLOT("Random Enc. -- Ice Day", 614) },
    { "Rand-IcelandNite01", XSTR_SLOT("Random Enc. -- Ice Night", 615) },
    { "Rand-Grassland01", XSTR_SLOT("Random Enc. -- Grassland", 616) },
    { "Rand-GrasslandNite01", XSTR_SLOT("Random Enc. -- Grassland Night", 617) },
    { "Rand-Orenia01", XSTR_SLOT("Random Enc. -- Orenia Day", 618) },
    { "Rand-OreniaNite01", XSTR_SLOT("Random Enc. -- Orenia Night", 619) },
    { "worldmap1", XSTR_SLOT("World Map", 620) },
    { "test-spells", XSTR_SLOT("Spell test level", 621) },
};

// ---- .bss --------------------------------------------------------------------------------------------
// 0x43d180 init_game_config_defaults_thunk (jmp, entry 133). Confidence: exact.
// 0x43d190 init_game_config_defaults: the inline constructor (SummonerOptions::set_defaults). Confidence: exact.
SummonerOptions Summoner_options;     // 0x60acc8

// The command-line parameters: each one an initialiser (sinit_ call + jmp, init_ calls the
// constructor) and an atexit destructor (register_dtor_ / dtor_). Help texts are all "" (0x5bd0d0);
// only -port takes an argument. Confidence: exact (all 36 functions).
// 0x43d220 sinit_cmdline_port (call init, jmp register; static_initializers entry 134). Confidence: exact.
// 0x43d230 init_cmdline_port (CmdLineParam constructor). Confidence: exact.
// 0x43d250 register_dtor_cmdline_port (atexit). Confidence: exact.
// 0x43d260 dtor_cmdline_port (CmdLineParam destructor). Confidence: exact.
CmdLineParam cmdline_param_port("-port", "", true);                    // 0x60aca8 0x43d220-0x43d265
// 0x43d270 sinit_cmdline_directsound3d (call init, jmp register; static_initializers entry 135). Confidence: exact.
// 0x43d280 init_cmdline_directsound3d (CmdLineParam constructor). Confidence: exact.
// 0x43d2a0 register_dtor_cmdline_directsound3d (atexit). Confidence: exact.
// 0x43d2b0 dtor_cmdline_directsound3d (CmdLineParam destructor). Confidence: exact.
CmdLineParam cmdline_param_directsound3d("-directsound3d", "", false); // 0x60ac38 0x43d270-0x43d2b5
// 0x43d2c0 sinit_cmdline_eax (call init, jmp register; static_initializers entry 136). Confidence: exact.
// 0x43d2d0 init_cmdline_eax (CmdLineParam constructor). Confidence: exact.
// 0x43d2f0 register_dtor_cmdline_eax (atexit). Confidence: exact.
// 0x43d300 dtor_cmdline_eax (CmdLineParam destructor). Confidence: exact.
CmdLineParam cmdline_param_eax("-eax", "", false);                     // 0x60abf8 0x43d2c0-0x43d305
// 0x43d310 sinit_cmdline_pack (call init, jmp register; static_initializers entry 137). Confidence: exact.
// 0x43d320 init_cmdline_pack (CmdLineParam constructor). Confidence: exact.
// 0x43d340 register_dtor_cmdline_pack (atexit). Confidence: exact.
// 0x43d350 dtor_cmdline_pack (CmdLineParam destructor). Confidence: exact.
CmdLineParam cmdline_param_pack("-pack", "", false);                   // 0x60ac68 0x43d310-0x43d355
// 0x43d360 sinit_cmdline_glide (call init, jmp register; static_initializers entry 138). Confidence: exact.
// 0x43d370 init_cmdline_glide (CmdLineParam constructor). Confidence: exact.
// 0x43d390 register_dtor_cmdline_glide (atexit). Confidence: exact.
// 0x43d3a0 dtor_cmdline_glide (CmdLineParam destructor). Confidence: exact.
CmdLineParam cmdline_param_glide("-glide", "", false);                 // 0x60ad50 0x43d360-0x43d3a5 (never read)
// 0x43d3b0 sinit_cmdline_direct3d (call init, jmp register; static_initializers entry 139). Confidence: exact.
// 0x43d3c0 init_cmdline_direct3d (CmdLineParam constructor). Confidence: exact.
// 0x43d3e0 register_dtor_cmdline_direct3d (atexit). Confidence: exact.
// 0x43d3f0 dtor_cmdline_direct3d (CmdLineParam destructor). Confidence: exact.
CmdLineParam cmdline_param_direct3d("-direct3d", "", false);           // 0x60abb8 0x43d3b0-0x43d3f5
// 0x43d400 sinit_cmdline_800 (call init, jmp register; static_initializers entry 140). Confidence: exact.
// 0x43d410 init_cmdline_800 (CmdLineParam constructor). Confidence: exact.
// 0x43d430 register_dtor_cmdline_800 (atexit). Confidence: exact.
// 0x43d440 dtor_cmdline_800 (CmdLineParam destructor). Confidence: exact.
CmdLineParam cmdline_param_800("-800", "", false);                     // 0x60ad18 0x43d400-0x43d445
// 0x43d450 sinit_cmdline_1024 (call init, jmp register; static_initializers entry 141). Confidence: exact.
// 0x43d460 init_cmdline_1024 (CmdLineParam constructor). Confidence: exact.
// 0x43d480 register_dtor_cmdline_1024 (atexit). Confidence: exact.
// 0x43d490 dtor_cmdline_1024 (CmdLineParam destructor). Confidence: exact.
CmdLineParam cmdline_param_1024("-1024", "", false);                   // 0x60ad30 0x43d450-0x43d495
// 0x43d4a0 sinit_cmdline_dsinit (call init, jmp register; static_initializers entry 142). Confidence: exact.
// 0x43d4b0 init_cmdline_dsinit (CmdLineParam constructor). Confidence: exact.
// 0x43d4d0 register_dtor_cmdline_dsinit (atexit). Confidence: exact.
// 0x43d4e0 dtor_cmdline_dsinit (CmdLineParam destructor). Confidence: exact.
CmdLineParam cmdline_param_dsinit("-dsinit", "", false);               // 0x60ac90 0x43d4a0-0x43d4e5

// Path ids (setup_data_paths). Only the ids someone reads are stored.
int path_root;                        // 0x60ac54
int path_data;                        // 0x60ac60
int path_multicache;                  // 0x60ad48
int path_interface;                   // 0x60ac10
int path_maps_characters;             // 0x60ac2c
int path_maps_effects;                // 0x60abb0
int path_maps_items;                  // 0x60ac58
int path_maps_levels;                 // 0x60ac14
int path_tables;                      // 0x60ac5c
int path_models_levels_copy;          // 0x60ac18
int path_scripts;                     // 0x60ac84
int path_tables_characterinfo;        // 0x60acc0
int path_internal_copy;               // 0x60ac30
int path_players;                     // 0x60ac80
int path_savegame;                    // 0x60ac88
int path_tables_levels;               // 0x60abe4
int path_movies;                      // 0x60ad08

int font_small;                       // 0x60abe0
int font_main;                        // 0x60ad0c
int font_bones_small;                 // 0x60ad10

int resolution_index;                 // 0x60ad68
bool cmdline_directsound3d;           // 0x60ad6c
bool cmdline_flag_60ad6d;             // 0x60ad6d read by LoadSettings, never written
bool cmdline_flag_60ad6e;             // 0x60ad6e read by LoadSettings, never written
bool cmdline_eax;                     // 0x60ad6f
bool cmdline_pack;                    // 0x60ad70 written by cmdline_read_params, never read

int console_sound_handle;             // 0x60ad74
int console_sound_buffer;             // 0x60ad78 (D600)

// ---- console commands -----------------------------------------------------------------------------
// The float command macro. "Usage: %s [float]\nSets %s to the specified value.\n" and "%s is %f\n" are
// this unit's literals (the first float command in link order).
// 0x43d4f0 init_ccmd_gamma_thunk (jmp, entry 143). Confidence: exact.
// 0x43d500 init_ccmd_gamma (console_command constructor). Confidence: exact.
void cmd_gamma();
console_command ccmd_gamma("gamma", "Sets Summoner_options.gamma_correction", cmd_gamma);   // 0x60abe8

// 0x43d520  Only stores the value: the gamma ramp is set again by options_apply_detail / GameInit.
void cmd_gamma()
{
    if (calling_a_command_function) {
        process_command(ARG_FLOAT);
        Summoner_options.gamma_correction = command_argument_float;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [float]\nSets %s to the specified value.\n", "gamma",
                "Summoner_options.gamma_correction");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %f\n", "gamma", Summoner_options.gamma_correction);
        print_to_console(string_buffer, 0);
    }
}

// 0x43d5b0 init_ccmd_play_sound_thunk (jmp). Confidence: exact.
// 0x43d5c0 init_ccmd_play_sound (console_command constructor). Confidence: exact.
void cmd_play_sound();
console_command ccmd_play_sound("play_sound", "Streams a wav file from disk", cmd_play_sound);   // 0x60ac20

// 0x43d5e0  Confidence: exact (stack order of name / File / msg: 0x40-byte name at esp+0, the File at
// +0x40, the message at +0x154). The File is built before process_command (EH state 0 at 0x43d613).
void cmd_play_sound()
{
    if (calling_a_command_function) {
        char name[64];
        File f;

        process_command(ARG_STRING);
        file_change_ext(name, next_arg, ".wav");
        if (!f.find(name, PATH_ANY)) {
            char msg[256];

            sprintf(msg, "Unable to locate sound %s\n", name);
            print_to_console(msg, 0);
            doing_help_for_comand = 1;
        } else {
            console_sound_stop();
            console_sound_buffer = snd_get_handle(name, 5.0f, 1.0f, 1.0f, 0, false);
            console_sound_handle = snd_play_streamed(console_sound_buffer, 1.0f, 0);
        }
    }
    if (doing_help_for_comand)
        print_to_console("Usage: play_sound <sound file>", 0);
}

// 0x43d710  Confidence: exact. The sound from snd_get_handle is forgotten, not released.
void console_sound_stop()
{
    if (snd_is_playing(console_sound_handle))
        snd_stop(console_sound_handle);
    if (console_sound_buffer)
        console_sound_buffer = 0;
}

// 0x43d750 init_ccmd_stop_sound_thunk (jmp). Confidence: exact.
// 0x43d760 init_ccmd_stop_sound (console_command constructor). Confidence: exact.
void cmd_stop_sound();
console_command ccmd_stop_sound("stop_sound", "Stops all sounds that are playing", cmd_stop_sound);   // 0x60abd0

// 0x43d780  Confidence: exact. Stops only the play_sound stream, despite the help text.
void cmd_stop_sound()
{
    if (calling_a_command_function)
        console_sound_stop();
    if (doing_help_for_comand)
        print_to_console("Usage: stop_sound", 0);
}

// ---- main loop -------------------------------------------------------------------------------------

// 0x43d7b0  Confidence: exact.
int MainLoop()
{
    atexit(atexit_nop_512590);
    LoadSettings();
    GameInit();
    do {
        os_poll();
    } while (GameTick() != 3);
    game_cleanup();
    game_shutdown_systems();
    return 0;
}

// 0x43d7f0  Confidence: exact (VC6 byte match, match14; was likely) (the empty branches of the sound card and video card tests: see
// NOTES.md; frame 0x99c bytes). width / height are only compared for equality (DWORD); card is an int
// passed as DWORD * (printed with %d, compared with -1). The registry may hold any size: one that is
// not in resolution_table leaves resolution_index at 0 (640x480) but the -800 / -1024 switches win.
// Summoner_options still holds its constructor defaults here (summoner_cfg_load runs in GameInit), so
// gr_init always gets cfg_34 = 1.0f.
void LoadSettings()
{
    bool no_sound;
    DWORD height;
    DWORD width;
    int card;
    char card_key[128];
    char install_path[260];
    char soundcard[1024];
    char card_name[1024];
    int renderer;
    int i;

    os_init("Volition", "Summoner");
    os_registry_set_app_key("Volition", "Summoner");
    cmdline_read_params();
    os_registry_write_dword("Version", "Major", 1);
    os_registry_write_dword("Version", "Minor", 40);
    os_registry_write_dword("Version", "Build", 0);

#ifdef FIXED_BUILD
    // FIXED_BUILD: launcher ResolutionDefault (not a numbered bug: a modern-PC layer of SummonerFix). Without the launcher (which cannot detect a modern video card, so its
    // registry key stays empty) the game fell back to 640x480; its interface is laid out for 1024x768, the size
    // SummonerFix uses by default. A value the registry does hold is still used.
    os_registry_read_dword(NULL, "Resolution Width", &width, 1024);
    os_registry_read_dword(NULL, "Resolution Height", &height, 768);
#else
    os_registry_read_dword(NULL, "Resolution Width", &width, 640);
    os_registry_read_dword(NULL, "Resolution Height", &height, 480);
#endif
    if (cmdline_param_800.found()) {
        width = 800;
        height = 600;
    } else if (cmdline_param_1024.found()) {
        width = 1024;
        height = 768;
    }
    const uint *h;
    for (i = 0, h = &resolution_table[0][1]; h < &resolution_table[3][1]; h += 2, i++) {
        if (width == h[-1] && height == *h) {
            resolution_index = i;
            break;
        }
    }
#ifdef FIXED_BUILD
    // FIXED_BUILD: launcher CustomResolution (not a numbered bug: SummonerFix's resolution table patch). A size the
    // table does not hold (SummonerFix answers the registry with [Display] GameWidth / GameHeight, e.g. 1366x768)
    // left resolution_index at 0 and the game ran at 640x480. It goes into the 1024x768 slot instead, which keeps
    // the 1024 layout of the interface (resolution_index 2), as SummonerFix does for the original exe.
    if (i == 3 && width >= 640 && height >= 480) {
        resolution_table[2][0] = width;
        resolution_table[2][1] = height;
        resolution_index = 2;
    }
#endif

    timer_init();
    file_init(NULL, true);
    setup_data_paths();
    localize_init();
    CdCheck();
    effects_load_bases();
    nw_init_networking(mp_net_port);
    key_init(0, 1);
    bm_init();
    mouse_init(resolution_table[resolution_index][0], resolution_table[resolution_index][1]);

    no_sound = false;
    if (!cmdline_directsound3d && !cmdline_flag_60ad6d && !cmdline_flag_60ad6e && !cmdline_eax) {
        os_registry_read_string(NULL, "Soundcard", soundcard, "DirectSound");
        if (!_stricmp(soundcard, "DirectSound")) {
        } else if (!_stricmp(soundcard, "EAX")) {
            cmdline_eax = true;
        } else if (!_stricmp(soundcard, "No Sound")) {
            no_sound = true;
        } else if (!_stricmp(soundcard, "")) {
        }
    }
    snd_init(os_get_main_wnd(), 44100, 16, 2, cmdline_directsound3d, cmdline_eax, no_sound,
             cmdline_param_dsinit.found());
    sounds_parse_table();
    ca_system_init(-1);
    ci_root_to_position = false;
    init_char_types(0);

#ifdef REBUILT_OPENGL
    // Standalone test builds (tools/link-build.mjs --define REBUILT_OPENGL): the game's own, never-selected
    // OpenGL renderer in a window, as SummonerFix patches it in the shipped exe (0x43da64, 0x43db45). Not the
    // shipped behaviour, which defaults to Glide fullscreen.
    renderer = GR_OPENGL;
#else
    renderer = GR_GLIDE;
#endif
    os_registry_read_dword(NULL, "Selected Video Card", (DWORD *)&card, (DWORD)-1);
    if (card != -1) {
        sprintf(card_key, "Video Card %d", card);
        os_registry_read_string(NULL, card_key, card_name, "None");
        if (_stricmp(card_name, "None")) {
            if (strstr(card_name, "3DFX")) {
                renderer = GR_GLIDE;
            } else if (strstr(card_name, "Direct 3D")) {
                renderer = GR_DIRECT3D;
            }
        }
    }
    if (cmdline_param_direct3d.found())
        renderer = GR_DIRECT3D;

#if defined(REBUILT_OPENGL) && defined(EXPERIMENTAL_BUILD)
    // Experimental build: the renderer SummonerFix's [Display] Renderer selects through the registry answer
    // (OpenGL, or Direct3D / Glide, e.g. through dgVoodoo 2: docs/future/dgvoodoo.md). OpenGL runs in a window
    // (SummonerFix's output window); Direct3D and Glide fullscreen (201, which Glide requires) unless
    // [Display] Windowed=1.
    gr_init(resolution_table[resolution_index][0], resolution_table[resolution_index][1], renderer,
            renderer == GR_OPENGL || exp_ini_int("Display", "Windowed", 0) ? 200 : 201,
            os_get_main_wnd(), Summoner_options.cfg_34 ? true : false, false);
#elif defined(REBUILT_OPENGL)
    gr_init(resolution_table[resolution_index][0], resolution_table[resolution_index][1], renderer, 200,
            os_get_main_wnd(), Summoner_options.cfg_34 ? true : false, false);
#else
    gr_init(resolution_table[resolution_index][0], resolution_table[resolution_index][1], renderer, 201,
            os_get_main_wnd(), Summoner_options.cfg_34 ? true : false, false);
#endif
    ui_init();
    gr_load_reflection_map();
    mouse_move_to(resolution_table[resolution_index][0] / 2, resolution_table[resolution_index][1] / 2);

    ui_button_colors[7].set(255, 176, 0, 48);
    ui_window_colors[2].set(96, 92, 88, 176);
    ui_window_colors[0].set(128, 128, 128, 176);
    ui_window_colors[3].set(96, 92, 88, 176);
    ui_window_colors[1].set(48, 48, 48, 176);

    file_make_path(0, NULL, install_path);
    os_registry_write_string(NULL, "InstallPath", install_path);
    vif_records_reset();
    gr_set_use_buffered_alpha_polys(true);
}

// 0x43dc30  Confidence: exact.
void cmdline_read_params()
{
    cmdline_init(false, 0);
    mp_net_port = 10234;
    if (cmdline_param_port.found())
        mp_net_port = cmdline_param_port.get_arg_int();
    cmdline_directsound3d = cmdline_param_directsound3d.found();
    cmdline_eax = cmdline_param_eax.found();
    cmdline_pack = cmdline_param_pack.found();
}

// 0x43dca0  Confidence: exact.
void setup_data_paths()
{
    path_root = file_add_path("", ".cfg .vpp .vcs .brw .arr .ico .sys .pre");
    path_data = file_add_path("data", ".cfg");
    file_add_path("data\\ca", ".mvf");
    file_add_path("data\\fonts", ".vf .tga .vbm");
    file_add_path("data\\pack\\svfs", ".svf");
    path_multicache = file_add_path("multicache", ".cfg .pcx");
    path_interface = file_add_path("data\\interface\\pc", ".vbm .tga .m2v .vaf ");
    file_add_path("data\\sounds\\pc", ".wav");
    file_add_path("data\\sounds\\pc\\cutscenes", ".wav");
    file_add_path("data\\sounds\\pc\\music", ".wav");
    path_maps_characters = file_add_path("data\\maps\\characters", ".vbm .tga .m2v");
    path_maps_effects = file_add_path("data\\maps\\effects", ".vbm .tga .m2v");
    path_maps_items = file_add_path("data\\maps\\items", ".vbm .tga .m2v");
    path_maps_levels = file_add_path("data\\maps\\levels", ".vbm .tga .m2v");
    file_add_path("data\\maps\\cutscenes", ".vbm .tga .m2v");
    file_add_path("data\\tables\\ui\\worldmap", ".ui");
    file_add_path("data\\effects\\characters", ".tga .m2v .vfx .vbm");
    file_add_path("data\\effects\\cutscenes", ".tga .m2v .vfx .vbm");
    file_add_path("data\\effects\\items", ".tga .m2v .vfx .vbm");
    file_add_path("data\\effects\\levels", ".tga .m2v .vfx .vbm");
    file_add_path("data\\effects\\spells", ".tga .m2v .vfx .vbm");
    file_add_path("data\\effects", ".vfx .rfx");
    file_add_path("data\\tables\\boss", ".tbl");
    path_tables = file_add_path("data\\tables", ".tbl");
    file_add_path("data\\v3d", ".v3d .s3d .vfx");
    path_models_levels = path_models_levels_copy = file_add_path("data\\models\\levels", ".v3d .s3d .vfx .vlm");
    file_add_path("data\\models\\characters", ".v3d .vfx .vim");
    file_add_path("data\\models\\items", ".v3d .vfx .vim");
    path_scripts = file_add_path("data\\scripts", ".scr .tbl");
    path_tables_characterinfo = file_add_path("data\\tables\\characterinfo", ".tbl");
    path_internal = path_internal_copy = file_add_path("data\\internal", ".pfg .mlo .vis .vex .bsp .lkf .eax");
    path_players = file_add_path("players", ".plr");
    path_savegame = file_add_path("savegame", ".sav");
    path_tables_levels = file_add_path("data\\tables\\levels", ".tbl");
    path_movies = file_add_path("data\\movies", "*.bik");
}

// 0x43df20  Confidence: exact (the last call is a tail jump).
void game_shutdown_systems()
{
    ca_system_close();
    bm_close();
    gr_close();
    snd_close();
    os_close_main_window();
}

// 0x43df40  Confidence: exact (VC6 byte match, match14; was likely) (the ComputerSpeed clamp shape; the font switch's case 0 shares the
// default). The ComputerSpeed presets are dead: summoner_cfg_load resets every field right after.
void GameInit()
{
    int speed;
    int start;
    const char *font_name;

#ifdef EXPERIMENTAL_BUILD
    exp_game_options_init();   // experimental build: SummonerFix's [Game] options (before the main menu)
#endif
    os_seed_random(-1);
    switch (resolution_index) {
    case 2:
        font_name = "sumbones1024.vf";
        break;
    case 1:
        font_name = "sumbones800.vf";
        break;
    case 0:
        font_name = "sumbones640.vf";
        break;
    default:
        font_name = "sumbones640.vf";
        break;
    }
    font_main = gr_load_font(font_name, -1);
    font_small = gr_load_font("font-small.vf", 9);
    console_set_font(font_small);
    font_bones_small = gr_load_font("SumBones-small.vf", -1);
    gr_fade_in(0);
    gameplay_load_cursors();
    effect_stream_init();
    handle_slots_reset_thunk();
    editor_dialog_init();

    start = timer_get(1000);
    spells_parse_table();
    items_init();
    alt_textures_parse_table();
    characterinfo_load_party(path_tables_characterinfo);
    experience_parse_table();
    shadows_init();
    cutscene_parse_table();
    objects_init();
    shadows_parse_table();
    party_lists_init();
    ai_global_init();
    conversation_db.alloc_storage();
    script_filenames_parse_table();
    savegame_alloc();
    pxo_read_config_thunk();
    printf("SYSTEM INIT TIME : %.2f\n", (timer_get(1000) - start) * 0.001f);

    gameseq_init();
    gameseq_set_state(1);
    game_pause_count_inc();
    music_feedback_parse_tables();
    interface_init_global();
    automap_clear_all();

    if (os_registry_read_dword(NULL, "ComputerSpeed", (DWORD *)&speed, 2) == 0) {
        if (speed < 0)
            speed = 0;
        else if (speed > 2)
            speed = 2;
        switch (speed) {
        case 0:
            Summoner_options.fog_quality = 0;
            Summoner_options.detail_14 = 0;
            Summoner_options.detail_18 = 2;
            Summoner_options.detail_24 = 0;
            break;
        case 1:
            Summoner_options.fog_quality = 1;
            Summoner_options.detail_14 = 2;
            Summoner_options.detail_18 = 2;
            Summoner_options.detail_24 = 1;
            break;
        case 2:
            Summoner_options.fog_quality = 2;
            Summoner_options.detail_14 = 1;
            Summoner_options.detail_18 = 2;
            Summoner_options.detail_24 = 2;
            break;
        }
    }

    summoner_cfg_load();
    gr_set_gamma(Summoner_options.gamma_correction);
    if (!gr_is_mode_supported(&gr_mode_lightmap_multitexture)) {
        Summoner_options.lightmaps_off = true;
        S3d_load_lightmaps = false;
    }
    if (!gr_antialias_enabled)
        Summoner_options.antialias = false;
    options_apply_detail();

    set_current_mempool(&joseph_char_mempool);
    character_load(&hero_model_names[0], 2, 1, 0, &joseph_model_mempool, &joseph_char_mempool);
    set_current_mempool(&jekhar_char_mempool);
    character_load(&hero_model_names[2], 2, 1, 0, &jekhar_model_mempool, &jekhar_char_mempool);
    set_current_mempool(&flece_char_mempool);
    character_load(&hero_model_names[4], 2, 1, 0, &flece_model_mempool, &flece_char_mempool);
    set_current_mempool(&rosalind_char_mempool);
    character_load(&hero_model_names[6], 1, 1, 0, &rosalind_model_mempool, &rosalind_char_mempool);
    set_current_mempool(NULL);
    chain_config_reset_defaults();
}

// 0x43e1f0  Confidence: exact (0x4f13e0 is a folded empty function of another unit, name not known).
void game_cleanup()
{
    conversation_db.free();
    level_filename[0] = 0;
    level_script_name[0] = 0;
    level_prev_filename[0] = 0;
    empty_function();
    cutscene_shutdown_stub();
    fx_shutdown();
    item_types_free();
    spells_free_strings();
    objects_free_all_thunk();
    debug_noop_cleanup();
    interface_reset_state();
    alt_textures_free();
    if (pxo_run_updater_on_exit)
        launch_updater();
}

// ---- summoner.cfg ----------------------------------------------------------------------------------

// 0x43e250  Confidence: exact (VC6 byte match, D25000: the sfx volume through memcpy into a one-float
// array; was likely) (the goto; the clamp is `>=` for the JL at 0x43e345, the final test
// unsigned for the JAE at 0x43e36a). A missing or short file keeps the defaults, and then 8 MB of
// video memory or less drops the texture quality to 0.
void summoner_cfg_load()
{
    File f;

    Summoner_options.set_defaults();
    if (f.open("summoner.cfg", 1, path_root) == 0) {
        int n = f.size(NULL, PATH_ANY);
        int size;
        if (n < 0x40)
            size = n;
        else
            size = 0x40;
        f.read(&Summoner_options, size, 0, 0);
        f.close();
        if ((unsigned int)size >= sizeof(Summoner_options))
            goto loaded;
    }
    if (gr_get_video_memory_mb() <= 8)
        Summoner_options.texture_quality = 0;
loaded:
    // D25000: the sfx volume copied with memcpy into a one-float array: the copy is still an integer move, and
    // it puts the tail's registers in the binary's EAX / ECX / EDX rotation.
    float vv[1];
    memcpy(vv, &Summoner_options.sfx_volume, sizeof(float));
    snd_set_group_volume(0, vv[0]);
    music_set_volume(Summoner_options.music_volume);
    if (snd_is_eax_enabled()) {
        if (Summoner_options.eax)
            eax_restore_reverb();
        else
            eax_mute_reverb();
    }
    if (Summoner_options.cfg_3c)
        pxo_cfg_flags = (pxo_cfg_flags & ~2) | 1;
    else
        pxo_cfg_flags = (pxo_cfg_flags & ~1) | 2;
}

// 0x43e410  Confidence: exact.
void summoner_cfg_save()
{
    File f;

    if (f.open("summoner.cfg", 2, path_root) == 0) {
        f.write(&Summoner_options, sizeof(Summoner_options));
        f.close();
    }
}

// 0x43e4a0  Confidence: exact (the order of the dir / STARTUPINFO clears). Starts the PXO updater
// (Summoner.exe in the install directory) when game_cleanup is told to; the process handles are not
// closed.
void launch_updater()
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    char dir[1024] = "";
    char cmd[2048];

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    file_get_full_dir(path_root, dir);
    strcpy(cmd, dir);
    strcat(cmd, "\\");
    strcat(cmd, "Summoner.exe");
    strcat(cmd, " -straight_to_update");
    CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si, &pi);
}

// ---- entry point -----------------------------------------------------------------------------------

// 0x43e5e0  Confidence: likely (unit membership: see the file comment; the code is exact). The
// __except filter writes errorlog.txt and returns EXCEPTION_CONTINUE_SEARCH on every path (D2905), so the
// __except block never runs and the crash goes on to the system handler; result = -1 is never returned.
// mingw has no __try / __except, so the syntax check sees the plain call (D601).
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int result;

    cd_select_drive();
    result = -1;
#ifdef _MSC_VER
    __try {
        result = MainLoop();
    } __except (RecordExceptionInfo(GetExceptionInformation(), "Summoner Main Thread")) {
    }
#else
    result = MainLoop();
#endif
    return result;
}
