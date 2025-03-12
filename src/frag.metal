#include <metal_stdlib>
using namespace metal;

// The fragment shader outputs a solid red color.
fragment float4 fragmentShader() {
    return float4(1.0, 0.0, 0.0, 1.0);
}
