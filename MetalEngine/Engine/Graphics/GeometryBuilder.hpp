// Engine/Graphics/GeometryBuilder.hpp
// UPDATED VERSION - Modern geometry builder using the new graphics abstraction
#pragma once

#include <vector>

namespace Engine {
namespace Graphics {

    // ========================================
    // Modern Vertex Structure
    // ========================================

    struct Vertex {
        Math::Vec3 position;
        Math::Vec3 color;
        Math::Vec3 normal;
        Math::Vec2 texCoord;
        
        Vertex() = default;
        Vertex(const Math::Vec3& pos, const Math::Vec3& col = {1,1,1}, const Math::Vec3& norm = {0,1,0}, const Math::Vec2& tex = {0,0})
            : position(pos), color(col), normal(norm), texCoord(tex) {}
    };

    // ========================================
    // Mesh Data Container
    // ========================================

    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
        PrimitiveType primitiveType = PrimitiveType::Triangles;
        
        // Helper methods
        void clear() {
            vertices.clear();
            indices.clear();
        }
        
        size_t getVertexDataSize() const {
            return vertices.size() * sizeof(Vertex);
        }
        
        size_t getIndexDataSize() const {
            return indices.size() * sizeof(uint16_t);
        }
        
        bool isEmpty() const {
            return vertices.empty() || indices.empty();
        }
    };

    // ========================================
    // High-Level Mesh Class
    // ========================================

    class Mesh {
    public:
        Mesh(RenderDevicePtr device, const MeshData& meshData);
        ~Mesh() = default;

        void draw(CommandBufferPtr cmd);
        void updateVertices(const std::vector<Vertex>& vertices);
        void updateIndices(const std::vector<uint16_t>& indices);

        BufferPtr getVertexBuffer() const { return vertexBuffer_; }
        BufferPtr getIndexBuffer() const { return indexBuffer_; }
        uint32_t getIndexCount() const { return indexCount_; }
        PrimitiveType getPrimitiveType() const { return primitiveType_; }

    private:
        RenderDevicePtr device_;
        BufferPtr vertexBuffer_;
        BufferPtr indexBuffer_;
        uint32_t indexCount_;
        PrimitiveType primitiveType_;
    };

    using MeshPtr = std::shared_ptr<Mesh>;

    // ========================================
    // Modern GeometryBuilder
    // ========================================

    class GeometryBuilder {
    public:
        // ========================================
        // Basic Shapes
        // ========================================

        static MeshData createCube(float size = 1.0f) {
            MeshData meshData;
            float h = size * 0.5f;

            // Define cube corners
            Math::Vec3 corners[8] = {
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
            Math::Vec3 colors[6] = {
                {1.0f, 0.0f, 0.0f}, // Red
                {0.0f, 1.0f, 0.0f}, // Green
                {0.0f, 0.0f, 1.0f}, // Blue
                {1.0f, 1.0f, 0.0f}, // Yellow
                {1.0f, 0.0f, 1.0f}, // Magenta
                {0.0f, 1.0f, 1.0f}  // Cyan
            };

            // Face normals
            Math::Vec3 normals[6] = {
                { 0,  0, -1}, // Front
                { 0,  0,  1}, // Back
                {-1,  0,  0}, // Left
                { 1,  0,  0}, // Right
                { 0,  1,  0}, // Top
                { 0, -1,  0}  // Bottom
            };

            // Build vertices and indices for each face
            for (int f = 0; f < 6; ++f) {
                uint16_t baseIndex = static_cast<uint16_t>(meshData.vertices.size());

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

                    meshData.vertices.push_back(vertex);
                }

                // Add two triangles for this face
                meshData.indices.insert(meshData.indices.end(), {
                    static_cast<uint16_t>(baseIndex + 0),
                    static_cast<uint16_t>(baseIndex + 1),
                    static_cast<uint16_t>(baseIndex + 2),
                    
                    static_cast<uint16_t>(baseIndex + 0),
                    static_cast<uint16_t>(baseIndex + 2),
                    static_cast<uint16_t>(baseIndex + 3)
                });
            }

            return meshData;
        }

        static MeshData createSphere(float radius = 1.0f, int segments = 32, int rings = 16) {
            MeshData meshData;

            // Generate vertices
            for (int ring = 0; ring <= rings; ++ring) {
                float theta = Math::Constants::PI * ring / rings;
                float sinTheta = sinf(theta);
                float cosTheta = cosf(theta);

                for (int seg = 0; seg <= segments; ++seg) {
                    float phi = 2.0f * Math::Constants::PI * seg / segments;
                    float sinPhi = sinf(phi);
                    float cosPhi = cosf(phi);

                    Vertex vertex;
                    vertex.position = {
                        radius * sinTheta * cosPhi,
                        radius * cosTheta,
                        radius * sinTheta * sinPhi
                    };

                    // Normal is just normalized position for a sphere
                    vertex.normal = Math::Vector::normalize(vertex.position);

                    // Color based on position (creates gradient effect)
                    vertex.color = {
                        (vertex.position.x + radius) / (2.0f * radius),
                        (vertex.position.y + radius) / (2.0f * radius),
                        (vertex.position.z + radius) / (2.0f * radius)
                    };

                    // UV mapping
                    vertex.texCoord = {
                        static_cast<float>(seg) / segments,
                        static_cast<float>(ring) / rings
                    };

                    meshData.vertices.push_back(vertex);
                }
            }

            // Generate indices
            for (int ring = 0; ring < rings; ++ring) {
                for (int seg = 0; seg < segments; ++seg) {
                    uint16_t first = ring * (segments + 1) + seg;
                    uint16_t second = first + segments + 1;

                    // First triangle
                    meshData.indices.insert(meshData.indices.end(), {
                        first, second, static_cast<uint16_t>(first + 1)
                    });

                    // Second triangle
                    meshData.indices.insert(meshData.indices.end(), {
                        second, static_cast<uint16_t>(second + 1), static_cast<uint16_t>(first + 1)
                    });
                }
            }

            return meshData;
        }

        static MeshData createPlane(float width = 10.0f, float depth = 10.0f, int subdivisions = 1) {
            MeshData meshData;

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
                        static_cast<float>(x) / (verticesPerSide - 1),
                        static_cast<float>(z) / (verticesPerSide - 1)
                    };
                    meshData.vertices.push_back(vertex);
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
                    meshData.indices.insert(meshData.indices.end(), {
                        topLeft, bottomLeft, topRight
                    });

                    // Second triangle
                    meshData.indices.insert(meshData.indices.end(), {
                        topRight, bottomLeft, bottomRight
                    });
                }
            }

            return meshData;
        }

        // ========================================
        // Advanced Shapes
        // ========================================

        static MeshData createCylinder(float radius = 1.0f, float height = 2.0f, int segments = 32) {
            MeshData meshData;

            float halfHeight = height * 0.5f;

            // Generate side vertices
            for (int i = 0; i <= segments; ++i) {
                float angle = 2.0f * Math::Constants::PI * i / segments;
                float x = radius * cosf(angle);
                float z = radius * sinf(angle);

                // Bottom vertex
                Vertex bottomVertex;
                bottomVertex.position = {x, -halfHeight, z};
                bottomVertex.normal = Math::Vector::normalize(Math::Vec3{x, 0, z});
                bottomVertex.color = {0.7f, 0.7f, 0.7f};
                bottomVertex.texCoord = {static_cast<float>(i) / segments, 0.0f};
                meshData.vertices.push_back(bottomVertex);

                // Top vertex
                Vertex topVertex;
                topVertex.position = {x, halfHeight, z};
                topVertex.normal    = Math::Vector::normalize(Math::Vec3{x, 0, z});
                topVertex.color = {0.9f, 0.9f, 0.9f};
                topVertex.texCoord = {static_cast<float>(i) / segments, 1.0f};
                meshData.vertices.push_back(topVertex);
            }

            // Generate side indices
            for (int i = 0; i < segments; ++i) {
                uint16_t bottomLeft = i * 2;
                uint16_t topLeft = bottomLeft + 1;
                uint16_t bottomRight = (i + 1) * 2;
                uint16_t topRight = bottomRight + 1;

                // First triangle
                meshData.indices.insert(meshData.indices.end(), {
                    bottomLeft, topLeft, bottomRight
                });

                // Second triangle
                meshData.indices.insert(meshData.indices.end(), {
                    bottomRight, topLeft, topRight
                });
            }

            return meshData;
        }

        // ========================================
        // Utility Methods
        // ========================================

        static MeshData createFromVertices(const std::vector<Math::Vec3>& positions,
                                         const Math::Vec3& color = {1, 1, 1}) {
            MeshData meshData;

            for (const auto& pos : positions) {
                Vertex vertex;
                vertex.position = pos;
                vertex.color = color;
                vertex.normal = {0, 1, 0}; // Default normal
                vertex.texCoord = {0, 0}; // Default UV
                meshData.vertices.push_back(vertex);
            }

            // Create indices for triangles
            for (size_t i = 0; i < positions.size(); i += 3) {
                if (i + 2 < positions.size()) {
                    meshData.indices.insert(meshData.indices.end(), {
                        static_cast<uint16_t>(i),
                        static_cast<uint16_t>(i + 1),
                        static_cast<uint16_t>(i + 2)
                    });
                }
            }

            return meshData;
        }

        static void calculateNormals(MeshData& meshData) {
            // Reset all normals to zero
            for (auto& vertex : meshData.vertices) {
                vertex.normal = {0, 0, 0};
            }

            // Calculate face normals and accumulate
            for (size_t i = 0; i < meshData.indices.size(); i += 3) {
                if (i + 2 < meshData.indices.size()) {
                    uint16_t i0 = meshData.indices[i];
                    uint16_t i1 = meshData.indices[i + 1];
                    uint16_t i2 = meshData.indices[i + 2];

                    if (i0 < meshData.vertices.size() &&
                        i1 < meshData.vertices.size() &&
                        i2 < meshData.vertices.size()) {

                        const Math::Vec3& v0 = meshData.vertices[i0].position;
                        const Math::Vec3& v1 = meshData.vertices[i1].position;
                        const Math::Vec3& v2 = meshData.vertices[i2].position;

                        Math::Vec3 edge1 = v1 - v0;
                        Math::Vec3 edge2 = v2 - v0;
                        Math::Vec3 faceNormal = Math::Vector::cross(edge1, edge2);

                        meshData.vertices[i0].normal += faceNormal;
                        meshData.vertices[i1].normal += faceNormal;
                        meshData.vertices[i2].normal += faceNormal;
                    }
                }
            }

            // Normalize all vertex normals
            for (auto& vertex : meshData.vertices) {
                vertex.normal = Math::Vector::normalize(vertex.normal);
            }
        }

        static void generateTangents(MeshData& meshData) {
            // Simplified tangent generation
            // In a full implementation, this would calculate proper tangent vectors for normal mapping
            for (auto& vertex : meshData.vertices) {
                // Create a tangent perpendicular to the normal
                Math::Vec3 up = (abs(vertex.normal.y) < 0.9f) ?
                    Math::Vec3{0, 1, 0} : Math::Vec3{1, 0, 0};
                Math::Vec3 tangent = Math::Vector::normalize(Math::Vector::cross(vertex.normal, up));
                // Store tangent in unused vertex data if needed
            }
        }

        // ========================================
        // Mesh Factory Functions
        // ========================================

        static MeshPtr createCubeMesh(RenderDevicePtr device, float size = 1.0f) {
            MeshData meshData = createCube(size);
            return std::make_shared<Mesh>(device, meshData);
        }

        static MeshPtr createSphereMesh(RenderDevicePtr device, float radius = 1.0f, int segments = 32, int rings = 16) {
            MeshData meshData = createSphere(radius, segments, rings);
            return std::make_shared<Mesh>(device, meshData);
        }

        static MeshPtr createPlaneMesh(RenderDevicePtr device, float width = 10.0f, float depth = 10.0f, int subdivisions = 1) {
            MeshData meshData = createPlane(width, depth, subdivisions);
            return std::make_shared<Mesh>(device, meshData);
        }

        static MeshPtr createCylinderMesh(RenderDevicePtr device, float radius = 1.0f, float height = 2.0f, int segments = 32) {
            MeshData meshData = createCylinder(radius, height, segments);
            return std::make_shared<Mesh>(device, meshData);
        }
    };

    // ========================================
    // Mesh Implementation
    // ========================================

    inline Mesh::Mesh(RenderDevicePtr device, const MeshData& meshData)
        : device_(device), indexCount_(static_cast<uint32_t>(meshData.indices.size())), primitiveType_(meshData.primitiveType) {

        if (meshData.isEmpty()) {
            throw std::runtime_error("Cannot create mesh from empty mesh data");
        }

        // Create vertex buffer
        vertexBuffer_ = device_->createBuffer(
            BufferType::Vertex,
            BufferUsage::Static,
            meshData.getVertexDataSize(),
            meshData.vertices.data()
        );

        // Create index buffer
        indexBuffer_ = device_->createBuffer(
            BufferType::Index,
            BufferUsage::Static,
            meshData.getIndexDataSize(),
            meshData.indices.data()
        );

        if (!vertexBuffer_ || !indexBuffer_) {
            throw std::runtime_error("Failed to create mesh buffers");
        }
    }

    inline void Mesh::draw(CommandBufferPtr cmd) {
        cmd->setVertexBuffer(vertexBuffer_, 0);
        cmd->setIndexBuffer(indexBuffer_);
        cmd->drawIndexed(primitiveType_, indexCount_);
    }

    inline void Mesh::updateVertices(const std::vector<Vertex>& vertices) {
        if (vertices.size() * sizeof(Vertex) <= vertexBuffer_->getSize()) {
            vertexBuffer_->update(vertices.data(), vertices.size() * sizeof(Vertex));
        } else {
            // Recreate buffer if size changed
            vertexBuffer_ = device_->createBuffer(
                BufferType::Vertex,
                BufferUsage::Dynamic,
                vertices.size() * sizeof(Vertex),
                vertices.data()
            );
        }
    }

    inline void Mesh::updateIndices(const std::vector<uint16_t>& indices) {
        indexCount_ = static_cast<uint32_t>(indices.size());

        if (indices.size() * sizeof(uint16_t) <= indexBuffer_->getSize()) {
            indexBuffer_->update(indices.data(), indices.size() * sizeof(uint16_t));
        } else {
            // Recreate buffer if size changed
            indexBuffer_ = device_->createBuffer(
                BufferType::Index,
                BufferUsage::Dynamic,
                indices.size() * sizeof(uint16_t),
                indices.data()
            );
        }
    }

} // namespace Graphics
} // namespace Engine
