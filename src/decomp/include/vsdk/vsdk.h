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

// The 32-bit clang build compiles with VC6's stdarg.h, whose va_start takes the address of the last named
// parameter; clang does not keep that parameter where the macro expects in a static variadic function (agent mode's
// output and the crash handler's hprintf came out wrong). clang's builtins give the same x86 va_list (a char
// pointer) that VC6's runtime reads. stdarg.h is included first, so its include guard keeps these.
#if defined(MODERN_BUILD) && !defined(_WIN64)
#include <stdarg.h>
#undef va_start
#undef va_end
#define va_start(ap, v) __builtin_va_start(ap, v)
#define va_end(ap) __builtin_va_end(ap)
#endif

// The handle of _findfirst / _findnext: long in the 32-bit builds (VC6's io.h), intptr_t in a 64-bit build, where a
// long cuts the handle in half and _findnext then crashes (L3).
#ifdef _WIN64
#include <stddef.h>
typedef intptr_t find_handle;
#else
typedef long find_handle;
#endif

// A WinSock SOCKET: unsigned int in the 32-bit builds (the original's declarations), pointer-sized in a 64-bit build,
// where an unsigned int never equals INVALID_SOCKET and a failed socket() goes unnoticed (L3).
#ifdef _WIN64
typedef unsigned __int64 net_socket;
#else
typedef unsigned int net_socket;
#endif

// A pointer member of a structure that is also the layout of file data (the effect, animation and skeleton files,
// loaded as one block and relocated in place): 4 bytes in every build (L3). The 64-bit exe is linked without
// large-address awareness, so every address the game uses lies below 2 GB and fits; clang's __ptr32 converts to and
// from an ordinary pointer. MinGW's _mingw.h (included by stddef.h above) defines __ptr32 away; it is restored here.
// Where an ordinary pointer is required (delete), a cast to the field's own type gives one.
#ifdef _WIN64
#undef __ptr32
#define PTR32 __ptr32 __uptr
#else
#define PTR32
#endif

// The byte size of a fixed memory pool (a static buffer behind a Mempool or NoRlsAllocPool): as the original in the
// 32-bit builds, doubled in a 64-bit build, whose objects with pointers are larger (none more than twice) (L3).
#ifdef _WIN64
#define POOL64(n) ((n) * 2)
#else
#define POOL64(n) (n)
#endif

// Compile-time layout check. VC6 has no static_assert; a negative array size fails the build. LAYOUT_CHECK holds in
// every build: the structure is a file or network format, or its fields are PTR32. LAYOUT_CHECK_32 holds in the
// 32-bit builds only: a layout that lives in memory only, which a 64-bit build may lay out with 8-byte pointers (L3).
#define LAYOUT_CHECK(name, cond) typedef char layout_check_##name[(cond) ? 1 : -1]
#ifdef _WIN64
#define LAYOUT_CHECK_32(name, cond) typedef char layout_check_##name[1]
#else
#define LAYOUT_CHECK_32(name, cond) LAYOUT_CHECK(name, cond)
#endif

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
