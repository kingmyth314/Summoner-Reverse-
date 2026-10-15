// summoner/unnamed_script_editor_0x4de0b0.h - the in-game level script editor (code 0x4de0b0-0x4ed04f, .data
// 0x5a08a8-0x5a1573, .bss 0x2546828-0x256b74f; source game/unnamed_script_editor_0x4de0b0.cpp, decomp55).
// THE FILE NAME IS A PLACEHOLDER: no path string, no strings.tbl block, sum.symbols module "-"; the
// directory is ours too. Not original source. Written from Sum.exe's disassembly.
// script_editor_frame and script_editor_no_window_open are declared in summoner/summoner.h.
#ifndef SUMMONER_UNNAMED_SCRIPT_EDITOR_0X4DE0B0_H
#define SUMMONER_UNNAMED_SCRIPT_EDITOR_0X4DE0B0_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/gr/gr.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/norlsallocpool.h"
#include "vsdk/os/console.h"
#include "vsdk/ui/ui.h"
#include "vsdk/ui/ui_gadgets.h"
#include "vsdk/ui/ui_container.h"
#include "vsdk/ui/window.h"
#include "vsdk/math/ix.h"

class Character;
class File;
struct navpoint;
struct script_action;
struct script_object;
struct lobj;
struct door_info;

// The editor modes (script_editor_mode, script_editor_mode_names; names ours).
#define SE_MODE_CREATURE      0
#define SE_MODE_NAVPOINT      1
#define SE_MODE_CAMERA        2
#define SE_MODE_AMBIENT       3
#define SE_MODE_DOOR          4
#define SE_MODE_CLICK_OBJECT  5
#define SE_NUM_MODES          6

// What the mouse is over / what is selected (script_editor_pick::kind; names ours).
#define SE_PICK_NONE       0
#define SE_PICK_CHARACTER  1      // object: Character
#define SE_PICK_NAVPOINT   2      // object: navpoint (level or cutscene table)
#define SE_PICK_DOOR       3      // object: lobj with lobj flag 0x100
#define SE_PICK_OBJECT     4      // object: lobj (a clickable object, lc_entries)

// The object under the mouse and the selection (0x44 bytes each; type name ours, D3500). sum.symbols names
// the fields as separate globals (script_editor_hover_kind 0x2546bd0 .. script_editor_hover_object
// 0x2546c10, script_editor_sel_kind 0x255a008 .. script_editor_sel_object 0x255a048); they are one object
// each: the handlers copy the whole hover record into the selection in member order (kind, size, pos,
// orient, object), as an implicit copy assignment does.
struct script_editor_pick {
    int kind;                 // +0x00 SE_PICK_*
    vector3 size;             // +0x04 box size (characters: script_editor_char_sizes; navpoints 0.8, 0.4, 0.8)
    vector3 pos;              // +0x10
    matrix orient;            // +0x1c
    void *object;             // +0x40
};
LAYOUT_CHECK(script_editor_pick, sizeof(script_editor_pick) == 0x44);

// Per action type defaults (12 bytes; script_action_names order, 33 records at 0x2556ab0). The type name
// is from sum.symbols' script_action_defaults__init (0x4de410), the out-of-line copy of the inline
// constructor; field names ours (script_create copies arg0c / arg10 into a new action, needs_file picks
// script_make_filename, has_params opens the parameter window; D3501).
struct script_action_defaults {
    script_action_defaults(int arg0c, int arg10, bool flag14, bool needs_nav, bool has_params)   // 0x4de410
    {
        this->arg0c = arg0c;
        this->arg10 = arg10;
        this->flag14 = flag14;
        this->needs_nav = needs_nav;
        this->has_params = has_params;
    }
    int arg0c;                // +0x0 the new action's arg0c
    int arg10;                // +0x4 the new action's arg10
    bool flag14;              // +0x8 the new action's flag14 (wander: walk)
    bool needs_nav;           // +0x9 the action takes a navpoint
    bool has_params;          // +0xa the action has parameters to edit
};
LAYOUT_CHECK(script_action_defaults, sizeof(script_action_defaults) == 0xc);

// ---- .data ----------------------------------------------------------------------------------------
extern const char *script_editor_mode_names[SE_NUM_MODES];   // 0x5a08a8 "Creature" .. "Click object"

// ---- .bss -----------------------------------------------------------------------------------------
extern script_action_defaults script_action_default_table[33];   // 0x2556ab0 (name ours)
extern RealtimeTimer script_editor_status_timer;     // 0x255434c 2 s per status text
extern char script_editor_status_text[256];          // 0x254ecc8
extern script_editor_pick script_editor_hover;       // 0x2546bd0 (object name ours, D3500)
extern script_editor_pick script_editor_sel;         // 0x255a008 (object name ours, D3500)
#define script_editor_hover_kind   (script_editor_hover.kind)     // 0x2546bd0
#define script_editor_hover_size   (script_editor_hover.size)     // 0x2546bd4
#define script_editor_hover_pos    (script_editor_hover.pos)      // 0x2546be0
#define script_editor_hover_orient (script_editor_hover.orient)   // 0x2546bec
#define script_editor_hover_object (script_editor_hover.object)   // 0x2546c10
#define script_editor_sel_kind     (script_editor_sel.kind)       // 0x255a008
#define script_editor_sel_size     (script_editor_sel.size)       // 0x255a00c
#define script_editor_sel_pos      (script_editor_sel.pos)        // 0x255a018
#define script_editor_sel_orient   (script_editor_sel.orient)     // 0x255a024
#define script_editor_sel_object   (script_editor_sel.object)     // 0x255a048
extern ix_ray script_editor_mouse_ray;               // 0x2552c08 gr_screen_to_world_ray (object name ours, D3500)
#define script_editor_mouse_ray_origin (script_editor_mouse_ray.origin)   // 0x2552c08
#define script_editor_mouse_ray_dir    (script_editor_mouse_ray.dir)      // 0x2552c14
extern vector3 script_editor_saved_pos;              // 0x2563a58 selection position when the drag began (D3502)
extern matrix script_editor_saved_orient;            // 0x254a8b0 its orientation (sum.symbols: script_editor_saved_pos, D3502)
extern bool script_editor_camera_held;               // 0x256b73d camera state pushed while the left button drags (name ours)
extern float script_editor_pick_distance;            // 0x255dc58 nearest hit so far (1e11 at the start)
extern int script_editor_pick_filter;                // 0x2566a84 what pick_hover tries (1 characters .. 4 objects)
extern int script_editor_mode;                       // 0x25629c0 SE_MODE_*
extern float script_editor_char_sizes[4][3];         // 0x255e9a8 {2 r, height, 2 r} per char_info size class (name ours)
extern float script_editor_char_size_radii[4];       // 0x255a7c4 the boxes' half diagonals (name ours)
extern Color script_editor_level_entry_colors[6];    // 0x254f1f0 action list colours, level scripts (name ours)
extern Color script_editor_cutscene_entry_colors[6]; // 0x254f6f8 action list colours, cutscene scripts (name ours)
extern bool script_editor_ui_created;                // 0x256b73c
extern script_object *script_editor_cur_script;      // 0x256b740
extern ui_listbox_entry *script_editor_cur_action;   // 0x256b744 the selected action list line (data = action index)
extern int script_editor_sel_action_index;           // 0x256b74c
extern bool script_editor_lists_dirty;               // 0x256b749
extern bool script_editor_insert_action;             // 0x256b748 I (insert) rather than A (append) pressed (name ours)
extern bool Render_s3d_navpoints;                    // 0x256b74a console "render_s3d_navpoints"
extern int script_editor_action_list_kind;           // 0x254da98
extern int script_editor_param_field_count;          // 0x256325c
extern int script_editor_new_selection;              // 0x256362c
extern NoRlsAllocPool script_editor_alloc_pool;      // 0x255dc60
extern char script_editor_alloc_pool_buffer[0x800];  // 0x2568f58
extern console_command ccmd_remove_nav;              // 0x254a8a0
extern console_command ccmd_render_s3d_navpoints;    // 0x2549d30
// script_ambients_dirty (0x255b4d8) and script_level_items_dirty (0x2563d88) are declared in
// levelscripts/script_internal.h.

// The gadgets (static objects, built by script_editor_create_ui).
extern ui_window script_editor_window;               // 0x254c4e0 main bar
extern ui_button script_editor_mode_button;          // 0x2556260
extern ui_button script_editor_pos_button;           // 0x2565bc8
extern ui_button script_editor_ori_button;           // 0x254edc8
extern ui_button script_editor_y_button;             // 0x254b6e0
extern ui_button script_editor_new_button;           // 0x254f710
extern ui_button script_editor_delete_button;        // 0x2546c18
extern ui_button script_editor_save_button;          // 0x2560eb8
extern ui_button script_editor_load_button;          // 0x2560668
extern ui_button script_editor_pause_button;         // 0x255ced8
extern ui_button script_editor_time_back_button;     // 0x2566a88
extern ui_inputbox script_editor_name_inputbox;      // 0x2555908
extern ui_button script_editor_prev_button;          // 0x2557b38
extern ui_button script_editor_next_button;          // 0x2565070
extern ui_button script_editor_cutscene_button;      // 0x25527e0
extern ui_container script_editor_selection_panel;   // 0x25499a8
extern ui_listbox script_editor_action_listbox;      // 0x254c9d0
extern ui_button script_editor_append_action_button; // 0x2560a90
extern ui_button script_editor_insert_action_button; // 0x255b4e0
extern ui_button script_editor_delete_action_button; // 0x2557f60
extern ui_button script_editor_action_up_button;     // 0x2563630 (name ours)
extern ui_button script_editor_action_down_button;   // 0x2551a60 (name ours)
extern ui_button script_editor_change_button;        // 0x255f178
extern ui_button script_editor_edit_button;          // 0x254bb08
extern ui_window script_editor_new_window;           // 0x25508e0
extern ui_button script_editor_new_ok_button;        // 0x255f5a0
extern ui_button script_editor_new_cancel_button;    // 0x254e050
extern ui_listbox script_editor_new_listbox;         // 0x2563a68
extern ui_window script_editor_action_window;        // 0x2554d28
extern ui_button script_editor_action_ok_button;     // 0x254ccf0
extern ui_button script_editor_action_cancel_button; // 0x2547040
extern ui_listbox script_editor_action_type_listbox; // 0x2564d50
extern ui_window script_editor_save_window;          // 0x2552c20
extern ui_static script_editor_save_name_label;      // 0x2553600
extern ui_static script_editor_save_filename_label;  // 0x2551690
extern ui_inputbox script_editor_save_name_inputbox; // 0x254daa0 (+0x580 is sum.symbols' script_editor_name_room, D3503)
extern ui_inputbox script_editor_save_filename_inputbox;   // 0x2567fd0
extern ui_container script_editor_save_kind_container;     // 0x2562638
extern ui_radio_button script_editor_save_level_radio;     // 0x2555eb8
extern ui_radio_button script_editor_save_cutscene_radio;  // 0x255a420
extern ui_button script_editor_save_ok_button;       // 0x256ae28
extern ui_button script_editor_save_cancel_button;   // 0x255bcb0
extern ui_window script_editor_load_window;          // 0x254f208
extern ui_inputbox script_editor_load_name_inputbox; // 0x254bf30
extern ui_listbox script_editor_load_listbox;        // 0x2557068
extern ui_container script_editor_load_kind_container;     // 0x2557388
extern ui_radio_button script_editor_load_level_radio;     // 0x2565498
extern ui_radio_button script_editor_load_cutscene_radio;  // 0x2552438
extern ui_button script_editor_load_ok_button;       // 0x2554900
extern ui_button script_editor_load_cancel_button;   // 0x254adc8
extern ui_window script_editor_ambient_window;       // 0x254a8d8
extern ui_inputbox script_editor_ambient_name_inputbox;    // 0x2554350
extern ui_listbox script_editor_ambient_listbox;     // 0x2564a30 (its entry ring +0x2d4 = sum.symbols' script_editor_ambient_file_list 0x2564d04, D3503)
extern ui_static script_editor_ambient_min_range_label;    // 0x255e230
extern ui_static script_editor_ambient_volume_label;       // 0x2563260
extern ui_static script_editor_ambient_rolloff_label;      // 0x255a050
extern ui_static script_editor_ambient_loop_delay_label;   // 0x255e9d8
extern ui_inputbox script_editor_ambient_min_range_inputbox;   // 0x256a878
extern ui_inputbox script_editor_ambient_volume_inputbox;      // 0x25539d0
extern ui_inputbox script_editor_ambient_rolloff_inputbox;     // 0x2569ea0
extern ui_inputbox script_editor_ambient_loop_delay_inputbox;  // 0x255af28
extern ui_button script_editor_ambient_ok_button;    // 0x256a450
extern ui_button script_editor_ambient_cancel_button;// 0x2556c40
extern ui_window script_editor_choice_window;        // 0x2550dd0
extern ui_button script_editor_choice_ok_button;     // 0x2566eb0
extern ui_button script_editor_choice_cancel_button; // 0x25672d8
extern ui_listbox script_editor_choice_listbox;      // 0x255ff78
extern ui_window script_editor_door_window;          // 0x25629c8
extern ui_container script_editor_door_sound_container;    // 0x2565840
extern ui_radio_button script_editor_door_opening_radio;   // 0x255a7d8
extern ui_radio_button script_editor_door_closing_radio;   // 0x255e600
extern ui_checkbox script_editor_door_locked_checkbox;     // 0x2546828
extern ui_static script_editor_door_skill_label;     // 0x2553f80
extern ui_static script_editor_door_percent_label;   // 0x25666b8
extern ui_static script_editor_door_min_range_label; // 0x25612e0
extern ui_static script_editor_door_volume_label;    // 0x25512c0
extern ui_static script_editor_door_rolloff_label;   // 0x255eda8
extern ui_inputbox script_editor_door_sound_inputbox;      // 0x25490d8
extern ui_inputbox script_editor_door_skill_inputbox;      // 0x2550330
extern ui_inputbox script_editor_door_key_inputbox;        // 0x2559688
extern ui_inputbox script_editor_door_open_min_range_inputbox;   // 0x255f9c8
extern ui_inputbox script_editor_door_open_volume_inputbox;      // 0x25689a8
extern ui_inputbox script_editor_door_open_rolloff_inputbox;     // 0x255d6a8
extern ui_inputbox script_editor_door_close_min_range_inputbox;  // 0x2567a20
extern ui_inputbox script_editor_door_close_volume_inputbox;     // 0x254a2f0
extern ui_inputbox script_editor_door_close_rolloff_inputbox;    // 0x25616b0
extern ui_listbox script_editor_door_open_sound_listbox;   // 0x2566398
extern ui_listbox script_editor_door_close_sound_listbox;  // 0x2567700
extern ui_listbox script_editor_door_key_listbox;    // 0x2569758
extern ui_button script_editor_door_ok_button;       // 0x2564118
extern ui_button script_editor_door_cancel_button;   // 0x254ff08
extern ui_window script_editor_door_lock_window;     // 0x254b1f0
extern ui_inputbox script_editor_door_lock_name_inputbox;  // 0x2549d40
extern ui_inputbox script_editor_door_lock_item_inputbox;  // 0x255c500
extern ui_checkbox script_editor_door_lock_give_checkbox;  // 0x255b908
extern ui_button script_editor_door_lock_ok_button;  // 0x255c0d8
extern ui_button script_editor_door_lock_remove_button;    // 0x254e478
extern ui_button script_editor_door_lock_unused_button;    // 0x255cab0 (constructed, never created; name ours)
extern ui_listbox script_editor_door_lock_item_listbox;    // 0x2555218
extern ui_inputbox script_editor_door_lock_value_inputbox; // 0x255dc80
extern ui_window script_editor_creature_window;      // 0x256b250
extern ui_static script_editor_creature_team_label;  // 0x254fb38
extern ui_static script_editor_creature_level_label; // 0x2559c38
extern ui_static script_editor_creature_info_label;  // 0x2555538
extern ui_container script_editor_creature_team_container; // 0x2563d90
extern ui_radio_button script_editor_creature_friendly_radio;  // 0x2565ff0
extern ui_radio_button script_editor_creature_neutral_radio;   // 0x255ab80
extern ui_radio_button script_editor_creature_hostile_radio;   // 0x2562eb8
extern ui_radio_button script_editor_creature_evil_radio;      // 0x255d300
extern ui_inputbox script_editor_creature_level_inputbox;  // 0x254d4e8
extern ui_button script_editor_creature_ok_button;   // 0x2557710
extern ui_button script_editor_creature_cancel_button;     // 0x254e8a0
extern ui_window script_editor_level_item_window;    // 0x2564540
extern ui_inputbox script_editor_level_item_name_inputbox; // 0x2551e88
extern ui_inputbox script_editor_level_item_count_inputbox;// 0x2561c60
extern ui_listbox script_editor_level_item_listbox;  // 0x2549688
extern ui_static script_editor_level_item_count_label;     // 0x254d118
extern ui_button script_editor_level_item_ok_button; // 0x2569a78
extern ui_button script_editor_level_item_cancel_button;   // 0x2562210
extern ui_window script_editor_params_window;        // 0x2553110
extern ui_static script_editor_params_title;         // 0x2560298
extern ui_static script_editor_param_labels[5];      // 0x2558388
extern ui_inputbox script_editor_param_inputboxes[5];// 0x2547468
extern ui_button script_editor_params_ok_button;     // 0x2556688
extern ui_button script_editor_params_cancel_button; // 0x2568580

// ---- functions (address order) ---------------------------------------------------------------------
// Several sum.symbols names describe something else (D3505): script_editor_delete_navpoint adds a camera
// navpoint, script_editor_refresh_action_editor tests whether an action type is offered,
// script_editor_show_action_params deletes the selected action, script_make_filename returns a navpoint.
// The names are kept.
struct file_find_node;
int  qsort_cmp_strptr(const void *a, const void *b);                 // 0x4dff50 strcmp of two char * (qsort)
bool script_character_uses_navpoint(Character *c, navpoint *np);     // 0x4dff90
void cmd_remove_nav();                                               // 0x4e0070
void script_remove_redundant_npc_navpoints();                        // 0x4e0080
bool script_editor_decimal_filter_cb(const char *text, char c);      // 0x4e0160 ui_inputbox char filter
void script_editor_create_ui();                                      // 0x4e0690
void script_editor_sync_cutscene_button();                           // 0x4e28f0
void script_editor_clear_selection();                                // 0x4e2910
void navpoint_set_name();                                            // 0x4e2990 (from the name box)
void script_editor_set_status(const char *text);                     // 0x4e2a90
void script_editor_close();                                          // 0x4e2ad0
void script_editor_pick_hover();                                     // 0x4e2bb0
bool script_editor_ray_hit_object(vector3 *pos, matrix *orient, vector3 *size, float radius);   // 0x4e2f30
void script_editor_draw_navpoints();                                 // 0x4e30d0
void script_editor_draw_marker(vector3 *pos, float height, matrix *orient, float size, gr::Mode mode);   // 0x4e3700
void script_editor_handle_input();                                   // 0x4e3a20
void script_editor_store_string(script_action *a, const char *s);    // 0x4e4890 into a->text
void script_action_format(char *name, char *args, script_action *a, Character *c);   // 0x4e4910
void script_editor_show_selection_panel();                           // 0x4e4be0
void script_editor_fill_action_list();                               // 0x4e4cf0
void script_editor_show_creature_info(Character *c);                 // 0x4e4f10
void script_editor_show_level_item(navpoint *np);                    // 0x4e5050
void script_editor_list_ambient_sounds(navpoint *np);                // 0x4e50f0
void script_editor_list_door_sounds(lobj *l);                        // 0x4e53a0
void script_editor_show_door_lock(lobj *l);                          // 0x4e57a0
void script_editor_place_creature();                                 // 0x4e58a0
void script_editor_add_navpoint(int type);                           // 0x4e5b10 NAVPOINT_TYPE_LEVEL / _CUTSCENE / _AMBIENT
void script_editor_update_buttons();                                 // 0x4e6040
void script_editor_fill_action_lists();                              // 0x4e62e0
bool script_editor_mode_button_cb(int event, int value);             // 0x4e68e0
bool script_editor_on_pos_ori(int event, int value);                 // 0x4e6930
bool script_editor_on_new(int event, int value);                     // 0x4e69a0
void script_editor_delete_navpoint();                                // 0x4e6a30 adds a $camera navpoint (D3505)
bool script_editor_on_delete(int event, int value);                  // 0x4e6ad0
bool script_editor_navpoint_in_use();                                // 0x4e6d90 the selected navpoint
void script_replace_navpoint_references(navpoint *from, navpoint *to);   // 0x4e6f10
bool script_editor_save_button_cb(int event, int value);             // 0x4e6fa0
void script_editor_show_script_dialog();                             // 0x4e6fe0
bool script_editor_on_load(int event, int value);                    // 0x4e70e0
void script_editor_list_scripts();                                   // 0x4e7120
bool script_editor_list_scripts_filter_cb(file_find_node *node);     // 0x4e71e0 file_find_files filter
bool script_editor_on_pause(int event, int value);                   // 0x4e7340
bool script_editor_on_time_back(int event, int value);               // 0x4e7380
bool script_editor_name_inputbox_cb(int event, int value);           // 0x4e73d0
bool script_editor_on_prev_next(int event, int value);               // 0x4e7410
void script_editor_select_prev();                                    // 0x4e7460
void script_editor_focus_selection();                                // 0x4e75c0
void script_editor_select_first();                                   // 0x4e7740
void script_editor_select_next();                                    // 0x4e7800
bool script_editor_cutscene_button_cb(int event, int value);         // 0x4e7920
bool script_editor_add_action_cb(int event, int value);              // 0x4e7970
bool script_editor_refresh_action_editor(int type, Character *c);    // 0x4e7a40 action type offered for c (D3505)
bool script_editor_delete_action_cb(int event, int value);           // 0x4e7c70
void script_editor_show_action_params();                             // 0x4e7cd0 deletes the selected action (D3505)
bool script_editor_action_up_cb(int event, int value);               // 0x4e7dc0
bool script_editor_action_down_cb(int event, int value);             // 0x4e7ee0
bool script_editor_change_cb(int event, int value);                  // 0x4e8000
bool script_editor_edit_button_cb(int event, int value);             // 0x4e8020
bool script_editor_new_ok_cb(int event, int value);                  // 0x4e83a0
bool script_editor_new_cancel_cb(int event, int value);              // 0x4e83d0
bool script_editor_new_listbox_cb(int event, int value);             // 0x4e8400
bool script_editor_action_window_cb(int event, int value);           // 0x4e8440
void script_editor_apply_action_params();                            // 0x4e84b0
void script_create();                                                // 0x4e88a0
navpoint *script_make_filename();                                    // 0x4e8cf0 the default navpoint of a new action (D3505)
bool script_editor_save_name_cb(int event, int value);               // 0x4e8d60
int  script_save_files();                                            // 0x4e8e80 0, or 1-6 (script_save)
int  script_write_level_script(const char *filename, bool level);    // 0x4e9310 File::open / close result
void script_write_navpoint(File *f, navpoint *np);                   // 0x4e9ba0
bool script_write_objects(File *f, Character *c, bool level);        // 0x4e9ce0 true when a record was written
int  script_write_level_table(const char *table, const char *temp);  // 0x4ea1e0
int  level_items_write_table(const char *temp);                      // 0x4ea500
void script_save(int result);                                        // 0x4eac70 (script_save_files' result)
bool script_editor_save_kind_radio_cb(int event, int value);         // 0x4eae10
bool script_editor_save_ok_cb(int event, int value);                 // 0x4eaf00
bool script_editor_save_cancel_cb(int event, int value);             // 0x4eaf40
bool script_editor_script_name_cb(int event, int value);             // 0x4eaf70 (0x4eaf87 __complete is a label inside)
bool script_editor_load_selected_script();                           // 0x4eb080
bool script_editor_load_listbox_cb(int event, int value);            // 0x4eb0f0
bool script_editor_load_kind_radio_cb(int event, int value);         // 0x4eb140
bool script_editor_load_ok_cb(int event, int value);                 // 0x4eb160
bool script_editor_load_cancel_cb(int event, int value);             // 0x4eb190
bool script_editor_ambient_name_cb(int event, int value);            // 0x4eb1c0 (0x4eb1d7 __complete is a label inside)
bool script_editor_ambient_listbox_cb(int event, int value);         // 0x4eb2b0
bool script_editor_ambient_field_cb(int event, int value);           // 0x4eb2e0
bool script_editor_ambient_ok(int event, int value);                 // 0x4eb3d0
bool script_editor_ambient_cancel_cb(int event, int value);          // 0x4eb500
bool script_editor_params_window_cb(int event, int value);           // 0x4eb530
bool script_editor_params_ok_cb(int event, int value);               // 0x4eb5c0
bool script_editor_params_cancel_cb(int event, int value);           // 0x4eb760
bool script_editor_choice_ok_cb(int event, int value);               // 0x4eb780
bool script_editor_choice_cancel_cb(int event, int value);           // 0x4eb8a0
bool script_editor_choice_listbox_cb(int event, int value);          // 0x4eb8d0
bool script_editor_door_sound_radio_cb(int event, int value);        // 0x4eb9e0
bool script_editor_door_sound_inputbox_cb(int event, int value);     // 0x4ebb20
bool script_editor_door_sound_listbox_cb(int event, int value);      // 0x4ebc80
bool script_editor_door_field_cb(int event, int value);              // 0x4ebcf0
bool script_editor_door_lock_value_cb(int event, int value);         // 0x4ebe10
bool script_editor_door_locked_cb(int event, int value);             // 0x4ebe90
bool script_editor_door_lock_give_checkbox_cb(int event, int value); // 0x4ebee0
bool script_editor_door_skill_cb(int event, int value);              // 0x4ebf50
bool script_editor_door_key_cb(int event, int value);                // 0x4ec000 (0x4ec029 __complete is a label inside)
bool script_editor_door_item_cb(int event, int value);               // 0x4ec100 (0x4ec129 __complete is a label inside)
bool script_editor_door_key_listbox_cb(int event, int value);        // 0x4ec1f0
bool script_editor_door_lock_item_listbox_cb(int event, int value);  // 0x4ec230
bool script_editor_door_ok(int event, int value);                    // 0x4ec270
bool script_editor_door_lock_ok_cb(int event, int value);            // 0x4ec570
bool script_editor_door_lock_remove_cb(int event, int value);        // 0x4ec630
bool script_editor_door_cancel_cb(int event, int value);             // 0x4ec670
bool script_editor_creature_level_cb(int event, int value);          // 0x4ec6a0
bool script_editor_creature_ok(int event, int value);                // 0x4ec730
bool script_editor_creature_cancel_cb(int event, int value);         // 0x4ec9d0
bool script_editor_level_item_cb(int event, int value);              // 0x4eca00 (0x4eca29 __complete is a label inside)
bool script_editor_level_item_listbox_cb(int event, int value);      // 0x4ecb00
bool script_editor_level_item_count_cb(int event, int value);        // 0x4ecb40
bool script_editor_level_item_ok(int event, int value);              // 0x4ecbe0
bool script_editor_level_item_cancel_cb(int event, int value);       // 0x4ecf30
void cmd_render_s3d_navpoints();                                     // 0x4ecf90

#endif
