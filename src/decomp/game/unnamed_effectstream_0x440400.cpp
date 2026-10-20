// unnamed_effectstream_0x440400.cpp - Sum.exe v1.40, code 0x440400-0x440b3a: the effect streamer. A queue
// of up to 100 files (.rfx effect bases, .mvf skeletons, .vim meshes; .peg queues nothing to read) is read
// one at a time with asynchronous File reads, polled once per frame by GameTick (effect_stream_update).
// THE FILE NAME IS A PLACEHOLDER: the original .cpp name of this unit is not known (no assert path, no
// strings.tbl block, sum.symbols module "-"; docs/sum-exe/source-map.md "Hand cuts inside the unnamed
// units"). Rename the file when the name turns up. Header: engine/effects/effect_stream.h.
// Written from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact (control flow, calls, constants and globals match), likely (same, but one
// detail of statement order or expression form is a guess), unsure (the shape itself is a guess).
// Data: .data 0x59513c-0x595157 (effect_stream_base = -1, then ".vim", ".rfx", ".peg"; ".mvf" 0x594e18 is
// a pooled copy, /Gf), .bss 0x60fe30-0x6513a7. Only effect_base_queue_stream (effect_mem.cpp) queues
// files, always .rfx names with (pool, NULL target), so the .mvf / .vim cases never run in v1.40.

#include <string.h>
#include "vsdk/os/memory.h"
#include "vsdk/vfile/file.h"
#include "vsdk/geom/effect_mem.h"
#include "vsdk/ca/skeleton.h"
#include "engine/effects/effect_stream.h"

#define EFFECT_FILE_PEG 0
#define EFFECT_FILE_RFX 1
#define EFFECT_FILE_MVF 2
#define EFFECT_FILE_VIM 3
#define EFFECT_HEADER_SIZE 0x14       // an .rfx file's header: the base's data follows it

// ---- .data (0x59513c) ----------------------------------------------------------------------------------
int effect_stream_base = -1;                                  // 0x59513c

// ---- .bss (0x60fe30) -----------------------------------------------------------------------------------
ubyte effect_stream_buffer[EFFECT_STREAM_BUFFER_SIZE];        // 0x60fe30
Mempool *effect_stream_saved_mempool;                         // 0x64fe30
effect_stream_entry effect_stream_queue[EFFECT_STREAM_QUEUE_SIZE];   // 0x64fe38
// 0x440400  sinit_file_651288 (call init, jmp register; static_initializers entry 153). Confidence: exact
// (compiler-generated).
// 0x440410  init_file_651288 (mov ecx, 0x651288; jmp File::File). Confidence: exact (compiler-generated).
// 0x440420  register_dtor_file_651288 (atexit(0x440430)). Confidence: exact (compiler-generated).
// 0x440430  dtor_file_651288 (mov ecx, 0x651288; jmp File::~File). Confidence: exact (compiler-generated).
File effect_stream_file;                                      // 0x651288
int effect_stream_state;                                      // 0x65139c
int effect_stream_queue_count;                                // 0x6513a0
int effect_stream_cur;                                        // 0x6513a4

// 0x440440  Confidence: exact (VC6 byte match, match24; was likely) (the four out values are locals whose order is a guess).
void effects_read_all_bases()
{
    int num_nodes;
    int table_size;
    int size;
    int version;
    int i;

    for (i = 0; i < effects_get_num_bases(); i++)
        effect_base_read(i, &num_nodes, &table_size, &size, &version);
}

// 0x440480  Confidence: exact (GameInit).
void effect_stream_init()
{
    effect_stream_queue_count = 0;
    effects_read_all_bases();
    effect_stream_state = EFFECT_STREAM_IDLE;
}

// 0x4404a0  Confidence: exact (VC6 byte match, match24; was likely) (the target is stored before the strncpy call, the pool after it; that is
// the source order, a store does not move across the call). Only while idle or reading.
int effect_stream_queue_add(const char *name, Mempool *pool, void *target)
{
    if (effect_stream_state != EFFECT_STREAM_IDLE && effect_stream_state != EFFECT_STREAM_READING)
        return 0;
    if (effect_stream_queue_count >= EFFECT_STREAM_QUEUE_SIZE)
        return 0;
    if (!name)
        return 0;
    effect_stream_queue[effect_stream_queue_count].target = target;
    strncpy(effect_stream_queue[effect_stream_queue_count].name, name, EFFECT_STREAM_NAME_LEN);
    effect_stream_queue[effect_stream_queue_count++].pool = pool;
    return 1;
}

// 0x440510  Confidence: exact.
int effect_stream_queue_contains(const char *name)
{
    int i;

    for (i = 0; i < effect_stream_queue_count; i++) {
        if (_stricmp(effect_stream_queue[i].name, name) == 0)
            return 1;
    }
    return 0;
}

// 0x440560  Confidence: exact (VC6 byte match, D16804: one `return 1` after nested ifs, so the READING store is
// not merged with the return value; 8966 copies the return into each path). effect_stream_next never returns a negative value, so its test is dead.
int effect_stream_start()
{
    if (effect_stream_state != EFFECT_STREAM_IDLE)
        return 0;
    effect_stream_cur = -1;
    if (effect_stream_queue_count <= 0)
        effect_stream_state = EFFECT_STREAM_DONE;
    else {
        if (effect_stream_next() < 0)
            return 0;
        if (effect_stream_cur >= effect_stream_queue_count)
            effect_stream_state = EFFECT_STREAM_DONE;
        else {
            if (!effect_stream_open_current())
                return 0;
            effect_stream_state = EFFECT_STREAM_READING;
        }
    }
    return 1;
}

// 0x4405e0  Confidence: exact. Skips the .rfx bases that are already resident; 1 (not the cursor) when
// the queue is done.
int effect_stream_next()
{
    for (effect_stream_cur++; effect_stream_cur < effect_stream_queue_count; effect_stream_cur++) {
        if (file_type_from_ext(effect_stream_queue[effect_stream_cur].name) != EFFECT_FILE_RFX
            || !effects_base_is_resident(effect_stream_queue[effect_stream_cur].name))
            return effect_stream_cur;
    }
    return 1;
}

// 0x440650  Confidence: exact (the last test is neg / sbb / and -4 / add 3).
int file_type_from_ext(const char *name)
{
    char *ext;

    if (!name)
        return -1;
    ext = file_get_ext(name);
    if (_stricmp(ext, ".peg") == 0)
        return EFFECT_FILE_PEG;
    if (_stricmp(ext, ".rfx") == 0)
        return EFFECT_FILE_RFX;
    if (_stricmp(ext, ".mvf") == 0)
        return EFFECT_FILE_MVF;
    return _stricmp(ext, ".vim") ? -1 : EFFECT_FILE_VIM;
}

// 0x4406d0  Confidence: exact (VC6 byte match, match14; was likely) (jump table on type + 1; the shared failure tail closes the file only when it
// is open). An .rfx base is read straight into its own buffer behind the 0x14-byte header; an .mvf / .vim
// file (at most 0x40000 bytes) into effect_stream_buffer. A .peg starts a read of nothing.
int effect_stream_open_current()
{
    ubyte *buf = NULL;
    int size = 0;

    switch (file_type_from_ext(effect_stream_queue[effect_stream_cur].name)) {
    case -1:
        return 0;
    case EFFECT_FILE_RFX:
        if (effect_stream_file.open(effect_stream_queue[effect_stream_cur].name, 1, PATH_ANY))
            goto none;
        effect_stream_push_mempool(effect_stream_queue[effect_stream_cur].pool);
        effect_stream_base = effects_find_base(effect_stream_queue[effect_stream_cur].name);
        if (effect_stream_base < 0) {
            effect_stream_pop_mempool();
            goto fail;
        }
        size = effect_stream_file.size(NULL, PATH_ANY) - EFFECT_HEADER_SIZE;
        buf = (ubyte *)effect_base_alloc_stream_buffer(effect_stream_base);
        if (buf == NULL) {
            effect_stream_pop_mempool();
            goto fail;
        }
        effect_stream_pop_mempool();
        effect_stream_file.seek(EFFECT_HEADER_SIZE, 0);
        break;
    case EFFECT_FILE_MVF:
    case EFFECT_FILE_VIM:
        if (effect_stream_file.open(effect_stream_queue[effect_stream_cur].name, 1, PATH_ANY))
            return 0;
        size = effect_stream_file.size(NULL, PATH_ANY);
        if (size > EFFECT_STREAM_BUFFER_SIZE)
            goto fail;
        buf = effect_stream_buffer;
        break;
    }
    effect_stream_file.read(buf, size, 0, 1);
    return 1;

fail:
    if (effect_stream_file.is_open())
        effect_stream_file.close();
none:
    return 0;
}

// 0x440860  Confidence: exact.
void effect_stream_push_mempool(Mempool *pool)
{
    effect_stream_saved_mempool = get_current_mempool();
    set_current_mempool(pool);
}

// 0x440880  Confidence: exact.
void effect_stream_pop_mempool()
{
    set_current_mempool(effect_stream_saved_mempool);
    effect_stream_saved_mempool = NULL;
}

// 0x4408a0  Confidence: exact (VC6 8966 byte match with each case storing the state and one return after the switch, m1019) (the 1 / 2 choice after the next open is neg / sbb / add 2). The state.
int effect_stream_update()
{
    if (effect_stream_state != EFFECT_STREAM_READING)
        return effect_stream_state;
    switch (effect_stream_file.get_async_status()) {
    case 1:
        if (!effect_stream_finish_entry())
            effect_stream_state = EFFECT_STREAM_ERROR;
        else if (effect_stream_cur >= effect_stream_queue_count)
            effect_stream_state = EFFECT_STREAM_DONE;
        else
            effect_stream_state = effect_stream_open_current() ? EFFECT_STREAM_READING : EFFECT_STREAM_ERROR;
        break;
    case 0:
        effect_stream_state = EFFECT_STREAM_READING;
        break;
    case -1:
        effect_stream_file.close();
        effect_stream_state = EFFECT_STREAM_ERROR;
        break;
    }
    return effect_stream_state;
}

// 0x440930  Confidence: exact (VC6 byte match, D16801: the `case -1` written last, so its return 0 is
// merged with the error tail after the switch; jump table on type + 1; the .vim case shares the .mvf
// case's tail). The read data goes to its owner: the .rfx base is relocated (effect_base_finish_stream),
// an .mvf is parsed into the target skeleton, a .vim loaded into *target, each with the entry's mempool
// current.
int effect_stream_finish_entry()
{
    FileReader reader;
    int size;

    switch (file_type_from_ext(effect_stream_queue[effect_stream_cur].name)) {
    case EFFECT_FILE_RFX:
        effect_stream_file.close();
        if (effect_stream_base < 0)
            return 0;
        effect_stream_push_mempool(effect_stream_queue[effect_stream_cur].pool);
        effect_base_finish_stream(effect_stream_base);
        effect_stream_pop_mempool();
        effect_stream_base = -1;
        break;
    case EFFECT_FILE_MVF:
        size = effect_stream_file.size(NULL, PATH_ANY);
        effect_stream_file.close();
        if (!effect_stream_queue[effect_stream_cur].target)
            return 0;
        effect_stream_push_mempool(effect_stream_queue[effect_stream_cur].pool);
        reader.attach_memory(effect_stream_buffer, size);
        ((skeleton *)effect_stream_queue[effect_stream_cur].target)->read(&reader, NULL, -1);
        effect_stream_pop_mempool();
        break;
    case EFFECT_FILE_VIM:
        size = effect_stream_file.size(NULL, PATH_ANY);
        effect_stream_file.close();
        if (!effect_stream_queue[effect_stream_cur].target)
            return 0;
        effect_stream_push_mempool(effect_stream_queue[effect_stream_cur].pool);
        reader.attach_memory(effect_stream_buffer, size);
        *(vim **)effect_stream_queue[effect_stream_cur].target = vim_load_file(&reader, -1);
        if (*(vim **)effect_stream_queue[effect_stream_cur].target == NULL) {
            effect_stream_pop_mempool();
            return 0;
        }
        effect_stream_pop_mempool();
        break;
    case -1:
        return 0;
    }
    if (effect_stream_next() < 0) {
        effect_stream_state = EFFECT_STREAM_ERROR;
        return 0;
    }
    return 1;
}

// 0x440b20  Confidence: exact.
void effect_stream_reset()
{
    effect_stream_queue_count = 0;
    effect_stream_state = EFFECT_STREAM_IDLE;
}

// 0x440b30  Confidence: exact (VC6 byte match, match14; was exact) (handle_slots_reset): reads the rest of the queue synchronously.
void effect_stream_flush()
{
    do {
    } while (effect_stream_update() == EFFECT_STREAM_READING);
    effect_stream_reset();
}
