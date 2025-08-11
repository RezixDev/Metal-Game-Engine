#pragma once
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include <simd/simd.h>
#include <vector>

// Vertex structure definition
struct Vertex {
    vector_float3 position;
    vector_float3 color;
    vector_float3 normal;  // Added for future lighting
    vector_float2 texCoord; // Added for future texturing
};

// Mesh data container
struct MeshData {
    NSData* vertexData;
    NSData* indexData;
    NSUInteger indexCount;
    MTLPrimitiveType primitiveType;
};

class GeometryBuilder {
public:
    static MeshData createCube(float size = 1.0f) {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        
        float h = size * 0.5f;
        
        // Define cube corners
        vector_float3 corners[8] = {
            {-h, -h, -h}, { h, -h, -h}, { h,  h, -h}, {-h,  h, -h},  // Front face
            {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h}   // Back face
        };
        
        // Face definitions [vertex indices for each face]
        int faces[6][4] = {
            {0, 1, 2, 3}, // Front (-Z)
            {5, 4, 7, 6}, // Back (+Z)
            {4, 0, 3, 7}, // Left (-X)
            {1, 5, 6, 2}, // Right (+X)
            {3, 2, 6, 7}, // Top (+Y)
            {4, 5, 1, 0}  // Bottom (-Y)
        };
        
        // Face colors
        vector_float3 colors[6] = {
            {1.0f, 0.0f, 0.0f}, // Red
            {0.0f, 1.0f, 0.0f}, // Green
            {0.0f, 0.0f, 1.0f}, // Blue
            {1.0f, 1.0f, 0.0f}, // Yellow
            {1.0f, 0.0f, 1.0f}, // Magenta
            {0.0f, 1.0f, 1.0f}  // Cyan
        };
        
        // Face normals
        vector_float3 normals[6] = {
            { 0,  0, -1}, // Front
            { 0,  0,  1}, // Back
            {-1,  0,  0}, // Left
            { 1,  0,  0}, // Right
            { 0,  1,  0}, // Top
            { 0, -1,  0}  // Bottom
        };
        
        // Build vertices and indices for each face
        for (int f = 0; f < 6; ++f) {
            uint16_t baseIndex = (uint16_t)vertices.size();
            
            // Add 4 vertices for this face
            for (int v = 0; v < 4; ++v) {
                Vertex vertex;
                vertex.position = corners[faces[f][v]];
                vertex.color = colors[f];
                vertex.normal = normals[f];
                
                // Simple UV mapping
                vertex.texCoord = {
                    (v == 1 || v == 2) ? 1.0f : 0.0f,
                    (v == 2 || v == 3) ? 1.0f : 0.0f
                };
                
                vertices.push_back(vertex);
            }
            
            // Add two triangles for this face
            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);
            
            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 3);
        }
        
        // Convert to NSData
        NSData* vertexData = [NSData dataWithBytes:vertices.data()
                                           length:vertices.size() * sizeof(Vertex)];
        NSData* indexData = [NSData dataWithBytes:indices.data()
                                          length:indices.size() * sizeof(uint16_t)];
        
        return {vertexData, indexData, indices.size(), MTLPrimitiveTypeTriangle};
    }
    
    static MeshData createSphere(float radius = 1.0f, int segments = 32, int rings = 16) {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        
        // Generate vertices
        for (int ring = 0; ring <= rings; ++ring) {
            float theta = M_PI * ring / rings;
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);
            
            for (int seg = 0; seg <= segments; ++seg) {
                float phi = 2.0f * M_PI * seg / segments;
                float sinPhi = sinf(phi);
                float cosPhi = cosf(phi);
                
                Vertex vertex;
                vertex.position = {
                    radius * sinTheta * cosPhi,
                    radius * cosTheta,
                    radius * sinTheta * sinPhi
                };
                
                // Normal is just normalized position for a sphere
                vertex.normal = {sinTheta * cosPhi, cosTheta, sinTheta * sinPhi};
                
                // Color based on position (creates gradient effect)
                vertex.color = {
                    (vertex.position.x + radius) / (2.0f * radius),
                    (vertex.position.y + radius) / (2.0f * radius),
                    (vertex.position.z + radius) / (2.0f * radius)
                };
                
                // UV mapping
                vertex.texCoord = {
                    (float)seg / segments,
                    (float)ring / rings
                };
                
                vertices.push_back(vertex);
            }
        }
        
        // Generate indices
        for (int ring = 0; ring < rings; ++ring) {
            for (int seg = 0; seg < segments; ++seg) {
                uint16_t first = ring * (segments + 1) + seg;
                uint16_t second = first + segments + 1;
                
                // First triangle
                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);
                
                // Second triangle
                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }
        
        NSData* vertexData = [NSData dataWithBytes:vertices.data()
                                           length:vertices.size() * sizeof(Vertex)];
        NSData* indexData = [NSData dataWithBytes:indices.data()
                                          length:indices.size() * sizeof(uint16_t)];
        
        return {vertexData, indexData, indices.size(), MTLPrimitiveTypeTriangle};
    }
    
    static MeshData createPlane(float width = 10.0f, float depth = 10.0f, int subdivisions = 1) {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        
        int verticesPerSide = subdivisions + 2;
        float stepX = width / (subdivisions + 1);
        float stepZ = depth / (subdivisions + 1);
        
        // Generate vertices
        for (int z = 0; z < verticesPerSide; ++z) {
            for (int x = 0; x < verticesPerSide; ++x) {
                Vertex vertex;
                vertex.position = {
                    -width/2.0f + x * stepX,
                    0.0f,
                    -depth/2.0f + z * stepZ
                };
                vertex.normal = {0, 1, 0};
                vertex.color = {0.5f, 0.5f, 0.5f};
                vertex.texCoord = {
                    (float)x / (verticesPerSide - 1),
                    (float)z / (verticesPerSide - 1)
                };
                vertices.push_back(vertex);
            }
        }
        
        // Generate indices
        for (int z = 0; z < verticesPerSide - 1; ++z) {
            for (int x = 0; x < verticesPerSide - 1; ++x) {
                uint16_t topLeft = z * verticesPerSide + x;
                uint16_t topRight = topLeft + 1;
                uint16_t bottomLeft = topLeft + verticesPerSide;
                uint16_t bottomRight = bottomLeft + 1;
                
                // First triangle
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);
                
                // Second triangle
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
        
        NSData* vertexData = [NSData dataWithBytes:vertices.data()
                                           length:vertices.size() * sizeof(Vertex)];
        NSData* indexData = [NSData dataWithBytes:indices.data()
                                          length:indices.size() * sizeof(uint16_t)];
        
        return {vertexData, indexData, indices.size(), MTLPrimitiveTypeTriangle};
    }
};
