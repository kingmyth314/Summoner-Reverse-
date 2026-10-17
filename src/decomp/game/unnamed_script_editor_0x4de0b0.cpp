// unnamed_script_editor_0x4de0b0.cpp - Sum.exe v1.40, code 0x4de0b0-0x4ed04f: the in-game level script
// editor (docs/sum-exe/script-editor.md): one initialiser run 0x4de0b0-0x4dff4f for its static gadgets,
// then the editor UI, picking and markers, navpoints, action lists, the script / level table / level item
// writers, the remove_nav and render_s3d_navpoints console commands.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range (checked): the previous unit (script_internal.cpp) ends with script_build_avoid_spline
// (0x4ddc60-0x4de0a3); this one starts with static_initializers entry 374 (0x4de0b0). It ends with
// cmd_render_s3d_navpoints (0x4ecf90-0x4ed04f); the cutscene script unit starts with its initialiser at
// entry 474 (0x4ed050, cutscene_object_timers).
// .data 0x5a08a8-0x5a1573: script_editor_mode_names (the unit's only initialised data, right after
// script_internal.cpp's last literal "-intro" 0x5a08a0), then the six mode names and the literal pool, in
// address order of the functions, each function's literals in reverse order of use. Pooled copies of
// other units' literals: "OK" / "Cancel" (0x5930b4 / 0x5930ac), "Load" (0x5931a8), "$npc" (0x59ab4c),
// "_script.tbl" (0x59e710), "_cscript.tbl" (0x5939b4), "gold" (0x594430), "%d" (0x589820), "%s %s"
// (0x594fd4), ".wav" (0x5949b8), "" (0x5bd0d0).
// .bss 0x2546828-0x256b74f (the last field is script_editor_sel_action_index 0x256b74c; cutscene_navpoints
// 0x256b750 belongs to the cutscene script unit). VC6 lays .bss out in no source order, so the order of the
// definitions below is ours.
// Float constants: 0.5 (0x57e6b0), 100.0 (0x57e6b4), 1.0 (0x57e990), 0 (0x57eb84), 1000.0 (0x57ef28),
// pi (0x57f028), -1.0 (0x580e1c), 2 pi (0x580e28), 0.01 (0x5818ec), 0.001 (0x581938), -pi (0x581a48),
// 0.008 (0x581ca0): __real@ COMDATs shared with other units.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vsdk/os/console.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/os/norlsallocpool.h"
#include "vsdk/os/plainstring.h"
#include "vsdk/os/key.h"
#include "vsdk/os/mouse.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/math/ix.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/snd/snd.h"
#include "vsdk/vfile/file.h"
#include "vsdk/parse/parse.h"
#include "vsdk/ui/ui.h"
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/ui/ui_container.h"
#include "vsdk/ui/window.h"
#include "engine/objects/objects.h"
#include "engine/objects/items.h"
#include "engine/objects/living_entity.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/camera/camera.h"
#include "engine/ai/boss_ai.h"
#include "engine/rendering/levelrender.h"
#include "engine/s3d/s3d.h"
#include "summoner/summoner.h"
#include "summoner/player/player.h"
#include "summoner/unnamed_cutscene_0x434900.h"
#include "summoner/unnamed_script_editor_0x4de0b0.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/script_internal.h"
#include "levelscripts/savegame.h"
#include "engine/ai/pathfinding.h"

// ---- .data (0x5a08a8) ----------------------------------------------------------------------------------
const char *script_editor_mode_names[SE_NUM_MODES] = {          // 0x5a08a8
    "Creature", "Navpoint", "Camera", "Ambient sound", "Door", "Click object",
};

// ---- the initialiser run (static_initializers entries 374-473) -------------------------------------------
// Every static object below has its initialiser (a thunk entry in the table, then the construction and an
// atexit registration for objects with a destructor). One declaration with several objects gives one
// initialiser that constructs them in order and one atexit handler that destroys them in reverse order
// (the _x2 / _x3 / _x4 entries). Confidence for all of them: exact (compiler-generated), except where an
// empty initialiser's object is named (unsure: which object it is).

// 0x4de0b0  static_init_script_editor_defaults (static_initializers entry 374). Confidence: exact (compiler-generated).
// 0x4de0c0  init_script_editor_defaults. Confidence: likely (one aggregate-initialised array).
// 0x4de410  script_action_defaults__init. Confidence: exact (compiler-generated).
// init_script_editor_defaults: the 33 constructor calls; the first 20 are expanded inline, the
// last 13 call the out-of-line copy (VC6 stops expanding inline functions in a function after a size
// budget). Confidence: exact (values), likely (that the table is one aggregate-initialised array).
// script_action_defaults::script_action_defaults, the out-of-line COMDAT of the inline
// constructor (thiscall, ret 0x14). Confidence: exact (compiler-generated).
script_action_defaults script_action_default_table[NUM_SCRIPT_ACTIONS] = {   // 0x2556ab0
    script_action_defaults(0, 0, false, true, false),       // walk to
    script_action_defaults(0, 0, false, true, false),       // run to
    script_action_defaults(0, 0, false, true, false),       // move through
    script_action_defaults(0, 0, false, true, false),       // wait at
    script_action_defaults(50, 0, true, true, false),       // wander
    script_action_defaults(50, 10, true, true, false),      // wander for
    script_action_defaults(0, 0, false, false, false),      // do state
    script_action_defaults(0, 0, false, false, true),       // play action
    script_action_defaults(-1, 0, false, false, false),     // play .mvf
    script_action_defaults(0, 0, false, false, true),       // play effect
    script_action_defaults(1, 0, false, false, false),      // turn hostile
    script_action_defaults(0, 0, false, false, false),      // wait for go
    script_action_defaults(5, 0, false, false, true),       // say
    script_action_defaults(0, 0, false, false, true),       // die
    script_action_defaults(1, 0, false, false, false),      // label
    script_action_defaults(1, 0, false, false, false),      // goto
    script_action_defaults(1, 0, false, false, false),      // no block
    script_action_defaults(5, 0, false, false, false),      // trigger
    script_action_defaults(1, 0, false, true, false),       // trigger nav
    script_action_defaults(0, 0, false, false, false),      // trigger visit
    script_action_defaults(0, 0, false, true, false),       // teleport to (0x4de346, first call)
    script_action_defaults(0, 0, false, false, true),       // pass go
    script_action_defaults(0, 0, false, false, false),      // show/hide
    script_action_defaults(1, 0, false, false, false),      // synch
    script_action_defaults(0, 0, false, true, false),       // synch teleport
    script_action_defaults(-1, 0, false, false, true),      // follow
    script_action_defaults(1, 0, false, false, false),      // combat mode
    script_action_defaults(0, 0, false, false, false),      // mount anim
    script_action_defaults(0, 0, false, false, false),      // dismount anim
    script_action_defaults(-1, 0, false, false, true),      // replace weapon
    script_action_defaults(0, 0, false, false, false),      // guard spline
    script_action_defaults(-1, 0, false, false, false),     // replace armor
    script_action_defaults(1, 0, false, false, false),      // one hand
};

// 0x4de440  static_init_empty_4de450 (static_initializers entry 375). Confidence: exact (compiler-generated).
// 0x4de450  init_empty_4de450. Confidence: unsure (which object it constructs).
// Five empty initialisers (a bare ret each: objects whose constructors are inline and empty) follow, and a
// sixth at 0x4dff00 near the end of the run. The candidates are the objects of types with an inline empty
// constructor (vector3, matrix, Color, and the structs that contain them): the two pick records, the mouse
// ray, the saved drag position and orientation, the two colour arrays. Which initialiser is which (and
// whether the colour arrays share one declaration) cannot be told from the code. Confidence: unsure.
script_editor_pick script_editor_hover;              // 0x2546bd0

// 0x4de460  static_init_empty_4de470 (static_initializers entry 376). Confidence: exact (compiler-generated).
// 0x4de470  init_empty_4de470. Confidence: unsure (which object it constructs).
script_editor_pick script_editor_sel;                // 0x255a008

// 0x4de480  static_init_empty_4de490 (static_initializers entry 377). Confidence: exact (compiler-generated).
// 0x4de490  init_empty_4de490. Confidence: unsure (which object it constructs).
ix_ray script_editor_mouse_ray;                      // 0x2552c08

// 0x4de4a0  static_init_empty_4de4b0 (static_initializers entry 378). Confidence: exact (compiler-generated).
// 0x4de4b0  init_empty_4de4b0. Confidence: unsure (which object it constructs).
vector3 script_editor_saved_pos;                     // 0x2563a58

// 0x4de4c0  static_init_empty_4de4d0 (static_initializers entry 379). Confidence: exact (compiler-generated).
// 0x4de4d0  init_empty_4de4d0. Confidence: unsure (which object it constructs).
Color script_editor_level_entry_colors[6], script_editor_cutscene_entry_colors[6];   // 0x254f1f0, 0x254f6f8

// 0x4de4e0  static_init_script_editor_status_timer (static_initializers entry 380). Confidence: exact (compiler-generated).
// 0x4de4f0  init_script_editor_status_timer. Confidence: exact (compiler-generated).
// init_script_editor_status_timer: RealtimeTimer() (0x5019e0). Confidence: exact.
RealtimeTimer script_editor_status_timer;            // 0x255434c

// 0x4de500  static_init_ui_window_254c4e0 (static_initializers entry 381). Confidence: exact (compiler-generated).
// 0x4de510  init_ui_window_254c4e0: ui_window::ui_window (0x52b020). Confidence: exact (compiler-generated).
// 0x4de520  register_atexit_ui_window_254c4e0: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de530  atexit_dtor_ui_window_254c4e0: ~ui_window (0x52b0a0). Confidence: exact (compiler-generated).
// The gadgets, in initialiser order. Each single object has four helpers (thunk: call init, jmp register;
// init: the constructor; register: atexit; the atexit handler: the destructor). ui_window's destructor is
// 0x52b0a0, ui_button's and ui_static's implicit ones are the COMDATs 0x4317e0 / 0x431780 kept from the
// editor dialog unit, ui_listbox's 0x431950 (boot / load-game unit). ui_inputbox, ui_container and
// ui_radio_button / ui_checkbox have no destructor of their own: their atexit handlers call ~ui_gadget
// through 0x4de800 (a jmp, see below).
ui_window script_editor_window;                      // 0x254c4e0

// 0x4de540  static_init_ui_button_255ced8 (static_initializers entry 382). Confidence: exact (compiler-generated).
// 0x4de550  init_ui_button_255ced8: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de560  register_atexit_ui_button_255ced8: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de570  atexit_dtor_ui_button_255ced8: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
// ten single ui_button objects (pause, cutscene, prev, next, pos,
// ori, y, mode, new, delete: initialiser order).
ui_button script_editor_pause_button;                // 0x255ced8

// 0x4de580  static_init_ui_button_25527e0 (static_initializers entry 383). Confidence: exact (compiler-generated).
// 0x4de590  init_ui_button_25527e0: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de5a0  register_atexit_ui_button_25527e0: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de5b0  atexit_dtor_ui_button_25527e0: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_cutscene_button;             // 0x25527e0

// 0x4de5c0  static_init_ui_button_2557b38 (static_initializers entry 384). Confidence: exact (compiler-generated).
// 0x4de5d0  init_ui_button_2557b38: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de5e0  register_atexit_ui_button_2557b38: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de5f0  atexit_dtor_ui_button_2557b38: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_prev_button;                 // 0x2557b38

// 0x4de600  static_init_ui_button_2565070 (static_initializers entry 385). Confidence: exact (compiler-generated).
// 0x4de610  init_ui_button_2565070: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de620  register_atexit_ui_button_2565070: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de630  atexit_dtor_ui_button_2565070: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_next_button;                 // 0x2565070

// 0x4de640  static_init_ui_button_2565bc8 (static_initializers entry 386). Confidence: exact (compiler-generated).
// 0x4de650  init_ui_button_2565bc8: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de660  register_atexit_ui_button_2565bc8: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de670  atexit_dtor_ui_button_2565bc8: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_pos_button;                  // 0x2565bc8

// 0x4de680  static_init_ui_button_254edc8 (static_initializers entry 387). Confidence: exact (compiler-generated).
// 0x4de690  init_ui_button_254edc8: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de6a0  register_atexit_ui_button_254edc8: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de6b0  atexit_dtor_ui_button_254edc8: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_ori_button;                  // 0x254edc8

// 0x4de6c0  static_init_ui_button_254b6e0 (static_initializers entry 388). Confidence: exact (compiler-generated).
// 0x4de6d0  init_ui_button_254b6e0: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de6e0  register_atexit_ui_button_254b6e0: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de6f0  atexit_dtor_ui_button_254b6e0: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_y_button;                    // 0x254b6e0

// 0x4de700  static_init_ui_button_2556260 (static_initializers entry 389). Confidence: exact (compiler-generated).
// 0x4de710  init_ui_button_2556260: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de720  register_atexit_ui_button_2556260: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de730  atexit_dtor_ui_button_2556260: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_mode_button;                 // 0x2556260

// 0x4de740  static_init_ui_button_254f710 (static_initializers entry 390). Confidence: exact (compiler-generated).
// 0x4de750  init_ui_button_254f710: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de760  register_atexit_ui_button_254f710: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de770  atexit_dtor_ui_button_254f710: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_new_button;                  // 0x254f710

// 0x4de780  static_init_ui_button_2546c18 (static_initializers entry 391). Confidence: exact (compiler-generated).
// 0x4de790  init_ui_button_2546c18: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de7a0  register_atexit_ui_button_2546c18: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de7b0  atexit_dtor_ui_button_2546c18: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_delete_button;               // 0x2546c18

// 0x4de7c0  static_init_ui_inputbox_2555908 (static_initializers entry 392). Confidence: exact (compiler-generated).
// 0x4de7d0  init_ui_inputbox_2555908: ui_inputbox::ui_inputbox (0x5526b0). Confidence: exact (compiler-generated).
// 0x4de7e0  register_atexit_ui_inputbox_2555908: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de7f0  atexit_dtor_ui_inputbox_2555908: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
// 0x4de800  ui_gadget__dtor_thunk. Confidence: exact (compiler-generated).
ui_inputbox script_editor_name_inputbox;             // 0x2555908

// 0x4de810  static_init_ui_container_25499a8 (static_initializers entry 393). Confidence: exact (compiler-generated).
// 0x4de820  init_ui_container_25499a8: ui_container::ui_container (0x553560). Confidence: exact (compiler-generated).
// 0x4de830  register_atexit_ui_container_25499a8: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de840  atexit_dtor_ui_container_25499a8: ~ui_container = 0x4de800. Confidence: exact (compiler-generated).
// ui_gadget__dtor_thunk: jmp ~ui_gadget (0x52c410). The implicit destructor of ui_inputbox (and
// of the other gadget classes without one of their own), a COMDAT emitted after its first use above; the
// later atexit handlers of inputboxes, containers, radio buttons and check boxes reach it. Confidence:
// exact (compiler-generated).
ui_container script_editor_selection_panel;          // 0x25499a8

// 0x4de850  static_init_ui_listbox_254c9d0 (static_initializers entry 394). Confidence: exact (compiler-generated).
// 0x4de860  init_ui_listbox_254c9d0: ui_listbox::ui_listbox (0x52d5c0). Confidence: exact (compiler-generated).
// 0x4de870  register_atexit_ui_listbox_254c9d0: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de880  atexit_dtor_ui_listbox_254c9d0: ~ui_listbox (0x431950). Confidence: exact (compiler-generated).
ui_listbox script_editor_action_listbox;             // 0x254c9d0

// 0x4de890  static_init_ui_button_2560a90 (static_initializers entry 395). Confidence: exact (compiler-generated).
// 0x4de8a0  init_ui_button_2560a90: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de8b0  register_atexit_ui_button_2560a90: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de8c0  atexit_dtor_ui_button_2560a90: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
// seven single ui_button objects.
ui_button script_editor_append_action_button;        // 0x2560a90

// 0x4de8d0  static_init_ui_button_255b4e0 (static_initializers entry 396). Confidence: exact (compiler-generated).
// 0x4de8e0  init_ui_button_255b4e0: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de8f0  register_atexit_ui_button_255b4e0: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de900  atexit_dtor_ui_button_255b4e0: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_insert_action_button;        // 0x255b4e0

// 0x4de910  static_init_ui_button_2557f60 (static_initializers entry 397). Confidence: exact (compiler-generated).
// 0x4de920  init_ui_button_2557f60: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de930  register_atexit_ui_button_2557f60: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de940  atexit_dtor_ui_button_2557f60: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_delete_action_button;        // 0x2557f60

// 0x4de950  static_init_ui_button_2563630 (static_initializers entry 398). Confidence: exact (compiler-generated).
// 0x4de960  init_ui_button_2563630: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de970  register_atexit_ui_button_2563630: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de980  atexit_dtor_ui_button_2563630: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_action_up_button;            // 0x2563630 (no sum.symbols name)

// 0x4de990  static_init_ui_button_2551a60 (static_initializers entry 399). Confidence: exact (compiler-generated).
// 0x4de9a0  init_ui_button_2551a60: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de9b0  register_atexit_ui_button_2551a60: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4de9c0  atexit_dtor_ui_button_2551a60: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_action_down_button;          // 0x2551a60 (no sum.symbols name)

// 0x4de9d0  static_init_ui_button_255f178 (static_initializers entry 400). Confidence: exact (compiler-generated).
// 0x4de9e0  init_ui_button_255f178: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4de9f0  register_atexit_ui_button_255f178: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dea00  atexit_dtor_ui_button_255f178: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_change_button;               // 0x255f178

// 0x4dea10  static_init_ui_button_254bb08 (static_initializers entry 401). Confidence: exact (compiler-generated).
// 0x4dea20  init_ui_button_254bb08: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4dea30  register_atexit_ui_button_254bb08: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dea40  atexit_dtor_ui_button_254bb08: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_edit_button;                 // 0x254bb08

// 0x4dea50  static_init_ui_window_25508e0 (static_initializers entry 402). Confidence: exact (compiler-generated).
// 0x4dea60  init_ui_window_25508e0: ui_window::ui_window (0x52b020). Confidence: exact (compiler-generated).
// 0x4dea70  register_atexit_ui_window_25508e0: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dea80  atexit_dtor_ui_window_25508e0: ~ui_window (0x52b0a0). Confidence: exact (compiler-generated).
ui_window script_editor_new_window;                  // 0x25508e0

// 0x4dea90  static_init_ui_listbox_2563a68 (static_initializers entry 403). Confidence: exact (compiler-generated).
// 0x4deaa0  init_ui_listbox_2563a68: ui_listbox::ui_listbox (0x52d5c0). Confidence: exact (compiler-generated).
// 0x4deab0  register_atexit_ui_listbox_2563a68: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4deac0  atexit_dtor_ui_listbox_2563a68: ~ui_listbox (0x431950). Confidence: exact (compiler-generated).
ui_listbox script_editor_new_listbox;                // 0x2563a68

// 0x4dead0  static_init_ui_button_255f5a0 (static_initializers entry 404). Confidence: exact (compiler-generated).
// 0x4deae0  init_ui_button_255f5a0: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4deaf0  register_atexit_ui_button_255f5a0: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4deb00  atexit_dtor_ui_button_255f5a0: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
// two single ui_button objects.
ui_button script_editor_new_ok_button;               // 0x255f5a0

// 0x4deb10  static_init_ui_button_254e050 (static_initializers entry 405). Confidence: exact (compiler-generated).
// 0x4deb20  init_ui_button_254e050: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4deb30  register_atexit_ui_button_254e050: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4deb40  atexit_dtor_ui_button_254e050: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_new_cancel_button;           // 0x254e050

// 0x4deb50  static_init_ui_window_2554d28 (static_initializers entry 406). Confidence: exact (compiler-generated).
// 0x4deb60  init_ui_window_2554d28: ui_window::ui_window (0x52b020). Confidence: exact (compiler-generated).
// 0x4deb70  register_atexit_ui_window_2554d28: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4deb80  atexit_dtor_ui_window_2554d28: ~ui_window (0x52b0a0). Confidence: exact (compiler-generated).
ui_window script_editor_action_window;               // 0x2554d28

// 0x4deb90  static_init_ui_listbox_2564d50 (static_initializers entry 407). Confidence: exact (compiler-generated).
// 0x4deba0  init_ui_listbox_2564d50: ui_listbox::ui_listbox (0x52d5c0). Confidence: exact (compiler-generated).
// 0x4debb0  register_atexit_ui_listbox_2564d50: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4debc0  atexit_dtor_ui_listbox_2564d50: ~ui_listbox (0x431950). Confidence: exact (compiler-generated).
ui_listbox script_editor_action_type_listbox;        // 0x2564d50

// 0x4debd0  static_init_ui_button_254ccf0 (static_initializers entry 408). Confidence: exact (compiler-generated).
// 0x4debe0  init_ui_button_254ccf0: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4debf0  register_atexit_ui_button_254ccf0: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dec00  atexit_dtor_ui_button_254ccf0: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
// five single ui_button objects.
ui_button script_editor_action_ok_button;            // 0x254ccf0

// 0x4dec10  static_init_ui_button_2547040 (static_initializers entry 409). Confidence: exact (compiler-generated).
// 0x4dec20  init_ui_button_2547040: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4dec30  register_atexit_ui_button_2547040: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dec40  atexit_dtor_ui_button_2547040: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_action_cancel_button;        // 0x2547040

// 0x4dec50  static_init_ui_button_2560eb8 (static_initializers entry 410). Confidence: exact (compiler-generated).
// 0x4dec60  init_ui_button_2560eb8: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4dec70  register_atexit_ui_button_2560eb8: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dec80  atexit_dtor_ui_button_2560eb8: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_save_button;                 // 0x2560eb8

// 0x4dec90  static_init_ui_button_2560668 (static_initializers entry 411). Confidence: exact (compiler-generated).
// 0x4deca0  init_ui_button_2560668: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4decb0  register_atexit_ui_button_2560668: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4decc0  atexit_dtor_ui_button_2560668: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_load_button;                 // 0x2560668

// 0x4decd0  static_init_ui_button_2566a88 (static_initializers entry 412). Confidence: exact (compiler-generated).
// 0x4dece0  init_ui_button_2566a88: ui_button::ui_button (0x526f70). Confidence: exact (compiler-generated).
// 0x4decf0  register_atexit_ui_button_2566a88: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4ded00  atexit_dtor_ui_button_2566a88: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_time_back_button;            // 0x2566a88

// 0x4ded10  static_init_ui_window_2552c20 (static_initializers entry 413). Confidence: exact (compiler-generated).
// 0x4ded20  init_ui_window_2552c20: ui_window::ui_window (0x52b020). Confidence: exact (compiler-generated).
// 0x4ded30  register_atexit_ui_window_2552c20: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4ded40  atexit_dtor_ui_window_2552c20: ~ui_window (0x52b0a0). Confidence: exact (compiler-generated).
ui_window script_editor_save_window;                 // 0x2552c20

// 0x4ded50  static_init_ui_static_2553600_x2 (static_initializers entry 414). Confidence: exact (compiler-generated).
// 0x4ded60  init_ui_static_2553600_x2: ui_static::ui_static (0x527ea0) x2. Confidence: exact (compiler-generated).
// 0x4ded80  register_atexit_ui_static_2553600_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4ded90  atexit_dtor_ui_static_2553600_x2: ~ui_static (0x431780). Confidence: exact (compiler-generated).
ui_static script_editor_save_name_label, script_editor_save_filename_label;   // 0x2553600, 0x2551690

// 0x4dedb0  static_init_ui_inputbox_254daa0_x2 (static_initializers entry 415). Confidence: exact (compiler-generated).
// 0x4dedc0  init_ui_inputbox_254daa0_x2: ui_inputbox::ui_inputbox (0x5526b0) x2. Confidence: exact (compiler-generated).
// 0x4dede0  register_atexit_ui_inputbox_254daa0_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dedf0  atexit_dtor_ui_inputbox_254daa0_x2: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_save_name_inputbox, script_editor_save_filename_inputbox;   // 0x254daa0, 0x2567fd0

// 0x4dee10  static_init_ui_container_2562638 (static_initializers entry 416). Confidence: exact (compiler-generated).
// 0x4dee20  init_ui_container_2562638: ui_container::ui_container (0x553560). Confidence: exact (compiler-generated).
// 0x4dee30  register_atexit_ui_container_2562638: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dee40  atexit_dtor_ui_container_2562638: ~ui_container = 0x4de800. Confidence: exact (compiler-generated).
ui_container script_editor_save_kind_container;      // 0x2562638

// 0x4dee50  static_init_ui_radio_2555eb8_x2 (static_initializers entry 417). Confidence: exact (compiler-generated).
// 0x4dee60  init_ui_radio_2555eb8_x2: ui_radio_button::ui_radio_button (0x4deeb0) x2. Confidence: exact (compiler-generated).
// 0x4dee80  register_atexit_ui_radio_2555eb8_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dee90  atexit_dtor_ui_radio_2555eb8_x2: ~ui_radio_button = 0x4de800. Confidence: exact (compiler-generated).
// 0x4deeb0  ui_radio_button__ui_radio_button. Confidence: exact (compiler-generated).
// ui_radio_button::ui_radio_button, the implicit constructor (ui_checkbox(), then the vtable
// 0x581c7c), a COMDAT emitted after its first use. Confidence: exact (compiler-generated).
ui_radio_button script_editor_save_level_radio, script_editor_save_cutscene_radio;   // 0x2555eb8, 0x255a420

// 0x4deed0  static_init_ui_button_256ae28_x2 (static_initializers entry 418). Confidence: exact (compiler-generated).
// 0x4deee0  init_ui_button_256ae28_x2: ui_button::ui_button (0x526f70) x2. Confidence: exact (compiler-generated).
// 0x4def00  register_atexit_ui_button_256ae28_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4def10  atexit_dtor_ui_button_256ae28_x2: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_save_ok_button, script_editor_save_cancel_button;   // 0x256ae28, 0x255bcb0

// 0x4def30  static_init_ui_window_254f208 (static_initializers entry 419). Confidence: exact (compiler-generated).
// 0x4def40  init_ui_window_254f208: ui_window::ui_window (0x52b020). Confidence: exact (compiler-generated).
// 0x4def50  register_atexit_ui_window_254f208: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4def60  atexit_dtor_ui_window_254f208: ~ui_window (0x52b0a0). Confidence: exact (compiler-generated).
ui_window script_editor_load_window;                 // 0x254f208

// 0x4def70  static_init_ui_inputbox_254bf30 (static_initializers entry 420). Confidence: exact (compiler-generated).
// 0x4def80  init_ui_inputbox_254bf30: ui_inputbox::ui_inputbox (0x5526b0). Confidence: exact (compiler-generated).
// 0x4def90  register_atexit_ui_inputbox_254bf30: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4defa0  atexit_dtor_ui_inputbox_254bf30: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_load_name_inputbox;        // 0x254bf30

// 0x4defb0  static_init_ui_listbox_2557068 (static_initializers entry 421). Confidence: exact (compiler-generated).
// 0x4defc0  init_ui_listbox_2557068: ui_listbox::ui_listbox (0x52d5c0). Confidence: exact (compiler-generated).
// 0x4defd0  register_atexit_ui_listbox_2557068: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4defe0  atexit_dtor_ui_listbox_2557068: ~ui_listbox (0x431950). Confidence: exact (compiler-generated).
ui_listbox script_editor_load_listbox;               // 0x2557068

// 0x4deff0  static_init_ui_container_2557388 (static_initializers entry 422). Confidence: exact (compiler-generated).
// 0x4df000  init_ui_container_2557388: ui_container::ui_container (0x553560). Confidence: exact (compiler-generated).
// 0x4df010  register_atexit_ui_container_2557388: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df020  atexit_dtor_ui_container_2557388: ~ui_container = 0x4de800. Confidence: exact (compiler-generated).
ui_container script_editor_load_kind_container;      // 0x2557388

// 0x4df030  static_init_ui_radio_2565498_x2 (static_initializers entry 423). Confidence: exact (compiler-generated).
// 0x4df040  init_ui_radio_2565498_x2: ui_radio_button::ui_radio_button (0x4deeb0) x2. Confidence: exact (compiler-generated).
// 0x4df060  register_atexit_ui_radio_2565498_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df070  atexit_dtor_ui_radio_2565498_x2: ~ui_radio_button = 0x4de800. Confidence: exact (compiler-generated).
ui_radio_button script_editor_load_level_radio, script_editor_load_cutscene_radio;   // 0x2565498, 0x2552438

// 0x4df090  static_init_ui_button_2554900_x2 (static_initializers entry 424). Confidence: exact (compiler-generated).
// 0x4df0a0  init_ui_button_2554900_x2: ui_button::ui_button (0x526f70) x2. Confidence: exact (compiler-generated).
// 0x4df0c0  register_atexit_ui_button_2554900_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df0d0  atexit_dtor_ui_button_2554900_x2: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_load_ok_button, script_editor_load_cancel_button;   // 0x2554900, 0x254adc8

// 0x4df0f0  static_init_ui_window_254a8d8 (static_initializers entry 425). Confidence: exact (compiler-generated).
// 0x4df100  init_ui_window_254a8d8: ui_window::ui_window (0x52b020). Confidence: exact (compiler-generated).
// 0x4df110  register_atexit_ui_window_254a8d8: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df120  atexit_dtor_ui_window_254a8d8: ~ui_window (0x52b0a0). Confidence: exact (compiler-generated).
ui_window script_editor_ambient_window;              // 0x254a8d8

// 0x4df130  static_init_ui_inputbox_2554350 (static_initializers entry 426). Confidence: exact (compiler-generated).
// 0x4df140  init_ui_inputbox_2554350: ui_inputbox::ui_inputbox (0x5526b0). Confidence: exact (compiler-generated).
// 0x4df150  register_atexit_ui_inputbox_2554350: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df160  atexit_dtor_ui_inputbox_2554350: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_ambient_name_inputbox;     // 0x2554350

// 0x4df170  static_init_ui_listbox_2564a30 (static_initializers entry 427). Confidence: exact (compiler-generated).
// 0x4df180  init_ui_listbox_2564a30: ui_listbox::ui_listbox (0x52d5c0). Confidence: exact (compiler-generated).
// 0x4df190  register_atexit_ui_listbox_2564a30: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df1a0  atexit_dtor_ui_listbox_2564a30: ~ui_listbox (0x431950). Confidence: exact (compiler-generated).
ui_listbox script_editor_ambient_listbox;            // 0x2564a30

// 0x4df1b0  static_init_ui_static_255e230_x3 (static_initializers entry 428). Confidence: exact (compiler-generated).
// 0x4df1c0  init_ui_static_255e230_x3: ui_static::ui_static (0x527ea0) x3. Confidence: exact (compiler-generated).
// 0x4df1e0  register_atexit_ui_static_255e230_x3: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df1f0  atexit_dtor_ui_static_255e230_x3: ~ui_static (0x431780). Confidence: exact (compiler-generated).
ui_static script_editor_ambient_min_range_label, script_editor_ambient_volume_label,
          script_editor_ambient_rolloff_label;       // 0x255e230, 0x2563260, 0x255a050

// 0x4df210  static_init_ui_static_255e9d8 (static_initializers entry 429). Confidence: exact (compiler-generated).
// 0x4df220  init_ui_static_255e9d8: ui_static::ui_static (0x527ea0). Confidence: exact (compiler-generated).
// 0x4df230  register_atexit_ui_static_255e9d8: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df240  atexit_dtor_ui_static_255e9d8: ~ui_static (0x431780). Confidence: exact (compiler-generated).
ui_static script_editor_ambient_loop_delay_label;    // 0x255e9d8

// 0x4df250  static_init_ui_inputbox_256a878_x3 (static_initializers entry 430). Confidence: exact (compiler-generated).
// 0x4df260  init_ui_inputbox_256a878_x3: ui_inputbox::ui_inputbox (0x5526b0) x3. Confidence: exact (compiler-generated).
// 0x4df280  register_atexit_ui_inputbox_256a878_x3: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df290  atexit_dtor_ui_inputbox_256a878_x3: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_ambient_min_range_inputbox, script_editor_ambient_volume_inputbox,
            script_editor_ambient_rolloff_inputbox;  // 0x256a878, 0x25539d0, 0x2569ea0

// 0x4df2b0  static_init_ui_inputbox_255af28 (static_initializers entry 431). Confidence: exact (compiler-generated).
// 0x4df2c0  init_ui_inputbox_255af28: ui_inputbox::ui_inputbox (0x5526b0). Confidence: exact (compiler-generated).
// 0x4df2d0  register_atexit_ui_inputbox_255af28: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df2e0  atexit_dtor_ui_inputbox_255af28: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_ambient_loop_delay_inputbox;   // 0x255af28

// 0x4df2f0  static_init_ui_button_256a450_x2 (static_initializers entry 432). Confidence: exact (compiler-generated).
// 0x4df300  init_ui_button_256a450_x2: ui_button::ui_button (0x526f70) x2. Confidence: exact (compiler-generated).
// 0x4df320  register_atexit_ui_button_256a450_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df330  atexit_dtor_ui_button_256a450_x2: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_ambient_ok_button, script_editor_ambient_cancel_button;   // 0x256a450, 0x2556c40

// 0x4df350  static_init_ui_window_2550dd0 (static_initializers entry 433). Confidence: exact (compiler-generated).
// 0x4df360  init_ui_window_2550dd0: ui_window::ui_window (0x52b020). Confidence: exact (compiler-generated).
// 0x4df370  register_atexit_ui_window_2550dd0: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df380  atexit_dtor_ui_window_2550dd0: ~ui_window (0x52b0a0). Confidence: exact (compiler-generated).
ui_window script_editor_choice_window;               // 0x2550dd0

// 0x4df390  static_init_ui_button_2566eb0_x2 (static_initializers entry 434). Confidence: exact (compiler-generated).
// 0x4df3a0  init_ui_button_2566eb0_x2: ui_button::ui_button (0x526f70) x2. Confidence: exact (compiler-generated).
// 0x4df3c0  register_atexit_ui_button_2566eb0_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df3d0  atexit_dtor_ui_button_2566eb0_x2: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_choice_ok_button, script_editor_choice_cancel_button;   // 0x2566eb0, 0x25672d8

// 0x4df3f0  static_init_ui_listbox_255ff78 (static_initializers entry 435). Confidence: exact (compiler-generated).
// 0x4df400  init_ui_listbox_255ff78: ui_listbox::ui_listbox (0x52d5c0). Confidence: exact (compiler-generated).
// 0x4df410  register_atexit_ui_listbox_255ff78: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df420  atexit_dtor_ui_listbox_255ff78: ~ui_listbox (0x431950). Confidence: exact (compiler-generated).
ui_listbox script_editor_choice_listbox;             // 0x255ff78

// 0x4df430  static_init_ui_window_25629c8 (static_initializers entry 436). Confidence: exact (compiler-generated).
// 0x4df440  init_ui_window_25629c8: ui_window::ui_window (0x52b020). Confidence: exact (compiler-generated).
// 0x4df450  register_atexit_ui_window_25629c8: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df460  atexit_dtor_ui_window_25629c8: ~ui_window (0x52b0a0). Confidence: exact (compiler-generated).
ui_window script_editor_door_window;                 // 0x25629c8

// 0x4df470  static_init_ui_container_2565840 (static_initializers entry 437). Confidence: exact (compiler-generated).
// 0x4df480  init_ui_container_2565840: ui_container::ui_container (0x553560). Confidence: exact (compiler-generated).
// 0x4df490  register_atexit_ui_container_2565840: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df4a0  atexit_dtor_ui_container_2565840: ~ui_container = 0x4de800. Confidence: exact (compiler-generated).
ui_container script_editor_door_sound_container;     // 0x2565840

// 0x4df4b0  static_init_ui_radio_255a7d8_x2 (static_initializers entry 438). Confidence: exact (compiler-generated).
// 0x4df4c0  init_ui_radio_255a7d8_x2: ui_radio_button::ui_radio_button (0x4deeb0) x2. Confidence: exact (compiler-generated).
// 0x4df4e0  register_atexit_ui_radio_255a7d8_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df4f0  atexit_dtor_ui_radio_255a7d8_x2: ~ui_radio_button = 0x4de800. Confidence: exact (compiler-generated).
ui_radio_button script_editor_door_opening_radio, script_editor_door_closing_radio;   // 0x255a7d8, 0x255e600

// 0x4df510  static_init_ui_checkbox_2546828 (static_initializers entry 439). Confidence: exact (compiler-generated).
// 0x4df520  init_ui_checkbox_2546828: ui_checkbox::ui_checkbox (0x553ce0). Confidence: exact (compiler-generated).
// 0x4df530  register_atexit_ui_checkbox_2546828: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df540  atexit_dtor_ui_checkbox_2546828: ~ui_checkbox = 0x4de800. Confidence: exact (compiler-generated).
ui_checkbox script_editor_door_locked_checkbox;      // 0x2546828

// 0x4df550  static_init_ui_static_2553f80_x2 (static_initializers entry 440). Confidence: exact (compiler-generated).
// 0x4df560  init_ui_static_2553f80_x2: ui_static::ui_static (0x527ea0) x2. Confidence: exact (compiler-generated).
// 0x4df580  register_atexit_ui_static_2553f80_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df590  atexit_dtor_ui_static_2553f80_x2: ~ui_static (0x431780). Confidence: exact (compiler-generated).
ui_static script_editor_door_skill_label, script_editor_door_percent_label;   // 0x2553f80, 0x25666b8

// 0x4df5b0  static_init_ui_static_25612e0_x3 (static_initializers entry 441). Confidence: exact (compiler-generated).
// 0x4df5c0  init_ui_static_25612e0_x3: ui_static::ui_static (0x527ea0) x3. Confidence: exact (compiler-generated).
// 0x4df5e0  register_atexit_ui_static_25612e0_x3: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df5f0  atexit_dtor_ui_static_25612e0_x3: ~ui_static (0x431780). Confidence: exact (compiler-generated).
ui_static script_editor_door_min_range_label, script_editor_door_volume_label,
          script_editor_door_rolloff_label;          // 0x25612e0, 0x25512c0, 0x255eda8

// 0x4df610  static_init_ui_inputbox_25490d8_x3 (static_initializers entry 442). Confidence: exact (compiler-generated).
// 0x4df620  init_ui_inputbox_25490d8_x3: ui_inputbox::ui_inputbox (0x5526b0) x3. Confidence: exact (compiler-generated).
// 0x4df640  register_atexit_ui_inputbox_25490d8_x3: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df650  atexit_dtor_ui_inputbox_25490d8_x3: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_door_sound_inputbox, script_editor_door_skill_inputbox,
            script_editor_door_key_inputbox;         // 0x25490d8, 0x2550330, 0x2559688

// 0x4df670  static_init_ui_inputbox_255f9c8_x3 (static_initializers entry 443). Confidence: exact (compiler-generated).
// 0x4df680  init_ui_inputbox_255f9c8_x3: ui_inputbox::ui_inputbox (0x5526b0) x3. Confidence: exact (compiler-generated).
// 0x4df6a0  register_atexit_ui_inputbox_255f9c8_x3: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df6b0  atexit_dtor_ui_inputbox_255f9c8_x3: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_door_open_min_range_inputbox, script_editor_door_open_volume_inputbox,
            script_editor_door_open_rolloff_inputbox;   // 0x255f9c8, 0x25689a8, 0x255d6a8

// 0x4df6d0  static_init_ui_inputbox_2567a20_x3 (static_initializers entry 444). Confidence: exact (compiler-generated).
// 0x4df6e0  init_ui_inputbox_2567a20_x3: ui_inputbox::ui_inputbox (0x5526b0) x3. Confidence: exact (compiler-generated).
// 0x4df700  register_atexit_ui_inputbox_2567a20_x3: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df710  atexit_dtor_ui_inputbox_2567a20_x3: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_door_close_min_range_inputbox, script_editor_door_close_volume_inputbox,
            script_editor_door_close_rolloff_inputbox;  // 0x2567a20, 0x254a2f0, 0x25616b0

// 0x4df730  static_init_ui_listbox_2566398_x3 (static_initializers entry 445). Confidence: exact (compiler-generated).
// 0x4df740  init_ui_listbox_2566398_x3: ui_listbox::ui_listbox (0x52d5c0) x3. Confidence: exact (compiler-generated).
// 0x4df760  register_atexit_ui_listbox_2566398_x3: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df770  atexit_dtor_ui_listbox_2566398_x3: ~ui_listbox (0x431950). Confidence: exact (compiler-generated).
ui_listbox script_editor_door_open_sound_listbox, script_editor_door_close_sound_listbox,
           script_editor_door_key_listbox;           // 0x2566398, 0x2567700, 0x2569758

// 0x4df790  static_init_ui_button_2564118_x2 (static_initializers entry 446). Confidence: exact (compiler-generated).
// 0x4df7a0  init_ui_button_2564118_x2: ui_button::ui_button (0x526f70) x2. Confidence: exact (compiler-generated).
// 0x4df7c0  register_atexit_ui_button_2564118_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df7d0  atexit_dtor_ui_button_2564118_x2: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_door_ok_button, script_editor_door_cancel_button;   // 0x2564118, 0x254ff08

// 0x4df7f0  static_init_ui_window_254b1f0 (static_initializers entry 447). Confidence: exact (compiler-generated).
// 0x4df800  init_ui_window_254b1f0: ui_window::ui_window (0x52b020). Confidence: exact (compiler-generated).
// 0x4df810  register_atexit_ui_window_254b1f0: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df820  atexit_dtor_ui_window_254b1f0: ~ui_window (0x52b0a0). Confidence: exact (compiler-generated).
ui_window script_editor_door_lock_window;            // 0x254b1f0

// 0x4df830  static_init_ui_inputbox_2549d40 (static_initializers entry 448). Confidence: exact (compiler-generated).
// 0x4df840  init_ui_inputbox_2549d40: ui_inputbox::ui_inputbox (0x5526b0). Confidence: exact (compiler-generated).
// 0x4df850  register_atexit_ui_inputbox_2549d40: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df860  atexit_dtor_ui_inputbox_2549d40: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_door_lock_name_inputbox;   // 0x2549d40

// 0x4df870  static_init_ui_inputbox_255c500 (static_initializers entry 449). Confidence: exact (compiler-generated).
// 0x4df880  init_ui_inputbox_255c500: ui_inputbox::ui_inputbox (0x5526b0). Confidence: exact (compiler-generated).
// 0x4df890  register_atexit_ui_inputbox_255c500: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df8a0  atexit_dtor_ui_inputbox_255c500: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_door_lock_item_inputbox;   // 0x255c500

// 0x4df8b0  static_init_ui_checkbox_255b908 (static_initializers entry 450). Confidence: exact (compiler-generated).
// 0x4df8c0  init_ui_checkbox_255b908: ui_checkbox::ui_checkbox (0x553ce0). Confidence: exact (compiler-generated).
// 0x4df8d0  register_atexit_ui_checkbox_255b908: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df8e0  atexit_dtor_ui_checkbox_255b908: ~ui_checkbox = 0x4de800. Confidence: exact (compiler-generated).
ui_checkbox script_editor_door_lock_give_checkbox;   // 0x255b908

// 0x4df8f0  static_init_ui_button_255c0d8_x3 (static_initializers entry 451). Confidence: exact (compiler-generated).
// 0x4df900  init_ui_button_255c0d8_x3: ui_button::ui_button (0x526f70) x3. Confidence: exact (compiler-generated).
// 0x4df920  register_atexit_ui_button_255c0d8_x3: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df930  atexit_dtor_ui_button_255c0d8_x3: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
// init_ui_button_255c0d8_x3. The third button (0x255cab0) is constructed and destroyed
// but never created or referenced (a gadget the window lost).
ui_button script_editor_door_lock_ok_button, script_editor_door_lock_remove_button,
          script_editor_door_lock_unused_button;     // 0x255c0d8, 0x254e478, 0x255cab0

// 0x4df950  static_init_ui_listbox_2555218 (static_initializers entry 452). Confidence: exact (compiler-generated).
// 0x4df960  init_ui_listbox_2555218: ui_listbox::ui_listbox (0x52d5c0). Confidence: exact (compiler-generated).
// 0x4df970  register_atexit_ui_listbox_2555218: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df980  atexit_dtor_ui_listbox_2555218: ~ui_listbox (0x431950). Confidence: exact (compiler-generated).
ui_listbox script_editor_door_lock_item_listbox;     // 0x2555218

// 0x4df990  static_init_ui_inputbox_255dc80 (static_initializers entry 453). Confidence: exact (compiler-generated).
// 0x4df9a0  init_ui_inputbox_255dc80: ui_inputbox::ui_inputbox (0x5526b0). Confidence: exact (compiler-generated).
// 0x4df9b0  register_atexit_ui_inputbox_255dc80: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4df9c0  atexit_dtor_ui_inputbox_255dc80: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_door_lock_value_inputbox;  // 0x255dc80

// 0x4df9d0  static_init_ui_window_256b250 (static_initializers entry 454). Confidence: exact (compiler-generated).
// 0x4df9e0  init_ui_window_256b250: ui_window::ui_window (0x52b020). Confidence: exact (compiler-generated).
// 0x4df9f0  register_atexit_ui_window_256b250: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfa00  atexit_dtor_ui_window_256b250: ~ui_window (0x52b0a0). Confidence: exact (compiler-generated).
ui_window script_editor_creature_window;             // 0x256b250

// 0x4dfa10  static_init_ui_static_254fb38_x3 (static_initializers entry 455). Confidence: exact (compiler-generated).
// 0x4dfa20  init_ui_static_254fb38_x3: ui_static::ui_static (0x527ea0) x3. Confidence: exact (compiler-generated).
// 0x4dfa40  register_atexit_ui_static_254fb38_x3: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfa50  atexit_dtor_ui_static_254fb38_x3: ~ui_static (0x431780). Confidence: exact (compiler-generated).
ui_static script_editor_creature_team_label, script_editor_creature_level_label,
          script_editor_creature_info_label;         // 0x254fb38, 0x2559c38, 0x2555538

// 0x4dfa70  static_init_ui_container_2563d90 (static_initializers entry 456). Confidence: exact (compiler-generated).
// 0x4dfa80  init_ui_container_2563d90: ui_container::ui_container (0x553560). Confidence: exact (compiler-generated).
// 0x4dfa90  register_atexit_ui_container_2563d90: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfaa0  atexit_dtor_ui_container_2563d90: ~ui_container = 0x4de800. Confidence: exact (compiler-generated).
ui_container script_editor_creature_team_container;  // 0x2563d90

// 0x4dfab0  static_init_ui_radio_2565ff0_x4 (static_initializers entry 457). Confidence: exact (compiler-generated).
// 0x4dfac0  init_ui_radio_2565ff0_x4: ui_radio_button::ui_radio_button (0x4deeb0) x4. Confidence: exact (compiler-generated).
// 0x4dfaf0  register_atexit_ui_radio_2565ff0_x4: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfb00  atexit_dtor_ui_radio_2565ff0_x4: ~ui_radio_button = 0x4de800. Confidence: exact (compiler-generated).
ui_radio_button script_editor_creature_friendly_radio, script_editor_creature_neutral_radio,
                script_editor_creature_hostile_radio, script_editor_creature_evil_radio;   // 0x2565ff0, 0x255ab80, 0x2562eb8, 0x255d300

// 0x4dfb30  static_init_ui_inputbox_254d4e8 (static_initializers entry 458). Confidence: exact (compiler-generated).
// 0x4dfb40  init_ui_inputbox_254d4e8: ui_inputbox::ui_inputbox (0x5526b0). Confidence: exact (compiler-generated).
// 0x4dfb50  register_atexit_ui_inputbox_254d4e8: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfb60  atexit_dtor_ui_inputbox_254d4e8: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_creature_level_inputbox;   // 0x254d4e8

// 0x4dfb70  static_init_ui_button_2557710_x2 (static_initializers entry 459). Confidence: exact (compiler-generated).
// 0x4dfb80  init_ui_button_2557710_x2: ui_button::ui_button (0x526f70) x2. Confidence: exact (compiler-generated).
// 0x4dfba0  register_atexit_ui_button_2557710_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfbb0  atexit_dtor_ui_button_2557710_x2: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_creature_ok_button, script_editor_creature_cancel_button;   // 0x2557710, 0x254e8a0

// 0x4dfbd0  static_init_ui_window_2564540 (static_initializers entry 460). Confidence: exact (compiler-generated).
// 0x4dfbe0  init_ui_window_2564540: ui_window::ui_window (0x52b020). Confidence: exact (compiler-generated).
// 0x4dfbf0  register_atexit_ui_window_2564540: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfc00  atexit_dtor_ui_window_2564540: ~ui_window (0x52b0a0). Confidence: exact (compiler-generated).
ui_window script_editor_level_item_window;           // 0x2564540

// 0x4dfc10  static_init_ui_inputbox_2551e88_x2 (static_initializers entry 461). Confidence: exact (compiler-generated).
// 0x4dfc20  init_ui_inputbox_2551e88_x2: ui_inputbox::ui_inputbox (0x5526b0) x2. Confidence: exact (compiler-generated).
// 0x4dfc40  register_atexit_ui_inputbox_2551e88_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfc50  atexit_dtor_ui_inputbox_2551e88_x2: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
ui_inputbox script_editor_level_item_name_inputbox, script_editor_level_item_count_inputbox;   // 0x2551e88, 0x2561c60

// 0x4dfc70  static_init_ui_listbox_2549688 (static_initializers entry 462). Confidence: exact (compiler-generated).
// 0x4dfc80  init_ui_listbox_2549688: ui_listbox::ui_listbox (0x52d5c0). Confidence: exact (compiler-generated).
// 0x4dfc90  register_atexit_ui_listbox_2549688: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfca0  atexit_dtor_ui_listbox_2549688: ~ui_listbox (0x431950). Confidence: exact (compiler-generated).
ui_listbox script_editor_level_item_listbox;         // 0x2549688

// 0x4dfcb0  static_init_ui_static_254d118 (static_initializers entry 463). Confidence: exact (compiler-generated).
// 0x4dfcc0  init_ui_static_254d118: ui_static::ui_static (0x527ea0). Confidence: exact (compiler-generated).
// 0x4dfcd0  register_atexit_ui_static_254d118: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfce0  atexit_dtor_ui_static_254d118: ~ui_static (0x431780). Confidence: exact (compiler-generated).
ui_static script_editor_level_item_count_label;      // 0x254d118

// 0x4dfcf0  static_init_ui_button_2569a78_x2 (static_initializers entry 464). Confidence: exact (compiler-generated).
// 0x4dfd00  init_ui_button_2569a78_x2: ui_button::ui_button (0x526f70) x2. Confidence: exact (compiler-generated).
// 0x4dfd20  register_atexit_ui_button_2569a78_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfd30  atexit_dtor_ui_button_2569a78_x2: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_level_item_ok_button, script_editor_level_item_cancel_button;   // 0x2569a78, 0x2562210

// 0x4dfd50  static_init_ui_window_2553110 (static_initializers entry 465). Confidence: exact (compiler-generated).
// 0x4dfd60  init_ui_window_2553110: ui_window::ui_window (0x52b020). Confidence: exact (compiler-generated).
// 0x4dfd70  register_atexit_ui_window_2553110: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfd80  atexit_dtor_ui_window_2553110: ~ui_window (0x52b0a0). Confidence: exact (compiler-generated).
ui_window script_editor_params_window;               // 0x2553110

// 0x4dfd90  static_init_ui_static_2560298 (static_initializers entry 466). Confidence: exact (compiler-generated).
// 0x4dfda0  init_ui_static_2560298: ui_static::ui_static (0x527ea0). Confidence: exact (compiler-generated).
// 0x4dfdb0  register_atexit_ui_static_2560298: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfdc0  atexit_dtor_ui_static_2560298: ~ui_static (0x431780). Confidence: exact (compiler-generated).
ui_static script_editor_params_title;                // 0x2560298

// 0x4dfdd0  static_init_ui_static_array_2558388 (static_initializers entry 467). Confidence: exact (compiler-generated).
// 0x4dfde0  init_ui_static_array_2558388: ui_static::ui_static (0x527ea0). Confidence: exact (compiler-generated).
// 0x4dfe00  register_atexit_ui_static_array_2558388: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfe10  atexit_dtor_ui_static_array_2558388: ~ui_static (0x431780). Confidence: exact (compiler-generated).
// init_ui_static_array_2558388: `vector constructor iterator` (0x56db8c) and
// `vector destructor iterator` (0x56dc0e) over five ui_static.
ui_static script_editor_param_labels[5];             // 0x2558388

// 0x4dfe30  static_init_ui_array_2547468 (static_initializers entry 468). Confidence: exact (compiler-generated).
// 0x4dfe40  init_ui_array_2547468: ui_inputbox::ui_inputbox (0x5526b0). Confidence: exact (compiler-generated).
// 0x4dfe60  register_atexit_ui_array_2547468: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfe70  atexit_dtor_ui_array_2547468: ~ui_inputbox = 0x4de800. Confidence: exact (compiler-generated).
// init_ui_array_2547468: the same over five ui_inputbox (destructor 0x4de800).
ui_inputbox script_editor_param_inputboxes[5];       // 0x2547468

// 0x4dfe90  static_init_ui_button_2556688_x2 (static_initializers entry 469). Confidence: exact (compiler-generated).
// 0x4dfea0  init_ui_button_2556688_x2: ui_button::ui_button (0x526f70) x2. Confidence: exact (compiler-generated).
// 0x4dfec0  register_atexit_ui_button_2556688_x2: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dfed0  atexit_dtor_ui_button_2556688_x2: ~ui_button (0x4317e0). Confidence: exact (compiler-generated).
ui_button script_editor_params_ok_button, script_editor_params_cancel_button;   // 0x2556688, 0x2568580

// 0x4dfef0  static_init_empty_4dff00 (static_initializers entry 470). Confidence: exact (compiler-generated).
// 0x4dff00  init_empty_4dff00. Confidence: unsure (which object it constructs).
// static_init_empty_4dff00 / init_empty_4dff00 (the sixth empty initialiser).
// Confidence: unsure (which object).
matrix script_editor_saved_orient;                   // 0x254a8b0

// 0x4dff10  static_init_alloc_pool_255dc60 (static_initializers entry 471). Confidence: exact (compiler-generated).
// 0x4dff20  init_alloc_pool_255dc60. Confidence: exact (compiler-generated).
// 0x4dff30  register_atexit_alloc_pool_255dc60: atexit(the handler). Confidence: exact (compiler-generated).
// 0x4dff40  atexit_dtor_alloc_pool_255dc60. Confidence: exact (compiler-generated).
// script_editor_alloc_pool: NoRlsAllocPool() (0x532360), atexit ~NoRlsAllocPool
// (0x532380).
NoRlsAllocPool script_editor_alloc_pool;             // 0x255dc60

// ---- the rest of .bss (no initialisers) ------------------------------------------------------------------
char script_editor_alloc_pool_buffer[0x800];         // 0x2568f58
char script_editor_status_text[256];                 // 0x254ecc8
bool script_editor_camera_held;                      // 0x256b73d
float script_editor_pick_distance;                   // 0x255dc58
int script_editor_pick_filter;                       // 0x2566a84
int script_editor_mode;                              // 0x25629c0
// Box per char_info size class: {2 r, height, 2 r} (script_editor_create_ui). A plain float array: as
// vector3 objects they would need a seventh empty initialiser (unsure).
float script_editor_char_sizes[4][3];                // 0x255e9a8
float script_editor_char_size_radii[4];              // 0x255a7c4
bool script_editor_ui_created;                       // 0x256b73c
script_object *script_editor_cur_script;             // 0x256b740
ui_listbox_entry *script_editor_cur_action;          // 0x256b744
int script_editor_sel_action_index;                  // 0x256b74c
bool script_editor_lists_dirty;                      // 0x256b749
bool Render_s3d_navpoints;                           // 0x256b74a
int script_editor_action_list_kind;                  // 0x254da98
int script_editor_param_field_count;                 // 0x256325c
int script_editor_new_selection;                     // 0x256362c
bool script_ambients_dirty;                          // 0x255b4d8
bool script_level_items_dirty;                       // 0x2563d88
bool script_editor_insert_action;                    // 0x256b748

// ---- code ------------------------------------------------------------------------------------------------

// 0x4dff50  Confidence: exact (strcmp expanded inline). qsort comparator of two string pointers.
int qsort_cmp_strptr(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

// 0x4dff90  Confidence: exact. The navpoint is c's start navpoint or the navpoint of one of the actions
// of c's level script. Summons (+0x320 bit 0x100), party members (type 7) and serpent parts never count.
bool script_character_uses_navpoint(Character *c, navpoint *np)
{
    script_object *so = NULL;
    int i;

    if (c->flags & 0x100)
        return false;
    if (c->type == 7)
        return false;
    if (c->info->species == 0x59 && character_is_serpent_part(c))
        return false;
    for (i = 0; i < script_object_count; i++) {
        if (script_objects[i].handle == c->handle) {
            so = &script_objects[i];
            break;
        }
    }
    if (c->start_navpoint == NULL)
        return false;
    if (c->start_navpoint == np)
        return true;
    if (so == NULL)
        return false;
    for (i = 0; i < so->num_actions; i++) {
        if (so->actions[i].nav == np)
            return true;
    }
    return false;
}

// 0x4e0040  static_init_ccmd_remove_nav (entry 472, jmp). Confidence: exact (compiler-generated).
// 0x4e0050  init_ccmd_remove_nav. Confidence: exact (compiler-generated).
console_command ccmd_remove_nav("remove_nav", "Remove redundant $npc navpoints", cmd_remove_nav);   // 0x254a8a0

// 0x4e0070  Confidence: exact (a tail jump).
void cmd_remove_nav()
{
    script_remove_redundant_npc_navpoints();
}

// 0x4e0080  Confidence: exact (the element pointer taken from the index, D8701). Every "$npc" level
// navpoint that no live or inactive character uses loses its name (it is emptied, not removed: the
// record stays in the table and is still written by script_write_level_script with an empty name).
void script_remove_redundant_npc_navpoints()
{
    int i;
    navpoint *np;
    Character *c;
    bool used;

    for (i = 0; i < navpoint_count; i++) {
        np = &navpoints[i];
        used = false;
        if (_strnicmp(np->name, "$npc", 4) != 0)
            continue;
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (script_character_uses_navpoint(c, np)) {
                used = true;
                break;
            }
        }
        for (c = living_list_inactive.live_next; c != &living_list_inactive; c = c->live_next) {
            if (script_character_uses_navpoint(c, np)) {
                used = true;
                break;
            }
        }
        if (!used)
            strcpy(np->name, "");
    }
}

// 0x4e0160  Confidence: exact. The character filter of the decimal input boxes (ui_inputbox +0x5ac): one
// decimal point, one digit after it.
bool script_editor_decimal_filter_cb(const char *text, char c)
{
    const char *dot = strchr(text, '.');

    if (dot != NULL) {
        if (c == '.')
            return false;
        if (strlen(dot) > 1)
            return false;
    }
    return true;
}

// 0x4e01a0 script_editor_no_window_open (declared in summoner/summoner.h). Confidence: exact.
bool script_editor_no_window_open()
{
    if (script_editor_action_window.is_disabled() && script_editor_save_window.is_disabled()
        && script_editor_load_window.is_disabled() && script_editor_params_window.is_disabled()
        && script_editor_new_window.is_disabled())
        return true;
    return false;
}

// The marker colour helper of script_editor_frame: one box arrow of a level object (the size is its
// bounding box). Written out in the binary each time; a macro here.
#define SE_DRAW_LOBJ_ARROW(l, mode)                                         \
    do {                                                                    \
        vector3 size_ = (l)->bbox_max - (l)->bbox_min;                      \
        gr_draw_arrow_3d(&(l)->pos, &(l)->orient, &size_, (mode));          \
    } while (0)

// 0x4e01f0 script_editor_frame (declared in summoner/summoner.h). Confidence: likely (the arrow blocks:
// each recomputes the box size; the order of the two draws per block is from the pushes). Game bug: the
// hover marker is drawn as a level object when the SELECTION is an object (it tests script_editor_sel.kind,
// then reads the hover object's +0x14 / +0x20 / +0x50 / +0x5c), so hovering a character or a navpoint while
// an object is selected draws a box from the wrong fields.
void script_editor_frame()
{
    int i;

    if (!(game_flags & 0x80)) {
        script_editor_close();
        return;
    }
    if (!script_editor_ui_created)
        script_editor_create_ui();
    gr_screen_to_world_ray(&script_editor_mouse_ray, (float)game_mouse_x, (float)game_mouse_y);
    script_editor_pick_hover();
    script_editor_update_buttons();
    script_editor_handle_input();

    if (script_editor_mode == SE_MODE_CLICK_OBJECT) {
        for (i = 0; i < lc_entry_count; i++) {
            gr_set_color(0xff, 0, 0, 0xa0);
            lobj *l = lc_entries[i].object;
            if (l != NULL)
                SE_DRAW_LOBJ_ARROW(l, gr_line_mode);
        }
    }

    if (script_editor_hover.kind != SE_PICK_NONE) {
        vector3 p = script_editor_hover.pos
                  + script_editor_hover.orient.uvec * (script_editor_hover.size.y * 0.5f);
        if (script_editor_sel.kind == SE_PICK_OBJECT) {
            lobj *l = (lobj *)script_editor_hover.object;
            gr_set_color(0xff, 0xff, 0xff, 0x80);
            SE_DRAW_LOBJ_ARROW(l, gr_line_mode);
            gr_set_color(0xff, 0xff, 0xff, 0x20);
            SE_DRAW_LOBJ_ARROW(l, gr_line_3d_mode);
        } else {
            gr_set_color(0xff, 0xff, 0xff, 0x80);
            gr_draw_arrow_3d(&p, &script_editor_hover.orient, &script_editor_hover.size, gr_line_3d_mode);
            gr_set_color(0xff, 0xff, 0xff, 0x20);
            gr_draw_arrow_3d(&p, &script_editor_hover.orient, &script_editor_hover.size, gr_line_mode);
        }
    }

    if (script_editor_sel.kind != SE_PICK_NONE) {
        vector3 p = script_editor_sel.pos + script_editor_sel.orient.uvec * (script_editor_sel.size.y * 0.5f);
        if (script_editor_sel.kind == SE_PICK_OBJECT) {
            lobj *l = (lobj *)script_editor_sel.object;
            gr_set_color(0xff, 0xff, 0, 0xa0);
            SE_DRAW_LOBJ_ARROW(l, gr_line_mode);
            gr_set_color(0xff, 0xff, 0, 0x30);
            SE_DRAW_LOBJ_ARROW(l, gr_line_3d_mode);
        } else {
            gr_set_color(0xff, 0xff, 0, 0xa0);
            gr_draw_arrow_3d(&p, &script_editor_sel.orient, &script_editor_sel.size, gr_line_3d_mode);
            gr_set_color(0xff, 0xff, 0, 0x30);
            gr_draw_arrow_3d(&p, &script_editor_sel.orient, &script_editor_sel.size, gr_line_mode);
        }
    }

    if (script_editor_sel.kind == SE_PICK_CHARACTER)
        game_camera.set_target((Character *)script_editor_sel.object);
    else if (script_editor_sel.kind == SE_PICK_NONE)
        game_camera.set_target(cur_character);

    script_editor_draw_navpoints();
    ui_draw(NULL);
    gr_set_color(0, 0xa0, 0x78, 0xff);
    if (script_editor_cutscene_button.is_latched())
        sprintf(string_buffer, "Cutscene time: %.1f", cutscene_time);
    else
        sprintf(string_buffer, "Level time: %.1f", level_session_time);
    gr_string(2, 0x195, string_buffer, -1, gr_string_mode);
    if (!script_editor_status_timer.elapsed()) {
        gr_set_color(0xa0, 0xf0, 0xa0, 0xff);
        gr_string(2, 2, script_editor_status_text, -1, gr_string_mode);
    }
}

// 0x4e0690  Confidence: likely (the order of the callback and font stores of each gadget: VC6 schedules the
// two stores around the next call's pushes, sometimes font first; every argument, text, id, hotkey,
// flag and the gadget order match). Builds every window and gadget once (script_editor_frame, first
// editor frame). Odd details kept from the binary: the ambient name box's font is set twice (the second
// store, after the list box, was probably meant for the list box), and the door lock window's first two
// input boxes both get id 3.
void script_editor_create_ui()
{
    int i, y, id;
    ui_listbox_entry *e;

    script_editor_alloc_pool.Init(script_editor_alloc_pool_buffer, 0x800);
    script_editor_mode = SE_MODE_CREATURE;
    script_editor_pick_filter = 1;
    for (i = 0; i < 4; i++) {
        script_editor_char_sizes[i][0] = script_editor_char_sizes[i][2] = character_size_radii[i] * 2.0f;
        script_editor_char_sizes[i][1] = character_size_heights[i];
        script_editor_char_size_radii[i] = (float)sqrt(
            script_editor_char_sizes[i][0] * script_editor_char_sizes[i][0]
            + script_editor_char_sizes[i][2] * script_editor_char_sizes[i][2]
            + script_editor_char_sizes[i][1] * script_editor_char_sizes[i][1]);
    }
    script_editor_status_timer.SetTimeout(0);
    ui_push_group();

    script_editor_level_entry_colors[0].set(0, 0xa0, 0xa0, 0xff);
    script_editor_level_entry_colors[1].set(0, 0, 0, 0xff);
    script_editor_level_entry_colors[2].set(0xaa, 0xaa, 0, 0xff);
    script_editor_level_entry_colors[3].set(0, 0, 0, 0xff);
    script_editor_level_entry_colors[4].set(0x8c, 0x8c, 0, 0xff);
    script_editor_level_entry_colors[5].set(0, 0, 0, 0xff);
    script_editor_cutscene_entry_colors[0].set(0x28, 0xa0, 0x28, 0xff);
    script_editor_cutscene_entry_colors[1].set(0, 0, 0, 0xff);
    script_editor_cutscene_entry_colors[2].set(0xaa, 0xaa, 0, 0xff);
    script_editor_cutscene_entry_colors[3].set(0, 0, 0, 0xff);
    script_editor_cutscene_entry_colors[4].set(0x8c, 0x8c, 0, 0xff);
    script_editor_cutscene_entry_colors[5].set(0, 0, 0, 0xff);

    // The main bar (640 x 60 at y 420).
    script_editor_window.init(0, 0x1a4, 0x280, 0x3c, UI_NO_ID, 1);
    script_editor_mode_button.create(&script_editor_window, 0x1f4, 3, 0x87, 0x10, 0, 0, false);
    script_editor_mode_button.set_text(script_editor_mode_names[script_editor_mode]);
    script_editor_mode_button.callback = script_editor_mode_button_cb;
    script_editor_mode_button.font = font_small;

    script_editor_pos_button.create(&script_editor_window, 0x1f4, 0x16, 0x2a, 0x10, 1, 0, false);
    script_editor_pos_button.set_text("Pos");
    script_editor_pos_button.set_hotkey(0x10, 0);
    script_editor_pos_button.callback = script_editor_on_pos_ori;
    script_editor_pos_button.font = font_small;

    script_editor_ori_button.create(&script_editor_window, 0x223, 0x16, 0x2a, 0x10, 2, 0, false);
    script_editor_ori_button.set_text("Ori");
    script_editor_ori_button.set_hotkey(0x11, 0);
    script_editor_ori_button.callback = script_editor_on_pos_ori;
    script_editor_ori_button.font = font_small;

    script_editor_y_button.create(&script_editor_window, 0x251, 0x16, 0x2a, 0x10, 3, 0, false);
    script_editor_y_button.set_text("Y");
    script_editor_y_button.set_hotkey(0x12, 0);
    script_editor_y_button.callback = script_editor_on_pos_ori;
    script_editor_y_button.font = font_small;

    script_editor_new_button.create(&script_editor_window, 0x1f4, 0x29, 0x41, 0x10, 4, 0, false);
    script_editor_new_button.set_text("New");
    script_editor_new_button.set_hotkey(0xd2, 0);
    script_editor_new_button.set_hotkey(0x2e, 1);
    script_editor_new_button.callback = script_editor_on_new;
    script_editor_new_button.font = font_small;

    script_editor_delete_button.create(&script_editor_window, 0x23a, 0x29, 0x41, 0x10, 5, 0, false);
    script_editor_delete_button.set_text("Del");
    script_editor_delete_button.set_hotkey(0xd3, 0);
    script_editor_delete_button.set_hotkey(0x20, 1);
    script_editor_delete_button.callback = script_editor_on_delete;
    script_editor_delete_button.font = font_small;

    script_editor_save_button.create(&script_editor_window, 3, 0x16, 0x32, 0x10, 6, 0, false);
    script_editor_save_button.set_text("Save");
    script_editor_save_button.set_hotkey(KEY_CTRLED | 0x1f, 0);
    script_editor_save_button.callback = script_editor_save_button_cb;
    script_editor_save_button.font = font_small;

    script_editor_load_button.create(&script_editor_window, 0x38, 0x16, 0x32, 0x10, 7, 0, false);
    script_editor_load_button.set_text("Load");
    script_editor_load_button.set_hotkey(KEY_CTRLED | 0x26, 0);
    script_editor_load_button.callback = script_editor_on_load;
    script_editor_load_button.font = font_small;

    script_editor_pause_button.create(&script_editor_window, 3, 0x29, 0x32, 0x10, 8, 0, false);
    script_editor_pause_button.set_text("Pause");
    script_editor_pause_button.set_hotkey(0x39, 0);
    script_editor_pause_button.callback = script_editor_on_pause;
    script_editor_pause_button.font = font_small;

    script_editor_time_back_button.create(&script_editor_window, 0x38, 0x29, 0x32, 0x10, 9, 0, false);
    script_editor_time_back_button.set_text("< Time");
    script_editor_time_back_button.set_hotkey(0x0e, 0);
    script_editor_time_back_button.callback = script_editor_on_time_back;
    script_editor_time_back_button.font = font_small;

    script_editor_name_inputbox.create(&script_editor_window, 3, 3, 0x67, 0x10, 0xa, 2, false);
    script_editor_name_inputbox.callback = script_editor_name_inputbox_cb;
    script_editor_name_inputbox.font = font_small;

    script_editor_prev_button.create(&script_editor_window, 0x177, 0x16, 0x37, 0x10, 0xb, 0, false);
    script_editor_prev_button.set_text("< Prev");
    script_editor_prev_button.set_hotkey(0x33, 0);
    script_editor_prev_button.callback = script_editor_on_prev_next;
    script_editor_prev_button.font = font_small;

    script_editor_next_button.create(&script_editor_window, 0x1b3, 0x16, 0x37, 0x10, 0xc, 0, false);
    script_editor_next_button.set_text("Next >");
    script_editor_next_button.set_hotkey(0x34, 0);
    script_editor_next_button.callback = script_editor_on_prev_next;
    script_editor_next_button.font = font_small;

    script_editor_cutscene_button.create(&script_editor_window, 0x177, 0x29, 0x73, 0x10, 0xd, 0x400, false);
    script_editor_cutscene_button.set_text("Cutscene Script");
    script_editor_cutscene_button.callback = script_editor_cutscene_button_cb;
    script_editor_cutscene_button.font = font_small;
    script_editor_sync_cutscene_button();

    // The selection panel: the action list and its buttons, shown for a selected character.
    script_editor_selection_panel.create(&script_editor_window, 0, 0, 0x280, 0x1a4, 0xe, 0, false);
    script_editor_action_listbox.create(&script_editor_window, 0x6f, 3, 0xc8, 0x37, 0, 0, 0xf, 2, false);
    script_editor_action_listbox.set_parent(&script_editor_selection_panel);
    script_editor_action_listbox.entry_spacing = 1;

    script_editor_append_action_button.create(&script_editor_window, 0x14a, 3, 0x11, 0x10, 0x10, 0, false);
    script_editor_append_action_button.set_parent(&script_editor_selection_panel);
    script_editor_append_action_button.set_text("A");
    script_editor_append_action_button.set_hotkey(KEY_CTRLED | 0x1e, 0);
    script_editor_append_action_button.callback = script_editor_add_action_cb;
    script_editor_append_action_button.font = font_small;

    script_editor_insert_action_button.create(&script_editor_window, 0x15e, 3, 0x11, 0x10, 0x11, 0, false);
    script_editor_insert_action_button.set_parent(&script_editor_selection_panel);
    script_editor_insert_action_button.set_text("I");
    script_editor_insert_action_button.callback = script_editor_add_action_cb;
    script_editor_insert_action_button.font = font_small;

    script_editor_delete_action_button.create(&script_editor_window, 0x14a, 0x16, 0x25, 0x10, 0x12, 0, false);
    script_editor_delete_action_button.set_parent(&script_editor_selection_panel);
    script_editor_delete_action_button.set_text("Del");
    script_editor_delete_action_button.callback = script_editor_delete_action_cb;
    script_editor_delete_action_button.font = font_small;

    script_editor_action_up_button.create(&script_editor_window, 0x14a, 0x29, 0x11, 0x10, 0x13, 0x40, false);
    script_editor_action_up_button.set_parent(&script_editor_selection_panel);
    script_editor_action_up_button.callback = script_editor_action_up_cb;
    script_editor_action_up_button.set_hotkey(KEY_SHIFTED | 0xc8, 0);
    script_editor_action_up_button.font = font_small;

    script_editor_action_down_button.create(&script_editor_window, 0x15e, 0x29, 0x11, 0x10, 0x14, 0x80, false);
    script_editor_action_down_button.set_parent(&script_editor_selection_panel);
    script_editor_action_down_button.callback = script_editor_action_down_cb;
    script_editor_action_down_button.set_hotkey(KEY_SHIFTED | 0xd0, 0);
    script_editor_action_down_button.font = font_small;

    script_editor_change_button.create(&script_editor_window, 0x177, 3, 0x37, 0x10, 0x15, 0, false);
    script_editor_change_button.set_parent(&script_editor_selection_panel);
    script_editor_change_button.set_text("Change");
    script_editor_change_button.callback = script_editor_change_cb;
    script_editor_change_button.font = font_small;

    script_editor_edit_button.create(&script_editor_window, 0x1b3, 3, 0x37, 0x10, 0x16, 2, false);
    script_editor_edit_button.set_parent(&script_editor_selection_panel);
    script_editor_edit_button.set_text("Edit");
    script_editor_edit_button.set_hotkey(KEY_CTRLED | 0x12, 0);
    script_editor_edit_button.font = font_small;
    script_editor_edit_button.callback = script_editor_edit_button_cb;

    // New: the character list.
    script_editor_new_window.init(0x1ae, 3, 0xc8, 0x1a0, UI_NO_ID, 0x22);
    script_editor_new_window.set_disabled(true);
    script_editor_new_ok_button.create(&script_editor_new_window, 7, 0x189, 0x2f, 0x10, 0, 0, false);
    script_editor_new_ok_button.set_text("Ok");
    script_editor_new_ok_button.set_hotkey(KEY_ENTER, 0);
    script_editor_new_ok_button.callback = script_editor_new_ok_cb;
    script_editor_new_ok_button.font = font_small;
    script_editor_new_cancel_button.create(&script_editor_new_window, 0x3b, 0x189, 0x86, 0x10, 1, 0, false);
    script_editor_new_cancel_button.set_text("Cancel");
    script_editor_new_cancel_button.set_hotkey(KEY_ESC, 0);
    script_editor_new_cancel_button.callback = script_editor_new_cancel_cb;
    script_editor_new_cancel_button.font = font_small;
    script_editor_new_listbox.create(&script_editor_new_window, 7, 7, 0xae, 0x17e, 0, 0, 2, 0x42, false);
    script_editor_new_listbox.callback = script_editor_new_listbox_cb;
    for (i = 0; i < num_char_infos; i++) {
        e = script_editor_new_listbox.add_entry(char_infos[i].name, -1, 0, 0, 0);
        e->data = i;
        e->set_font(font_small);
    }

    // The action type list (filled by script_editor_fill_action_lists).
    script_editor_action_window.init(0x14a, 3, 0x89, 0x1a0, UI_NO_ID, 0x22);
    script_editor_action_window.callback = script_editor_action_window_cb;
    script_editor_action_window.set_disabled(true);
    script_editor_action_ok_button.create(&script_editor_action_window, 7, 0x189, 0x2f, 0x10, 0, 0, false);
    script_editor_action_ok_button.set_text("Ok");
    script_editor_action_ok_button.set_hotkey(KEY_ENTER, 0);
    script_editor_action_ok_button.font = font_small;
    script_editor_action_cancel_button.create(&script_editor_action_window, 0x3b, 0x189, 0x47, 0x10, 1, 0, false);
    script_editor_action_cancel_button.set_text("Cancel");
    script_editor_action_cancel_button.set_hotkey(KEY_ESC, 0);
    script_editor_action_cancel_button.font = font_small;
    script_editor_action_type_listbox.create(&script_editor_action_window, 7, 7, 0x6f, 0x17e, 0, 0, 2, 0x12, false);
    script_editor_action_type_listbox.entry_spacing = 1;

    // Save.
    script_editor_save_window.init(0x82, 0x96, 0x17c, 0x64, UI_NO_ID, 0x22);
    script_editor_save_window.set_disabled(true);
    script_editor_save_name_label.create(&script_editor_save_window, 0xa, 5, 0x5a, 0x10, 0, false);
    script_editor_save_name_label.set_text("Script name:");
    script_editor_save_name_label.font = font_small;
    script_editor_save_name_inputbox.create(&script_editor_save_window, 0x5f, 5, 0x113, 0x10, 1, 0, false);
    script_editor_save_name_inputbox.max_length = 0x20 - strlen("_script.tbl");
    script_editor_save_name_inputbox.set_text(level_script_name ? level_script_name : level_filename);
    script_editor_save_name_inputbox.set_valid_chars("A-Za-z0-9$_-");
    script_editor_save_name_inputbox.callback = script_editor_save_name_cb;
    script_editor_save_name_inputbox.set_hidden(true);
    script_editor_save_name_inputbox.font = font_small;
    script_editor_save_filename_label.create(&script_editor_save_window, 0xa, 0x19, 0x4b, 0x10, 2, false);
    script_editor_save_filename_label.set_text("Filename:");
    script_editor_save_filename_label.font = font_small;
    script_editor_save_filename_inputbox.create(&script_editor_save_window, 0x5f, 0x19, 0x113, 0x10, 3, 0, false);
    script_editor_save_filename_inputbox.set_hidden(true);
    script_editor_save_filename_inputbox.font = font_small;
    script_editor_save_kind_container.create(&script_editor_save_window, 0x5f, 0x2f, 0x113, 0x17, 4, 0, false);
    script_editor_save_level_radio.create(&script_editor_save_kind_container, 0, 0, 10, 10, "Level script",
                                          3, 0, font_small, 5, false);
    script_editor_save_level_radio.callback = script_editor_save_kind_radio_cb;
    script_editor_save_cutscene_radio.create(&script_editor_save_kind_container, 0x8c, 0, 10, 10,
                                             "Cutscene script", 3, 0, font_small, 6, false);
    script_editor_save_cutscene_radio.callback = script_editor_save_kind_radio_cb;
    script_editor_save_ok_button.create(&script_editor_save_window, 0xff, 0x4b, 0x37, 0x10, 7, 0, false);
    script_editor_save_ok_button.set_text("Save");
    script_editor_save_ok_button.set_hotkey(KEY_ENTER, 0);
    script_editor_save_ok_button.font = font_small;
    script_editor_save_ok_button.callback = script_editor_save_ok_cb;
    script_editor_save_cancel_button.create(&script_editor_save_window, 0x13b, 0x4b, 0x37, 0x10, 8, 0, false);
    script_editor_save_cancel_button.set_text("Cancel");
    script_editor_save_cancel_button.set_hotkey(KEY_ESC, 0);
    script_editor_save_cancel_button.callback = script_editor_save_cancel_cb;
    script_editor_save_cancel_button.font = font_small;

    // Load.
    script_editor_load_window.init(0x82, 0x7d, 0x17c, 0x96, UI_NO_ID, 0x22);
    script_editor_load_window.set_disabled(true);
    script_editor_load_name_inputbox.create(&script_editor_load_window, 0xa, 5, 0x168, 0x10, 0, 0, false);
    script_editor_load_name_inputbox.callback = script_editor_script_name_cb;
    script_editor_load_name_inputbox.font = font_small;
    script_editor_load_listbox.create(&script_editor_load_window, 0xa, 0x19, 0x15c, 0x45, 0, 0, 1, 0x42, false);
    script_editor_load_listbox.entry_spacing = 1;
    script_editor_load_listbox.callback = script_editor_load_listbox_cb;
    script_editor_load_kind_container.create(&script_editor_load_window, 0xa, 0x61, 0x168, 0x17, 2, 0, false);
    script_editor_load_level_radio.create(&script_editor_load_kind_container, 0, 0, 10, 10, "Level scripts",
                                          3, 0, font_small, 3, false);
    script_editor_load_level_radio.callback = script_editor_load_kind_radio_cb;
    script_editor_load_cutscene_radio.create(&script_editor_load_kind_container, 0xb4, 0, 10, 10,
                                             "Cutscene scripts", 3, 0, font_small, 4, false);
    script_editor_load_cutscene_radio.callback = script_editor_load_kind_radio_cb;
    script_editor_load_ok_button.create(&script_editor_load_window, 0xff, 0x7d, 0x37, 0x10, 5, 0, false);
    script_editor_load_ok_button.set_text("Load");
    script_editor_load_ok_button.callback = script_editor_load_ok_cb;
    script_editor_load_ok_button.font = font_small;
    script_editor_load_cancel_button.create(&script_editor_load_window, 0x13b, 0x7d, 0x37, 0x10, 6, 0, false);
    script_editor_load_cancel_button.set_text("Cancel");
    script_editor_load_cancel_button.font = font_small;
    script_editor_load_cancel_button.callback = script_editor_load_cancel_cb;

    // Ambient sound.
    script_editor_ambient_window.init(0x82, 0x7d, 0x17c, 0x96, UI_NO_ID, 0x22);
    script_editor_ambient_window.set_disabled(true);
    script_editor_ambient_name_inputbox.create(&script_editor_ambient_window, 0xa, 5, 0x168, 0x10, 0, 0, false);
    script_editor_ambient_name_inputbox.callback = script_editor_ambient_name_cb;
    script_editor_ambient_name_inputbox.font = font_small;
    script_editor_ambient_listbox.create(&script_editor_ambient_window, 0xa, 0x19, 0x15c, 0x45, 0, 0, 1, 0x42, false);
    script_editor_ambient_listbox.entry_spacing = 1;
    script_editor_ambient_listbox.callback = script_editor_ambient_listbox_cb;
    script_editor_ambient_name_inputbox.font = font_small;
    script_editor_ambient_min_range_label.create(&script_editor_ambient_window, 0xa, 0x61, 0x46, 0x10, 2, false);
    script_editor_ambient_min_range_label.set_text("Min range:");
    script_editor_ambient_min_range_label.font = font_small;
    script_editor_ambient_min_range_label.h_justify = 2;
    script_editor_ambient_min_range_inputbox.create(&script_editor_ambient_window, 0x55, 0x61, 0x32, 0x10, 3, 0, false);
    script_editor_ambient_min_range_inputbox.set_valid_chars("01234567890.");
    script_editor_ambient_min_range_inputbox.callback = script_editor_ambient_field_cb;
    script_editor_ambient_min_range_inputbox.char_filter = script_editor_decimal_filter_cb;
    script_editor_ambient_min_range_inputbox.font = font_small;
    script_editor_ambient_volume_label.create(&script_editor_ambient_window, 0x76, 0x61, 0x46, 0x10, 4, false);
    script_editor_ambient_volume_label.set_text("Volume:");
    script_editor_ambient_volume_label.h_justify = 2;
    script_editor_ambient_volume_label.font = font_small;
    script_editor_ambient_volume_inputbox.create(&script_editor_ambient_window, 0xc1, 0x61, 0x32, 0x10, 5, 0, false);
    script_editor_ambient_volume_inputbox.set_valid_chars("01234567890.");
    script_editor_ambient_volume_inputbox.font = font_small;
    script_editor_ambient_volume_inputbox.callback = script_editor_ambient_field_cb;
    script_editor_ambient_volume_inputbox.char_filter = script_editor_decimal_filter_cb;
    script_editor_ambient_rolloff_label.create(&script_editor_ambient_window, 0xf5, 0x61, 0x46, 0x10, 6, false);
    script_editor_ambient_rolloff_label.set_text("Roll-off:");
    script_editor_ambient_rolloff_label.h_justify = 2;
    script_editor_ambient_rolloff_label.font = font_small;
    script_editor_ambient_rolloff_inputbox.create(&script_editor_ambient_window, 0x140, 0x61, 0x32, 0x10, 7, 0, false);
    script_editor_ambient_rolloff_inputbox.set_valid_chars("01234567890.");
    script_editor_ambient_rolloff_inputbox.callback = script_editor_ambient_field_cb;
    script_editor_ambient_rolloff_inputbox.char_filter = script_editor_decimal_filter_cb;
    script_editor_ambient_rolloff_inputbox.font = font_small;
    script_editor_ambient_loop_delay_label.create(&script_editor_ambient_window, 0xa, 0x7d, 0x46, 0x10, 8, false);
    script_editor_ambient_loop_delay_label.set_text("Loop Delay:");
    script_editor_ambient_loop_delay_label.font = font_small;
    script_editor_ambient_loop_delay_label.h_justify = 2;
    script_editor_ambient_loop_delay_inputbox.create(&script_editor_ambient_window, 0x55, 0x7d, 0x32, 0x10, 9, 0, false);
    script_editor_ambient_loop_delay_inputbox.set_valid_chars("01234567890.");
    script_editor_ambient_loop_delay_inputbox.callback = script_editor_ambient_field_cb;
    script_editor_ambient_loop_delay_inputbox.char_filter = script_editor_decimal_filter_cb;
    script_editor_ambient_loop_delay_inputbox.font = font_small;
    script_editor_ambient_ok_button.create(&script_editor_ambient_window, 0xff, 0x7d, 0x37, 0x10, 0xa, 0, false);
    script_editor_ambient_ok_button.set_text("OK");
    script_editor_ambient_ok_button.set_hotkey(KEY_ENTER, 0);
    script_editor_ambient_ok_button.callback = script_editor_ambient_ok;
    script_editor_ambient_ok_button.font = font_small;
    script_editor_ambient_cancel_button.create(&script_editor_ambient_window, 0x13b, 0x7d, 0x37, 0x10, 0xb, 0, false);
    script_editor_ambient_cancel_button.set_text("Cancel");
    script_editor_ambient_cancel_button.set_hotkey(KEY_ESC, 0);
    script_editor_ambient_cancel_button.font = font_small;
    script_editor_ambient_cancel_button.callback = script_editor_ambient_cancel_cb;

    // Action parameters: five label / input box pairs, ids 3 / 4 .. 11 / 12.
    script_editor_params_window.init(0xf5, 0xd2, 0xc8, 0x7d, UI_NO_ID, 0x22);
    script_editor_params_window.callback = script_editor_params_window_cb;
    script_editor_params_window.set_disabled(true);
    script_editor_params_title.create(&script_editor_params_window, 5, 5, 0x8c, 0xa, 0, false);
    script_editor_params_title.set_text("Edit Script Parameters");
    script_editor_params_title.font = font_small;
    script_editor_params_ok_button.create(&script_editor_params_window, 5, 0x69, 0x1e, 0xf, 1, 0, false);
    script_editor_params_ok_button.set_text("Ok");
    script_editor_params_ok_button.callback = script_editor_params_ok_cb;
    script_editor_params_ok_button.font = font_small;
    script_editor_params_cancel_button.create(&script_editor_params_window, 0x32, 0x69, 0x32, 0xf, 2, 0, false);
    script_editor_params_cancel_button.set_text("Cancel");
    script_editor_params_cancel_button.callback = script_editor_params_cancel_cb;
    script_editor_params_cancel_button.font = font_small;
    for (i = 0, y = 0x14, id = 4; id < 0xe; i++, y += 0xf, id += 2) {
        script_editor_param_labels[i].create(&script_editor_params_window, 5, y, 0x6e, 0xf, id - 1, false);
        script_editor_param_labels[i].font = font_small;
        script_editor_param_inputboxes[i].create(&script_editor_params_window, 0x6e, y, 0x55, 0xf, id, 0, false);
        script_editor_param_inputboxes[i].font = font_small;
    }

    // The mode list (and the other lists script_editor_fill_action_lists puts here).
    script_editor_choice_window.init(0x1f4, 0x13b, 0x87, 0x73, UI_NO_ID, 0x22);
    script_editor_choice_window.set_disabled(true);
    script_editor_choice_ok_button.create(&script_editor_choice_window, 5, 0x5f, 0x3c, 0xf, 0, 0, false);
    script_editor_choice_ok_button.set_text("Ok");
    script_editor_choice_ok_button.callback = script_editor_choice_ok_cb;
    script_editor_choice_ok_button.font = font_small;
    script_editor_choice_cancel_button.create(&script_editor_choice_window, 0x46, 0x5f, 0x3c, 0xf, 1, 0, false);
    script_editor_choice_cancel_button.set_text("Cancel");
    script_editor_choice_cancel_button.callback = script_editor_choice_cancel_cb;
    script_editor_choice_cancel_button.font = font_small;
    script_editor_choice_listbox.create(&script_editor_choice_window, 5, 5, 0x71, 0x55, 0, 0, 2, 2, false);
    script_editor_choice_listbox.callback = script_editor_choice_listbox_cb;
    for (i = 0; i < SE_NUM_MODES; i++) {
        e = script_editor_choice_listbox.add_entry(script_editor_mode_names[i], -1, 0, 0, 0);
        e->data = i;
        e->set_font(font_small);
    }
    script_editor_choice_listbox.select_by_data(script_editor_mode, true);

    // Door: two sets of sound fields at the same place, one per radio button.
    script_editor_door_window.init(0x82, 0x3c, 0x17c, 0x11d, UI_NO_ID, 0x22);
    script_editor_door_window.set_disabled(true);
    script_editor_door_sound_container.create(&script_editor_door_window, 0xa, 0xa, 0x168, 0xa, 0, 0, false);
    script_editor_door_opening_radio.create(&script_editor_door_sound_container, 0, 0, 10, 10, "Opening sound",
                                            3, 0, font_small, 1, false);
    script_editor_door_opening_radio.callback = script_editor_door_sound_radio_cb;
    script_editor_door_closing_radio.create(&script_editor_door_sound_container, 0xb4, 0, 10, 10,
                                            "Closing sound", 3, 0, font_small, 2, false);
    script_editor_door_closing_radio.callback = script_editor_door_sound_radio_cb;
    script_editor_door_sound_inputbox.create(&script_editor_door_window, 0xa, 0x19, 0x168, 0x10, 3, 0, false);
    script_editor_door_sound_inputbox.callback = script_editor_door_sound_inputbox_cb;
    script_editor_door_sound_inputbox.font = font_small;
    script_editor_door_open_sound_listbox.create(&script_editor_door_window, 0xa, 0x2d, 0x15c, 0x45, 0, 0, 4, 0x42, false);
    script_editor_door_open_sound_listbox.entry_spacing = 1;
    script_editor_door_open_sound_listbox.callback = script_editor_door_sound_listbox_cb;
    script_editor_door_close_sound_listbox.create(&script_editor_door_window, 0xa, 0x2d, 0x15c, 0x45, 0, 0, 5, 0x42, false);
    script_editor_door_close_sound_listbox.entry_spacing = 1;
    script_editor_door_close_sound_listbox.callback = script_editor_door_sound_listbox_cb;
    script_editor_door_min_range_label.create(&script_editor_door_window, 0, 0x75, 0x46, 0x10, 6, false);
    script_editor_door_min_range_label.set_text("Min range:");
    script_editor_door_min_range_label.h_justify = 2;
    script_editor_door_min_range_label.font = font_small;
    script_editor_door_open_min_range_inputbox.create(&script_editor_door_window, 0x4b, 0x75, 0x32, 0x10, 7, 0, false);
    script_editor_door_open_min_range_inputbox.set_valid_chars("01234567890.");
    script_editor_door_open_min_range_inputbox.font = font_small;
    script_editor_door_open_min_range_inputbox.callback = script_editor_door_field_cb;
    script_editor_door_open_min_range_inputbox.char_filter = script_editor_decimal_filter_cb;
    script_editor_door_close_min_range_inputbox.create(&script_editor_door_window, 0x4b, 0x75, 0x32, 0x10, 8, 0, false);
    script_editor_door_close_min_range_inputbox.set_valid_chars("01234567890.");
    script_editor_door_close_min_range_inputbox.callback = script_editor_door_field_cb;
    script_editor_door_close_min_range_inputbox.char_filter = script_editor_decimal_filter_cb;
    script_editor_door_close_min_range_inputbox.font = font_small;
    script_editor_door_volume_label.create(&script_editor_door_window, 0x76, 0x75, 0x46, 0x10, 9, false);
    script_editor_door_volume_label.set_text("Volume:");
    script_editor_door_volume_label.h_justify = 2;
    script_editor_door_volume_label.font = font_small;
    script_editor_door_open_volume_inputbox.create(&script_editor_door_window, 0xc1, 0x75, 0x32, 0x10, 0xa, 0, false);
    script_editor_door_open_volume_inputbox.set_valid_chars("01234567890.");
    script_editor_door_open_volume_inputbox.font = font_small;
    script_editor_door_open_volume_inputbox.callback = script_editor_door_field_cb;
    script_editor_door_open_volume_inputbox.char_filter = script_editor_decimal_filter_cb;
    script_editor_door_close_volume_inputbox.create(&script_editor_door_window, 0xc1, 0x75, 0x32, 0x10, 0xb, 0, false);
    script_editor_door_close_volume_inputbox.set_valid_chars("01234567890.");
    script_editor_door_close_volume_inputbox.callback = script_editor_door_field_cb;
    script_editor_door_close_volume_inputbox.char_filter = script_editor_decimal_filter_cb;
    script_editor_door_close_volume_inputbox.font = font_small;
    script_editor_door_rolloff_label.create(&script_editor_door_window, 0xf5, 0x75, 0x46, 0x10, 0xc, false);
    script_editor_door_rolloff_label.set_text("Roll-off:");
    script_editor_door_rolloff_label.h_justify = 2;
    script_editor_door_rolloff_label.font = font_small;
    script_editor_door_open_rolloff_inputbox.create(&script_editor_door_window, 0x140, 0x75, 0x32, 0x10, 0xd, 0, false);
    script_editor_door_open_rolloff_inputbox.set_valid_chars("01234567890.");
    script_editor_door_open_rolloff_inputbox.font = font_small;
    script_editor_door_open_rolloff_inputbox.callback = script_editor_door_field_cb;
    script_editor_door_open_rolloff_inputbox.char_filter = script_editor_decimal_filter_cb;
    script_editor_door_close_rolloff_inputbox.create(&script_editor_door_window, 0x140, 0x75, 0x32, 0x10, 0xe, 0, false);
    script_editor_door_close_rolloff_inputbox.set_valid_chars("01234567890.");
    script_editor_door_close_rolloff_inputbox.callback = script_editor_door_field_cb;
    script_editor_door_close_rolloff_inputbox.char_filter = script_editor_decimal_filter_cb;
    script_editor_door_close_rolloff_inputbox.font = font_small;
    script_editor_door_locked_checkbox.create(&script_editor_door_window, 0xa, 0x96, 10, 10, "Originally locked",
                                              3, 0, font_small, false, 0xf, false);
    script_editor_door_locked_checkbox.callback = script_editor_door_locked_cb;
    script_editor_door_skill_label.create(&script_editor_door_window, 0xbe, 0x96, 0x78, 0xa, 0x10, false);
    script_editor_door_skill_label.set_text("Pick skill modifier:");
    script_editor_door_skill_label.font = font_small;
    script_editor_door_skill_inputbox.create(&script_editor_door_window, 0x136, 0x93, 0x2d, 0x10, 0x11, 0, false);
    script_editor_door_skill_inputbox.set_valid_chars("0123456789");
    script_editor_door_skill_inputbox.max_length = 4;
    script_editor_door_skill_inputbox.callback = script_editor_door_skill_cb;
    script_editor_door_skill_inputbox.font = font_small;
    script_editor_door_percent_label.create(&script_editor_door_window, 0x165, 0x96, 0xf, 0xa, 0x12, false);
    script_editor_door_percent_label.set_text("%");
    script_editor_door_percent_label.font = font_small;
    script_editor_door_key_inputbox.create(&script_editor_door_window, 0xa, 0xa5, 0x168, 0x10, 0x13, 0, false);
    script_editor_door_key_inputbox.callback = script_editor_door_key_cb;
    script_editor_door_key_inputbox.font = font_small;
    script_editor_door_key_listbox.create(&script_editor_door_window, 0xa, 0xb9, 0x15c, 0x45, 0, 0, 0x14, 0x442, false);
    script_editor_door_key_listbox.entry_spacing = 1;
    script_editor_door_key_listbox.callback = script_editor_door_key_listbox_cb;
    for (i = 0; i < num_item_types; i++) {
        if (item_types[i].kind == 2 && _stricmp(item_types[i].name, "gold") != 0) {
            e = script_editor_door_key_listbox.add_entry(item_types[i].name, -1, 0, 0, 0);
            e->set_font(font_small);
        }
    }
    script_editor_door_ok_button.create(&script_editor_door_window, 0xff, 0x104, 0x37, 0x10, 0x15, 0, false);
    script_editor_door_ok_button.set_text("OK");
    script_editor_door_ok_button.set_hotkey(KEY_ENTER, 0);
    script_editor_door_ok_button.callback = script_editor_door_ok;
    script_editor_door_ok_button.font = font_small;
    script_editor_door_cancel_button.create(&script_editor_door_window, 0x13b, 0x104, 0x37, 0x10, 0x16, 0, false);
    script_editor_door_cancel_button.set_text("Cancel");
    script_editor_door_cancel_button.set_hotkey(KEY_ESC, 0);
    script_editor_door_cancel_button.font = font_small;
    script_editor_door_cancel_button.callback = script_editor_door_cancel_cb;

    // Door lock / clickable object: an item to give, gold.
    script_editor_door_lock_window.init(0x82, 0x3c, 0x17c, 0x11d, UI_NO_ID, 0x22);
    script_editor_door_lock_window.set_disabled(true);
    script_editor_door_lock_name_inputbox.create(&script_editor_door_lock_window, 0xa, 0x19, 0x168, 0x10, 3, 0, false);
    script_editor_door_lock_item_inputbox.create(&script_editor_door_lock_window, 0xa, 0x32, 0x168, 0x10, 3, 0, false);
    script_editor_door_lock_item_inputbox.callback = script_editor_door_item_cb;
    script_editor_door_lock_item_inputbox.font = font_small;
    script_editor_door_lock_item_listbox.create(&script_editor_door_lock_window, 0xa, 0x46, 0x168, 0x45, 0, 0, 0x14, 0x442, false);
    script_editor_door_lock_item_listbox.entry_spacing = 1;
    script_editor_door_lock_item_listbox.callback = script_editor_door_lock_item_listbox_cb;
    for (i = 0; i < num_item_types; i++) {
        if (item_types[i].kind == 2) {
            e = script_editor_door_lock_item_listbox.add_entry(item_types[i].name, -1, 0, 0, 0);
            e->set_font(font_small);
        }
    }
    script_editor_door_lock_give_checkbox.create(&script_editor_door_lock_window, 0xa, 0x96, 10, 10, "Give item",
                                                 3, 0, font_small, false, 0xf, false);
    script_editor_door_lock_give_checkbox.callback = script_editor_door_lock_give_checkbox_cb;
    script_editor_door_lock_value_inputbox.create(&script_editor_door_lock_window, 0x136, 0x93, 0x2d, 0x10, 0x11, 0, false);
    script_editor_door_lock_value_inputbox.set_valid_chars("0123456789");
    script_editor_door_lock_value_inputbox.max_length = 4;
    script_editor_door_lock_value_inputbox.callback = script_editor_door_lock_value_cb;
    script_editor_door_lock_value_inputbox.font = font_small;
    script_editor_door_lock_ok_button.create(&script_editor_door_lock_window, 0xff, 0x104, 0x37, 0x10, 0x15, 0, false);
    script_editor_door_lock_ok_button.set_text("OK");
    script_editor_door_lock_ok_button.set_hotkey(KEY_ENTER, 0);
    script_editor_door_lock_ok_button.font = font_small;
    script_editor_door_lock_ok_button.callback = script_editor_door_lock_ok_cb;
    script_editor_door_lock_remove_button.create(&script_editor_door_lock_window, 0x13b, 0x104, 0x37, 0x10, 0x16, 0, false);
    script_editor_door_lock_remove_button.set_text("Remove");
    script_editor_door_lock_remove_button.set_hotkey(KEY_ESC, 0);
    script_editor_door_lock_remove_button.callback = script_editor_door_lock_remove_cb;
    script_editor_door_lock_remove_button.font = font_small;

    // Creature: team and level.
    script_editor_creature_window.init(0x82, 0x7d, 0x17c, 0x96, UI_NO_ID, 0x22);
    script_editor_creature_window.set_disabled(true);
    script_editor_creature_team_label.create(&script_editor_creature_window, 0xa, 7, 0x32, 0x10, 0, false);
    script_editor_creature_team_label.set_text("Team:");
    script_editor_creature_team_label.font = font_small;
    script_editor_creature_team_container.create(&script_editor_creature_window, 0xa, 0x1a, 0x168, 0xa, 1, 0, false);
    script_editor_creature_friendly_radio.create(&script_editor_creature_team_container, 0, 0, 10, 10, "Friendly",
                                                 3, 0, font_small, 2, false);
    script_editor_creature_neutral_radio.create(&script_editor_creature_team_container, 0x5a, 0, 10, 10, "Neutral",
                                                3, 0, font_small, 3, false);
    script_editor_creature_hostile_radio.create(&script_editor_creature_team_container, 0xb4, 0, 10, 10, "Hostile",
                                                3, 0, font_small, 4, false);
    script_editor_creature_evil_radio.create(&script_editor_creature_team_container, 0x10e, 0, 10, 10, "Evil",
                                             3, 0, font_small, 5, false);
    script_editor_creature_level_label.create(&script_editor_creature_window, 0xa, 0x32, 0x28, 0x10, 6, false);
    script_editor_creature_level_label.set_text("Level:");
    script_editor_creature_level_label.font = font_small;
    script_editor_creature_level_inputbox.create(&script_editor_creature_window, 0x37, 0x32, 0x19, 0x10, 7, 0, false);
    script_editor_creature_level_inputbox.set_valid_chars("0123456789");
    script_editor_creature_level_inputbox.max_length = 2;
    script_editor_creature_level_inputbox.callback = script_editor_creature_level_cb;
    script_editor_creature_level_inputbox.font = font_small;
    script_editor_creature_info_label.create(&script_editor_creature_window, 0x55, 0x32, 0x96, 0x10, 8, false);
    script_editor_creature_info_label.font = font_small;
    script_editor_creature_ok_button.create(&script_editor_creature_window, 0xff, 0x7d, 0x37, 0x10, 9, 0, false);
    script_editor_creature_ok_button.set_text("OK");
    script_editor_creature_ok_button.set_hotkey(KEY_ENTER, 0);
    script_editor_creature_ok_button.callback = script_editor_creature_ok;
    script_editor_creature_ok_button.font = font_small;
    script_editor_creature_cancel_button.create(&script_editor_creature_window, 0x13b, 0x7d, 0x37, 0x10, 0xa, 0, false);
    script_editor_creature_cancel_button.set_text("Cancel");
    script_editor_creature_cancel_button.set_hotkey(KEY_ESC, 0);
    script_editor_creature_cancel_button.callback = script_editor_creature_cancel_cb;
    script_editor_creature_cancel_button.font = font_small;

    // Level item: an item placed at a navpoint.
    script_editor_level_item_window.init(0x82, 0x7d, 0x17c, 0x96, UI_NO_ID, 0x22);
    script_editor_level_item_window.set_disabled(true);
    script_editor_level_item_name_inputbox.create(&script_editor_level_item_window, 0xa, 5, 0x168, 0x10, 0, 0, false);
    script_editor_level_item_name_inputbox.font = font_small;
    script_editor_level_item_name_inputbox.callback = script_editor_level_item_cb;
    script_editor_level_item_listbox.create(&script_editor_level_item_window, 0xa, 0x19, 0x15c, 0x45, 0, 0, 1, 0x42, false);
    script_editor_level_item_listbox.entry_spacing = 1;
    script_editor_level_item_listbox.callback = script_editor_level_item_listbox_cb;
    for (i = 0; i < num_item_types; i++) {
        e = script_editor_level_item_listbox.add_entry(item_types[i].name, -1, 0, 0, 0);
        e->data = i;
        e->set_font(font_small);
    }
    script_editor_level_item_count_label.create(&script_editor_level_item_window, 0xa, 0x61, 0x28, 0x10, 2, false);
    script_editor_level_item_count_label.set_text("Count:");
    script_editor_level_item_count_label.font = font_small;
    script_editor_level_item_count_inputbox.create(&script_editor_level_item_window, 0x37, 0x61, 0x19, 0x10, 3, 0, false);
    script_editor_level_item_count_inputbox.set_valid_chars("0123456789");
    script_editor_level_item_count_inputbox.max_length = 2;
    script_editor_level_item_count_inputbox.callback = script_editor_level_item_count_cb;
    script_editor_level_item_count_inputbox.font = font_small;
    script_editor_level_item_ok_button.create(&script_editor_level_item_window, 0xff, 0x7d, 0x37, 0x10, 4, 0, false);
    script_editor_level_item_ok_button.set_text("OK");
    script_editor_level_item_ok_button.set_hotkey(KEY_ENTER, 0);
    script_editor_level_item_ok_button.font = font_small;
    script_editor_level_item_ok_button.callback = script_editor_level_item_ok;
    script_editor_level_item_cancel_button.create(&script_editor_level_item_window, 0x13b, 0x7d, 0x37, 0x10, 5, 0, false);
    script_editor_level_item_cancel_button.set_text("Cancel");
    script_editor_level_item_cancel_button.set_hotkey(KEY_ESC, 0);
    script_editor_level_item_cancel_button.callback = script_editor_level_item_cancel_cb;
    script_editor_level_item_cancel_button.font = font_small;

    script_editor_ui_created = true;
    script_editor_clear_selection();
}

// 0x4e28f0  Confidence: exact. The Cutscene Script button is shown only while a cutscene script is loaded.
void script_editor_sync_cutscene_button()
{
    if (cutscene_script_is_active())
        script_editor_cutscene_button.set_visible(true);
    else
        script_editor_cutscene_button.set_hidden(true);
}

// 0x4e2910  Confidence: exact. Applies a pending rename first (the name box), then hides the selection
// panel's gadgets.
void script_editor_clear_selection()
{
    navpoint_set_name();
    script_editor_sel.kind = SE_PICK_NONE;
    script_editor_cur_script = NULL;
    script_editor_name_inputbox.set_text(NULL);
    script_editor_name_inputbox.set_hidden(true);
    script_editor_action_listbox.clear();
    script_editor_action_listbox.set_hidden(true);
    script_editor_append_action_button.set_hidden(true);
    script_editor_insert_action_button.set_hidden(true);
    script_editor_delete_action_button.set_hidden(true);
    script_editor_cur_action = NULL;
}

// 0x4e2990  Confidence: exact. The name box into the selected character or navpoint (trimmed). A navpoint
// keeps its name when another navpoint already has the new one. The box is limited to 14 characters for
// navpoints (script_editor_show_selection_panel), so the strcpy fits navpoint::name[16].
void navpoint_set_name()
{
    char name[256];

    switch (script_editor_sel.kind) {
    case SE_PICK_CHARACTER:
        trim_whitespace(name, script_editor_name_inputbox.get_text(), -1);
        ((Character *)script_editor_sel.object)->set_name(name);
        break;
    case SE_PICK_NAVPOINT:
        trim_whitespace(name, script_editor_name_inputbox.get_text(), -1);
        if (strcmp(((navpoint *)script_editor_sel.object)->name, name) != 0) {
            if (navpoint_find(name) != NULL) {
                script_editor_set_status("Unable to assign name to navpoint -- duplicate navpoint name found.");
                return;
            }
            strcpy(((navpoint *)script_editor_sel.object)->name, name);
        }
        break;
    }
}

// 0x4e2a90  Confidence: exact. The status line of script_editor_frame, shown for 2 s.
void script_editor_set_status(const char *text)
{
    strcpy(script_editor_status_text, text);
    script_editor_status_timer.SetTimeout(2000);
}

// 0x4e2ad0  Confidence: exact. Every frame while game_flags bit 0x80 is clear (script_editor_frame): once
// the UI exists, destroys the twelve windows, gives the camera back to the player and unloads the
// cutscene script (and the cutscene, when one is named).
void script_editor_close()
{
    if (!script_editor_ui_created)
        return;
    script_editor_window.dtor();
    script_editor_action_window.dtor();
    script_editor_save_window.dtor();
    script_editor_load_window.dtor();
    script_editor_ambient_window.dtor();
    script_editor_params_window.dtor();
    script_editor_new_window.dtor();
    script_editor_choice_window.dtor();
    script_editor_door_window.dtor();
    script_editor_creature_window.dtor();
    script_editor_level_item_window.dtor();
    script_editor_door_lock_window.dtor();
    ui_pop_group();
    script_editor_ui_created = false;
    script_editor_sel.kind = SE_PICK_NONE;
    script_editor_hover.kind = SE_PICK_NONE;
    script_editor_cur_script = NULL;
    game_camera.set_target(cur_character);
    cutscene_script_unload();
    if (strlen(cutscene_script_table()) != 0)
        the_cutscene.unload();
}

// 0x4e2bb0  Confidence: likely (the character box: the y / x / z order of the sum of squares). The object
// under the mouse ray, of the kind script_editor_pick_filter selects (jump table 0x4e2f18): characters
// (summon-type objects, type 9, get a box from their own size), navpoints (a 0.8 x 0.4 x 0.8 box, level
// table first, then the cutscene table), doors (level objects with flag 0x100) or any level object. Not
// while the left button is down (a drag keeps its object).
void script_editor_pick_hover()
{
    Character *c;
    navpoint *np;
    lobj *l;
    vector3 size, hit;
    matrix orient;
    float radius;
    int i, index, face;

    if (mouse_is_down(0))
        return;
    script_editor_pick_distance = 9.9e10f;
    script_editor_hover.kind = SE_PICK_NONE;
    switch (script_editor_pick_filter) {
    case 1:
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (c->info->species == 0x59 && character_is_serpent_part(c))
                continue;
            if (c->type == 9) {
                size.x = size.z = c->size_radius() * 2.0f;
                size.y = c->size_height();
                radius = (float)sqrt(size.y * size.y + size.x * size.x + size.z * size.z);
            } else {
                size.x = script_editor_char_sizes[c->info->size][0];
                size.y = script_editor_char_sizes[c->info->size][1];
                size.z = script_editor_char_sizes[c->info->size][2];
                radius = script_editor_char_size_radii[c->info->size];
            }
            if (script_editor_ray_hit_object(&c->pos, &c->orient, &size, radius)) {
                script_editor_hover.kind = SE_PICK_CHARACTER;
                script_editor_hover.object = c;
            }
        }
        return;
    case 2:
        size.x = 0.8f;
        size.y = 0.4f;
        size.z = 0.8f;
        for (i = 0, np = navpoints; i < navpoint_count; i++, np++) {
            orient.set_from_angles(0, 0, np->heading);
            if (script_editor_ray_hit_object(&np->pos, &orient, &size, 0.693f)) {
                script_editor_hover.object = np;
                script_editor_hover.kind = SE_PICK_NAVPOINT;
                return;
            }
        }
        if (script_editor_hover.kind == SE_PICK_NAVPOINT)
            return;
        for (i = 0, np = cutscene_navpoints; i < cutscene_navpoint_count; i++, np++) {
            orient.set_from_angles(0, 0, np->heading);
            if (script_editor_ray_hit_object(&np->pos, &orient, &size, 0.693f)) {
                script_editor_hover.object = np;
                script_editor_hover.kind = SE_PICK_NAVPOINT;
                return;
            }
        }
        return;
    case 3:
        index = world_pick_lobj(&camera_view_pos, &script_editor_mouse_ray.dir, &hit, &face);
        if (index < 0)
            return;
        l = lobj_from_index(index);
        if (!(l->flags & 0x100))
            return;
        size = l->bbox_max - l->bbox_min;
        if (script_editor_ray_hit_object(&l->pos, &l->orient, &size, l->mesh->_unk30)) {
            script_editor_hover.object = l;
            script_editor_hover.kind = SE_PICK_DOOR;
        }
        return;
    case 4:
        index = world_pick_lobj(&camera_view_pos, &script_editor_mouse_ray.dir, &hit, &face);
        if (index < 0)
            return;
        l = lobj_from_index(index);
        size = l->bbox_max - l->bbox_min;
        if (script_editor_ray_hit_object(&l->pos, &l->orient, &size, l->mesh->_unk30)) {
            script_editor_hover.kind = SE_PICK_OBJECT;
            script_editor_hover.object = l;
        }
        return;
    }
}

// 0x4e2f30  Confidence: likely (the function has an exception frame with nothing to unwind, so a local
// of a class with a destructor may be missing; and t: the sphere test's second output is written into
// the stack slot of the radius argument, as if t were the parameter itself). The mouse ray hits the
// bounding sphere (within 1000 units) and the box (its centre half the box height up the up axis), nearer
// than the best hit so far: the hover record takes the object's position, orientation and size.
bool script_editor_ray_hit_object(vector3 *pos, matrix *orient, vector3 *size, float radius)
{
    vector3 hit;
    float d;

    if (!ix_ray_sphere(&hit, &radius, &script_editor_mouse_ray, 1000.0f, pos, radius))
        return false;
    vector3 centre = *pos + orient->uvec * (size->y * 0.5f);
    if (!ix_ray_box_oriented(&script_editor_mouse_ray, &centre, orient, size, &hit))
        return false;
    d = distance(pos, &hit);
    if (d < script_editor_pick_distance) {
        script_editor_pick_distance = d;
        script_editor_hover.pos = *pos;
        script_editor_hover.orient = *orient;
        script_editor_hover.size = *size;
        return true;
    }
    return false;
}

// The navpoint name label of script_editor_draw_navpoints (written out twice in the binary): a dark box
// and the name at the projected point.
#define SE_DRAW_NAVPOINT_LABEL(np)                                                          \
    do {                                                                                    \
        gr_rotate_vertex(&v, &(np)->pos);                                                   \
        gr_project_vertex(&v);                                                              \
        if (v.flags & 1) {                                                                  \
            gr_get_string_size(&w, &h, (np)->name, -1, -1);                                 \
            gr_set_color(0, 0, 0, 0x8c);                                                    \
            gr_rect((int)v.sx, (int)v.sy, w + 2, h + 2, gr_rect_mode, 1.0f);                \
            gr_set_color(0xa0, 0xa0, 0xff, 0xff);                                           \
            gr_string((int)(v.sx + 1.0f), (int)(v.sy + 1.0f), (np)->name, -1, gr_string_mode); \
        }                                                                                   \
    } while (0)

// 0x4e30d0  Confidence: likely (the label blocks as a macro; jump order of the type switch). The markers
// of the level navpoints (by type; the selected character's start navpoint in red), the ambient sound
// ranges of the selected ambient navpoint, the cutscene navpoints, and with Render_s3d_navpoints the
// level file's navpoints. Names are shown in navpoint mode while Shift is held, or for the selected
// navpoint while Ctrl is held. A cutscene-type navpoint in the level table is skipped entirely.
void script_editor_draw_navpoints()
{
    int i, w, h, a;
    navpoint *np;
    matrix orient;
    gr_vertex v;

    for (i = 0, np = navpoints; i < navpoint_count; i++, np++) {
        orient.set_from_angles(0, 0, np->heading);
        switch (np->type) {
        case NAVPOINT_TYPE_LEVEL:
            if (script_editor_sel.kind == SE_PICK_CHARACTER
                && ((Character *)script_editor_sel.object)->start_navpoint == np) {
                gr_set_color(0xff, 0, 0x40, 0xff);
                script_editor_draw_marker(&np->pos, np->ground_height, &orient, 0.3f, gr_line_3d_mode);
                gr_set_color(0xff, 0, 0x40, 0x80);
            } else {
                gr_set_color(0, 0xa0, 0xa0, 0xff);
                script_editor_draw_marker(&np->pos, np->ground_height, &orient, 0.3f, gr_line_3d_mode);
                gr_set_color(0, 0xa0, 0xa0, 0x80);
            }
            script_editor_draw_marker(&np->pos, np->ground_height, &orient, 0.3f, gr_line_mode);
            break;
        case NAVPOINT_TYPE_CAMERA:
            gr_set_color(0xa0, 0xa0, 0xff, 0xff);
            script_editor_draw_marker(&np->pos, np->ground_height, &orient, 0.3f, gr_line_3d_mode);
            gr_set_color(0xa0, 0xa0, 0xff, 0x80);
            script_editor_draw_marker(&np->pos, np->ground_height, &orient, 0.3f, gr_line_mode);
            break;
        case NAVPOINT_TYPE_AMBIENT:
            gr_set_color(0xff, 0xff, 0, 0xff);
            script_editor_draw_marker(&np->pos, np->ground_height, &orient, 0.3f, gr_line_3d_mode);
            gr_set_color(0xff, 0xff, 0, 0x80);
            script_editor_draw_marker(&np->pos, np->ground_height, &orient, 0.3f, gr_line_mode);
            break;
        default:
            continue;
        }
        if (script_editor_sel.kind == SE_PICK_NAVPOINT && script_editor_sel.object == np
            && np->type == NAVPOINT_TYPE_AMBIENT) {
            a = level_ambient_find_at(&np->pos);
            if (a >= 0) {
                snd_sound *s = &sounds[sound_emitter_get_sound(level_ambients[a].emitter)];
                gr_set_color(0x80, 0x80, 0, 0xff);
                gr_draw_sphere(&np->pos, s->max_range, gr_line_mode);
                gr_set_color(0xff, 0xff, 0, 0xff);
                gr_draw_sphere(&np->pos, s->min_range, gr_line_mode);
            }
        }
        if (script_editor_mode == SE_MODE_NAVPOINT
            && (key_is_down(KEY_LSHIFT) || key_is_down(KEY_RSHIFT)
                || ((key_is_down(KEY_LCONTROL) || key_is_down(KEY_RCONTROL)) && script_editor_sel.object == np)))
            SE_DRAW_NAVPOINT_LABEL(np);
    }

    for (i = 0, np = cutscene_navpoints; i < cutscene_navpoint_count; i++, np++) {
        orient.set_from_angles(0, 0, np->heading);
        if (script_editor_sel.kind == SE_PICK_CHARACTER
            && ((Character *)script_editor_sel.object)->start_navpoint == np) {
            gr_set_color(0xff, 0, 0x40, 0xff);
            script_editor_draw_marker(&np->pos, np->ground_height, &orient, 0.3f, gr_line_3d_mode);
            gr_set_color(0xff, 0, 0x40, 0x80);
        } else {
            gr_set_color(0x28, 0xa0, 0x28, 0xff);
            script_editor_draw_marker(&np->pos, np->ground_height, &orient, 0.3f, gr_line_3d_mode);
            gr_set_color(0x28, 0xa0, 0x28, 0x80);
        }
        script_editor_draw_marker(&np->pos, np->ground_height, &orient, 0.3f, gr_line_mode);
        if (script_editor_mode == SE_MODE_NAVPOINT
            && (key_is_down(KEY_LSHIFT) || key_is_down(KEY_RSHIFT)
                || ((key_is_down(KEY_LCONTROL) || key_is_down(KEY_RCONTROL)) && script_editor_sel.object == np)))
            SE_DRAW_NAVPOINT_LABEL(np);
    }

    if (Render_s3d_navpoints) {
        for (i = 0; i < level_num_navpoints; i++) {
            s3d_navpoint *sn = &level_navpoints[i];
            gr_set_color(0x40, 0, 0, 0xff);
            script_editor_draw_marker(&sn->pos, 0.0f, &sn->orient, 0.3f, gr_line_3d_mode);
            gr_set_color(0x40, 0, 0, 0x80);
            script_editor_draw_marker(&sn->pos, 0.0f, &sn->orient, 0.3f, gr_line_mode);
        }
    }
}

// 0x4e3700  Confidence: exact (the five points as array assignments; VC6 expands some of the inline vector3
// operators and calls others out of line, which the source does not show). An arrow head of size `size`
// along the forward axis at pos (lines 0-1, 0-2, 2-3, 3-0, 0-4, 4-1), and a drop line down `height` to
// the ground when the navpoint floats.
void script_editor_draw_marker(vector3 *pos, float height, matrix *orient, float size, gr::Mode mode)
{
    vector3 p[5];
    gr_vertex v[5];
    int i;

    p[0] = orient->fvec;
    p[1] = -orient->fvec;
    p[2] = -orient->fvec + orient->rvec;
    p[3] = -orient->fvec - orient->rvec;
    p[4] = -orient->fvec + orient->uvec;
    for (i = 0; i < 5; i++) {
        p[i] = *pos + p[i] * size;
        gr_rotate_vertex(&v[i], &p[i]);
    }
    gr_line_3d(&v[0], &v[1], mode);
    gr_line_3d(&v[0], &v[2], mode);
    gr_line_3d(&v[2], &v[3], mode);
    gr_line_3d(&v[3], &v[0], mode);
    gr_line_3d(&v[0], &v[4], mode);
    gr_line_3d(&v[4], &v[1], mode);
    if (height != 0.0f) {
        p[0] = *pos;
        p[0].y -= height;
        gr_line_3d_world(pos, &p[0], mode);
    }
}

// The hover record into the selection (written out in each branch of script_editor_handle_input).
#define SE_SELECT_HOVER() (script_editor_sel = script_editor_hover)

// 0x4e3a20  Confidence: likely (the branch layout; jump tables 0x4e4850 / 0x4e4858 over the action types
// and 0x4e4874 over the hover kind). Nothing while a modal window is open.
//   Left click: with New latched, places by mode (creature: on the hovered navpoint, or on a new navpoint
//   at the mouse; navpoint mode: a level or cutscene navpoint; ambient mode: an ambient navpoint); with
//   Change latched, puts the hovered navpoint (or the hovered character's name, for pass go / follow)
//   into the selected action; else selects what is under the mouse (Shift keeps the selection). The
//   camera state is pushed (follow off) until the button is released.
//   Right click: opens the editing window of the hovered object (creature, level item, ambient sound,
//   door, door lock / clickable object) and pauses the game.
//   Left drag: Pos moves the selection over the ground (Pos + Y: up and down, along a camera-facing
//   plane), Ori turns it (Ori + Y on a character: pitch only, which is never applied); a right click
//   during the drag puts it back. Camera navpoints cannot be dragged.
void script_editor_handle_input()
{
    bool cutscene;
    int index, layer, dx, dy;
    script_action *a;
    navpoint *np;
    float *height;
    char args[0x104], name[0x104];
    vector3 pos, n, hit, vel;
    plane pl;
    lobj_ray ray;
    lobj_hit lhit;
    matrix rot;

    cutscene = script_editor_cutscene_button.is_latched();
    if (ui_get_top_modal_window() != NULL)
        return;
    if (!mouse_is_down(0) && script_editor_camera_held) {
        game_camera.pop_state();
        script_editor_camera_held = false;
    }

    if (mouse_was_pressed(0)) {
        navpoint_set_name();
        if (script_editor_new_button.is_latched()) {
            switch (script_editor_mode) {
            case SE_MODE_CREATURE:
                if (script_editor_hover.kind == SE_PICK_NAVPOINT) {
                    script_editor_place_creature();
                    return;
                }
                if (script_editor_cutscene_button.is_latched())
                    script_editor_add_navpoint(NAVPOINT_TYPE_CUTSCENE);
                else
                    script_editor_add_navpoint(NAVPOINT_TYPE_LEVEL);
                script_editor_place_creature();
                return;
            case SE_MODE_NAVPOINT:
                if (script_editor_hover.kind != SE_PICK_NONE)
                    return;
                if (cutscene)
                    script_editor_add_navpoint(NAVPOINT_TYPE_CUTSCENE);
                else
                    script_editor_add_navpoint(NAVPOINT_TYPE_LEVEL);
                return;
            case SE_MODE_AMBIENT:
                if (script_editor_hover.kind != SE_PICK_NONE)
                    return;
                script_editor_add_navpoint(NAVPOINT_TYPE_AMBIENT);
                return;
            }
            return;
        }
        if (script_editor_change_button.is_latched()) {
            if (script_editor_hover.kind == SE_PICK_NAVPOINT) {
                np = (navpoint *)script_editor_hover.object;
                index = script_editor_cur_action->data;
                if (!cutscene && np->type == NAVPOINT_TYPE_CUTSCENE) {
                    script_editor_set_status("Cannot use a cutscene navpoint in a level script.");
                    return;
                }
                a = &script_editor_cur_script->actions[index];
                switch (a->type) {
                case SCRIPT_ACTION_WALK_TO: case SCRIPT_ACTION_RUN_TO: case SCRIPT_ACTION_MOVE_THROUGH:
                case SCRIPT_ACTION_WAIT_AT: case SCRIPT_ACTION_WANDER: case SCRIPT_ACTION_WANDER_FOR:
                case SCRIPT_ACTION_TRIGGER_NAV: case SCRIPT_ACTION_TELEPORT_TO: case SCRIPT_ACTION_SYNCH_TELEPORT:
                    if (np->type != NAVPOINT_TYPE_LEVEL && np->type != NAVPOINT_TYPE_CUTSCENE) {
                        script_editor_set_status("Invalid navpoint for script action.");
                        return;
                    }
                    break;
                }
                a->nav = np;
            } else if (script_editor_hover.kind == SE_PICK_CHARACTER) {
                index = script_editor_cur_action->data;
                a = &script_editor_cur_script->actions[index];
                if (a->type == SCRIPT_ACTION_PASS_GO || a->type == SCRIPT_ACTION_FOLLOW)
                    script_editor_store_string(a, ((Character *)script_editor_hover.object)->get_name());
            } else {
                script_editor_change_button.set_latched(false);
                return;
            }
            if (index != -1) {
                script_action_format(name, args, &script_editor_cur_script->actions[index],
                                     (Character *)script_editor_sel.object);
                sprintf(string_buffer, "%s %s", name, args);
                script_editor_cur_action->set_text(string_buffer);
            }
            script_editor_change_button.set_latched(false);
            return;
        }
        if (!key_is_down(KEY_LSHIFT) && !key_is_down(KEY_RSHIFT)) {
            SE_SELECT_HOVER();
            if (script_editor_sel.kind == SE_PICK_NONE)
                script_editor_clear_selection();
            else
                script_editor_show_selection_panel();
        }
        if (!script_editor_camera_held) {
            game_camera.push_state();
            game_camera.set_follow_flags(false, false);
            script_editor_camera_held = true;
        }
        if (script_editor_sel.kind != SE_PICK_NONE) {
            script_editor_saved_pos = script_editor_sel.pos;
            script_editor_saved_orient = script_editor_sel.orient;
        }
    }

    if (mouse_was_pressed(1)) {
        switch (script_editor_hover.kind) {
        case SE_PICK_CHARACTER:
            SE_SELECT_HOVER();
            script_editor_show_selection_panel();
            game_pause(0, 0);
            script_editor_show_creature_info((Character *)script_editor_hover.object);
            script_editor_creature_window.set_enabled(true);
            break;
        case SE_PICK_NAVPOINT:
            np = (navpoint *)script_editor_hover.object;
            if (np->type == NAVPOINT_TYPE_LEVEL) {
                SE_SELECT_HOVER();
                script_editor_show_selection_panel();
                game_pause(0, 0);
                script_editor_show_level_item((navpoint *)script_editor_hover.object);
                script_editor_level_item_window.set_enabled(true);
            } else if (np->type == NAVPOINT_TYPE_AMBIENT) {
                SE_SELECT_HOVER();
                script_editor_show_selection_panel();
                game_pause(0, 0);
                script_editor_list_ambient_sounds((navpoint *)script_editor_hover.object);
                script_editor_ambient_window.set_enabled(true);
            }
            break;
        case SE_PICK_DOOR:
            SE_SELECT_HOVER();
            script_editor_show_selection_panel();
            game_pause(0, 0);
            script_editor_list_door_sounds((lobj *)script_editor_hover.object);
            script_editor_door_window.set_enabled(true);
            break;
        case SE_PICK_OBJECT:
            SE_SELECT_HOVER();
            if (!lc_entry_exists((lobj *)script_editor_hover.object))
                lc_entry_add((lobj *)script_editor_hover.object);
            script_editor_show_selection_panel();
            game_pause(0, 0);
            script_editor_show_door_lock((lobj *)script_editor_hover.object);
            script_editor_door_lock_window.set_enabled(true);
            break;
        }
    }

    if (!script_editor_camera_held)
        return;
    if (mouse_was_pressed(1)) {
        if (script_editor_sel.kind != SE_PICK_NONE) {
            script_editor_sel.pos = script_editor_saved_pos;
            script_editor_sel.orient = script_editor_saved_orient;
        }
        game_camera.pop_state();
        script_editor_camera_held = false;
        return;
    }
    if (!script_editor_camera_held || script_editor_sel.kind == SE_PICK_NONE)
        return;
    if (script_editor_sel.kind == SE_PICK_NAVPOINT
        && ((navpoint *)script_editor_sel.object)->type == NAVPOINT_TYPE_CAMERA)
        return;

    if (script_editor_pos_button.is_latched()) {
        if (script_editor_y_button.is_latched()) {
            pos = script_editor_sel.pos;
            height = NULL;
            if (script_editor_sel.kind == SE_PICK_NAVPOINT)
                height = &((navpoint *)script_editor_sel.object)->ground_height;
            n.x = camera_view_orient.fvec.x;
            n.y = 0.0f;
            n.z = camera_view_orient.fvec.z;
            if (n.is_zero())
                return;
            if (height == NULL)
                return;
            pl.set_normal_point(&n, &script_editor_sel.pos);
            ix_ray_plane(&script_editor_mouse_ray, &pl, &hit);
            *height = hit.y - script_editor_sel.pos.y + *height;
            if (*height < 0.0f) {
                pos.y = hit.y - *height;
                *height = 0.0f;
            } else {
                pos.y = hit.y;
            }
            np = (navpoint *)script_editor_sel.object;
            if (np->type == NAVPOINT_TYPE_AMBIENT) {
                index = level_ambient_find_at(&np->pos);
                if (index >= 0) {
                    vel.x = 0.0f;
                    vel.y = 0.0f;
                    level_ambients[index].pos = pos;
                    vel.z = 0.0f;
                    sound_emitter_update_position(level_ambients[index].emitter, &pos, &vel, 1.75f);
                }
            }
            script_editor_sel.pos = pos;
            return;
        }
        if (script_editor_sel.kind == SE_PICK_NAVPOINT) {
            np = (navpoint *)script_editor_sel.object;
            ray.start = script_editor_mouse_ray.origin;
            ray.dir = script_editor_mouse_ray.dir * 1000.0f;
            ray._unk18 = 0.0f;
            ray.flags = 0;
            ray.require_flags = np->type != NAVPOINT_TYPE_AMBIENT ? 0x40 : 0;
            ray.exclude_flags = 0;
            lobj_raycast_all(&ray, &lhit);
            if (lhit.num_hits == 0)
                return;
            pos = ray.start + ray.dir * lhit.t;
            pos.y = pos.y + np->ground_height;
            if (np->type == NAVPOINT_TYPE_AMBIENT) {
                index = level_ambient_find_at(&np->pos);
                if (index >= 0) {
                    vel.x = 0.0f;
                    vel.y = 0.0f;
                    level_ambients[index].pos = pos;
                    vel.z = 0.0f;
                    sound_emitter_update_position(level_ambients[index].emitter, &pos, &vel, 1.75f);
                }
                layer = -1;
            } else {
                layer = lobj_from_index(lhit.face)->get_layer_n(0);
                if (layer < 0 || layer >= 20)
                    return;
            }
            np->pos = pos;
            np->layer = layer;
            return;
        }
        if (script_editor_sel.kind == SE_PICK_CHARACTER) {
            Character *c;
            ray.start = script_editor_mouse_ray.origin;
            ray.dir = script_editor_mouse_ray.dir * 1000.0f;
            ray._unk18 = 0.0f;
            ray.flags = 0;
            ray.require_flags = 0x40;
            ray.exclude_flags = 0;
            lobj_raycast_all(&ray, &lhit);
            if (lhit.num_hits == 0)
                return;
            pos = ray.start + ray.dir * lhit.t;
            layer = lobj_from_index(lhit.face)->get_layer_n(0);
            c = (Character *)script_editor_sel.object;
            c->pos = pos;
            c->layer = layer;
        }
        return;
    }

    if (script_editor_ori_button.is_latched()) {
        mouse_get_delta(&dx, &dy, &game_mouse_z);
        if (script_editor_y_button.is_latched()) {
            rot.set_from_angles(dy * 0.008f, 0, 0);
        } else {
            int ndx = -dx;
            rot.set_from_angles(0, 0, ndx * 0.008f);
        }
        switch (script_editor_sel.kind) {
        case SE_PICK_CHARACTER:
            if (!script_editor_y_button.is_latched()) {
                Character *c = (Character *)script_editor_sel.object;
                c->orient = c->orient * rot;
            }
            break;
        case SE_PICK_NAVPOINT:
            np = (navpoint *)script_editor_sel.object;
            np->heading += dx * 0.008f;
            if (np->heading > 3.1415927f)
                np->heading -= 6.2831855f;
            if (np->heading < -3.1415927f)
                np->heading += 6.2831855f;
            break;
        }
    }
}

// 0x4e4890  Confidence: exact. A text argument into an action: overwritten in place when it fits, else a
// new copy in the string pool of the script kind being edited (the old one is lost: the pools never free).
void script_editor_store_string(script_action *a, const char *s)
{
    if (a->text != NULL && strlen(s) <= strlen(a->text)) {
        strcpy((char *)a->text, s);
        return;
    }
    if (script_editor_cutscene_button.is_latched())
        a->text = cutscene_script_stringpool.Add(s);
    else
        a->text = level_script_stringpool.Add(s);
}

// 0x4e4910  Confidence: likely (the tail merges of the sprintf cases; jump table 0x4e4b50). The action list
// line of one action: its type name into name, its arguments into args. Moves (play action, die) are
// shown by their $Variation id (the move of c's char_info with that anim), "-1" when c has none. Game
// bug: a type above 0x20 leaves args as the uninitialised buffer (script_parse_objects never stores one).
void script_action_format(char *name, char *args, script_action *a, Character *c)
{
    char buf[0x400];
    move_info *move;

    strcpy(name, script_action_names[a->type]);
    switch (a->type) {
    case SCRIPT_ACTION_WALK_TO:
    case SCRIPT_ACTION_RUN_TO:
    case SCRIPT_ACTION_MOVE_THROUGH:
    case SCRIPT_ACTION_TELEPORT_TO:
        sprintf(buf, "\"%s\"", a->nav->name);
        break;
    case SCRIPT_ACTION_WAIT_AT:
    case SCRIPT_ACTION_TRIGGER_NAV:
        sprintf(buf, "\"%s\" %d", a->nav->name, a->arg0c);
        break;
    case SCRIPT_ACTION_WANDER:
        sprintf(buf, "\"%s\" %d", a->nav->name, a->arg0c);
        if (!a->flag14)
            strcat(buf, " run");
        break;
    case SCRIPT_ACTION_WANDER_FOR:
        sprintf(buf, "\"%s\" %d %d", a->nav->name, a->arg0c, a->arg10);
        if (!a->flag14)
            strcat(buf, " run");
        break;
    case SCRIPT_ACTION_DO_STATE:
        sprintf(buf, "\"%s\"", locomotion_anim_names[a->arg0c]);
        break;
    case SCRIPT_ACTION_PLAY_ACTION:
    case SCRIPT_ACTION_DIE:
        move = charinfo_find_move_any(c->info, a->arg0c, true);
        if (move != NULL)
            sprintf(buf, "%d", move->id);
        else
            strcpy(buf, "-1");
        break;
    case SCRIPT_ACTION_PLAY_MVF:
    case SCRIPT_ACTION_PLAY_EFFECT:
        sprintf(buf, "\"%s\"", a->text);
        break;
    case SCRIPT_ACTION_TURN_HOSTILE:
    case SCRIPT_ACTION_LABEL:
    case SCRIPT_ACTION_GOTO:
    case SCRIPT_ACTION_NO_BLOCK:
    case SCRIPT_ACTION_TRIGGER:
    case SCRIPT_ACTION_TRIGGER_VISIT:
    case SCRIPT_ACTION_SHOW_HIDE:
    case SCRIPT_ACTION_SYNCH:
    case SCRIPT_ACTION_COMBAT_MODE:
    case SCRIPT_ACTION_GUARD_SPLINE:
    case SCRIPT_ACTION_ONE_HAND:
        sprintf(buf, "%d", a->arg0c);
        break;
    case SCRIPT_ACTION_WAIT_GO:
    case SCRIPT_ACTION_MOUNT_ANIM:
    case SCRIPT_ACTION_DISMOUNT_ANIM:
        buf[0] = 0;
        break;
    case SCRIPT_ACTION_SAY:
        sprintf(buf, "\"%s\" %d", a->text, a->arg0c);
        break;
    case SCRIPT_ACTION_PASS_GO:
        sprintf(buf, "\"%s\"", a->text);
        break;
    case SCRIPT_ACTION_SYNCH_TELEPORT:
        sprintf(buf, "%d \"%s\"", a->arg0c, a->nav->name);
        break;
    case SCRIPT_ACTION_FOLLOW:
        sprintf(buf, "\"%s\" %d", a->text, a->arg0c);
        break;
    case SCRIPT_ACTION_REPLACE_WEAPON:
    case SCRIPT_ACTION_REPLACE_ARMOR:
        if (a->arg0c < 0)
            sprintf(buf, "\"(none)\"");
        else
            sprintf(buf, "\"%s\"", item_types[a->arg0c].name);
        break;
    }
    strcpy(args, buf);
}

// 0x4e4be0  Confidence: exact (jump table 0x4e4cd8). Fills the main bar for the new selection: a character
// gets the action list and its name (the party members' panel only while editing a cutscene script), a
// navpoint its name, a level object only its name in the (hidden) box. The box's length limit follows
// the record: 31 for character names, 14 for navpoint::name[16], 256 for objects.
void script_editor_show_selection_panel()
{
    switch (script_editor_sel.kind) {
    case SE_PICK_CHARACTER:
        script_editor_selection_panel.set_visible(script_editor_cutscene_button.is_latched()
                                                  || ((Character *)script_editor_sel.object)->type != 7);
        script_editor_name_inputbox.set_visible(script_editor_cutscene_button.is_latched()
                                                || ((Character *)script_editor_sel.object)->type != 7);
        script_editor_action_listbox.set_visible(true);
        script_editor_fill_action_list();
        script_editor_name_inputbox.max_length = 0x1f;
        script_editor_name_inputbox.set_text(((Character *)script_editor_sel.object)->get_name());
        break;
    case SE_PICK_NAVPOINT:
        script_editor_name_inputbox.max_length = 0xe;
        script_editor_name_inputbox.set_text(((navpoint *)script_editor_sel.object)->name);
        script_editor_name_inputbox.set_visible(true);
        break;
    case SE_PICK_DOOR:
    case SE_PICK_OBJECT:
        script_editor_name_inputbox.max_length = 0x100;
        script_editor_name_inputbox.set_text(((lobj *)script_editor_sel.object)->name);
        script_editor_name_inputbox.set_hidden(true);
        break;
    }
}

// 0x4e4cf0  Confidence: exact. The selected character's script (cutscene or level, by the Cutscene
// Script button) into the action list; append / insert are offered while there is room (200 scripts,
// 54 actions).
void script_editor_fill_action_list()
{
    char args[0x104], name[0x104];
    script_object *so = NULL;
    ui_listbox_entry *e;
    int i;

    script_editor_cur_action = NULL;
    script_editor_action_listbox.clear();
    if (script_editor_sel.object == NULL)
        return;
    if (script_editor_cutscene_button.is_latched()) {
        for (i = 0; i < cutscene_script_object_count; i++) {
            if (cutscene_script_objects[i].handle == ((Character *)script_editor_sel.object)->handle) {
                so = &cutscene_script_objects[i];
                break;
            }
        }
    } else {
        for (i = 0; i < script_object_count; i++) {
            if (script_objects[i].handle == ((Character *)script_editor_sel.object)->handle) {
                so = &script_objects[i];
                break;
            }
        }
    }
    script_editor_cur_script = so;
    script_editor_delete_action_button.set_hidden(true);
    if (so == NULL) {
        if (script_object_count < MAX_SCRIPT_OBJECTS) {
            script_editor_append_action_button.set_visible(true);
            script_editor_insert_action_button.set_visible(true);
        } else {
            script_editor_append_action_button.set_hidden(true);
            script_editor_insert_action_button.set_hidden(true);
        }
        return;
    }
    if (so->num_actions < 54) {
        script_editor_append_action_button.set_visible(true);
        script_editor_insert_action_button.set_visible(true);
    } else {
        script_editor_append_action_button.set_hidden(true);
        script_editor_insert_action_button.set_hidden(true);
    }
    for (i = 0; i < so->num_actions; i++) {
        script_action_format(name, args, &so->actions[i], (Character *)script_editor_sel.object);
        sprintf(string_buffer, "%s %s", name, args);
        e = script_editor_action_listbox.add_entry(string_buffer, -1, 0, 0, 0);
        e->data = i;
        e->set_font(font_small);
        if (script_editor_cutscene_button.is_latched())
            e->colors = script_editor_cutscene_entry_colors;
        else
            e->colors = script_editor_level_entry_colors;
    }
    if (script_editor_sel_action_index < so->num_actions)
        script_editor_action_listbox.select_by_data(script_editor_sel_action_index, true);
}

// 0x4e4f10  Confidence: exact (jump table 0x4e5040). The creature window: team radios, level (the
// override when the character has +0x320 bit 0x400000, else "0") and the char_info's default level. A
// team above 3 leaves the window unfilled. The team comes from c, the level fields from the selection
// (the same character at both call sites).
void script_editor_show_creature_info(Character *c)
{
    char buf[64];

    switch (c->get_team()) {
    case 0:
        script_editor_creature_friendly_radio.set_checked(true);
        script_editor_creature_neutral_radio.set_checked(false);
        script_editor_creature_hostile_radio.set_checked(false);
        script_editor_creature_evil_radio.set_checked(false);
        break;
    case 1:
        script_editor_creature_friendly_radio.set_checked(false);
        script_editor_creature_neutral_radio.set_checked(true);
        script_editor_creature_hostile_radio.set_checked(false);
        script_editor_creature_evil_radio.set_checked(false);
        break;
    case 2:
        script_editor_creature_friendly_radio.set_checked(false);
        script_editor_creature_neutral_radio.set_checked(false);
        script_editor_creature_hostile_radio.set_checked(true);
        script_editor_creature_evil_radio.set_checked(false);
        break;
    case 3:
        script_editor_creature_friendly_radio.set_checked(false);
        script_editor_creature_neutral_radio.set_checked(false);
        script_editor_creature_hostile_radio.set_checked(false);
        script_editor_creature_evil_radio.set_checked(true);
        break;
    default:
        return;
    }
    if (((Character *)script_editor_sel.object)->flags & 0x400000)
        sprintf(buf, "%d", ((Character *)script_editor_sel.object)->level);
    else
        sprintf(buf, "0");
    script_editor_creature_level_inputbox.set_text(buf);
    sprintf(buf, "(default level = %d)", ((Character *)script_editor_sel.object)->info->level);
    script_editor_creature_info_label.set_text(buf);
}

// 0x4e5050  Confidence: exact. The level item window for a level navpoint: its item and count, or an
// empty name and a count of 1.
void script_editor_show_level_item(navpoint *np)
{
    char buf[8];

    if (np->level_item >= 0) {
        level_item_rec *li = &level_items[np->level_item];
        script_editor_level_item_name_inputbox.set_text(li->type->name);
        script_editor_level_item_listbox.select_by_text(li->type->name, true);
        sprintf(buf, "%d", li->count);
        script_editor_level_item_count_inputbox.set_text(buf);
    } else {
        script_editor_level_item_name_inputbox.set_text("");
        script_editor_level_item_listbox.clear_selection();
        script_editor_level_item_count_inputbox.set_text("1");
    }
}

// 0x4e50f0  Confidence: exact (the four defaults: 1.0 min range, volume and roll-off, 0 delay). The ambient
// window for an ambient navpoint: the emitter's sound and parameters (repeat delay in seconds), the .wav
// files of data\sounds\ambient, the current one selected; focus on the name box.
void script_editor_list_ambient_sounds(navpoint *np)
{
    char path[0x100];
    char name[0x100] = "";
    char buf[0x20];
    float min_range = 1.0f, volume = 1.0f, rolloff = 1.0f, delay = 0.0f;
    file_find_node list;
    file_find_node *node;
    ui_listbox_entry *e;
    int index, path_id;

    index = level_ambient_find_at(&np->pos);
    if (index >= 0) {
        snd_sound *s = &sounds[sound_emitter_get_sound(level_ambients[index].emitter)];
        level_ambient *amb = &level_ambients[index];
        strcpy(path, s->name);
        file_get_filename(name, path);
        min_range = s->min_range;
        volume = s->volume;
        rolloff = s->rolloff;
        delay = sound_emitter_get_repeat_delay(amb->emitter) * 0.001f;
    }
    script_editor_ambient_name_inputbox.set_text(name);
    sprintf(buf, "%.1f", min_range);
    script_editor_ambient_min_range_inputbox.set_text(buf);
    sprintf(buf, "%.1f", volume);
    script_editor_ambient_volume_inputbox.set_text(buf);
    sprintf(buf, "%.1f", rolloff);
    script_editor_ambient_rolloff_inputbox.set_text(buf);
    sprintf(buf, "%.1f", delay);
    script_editor_ambient_loop_delay_inputbox.set_text(buf);

    path_id = file_add_path("data\\sounds\\ambient", ".wav");
    file_find_files(&list, path_id, "*.wav", 5, 0);
    script_editor_ambient_listbox.clear();
    for (node = list.next; node != (file_find_node *)&list; node = node->next) {
        e = script_editor_ambient_listbox.add_entry(node->name, -1, 0, 0, 0);
        e->set_font(font_small);
    }
    for (e = script_editor_ambient_listbox.entries.next; e != &script_editor_ambient_listbox.entries; e = e->next) {
        if (_stricmp(e->text, name) == 0) {
            script_editor_ambient_listbox.select_entry(e, true);
            break;
        }
    }
    file_remove_path(path_id);
    while (list.next != (file_find_node *)&list) {
        list.prev = list.next;
        list.next = list.next->next;
        delete list.prev;
    }
    list.next = (file_find_node *)&list;
    list.prev = (file_find_node *)&list;
    script_editor_ambient_name_inputbox.set_focus();
}

// 0x4e53a0  Confidence: likely (the order of the enable / disable calls). The door window: the .wav files of
// data\sounds\Doors in both sound lists (the opening set active), the door's two sounds (name after the
// backslash selected in the list), the lock, its pick skill (percent) and key. Game bug: the three sound
// parameters are bytes but are printed with "%.1f", which reads a double from the four-byte argument
// and the next stack slot, so the fields show garbage (script_editor_door_ok writes them back with atof).
void script_editor_list_door_sounds(lobj *l)
{
    char buf[24];
    file_find_node list;
    file_find_node *node;
    ui_listbox_entry *e;
    door_info *d;
    int path_id;

    d = door_from_lobj(l);
    path_id = file_add_path("data\\sounds\\Doors", ".wav");
    file_find_files(&list, path_id, "*.wav", 5, 0);
    script_editor_door_open_sound_listbox.clear();
    script_editor_door_close_sound_listbox.clear();
    for (node = list.next; node != (file_find_node *)&list; node = node->next) {
        e = script_editor_door_open_sound_listbox.add_entry(node->name, -1, 0, 0, 0);
        e->set_font(font_small);
        e = script_editor_door_close_sound_listbox.add_entry(node->name, -1, 0, 0, 0);
        e->set_font(font_small);
    }
    file_remove_path(path_id);
    while (list.next != (file_find_node *)&list) {
        list.prev = list.next;
        list.next = list.next->next;
        delete list.prev;
    }
    list.next = (file_find_node *)&list;
    list.prev = (file_find_node *)&list;

    script_editor_door_opening_radio.set_checked(true);
    script_editor_door_open_sound_listbox.set_enabled(true);
    script_editor_door_open_min_range_inputbox.set_enabled(true);
    script_editor_door_open_volume_inputbox.set_enabled(true);
    script_editor_door_open_rolloff_inputbox.set_enabled(true);
    script_editor_door_closing_radio.set_checked(false);
    script_editor_door_close_sound_listbox.set_disabled(true);
    script_editor_door_close_min_range_inputbox.set_disabled(true);
    script_editor_door_close_volume_inputbox.set_disabled(true);
    script_editor_door_close_rolloff_inputbox.set_disabled(true);

    if (d->open_sound.name != NULL) {
        const char *file = strchr(d->open_sound.name, '\\') + 1;
        script_editor_door_open_sound_listbox.select_by_text(file, true);
        script_editor_door_sound_inputbox.set_text(d->open_sound.name);
        sprintf(buf, "%.1f", d->open_sound.params[1]);
        script_editor_door_open_min_range_inputbox.set_text(buf);
        sprintf(buf, "%.1f", d->open_sound.params[0]);
        script_editor_door_open_volume_inputbox.set_text(buf);
        sprintf(buf, "%.1f", d->open_sound.params[2]);
        script_editor_door_open_rolloff_inputbox.set_text(buf);
    } else {
        script_editor_door_sound_inputbox.set_text("");
        script_editor_door_open_min_range_inputbox.set_text("0.0");
        script_editor_door_open_volume_inputbox.set_text("0.0");
        script_editor_door_open_rolloff_inputbox.set_text("1.0");
    }
    if (d->close_sound.name != NULL) {
        const char *file = strchr(d->close_sound.name, '\\') + 1;
        script_editor_door_close_sound_listbox.select_by_text(file, true);
        sprintf(buf, "%.1f", d->close_sound.params[1]);
        script_editor_door_close_min_range_inputbox.set_text(buf);
        sprintf(buf, "%.1f", d->close_sound.params[0]);
        script_editor_door_close_volume_inputbox.set_text(buf);
        sprintf(buf, "%.1f", d->close_sound.params[2]);
        script_editor_door_close_rolloff_inputbox.set_text(buf);
    } else {
        script_editor_door_close_min_range_inputbox.set_text("0.0");
        script_editor_door_close_volume_inputbox.set_text("0.0");
        script_editor_door_close_rolloff_inputbox.set_text("0.1");
    }

    if (d->state & 8) {
        script_editor_door_locked_checkbox.set_checked(true);
        sprintf(buf, "%d", (int)(d->lock * 100.0f));
        script_editor_door_skill_inputbox.set_text(buf);
        script_editor_door_skill_inputbox.set_visible(true);
        script_editor_door_key_inputbox.set_visible(true);
        script_editor_door_key_listbox.set_visible(true);
        if (d->key != NULL) {
            script_editor_door_key_inputbox.set_text(d->key->name);
            script_editor_door_key_listbox.select_by_text(d->key->name, true);
            return;
        }
    } else {
        script_editor_door_locked_checkbox.set_checked(false);
        script_editor_door_skill_inputbox.set_text("100");
        script_editor_door_skill_inputbox.set_hidden(true);
        script_editor_door_key_inputbox.set_hidden(true);
        script_editor_door_key_listbox.set_hidden(true);
    }
    script_editor_door_key_inputbox.set_text("");
    script_editor_door_key_listbox.clear_selection();
}

// 0x4e57a0  Confidence: exact. The door lock / clickable object window: the object's name (set twice, the
// box stays hidden), the lc_entries record's text and its gold (Give item). Game bug: an object without a
// record (none: script_editor_handle_input adds one first) would read the record at NULL.
void script_editor_show_door_lock(lobj *l)
{
    char buf[16];
    lc_entry *rec;
    int i;

    script_editor_door_lock_name_inputbox.set_text(l->name);
    script_editor_door_lock_name_inputbox.set_hidden(true);
    rec = NULL;
    for (i = 0; i < lc_entry_count; i++) {
        if (lc_entries[i].object == l)
            rec = &lc_entries[i];
    }
    script_editor_door_lock_name_inputbox.set_text(l->name);
    script_editor_door_lock_name_inputbox.set_hidden(true);
    script_editor_door_lock_item_inputbox.set_text(rec->name);
    script_editor_door_lock_item_inputbox.set_visible(true);
    if (rec->gold != 0) {
        script_editor_door_lock_give_checkbox.set_checked(true);
        script_editor_door_lock_item_listbox.set_visible(true);
        script_editor_door_lock_value_inputbox.set_visible(true);
        sprintf(buf, "%d", rec->gold);
        script_editor_door_lock_value_inputbox.set_text(buf);
    } else {
        script_editor_door_lock_give_checkbox.set_checked(false);
        script_editor_door_lock_item_listbox.set_hidden(true);
        script_editor_door_lock_value_inputbox.set_hidden(true);
    }
}

// 0x4e58a0  Confidence: likely (jump table 0x4e5afc over the char_info team; the empty loop). New creature
// mode: a character of the type picked in the New window at the hovered navpoint, unless a character
// already starts there or stands exactly there. Teams 0 / 1 make a type 4 (friendly), teams 2 / 3 a type 5
// (monster, put on team 2), a boss species a type 9. The name is "<char_info>#<navpoint>", the char_info
// name cut so that the whole stays within 31 characters. The loop over the live list before set_name has
// no body (it is in the binary: the walk to the end of the list is all that is left).
void script_editor_place_creature()
{
    Character *c, *nc;
    navpoint *np;
    matrix orient;
    char name[0x100];
    int type, n;

    type = 5;
    if (script_editor_hover.kind != SE_PICK_NAVPOINT || script_editor_hover.object == NULL)
        return;
    np = (navpoint *)script_editor_hover.object;
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->info->species == 0x59 && character_is_serpent_part(c))
            continue;
        if (c->start_navpoint == np) {
            sprintf(string_buffer, "That navpoint is already the start for creature \"%s\"", c->get_name());
            script_editor_set_status(string_buffer);
            return;
        }
        if (c->pos.x == np->pos.x && c->pos.y == np->pos.y && c->pos.z == np->pos.z && c->layer == np->layer) {
            sprintf(string_buffer, "Creature \"%s\" already occupies that point in space", c->get_name());
            script_editor_set_status(string_buffer);
            return;
        }
    }
    switch (char_infos[script_editor_new_selection].team) {
    case 0:
    case 1:
        type = 4;
        break;
    case 2:
    case 3:
        type = 5;
        break;
    }
    if (species_is_boss(char_infos[script_editor_new_selection].species))
        type = 9;
    orient.set_from_angles(0, 0, np->heading);
    nc = living_entity_create(type, script_editor_new_selection, &np->pos, &orient, true, np->layer, 0, 0, false);
    nc->start_navpoint = np;
    if (type == 5)
        nc->set_team(2);
    sprintf(name, "%s#%s", nc->info->name, ((navpoint *)nc->start_navpoint)->name);
    if (strlen(name) > 0x1f) {
        n = 0x1e - strlen(((navpoint *)nc->start_navpoint)->name);
        strncpy(name, nc->info->name, n);
        name[n] = '#';
        strcpy(&name[n + 1], ((navpoint *)nc->start_navpoint)->name);
    }
    for (c = living_list.live_next; c != &living_list; c = c->live_next)
        ;
    nc->set_name(name);
    script_editor_new_button.set_latched(false);
}

// 0x4e5b10  Confidence: likely (the type tests: the binary checks the limit per type, then 0 / 1 and 3).
// A navpoint where the mouse ray meets the level (1000 units): a level navpoint ("$npc%.3d", on a
// layer-flagged floor the pathfinder can use), a cutscene navpoint ("$cut%.3d", 512 at most although the
// table holds 726), or an ambient navpoint ("$ambient%.3d", on any geometry, no layer: the layer
// lookup's result is dropped). The first free number is used; the new navpoint becomes the hover. A miss
// returns with New still latched.
void script_editor_add_navpoint(int type)
{
    char name[0x20];
    vector3 pos, size;
    lobj_ray ray;
    lobj_hit hit;
    matrix orient;
    navpoint *np;
    int i, layer;

    size.x = 0.8f;
    size.y = 0.4f;
    size.z = 0.8f;
    switch (type) {
    case NAVPOINT_TYPE_LEVEL:
    case NAVPOINT_TYPE_AMBIENT:
        if (navpoint_count == MAX_NAVPOINTS) {
            script_editor_set_status("Limit reached - can't add any more level navpoints");
            return;
        }
        break;
    case NAVPOINT_TYPE_CUTSCENE:
        if (cutscene_navpoint_count == 0x200) {
            script_editor_set_status("Limit reached - can't add any more cutscene navpoints");
            return;
        }
        break;
    }
    if (type == NAVPOINT_TYPE_LEVEL || type == NAVPOINT_TYPE_CUTSCENE) {
        ray.start = script_editor_mouse_ray.origin;
        ray.dir = script_editor_mouse_ray.dir * 1000.0f;
        ray._unk18 = 0.0f;
        ray.flags = 0;
        ray.require_flags = 0x40;
        ray.exclude_flags = 0;
        lobj_raycast_all(&ray, &hit);
        if (hit.num_hits == 0)
            return;
        pos = ray.start + ray.dir * hit.t;
        layer = lobj_from_index(hit.face)->pick_layer(&pos, 1);
        if (layer < 0 || layer >= 20)
            return;
        if (type == NAVPOINT_TYPE_LEVEL) {
            int l = lobj_from_index(hit.face)->pick_layer(&pos, 1);
            if (l == -1)
                return;
            if (!pf_point_is_navigable(&pos, 1, l))
                return;
            np = &navpoints[navpoint_count++];
        } else {
            np = &cutscene_navpoints[cutscene_navpoint_count++];
        }
        np->pos = pos;
        np->heading = 0.0f;
        np->ground_height = 0;
        np->layer = layer;
        np->type = type;
        i = 1;
        do {
            switch (type) {
            case NAVPOINT_TYPE_LEVEL:
                sprintf(name, "$npc%.3d", i);
                break;
            case NAVPOINT_TYPE_CUTSCENE:
                sprintf(name, "$cut%.3d", i);
                break;
            }
            i++;
        } while (navpoint_find(name) != NULL);
        strcpy(np->name, name);
        orient.set_from_angles(0, 0, np->heading);
        if (script_editor_ray_hit_object(&np->pos, &orient, &size, 0.693f)) {
            script_editor_hover.kind = SE_PICK_NAVPOINT;
            script_editor_hover.object = np;
        }
        np->level_item = -1;
    } else if (type == NAVPOINT_TYPE_AMBIENT) {
        ray.start = script_editor_mouse_ray.origin;
        ray.dir = script_editor_mouse_ray.dir * 1000.0f;
        ray._unk18 = 0.0f;
        ray.flags = 0;
        ray.require_flags = 0;
        ray.exclude_flags = 0;
        lobj_raycast_all(&ray, &hit);
        if (hit.num_hits == 0)
            return;
        pos = ray.start + ray.dir * hit.t;
        lobj_from_index(hit.face)->pick_layer(&pos, 1);
        np = &navpoints[navpoint_count++];
        np->pos = pos;
        np->heading = 0.0f;
        np->ground_height = 0;
        np->layer = -1;
        np->type = type;
        i = 1;
        do {
            sprintf(name, "$ambient%.3d", i);
            i++;
        } while (navpoint_find(name) != NULL);
        strcpy(np->name, name);
        orient.set_from_angles(0, 0, np->heading);
        if (script_editor_ray_hit_object(&np->pos, &orient, &size, 0.693f)) {
            script_editor_hover.kind = SE_PICK_NAVPOINT;
            script_editor_hover.object = np;
        }
        np->level_item = -1;
    }
    script_editor_new_button.set_latched(false);
}

// 0x4e6040  Confidence: likely (the change / edit flags as two locals; jump tables 0x4e6288 / 0x4e6298 over
// the action type and 0x4e62bc over the mode). Once per editor frame: the button states. A selected
// action line shows Change (the argument is picked in the world or from a list) and Edit (it has numbers to
// type) by its type; the pick filter follows the mode unless New or Change is latched, and a selection of
// another kind is dropped.
void script_editor_update_buttons()
{
    ui_listbox_entry *e;
    bool show_change, show_edit;

    script_editor_pause_button.set_latched(game_is_paused());
    e = script_editor_action_listbox.get_selected();
    if (e != NULL && script_editor_sel.kind != SE_PICK_NONE) {
        show_change = false;
        show_edit = false;
        if (script_editor_cur_action != e)
            script_editor_change_button.set_latched(false);
        script_editor_cur_action = e;
        script_editor_new_button.set_latched(false);
        script_editor_delete_action_button.set_visible(true);
        if (script_editor_cur_script->num_actions > 1) {
            script_editor_action_up_button.set_visible(true);
            script_editor_action_down_button.set_visible(true);
        } else {
            script_editor_action_up_button.set_hidden(true);
            script_editor_action_down_button.set_hidden(true);
        }
        switch (script_editor_cur_script->actions[e->data].type) {
        case SCRIPT_ACTION_WALK_TO: case SCRIPT_ACTION_RUN_TO: case SCRIPT_ACTION_MOVE_THROUGH:
        case SCRIPT_ACTION_DO_STATE: case SCRIPT_ACTION_PLAY_ACTION: case SCRIPT_ACTION_PLAY_MVF:
        case SCRIPT_ACTION_PLAY_EFFECT: case SCRIPT_ACTION_DIE: case SCRIPT_ACTION_TELEPORT_TO:
        case SCRIPT_ACTION_REPLACE_WEAPON: case SCRIPT_ACTION_REPLACE_ARMOR:
            show_change = true;
            show_edit = false;
            break;
        case SCRIPT_ACTION_WAIT_AT: case SCRIPT_ACTION_WANDER: case SCRIPT_ACTION_WANDER_FOR:
        case SCRIPT_ACTION_TRIGGER_NAV: case SCRIPT_ACTION_PASS_GO: case SCRIPT_ACTION_SYNCH_TELEPORT:
        case SCRIPT_ACTION_FOLLOW:
            show_change = true;
            show_edit = true;
            break;
        case SCRIPT_ACTION_TURN_HOSTILE: case SCRIPT_ACTION_SAY: case SCRIPT_ACTION_LABEL:
        case SCRIPT_ACTION_GOTO: case SCRIPT_ACTION_NO_BLOCK: case SCRIPT_ACTION_TRIGGER:
        case SCRIPT_ACTION_TRIGGER_VISIT: case SCRIPT_ACTION_SHOW_HIDE: case SCRIPT_ACTION_SYNCH:
        case SCRIPT_ACTION_COMBAT_MODE: case SCRIPT_ACTION_GUARD_SPLINE: case SCRIPT_ACTION_ONE_HAND:
            show_change = false;
            show_edit = true;
            break;
        case SCRIPT_ACTION_WAIT_GO: case SCRIPT_ACTION_MOUNT_ANIM: case SCRIPT_ACTION_DISMOUNT_ANIM:
            show_change = false;
            show_edit = false;
            break;
        }
        script_editor_change_button.set_visible(show_change);
        script_editor_edit_button.set_visible(show_edit);
    } else {
        script_editor_delete_action_button.set_hidden(true);
        script_editor_action_up_button.set_hidden(true);
        script_editor_action_down_button.set_hidden(true);
        script_editor_change_button.set_hidden(true);
        script_editor_edit_button.set_hidden(true);
    }
    if (script_editor_lists_dirty) {
        script_editor_lists_dirty = false;
        script_editor_fill_action_lists();
    }
    if (!script_editor_new_button.is_latched() && !script_editor_change_button.is_latched()) {
        switch (script_editor_mode) {
        case SE_MODE_CREATURE:
            script_editor_pick_filter = 1;
            break;
        case SE_MODE_NAVPOINT:
        case SE_MODE_CAMERA:
        case SE_MODE_AMBIENT:
            script_editor_pick_filter = 2;
            break;
        case SE_MODE_DOOR:
            script_editor_pick_filter = 3;
            break;
        case SE_MODE_CLICK_OBJECT:
            script_editor_pick_filter = 4;
            break;
        }
    }
    if (script_editor_sel.kind != script_editor_pick_filter && !script_editor_change_button.is_latched())
        script_editor_clear_selection();
    if (!script_editor_new_ok_button.is_disabled()) {
        if (script_editor_new_listbox.get_selected() != NULL)
            script_editor_new_ok_button.set_visible(true);
        else
            script_editor_new_ok_button.set_hidden(true);
    }
    if (!script_editor_action_ok_button.is_disabled()) {
        if (script_editor_action_type_listbox.get_selected() != NULL)
            script_editor_action_ok_button.set_visible(true);
        else
            script_editor_action_ok_button.set_hidden(true);
    }
}

// The file list free loop (the head left dangling; the binary does not reset it here).
#define SE_FREE_FILE_LIST(list)                                                 \
    do {                                                                        \
        while ((list).next != (file_find_node *)&(list)) {                      \
            (list).prev = (list).next;                                          \
            (list).next = (list).next->next;                                    \
            delete (list).prev;                                                 \
        }                                                                       \
    } while (0)

// 0x4e62e0  Confidence: likely (jump table 0x4e6890 / 0x4e68b4 over the action type; the .mvf / .vfx free
// loops). Change for the selected action (a second press unlatches): navpoint and character arguments are
// picked in the world (Change stays latched, the pick filter set), the rest from a list in the action
// window: locomotion anims (kind 2), the char_info's category 8 moves 1..30 (kind 3, and kind 9 for die,
// which offers the 6 / 0x22 death move when there is one), the cutscene's .mvf (4) and .vfx (5) files,
// weapons or the body armour / armour slots the character can use (10, sorted, "(none)" = -1).
void script_editor_fill_action_lists()
{
    script_action *a;
    ui_listbox_entry *e;
    move_info *m;
    file_find_node list;
    file_find_node *node;
    int i;

    if (script_editor_change_button.is_latched()) {
        script_editor_change_button.set_latched(false);
        return;
    }
    a = &script_editor_cur_script->actions[script_editor_cur_action->data];
    switch (a->type) {
    case SCRIPT_ACTION_WALK_TO: case SCRIPT_ACTION_RUN_TO: case SCRIPT_ACTION_MOVE_THROUGH:
    case SCRIPT_ACTION_WAIT_AT: case SCRIPT_ACTION_WANDER: case SCRIPT_ACTION_WANDER_FOR:
    case SCRIPT_ACTION_TRIGGER_NAV: case SCRIPT_ACTION_TELEPORT_TO: case SCRIPT_ACTION_SYNCH_TELEPORT:
        script_editor_change_button.set_latched(true);
        script_editor_pick_filter = 2;
        script_editor_action_type_listbox.set_focus();
        return;
    case SCRIPT_ACTION_PASS_GO:
    case SCRIPT_ACTION_FOLLOW:
        script_editor_change_button.set_latched(true);
        script_editor_pick_filter = 1;
        script_editor_action_type_listbox.set_focus();
        return;
    case SCRIPT_ACTION_DO_STATE:
        script_editor_action_list_kind = 2;
        script_editor_action_window.set_enabled(true);
        script_editor_action_type_listbox.clear();
        for (i = 0; i < NUM_LOCO_ANIMS; i++) {
            if (((Character *)script_editor_sel.object)->loco_anims[i] >= 0) {
                e = script_editor_action_type_listbox.add_entry(locomotion_anim_names[i], -1, 0, 0, 0);
                e->data = i;
                e->set_font(font_small);
                if (i == 0)
                    script_editor_action_type_listbox.select_entry(e, true);
            }
        }
        script_editor_action_type_listbox.set_focus();
        return;
    case SCRIPT_ACTION_DIE:
        script_editor_action_list_kind = 9;
        script_editor_action_window.set_enabled(true);
        script_editor_action_type_listbox.clear();
        m = charinfo_find_move(((Character *)script_editor_sel.object)->info, 6, 0x22, true);
        if (m != NULL) {
            e = script_editor_action_type_listbox.add_entry(m->anim_file, -1, 0, 0, 0);
            e->data = 0;
            e->set_font(font_small);
            script_editor_action_type_listbox.select_entry(e, true);
            script_editor_action_type_listbox.set_focus();
            return;
        }
        for (i = 1; i <= 30; i++) {
            m = charinfo_find_move(((Character *)script_editor_sel.object)->info, 8, i, true);
            if (m != NULL) {
                e = script_editor_action_type_listbox.add_entry(m->anim_file, -1, 0, 0, 0);
                e->data = i;
                e->set_font(font_small);
                if (i == 0)
                    script_editor_action_type_listbox.select_entry(e, true);
            }
        }
        script_editor_action_type_listbox.set_focus();
        return;
    case SCRIPT_ACTION_PLAY_ACTION:
        script_editor_action_list_kind = 3;
        script_editor_action_window.set_enabled(true);
        script_editor_action_type_listbox.clear();
        for (i = 1; i <= 30; i++) {
            m = charinfo_find_move(((Character *)script_editor_sel.object)->info, 8, i, true);
            if (m != NULL) {
                e = script_editor_action_type_listbox.add_entry(m->anim_file, -1, 0, 0, 0);
                e->data = i;
                e->set_font(font_small);
                if (i == 0)
                    script_editor_action_type_listbox.select_entry(e, true);
            }
        }
        script_editor_action_type_listbox.set_focus();
        return;
    case SCRIPT_ACTION_PLAY_MVF:
        script_editor_action_list_kind = 4;
        script_editor_action_window.set_enabled(true);
        script_editor_action_type_listbox.clear();
        file_find_files(&list, cutscene_path_id, "*.mvf", 5, 0);
        for (i = 0, node = list.next; node != (file_find_node *)&list; node = node->next) {
            e = script_editor_action_type_listbox.add_entry(node->name, -1, 0, 0, 0);
            e->data = i++;
            e->set_font(font_small);
        }
        SE_FREE_FILE_LIST(list);
        break;
    case SCRIPT_ACTION_PLAY_EFFECT:
        script_editor_action_list_kind = 5;
        script_editor_action_window.set_enabled(true);
        script_editor_action_type_listbox.clear();
        file_find_files(&list, cutscene_path_id, "*.vfx", 5, 0);
        for (i = 0, node = list.next; node != (file_find_node *)&list; node = node->next) {
            e = script_editor_action_type_listbox.add_entry(node->name, -1, 0, 0, 0);
            e->data = i++;
            e->set_font(font_small);
        }
        SE_FREE_FILE_LIST(list);
        break;
    case SCRIPT_ACTION_REPLACE_WEAPON:
    case SCRIPT_ACTION_REPLACE_ARMOR:
        script_editor_action_list_kind = 10;
        script_editor_action_window.set_enabled(true);
        script_editor_action_type_listbox.clear();
        script_editor_action_type_listbox.set_sorted(true);
        e = script_editor_action_type_listbox.add_entry("(none)", -1, 0, 0, 0);
        e->data = -1;
        e->set_font(font_small);
        if (a->type == SCRIPT_ACTION_REPLACE_WEAPON) {
            for (i = 0; i < num_item_types; i++) {
                if (item_types[i].kind == 0
                    && character_species_can_use_type((Character *)script_editor_sel.object, &item_types[i])) {
                    e = script_editor_action_type_listbox.add_entry(item_types[i].name, -1, 0, 0, 0);
                    e->data = i;
                    e->set_font(font_small);
                }
            }
        } else if (a->type == SCRIPT_ACTION_REPLACE_ARMOR) {
            for (i = 0; i < num_item_types; i++) {
                if (item_types[i].kind == 1 && (item_types[i].slot == 6 || item_types[i].slot == 8)
                    && character_species_can_use_type((Character *)script_editor_sel.object, &item_types[i])) {
                    e = script_editor_action_type_listbox.add_entry(item_types[i].name, -1, 0, 0, 0);
                    e->data = i;
                    e->set_font(font_small);
                }
            }
        }
        script_editor_action_type_listbox.set_sorted(false);
        script_editor_action_type_listbox.select_by_data(a->arg0c, true);
        break;
    }
    script_editor_action_type_listbox.set_focus();
}

// 0x4e68e0  Confidence: exact. The mode button opens the mode list on the current mode.
bool script_editor_mode_button_cb(int event, int value)
{
    if (value == script_editor_mode_button.id && event == UI_EVENT_PRESSED) {
        script_editor_choice_window.set_enabled(true);
        script_editor_choice_listbox.set_focus();
        script_editor_choice_listbox.select_by_data(script_editor_mode, true);
        return true;
    }
    return false;
}

// 0x4e6930  Confidence: exact. Pos and Ori exclude each other; Y toggles on its own.
bool script_editor_on_pos_ori(int event, int value)
{
    ui_gadget *g;

    if (event != UI_EVENT_PRESSED)
        return false;
    g = script_editor_window.find_gadget_by_id(value);
    if (g == &script_editor_pos_button) {
        script_editor_pos_button.toggle_latched();
        script_editor_ori_button.set_latched(false);
        return true;
    }
    if (g == &script_editor_ori_button) {
        script_editor_ori_button.toggle_latched();
        script_editor_pos_button.set_latched(false);
        return true;
    }
    if (g == &script_editor_y_button) {
        script_editor_y_button.toggle_latched();
        return true;
    }
    return false;
}

// 0x4e69a0  Confidence: exact (jump table 0x4e6a20). New: in creature mode opens the character list first;
// creature, navpoint and ambient modes latch New for a click in the world (pick filter navpoints); camera
// mode adds a camera navpoint at the eye at once.
bool script_editor_on_new(int event, int value)
{
    if (value != script_editor_new_button.id || event != UI_EVENT_PRESSED)
        return false;
    switch (script_editor_mode) {
    case SE_MODE_CREATURE:
        if (!script_editor_new_button.is_latched()) {
            script_editor_new_window.set_enabled(true);
            script_editor_new_listbox.set_focus();
            script_editor_new_listbox.select_by_data(script_editor_new_selection, true);
        }
        // fall through
    case SE_MODE_NAVPOINT:
    case SE_MODE_AMBIENT:
        script_editor_pick_filter = 2;
        script_editor_new_button.toggle_latched();
        return true;
    case SE_MODE_CAMERA:
        script_editor_add_camera_navpoint();
        return true;
    }
    return true;
}

// 0x4e6a30  Confidence: exact. MISNAMED in sum.symbols (D3505): it ADDS a camera navpoint ("$camera%.3d",
// the first free number) at the camera eye, heading 0, no layer. Game bug: there is no check against the
// 750 records, so a 751st navpoint is written past navpoints[] over the next .bss variables.
void script_editor_add_camera_navpoint()
{
    char name[0x20];
    navpoint *np;
    int i;

    np = &navpoints[navpoint_count++];
    np->pos = camera_view_pos;
    np->heading = 0.0f;
    np->layer = -1;
    np->ground_height = 0;
    np->type = NAVPOINT_TYPE_CAMERA;
    i = 1;
    do {
        sprintf(name, "$camera%.3d", i);
        i++;
    } while (navpoint_find(name) != NULL);
    strcpy(np->name, name);
}

// 0x4e6ad0  Confidence: exact (one exit, D8701; the record copies need D8703's navpoint; jump tables
// 0x4e6d5c over the navpoint type, 0x4e6d6c over the character type). Del: a navpoint nobody uses
// (script_editor_navpoint_in_use) is removed from its table, the later records moving down one with
// their references (an ambient navpoint also loses its emitter); a monster,
// NPC or boss is removed (the camera goes back to the player, its level script entry is emptied).
// Game bugs: the move loop copies record [count] into [count - 1] (one past the live records; with 750
// level navpoints that reads past the table), and level_items[].navpoint pointers and cutscene script
// actions are not moved with the records (script_replace_navpoint_references does not visit them), so a
// level item placed on a later navpoint is saved under its neighbour's name (level_items.tbl).
bool script_editor_on_delete(int event, int value)
{
    navpoint *np;
    Character *c;
    int i, index;

    if (value == script_editor_delete_button.id && event == UI_EVENT_PRESSED) {
        if (script_editor_sel.kind == SE_PICK_NAVPOINT) {
            if (!script_editor_navpoint_in_use()) {
                if (script_editor_hover.kind == SE_PICK_NAVPOINT
                    && script_editor_hover.object == script_editor_sel.object)
                    script_editor_hover.kind = SE_PICK_NONE;
                np = (navpoint *)script_editor_sel.object;
                switch (np->type) {
                case NAVPOINT_TYPE_CUTSCENE:
                    for (i = np - cutscene_navpoints; i < cutscene_navpoint_count; i++) {
                        script_replace_navpoint_references(&cutscene_navpoints[i + 1], &cutscene_navpoints[i]);
                        cutscene_navpoints[i] = cutscene_navpoints[i + 1];
                    }
                    cutscene_navpoint_count--;
                    break;
                case NAVPOINT_TYPE_AMBIENT:
                    index = level_ambient_find_at(&np->pos);
                    if (index >= 0)
                        level_ambient_remove(index);
                    // fall through
                case NAVPOINT_TYPE_LEVEL:
                case NAVPOINT_TYPE_CAMERA:
                    for (i = (navpoint *)script_editor_sel.object - navpoints; i < navpoint_count; i++) {
                        script_replace_navpoint_references(&navpoints[i + 1], &navpoints[i]);
                        navpoints[i] = navpoints[i + 1];
                    }
                    navpoint_count--;
                    break;
                }
                script_editor_sel.kind = SE_PICK_NONE;
            }
        } else if (script_editor_sel.kind == SE_PICK_CHARACTER) {
            c = (Character *)script_editor_sel.object;
            if (script_editor_hover.kind == SE_PICK_CHARACTER && script_editor_hover.object == c)
                script_editor_hover.kind = SE_PICK_NONE;
            switch (c->type) {
            case 7:
                script_editor_set_status("Can't delete players.");
                break;
            case 4:
            case 5:
            case 9:
                if (game_camera.target == c)
                    game_camera.set_target(cur_character);
                index = script_object_find(((Character *)script_editor_sel.object)->get_name());
                if (index != -1)
                    script_objects[index].num_actions = 0;
                ((Character *)script_editor_sel.object)->deinit();
                script_editor_sel.object = NULL;
                script_editor_sel.kind = SE_PICK_NONE;
                break;
            }
        }
        return true;
    }
    return false;
}

// 0x4e6d90  Confidence: exact. Whether the selected navpoint is in use (status text says by what): a live
// character's start, a level script action (the character found by its handle), or a location trigger.
// Cutscene scripts and level items are not checked.
bool script_editor_navpoint_in_use()
{
    navpoint *np;
    Character *c;
    int i, j;

    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->info->species == 0x59 && character_is_serpent_part(c))
            continue;
        if (c->start_navpoint == script_editor_sel.object) {
            sprintf(string_buffer, "This navpoint is used as starting point for \"%s\"", c->get_name());
            script_editor_set_status(string_buffer);
            return true;
        }
    }
    np = (navpoint *)script_editor_sel.object;
    for (i = 0; i < script_object_count; i++) {
        for (j = 0; j < script_objects[i].num_actions; j++) {
            if (script_objects[i].actions[j].nav == np) {
                c = living_from_handle(script_objects[i].handle);
                if (c != NULL) {
                    sprintf(string_buffer, "This navpoint is used in script for \"%s\"", c->get_name());
                    script_editor_set_status(string_buffer);
                    return true;
                }
                script_editor_set_status("This navpoint is used in script for unknown entity -- find Allender for help");
                return true;
            }
        }
    }
    for (i = 0; i < level_trigger_count; i++) {
        if (level_triggers[i].type == 1 && level_triggers[i].nav == np) {
            sprintf(string_buffer, "Navpoint in use by trigger %s", level_triggers[i].name);
            script_editor_set_status(string_buffer);
            return true;
        }
    }
    return false;
}

// 0x4e6f10  Confidence: exact. Moves every reference to `from` onto `to`: the first live character that
// starts there (only the first: the loop stops at it), level script actions and location triggers.
void script_replace_navpoint_references(navpoint *from, navpoint *to)
{
    Character *c;
    int i, j;

    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (c->start_navpoint == from) {
            c->start_navpoint = to;
            break;
        }
    }
    for (i = 0; i < script_object_count; i++) {
        for (j = 0; j < script_objects[i].num_actions; j++) {
            if (script_objects[i].actions[j].nav == from)
                script_objects[i].actions[j].nav = to;
        }
    }
    for (i = 0; i < level_trigger_count; i++) {
        if (level_triggers[i].type == 1 && level_triggers[i].nav == from)
            level_triggers[i].nav = to;
    }
}

// 0x4e6fa0  Confidence: exact.
bool script_editor_save_button_cb(int event, int value)
{
    if (value == script_editor_save_button.id && event == UI_EVENT_PRESSED) {
        game_pause(0, 0);
        script_editor_show_script_dialog();
        script_editor_save_window.set_enabled(true);
        return true;
    }
    return false;
}

// 0x4e6fe0  Confidence: exact. The save window for the script being edited: the cutscene script's name
// (cutscene_script_table without "_cscript.tbl") or the level's ("<level>_script.tbl"). Game bug: a
// cutscene script table name without the suffix makes strstr return NULL, which is written to.
void script_editor_show_script_dialog()
{
    char name[0x24];

    if (script_editor_cutscene_button.is_latched()) {
        strcpy(name, cutscene_script_table());
        *strstr(name, "_cscript.tbl") = 0;
        script_editor_save_level_radio.set_checked(false);
        script_editor_save_cutscene_radio.set_checked(true);
        script_editor_save_name_inputbox.set_text(name);
        script_editor_save_filename_inputbox.set_text(cutscene_script_table());
    } else {
        str_copy_append_32(name, level_script_name ? level_script_name : level_filename, "_script.tbl");
        script_editor_save_level_radio.set_checked(true);
        script_editor_save_cutscene_radio.set_checked(false);
        script_editor_save_name_inputbox.set_text(level_script_name);
        script_editor_save_filename_inputbox.set_text(name);
    }
    script_editor_save_ok_button.set_visible(true);
    script_editor_save_ok_button.set_focus();
}

// 0x4e70e0  Confidence: exact.
bool script_editor_on_load(int event, int value)
{
    if (value == script_editor_load_button.id && event == UI_EVENT_PRESSED) {
        game_pause(0, 0);
        script_editor_list_scripts();
        script_editor_load_window.set_enabled(true);
        return true;
    }
    return false;
}

// 0x4e7120  Confidence: exact. The scripts of data\scripts that script_editor_list_scripts_filter_cb
// accepts into the load list.
void script_editor_list_scripts()
{
    file_find_node list;
    file_find_node *node;
    ui_listbox_entry *e;

    script_editor_load_name_inputbox.set_text("");
    file_find_files(&list, path_scripts, "*.*", 5, (void *)script_editor_list_scripts_filter_cb);
    script_editor_load_listbox.clear();
    for (node = list.next; node != (file_find_node *)&list; node = node->next) {
        e = script_editor_load_listbox.add_entry(node->name, -1, 0, 0, 0);
        e->set_font(font_small);
    }
    script_editor_load_ok_button.set_hidden(true);
    SE_FREE_FILE_LIST(list);
}

// 0x4e71e0  Confidence: exact (name is 0x24 bytes: the frame is 0xb4; the cutscene test as a positive if,
// D8701). The file_find_files filter: with Level scripts the level's own "<level>_script.tbl" only;
// with Cutscene scripts every "*_cscript.tbl" whose "$Level:" is this level.
bool script_editor_list_scripts_filter_cb(file_find_node *node)
{
    char name[0x24];
    char level[0x40];

    if (script_editor_load_level_radio.is_checked()) {
        str_copy_append_32(name, level_script_name ? level_script_name : level_filename, "_script.tbl");
        return _stricmp(node->name, name) == 0;
    }
    if (filename_has_cscript_suffix(node->name)) {
        TextFile tf;

        if (tf.Open(node->name, path_scripts) == 0 && tf.Accept("$Level:")) {
            tf.ParseString(level, 0x40, '"', '"');
            return _stricmp(level, level_filename) == 0;
        }
    }
    return false;
}

// 0x4e7340  Confidence: exact.
bool script_editor_on_pause(int event, int value)
{
    if (value == script_editor_pause_button.id && event == UI_EVENT_PRESSED) {
        if (game_is_paused())
            game_unpause();
        else
            game_pause(0, 0);
        return true;
    }
    return false;
}

// 0x4e7380  Confidence: exact. < Time: every inactive (dead) character comes back, the level script and
// the cutscene actors start over.
bool script_editor_on_time_back(int event, int value)
{
    Character *c, *next;

    if (value == script_editor_time_back_button.id && event == UI_EVENT_PRESSED) {
        for (c = living_list_inactive.live_next, next = c->live_next; c != &living_list_inactive;
             c = next, next = next->live_next)
            c->resurrect();
        level_script_restart();
        cutscene_script_rewind_actors();
        return true;
    }
    return false;
}

// 0x4e73d0  Confidence: exact. Esc or Enter leaves the name box; any other key applies the name at once.
bool script_editor_name_inputbox_cb(int event, int value)
{
    if (value == script_editor_name_inputbox.id && event == 0x14) {
        if (ui_key == KEY_ESC || ui_key == KEY_ENTER) {
            script_editor_name_inputbox.clear_focus();
            return true;
        }
        navpoint_set_name();
    }
    return false;
}

// 0x4e7410  Confidence: exact.
bool script_editor_on_prev_next(int event, int value)
{
    bool prev;

    if (value == script_editor_prev_button.id)
        prev = true;
    else if (value == script_editor_next_button.id)
        prev = false;
    else
        return false;
    if (event != UI_EVENT_PRESSED)
        return false;
    navpoint_set_name();
    if (prev) {
        script_editor_select_prev();
        script_editor_show_selection_panel();
        return true;
    }
    script_editor_select_next();
    script_editor_show_selection_panel();
    return true;
}

// 0x4e7460  Confidence: likely (the loop shape; jump table 0x4e75a8). The previous character in the live
// list (skipping non-living objects and serpent parts) or the previous navpoint (level table, then on to
// the end of the cutscene table and back).
void script_editor_select_prev()
{
    int i;

    if (script_editor_sel.kind == SE_PICK_NONE) {
        script_editor_select_first();
        return;
    }
    if (script_editor_sel.kind == SE_PICK_CHARACTER) {
        do {
            script_editor_sel.object = ((Character *)script_editor_sel.object)->live_prev;
            if (((Character *)script_editor_sel.object)->is_living()
                && (((Character *)script_editor_sel.object)->info->species != 0x59
                    || !character_is_serpent_part((Character *)script_editor_sel.object)))
                break;
        } while (script_editor_sel.object != ((Character *)script_editor_sel.object)->live_prev);
        script_editor_focus_selection();
        return;
    }
    if (script_editor_sel.kind == SE_PICK_NAVPOINT) {
        switch (((navpoint *)script_editor_sel.object)->type) {
        case NAVPOINT_TYPE_LEVEL:
        case NAVPOINT_TYPE_CAMERA:
        case NAVPOINT_TYPE_AMBIENT:
            i = (navpoint *)script_editor_sel.object - navpoints - 1;
            if (i < 0) {
                if (cutscene_navpoint_count > 0)
                    script_editor_sel.object = &cutscene_navpoints[cutscene_navpoint_count - 1];
                else
                    script_editor_sel.object = &navpoints[navpoint_count - 1];
            } else {
                script_editor_sel.object = &navpoints[i];
            }
            break;
        case NAVPOINT_TYPE_CUTSCENE:
            i = (navpoint *)script_editor_sel.object - cutscene_navpoints - 1;
            if (i < 0) {
                if (navpoint_count > 0)
                    script_editor_sel.object = &navpoints[navpoint_count - 1];
                else
                    script_editor_sel.object = &cutscene_navpoints[cutscene_navpoint_count - 1];
            } else {
                script_editor_sel.object = &cutscene_navpoints[i];
            }
            break;
        }
        script_editor_focus_selection();
    }
}

// 0x4e75c0  Confidence: likely (the orientation copy expands its first two rows inline and calls the copy
// for the third). The selection record from its object; New and Change unlatch.
void script_editor_focus_selection()
{
    if (script_editor_sel.kind == SE_PICK_CHARACTER) {
        Character *c = (Character *)script_editor_sel.object;
        script_editor_sel.pos = c->pos;
        script_editor_sel.orient = c->orient;
        if (((Character *)script_editor_sel.object)->type == 9) {
            script_editor_sel.size.x = script_editor_sel.size.z
                = ((Character *)script_editor_sel.object)->size_radius() * 2.0f;
            script_editor_sel.size.y = ((Character *)script_editor_sel.object)->size_height();
        } else {
            int s = ((Character *)script_editor_sel.object)->info->size;
            script_editor_sel.size.x = script_editor_char_sizes[s][0];
            script_editor_sel.size.y = script_editor_char_sizes[s][1];
            script_editor_sel.size.z = script_editor_char_sizes[s][2];
        }
    } else if (script_editor_sel.kind == SE_PICK_NAVPOINT) {
        navpoint *np = (navpoint *)script_editor_sel.object;
        script_editor_sel.size.x = 0.8f;
        script_editor_sel.size.y = 0.4f;
        script_editor_sel.size.z = 0.8f;
        script_editor_sel.pos = np->pos;
        script_editor_sel.orient.set_from_angles(0, 0, np->heading);
    }
    script_editor_new_button.set_latched(false);
    script_editor_change_button.set_latched(false);
}

// 0x4e7740  Confidence: exact (the serpent skip loop). The first navpoint of the table being edited, or the
// first live character that is not a serpent part, by the pick filter.
void script_editor_select_first()
{
    if (script_editor_pick_filter == 2) {
        if (script_editor_cutscene_button.is_latched()) {
            if (cutscene_navpoint_count != 0) {
                script_editor_sel.object = cutscene_navpoints;
                script_editor_sel.kind = SE_PICK_NAVPOINT;
                script_editor_focus_selection();
            }
        } else if (navpoint_count != 0) {
            script_editor_sel.object = navpoints;
            script_editor_sel.kind = SE_PICK_NAVPOINT;
            script_editor_focus_selection();
        }
    }
    if (script_editor_pick_filter == 1 && living_list.live_next != &living_list) {
        script_editor_sel.object = living_list.live_next;
        while (((Character *)script_editor_sel.object)->info->species == 0x59
               && character_is_serpent_part((Character *)script_editor_sel.object)
               && script_editor_sel.object != ((Character *)script_editor_sel.object)->live_next)
            script_editor_sel.object = ((Character *)script_editor_sel.object)->live_next;
        script_editor_sel.kind = SE_PICK_CHARACTER;
        script_editor_focus_selection();
    }
}

// 0x4e7800  Confidence: exact (jump table 0x4e7908). The next living object (no serpent check, unlike
// select_prev) or the next navpoint, from the end of one table to the start of the other.
void script_editor_select_next()
{
    int i;

    if (script_editor_sel.kind == SE_PICK_NONE) {
        script_editor_select_first();
        return;
    }
    if (script_editor_sel.kind == SE_PICK_CHARACTER) {
        script_editor_sel.object = ((Character *)script_editor_sel.object)->live_next;
        while (!((Character *)script_editor_sel.object)->is_living())
            script_editor_sel.object = ((Character *)script_editor_sel.object)->live_next;
        script_editor_focus_selection();
        return;
    }
    if (script_editor_sel.kind == SE_PICK_NAVPOINT) {
        switch (((navpoint *)script_editor_sel.object)->type) {
        case NAVPOINT_TYPE_LEVEL:
        case NAVPOINT_TYPE_CAMERA:
        case NAVPOINT_TYPE_AMBIENT:
            i = (navpoint *)script_editor_sel.object - navpoints + 1;
            if (i == navpoint_count) {
                if (cutscene_navpoint_count > 0)
                    script_editor_sel.object = cutscene_navpoints;
                else
                    script_editor_sel.object = navpoints;
            } else {
                script_editor_sel.object = &navpoints[i];
            }
            break;
        case NAVPOINT_TYPE_CUTSCENE:
            i = (navpoint *)script_editor_sel.object - cutscene_navpoints + 1;
            if (i == cutscene_navpoint_count) {
                if (navpoint_count > 0)
                    script_editor_sel.object = navpoints;
                else
                    script_editor_sel.object = cutscene_navpoints;
            } else {
                script_editor_sel.object = &cutscene_navpoints[i];
            }
            break;
        }
        script_editor_focus_selection();
    }
}

// 0x4e7920  Confidence: exact. A press toggles which script (level / cutscene) the panel edits; event 0xf
// unloads the cutscene script.
bool script_editor_cutscene_button_cb(int event, int value)
{
    switch (event) {
    case UI_EVENT_PRESSED:
        script_editor_cutscene_button.toggle_latched();
        if (script_editor_sel.kind == SE_PICK_CHARACTER)
            script_editor_fill_action_list();
        script_editor_show_selection_panel();
        return true;
    case 0xf:
        cutscene_script_unload();
        script_editor_sync_cutscene_button();
        script_editor_fill_action_list();
        script_editor_show_selection_panel();
        return true;
    }
    return false;
}

// 0x4e7970  Confidence: exact. Append (A) and insert (I): the action types the selected character can use
// into the action window (kind 1); the first is selected.
bool script_editor_add_action_cb(int event, int value)
{
    ui_listbox_entry *e;
    int i;

    if ((value == script_editor_append_action_button.id || value == script_editor_insert_action_button.id)
        && event == UI_EVENT_PRESSED) {
        script_editor_insert_action = value == script_editor_insert_action_button.id;
        script_editor_action_list_kind = 1;
        script_editor_action_window.set_enabled(true);
        script_editor_action_type_listbox.clear();
        for (i = 0; i < NUM_SCRIPT_ACTIONS; i++) {
            if (script_editor_action_type_allowed(i, (Character *)script_editor_sel.object)) {
                e = script_editor_action_type_listbox.add_entry(script_action_names[i], -1, 0, 0, 0);
                e->data = i;
                e->set_font(font_small);
            }
        }
        script_editor_action_type_listbox.select_entry(script_editor_action_type_listbox.entries.next, true);
        script_editor_action_type_listbox.set_focus();
        return true;
    }
    return false;
}

// 0x4e7a40  Confidence: likely (jump tables 0x4e7c2c / 0x4e7c50 over type - 6). MISNAMED in sum.symbols
// (D3505): whether action `type` is offered for c. Do state needs a locomotion anim, play action a
// category 8 move, .mvf / .vfx a file of that kind in the cutscene's folder; synch, synch teleport and
// replace weapon / armor only in cutscene scripts, guard spline only in level scripts; mount / dismount
// for species 0x57-0x5a and one hand for Joseph (species 0), both in cutscene scripts only.
bool script_editor_action_type_allowed(int type, Character *c)
{
    file_find_node list;
    bool found;
    int i;

    switch (type) {
    case SCRIPT_ACTION_DO_STATE:
        for (i = 0; i < NUM_LOCO_ANIMS; i++) {
            if (c->loco_anims[i] >= 0)
                return true;
        }
        return false;
    case SCRIPT_ACTION_PLAY_ACTION:
        for (i = 1; i <= 30; i++) {
            if (charinfo_find_move(c->info, 8, i, true) != NULL)
                return true;
        }
        return false;
    case SCRIPT_ACTION_PLAY_MVF:
        if (!script_editor_cutscene_button.is_latched())
            return false;
        file_find_files(&list, cutscene_path_id, "*.mvf", 5, 0);
        found = list.next != (file_find_node *)&list;
        SE_FREE_FILE_LIST(list);
        return found;
    case SCRIPT_ACTION_PLAY_EFFECT:
        if (!script_editor_cutscene_button.is_latched())
            return false;
        file_find_files(&list, cutscene_path_id, "*.vfx", 5, 0);
        found = list.next != (file_find_node *)&list;
        SE_FREE_FILE_LIST(list);
        return found;
    case SCRIPT_ACTION_SYNCH:
    case SCRIPT_ACTION_SYNCH_TELEPORT:
    case SCRIPT_ACTION_REPLACE_WEAPON:
    case SCRIPT_ACTION_REPLACE_ARMOR:
        return script_editor_cutscene_button.is_latched();
    case SCRIPT_ACTION_MOUNT_ANIM:
    case SCRIPT_ACTION_DISMOUNT_ANIM:
        if (!script_editor_cutscene_button.is_latched())
            return false;
        if (c->info->species >= 0x57 && c->info->species <= 0x5a)
            return true;
        return false;
    case SCRIPT_ACTION_GUARD_SPLINE:
        return !script_editor_cutscene_button.is_latched();
    case SCRIPT_ACTION_ONE_HAND:
        if (script_editor_cutscene_button.is_latched() && c->info->species == 0)
            return true;
        return false;
    }
    return true;
}

// 0x4e7c70  Confidence: exact. Del under the action list: removes the selected action
// (script_editor_delete_selected_action) and rebuilds the list at the same scroll position.
bool script_editor_delete_action_cb(int event, int value)
{
    int hscroll, vscroll;

    if (value != script_editor_delete_action_button.id || event != UI_EVENT_PRESSED)
        return false;
    hscroll = script_editor_action_listbox.hscroll;
    vscroll = script_editor_action_listbox.vscroll;
    script_editor_delete_selected_action();
    script_editor_sel_action_index = 0;
    script_editor_fill_action_list();
    script_editor_action_listbox.set_hscroll(hscroll);
    script_editor_action_listbox.set_vscroll(vscroll, false);
    return true;
}

// 0x4e7cd0  Confidence: exact. MISNAMED in sum.symbols (D3505): deletes the selected action of the current
// script; a script left without actions is removed from script_objects. Game bug: the script may be a
// cutscene_script_objects record, but the removal always works on the level table: the index computed
// from the wrong base is out of range (no move) and script_object_count still drops by one, so the last
// level script is lost (and the emptied cutscene record stays counted).
void script_editor_delete_selected_action()
{
    int i;

    if (script_editor_cur_action == script_editor_action_listbox.get_selected())
        script_editor_cur_action = NULL;
    for (i = script_editor_action_listbox.get_selected()->data + 1; i < script_editor_cur_script->num_actions; i++)
        script_editor_cur_script->actions[i - 1] = script_editor_cur_script->actions[i];
    script_editor_cur_script->num_actions--;
    if (script_editor_cur_script->num_actions == 0) {
        for (i = script_editor_cur_script - script_objects + 1; i < script_object_count; i++)
            script_objects[i - 1] = script_objects[i];
        script_object_count--;
        script_editor_cur_script = NULL;
    }
}

// 0x4e7dc0  Confidence: exact (the relinking statements in the order of the stores). Moves the selected
// action up one: swaps the two actions and the two list lines.
bool script_editor_action_up_cb(int event, int value)
{
    script_action tmp;
    ui_listbox_entry *prev;
    int index;

    if (value != script_editor_action_up_button.id || event != UI_EVENT_PRESSED)
        return false;
    index = script_editor_cur_action->data;
    if (index > 0) {
        tmp = script_editor_cur_script->actions[index];
        script_editor_cur_script->actions[index] = script_editor_cur_script->actions[index - 1];
        script_editor_cur_script->actions[index - 1] = tmp;
        prev = script_editor_cur_action->prev;
        list_remove(script_editor_cur_action);
        prev->prev->next = script_editor_cur_action;
        script_editor_cur_action->prev = prev->prev;
        prev->prev = script_editor_cur_action;
        script_editor_cur_action->next = prev;
        script_editor_cur_action->data--;
        prev->data++;
        script_editor_action_listbox.recalc_content_size();
        script_editor_action_listbox.select_entry(script_editor_cur_action, true);
    }
    return true;
}

// 0x4e7ee0  Confidence: exact (VC6 byte match) (as script_editor_action_up_cb). Moves the selected action down one.
bool script_editor_action_down_cb(int event, int value)
{
    script_action tmp;
    ui_listbox_entry *next;
    int index;

    if (value != script_editor_action_down_button.id || event != UI_EVENT_PRESSED)
        return false;
    index = script_editor_cur_action->data;
    if (index < script_editor_cur_script->num_actions - 1) {
        tmp = script_editor_cur_script->actions[index];
        script_editor_cur_script->actions[index] = script_editor_cur_script->actions[index + 1];
        script_editor_cur_script->actions[index + 1] = tmp;
        next = script_editor_cur_action->next;
        list_remove(script_editor_cur_action);
        next->next->prev = script_editor_cur_action;
        script_editor_cur_action->next = next->next;
        script_editor_cur_action->prev = next;
        next->next = script_editor_cur_action;
        script_editor_cur_action->data++;
        next->data--;
        script_editor_action_listbox.recalc_content_size();
        script_editor_action_listbox.select_entry(script_editor_cur_action, true);
    }
    return true;
}

// 0x4e8000  Confidence: exact.
bool script_editor_change_cb(int event, int value)
{
    if (value == script_editor_change_button.id && event == UI_EVENT_PRESSED) {
        script_editor_fill_action_lists();
        return true;
    }
    return false;
}

// One label / input box pair of the parameter window, the box holding an int.
#define SE_PARAM_INT(i, label, value)                                                   \
    do {                                                                                \
        script_editor_param_labels[i].set_text(label);                                  \
        script_editor_param_inputboxes[i].set_text(int_to_string(buf, 0x20, (value)));  \
    } while (0)

// 0x4e8020  Confidence: likely (jump tables 0x4e8334 / 0x4e8378 over type - 3; the shared tails). Edit
// (Ctrl+E): the parameter window for the selected action (only while it is closed). Up to five fields;
// the unused boxes are hidden. Types without numbers open nothing.
bool script_editor_edit_button_cb(int event, int value)
{
    char buf[0x20];
    script_action *a;
    int i, n;

    if (event != UI_EVENT_PRESSED || script_editor_params_window.is_disabled() != true)
        return false;
    for (i = 0; i < 5; i++) {
        script_editor_param_labels[i].set_text("");
        script_editor_param_inputboxes[i].set_text("");
    }
    if (script_editor_cur_script == NULL || script_editor_cur_action == NULL)
        return true;
    a = &script_editor_cur_script->actions[script_editor_cur_action->data];
    switch (a->type) {
    case SCRIPT_ACTION_WAIT_AT:
        SE_PARAM_INT(0, "Time:", a->arg0c);
        n = 1;
        break;
    case SCRIPT_ACTION_WANDER:
    case SCRIPT_ACTION_WANDER_FOR:
        SE_PARAM_INT(0, "dist from nav:", a->arg0c);
        n = 1;
        if (a->type == SCRIPT_ACTION_WANDER_FOR) {
            SE_PARAM_INT(1, "time:", a->arg10);
            n = 2;
        }
        SE_PARAM_INT(n, "Walk?", a->flag14);
        n++;
        if (n <= 0)
            return true;
        break;
    case SCRIPT_ACTION_TURN_HOSTILE:
        SE_PARAM_INT(0, "npx(0), monster(1)", a->arg0c);
        n = 1;
        break;
    case SCRIPT_ACTION_SAY:
        SE_PARAM_INT(0, "display time", a->arg0c);
        n = 1;
        break;
    case SCRIPT_ACTION_LABEL:
        SE_PARAM_INT(0, "Label id", a->arg0c);
        n = 1;
        break;
    case SCRIPT_ACTION_GOTO:
        SE_PARAM_INT(0, "Goto id", a->arg0c);
        n = 1;
        break;
    case SCRIPT_ACTION_NO_BLOCK:
        SE_PARAM_INT(0, "block(1), noblock(0)", a->arg0c);
        n = 1;
        break;
    case SCRIPT_ACTION_TRIGGER:
        SE_PARAM_INT(0, "dist", a->arg0c);
        n = 1;
        break;
    case SCRIPT_ACTION_TRIGGER_NAV:
        SE_PARAM_INT(0, "dist", a->arg0c);
        n = 1;
        break;
    case SCRIPT_ACTION_TRIGGER_VISIT:
        SE_PARAM_INT(0, "visits", a->arg0c);
        n = 1;
        break;
    case SCRIPT_ACTION_PASS_GO:
        script_editor_param_labels[0].set_text("NPC to go");
        script_editor_param_inputboxes[0].set_text(a->text);
        n = 1;
        break;
    case SCRIPT_ACTION_SHOW_HIDE:
        SE_PARAM_INT(0, "show(1) hide(0)", a->arg0c);
        n = 1;
        break;
    case SCRIPT_ACTION_SYNCH:
    case SCRIPT_ACTION_SYNCH_TELEPORT:
        SE_PARAM_INT(0, "synch_point", a->arg0c);
        n = 1;
        break;
    case SCRIPT_ACTION_FOLLOW:
        script_editor_param_labels[0].set_text("NPC to follow");
        script_editor_param_inputboxes[0].set_text(a->text);
        SE_PARAM_INT(1, "time (-1=inf)", a->arg0c);
        n = 2;
        break;
    case SCRIPT_ACTION_COMBAT_MODE:
        SE_PARAM_INT(0, "norm(0), com(1)", a->arg0c);
        n = 1;
        break;
    case SCRIPT_ACTION_GUARD_SPLINE:
    case SCRIPT_ACTION_ONE_HAND:
        SE_PARAM_INT(0, "off(0), on(1)", a->arg0c);
        n = 1;
        break;
    default:
        return true;
    }
    for (i = n; i < 5; i++)
        script_editor_param_inputboxes[i].set_hidden(true);
    for (i = 0; i < n; i++)
        script_editor_param_inputboxes[i].set_visible(true);
    script_editor_param_field_count = n;
    script_editor_params_window.set_enabled(true);
    script_editor_param_inputboxes[0].set_focus();
    return true;
}

// 0x4e83a0  Confidence: exact.
bool script_editor_new_ok_cb(int event, int value)
{
    if (event == UI_EVENT_PRESSED) {
        script_editor_new_selection = script_editor_new_listbox.get_selected()->data;
        script_editor_new_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4e83d0  Confidence: exact.
bool script_editor_new_cancel_cb(int event, int value)
{
    if (event == UI_EVENT_PRESSED) {
        script_editor_new_button.set_latched(false);
        script_editor_new_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4e8400  Confidence: exact. A double click picks the character.
bool script_editor_new_listbox_cb(int event, int value)
{
    if (script_editor_new_listbox.id == value && event == UI_EVENT_LIST_DBLCLICK) {
        script_editor_new_selection = script_editor_new_listbox.get_selected()->data;
        script_editor_new_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4e8440  Confidence: exact. The action window's handler: a double click or Ok applies the list choice,
// event 10 on the list keeps the focus there, Cancel closes.
bool script_editor_action_window_cb(int event, int value)
{
    if (script_editor_action_type_listbox.id == value) {
        if (event == UI_EVENT_LIST_DBLCLICK) {
            script_editor_apply_action_params();
            return true;
        }
        if (event == 0xa) {
            script_editor_action_type_listbox.set_focus();
            return true;
        }
        return false;
    }
    if (script_editor_action_ok_button.id == value && event == UI_EVENT_PRESSED) {
        script_editor_apply_action_params();
        return true;
    }
    if (script_editor_action_cancel_button.id == value && event == UI_EVENT_PRESSED) {
        script_editor_action_window.set_disabled(true);
        return true;
    }
    return false;
}

// Re-formats the selected action's list line (written out after each case in the binary).
#define SE_UPDATE_ACTION_LINE(a)                                                           \
    do {                                                                                   \
        script_action_format(name, args, (a), (Character *)script_editor_sel.object);      \
        sprintf(string_buffer, "%s %s", name, args);                                       \
        script_editor_cur_action->set_text(string_buffer);                                 \
    } while (0)

// 0x4e84b0  Confidence: likely (jump table 0x4e886c over the list kind; kinds 3 and 9 share the move
// lookup tail, kinds 2 / 5 / 9 / 10 the line update). The action window's choice by list kind: 1 a new
// action (script_create), 2 a locomotion anim, 3 a move's anim, 4 a .mvf (its anim loaded for the
// character), 5 a .vfx, 6 / 7 select a character / a level navpoint, 8 equips the weapon at once, 9 the
// death move, 10 an item. Kinds 6-8 are set nowhere in this unit. Game bug: kind 9 ignores the choice and
// takes the char_info's move 6 / 0x22, which is NULL when the list showed the category 8 moves instead
// (the window offers those when there is no death move): a NULL dereference.
void script_editor_apply_action_params()
{
    char args[0x104], name[0x104];
    script_action *a;
    move_info *m;
    item_info *old1, *old2;
    int index;

    script_editor_action_window.set_disabled(true);
    script_editor_action_type_listbox.set_sorted(false);
    switch (script_editor_action_list_kind) {
    case 1:
        script_create();
        return;
    case 2:
        index = script_editor_action_listbox.get_selected()->data;
        script_editor_cur_script->actions[index].arg0c = script_editor_action_type_listbox.get_selected()->data;
        SE_UPDATE_ACTION_LINE(&script_editor_cur_script->actions[index]);
        return;
    case 3:
        index = script_editor_action_listbox.get_selected()->data;
        m = charinfo_find_move(((Character *)script_editor_sel.object)->info, 8,
                               script_editor_action_type_listbox.get_selected()->data, true);
        script_editor_cur_script->actions[index].arg0c = m->anim;
        SE_UPDATE_ACTION_LINE(&script_editor_cur_script->actions[index]);
        return;
    case 4:
        index = script_editor_action_listbox.get_selected()->data;
        a = &script_editor_cur_script->actions[index];
        script_editor_store_string(a, script_editor_action_type_listbox.get_selected()->text);
        a->arg0c = charinfo_load_script_anim(((Character *)script_editor_sel.object)->info, a->text, cutscene_path_id);
        SE_UPDATE_ACTION_LINE(&script_editor_cur_script->actions[index]);
        return;
    case 5:
        index = script_editor_action_listbox.get_selected()->data;
        script_editor_store_string(&script_editor_cur_script->actions[index],
                                   script_editor_action_type_listbox.get_selected()->text);
        SE_UPDATE_ACTION_LINE(&script_editor_cur_script->actions[index]);
        return;
    case 6:
        script_editor_action_type_listbox.get_selected();
        script_editor_sel.kind = SE_PICK_CHARACTER;
        script_editor_sel.object = (void *)script_editor_action_type_listbox.get_selected()->data;
        script_editor_focus_selection();
        return;
    case 7: {
        navpoint *np;
        script_editor_action_type_listbox.get_selected();
        np = &navpoints[script_editor_action_type_listbox.get_selected()->data];
        navpoint_set_name();
        script_editor_sel.object = np;
        script_editor_sel.kind = SE_PICK_NAVPOINT;
        script_editor_focus_selection();
        script_editor_show_selection_panel();
        return;
    }
    case 8:
        character_equip_item(&old1, &old2, (Character *)script_editor_sel.object, 5,
                             item_create_weapon(script_editor_action_type_listbox.get_selected()->data));
        if (old1 != NULL)
            item_free(old1);
        if (old2 != NULL)
            item_free(old2);
        return;
    case 9:
        index = script_editor_action_listbox.get_selected()->data;
        m = charinfo_find_move(((Character *)script_editor_sel.object)->info, 6, 0x22, true);
        script_editor_cur_script->actions[index].arg0c = m->anim;
        SE_UPDATE_ACTION_LINE(&script_editor_cur_script->actions[index]);
        return;
    case 10:
        a = &script_editor_cur_script->actions[script_editor_action_listbox.get_selected()->data];
        a->arg0c = script_editor_action_type_listbox.get_selected()->data;
        SE_UPDATE_ACTION_LINE(a);
        return;
    }
}

// 0x4e88a0  Confidence: likely (jump tables 0x4e8cb0 / 0x4e8cd0 over type - 7). Adds an action of the type
// picked in the action window to the selected character's script, creating the script first (32 cutscene
// / 200 level scripts, 54 actions each). The defaults come from script_action_default_table; an action
// that takes a navpoint gets the last one (script_editor_default_action_navpoint), some get a first argument (the first
// move, the first .vfx, "New Dialogue Text", the character's own name). With I (insert) the action moves
// up to the selected line.
void script_create()
{
    bool cutscene;
    int type, n, i;
    script_action *a;
    script_action tmp;
    move_info *m;
    file_find_node list;

    cutscene = script_editor_cutscene_button.is_latched();
    type = script_editor_action_type_listbox.get_selected()->data;
    if (script_editor_cur_script == NULL) {
        if (cutscene) {
            if (cutscene_script_object_count == MAX_CUTSCENE_SCRIPT_OBJECTS) {
                script_editor_set_status("Unable to create cutscene script -- maximum cutscene scripts reached.");
                return;
            }
            i = cutscene_script_object_count++;
            script_editor_cur_script = &cutscene_script_objects[i];
            ((Character *)script_editor_sel.object)->cutscene_object = i;
        } else {
            if (script_object_count == MAX_SCRIPT_OBJECTS) {
                script_editor_set_status("Unable to create level script -- maximum level scripts reached.");
                return;
            }
            i = script_object_count++;
            script_editor_cur_script = &script_objects[i];
            ((Character *)script_editor_sel.object)->script_object = i;
        }
        script_editor_cur_script->cur_action = -1;
        script_editor_cur_script->num_actions = 0;
        script_editor_cur_script->trigger_level = -1;
        script_editor_cur_script->handle = ((Character *)script_editor_sel.object)->handle;
        script_editor_cur_script->end_mode = cutscene ? 2 : 0;
    }
    n = script_editor_cur_script->num_actions;
    if (n == 54) {
        script_editor_set_status("Unable to add script entry -- maximum script entries reached for this script.");
        return;
    }
    script_editor_cur_script->num_actions = n + 1;
    a = &script_editor_cur_script->actions[n];
    a->type = type;
    a->arg0c = script_action_default_table[type].arg0c;
    a->arg10 = script_action_default_table[type].arg10;
    a->flag14 = script_action_default_table[type].flag14;
    if (script_action_default_table[type].needs_nav) {
        a->nav = script_editor_default_action_navpoint();
        script_editor_lists_dirty = true;
    } else {
        a->nav = NULL;
    }
    a->text = NULL;
    if (script_action_default_table[type].has_params) {
        switch (type) {
        case SCRIPT_ACTION_PLAY_ACTION:
            for (i = 1; i <= 30; i++) {
                m = charinfo_find_move(((Character *)script_editor_sel.object)->info, 8, i, true);
                if (m != NULL)
                    break;
            }
            if (m != NULL)
                a->arg0c = m->anim;
            else
                a->arg0c = -1;
            script_editor_lists_dirty = true;
            break;
        case SCRIPT_ACTION_PLAY_MVF:
        case SCRIPT_ACTION_REPLACE_WEAPON:
        case SCRIPT_ACTION_REPLACE_ARMOR:
            script_editor_lists_dirty = true;
            break;
        case SCRIPT_ACTION_PLAY_EFFECT:
            file_find_files(&list, cutscene_path_id, "*.vfx", 5, 0);
            script_editor_store_string(a, list.next->name);
            SE_FREE_FILE_LIST(list);
            break;
        case SCRIPT_ACTION_SAY:
            script_editor_store_string(a, "New Dialogue Text");
            break;
        case SCRIPT_ACTION_DIE:
            m = charinfo_find_move(((Character *)script_editor_sel.object)->info, 6, 0x22, true);
            if (m == NULL) {
                for (i = 1; i <= 30; i++) {
                    m = charinfo_find_move(((Character *)script_editor_sel.object)->info, 8, i, true);
                    if (m != NULL)
                        break;
                }
                if (m == NULL) {
                    a->arg0c = -1;
                    script_editor_lists_dirty = true;
                    break;
                }
            }
            a->arg0c = m->anim;
            script_editor_lists_dirty = true;
            break;
        case SCRIPT_ACTION_PASS_GO:
            script_editor_store_string(a, ((Character *)script_editor_sel.object)->get_name());
            break;
        case SCRIPT_ACTION_FOLLOW:
            script_editor_store_string(a, ((Character *)script_editor_sel.object)->get_name());
            break;
        }
    }
    if (script_editor_insert_action && script_editor_cur_action != NULL && script_editor_cur_action->data >= 0) {
        for (i = n; i > script_editor_cur_action->data; i--) {
            tmp = script_editor_cur_script->actions[i];
            script_editor_cur_script->actions[i] = script_editor_cur_script->actions[i - 1];
            script_editor_cur_script->actions[i - 1] = tmp;
        }
        script_editor_sel_action_index = i;
        script_editor_fill_action_list();
        return;
    }
    script_editor_sel_action_index = script_editor_cur_script->num_actions - 1;
    script_editor_fill_action_list();
}

// 0x4e8cf0  Confidence: likely (the cutscene search loop). MISNAMED in sum.symbols (D3505): the navpoint a new
// action starts with: the last level navpoint, or in a cutscene script the last cutscene navpoint. Game
// bugs: without cutscene navpoints the search over the level table tests the type through a pointer that
// is still NULL (the binary reads address 0x28: an access violation whenever there is a level navpoint),
// and with no level navpoint at all the level case returns &navpoints[-1]. LOGIC FIX (D8702): when the
// search loop runs to its end the binary returns the last level navpoint too (0x4e8d41 returns EAX), not
// NULL; only an empty table gives NULL (the binary keeps that NULL in ESI, which the source here does not
// reproduce).
navpoint *script_editor_default_action_navpoint()
{
    navpoint *np = NULL;
    int i;

    if (script_editor_cutscene_button.is_latched()) {
        if (cutscene_navpoint_count > 0)
            return &cutscene_navpoints[cutscene_navpoint_count - 1];
        if (navpoint_count > 0) {
            for (i = navpoint_count - 1; i >= 0; i--) {
                if (np->type == NAVPOINT_TYPE_LEVEL)
                    return &navpoints[navpoint_count - 1];
            }
            return &navpoints[navpoint_count - 1];
        }
        return np;
    }
    return &navpoints[navpoint_count - 1];
}

// 0x4e8d60  Confidence: exact. The save window's name box: Enter saves (as Save does); every edit shows the
// file name the script will get ("*** INVALID FILENAME ***" and no Save button while empty).
bool script_editor_save_name_cb(int event, int value)
{
    char name[0x24];
    char filename[0x24];
    int result;

    strcpy(name, script_editor_save_name_inputbox.get_text());
    switch (event) {
    case 0x14:
        if (ui_key == KEY_ENTER && strlen(name) != 0) {
            result = script_save_files();
            script_save(result);
            if (result == 0)
                script_editor_save_window.set_disabled(true);
        }
        return true;
    case 0x15:
        if (strlen(name) != 0) {
            if (script_editor_save_level_radio.is_checked())
                str_copy_append_32(filename, name, "_script.tbl");
            else
                str_copy_append_32(filename, name, "_cscript.tbl");
            script_editor_save_filename_inputbox.set_text(filename);
            script_editor_save_ok_button.set_visible(true);
            return true;
        }
        script_editor_save_filename_inputbox.set_text("*** INVALID FILENAME ***");
        script_editor_save_ok_button.set_hidden(true);
        return true;
    }
    return false;
}

// 0x4e8e80  Confidence: exact. Save: the script file (data\scripts), and for a level script the level
// table (data\tables\levels, when an ambient changed) and level_items.tbl (data\tables, when a level item
// changed). Read-only files stop it first (1 script, 2 table, 3 level items); a failed writer returns
// 4 / 5 / 6. The tables are written to a free "<n>.int" in their folder, which then replaces the table.
int script_save_files()
{
    File script_file, table_file, items_file;
    char table_name[0x40];
    char filename[0x40];
    char temp_name[0x40];
    bool level;
    int i;

    level = script_editor_save_level_radio.is_checked();
    strcpy(filename, script_editor_save_filename_inputbox.get_text());
    if (level)
        sprintf(table_name, "%s.tbl", level_filename);
    if (script_file.find(filename, path_scripts) && (script_file.get_attributes(NULL, PATH_ANY) & 2))
        return 1;
    if (level) {
        if (script_ambients_dirty && table_file.find(table_name, path_tables_levels)
            && (table_file.get_attributes(NULL, PATH_ANY) & 2))
            return 2;
        if (script_level_items_dirty && items_file.find("level_items.tbl", path_tables)
            && (items_file.get_attributes(NULL, PATH_ANY) & 2))
            return 3;
    }
    if (script_write_level_script(filename, level) != 0)
        return 4;
    if (level) {
        File temp;
        if (script_ambients_dirty) {
            i = 0;
            do {
                sprintf(temp_name, "%d.int", i++);
            } while (temp.find(temp_name, path_tables_levels));
            if (script_write_level_table(table_name, temp_name) != 0)
                return 5;
            script_ambients_dirty = false;
        }
        if (script_level_items_dirty) {
            i = 0;
            do {
                sprintf(temp_name, "%d.int", i++);
            } while (temp.find(temp_name, path_tables));
            if (level_items_write_table(temp_name) != 0)
                return 6;
            script_level_items_dirty = false;
        }
    }
    return 0;
}

// 0x4e9310  Confidence: likely (the order of the backup's statements). The script file: the old one is
// renamed to "<file>.bak" first (an older .bak deleted). A cutscene script gets "$Level:", a level script
// "#Town" for a town; #Navpoints is the level or the cutscene table; #Objects one record per character
// (script_write_objects); a level script adds #Doors, #Clicks and a comment listing the creatures that
// start at a level navpoint, sorted, with "*** DUPLICATE NAME ***" marks (names from the 0x800-byte alloc
// pool: 512 names at most, else no list). Returns File::open's or File::close's result.
// Game bug: the inactive characters are counted without the serpent-part test that the second pass
// applies, so an inactive serpent part leaves the last name slot unset (garbage pointer for qsort).
int script_write_level_script(const char *filename, bool level)
{
    File f;
    char buf[0x100];
    const char **names;
    Character *c;
    door_info *d;
    lobj *l;
    int i, count, n, dups, err;

    if (f.find(filename, path_scripts)) {
        strcpy(buf, filename);
        strcat(buf, ".bak");
        File bak;
        if (bak.find(buf, path_scripts))
            bak.Delete(NULL, PATH_ANY);
        f.make_backup(buf, NULL, PATH_ANY);
    }
    err = f.open(filename, 0x80000002, path_scripts);
    if (err != 0)
        return err;
    if (!level) {
        sprintf(buf, "$Level: \"%s\"", level_filename);
        f.write_line(buf, -1);
        f.write_line(NULL, -1);
    } else if (level_is_town) {
        f.write_line("#Town", -1);
    }
    f.write_line("#Navpoints", -1);
    if (level) {
        for (i = 0; i < navpoint_count; i++) {
            f.write_line(NULL, -1);
            script_write_navpoint(&f, &navpoints[i]);
        }
    } else {
        for (i = 0; i < cutscene_navpoint_count; i++) {
            f.write_line(NULL, -1);
            script_write_navpoint(&f, &cutscene_navpoints[i]);
        }
    }
    f.write_line(NULL, -1);
    f.write_line("#Objects", -1);
    f.write_line(NULL, -1);
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        if (script_write_objects(&f, c, level))
            f.write_line(NULL, -1);
    }
    for (c = living_list_inactive.live_next; c != &living_list_inactive; c = c->live_next) {
        if (script_write_objects(&f, c, level))
            f.write_line(NULL, -1);
    }
    if (level) {
        f.write_line("#Doors", -1);
        f.write_line(NULL, -1);
        for (i = 0; i < num_lobjs; i++) {
            l = &lobjs[i];
            if (!(l->flags & 0x100))
                continue;
            d = door_from_lobj(l);
            sprintf(buf, "$Door: \"%s\"", l->name);
            f.write_line(buf, -1);
            if (d->open_sound.name != NULL) {
                sprintf(buf, "+Open sound: \"%s\" %.1f %.1f %.1f", d->open_sound.name, (float)d->open_sound.params[1],
                        d->open_sound.params[0] * (1.0f / 255.0f), (float)d->open_sound.params[2]);
                f.write_line(buf, -1);
            }
            if (d->close_sound.name != NULL) {
                sprintf(buf, "+Close sound: \"%s\" %.1f %.1f %.1f", d->close_sound.name, (float)d->close_sound.params[1],
                        d->close_sound.params[0] * (1.0f / 255.0f), (float)d->close_sound.params[2]);
                f.write_line(buf, -1);
            }
            if (d->state & 8) {
                sprintf(buf, "+Locked: %.2f", d->lock);
                f.write_line(buf, -1);
                if (d->key != NULL) {
                    sprintf(buf, "+Key: \"%s\"", d->key->name);
                    f.write_line(buf, -1);
                }
            }
            f.write_line(NULL, -1);
        }
        f.write_line("#Clicks", -1);
        for (i = 0; i < lc_entry_count; i++) {
            if (lc_entries[i].object == NULL)
                continue;
            sprintf(buf, "$Name: \"%s\"", lc_entries[i].object->name);
            f.write_line(buf, -1);
            if (lc_entries[i].gold > 0) {
                sprintf(buf, "+Give: %d", lc_entries[i].gold);
                f.write_line(buf, -1);
            }
            if (strlen(lc_entries[i].name) != 0) {
                sprintf(buf, "+Messagebox: \"%s\"", lc_entries[i].name);
                f.write_line(buf, -1);
            }
            f.write_line(NULL, -1);
        }

        count = 0;
        for (c = living_list.live_next; c != &living_list; c = c->live_next) {
            if (c->info->species == 0x59 && character_is_serpent_part(c))
                continue;
            if (c->start_navpoint != NULL && !(c->flags & 0x100) && c->type != 7
                && ((navpoint *)c->start_navpoint)->type == NAVPOINT_TYPE_LEVEL)
                count++;
        }
        for (c = living_list_inactive.live_next; c != &living_list_inactive; c = c->live_next) {
            if (c->start_navpoint != NULL && !(c->flags & 0x100) && c->type != 7
                && ((navpoint *)c->start_navpoint)->type == NAVPOINT_TYPE_LEVEL)
                count++;
        }
        names = (const char **)script_editor_alloc_pool.Alloc(count * 4);
        if (names != NULL) {
            dups = 0;
            n = 0;
            for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                if (c->info->species == 0x59 && character_is_serpent_part(c))
                    continue;
                if (c->start_navpoint != NULL && !(c->flags & 0x100) && c->type != 7
                    && ((navpoint *)c->start_navpoint)->type == NAVPOINT_TYPE_LEVEL)
                    names[n++] = c->get_name();
            }
            for (c = living_list_inactive.live_next; c != &living_list_inactive; c = c->live_next) {
                if (c->info->species == 0x59 && character_is_serpent_part(c))
                    continue;
                if (c->start_navpoint != NULL && !(c->flags & 0x100) && c->type != 7
                    && ((navpoint *)c->start_navpoint)->type == NAVPOINT_TYPE_LEVEL)
                    names[n++] = c->get_name();
            }
            qsort(names, count, 4, qsort_cmp_strptr);
            f.write_line("/* Level script creatures:", -1);
            for (i = 0; i < count; i++) {
                if (i > 0 && strcmp(names[i], names[i - 1]) == 0) {
                    dups++;
                    sprintf(string_buffer, "%s\t*** DUPLICATE NAME ***", names[i]);
                    f.write_line(string_buffer, -1);
                } else {
                    f.write_line(names[i], -1);
                }
            }
            f.write_line(NULL, -1);
            sprintf(string_buffer, "Total duplicate names found: %d", dups);
            f.write_line(string_buffer, -1);
            f.write_line("*/", -1);
            f.write_line(NULL, -1);
        }
        script_editor_alloc_pool.Init(script_editor_alloc_pool_buffer, 0x800);
    }
    f.write_line("#End", -1);
    return f.close();
}

// 0x4e9ba0  Confidence: exact. One #Navpoints record; a navpoint whose name was emptied (remove_nav) is left
// out. The orientation is rebuilt from the heading (precision 6), the position written with precision 2.
void script_write_navpoint(File *f, navpoint *np)
{
    char buf[0x100];
    matrix orient;

    if (strlen(np->name) == 0)
        return;
    sprintf(string_buffer, "$Name:\t\"%s\"", np->name);
    f->write_line(string_buffer, -1);
    sprintf(string_buffer, "$Type:\t\"%s\"", script_navpoint_type_names[np->type]);
    f->write_line(string_buffer, -1);
    if (np->layer >= 0) {
        sprintf(string_buffer, "+Plane:\t%d", np->layer);
        f->write_line(string_buffer, -1);
    }
    vector3_to_cstr(buf, 0x100, &np->pos, 2);
    sprintf(string_buffer, "$Position:\t%s", buf);
    f->write_line(string_buffer, -1);
    orient.set_from_angles(0, 0, np->heading);
    matrix3_to_cstr(buf, 0x100, &orient, 6);
    sprintf(string_buffer, "$Orientation:\t%s", buf);
    f->write_line(string_buffer, -1);
}

// 0x4e9ce0  Confidence: likely (jump table 0x4ea1c8 over type - 5; the move-through check's flag). One
// #Objects record: the header for a character that starts at a navpoint of the script's own kind (level
// script: type 0; cutscene script: type 1, and a level navpoint gives a "+No create" record when the
// character has a cutscene script), then its actions. A dialog warns about "move through" actions that
// are not followed by a walk / run to. Returns whether anything was written (the caller adds a blank
// line). Game bugs (.tbl corruption): a character with a script whose start navpoint is of another type
// (a camera / ambient navpoint, or a cutscene navpoint in a level script) gets its "+Action:" lines
// written with no "$Name:" header, so the parser adds them to the previous record (or fails on them at
// the top of #Objects); and "+Trigger:" ends with its own "\n" before write_line's.
bool script_write_objects(File *f, Character *c, bool level)
{
    char args[0x104], name[0x104];
    script_object *so = NULL;
    navpoint *np;
    bool pending;
    int i;

    if (c->flags & 0x100)
        return false;
    if (level && c->type == 7)
        return false;
    if (c->info->species == 0x59 && character_is_serpent_part(c))
        return false;
    if (level) {
        for (i = 0; i < script_object_count; i++) {
            if (script_objects[i].handle == c->handle) {
                so = &script_objects[i];
                break;
            }
        }
    } else {
        for (i = 0; i < cutscene_script_object_count; i++) {
            if (cutscene_script_objects[i].handle == c->handle) {
                so = &cutscene_script_objects[i];
                break;
            }
        }
    }
    np = (navpoint *)c->start_navpoint;
    if (np == NULL)
        return false;
    if (level == true) {
        if (np->type != NAVPOINT_TYPE_LEVEL)
            goto actions;
    } else if (np->type != NAVPOINT_TYPE_CUTSCENE) {
        if (np->type != NAVPOINT_TYPE_LEVEL)
            goto actions;
        if (so == NULL)
            return false;
        sprintf(string_buffer, "$Name:\t\t\t\t\"%s\"", c->get_name());
        f->write_line(string_buffer, -1);
        f->write_line("+No create", -1);
        sprintf(string_buffer, "$Start position:\t\"%s\"", ((navpoint *)c->start_navpoint)->name);
        f->write_line(string_buffer, -1);
        goto actions;
    }
    sprintf(string_buffer, "$Name:\t\t\t\t\"%s\"", c->get_name());
    f->write_line(string_buffer, -1);
    sprintf(string_buffer, "$Character:\t\t\t\"%s\"", c->info->name);
    f->write_line(string_buffer, -1);
    switch (c->type) {
    case 5:
    case 6:
        f->write_line("+Monster", -1);
        break;
    case 9:
    case 10:
        f->write_line("+Boss", -1);
        break;
    }
    if (c->flags & 0x40)
        f->write_line("+Miniboss", -1);
    if (so != NULL) {
        if (so->hidden) {
            sprintf(string_buffer, "+Hidden");
            f->write_line(string_buffer, -1);
        }
        if (so->trigger_level != -1) {
            sprintf(string_buffer, "+Trigger: \"%s\" \"%s\"\n", level_script_table[so->trigger_level].name, so->trigger);
            f->write_line(string_buffer, -1);
        }
    }
    if (c->get_team() != c->info->team) {
        sprintf(string_buffer, "+Team:\t\"%s\"", team_names[c->get_team()]);
        f->write_line(string_buffer, -1);
    }
    if (c->type != 7 && (c->flags & 0x400000)) {
        sprintf(string_buffer, "+Level:\t%d", c->level);
        f->write_line(string_buffer, -1);
    }
    sprintf(string_buffer, "$Start position:\t\"%s\"", ((navpoint *)c->start_navpoint)->name);
    f->write_line(string_buffer, -1);
actions:
    if (so == NULL)
        return true;
    pending = false;
    for (i = 0; i < so->num_actions; i++) {
        int type = so->actions[i].type;
        if (type == SCRIPT_ACTION_MOVE_THROUGH) {
            pending = true;
        } else if (type == SCRIPT_ACTION_WALK_TO || type == SCRIPT_ACTION_RUN_TO) {
            if (pending == true)
                pending = false;
        } else if (pending) {
            sprintf(name, "Script for %s contains walk/run through commands that don't terminate with a walk/run to command.",
                    c->get_name());
            editor_dialog_show(400, "Warning!", name, EDITOR_DIALOG_OK, NULL, NULL, NULL, 0xb);
            break;
        }
    }
    for (i = 0; i < so->num_actions; i++) {
        script_action_format(name, args, &so->actions[i], c);
        sprintf(string_buffer, "+Action:\t\t\t\"%s\"", name);
        if (strlen(args) != 0) {
            strcat(string_buffer, "\t");
            strcat(string_buffer, args);
        }
        f->write_line(string_buffer, -1);
    }
    return true;
}

// 0x4ea1e0  Confidence: likely (the copy loops). <level>.tbl with the editor's ambients: copied through the
// "#Ambient Sounds" line into the temp file, then one "$Ambient:" per ambient navpoint with an emitter
// at its position, then everything from "#Triggers" on; the old table is deleted and the temp file renamed
// to it. Game bugs (.tbl): the copy loop runs until a line equals "#Ambient Sounds" exactly, so a table
// without that line (or with trailing blanks on it) never ends the loop (the editor hangs at end of file);
// ambients the table placed on level-file markers (level_parse_ambient also accepts level_marker_find
// names) are not in the navpoint table and are dropped from the file.
int script_write_level_table(const char *table, const char *temp)
{
    TextFile tf;
    File old, out;
    char line[0x200];
    navpoint *np;
    snd_sound *s;
    int i, j, err;

    tf.Open(table, path_tables_levels);
    out.open(temp, 0x80000002, path_tables_levels);
    do {
        tf.ReadLine(line, 0x200);
        out.write_line(line, -1);
    } while (strcmp(line, "#Ambient Sounds") != 0);
    out.write_line("//-------------", -1);
    out.write_line(NULL, -1);
    for (i = 0, np = navpoints; i < navpoint_count; i++, np++) {
        if (np->type != NAVPOINT_TYPE_AMBIENT)
            continue;
        for (j = 0; j < level_ambient_count; j++) {
            if (level_ambients[j].pos.x == np->pos.x && level_ambients[j].pos.y == np->pos.y
                && level_ambients[j].pos.z == np->pos.z)
                break;
        }
        if (j == level_ambient_count)
            continue;
        if (level_ambients[j].emitter < 0)
            continue;
        s = &sounds[sound_emitter_get_sound(level_ambients[j].emitter)];
        sprintf(line, "$Ambient: \"%s\"\t\"%s\"\t%0.1f\t%0.1f\t%0.1f\t%0.1f", np->name, s->name, s->min_range,
                s->volume, s->rolloff, sound_emitter_get_repeat_delay(level_ambients[j].emitter) * 0.001f);
        out.write_line(line, -1);
    }
    out.write_line(NULL, -1);
    out.write_line("//-------------", -1);
    tf.SkipTo("#Triggers", NULL);
    while (tf.ReadLine(line, 0x200))
        out.write_line(line, -1);
    tf.Close();
    err = old.Delete(table, path_tables_levels);
    if (err == 0) {
        err = out.close();
        if (err == 0)
            err = out.make_backup(table, temp, path_tables_levels);
    }
    return err;
}

// 0x4ea500  Confidence: likely (the line loops). level_items.tbl: the file is copied through this level's
// "#Level:" line (a new section is opened before "#End" when there is none); in the section the records of
// the level's other scripts are kept, this script's are replaced by the level_items records that have a
// navpoint; the rest of the file follows. Temp file and rename as script_write_level_table. Game bugs
// (.tbl): level_items_parse_table sets the navpoint only for this script's records, so a record of another
// script keeps whatever navpoint pointer its slot held before (a previous level's), and when that is not
// NULL the record is written a second time with that navpoint's name; the "#Level:" search compares whole
// lines, so a section line spelled differently is missed and a second section is appended that the parser
// (first match) never reads; at end of file without "#End" the first loop never ends.
int level_items_write_table(const char *temp)
{
    TextFile tf;
    File old, out;
    char line[0x200];
    char level_line[0x40];
    char name[0x40], script[0x40], buf[0x40];
    level_item_rec *li;
    bool keep, at_end;
    int i, count;

    tf.Open("level_items.tbl", path_tables);
    out.open(temp, 0x80000002, path_tables);
    sprintf(level_line, "#Level: \"%s\"", level_script_table[cur_level_index].name);
    for (;;) {
        tf.ReadLine(line, 0x200);
        if (strncmp(line, "#End", 4) == 0) {
            out.write_line("// -----------------------------------------------------------------------------", -1);
            out.write_line("//", -1);
            sprintf(line, "#Level: \"%s\"", level_script_table[cur_level_index].name);
            out.write_line(line, -1);
            out.write_line(NULL, -1);
            at_end = true;
            goto write_items;
        }
        out.write_line(line, -1);
        if (strcmp(line, level_line) == 0)
            break;
    }
    out.write_line(NULL, -1);
    while (tf.Accept("$Name:")) {
        tf.ParseString(name, 0x40, '"', '"');
        tf.Expect("$Script:");
        tf.ParseString(script, 0x40, '"', '"');
        keep = _stricmp(script, level_script_name) != 0;
        if (keep) {
            sprintf(line, "$Name: \"%s\"", name);
            out.write_line(line, -1);
            sprintf(line, "$Script: \"%s\"", script);
            out.write_line(line, -1);
        }
        tf.Expect("$Item:");
        tf.ParseString(buf, 0x40, '"', '"');
        if (keep) {
            sprintf(line, "$Item: \"%s\"", buf);
            out.write_line(line, -1);
        }
        tf.Expect("$Navpoint:");
        tf.ParseString(buf, 0x40, '"', '"');
        if (keep) {
            sprintf(line, "$Navpoint: \"%s\"", buf);
            out.write_line(line, -1);
        }
        if (tf.Peek("+Count:")) {
            tf.Expect("+Count:");
            count = tf.ParseInt();
            if (keep) {
                sprintf(line, "+Count: %d", count);
                out.write_line(line, -1);
            }
        }
        if (tf.Peek("+Event:")) {
            tf.Expect("+Event:");
            tf.ParseString(buf, 0x40, '"', '"');
            sprintf(line, "+Event: \"%s\" \"", buf);
            tf.ParseString(buf, 0x40, '"', '"');
            strcat(line, buf);
            strcat(line, "\"");
            if (keep)
                out.write_line(line, -1);
        }
        if (keep)
            out.write_line(NULL, -1);
    }
    at_end = !tf.SkipTo("#Level:", "#End");
write_items:
    for (i = 0, li = level_items; i < level_item_count; i++, li++) {
        if (li->navpoint == NULL)
            continue;
        sprintf(line, "$Name: \"%s\"", li->name);
        out.write_line(line, -1);
        sprintf(line, "$Script: \"%s\"", li->script);
        out.write_line(line, -1);
        sprintf(line, "$Item: \"%s\"", li->type->name);
        out.write_line(line, -1);
        sprintf(line, "$Navpoint: \"%s\"", li->navpoint->name);
        out.write_line(line, -1);
        if (li->count > 1) {
            sprintf(line, "+Count: %d", li->count);
            out.write_line(line, -1);
        }
        if (li->flag_level != -1) {
            sprintf(line, "+Event: \"%s\" \"%s\"", level_script_table[li->flag_level].name, li->flag);
            out.write_line(line, -1);
        }
        out.write_line(NULL, -1);
    }
    if (at_end) {
        out.write_line("#End", -1);
    } else {
        out.write_line("// -----------------------------------------------------------------------------", -1);
        out.write_line("//", -1);
        while (tf.ReadLine(line, 0x200))
            out.write_line(line, -1);
    }
    tf.Close();
    i = old.Delete("level_items.tbl", path_tables);
    if (i == 0) {
        i = out.close();
        if (i == 0)
            i = out.make_backup("level_items.tbl", temp, path_tables);
    }
    return i;
}

// The save error box (written out for each case in the binary, tail-merged).
#define SE_SAVE_ERROR(fmt, arg)                                                     \
    do {                                                                            \
        sprintf(msg, (fmt), (arg));                                                 \
        editor_dialog_show(400, "Error!", msg, EDITOR_DIALOG_OK, NULL, NULL, NULL, 0xb); \
    } while (0)

// 0x4eac70  Confidence: exact (VC6 byte match) (jump table 0x4eadf4; the tail merges). The result of script_save_files:
// a status line and the save window closed, or an error box.
void script_save(int result)
{
    char filename[0x24];
    char msg[0x100];
    char table_name[0x100];

    strcpy(filename, script_editor_save_filename_inputbox.get_text());
    sprintf(table_name, "%s.tbl", level_filename);
    switch (result) {
    case 0:
        sprintf(msg, "Script \"%s\" saved.", filename);
        script_editor_set_status(msg);
        script_editor_save_window.set_disabled(true);
        return;
    case 1:
        SE_SAVE_ERROR("Unable to save because the script file \"%s\" is read-only. Make sure you have the file checked out.", filename);
        return;
    case 2:
        SE_SAVE_ERROR("Unable to save because the table file \"%s\" is read-only. Make sure you have the file checked out.", table_name);
        return;
    case 3:
        SE_SAVE_ERROR("Unable to save because the table file \"%s\" is read-only. Make sure you have the file checked out.", "level_items.tbl");
        return;
    case 4:
        SE_SAVE_ERROR("Error occurred while writing \"%s\".", filename);
        return;
    case 5:
        SE_SAVE_ERROR("Error occurred while writing \"%s\".", table_name);
        return;
    case 6:
        SE_SAVE_ERROR("Error occurred while writing \"%s\".", "level_items.tbl");
        return;
    }
    sprintf(msg, "Unknown error occurred... Don't panic -- get Niq!");
    editor_dialog_show(400, "Error!", msg, EDITOR_DIALOG_OK, NULL, NULL, NULL, 0xb);
}

// 0x4eae10  Confidence: exact. The save window's Level / Cutscene radios: a level script is saved under the
// level's name (box hidden), a cutscene script under a typed name.
bool script_editor_save_kind_radio_cb(int event, int value)
{
    char filename[0x24];

    if (event == 0x13) {
        if (script_editor_save_level_radio.is_checked()) {
            script_editor_save_name_inputbox.set_text(level_script_name ? level_script_name : level_filename);
            script_editor_save_name_inputbox.set_hidden(true);
            script_editor_save_name_inputbox.max_length = 0x20 - strlen("_script.tbl");
            str_copy_append_32(filename, level_script_name ? level_script_name : level_filename, "_script.tbl");
        } else {
            script_editor_save_name_inputbox.set_visible(true);
            script_editor_save_name_inputbox.set_focus();
            script_editor_save_name_inputbox.max_length = 0x20 - strlen("_cscript.tbl");
            str_copy_append_32(filename, script_editor_save_name_inputbox.get_text(), "_cscript.tbl");
        }
        script_editor_save_filename_inputbox.set_text(filename);
        return true;
    }
    return false;
}

// 0x4eaf00  Confidence: exact.
bool script_editor_save_ok_cb(int event, int value)
{
    int result;

    if (value == script_editor_save_ok_button.id && event == UI_EVENT_PRESSED) {
        result = script_save_files();
        script_save(result);
        if (result == 0)
            script_editor_save_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4eaf40  Confidence: exact.
bool script_editor_save_cancel_cb(int event, int value)
{
    if (value == script_editor_save_cancel_button.id && event == UI_EVENT_PRESSED) {
        script_editor_save_window.set_disabled(true);
        return true;
    }
    return false;
}

// The type-ahead of the editor's name boxes (event 0x15, "__complete" in sum.symbols): the first line that
// starts with the text is scrolled to, the line equal to it (or the last tried) selected; no match clears
// the selection. Written out in every callback in the binary. `first` is the caller's function-scope
// `ui_listbox_entry *first = NULL;` (the binary clears it in the prologue, D7803 / D8701), and the case has
// one exit (`goto complete_done`): with two `return true;` cl 8966 saves EBX / ESI / EDI at the function's
// entry, with one it saves them only on the 0x15 path, as Sum.exe (shrink-wrapping, D8701).
#define SE_COMPLETE(box, list, found_stmt, none_stmt)                                       \
    do {                                                                                    \
        const char *text_ = (box).get_text();                                               \
        int len_ = strlen(text_);                                                           \
        ui_listbox_entry *e_;                                                               \
        if (len_ > 0) {                                                                     \
            for (e_ = (list).entries.next; e_ != &(list).entries; e_ = e_->next) {          \
                if (_strnicmp(text_, e_->text, len_) == 0) {                                \
                    if (first == NULL)                                                      \
                        first = e_;                                                         \
                    if (_stricmp(text_, e_->text) == 0)                                     \
                        break;                                                              \
                }                                                                           \
            }                                                                               \
            if (first != NULL)                                                              \
                (list).set_vscroll(first->y, false);                                        \
            if (e_ != &(list).entries) {                                                    \
                (list).select_entry(e_, true);                                              \
                found_stmt;                                                                 \
                goto complete_done;                                                         \
            }                                                                               \
        }                                                                                   \
        (list).clear_selection();                                                           \
        none_stmt;                                                                          \
    complete_done:                                                                          \
        return true;                                                                        \
    } while (0)

// 0x4eaf70  Confidence: exact (the type-ahead as a macro, D8701). 0x4eaf87 script_editor_script_name_cb__complete:
// 0x4eaf87  script_editor_script_name_cb__complete: the label of the event 0x15 (type-ahead) case inside the callback described here, not a function. Confidence: exact.
// a label inside (the event 0x15 case), not a function. The load window's name box.
bool script_editor_script_name_cb(int event, int value)
{
    ui_listbox_entry *first = NULL;

    switch (event) {
    case 0x14:
        if (ui_key == KEY_ENTER && script_editor_load_ok_button.is_shown() && script_editor_load_selected_script())
            script_editor_load_window.set_disabled(true);
        return true;
    case 0x15:
        SE_COMPLETE(script_editor_load_name_inputbox, script_editor_load_listbox,
                    script_editor_load_ok_button.set_visible(true),
                    script_editor_load_ok_button.set_hidden(true));
    }
    return false;
}

// 0x4eb080  Confidence: exact. Loads the level script again, or the named cutscene script (false when its
// cutscene folder is not found).
bool script_editor_load_selected_script()
{
    if (script_editor_load_level_radio.is_checked()) {
        level_script_reload();
        script_editor_hover.kind = SE_PICK_NONE;
        script_editor_sel.kind = SE_PICK_NONE;
        script_editor_sync_cutscene_button();
        return true;
    }
    if (the_cutscene.set_data_path(script_editor_load_name_inputbox.get_text()) < 0)
        return false;
    cutscene_script_load(script_editor_load_name_inputbox.get_text());
    script_editor_hover.kind = SE_PICK_NONE;
    script_editor_sel.kind = SE_PICK_NONE;
    script_editor_sync_cutscene_button();
    return true;
}

// 0x4eb0f0  Confidence: exact.
bool script_editor_load_listbox_cb(int event, int value)
{
    switch (event) {
    case UI_EVENT_LIST_SELECT:
        script_editor_load_name_inputbox.set_text(script_editor_load_listbox.get_selected()->text);
        script_editor_load_ok_button.set_visible(true);
        return true;
    case UI_EVENT_LIST_DBLCLICK:
        if (script_editor_load_selected_script())
            script_editor_load_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4eb140  Confidence: exact.
bool script_editor_load_kind_radio_cb(int event, int value)
{
    if (event == 0x13) {
        script_editor_list_scripts();
        return true;
    }
    return false;
}

// 0x4eb160  Confidence: exact.
bool script_editor_load_ok_cb(int event, int value)
{
    if (value == script_editor_load_ok_button.id && event == UI_EVENT_PRESSED) {
        if (script_editor_load_selected_script())
            script_editor_load_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4eb190  Confidence: exact.
bool script_editor_load_cancel_cb(int event, int value)
{
    if (value == script_editor_load_cancel_button.id && event == UI_EVENT_PRESSED) {
        script_editor_load_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4eb1c0  Confidence: exact (as script_editor_script_name_cb). 0x4eb1d7 script_editor_ambient_name_cb__complete
// 0x4eb1d7  script_editor_ambient_name_cb__complete: the label of the event 0x15 (type-ahead) case inside the callback described here, not a function. Confidence: exact.
// is a label inside. Tab goes on to Min range, Enter leaves the box.
bool script_editor_ambient_name_cb(int event, int value)
{
    ui_listbox_entry *first = NULL;

    switch (event) {
    case 0x14:
        if (ui_key == KEY_TAB) {
            script_editor_ambient_min_range_inputbox.set_focus();
            return true;
        }
        if (ui_key == KEY_ENTER)
            script_editor_ambient_name_inputbox.clear_focus();
        return true;
    case 0x15:
        SE_COMPLETE(script_editor_ambient_name_inputbox, script_editor_ambient_listbox, (void)0, (void)0);
    }
    return false;
}

// 0x4eb2b0  Confidence: exact.
bool script_editor_ambient_listbox_cb(int event, int value)
{
    if (event == UI_EVENT_LIST_SELECT) {
        script_editor_ambient_name_inputbox.set_text(script_editor_ambient_listbox.get_selected()->text);
        return true;
    }
    return false;
}

// 0x4eb2e0  Confidence: exact. The four number boxes of the ambient window: leaving one rewrites it with one
// decimal (roll-off at least 0.1); Tab goes to the next box, Enter leaves the box.
bool script_editor_ambient_field_cb(int event, int value)
{
    char buf[0x20];
    ui_inputbox *g, *next;
    float v;

    g = (ui_inputbox *)script_editor_ambient_window.find_gadget_by_id(value);
    if (g == &script_editor_ambient_min_range_inputbox)
        next = &script_editor_ambient_volume_inputbox;
    else if (g == &script_editor_ambient_volume_inputbox)
        next = &script_editor_ambient_rolloff_inputbox;
    else if (g == &script_editor_ambient_rolloff_inputbox)
        next = &script_editor_ambient_loop_delay_inputbox;
    else if (g == &script_editor_ambient_loop_delay_inputbox)
        next = &script_editor_ambient_name_inputbox;
    else
        return false;
    switch (event) {
    case 4:
        v = atof(g->get_text());
        if (g == &script_editor_ambient_rolloff_inputbox && v < 0.1f)
            v = 0.1f;
        sprintf(buf, "%.1f", v);
        g->set_text(buf);
        return true;
    case 0x14:
        if (ui_key == KEY_TAB) {
            next->set_focus();
            return true;
        }
        if (ui_key == KEY_ENTER)
            g->clear_focus();
        return true;
    }
    return false;
}

// 0x4eb3d0  Confidence: exact. OK of the ambient window: the ambient at the selected navpoint is replaced
// by one with the chosen sound ("ambient\<name>") and parameters; the level table is marked for saving.
// Game bug: the replaced sound's handle is not released (sounds[] slots are used up by repeated edits).
bool script_editor_ambient_ok(int event, int value)
{
    char name[0x100];
    float min_range, volume, rolloff;
    int delay, index, snd;

    if (value != script_editor_ambient_ok_button.id || event != UI_EVENT_PRESSED)
        return false;
    sprintf(name, "ambient\\%s", script_editor_ambient_name_inputbox.get_text());
    min_range = atof(script_editor_ambient_min_range_inputbox.get_text());
    volume = atof(script_editor_ambient_volume_inputbox.get_text());
    rolloff = atof(script_editor_ambient_rolloff_inputbox.get_text());
    delay = (int)(atof(script_editor_ambient_loop_delay_inputbox.get_text()) * 1000.0);
    index = level_ambient_find_at(&((navpoint *)script_editor_sel.object)->pos);
    if (index >= 0)
        level_ambient_remove(index);
    snd = snd_get_handle(name, min_range, volume, rolloff, 0, false);
    if (snd >= 0)
        level_ambient_add(snd, &((navpoint *)script_editor_sel.object)->pos, delay);
    script_ambients_dirty = true;
    script_editor_ambient_window.set_disabled(true);
    return true;
}

// 0x4eb500  Confidence: exact.
bool script_editor_ambient_cancel_cb(int event, int value)
{
    if (value == script_editor_ambient_cancel_button.id && event == UI_EVENT_PRESSED) {
        script_editor_ambient_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4eb530  Confidence: exact (the search loop's form). The parameter window's keys: Enter presses Ok, Esc
// Cancel, Tab moves to the next shown box.
bool script_editor_params_window_cb(int event, int value)
{
    int i;

    if (event != 0x14)
        return false;
    if (ui_key == KEY_ENTER) {
        script_editor_params_ok_button.press();
        return true;
    }
    if (ui_key == KEY_ESC) {
        script_editor_params_cancel_button.press();
        return true;
    }
    if (ui_key == KEY_TAB) {
        for (i = 0; i < 5 && &script_editor_param_inputboxes[i] != ui_current_gadget; i++)
            ;
        if (i < 5)
            script_editor_param_inputboxes[(i + 1) % script_editor_param_field_count].set_focus();
        return true;
    }
    return false;
}

// 0x4eb5c0  Confidence: likely (jump tables 0x4eb72c / 0x4eb740 over type - 3). Ok of the parameter window:
// the boxes back into the action (atoi), the list line updated.
bool script_editor_params_ok_cb(int event, int value)
{
    char args[0x104], name[0x104];
    script_action *a;
    int n;

    if (event != UI_EVENT_PRESSED)
        return false;
    a = &script_editor_cur_script->actions[script_editor_cur_action->data];
    switch (a->type) {
    case SCRIPT_ACTION_WAIT_AT: case SCRIPT_ACTION_TURN_HOSTILE: case SCRIPT_ACTION_SAY:
    case SCRIPT_ACTION_LABEL: case SCRIPT_ACTION_GOTO: case SCRIPT_ACTION_NO_BLOCK:
    case SCRIPT_ACTION_TRIGGER: case SCRIPT_ACTION_TRIGGER_NAV: case SCRIPT_ACTION_TRIGGER_VISIT:
    case SCRIPT_ACTION_SHOW_HIDE: case SCRIPT_ACTION_SYNCH: case SCRIPT_ACTION_SYNCH_TELEPORT:
    case SCRIPT_ACTION_COMBAT_MODE: case SCRIPT_ACTION_GUARD_SPLINE: case SCRIPT_ACTION_ONE_HAND:
        a->arg0c = atoi(script_editor_param_inputboxes[0].get_text());
        break;
    case SCRIPT_ACTION_WANDER:
    case SCRIPT_ACTION_WANDER_FOR:
        n = 1;
        a->arg0c = atoi(script_editor_param_inputboxes[0].get_text());
        if (a->type == SCRIPT_ACTION_WANDER_FOR) {
            n = 2;
            a->arg10 = atoi(script_editor_param_inputboxes[1].get_text());
        }
        a->flag14 = atoi(script_editor_param_inputboxes[n].get_text()) != 0;
        break;
    case SCRIPT_ACTION_PASS_GO:
        script_editor_store_string(a, script_editor_param_inputboxes[0].get_text());
        break;
    case SCRIPT_ACTION_FOLLOW:
        script_editor_store_string(a, script_editor_param_inputboxes[0].get_text());
        a->arg0c = atoi(script_editor_param_inputboxes[1].get_text());
        break;
    }
    SE_UPDATE_ACTION_LINE(a);
    script_editor_params_window.set_disabled(true);
    return true;
}

// 0x4eb760  Confidence: exact.
bool script_editor_params_cancel_cb(int event, int value)
{
    if (event == UI_EVENT_PRESSED) {
        script_editor_params_window.set_disabled(true);
        return true;
    }
    return false;
}

// The mode choice (written out twice in the binary): the mode, its name on the button, and the drag / New /
// Del / Prev / Next buttons hidden in door and click-object modes.
#define SE_SET_MODE()                                                                          \
    do {                                                                                       \
        script_editor_mode = script_editor_choice_listbox.get_selected()->data;                \
        script_editor_choice_window.set_disabled(true);                                        \
        script_editor_mode_button.set_text(script_editor_mode_names[script_editor_mode]);      \
        if (script_editor_mode == SE_MODE_DOOR || script_editor_mode == SE_MODE_CLICK_OBJECT) { \
            script_editor_pos_button.set_hidden(true);                                         \
            script_editor_ori_button.set_hidden(true);                                         \
            script_editor_y_button.set_hidden(true);                                           \
            script_editor_new_button.set_hidden(true);                                         \
            script_editor_delete_button.set_hidden(true);                                      \
            script_editor_prev_button.set_hidden(true);                                        \
            script_editor_next_button.set_hidden(true);                                        \
        } else {                                                                               \
            script_editor_pos_button.set_visible(true);                                        \
            script_editor_ori_button.set_visible(true);                                        \
            script_editor_y_button.set_visible(true);                                          \
            script_editor_new_button.set_visible(true);                                        \
            script_editor_delete_button.set_visible(true);                                     \
            script_editor_prev_button.set_visible(true);                                       \
            script_editor_next_button.set_visible(true);                                       \
        }                                                                                      \
    } while (0)

// 0x4eb780  Confidence: exact (VC6 byte match) (the shared block as a macro).
bool script_editor_choice_ok_cb(int event, int value)
{
    if (value == script_editor_choice_ok_button.id && event == UI_EVENT_PRESSED) {
        SE_SET_MODE();
        return true;
    }
    return false;
}

// 0x4eb8a0  Confidence: exact.
bool script_editor_choice_cancel_cb(int event, int value)
{
    if (value == script_editor_choice_cancel_button.id && event == UI_EVENT_PRESSED) {
        script_editor_choice_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4eb8d0  Confidence: exact (VC6 byte match) (as script_editor_choice_ok_cb). A double click picks the mode.
bool script_editor_choice_listbox_cb(int event, int value)
{
    if (value != script_editor_choice_listbox.id)
        return false;
    if (event != UI_EVENT_LIST_DBLCLICK)
        return false;
    SE_SET_MODE();
    return true;
}

// 0x4eb9e0  Confidence: likely (the shared not-found tail). The Opening / Closing radios switch which set of
// sound fields is active and show that set's sound in the name box.
bool script_editor_door_sound_radio_cb(int event, int value)
{
    ui_listbox_entry *e;

    if (event != 0x13)
        return false;
    if (value == script_editor_door_opening_radio.id) {
        script_editor_door_open_sound_listbox.set_enabled(true);
        script_editor_door_open_min_range_inputbox.set_enabled(true);
        script_editor_door_open_volume_inputbox.set_enabled(true);
        script_editor_door_open_rolloff_inputbox.set_enabled(true);
        script_editor_door_close_sound_listbox.set_disabled(true);
        script_editor_door_close_min_range_inputbox.set_disabled(true);
        script_editor_door_close_volume_inputbox.set_disabled(true);
        script_editor_door_close_rolloff_inputbox.set_disabled(true);
        e = script_editor_door_open_sound_listbox.get_selected();
    } else if (value == script_editor_door_closing_radio.id) {
        script_editor_door_open_sound_listbox.set_disabled(true);
        script_editor_door_open_min_range_inputbox.set_disabled(true);
        script_editor_door_open_volume_inputbox.set_disabled(true);
        script_editor_door_open_rolloff_inputbox.set_disabled(true);
        script_editor_door_close_sound_listbox.set_enabled(true);
        script_editor_door_close_min_range_inputbox.set_enabled(true);
        script_editor_door_close_volume_inputbox.set_enabled(true);
        script_editor_door_close_rolloff_inputbox.set_enabled(true);
        e = script_editor_door_close_sound_listbox.get_selected();
    } else {
        return false;
    }
    if (e != NULL) {
        script_editor_door_sound_inputbox.set_text(e->text);
        return true;
    }
    script_editor_door_sound_inputbox.set_text("");
    return true;
}

// 0x4ebb20  Confidence: likely (the type-ahead as a macro). The door window's sound name box: type-ahead in
// the active sound list; Tab goes to the active Min range box. Game bug: Enter clears the focus of the
// key item box, not of this box.
bool script_editor_door_sound_inputbox_cb(int event, int value)
{
    ui_listbox_entry *first = NULL;
    ui_listbox *list;

    if (value != script_editor_door_sound_inputbox.id)
        return false;
    switch (event) {
    case 0x14:
        if (ui_key == KEY_TAB) {
            if (script_editor_door_opening_radio.is_checked())
                script_editor_door_open_min_range_inputbox.set_focus();
            else
                script_editor_door_close_min_range_inputbox.set_focus();
            return true;
        }
        if (ui_key == KEY_ENTER)
            script_editor_door_key_inputbox.clear_focus();
        return true;
    case 0x15:
        if (script_editor_door_opening_radio.is_checked())
            list = &script_editor_door_open_sound_listbox;
        else if (script_editor_door_closing_radio.is_checked())
            list = &script_editor_door_close_sound_listbox;
        else
            return false;
        SE_COMPLETE(script_editor_door_sound_inputbox, *list, (void)0, (void)0);
    }
    return false;
}

// 0x4ebc80  Confidence: exact.
bool script_editor_door_sound_listbox_cb(int event, int value)
{
    if ((value == script_editor_door_open_sound_listbox.id || value == script_editor_door_close_sound_listbox.id)
        && event == UI_EVENT_LIST_SELECT) {
        if (value == script_editor_door_open_sound_listbox.id) {
            script_editor_door_sound_inputbox.set_text(script_editor_door_open_sound_listbox.get_selected()->text);
            return true;
        }
        if (value == script_editor_door_close_sound_listbox.id) {
            script_editor_door_sound_inputbox.set_text(script_editor_door_close_sound_listbox.get_selected()->text);
            return true;
        }
    }
    return false;
}

// 0x4ebcf0  Confidence: exact. The six sound number boxes of the door window (as the ambient fields); Tab
// after a roll-off goes to the pick skill box when it is shown, else to the sound name.
bool script_editor_door_field_cb(int event, int value)
{
    char buf[0x20];
    ui_inputbox *g, *next;
    float v;

    g = (ui_inputbox *)script_editor_door_window.find_gadget_by_id(value);
    if (g == &script_editor_door_open_min_range_inputbox)
        next = &script_editor_door_open_volume_inputbox;
    else if (g == &script_editor_door_close_min_range_inputbox)
        next = &script_editor_door_close_volume_inputbox;
    else if (g == &script_editor_door_open_volume_inputbox)
        next = &script_editor_door_open_rolloff_inputbox;
    else if (g == &script_editor_door_close_volume_inputbox)
        next = &script_editor_door_close_rolloff_inputbox;
    else if (g == &script_editor_door_open_rolloff_inputbox || g == &script_editor_door_close_rolloff_inputbox)
        next = script_editor_door_skill_inputbox.is_shown() ? &script_editor_door_skill_inputbox
                                                            : &script_editor_door_sound_inputbox;
    else
        return false;
    switch (event) {
    case 4:
        v = atof(g->get_text());
        if ((g == &script_editor_door_open_rolloff_inputbox || g == &script_editor_door_close_rolloff_inputbox)
            && v < 0.1f)
            v = 0.1f;
        sprintf(buf, "%.1f", v);
        g->set_text(buf);
        return true;
    case 0x14:
        if (ui_key == KEY_TAB) {
            next->set_focus();
            return true;
        }
        if (ui_key == KEY_ENTER) {
            g->clear_focus();
            return true;
        }
        return true;
    }
    return false;
}

// 0x4ebe10  Confidence: exact. The gold box of the door lock window, rewritten as an integer.
bool script_editor_door_lock_value_cb(int event, int value)
{
    char buf[0x20];
    ui_inputbox *g;

    g = (ui_inputbox *)script_editor_door_lock_window.find_gadget_by_id(value);
    switch (event) {
    case 4:
        sprintf(buf, "%d", atoi(g->get_text()));
        g->set_text(buf);
        return true;
    case 0x14:
        if (ui_key == KEY_ENTER) {
            g->clear_focus();
            return true;
        }
        return true;
    }
    return false;
}

// 0x4ebe90  Confidence: exact. "Originally locked" shows the pick skill and key fields.
bool script_editor_door_locked_cb(int event, int value)
{
    bool locked;

    if (value == script_editor_door_locked_checkbox.id && event == 0x13) {
        locked = script_editor_door_locked_checkbox.is_checked();
        script_editor_door_skill_inputbox.set_visible(locked);
        script_editor_door_key_inputbox.set_visible(locked);
        script_editor_door_key_listbox.set_visible(locked);
        return true;
    }
    return false;
}

// 0x4ebee0  Confidence: exact. "Give item" shows the gold box (1 when ticked, else 0) and the item list.
bool script_editor_door_lock_give_checkbox_cb(int event, int value)
{
    bool give;

    if (value == script_editor_door_lock_give_checkbox.id && event == 0x13) {
        give = script_editor_door_lock_give_checkbox.is_checked();
        script_editor_door_lock_value_inputbox.set_visible(give);
        script_editor_door_lock_item_listbox.set_visible(give);
        if (give) {
            script_editor_door_lock_value_inputbox.set_text("1");
            return true;
        }
        script_editor_door_lock_value_inputbox.set_text("0");
        return true;
    }
    return false;
}

// 0x4ebf50  Confidence: exact. The pick skill box, at most 1000 (percent; 1000 = a lock value of 10).
bool script_editor_door_skill_cb(int event, int value)
{
    char buf[0x20];
    int n;

    if (value != script_editor_door_skill_inputbox.id)
        return false;
    switch (event) {
    case 4:
        n = atoi(script_editor_door_skill_inputbox.get_text());
        if (n > 1000)
            n = 1000;
        sprintf(buf, "%d", n);
        script_editor_door_skill_inputbox.set_text(buf);
        return true;
    case 0x14:
        if (ui_key == KEY_TAB) {
            script_editor_door_key_inputbox.set_focus();
            return true;
        }
        if (ui_key == KEY_ENTER) {
            script_editor_door_skill_inputbox.clear_focus();
            return true;
        }
        return true;
    }
    return false;
}

// 0x4ec000  Confidence: exact (the type-ahead as a macro, D8701). 0x4ec029 script_editor_door_key_cb__complete is a
// 0x4ec029  script_editor_door_key_cb__complete: the label of the event 0x15 (type-ahead) case inside the callback described here, not a function. Confidence: exact.
// label inside. The key item box of the door window.
bool script_editor_door_key_cb(int event, int value)
{
    ui_listbox_entry *first = NULL;

    if (value != script_editor_door_key_inputbox.id)
        return false;
    switch (event) {
    case 0x14:
        if (ui_key == KEY_TAB) {
            script_editor_door_sound_inputbox.set_focus();
            return true;
        }
        if (ui_key == KEY_ENTER)
            script_editor_door_key_inputbox.clear_focus();
        return true;
    case 0x15:
        SE_COMPLETE(script_editor_door_key_inputbox, script_editor_door_key_listbox, (void)0, (void)0);
    }
    return false;
}

// 0x4ec100  Confidence: exact (as script_editor_door_key_cb). 0x4ec129 script_editor_door_item_cb__complete
// 0x4ec129  script_editor_door_item_cb__complete: the label of the event 0x15 (type-ahead) case inside the callback described here, not a function. Confidence: exact.
// is a label inside. The text box of the door lock / clickable object window.
bool script_editor_door_item_cb(int event, int value)
{
    ui_listbox_entry *first = NULL;

    if (value != script_editor_door_lock_item_inputbox.id)
        return false;
    switch (event) {
    case 0x14:
        if (ui_key != KEY_TAB && ui_key == KEY_ENTER)
            script_editor_door_lock_item_inputbox.clear_focus();
        return true;
    case 0x15:
        SE_COMPLETE(script_editor_door_lock_item_inputbox, script_editor_door_lock_item_listbox, (void)0, (void)0);
    }
    return false;
}

// 0x4ec1f0  Confidence: exact.
bool script_editor_door_key_listbox_cb(int event, int value)
{
    if (value == script_editor_door_key_listbox.id && event == UI_EVENT_LIST_SELECT) {
        script_editor_door_key_inputbox.set_text(script_editor_door_key_listbox.get_selected()->text);
        return true;
    }
    return false;
}

// 0x4ec230  Confidence: exact.
bool script_editor_door_lock_item_listbox_cb(int event, int value)
{
    if (value == script_editor_door_lock_item_listbox.id && event == UI_EVENT_LIST_SELECT) {
        script_editor_door_lock_item_inputbox.set_text(script_editor_door_lock_item_listbox.get_selected()->text);
        return true;
    }
    return false;
}

// One door sound from the window (written out twice in the binary): "Doors\<file>" in the level script
// string pool, the three parameter bytes, then the sound loaded; none clears the reference.
#define SE_DOOR_SOUND(ref, list, min_box, volume_box, rolloff_box)                          \
    do {                                                                                    \
        e = (list).get_selected();                                                          \
        if (e != NULL) {                                                                    \
            strcpy(name, "Doors\\");                                                        \
            strcat(name, e->text);                                                          \
            (ref).name = level_script_stringpool.AddUnique(name);                           \
            (ref).params[1] = (ubyte)(int)atof((min_box).get_text());                       \
            (ref).params[0] = (ubyte)(int)(atof((volume_box).get_text()) * 255.0);          \
            (ref).params[2] = (ubyte)(int)atof((rolloff_box).get_text());                   \
            sound_ref_load(&(ref), false);                                                  \
        } else {                                                                            \
            (ref).name = NULL;                                                              \
            (ref).handle = -1;                                                              \
        }                                                                                   \
    } while (0)

// 0x4ec270  Confidence: exact (the lock branch's shared key store). OK of the door window: both sounds,
// the lock (pick skill percent / 100, key item), and the saved open / locked state of the door. Game bug:
// the parameter boxes were filled with "%.1f" of the bytes (script_editor_list_door_sounds), so OK after
// opening the window stores whatever those boxes show back into the door.
bool script_editor_door_ok(int event, int value)
{
    char name[0x100];
    door_info *d;
    ui_listbox_entry *e;
    bool open, locked;
    int skill;

    if (value != script_editor_door_ok_button.id || event != UI_EVENT_PRESSED)
        return false;
    d = door_from_lobj((lobj *)script_editor_sel.object);
    SE_DOOR_SOUND(d->open_sound, script_editor_door_open_sound_listbox, script_editor_door_open_min_range_inputbox,
                  script_editor_door_open_volume_inputbox, script_editor_door_open_rolloff_inputbox);
    SE_DOOR_SOUND(d->close_sound, script_editor_door_close_sound_listbox, script_editor_door_close_min_range_inputbox,
                  script_editor_door_close_volume_inputbox, script_editor_door_close_rolloff_inputbox);
    savegame_state.two_stage_get(&open, &locked, (lobj *)script_editor_sel.object);
    if (script_editor_door_locked_checkbox.is_checked()) {
        d->state |= 8;
        savegame_state.two_stage_set(open, true, (lobj *)script_editor_sel.object);
        skill = atoi(script_editor_door_skill_inputbox.get_text());
        d->lock = skill * 0.01f;
        e = script_editor_door_key_listbox.get_selected();
        if (e != NULL)
            d->key = &item_types[item_type_find(e->text)];
        else
            d->key = NULL;
    } else {
        d->state &= ~8;
        savegame_state.two_stage_set(open, false, (lobj *)script_editor_sel.object);
        d->lock = 0;
        d->key = NULL;
    }
    script_editor_door_window.set_disabled(true);
    return true;
}

// 0x4ec570  Confidence: exact (`rec = NULL` after the early return, D8701). OK of the door lock /
// clickable object window: the text (the "+Messagebox:" of #Clicks) and the gold ("+Give:"). The window
// says "Give item", but what is given is gold.
bool script_editor_door_lock_ok_cb(int event, int value)
{
    lc_entry *rec;
    int i;

    if (value != script_editor_door_lock_ok_button.id || event != UI_EVENT_PRESSED)
        return false;
    rec = NULL;
    for (i = 0; i < lc_entry_count; i++) {
        if (lc_entries[i].object == script_editor_sel.object)
            rec = &lc_entries[i];
    }
    strcpy(rec->name, script_editor_door_lock_item_inputbox.get_text());
    if (script_editor_door_lock_give_checkbox.is_checked())
        rec->gold = atoi(script_editor_door_lock_value_inputbox.get_text());
    else
        rec->gold = 0;
    script_editor_door_lock_window.set_disabled(true);
    return true;
}

// 0x4ec630  Confidence: exact. Remove: the object stops being clickable.
bool script_editor_door_lock_remove_cb(int event, int value)
{
    if (value == script_editor_door_lock_remove_button.id && event == UI_EVENT_PRESSED) {
        lc_entry_clear((lobj *)script_editor_sel.object);
        script_editor_door_lock_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4ec670  Confidence: exact.
bool script_editor_door_cancel_cb(int event, int value)
{
    if (value == script_editor_door_cancel_button.id && event == UI_EVENT_PRESSED) {
        script_editor_door_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4ec6a0  Confidence: exact. The creature level box, never negative.
bool script_editor_creature_level_cb(int event, int value)
{
    char buf[0x20];
    int level;

    if (value != script_editor_creature_level_inputbox.id)
        return false;
    switch (event) {
    case 4:
        level = atoi(script_editor_creature_level_inputbox.get_text());
        if (level < 0)
            level = 0;
        sprintf(buf, "%d", level);
        script_editor_creature_level_inputbox.set_text(buf);
        return true;
    case 0x14:
        if (ui_key == KEY_ENTER) {
            script_editor_creature_level_inputbox.clear_focus();
            return true;
        }
        return true;
    }
    return false;
}

// 0x4ec730  Confidence: likely (the skill test's form; the level is kept in the event argument's slot). OK of
// the creature window: team and object type (friendly / neutral 4, hostile / evil 5), the level (0 = the
// party leader's, flag 0x400000 cleared) with its XP, and the stats scaled by level / char_info level:
// base HP, HP, base AP, AP, the skills. Game bugs: with level 0 the scale is 0 (it uses the typed level,
// not the leader's), so base HP and AP become 0; and the skill clamp tests `v > 0 || v < 100`, true for
// every value, so every skill but skill 2 is set to 100.
bool script_editor_creature_ok(int event, int value)
{
    float scale, v;
    int level, i;

    if (value != script_editor_creature_ok_button.id || event != UI_EVENT_PRESSED)
        return false;
    if (script_editor_creature_friendly_radio.is_checked()) {
        ((Character *)script_editor_sel.object)->set_team(0);
        ((Character *)script_editor_sel.object)->type = 4;
    } else if (script_editor_creature_neutral_radio.is_checked()) {
        ((Character *)script_editor_sel.object)->set_team(1);
        ((Character *)script_editor_sel.object)->type = 4;
    } else if (script_editor_creature_hostile_radio.is_checked()) {
        ((Character *)script_editor_sel.object)->set_team(2);
        ((Character *)script_editor_sel.object)->type = 5;
    } else if (script_editor_creature_evil_radio.is_checked()) {
        ((Character *)script_editor_sel.object)->set_team(3);
        ((Character *)script_editor_sel.object)->type = 5;
    } else {
        return false;
    }
    level = atoi(script_editor_creature_level_inputbox.get_text());
    if (level > 0) {
        ((Character *)script_editor_sel.object)->level = level;
        ((Character *)script_editor_sel.object)->xp = xp_for_level(level);
        ((Character *)script_editor_sel.object)->flags |= 0x400000;
    } else {
        ((Character *)script_editor_sel.object)->level = cur_character->level;
        ((Character *)script_editor_sel.object)->xp = xp_for_level(cur_character->level);
        ((Character *)script_editor_sel.object)->flags &= ~0x400000;
    }
    Character *c = (Character *)script_editor_sel.object;
    scale = (float)level / c->info->level;
    c->base_max_hp = (float)(int)(c->info->max_hp * scale + 0.5f);
    ((Character *)script_editor_sel.object)->hp = (float)((Character *)script_editor_sel.object)->get_max_stat(0);
    c = (Character *)script_editor_sel.object;
    c->base_max_ap = (short)(int)(c->info->max_ap * scale + 0.5f);
    ((Character *)script_editor_sel.object)->ap = (short)((Character *)script_editor_sel.object)->get_max_stat(1);
    c = (Character *)script_editor_sel.object;
    memcpy(c->skill, c->info->skill, 0x54);
    for (i = 0; i < 0x54; i++) {
        if (i == 2)
            continue;
        c = (Character *)script_editor_sel.object;
        v = c->skill[i] * scale;
        if (v > 0.0f || v < 100.0f)
            v = 100.0f;
        c->skill[i] = (ubyte)(int)v;
    }
    script_editor_creature_window.set_disabled(true);
    return true;
}

// 0x4ec9d0  Confidence: exact.
bool script_editor_creature_cancel_cb(int event, int value)
{
    if (value == script_editor_creature_cancel_button.id && event == UI_EVENT_PRESSED) {
        script_editor_creature_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4eca00  Confidence: exact (the type-ahead as a macro, D8701). 0x4eca29 script_editor_level_item_cb__complete
// 0x4eca29  script_editor_level_item_cb__complete: the label of the event 0x15 (type-ahead) case inside the callback described here, not a function. Confidence: exact.
// is a label inside. The item name box of the level item window.
bool script_editor_level_item_cb(int event, int value)
{
    ui_listbox_entry *first = NULL;

    if (value != script_editor_level_item_name_inputbox.id)
        return false;
    switch (event) {
    case 0x14:
        if (ui_key == KEY_TAB) {
            script_editor_level_item_count_inputbox.set_focus();
            return true;
        }
        if (ui_key == KEY_ENTER)
            script_editor_level_item_name_inputbox.clear_focus();
        return true;
    case 0x15:
        SE_COMPLETE(script_editor_level_item_name_inputbox, script_editor_level_item_listbox, (void)0, (void)0);
    }
    return false;
}

// 0x4ecb00  Confidence: exact.
bool script_editor_level_item_listbox_cb(int event, int value)
{
    if (value == script_editor_level_item_listbox.id && event == UI_EVENT_LIST_SELECT) {
        script_editor_level_item_name_inputbox.set_text(script_editor_level_item_listbox.get_selected()->text);
        return true;
    }
    return false;
}

// 0x4ecb40  Confidence: exact. The count box, rewritten as an integer; Tab goes to OK.
bool script_editor_level_item_count_cb(int event, int value)
{
    char buf[0x20];

    if (value != script_editor_level_item_count_inputbox.id)
        return false;
    switch (event) {
    case 4:
        sprintf(buf, "%d", atoi(script_editor_level_item_count_inputbox.get_text()));
        script_editor_level_item_count_inputbox.set_text(buf);
        return true;
    case 0x14:
        if (ui_key == KEY_TAB) {
            script_editor_level_item_ok_button.set_focus();
            return true;
        }
        if (ui_key == KEY_ENTER) {
            script_editor_level_item_count_inputbox.clear_focus();
            return true;
        }
        return true;
    }
    return false;
}

// Deletes the level item object of record `index` (the first type 3 object with that level_items index).
#define SE_DELETE_LEVEL_ITEM_OBJECT(index)                                                   \
    do {                                                                                     \
        obj *o_;                                                                             \
        for (o_ = obj_active_list.next; o_ != &obj_active_list; o_ = o_->next) {             \
            if (o_->type == OBJ_TYPE_ITEM && ((item_obj *)o_)->level_item == (index)) {      \
                o_->deinit();                                                                \
                break;                                                                       \
            }                                                                                \
        }                                                                                    \
    } while (0)

// 0x4ecbe0  Confidence: likely (the name search loop; the dirty flag's form). OK of the level item window
// for a level navpoint: an item and count change the navpoint's level item (its object respawned), a
// new one is added ("<level>-000", the number bumped to one past the index of a clash until it is free;
// 32 at most), an empty choice or count 0 removes it. The level_items table is marked for saving.
// Game bug: removing a record moves the later records down without fixing the indices that point at
// them (navpoint::level_item, the spawned objects' level_item), so later edits act on the wrong record.
bool script_editor_level_item_ok(int event, int value)
{
    bool changed;
    navpoint *np;
    level_item_rec *li;
    ui_listbox_entry *e;
    vector3 pos;
    matrix orient;
    int index, count, i;

    changed = false;
    if (value != script_editor_level_item_ok_button.id || event != UI_EVENT_PRESSED)
        return false;
    np = (navpoint *)script_editor_sel.object;
    index = np->level_item;
    li = index >= 0 ? &level_items[index] : NULL;
    e = script_editor_level_item_listbox.get_selected();
    count = atoi(script_editor_level_item_count_inputbox.get_text());
    if (e != NULL && count > 0) {
        if (li != NULL) {
            if (li->type != &item_types[e->data]) {
                li->type = &item_types[e->data];
                changed = true;
            }
            if (li->count != count) {
                li->count = count;
                changed = true;
            } else if (!changed) {
                goto done;
            }
            SE_DELETE_LEVEL_ITEM_OBJECT(index);
            navpoint_get_position(li->navpoint->name, &pos, &orient);
            level_item_spawn(item_create_by_name(li->type->name), &pos, &orient, li->count, index, -1);
        } else if (level_item_count < MAX_LEVEL_ITEMS) {
            li = &level_items[level_item_count];
            sprintf(li->name, "%s-000", level_script_table[cur_level_index].name);
            for (i = 0; i < level_item_count; i++) {
                if (strcmp(li->name, level_items[i].name) == 0) {
                    sprintf(li->name, "%s-%.3d", level_script_table[cur_level_index].name, i + 1);
                    i = -1;
                }
            }
            strcpy(li->script, level_script_name);
            li->count = count;
            li->type = &item_types[e->data];
            li->navpoint = (navpoint *)script_editor_sel.object;
            li->flag_level = -1;
            li->flag[0] = 0;
            ((navpoint *)script_editor_sel.object)->level_item = level_item_count;
            navpoint_get_position(li->navpoint->name, &pos, &orient);
            level_item_spawn(item_create_by_name(li->type->name), &pos, &orient, li->count, level_item_count, -1);
            level_item_count++;
            savegame_state.level_item_add(li->name);
            changed = true;
        }
    } else if (li != NULL) {
        savegame_state.level_item_remove(li->name);
        SE_DELETE_LEVEL_ITEM_OBJECT(index);
        i = level_item_count - index - 1;
        if (i > 0)
            memmove(li, li + 1, i * sizeof(level_item_rec));
        level_item_count--;
        ((navpoint *)script_editor_sel.object)->level_item = -1;
        changed = true;
    }
done:
    if (script_level_items_dirty || changed)
        script_level_items_dirty = true;
    else
        script_level_items_dirty = false;
    script_editor_level_item_window.set_disabled(true);
    return true;
}

// 0x4ecf30  Confidence: exact.
bool script_editor_level_item_cancel_cb(int event, int value)
{
    if (value == script_editor_level_item_cancel_button.id && event == UI_EVENT_PRESSED) {
        script_editor_level_item_window.set_disabled(true);
        return true;
    }
    return false;
}

// 0x4ecf60  static_init_ccmd_render_s3d_navpoints (entry 473, jmp). Confidence: exact (compiler-generated).
// 0x4ecf70  init_ccmd_render_s3d_navpoints. Confidence: exact (compiler-generated).
console_command ccmd_render_s3d_navpoints("render_s3d_navpoints", "Toggles Render_s3d_navpoints",
                                          cmd_render_s3d_navpoints);   // 0x2549d30

// 0x4ecf90  Confidence: exact (the console's bool command macro).
void cmd_render_s3d_navpoints()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Render_s3d_navpoints = true;
        else if (arg_type & ARG_FALSE)
            Render_s3d_navpoints = false;
        else if (arg_type & ARG_NONE)
            Render_s3d_navpoints ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "render_s3d_navpoints", "Render_s3d_navpoints");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "render_s3d_navpoints", Render_s3d_navpoints ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}
