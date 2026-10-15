// vsdk/os/mouse.h - mouse state (vsdk\os\mouse in sum.symbols, 0x5012c0-0x5013c0). Not original source.
#ifndef VSDK_OS_MOUSE_H
#define VSDK_OS_MOUSE_H

#include "vsdk/vsdk.h"

bool mouse_is_down(int button);                   // 0x5012c0
bool mouse_was_double_clicked(int button);        // 0x5012e0
bool mouse_was_pressed(int button);               // 0x501360
void mouse_get_delta(int *dx, int *dy, int *dz);  // 0x5013c0
extern int game_mouse_z;                          // 0x257ef80 (wheel)

#endif
