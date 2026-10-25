// skeleton.cpp - Sum.exe v1.40, code 0x543040-0x544a8d (vsdk\ca\skeleton.cpp): .vim meshes and .mvf
// skeletons (animations), followed by the key classes' inline and implicit constructors this object
// emitted out of line (0x544ad0-0x544b7e; DECISIONS.md D502, bodies in skeleton.h). Written from the
// Sum.exe disassembly (build/sum.asm, tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966). Confidence per
// function: exact = expect identical code, likely = same semantics, small layout or scheduling risk,
// unsure = the source shape is a guess.
//
// Build: optimised, no inline expansion (vector3::operator=, matrix(), matrix43::get_position are
// called), /GX (new[] of a class with a constructor and stack File objects get EH frames).
//
// .vim: a header (vim_header), one data block holding the sub-meshes and their arrays (16-byte aligned),
// the bones, the named extra bones and the tags, then the extra bone names and the texture file names.
// .mvf: "VMVF", version, start / end ticks, root rotation and displacement, per bone a weight, rotation
// keys and position keys; then fades, five triggers and (version 4+) morph tracks. Versions below 5 have
// no key totals, so a first pass counts them.

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "vsdk/ca/skeleton.h"
#include "vsdk/ca/character.h"
#include "vsdk/vfile/file.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/debug.h"

#define SKELETON_CPP "D:\\projects\\Summoner\\pccode\\vsdk\\ca\\skeleton.cpp"

// The file name buffers: 41 bytes ("" then 40 zero bytes: `mov byte; rep stosd` x10), 44 with the stack
// alignment. Name ours.
#define VIM_NAME_LEN 41

#define MVF_SIGNATURE 0x46564d56      // "VMVF" in the file
#define MVF_TICKS_PER_FRAME 160
#define MVF_SECONDS(t) ((float)(t) * (1.0f / 160.0f) * (1.0f / 30.0f))
#define MVF_ROOT_EPSILON 0.0001f      // 0x581930

// The sub-mesh arrays start on 16-byte boundaries of the data block.
#define VIM_ALIGN16(n) ((n) += ((n) % 16 == 0) ? 0 : 16 - (n) % 16)

// ---- .data (0x5ac8e0-0x5ac9e3) ------------------------------------------------------------------------
// Literals only, one copy per use: "vim" x3, "VIF LOADING : %s\n", "Unable to load vim: %s\n",
// "Too many skeletons", the path twice, the morphing warning, "Skeletons using too much memory".

// ---- .bss (0x2ee4524-0x2ee45ab) -------------------------------------------------------------------
int vim_saved_bitmaps[20];            // 0x2ee4524
int vim_saved_bitmap_count;           // 0x2ee4574
// 0x2ee4578-0x2ee4590: the seven "" literals of the name buffers (vim_write_cache 2, vim_read 1,
// vim_load 2, vim_reload_bitmaps 2). 0x2ee4594: no reference (a literal or variable of unknown use).

// 0x543030  Confidence: exact (the placement; the body is exact). sum.symbols ui_stub_ret_543030: a bare ret
// that nothing references. Not a COMDAT (the linker drops unreferenced COMDATs), and after
// ui_element's closing COMDAT (ui_tooltip's scalar deleting destructor 0x543010), so it is the first
// function of this object (D5609). Name from sum.symbols.
void ui_stub_ret_543030()
{
}

// 0x543040  Confidence: exact. No check of the new: a failed allocation writes through NULL.
vim *vim_alloc()
{
    vim *mesh = new vim;

    mesh->flags = 0;
    if (is_mempool_active())
        mesh->flags |= VIM_IN_MEMPOOL;
    return mesh;
}

// 0x5430b0  Confidence: exact. A mesh without a data block (or from the mempool) is not freed.
void vim_free(vim *mesh)
{
    if (mesh && mesh->data && !(mesh->flags & VIM_IN_MEMPOOL)) {
        delete mesh->data;
        delete mesh;
    }
}

// 0x5430e0  Confidence: exact.
// Writes the mesh as name.vim into path_id. Unreferenced.
int vim_write_cache(vim *mesh, const char *name, int path_id)
{
    File f;
    char base[VIM_NAME_LEN] = "";
    char fname[VIM_NAME_LEN] = "";

    file_strip_ext(base, name);
    file_change_ext(fname, base, "vim");
    if (mesh == NULL)
        return 0;
    if (f.open(fname, 2, path_id))
        return 0;
    int ok = vim_write_header(mesh, &f);
    f.close();
    return ok;
}

// 0x543230  Confidence: exact.
// The whole mesh in vim_read's order (the name says header: sum.symbols).
int vim_write_header(vim *mesh, File *f)
{
    int i;

    f->write_int(VIM_VERSION);
    f->write_int(mesh->flags);
    f->write_int(mesh->num_vertices);
    f->write_uint16(mesh->num_submeshes);
    f->write_vector(&mesh->center);
    f->write_float(mesh->radius);
    f->write_int(mesh->data_size);
    f->write(mesh->data, mesh->data_size);
    f->write_int(mesh->_unk34);
    for (i = 0; i < mesh->num_submeshes; i++) {
        f->write_uint16(mesh->submeshes[i].size0);
        f->write_uint16(mesh->submeshes[i].size1);
        f->write_uint16(mesh->submeshes[i].size2);
        f->write_uint16(mesh->submeshes[i].size3);
        f->write_uint16(mesh->submeshes[i].size6);
        f->write_uint16(mesh->submeshes[i].size5);
        f->write_uint16(mesh->submeshes[i].size4);
    }
    f->write_int(mesh->skin.num_bones);
    f->write_int(mesh->skin.root_bone);
    f->write_int(mesh->skin.num_ext);
    for (i = 0; i < mesh->skin.num_ext; i++)
        f->write_string_z(mesh->skin.ext[i].name);
    f->write_int(mesh->num_tags);
    f->write_int(mesh->num_textures);
    for (i = 0; i < mesh->num_textures; i++) {
        const char *bm_name = bm_get_filename(mesh->textures[i]);
        if (bm_name == NULL)
            return 0;
        f->write_string_z(bm_name);
    }
    return 1;
}

// 0x5433d0  Confidence: exact. Stops after the version below 6.
void vim_read_header(FileReader *r, vim_header *hdr)
{
    hdr->version = r->read_int();
    if (hdr->version < VIM_VERSION)
        return;
    hdr->flags = r->read_int();
    hdr->num_vertices = r->read_int();
    hdr->num_submeshes = r->read_uint16();
    r->read_vector(&hdr->center);
    hdr->radius = r->read_float();
    hdr->data_size = r->read_int();
}

// 0x543430  Confidence: exact (D14003: the sizes stored and re-read through mesh->submeshes[i], the block
// pointers through sm; the bone-name loop re-reads mesh->skin.num_ext). The seven sizes are kept in locals before the stores.
// reload: the mesh is already set up; skip the data block and the counts, read the texture names again
// (vim_reload_bitmaps). Otherwise the data block is read and the pointers into it are laid out.
int vim_read(FileReader *r, vim *mesh, int reload, int bm_arg, vim_header *hdr)
{
    int offset = 0;
    int i;
    char name[128];

    if (reload) {
        r->seek(hdr->data_size, 1);
    } else {
        mesh->data = NULL;
        mesh->submeshes = NULL;
        mesh->flags |= hdr->flags;
        mesh->num_vertices = hdr->num_vertices;
        mesh->num_submeshes = hdr->num_submeshes;
        mesh->center = hdr->center;
        mesh->radius = hdr->radius;
        mesh->data_size = hdr->data_size;
        mesh->data = new ubyte[mesh->data_size];
        if (mesh->data == NULL)
            return 0;
        r->read(mesh->data, mesh->data_size);
        mesh->submeshes = (vim_submesh *)mesh->data;
        offset = mesh->num_submeshes * sizeof(vim_submesh);
        VIM_ALIGN16(offset);
    }

    int unk34 = r->read_int();
    if (!reload)
        mesh->_unk34 = unk34;

    for (i = 0; i < mesh->num_submeshes; i++) {
        ushort s0 = r->read_uint16();
        ushort s1 = r->read_uint16();
        ushort s2 = r->read_uint16();
        ushort s3 = r->read_uint16();
        ushort s6 = r->read_uint16();
        ushort s5 = r->read_uint16();
        ushort s4 = r->read_uint16();
        if (!reload) {
            mesh->submeshes[i].size0 = s0;
            mesh->submeshes[i].size1 = s1;
            mesh->submeshes[i].size2 = s2;
            mesh->submeshes[i].size3 = s3;
            mesh->submeshes[i].size6 = s6;
            mesh->submeshes[i].size5 = s5;
            mesh->submeshes[i].size4 = s4;
            vim_submesh *sm = &mesh->submeshes[i];
            sm->block0 = mesh->data + offset;
            offset += mesh->submeshes[i].size2;
            VIM_ALIGN16(offset);
            sm->block1 = mesh->data + offset;
            offset += mesh->submeshes[i].size2;
            VIM_ALIGN16(offset);
            sm->block2 = mesh->data + offset;
            offset += mesh->submeshes[i].size4;
            VIM_ALIGN16(offset);
            sm->block3 = mesh->data + offset;
            offset += mesh->submeshes[i].size3;
            VIM_ALIGN16(offset);
            sm->block4 = mesh->data + offset;
            offset += mesh->submeshes[i].size6;
            VIM_ALIGN16(offset);
            if (mesh->submeshes[i].size5 > 0) {
                sm->block5 = mesh->data + offset;
                offset += mesh->submeshes[i].size5;
                VIM_ALIGN16(offset);
            }
            sm->vertex_extra = NULL;
            if (mesh->flags & VIM_HAS_VERTEX_EXTRA) {
                sm->vertex_extra = mesh->data + offset;
                offset += hdr->num_vertices * 2;
                VIM_ALIGN16(offset);
            }
        }
    }

    int num_bones = r->read_int();
    int root_bone = r->read_int();
    int num_ext = r->read_int();
    if (reload) {
        for (i = 0; i < mesh->skin.num_ext; i++)
            r->read_string(name, 0x7f);
    } else {
        mesh->skin.num_bones = num_bones;
        if (num_bones > 0) {
            mesh->skin.bones = (ca_bone *)(mesh->data + offset);
            offset += num_bones * sizeof(ca_bone);
        }
        mesh->skin.num_ext = num_ext;
        mesh->skin.root_bone = root_bone;
        if (num_ext > 0) {
            mesh->skin.ext = (ca_bone_ext *)(mesh->data + offset);
            offset += num_ext * sizeof(ca_bone_ext);
            for (i = 0; i < mesh->skin.num_ext; i++) {
                r->read_string(name, 0x7f);
                mesh->skin.ext[i].name = ca_string_pool.AddUnique(name);
            }
        }
    }

    int num_tags = r->read_int();
    if (!reload) {
        mesh->num_tags = num_tags;
        mesh->tags = NULL;
        if (num_tags)
            mesh->tags = (vim_tag *)(mesh->data + offset);
    }

    mesh->num_textures = r->read_int();
    for (i = 0; i < mesh->num_textures; i++) {
        char texname[512] = "";
        r->read_string(texname, 0x1ff);
        mesh->textures[i] = bm_load(texname, -1, bm_arg);
    }
    return 1;
}

// 0x5438b0  Confidence: exact.
// Game bug (minor): the mesh from vim_alloc leaks when the version is below 6 or vim_read fails.
vim *vim_load_file(FileReader *r, int bm_arg)
{
    vim_header hdr;
    vim *mesh = vim_alloc();

    if (mesh == NULL)
        return NULL;
    vim_read_header(r, &hdr);
    if (hdr.version < VIM_VERSION)
        return NULL;
    return vim_read(r, mesh, 0, bm_arg, &hdr) ? mesh : NULL;
}

// 0x543920  Confidence: exact (the error text is formatted into a local buffer and never shown; the
// message call of the original was probably compiled out).
vim *vim_load(const char *name, int bm_arg)
{
    File f;
    FileReader r;
    char base[VIM_NAME_LEN] = "";
    char fname[VIM_NAME_LEN] = "";
    char err[256];

    file_strip_ext(base, name);
    file_change_ext(fname, base, "vim");
    printf("VIF LOADING : %s\n", name);
    if (f.open(fname, 1, PATH_ANY)) {
        sprintf(err, "Unable to load vim: %s\n", fname);
        return NULL;
    }
    r.attach_file(&f);
    vim *mesh = vim_load_file(&r, bm_arg);
    f.close();
    return mesh;
}

// 0x543a80  Confidence: exact.
void vim_reload_bitmaps(const char *name, vim *mesh)
{
    File f;
    FileReader r;
    char base[VIM_NAME_LEN] = "";
    char fname[VIM_NAME_LEN] = "";
    vim_header hdr;

    file_strip_ext(base, name);
    file_change_ext(fname, base, "vim");
    if (f.open(fname, 1, PATH_ANY) == 0) {
        r.attach_file(&f);
        vim_read_header(&r, &hdr);
        vim_read(&r, mesh, 1, -1, &hdr);
        f.close();
    }
}

// 0x543bb0  Confidence: exact (VC6 byte match, match31; was likely; an element loop, which 8966 turns into the
// binary's count test and dword rep movsd; the memcpy form loads dst after the pushes, D9002).
void vim_copy_bitmaps(vim *dst, const vim *src)
{
    int i;
    for (i = 0; i < src->num_textures; i++)
        dst->textures[i] = src->textures[i];
}

// 0x543bd0  Confidence: exact (VC6 byte match, match31; was likely; as vim_copy_bitmaps, D9002).
void vim_save_bitmaps(const vim *mesh)
{
    int i;
    for (i = 0; i < mesh->num_textures; i++)
        vim_saved_bitmaps[i] = mesh->textures[i];
    vim_saved_bitmap_count = mesh->num_textures;
}

// 0x543c00  Confidence: exact (as vim_copy_bitmaps).
void vim_restore_bitmaps(vim *mesh)
{
    if (vim_saved_bitmap_count > 0)
        memcpy(mesh->textures, vim_saved_bitmaps, vim_saved_bitmap_count * sizeof(vim_saved_bitmaps[0]));
}

// 0x543c20  Confidence: exact.
int vim_find_tag(vim *mesh, const char *name)
{
    if (mesh) {
        for (int i = 0; i < mesh->num_tags; i++) {
            if (_stricmp(name, mesh->tags[i].name) == 0)
                return i;
        }
    }
    return -1;
}

// 0x543c70  Confidence: exact (the unused local matrix is constructed on entry).
void vim_get_tag_transform(vim *mesh, int index, vector3 *pos, matrix *orient)
{
    matrix unused;

    if (mesh && index >= 0 && index < mesh->num_tags) {
        *pos = mesh->tags[index].pos;
        mesh->tags[index].orient.to_matrix_b(orient);
    }
}

// 0x543cd0  Confidence: exact.
int vim_bitmaps_loaded(vim *mesh)
{
    if (mesh == NULL)
        return 0;
    for (int i = 0; i < mesh->num_textures; i++) {
        if (mesh->textures[i] <= 0)
            return 0;
    }
    return 1;
}

// 0x543d00  Confidence: exact (written out because it sits among the unit's functions; an implicit
// constructor would compile the same).
vim::vim()
{
}

// 0x543d10  Confidence: exact (as vim::vim).
vim_header::vim_header()
{
}

// ==== skeletons ======================================================================================

// 0x2ee4598-0x2ee45a8 (ca_init_globals_2ee4598 sets them).
int skeleton_count;                   // 0x2ee4598
int skeleton_count_limit;             // 0x2ee459c
int skeleton_mem_used;                // 0x2ee45a0
int skeleton_mem_peak;                // 0x2ee45a4
int skeleton_mem_limit;               // 0x2ee45a8

// 0x543d20  Confidence: exact (the member constructors, then the memset).
skeleton::skeleton()
{
    memset((void *)this, 0, sizeof(skeleton));
}

// 0x543d50 (sum.symbols: skeleton_add)  Confidence: exact (store order; the fourth argument is not read).
skeleton::skeleton(const char *filename, const char *name, bool is_state, int unused, bool root_rotation)
{
    skeleton_count++;
    if (skeleton_count_limit >= 0 && skeleton_count > skeleton_count_limit)
        VSDK_ERROR(SKELETON_CPP, 150, "Too many skeletons");
    num_rot_keys = 0;
    num_pos_keys = 0;
    rot_keys = NULL;
    pos_keys = NULL;
    this->is_state = is_state;
    this->root_rotation = root_rotation;
    this->name = ca_string_pool.AddUnique(name);
    num_morphs = 0;
    num_morph_frames = 0;
    morph_ids = NULL;
    morphs = NULL;
    strcpy(this->filename, filename);
}

// 0x543e10  Confidence: exact (called by the compiler's scalar deleting destructor 0x541980).
skeleton::~skeleton()
{
    skeleton_count--;
    skeleton_mem_used -= sizeof(skeleton);
    list_remove(this);
    if (bone_weights) {
        delete[] bone_weights;
        skeleton_mem_used -= num_bones;
    }
    if (tracks) {
        delete[] tracks;
        skeleton_mem_used -= num_bones * sizeof(anim_track);
    }
    if (rot_keys) {
        delete[] rot_keys;
        skeleton_mem_used -= num_rot_keys * sizeof(anim_rot_key);
    }
    if (pos_keys) {
        delete[] pos_keys;
        skeleton_mem_used -= num_pos_keys * sizeof(anim_pos_key);
    }
    if (morph_ids) {
        delete[] morph_ids;
        morph_ids = NULL;
        skeleton_mem_used -= num_morphs * sizeof(int);
    }
    if (morphs) {
        for (int i = 0; i < num_morphs; i++) {
            delete[] morphs[i].keys;
            skeleton_mem_used -= morphs[i].num_keys * sizeof(anim_vec_key);
        }
        delete[] morphs;
        morphs = NULL;
        skeleton_mem_used -= num_morphs * sizeof(skeleton_morph);
    }
    filename[0] = 0;
}

// 0x543fc0  Confidence: exact (VC6 8966 byte match: the product goes into a named local that is
// returned; the time constants are two multiplies, not folded, D6600).
// Root displacement per second.
vector3 skeleton::get_root_velocity()
{
    float t = (float)(end - start) * (1.0f / 160.0f);
    vector3 v = root_disp * (1.0f / (t * (1.0f / 30.0f)));
    return v;
}

// 0x544010  Confidence: exact (VC6 8966 byte match with the frame count through a float local, so the
// two time constants stay unfolded as in get_root_velocity). No direct call.
// Game bug (latent): the slerp starts from a default-constructed quaternion, i.e. uninitialised stack
// (quaternion() is empty), not from the identity.
quaternion skeleton::get_root_rotation(float t)
{
    quaternion q;
    float d = (float)(end - start) * (1.0f / 160.0f);

    quaternion_slerp(&q, &(const quaternion &)quaternion(), &root_rot, (1.0f / (d * (1.0f / 30.0f))) * t);
    q.normalize();
    return q;
}

// 0x544090 (sum.symbols: skeleton_read)  Confidence: likely (D14004: 17 differing lines left; statement order in the key loops; the
// common tails of the three rotation key formats are merged by the compiler).
// Returns false only when a position key track fails to read.
bool skeleton::read(FileReader *r, const char *name, int path_id)
{
    int version;
    int i, k;

    progress_update(0);
    start = r->read_int();
    if (start == MVF_SIGNATURE) {
        version = r->read_int();
        start = r->read_int();
    } else {
        version = 0;
    }
    end = r->read_int();

    r->read_quaternion(&root_rot);
    if ((float)fabs(root_rot.x) < MVF_ROOT_EPSILON)
        root_rot.x = 0.0f;
    if ((float)fabs(root_rot.y) < MVF_ROOT_EPSILON)
        root_rot.y = 0.0f;
    if ((float)fabs(root_rot.z) < MVF_ROOT_EPSILON)
        root_rot.z = 0.0f;
    root_rot.normalize();

    r->read_vector(&root_disp);
    if ((float)fabs(root_disp.x) < MVF_ROOT_EPSILON)
        root_disp.x = 0.0f;
    root_disp.y = 0.0f;
    if ((float)fabs(root_disp.z) < MVF_ROOT_EPSILON)
        root_disp.z = 0.0f;

    num_bones = r->read_int();
    if (num_bones > 0) {
        bone_weights = new ubyte[num_bones];
        tracks = new anim_track[num_bones];
        skeleton_mem_used += num_bones + num_bones * sizeof(anim_track);
    }

    if (version >= 5) {
        num_rot_keys = r->read_int();
        num_pos_keys = r->read_int();
    } else {
        // Count the keys: old rotation keys are 0x28 (version 0/1) or 0x24 bytes.
        int pos = r->tell();
        for (i = 0; i < num_bones; i++) {
            r->read_uint8();
            int nrot = r->read_int();
            if (version >= 2)
                r->seek(nrot * 0x24, 1);
            else
                r->seek(nrot * 0x28, 1);
            anim_pos_key key;
            int key_size = key.get_size();
            int npos = r->read_int();
            if (i != num_bones - 1)
                r->seek(npos * key_size, 1);
            num_rot_keys += nrot;
            num_pos_keys += npos;
        }
        r->seek(pos, 0);
    }

    rot_keys = new anim_rot_key[num_rot_keys];
    pos_keys = new anim_pos_key[num_pos_keys];
    skeleton_mem_used += num_rot_keys * sizeof(anim_rot_key) + num_pos_keys * sizeof(anim_pos_key);

    int rot_used = 0;
    int pos_used = 0;
    for (i = 0; i < num_bones; i++) {
        bone_weights[i] = r->read_uint8();
        anim_track *t = &tracks[i];
        memset(t, 0, sizeof(anim_track));

        t->num_rot_keys = r->read_int();
        if (t->num_rot_keys > 0) {
            t->rot_keys = &rot_keys[rot_used];
            rot_used += t->num_rot_keys;
            for (k = 0; k < t->num_rot_keys; k++) {
                t->rot_keys[k].time = r->read_int();
                if (version >= 5) {
                    t->rot_keys[k].q.x = r->read_int16();
                    t->rot_keys[k].q.y = r->read_int16();
                    t->rot_keys[k].q.z = r->read_int16();
                    t->rot_keys[k].q.w = r->read_int16();
                    for (int skip = 0; skip < 8; skip++)
                        r->read_int16();
                    t->rot_keys[k].ease_in = r->read_int8();
                    t->rot_keys[k].ease_out = r->read_int8();
                } else if (version >= 2) {
                    t->rot_keys[k].q.x = r->read_int16();
                    t->rot_keys[k].q.y = r->read_int16();
                    t->rot_keys[k].q.z = r->read_int16();
                    t->rot_keys[k].q.w = r->read_int16();
                    for (int skip = 0; skip < 8; skip++)
                        r->read_int16();
                    t->rot_keys[k].ease_in = (signed char)(r->read_float() * 2.54f);
                    t->rot_keys[k].ease_out = (signed char)(r->read_float() * 2.54f);
                } else {
                    quaternion q;
                    r->read_quaternion(&q);
                    q.normalize();
                    t->rot_keys[k].q.x = (short)(q.x * 16383.0f);
                    t->rot_keys[k].q.y = (short)(q.y * 16383.0f);
                    t->rot_keys[k].q.z = (short)(q.z * 16383.0f);
                    t->rot_keys[k].q.w = (short)(q.w * 16383.0f);
                    t->rot_keys[k].tangent[0] = (signed char)(r->read_float() * 127.0f);
                    t->rot_keys[k].tangent[1] = (signed char)(r->read_float() * 127.0f);
                    t->rot_keys[k].tangent[2] = (signed char)(r->read_float() * 127.0f);
                    t->rot_keys[k].ease_in = (signed char)(r->read_float() * 2.54f);
                    t->rot_keys[k].ease_out = (signed char)(r->read_float() * 2.54f);
                }
            }
        }

        t->num_pos_keys = r->read_int();
        if (t->num_pos_keys > 0) {
            t->pos_keys = &pos_keys[pos_used];
            pos_used += t->num_pos_keys;
            if (skeleton_read_tag_records(r, t->pos_keys, t->num_pos_keys))
                return false;
        }
        if (version < 2)
            t->init_tangents(1);
    }

    if (r->eof()) {
        fade_out = fade_in = (end - start) / 5;
        for (i = 0; i < 5; i++) {
            triggers[i].name = NULL;
            triggers[i].time = start;
        }
    } else {
        fade_in = r->read_int() * MVF_TICKS_PER_FRAME;
        fade_out = r->read_int() * MVF_TICKS_PER_FRAME;
        for (i = 0; i < 5; i++) {
            char buf[256];
            r->read(buf, 0xff);
            if (strlen(buf) != 0)
                triggers[i].name = ca_string_pool.AddUnique(buf);
            else
                triggers[i].name = NULL;
            triggers[i].time = r->read_int();
        }
        if (version >= 4) {
            num_morphs = r->read_int();
            num_morph_frames = r->read_int();
            if (num_morphs && num_morph_frames) {
                if (version < 8 && name)
                    printf("FILE ISN'T MVFREDUCED AND HAS MORPHING!!! %s\n", name);
                morph_ids = new int[num_morphs];
                morphs = new skeleton_morph[num_morphs];
                skeleton_mem_used += num_morphs * (sizeof(int) + sizeof(skeleton_morph));
                for (int i = 0; i < num_morphs; i++) {
                    if (version < 8)
                        morph_ids[i] = r->read_int();
                    else
                        morph_ids[i] = r->read_int16();
                    if (version < 8)
                        morphs[i].num_keys = (short)num_morph_frames;
                    else
                        morphs[i].num_keys = r->read_int16();
                    morphs[i]._unk08 = 1;
                    morphs[i].keys = new anim_vec_key[morphs[i].num_keys];
                    skeleton_mem_used += morphs[i].num_keys * sizeof(anim_vec_key);
                    for (short j = 0; j < morphs[i].num_keys; j++) {
                        if (version >= 8)
                            morphs[i].keys[j].time = r->read_int16();
                        else
                            morphs[i].keys[j].time = j;
                        morphs[i].keys[j].v.x = r->read_float();
                        morphs[i].keys[j].v.y = r->read_float();
                        morphs[i].keys[j].v.z = r->read_float();
                    }
                }
            }
        }
    }

    skeleton_mem_used += sizeof(skeleton);
    if (skeleton_mem_used > skeleton_mem_peak)
        skeleton_mem_peak = skeleton_mem_used;
    if (skeleton_mem_limit >= 0 && skeleton_mem_used > skeleton_mem_limit)
        VSDK_ERROR(SKELETON_CPP, 692, "Skeletons using too much memory");
    return true;
}

// 0x544a10  Confidence: exact.
void ca_init_globals_2ee4598()
{
    skeleton_count = 0;
    skeleton_count_limit = -1;
    skeleton_mem_used = 0;
    skeleton_mem_peak = 0;
    skeleton_mem_limit = -1;
}

// 0x544a40  Confidence: exact (a bare ret; no caller found, label ours).
void ca_close_globals_544a40()
{
}

// 0x544a50  Confidence: exact.
int skeleton_read_tag_record(FileReader *r, anim_pos_key *key)
{
    key->time = r->read_int();
    r->read_vector(&key->pos);
    r->read_vector(&key->in);
    r->read_vector(&key->out);
    return r->error();
}

// 0x544a90  Confidence: exact.
int skeleton_read_tag_records(FileReader *r, anim_pos_key *keys, unsigned int n)
{
    for (unsigned int i = 0; i < n; i++) {
        int err = skeleton_read_tag_record(r, &keys[i]);
        if (err)
            return err;
    }
    return 0;
}

// 0x544ad0 anim_pos_key::anim_pos_key,
// 0x544b20 anim_key__ctor: anim_key::anim_key(time), +0 = time, returns this; inline in skeleton.h.
// Confidence: exact.
// 0x544b30 anim_pos_key__get_size: anim_pos_key::get_size(), returns 0x28; inline in skeleton.h.
// Confidence: exact.
// 0x544b40 anim_rot_key__ctor: anim_rot_key's implicit default constructor (anim_key(0), then the vector3
// at +4). Compiler-generated. Confidence: exact.
// 0x544b60 skeleton_node__ctor_default: anim_pos_key's default constructor closure for new[], calling it
// with 0 and identity_matrix43. Compiler-generated. Confidence: exact.
// 0x544b70 anim_vec_key__ctor: anim_vec_key's implicit default constructor (the vector3 at +4).
// Compiler-generated. Confidence: exact.
