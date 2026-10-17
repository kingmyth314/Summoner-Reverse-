// unnamed_states_0x431870.cpp - Sum.exe v1.40, code 0x431870-0x4323da: the boot, title and load-game states
// (gameseq_state_table entries 1, 2 and 8), the build stamp screen, game_change_level and the us_open_demo
// console command.
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl section, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
//
// Unit range: static_initializers entry 94 (0x431870) to state_loadgame_leave (0x4323d0); the frame-end unit
// starts with entry 100 at 0x4323e0.
// Data: .data 0x5930b8-0x5931af: three initialised objects (0x5930b8-0x5930bd), then the literal pool
// ("masad" 0x5930c0 .. "Load" 0x5931a8). The first object follows the editor dialog unit's literals, and
// the frame-end unit's first initialised object (Gamewide_show_net_stats, 0x5931b0) follows "Load": VC6
// puts a unit's initialised data before its literals, so both are unit boundaries. Pooled with earlier
// units (/Gf): "Title" (0x58a144), "Cancel" (0x5930ac, the editor dialogs), the console strings
// (0x588c34-0x588c50).
// .bss 0x5fa2c8-0x5fbf1a: the load-game window and gadgets, ccmd_us_open_demo (0x5fade0), three flags.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "vsdk/os/console.h"
#include "vsdk/os/key.h"
#include "vsdk/vfile/file.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/gr_font.h"
#include "vsdk/ui/ui.h"
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/ui/window.h"
#include "engine/objects/objects.h"
#include "engine/gamesound/gamesound.h"
#include "levelscripts/level_scripts.h"
#include "levelscripts/conversation.h"
#include "levelscripts/savegame.h"
#include "summoner/summoner.h"
#include "summoner/interface/interface.h"

#define STATE_BOOT          1
#define STATE_GAME          4
#define STATE_MP_SCREEN_18  14     // gameseq_state_table entry 14 (main-loop.md "Multiplayer screen 18")
#define SCREEN_MAIN_MENU    14
#define LEVEL_SCRIPTS_PER_LEVEL 7  // conversation_save_table: 7 slots of 4 pointers per level
#define LEVEL_MASAD         3      // level_script_table index of "masad" (mainmenu_start_new_game)
#define BUILD_STAMP_NONE    3      // build_stamp_check: not stamped (every retail exe)

// conversation_save_table slot name (the first pointer of each 0x10-byte record).
#define LEVEL_SCRIPT_SLOT_NAME(level, slot) \
    ((char *)conversation_save_table[((level) * LEVEL_SCRIPTS_PER_LEVEL + (slot)) * 4])

// ---- .data (0x5930b8) ----------------------------------------------------------------------------------
int  build_stamp_pending  = 1;        // 0x5930b8
bool states_flag_5930bc   = true;     // 0x5930bc (no reference)
bool play_intro_movies    = true;     // 0x5930bd (read at 0x41d8b0 / 0x41d8f0)

// ---- .bss ----------------------------------------------------------------------------------------------
// The load-game screen (state 8). Each object is a static initialiser (sinit_: call init_, then jmp to
// register_dtor_) and an atexit destructor (register_dtor_: atexit(dtor_); dtor_: the destructor).
// 0x431870 sinit_ui_window_5fb110 (static_initializers entry 94). Confidence: exact (compiler-generated).
// 0x431880 init_ui_window_5fb110 (ui_window()). Confidence: exact.
// 0x431890 register_dtor_ui_window_5fb110 (atexit). Confidence: exact.
// 0x4318a0 dtor_ui_window_5fb110 (~ui_window, jmp 0x52b0a0). Confidence: exact.
ui_window loadgame_window;            // 0x5fb110
// A second window that nothing else uses; the object's purpose is not known (name ours).
// 0x4318b0 sinit_ui_window_5fb600 (entry 95). Confidence: exact (compiler-generated).
// 0x4318c0 init_ui_window_5fb600. Confidence: exact.
// 0x4318d0 register_dtor_ui_window_5fb600. Confidence: exact.
// 0x4318e0 dtor_ui_window_5fb600. Confidence: exact.
ui_window loadgame_window_unused;     // 0x5fb600
// ONE initialiser constructs both list boxes (0x5faac0, then 0x5fadf0) and one atexit handler destroys them
// in reverse, so they are one declaration (as player.cpp's party_list, player_free_list).
// 0x4318f0 sinit_ui_listboxes_5faac0 (entry 96). Confidence: likely (one dynamic initialiser per declaration).
// 0x431900 init_ui_listboxes_5faac0. Confidence: likely.
// 0x431920 register_dtor_ui_listboxes_5faac0. Confidence: likely.
// 0x431930 dtor_ui_listboxes_5faac0 (0x5fadf0 first). Confidence: likely.
// 0x431950  ui_listbox::~ui_listbox, implicit: ~ui_listbox_entry (0x52d560) on +0x2d4, then ~ui_gadget;
// the COMDAT is emitted after its first use, dtor_ui_listboxes_5faac0. Confidence: exact (compiler-generated, D3100).
ui_listbox loadgame_level_listbox, loadgame_save_listbox;     // 0x5faac0, 0x5fadf0
// The destructor is the COMDAT 0x431780 of the editor dialog unit.
// 0x4319a0 sinit_ui_static_5fa2c8 (entry 97). Confidence: exact (compiler-generated).
// 0x4319b0 init_ui_static_5fa2c8. Confidence: exact.
// 0x4319c0 register_dtor_ui_static_5fa2c8. Confidence: exact.
// 0x4319d0 dtor_ui_static_5fa2c8. Confidence: exact.
ui_static loadgame_title_static;      // 0x5fa2c8 created, never given a text
// One declaration, as the list boxes (0x5fbaf0 first).
// 0x4319e0 sinit_ui_buttons_5fbaf0 (entry 98). Confidence: likely (one declaration).
// 0x4319f0 init_ui_buttons_5fbaf0. Confidence: likely.
// 0x431a10 register_dtor_ui_buttons_5fbaf0. Confidence: likely.
// 0x431a20 dtor_ui_buttons_5fbaf0 (0x5fa698 first). Confidence: likely.
ui_button loadgame_load_button, loadgame_cancel_button;       // 0x5fbaf0, 0x5fa698

bool game_change_level_early;         // 0x5fbf18
bool cheat_gold_5fbf19;               // 0x5fbf19 nothing writes it (state_game_enter, lsf_catacombs_boss_dead read it)
bool US_open_demo;                    // 0x5fbf1a

// ---- code ----------------------------------------------------------------------------------------------

// The level's save slots into the second list box, the first one selected. Written out three times in
// the binary (state_loadgame_enter twice, loadgame_level_list_cb), so an inline function (/Ob1 inlines
// what is marked inline). Name ours. Confidence: likely.
static inline void loadgame_fill_save_list(const char *level_name)
{
    int level = level_script_find(level_name);
    int i;

    loadgame_save_listbox.clear();
    if (level == -1)
        return;
    for (i = 0; i < LEVEL_SCRIPTS_PER_LEVEL; i++) {
        if (LEVEL_SCRIPT_SLOT_NAME(level, i) == NULL)
            break;
        loadgame_save_listbox.add_entry(LEVEL_SCRIPT_SLOT_NAME(level, i), font_small, 0, 0, 0);
        if (i == 0)
            loadgame_save_listbox.select_by_text(LEVEL_SCRIPT_SLOT_NAME(level, i), true);
    }
}

// 0x431a40  Confidence: exact (VC6 byte match, match24; was likely) (the three copies into level_script_name: the binary has two inline strcpy
// expansions of level_filename, one shared with the other sources). (level file name, script name or NULL
// = the first conversation_save_table slot of the level, or the level name when it has none / is not in
// level_script_table). The second parameter stays `void *` as summoner.h declares it (D3105).
void game_change_level(const char *filename, void *arg)
{
    const char *script = (const char *)arg;
    int level = level_script_find(filename);
    int i;

    level_prev_filename[0] = 0;
    level_entry_point = 1;
    strcpy(level_filename, filename);
    if (script) {
        strcpy(level_script_name, script);
    } else if (level != -1) {
        for (i = 0; i < LEVEL_SCRIPTS_PER_LEVEL; i++) {
            if (LEVEL_SCRIPT_SLOT_NAME(level, i))
                break;
        }
        if (i < LEVEL_SCRIPTS_PER_LEVEL)
            strcpy(level_script_name, LEVEL_SCRIPT_SLOT_NAME(level, i));
        else
            strcpy(level_script_name, level_filename);
    } else {
        strcpy(level_script_name, level_filename);
    }

    // A pending savegame, or a new game's first script, counts as "entered by load" (the tutorial runs).
    if (savegame_state.pending_load_length() || (level == LEVEL_MASAD && !_stricmp(level_script_name, "masad")))
        level_entered_by_load = true;
    else
        level_entered_by_load = false;
    gameseq_set_state(2);
    if (gameseq_get_state() == 0)
        game_change_level_early = true;
}

// 0x431b70  init_ccmd_us_open_demo_thunk (static_initializers entry 99, jmp). Confidence: exact (compiler-generated).
// 0x431b80  init_ccmd_us_open_demo: console_command::console_command. Confidence: exact (compiler-generated).
console_command ccmd_us_open_demo("us_open_demo", "Toggles US_open_demo", cmd_us_open_demo);   // 0x5fade0

// 0x431ba0  Confidence: exact (the console's bool command macro).
void cmd_us_open_demo()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            US_open_demo = true;
        else if (arg_type & ARG_FALSE)
            US_open_demo = false;
        else if (arg_type & ARG_NONE)
            US_open_demo ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "us_open_demo", "US_open_demo");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "us_open_demo", US_open_demo ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x431c60  Confidence: exact. State 1 enter: back to the main menu, single player.
void state_boot_enter()
{
    gameseq_get_prev_state();         // result unused
    game_flags = GF_SINGLE_PLAYER;
    build_stamp_show();
    cursor_set_type(cursor_walk_bm);
    input_flush();
    level_prev_filename[0] = 0;
    level_entry_point = 1;
    interface_set_screen(SCREEN_MAIN_MENU, 0);
}

// 0x431cb0  Confidence: exact. Once: the recipient of a stamped build, or "expired" and exit(1). Every
// retail exe is unstamped (build_stamp_check returns 3), so only the flag is cleared.
void build_stamp_show()
{
    char name[512];
    int status;

    if (!build_stamp_pending)
        return;
    status = build_stamp_check();
    if (status != BUILD_STAMP_NONE) {
        build_stamp_get_name(name, sizeof(name));
        do {
            gr_clear();
            gr_set_color(255, 255, 255, 127);
            if (status == 0) {
                gr_string(GR_CENTER_X, 100, "This version of Summoner has been built for:", font_small, gr_string_mode);
                gr_string(GR_CENTER_X, 150, name, font_small, gr_string_mode);
                gr_string(GR_CENTER_X, 400, "Press any key to continue.", font_small, gr_string_mode);
            } else {
                gr_string(GR_CENTER_X, 150, "This version of Summoner has expired.", font_small, gr_string_mode);
                gr_string(GR_CENTER_X, 400, "Press any key to exit the game.", font_small, gr_string_mode);
            }
            gr_flip();
        } while (!key_inkey());
        if (status)
            exit(1);
    }
    build_stamp_pending = 0;
}

// 0x431df0  Confidence: exact. State 1 leave.
void state_boot_leave()
{
    cursor_visible = false;
}

// 0x431e00  Confidence: exact. State 1 frame.
void state_boot_frame()
{
    cursor_visible = true;
    interface_frame(PollInputEvents());
    interface_draw();
}

// 0x431e20  Confidence: likely (the flag expression: sete / inc for the ?: is VC6's usual code). State 2
// enter, on every game_change_level: new objects, party inventory and item types; multiplayer when the
// previous state was 14.
void state_title_enter()
{
    int prev = gameseq_get_prev_state();

    music_play(music_find_by_name("Title"), true);
    objects_reset();
    party_inventory_reset();
    items_reset_for_new_game();
    game_flags = (game_flags & 0x1c) | (prev == STATE_MP_SCREEN_18 ? GF_MULTIPLAYER : GF_SINGLE_PLAYER);
}

// 0x431e70  Confidence: exact. State 2 frame.
void state_title_frame()
{
    gameseq_set_state(STATE_GAME);
}

// The four uses of the level name pass it to a bare ret (0x40acc0, the folded copy in new_gamelist.cpp's
// range; D3102). No address of its own: identical-code folding keeps one bare ret. Name ours.
// Confidence: unsure (whether and where the source defined it; a compiled-out debug print is likely).
void loadgame_debug_hook(const char *level)
{
}

// 0x431e80  Confidence: likely (the order of the font / callback stores). State 8 enter (never entered in
// v1.40): the debug "Load" screen, a list of the *.s3d levels (or all level_script_table names when there
// are none, with "catacombs" selected) and the selected level's save slots.
void state_loadgame_enter()
{
    file_find_node list;
    file_find_node *node;
    ui_listbox_entry *e;
    int count, i;

    loadgame_window.init(0, 0, gr_screen_width(), gr_screen_height(), -1, UI_WINDOW_ALLOW_WRAP);
    loadgame_load_button.create(&loadgame_window, 240, 420, 50, 25, 0, 0, false);
    loadgame_load_button.set_text("Load");
    loadgame_load_button.callback = loadgame_load_cb;
    loadgame_load_button.font = font_small;
    loadgame_cancel_button.create(&loadgame_window, 350, 420, 50, 25, 1, 0, false);
    loadgame_cancel_button.set_text("Cancel");
    loadgame_cancel_button.font = font_small;
    loadgame_cancel_button.callback = loadgame_load_cb;
    loadgame_level_listbox.create(&loadgame_window, 170, 70, 200, 300, 200, 0, 0, 0x42, false);
    loadgame_level_listbox.margin = 2;
    loadgame_level_listbox.v_margin = 2;
    loadgame_level_listbox.callback = loadgame_level_list_cb;
    loadgame_level_listbox.set_focus();
    loadgame_save_listbox.create(&loadgame_window, 400, 70, 150, 100, 150, 0, 0, 0x42, false);
    loadgame_save_listbox.margin = 2;
    loadgame_save_listbox.v_margin = 2;
    loadgame_save_listbox.callback = loadgame_list_dblclick_cb;
    loadgame_title_static.create(&loadgame_window, 220, 390, 300, 30, -1, false);

    file_find_files(&list, path_models_levels, "*.s3d", 1, 0);
    count = 0;
    for (node = list.next; node != (file_find_node *)&list; node = node->next) {
        e = loadgame_level_listbox.add_entry(node->name, font_small, 0, 0, 0);
        if (count == 0) {
            loadgame_level_listbox.select_entry(e, true);
            loadgame_debug_hook(e->text);
            loadgame_fill_save_list(e->text);
        }
        count++;
    }
    while (list.next != (file_find_node *)&list) {
        list.prev = list.next;
        list.next = list.next->next;
        delete list.prev;
    }
    list.next = list.prev = (file_find_node *)&list;

    if (count == 0) {
        for (i = 0; i < NUM_LEVEL_SCRIPTS; i++)
            loadgame_level_listbox.add_entry(level_script_table[i].name, font_small, 0, 0, 0);
        e = loadgame_level_listbox.find_by_text("catacombs");
        if (e) {
            loadgame_level_listbox.select_entry(e, true);
            loadgame_debug_hook(e->text);
            loadgame_fill_save_list(e->text);
        }
    }
    cursor_visible = true;
}

// 0x432200  Confidence: likely (the shape around the null check). The level list box's callback: a new
// selection refills the save slots, a double click presses Load. Game bug (state 8 only): the selected
// entry is checked for NULL before the debug hook but dereferenced unchecked for the refill (0x43222c).
bool loadgame_level_list_cb(int event, int value)
{
    if (event == UI_EVENT_LIST_SELECT) {
        ui_listbox_entry *e = loadgame_level_listbox.get_selected();
        if (e)
            loadgame_debug_hook(e->text);
        loadgame_fill_save_list(e->text);
        return true;
    }
    if (event == UI_EVENT_LIST_DBLCLICK) {
        loadgame_load_button.press();
        return true;
    }
    return false;
}

// 0x4322c0  Confidence: exact. The save list box's callback.
bool loadgame_list_dblclick_cb(int event, int value)
{
    if (event == UI_EVENT_LIST_DBLCLICK) {
        loadgame_load_button.press();
        return true;
    }
    return false;
}

// 0x4322e0  Confidence: exact. Both buttons' callback: Load (id 0) changes to the selected level and slot,
// Cancel (id 1) goes back to state 1.
bool loadgame_load_cb(int event, int value)
{
    if (event == UI_EVENT_PRESSED) {
        if (value == 0) {
            ui_listbox_entry *level = loadgame_level_listbox.get_selected();
            ui_listbox_entry *save = loadgame_save_listbox.get_selected();
            if (level) {
                loadgame_debug_hook(level->text);
                gr_set_vsync(false);
                game_change_level(level->text, save ? save->text : NULL);
            }
        } else if (value == 1) {
            gameseq_set_state(STATE_BOOT);
        }
        return true;
    }
    return false;
}

// 0x432370  Confidence: exact. State 8 frame: Esc back to state 1, Enter presses Load.
void state_loadgame_frame()
{
    int key = PollInputEvents();

    if (key == KEY_ESC)
        gameseq_set_state(STATE_BOOT);
    else if (key == KEY_ENTER)
        loadgame_load_button.press();
    else
        ui_process(key, 1);
    gr_clear();
    ui_draw(NULL);
}

// 0x4323d0  Confidence: exact. State 8 leave: the window's destroy (slot 0, called directly on the object).
void state_loadgame_leave()
{
    loadgame_window.dtor();
}
