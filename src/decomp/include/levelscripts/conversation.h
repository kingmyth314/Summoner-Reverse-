// levelscripts/conversation.h - the conversation objects and dialogue boxes (levelscripts\conversation.cpp,
// path reference 0x4d7c43; conversationctl.cpp 0x4d87d0-0x4db249 by string ids). The two boxes are built by
// static initialisers at 0x4d6660 / 0x4d66e0 and live in .bss at 0x2544408 / 0x2545b68. Class and field
// names are ours unless sum.symbols names the method. The real box classes have virtual functions (vtables
// 0x581c4c / 0x581c5c / 0x581c68 / 0x581c6c); the vtable pointer is a plain member here so this header
// defines nothing. Not original source.
//
// decomp20 (conversation.cpp / conversationctl.cpp): the conversation records (speakers, topics, lines),
// the conversation object 0x25461a0 (class conversation, 0x310 bytes) and the controller 0x25464b0 (class
// conversation_controller, 0x27c bytes) with their full layouts; DECISIONS.md C1-C7.
#ifndef LEVELSCRIPTS_CONVERSATION_H
#define LEVELSCRIPTS_CONVERSATION_H

#include "vsdk/vsdk.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/os/norlsallocpool.h"
#include "vsdk/math/vector.h"

class Character;
class conversation_slot;
struct conversation_topic;

// Common part (dialogue_box_base__ctor 0x4d70f0), 0x2c bytes.
class dialogue_box_base {
public:
    void set_rect(int x, int y, int w, int lines);   // 0x4d7160 dialogue_box__set_rect
    bool item_visible(int item);                     // 0x4d78b0 dialogue_box__item_visible
    int  item_y(int item);                           // 0x4d7900 dialogue_box__item_y
    void reset();                                    // 0x4d7920 dialogue_box_reset_thunk (jmp 0x4d7140)

    void *vtable;         // +0x00
    int   x;              // +0x04
    int   y;              // +0x08
    float scroll;         // +0x0c current scroll offset (animated towards scroll_target)
    int   scroll_target;  // +0x10
    int   w;              // +0x14
    int   text_height;    // +0x18
    int   lines;          // +0x1c visible lines
    int   first;          // +0x20 ring start (64 entries)
    int   count;          // +0x24
    int   timer;          // +0x28 GameTimer
};

// A wrapped text line of the text box (0x1c bytes, vtable 0x581c58). Fields from conversationctl.cpp
// (decomp20): the same top / height slots as dialogue_topic_entry.
struct dialogue_line {
    void *vtable;                 // +0x00
    int   top;                    // +0x04 y offset inside the box
    int   _unk08;
    int   height;                 // +0x0c
    int   kind;                   // +0x10 dialogue_show_line kind: 0 a spoken line, 1 a notice, 2 the topic
    int   _unk14;                 // +0x14 nonzero: conversation_select_topic keeps the scroll on this line
    conversation_slot *talker;    // +0x18 who speaks it (conversation_update_talk_anims)
};

// The conversation text box at 0x2544408 (0x1750 bytes).
class dialogue_box : public dialogue_box_base {
public:
    void start();                                    // 0x4d7650 conversation_start
    bool line_fits(int line);                        // 0x4d7600 dialogue_box__line_fits
    void scroll_next_page();                         // 0x4d74f0 dialogue_box__scroll_next_page
    void draw();                                     // 0x4d7680 dialogue_text_box_draw
    int  first_visible_line();                       // 0x4d7410 dialogue_box__first_visible_line, -1 none
    int  last_visible_line();                        // 0x4d7480 dialogue_box__last_visible_line, -1 none

    dialogue_line line[64];                          // +0x02c
    char label[32][0x81];                            // +0x72c
    unsigned char _unk174c[4];
};

// One topic entry of the topic box (0x18 bytes, vtable 0x581c68).
struct dialogue_topic_entry {
    void *vtable;         // +0x00
    int   top;            // +0x04 y offset inside the box
    int   _unk08;
    int   height;         // +0x0c
    int   state;          // +0x10 3 normal, 4 under the mouse (set by dialogue_draw)
    conversation_topic *topic;   // +0x14 (an int "topic id" before decomp20: DECISIONS.md C5)
};

// The topic list at 0x2545b68 (0x62c bytes).
class dialogue_topic_list : public dialogue_box_base {
public:
    void add(conversation_topic *topic, int kind);   // 0x4d7800 dialogue_topic_box_add
    void draw();                                     // 0x4d7930 dialogue_topic_box_draw

    dialogue_topic_entry entry[64];                  // +0x02c
};

extern dialogue_box dialogue_text_box;               // 0x2544408
extern dialogue_topic_list dialogue_topic_box;        // 0x2545b68
extern int dialogue_selected_topic;                  // 0x25443b4

// Free helpers of the dialogue box unit (0x4d7ab0-0x4d7b10, untagged; decomp20).
void dialogue_topic_box_reset();                     // 0x4d7ab0
int  dialogue_topic_box_add_kind3(conversation_topic *topic);   // 0x4d7ac0 add(topic, 3), returns 0
void dialogue_screen_open(const char *name);         // 0x4d7ae0 interface_set_screen(13, name)
void dialogue_screen_close();                        // 0x4d7b00 interface_set_screen(0, 0)
// Adds a line to the text box (kind 0 a spoken line, 1 a notice, 2 the topic text).
void dialogue_show_line(const char *text, int kind, conversation_slot *talker);   // 0x4d7b10

// The view new_dialogue.cpp has of conversation +0x204 / +0x208 (one flag word, read as bitfields). Kept
// for that file; the typed records are conversation_speaker and conversation_topic (DECISIONS.md C2).
struct conversation_node {
    int _unk00;
    unsigned int flag0 : 1;
    unsigned int flag1 : 1;
    unsigned int flag2 : 1;  // +0x04 bit 2: on the speaker CFLAG_NO_ABORT, on the topic `farewell`
    unsigned int flag3 : 1;
    unsigned int flag4 : 1;
    unsigned int flag5 : 1;
    unsigned int flag6 : 1;  // +0x04 bit 6: on the topic `shop` (end_cleanup(1) opens the shop)
};

// ---- the conversation records (dialogue_parse_table 0x4fee50 builds them in conversation.pool) ----------
// Speaker flags (+Flags:, dialogue_parse_cflags 0x4fed30; names from its strings).
#define CFLAG_NO_CAMERA_MOVEMENT 0x01   // Cflag_NoCameraMovement
#define CFLAG_DONT_FACE_PLAYER   0x02   // Cflag_DontFacePlayer
#define CFLAG_NO_ABORT           0x04   // Cflag_NoAbort: Esc and the close button do nothing
#define CFLAG_IMPORTANT          0x08   // Cflag_Important: dialogue camera option 2 zooms only on these
#define CFLAG_PERSISTENT         0x10   // Cflag_Persistent: saved state goes to the level's other slots

struct conversation_speaker;
struct conversation_line;
struct dialogue_action;

// A topic reference of the +Add topic: .. +Set hail topic: lists (0xc bytes).
struct conversation_topic_ref {
    conversation_topic *topic;           // +0x0
    conversation_speaker *speaker;       // +0x4 the topic's speaker
    conversation_topic_ref *next;        // +0x8
};
// +Hide quest: / +Unhide quest: (8 bytes).
struct conversation_quest_ref {
    int quest;                           // +0x0 quest_table index
    conversation_quest_ref *next;        // +0x4
};
// +Set important / +Clear important (8 bytes).
struct conversation_speaker_ref {
    conversation_speaker *speaker;       // +0x0 NULL when the name is not a speaker of this conversation
    conversation_speaker_ref *next;      // +0x4
};

// One $Character: block (0x1c bytes, conversation_alloc_speaker).
struct conversation_speaker {
    void init(const char *name);                     // 0x4feb50 conversation_speaker__init
    conversation_topic *find_topic(const char *name);// 0x4feb80 conversation_speaker__find_topic
    void refresh_topic_flags();                      // 0x4febe0 conversation_speaker__refresh_topic_flags
    void add_topic(conversation_topic *topic);       // 0x4fec50 conv_speaker__add_topic

    conversation_speaker *next;          // +0x00 conversation.speakers list
    ushort flags;                        // +0x04 CFLAG_*
    ushort move_in_time;                 // +0x06 +Move in time: (ms, 3000)
    ushort move_out_time;                // +0x08 +Move out time: (ms, 1500)
    ushort _pad0a;
    const char *name;                    // +0x0c
    conversation_topic *topics;          // +0x10 head
    conversation_topic *topics_tail;     // +0x14
    conversation_topic *greeting;        // +0x18 hail topic, said when the conversation starts
};
LAYOUT_CHECK(conversation_speaker, sizeof(conversation_speaker) == 0x1c);

// One action of a line (0x18 bytes, dialogue_rec__init4 0x500990); run by conversation_execute_action.
struct dialogue_action {
    int _unk00;                          // +0x00
    int _unk04;                          // +0x04
    int kind;                            // +0x08 0..12 (conversation_execute_action's switch)
    int _unk0c;                          // +0x0c
    int _unk10;                          // +0x10
    dialogue_action *next;               // +0x14
};
LAYOUT_CHECK(dialogue_action, sizeof(dialogue_action) == 0x18);

// One +PCText: .. +JekharText: line of a topic (0x14 bytes, dialogue_rec__init3 0x5009c0).
#define CLINE_PC       0
#define CLINE_NPC      1
#define CLINE_JOSEPH   2
#define CLINE_ROSALIND 3
#define CLINE_FLECE    4
#define CLINE_JEKHAR   5
struct conversation_line {
    void init(const char *text, int kind, const char *who);   // 0x5009c0 dialogue_rec__init3
    void append_action(dialogue_action *a);                   // 0x5009f0 dialogue_rec__append_child

    conversation_line *next;             // +0x00
    const char *text;                    // +0x04
    int kind;                            // +0x08 CLINE_*
    const char *who;                     // +0x0c speaker name (or a species / party name)
    dialogue_action *actions;            // +0x10
};
LAYOUT_CHECK(conversation_line, sizeof(conversation_line) == 0x14);

// One +Topic: (0x44 bytes, conversation_alloc_topic). The flag word is read as bitfields (shr / test) and
// written with or / and masks by the parser, so both views are given.
#define CTOPIC_LISTED   0x02
#define CTOPIC_FAREWELL 0x04
#define CTOPIC_INITIAL  0x08
#define CTOPIC_FRESH    0x10
#define CTOPIC_HIDDEN   0x20
#define CTOPIC_SHOP     0x40
struct conversation_topic {
    void init(const char *name, const char *text);   // 0x4fec10 conversation_topic__init
    void push_add_topic(conversation_topic_ref *r);           // 0x4fec80 conversation_topic__push_list18
    void push_remove_topic(conversation_topic_ref *r);        // 0x4fec90 .._push_list1c
    void push_add_initial_topic(conversation_topic_ref *r);   // 0x4feca0 .._push_list20
    void push_remove_initial_topic(conversation_topic_ref *r);// 0x4fecb0 .._push_list24
    void push_hide_topic(conversation_topic_ref *r);          // 0x4fecc0 .._push_list28
    void push_unhide_topic(conversation_topic_ref *r);        // 0x4fecd0 .._push_list2c
    void push_hide_quest(conversation_quest_ref *r);          // 0x4fece0 .._push_list30
    void push_unhide_quest(conversation_quest_ref *r);        // 0x4fecf0 .._push_list34
    void push_set_important(conversation_speaker_ref *r);     // 0x4fed00 .._push_list38
    void push_clear_important(conversation_speaker_ref *r);   // 0x4fed10 .._push_list3c
    void push_set_hail_topic(conversation_topic_ref *r);      // 0x4fed20 .._push_list40

    conversation_topic *next;            // +0x00 speaker's topic list
    union {
        unsigned int flags;              // +0x04 CTOPIC_*
        struct {
            unsigned int flag0 : 1;      // bit 0 saved and restored (topic bit kind 0); not read here
            unsigned int listed : 1;     // bit 1 offered (conversation_first_topic / next_topic)
            unsigned int farewell : 1;   // bit 2 +Farewell: without lines the conversation ends
            unsigned int initial : 1;    // bit 3 offered when the conversation starts (refresh_topic_flags)
            unsigned int fresh : 1;      // bit 4 (re)offered: conversation_build_topic_list shows it
            unsigned int hidden : 1;     // bit 5 +Hidden, +Hide topic:, dialogue_make_topic_unavailable
            unsigned int shop : 1;       // bit 6 +Shop: the shop opens after the topic
        };
    };
    const char *name;                    // +0x08 the +Topic: label (name_hash_nocase in the save)
    const char *text;                    // +0x0c the text the player says
    conversation_line *lines;            // +0x10
    conversation_line *lines_tail;       // +0x14
    conversation_topic_ref *add_topics;              // +0x18 +Add topic:
    conversation_topic_ref *remove_topics;           // +0x1c +Remove topic:
    conversation_topic_ref *add_initial_topics;      // +0x20 +Add initial topic:
    conversation_topic_ref *remove_initial_topics;   // +0x24 +Remove initial topic:
    conversation_topic_ref *hide_topics;             // +0x28 +Hide topic:
    conversation_topic_ref *unhide_topics;           // +0x2c +Unhide topic:
    conversation_quest_ref *hide_quests;             // +0x30 +Hide quest:
    conversation_quest_ref *unhide_quests;           // +0x34 +Unhide quest:
    conversation_speaker_ref *set_important;         // +0x38 +Set important
    conversation_speaker_ref *clear_important;       // +0x3c +Clear important
    conversation_topic_ref *set_hail_topics;         // +0x40 +Set hail topic:
};
LAYOUT_CHECK(conversation_topic, sizeof(conversation_topic) == 0x44);

// An entry of the offered-topic list (0xc bytes; 16 of them in conversation).
struct conversation_topic_entry {
    conversation_topic_entry *next;      // +0x0
    conversation_topic_entry *prev;      // +0x4
    conversation_topic *topic;           // +0x8
};

// ---- conversation_db at 0x25461a0 (0x310 bytes): the level's conversation file and its state ---------
// sum.symbols names its constructor conversation__ctor (0x4d7b80); conversation_storage is the earlier
// name (DECISIONS.md D14, C1). The destructor is implicit (0x4d7b70 conversation__dtor: the pool's).
class conversation {
public:
    conversation();                                       // 0x4d7b80 conversation__ctor
    int  alloc_storage();                                 // 0x4d7c20 conversation_alloc_storage
    void clear();                                         // 0x4d7c70 conversation_clear
    void free();                                          // 0x4d7ca0 conversation_free
    void add_speaker(conversation_speaker *s);            // 0x4d7cc0 conversation_add_speaker
    conversation_speaker *alloc_speaker(const char *name);// 0x4d7ce0
    conversation_topic *alloc_topic(const char *name, const char *text, bool listed);   // 0x4d7d10
    int  set_speaker(const char *name);                   // 0x4d7d50, 0 = started, -1 = no such speaker
    void clear_speaker();                                 // 0x4d7e10
    void set_topic(conversation_topic *topic);            // 0x4d7e40
    conversation_topic *select_greeting();                // 0x4d7e90
    void apply_topic_effects();                           // 0x4d7eb0
    bool has_speaker(const char *name);                   // 0x4d80c0
    bool has_speaker_hash(unsigned int hash);             // 0x4d8100
    conversation_topic *first_topic();                    // 0x4d8140
    conversation_topic *next_topic(conversation_topic *t);// 0x4d8170
    int  add_name(char *name);                            // 0x4d81a0
    int  find_name(const char *name);                     // 0x4d81d0, -1 = none
    int  set_topic_available(const char *speaker_name, const char *topic_name, bool available);   // 0x4d8220
    conversation_speaker *find_speaker(const char *name); // 0x4d82b0
    int  load(const char *level_name, const char *script_name);  // 0x4d82f0, dialogue_parse_table's result
    void save_state();                                    // 0x4d8390 conversation_save_state
    void save_topics();                                   // 0x4d83a0 conversation_save_topics
    void restore_state();                                 // 0x4d84d0 conversation_restore_state
    void build_topic_list();                              // 0x4d8620
    void remove_topic_entry(conversation_topic *topic);   // 0x4d86f0
    void farewell();                                      // 0x4d8760 conversation_farewell
    int  parse_table(const char *script_name);            // 0x4fee50 dialogue_parse_table ("_dlg.tbl"; other unit)

    char *names[128];                    // +0x000 pooled speaker names (conversation_add_name)
    unsigned int name_count;             // +0x200
    union {                              // +0x204 the speaker talked to (DECISIONS.md C2)
        conversation_speaker *speaker;
        conversation_node *cur_topic;    //   new_dialogue.cpp's name
    };
    union {                              // +0x208 the topic being said
        conversation_topic *topic;
        conversation_node *cur_line;     //   new_dialogue.cpp's name
    };
    conversation_speaker *speakers;      // +0x20c list (conversation_add_speaker pushes on the front)
    NoRlsAllocPool pool;                 // +0x210 128 KB (conversation_alloc_storage)
    int level;                           // +0x22c level_script_table index, -1 none
    int slot;                            // +0x230 conversation slot of the level
    int journal_updated;                 // +0x234 set when a topic hid or unhid a quest (dialogue_leave)
    conversation_topic_entry topic_list; // +0x238 offered topics
    conversation_topic_entry free_entries;   // +0x244
    conversation_topic_entry entries[16];    // +0x250
};
typedef conversation conversation_storage;           // the name before decomp20 (DECISIONS.md C1)
extern conversation conversation_db;                 // 0x25461a0

// ---- conversation_active at 0x25464b0 (0x27c bytes): the running conversation ------------------------
// A camera position and look direction (0x18 bytes). Both members are inline in the original header
// (DECISIONS.md C3): 0x4d8f00 dialog_camera_pose__set and 0x4d9aa0 dialog_camera_pose__assign are their
// out-of-line copies; conversation_end_cleanup expands the assignment in place.
struct dialog_camera_pose {
    void set(vector3 p, vector3 d)
    {
        pos.x = p.x; pos.y = p.y; pos.z = p.z;
        dir.x = d.x; dir.y = d.y; dir.z = d.z;
    }
    dialog_camera_pose &operator=(const dialog_camera_pose &p)
    {
        pos.x = p.pos.x; pos.y = p.pos.y; pos.z = p.pos.z;
        dir.x = p.dir.x; dir.y = p.dir.y; dir.z = p.dir.z;
        return *this;
    }

    vector3 pos;                         // +0x00
    vector3 dir;                         // +0x0c forward (not normalised)
};
LAYOUT_CHECK(dialog_camera_pose, sizeof(dialog_camera_pose) == 0x18);

// A talker of the text box lines (0x18 bytes): circular lists through next / prev, the heads are slots too.
class conversation_slot {
public:
    conversation_slot();                 // 0x4d8ca0 conversation_slot__ctor
    ~conversation_slot();                // empty; folded into empty_function 0x4f13e0
    void reset();                        // 0x4d8cc0 conversation_slot__reset

    conversation_slot *next;             // +0x00
    conversation_slot *prev;             // +0x04
    Character *character;                // +0x08
    int  move;                           // +0x0c talk move playing, -1 none
    bool talking;                        // +0x10 one of its lines is on screen
    int  kind;                           // +0x14 0 the player's topic, 1 a line
};
LAYOUT_CHECK(conversation_slot, sizeof(conversation_slot) == 0x18);

// sum.symbols names the constructor conversation_controller__ctor (0x4d8ce0); class `conversation`
// before decomp20 (DECISIONS.md C1). The destructor is implicit (0x4d8810).
class conversation_controller {
public:
    conversation_controller();                                    // 0x4d8ce0
    conversation_slot *add_talker(Character *c, int kind);        // 0x4d8880 conversation_controller__add_talker
    conversation_slot *topic_talker(conversation_topic *topic);   // 0x4d8900
    conversation_slot *line_talker(conversation_line *line);      // 0x4d8940
    void update_talk_anims();                                     // 0x4d8a90 conversation_update_talk_anims
    void open_dialogue();                                         // 0x4d8db0 conversation_open_dialogue
    void camera_capture();                                        // 0x4d8e10
    void camera_compute_shot();                                   // 0x4d8f50
    void camera_blend(dialog_camera_pose *out, dialog_camera_pose *a, dialog_camera_pose *b, float t);   // 0x4d9ad0
    int  camera_update();                                         // 0x4d9d90, -1 = camera back, conversation over
    int  frame();                                                 // 0x4da010 conversation_frame, -1 = idle
    int  camera_begin(const char *name, Character *speaker);      // 0x4da890, 0 = started
    void end_cleanup(bool open_shop);                             // 0x4daa70 conversation_end_cleanup
    void choose_topic(conversation_topic *topic);                 // 0x4dabb0 (int before decomp20: C5)
    void request_end();                                           // 0x4dabf0 conversation_request_end
    void reset_state();                                           // 0x4dac10 conversation_reset_state
    bool is_idle();                                               // 0x4dac20 conversation_is_idle
    void end();                                                   // 0x4dac30 conversation_end
    void select_topic(conversation_topic *topic);                 // 0x4dacb0 conversation_select_topic
    int  load_for_level(const char *level_filename, const char *script_name);   // 0x4daef0

    Character *speaker;                  // +0x00 the character talked to
    GameTimer timer;                     // +0x04 4 s to walk up (conversation_camera_begin)
    int   state;                         // +0x08 0 idle, 1 choosing a topic, 2 showing lines, 3 walking up
    int   approach;                      // +0x0c 0 walking up, 1 waiting / sliding, 2 turning, 3 in place
    int   cam_mode;                      // +0x10 0 none, 1 moving in, 2 holding, 3 moving out
    int   facing;                        // +0x14 1 once the speaker has turned to the player
    float slide_x;                       // +0x18 slide target of cur_character (x / z)
    float slide_z;                       // +0x1c
    GameTimer cam_timer;                 // +0x20
    float move_in_time;                  // +0x24 ms, from the speaker
    float move_out_time;                 // +0x28 ms
    const char *name;                    // +0x2c speaker name given to conversation_begin
    float frame_height;                  // +0x30 (both heights) / 2 + 0.25, not read in this unit
    int   speaker_flags;                 // +0x34 CFLAG_*
    unsigned int flag0 : 1;              // +0x38 bit 0 cleared by camera_begin, not read in this unit
    unsigned int busy : 1;               //   bit 1 inside select_topic / end requested
    unsigned int shot_valid : 1;         //   bit 2 shot_side chosen
    unsigned int shot_side : 1;          //   bit 3 which side of the two speakers the camera is on
    unsigned int shot_cut : 1;           //   bit 4 the side changed: blend from the current view
    unsigned int sliding : 1;            //   bit 5 cur_character slides to slide_x / slide_z
    vector3 player_pos;                  // +0x3c cur_character's centre when the speaker last turned
    dialog_camera_pose game;             // +0x48 the game camera this frame
    dialog_camera_pose start;            // +0x60 blend start
    dialog_camera_pose shot;             // +0x78 the conversation shot
    dialog_camera_pose blend;            // +0x90 blend result
    dialog_camera_pose out;              // +0xa8 the view used
    vector3 speaker_dir;                 // +0xc0 the speaker's forward before the conversation
    conversation_slot active;            // +0xcc talkers
    conversation_slot free_slots;        // +0xe4
    conversation_slot slots[16];         // +0xfc
};
LAYOUT_CHECK(conversation_controller, sizeof(conversation_controller) == 0x27c);
extern conversation_controller conversation_active;  // 0x25464b0

int  conversation_begin(const char *name, Character *speaker);   // 0x4daf50 (player_talk_to, script_object_talk)
bool character_has_ai_goal_10(Character *c);         // 0x4da860 (cdecl; conversationctl.cpp)
void cmd_dialogue_make_topic_available();            // 0x4db010
void cmd_dialogue_make_topic_unavailable();          // 0x4db100
void cmd_dcamera();                                  // 0x4db1f0

LAYOUT_CHECK(dialogue_box_base, sizeof(dialogue_box_base) == 0x2c);
LAYOUT_CHECK(dialogue_box, sizeof(dialogue_box) == 0x1750);
LAYOUT_CHECK(dialogue_topic_list, sizeof(dialogue_topic_list) == 0x62c);
LAYOUT_CHECK(conversation, sizeof(conversation) == 0x310);

// The conversation save slots (0x10-byte records {name, speakers, topics, ?}; index (level * 7 + slot) * 4
// is the slot name, which the main menu and the load screen pass to game_change_level).
extern void *conversation_save_table[357 * 4];      // 0x25388c0

#endif
