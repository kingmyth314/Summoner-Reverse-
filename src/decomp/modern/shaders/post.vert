// post.vert - the Vulkan back end's end-of-frame passes (modern/vk_post.cpp; rung L4). Not original source.
// One triangle over the whole viewport; the scissor limits it (the badge's box).
#version 450

void main()
{
    vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
