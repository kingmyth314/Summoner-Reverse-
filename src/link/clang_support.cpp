// Link support for the clang build (tools/link-build.mjs --compiler clang; docs/future/modern-port.md, rung L1).
// clang emits operator new[], which VC6's runtime does not have: VC6 compiled new[] as a call of the plain operator.
// This forwards to it, so the game's own operator new still does the work. (operator delete[] is the game's own,
// vsdk/memory.cpp; the sized delete is switched off with -fno-sized-deallocation.)
#include <stddef.h>

void *operator new(size_t n);

void *operator new[](size_t n)
{
    return operator new(n);
}
