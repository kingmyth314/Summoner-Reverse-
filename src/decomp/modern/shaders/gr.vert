// gr.vert - the Vulkan back end's vertex shader (rung L4, docs/future/vulkan-renderer.md). Not original source.
// The game projects on the CPU and sends screen-space vertices, as to the OpenGL back end (glOrtho over the screen):
// x, y in pixels, z the depth, and projective texture coordinates (s * w, t * w, 0, w) for perspective-correct
// texturing. gl_Position.w stays 1, so colours interpolate linearly in screen space as under OpenGL.
#version 450

layout(push_constant) uniform PushConstants {
    vec2 scale;      // 2 / width, 2 / height of the viewport
    vec2 offset;     // -1, -1: pixel (0, 0) is the top-left corner (no flip: Vulkan's origin is the top left)
    uint flags;      // gr.frag
} pc;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec4 in_uv0;
layout(location = 3) in vec4 in_uv1;

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec4 v_uv0;
layout(location = 2) out vec4 v_uv1;

void main()
{
    gl_Position = vec4(in_pos.xy * pc.scale + pc.offset, in_pos.z, 1.0);
    v_color = in_color;
    v_uv0 = in_uv0;
    v_uv1 = in_uv1;
}
