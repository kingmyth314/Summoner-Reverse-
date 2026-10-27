// vk_tcache.cpp - the Vulkan back end's texture cache (rung L4, docs/future/vulkan-renderer.md). Not original
// source. 64-bit MODERN_BUILD with the Vulkan SDK only. See modern/vk_internal.h for the units.
//
// gr_opengl_tcache_set / create_texture / delete_texture / mark_texture_dirty / tcache_free / tcache_frame /
// tcache_flush with Vulkan images: one slot per bitmap frame (bm_get_current_frame), the same early-out and
// bookkeeping, the same sizes (power of two, at most 2048, as the 64-bit OpenGL build caps GL_MAX_TEXTURE_SIZE) and
// the same conversions (vsdk/gr_opengl.cpp's gl_convert_*, gl_build_mipmaps). OpenGL's packed 16-bit formats are not
// used: the game takes them only when the driver's version string contains "1.2", and on this PC's OpenGL it does
// not (gl_packed_pixels_supported read as 0 in the running 64-bit build, 2026-10-26), so textures are 8 bits per
// channel there too. Every texture is an RGBA8 image; the RGB conversions get alpha 255.
//
// Images live in 32 MB device memory blocks (a first-fit list per block). An image the game replaces or frees may
// still be read by a frame in flight, so it, its descriptor slot and its memory are freed when that frame is done.

#if defined(_WIN64) && defined(MODERN_VULKAN)
#include <string.h>
#include <stdio.h>
#include "vk_internal.h"
#include "vsdk/bmpman/bmpman.h"
#include "vsdk/os/memory.h"
#include "vsdk/os/registry.h"

#define VK_TEX_MAX_SIZE 2048                   // gr_opengl_texture_init in REBUILT_OPENGL builds: GL_MAX_TEXTURE_SIZE capped
#define VK_BLOCK_SIZE (32u << 20)
#define VK_MAX_BLOCKS 64
#define VK_STAGING_SIZE (32u << 20)

// ---- device memory for images ----
struct vk_range { VkDeviceSize offset, size; };
struct vk_block {
    VkDeviceMemory memory;
    uint32_t type;
    vk_range *free_list;                       // sorted by offset
    int free_count, free_cap;
};
static vk_block vk_blocks[VK_MAX_BLOCKS];
static int vk_block_count;

struct vk_alloc { int block; VkDeviceSize offset, size; };

static void vk_block_free_insert(vk_block *b, VkDeviceSize offset, VkDeviceSize size)
{
    int i = 0;
    while (i < b->free_count && b->free_list[i].offset < offset)
        i++;
    if (b->free_count == b->free_cap) {
        b->free_cap = b->free_cap ? b->free_cap * 2 : 64;
        vk_range *n = new vk_range[b->free_cap];
        if (b->free_count)
            memcpy(n, b->free_list, sizeof(vk_range) * b->free_count);
        delete[] b->free_list;
        b->free_list = n;
    }
    memmove(&b->free_list[i + 1], &b->free_list[i], sizeof(vk_range) * (b->free_count - i));
    b->free_list[i].offset = offset;
    b->free_list[i].size = size;
    b->free_count++;
    // Merge with the neighbours.
    if (i + 1 < b->free_count && b->free_list[i].offset + b->free_list[i].size == b->free_list[i + 1].offset) {
        b->free_list[i].size += b->free_list[i + 1].size;
        memmove(&b->free_list[i + 1], &b->free_list[i + 2], sizeof(vk_range) * (b->free_count - i - 2));
        b->free_count--;
    }
    if (i > 0 && b->free_list[i - 1].offset + b->free_list[i - 1].size == b->free_list[i].offset) {
        b->free_list[i - 1].size += b->free_list[i].size;
        memmove(&b->free_list[i], &b->free_list[i + 1], sizeof(vk_range) * (b->free_count - i - 1));
        b->free_count--;
    }
}

static bool vk_mem_alloc(const VkMemoryRequirements *req, vk_alloc *out)
{
    uint32_t type = vk_memory_type(req->memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (type == UINT32_MAX)
        return false;
    for (int pass = 0; pass < 2; pass++) {
        for (int bi = 0; bi < vk_block_count; bi++) {
            vk_block *b = &vk_blocks[bi];
            if (b->type != type)
                continue;
            for (int i = 0; i < b->free_count; i++) {
                VkDeviceSize start = (b->free_list[i].offset + req->alignment - 1) & ~(req->alignment - 1);
                VkDeviceSize end = b->free_list[i].offset + b->free_list[i].size;
                if (start + req->size > end)
                    continue;
                VkDeviceSize r_off = b->free_list[i].offset;
                // Take [r_off, start + size) and give back the front padding and the rest.
                memmove(&b->free_list[i], &b->free_list[i + 1], sizeof(vk_range) * (b->free_count - i - 1));
                b->free_count--;
                if (start > r_off)
                    vk_block_free_insert(b, r_off, start - r_off);
                if (start + req->size < end)
                    vk_block_free_insert(b, start + req->size, end - start - req->size);
                out->block = bi;
                out->offset = start;
                out->size = req->size;
                return true;
            }
        }
        if (pass == 1 || vk_block_count == VK_MAX_BLOCKS)
            break;
        // A new block (or a dedicated one for an image larger than a block).
        VkDeviceSize size = req->size > VK_BLOCK_SIZE ? req->size : VK_BLOCK_SIZE;
        VkMemoryAllocateInfo ai = {};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = size;
        ai.memoryTypeIndex = type;
        vk_block *b = &vk_blocks[vk_block_count];
        memset(b, 0, sizeof *b);
        if (vkAllocateMemory(vkc.device, &ai, NULL, &b->memory) != VK_SUCCESS)
            return false;
        b->type = type;
        vk_block_free_insert(b, 0, size);
        vk_block_count++;
    }
    return false;
}

static void vk_mem_free(const vk_alloc *a)
{
    if (a->block >= 0 && a->size)
        vk_block_free_insert(&vk_blocks[a->block], a->offset, a->size);
}

// ---- texture slots ----
struct vk_texture {
    VkImage image;
    VkImageView view;
    vk_alloc mem;
    int slot;                                  // in the shaders' images[]; -1 without an image
    int w, h, levels;
};

// One per bitmap frame, as gl_tcache_entry (the fields the cache logic uses).
struct vk_entry {
    int bitmap;                                // the bitmap handle uploaded, -1 when not resident
    int used;                                  // uses this frame
    bool dirty;
    bool created;                              // a texture object exists (OpenGL's e->gl_id with a texture)
    float u_scale, v_scale;
    bool clamp;                                // the texture object's GL_TEXTURE_WRAP_S / T: GL_CLAMP or GL_REPEAT
    vk_texture tex;
};
static vk_entry *vk_entries;
static int vk_bound_bitmap[2] = { -1, -1 };   // gr_opengl_bound_texture: the early-out cache, reset every flip
static int vk_gl_binding[2] = { -1, -1 };     // the entry glBindTexture left on each stage (persists)

// Slots and images to free once the frames that may use them are done: [frame slot], and one list for "after
// everything submitted".
struct vk_garbage { vk_texture *items; int count, cap; };
static vk_garbage vk_garbage_lists[VK_FRAMES];
static int vk_free_slots[VK_MAX_TEXTURES];
static int vk_free_slot_count;

static vk_buffer vk_staging[VK_FRAMES];
static VkDeviceSize vk_staging_used[VK_FRAMES];
static ubyte *vk_convert_buffer;               // gl_convert_buffer's role: the mipmap chain and one converted level

static void vk_destroy_texture_now(vk_texture *t)
{
    if (t->view)
        vkDestroyImageView(vkc.device, t->view, NULL);
    if (t->image)
        vkDestroyImage(vkc.device, t->image, NULL);
    vk_mem_free(&t->mem);
    if (t->slot >= 0)
        vk_free_slots[vk_free_slot_count++] = t->slot;
    memset(t, 0, sizeof *t);
    t->slot = -1;
    t->mem.block = -1;
}

// The texture may still be read by the frames in flight: freed when the frame slot it is listed with comes round
// again and its fence has been waited for (vk_tcache_frame_done).
static void vk_retire_texture(vk_texture *t)
{
    if (!t->image) {
        memset(t, 0, sizeof *t);
        t->slot = -1;
        t->mem.block = -1;
        return;
    }
    // While a frame is recorded, with that frame (its slot's fence covers it and the frame before). Between frames
    // vk_frame_index already names the next slot, whose fence is two frames old: the texture goes with the frame just
    // submitted instead.
    vk_garbage *g = &vk_garbage_lists[vk_recording ? vk_frame_index : (vk_frame_index + VK_FRAMES - 1) % VK_FRAMES];
    if (g->count == g->cap) {
        g->cap = g->cap ? g->cap * 2 : 64;
        vk_texture *n = new vk_texture[g->cap];
        if (g->count)
            memcpy(n, g->items, sizeof(vk_texture) * g->count);
        delete[] g->items;
        g->items = n;
    }
    g->items[g->count++] = *t;
    memset(t, 0, sizeof *t);
    t->slot = -1;
    t->mem.block = -1;
}

void vk_tcache_frame_done(int frame)
{
    for (int f = 0; f < VK_FRAMES; f++) {
        if (frame >= 0 && f != frame)
            continue;
        vk_garbage *g = &vk_garbage_lists[f];
        for (int i = 0; i < g->count; i++)
            vk_destroy_texture_now(&g->items[i]);
        g->count = 0;
        vk_staging_used[f] = 0;
    }
}

bool vk_tcache_init()
{
    for (int i = 0; i < VK_MAX_TEXTURES; i++)
        vk_free_slots[i] = VK_MAX_TEXTURES - 1 - i;
    vk_free_slot_count = VK_MAX_TEXTURES;
    for (int f = 0; f < VK_FRAMES; f++)
        if (!vk_create_buffer(VK_STAGING_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, &vk_staging[f]))
            return false;
    return true;
}

void vk_tcache_close_all()
{
    vk_tcache_frame_done(-1);
    if (vk_entries) {
        for (int i = 0; i < MAX_BITMAPS; i++)
            vk_destroy_texture_now(&vk_entries[i].tex);
        delete[] vk_entries;
        vk_entries = NULL;
    }
    for (int f = 0; f < VK_FRAMES; f++) {
        vk_destroy_buffer(&vk_staging[f]);
        delete[] vk_garbage_lists[f].items;
        memset(&vk_garbage_lists[f], 0, sizeof vk_garbage_lists[f]);
    }
    for (int b = 0; b < vk_block_count; b++) {
        vkFreeMemory(vkc.device, vk_blocks[b].memory, NULL);
        delete[] vk_blocks[b].free_list;
    }
    memset(vk_blocks, 0, sizeof vk_blocks);
    vk_block_count = 0;
    delete[] vk_convert_buffer;
    vk_convert_buffer = NULL;
}

int vk_tcache_bound(int stage)
{
    int e = vk_gl_binding[stage];
    return e >= 0 && vk_entries && vk_entries[e].tex.image ? vk_entries[e].tex.slot : -1;
}

bool vk_tcache_clamps(int stage)
{
    int e = vk_gl_binding[stage];
    return e >= 0 && vk_entries ? vk_entries[e].clamp : false;
}

void vk_tcache_set_clamp(int stage, bool clamp)
{
    int e = vk_gl_binding[stage];
    if (e >= 0 && vk_entries)
        vk_entries[e].clamp = clamp;
}

static void vk_delete_texture(vk_entry *e);

// ---- uploads ----
static bool vk_make_image(vk_texture *t, int w, int h, int levels)
{
    VkImageCreateInfo ii = {};
    ii.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = VK_FORMAT_R8G8B8A8_UNORM;
    ii.extent.width = (uint32_t)w;
    ii.extent.height = (uint32_t)h;
    ii.extent.depth = 1;
    ii.mipLevels = (uint32_t)levels;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (vkCreateImage(vkc.device, &ii, NULL, &t->image) != VK_SUCCESS)
        return false;
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(vkc.device, t->image, &req);
    if (!vk_mem_alloc(&req, &t->mem) || !vk_free_slot_count) {
        vkDestroyImage(vkc.device, t->image, NULL);
        t->image = VK_NULL_HANDLE;
        return false;
    }
    vkBindImageMemory(vkc.device, t->image, vk_blocks[t->mem.block].memory, t->mem.offset);
    VkImageViewCreateInfo vi = {};
    vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image = t->image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = VK_FORMAT_R8G8B8A8_UNORM;
    vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vi.subresourceRange.levelCount = (uint32_t)levels;
    vi.subresourceRange.layerCount = 1;
    vkCreateImageView(vkc.device, &vi, NULL, &t->view);
    t->slot = vk_free_slots[--vk_free_slot_count];
    t->w = w;
    t->h = h;
    t->levels = levels;
    VkDescriptorImageInfo di = {};
    di.imageView = t->view;
    di.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet wr = {};
    wr.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    wr.dstSet = vk_texture_set;
    wr.dstBinding = 0;
    wr.dstArrayElement = (uint32_t)t->slot;
    wr.descriptorCount = 1;
    wr.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    wr.pImageInfo = &di;
    vkUpdateDescriptorSets(vkc.device, 1, &wr, 0, NULL);
    return true;
}

// The levels of one texture, converted (RGBA8, level after level) in data: copied through staging into the image.
static void vk_upload_levels(vk_texture *t, const ubyte *data, VkDeviceSize size, bool fresh)
{
    VkCommandBuffer cmd = vk_begin_transfer();
    vk_buffer own = {};
    vk_buffer *stage;
    VkDeviceSize at = 0;
    bool in_frame = vk_recording;
    if (in_frame) {
        int f = vk_frame_index;
        VkDeviceSize start = (vk_staging_used[f] + 15) & ~(VkDeviceSize)15;
        if (start + size > VK_STAGING_SIZE) {
            vk_wait_idle();   // the frame's staging is free again
            cmd = vk_begin_transfer();
            start = 0;
        }
        if (start + size > VK_STAGING_SIZE) {
            vk_create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, &own);
            stage = &own;
        } else {
            stage = &vk_staging[f];
            at = start;
            vk_staging_used[f] = start + size;
        }
    } else {
        vk_create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, &own);
        stage = &own;
    }
    if (!stage->buffer) {
        vk_end_transfer();
        return;
    }
    memcpy((ubyte *)stage->mapped + at, data, (size_t)size);

    VkImageMemoryBarrier2 b = {};
    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    // A re-upload (a dirty bitmap) waits for the draws before it that sample the image, in this frame and the last.
    b.srcStageMask = fresh ? VK_PIPELINE_STAGE_2_NONE : VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    b.srcAccessMask = 0;
    b.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    b.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    b.oldLayout = fresh ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = t->image;
    b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    b.subresourceRange.levelCount = (uint32_t)t->levels;
    b.subresourceRange.layerCount = 1;
    VkDependencyInfo di = {};
    di.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    di.imageMemoryBarrierCount = 1;
    di.pImageMemoryBarriers = &b;
    vkCmdPipelineBarrier2(cmd, &di);

    VkBufferImageCopy regions[16];
    VkDeviceSize off = at;
    int w = t->w, h = t->h;
    for (int l = 0; l < t->levels && l < 16; l++) {
        VkBufferImageCopy *r = &regions[l];
        memset(r, 0, sizeof *r);
        r->bufferOffset = off;
        r->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        r->imageSubresource.mipLevel = (uint32_t)l;
        r->imageSubresource.layerCount = 1;
        r->imageExtent.width = (uint32_t)w;
        r->imageExtent.height = (uint32_t)h;
        r->imageExtent.depth = 1;
        off += (VkDeviceSize)w * h * 4;
        w = w > 1 ? w / 2 : 1;
        h = h > 1 ? h / 2 : 1;
    }
    vkCmdCopyBufferToImage(cmd, stage->buffer, t->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           (uint32_t)(t->levels < 16 ? t->levels : 16), regions);

    b.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    b.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    b.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    b.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier2(cmd, &di);
    vk_end_transfer();
    if (own.buffer) {
        if (in_frame)
            vk_wait_idle();   // a one-off staging buffer larger than the ring: free it once the copy is done
        vk_destroy_buffer(&own);
    }
}

// RGB888 rows (the 565 / 888 conversions) to RGBA8 with alpha 255.
static void vk_rgb_to_rgba(const ubyte *src, ubyte *dst, int pixels)
{
    for (int i = pixels - 1; i >= 0; i--) {
        dst[i * 4 + 0] = src[i * 3 + 0];
        dst[i * 4 + 1] = src[i * 3 + 1];
        dst[i * 4 + 2] = src[i * 3 + 2];
        dst[i * 4 + 3] = 255;
    }
}

// gr_opengl_create_texture: the bitmap's levels at power-of-two sizes, converted as the OpenGL back end converts
// them without packed pixels. update: the bitmap changed (a movie frame); the image is rewritten in place when its
// size is the same.
static void vk_create_texture(int bm, vk_entry *e, int update)
{
    int w, h, num_pixels, levels;
    ubyte *data, *palette;
    int tex_w, tex_h, i;

    Mempool *old_pool = get_current_mempool();
    set_current_mempool(NULL);
    if (!update)
        vk_delete_texture(e);   // as gr_opengl_create_texture (tcache_set never gets here with a resident bitmap)
    bm_get_mipmap_info(bm, &w, &h, &num_pixels, &levels);
    int format = bm_lock(bm, &data, &palette);
    int bpp = bm_bytes_per_pixel(format);
    if (!format) {
        set_current_mempool(old_pool);
        return;
    }
    tex_w = w;
    tex_h = h;
    for (i = 0; i < 16; i++)
        if (w > (1 << i) && w <= (1 << (i + 1))) {
            tex_w = 1 << (i + 1);
            break;
        }
    for (i = 0; i < 16; i++)
        if (h > (1 << i) && h <= (1 << (i + 1))) {
            tex_h = 1 << (i + 1);
            break;
        }
    if (tex_w < gl_min_texture_w)
        tex_w = gl_min_texture_w;
    else if (tex_w > gr_screen.max_bitmap_w)
        tex_w = gr_screen.max_bitmap_w;
    if (tex_h < gl_min_texture_h)
        tex_h = gl_min_texture_h;
    else if (tex_h > gr_screen.max_bitmap_h)
        tex_h = gr_screen.max_bitmap_h;
    if (tex_w < 1 || tex_h < 1) {
        bm_unlock(bm);
        return;
    }
    e->u_scale = (float)w / ((float)tex_w);
    e->v_scale = (float)h / ((float)tex_h);
    int w0 = tex_w, h0 = tex_h;

    // The level count OpenGL ends up with: the loop stops after level 0 for one level, else when the bitmap's
    // level reaches 1x1 (the texture's sizes halve alongside).
    int gl_levels = 1;
    if (levels != 1) {
        int lw = w, lh = h;
        while (!(lw == 1 && lh == 1)) {
            lw = lw > 1 ? lw / 2 : 1;
            lh = lh > 1 ? lh / 2 : 1;
            gl_levels++;
        }
    }
    // An image whose levels OpenGL would leave incomplete (the texture's sizes reach 1x1 before or after the
    // bitmap's) samples level 0 only here.
    int full_levels = 1;
    for (int lw = tex_w, lh = tex_h; lw > 1 || lh > 1; lw = lw > 1 ? lw / 2 : 1, lh = lh > 1 ? lh / 2 : 1)
        full_levels++;
    int img_levels = gl_levels == full_levels ? gl_levels : 1;

    VkDeviceSize total = 0;
    for (int l = 0, lw = tex_w, lh = tex_h; l < img_levels; l++, lw = lw > 1 ? lw / 2 : 1, lh = lh > 1 ? lh / 2 : 1)
        total += (VkDeviceSize)lw * lh * 4;
    if (!vk_convert_buffer)
        vk_convert_buffer = new ubyte[(size_t)VK_TEX_MAX_SIZE * VK_TEX_MAX_SIZE * 4 * 2];
    ubyte *out = new ubyte[(size_t)total];

    ubyte *src, *dst;
    if (levels > 1) {
        src = vk_convert_buffer;
        dst = gl_build_mipmaps(format, levels, w, h, data, vk_convert_buffer);
    } else {
        src = data;
        dst = vk_convert_buffer;
    }
    VkDeviceSize out_at = 0;
    for (int level = 0; level < img_levels; level++) {
        bool rgb = false;
        switch (format) {
        case BM_FORMAT_8_ALPHA:
            gl_convert_a8_to_rgba8888(tex_h, tex_w, h, w, src, dst);
            break;
        case BM_FORMAT_888:
            gl_convert_888_to_rgb888(tex_h, tex_w, h, w, src, dst);
            rgb = true;
            break;
        case BM_FORMAT_4444:
            gl_convert_4444_to_rgba8888(tex_h, tex_w, h, w, src, dst);
            break;
        case BM_FORMAT_1555:
            gl_convert_1555_to_rgba8888(tex_h, tex_w, h, w, src, dst);
            break;
        case BM_FORMAT_565:
            gl_convert_565_to_rgb888(tex_h, tex_w, h, w, src, dst);
            rgb = true;
            break;
        case BM_FORMAT_8888:
            gl_convert_8888_to_rgba8888(tex_h, tex_w, h, w, src, dst);
            break;
        default:
            // OpenGL uploads the convert buffer's previous contents as RGBA; here a transparent level.
            memset(dst, 0, (size_t)tex_w * tex_h * 4);
            break;
        }
        if (rgb)
            vk_rgb_to_rgba(dst, out + out_at, tex_w * tex_h);
        else
            memcpy(out + out_at, dst, (size_t)tex_w * tex_h * 4);
        out_at += (VkDeviceSize)tex_w * tex_h * 4;
        if (levels == 1 || (w == 1 && h == 1))
            break;
        src += bpp * w * h;
        tex_w = tex_w > 1 ? tex_w / 2 : 1;
        tex_h = tex_h > 1 ? tex_h / 2 : 1;
        w = w > 1 ? w / 2 : 1;
        h = h > 1 ? h / 2 : 1;
    }
    bm_unlock(bm);

    bool fresh = true;
    if (e->tex.image && update && e->tex.w == w0 && e->tex.h == h0 && e->tex.levels == img_levels) {
        fresh = false;   // glTexSubImage2D / glTexImage2D into the same texture object
    } else {
        if (e->tex.image)
            vk_retire_texture(&e->tex);   // a texture object re-specified (or one still used this frame)
        if (!e->created)
            e->clamp = false;              // a new texture object: GL_REPEAT, OpenGL's default
        if (!vk_make_image(&e->tex, w0, h0, img_levels)) {
            delete[] out;
            set_current_mempool(old_pool);
            return;
        }
    }
    vk_upload_levels(&e->tex, out, out_at, fresh);
    delete[] out;
    e->bitmap = bm;
    e->created = true;
    set_current_mempool(old_pool);
}

int vk_tcache_alloc_slot(VkImageView view)
{
    if (!vk_free_slot_count)
        return -1;
    int slot = vk_free_slots[--vk_free_slot_count];
    VkDescriptorImageInfo di = {};
    di.imageView = view;
    di.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet wr = {};
    wr.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    wr.dstSet = vk_texture_set;
    wr.dstBinding = 0;
    wr.dstArrayElement = (uint32_t)slot;
    wr.descriptorCount = 1;
    wr.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    wr.pImageInfo = &di;
    vkUpdateDescriptorSets(vkc.device, 1, &wr, 0, NULL);
    return slot;
}

void vk_tcache_free_slot(int slot)
{
    if (slot >= 0)
        vk_free_slots[vk_free_slot_count++] = slot;
}

// ---- the entry points ----
void vk_gr_opengl_texture_init()
{
    os_registry_read_dword(NULL, "PackedPixels", (DWORD *)&gr_opengl_packed_pixels, 1);
    if (!vk_entries)
        vk_entries = new vk_entry[MAX_BITMAPS];
    for (int i = 0; i < MAX_BITMAPS; i++) {
        memset(&vk_entries[i], 0, sizeof vk_entries[i]);
        vk_entries[i].bitmap = -1;
        vk_entries[i].tex.slot = -1;
        vk_entries[i].tex.mem.block = -1;
    }
    vk_bound_bitmap[0] = vk_bound_bitmap[1] = -1;
    vk_gl_binding[0] = vk_gl_binding[1] = -1;
    int max_size = (int)vkc.gpu_props.limits.maxImageDimension2D;
    if (max_size > VK_TEX_MAX_SIZE)
        max_size = VK_TEX_MAX_SIZE;
    gr_screen.max_bitmap_w = max_size;
    gr_screen.max_bitmap_h = max_size;
    gl_packed_pixels_supported = false;
}

void vk_gr_opengl_tcache_close()
{
    vk_gr_opengl_tcache_flush();
}

static void vk_delete_texture(vk_entry *e)
{
    if (e->bitmap > -1 && e->used == 0) {
        int idx = (int)(e - vk_entries);
        vk_retire_texture(&e->tex);
        e->created = false;
        if (vk_bound_bitmap[0] == e->bitmap)
            vk_bound_bitmap[0] = -1;
        if (vk_bound_bitmap[1] == e->bitmap)
            vk_bound_bitmap[1] = -1;
        // glDeleteTextures unbinds a bound texture (the stage falls back to texture 0, which has no image).
        for (int s = 0; s < 2; s++)
            if (vk_gl_binding[s] == idx)
                vk_gl_binding[s] = -1;
        e->bitmap = -1;
    }
}

void vk_gr_opengl_tcache_flush()
{
    if (!vk_entries)
        return;
    for (int i = 0; i < MAX_BITMAPS; i++)
        vk_delete_texture(&vk_entries[i]);
    vk_bound_bitmap[0] = vk_bound_bitmap[1] = -1;
}

void vk_gr_opengl_tcache_frame()
{
    vk_bound_bitmap[0] = vk_bound_bitmap[1] = -1;
    if (vk_entries)
        for (int i = 0; i < MAX_BITMAPS; i++)
            vk_entries[i].used = 0;
}

int vk_gr_opengl_tcache_set(int bm, float *u_scale, float *v_scale, int stage)
{
    if (bm < 0 || !vk_entries) {
        vk_bound_bitmap[stage] = -1;
        return 0;
    }
    int idx = bm_get_current_frame(bm);
    vk_entry *e = &vk_entries[idx];
    if (vk_bound_bitmap[stage] == bm && e->bitmap == bm) {
        e->used++;
        *u_scale = e->u_scale;
        *v_scale = e->v_scale;
        return 1;
    }
    vk_gl_binding[stage] = idx;   // glBindTexture
    if (e->bitmap < 0) {
        vk_create_texture(bm, e, 0);
        e->dirty = false;
    }
    if (e->dirty) {
        vk_create_texture(bm, e, 1);
        e->dirty = false;
    }
    *u_scale = e->u_scale;
    *v_scale = e->v_scale;
    vk_bound_bitmap[stage] = e->bitmap;
    e->used++;
    vk_gr_opengl_get_current_mode()->invalidate();
    return 1;
}

void vk_gr_opengl_mark_texture_dirty(int bm)
{
    if (!vk_entries)
        return;
    vk_entries[bm_get_current_frame(bm)].dirty = true;
    if (vk_bound_bitmap[0] == bm)
        vk_bound_bitmap[0] = -1;
    if (vk_bound_bitmap[1] == bm)
        vk_bound_bitmap[1] = -1;
}

void vk_gr_opengl_tcache_free(int bm)
{
    if (!vk_entries)
        return;
    int frame = bm_get_current_frame(bm);
    if (vk_bound_bitmap[0] == bm)
        vk_bound_bitmap[0] = -1;
    if (vk_bound_bitmap[1] == bm)
        vk_bound_bitmap[1] = -1;
    vk_delete_texture(&vk_entries[frame]);
    vk_entries[frame].bitmap = -1;
}
#endif
