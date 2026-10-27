// gr.vert - the Vulkan back end's vertex shader (rung L4, docs/future/vulkan-renderer.md). Not original source.
// The game projects on the CPU and sends screen-space vertices, as to the OpenGL back end (glOrtho over the clip
// rectangle, near -1, far 1): x, y in game pixels, z as the back end passes it (nearer is larger), and projective
// texture coordinates (s * w, t * w, 0, w) for perspective-correct texturing. gl_Position.w stays 1, so colours
// interpolate linearly in screen space as under OpenGL.
#version 450

layout(push_constant) uniform PushConstants {
    vec2 scale;      // game pixels -> NDC over the clip rectangle's viewport (gr_opengl_set_viewport's glOrtho)
    vec2 offset;
    uint flags;      // gr.frag
    uint tex0;
    uint tex1;
    uint samplers;
    vec4 fog_color;
    float fog_start;
    float fog_end;
} pc;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec4 in_uv0;
layout(location = 3) in vec4 in_uv1;

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec4 v_uv0;
layout(location = 2) out vec4 v_uv1;
layout(location = 3) out float v_fog_z;

void main()
{
    // glOrtho's z with near -1 / far 1 is -z, and the window depth (1 - z) / 2. Vulkan's NDC y points down, as the
    // game's pixels do, so y needs no flip.
    gl_Position = vec4(in_pos.xy * pc.scale + pc.offset, (1.0 - in_pos.z) * 0.5, 1.0);
    v_color = in_color;
    v_uv0 = in_uv0;
    v_uv1 = in_uv1;
    v_fog_z = abs(in_pos.z);   // OpenGL's fog coordinate: the eye distance, with the identity modelview |z|
}
