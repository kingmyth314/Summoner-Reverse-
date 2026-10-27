// fsr_rcas.frag - [Display] RenderScale below 1, second pass (modern/vk_post.cpp; rung L4). Not original source.
// AMD FidelityFX Super Resolution 1's RCAS (robust contrast-adaptive sharpening, fsr/ffx_fsr1.h, MIT licence in
// fsr/license.txt): EASU's window-sized image sharpened into the swapchain image. Sharpness is in stops, 0 the
// strongest ([Display] FsrSharpness).
#version 450
#extension GL_GOOGLE_include_directive : require

layout(push_constant) uniform PushConstants {
    vec2 in_size;    // the intermediate image (the window's size)
    vec2 out_size;
    uint slot;       // the intermediate image in images[]
    float sharpness;
} pc;

layout(set = 0, binding = 0) uniform texture2D images[4096];
layout(set = 0, binding = 1) uniform sampler samplers[3];

layout(location = 0) out vec4 out_color;

#define A_GPU 1
#define A_GLSL 1
#include "fsr/ffx_a.h"
#define FSR_RCAS_F 1
// RCAS reads the four neighbours; at the border they are clamped into the image.
AF4 FsrRcasLoadF(ASU2 p)
{
    p = clamp(p, ASU2(0, 0), ASU2(pc.in_size) - ASU2(1, 1));
    return texelFetch(sampler2D(images[pc.slot], samplers[0]), p, 0);
}
void FsrRcasInputF(inout AF1 r, inout AF1 g, inout AF1 b) {}
#include "fsr/ffx_fsr1.h"

void main()
{
    AU4 con;
    FsrRcasCon(con, pc.sharpness);
    AF3 c;
    FsrRcasF(c.r, c.g, c.b, AU2(gl_FragCoord.xy), con);
    out_color = vec4(c, 1.0);
}
