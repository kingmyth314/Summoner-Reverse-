// vk_dispatch.cpp - the OpenGL back end's entry points, sent to the Vulkan back end when it is active (rung L4,
// docs/future/vulkan-renderer.md). Not original source. 64-bit MODERN_BUILD only.
//
// vsdk/gr_opengl.cpp defines these entry points as <name>_gl (include/modern/gl_redirect.h); this unit defines the
// original names, which the game calls. Vulkan runs in GR_OPENGL's slot, so the game's dispatch is unchanged.
// The choice is made once, at the first call into the back end (gr_init's gr_opengl_init_context or
// gr_opengl_init): [Display] Renderer=Vulkan and a Vulkan device that starts, else OpenGL as before.

#ifdef _WIN64
#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"
#include "vsdk/gr/opengl/gr_opengl.h"
#include "vsdk/ca/unnamed_ca_mesh_0x50af80.h"
#include "engine/rendering/levelrender.h"
#include "modern/modern.h"
#include "modern/vk.h"

#define DECLARE_GL(ret, name, params, args) ret name##_gl params;
GL_REDIRECT_ENTRIES(DECLARE_GL)
#undef DECLARE_GL
void gr_opengl_init_context_gl(HWND hwnd);
void gr_opengl_init_gl(HWND hwnd);
void levelrender_draw_list_opengl_gl(lobj *l, int alpha, unsigned int num_lights, attached_light **lights);
void gr_opengl_render_character_mesh_gl(vim *mesh, vector3 *pos, matrix *orient, gr_light_desc *light, int a5,
                                        int has_instance, int a7, int a8);

bool modern_vk_active;
static bool vk_decided;

// The first call into the back end, with the game's window: Vulkan if it is asked for and starts.
static void vk_decide(HWND hwnd)
{
    if (vk_decided)
        return;
    vk_decided = true;
    char r[32];
    modern_ini_str("Display", "Renderer", "OpenGL", r, sizeof r);
    if (_stricmp(r, "Vulkan"))
        return;
    modern_vk_active = vk_init(hwnd);
    modern_log("renderer: Vulkan %s", modern_vk_active ? "started" : "did not start; using OpenGL");
}

void gr_opengl_init_context(HWND hwnd)
{
    vk_decide(hwnd);
    if (modern_vk_active)
        vk_init_context(hwnd);
    else
        gr_opengl_init_context_gl(hwnd);
}

void gr_opengl_init(HWND hwnd)
{
    vk_decide(hwnd);
    if (modern_vk_active)
        vk_gr_init(hwnd);
    else
        gr_opengl_init_gl(hwnd);
}

#define DISPATCH(ret, name, params, args) \
    ret name params \
    { \
        if (modern_vk_active) \
            return vk_##name args; \
        return name##_gl args; \
    }
GL_REDIRECT_ENTRIES(DISPATCH)

void levelrender_draw_list_opengl(lobj *l, int alpha, unsigned int num_lights, attached_light **lights)
{
    if (modern_vk_active)
        vk_levelrender_draw_list(l, alpha, num_lights, lights);
    else
        levelrender_draw_list_opengl_gl(l, alpha, num_lights, lights);
}

void gr_opengl_render_character_mesh(vim *mesh, vector3 *pos, matrix *orient, gr_light_desc *light, int a5,
                                     int has_instance, int a7, int a8)
{
    if (modern_vk_active)
        vk_render_character_mesh(mesh, pos, orient, light, a5, has_instance, a7, a8);
    else
        gr_opengl_render_character_mesh_gl(mesh, pos, orient, light, a5, has_instance, a7, a8);
}
#endif
