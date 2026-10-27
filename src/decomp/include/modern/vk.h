// modern/vk.h - the Vulkan back end (rung L4, docs/future/vulkan-renderer.md). Not original source. 64-bit
// MODERN_BUILD only; the Vulkan code itself needs the Vulkan SDK's headers (MODERN_VULKAN, set by link-build when
// VULKAN_SDK is set). Without them vk_init fails and the game uses OpenGL.
//
// The entry points have the OpenGL back end's signatures (include/modern/gl_redirect.h) with a vk_ prefix:
// modern/vk_dispatch.cpp calls them while modern_vk_active is set.
#ifndef MODERN_VK_H
#define MODERN_VK_H

#if defined(MODERN_BUILD) && defined(_WIN64)
#include "vsdk/gr/opengl/gr_opengl.h"

struct lobj;
class attached_light;
struct vim;

extern bool modern_vk_active;                  // vk_dispatch.cpp: the Vulkan back end is in use

bool vk_init(HWND hwnd);                       // device and swapchain for the game's window; false = use OpenGL
void vk_init_context(HWND hwnd);               // gr_opengl_init_context's place
void vk_gr_init(HWND hwnd);                    // gr_opengl_init's place

#define DECLARE_VK(ret, name, params, args) ret vk_##name params;
GL_REDIRECT_ENTRIES(DECLARE_VK)
#undef DECLARE_VK

void vk_levelrender_draw_list(lobj *l, int alpha, unsigned int num_lights, attached_light **lights);
void vk_render_character_mesh(vim *mesh, vector3 *pos, matrix *orient, gr_light_desc *light, int a5,
                              int has_instance, int a7, int a8);
#endif
#endif
