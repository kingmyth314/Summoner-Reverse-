// fsr_easu.frag - [Display] RenderScale below 1, first pass (modern/vk_post.cpp; rung L4). Not original source.
// AMD FidelityFX Super Resolution 1's EASU (edge-adaptive spatial upsampling, fsr/ffx_fsr1.h, MIT licence in
// fsr/license.txt): the scene image upscaled to the window's size, into the intermediate image that RCAS sharpens.
#version 450
#extension GL_GOOGLE_include_directive : require

layout(push_constant) uniform PushConstants {
    vec2 in_size;    // the scene image
    vec2 out_size;   // the window
    uint slot;       // the scene image in images[]
    float sharpness;
} pc;

layout(set = 0, binding = 0) uniform texture2D images[4096];
layout(set = 0, binding = 1) uniform sampler samplers[3];

layout(location = 0) out vec4 out_color;

#define A_GPU 1
#define A_GLSL 1
#include "fsr/ffx_a.h"
#define FSR_EASU_F 1
// Sampler 2 clamps to the edge, so the gathers at the image's border repeat its last texels.
AF4 FsrEasuRF(AF2 p) { return textureGather(sampler2D(images[pc.slot], samplers[2]), p, 0); }
AF4 FsrEasuGF(AF2 p) { return textureGather(sampler2D(images[pc.slot], samplers[2]), p, 1); }
AF4 FsrEasuBF(AF2 p) { return textureGather(sampler2D(images[pc.slot], samplers[2]), p, 2); }
#include "fsr/ffx_fsr1.h"

void main()
{
    AU4 con0, con1, con2, con3;
    FsrEasuCon(con0, con1, con2, con3, pc.in_size.x, pc.in_size.y, pc.in_size.x, pc.in_size.y, pc.out_size.x,
               pc.out_size.y);
    AF3 c;
    FsrEasuF(c, AU2(gl_FragCoord.xy), con0, con1, con2, con3);
    out_color = vec4(c, 1.0);
}
