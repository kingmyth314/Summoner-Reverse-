// vsdk/gr/unnamed_poly_split_0x55f110.h - the splitting of multipass polygons along texture section lines
// (vsdk\gr, 0x55f110-0x55fc6f; source at src/decomp/vsdk/unnamed_poly_split_0x55f110.cpp, decomp66; file
// name a placeholder). gr_poly_get_section (0x55fb00) is declared in gr.h (decomp36). Not original source.
#ifndef VSDK_GR_UNNAMED_POLY_SPLIT_0X55F110_H
#define VSDK_GR_UNNAMED_POLY_SPLIT_0X55F110_H

#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"

// A polygon piece (0x8c bytes; 64 in a pool rebuilt by each gr_poly_split_sections_draw call; names ours).
#define GR_POLY_SPLIT_MAX_VERTS 32
#define GR_POLY_SPLIT_POOL      64
struct gr_poly_split_rec {
    int nv;                                       // +0x00
    gr_multipass_vertex *verts[GR_POLY_SPLIT_MAX_VERTS];   // +0x04
    gr_poly_split_rec *next;                      // +0x84
    gr_poly_split_rec *prev;                      // +0x88
};
LAYOUT_CHECK_32(gr_poly_split_rec, sizeof(gr_poly_split_rec) == 0x8c);

// .bss 0x2f8f750-0x2f91a5b (0x2f8f754 has no reference).
extern gr_poly_split_rec *gr_poly_split_free_list;   // 0x2f8f750
extern gr_poly_split_rec gr_poly_split_pool[GR_POLY_SPLIT_POOL];   // 0x2f8f758
extern gr_poly_split_rec *gr_poly_split_active_list; // 0x2f91a58

// Another vsdk\gr object (the buffered polygon code): the next multipass vertex of the clip pool (its
// gr_vertex flags cleared). decomp69: a header inline of that unit (0x546130), defined in its header.
#include "vsdk/gr/unnamed_gr_poly_0x5450c0.h"

// (nv, verts, npasses, passes): cuts the polygon along every 256-texel line of each sectioned pass's
// bitmap (both axes), then draws the pieces with gr_tmapper_multipass. Nothing is drawn when the 64
// pieces run out.
void gr_poly_split_sections_draw(int nv, gr_multipass_vertex **verts, int npasses, gr_tmapper_pass *passes);   // 0x55f110
// (piece, out a, out b, npasses, passes, pass, axis 0 u / 1 v, line): false when the line does not cut it.
bool gr_poly_split_at_tex_line(gr_poly_split_rec *p, gr_poly_split_rec *a, gr_poly_split_rec *b, int npasses,
                               gr_tmapper_pass *passes, int pass, int axis, float line);   // 0x55f580
bool float_strictly_between(float a, float b, float x);   // 0x55f8b0 x inside (min, max), ends excluded
// (a, b, t, npasses, passes): a new vertex t of the way from a to b (position, then each pass's uv /
// colour / alpha as the pass uses them), projected.
gr_multipass_vertex *gr_poly_split_edge_vertex(gr_multipass_vertex *a, gr_multipass_vertex *b, float t,
                                               int npasses, gr_tmapper_pass *passes);   // 0x55f900

#endif
