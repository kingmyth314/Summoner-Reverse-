// s3d.cpp - Sum.exe v1.40, code 0x46dd30-0x4705a3 (Engine\s3d\s3d.cpp, path literal 0x59ad10)
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966).
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// Unit range: sum.symbols tags 52 functions from 0x46dd30 (sinit_level_names_a5a2a0) to 0x470560
// (s3d_read_textured_records) with s3d.cpp; the previous unit (multisave.cpp) ends at 0x46dd2f, the next
// (multitrade.cpp) starts at 0x470e30 after other code. Optimised build (/O2: no frame pointer, 0x90
// padding).
//
// The level is one object, level_s3d (0xa5a2a0, 0xe0 bytes, engine/s3d/s3d.h). Its loader reads the
// .s3d chunks into arrays allocated once per level in level_mempool (s3d_alloc_level_arrays makes the
// pool current, so plain new reaches it), and hands out slices of the big per-level pools through the
// cursors below. Every reader is a "sync" function: with File::check_flags(1) (read mode) it fills the
// record, otherwise it writes it out again (the write half is reachable only from tools that open the
// file for writing; nothing in Sum.exe does).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "engine/s3d/s3d.h"
#include "engine/rendering/levelrender.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/console.h"
#include "vsdk/os/debug.h"
#include "vsdk/os/stringpool.h"
#include "vsdk/os/timestamp.h"
#include "vsdk/vfile/file.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/gr/gr.h"
#include "vsdk/geom/effect_mem.h"
#include "summoner/summoner.h"
#include "levelscripts/level_scripts.h"

// Original __FILE__ of this unit (literal 0x59ad10).
#define S3D_CPP "D:\\projects\\Summoner\\pccode\\Engine\\s3d\\s3d.cpp"

// ---- .rdata ----------------------------------------------------------------------------------------
// 0.03f (0x5819c0, the orientation check) and 0.998f (0x5819f8, s3d_read_color_record) sit next to
// objects.cpp's constants; 1.0f (0x57e990) and 0.0f (0x57eb84) are shared __real@ constants.

// ---- .data (0x59aa58; the literals 0x59aa5c-0x59ad5f follow) ---------------------------------------
bool S3d_load_lightmaps = true;           // 0x59aa58

// ---- .bss (0xa5a2a0-0x22e4425) -----------------------------------------------------------------------
// In address order, except the objects with dynamic initialisers: those are defined below in the order
// of their initialisers (static_initializers entries 207-213), each next to its functions.
char level_mempool_buffer[0x186a000];  // 0xa5a380 25.6 MB (name ours)
int s3d_vertex_cursor;                    // 0x22c4380 level +0xb4 (names of the cursors ours)
int s3d_cursor_22c4384;                   // 0x22c4384 reset only
// 0x22c4388 ccmd_load_lightmaps
int s3d_normal_cursor;                    // 0x22c4394 level +0xc0
char s3d_lightmap_buffer[0x20000];        // 0x22c4398 one lightmap (16-bit texels)
// 0x22e4398 ccmd_lmap_reset, 0x22e43a8 ccmd_p3d_files
int s3d_cursor_22e43b4;                   // 0x22e43b4 reset only
int s3d_short_cursor;                     // 0x22e43b8 level +0xc8
int s3d_byte_cursor;                      // 0x22e43bc level +0xcc
// 0x22e43c0 level_mempool
int s3d_uv_cursor;                        // 0x22e43dc level +0xb8
int s3d_mesh_face_cursor;                 // 0x22e43e0 level +0xb0
// 0x22e43e8 ccmd_test_meshes_on_load
const char *level_vis_base_name;          // 0x22e43f8
ushort level_vis_base_count;              // 0x22e43fc
int *level_vis_base_lobjs;                // 0x22e4400
// 0x22e4408 ccmd_lmap_off
int s3d_cursor_22e4414;                   // 0x22e4414 reset only
int s3d_face_cursor;                      // 0x22e4418 level +0xac
int s3d_int_cursor;                       // 0x22e441c level +0xbc
bool s3d_lightmaps_loaded;                // 0x22e4420 (name ours; written only)
bool level_vis_base_built;                // 0x22e4422
bool Test_meshes_on_load;                 // 0x22e4423
bool P3D_files;                           // 0x22e4424
bool s3d_version_flag;                    // 0x22e4425

// The constructor and destructor are the compiler's for s3d_level (its only member with either is the
// StringPool):
// 0x46dd30 sinit_level_names_a5a2a0 (call init, jmp register; entry 207). Confidence: exact.
// 0x46dd40 init_level_names_a5a2a0 (mov ecx / jmp the constructor). Confidence: exact.
// 0x46dd50 register_dtor_level_names_a5a2a0 (atexit(0x46dd60)). Confidence: exact.
// 0x46dd60 dtor_level_names_a5a2a0 (mov ecx / jmp the destructor). Confidence: exact.
// 0x46dd70 s3d_level::~s3d_level (sum.symbols level_names__dtor: ~StringPool() on +8, a jmp). Confidence:
//          exact (compiler-generated).
// 0x46dd80 s3d_level::s3d_level (sum.symbols level_names__ctor: StringPool() on +8, returns this).
//          Confidence: exact (compiler-generated).
s3d_level level_s3d;                      // 0xa5a2a0

// Mempool's inline constructor is a bare Copy call (D12).
// 0x46dd90 init_mempool_level_thunk (jmp). Confidence: exact.
// 0x46dda0 init_mempool_level (Mempool::Copy). Confidence: exact.
Mempool level_mempool(level_mempool_buffer, 0x186a000, "level");   // 0x22e43c0

// ---- console commands --------------------------------------------------------------------------------
void cmd_test_meshes_on_load();
void cmd_load_lightmaps();
void cmd_p3d_files();

// 0x46ddc0 init_ccmd_test_meshes_on_load_thunk (jmp). Confidence: exact.
// 0x46ddd0 init_ccmd_test_meshes_on_load (console_command constructor). Confidence: exact.
console_command ccmd_test_meshes_on_load("test_meshes_on_load", "Toggles Test_meshes_on_load",
                                         cmd_test_meshes_on_load);   // 0x22e43e8

// 0x46ddf0  Confidence: exact. The console's bool command macro (as cmd_map_mask in new_automap.cpp).
void cmd_test_meshes_on_load()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            Test_meshes_on_load = true;
        else if (arg_type & ARG_FALSE)
            Test_meshes_on_load = false;
        else if (arg_type & ARG_NONE)
            Test_meshes_on_load ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "test_meshes_on_load", "Test_meshes_on_load");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "test_meshes_on_load", Test_meshes_on_load ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x46deb0 init_ccmd_load_lightmaps_thunk (jmp). Confidence: exact.
// 0x46dec0 init_ccmd_load_lightmaps (console_command constructor). Confidence: exact.
console_command ccmd_load_lightmaps("load_lightmaps", "Toggles S3d_load_lightmaps",
                                    cmd_load_lightmaps);   // 0x22c4388

// 0x46dee0  Confidence: exact.
void cmd_load_lightmaps()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            S3d_load_lightmaps = true;
        else if (arg_type & ARG_FALSE)
            S3d_load_lightmaps = false;
        else if (arg_type & ARG_NONE)
            S3d_load_lightmaps ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "load_lightmaps", "S3d_load_lightmaps");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "load_lightmaps", S3d_load_lightmaps ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// 0x46dfa0 init_ccmd_p3d_files_thunk (jmp). Confidence: exact.
// 0x46dfb0 init_ccmd_p3d_files (console_command constructor). Confidence: exact.
console_command ccmd_p3d_files("p3d_files", "Toggles P3D_files", cmd_p3d_files);   // 0x22e43a8

// 0x46dfd0  Confidence: exact.
void cmd_p3d_files()
{
    if (calling_a_command_function) {
        process_command(ARG_TRUE | ARG_FALSE | ARG_NONE);
        if (arg_type & ARG_TRUE)
            P3D_files = true;
        else if (arg_type & ARG_FALSE)
            P3D_files = false;
        else if (arg_type & ARG_NONE)
            P3D_files ^= 1;
    }
    if (doing_help_for_comand) {
        sprintf(string_buffer, "Usage: %s [bool]\nSets %s to true or false. If nothing passed, then toggles it.\n",
                "p3d_files", "P3D_files");
        print_to_console(string_buffer, 0);
    }
    if (checking_status_for_command) {
        sprintf(string_buffer, "%s is %s\n", "p3d_files", P3D_files ? "TRUE" : "FALSE");
        print_to_console(string_buffer, 0);
    }
}

// The two lightmap commands take their help text from the pooled "" (0x5bd0d0) and call a body that
// follows the handler (the handler is a single tail jump). Body names ours.
void lmap_reset_all();
void lmap_off_all();

// 0x46e0c0 ccmd_lmap_reset_handler  Confidence: exact.
void ccmd_lmap_reset_handler()
{
    lmap_reset_all();
}

// 0x46e090 init_ccmd_lmap_reset_thunk (jmp). Confidence: exact.
// 0x46e0a0 init_ccmd_lmap_reset (console_command constructor). Confidence: exact.
console_command ccmd_lmap_reset("lmap_reset", "", ccmd_lmap_reset_handler);   // 0x22e4398

// 0x46e0d0 (no symbol)  Confidence: exact. Turns the lightmaps of every level object back on.
void lmap_reset_all()
{
    int i;

    for (i = 0; i < num_lobjs; i++)
        lobjs[i].flags &= ~0x80000;
}

// 0x46e140 ccmd_lmap_off_handler  Confidence: exact.
void ccmd_lmap_off_handler()
{
    lmap_off_all();
}

// 0x46e110 init_ccmd_lmap_off_thunk (jmp). Confidence: exact.
// 0x46e120 init_ccmd_lmap_off (console_command constructor). Confidence: exact.
console_command ccmd_lmap_off("lmap_off", "", ccmd_lmap_off_handler);   // 0x22e4408

// 0x46e150 (no symbol)  Confidence: exact.
void lmap_off_all()
{
    int i;

    for (i = 0; i < num_lobjs; i++)
        lobjs[i].flags |= 0x80000;
}

// 0x46e190 s3d_geometry__dtor: the compiler's destructor of s3d_geometry (add ecx, 0x38 / jmp
// ~s3d_geom_block), passed to the eh vector constructor iterator by s3d_alloc_level_arrays.
// Confidence: exact (compiler-generated).

// ---- the loader --------------------------------------------------------------------------------------
void s3d_level_clear(s3d_level *level);
bool s3d_read_header(s3d_level *level, File *f);
void s3d_alloc_level_arrays(s3d_level *level);
void s3d_read_texture_names(s3d_level *level, File *f);
void s3d_load_lightmap(s3d_level *level, File *f);
void s3d_read_geometry(s3d_level *level, s3d_geometry *g, File *f);
void s3d_read_meshes(s3d_level *level, lobj *mesh, File *f);
void s3d_read_vertex_set(s3d_level *level, s3d_vertex_set *rec, File *f);
void s3d_read_navpoint(s3d_level *level, s3d_navpoint *np, File *f);
void s3d_read_named_path(s3d_level *level, s3d_named_path *rec, File *f);
int  s3d_read_door(s3d_level *level, File *f, door_info *door);
void s3d_read_light(s3d_light *rec, File *f);
void s3d_read_vis_set(s3d_level *level, s3d_vis_set *rec, File *f);
void s3d_read_color_record(File *f, s3d_color_record *rec);
int  s3d_read_textured_records(File *f, s3d_textured_record *recs, unsigned int count);
void s3d_load_textures(s3d_level *level);
void s3d_mark_alpha_meshes(s3d_level *level);
int  s3d_read_faces(File *f, s3d_face *faces, int count);
int  s3d_read_mesh_refs(File *f, lobj *mesh, lobj_face *refs, unsigned int count);
int  s3d_read_boxes(File *f, s3d_box *boxes, unsigned int count);

// 0x46e1a0  Confidence: exact (memset is the rep stosd intrinsic, 0x38 dwords).
void s3d_level_clear(s3d_level *level)
{
    memset((void *)level, 0, sizeof(s3d_level));
}

// 0x46e1b0  Confidence: likely. Caller level_load_files (filename, &level_s3d, 0x2d000). EH frame for
// the File; every early return runs its destructor. The header and record readers are called in file
// order; the doors are sorted by name afterwards and the $vis_base list is rebuilt. The return value is
// File::close's (D804). Locals: File at esp+0x18, the id at esp+0x14, the start count at esp+0x10.
int level_load_s3d(const char *filename, s3d_level *level, int names_size)
{
    File f;
    char id[4];
    int start_mem;
    int result;
    int i, j;

    effects_reset_level();
    s3d_level_clear(level);
    if (f.open(filename, 1, PATH_ANY) < 0)
        return -1;
    start_mem = mem_bytes_allocated;
    level->names.Allocate(names_size);
    level->filename = level->names.Add(filename);
    s3d_version_flag = (_stricmp(filename, "ionaext.s3d") == 0);
    f.read(id, 4, 0, 0);
    if (strncmp(id, "S3DF", 4)) {
        f.close();
        return -1;
    }
    if (!s3d_read_header(level, &f)) {
        f.close();
        return -1;
    }
    s3d_alloc_level_arrays(level);
    s3d_read_texture_names(level, &f);
    progress_update(0);
    s3d_load_lightmap(level, &f);
    progress_update(0);
    for (i = 0; i < level->num_geometry; i++) {
        s3d_read_geometry(level, &level->geometry[i], &f);
        progress_update(0);
    }
    set_current_mempool(&level_mempool);
    for (i = 0; i < level->num_geometry; i++)
        level->geometry[i].block.read(&f);
    set_current_mempool(NULL);
    for (i = 0; i < level->num_meshes; i++) {
        s3d_read_meshes(level, &level->meshes[i], &f);
        progress_update(0);
    }

    // The palace interior's carpets and tapestries get flag 0x200000.
    if (!_strnicmp(filename, "lpalaceint02", 12)) {
        for (i = 0; i < level->num_meshes; i++) {
            lobj *m = &level->meshes[i];
            if (!_strnicmp(m->name, "carp", 4) || !_strnicmp(m->name, "tape", 4)) {
                if (!_stricmp(m->name, "Carpet01-End02") || !_stricmp(m->name, "Carpet01-End02-02")
                    || !_stricmp(m->name, "Carpet01-End02-03") || !_stricmp(m->name, "Carpet01-End07")
                    || !_stricmp(m->name, "Carpet01-End15") || !_stricmp(m->name, "Carpet01-End16")
                    || !_stricmp(m->name, "Carpet01-Tile") || !_stricmp(m->name, "Carpet01-Tile17")
                    || !_stricmp(m->name, "Carpet02-02") || !_stricmp(m->name, "Tapestry01-01-LCF")
                    || !_stricmp(m->name, "Tapestry01-06-LCF") || !_stricmp(m->name, "Tapestry01-08-LCF"))
                    m->flags |= 0x200000;
            }
        }
    }

    for (i = 0; i < level->num_vertex_sets; i++)
        s3d_read_vertex_set(level, &level->vertex_sets[i], &f);
    for (i = 0; i < level->num_navpoints; i++) {
        s3d_navpoint *np = &level->navpoints[i];
        s3d_read_navpoint(level, np, &f);
        if (!_strnicmp(np->name, "$npc", 4) || !_strnicmp(np->name, "$hostile", 8)
            || !_strnicmp(np->name, "$player", 7))
            level_add_navpoint(np);
        progress_update(0);
    }
    for (i = 0; i < level->num_named_paths; i++) {
        s3d_read_named_path(level, &level->named_paths[i], &f);
        progress_update(0);
    }
    for (i = 0; i < level->num_door_records; i++)
        s3d_read_door(level, &f, &level->door_records[i]);
    for (i = 0; i < level->num_lights; i++) {
        s3d_read_light(&level->lights[i], &f);
        progress_update(0);
    }
    for (i = 0; i < level->num_vis_sets; i++) {
        s3d_vis_set *vs = &level->vis_sets[i];
        s3d_read_vis_set(level, vs, &f);
        progress_update(0);
        if (!_strnicmp(vs->name, "$vis", 4)) {
            for (j = 0; j < vs->count; j++)
                level->meshes[vs->indices[j]].flags |= 0x4000;
        }
    }
    for (i = 0; i < level->num_color_records; i++)
        s3d_read_color_record(&f, &level->color_records[i]);
    if (level->num_textured) {
        if (s3d_read_textured_records(&f, level->textured, level->num_textured))
            return -1;
    } else
        level->textured = NULL;
    result = f.close();

    // Door name index for door_find_by_name: a door takes the name of its named path.
    for (i = 0; i < level->num_door_records; i++) {
        level->door_names[i].door = i;
        if (level->door_records[i].named_path >= 0)
            level->door_names[i].name = level->named_paths[level->door_records[i].named_path].name;
        else
            level->door_names[i].name = "";
    }
    qsort(level->door_names, level->num_door_records, sizeof(s3d_door_name), name_ptr_stricmp_compare);
    s3d_load_textures(level);
    s3d_mark_alpha_meshes(level);

    // $vis_base: the level objects that no $vis set names. The old list is freed only when it was
    // built; otherwise the name is set (the else is the code's).
    if (level_vis_base_built && level_vis_base_lobjs)
        delete level_vis_base_lobjs;
    else
        level_vis_base_name = "$vis_base";
    level_vis_base_count = 0;
    for (i = 0; i < level->num_meshes; i++) {
        if (!(level->meshes[i].flags & 0x4000))
            level_vis_base_count++;
    }
    if (level_vis_base_count) {
        level_vis_base_lobjs = new int[level_vis_base_count];
        if (!level_vis_base_lobjs)
            return -1;
    }
    level_vis_base_built = true;
    for (i = 0, j = 0; i < level->num_meshes; i++) {
        if (!(level->meshes[i].flags & 0x4000))
            level_vis_base_lobjs[j++] = i;
        level->meshes[i].flags &= ~0x6000;
    }
    level->flags |= 1;
    level_s3d_mem += mem_bytes_allocated - start_mem;
    return result;
}

// 0x46e910  Confidence: likely. One geometry record: the arrays come from the level pools in read mode
// (the int array from level_mempool), then the face normals are rebuilt from the vertices. The three
// corners are copied into a local array (vector3's inline operator=, D500) before vector3_perp.
void s3d_read_geometry(s3d_level *level, s3d_geometry *g, File *f)
{
    int i, j;

    if (f->check_flags(1)) {
        char name[0x20];
        f->sync_string_buf(name, 0x20, 0, 0);
        g->name = level->names.Add(name);
    } else
        f->sync_string_buf(g->name, -1, 0, 0);
    f->sync_int(&g->_unk04, 0, 0);
    f->sync_int(&g->num_vertices, 0, 0);
    f->sync_int(&g->num_faces, 0, 0);
    f->sync_int(&g->_unk50, 0, 0);
    f->sync_vector(&g->_unk24, 0, &file_zero_vector);
    f->sync_float(&g->_unk30, 0, 0);
    if (f->check_flags(1)) {
        g->vertices = level->vertex_pool + s3d_vertex_cursor;
        s3d_vertex_cursor += g->num_vertices;
        g->faces = level->face_pool + s3d_face_cursor;
        s3d_face_cursor += g->num_faces;
        g->normals = level->normal_pool + s3d_normal_cursor;
        s3d_normal_cursor += g->num_vertices;
        g->uvs = level->uv_pool + s3d_uv_cursor;
        s3d_uv_cursor += g->num_vertices;
        g->_unk18 = level->short_pool + s3d_short_cursor;
        s3d_short_cursor += g->num_vertices;
        if (g->_unk50)
            g->_unk4c = (int *)level_mempool.allocate_in_mempool(g->_unk50 * 4);
        else
            g->_unk4c = NULL;
    }
    if (g->num_vertices > 0) {
        file_sync_vector_array(f, g->vertices, g->num_vertices, 0, &file_zero_vector);
        file_sync_vector_array(f, g->normals, g->num_vertices, 0, &file_zero_vector);
        file_sync_float_pair_array(f, g->uvs, g->num_vertices, 0, &vector2_zero);
        file_sync_int16_array(f, g->_unk18, g->num_vertices);
    }
    s3d_read_faces(f, g->faces, g->num_faces);
    if (g->_unk50)
        f->sync_bytes(g->_unk4c, g->_unk50 * 4, 0);
    if (f->check_flags(1)) {
        for (i = 0; i < g->num_faces; i++) {
            s3d_face *face = &g->faces[i];
            vector3 v[3];
            for (j = 0; j < 3; j++)
                v[j] = g->vertices[face->v[j]];
            vector3_perp(&face->normal, &v[0], &v[1], &v[2]);
            g->faces[i].normal.normalize();
        }
    }
}

// 0x46ebe0  Confidence: likely. One level object. The second name in the file is read and dropped
// (written back as ""). After the transform the orientation is checked (dot products and lengths
// within 0.03; the code sums the z terms first, VC6 reordering, D504; the lengths are vector3::len
// expanded), the layer and flags are read, and per-name hacks set flag bits: 0x400000 black polygons, 0x200000 (and not 0x400) sidewalks and
// rubble, 0x100000 sewer canal / mosaics (only the first 7 characters of "s1-sewercanal" are
// compared), 0x800000 Glide-only stalagmites, 0x1000000 the ocean of ionaext.s3d. Flag 0x8000: no face
// has a texture. In read mode the lightmap index of every face is cleared when the object has flag 1
// or lightmaps are off.
void s3d_read_meshes(s3d_level *level, lobj *mesh, File *f)
{
    bool untextured = true;
    char name[0x20];
    char name2[0x20];
    int layer;
    int num_faces;
    int i;
    matrix *m;

    if (f->check_flags(1)) {
        f->sync_string_buf(name, 0x20, 0, 0);
        f->sync_string_buf(name2, 0x20, 0, 0);
        mesh->name = level->names.Add(name);
        mesh->mesh = &level->geometry[f->read_int(0, 0)];
        mesh->lightmap_boxes = NULL;
        mesh->faces = level->mesh_face_pool + s3d_mesh_face_cursor;
        s3d_mesh_face_cursor += mesh->mesh->num_faces;
        mesh->vertex_rgb = level->byte_pool + s3d_byte_cursor;
        s3d_byte_cursor += mesh->mesh->num_vertices * 3;
    } else {
        f->sync_string_buf(mesh->name, -1, 0, 0);
        f->sync_string_buf("", -1, 0, 0);
        f->write_int(mesh->mesh - level->geometry);
    }
    num_faces = mesh->mesh->num_faces;
    s3d_read_mesh_refs(f, mesh, mesh->faces, num_faces);
    f->sync_bytes(mesh->vertex_rgb, mesh->mesh->num_vertices * 3, 0);
    if (f->check_version(0x10012)) {
        if (f->check_flags(1)) {
            if (mesh->faces[0].lightmap >= 0)
                mesh->lightmap_boxes = (s3d_box *)level_mempool.allocate_in_mempool(num_faces * sizeof(s3d_box));
            else
                mesh->lightmap_boxes = NULL;
        }
        if (mesh->lightmap_boxes)
            s3d_read_boxes(f, mesh->lightmap_boxes, num_faces);
    }
    f->sync_vector(&mesh->pos, 0, &file_zero_vector);
    f->sync_matrix(&mesh->orient, 0, &file_default_matrix);
    f->sync_vector(&mesh->centre, 0, &file_zero_vector);
    f->sync_vector(&mesh->bbox_min, 0, &file_zero_vector);
    f->sync_vector(&mesh->bbox_max, 0, &file_zero_vector);

    m = &mesh->orient;
    if (fabs(m->uvec.x * m->fvec.x + m->uvec.y * m->fvec.y + m->uvec.z * m->fvec.z) > 0.03f
        || fabs(m->rvec.x * m->fvec.x + m->rvec.y * m->fvec.y + m->rvec.z * m->fvec.z) > 0.03f
        || fabs(m->rvec.x * m->uvec.x + m->rvec.y * m->uvec.y + m->rvec.z * m->uvec.z) > 0.03f
        || fabs(m->fvec.len() - 1.0f) > 0.03f
        || fabs(m->uvec.len() - 1.0f) > 0.03f
        || fabs(m->rvec.len() - 1.0f) > 0.03f)
        printf("WARNING: mesh '%s' may have an invalid orientation matrix!\n", mesh->name);

    layer = mesh->get_layer_n(0);
    f->sync_int(&layer, 0, 0);
    mesh->set_layer_n(0, layer);
    f->sync_int((int *)&mesh->flags, 0, 0);
    if (!_strnicmp(mesh->name, "blackpoly", 9) || !_strnicmp(mesh->name, "shackfloor-black", 16))
        mesh->flags |= 0x400000;
    if (!_strnicmp(mesh->name, "sidewalk", 8) || !_strnicmp(mesh->name, "rubblepile02", 12)
        || !_strnicmp(mesh->name, "rubblepile01", 12))
        mesh->flags = (mesh->flags & ~0x400) | 0x200000;
    if (!_strnicmp(mesh->name, "s1-sewercanal", 7) || !_strnicmp(mesh->name, "mosaic0", 7))
        mesh->flags |= 0x100000;
    if (gr_screen.mode == 0x67 && !_strnicmp(mesh->name, "Stalag-Small", 12))
        mesh->flags |= 0x800000;
    if (s3d_version_flag && !_strnicmp(mesh->name, "Ocean", 5))
        mesh->flags |= 0x1000000;

    num_faces = mesh->mesh->num_faces;
    for (i = 0; i < num_faces && untextured; i++) {
        if (mesh->faces[i].texture > 0)
            untextured = false;
    }
    if (untextured)
        mesh->flags |= 0x8000;
    else
        mesh->flags &= ~0x8000;
    mesh->flags &= ~0x6000;
    if (f->check_flags(1)) {
        if (mesh->flags & 1) {
            for (i = 0; i < mesh->mesh->num_faces; i++)
                mesh->faces[i].lightmap = -1;
        }
        if (!S3d_load_lightmaps) {
            for (i = 0; i < mesh->mesh->num_faces; i++)
                mesh->faces[i].lightmap = -1;
        }
    }
}

// 0x46f100  Confidence: likely. The lightmaps: (w, h) int16 then w * h 16-bit texels, read through the
// 128 KB buffer into a format-5 bitmap and paged in, or skipped when S3d_load_lightmaps is off. The
// size check is the vsdk error loop (line 0xa7b).
void s3d_load_lightmap(s3d_level *level, File *f)
{
    int i;
    int w;
    int h;
    int size;

    if (level->num_lightmaps == 0) {
        level->lightmap_bms = NULL;
        return;
    }
    level->lightmap_bms = level->int_pool + s3d_int_cursor;
    s3d_int_cursor += level->num_lightmaps;
    for (i = 0; i < level->num_lightmaps; i++) {
        w = f->read_int16(0, 0);
        h = f->read_int16(0, 0);
        size = w * h * 2;
        if (S3d_load_lightmaps) {
            if (size > 0x20000)
                VSDK_ERROR(S3D_CPP, 0xa7b, "Lightmap too big to be loaded!");
            f->read(s3d_lightmap_buffer, size, 0, 0);
            level->lightmap_bms[i] = bm_create(5, w, h, s3d_lightmap_buffer, NULL, -1);
            gr_page_in(level->lightmap_bms[i], 1);
        } else {
            f->seek(size, 1);
            level->lightmap_bms[i] = -1;
        }
    }
    s3d_lightmaps_loaded = (S3d_load_lightmaps != 0);
}

// 0x46f210  Confidence: exact.
void s3d_read_vertex_set(s3d_level *level, s3d_vertex_set *rec, File *f)
{
    f->sync_int(&rec->count, 0, 0);
    f->sync_int(&rec->_unk04, 0, 0);
    f->sync_uint8(&rec->_unk08[0], 0, 0);
    f->sync_uint8(&rec->_unk08[1], 0, 0);
    f->sync_uint8(&rec->_unk08[2], 0, 0);
    f->sync_uint8(&rec->_unk08[3], 0, 0);
    if (f->check_flags(1)) {
        rec->verts = level->vertex_pool + s3d_vertex_cursor;
        s3d_vertex_cursor += rec->count;
    }
    file_sync_vector_array(f, rec->verts, rec->count, 0, &file_zero_vector);
}

// 0x46f2c0  Confidence: likely. The orientation is a quaternion in the file. In write mode the
// quaternion is built as a temporary (the pointer constructor 0x522790, D806) and copied into q with
// the inline operator= before writing.
void s3d_read_navpoint(s3d_level *level, s3d_navpoint *np, File *f)
{
    quaternion q;

    if (f->check_flags(1)) {
        char name[0x20];
        f->sync_string_buf(name, 0x20, 0, 0);
        np->name = level->names.Add(name);
    } else
        f->sync_string_buf(np->name, -1, 0, 0);
    if (f->check_flags(1)) {
        f->read_quaternion(&q, 0, &file_identity_quaternion);
        q.to_matrix_b(&np->orient);
    } else {
        q = quaternion(&np->orient);
        f->write_quaternion(&q);
    }
    f->sync_vector(&np->pos, 0, &file_zero_vector);
}

// 0x46f3a0  Confidence: exact.
void s3d_read_named_path(s3d_level *level, s3d_named_path *rec, File *f)
{
    char name[0x20];

    if (f->check_flags(1)) {
        f->sync_string_buf(name, 0x20, 0, 0);
        rec->name = level->names.Add(name);
    } else
        f->sync_string_buf(rec->name, -1, 0, 0);
    f->sync_int(&rec->_unk04, 0, 0);
    f->sync_int(&rec->count, 0, 0);
    if (f->check_flags(1)) {
        rec->points = level->vertex_pool + s3d_vertex_cursor;
        s3d_vertex_cursor += rec->count;
    }
    file_sync_vector_array(f, rec->points, rec->count, 0, &file_zero_vector);
}

// 0x46f460  Confidence: likely (the type goes through a local int and back, so the field is probably
// not a plain int in the original: an enum or a narrower type stored as a dword).
void s3d_read_light(s3d_light *rec, File *f)
{
    int type;

    type = rec->type;
    f->sync_int(&type, 0, 0);
    rec->type = type;
    f->sync_quaternion(&rec->orient, 0, &file_identity_quaternion);
    f->sync_vector(&rec->pos, 0, &file_zero_vector);
    f->sync_float(&rec->r, 0, 0);
    f->sync_float(&rec->g, 0, 0);
    f->sync_float(&rec->b, 0, 0);
    f->sync_float(&rec->intensity, 0, 0);
    f->sync_float(&rec->_unk30, 0, 0);
    f->sync_float(&rec->_unk34, 0, 0);
    f->sync_float(&rec->range_a, 0, 0);
    f->sync_float(&rec->range_b, 0, 0);
    f->sync_int(&rec->flags, 0, 0);
    f->sync_float(&rec->_unk44, 0, 0);
    f->sync_int(&rec->texture, 0, 0);
}

// 0x46f560  Confidence: likely. The count goes through an int local (the dead file-argument slot); the
// indices are read with read_int, not sync_int.
void s3d_read_vis_set(s3d_level *level, s3d_vis_set *rec, File *f)
{
    char name[0x20];
    int count;
    int i;

    if (f->check_flags(1)) {
        f->sync_string_buf(name, 0x20, 0, 0);
        rec->name = level->names.Add(name);
    } else {
        f->sync_string_buf(rec->name, -1, 0, 0);
        count = rec->count;
    }
    f->sync_int(&count, 0, 0);
    if (f->check_flags(1)) {
        rec->count = (ushort)count;
        rec->indices = level->int_pool + s3d_int_cursor;
        s3d_int_cursor += rec->count;
        for (i = 0; i < rec->count; i++)
            rec->indices[i] = f->read_int(0, 0);
    } else {
        for (i = 0; i < rec->count; i++)
            f->write_int(rec->indices[i]);
    }
}

// 0x46f660  Confidence: exact. The names blob itself is only read (sync_bytes inside the read branch).
void s3d_read_texture_names(s3d_level *level, File *f)
{
    f->sync_int(&level->texture_names_size, 0, 0);
    if (f->check_flags(1)) {
        level->texture_bms = level->int_pool + s3d_int_cursor;
        s3d_int_cursor += level->num_textures;
        f->sync_bytes(level->texture_names, level->texture_names_size, 0);
    }
}

// 0x46f6d0  Confidence: exact. The version becomes the file's version for the version-gated readers.
bool s3d_read_header(s3d_level *level, File *f)
{
    f->sync_int(&level->version, 0, 0);
    f->set_version(level->version);
    f->sync_int(&level->num_geometry, 0, 0);
    f->sync_int(&level->num_meshes, 0, 0);
    f->sync_int(&level->num_navpoints, 0, 0);
    f->sync_int(&level->num_vertex_sets, 0, 0);
    f->sync_int(&level->num_named_paths, 0, 0);
    f->sync_int(&level->num_lights, 0, 0);
    f->sync_int(&level->num_vis_sets, 0, 0);
    f->sync_int(&level->num_door_records, 0, 0);
    f->sync_int(&level->num_lightmaps, 0, 0);
    f->sync_int(&level->num_textures, 0, 0);
    f->sync_int(&level->num_textured, 0, 0);
    f->sync_int(&level->num_pool_faces, 0, 0);
    f->sync_int(&level->num_pool_mesh_faces, 0, 0);
    f->sync_int(&level->num_pool_normals, 0, 0);
    f->sync_int(&level->num_pool_vertices, 0, 0);
    f->sync_int(&level->num_pool_ints, 0, 0);
    f->sync_int(&level->num_pool_bytes, 0, 0);
    f->sync_int(&level->texture_names_size, 0, 0);
    f->sync_int(&level->num_color_records, 0, 0);
    f->sync_float(&level->ambient.x, 0, 0);
    f->sync_float(&level->ambient.y, 0, 0);
    f->sync_float(&level->ambient.z, 0, 0);
    if (f->error() == 0)
        return true;
    return false;
}

// 0x46f870  Confidence: likely (the face flag is set with a byte or: the field may be a byte in the
// original). A face whose texture has alpha gets flag 0x20 in the shared geometry, its object 0x10.
void s3d_mark_alpha_meshes(s3d_level *level)
{
    int i, j;
    int n;
    int tex;
    lobj *m;
    s3d_face *face;
    bool alpha;

    for (i = 0; i < level->num_meshes; i++) {
        m = &level->meshes[i];
        alpha = false;
        n = m->mesh->num_faces;
        for (j = 0; j < n; j++) {
            face = &m->mesh->faces[j];
            tex = m->faces[j].texture;
            if (tex && bm_has_alpha(level->texture_bms[tex - 1])) {
                face->flags |= 0x20;
                alpha = true;
            }
        }
        if (alpha)
            m->flags |= 0x10;
    }
}

// 0x46f930  Confidence: exact.
void s3d_load_textures(s3d_level *level)
{
    int i;
    char *name;

    name = level->texture_names;
    for (i = 0; i < level->num_textures; i++) {
        level->texture_bms[i] = bm_load(name, path_maps_levels, -1);
        progress_update(0);
        name += strlen(name) + 1;
    }
}

// 0x46f990  Confidence: likely. Every array of the level, in the level mempool (reset to its start).
// EH states 0-2 are the compiler's for the three `new T[n]` whose constructors are out of line
// (s3d_geometry: eh vector constructor iterator with the count cookie; lobj and door_info: inline
// loops). The other element types have empty inline constructors (the loop counter survives as the
// `dec / mov [esp+0x20]` vestige): vector3 / matrix / quaternion by D500, and vector2, whose
// constructor is not declared yet (NOTES.md).
void s3d_alloc_level_arrays(s3d_level *level)
{
    int i;

    level_mempool.offset = 0;
    set_current_mempool(&level_mempool);
    level->texture_names = NULL;
    if (level->num_textures > 0)
        level->texture_names = new char[level->texture_names_size];
    level->geometry = NULL;
    if (level->num_geometry > 0)
        level->geometry = new s3d_geometry[level->num_geometry];
    level->meshes = NULL;
    if (level->num_meshes > 0)
        level->meshes = new lobj[level->num_meshes];
    level->navpoints = NULL;
    if (level->num_navpoints > 0)
        level->navpoints = new s3d_navpoint[level->num_navpoints];
    level->vertex_sets = NULL;
    if (level->num_vertex_sets > 0)
        level->vertex_sets = new s3d_vertex_set[level->num_vertex_sets];
    level->named_paths = NULL;
    if (level->num_named_paths > 0)
        level->named_paths = new s3d_named_path[level->num_named_paths];
    level->lights = NULL;
    if (level->num_lights > 0)
        level->lights = new s3d_light[level->num_lights];
    level->vis_sets = NULL;
    if (level->num_vis_sets > 0)
        level->vis_sets = new s3d_vis_set[level->num_vis_sets];
    level->color_records = NULL;
    if (level->num_color_records > 0)
        level->color_records = new s3d_color_record[level->num_color_records];
    level->textured = NULL;
    if (level->num_textured > 0) {
        level->textured = new s3d_textured_record[level->num_textured];
        for (i = 0; i < level->num_textured; i++)
            level->textured[i].weights = NULL;
    }
    level->door_records = NULL;
    level->door_names = NULL;
    if (level->num_door_records > 0) {
        level->door_records = new door_info[level->num_door_records];
        level->door_names = new s3d_door_name[level->num_door_records];
    }
    level->face_pool = NULL;
    if (level->num_pool_faces > 0)
        level->face_pool = new s3d_face[level->num_pool_faces];
    level->mesh_face_pool = NULL;
    if (level->num_pool_mesh_faces > 0)
        level->mesh_face_pool = new lobj_face[level->num_pool_mesh_faces];
    level->byte_pool = NULL;
    level->vertex_pool = NULL;
    level->normal_pool = NULL;
    level->uv_pool = NULL;
    level->short_pool = NULL;
    if (level->num_pool_vertices > 0)
        level->vertex_pool = new vector3[level->num_pool_vertices];
    if (level->num_pool_normals > 0) {
        level->normal_pool = new vector3[level->num_pool_normals];
        level->uv_pool = new vector2[level->num_pool_normals];
        level->short_pool = new short[level->num_pool_normals];
    }
    if (level->num_pool_bytes)
        level->byte_pool = new ubyte[level->num_pool_bytes];
    level->int_pool = NULL;
    if (level->num_pool_ints > 0)
        level->int_pool = new int[level->num_pool_ints];
    s3d_vertex_cursor = 0;
    s3d_face_cursor = 0;
    s3d_mesh_face_cursor = 0;
    s3d_int_cursor = 0;
    s3d_normal_cursor = 0;
    s3d_cursor_22c4384 = 0;
    s3d_cursor_22e4414 = 0;
    s3d_cursor_22e43b4 = 0;
    s3d_uv_cursor = 0;
    s3d_byte_cursor = 0;
    s3d_short_cursor = 0;
    set_current_mempool(NULL);
}

// 0x46fdb0 s3d_geometry__ctor: the compiler's constructor of s3d_geometry (s3d_geom_block() on +0x38,
// returns this). Confidence: exact (compiler-generated).

// 0x46fdc0  Confidence: likely. Caller level_unload. The arrays are not freed one by one: they live in
// level_mempool, which s3d_alloc_level_arrays resets. Note that the navpoint pointer (+0x3c) is cleared
// but not the navpoint count (+0x38), and the colour-record pointer but not its count (NOTES).
void level_free_s3d(s3d_level *level)
{
    int i;
    int bm;
    void *data;
    void *palette;

    if (level_vis_base_built && level_vis_base_count) {
        delete level_vis_base_lobjs;
        level_vis_base_lobjs = NULL;
        level_vis_base_built = false;
    }
    level->names.Release();
    if (level->texture_bms) {
        for (i = 0; i < level->num_lights; i++) {
            if (level->lights[i].texture >= 0) {
                bm = level->texture_bms[level->lights[i].texture];
                bm_get_data(bm, &data, &palette);
                if (data)
                    bm_unlock(bm);
            }
        }
    }
    if (S3d_load_lightmaps) {
        for (i = 0; i < level->num_lightmaps; i++) {
            if (level->lightmap_bms[i] != -1)
                bm_release(level->lightmap_bms[i]);
            level->lightmap_bms[i] = -1;
        }
    }
    level->num_lightmaps = 0;
    level->num_textures = 0;
    level->num_textured = 0;
    level->textured = NULL;
    level->num_geometry = 0;
    level->num_meshes = 0;
    level->navpoints = NULL;
    level->num_vertex_sets = 0;
    level->num_named_paths = 0;
    level->num_lights = 0;
    level->num_vis_sets = 0;
    level->face_pool = NULL;
    level->vertex_pool = NULL;
    level->int_pool = NULL;
    level->normal_pool = NULL;
    level->color_records = NULL;
    level->mesh_face_pool = NULL;
    level->num_pool_mesh_faces = 0;
    level->num_door_records = 0;
    level->flags = 0;
}

// 0x46fef0  Confidence: exact. Caller level_page_in_characters.
void level_page_in_textures()
{
    int i;
    int start_mem;

    start_mem = mem_bytes_allocated;
    for (i = 0; i < level_num_textures; i++) {
        gr_page_in(level_texture_bms[i], 0);
        progress_update(0);
    }
    level_texture_mem += mem_bytes_allocated - start_mem;
    level_s3d.flags |= 2;
}

// 0x46ff50  Confidence: exact.
int s3d_read_face(File *f, s3d_face *face)
{
    int i;

    f->sync_uint16(&face->flags, 0, 0);
    for (i = 0; i < 3; i++)
        f->sync_uint16(&face->v[i], 0, 0);
    f->sync_vector(&face->normal, 0, &file_zero_vector);
    return f->error();
}

// 0x46ffa0  Confidence: exact. Returns the first nonzero result.
int s3d_read_faces(File *f, s3d_face *faces, int count)
{
    int i;
    int err;

    for (i = 0; i < count; i++) {
        err = s3d_read_face(f, &faces[i]);
        if (err)
            return err;
    }
    return 0;
}

// 0x46ffe0  Confidence: likely. Before version 0x10012 each face reference carried its lightmap box;
// it is kept (in the object's box array, allocated on first use) when the face has a lightmap. The
// lightmap index and the byte share a dword (bit fields, D801); the corner copies are vector3's inline
// operator= (D500).
// The write branch comes first in the code (`jne` to the read branch).
int s3d_read_mesh_ref(File *f, lobj *mesh, lobj_face *ref)
{
    int lightmap;
    vector3 mins;
    vector3 maxs;
    short hi;
    int i;
    s3d_box *box;

    if (f->get_version() < 0x10012) {
        for (i = 0; i < 3; i++)
            f->sync_float(&(&mins.x)[i], 0, 0);
        for (i = 0; i < 3; i++)
            f->sync_float(&(&maxs.x)[i], 0, 0);
    }
    f->sync_uint16(&ref->texture, 0, 0);
    if (!f->check_flags(1)) {
        hi = ref->_unk7;
        lightmap = ref->lightmap;
        f->sync_int(&lightmap, 0, 0);
        f->sync_int16(&hi, 0, 0);
    } else {
        f->sync_int(&lightmap, 0, 0);
        f->sync_int16(&hi, 0, 0);
        ref->lightmap = lightmap;
        ref->_unk7 = hi;
    }
    if (f->get_version() < 0x10012 && ref->lightmap >= 0) {
        if (!mesh->lightmap_boxes)
            mesh->lightmap_boxes = (s3d_box *)level_mempool.allocate_in_mempool(mesh->mesh->num_faces * sizeof(s3d_box));
        box = &mesh->lightmap_boxes[ref - mesh->faces];
        box->mins = mins;
        box->maxs = maxs;
    }
    return f->error();
}

// 0x470150  Confidence: exact (unsigned count: jbe / jb).
int s3d_read_mesh_refs(File *f, lobj *mesh, lobj_face *refs, unsigned int count)
{
    unsigned int i;
    int err;

    for (i = 0; i < count; i++) {
        err = s3d_read_mesh_ref(f, mesh, &refs[i]);
        if (err)
            return err;
    }
    return 0;
}

// 0x470190  Confidence: likely (the component loops over a vector: the original may index the vector
// through an inline operator[]).
int s3d_read_box(File *f, s3d_box *box)
{
    int i;

    for (i = 0; i < 3; i++)
        f->sync_float(&(&box->mins.x)[i], 0, 0);
    for (i = 0; i < 3; i++)
        f->sync_float(&(&box->maxs.x)[i], 0, 0);
    return f->error();
}

// 0x4701e0  Confidence: exact.
int s3d_read_boxes(File *f, s3d_box *boxes, unsigned int count)
{
    unsigned int i;
    int err;

    for (i = 0; i < count; i++) {
        err = s3d_read_box(f, &boxes[i]);
        if (err)
            return err;
    }
    return 0;
}

// 0x470220  Confidence: likely. Read only (no sync): the door's object, named path, the two poses,
// then the run-time state is reset. The level argument is unused. It returns -1 when the file has no
// error and 0 when it has one (neg / sbb / neg / dec), the opposite of the other readers; the caller
// ignores it.
int s3d_read_door(s3d_level *level, File *f, door_info *door)
{
    door->lobj_index = f->read_int(0, 0);
    door->named_path = f->read_int16(0, 0);
    door->_unk06 = f->read_int16(0, 0);
    door->_unk08 = f->read_float(0, 0);
    f->read_vector(&door->closed_pos, 0, &file_zero_vector);
    f->read_quaternion(&door->closed_orient, 0, &file_identity_quaternion);
    f->read_vector(&door->open_pos, 0, &file_zero_vector);
    f->read_quaternion(&door->open_orient, 0, &file_identity_quaternion);
    door->timer.invalidate();
    door->lock = 0;
    door->state &= ~0xb;
    door->_unk5c = 0;
    door->_unk48[0] = 0;
    door->_unk48[1] = 0;
    door->_unk48[2] = 0;
    door->_unk60 = 0;
    door->_unk64 = -1;
    door->_unk68[0] = 0;
    door->_unk68[1] = 0;
    door->_unk68[2] = 0;
    door->_unk6c = 0;
    door->_unk70 = -1;
    door->_unk74[0] = 0;
    door->_unk74[1] = 0;
    door->_unk74[2] = 0;
    return f->error() ? 0 : -1;
}

// 0x470300  Confidence: likely (the clamp shape). The colour is clamped to 0..1 with a local copy;
// the third clamp reads green but stores into blue, with the upper bound 0.998: green is never
// clamped and blue is replaced by the clamped green (bug; the records are not read anywhere, NOTES).
void s3d_read_color_record(File *f, s3d_color_record *rec)
{
    float v;

    f->sync_float(&rec->r, 0, 0);
    f->sync_float(&rec->g, 0, 0);
    f->sync_float(&rec->b, 0, 0);
    f->sync_int(&rec->_unk0c, 0, 0);
    v = rec->r;
    rec->r = (v > 0.0f) ? ((v < 1.0f) ? v : 1.0f) : 0.0f;
    v = rec->b;
    rec->b = (v > 0.0f) ? ((v < 1.0f) ? v : 1.0f) : 0.0f;
    v = rec->g;
    rec->b = (v > 0.0f) ? ((v < 0.998f) ? v : 0.998f) : 0.0f;
}

// 0x470410  Confidence: likely. The bitmap is stored by name (empty = none).
int s3d_read_textured_record(File *f, s3d_textured_record *rec)
{
    char name[0xdc];
    unsigned int i;

    f->sync_int(&rec->lobj, 0, 0);
    f->sync_float(&rec->scale_x, 0, 0);
    f->sync_float(&rec->scale_z, 0, 0);
    f->sync_float(&rec->amplitude, 0, 0);
    f->sync_float(&rec->period, 0, 0);
    if (f->check_flags(1)) {
        f->read_string_buf(name, 0xdb, 0, 0);
        if (name[0])
            rec->bm = bm_load(name, -1, -1);
        else
            rec->bm = -1;
    } else
        f->write_string(rec->bm != -1 ? bm_get_filename(rec->bm) : "", -1);
    f->sync_uint(&rec->num_weights, 0, 0);
    if (f->check_flags(1)) {
        rec->weights = new float[rec->num_weights];
        if (!rec->weights)
            return -1;
    }
    for (i = 0; i < rec->num_weights; i++)
        f->sync_float(&rec->weights[i], 0, 0);
    return f->error() ? -1 : 0;
}

// 0x470560  Confidence: exact.
int s3d_read_textured_records(File *f, s3d_textured_record *recs, unsigned int count)
{
    unsigned int i;

    for (i = 0; i < count; i++) {
        if (s3d_read_textured_record(f, &recs[i]))
            return -1;
    }
    return f->error() ? -1 : 0;
}
