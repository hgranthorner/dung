#include <metal_stdlib>
using namespace metal;

// struct VertexInput {
//     float4 position
// };

// Define a vertex structure that matches your vertex buffer layout.
// The vertex shader reads the bound vertex buffer and outputs a clip-space position.
vertex float4 vertexShader(uint vertexId [[vertex_id]],
                           float4 vertices [[stage_in]] [[attribute(0)]]) {
    // Convert the 3D position to a 4D homogeneous coordinate.
    return vertices;
}
