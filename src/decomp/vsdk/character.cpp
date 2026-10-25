// character.cpp - Sum.exe v1.40, code 0x540810-0x541952 (vsdk\ca\character.cpp): the base characters
// (ca_characters[200]), their skeletons (animations) and meshes, the mesh slot table and the per-level
// preload index; then the compiler-generated helpers this object emitted (0x541960-0x54199b). Written
// from the Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout or
// scheduling risk, unsure = the source shape is a guess.
//
// Build: optimised, no inline expansion (Mempool::ResetOffset is called, 0x505540), intrinsic strcpy /
// strcat / strcmp (rep movs, inline compare loops), /GX (new of skeleton and the stack File objects get
// EH frames).
//
// A character is created from a list of mesh names (names[0] the full mesh, names[1] a LOD mesh): each
// gets a 0x30-byte mesh slot. Only the first mesh is loaded (load_shadow: name.vim and, unless the
// char_info says otherwise, name_shadow.vim); reload_mesh swaps to another LOD later, reusing the
// texture handles. Skeletons come from the level's preload index (one file read whole into
// ca_preload_data) or from their own .mvf files.
//
// Source order: the error lines (470 create_base, 641 / 692 add_skeleton, 1278 character_load) do not
// follow the code order; the functions are written in address order (NOTES.md).

#include <string.h>
#include <stdio.h>
#include "vsdk/ca/character.h"
#include "vsdk/ca/skeleton.h"
#include "vsdk/vfile/file.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/console.h"
#include "vsdk/os/stringpool.h"
#include "engine/characterinfo/characterinfo.h"

#define CHARACTER_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\ca\\character.cpp"

// The stripped name buffer of character_create_base: "" then 40 zero bytes (`mov al, ""; rep stosd`
// x10), 44 with the stack alignment (as skeleton.cpp's VIM_NAME_LEN).
#define CA_NAME_LEN 41

// Trigger times are given in frames: frames -> seconds -> ticks (fmul 1/30, fmul 30, fmul 160).
// Frames to ticks: frame * (1/30) * 30 * 160, three separate fmuls in the binary (not folded). The two
// users spell it with float locals, which is what makes VC6 12.00.8168 keep the three multiplies (D6800);
// the old macro CA_FRAMES_TO_TICKS (D4903) folded two of them.

#define CA_DEFAULT_POOL_SIZE 0x1400

// The object at 0x2ee22f8 (0x1f8 bytes up to ca_num_characters): a class whose only member (or base)
// is a ca_character. Its constructor and destructor are compiler-generated (0x541970 / 0x541960 at the
// end of the unit), so it has none of its own. No code references it. Type name from sum.symbols'
// ca_character_holder__ctor; object name ours.
struct ca_character_holder {
    ca_character c;
};

// ---- .data (0x5ac0e0-0x5ac367) ------------------------------------------------------------------------
// Literals only, one copy per use: the four error texts with a path each, "Error opening : %s\n" twice,
// "_shadow.vim", the shadow messages, the memory report labels with their own "%d", "RE-LOADING ...".
// The "p" at 0x5ac350 has no reference.

// ---- .bss ---------------------------------------------------------------------------------------------
// Addresses: ca_string_pool 0x2dc7070, ca_characters 0x2dc7088, ca_preload_slots 0x2ddfa48,
// ca_preload_loaded .. ca_preload_size 0x2de20b8-0x2de20c0, ca_preload_data 0x2de20c4, the preload
// offsets / sizes 0x2ee20c4 / 0x2ee21dc, 0x2ee22f4 (no reference), the holder 0x2ee22f8, the five
// counters 0x2ee24f0-0x2ee2500, the "" literals 0x2ee2504 / 0x2ee2508 (report_memory, create_base).
// The definitions are in initialiser order (initialiser table 0x5889f0-0x5889f8: ca_characters, the
// holder, ca_string_pool), which is not the address order.

// 0x540810  Confidence: likely (generated: initialiser thunk, table entry 0x5889f0)
// 0x540820  Confidence: likely (generated: eh vector constructor over ca_characters)
// 0x540840  Confidence: likely (generated: atexit(0x540850))
// 0x540850  Confidence: likely (generated: eh vector destructor over ca_characters)
ca_character ca_characters[CA_MAX_CHARACTERS];

// 0x540870  Confidence: likely (generated: initialiser thunk, table entry 0x5889f4)
// 0x540880  Confidence: likely (generated: ca_character_holder() on 0x2ee22f8)
// 0x540890  Confidence: likely (generated: atexit(0x5408a0))
// 0x5408a0  Confidence: likely (generated: ~ca_character_holder() on 0x2ee22f8)
ca_character_holder ca_character_2ee22f8;

// 0x5408b0  Confidence: likely (generated: initialiser thunk, table entry 0x5889f8)
// 0x5408c0  Confidence: likely (generated: StringPool() on ca_string_pool)
// 0x5408d0  Confidence: likely (generated: atexit(0x5408e0))
// 0x5408e0  Confidence: likely (generated: ~StringPool() on ca_string_pool)
StringPool ca_string_pool;

ca_mesh ca_preload_slots[CA_MAX_MESHES];
int ca_preload_loaded;
int ca_preload_count;
int ca_preload_size;
ubyte ca_preload_data[CA_PRELOAD_DATA_SIZE];
ca_preload_table ca_preload_tbl;     // 0x2ee20c4 offsets, 0x2ee21dc sizes (D11801)

// Set by ca_init like skeleton.cpp's five counters (count, limit -1, used, peak, limit -1); only
// ca_num_characters is used. Names ours except ca_num_characters.
int ca_num_characters;                // 0x2ee24f0
int ca_num_characters_limit;          // 0x2ee24f4
int ca_mem_used;                      // 0x2ee24f8
int ca_mem_peak;                      // 0x2ee24fc
int ca_mem_limit;                     // 0x2ee2500

// 0x2ee45ac: after skeleton.cpp's .bss (0x2ee4524-0x2ee45a8); only add_skeleton writes it, nothing
// reads it. Defined here because this unit is its only user (placement open, NOTES.md).
ca_character *ca_current_character;

// ---- ca_vertex_weights (no caller) -------------------------------------------------------------------

// 0x5408f0  Confidence: exact (returns this, so a constructor; sum.symbols ca_vertex_weights__clear)
ca_vertex_weights::ca_vertex_weights()
{
    bones[0] = 0;
    bones[1] = 0;
    bones[2] = 0;
    bones[3] = 0;
}

// 0x540900  Confidence: exact (a bare ret right after the constructor; taken as the empty destructor)
ca_vertex_weights::~ca_vertex_weights()
{
}

// 0x540910  Confidence: exact. Full: nothing happens.
void ca_vertex_weights::add(ubyte bone, ubyte weight)
{
    for (int i = 0; i < 4; i++) {
        if (bones[i] == 0) {
            bones[i] = bone;
            weights[i] = weight;
            return;
        }
    }
}

// 0x540940  Confidence: exact (thiscall, two stack arguments it does not read, ret 8, no result).
void ca_vertex_weights::clear_b(int a, int b)
{
    bones[0] = 0;
    bones[1] = 0;
    bones[2] = 0;
    bones[3] = 0;
}

// ---- ca_character -------------------------------------------------------------------------------------

// 0x540950  Confidence: exact
ca_character::ca_character()
{
}

// 0x540960  Confidence: exact
ca_character::~ca_character()
{
}

// 0x540970  Confidence: likely (EH state numbering: new-expression 0, File 1; the order of the two
// error branches; D21201: the frame; D25800: the dup hit leaves through `goto found` to the one `return i`
// after the creation block, and the slot store goes through `int n = num_anims`, so the main path falls into
// the epilogue as in the binary; left: 8966 computes the main result into the dup index's register, the
// binary keeps the two results apart). check_dup: a skeleton of the same file is reused. The skeleton comes from the
// preload index when it is there and smaller than the whole preload buffer, else from its file.
// filename is also the name skeleton::read gets; name is the anim name.
int ca_character::add_skeleton(const char *filename, const char *name, bool is_state, int unused,
                               bool check_dup, bool root_rotation)
{
    int i;

    ca_current_character = this;
    if (num_anims == CA_MAX_SKELETONS) {
        sprintf(string_buffer, "No more room for skeletons in character %s -- see allender\n", this->name);
        VSDK_ERROR(CHARACTER_CPP, 641, string_buffer);
    }
    if (check_dup == true) {
        for (i = 0; i < num_anims; i++) {
            if (_stricmp(skeletons[i]->filename, filename) == 0)
                goto found;   // D25800
        }
    }

    {   // D25800: the creation in a block (the goto may not skip its initialisations)
        skeleton *s = new skeleton(filename, name, is_state, unused, root_rotation);
        FileReader reader;
        File f;
        {   // D21201: offset / size in a block after the new, so they share the new-expression's
            // temporary slot as in the binary (frame 0x1b0, was 0x1b4)
            int offset;
            int size;

            if (ca_preload_loaded && ca_preload_find(filename, &offset, &size) && size < CA_PRELOAD_DATA_SIZE) {
                reader.attach_memory(ca_preload_data + offset, size);
                s->read(&reader, filename, PATH_ANY);
            } else {
                if (!f.open(filename, 1, PATH_ANY)) {
                    reader.attach_file(&f);
                    s->read(&reader, filename, PATH_ANY);
                    f.close();
                } else {
                    char buf[0x80];
                    sprintf(buf, "Error opening : %s\n", filename);
                    VSDK_ERROR(CHARACTER_CPP, 692, buf);
                }
            }
        }
        int n = num_anims;
        skeletons[n] = s;
        num_anims++;
        i = num_anims - 1;
    }
found:
    return i;
}

// 0x540b80  Confidence: exact (the count store is duplicated into both paths by the compiler).
void ca_character::free_skeletons()
{
    if (!(flags & CA_IN_MEMPOOL)) {
        for (int i = 0; i < num_anims; i++)
            delete skeletons[i];
    }
    num_anims = 0;
}

// 0x540bd0  Confidence: exact (num_anims-- or num_anims = index: both store index here).
void ca_character::free_skeleton(int index)
{
    if (!(flags & CA_IN_MEMPOOL))
        delete skeletons[index];
    skeletons[index] = NULL;
    if (index == num_anims - 1)
        num_anims--;
}

// 0x540c10  Confidence: exact
void ca_character::lock_skeleton_count()
{
    if (!(flags & CA_SKELETONS_LOCKED)) {
        flags |= CA_SKELETONS_LOCKED;
        saved_num_anims = num_anims;
    }
}

// 0x540c30  Confidence: exact. Frees the skeletons added since lock_skeleton_count.
void ca_character::unlock_skeleton_count()
{
    if (flags & CA_SKELETONS_LOCKED) {
        for (int i = saved_num_anims; i < num_anims; i++) {
            if (!(flags & CA_IN_MEMPOOL))
                delete skeletons[i];
            skeletons[i] = NULL;
        }
        num_anims = saved_num_anims;
        flags &= ~CA_SKELETONS_LOCKED;
    }
}

// 0x540c80  Confidence: exact. First free of the five trigger slots; nothing when all are used.
void ca_character::add_skeleton_trigger(int anim, const char *name, float frame)
{
    skeleton *s = skeletons[anim];
    int i;

    for (i = 0; i < CA_NUM_TRIGGERS; i++) {
        if (s->triggers[i].name == NULL)
            break;
    }
    if (i == CA_NUM_TRIGGERS)
        return;
    s->triggers[i].name = ca_string_pool.AddUnique(name);
    float secs = frame * (1.0f / 30.0f);
    float frames = secs * 30.0f;
    s->triggers[i].time = (int)(frames * 160.0f);
}

// 0x540cf0  Confidence: exact. No reference found.
void ca_character::ensure_skeleton_trigger(int anim, const char *name, float frame)
{
    skeleton *s = skeletons[anim];
    int i;

    for (i = 0; i < CA_NUM_TRIGGERS; i++) {
        if (s->triggers[i].name != NULL && strcmp(s->triggers[i].name, name) == 0)
            break;
    }
    if (i == CA_NUM_TRIGGERS)
        add_skeleton_trigger(anim, name, frame);
}

// 0x540d70  Confidence: exact. No reference found. Game bug (in the binary, kept): the time is set on
// every named slot whose name DIFFERS from name (`test eax, eax / je` skips the equal ones at 0x540dbe),
// not on the slots of that name; the earlier source had `== 0` (D6800).
void ca_character::set_skeleton_trigger_time(int anim, const char *name, float frame)
{
    skeleton *s = skeletons[anim];

    for (int i = 0; i < CA_NUM_TRIGGERS; i++) {
        if (s->triggers[i].name != NULL && strcmp(s->triggers[i].name, name) != 0) {
            float secs = frame * (1.0f / 30.0f);
            float frames = secs * 30.0f;
            s->triggers[i].time = (int)(frames * 160.0f);
        }
    }
}

// 0x540df0  Confidence: exact (VC6 byte match, match23; was likely; the printf of the two shadow messages: if / else or a ternary).
// Loads the mesh name.vim into the slot and, unless the char_info of this character has flag 1, the
// shadow mesh name_shadow.vim.
bool ca_character::load_shadow(const char *name, ca_mesh *m)
{
    char buf[0x80];

    m->shadow = NULL;
    m->vim = vim_load(name, -1);
    m->skin = &m->vim->skin;
    int index = charinfo_find_by_name(this->name);
    if (index >= 0 && !(char_infos[index].flags & 1)) {
        file_strip_ext(buf, name);
        strcat(buf, "_shadow.vim");
        printf("ATTEMPTING TO LOAD : %s\n", buf);
        m->shadow = vim_load(buf, -1);
        if (m->shadow)
            printf("SHADOW LOADED!\n");
        else
            printf("SHADOW FAILED TO LOAD!\n");
    }
    flags |= CA_MESH_LOADED;
    return true;
}

// 0x540ed0  Confidence: exact (is_state compared with 1: `== true`).
int ca_character::find_state_skeleton(const char *name)
{
    for (int i = 0; i < num_anims; i++) {
        if (skeletons[i]->is_state == true && _stricmp(name, skeletons[i]->name) == 0)
            return i;
    }
    return -1;
}

// 0x540f20  Confidence: exact
int ca_character::find_action_skeleton(const char *name)
{
    for (int i = 0; i < num_anims; i++) {
        if (!skeletons[i]->is_state && _stricmp(name, skeletons[i]->name) == 0)
            return i;
    }
    return -1;
}

// 0x540f70  Confidence: exact. The bones of the current mesh by name, then its named extra bones
// (numbered after the bones).
int ca_character::find_mesh_tag(const char *name)
{
    int i;

    for (i = 0; i < mesh->skin->num_bones; i++) {
        if (_stricmp(name, mesh->skin->bones[i].name) == 0)
            return i;
    }
    for (i = 0; i < mesh->skin->num_ext; i++) {
        if (_stricmp(name, mesh->skin->ext[i].name) == 0)
            return mesh->skin->num_bones + i;
    }
    return -1;
}

// 0x541020  Confidence: exact (D21200: the two key counts through pointer locals). The sizes are the report's own: 0xa0 per bone,
// 0x38 per extra bone plus its name, per skeleton 0xd4 (object and pointer), 64 per position key, 36 per
// rotation key, 45 per bone (track and weight byte) for every non-NULL slot of the 100.
int ca_character::report_memory()
{
    int bones;
    int tags;
    int skel;
    int i;

    mem_report_indent();
    mem_report_line(this->name, "");
    mem_report_indent();
    bones = mesh->skin->num_bones * sizeof(ca_bone);
    mem_report_line("bones :", "%d", bones);
    mem_report_line("bone weights :", "%d", 0);
    ca_skin *skin = mesh->skin;
    tags = skin->num_ext * sizeof(ca_bone_ext);
    for (i = 0; i < skin->num_ext; i++) {
        if (skin->ext[i].name)
            tags += strlen(skin->ext[i].name);
    }
    mem_report_line("tags :", "%d", tags);
    skel = num_anims * (sizeof(skeleton) + sizeof(skeleton *));
    for (i = 0; i < CA_MAX_SKELETONS; i++) {
        skeleton *s = skeletons[i];
        if (s) {
            // D21200: the position and rotation counts each through a local and a pointer to it: 8966
            // then adds pos * 64 to skel first and loads the counts in the binary's order.
            int pk = s->num_pos_keys;
            int *pp = &pk;
            int rk = s->num_rot_keys;
            int *rp = &rk;
            skel += *pp * 64 + *rp * 36 + s->num_bones * 45;
        }
    }
    mem_report_line("skeleton :", "%d", skel);
    mem_report_unindent();
    int total = skel + tags + bones;
    mem_report_line("TOTAL :", "%d", total);
    mem_report_unindent();
    return total;
}

// 0x541160  Confidence: exact (the pool read before the vim store, D6800). Switches to LOD lod (0 or 1)
// when it has a slot and the character has a model pool: the pool is emptied and the new mesh loaded
// into it with the old mesh's texture handles.
void ca_character::reload_mesh(int lod)
{
    if (lod < 0 || lod >= CA_NUM_LODS)
        return;
    int cur = get_lod_index();
    if (cur == lod || lods[lod] == NULL || mesh == lods[lod] || pool == NULL)
        return;
    mesh = lods[lod];
    vim_save_bitmaps(lods[cur]->vim);
    Mempool *p = pool;
    lods[cur]->vim = NULL;
    p->ResetOffset();
    set_current_mempool(pool);
    load_shadow(lods[lod]->filename, lods[lod]);
    vim_restore_bitmaps(lods[lod]->vim);
    if (lods[lod]->shadow)
        vim_restore_bitmaps(lods[lod]->shadow);
    set_current_mempool(NULL);
}

// 0x541240  Confidence: exact
void ca_character::replace_texture(int index, const char *file)
{
    bm_replace_file(mesh->vim->textures[index], file, -1);
}

// 0x541270  Confidence: exact
int ca_character::get_lod_index()
{
    for (int i = 0; i < CA_NUM_LODS; i++) {
        if (mesh == lods[i])
            return i;
    }
    return -1;
}

// ---- the character table --------------------------------------------------------------------------

// 0x541290  Confidence: exact (store order of the counters around the compare).
void ca_init(int pool_size)
{
    int i;

    ca_num_characters = 0;
    ca_num_characters_limit = -1;
    ca_mem_used = 0;
    ca_mem_peak = 0;
    ca_mem_limit = -1;
    if (pool_size == -1)
        pool_size = CA_DEFAULT_POOL_SIZE;
    ca_string_pool.Allocate(pool_size);
    for (i = 0; i < CA_MAX_MESHES; i++)
        ca_preload_slots[i].owner = NULL;
    for (i = 0; i < CA_MAX_CHARACTERS; i++)
        character_clear(&ca_characters[i]);
}

// 0x541300  Confidence: exact (the LOD slots cleared by a loop: their own zero register, D6800).
void character_clear(ca_character *c)
{
    c->name[0] = '\0';
    c->num_anims = 0;
    c->_unk050 = 0;
    for (int i = 0; i < CA_NUM_LODS; i++)
        c->lods[i] = NULL;
    c->_unk1f0 = 0;
    c->flags = 0;
}

// 0x541330  Confidence: exact
void ca_close()
{
    for (int i = 0; i < CA_MAX_CHARACTERS; i++) {
        if (ca_characters[i].flags & CA_LOADED)
            character_unload(&ca_characters[i]);
    }
    ca_string_pool.Release();
}

// 0x541360  Confidence: exact. The shadow meshes are not freed.
void character_unload(ca_character *c)
{
    int i;

    ca_num_characters--;
    if (!(c->flags & CA_IN_MEMPOOL)) {
        for (i = 0; i < c->num_anims; i++)
            delete c->skeletons[i];
        for (i = 0; i < CA_NUM_LODS; i++) {
            ca_mesh *m = c->lods[i];
            if (m && m->vim) {
                vim_free(m->vim);
                m->vim = NULL;
            }
        }
    }
    for (i = 0; i < CA_NUM_LODS; i++) {
        if (c->lods[i])
            c->lods[i]->owner = NULL;
    }
    c->name[0] = '\0';
    character_clear(c);
}

// 0x541400  Confidence: exact
ca_character *ca_find_character(const char *name)
{
    for (int i = 0; i < CA_MAX_CHARACTERS; i++) {
        if (_stricmp(name, ca_characters[i].name) == 0)
            return &ca_characters[i];
    }
    return NULL;
}

// 0x541450  Confidence: exact. unused: never read.
ca_character *character_load(const char *const *names, int num_names, bool keep_loaded, int unused,
                             Mempool *model_pool, Mempool *char_pool)
{
    ca_character *c = ca_find_character(names[0]);

    if (c == NULL) {
        c = character_create_base(names, num_names, model_pool, char_pool);
        if (c == NULL) {
            sprintf(string_buffer, "Couldn't load character %s in charanim.tbl file.", names[0]);
            VSDK_ERROR(CHARACTER_CPP, 1278, string_buffer);
        }
    } else if (!vim_bitmaps_loaded(c->mesh->vim)) {
        printf("RE-LOADING bitmaps for : %s\n", names[0]);
        vim_reload_bitmaps(names[0], c->mesh->vim);
    }
    if (keep_loaded == true)
        c->flags |= CA_KEEP_LOADED;
    return c;
}

// 0x541500  Confidence: exact (VC6 byte match, match23; was likely; the LOD loop's register / stack use). Never returns NULL (the table full
// is a fatal error). Only names[0] is loaded; the other names only get a mesh slot with their file name.
// Neither the slot allocations nor the number of names are checked (NOTES.md).
ca_character *character_create_base(const char *const *names, int num_names, Mempool *model_pool,
                                    Mempool *char_pool)
{
    char buf[CA_NAME_LEN] = "";
    int i;

    for (i = 0; i < CA_MAX_CHARACTERS; i++) {
        if (!(ca_characters[i].flags & CA_LOADED))
            break;
    }
    if (i == CA_MAX_CHARACTERS) {
        sprintf(string_buffer, "Error loading character %s.  No more base character room.\n"
                "Please up the MAX_BASE_CHARACTERS constant.", names[0]);
        VSDK_ERROR(CHARACTER_CPP, 470, string_buffer);
    }

    ca_character *c = &ca_characters[i];
    character_clear(c);
    c->flags |= CA_LOADED;
    ca_num_characters++;
    file_strip_ext(buf, names[0]);
    strcpy(c->name, buf);

    ca_mesh *m = ca_preload_alloc_slot();
    c->lods[0] = m;
    m->owner = c;
    c->mesh = m;
    if (model_pool) {
        set_current_mempool(model_pool);
        c->pool = model_pool;
    } else {
        c->pool = NULL;
    }
    c->load_shadow(c->name, c->lods[0]);
    strcpy(c->lods[0]->filename, c->name);
    if (model_pool) {
        set_current_mempool(NULL);
        if (char_pool)
            set_current_mempool(char_pool);
    }

    for (i = 1; i < num_names; i++) {
        if (names[i]) {
            file_strip_ext(buf, names[i]);
            m = ca_preload_alloc_slot();
            strcpy(m->filename, buf);
            c->lods[i] = m;
            m->owner = c;
        }
    }
    if (is_mempool_active())
        c->flags |= CA_IN_MEMPOOL;
    return c;
}

// 0x541700  Confidence: exact
ca_mesh *ca_preload_alloc_slot()
{
    for (int i = 0; i < CA_MAX_MESHES; i++) {
        if (ca_preload_slots[i].owner == NULL) {
            ca_preload_slots[i].vim = NULL;
            return &ca_preload_slots[i];
        }
    }
    return NULL;
}

// 0x541740  Confidence: exact (character_unload has cleared the flags already).
void character_release(ca_character *c)
{
    character_unload(c);
    c->flags &= ~CA_LOADED;
}

// 0x541760  Confidence: exact
void character_release_all()
{
    for (int i = 0; i < CA_MAX_CHARACTERS; i++) {
        if ((ca_characters[i].flags & CA_LOADED) && !(ca_characters[i].flags & CA_KEEP_LOADED))
            character_release(&ca_characters[i]);
    }
}

// ---- the preload index ------------------------------------------------------------------------------

// 0x541790  Confidence: exact (VC6 byte match, m-1020 w9; was likely) (one buffer for the entry names and the error text: both live at the same
// stack slot). The entries are read for their offsets and sizes, then the whole file again into
// ca_preload_data, where ca_preload_find looks up the names. An open failure is formatted and dropped.
// Neither the entry count (70 slots) nor the size (1 MB) is checked.
void ca_load_preload_index(const char *name)
{
    File f;
    char buf[0x80];

    if (f.open(name, 1, PATH_ANY) == 0) {
        f.read_int(0, 0);
        f.read_int(0, 0);
        ca_preload_count = f.read_int(0, 0);
        ca_preload_size = f.read_int(0, 0);
        for (int i = 0; i < ca_preload_count; i++) {
            f.read(buf, 0x40, 0, 0);
            ca_preload_offsets[i] = f.read_int(0, 0);
            ca_preload_sizes[i] = f.read_int(0, 0);
        }
        f.seek(0, 0);
        f.read(ca_preload_data, ca_preload_size, 0, 0);
        ca_preload_loaded = 1;
        f.close();
    } else {
        sprintf(buf, "Error opening : %s\n", name);
    }
}

// 0x5418f0  Confidence: exact (VC6 byte match, match23; was likely; the entry names addressed through the header at the start of
// ca_preload_data: the code uses the constant 0x2de20d4).
int ca_preload_find(const char *name, int *offset, int *size)
{
    if (ca_preload_loaded) {
        int n = ca_preload_count;
        int i = 0;
        if (n > 0) {
            ca_preload_entry *e = ((ca_preload_header *)ca_preload_data)->entries;
            for (; i < n; e++, i++) {
                if (_stricmp(name, e->name) == 0) {
                    *offset = ca_preload_offsets[i];
                    *size = ca_preload_sizes[i];
                    return 1;
                }
            }
        }
    }
    return 0;
}

// ---- compiler-generated (0x541960-0x54199b) -----------------------------------------------------------
// 0x541960  Confidence: exact (generated: ~ca_character_holder, jmp ~ca_character)
// 0x541970  Confidence: exact (generated: ca_character_holder(), sum.symbols ca_character_holder__ctor)
// 0x541980  Confidence: exact (generated: skeleton's scalar deleting destructor, from the deletes above)
