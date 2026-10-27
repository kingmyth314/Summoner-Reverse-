// vk_scene.cpp - the Vulkan back end's level and character draw lists (rung L4, docs/future/vulkan-renderer.md).
// Not original source. 64-bit MODERN_BUILD with the Vulkan SDK only. See modern/vk_internal.h for the units.
// M3: gr_opengl_render_character_mesh in Vulkan. Until then nothing is drawn. The level draw list is in
// modern/vk_scene_level.cpp.

#if defined(_WIN64) && defined(MODERN_VULKAN)
#include "vk_internal.h"

void vk_render_character_mesh(vim *mesh, vector3 *pos, matrix *orient, gr_light_desc *light, int a5,
                              int has_instance, int a7, int a8)
{
}
#endif
