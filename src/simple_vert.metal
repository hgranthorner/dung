#include <metal_stdlib>
using namespace metal;

// The vertex shader reads the bound vertex buffer and outputs a clip-space position.
vertex float4 vertexShader(uint vertexId [[vertex_id]]) {
    if (vertexId == 0) {
        return float4(-0.5, -0.5, 0, 1.0);
    }
    if (vertexId == 2) {
        return float4(0.0, 0.5, 0, 1.0);
    }
    if (vertexId == 1) {
        return float4(0.5, -0.5, 0, 1.0);
    }
}
