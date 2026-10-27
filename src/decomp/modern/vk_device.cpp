// vk_device.cpp - the Vulkan back end's device and frame (rung L4, docs/future/vulkan-renderer.md). Not original
// source. 64-bit MODERN_BUILD only. See modern/vk_internal.h for the units.
//
// Vulkan 1.3 (dynamic rendering, synchronization2, extended dynamic state) with descriptor indexing from 1.2. The
// game's frame is drawn straight into the swapchain image, mapped into the window as modern/display.cpp maps the
// OpenGL viewport. One command buffer per frame; texture uploads and read-backs inside a frame suspend the rendering
// pass, record their copy and resume it, so they are ordered with the draws as under OpenGL.
//
// Settings (summonerfix.ini): [Display] Renderer=Vulkan selects this back end; [Display] VulkanPresent = Fifo (the
// default: vsync), Mailbox or Immediate; [Debug] VulkanValidation=1 loads the Khronos validation layer (the Vulkan
// SDK) and logs its messages to summonerfix.log.

#ifdef _WIN64
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "vsdk/vsdk.h"
#include "modern/modern.h"
#include "modern/vk.h"

#ifdef MODERN_VULKAN
#include "vk_internal.h"

#define VK_DEFINE_FUNC(name) PFN_##name name;
VK_INSTANCE_FUNCS(VK_DEFINE_FUNC)
VK_DEVICE_FUNCS(VK_DEFINE_FUNC)
#undef VK_DEFINE_FUNC

vk_context vkc;
vk_frame vk_frames[VK_FRAMES];
int vk_frame_index;
uint32_t vk_image_index;
bool vk_recording;
VkSemaphore vk_render_done[8];

static HMODULE vk_lib;
static PFN_vkGetInstanceProcAddr vk_get_instance_proc;
static bool vk_rendering;                      // inside vkCmdBeginRendering
static bool vk_frame_cleared;                  // the frame's first rendering pass (cleared) has begun
static bool vk_acquire_waited;                 // a submission of this frame already waited for the acquire
static bool vk_skip_frame;                     // minimised, or the swapchain could not be made: nothing is drawn
static bool vk_context_initialized;          // vk_init_context has run once (the texture cache exists)

static void vk_destroy_transfer();

#define VK_VERTEX_RING (8u << 20)
#define VK_INDEX_RING (2u << 20)

void vk_log(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, sizeof buf - 1, fmt, ap);
    va_end(ap);
    buf[sizeof buf - 1] = 0;
    modern_log("vulkan: %s", buf);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                         VkDebugUtilsMessageTypeFlagsEXT, const
                                                         VkDebugUtilsMessengerCallbackDataEXT *data, void *)
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        vkc.validation_errors++;
    // The first 200 messages; a broken frame repeats its errors every frame.
    static int logged;
    if (logged++ < 200)
        vk_log("%s %s", severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? "ERROR" : "warning",
               data->pMessage);
    return VK_FALSE;
}

// ---- memory ----
uint32_t vk_memory_type(uint32_t type_bits, VkMemoryPropertyFlags flags)
{
    for (uint32_t i = 0; i < vkc.mem_props.memoryTypeCount; i++)
        if ((type_bits & (1u << i)) && (vkc.mem_props.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    return UINT32_MAX;
}

bool vk_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, bool host_visible, vk_buffer *out)
{
    memset(out, 0, sizeof *out);
    VkBufferCreateInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bi.size = size;
    bi.usage = usage;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(vkc.device, &bi, NULL, &out->buffer) != VK_SUCCESS)
        return false;
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(vkc.device, out->buffer, &req);
    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = vk_memory_type(req.memoryTypeBits, host_visible
        ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (ai.memoryTypeIndex == UINT32_MAX || vkAllocateMemory(vkc.device, &ai, NULL, &out->memory) != VK_SUCCESS) {
        vkDestroyBuffer(vkc.device, out->buffer, NULL);
        out->buffer = VK_NULL_HANDLE;
        return false;
    }
    vkBindBufferMemory(vkc.device, out->buffer, out->memory, 0);
    if (host_visible)
        vkMapMemory(vkc.device, out->memory, 0, VK_WHOLE_SIZE, 0, &out->mapped);
    out->size = size;
    return true;
}

void vk_destroy_buffer(vk_buffer *b)
{
    if (b->buffer)
        vkDestroyBuffer(vkc.device, b->buffer, NULL);
    if (b->memory)
        vkFreeMemory(vkc.device, b->memory, NULL);
    memset(b, 0, sizeof *b);
}

// ---- loader, instance, device ----
static bool vk_load_loader()
{
    if (!vk_lib)
        vk_lib = LoadLibraryA("vulkan-1.dll");
    if (!vk_lib) {
        vk_log("no vulkan-1.dll (the graphics driver has no Vulkan)");
        return false;
    }
    vk_get_instance_proc = (PFN_vkGetInstanceProcAddr)(void *)GetProcAddress(vk_lib, "vkGetInstanceProcAddr");
    return vk_get_instance_proc != NULL;
}

static bool vk_has_layer(const char *name)
{
    PFN_vkEnumerateInstanceLayerProperties enum_layers =
        (PFN_vkEnumerateInstanceLayerProperties)vk_get_instance_proc(NULL, "vkEnumerateInstanceLayerProperties");
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

static bool vk_create_instance()
{
    PFN_vkCreateInstance create_instance = (PFN_vkCreateInstance)vk_get_instance_proc(NULL, "vkCreateInstance");
    if (!create_instance)
        return false;
    const char *validation = "VK_LAYER_KHRONOS_validation";
    bool want_validation = modern_ini_int("Debug", "VulkanValidation", 0) != 0;
    bool use_validation = want_validation && vk_has_layer(validation);
    if (want_validation && !use_validation)
        vk_log("VulkanValidation=1 but %s is not installed (the Vulkan SDK)", validation);

    const char *extensions[3] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
                                  VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    VkApplicationInfo app = {};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "Summoner";
    app.pEngineName = "VSDK";
    app.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = use_validation ? 3 : 2;
    ci.ppEnabledExtensionNames = extensions;
    ci.enabledLayerCount = use_validation ? 1 : 0;
    ci.ppEnabledLayerNames = &validation;
    VkResult r = create_instance(&ci, NULL, &vkc.instance);
    if (r != VK_SUCCESS) {
        vk_log("vkCreateInstance failed (%d)", (int)r);
        return false;
    }
#define VK_LOAD_INSTANCE(name) name = (PFN_##name)vk_get_instance_proc(vkc.instance, #name);
    VK_INSTANCE_FUNCS(VK_LOAD_INSTANCE)
#undef VK_LOAD_INSTANCE
    if (use_validation && vkCreateDebugUtilsMessengerEXT) {
        VkDebugUtilsMessengerCreateInfoEXT mi = {};
        mi.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        mi.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        mi.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        mi.pfnUserCallback = vk_debug_callback;
        vkCreateDebugUtilsMessengerEXT(vkc.instance, &mi, NULL, &vkc.messenger);
        vk_log("validation layer on");
    }
    return true;
}

// The first GPU with Vulkan 1.3 and a queue that draws and presents to the window; a discrete GPU before others.
static bool vk_pick_gpu()
{
    uint32_t n = 0;
    vkEnumeratePhysicalDevices(vkc.instance, &n, NULL);
    VkPhysicalDevice gpus[8];
    if (n > 8)
        n = 8;
    vkEnumeratePhysicalDevices(vkc.instance, &n, gpus);
    int best = -1, best_score = -1;
    uint32_t best_family = 0;
    for (uint32_t i = 0; i < n; i++) {
        VkPhysicalDeviceProperties p;
        vkGetPhysicalDeviceProperties(gpus[i], &p);
        vk_log("GPU %u: %s, Vulkan %u.%u.%u", i, p.deviceName, VK_API_VERSION_MAJOR(p.apiVersion),
               VK_API_VERSION_MINOR(p.apiVersion), VK_API_VERSION_PATCH(p.apiVersion));
        if (p.apiVersion < VK_API_VERSION_1_3)
            continue;
        uint32_t nf = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &nf, NULL);
        VkQueueFamilyProperties families[16];
        if (nf > 16)
            nf = 16;
        vkGetPhysicalDeviceQueueFamilyProperties(gpus[i], &nf, families);
        for (uint32_t f = 0; f < nf; f++) {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpus[i], f, vkc.surface, &present);
            if (!(families[f].queueFlags & VK_QUEUE_GRAPHICS_BIT) || !present)
                continue;
            int score = p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 2 : 1;
            if (score > best_score) {
                best = (int)i;
                best_score = score;
                best_family = f;
            }
            break;
        }
    }
    if (best < 0) {
        vk_log("no GPU with Vulkan 1.3 that can draw into the window");
        return false;
    }
    vkc.gpu = gpus[best];
    vkc.queue_family = best_family;
    vkGetPhysicalDeviceProperties(vkc.gpu, &vkc.gpu_props);
    vkGetPhysicalDeviceMemoryProperties(vkc.gpu, &vkc.mem_props);
    vk_log("using %s", vkc.gpu_props.deviceName);
    return true;
}

static bool vk_create_device()
{
    VkPhysicalDeviceVulkan12Features f12 = {};
    f12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    VkPhysicalDeviceVulkan13Features f13 = {};
    f13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    f12.pNext = &f13;
    VkPhysicalDeviceFeatures2 f = {};
    f.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    f.pNext = &f12;
    vkGetPhysicalDeviceFeatures2(vkc.gpu, &f);
    if (!f13.dynamicRendering || !f13.synchronization2 || !f12.runtimeDescriptorArray ||
        !f12.descriptorBindingPartiallyBound || !f12.descriptorBindingSampledImageUpdateAfterBind ||
        !f12.descriptorBindingUpdateUnusedWhilePending) {
        vk_log("the GPU lacks a required feature (dynamic rendering, synchronization2, descriptor indexing)");
        return false;
    }
    VkPhysicalDeviceVulkan12Features e12 = {};
    e12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    e12.runtimeDescriptorArray = VK_TRUE;
    e12.descriptorBindingPartiallyBound = VK_TRUE;
    e12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    e12.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    VkPhysicalDeviceVulkan13Features e13 = {};
    e13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    e13.dynamicRendering = VK_TRUE;
    e13.synchronization2 = VK_TRUE;
    e12.pNext = &e13;
    VkPhysicalDeviceFeatures2 enable = {};
    enable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    enable.pNext = &e12;
    enable.features.wideLines = f.features.wideLines;

    float priority = 1.0f;
    VkDeviceQueueCreateInfo qi = {};
    qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qi.queueFamilyIndex = vkc.queue_family;
    qi.queueCount = 1;
    qi.pQueuePriorities = &priority;
    const char *extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo di = {};
    di.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    di.pNext = &enable;
    di.queueCreateInfoCount = 1;
    di.pQueueCreateInfos = &qi;
    di.enabledExtensionCount = 1;
    di.ppEnabledExtensionNames = extensions;
    VkResult r = vkCreateDevice(vkc.gpu, &di, NULL, &vkc.device);
    if (r != VK_SUCCESS) {
        vk_log("vkCreateDevice failed (%d)", (int)r);
        return false;
    }
#define VK_LOAD_DEVICE(name) name = (PFN_##name)vkGetDeviceProcAddr(vkc.device, #name);
    VK_DEVICE_FUNCS(VK_LOAD_DEVICE)
#undef VK_LOAD_DEVICE
    vkGetDeviceQueue(vkc.device, vkc.queue_family, 0, &vkc.queue);
    return true;
}

// ---- swapchain ----
static void vk_destroy_swapchain_images()
{
    for (uint32_t i = 0; i < vkc.image_count; i++) {
        if (vkc.views[i])
            vkDestroyImageView(vkc.device, vkc.views[i], NULL);
        vkc.views[i] = VK_NULL_HANDLE;
    }
    if (vkc.depth_view)
        vkDestroyImageView(vkc.device, vkc.depth_view, NULL);
    if (vkc.depth_image)
        vkDestroyImage(vkc.device, vkc.depth_image, NULL);
    if (vkc.depth_memory)
        vkFreeMemory(vkc.device, vkc.depth_memory, NULL);
    vkc.depth_view = VK_NULL_HANDLE;
    vkc.depth_image = VK_NULL_HANDLE;
    vkc.depth_memory = VK_NULL_HANDLE;
}

static VkImageView vk_make_view(VkImage image, VkFormat format, VkImageAspectFlags aspect)
{
    VkImageViewCreateInfo vi = {};
    vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image = image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = format;
    vi.subresourceRange.aspectMask = aspect;
    vi.subresourceRange.levelCount = 1;
    vi.subresourceRange.layerCount = 1;
    VkImageView view = VK_NULL_HANDLE;
    vkCreateImageView(vkc.device, &vi, NULL, &view);
    return view;
}

static VkPresentModeKHR vk_choose_present_mode()
{
    char want[32];
    modern_ini_str("Display", "VulkanPresent", "Fifo", want, sizeof want);
    VkPresentModeKHR wanted = !_stricmp(want, "Immediate") ? VK_PRESENT_MODE_IMMEDIATE_KHR
                            : !_stricmp(want, "Mailbox")   ? VK_PRESENT_MODE_MAILBOX_KHR
                                                           : VK_PRESENT_MODE_FIFO_KHR;
    uint32_t n = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkc.gpu, vkc.surface, &n, NULL);
    VkPresentModeKHR modes[16];
    if (n > 16)
        n = 16;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkc.gpu, vkc.surface, &n, modes);
    for (uint32_t i = 0; i < n; i++)
        if (modes[i] == wanted)
            return wanted;
    vk_log("present mode %s is not available; using Fifo (vsync)", want);
    return VK_PRESENT_MODE_FIFO_KHR;   // always available
}

// The swapchain's format (the pipelines are made for it) and present mode, once.
static void vk_choose_surface_format()
{
    uint32_t n = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkc.gpu, vkc.surface, &n, NULL);
    VkSurfaceFormatKHR formats[32];
    if (n > 32)
        n = 32;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkc.gpu, vkc.surface, &n, formats);
    vkc.swap_format = n ? formats[0].format : VK_FORMAT_B8G8R8A8_UNORM;
    // UNORM, as OpenGL's default framebuffer: the game's colours are written as they are, without sRGB encoding.
    for (uint32_t i = 0; i < n; i++)
        if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            vkc.swap_format = VK_FORMAT_B8G8R8A8_UNORM;
    vkc.present_mode = vk_choose_present_mode();
}

// (Re)creates the swapchain for the window's current client size; false while it is minimised.
static bool vk_create_swapchain()
{
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkc.gpu, vkc.surface, &caps);
    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        RECT rc;
        GetClientRect(vkc.hwnd, &rc);
        extent.width = (uint32_t)(rc.right - rc.left);
        extent.height = (uint32_t)(rc.bottom - rc.top);
    }
    if (extent.width == 0 || extent.height == 0)
        return false;

    VkSwapchainKHR old = vkc.swapchain;
    VkSwapchainCreateInfoKHR si = {};
    si.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    si.surface = vkc.surface;
    si.minImageCount = caps.minImageCount + 1;
    if (caps.maxImageCount && si.minImageCount > caps.maxImageCount)
        si.minImageCount = caps.maxImageCount;
    if (si.minImageCount > 8)
        si.minImageCount = 8;
    si.imageFormat = vkc.swap_format;
    si.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    si.imageExtent = extent;
    si.imageArrayLayers = 1;
    // Transfer source: read-backs (save thumbnails, screenshots, agent captures).
    si.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    si.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    si.preTransform = caps.currentTransform;
    si.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    si.presentMode = vkc.present_mode;
    si.clipped = VK_TRUE;
    si.oldSwapchain = old;
    VkResult r = vkCreateSwapchainKHR(vkc.device, &si, NULL, &vkc.swapchain);
    if (old) {
        vk_destroy_swapchain_images();
        vkDestroySwapchainKHR(vkc.device, old, NULL);
    }
    if (r != VK_SUCCESS) {
        vk_log("vkCreateSwapchainKHR failed (%d)", (int)r);
        vkc.swapchain = VK_NULL_HANDLE;
        return false;
    }
    vkc.extent = extent;
    vkc.image_count = 0;
    vkGetSwapchainImagesKHR(vkc.device, vkc.swapchain, &vkc.image_count, NULL);
    if (vkc.image_count > 8)
        vkc.image_count = 8;
    vkGetSwapchainImagesKHR(vkc.device, vkc.swapchain, &vkc.image_count, vkc.images);
    for (uint32_t i = 0; i < vkc.image_count; i++) {
        vkc.views[i] = vk_make_view(vkc.images[i], vkc.swap_format, VK_IMAGE_ASPECT_COLOR_BIT);
        if (!vk_render_done[i]) {
            VkSemaphoreCreateInfo sci = {};
            sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            vkCreateSemaphore(vkc.device, &sci, NULL, &vk_render_done[i]);
        }
    }

    // The depth buffer, at the window's size like the colour (OpenGL's default framebuffer has both).
    VkImageCreateInfo ii = {};
    ii.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = vkc.depth_format;
    ii.extent.width = extent.width;
    ii.extent.height = extent.height;
    ii.extent.depth = 1;
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    vkCreateImage(vkc.device, &ii, NULL, &vkc.depth_image);
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(vkc.device, vkc.depth_image, &req);
    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = vk_memory_type(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(vkc.device, &ai, NULL, &vkc.depth_memory);
    vkBindImageMemory(vkc.device, vkc.depth_image, vkc.depth_memory, 0);
    vkc.depth_view = vk_make_view(vkc.depth_image, vkc.depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
    vk_log("swapchain %ux%u, %u images, present mode %d", extent.width, extent.height, vkc.image_count,
           (int)vkc.present_mode);
    return true;
}

static bool vk_create_frames()
{
    for (int i = 0; i < VK_FRAMES; i++) {
        vk_frame *f = &vk_frames[i];
        VkCommandPoolCreateInfo pi = {};
        pi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pi.queueFamilyIndex = vkc.queue_family;
        if (vkCreateCommandPool(vkc.device, &pi, NULL, &f->pool) != VK_SUCCESS)
            return false;
        VkCommandBufferAllocateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ci.commandPool = f->pool;
        ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ci.commandBufferCount = 1;
        vkAllocateCommandBuffers(vkc.device, &ci, &f->cmd);
        VkSemaphoreCreateInfo si = {};
        si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(vkc.device, &si, NULL, &f->acquired);
        VkFenceCreateInfo fi = {};
        fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(vkc.device, &fi, NULL, &f->done);
        if (!vk_create_buffer(VK_VERTEX_RING, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true, &f->vertices) ||
            !vk_create_buffer(VK_INDEX_RING, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, true, &f->indices))
            return false;
    }
    return true;
}

static VkFormat vk_choose_depth_format()
{
    // 24-bit depth without stencil (OpenGL's usual default framebuffer depth) where it exists, else 32-bit float,
    // which every GPU supports.
    const VkFormat candidates[] = { VK_FORMAT_X8_D24_UNORM_PACK32, VK_FORMAT_D32_SFLOAT };
    for (int i = 0; i < 2; i++) {
        VkFormatProperties p;
        vkGetPhysicalDeviceFormatProperties(vkc.gpu, candidates[i], &p);
        if (p.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return candidates[i];
    }
    return VK_FORMAT_D32_SFLOAT;
}

static void vk_shutdown()
{
    if (vkc.device) {
        vkDeviceWaitIdle(vkc.device);
        // A frame being recorded is abandoned: its command buffer is reset before the textures it names go.
        for (int i = 0; i < VK_FRAMES; i++)
            if (vk_frames[i].pool)
                vkResetCommandPool(vkc.device, vk_frames[i].pool, 0);
        vk_tcache_close_all();
        vk_draw_close();
        vk_destroy_transfer();
        for (int i = 0; i < VK_FRAMES; i++) {
            vk_frame *f = &vk_frames[i];
            vk_destroy_buffer(&f->vertices);
            vk_destroy_buffer(&f->indices);
            if (f->done)
                vkDestroyFence(vkc.device, f->done, NULL);
            if (f->acquired)
                vkDestroySemaphore(vkc.device, f->acquired, NULL);
            if (f->pool)
                vkDestroyCommandPool(vkc.device, f->pool, NULL);
        }
        memset(vk_frames, 0, sizeof vk_frames);
        for (int i = 0; i < 8; i++)
            if (vk_render_done[i])
                vkDestroySemaphore(vkc.device, vk_render_done[i], NULL);
        memset(vk_render_done, 0, sizeof vk_render_done);
        vk_destroy_swapchain_images();
        if (vkc.swapchain)
            vkDestroySwapchainKHR(vkc.device, vkc.swapchain, NULL);
        vkDestroyDevice(vkc.device, NULL);
    }
    if (vkc.instance) {
        if (vkc.surface)
            vkDestroySurfaceKHR(vkc.instance, vkc.surface, NULL);
        if (vkc.messenger)
            vkDestroyDebugUtilsMessengerEXT(vkc.instance, vkc.messenger, NULL);
        vkDestroyInstance(vkc.instance, NULL);
    }
    memset(&vkc, 0, sizeof vkc);
    vk_context_initialized = false;
    vk_recording = false;
    vk_rendering = false;
}

bool vk_init(HWND hwnd)
{
    if (!vk_load_loader() || !vk_create_instance())
        return false;
    vkc.hwnd = hwnd;
    VkWin32SurfaceCreateInfoKHR si = {};
    si.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    si.hinstance = GetModuleHandleA(NULL);
    si.hwnd = hwnd;
    if (vkCreateWin32SurfaceKHR(vkc.instance, &si, NULL, &vkc.surface) != VK_SUCCESS) {
        vk_log("vkCreateWin32SurfaceKHR failed");
        vk_shutdown();
        return false;
    }
    if (!vk_pick_gpu() || !vk_create_device()) {
        vk_shutdown();
        return false;
    }
    vkc.depth_format = vk_choose_depth_format();
    vk_choose_surface_format();
    if (!vk_create_frames() || !vk_draw_init() || !vk_tcache_init()) {
        vk_log("setting up the frames, textures or pipelines failed");
        vk_shutdown();
        return false;
    }
    vk_create_swapchain();   // may fail while the window is minimised; vk_begin_frame retries
    return true;
}

// ---- the frame ----
void vk_image_barrier(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect, VkImageLayout from,
                             VkImageLayout to, VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access,
                             VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access)
{
    VkImageMemoryBarrier2 b = {};
    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    b.srcStageMask = src_stage;
    b.srcAccessMask = src_access;
    b.dstStageMask = dst_stage;
    b.dstAccessMask = dst_access;
    b.oldLayout = from;
    b.newLayout = to;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = image;
    b.subresourceRange.aspectMask = aspect;
    b.subresourceRange.levelCount = 1;
    b.subresourceRange.layerCount = 1;
    VkDependencyInfo di = {};
    di.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    di.imageMemoryBarrierCount = 1;
    di.pImageMemoryBarriers = &b;
    vkCmdPipelineBarrier2(cmd, &di);
}

void vk_resume_rendering()
{
    if (!vk_recording || vk_rendering)
        return;
    vk_frame *f = &vk_frames[vk_frame_index];
    VkRenderingAttachmentInfo color = {};
    color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color.imageView = vkc.views[vk_image_index];
    color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.loadOp = vk_frame_cleared ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.clearValue.color.float32[3] = 1.0f;
    VkRenderingAttachmentInfo depth = {};
    depth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depth.imageView = vkc.depth_view;
    depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth.loadOp = vk_frame_cleared ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth.clearValue.depthStencil.depth = 1.0f;
    VkRenderingInfo ri = {};
    ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    ri.renderArea.extent = vkc.extent;
    ri.layerCount = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &color;
    ri.pDepthAttachment = &depth;
    vkCmdBeginRendering(f->cmd, &ri);
    vk_frame_cleared = true;
    vk_rendering = true;
    vk_draw_frame_start();
}

void vk_suspend_rendering()
{
    if (!vk_rendering)
        return;
    vkCmdEndRendering(vk_frames[vk_frame_index].cmd);
    vk_rendering = false;
}

static void vk_begin_cmd(vk_frame *f)
{
    VkCommandBufferBeginInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(f->cmd, &bi);
}

void vk_begin_frame()
{
    if (vk_recording || !vkc.device)
        return;
    vk_skip_frame = true;
    RECT rc;
    GetClientRect(vkc.hwnd, &rc);
    if (rc.right - rc.left <= 0 || rc.bottom - rc.top <= 0)
        return;   // minimised
    if (!vkc.swapchain || (uint32_t)(rc.right - rc.left) != vkc.extent.width ||
        (uint32_t)(rc.bottom - rc.top) != vkc.extent.height) {
        vkDeviceWaitIdle(vkc.device);
        if (!vk_create_swapchain())
            return;
    }
    vk_frame *f = &vk_frames[vk_frame_index];
    vkWaitForFences(vkc.device, 1, &f->done, VK_TRUE, UINT64_MAX);
    vk_tcache_frame_done(vk_frame_index);
    VkResult r = vkAcquireNextImageKHR(vkc.device, vkc.swapchain, UINT64_MAX, f->acquired, VK_NULL_HANDLE,
                                       &vk_image_index);
    if (r == VK_ERROR_OUT_OF_DATE_KHR) {
        vkDeviceWaitIdle(vkc.device);
        if (!vk_create_swapchain())
            return;
        r = vkAcquireNextImageKHR(vkc.device, vkc.swapchain, UINT64_MAX, f->acquired, VK_NULL_HANDLE,
                                  &vk_image_index);
    }
    if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
        vk_log("vkAcquireNextImageKHR failed (%d)", (int)r);
        return;
    }
    vkResetFences(vkc.device, 1, &f->done);
    vkResetCommandPool(vkc.device, f->pool, 0);
    f->vertex_used = 0;
    f->index_used = 0;
    vk_begin_cmd(f);
    vk_image_barrier(f->cmd, vkc.images[vk_image_index], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    vk_image_barrier(f->cmd, vkc.depth_image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
    vk_skip_frame = false;
    vk_recording = true;
    vk_frame_cleared = false;
    vk_acquire_waited = false;
    vk_resume_rendering();
}

VkCommandBuffer vk_cmd()
{
    if (!vk_recording)
        vk_begin_frame();
    if (!vk_recording)
        return VK_NULL_HANDLE;   // nothing is drawn this frame (minimised)
    vk_resume_rendering();
    return vk_frames[vk_frame_index].cmd;
}

// Submits what is recorded. last: the frame's end (signals the image's render_done for the present).
static void vk_submit(bool last)
{
    vk_frame *f = &vk_frames[vk_frame_index];
    vkEndCommandBuffer(f->cmd);
    VkCommandBufferSubmitInfo cb = {};
    cb.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cb.commandBuffer = f->cmd;
    VkSemaphoreSubmitInfo wait = {};
    wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    wait.semaphore = f->acquired;
    wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSemaphoreSubmitInfo signal = {};
    signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signal.semaphore = vk_render_done[vk_image_index];
    signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkSubmitInfo2 si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    si.waitSemaphoreInfoCount = vk_acquire_waited ? 0 : 1;
    si.pWaitSemaphoreInfos = &wait;
    si.commandBufferInfoCount = 1;
    si.pCommandBufferInfos = &cb;
    si.signalSemaphoreInfoCount = last ? 1 : 0;
    si.pSignalSemaphoreInfos = &signal;
    vkQueueSubmit2(vkc.queue, 1, &si, f->done);
    vk_acquire_waited = true;
}

// Inside a frame: submits what is recorded, waits for it, and starts recording again (read-backs, and a texture
// upload that does not fit the staging memory). The draws so far stay in the image.
void vk_wait_idle()
{
    if (!vk_recording) {
        if (vkc.device)
            vkDeviceWaitIdle(vkc.device);
        return;
    }
    vk_suspend_rendering();
    vk_frame *f = &vk_frames[vk_frame_index];
    vk_submit(false);
    vkWaitForFences(vkc.device, 1, &f->done, VK_TRUE, UINT64_MAX);
    vk_tcache_frame_done(-1);   // everything submitted has finished
    vkResetFences(vkc.device, 1, &f->done);
    vkResetCommandPool(vkc.device, f->pool, 0);
    f->vertex_used = 0;
    f->index_used = 0;
    vk_begin_cmd(f);
}

// ---- transfers outside the rendering pass ----
static VkCommandPool vk_transfer_pool;
static VkCommandBuffer vk_transfer_cmd;
static VkFence vk_transfer_fence;
static bool vk_transfer_own;                   // a one-time command buffer (no frame is being recorded)

VkCommandBuffer vk_begin_transfer()
{
    if (!vk_recording && !vk_skip_frame)
        vk_begin_frame();   // a frame would start with the next draw anyway
    if (vk_recording) {
        vk_suspend_rendering();
        vk_transfer_own = false;
        return vk_frames[vk_frame_index].cmd;
    }
    if (!vk_transfer_pool) {
        VkCommandPoolCreateInfo pi = {};
        pi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pi.queueFamilyIndex = vkc.queue_family;
        vkCreateCommandPool(vkc.device, &pi, NULL, &vk_transfer_pool);
        VkCommandBufferAllocateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ci.commandPool = vk_transfer_pool;
        ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ci.commandBufferCount = 1;
        vkAllocateCommandBuffers(vkc.device, &ci, &vk_transfer_cmd);
        VkFenceCreateInfo fi = {};
        fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vkCreateFence(vkc.device, &fi, NULL, &vk_transfer_fence);
    }
    vkResetCommandPool(vkc.device, vk_transfer_pool, 0);
    VkCommandBufferBeginInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(vk_transfer_cmd, &bi);
    vk_transfer_own = true;
    return vk_transfer_cmd;
}

void vk_end_transfer()
{
    if (!vk_transfer_own)
        return;   // the frame's command buffer: the next draw resumes the rendering pass
    vkEndCommandBuffer(vk_transfer_cmd);
    VkCommandBufferSubmitInfo cb = {};
    cb.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cb.commandBuffer = vk_transfer_cmd;
    VkSubmitInfo2 si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    si.commandBufferInfoCount = 1;
    si.pCommandBufferInfos = &cb;
    vkQueueSubmit2(vkc.queue, 1, &si, vk_transfer_fence);
    vkWaitForFences(vkc.device, 1, &vk_transfer_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vkc.device, 1, &vk_transfer_fence);
    vk_transfer_own = false;
}

static void vk_destroy_transfer()
{
    if (vk_transfer_fence)
        vkDestroyFence(vkc.device, vk_transfer_fence, NULL);
    if (vk_transfer_pool)
        vkDestroyCommandPool(vkc.device, vk_transfer_pool, NULL);
    vk_transfer_fence = VK_NULL_HANDLE;
    vk_transfer_pool = VK_NULL_HANDLE;
    vk_transfer_cmd = VK_NULL_HANDLE;
}

void vk_read_window(int x, int y, int w, int h, ubyte *bgra)
{
    memset(bgra, 0, (size_t)w * h * 4);
    if (!vk_recording || w <= 0 || h <= 0)
        return;
    // Inside the image only; the rest stays black.
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > (int)vkc.extent.width ? (int)vkc.extent.width : x + w;
    int y1 = y + h > (int)vkc.extent.height ? (int)vkc.extent.height : y + h;
    if (x1 <= x0 || y1 <= y0)
        return;
    vk_buffer buf;
    if (!vk_create_buffer((VkDeviceSize)(x1 - x0) * (y1 - y0) * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, &buf))
        return;
    vk_suspend_rendering();
    VkCommandBuffer cmd = vk_frames[vk_frame_index].cmd;
    VkImage image = vkc.images[vk_image_index];
    vk_image_barrier(cmd, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageOffset.x = x0;
    region.imageOffset.y = y0;
    region.imageExtent.width = (uint32_t)(x1 - x0);
    region.imageExtent.height = (uint32_t)(y1 - y0);
    region.imageExtent.depth = 1;
    vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf.buffer, 1, &region);
    vk_image_barrier(cmd, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    vk_wait_idle();
    // The swapchain's B, G, R, A (VK_FORMAT_B8G8R8A8_UNORM); an R, G, B, A swapchain is swapped to that order.
    bool rgba = vkc.swap_format == VK_FORMAT_R8G8B8A8_UNORM;
    const ubyte *src = (const ubyte *)buf.mapped;
    for (int row = y0; row < y1; row++)
        for (int col = x0; col < x1; col++) {
            const ubyte *s = src + ((size_t)(row - y0) * (x1 - x0) + (col - x0)) * 4;
            ubyte *d = bgra + ((size_t)(row - y) * w + (col - x)) * 4;
            d[0] = rgba ? s[2] : s[0];
            d[1] = s[1];
            d[2] = rgba ? s[0] : s[2];
            d[3] = s[3];
        }
    vk_destroy_buffer(&buf);
}

void vk_end_frame()
{
    if (!vk_recording)
        return;
    if (modern_capture_wanted()) {
        // Agent mode's capture (modern/display.cpp) of the finished picture.
        int w = (int)vkc.extent.width, h = (int)vkc.extent.height;
        ubyte *pixels = new ubyte[(size_t)w * h * 4];
        vk_read_window(0, 0, w, h, pixels);
        modern_capture_bgra(pixels, w, h);
        delete[] pixels;
    }
    vk_suspend_rendering();
    vk_frame *f = &vk_frames[vk_frame_index];
    vk_image_barrier(f->cmd, vkc.images[vk_image_index], VK_IMAGE_ASPECT_COLOR_BIT,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_2_NONE, 0);
    vk_submit(true);
    VkPresentInfoKHR pi = {};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &vk_render_done[vk_image_index];
    pi.swapchainCount = 1;
    pi.pSwapchains = &vkc.swapchain;
    pi.pImageIndices = &vk_image_index;
    VkResult r = vkQueuePresentKHR(vkc.queue, &pi);
    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR)
        vkc.extent.width = 0;   // recreated at the next frame
    vk_recording = false;
    vk_frame_index = (vk_frame_index + 1) % VK_FRAMES;
}

void vk_game_to_window(float *scale, float *off_x, float *off_y)
{
    int cw, ch, ox, oy;
    double s;
    bool menu_shift;
    if (modern_display_map(&cw, &ch, &s, &ox, &oy, &menu_shift)) {
        *scale = (float)s;
        *off_x = (float)ox;
        *off_y = (float)oy;
    } else {
        *scale = 1.0f;
        *off_x = 0.0f;
        *off_y = 0.0f;
    }
}

// ---- the entry points of the frame ----
// gr_opengl_init_context: the mode reset, both stages unbound after the first time, the clip rectangle's projection,
// and the first time the texture cache. The surface stays: the game keeps its window.
void vk_init_context(HWND hwnd)
{
    vk_gr_opengl_get_current_mode()->invalidate();
    if (vk_context_initialized) {
        vk_gr_opengl_tcache_set(-1, NULL, NULL, 0);
        vk_gr_opengl_tcache_set(-1, NULL, NULL, 1);
    }
    vk_gr_opengl_set_viewport();
    if (!vk_context_initialized) {
        vk_context_initialized = true;
        vk_gr_opengl_texture_init();
    }
}

void vk_gr_init(HWND hwnd)
{
    vk_init_context(hwnd);
}

void vk_gr_opengl_close()
{
    vk_shutdown();
}

void vk_gr_opengl_flip()
{
    vk_end_frame();
    vk_gr_opengl_tcache_frame();
}

#else
bool vk_init(HWND hwnd)
{
    modern_log("vulkan: this exe was built without the Vulkan SDK (VULKAN_SDK); using OpenGL");
    return false;
}
#endif
#endif
