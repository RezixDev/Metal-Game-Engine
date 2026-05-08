// Shaders.metal
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 color    [[attribute(1)]];
    float3 normal   [[attribute(2)]];
    float2 texCoord [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 color;
    float3 normal;
    float2 texCoord;
    float3 worldPosition;
};

struct Uniforms {
    float4x4 modelViewProjection;
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

vertex VertexOut vs_main(VertexIn in [[stage_in]],
                         constant Uniforms& uniforms [[buffer(1)]]) {
    VertexOut out;
    out.position      = uniforms.modelViewProjection * float4(in.position, 1.0);
    float4 worldPos   = uniforms.model               * float4(in.position, 1.0);
    out.worldPosition = worldPos.xyz;
    out.normal        = normalize((uniforms.model * float4(in.normal, 0.0)).xyz);
    out.color         = in.color;
    out.texCoord      = in.texCoord;
    return out;
}

fragment float4 fs_main_lit(VertexOut in [[stage_in]]) {
    constexpr float3 lightDirection = float3(0.5, -0.7, -0.5);
    constexpr float3 lightColor     = float3(1.0,  1.0,  1.0);
    constexpr float3 ambient        = float3(0.2,  0.2,  0.2);

    float3 normal      = normalize(in.normal);
    float  diffuse     = max(dot(normal, -normalize(lightDirection)), 0.0);
    float3 finalColor  = in.color * (ambient + diffuse * lightColor);
    return float4(finalColor, 1.0);
}
