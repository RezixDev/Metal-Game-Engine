// Engine/Graphics/GeometryBuilder.hpp
#pragma once

#include "RenderDevice.hpp"
#include "../Math/Math.hpp"

#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

namespace Engine {
namespace Graphics {

// Vertex layout shared with VertexLayout::PositionColorNormalTexCoord(). Each
// member uses simd vector_floatN, so trailing padding makes the struct match
// the 64-byte stride the GPU pipeline expects.
struct Vertex {
    Math::Vec3 position;
    Math::Vec3 color;
    Math::Vec3 normal;
    Math::Vec2 texCoord;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    PrimitiveType primitiveType = PrimitiveType::Triangles;

    bool empty() const { return vertices.empty() || indices.empty(); }
};

class Mesh {
public:
    Mesh(RenderDevicePtr device, const MeshData& data)
        : indexCount_(static_cast<uint32_t>(data.indices.size())),
          primitiveType_(data.primitiveType) {

        if (data.empty()) throw std::runtime_error("Cannot create mesh from empty data");

        vertexBuffer_ = device->createBuffer(
            BufferType::Vertex, BufferUsage::Static,
            data.vertices.size() * sizeof(Vertex), data.vertices.data());

        indexBuffer_ = device->createBuffer(
            BufferType::Index, BufferUsage::Static,
            data.indices.size() * sizeof(uint16_t), data.indices.data());
    }

    void draw(CommandBufferPtr cmd) const {
        cmd->setVertexBuffer(vertexBuffer_, 0);
        cmd->setIndexBuffer(indexBuffer_);
        cmd->drawIndexed(primitiveType_, indexCount_);
    }

private:
    BufferPtr vertexBuffer_;
    BufferPtr indexBuffer_;
    uint32_t indexCount_;
    PrimitiveType primitiveType_;
};

using MeshPtr = std::shared_ptr<Mesh>;

class GeometryBuilder {
public:
    static MeshData createCube(float size = 1.0f) {
        MeshData mesh;
        const float h = size * 0.5f;

        const Math::Vec3 corners[8] = {
            {-h, -h, -h}, { h, -h, -h}, { h,  h, -h}, {-h,  h, -h},
            {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h},
        };

        const int faces[6][4] = {
            {0, 1, 2, 3}, // Front  (-Z)
            {5, 4, 7, 6}, // Back   (+Z)
            {4, 0, 3, 7}, // Left   (-X)
            {1, 5, 6, 2}, // Right  (+X)
            {3, 2, 6, 7}, // Top    (+Y)
            {4, 5, 1, 0}, // Bottom (-Y)
        };

        const Math::Vec3 colors[6] = {
            {1, 0, 0}, {0, 1, 0}, {0, 0, 1},
            {1, 1, 0}, {1, 0, 1}, {0, 1, 1},
        };

        const Math::Vec3 normals[6] = {
            { 0,  0, -1}, { 0,  0,  1},
            {-1,  0,  0}, { 1,  0,  0},
            { 0,  1,  0}, { 0, -1,  0},
        };

        for (int f = 0; f < 6; ++f) {
            const uint16_t base = static_cast<uint16_t>(mesh.vertices.size());
            for (int v = 0; v < 4; ++v) {
                mesh.vertices.push_back({
                    corners[faces[f][v]],
                    colors[f],
                    normals[f],
                    {(v == 1 || v == 2) ? 1.0f : 0.0f,
                     (v == 2 || v == 3) ? 1.0f : 0.0f},
                });
            }
            mesh.indices.insert(mesh.indices.end(),
                {uint16_t(base + 0), uint16_t(base + 1), uint16_t(base + 2),
                 uint16_t(base + 0), uint16_t(base + 2), uint16_t(base + 3)});
        }
        return mesh;
    }

    static MeshData createSphere(float radius = 1.0f, int segments = 32, int rings = 16) {
        MeshData mesh;

        for (int ring = 0; ring <= rings; ++ring) {
            const float theta = Math::Constants::PI * ring / rings;
            const float sinTheta = sinf(theta);
            const float cosTheta = cosf(theta);

            for (int seg = 0; seg <= segments; ++seg) {
                const float phi = 2.0f * Math::Constants::PI * seg / segments;
                Math::Vec3 position{
                    radius * sinTheta * cosf(phi),
                    radius * cosTheta,
                    radius * sinTheta * sinf(phi),
                };
                mesh.vertices.push_back({
                    position,
                    {(position.x + radius) / (2 * radius),
                     (position.y + radius) / (2 * radius),
                     (position.z + radius) / (2 * radius)},
                    Math::Vector::normalize(position),
                    {static_cast<float>(seg) / segments,
                     static_cast<float>(ring) / rings},
                });
            }
        }

        for (int ring = 0; ring < rings; ++ring) {
            for (int seg = 0; seg < segments; ++seg) {
                const uint16_t a = ring * (segments + 1) + seg;
                const uint16_t b = a + segments + 1;
                mesh.indices.insert(mesh.indices.end(),
                    {a, b, uint16_t(a + 1),
                     b, uint16_t(b + 1), uint16_t(a + 1)});
            }
        }
        return mesh;
    }

    static MeshData createPlane(float width = 10.0f, float depth = 10.0f, int subdivisions = 1) {
        MeshData mesh;
        const int verts = subdivisions + 2;
        const float stepX = width / (verts - 1);
        const float stepZ = depth / (verts - 1);

        for (int z = 0; z < verts; ++z) {
            for (int x = 0; x < verts; ++x) {
                mesh.vertices.push_back({
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
                mesh.indices.insert(mesh.indices.end(), {tl, bl, tr, tr, bl, br});
            }
        }
        return mesh;
    }

    static MeshPtr createCubeMesh(RenderDevicePtr device, float size = 1.0f) {
        return std::make_shared<Mesh>(device, createCube(size));
    }

    static MeshPtr createSphereMesh(RenderDevicePtr device, float radius = 1.0f,
                                    int segments = 32, int rings = 16) {
        return std::make_shared<Mesh>(device, createSphere(radius, segments, rings));
    }

    static MeshPtr createPlaneMesh(RenderDevicePtr device, float width = 10.0f,
                                   float depth = 10.0f, int subdivisions = 1) {
        return std::make_shared<Mesh>(device, createPlane(width, depth, subdivisions));
    }
};

} // namespace Graphics
} // namespace Engine
