// vk_internal.h - shared by the Vulkan back end's units (modern/vk_*.cpp; rung L4, docs/future/vulkan-renderer.md).
// Not original source. Only compiled with the Vulkan SDK's headers (MODERN_VULKAN).
//
// The units:
//   vk_device.cpp  loader, instance, device, swapchain, the frame (acquire, record, submit, present), read-back
//   vk_draw.cpp    the OpenGL back end's state (gr::Mode), pipelines, and the drawing entry points
//   vk_tcache.cpp  the texture cache: bitmaps to images, as gr_opengl_create_texture converts them
//   vk_scene.cpp   the level and character mesh draw lists (levelrender_draw_list_opengl, gr_opengl_render_
//                  character_mesh) in Vulkan
// The game draws in its own pixels (gr_screen.max_w x max_h); the frame maps them into the window the way
// modern/display.cpp maps the OpenGL viewport (modern_display_map), so the output is the same.
#ifndef MODERN_VK_INTERNAL_H
#define MODERN_VK_INTERNAL_H

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include "vsdk/vsdk.h"
#include "vsdk/gr/gr.h"
#include "modern/modern.h"
#include "modern/vk.h"

// ---- function pointers (vk_device.cpp loads them) ----
#define VK_INSTANCE_FUNCS(X) \
    X(vkDestroyInstance) X(vkEnumeratePhysicalDevices) X(vkGetPhysicalDeviceProperties) \
    X(vkGetPhysicalDeviceFeatures2) X(vkGetPhysicalDeviceQueueFamilyProperties) \
    X(vkGetPhysicalDeviceMemoryProperties) X(vkGetPhysicalDeviceFormatProperties) X(vkCreateDevice) \
    X(vkGetDeviceProcAddr) X(vkEnumerateDeviceExtensionProperties) X(vkCreateWin32SurfaceKHR) \
    X(vkDestroySurfaceKHR) X(vkGetPhysicalDeviceSurfaceSupportKHR) X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    X(vkGetPhysicalDeviceSurfaceFormatsKHR) X(vkGetPhysicalDeviceSurfacePresentModesKHR) \
    X(vkCreateDebugUtilsMessengerEXT) X(vkDestroyDebugUtilsMessengerEXT)
#define VK_DEVICE_FUNCS(X) \
    X(vkDestroyDevice) X(vkGetDeviceQueue) X(vkDeviceWaitIdle) X(vkQueueSubmit2) X(vkQueueWaitIdle) \
    X(vkCreateSwapchainKHR) X(vkDestroySwapchainKHR) X(vkGetSwapchainImagesKHR) X(vkAcquireNextImageKHR) \
    X(vkQueuePresentKHR) X(vkCreateImageView) X(vkDestroyImageView) X(vkCreateImage) X(vkDestroyImage) \
    X(vkGetImageMemoryRequirements) X(vkBindImageMemory) X(vkCreateBuffer) X(vkDestroyBuffer) \
    X(vkGetBufferMemoryRequirements) X(vkBindBufferMemory) X(vkAllocateMemory) X(vkFreeMemory) X(vkMapMemory) \
    X(vkUnmapMemory) X(vkCreateCommandPool) X(vkDestroyCommandPool) X(vkAllocateCommandBuffers) \
    X(vkResetCommandPool) X(vkBeginCommandBuffer) X(vkEndCommandBuffer) X(vkCreateFence) X(vkDestroyFence) \
    X(vkWaitForFences) X(vkResetFences) X(vkCreateSemaphore) X(vkDestroySemaphore) X(vkCmdPipelineBarrier2) \
    X(vkCmdBeginRendering) X(vkCmdEndRendering) X(vkCmdCopyBufferToImage) X(vkCmdCopyImageToBuffer) X(vkCmdCopyImage) \
    X(vkCmdClearAttachments) X(vkCreateShaderModule) X(vkDestroyShaderModule) X(vkCreatePipelineLayout) \
    X(vkDestroyPipelineLayout) X(vkCreateGraphicsPipelines) X(vkDestroyPipeline) X(vkCreateDescriptorSetLayout) \
    X(vkDestroyDescriptorSetLayout) X(vkCreateDescriptorPool) X(vkDestroyDescriptorPool) \
    X(vkAllocateDescriptorSets) X(vkUpdateDescriptorSets) X(vkCreateSampler) X(vkDestroySampler) \
    X(vkCmdBindPipeline) X(vkCmdBindDescriptorSets) X(vkCmdBindVertexBuffers) X(vkCmdBindIndexBuffer) \
    X(vkCmdPushConstants) X(vkCmdSetViewport) X(vkCmdSetScissor) X(vkCmdSetDepthTestEnable) \
    X(vkCmdSetDepthWriteEnable) X(vkCmdSetDepthCompareOp) X(vkCmdSetPrimitiveTopology) X(vkCmdSetCullMode) \
    X(vkCmdDraw) X(vkCmdDrawIndexed)
#define VK_DECLARE_FUNC(name) extern PFN_##name name;
VK_INSTANCE_FUNCS(VK_DECLARE_FUNC)
VK_DEVICE_FUNCS(VK_DECLARE_FUNC)
#undef VK_DECLARE_FUNC

// ---- vk_device.cpp ----
#define VK_FRAMES 2                            // frames in flight

struct vk_buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void *mapped;                              // host-visible buffers stay mapped
    VkDeviceSize size;
};

struct vk_context {
    VkInstance instance;
    VkPhysicalDevice gpu;
    VkPhysicalDeviceProperties gpu_props;
    VkPhysicalDeviceMemoryProperties mem_props;
    VkDevice device;
    uint32_t queue_family;
    VkQueue queue;
    VkSurfaceKHR surface;
    HWND hwnd;
    VkSwapchainKHR swapchain;
    VkFormat swap_format;
    VkExtent2D extent;                         // the window's client size
    uint32_t image_count;
    VkImage images[8];
    VkImageView views[8];
    VkFormat depth_format;
    VkImage depth_image;
    VkDeviceMemory depth_memory;
    VkImageView depth_view;
    VkDebugUtilsMessengerEXT messenger;
    VkPresentModeKHR present_mode;
    int validation_errors;                     // counted by the debug messenger
};
extern vk_context vkc;

// One frame in flight: its command buffer, the semaphores and fence of its submission, and the ring of host-visible
// vertex / index memory its draws use.
struct vk_frame {
    VkCommandPool pool;
    VkCommandBuffer cmd;
    VkSemaphore acquired;                      // the swapchain image is ready to be drawn into
    VkFence done;                              // this frame's submission finished on the GPU
    vk_buffer vertices;
    VkDeviceSize vertex_used;
    vk_buffer indices;
    VkDeviceSize index_used;
};
extern vk_frame vk_frames[VK_FRAMES];
extern int vk_frame_index;                     // the frame being recorded
extern uint32_t vk_image_index;                // its swapchain image
extern bool vk_recording;                      // between vk_begin_frame and the flip
extern VkSemaphore vk_render_done[8];          // per swapchain image: drawing finished, present may start

bool vk_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, bool host_visible, vk_buffer *out);
void vk_destroy_buffer(vk_buffer *b);
uint32_t vk_memory_type(uint32_t type_bits, VkMemoryPropertyFlags flags);
VkCommandBuffer vk_cmd();                      // the current frame's command buffer, inside rendering
void vk_begin_frame();                         // acquire and start recording (the first draw of a frame)
void vk_end_frame();                           // gr_opengl_flip: submit and present
void vk_suspend_rendering();                   // ends the rendering pass (for transfers inside a frame)
void vk_resume_rendering();                    // starts it again, keeping the contents
void vk_wait_idle();                           // glFinish's place: the GPU has finished everything submitted
// Reads a rectangle of the frame being drawn (window pixels, top row first) as B, G, R, A bytes; black when no frame
// is being drawn. Suspends the rendering pass and waits for the GPU.
void vk_read_window(int x, int y, int w, int h, ubyte *bgra);
// Records commands outside the rendering pass (texture uploads): the frame's command buffer when a frame is being
// recorded, else a one-time command buffer that vk_end_transfer submits and waits for.
VkCommandBuffer vk_begin_transfer();
void vk_image_barrier(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect, VkImageLayout from,
                      VkImageLayout to, VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access,
                      VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access);
void vk_end_transfer();
// The game's pixels in the window: scale and offset, as modern/display.cpp maps the OpenGL viewport.
void vk_game_to_window(float *scale, float *off_x, float *off_y);

// ---- vk_draw.cpp ----
// One vertex as the shaders take it (modern/shaders/gr.vert): game pixels, depth as the OpenGL back end passes it
// (z in [-1, 1], glOrtho's near / far), the colour as bytes R, G, B, A, and the two projective texture coordinates.
struct vk_vertex {
    float x, y, z;
    uint32_t color;
    float uv0[4];
    float uv1[4];
};
bool vk_draw_init();                           // pipelines, samplers, descriptor layout; after the device
void vk_draw_close();
void vk_draw_frame_start();                    // binds the frame's state after vk_begin_frame
// Draws with the current gr::Mode state: a triangle fan (the OpenGL back end's GL_POLYGON), triangles with
// indices (the draw lists), or lines (GL_LINE_STRIP).
enum vk_primitive { VK_PRIM_FAN, VK_PRIM_TRIANGLES, VK_PRIM_LINES };
void vk_draw(vk_primitive prim, const vk_vertex *v, int nv, const unsigned short *idx, int nidx);
uint32_t vk_pack_color(const ubyte *rgba);

// ---- vk_tcache.cpp ----
// OpenGL's texture objects as gr_opengl_tcache_set leaves them: what is bound to each stage persists across frames
// (glBindTexture; gr_opengl_bound_texture only caches the early-out), and each texture has its own wrap mode, set by
// gr_opengl_set_mode's glTexParameteri on the texture bound to stage 0 and reset when the texture is made again.
bool vk_tcache_init();
void vk_tcache_close_all();
int vk_tcache_bound(int stage);                // the descriptor slot of the texture bound to the stage, -1 if none
bool vk_tcache_clamps(int stage);              // its wrap mode: GL_CLAMP (true) or GL_REPEAT
void vk_tcache_set_clamp(int stage, bool clamp);
void vk_tcache_frame_done(int frame);          // the frame's GPU work finished: free what it still used
// Descriptor slots for the back end's own images (modern/vk_post.cpp).
int vk_tcache_alloc_slot(VkImageView view);   // -1 when none is free
void vk_tcache_free_slot(int slot);            // only once no frame in flight uses it

// ---- vk_post.cpp ----
// The end of a frame, before the capture and the present: the brightness pass and the fast-mode badge, as
// modern_swap_buffers draws them for OpenGL. Outside the rendering pass.
bool vk_post_init();
void vk_post_close();
void vk_post_frame(VkCommandBuffer cmd);
VkPipelineLayout vk_draw_pipeline_layout();
extern VkDescriptorSetLayout vk_texture_layout;
extern VkDescriptorSet vk_texture_set;
#define VK_MAX_TEXTURES 4096                   // descriptor slots, more than MAX_BITMAPS (2800)

void vk_log(const char *fmt, ...);
#endif
