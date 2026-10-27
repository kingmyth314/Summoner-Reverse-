// downscale.frag - [Display] RenderScale above 1 (modern/vk_post.cpp; rung L4). Not original source.
// The scene image scaled down into the swapchain image: each window pixel is the mean of an n x n grid of filtered
// samples over its footprint in the scene image (n = the scale rounded up, at most 2), a box filter: supersampling.
#version 450

layout(push_constant) uniform PushConstants {
    vec2 in_size;    // the scene image
    vec2 out_size;   // the window
    uint slot;       // the scene image in images[]
    float sharpness; // unused
} pc;

layout(set = 0, binding = 0) uniform texture2D images[4096];
layout(set = 0, binding = 1) uniform sampler samplers[3];

layout(location = 0) out vec4 out_color;

void main()
{
    int n = int(ceil(pc.in_size.x / pc.out_size.x - 0.001));
    n = clamp(n, 1, 2);
    vec2 p = floor(gl_FragCoord.xy);
    vec3 sum = vec3(0.0);
    for (int j = 0; j < n; j++)
        for (int i = 0; i < n; i++) {
            vec2 uv = (p + (vec2(i, j) + 0.5) / float(n)) / pc.out_size;
            sum += texture(sampler2D(images[pc.slot], samplers[2]), uv).rgb;
        }
    out_color = vec4(sum / float(n * n), 1.0);
}
