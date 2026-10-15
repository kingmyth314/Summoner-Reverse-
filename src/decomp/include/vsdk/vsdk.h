// vsdk/vsdk.h - basic types and helpers shared by every header of src/decomp/include/ (ours; no
// original header of this name is known). Every other header includes it first.
//
// Not original source: written for the Phase 3 pilot from Sum.exe v1.40 (build/sum.asm, tools/asm.mjs).
// Layout scheme of src/decomp/include/: see include/README.md.
#ifndef VSDK_VSDK_H
#define VSDK_VSDK_H

// The C runtime is VC6's static CRT (0x56a000-0x57dfff); its own headers are used.
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>   // int8_t..uint32_t as in symbols/types.h (VC6 has no stdint.h: the real build needs a shim)

typedef unsigned char  ubyte;
typedef unsigned short ushort;
typedef unsigned int   uint;

// Compile-time layout check. VC6 has no static_assert; a negative array size fails the build.
#define LAYOUT_CHECK(name, cond) typedef char layout_check_##name[(cond) ? 1 : -1]

// windef.h style min / max: the game's code has the setle / dec / and pattern of `a > b ? a : b`.
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

// Circular doubly linked lists through next / prev (the head is an object of the element type).
// Used by player.cpp; the free loops of other units look like these macros too.
#define list_init(head)          { (head)->next = (head); (head)->prev = (head); }
#define list_insert(head, elem)  { (elem)->next = (head)->next; (head)->next->prev = (elem); \
                                   (elem)->prev = (head); (head)->next = (elem); }
#define list_append(head, elem)  { (elem)->prev = (head)->prev; (elem)->next = (head); \
                                   (head)->prev->next = (elem); (head)->prev = (elem); }
#define list_remove(elem)        { (elem)->prev->next = (elem)->next; (elem)->next->prev = (elem)->prev; \
                                   (elem)->next = NULL; (elem)->prev = NULL; }

#endif
