// gr.frag - the Vulkan back end's fragment shader (rung L4, docs/future/vulkan-renderer.md). Not original source.
// The OpenGL back end's fixed-function state (gr_opengl_set_mode) as flags:
//   one texture: stage 0 GL_MODULATE;
//   two textures (texture source 4+): stage 0 GL_DECAL, stage 1 GL_MODULATE (the lightmap);
//   the alpha test GL_GREATER 0 (always on);
//   linear fog on the eye distance (GL_FOG, glFogi GL_LINEAR).
#version 450

layout(push_constant) uniform PushConstants {
    vec2 scale;
    vec2 offset;
    uint flags;
    uint tex0;       // slots in images[]
    uint tex1;
    uint samplers;   // bit 0: stage 0 clamps, bit 1: stage 1 clamps (else repeat)
    vec4 fog_color;
    float fog_start;
    float fog_end;
} pc;

const uint FLAG_TEXTURE0 = 1u;
const uint FLAG_TEXTURE1 = 2u;
const uint FLAG_FOG = 4u;

layout(set = 0, binding = 0) uniform texture2D images[4096];
layout(set = 0, binding = 1) uniform sampler samplers[2];   // 0 repeat, 1 clamp to a transparent black border (GL_CLAMP)

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec4 v_uv0;
layout(location = 2) in vec4 v_uv1;
layout(location = 3) in float v_fog_z;

layout(location = 0) out vec4 out_color;

void main()
{
    vec4 c = v_color;
    if ((pc.flags & FLAG_TEXTURE0) != 0u) {
        vec4 t0 = textureProj(sampler2D(images[pc.tex0], samplers[pc.samplers & 1u]), v_uv0.xyw);
        if ((pc.flags & FLAG_TEXTURE1) != 0u) {
            c.rgb = mix(c.rgb, t0.rgb, t0.a);   // GL_DECAL: the alpha stays the fragment's
            c *= textureProj(sampler2D(images[pc.tex1], samplers[(pc.samplers >> 1) & 1u]), v_uv1.xyw);
        } else {
            c *= t0;
        }
    }
    if (c.a <= 0.0)
        discard;
    if ((pc.flags & FLAG_FOG) != 0u) {
        float f = clamp((pc.fog_end - v_fog_z) / (pc.fog_end - pc.fog_start), 0.0, 1.0);
        c.rgb = mix(pc.fog_color.rgb, c.rgb, f);
    }
    out_color = c;
}
