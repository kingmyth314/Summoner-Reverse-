// vsdk/gr/unnamed_gr_line_0x53e520.h - the 3D drawing helpers of the renderer front end (vsdk\gr,
// 0x53e520-0x54080f; source src/decomp/vsdk/unnamed_gr_line_0x53e520.cpp, decomp69; file name a
// placeholder): 3D lines, the polygon entry gr_poly, camera-facing and oriented bitmaps, line quads and the
// debug wire shapes. gr_line_3d, gr_line_3d_world, gr_set_use_buffered_alpha_polys, gr_clip_and_project_poly,
// gr_poly, gr_bitmap_3d, gr_line_quad_3d, gr_draw_sphere and gr_draw_arrow_3d are declared in gr.h. Not
// original source.
#ifndef VSDK_GR_UNNAMED_GR_LINE_0X53E520_H
#define VSDK_GR_UNNAMED_GR_LINE_0X53E520_H

#include "vsdk/vsdk.h"
#include "vsdk/math/vector.h"
#include "vsdk/math/matrix.h"
#include "vsdk/os/console.h"
#include "vsdk/gr/gr.h"

// ---- data (.bss 0x2dc7060-0x2dc706f, the literals 0x5abfe0-0x5ac0dd) ---------------------------------------
extern console_command ccmd_use_buffered_alpha_polys; // 0x2dc7060
extern int Use_buffered_alpha_polys;                  // 0x2dc706c console "use_buffered_alpha_polys"
void cmd_use_buffered_alpha_polys();                  // 0x53e6a0

// (n, verts, out): clips and projects the polygon and writes each result vertex's screen x / y to out;
// returns the vertex count, 0 when it is rejected or a vertex is behind the viewer. No caller.
int gr_poly_project_to_screen(int n, gr_vertex **verts, vector2 *out);   // 0x53e8d0
// (projected centre, angle, size, mode): a quad of the current texture's proportions around the point,
// rotated by angle (radians), drawn buffered with its depth pulled towards the viewer by the size.
bool gr_bitmap_3d_projected(gr_vertex *pnt, float angle, float size, gr::Mode mode);   // 0x53eae0
// (projected centre, angle, width, height, mode): the same quad with an explicit size (halved inside).
bool gr_bitmap_3d_oriented_quad(gr_vertex *pnt, float angle, float width, float height, gr::Mode mode);   // 0x53edd0
// (pos, orient, size, mode): a quad in the orient's right / up plane, drawn only when its forward vector
// faces the viewer; not buffered. No caller.
bool gr_bitmap_3d_oriented_quad_facing(const vector3 *pos, const matrix *orient, float size, gr::Mode mode);   // 0x53f060
// (pos, angle, width, height, mode): gr_bitmap_3d with the explicit size (the vfx mesh billboards).
bool gr_bitmap_3d_oriented(const vector3 *pos, float angle, float width, float height, gr::Mode mode);   // 0x53f410
// Debug lines in a colour (r, g, b; alpha 255) with gr_line_3d_mode, the points as six floats. No callers
// except gr_draw_axes_3d's calls of gr_draw_arrow_3d_between.
void gr_line_3d_points(float x0, float y0, float z0, float x1, float y1, float z1, int r, int g, int b);   // 0x53fff0
void gr_draw_arrow_3d_points(float x0, float y0, float z0, float x1, float y1, float z1, int r, int g, int b);   // 0x5400a0
void gr_draw_arrow_3d_between(float x0, float y0, float z0, float x1, float y1, float z1, int r, int g, int b);   // 0x540330
// (pos, orient, length): the three axes as red / green / blue arrows; the colour is restored.
void gr_draw_axes_3d(const vector3 *pos, const matrix *orient, float len);   // 0x540590

#endif
