// engine/effects/effect_stream.h - the game's effect streamer (0x4404a0-0x440b20; unit and directory not
// known: it sits between summoner_cd.cpp and characterinfo.cpp). Called by vsdk\geom\effect_mem.cpp.
// Not original source.
#ifndef ENGINE_EFFECTS_EFFECT_STREAM_H
#define ENGINE_EFFECTS_EFFECT_STREAM_H

#include "vsdk/os/memory.h"

int  effect_stream_queue_add(const char *name, Mempool *pool, int arg); // 0x4404a0 (0 = refused)
int  effect_stream_queue_contains(const char *name);                    // 0x440510
void effect_stream_start();                                             // 0x440560
int  effect_stream_update();                                            // 0x4408a0 (1 = still reading)
void effect_stream_reset();                                             // 0x440b20

#endif
