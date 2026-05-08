// Engine/Graphics/VertexLayout.cpp
#include "RenderDevice.hpp"

namespace Engine { namespace Graphics {

// Round n up to the next multiple of 16. simd vector_floatN types want 16-byte
// alignment, and the matching C++ Vertex struct has trailing padding that puts
// successive fields on 16-byte boundaries — the layout must agree.
static constexpr uint32_t alignTo16(uint32_t n) { return (n + 15u) & ~15u; }

VertexLayout VertexLayout::PositionColorNormalTexCoord() {
    constexpr uint32_t offPos = 0;
    constexpr uint32_t offCol = alignTo16(sizeof(Math::Vec3));        // 16
    constexpr uint32_t offNrm = offCol + sizeof(Math::Vec3);          // 32
    constexpr uint32_t offUV  = offNrm + sizeof(Math::Vec3);          // 48

    VertexLayout layout;
    layout.attributes = {
        {"position", VertexFormat::Float3, offPos, 0},
        {"color",    VertexFormat::Float3, offCol, 0},
        {"normal",   VertexFormat::Float3, offNrm, 0},
        {"texCoord", VertexFormat::Float2, offUV,  0},
    };
    layout.stride = alignTo16(offUV + sizeof(Math::Vec2));            // 64
    return layout;
}

}} // namespace Engine::Graphics
