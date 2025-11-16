#include "baked_models.hpp"

#include <cstdio>
#include <limits>
#include "model_data.h"

namespace mcu_game::assets::baked
{

    const MeshData &getMesh(MeshId id)
    {
        switch (id)
        {
        case MeshId::PlayerIdle:
            return PLAYERIDLE_MESH;
        case MeshId::PlayerRun1:
            return PLAYERRUN1_MESH;
        case MeshId::PlayerRun2:
            return PLAYERRUN2_MESH;
        case MeshId::PlayerJumpUp:
            return PLAYERJUMPUP_MESH;
        case MeshId::PlayerJumpDown:
            return PLAYERJUMPDOWN_MESH;
        case MeshId::Platform:
            return PLATFORM_MESH;
        case MeshId::Collision:
            return COLLISION_MESH_DATA;
        }
        return EMPTY_MESH;
    }

    const char *getMeshName(MeshId id)
    {
        switch (id)
        {
        case MeshId::PlayerIdle:
            return "PlayerIdle";
        case MeshId::PlayerRun1:
            return "PlayerRun1";
        case MeshId::PlayerRun2:
            return "PlayerRun2";
        case MeshId::PlayerJumpUp:
            return "PlayerJumpUp";
        case MeshId::PlayerJumpDown:
            return "PlayerJumpDown";
        case MeshId::Platform:
            return "Platform";
        }
        return "Unknown";
    }

    bool createBuffers(MeshId id,
                       Rasterizer::IRasterizer &gfx,
                       uint32_t &outVertexId,
                       uint32_t &outTriangleId)
    {
        outVertexId = 0xFF;
        outTriangleId = 0xFF;

        const auto &mesh = getMesh(id);
        const char *meshName = getMeshName(id);

        if (!mesh.vertices || mesh.vertexCount == 0 || !mesh.triangles || mesh.triangleCount == 0)
        {
            std::printf("[Model] Baked mesh %s has no geometry data\n", meshName);
            return false;
        }

        if (mesh.vertexCount > std::numeric_limits<uint16_t>::max() ||
            mesh.triangleCount > std::numeric_limits<uint16_t>::max())
        {
            std::printf("[Model] Baked mesh %s exceeds rasterizer limits (%zu verts, %zu tris)\n",
                        meshName,
                        mesh.vertexCount,
                        mesh.triangleCount);
            return false;
        }

        const auto vertResp = gfx.createVertex(mesh.vertices, static_cast<uint16_t>(mesh.vertexCount));
        if (!vertResp.isSuccess())
        {
            std::printf("[Model] Failed to create baked vertex buffer for %s (status=%u)\n",
                        meshName,
                        static_cast<unsigned>(vertResp.getStatus()));
            return false;
        }

        const auto triResp = gfx.createTriangle(mesh.triangles, static_cast<uint16_t>(mesh.triangleCount));
        if (!triResp.isSuccess())
        {
            std::printf("[Model] Failed to create baked triangle buffer for %s (status=%u)\n",
                        meshName,
                        static_cast<unsigned>(triResp.getStatus()));
            return false;
        }

        outVertexId = vertResp.getVertexId();
        outTriangleId = triResp.getTriangleId();
        return true;
    }

} // namespace mcu_game::assets::baked
