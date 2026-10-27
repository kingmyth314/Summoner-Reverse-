// post.frag - the Vulkan back end's end-of-frame passes (modern/vk_post.cpp; rung L4). Not original source.
// Mode 0, brightness: the frame's copy through pow(c, 1 / gamma), the curve gr_set_gamma builds (as the OpenGL host's
// shader pass, modern/gamma.cpp). Mode 1, the fast-mode badge: its image copied as it is, its top-left at origin.
// Window pixels map one to one, so the images are read with texelFetch.
#version 450

layout(push_constant) uniform PushConstants {
    vec2 origin;     // window pixel of the image's (0, 0)
    float e;         // 1 / gamma
    uint slot;       // in images[]
    uint mode;
} pc;

layout(set = 0, binding = 0) uniform texture2D images[4096];
layout(set = 0, binding = 1) uniform sampler samplers[2];

layout(location = 0) out vec4 out_color;

void main()
{
    ivec2 p = ivec2(gl_FragCoord.xy - pc.origin);
    vec3 c = texelFetch(sampler2D(images[pc.slot], samplers[0]), p, 0).rgb;
    out_color = vec4(pc.mode == 0u ? pow(c, vec3(pc.e)) : c, 1.0);
}
