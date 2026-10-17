// ui_container.h - Sum.exe v1.40 container gadget (vsdk\ui\ui_container.cpp, 0x5534f0-0x553b9b)
// Written from the Sum.exe disassembly. Names from symbols/sum.symbols; the class name is ours (no RTTI).
#ifndef UI_CONTAINER_H
#define UI_CONTAINER_H

#include "vsdk/ui/ui.h"

// A group of gadgets (0x388 bytes; gadget type 1). With a label it draws an etched frame with the label
// in its top edge; without one, flag 1 draws a double bevel. A mask bitmap (one byte per pixel) can
// give the container a non-rectangular hit area (get_mask_pixel).
//
// Vtable 0x581f40, same nine slots as ui_gadget (0x581e08):
//   0 dtor 0x553530 | 1 send_event 0x52c330 | 2 set_disabled 0x52c890 | 3 set_enabled 0x52c8c0 |
//   4 is_disabled 0x52c530 | 5 process 0x5534f0 | 6 get_mask_pixel 0x5537a0 | 7 draw 0x553800 |
//   8 is_selectable 0x52c880
// No user-declared destructor: the implicit one (~ui_gadget 0x52c410) is what the atexit handlers of the
// static containers run.
class ui_container : public ui_gadget {
public:
    ui_container();                                          // 0x553560
    virtual void dtor();                                     // slot 0, 0x553530
    virtual int  process(int key);                           // slot 5, 0x5534f0
    virtual int  get_mask_pixel(int x, int y);               // slot 6, 0x5537a0
    virtual void draw();                                     // slot 7, 0x553800

    void create(ui_element *window, int x, int y, int w, int h, int id, int flags,
                bool allocated);                             // 0x553590
    void set_tooltip(const char *text, int font, int x, int y, int delay);   // 0x5535f0
    void set_label(const char *label);                       // 0x553680
    void load_mask_bitmap(const char *name);                 // 0x5536b0
    void set_mask_bitmap(int bm, const char *name);          // 0x553710
    // D8403 (amends D4206): not inline; defined in the UI manager unit, whose ui_draw calls 0x530b70.
    ubyte *get_mask_buffer();                                // 0x530b70 ui_container__get_mask_buffer

    char   label[0x100];      // +0x274 $Label:, "" = no frame
    int    flags;             // +0x374 bit 0: draw the bevel when there is no label
    int    mask_bm;           // +0x378 -1 none
    ubyte *mask_data;         // +0x37C locked mask pixels, one byte each
    int    font;              // +0x380 -1 = default font
    Color *colors;            // +0x384 ui_container_colors: [0] dark, [1] light, [2] label, [3] label shadow
};
LAYOUT_CHECK(ui_container, sizeof(ui_container) == 0x388);

#endif
