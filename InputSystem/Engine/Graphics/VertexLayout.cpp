// Engine/Graphics/VertexLayout.cpp
#include "RenderDevice.hpp"

namespace Engine { namespace Graphics {

static constexpr uint32_t A16(uint32_t v) { return (v + 15u) & ~15u; } // align to 16

VertexLayout VertexLayout::Position3D() {
    VertexLayout layout;
    layout.attributes = {
        {"position", VertexFormat::Float3, 0, 0}
    };
    layout.stride = A16(sizeof(Math::Vec3));          // 16
    return layout;
}

VertexLayout VertexLayout::PositionColor() {
    constexpr uint32_t offPos = 0;                    // 0
    constexpr uint32_t offCol = A16(sizeof(Math::Vec3)); // 16
    VertexLayout layout;
    layout.attributes = {
        {"position", VertexFormat::Float3, offPos, 0},
        {"color",    VertexFormat::Float3, offCol, 0}
    };
    layout.stride = A16(offCol + sizeof(Math::Vec3)); // 32
    return layout;
}

VertexLayout VertexLayout::PositionNormalTexCoord() {
    constexpr uint32_t offPos = 0;                    // 0
    constexpr uint32_t offNrm = A16(sizeof(Math::Vec3)); // 16
    constexpr uint32_t offUV  = offNrm + sizeof(Math::Vec3); // 32
    VertexLayout layout;
    layout.attributes = {
        {"position", VertexFormat::Float3, offPos, 0},
        {"normal",   VertexFormat::Float3, offNrm, 0},
        {"texCoord", VertexFormat::Float2, offUV,  0}
    };
    layout.stride = A16(offUV + sizeof(Math::Vec2));  // 64
    return layout;
}

VertexLayout VertexLayout::PositionColorNormalTexCoord() {
    constexpr uint32_t offPos = 0;                    // 0
    constexpr uint32_t offCol = A16(sizeof(Math::Vec3)); // 16
    constexpr uint32_t offNrm = offCol + sizeof(Math::Vec3); // 32
    constexpr uint32_t offUV  = offNrm + sizeof(Math::Vec3); // 48
    VertexLayout layout;
    layout.attributes = {
        {"position", VertexFormat::Float3, offPos, 0},
        {"color",    VertexFormat::Float3, offCol, 0},
        {"normal",   VertexFormat::Float3, offNrm, 0},
        {"texCoord", VertexFormat::Float2, offUV,  0}
    };
    layout.stride = A16(offUV + sizeof(Math::Vec2));  // 64
    return layout;
}

}} // namespace
