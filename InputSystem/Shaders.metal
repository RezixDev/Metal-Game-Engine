//
// Updated Shaders.metal
//
#include <metal_stdlib>
using namespace metal;

// Vertex input structure - matches the Vertex struct in GeometryBuilder.hpp
struct VertexIn {
    float3 position [[attribute(0)]];
    float3 color    [[attribute(1)]];
    float3 normal   [[attribute(2)]];
    float2 texCoord [[attribute(3)]];
};

// Vertex output / Fragment input
struct VertexOut {
    float4 position [[position]];
    float3 color;
    float3 normal;
    float2 texCoord;
    float3 worldPosition;
    float3 viewPosition;
};

// Uniform buffer
struct Uniforms {
    float4x4 modelViewProjection;
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

// Vertex shader
vertex VertexOut vs_main(VertexIn in [[stage_in]],
                         constant Uniforms& uniforms [[buffer(1)]],
                         uint vertexID [[vertex_id]]) {
    VertexOut out;
    
    // Transform position to world space
    float4 worldPos = uniforms.model * float4(in.position, 1.0);
    out.worldPosition = worldPos.xyz;

    // Transform to view space
    float4 viewPos = uniforms.view * worldPos;
    out.viewPosition = viewPos.xyz;

    // Transform to clip space
    out.position = uniforms.projection * viewPos;

    // Transform normal to world space (assumes uniform scaling)
    float3 worldNormal = (uniforms.model * float4(in.normal, 0.0)).xyz;
    out.normal = normalize(worldNormal);
    
    // Pass through other attributes
    out.color = in.color;
    out.texCoord = in.texCoord;
    
    return out;
}

// Fragment shader
fragment float4 fs_main(VertexOut in [[stage_in]]) {
    // For now, just use the vertex color
    // You can easily extend this to use:
    // - Textures (using in.texCoord)
    // - Lighting (using in.normal)
    // - Distance-based effects (using in.worldPosition)

    return float4(in.color, 1.0);
}

// Enhanced fragment shader with lighting
fragment float4 fs_main_lit(VertexOut in [[stage_in]]) {
    // Light parameters
    float3 lightDirection = normalize(float3(0.5, -0.7, -0.5));
    float3 lightColor = float3(1.0, 1.0, 1.0);
    float3 ambientColor = float3(0.2, 0.2, 0.2);

    // Normalize the normal
    float3 normal = normalize(in.normal);
    
    // Calculate diffuse lighting
    float diffuseStrength = max(dot(normal, -lightDirection), 0.0);
    float3 diffuse = diffuseStrength * lightColor;
    
    // Combine lighting with vertex color
    float3 finalColor = in.color * (ambientColor + diffuse);

    return float4(finalColor, 1.0);
}

// Distance-based color fragment shader
fragment float4 fs_distance_color(VertexOut in [[stage_in]]) {
    // Calculate distance from camera (view space Z is distance)
    float distance = length(in.viewPosition);

    // Create color based on distance
    float normalizedDistance = distance / 10.0; // Normalize to 0-1 over 10 units
    float3 nearColor = float3(1.0, 0.0, 0.0);   // Red when close
    float3 farColor = float3(0.0, 0.0, 1.0);    // Blue when far

    float3 distanceColor = mix(nearColor, farColor, clamp(normalizedDistance, 0.0, 1.0));

    // Mix with original color
    float3 finalColor = mix(in.color, distanceColor, 0.5);

    return float4(finalColor, 1.0);
}

// Fragment shader with view-dependent effects
fragment float4 fs_view_dependent(VertexOut in [[stage_in]]) {
    // Calculate view direction (from fragment to camera)
    float3 viewDir = normalize(-in.viewPosition);
    float3 normal = normalize(in.normal);

    // Fresnel-like effect
    float fresnel = 1.0 - max(dot(viewDir, normal), 0.0);
    fresnel = pow(fresnel, 2.0);

    // Base color with fresnel rim lighting
    float3 baseColor = in.color;
    float3 rimColor = float3(0.5, 0.8, 1.0);
    float3 finalColor = mix(baseColor, rimColor, fresnel * 0.5);
    
    return float4(finalColor, 1.0);
}