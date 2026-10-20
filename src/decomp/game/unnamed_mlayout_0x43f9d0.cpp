// unnamed_mlayout_0x43f9d0.cpp - Sum.exe v1.40, code 0x43f9d0-0x4403f2: game state 6 (a small character
// picker: one button showing a char_infos name that opens a list box of all of them) and the monster
// layout files (.mlo: the living entities of the level with their positions, the load_mlayout /
// save_mlayout console commands).
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl block, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .data 0x594ff8-0x595139 (literals only; "" 0x5bd0d0 and " " 0x58e0cc are pooled copies, /Gf),
// .bss 0x60f1d0-0x60fe27. The cut from the lobby unit before it rests on the static initialiser run that
// starts at 0x43f9d0 and the 8-byte .bss gap 0x60f1c8-0x60f1cf (no data variable marks it); see NOTES.md.

#include <string.h>
#include <stdio.h>
#include "vsdk/vfile/file.h"
#include "vsdk/os/console.h"
#include "vsdk/os/timer.h"
#include "vsdk/ui/ui.h"
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/ui/window.h"
#include "vsdk/ca/character.h"
#include "vsdk/ca/character_instance.h"
#include "engine/characterinfo/characterinfo.h"
#include "engine/objects/living_entity.h"
#include "engine/ai/ai.h"
#include "summoner/summoner.h"
#include "summoner/gameplay.h"
#include "summoner/unnamed_mlayout_0x43f9d0.h"
#include "summoner/multi/multi.h"

#define MLO_VERSION        1
#define MLO_NAME_LEN       0x100
#define MLO_CMD_LINE_LEN   0x200
// Living entity types (obj::type) a layout keeps; 7 (the party members) are only moved. Names ours.
#define MLO_TYPE_FIRST     4
#define MLO_TYPE_SECOND    5
#define MLO_TYPE_PARTY     7
#define MLO_TYPE_LAST      9

#define STATE6_BUTTON_ID   200
#define STATE6_LISTBOX_ID  100
#define STATE6_LISTBOX_FLAGS 0x642

// ---- .bss (0x60f1d0) -----------------------------------------------------------------------------------
// The objects with constructors, in the order of their static initialisers (entries 148-150; the two
// command objects, entries 151 / 152, are defined at their initialisers further down). Their .bss order is
// listbox 0x60f1d0, ccmd_save_mlayout 0x60f4f0, button 0x60f500, ccmd_load_mlayout 0x60f928, window
// 0x60f938, state6_selection 0x60fe24.
// 0x43f9d0  sinit_ui_window_60f938 (call init, jmp register; static_initializers entry 148). Confidence:
// exact (compiler-generated).
// 0x43f9e0  init_ui_window_60f938 (mov ecx, 0x60f938; jmp ui_window::ui_window). Confidence: exact
// (compiler-generated).
// 0x43f9f0  register_dtor_ui_window_60f938 (atexit(0x43fa00)). Confidence: exact (compiler-generated).
// 0x43fa00  dtor_ui_window_60f938 (mov ecx, 0x60f938; jmp ui_window::~ui_window). Confidence: exact
// (compiler-generated).
ui_window state6_window;                                      // 0x60f938
// 0x43fa10  sinit_ui_button_60f500 (entry 149). Confidence: exact (compiler-generated).
// 0x43fa20  init_ui_button_60f500 (jmp ui_button::ui_button). Confidence: exact (compiler-generated).
// 0x43fa30  register_dtor_ui_button_60f500 (atexit(0x43fa40)). Confidence: exact (compiler-generated).
// 0x43fa40  dtor_ui_button_60f500 (jmp 0x4317e0, the implicit ~ui_button the linker kept, D3100).
// Confidence: exact (compiler-generated).
ui_button state6_button;                                      // 0x60f500
// 0x43fa50  sinit_ui_listbox_60f1d0 (entry 150). Confidence: exact (compiler-generated).
// 0x43fa60  init_ui_listbox_60f1d0 (jmp ui_listbox::ui_listbox). Confidence: exact (compiler-generated).
// 0x43fa70  register_dtor_ui_listbox_60f1d0 (atexit(0x43fa80)). Confidence: exact (compiler-generated).
// 0x43fa80  dtor_ui_listbox_60f1d0 (jmp 0x431950, the implicit ~ui_listbox, D3100). Confidence: exact
// (compiler-generated).
ui_listbox state6_listbox;                                    // 0x60f1d0
int state6_selection;                                         // 0x60fe24

// 0x43fa90  Confidence: exact. The button opens the list (window 100 x 400) on the current entry.
bool state6_button_cb(int event, int value)
{
    if (event == UI_EVENT_PRESSED) {
        state6_button.set_disabled(true);
        state6_listbox.set_enabled(true);
        state6_listbox.select_by_data(state6_selection, true);
        state6_listbox.set_focus();
        state6_window.set_size(100, 400);
        return true;
    }
    return false;
}

// 0x43faf0  Confidence: exact. A double click picks the entry, closes the list (window 100 x 15) and
// shows its name on the button.
bool state6_listbox_cb(int event, int value)
{
    if (event == UI_EVENT_LIST_DBLCLICK) {
        state6_selection = state6_listbox.get_selected()->data;
        state6_listbox.set_disabled(true);
        state6_button.set_enabled(true);
        state6_window.set_size(100, 15);
        state6_button.set_text(char_infos[state6_selection].name);
        return true;
    }
    return false;
}

// 0x43fb60  Confidence: exact (VC6 8966 byte match with first set to NULL just before the loop, m1019) (the first entry is kept in a register: `if (!first) first = e`). The list
// gets every char_infos name with its index as the data; it starts disabled (hidden behind the button).
void state6_enter()
{
    ui_listbox_entry *first;
    ui_listbox_entry *e;
    int i;

    ui_push_group();
    state6_window.init(0, 0, 100, 15, UI_NO_ID, 9);
    state6_button.create(&state6_window, 0, 0, 100, 15, STATE6_BUTTON_ID, 0, false);
    state6_button.set_text(char_infos[state6_selection].name);
    state6_button.set_hotkey(15, 0);
    state6_button.callback = state6_button_cb;
    state6_listbox.create(&state6_window, 0, 0, 100, 400, 0, 0, STATE6_LISTBOX_ID, STATE6_LISTBOX_FLAGS, false);
    state6_listbox.callback = state6_listbox_cb;
    first = NULL;
    for (i = 0; i < num_char_infos; i++) {
        e = state6_listbox.add_entry(char_infos[i].name, -1, 0, 0, 0);
        e->data = i;
        if (!first)
            first = e;
    }
    state6_listbox.select_entry(first, true);
    state6_listbox.set_disabled(true);
}

// 0x43fc70  Confidence: exact (ui_window__destroy is the window's vtable slot 0, called directly).
void state6_leave()
{
    state6_window.dtor();
    ui_pop_group();
}

// 0x43fc80  Confidence: exact.
void state6_frame()
{
    timer_wait(10);
}

// 0x43fc90  Confidence: exact (VC6 byte match, match24; was likely) (the switch on the type is a jump table 4..9 with 6 and 8 skipped). File:
// "" (an empty header string), version 1, the level file name, then per living entity of types 4, 5, 7 and
// 9 (type, the character name, position, orientation), ended by a 0 type.
int monster_layout_save(const char *filename)
{
    File f;
    Character *c;

    if (f.open(filename, 2, path_internal_copy))
        return -1;
    f.write_string("", -1);
    f.write_int(MLO_VERSION);
    f.write_string(level_filename, -1);
    for (c = living_list.live_next; c != &living_list; c = c->live_next) {
        switch (c->type) {
        case MLO_TYPE_FIRST:
        case MLO_TYPE_SECOND:
        case MLO_TYPE_PARTY:
        case MLO_TYPE_LAST:
            f.write_int(c->type);
            f.write_string(c->instance->character->name, -1);
            f.write_vector(&c->pos);
            f.write_matrix(&c->orient);
            break;
        }
    }
    f.write_int(0);
    return f.close();
}

// 0x43fe00  Confidence: likely (the char_infos lookup of the map name is the order of the code: its result
// stays in EBP for every living_entity_create; the source position of the call is a guess). An empty first
// string is the versioned header ("obsolete" when check_version fails); an older file starts with the map
// name directly. Every entity of types 4, 5 and 9 is deleted, the host tells the clients (NEW_MON), then
// each record creates its entity; a type 7 record only moves the party member of that character name.
// Game bug (D3308): every created entity gets char_info = charinfo_find_by_name(map name), not the
// record's name (read into the same buffer later and used only for type 7).
int monster_layout_load(const char *filename)
{
    File f;
    char name[MLO_NAME_LEN];
    vector3 pos;
    matrix orient;
    Character *c;
    Character *next;
    int char_info;
    int type;

    if (f.open(filename, 1, path_internal_copy))
        return -1;
    f.read_string_buf(name, MLO_NAME_LEN, 0, NULL);
    if (strlen(name) == 0) {
        f.set_version(f.read_int(0, 0));
        if (!f.check_version(0)) {
            print_to_console("This monster layout is obsolete\n", 0);
            return -1;
        }
        f.read_string_buf(name, MLO_NAME_LEN, 0, NULL);
    }
    char_info = charinfo_find_by_name(name);
    if (strcmp(level_filename, name) != 0) {
        sprintf(string_buffer, "This monster layout is for map \"%s\"\n", name);
        print_to_console(string_buffer, 0);
        return -1;
    }
    for (c = living_list.live_next; c != &living_list; c = next) {
        next = c->live_next;
        if ((c->type >= MLO_TYPE_FIRST && c->type <= MLO_TYPE_SECOND) || c->type == MLO_TYPE_LAST)
            delete c;
    }
    if (MP_IS_HOST())
        mp_broadcast_packet_3e();
    while ((type = f.read_int(0, 0)) != 0) {
        f.read_string_buf(name, MLO_NAME_LEN, 0, NULL);
        f.read_vector(&pos, 0, &file_zero_vector);
        f.read_matrix(&orient, 0, &file_default_matrix);
        if (type == MLO_TYPE_PARTY) {
            for (c = living_list.live_next; c != &living_list; c = c->live_next) {
                if (c->type == MLO_TYPE_PARTY && _stricmp(c->instance->character->name, name) == 0) {
                    c->pos = pos;
                    c->orient = orient;
                    c->ai.reset();
                    break;
                }
            }
        } else {
            c = living_entity_create(type, char_info, &pos, &orient, true, 0, 0, 0, false);
            if (game_flags & GF_MULTIPLAYER)
                mp_send_new_lep(c);
        }
    }
    f.close();
    return 0;
}

// 0x440170  init_ccmd_load_mlayout_thunk (static_initializers entry 151, jmp). Confidence: exact
// (compiler-generated).
// 0x440180  init_ccmd_load_mlayout: console_command::console_command("load_mlayout", ...). Confidence:
// exact (compiler-generated).
console_command ccmd_load_mlayout("load_mlayout", "Load a monster layout file", cmd_load_mlayout);   // 0x60f928

// 0x4401a0  Confidence: exact (VC6 byte match, match24; was likely) (the words are joined with inline strcat; process_command(3) reads the next
// word). The file name may have spaces: every word is appended with a space and the last space is cut
// (with no argument, filename[-1] is cleared: one byte below the buffer). A failed load returns before
// the help text; a good one pauses the game.
void cmd_load_mlayout()
{
    char filename[MLO_CMD_LINE_LEN];

    filename[0] = 0;
    if (calling_a_command_function) {
        process_command(ARG_STRING);
        while (!(arg_type & ARG_NONE)) {
            strcat(filename, next_arg);
            strcat(filename, " ");
            process_command(ARG_NONE | ARG_STRING);
        }
        filename[strlen(filename) - 1] = 0;
        file_change_ext(filename, filename, ".mlo");
        if (monster_layout_load(filename) < 0) {
            print_to_console("Unable to load that monster layout file\n", 0);
            return;
        }
        game_pause(0, 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: load_mlayout <string>\n", 0);
}

// 0x4402c0  init_ccmd_save_mlayout_thunk (static_initializers entry 152, jmp). Confidence: exact
// (compiler-generated).
// 0x4402d0  init_ccmd_save_mlayout: console_command::console_command("save_mlayout", ...). Confidence:
// exact (compiler-generated).
console_command ccmd_save_mlayout("save_mlayout", "Save a monster layout file", cmd_save_mlayout);   // 0x60f4f0

// 0x4402f0  Confidence: exact (VC6 byte match, match24; was likely) (as cmd_load_mlayout; a failed save falls through to the help test).
void cmd_save_mlayout()
{
    char filename[MLO_CMD_LINE_LEN];

    filename[0] = 0;
    if (calling_a_command_function) {
        process_command(ARG_STRING);
        while (!(arg_type & ARG_NONE)) {
            strcat(filename, next_arg);
            strcat(filename, " ");
            process_command(ARG_NONE | ARG_STRING);
        }
        filename[strlen(filename) - 1] = 0;
        file_change_ext(filename, filename, ".mlo");
        if (monster_layout_save(filename) < 0)
            print_to_console("Unable to save that monster layout file\n", 0);
    }
    if (doing_help_for_comand)
        print_to_console("Usage: save_mlayout <string>\n", 0);
}
