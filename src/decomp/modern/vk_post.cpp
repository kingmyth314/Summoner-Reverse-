// vk_post.cpp - the Vulkan back end's end-of-frame passes (rung L4, docs/future/vulkan-renderer.md). Not original
// source. 64-bit MODERN_BUILD with the Vulkan SDK only. See modern/vk_internal.h for the units.
//
// What modern_swap_buffers draws over the OpenGL frame (modern/display.cpp, modern/gamma.cpp), for the Vulkan one:
//   brightness  [Display] Brightness's choice (modern_brightness_pass_gamma): the monitor ramp, or this pass: the
//               frame is copied into an image and drawn back through pow(c, 1 / gamma) (modern/shaders/post.frag);
//   the badge   "FAST xN" in the picture's top-left corner while the speed key's fast mode is on in play: the text is
//               drawn by GDI into a small image (as the OpenGL host draws it with wglUseFontBitmaps) and copied over.
// The bars need nothing: the Vulkan frame starts cleared to black.
//
// [Display] RenderScale (vkc.render_scale, not 1): the game draws into the scene image, the window's size times the
// scale, with its depth buffer at that size; brightness is applied there; then the scale pass draws it into the
// swapchain image, and the badge goes over that at window pixels.
//   below 1  AMD FidelityFX Super Resolution 1 (modern/shaders/fsr/, MIT): EASU upscales into an intermediate image
//            of the window's size, RCAS sharpens it into the swapchain image ([Display] FsrSharpness, in stops,
//            0 = the strongest, default 0.2);
//   above 1  a box filter over each window pixel's footprint (modern/shaders/downscale.frag): supersampling.
// The interface is scaled with the rest, so below 1 the menus are softer than at 1.

#if defined(_WIN64) && defined(MODERN_VULKAN)
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "vk_internal.h"
#include "vk_shaders.h"

struct vk_post_image {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    int slot;
    int w, h;
};

static VkPipeline vk_post_pipeline;            // post.frag with the target's depth attachment format
static VkPipeline vk_post_pipeline_window;     // post.frag for a colour image alone (the badge after scaling)
static VkPipeline vk_easu_pipeline, vk_rcas_pipeline, vk_down_pipeline;
static vk_post_image vk_copy;                  // the frame's copy for the brightness pass
static vk_post_image vk_scene;                 // RenderScale: what the game draws into
static vk_post_image vk_mid;                   // RenderScale below 1: EASU's output, RCAS's input
static float vk_fsr_sharpness = 0.2f;
static vk_post_image vk_badge;
static vk_buffer vk_badge_staging;
static char vk_badge_text[32];
static int vk_badge_fh;

struct vk_post_push {
    float origin[2];
    float e;
    uint32_t slot;
    uint32_t mode;
};

// The scale pass's shaders (fsr_easu.frag, fsr_rcas.frag, downscale.frag).
struct vk_scale_push {
    float in_size[2];
    float out_size[2];
    uint32_t slot;
    float sharpness;
};

static void vk_post_destroy_image(vk_post_image *p)
{
    if (p->view)
        vkDestroyImageView(vkc.device, p->view, NULL);
    if (p->image)
        vkDestroyImage(vkc.device, p->image, NULL);
    if (p->memory)
        vkFreeMemory(vkc.device, p->memory, NULL);
    vk_tcache_free_slot(p->slot);
    memset(p, 0, sizeof *p);
    p->slot = -1;
}

static bool vk_post_make_image(vk_post_image *p, int w, int h, VkFormat format,
                               VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
{
    // An image in use by the frame before goes once the queue is idle (a resize or a new badge text: rare).
    if (p->image) {
        vkQueueWaitIdle(vkc.queue);
        vk_post_destroy_image(p);
    }
    VkImageCreateInfo ii = {};
    ii.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = format;
    ii.extent.width = (uint32_t)w;
    ii.extent.height = (uint32_t)h;
    ii.extent.depth = 1;
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = usage;
    if (vkCreateImage(vkc.device, &ii, NULL, &p->image) != VK_SUCCESS)
        return false;
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(vkc.device, p->image, &req);
    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = vk_memory_type(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(vkc.device, &ai, NULL, &p->memory) != VK_SUCCESS) {
        vk_post_destroy_image(p);
        return false;
    }
    vkBindImageMemory(vkc.device, p->image, p->memory, 0);
    VkImageViewCreateInfo vi = {};
    vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image = p->image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = format;
    vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vi.subresourceRange.levelCount = 1;
    vi.subresourceRange.layerCount = 1;
    vkCreateImageView(vkc.device, &vi, NULL, &p->view);
    p->slot = vk_tcache_alloc_slot(p->view);
    p->w = w;
    p->h = h;
    if (p->slot < 0) {
        vk_post_destroy_image(p);
        return false;
    }
    return true;
}

// A pipeline drawing post.vert's triangle with the fragment shader: into the target (with its depth attachment
// format, as the game's rendering pass has it) or into a colour image alone.
static VkPipeline vk_post_make_pipeline(const unsigned int *spv, size_t size, bool with_depth)
{
    VkShaderModuleCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    VkShaderModule vert = VK_NULL_HANDLE, frag = VK_NULL_HANDLE;
    ci.codeSize = sizeof vk_spv_post_vert;
    ci.pCode = (const uint32_t *)vk_spv_post_vert;
    vkCreateShaderModule(vkc.device, &ci, NULL, &vert);
    ci.codeSize = size;
    ci.pCode = (const uint32_t *)spv;
    vkCreateShaderModule(vkc.device, &ci, NULL, &frag);
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName = "main";
    VkPipelineVertexInputStateCreateInfo vi = {};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VkPipelineInputAssemblyStateCreateInfo ia = {};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp = {};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs = {};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo ms = {};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds = {};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    VkPipelineColorBlendAttachmentState att = {};
    att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                         VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cb = {};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &att;
    VkDynamicState dyn[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
                             VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE, VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY };
    VkPipelineDynamicStateCreateInfo dy = {};
    dy.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dy.dynamicStateCount = sizeof dyn / sizeof dyn[0];
    dy.pDynamicStates = dyn;
    VkPipelineRenderingCreateInfo rinfo = {};
    rinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rinfo.colorAttachmentCount = 1;
    rinfo.pColorAttachmentFormats = &vkc.swap_format;
    rinfo.depthAttachmentFormat = with_depth ? vkc.depth_format : VK_FORMAT_UNDEFINED;
    VkGraphicsPipelineCreateInfo pi = {};
    pi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pi.pNext = &rinfo;
    pi.stageCount = 2;
    pi.pStages = stages;
    pi.pVertexInputState = &vi;
    pi.pInputAssemblyState = &ia;
    pi.pViewportState = &vp;
    pi.pRasterizationState = &rs;
    pi.pMultisampleState = &ms;
    pi.pDepthStencilState = &ds;
    pi.pColorBlendState = &cb;
    pi.pDynamicState = &dy;
    pi.layout = vk_draw_pipeline_layout();   // the same set and push range as the game's pipelines
    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vert && frag && vkCreateGraphicsPipelines(vkc.device, VK_NULL_HANDLE, 1, &pi, NULL, &pipeline) != VK_SUCCESS)
        pipeline = VK_NULL_HANDLE;
    if (vert)
        vkDestroyShaderModule(vkc.device, vert, NULL);
    if (frag)
        vkDestroyShaderModule(vkc.device, frag, NULL);
    return pipeline;
}

bool vk_post_init()
{
    vk_copy.slot = vk_badge.slot = vk_scene.slot = vk_mid.slot = -1;
    char s[32];
    modern_ini_str("Display", "FsrSharpness", "0.2", s, sizeof s);
    vk_fsr_sharpness = (float)atof(s);
    vk_fsr_sharpness = vk_fsr_sharpness < 0.0f ? 0.0f : vk_fsr_sharpness > 2.0f ? 2.0f : vk_fsr_sharpness;
    vk_post_pipeline = vk_post_make_pipeline(vk_spv_post_frag, sizeof vk_spv_post_frag, true);
    vk_post_pipeline_window = vk_post_make_pipeline(vk_spv_post_frag, sizeof vk_spv_post_frag, false);
    vk_easu_pipeline = vk_post_make_pipeline(vk_spv_fsr_easu_frag, sizeof vk_spv_fsr_easu_frag, false);
    vk_rcas_pipeline = vk_post_make_pipeline(vk_spv_fsr_rcas_frag, sizeof vk_spv_fsr_rcas_frag, false);
    vk_down_pipeline = vk_post_make_pipeline(vk_spv_downscale_frag, sizeof vk_spv_downscale_frag, false);
    return vk_post_pipeline && vk_post_pipeline_window && vk_easu_pipeline && vk_rcas_pipeline && vk_down_pipeline;
}

bool vk_post_resize()
{
    vk_post_destroy_image(&vk_scene);
    vk_post_destroy_image(&vk_mid);
    vkc.scene_image = VK_NULL_HANDLE;
    vkc.scene_view = VK_NULL_HANDLE;
    vkc.target_extent = vkc.extent;
    if (vkc.render_scale == 1.0f)
        return true;
    uint32_t max_size = vkc.gpu_props.limits.maxImageDimension2D;
    int w = (int)(vkc.extent.width * vkc.render_scale + 0.5f), h = (int)(vkc.extent.height * vkc.render_scale + 0.5f);
    if (w < 1 || h < 1 || (uint32_t)w > max_size || (uint32_t)h > max_size)
        return false;
    const VkImageUsageFlags target = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (!vk_post_make_image(&vk_scene, w, h, vkc.swap_format, target))
        return false;
    if (vkc.render_scale < 1.0f &&
        !vk_post_make_image(&vk_mid, (int)vkc.extent.width, (int)vkc.extent.height, vkc.swap_format,
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)) {
        vk_post_destroy_image(&vk_scene);
        return false;
    }
    vkc.scene_image = vk_scene.image;
    vkc.scene_view = vk_scene.view;
    vkc.target_extent.width = (uint32_t)w;
    vkc.target_extent.height = (uint32_t)h;
    return true;
}

void vk_post_close()
{
    vk_post_destroy_image(&vk_copy);
    vk_post_destroy_image(&vk_badge);
    vk_post_destroy_image(&vk_scene);
    vk_post_destroy_image(&vk_mid);
    vkc.scene_image = VK_NULL_HANDLE;
    vkc.scene_view = VK_NULL_HANDLE;
    vk_destroy_buffer(&vk_badge_staging);
    VkPipeline *pipelines[] = { &vk_post_pipeline, &vk_post_pipeline_window, &vk_easu_pipeline, &vk_rcas_pipeline,
                                &vk_down_pipeline };
    for (VkPipeline *p : pipelines) {
        if (*p)
            vkDestroyPipeline(vkc.device, *p, NULL);
        *p = VK_NULL_HANDLE;
    }
    vk_badge_text[0] = 0;
    vk_badge_fh = 0;
}

// Draws the post pipeline's triangle over [x, y, w, h] of an image of size ext (the target, or the swapchain image
// after the scale pass), reading image slot with origin at (x, y).
static void vk_post_draw(VkCommandBuffer cmd, VkPipeline pipeline, VkExtent2D ext, int x, int y, int w, int h, int slot,
                         uint32_t mode, float e)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    VkViewport vp = { 0.0f, 0.0f, (float)ext.width, (float)ext.height, 0.0f, 1.0f };
    vkCmdSetViewport(cmd, 0, 1, &vp);
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > (int)ext.width ? (int)ext.width : x + w;
    int y1 = y + h > (int)ext.height ? (int)ext.height : y + h;
    if (x1 <= x0 || y1 <= y0)
        return;
    VkRect2D sc = { { x0, y0 }, { (uint32_t)(x1 - x0), (uint32_t)(y1 - y0) } };
    vkCmdSetScissor(cmd, 0, 1, &sc);
    vkCmdSetDepthTestEnable(cmd, VK_FALSE);
    vkCmdSetDepthWriteEnable(cmd, VK_FALSE);
    vkCmdSetPrimitiveTopology(cmd, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vk_post_push pc = { { (float)x, (float)y }, e, (uint32_t)slot, mode };
    vkCmdPushConstants(cmd, vk_draw_pipeline_layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof pc, &pc);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}

static void vk_post_brightness(VkCommandBuffer cmd, float gamma)
{
    int w = (int)vkc.target_extent.width, h = (int)vkc.target_extent.height;
    if ((vk_copy.w != w || vk_copy.h != h) && !vk_post_make_image(&vk_copy, w, h, vkc.swap_format))
        return;
    vk_suspend_rendering();
    VkImage swap = vk_target_image();
    vk_image_barrier(cmd, swap, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
    vk_image_barrier(cmd, vk_copy.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, 0,
                     VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    VkImageCopy region = {};
    region.srcSubresource.aspectMask = region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.layerCount = region.dstSubresource.layerCount = 1;
    region.extent.width = (uint32_t)w;
    region.extent.height = (uint32_t)h;
    region.extent.depth = 1;
    vkCmdCopyImage(cmd, swap, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vk_copy.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &region);
    vk_image_barrier(cmd, vk_copy.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT,
                     VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                     VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    vk_image_barrier(cmd, swap, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
    vk_resume_rendering();
    vk_post_draw(cmd, vk_post_pipeline, vkc.target_extent, 0, 0, w, h, vk_copy.slot, 0, 1.0f / gamma);
}

// The badge's image: a black box with the text in gold, drawn by GDI (as modern_speed_badge's glCallLists text).
static bool vk_post_badge_image(VkCommandBuffer cmd, const char *text, int fh, int bw, int bh, int pad)
{
    if (!strcmp(text, vk_badge_text) && fh == vk_badge_fh && vk_badge.image)
        return true;
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = bw;
    bi.bmiHeader.biHeight = -bh;   // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void *bits = NULL;
    HDC dc = CreateCompatibleDC(NULL);
    HBITMAP bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!dc || !bmp || !bits) {
        if (bmp)
            DeleteObject(bmp);
        if (dc)
            DeleteDC(dc);
        return false;
    }
    HGDIOBJ old_bmp = SelectObject(dc, bmp);
    memset(bits, 0, (size_t)bw * bh * 4);
    HFONT font = CreateFontA(-fh, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY, DEFAULT_PITCH, "Arial");
    HGDIOBJ old_font = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 209, 64));   // gold (1.0, 0.82, 0.25), like the game's highlight text
    TextOutA(dc, pad, pad, text, (int)strlen(text));
    GdiFlush();
    bool ok = vk_post_make_image(&vk_badge, bw, bh, VK_FORMAT_B8G8R8A8_UNORM);
    if (ok && (!vk_badge_staging.buffer || vk_badge_staging.size < (VkDeviceSize)bw * bh * 4)) {
        vk_destroy_buffer(&vk_badge_staging);
        ok = vk_create_buffer((VkDeviceSize)bw * bh * 4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, &vk_badge_staging);
    }
    if (ok) {
        memcpy(vk_badge_staging.mapped, bits, (size_t)bw * bh * 4);
        vk_image_barrier(cmd, vk_badge.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_NONE, 0, VK_PIPELINE_STAGE_2_COPY_BIT,
                         VK_ACCESS_2_TRANSFER_WRITE_BIT);
        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = (uint32_t)bw;
        region.imageExtent.height = (uint32_t)bh;
        region.imageExtent.depth = 1;
        vkCmdCopyBufferToImage(cmd, vk_badge_staging.buffer, vk_badge.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);
        vk_image_barrier(cmd, vk_badge.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT,
                         VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                         VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
        lstrcpynA(vk_badge_text, text, sizeof vk_badge_text);
        vk_badge_fh = fh;
    }
    SelectObject(dc, old_font);
    DeleteObject(font);
    SelectObject(dc, old_bmp);
    DeleteObject(bmp);
    DeleteDC(dc);
    return ok;
}

// The badge in window pixels. At a render scale of 1 it is drawn into the target inside the game's rendering pass;
// otherwise its image is made before the scale pass (prepare) and drawn into the swapchain image inside the scale
// pass's rendering (draw).
enum vk_badge_step { VK_BADGE_BOTH, VK_BADGE_PREPARE, VK_BADGE_DRAW };

static void vk_post_badge(VkCommandBuffer cmd, float factor, vk_badge_step step)
{
    int cw, ch, off_x, off_y;
    double scale;
    bool menu_shift;
    if (!modern_display_map(&cw, &ch, &scale, &off_x, &off_y, &menu_shift))
        return;
    char text[32];
    _snprintf(text, sizeof text, "FAST x%g", (double)factor);
    text[sizeof text - 1] = 0;
    // modern_speed_badge's box: 18 game pixels high text, 6 of padding, 12 from the picture's top-left corner.
    int fh = (int)(18 * scale + 0.5);
    int pad = (int)(6 * scale);
    int bw = (int)(strlen(text) * fh * 0.62f + 2 * pad), bh = fh + 2 * pad;
    int x = off_x + (int)(12 * scale), y = off_y + (int)(12 * scale);
    if (step == VK_BADGE_DRAW) {
        if (vk_badge.image && !strcmp(text, vk_badge_text))
            vk_post_draw(cmd, vk_post_pipeline_window, vkc.extent, x, y, vk_badge.w, vk_badge.h, vk_badge.slot, 1,
                         1.0f);
        return;
    }
    vk_suspend_rendering();
    if (!vk_post_badge_image(cmd, text, fh, bw, bh, pad) || step == VK_BADGE_PREPARE)
        return;
    vk_resume_rendering();
    vk_post_draw(cmd, vk_post_pipeline, vkc.target_extent, x, y, vk_badge.w, vk_badge.h, vk_badge.slot, 1, 1.0f);
}

// Begins a rendering pass into a colour image alone (no depth); the pass draws over all of it.
static void vk_post_begin_window_pass(VkCommandBuffer cmd, VkImageView view, VkExtent2D ext)
{
    VkRenderingAttachmentInfo color = {};
    color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color.imageView = view;
    color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkRenderingInfo ri = {};
    ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    ri.renderArea.extent = ext;
    ri.layerCount = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &color;
    vkCmdBeginRendering(cmd, &ri);
}

// Draws one of the scale pass's shaders over all of an image of size ext, reading the image in slot (size in).
static void vk_post_scale_draw(VkCommandBuffer cmd, VkPipeline pipeline, VkExtent2D in, VkExtent2D ext, int slot)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    VkViewport vp = { 0.0f, 0.0f, (float)ext.width, (float)ext.height, 0.0f, 1.0f };
    vkCmdSetViewport(cmd, 0, 1, &vp);
    VkRect2D sc = { { 0, 0 }, ext };
    vkCmdSetScissor(cmd, 0, 1, &sc);
    vkCmdSetDepthTestEnable(cmd, VK_FALSE);
    vkCmdSetDepthWriteEnable(cmd, VK_FALSE);
    vkCmdSetPrimitiveTopology(cmd, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vk_scale_push pc = { { (float)in.width, (float)in.height }, { (float)ext.width, (float)ext.height },
                         (uint32_t)slot, vk_fsr_sharpness };
    vkCmdPushConstants(cmd, vk_draw_pipeline_layout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof pc, &pc);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}

// RenderScale: the scene image into the swapchain image, through FSR 1 (below 1) or the box filter (above 1). Leaves
// the swapchain image's rendering pass open for the badge.
static void vk_post_scale(VkCommandBuffer cmd)
{
    vk_suspend_rendering();
    const VkPipelineStageFlags2 out = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    const VkPipelineStageFlags2 frag = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    vk_image_barrier(cmd, vk_scene.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, out, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, frag,
                     VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    VkExtent2D scene = vkc.target_extent;
    if (vk_mid.image) {
        vk_image_barrier(cmd, vk_mid.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, frag, 0, out, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
        vk_post_begin_window_pass(cmd, vk_mid.view, vkc.extent);
        vk_post_scale_draw(cmd, vk_easu_pipeline, scene, vkc.extent, vk_scene.slot);
        vkCmdEndRendering(cmd);
        vk_image_barrier(cmd, vk_mid.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, out, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, frag,
                         VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
        vk_post_begin_window_pass(cmd, vkc.views[vk_image_index], vkc.extent);
        vk_post_scale_draw(cmd, vk_rcas_pipeline, vkc.extent, vkc.extent, vk_mid.slot);
    } else {
        vk_post_begin_window_pass(cmd, vkc.views[vk_image_index], vkc.extent);
        vk_post_scale_draw(cmd, vk_down_pipeline, scene, vkc.extent, vk_scene.slot);
    }
}

void vk_post_frame(VkCommandBuffer cmd)
{
    if (!vk_post_pipeline)
        return;
    float gamma = modern_brightness_pass_gamma(vkc.hwnd);
    float factor = modern_speed_badge_factor();
    if (gamma > 0.0f)
        vk_post_brightness(cmd, gamma);
    if (!vk_scene.image) {
        if (factor > 0.0f)
            vk_post_badge(cmd, factor, VK_BADGE_BOTH);
        vk_suspend_rendering();   // the game's draws rebind their state at the next rendering pass
        return;
    }
    if (factor > 0.0f)
        vk_post_badge(cmd, factor, VK_BADGE_PREPARE);
    vk_post_scale(cmd);
    if (factor > 0.0f)
        vk_post_badge(cmd, factor, VK_BADGE_DRAW);
    vkCmdEndRendering(cmd);
}
#endif
