#pragma once

#include <cstddef>
#include <cstdint>

#include "platform/irasterizer.hpp"

namespace mcu_game::assets::baked
{

    enum class MeshId : uint8_t
    {
        PlayerIdle,
        PlayerRun1,
        PlayerRun2,
        PlayerJumpUp,
        PlayerJumpDown,
        Platform,
        Collision,
    };

    struct MeshData
    {
        const Rasterizer::Vertex *vertices;
        std::size_t vertexCount;
        const Rasterizer::Triangle *triangles;
        std::size_t triangleCount;
    };

    const MeshData &getMesh(MeshId id);
    const char *getMeshName(MeshId id);
    bool createBuffers(MeshId id,
                       Rasterizer::IRasterizer &gfx,
                       uint32_t &outVertexId,
                       uint32_t &outTriangleId);

} // namespace mcu_game::assets::baked
