// Engine/Geometry.hpp
// Builders for primitive meshes. The Vertex layout matches the vertex
// descriptor configured in MetalRenderer (stride 64, simd_float3 alignment).
#pragma once

#include "Math.hpp"
#include <vector>
#include <cmath>
#include <cstdint>

namespace Engine {

struct Vertex {
    Math::Vec3 position;
    Math::Vec3 color;
    Math::Vec3 normal;
    Math::Vec2 texCoord;
};

struct MeshData {
    std::vector<Vertex>   vertices;
    std::vector<uint16_t> indices;
};

namespace Geometry {

inline MeshData createCube(float size = 1.0f) {
    MeshData m;
    const float h = size * 0.5f;

    const Math::Vec3 corners[8] = {
        {-h, -h, -h}, { h, -h, -h}, { h,  h, -h}, {-h,  h, -h},
        {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h},
    };
    const int faces[6][4] = {
        {0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7},
        {1, 5, 6, 2}, {3, 2, 6, 7}, {4, 5, 1, 0},
    };
    const Math::Vec3 colors[6] = {
        {1, 0, 0}, {0, 1, 0}, {0, 0, 1},
        {1, 1, 0}, {1, 0, 1}, {0, 1, 1},
    };
    const Math::Vec3 normals[6] = {
        { 0,  0, -1}, { 0,  0,  1}, {-1,  0,  0},
        { 1,  0,  0}, { 0,  1,  0}, { 0, -1,  0},
    };

    for (int f = 0; f < 6; ++f) {
        const uint16_t base = static_cast<uint16_t>(m.vertices.size());
        for (int v = 0; v < 4; ++v) {
            m.vertices.push_back({
                corners[faces[f][v]],
                colors[f],
                normals[f],
                {(v == 1 || v == 2) ? 1.0f : 0.0f,
                 (v == 2 || v == 3) ? 1.0f : 0.0f},
            });
        }
        m.indices.insert(m.indices.end(), {
            uint16_t(base + 0), uint16_t(base + 1), uint16_t(base + 2),
            uint16_t(base + 0), uint16_t(base + 2), uint16_t(base + 3),
        });
    }
    return m;
}

inline MeshData createSphere(float radius = 1.0f, int segments = 32, int rings = 16) {
    MeshData m;

    for (int ring = 0; ring <= rings; ++ring) {
        const float theta = Math::Constants::PI * ring / rings;
        const float st = sinf(theta), ct = cosf(theta);
        for (int seg = 0; seg <= segments; ++seg) {
            const float phi = 2.0f * Math::Constants::PI * seg / segments;
            const Math::Vec3 pos{
                radius * st * cosf(phi),
                radius * ct,
                radius * st * sinf(phi),
            };
            m.vertices.push_back({
                pos,
                {(pos.x + radius) / (2 * radius),
                 (pos.y + radius) / (2 * radius),
                 (pos.z + radius) / (2 * radius)},
                Math::Vector::normalize(pos),
                {static_cast<float>(seg) / segments,
                 static_cast<float>(ring) / rings},
            });
        }
    }
    for (int ring = 0; ring < rings; ++ring) {
        for (int seg = 0; seg < segments; ++seg) {
            const uint16_t a = ring * (segments + 1) + seg;
            const uint16_t b = a + segments + 1;
            m.indices.insert(m.indices.end(),
                {a, b, uint16_t(a + 1), b, uint16_t(b + 1), uint16_t(a + 1)});
        }
    }
    return m;
}

inline MeshData createPlane(float width = 10.0f, float depth = 10.0f, int subdivisions = 1) {
    MeshData m;
    const int verts  = subdivisions + 2;
    const float stepX = width / (verts - 1);
    const float stepZ = depth / (verts - 1);

    for (int z = 0; z < verts; ++z) {
        for (int x = 0; x < verts; ++x) {
            m.vertices.push_back({
                {-width * 0.5f + x * stepX, 0.0f, -depth * 0.5f + z * stepZ},
                {0.5f, 0.5f, 0.5f},
                {0, 1, 0},
                {static_cast<float>(x) / (verts - 1),
                 static_cast<float>(z) / (verts - 1)},
            });
        }
    }
    for (int z = 0; z < verts - 1; ++z) {
        for (int x = 0; x < verts - 1; ++x) {
            const uint16_t tl = z * verts + x;
            const uint16_t tr = tl + 1;
            const uint16_t bl = tl + verts;
            const uint16_t br = bl + 1;
            m.indices.insert(m.indices.end(), {tl, bl, tr, tr, bl, br});
        }
    }
    return m;
}

} // namespace Geometry
} // namespace Engine
