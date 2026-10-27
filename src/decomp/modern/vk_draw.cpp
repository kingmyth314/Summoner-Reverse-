// vk_draw.cpp - the Vulkan back end's state and drawing (rung L4, docs/future/vulkan-renderer.md). Not original
// source. 64-bit MODERN_BUILD with the Vulkan SDK only. See modern/vk_internal.h for the units.
//
// Each entry point does what its OpenGL counterpart in vsdk/gr_opengl.cpp does, in the same order, so the pictures
// can be compared: gr_opengl_set_mode's state (applied only when the mode changes, each switch keeping the previous
// state for a value it does not list), the vertex colours of gr_opengl_bitmap / tmapper / line, the clip
// rectangle's projection taken at gr_opengl_set_viewport. The CPU-side state the game's own OpenGL draw lists read
// (gr_opengl_current_mode, gr_opengl_vertex_color / _alpha, gr_opengl_texture_u_scale / _v_scale) is the OpenGL
// back end's variables, shared, so modern/vk_scene.cpp can follow levelrender_draw_list_opengl line by line.

#if defined(_WIN64) && defined(MODERN_VULKAN)
#include <string.h>
#include <math.h>
#include "vk_internal.h"
#include "vsdk/bmpman/bmpman.h"
#include "vk_shaders.h"

void gr_opengl_apply_tint_gl(ubyte *rgb);   // vsdk/gr_opengl.cpp: CPU only, shared

VkDescriptorSetLayout vk_texture_layout;
VkDescriptorSet vk_texture_set;
static VkDescriptorPool vk_descriptor_pool;
static VkSampler vk_samplers[3];               // 0 repeat, 1 clamp to a transparent black border (GL_CLAMP),
                                               // 2 clamp to the edge (vk_post.cpp's scale pass)
static VkPipelineLayout vk_pipeline_layout;
// [blend pair][0 triangles, 1 lines]
#define VK_BLEND_PAIRS 6
static VkPipeline vk_pipelines[VK_BLEND_PAIRS][2];

// The shaders' push constants (modern/shaders/gr.vert, gr.frag).
struct vk_push {
    float scale[2];
    float offset[2];
    uint32_t flags;
    uint32_t tex0, tex1;
    uint32_t samplers;
    float fog_color[4];
    float fog_start, fog_end;
};
#define VK_FLAG_TEXTURE0 1u
#define VK_FLAG_TEXTURE1 2u
#define VK_FLAG_FOG 4u

// OpenGL's state as gr_opengl_set_mode / set_fog / set_viewport leave it.
static struct {
    int texture_source;                        // 0 off, 1-3 one texture, 4+ two
    int blend_pair;                            // vk_pipelines' first index
    bool depth_test, depth_write;
    VkCompareOp depth_op;
    bool fog;                                  // GL_FOG enabled
    float fog_start, fog_end, fog_color[3];
    // The projection and viewport of the clip rectangle at the last gr_opengl_set_viewport, in window pixels.
    float view_x, view_y, view_w, view_h;      // the viewport (the clip rectangle mapped into the window)
    float clip_x, clip_y, clip_w, clip_h;      // the clip rectangle in game pixels (glOrtho's bounds)
} vs;

// What the command buffer has bound (reset at each rendering pass).
static int bound_pipeline_pair = -1, bound_pipeline_class = -1;
static int bound_depth_test = -1, bound_depth_write = -1, bound_depth_op = -1, bound_topology = -1;
static float bound_view[4];

uint32_t vk_pack_color(const ubyte *rgba)
{
    return (uint32_t)rgba[0] | ((uint32_t)rgba[1] << 8) | ((uint32_t)rgba[2] << 16) | ((uint32_t)rgba[3] << 24);
}

static VkShaderModule vk_shader(const unsigned int *code, size_t size)
{
    VkShaderModuleCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = size;
    ci.pCode = (const uint32_t *)code;
    VkShaderModule m = VK_NULL_HANDLE;
    vkCreateShaderModule(vkc.device, &ci, NULL, &m);
    return m;
}

static bool vk_create_pipelines()
{
    VkShaderModule vert = vk_shader(vk_spv_gr_vert, sizeof vk_spv_gr_vert);
    VkShaderModule frag = vk_shader(vk_spv_gr_frag, sizeof vk_spv_gr_frag);
    if (!vert || !frag)
        return false;
    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding = { 0, sizeof(vk_vertex), VK_VERTEX_INPUT_RATE_VERTEX };
    VkVertexInputAttributeDescription attrs[4] = {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vk_vertex, x) },
        { 1, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(vk_vertex, color) },
        { 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vk_vertex, uv0) },
        { 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vk_vertex, uv1) },
    };
    VkPipelineVertexInputStateCreateInfo vi = {};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &binding;
    vi.vertexAttributeDescriptionCount = 4;
    vi.pVertexAttributeDescriptions = attrs;
    VkPipelineInputAssemblyStateCreateInfo ia = {};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    VkPipelineViewportStateCreateInfo vp = {};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs = {};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;          // the OpenGL back end sets glCullFace but never enables culling
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo ms = {};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo ds = {};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    VkDynamicState dyn[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
                             VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE, VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
                             VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY };
    VkPipelineDynamicStateCreateInfo dy = {};
    dy.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dy.dynamicStateCount = sizeof dyn / sizeof dyn[0];
    dy.pDynamicStates = dyn;
    VkPipelineRenderingCreateInfo rinfo = {};
    rinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rinfo.colorAttachmentCount = 1;
    rinfo.pColorAttachmentFormats = &vkc.swap_format;
    rinfo.depthAttachmentFormat = vkc.depth_format;

    // gr_opengl_set_mode's glBlendFunc pairs (the same factors for colour and alpha, as glBlendFunc sets them).
    static const VkBlendFactor pairs[VK_BLEND_PAIRS][2] = {
        { VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO },                       // 0 (and blending off)
        { VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE },                        // 1 with gl_blend_add_one_one
        { VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE },                  // 1 without it, 2
        { VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA },  // 3
        { VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR },  // 4
        { VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_ZERO },                 // 5
    };
    bool ok = true;
    for (int p = 0; p < VK_BLEND_PAIRS && ok; p++) {
        VkPipelineColorBlendAttachmentState att = {};
        att.blendEnable = VK_TRUE;
        att.srcColorBlendFactor = pairs[p][0];
        att.dstColorBlendFactor = pairs[p][1];
        att.colorBlendOp = VK_BLEND_OP_ADD;
        // glBlendFunc's alpha factors; a DST_COLOR / ONE_MINUS_SRC_COLOR factor applies to alpha as its alpha form.
        att.srcAlphaBlendFactor = pairs[p][0] == VK_BLEND_FACTOR_DST_COLOR ? VK_BLEND_FACTOR_DST_ALPHA : pairs[p][0];
        att.dstAlphaBlendFactor = pairs[p][1] == VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR
                                ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : pairs[p][1];
        att.alphaBlendOp = VK_BLEND_OP_ADD;
        att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                             VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo cb = {};
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.attachmentCount = 1;
        cb.pAttachments = &att;
        for (int cls = 0; cls < 2 && ok; cls++) {
            ia.topology = cls ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
            pi.layout = vk_pipeline_layout;
            ok = vkCreateGraphicsPipelines(vkc.device, VK_NULL_HANDLE, 1, &pi, NULL, &vk_pipelines[p][cls]) == VK_SUCCESS;
        }
    }
    vkDestroyShaderModule(vkc.device, vert, NULL);
    vkDestroyShaderModule(vkc.device, frag, NULL);
    return ok;
}

VkPipelineLayout vk_draw_pipeline_layout()
{
    return vk_pipeline_layout;
}

bool vk_draw_init()
{
    VkSamplerCreateInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // GL_LINEAR, or GL_LINEAR_MIPMAP_LINEAR for a texture with mipmaps: the same sampler, since an image without
    // mipmaps has only level 0.
    si.magFilter = VK_FILTER_LINEAR;
    si.minFilter = VK_FILTER_LINEAR;
    si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    si.maxLod = VK_LOD_CLAMP_NONE;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (vkCreateSampler(vkc.device, &si, NULL, &vk_samplers[0]) != VK_SUCCESS)
        return false;
    // GL_CLAMP: coordinates clamped to [0, 1], and linear filtering at the edge takes in the border colour, OpenGL's
    // default (0, 0, 0, 0).
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    si.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    if (vkCreateSampler(vkc.device, &si, NULL, &vk_samplers[1]) != VK_SUCCESS)
        return false;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (vkCreateSampler(vkc.device, &si, NULL, &vk_samplers[2]) != VK_SUCCESS)
        return false;

    VkDescriptorSetLayoutBinding b[2] = {};
    b[0].binding = 0;
    b[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    b[0].descriptorCount = VK_MAX_TEXTURES;
    b[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    b[1].binding = 1;
    b[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    b[1].descriptorCount = 3;
    b[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    b[1].pImmutableSamplers = vk_samplers;
    VkDescriptorBindingFlags bf[2] = { VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                                       VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
                                       VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT, 0 };
    VkDescriptorSetLayoutBindingFlagsCreateInfo bfi = {};
    bfi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bfi.bindingCount = 2;
    bfi.pBindingFlags = bf;
    VkDescriptorSetLayoutCreateInfo li = {};
    li.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    li.pNext = &bfi;
    li.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    li.bindingCount = 2;
    li.pBindings = b;
    if (vkCreateDescriptorSetLayout(vkc.device, &li, NULL, &vk_texture_layout) != VK_SUCCESS)
        return false;
    VkDescriptorPoolSize sizes[2] = { { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_MAX_TEXTURES },
                                      { VK_DESCRIPTOR_TYPE_SAMPLER, 3 } };
    VkDescriptorPoolCreateInfo pi = {};
    pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pi.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    pi.maxSets = 1;
    pi.poolSizeCount = 2;
    pi.pPoolSizes = sizes;
    if (vkCreateDescriptorPool(vkc.device, &pi, NULL, &vk_descriptor_pool) != VK_SUCCESS)
        return false;
    VkDescriptorSetAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = vk_descriptor_pool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts = &vk_texture_layout;
    if (vkAllocateDescriptorSets(vkc.device, &ai, &vk_texture_set) != VK_SUCCESS)
        return false;

    VkPushConstantRange pr = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vk_push) };
    VkPipelineLayoutCreateInfo pli = {};
    pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &vk_texture_layout;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &pr;
    if (vkCreatePipelineLayout(vkc.device, &pli, NULL, &vk_pipeline_layout) != VK_SUCCESS)
        return false;
    if (!vk_create_pipelines())
        return false;

    // gr_opengl_init_context's state: blending on, the alpha test, the modulate stage; the projection of the clip
    // rectangle follows from gr_opengl_set_viewport.
    memset(&vs, 0, sizeof vs);
    vs.depth_op = VK_COMPARE_OP_LESS_OR_EQUAL;
    return true;
}

void vk_draw_close()
{
    for (int p = 0; p < VK_BLEND_PAIRS; p++)
        for (int c = 0; c < 2; c++)
            if (vk_pipelines[p][c])
                vkDestroyPipeline(vkc.device, vk_pipelines[p][c], NULL);
    memset(vk_pipelines, 0, sizeof vk_pipelines);
    if (vk_pipeline_layout)
        vkDestroyPipelineLayout(vkc.device, vk_pipeline_layout, NULL);
    if (vk_descriptor_pool)
        vkDestroyDescriptorPool(vkc.device, vk_descriptor_pool, NULL);
    if (vk_texture_layout)
        vkDestroyDescriptorSetLayout(vkc.device, vk_texture_layout, NULL);
    for (int i = 0; i < 3; i++)
        if (vk_samplers[i])
            vkDestroySampler(vkc.device, vk_samplers[i], NULL);
    vk_pipeline_layout = VK_NULL_HANDLE;
    vk_descriptor_pool = VK_NULL_HANDLE;
    vk_texture_layout = VK_NULL_HANDLE;
    vk_texture_set = VK_NULL_HANDLE;
    memset(vk_samplers, 0, sizeof vk_samplers);
}

void vk_draw_frame_start()
{
    bound_pipeline_pair = bound_pipeline_class = -1;
    bound_depth_test = bound_depth_write = bound_depth_op = bound_topology = -1;
    bound_view[2] = -1.0f;
    vkCmdBindDescriptorSets(vk_frames[vk_frame_index].cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_layout, 0, 1,
                            &vk_texture_set, 0, NULL);
}

// Copies n items into the frame's ring; false when it is full (the caller flushes and retries).
static bool vk_ring_alloc(vk_buffer *b, VkDeviceSize *used, const void *data, VkDeviceSize size, VkDeviceSize *at)
{
    VkDeviceSize start = (*used + 15) & ~(VkDeviceSize)15;
    if (start + size > b->size)
        return false;
    memcpy((char *)b->mapped + start, data, (size_t)size);
    *used = start + size;
    *at = start;
    return true;
}

void vk_draw(vk_primitive prim, const vk_vertex *v, int nv, const unsigned short *idx, int nidx)
{
    if (nv <= 0 || (prim == VK_PRIM_TRIANGLES && nidx <= 0))
        return;
    VkCommandBuffer cmd = vk_cmd();
    if (!cmd)
        return;
    vk_frame *f = &vk_frames[vk_frame_index];
    VkDeviceSize vat = 0, iat = 0;
    if (!vk_ring_alloc(&f->vertices, &f->vertex_used, v, (VkDeviceSize)nv * sizeof(vk_vertex), &vat) ||
        (idx && !vk_ring_alloc(&f->indices, &f->index_used, idx, (VkDeviceSize)nidx * 2, &iat))) {
        vk_wait_idle();   // the frame's rings are free again
        cmd = vk_cmd();
        f->vertex_used = f->index_used = 0;
        if (!vk_ring_alloc(&f->vertices, &f->vertex_used, v, (VkDeviceSize)nv * sizeof(vk_vertex), &vat) ||
            (idx && !vk_ring_alloc(&f->indices, &f->index_used, idx, (VkDeviceSize)nidx * 2, &iat)))
            return;   // larger than a ring: never from the game (30-vertex polygons, 4000-vertex lists)
    }

    int cls = prim == VK_PRIM_LINES ? 1 : 0;
    if (bound_pipeline_pair != vs.blend_pair || bound_pipeline_class != cls) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipelines[vs.blend_pair][cls]);
        bound_pipeline_pair = vs.blend_pair;
        bound_pipeline_class = cls;
    }
    int topology = prim == VK_PRIM_LINES ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST
                 : prim == VK_PRIM_FAN   ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
                                         : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    if (bound_topology != topology) {
        vkCmdSetPrimitiveTopology(cmd, (VkPrimitiveTopology)topology);
        bound_topology = topology;
    }
    if (bound_depth_test != (int)vs.depth_test) {
        vkCmdSetDepthTestEnable(cmd, vs.depth_test);
        bound_depth_test = vs.depth_test;
    }
    if (bound_depth_write != (int)vs.depth_write) {
        vkCmdSetDepthWriteEnable(cmd, vs.depth_write);
        bound_depth_write = vs.depth_write;
    }
    if (bound_depth_op != (int)vs.depth_op) {
        vkCmdSetDepthCompareOp(cmd, vs.depth_op);
        bound_depth_op = vs.depth_op;
    }
    if (bound_view[0] != vs.view_x || bound_view[1] != vs.view_y || bound_view[2] != vs.view_w ||
        bound_view[3] != vs.view_h) {
        VkViewport vp = { vs.view_x, vs.view_y, vs.view_w, vs.view_h, 0.0f, 1.0f };
        vkCmdSetViewport(cmd, 0, 1, &vp);
        // OpenGL clips to the viewport; the scissor does that for lines and points outside it.
        float x0 = vs.view_x < 0 ? 0 : vs.view_x, y0 = vs.view_y < 0 ? 0 : vs.view_y;
        float x1 = vs.view_x + vs.view_w, y1 = vs.view_y + vs.view_h;
        if (x1 > (float)vkc.target_extent.width) x1 = (float)vkc.target_extent.width;
        if (y1 > (float)vkc.target_extent.height) y1 = (float)vkc.target_extent.height;
        VkRect2D sc = { { (int32_t)x0, (int32_t)y0 },
                        { x1 > x0 ? (uint32_t)(x1 - x0) : 0u, y1 > y0 ? (uint32_t)(y1 - y0) : 0u } };
        vkCmdSetScissor(cmd, 0, 1, &sc);
        bound_view[0] = vs.view_x;
        bound_view[1] = vs.view_y;
        bound_view[2] = vs.view_w;
        bound_view[3] = vs.view_h;
    }

    vk_push pc = {};
    pc.scale[0] = 2.0f / vs.clip_w;
    pc.scale[1] = 2.0f / vs.clip_h;
    pc.offset[0] = -1.0f - 2.0f * vs.clip_x / vs.clip_w;
    pc.offset[1] = -1.0f - 2.0f * vs.clip_y / vs.clip_h;
    // Texturing: stage 0 when the mode has a texture source (and a texture is bound: an unbound or deleted texture
    // object leaves the stage as if disabled), stage 1 for two.
    int t0 = vs.texture_source != 0 && prim != VK_PRIM_LINES ? vk_tcache_bound(0) : -1;
    if (t0 >= 0) {
        pc.flags |= VK_FLAG_TEXTURE0;
        pc.tex0 = (uint32_t)t0;
        pc.samplers |= vk_tcache_clamps(0) ? 1u : 0u;
        int t1 = vs.texture_source >= 4 ? vk_tcache_bound(1) : -1;
        if (t1 >= 0) {
            pc.flags |= VK_FLAG_TEXTURE1;
            pc.tex1 = (uint32_t)t1;
            pc.samplers |= vk_tcache_clamps(1) ? 2u : 0u;
        }
    }
    if (vs.fog) {
        pc.flags |= VK_FLAG_FOG;
        pc.fog_color[0] = vs.fog_color[0];
        pc.fog_color[1] = vs.fog_color[1];
        pc.fog_color[2] = vs.fog_color[2];
        pc.fog_color[3] = 1.0f;
        pc.fog_start = vs.fog_start;
        pc.fog_end = vs.fog_end;
    }
    vkCmdPushConstants(cmd, vk_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                       sizeof pc, &pc);
    vkCmdBindVertexBuffers(cmd, 0, 1, &f->vertices.buffer, &vat);
    if (idx) {
        vkCmdBindIndexBuffer(cmd, f->indices.buffer, iat, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(cmd, (uint32_t)nidx, 1, 0, 0, 0);
    } else {
        vkCmdDraw(cmd, (uint32_t)nv, 1, 0, 0);
    }
}

// ---- state ----
gr::Mode *vk_gr_opengl_get_current_mode()
{
    return &gr_opengl_current_mode;
}

// gr_opengl_set_mode: the same switches, each value it does not list keeping the previous state.
void vk_gr_opengl_set_mode(gr::Mode mode)
{
    gr::Mode *cur = vk_gr_opengl_get_current_mode();
    if (mode == *cur)
        return;
    *cur = mode;
    int texture_source = mode.get_texture_source();
    int color_source = mode.get_color_source();
    int alpha_source = mode.get_alpha_source();
    int alpha_blend = mode.get_alpha_blend();
    int zbuffer_type = mode.get_zbuffer_type();
    int fog_type = mode.get_fog_type();

    vs.texture_source = texture_source;
    switch (texture_source) {
    case 0:
        vk_gr_opengl_tcache_set(-1, NULL, NULL, 0);
        break;
    case 1:
    case 4:
        vk_tcache_set_clamp(0, false);   // glTexParameteri on the texture bound to stage 0
        break;
    case 2:
    case 3:
        vk_tcache_set_clamp(0, true);
        break;
    }

    switch (alpha_blend) {
    case 0:
        vs.blend_pair = 0;
        break;
    case 1:
        vs.blend_pair = gl_blend_add_one_one ? 1 : 2;
        break;
    case 2:
        vs.blend_pair = 2;
        break;
    case 3:
        vs.blend_pair = 3;
        break;
    case 4:
        vs.blend_pair = 4;
        break;
    case 5:
        vs.blend_pair = 5;
        break;
    }

    switch (color_source) {
    case 0:
    case 2:
        gr_opengl_vertex_color = false;
        break;
    case 1:
        gr_opengl_vertex_color = true;
        break;
    }

    switch (alpha_source) {
    case 0:
    case 3:
        gr_opengl_vertex_alpha = false;
        break;
    case 1:
    case 2:
        gr_opengl_vertex_alpha = true;
        break;
    }

    switch (zbuffer_type) {
    case 0:
        vs.depth_test = false;
        vs.depth_write = false;
        break;
    case 1:
        vs.depth_test = true;
        vs.depth_op = VK_COMPARE_OP_LESS_OR_EQUAL;
        vs.depth_write = false;
        break;
    case 2:
        vs.depth_test = true;
        vs.depth_op = VK_COMPARE_OP_EQUAL;
        vs.depth_write = false;
        break;
    case 3:
        // OpenGL writes depth only while the depth test is on, so this mode writes nothing.
        vs.depth_test = false;
        vs.depth_write = true;
        break;
    case 4:
        vs.depth_test = true;
        vs.depth_op = VK_COMPARE_OP_LESS_OR_EQUAL;
        vs.depth_write = true;
        break;
    }

    switch (fog_type) {
    case 0:
    case 1:
    case 2:
        vs.fog = gr_fog_enabled != 0;
        break;
    case 3:
        vs.fog = false;
        break;
    }
}

void vk_gr_opengl_page_in(int bm, int stage)
{
    if (bm > -1)
        vk_gr_opengl_tcache_set(bm, &gr_opengl_texture_u_scale, &gr_opengl_texture_v_scale, stage);
}

void vk_gr_opengl_set_bitmap(gr::Mode mode, int bm, int stage)
{
    vk_gr_opengl_page_in(bm, stage);
    vk_gr_opengl_set_mode(mode);
}

// gr_opengl_set_viewport: glOrtho over the clip rectangle and the viewport on it, which the 64-bit host maps into
// the window (modern_gl_viewport: edges mapped, so adjacent viewports stay seamless). Taken now, as OpenGL does.
void vk_gr_opengl_set_viewport()
{
    int x = gr_screen.offset_x + gr_screen.clip_left, y = gr_screen.offset_y + gr_screen.clip_top;
    int w = gr_screen.clip_width, h = gr_screen.clip_height;
    vs.clip_x = (float)x;
    vs.clip_y = (float)y;
    vs.clip_w = (float)(w > 0 ? w : 1);
    vs.clip_h = (float)(h > 0 ? h : 1);
    float s, ox, oy;
    vk_game_to_window(&s, &ox, &oy);
    // OpenGL's viewport is bottom-up (y = max_h - h - y); the host maps it from the bottom. The same edges from the
    // top: the window's height less the bottom-up mapping.
    int gl_y = gr_screen.max_h - h - y;
    float x0 = ox + floorf(x * s + 0.5f), x1 = ox + floorf((x + w) * s + 0.5f);
    float by0 = oy + floorf(gl_y * s + 0.5f), by1 = oy + floorf((gl_y + h) * s + 0.5f);
    float ch = vkc.target_extent.height ? (float)vkc.target_extent.height : (float)gr_screen.max_h;
    if (s == 1.0f && ox == 0.0f && oy == 0.0f)
        ch = (float)gr_screen.max_h;   // no host map: the window is the game's size
    vs.view_x = x0;
    vs.view_w = x1 - x0;
    vs.view_y = ch - by1;
    vs.view_h = by1 - by0;
}

void vk_gr_opengl_set_fog(int set_range)
{
    vk_gr_opengl_get_current_mode()->invalidate();
    if (!gr_fog_enabled) {
        vs.fog = false;
        return;
    }
    vs.fog = true;
    if (set_range) {
        vector3 p(0.0f, 0.0f, gr_scale_by_far_clip(gr_fog_near));
        vs.fog_start = gr_vertex_depth((gr_vertex *)&p);
        p.z = gr_scale_by_far_clip(gr_fog_far) * 1.01f;
        vs.fog_end = gr_vertex_depth((gr_vertex *)&p);
    }
    vs.fog_color[0] = gr_fog_color.r * (1.0f / 255.0f);
    vs.fog_color[1] = gr_fog_color.g * (1.0f / 255.0f);
    vs.fog_color[2] = gr_fog_color.b * (1.0f / 255.0f);
}

void vk_gr_opengl_apply_tint(ubyte *rgb)
{
    gr_opengl_apply_tint_gl(rgb);
}

void vk_gr_opengl_set_color()
{
}

void vk_gr_opengl_set_antialias(bool on)
{
}

// ---- drawing ----
static void vk_set_vertex(vk_vertex *o, float x, float y, float z, const ubyte *c, const float *uv)
{
    o->x = x;
    o->y = y;
    o->z = z;
    o->color = vk_pack_color(c);
    memcpy(o->uv0, uv, sizeof o->uv0);
    memset(o->uv1, 0, sizeof o->uv1);
}

// gr_opengl_clear: a black rectangle over the clip rectangle, blending off.
void vk_gr_opengl_clear()
{
    vk_gr_opengl_set_mode(gr::Mode(0, 0, 0, 0, 0, 0));
    int pair = vs.blend_pair;
    vs.blend_pair = 0;   // ONE / ZERO is blending off
    float x0 = (float)(gr_screen.clip_left + gr_screen.offset_x), y0 = (float)(gr_screen.clip_top + gr_screen.offset_y);
    float x1 = (float)(gr_screen.clip_right + gr_screen.offset_x + 1);
    float y1 = (float)(gr_screen.clip_bottom + gr_screen.offset_y + 1);
    static const ubyte black[4] = { 0, 0, 0, 255 };
    static const float uv[4] = { 0, 0, 0, 1 };
    vk_vertex q[4];
    // glRecti: z 0.
    vk_set_vertex(&q[0], x0, y0, 0.0f, black, uv);
    vk_set_vertex(&q[1], x1, y0, 0.0f, black, uv);
    vk_set_vertex(&q[2], x1, y1, 0.0f, black, uv);
    vk_set_vertex(&q[3], x0, y1, 0.0f, black, uv);
    int ts = vs.texture_source;
    vs.texture_source = 0;
    vk_draw(VK_PRIM_FAN, q, 4, NULL, 0);
    vs.texture_source = ts;
    vs.blend_pair = pair;
}

// gr_opengl_zbuffer_clear: glClear(GL_DEPTH_BUFFER_BIT), the whole buffer.
void vk_gr_opengl_zbuffer_clear()
{
    vk_gr_opengl_set_mode(gr::Mode(0, 0, 0, 0, 4, 0));
    VkCommandBuffer cmd = vk_cmd();
    if (!cmd)
        return;
    VkClearAttachment ca = {};
    ca.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    ca.clearValue.depthStencil.depth = 1.0f;
    VkClearRect r = {};
    r.rect.extent = vkc.target_extent;
    r.layerCount = 1;
    vkCmdClearAttachments(cmd, 1, &ca, 1, &r);
}

void vk_gr_opengl_bitmap(int bm, int x, int y, int w, int h, int sx, int sy, int sw, int sh, gr::Mode mode)
{
    ubyte r, g, b, a;
    int bw, bh;

    if (w < 1 || h < 1)
        return;
    vk_gr_opengl_set_bitmap(mode, bm, 0);
    if (gr_opengl_vertex_color) {
        r = 255;
        g = 255;
        b = 255;
    } else {
        r = gr_color.r;
        g = gr_color.g;
        b = gr_color.b;
    }
    if (gr_opengl_vertex_alpha)
        a = 255;
    else
        a = gr_color.a;
    bm_get_dimensions(bm, &bw, &bh);
    float u0 = (float)sx * gr_opengl_texture_u_scale / (float)bw;
    float v0 = (float)sy * gr_opengl_texture_v_scale / (float)bh;
    float u1 = (float)(sx + sw) * gr_opengl_texture_u_scale / (float)bw;
    float v1 = (float)(sy + sh) * gr_opengl_texture_v_scale / (float)bh;
    float x0 = (float)(gr_screen.offset_x + x);
    float y0 = (float)(y + gr_screen.offset_y);
    float x1 = (float)(gr_screen.offset_x + x + w);
    float y1 = (float)(y + gr_screen.offset_y + h);
    ubyte c[4][4];
    for (int i = 0; i < 4; i++) {
        c[i][0] = r;
        c[i][1] = g;
        c[i][2] = b;
        c[i][3] = a;
        if (gr_tint_enabled)
            vk_gr_opengl_apply_tint(c[i]);
    }
    float tc[4][4] = { { u0, v0, 0.0f, 1.0f }, { u1, v0, 0.0f, 1.0f }, { u1, v1, 0.0f, 1.0f }, { u0, v1, 0.0f, 1.0f } };
    vk_vertex q[4];
    vk_set_vertex(&q[0], x0, y0, -1.0f, c[0], tc[0]);
    vk_set_vertex(&q[1], x1, y0, -1.0f, c[1], tc[1]);
    vk_set_vertex(&q[2], x1, y1, -1.0f, c[2], tc[2]);
    vk_set_vertex(&q[3], x0, y1, -1.0f, c[3], tc[3]);
    vk_gr_opengl_set_bitmap(mode, bm, 0);
    vk_draw(VK_PRIM_FAN, q, 4, NULL, 0);
}

static void vk_line(float x1, float y1, float z1, float x2, float y2, float z2)
{
    ubyte c[4];
    c[0] = gr_color.r;
    c[1] = gr_color.g;
    c[2] = gr_color.b;
    c[3] = gr_color.a;
    if (gr_tint_enabled)
        vk_gr_opengl_apply_tint(c);
    static const float uv[4] = { 0, 0, 0, 1 };
    vk_vertex l[2];
    vk_set_vertex(&l[0], x1, y1, z1, c, uv);
    vk_set_vertex(&l[1], x2, y2, z2, c, uv);
    vk_draw(VK_PRIM_LINES, l, 2, NULL, 0);
}

void vk_gr_opengl_line(float x1, float y1, float x2, float y2, gr::Mode mode)
{
    vk_gr_opengl_set_bitmap(mode, -1, 0);
    // glVertex2i: integer pixels (pixel corners), z 0; passed as they are, so both APIs rasterise the same input.
    vk_line((float)(int)(gr_screen.offset_x + x1), (float)(int)(gr_screen.offset_y + y1), 0.0f,
            (float)(int)(gr_screen.offset_x + x2), (float)(int)(gr_screen.offset_y + y2), 0.0f);
}

void vk_gr_opengl_line_3d(gr_vertex *v1, gr_vertex *v2, gr::Mode mode)
{
    vk_gr_opengl_set_bitmap(mode, -1, 0);
    vk_line(v1->sx, v1->sy, gr_vertex_depth(v1), v2->sx, v2->sy, gr_vertex_depth(v2));
}

// gr_opengl_tmapper: one polygon of up to 30 projected vertices, a triangle fan (GL_POLYGON).
void vk_gr_opengl_tmapper(int nv, gr_vertex **verts, gr::Mode mode)
{
    vk_vertex out[30];
    if (nv > 30)
        nv = 30;
    vk_gr_opengl_set_bitmap(mode, gr_texture, 0);
    for (int i = 0; i < nv; i++) {
        gr_vertex *v = verts[i];
        vk_vertex *o = &out[i];
        ubyte c[4];
        if (gr_opengl_vertex_alpha)
            c[3] = 255;
        else if (v->_unk1a & 8)
            c[3] = v->a;
        else
            c[3] = gr_color.a;
        if (gr_opengl_vertex_color) {
            c[0] = 255;
            c[1] = 255;
            c[2] = 255;
        } else if (v->_unk1a & 4) {
            c[0] = (ubyte)gr_color_gamma_lut[v->r];
            c[1] = (ubyte)gr_color_gamma_lut[v->g];
            c[2] = (ubyte)gr_color_gamma_lut[v->b];
        } else {
            c[0] = gr_color.r;
            c[1] = gr_color.g;
            c[2] = gr_color.b;
        }
        if (gr_tint_enabled)
            vk_gr_opengl_apply_tint(c);
        float t[4] = { 0.0f, 0.0f, 0.0f, 1.0f };   // OpenGL keeps a stale coordinate without flag 1; 0 here
        if (v->_unk1a & 1) {
            t[3] = v->sw;
            t[0] = t[3] * v->u * gr_opengl_texture_u_scale;
            t[1] = t[3] * (gr_opengl_texture_v_scale * v->v);
        }
        vk_set_vertex(o, v->sx, v->sy, gr_vertex_depth(v), c, t);
    }
    vk_gr_opengl_set_bitmap(mode, gr_texture, 0);
    vk_draw(VK_PRIM_FAN, out, nv, NULL, 0);
}

// gr_opengl_read_backbuffer: the rectangle as R, G, B, A bytes, top row first, alpha 255; the 64-bit host's
// mapping as modern_gl_read_pixels (the scaled rectangle box-filtered down to the size asked).
int vk_gr_opengl_read_backbuffer(int x, int y, int w, int h, ubyte *buf)
{
    if (w <= 0 || h <= 0)
        return BM_FORMAT_8888;
    float s, ox, oy;
    vk_game_to_window(&s, &ox, &oy);
    int x0 = (int)(ox + floorf(x * s + 0.5f)), x1 = (int)(ox + floorf((x + w) * s + 0.5f));
    int y0 = (int)(oy + floorf(y * s + 0.5f)), y1 = (int)(oy + floorf((y + h) * s + 0.5f));
    int fw = x1 - x0, fh = y1 - y0;
    if (fw <= 0 || fh <= 0) {
        memset(buf, 0, (size_t)w * h * 4);
        return BM_FORMAT_8888;
    }
    ubyte *src = new ubyte[(size_t)fw * fh * 4];
    vk_read_window(x0, y0, fw, fh, src);   // B, G, R, A (the swapchain's order)
    for (int dy = 0; dy < h; dy++) {
        int sy0 = dy * fh / h, sy1 = (dy + 1) * fh / h;
        if (sy1 <= sy0)
            sy1 = sy0 + 1;
        for (int dx = 0; dx < w; dx++) {
            int sx0 = dx * fw / w, sx1 = (dx + 1) * fw / w;
            if (sx1 <= sx0)
                sx1 = sx0 + 1;
            unsigned sum[3] = { 0, 0, 0 }, n = 0;
            for (int sy = sy0; sy < sy1 && sy < fh; sy++)
                for (int sx = sx0; sx < sx1 && sx < fw; sx++, n++)
                    for (int c = 0; c < 3; c++)
                        sum[c] += src[((size_t)sy * fw + sx) * 4 + c];
            ubyte *d = &buf[((size_t)dy * w + dx) * 4];
            d[0] = (ubyte)(n ? sum[2] / n : 0);
            d[1] = (ubyte)(n ? sum[1] / n : 0);
            d[2] = (ubyte)(n ? sum[0] / n : 0);
            d[3] = 0xff;
        }
    }
    delete[] src;
    return BM_FORMAT_8888;
}
#endif
