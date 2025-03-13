#include <metal_stdlib>
using namespace metal;

struct VertexInput {
    float3 position [[attribute(0)]];
    float4 color    [[attribute(1)]];
};

// Define a vertex structure that matches your vertex buffer layout.
// The vertex shader reads the bound vertex buffer and outputs a clip-space position.
vertex float4 vertexShader(uint vertexId [[vertex_id]],
                           VertexInput input [[stage_in]]) {
    // Convert the 3D position to a 4D homogeneous coordinate.
    return float4(input.position, 1.0);
}
