// vsdk/ca/unnamed_ca_mesh_0x50af80.h - the character mesh renderer (vsdk\ca in sum.symbols; one object
// 0x50af80-0x50d9ab, source src/decomp/vsdk/unnamed_ca_mesh_0x50af80.cpp; file name a placeholder, no path
// string or strings.tbl block; decomp65). Included at the end of vsdk/ca/character.h, which declares
// ca_render_set_instance, ca_render_set_mesh and ca_render_mesh. Not original source; names ours unless
// sum.symbols names them.
#ifndef VSDK_CA_UNNAMED_CA_MESH_0X50AF80_H
#define VSDK_CA_UNNAMED_CA_MESH_0X50AF80_H

#include "vsdk/vsdk.h"
#include "vsdk/ca/character.h"

class character_instance;

// What the renderers read from a vim_submesh (vsdk/ca/skeleton.h keeps its generic names):
//   block0  vector3 per vertex, the bind positions        block1  vector3 per vertex, the normals
//   block2  float u, v per vertex                          block3  ca_mesh_tri per triangle
//   block4  short per vertex: 0 or more = a copy of the vertex that many places back (a uv seam), -1 own
//   block5  8 bytes per vertex: four bone weights (sum 16, 0 = unused), then the four bone indices
//   _unk18  the texture slot (vim::textures, or the gr_light_desc appearance table), -1 = not drawn
//   vertex_extra  short per morph target id: the vertex it moves
//   size0   vertex count (at most CA_MAX_MESH_VERTS)    size1  triangle count
#define CA_MAX_MESH_VERTS   2000

// One triangle of a sub-mesh (0x10 bytes, vim_submesh::block3).
struct ca_mesh_tri {
    int v[3];                         // +0x00 vertex indices
    short flags;                      // +0x0c group bits (hidden when they meet gr_light_desc +0x1c << 6),
                                      //       CA_TRI_TWO_SIDED
    short _pad0e;
};
LAYOUT_CHECK(ca_mesh_tri, sizeof(ca_mesh_tri) == 0x10);
#define CA_TRI_TWO_SIDED 0x20         // no back-face test

// ---- .data (0x5a7280-0x5a7293) ----------------------------------------------------------------------
extern int gr_reflection_map_bm;                      // 0x5a7280 -1, refmap01.tga after gr_load_reflection_map

// ---- .bss (0x25db930-0x2682997; the work buffers are in the .cpp) -----------------------------------
extern vector3 ca_morph_vertices[CA_MAX_MESH_VERTS];  // 0x266f108
extern vim *ca_render_current_mesh;                   // 0x2682988 (sum.symbols void *)
extern ca_character *ca_render_character;             // 0x268298c (name ours)
extern character_instance *ca_render_instance;        // 0x2682990 (name ours)
extern int ca_render_flags;                           // 0x2682994 written by ca_render_set_flags only

// ---- functions ------------------------------------------------------------------------------------
// (sub-mesh): ca_morph_vertices = the bind positions with the morph keys of the instance's current
// animation applied.
void ca_render_morph_vertices(vim_submesh *sm);       // 0x50b150
// (t): the Bezier ramp through ca_ramp_points at t, times 255 (no caller).
int  ca_ramp_curve_eval(float t);                     // 0x50b450
// The three back ends of ca_render_mesh: (mesh, pos, orient, light, 1, instance set, -1, -1). The fifth,
// seventh and eighth arguments are not read.
void gr_glide_render_character_mesh(vim *mesh, vector3 *pos, matrix *orient, gr_light_desc *light,
                                    int a5, int has_instance, int a7, int a8);   // 0x50b4c0
void gr_d3d_render_character_mesh(vim *mesh, vector3 *pos, matrix *orient, gr_light_desc *light,
                                  int a5, int has_instance, int a7, int a8);     // 0x50c130
void gr_opengl_render_character_mesh(vim *mesh, vector3 *pos, matrix *orient, gr_light_desc *light,
                                     int a5, int has_instance, int a7, int a8);  // 0x50cd00
int  ca_render_has_mesh();                            // 0x50d7b0 mesh != NULL (no caller)
void ca_render_set_flags(int flags);                  // 0x50d7c0 (no caller)
void ca_render_stub_ret();                            // 0x50d7d0 empty (no caller)

#endif
