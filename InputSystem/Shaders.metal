
// Complete Shaders.metal

#include <metal_stdlib>
using namespace metal;

// ========================================
// Shared Structures
// ========================================

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

// Simple vertex shader for basic geometry (no normals needed)
vertex VertexOut vs_simple(VertexIn in [[stage_in]],
                           constant Uniforms& uniforms [[buffer(1)]]) {
    VertexOut out;
    out.position = uniforms.modelViewProjection * float4(in.position, 1.0);
    out.color = in.color;
    out.texCoord = in.texCoord;
    out.normal = float3(0, 1, 0); // Default up normal
    out.worldPosition = in.position;
    out.viewPosition = in.position;
    return out;
}

// ========================================
// Fragment Shaders - Basic
// ========================================

// Basic unlit fragment shader - just vertex color
fragment float4 fs_main(VertexOut in [[stage_in]]) {
    return float4(in.color, 1.0);
}

// Colored fragment shader (alias for fs_main for RenderSystem compatibility)
fragment float4 fs_colored(VertexOut in [[stage_in]]) {
    return float4(in.color, 1.0);
}

// ========================================
// Fragment Shaders - Lighting
// ========================================

// Main lit fragment shader - simple directional lighting
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

// Enhanced lighting with specular highlights
fragment float4 fs_phong(VertexOut in [[stage_in]]) {
    // Light parameters
    float3 lightDirection = normalize(float3(0.5, -0.7, -0.5));
    float3 lightColor = float3(1.0, 1.0, 1.0);
    float3 ambientColor = float3(0.15, 0.15, 0.15);

    // Material parameters
    float shininess = 32.0;
    float3 specularColor = float3(0.3, 0.3, 0.3);

    // Normalize vectors
    float3 normal = normalize(in.normal);
    float3 viewDir = normalize(-in.viewPosition);

    // Ambient
    float3 ambient = ambientColor * in.color;

    // Diffuse
    float diffuseStrength = max(dot(normal, -lightDirection), 0.0);
    float3 diffuse = diffuseStrength * lightColor * in.color;

    // Specular (Phong)
    float3 reflectDir = reflect(lightDirection, normal);
    float specularStrength = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    float3 specular = specularStrength * specularColor * lightColor;

    float3 finalColor = ambient + diffuse + specular;
    return float4(finalColor, 1.0);
}

// ========================================
// Fragment Shaders - Textured
// ========================================

// Basic textured fragment shader
fragment float4 fs_textured(VertexOut in [[stage_in]],
                            texture2d<float> colorTexture [[texture(0)]],
                            sampler colorSampler [[sampler(0)]]) {
    return colorTexture.sample(colorSampler, in.texCoord);
}

// Textured with lighting
fragment float4 fs_textured_lit(VertexOut in [[stage_in]],
                                texture2d<float> colorTexture [[texture(0)]],
                                sampler colorSampler [[sampler(0)]]) {
    // Sample texture
    float4 texColor = colorTexture.sample(colorSampler, in.texCoord);

    // Light parameters
    float3 lightDirection = normalize(float3(0.5, -0.7, -0.5));
    float3 lightColor = float3(1.0, 1.0, 1.0);
    float3 ambientColor = float3(0.2, 0.2, 0.2);

    // Calculate lighting
    float3 normal = normalize(in.normal);
    float diffuseStrength = max(dot(normal, -lightDirection), 0.0);
    float3 diffuse = diffuseStrength * lightColor;

    // Combine
    float3 finalColor = texColor.rgb * (ambientColor + diffuse);
    return float4(finalColor, texColor.a);
}

// ========================================
// Fragment Shaders - Special Effects
// ========================================

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

// Fresnel/rim lighting effect
fragment float4 fs_fresnel(VertexOut in [[stage_in]]) {
    float3 viewDir = normalize(-in.viewPosition);
    float3 normal = normalize(in.normal);

    // Fresnel-like effect
    float fresnel = 1.0 - max(dot(viewDir, normal), 0.0);
    fresnel = pow(fresnel, 2.0);

    // Base color with fresnel rim lighting
    float3 baseColor = in.color;
    float3 rimColor = float3(0.5, 0.8, 1.0);
    float3 finalColor = mix(baseColor, rimColor, fresnel * 0.6);

    return float4(finalColor, 1.0);
}

// Normal visualization (for debugging)
fragment float4 fs_normal_debug(VertexOut in [[stage_in]]) {
    float3 normal = normalize(in.normal);
    // Convert from [-1,1] to [0,1] range for color display
    return float4(normal * 0.5 + 0.5, 1.0);
}

// Wireframe shader
fragment float4 fs_wireframe(VertexOut in [[stage_in]]) {
    return float4(1.0, 1.0, 1.0, 1.0); // White wireframe
}

// UV coordinate visualization
fragment float4 fs_uv_debug(VertexOut in [[stage_in]]) {
    return float4(in.texCoord.x, in.texCoord.y, 0.0, 1.0);
}
    
// World position visualization
fragment float4 fs_world_pos_debug(VertexOut in [[stage_in]]) {
    float3 pos = in.worldPosition;
    // Normalize world position to [0,1] range (assuming objects are within [-10,10] bounds)
    float3 normalizedPos = (pos + 10.0) / 20.0;
    return float4(normalizedPos, 1.0);
}

// ========================================
// Fragment Shaders - Animated Effects
// ========================================

// Time-based color cycling (requires time uniform)
struct TimeUniforms {
    float time;
    float deltaTime;
    uint frameCount;
};

fragment float4 fs_animated_color(VertexOut in [[stage_in]],
                                  constant TimeUniforms& timeUniforms [[buffer(2)]]) {
    float time = timeUniforms.time;

    // Create animated color based on time and position
    float3 animatedColor;
    animatedColor.r = (sin(time + in.worldPosition.x) + 1.0) * 0.5;
    animatedColor.g = (cos(time + in.worldPosition.y) + 1.0) * 0.5;
    animatedColor.b = (sin(time * 0.7 + in.worldPosition.z) + 1.0) * 0.5;

    // Mix with vertex color
    float3 finalColor = mix(in.color, animatedColor, 0.7);

    return float4(finalColor, 1.0);
}

// Pulsing effect
fragment float4 fs_pulse(VertexOut in [[stage_in]],
                         constant TimeUniforms& timeUniforms [[buffer(2)]]) {
    float pulse = (sin(timeUniforms.time * 3.0) + 1.0) * 0.5;
    float3 finalColor = in.color * (0.5 + pulse * 0.5);
    return float4(finalColor, 1.0);
}
