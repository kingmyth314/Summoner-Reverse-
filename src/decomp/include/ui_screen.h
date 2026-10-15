// ui_screen.h - the button records shared by the hand-made interface screens (new_*.cpp).
// Layouts observed in the *_load_bitmaps / *_handle_click / *_draw functions of Sum.exe v1.40.
// Struct and field names are ours (no symbol names exist for them).
#ifndef UI_SCREEN_H
#define UI_SCREEN_H

// One entry of a screen's static button table in .data (0x40 bytes).
struct ui_button_def {
    char name[0x24];      // +0x00 bitmap base name; "640.tga"/"1024.tga" is appended unless it has a '.'
    int  pos[3][2];       // +0x24 x, y for each resolution_index (640, 800, 1024)
    int  flags;           // +0x3c screen-specific: which entries react to the mouse (1, 2 or 4)
};

// Runtime state of one button (0x18 bytes), filled by *_load_bitmaps.
struct ui_button_state {
    int flags;            // +0x00 1 always drawn, 2 highlighted / pressed this frame, 4 disabled
    int bitmap;           // +0x04 bm handle, -1 none
    int x, y;             // +0x08 / +0x0c
    int w, h;             // +0x10 / +0x14 bitmap size times ui_get_scale()
};

#endif
