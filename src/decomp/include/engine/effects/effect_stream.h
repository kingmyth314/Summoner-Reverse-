// engine/effects/effect_stream.h - the game's effect streamer (0x440400-0x440b3a; source
// game/unnamed_effectstream_0x440400.cpp, decomp53). THE FILE NAME IS NOT KNOWN (no assert path, no
// strings.tbl block); it lies between the state 6 / monster layout unit and the net session unit.
// Directory ours. Called by vsdk\geom\effect_mem.cpp, GameInit and GameTick. Not original source.
#ifndef ENGINE_EFFECTS_EFFECT_STREAM_H
#define ENGINE_EFFECTS_EFFECT_STREAM_H

#include "vsdk/os/memory.h"
#include "vsdk/vfile/file.h"

#define EFFECT_STREAM_QUEUE_SIZE  100
#define EFFECT_STREAM_NAME_LEN    40          // strncpy length; the field is 0x2c bytes
#define EFFECT_STREAM_BUFFER_SIZE 0x40000     // the .mvf / .vim read buffer

// effect_stream_state
#define EFFECT_STREAM_IDLE    0
#define EFFECT_STREAM_READING 1
#define EFFECT_STREAM_ERROR   2
#define EFFECT_STREAM_DONE    3

// One queued file (0x34 bytes; sum.symbols effect_stream_queue: file name +0, target +0x2c, mempool +0x30).
// Type and field names ours.
struct effect_stream_entry {
    char name[0x2c];                  // +0x00
    void *target;                     // +0x2c .mvf: the skeleton to read into; .vim: where the vim * goes
    Mempool *pool;                    // +0x30 current mempool while the entry is read
};
LAYOUT_CHECK(effect_stream_entry, sizeof(effect_stream_entry) == 0x34);

// .data
extern int effect_stream_base;                                          // 0x59513c -1, the .rfx base being read
// .bss
extern ubyte effect_stream_buffer[EFFECT_STREAM_BUFFER_SIZE];           // 0x60fe30
extern Mempool *effect_stream_saved_mempool;                            // 0x64fe30
extern effect_stream_entry effect_stream_queue[EFFECT_STREAM_QUEUE_SIZE];   // 0x64fe38
extern File effect_stream_file;                                         // 0x651288
extern int effect_stream_state;                                         // 0x65139c
extern int effect_stream_queue_count;                                   // 0x6513a0
extern int effect_stream_cur;                                           // 0x6513a4

void effects_read_all_bases();                                          // 0x440440
void effect_stream_init();                                              // 0x440480 (GameInit)
// (name, mempool, target): 0 = refused (busy, full or no name). decomp53 (D3300): the third argument is
// the target (+0x2c), declared int before; effect_base_queue_stream passes (name, pool, 0).
int  effect_stream_queue_add(const char *name, Mempool *pool, void *target);   // 0x4404a0
int  effect_stream_queue_contains(const char *name);                    // 0x440510
// 0 when busy or the first file cannot be opened, else 1 (D3300: returned void before).
int  effect_stream_start();                                             // 0x440560
int  effect_stream_next();                                              // 0x4405e0 the cursor, 1 past the end
// ".peg" 0, ".rfx" 1, ".mvf" 2, ".vim" 3, else -1.
int  file_type_from_ext(const char *name);                              // 0x440650
int  effect_stream_open_current();                                      // 0x4406d0 1 = the read is started
void effect_stream_push_mempool(Mempool *pool);                         // 0x440860
void effect_stream_pop_mempool();                                       // 0x440880
int  effect_stream_update();                                            // 0x4408a0 the state (1 = still reading)
int  effect_stream_finish_entry();                                      // 0x440930
void effect_stream_reset();                                             // 0x440b20
void effect_stream_flush();                                             // 0x440b30 (handle_slots_reset)

#endif
