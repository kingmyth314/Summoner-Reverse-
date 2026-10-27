// gr.frag - the Vulkan back end's fragment shader (rung L4, docs/future/vulkan-renderer.md). Not original source.
// The OpenGL back end's fixed-function state as flags: texture stage 0 modulated by the vertex colour. Stage 1,
// alpha test and fog follow in M2 / M3.
#version 450

layout(push_constant) uniform PushConstants {
    vec2 scale;
    vec2 offset;
    uint flags;
} pc;

const uint FLAG_TEXTURE0 = 1u;

layout(set = 0, binding = 0) uniform sampler2D tex0;

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec4 v_uv0;
layout(location = 2) in vec4 v_uv1;

layout(location = 0) out vec4 out_color;

void main()
{
    vec4 c = v_color;
    if ((pc.flags & FLAG_TEXTURE0) != 0u)
        c *= textureProj(tex0, v_uv0.xyw);
    out_color = c;
}
