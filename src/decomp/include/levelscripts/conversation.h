// levelscripts/conversation.h - the conversation objects and dialogue boxes (levelscripts\conversation.cpp,
// path reference 0x4d7c43; conversationctl.cpp 0x4dacb0 by string ids). The two boxes are built by static
// initialisers at 0x4d6660 / 0x4d66e0 and live in .bss at 0x2544408 / 0x2545b68. Layouts from the fields
// new_dialogue.cpp touches; class and field names are ours unless sum.symbols names the method. The real
// classes have virtual functions (vtables 0x581c4c / 0x581c5c / 0x581c68 / 0x581c6c); the vtable pointer is
// a plain member here so this header defines nothing. Not original source.
#ifndef LEVELSCRIPTS_CONVERSATION_H
#define LEVELSCRIPTS_CONVERSATION_H

#include "vsdk/vsdk.h"
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

// A wrapped text line of the text box (0x1c bytes, vtable 0x581c58).
struct dialogue_line {
    void *vtable;
    unsigned char _unk04[0x18];
};

// The conversation text box at 0x2544408 (0x1750 bytes).
class dialogue_box : public dialogue_box_base {
public:
    void start();                                    // 0x4d7650 conversation_start
    bool line_fits(int line);                        // 0x4d7600 dialogue_box__line_fits
    void scroll_next_page();                         // 0x4d74f0 dialogue_box__scroll_next_page
    void draw();                                     // 0x4d7680 dialogue_text_box_draw

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
    int   topic;          // +0x14 conversation topic id
};

// The topic list at 0x2545b68 (0x62c bytes).
class dialogue_topic_list : public dialogue_box_base {
public:
    void add(int topic, int kind);                   // 0x4d7800 dialogue_topic_box_add
    void draw();                                     // 0x4d7930 dialogue_topic_box_draw

    dialogue_topic_entry entry[64];                  // +0x02c
};

extern dialogue_box dialogue_text_box;               // 0x2544408
extern dialogue_topic_list dialogue_topic_box;        // 0x2545b68
extern int dialogue_selected_topic;                  // 0x25443b4

// A conversation node referenced by conversation_db (+0x204 / +0x208). Only the flag word is known; the two
// bits are read as bitfields (shr/and in conversation_end and dialogue_frame). Bit names are ours.
struct conversation_node {
    int _unk00;
    unsigned int flag0 : 1;
    unsigned int flag1 : 1;
    unsigned int flag2 : 1;  // +0x04 bit 2: on cur_topic, Esc and the close button are ignored; on cur_line, the conversation goes on (no request_end)
    unsigned int flag3 : 1;
    unsigned int flag4 : 1;
    unsigned int flag5 : 1;
    unsigned int flag6 : 1;  // +0x04 bit 6: on the line, end the conversation (end_cleanup(1)) after it
};

// conversation_db at 0x25461a0 (0x310 bytes, tagged new_saveload.cpp in sum.symbols).
class conversation_storage {           // merged with stubs_options_saveload.h (DECISIONS.md D14)
public:
    void save_state();                                   // 0x4d8390 conversation_save_state

    unsigned char _unk000[0x204];
    conversation_node *cur_topic;    // +0x204 (name ours)
    conversation_node *cur_line;     // +0x208 (name ours)
    unsigned char _unk20c[0x28];
    int journal_updated;             // +0x234 set when a quest was updated during the conversation (name ours)
    unsigned char _unk238[0xd8];
};
extern conversation_storage conversation_db;         // 0x25461a0

// conversation_active at 0x25464b0 (0x280 bytes).
class conversation {
public:
    void end_cleanup(bool flag);                     // 0x4daa70 conversation_end_cleanup
    void choose_topic(int topic);                    // 0x4dabb0 conversation_choose_topic
    void request_end();                              // 0x4dabf0 conversation_request_end

    unsigned char _unk000[0x280];
};
extern conversation conversation_active;             // 0x25464b0
LAYOUT_CHECK(dialogue_box_base, sizeof(dialogue_box_base) == 0x2c);
LAYOUT_CHECK(dialogue_box, sizeof(dialogue_box) == 0x1750);
LAYOUT_CHECK(dialogue_topic_list, sizeof(dialogue_topic_list) == 0x62c);
LAYOUT_CHECK(conversation_storage, sizeof(conversation_storage) == 0x310);
LAYOUT_CHECK(conversation, sizeof(conversation) == 0x280);

// The conversation save slots (0x10-byte records {name, speakers, topics, ?}; index (level * 7 + slot) * 4
// is the slot name, which the main menu and the load screen pass to game_change_level).
extern void *conversation_save_table[357 * 4];      // 0x25388c0

#endif
