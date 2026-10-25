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
// int8_t..uint32_t as in symbols/types.h. VC6 has no stdint.h, so the typedefs are spelled out for it
// (DECISIONS.md D4900); every other compiler uses its own header.
#if defined(_MSC_VER) && _MSC_VER < 1600
typedef signed char      int8_t;
typedef unsigned char    uint8_t;
typedef short            int16_t;
typedef unsigned short   uint16_t;
typedef int              int32_t;
typedef unsigned int     uint32_t;
typedef __int64          int64_t;
typedef unsigned __int64 uint64_t;
typedef int              intptr_t;
typedef unsigned int     uintptr_t;
#else
#include <stdint.h>
#endif

typedef unsigned char  ubyte;
typedef unsigned short ushort;
typedef unsigned int   uint;

// A class with a non-virtual destructor that nothing derives from, deleted through its own type: final in the modern
// build (docs/future/modern-port.md, L2), so clang knows the delete runs the right destructor. VC6 has no final.
#ifdef MODERN_BUILD
#define MODERN_FINAL final
#else
#define MODERN_FINAL
#endif

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
// Used by player.cpp; the free loops of other units look like these macros too. The bodies are
// do { } while (0): the unoptimised objects.cpp keeps the `xor reg, reg / test / jne` of the while (0)
// after each expansion (obj_link_new 0x469c12, obj__deinit 0x469e5d, objects_init 0x469918;
// DECISIONS.md D59). Optimised units drop the test, so their code is unchanged.
#define list_init(head)          do { (head)->next = (head); (head)->prev = (head); } while (0)
#define list_insert(head, elem)  do { (elem)->next = (head)->next; (head)->next->prev = (elem); \
                                      (elem)->prev = (head); (head)->next = (elem); } while (0)
#define list_append(head, elem)  do { (elem)->prev = (head)->prev; (elem)->next = (head); \
                                      (head)->prev->next = (elem); (head)->prev = (elem); } while (0)
#define list_remove(elem)        do { (elem)->prev->next = (elem)->next; (elem)->next->prev = (elem)->prev; \
                                      (elem)->next = NULL; (elem)->prev = NULL; } while (0)

// The same lists with a pointer as the head (NULL = empty): objects.cpp's handle slot lists
// (obj_free_list, obj_used_list). Names ours. plist_remove unlinks the last element with a `break`
// out of the while (0) (obj_create 0x469cb1 jumps past the test).
#define plist_init(head)         do { (head) = NULL; } while (0)
#define plist_append(head, elem) do { if ((head) == NULL) { (elem)->next = (elem); (elem)->prev = (elem); \
                                          (head) = (elem); } \
                                      else { (elem)->prev = (head)->prev; (elem)->next = (head); \
                                          (head)->prev->next = (elem); (head)->prev = (elem); } } while (0)
#define plist_remove(head, elem) do { if ((head) == (elem)) { \
                                          if ((elem)->next == (elem)) { (elem)->next = NULL; (elem)->prev = NULL; \
                                              (head) = NULL; break; } \
                                          (head) = (elem)->next; } \
                                      (elem)->prev->next = (elem)->next; (elem)->next->prev = (elem)->prev; \
                                      (elem)->next = NULL; (elem)->prev = NULL; } while (0)

#endif
