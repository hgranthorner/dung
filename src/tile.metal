#include <metal_stdlib>
using namespace metal;

struct VertexInput {
    float4 position [[attribute(0)]];
    float4 color    [[attribute(1)]];
};

struct FragmentInput {
    float4 position [[position]];
    float4 color;
};

// Define a vertex structure that matches your vertex buffer layout.
// The vertex shader reads the bound vertex buffer and outputs a clip-space position.
vertex FragmentInput vertexShader(
    uint vertexId [[vertex_id]],
    constant float4x4 *mvp [[buffer(0)]],
    VertexInput input [[stage_in]]) {
    // Convert the 3D position to a 4D homogeneous coordinate.
    FragmentInput frag = {};
    frag.position = *mvp * input.position;
    frag.color = input.color;
    return frag;
}
// The fragment shader outputs a solid red color.
fragment float4 fragmentShader(FragmentInput input [[stage_in]]) {
    return input.color;
}
