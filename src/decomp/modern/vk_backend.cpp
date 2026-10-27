// vk_backend.cpp - the Vulkan back end (rung L4, docs/future/vulkan-renderer.md). Not original source. 64-bit
// MODERN_BUILD only; with the Vulkan SDK's headers (MODERN_VULKAN) it is built, without them vk_init fails and the
// game uses OpenGL.
//
// M0 (the build wiring): vk_init loads vulkan-1.dll (the graphics driver's loader), creates an instance (with the
// Khronos validation layer when [Debug] VulkanValidation=1), logs the GPUs and the embedded shaders, and returns
// false, so the game keeps OpenGL. The entry points are stubs until M1 / M2.

#ifdef _WIN64
#include <string.h>
#include "vsdk/vsdk.h"
#include "modern/modern.h"
#include "modern/vk.h"

#ifdef MODERN_VULKAN
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include "vk_shaders.h"

static HMODULE vk_lib;
static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_;

// Loads the loader and the global functions; false when the driver has no Vulkan.
static bool vk_load_loader()
{
    if (!vk_lib)
        vk_lib = LoadLibraryA("vulkan-1.dll");
    if (!vk_lib) {
        modern_log("vulkan: no vulkan-1.dll (the graphics driver has no Vulkan)");
        return false;
    }
    vkGetInstanceProcAddr_ = (PFN_vkGetInstanceProcAddr)(void *)GetProcAddress(vk_lib, "vkGetInstanceProcAddr");
    return vkGetInstanceProcAddr_ != NULL;
}

#define VK_GLOBAL(name) ((PFN_##name)vkGetInstanceProcAddr_(NULL, #name))
#define VK_INSTANCE(inst, name) ((PFN_##name)vkGetInstanceProcAddr_(inst, #name))

static bool vk_has_layer(const char *name)
{
    PFN_vkEnumerateInstanceLayerProperties enum_layers = VK_GLOBAL(vkEnumerateInstanceLayerProperties);
    uint32_t n = 0;
    if (!enum_layers || enum_layers(&n, NULL) != VK_SUCCESS || n == 0)
        return false;
    VkLayerProperties *props = new VkLayerProperties[n];
    bool found = false;
    if (enum_layers(&n, props) == VK_SUCCESS)
        for (uint32_t i = 0; i < n && !found; i++)
            found = !strcmp(props[i].layerName, name);
    delete[] props;
    return found;
}

bool vk_init(HWND hwnd)
{
    if (!vk_load_loader())
        return false;
    PFN_vkCreateInstance create_instance = VK_GLOBAL(vkCreateInstance);
    if (!create_instance)
        return false;

    const char *validation = "VK_LAYER_KHRONOS_validation";
    bool want_validation = modern_ini_int("Debug", "VulkanValidation", 0) != 0;
    bool use_validation = want_validation && vk_has_layer(validation);
    if (want_validation && !use_validation)
        modern_log("vulkan: VulkanValidation=1 but %s is not installed (the Vulkan SDK)", validation);

    const char *extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
    VkApplicationInfo app = {};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "Summoner";
    app.pEngineName = "VSDK";
    app.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = 2;
    ci.ppEnabledExtensionNames = extensions;
    ci.enabledLayerCount = use_validation ? 1 : 0;
    ci.ppEnabledLayerNames = &validation;
    VkInstance instance = VK_NULL_HANDLE;
    VkResult r = create_instance(&ci, NULL, &instance);
    if (r != VK_SUCCESS) {
        modern_log("vulkan: vkCreateInstance failed (%d)", (int)r);
        return false;
    }

    PFN_vkEnumeratePhysicalDevices enum_devices = VK_INSTANCE(instance, vkEnumeratePhysicalDevices);
    PFN_vkGetPhysicalDeviceProperties get_props = VK_INSTANCE(instance, vkGetPhysicalDeviceProperties);
    uint32_t n = 0;
    enum_devices(instance, &n, NULL);
    VkPhysicalDevice devices[8];
    if (n > 8)
        n = 8;
    enum_devices(instance, &n, devices);
    for (uint32_t i = 0; i < n; i++) {
        VkPhysicalDeviceProperties p;
        get_props(devices[i], &p);
        modern_log("vulkan: GPU %u: %s, Vulkan %u.%u.%u%s", i, p.deviceName, VK_API_VERSION_MAJOR(p.apiVersion),
                   VK_API_VERSION_MINOR(p.apiVersion), VK_API_VERSION_PATCH(p.apiVersion),
                   use_validation ? ", validation on" : "");
    }
    modern_log("vulkan: shaders gr.vert %u bytes, gr.frag %u bytes", (unsigned)sizeof(vk_spv_gr_vert),
               (unsigned)sizeof(vk_spv_gr_frag));

    VK_INSTANCE(instance, vkDestroyInstance)(instance, NULL);
    modern_log("vulkan: the renderer is not implemented yet (M0); using OpenGL");
    return false;
}
#else
bool vk_init(HWND hwnd)
{
    modern_log("vulkan: this exe was built without the Vulkan SDK (VULKAN_SDK); using OpenGL");
    return false;
}
#endif

// ---- the back end's entry points (M1 / M2) ----
static gr::Mode vk_current_mode;

void vk_init_context(HWND hwnd) {}
void vk_gr_init(HWND hwnd) {}
void vk_gr_opengl_tcache_close() {}
void vk_gr_opengl_texture_init() {}
void vk_gr_opengl_tcache_flush() {}
void vk_gr_opengl_tcache_frame() {}
int vk_gr_opengl_tcache_set(int bm, float *u_scale, float *v_scale, int stage) { return 0; }
void vk_gr_opengl_mark_texture_dirty(int bm) {}
void vk_gr_opengl_tcache_free(int bm) {}
gr::Mode *vk_gr_opengl_get_current_mode() { return &vk_current_mode; }
void vk_gr_opengl_set_mode(gr::Mode mode) { vk_current_mode = mode; }
void vk_gr_opengl_page_in(int bm, int stage) {}
void vk_gr_opengl_set_bitmap(gr::Mode mode, int bm, int stage) {}
void vk_gr_opengl_close() {}
void vk_gr_opengl_flip() {}
void vk_gr_opengl_clear() {}
void vk_gr_opengl_zbuffer_clear() {}
void vk_gr_opengl_set_viewport() {}
void vk_gr_opengl_apply_tint(ubyte *rgb) {}
void vk_gr_opengl_bitmap(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh, gr::Mode mode) {}
void vk_gr_opengl_set_color() {}
void vk_gr_opengl_line(float x1, float y1, float x2, float y2, gr::Mode mode) {}
void vk_gr_opengl_line_3d(gr_vertex *v1, gr_vertex *v2, gr::Mode mode) {}
void vk_gr_opengl_tmapper(int nv, gr_vertex **verts, gr::Mode mode) {}
int vk_gr_opengl_read_backbuffer(int x, int y, int w, int h, ubyte *buf) { return 0; }
void vk_gr_opengl_set_fog(int set_range) {}
void vk_gr_opengl_set_antialias(bool on) {}
void vk_levelrender_draw_list(lobj *l, int alpha, unsigned int num_lights, attached_light **lights) {}
void vk_render_character_mesh(vim *mesh, vector3 *pos, matrix *orient, gr_light_desc *light, int a5,
                              int has_instance, int a7, int a8) {}
#endif
