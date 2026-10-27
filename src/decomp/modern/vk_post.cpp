// vk_post.cpp - the Vulkan back end's end-of-frame passes (rung L4, docs/future/vulkan-renderer.md). Not original
// source. 64-bit MODERN_BUILD with the Vulkan SDK only. See modern/vk_internal.h for the units.
//
// What modern_swap_buffers draws over the OpenGL frame (modern/display.cpp, modern/gamma.cpp), for the Vulkan one:
//   brightness  [Display] Brightness's choice (modern_brightness_pass_gamma): the monitor ramp, or this pass: the
//               frame is copied into an image and drawn back through pow(c, 1 / gamma) (modern/shaders/post.frag);
//   the badge   "FAST xN" in the picture's top-left corner while the speed key's fast mode is on in play: the text is
//               drawn by GDI into a small image (as the OpenGL host draws it with wglUseFontBitmaps) and copied over.
// The bars need nothing: the Vulkan frame starts cleared to black.

#if defined(_WIN64) && defined(MODERN_VULKAN)
#include <string.h>
#include <stdio.h>
#include "vk_internal.h"
#include "vk_shaders.h"

struct vk_post_image {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    int slot;
    int w, h;
};

static VkPipeline vk_post_pipeline;
static vk_post_image vk_copy;                  // the frame's copy for the brightness pass
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

static bool vk_post_make_image(vk_post_image *p, int w, int h, VkFormat format)
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
    ii.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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

bool vk_post_init()
{
    vk_copy.slot = vk_badge.slot = -1;
    VkShaderModuleCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    VkShaderModule vert = VK_NULL_HANDLE, frag = VK_NULL_HANDLE;
    ci.codeSize = sizeof vk_spv_post_vert;
    ci.pCode = (const uint32_t *)vk_spv_post_vert;
    vkCreateShaderModule(vkc.device, &ci, NULL, &vert);
    ci.codeSize = sizeof vk_spv_post_frag;
    ci.pCode = (const uint32_t *)vk_spv_post_frag;
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
    rinfo.depthAttachmentFormat = vkc.depth_format;
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
    bool ok = vert && frag &&
              vkCreateGraphicsPipelines(vkc.device, VK_NULL_HANDLE, 1, &pi, NULL, &vk_post_pipeline) == VK_SUCCESS;
    if (vert)
        vkDestroyShaderModule(vkc.device, vert, NULL);
    if (frag)
        vkDestroyShaderModule(vkc.device, frag, NULL);
    return ok;
}

void vk_post_close()
{
    vk_post_destroy_image(&vk_copy);
    vk_post_destroy_image(&vk_badge);
    vk_destroy_buffer(&vk_badge_staging);
    if (vk_post_pipeline)
        vkDestroyPipeline(vkc.device, vk_post_pipeline, NULL);
    vk_post_pipeline = VK_NULL_HANDLE;
    vk_badge_text[0] = 0;
    vk_badge_fh = 0;
}

// Draws the post pipeline's triangle over [x, y, w, h] of the window, reading image slot with origin at (x, y).
static void vk_post_draw(VkCommandBuffer cmd, int x, int y, int w, int h, int slot, uint32_t mode, float e)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_post_pipeline);
    VkViewport vp = { 0.0f, 0.0f, (float)vkc.extent.width, (float)vkc.extent.height, 0.0f, 1.0f };
    vkCmdSetViewport(cmd, 0, 1, &vp);
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > (int)vkc.extent.width ? (int)vkc.extent.width : x + w;
    int y1 = y + h > (int)vkc.extent.height ? (int)vkc.extent.height : y + h;
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
    int w = (int)vkc.extent.width, h = (int)vkc.extent.height;
    if ((vk_copy.w != w || vk_copy.h != h) && !vk_post_make_image(&vk_copy, w, h, vkc.swap_format))
        return;
    vk_suspend_rendering();
    VkImage swap = vkc.images[vk_image_index];
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
    vk_post_draw(cmd, 0, 0, w, h, vk_copy.slot, 0, 1.0f / gamma);
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

static void vk_post_badge(VkCommandBuffer cmd, float factor)
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
    vk_suspend_rendering();
    if (!vk_post_badge_image(cmd, text, fh, bw, bh, pad))
        return;
    vk_resume_rendering();
    vk_post_draw(cmd, x, y, vk_badge.w, vk_badge.h, vk_badge.slot, 1, 1.0f);
}

void vk_post_frame(VkCommandBuffer cmd)
{
    if (!vk_post_pipeline)
        return;
    float gamma = modern_brightness_pass_gamma(vkc.hwnd);
    float factor = modern_speed_badge_factor();
    if (gamma > 0.0f)
        vk_post_brightness(cmd, gamma);
    if (factor > 0.0f)
        vk_post_badge(cmd, factor);
    vk_suspend_rendering();   // the game's draws rebind their state at the next rendering pass
}
#endif
