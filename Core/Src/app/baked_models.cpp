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
        case MeshId::PlayerSleep:
            return PLAYER_SLEEP_MESH;
        case MeshId::Platform:
            return PLATFORM_MESH;
        case MeshId::Collision:
            return COLLISION_MESH_DATA;
        case MeshId::BasePlatform:
            return BASE_PLATFORM_MESH;
        case MeshId::Fence:
            return FENCE_MESH;
        case MeshId::Fire:
            return FIRE_MESH;
        case MeshId::Banana:
            return BANANA_MESH;
        case MeshId::Mushroom:
            return MUSHROOM_MESH;
        case MeshId::Burger:
            return BURGER_MESH;
        case MeshId::Logo:
            return UPWARD_LOGO_MIRRORED_MESH;
        case MeshId::PressXToStart:
            return PRESS_X_MIRRORED_MESH;
        case MeshId::FishingPlatform:
            return FISHING_PLATFORM_MESH;
        case MeshId::PlayerFish:
            return PLAYER_FISH_MESH;
        case MeshId::Cloud:
            return CLOUD_MESH;
        case MeshId::Star:
            return STAR_MESH;
        }
        return EMPTY_MESH;
    }

    const std::string getMeshName(MeshId id)
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
        case MeshId::PlayerSleep:
            return "PlayerSleep";
        case MeshId::Platform:
            return "Platform";
        case MeshId::Collision:
            return "Collision";
        case MeshId::BasePlatform:
            return "BasePlatform";
        case MeshId::Fence:
            return "Fence";
        case MeshId::Fire:
            return "Fire";
        case MeshId::Banana:
            return "Banana";
        case MeshId::Mushroom:
            return "Mushroom";
        case MeshId::Burger:
            return "Burger";
        case MeshId::Logo:
            return "Logo";
        case MeshId::PressXToStart:
            return "PressXToStart";
        case MeshId::FishingPlatform:
            return "FishingPlatform";
        case MeshId::PlayerFish:
            return "PlayerFish";
        case MeshId::Cloud:
            return "Cloud";
        case MeshId::Star:
            return "Star";
        }
        return "Unknown";
    }

    const std::string getMeshFileName(MeshId id)
    {
        switch (id)
        {
        case MeshId::PlayerIdle:
            return "playerIdle.obj";
        case MeshId::PlayerRun1:
            return "playerRun1.obj";
        case MeshId::PlayerRun2:
            return "playerRun2.obj";
        case MeshId::PlayerJumpUp:
            return "playerJumpUp.obj";
        case MeshId::PlayerJumpDown:
            return "playerJumpDown.obj";
        case MeshId::PlayerSleep:
            return "playerSleep.obj";
        case MeshId::Platform:
            return "platform.obj";
        case MeshId::Collision:
            return "collision.obj";
        case MeshId::BasePlatform:
            return "base_platform.obj";
        case MeshId::Fence:
            return "fence.obj";
        case MeshId::Fire:
            return "fire.obj";
        case MeshId::Banana:
            return "banana.obj";
        case MeshId::Mushroom:
            return "mushroom.obj";
        case MeshId::Burger:
            return "burger.obj";
        case MeshId::Logo:
            return "upward_logo_mirrored.obj";
        case MeshId::PressXToStart:
            return "press_x_mirrored.obj";
        case MeshId::FishingPlatform:
            return "fishing_platform.obj";
        case MeshId::PlayerFish:
            return "playerFish.obj";
        case MeshId::Cloud:
            return "cloud.obj";
        case MeshId::Star:
            return "star.obj";
        }
        return "unknown.obj";
    }

    bool createBuffers(MeshId id,
                       Rasterizer::IRasterizer &gfx,
                       uint32_t &outVertexId,
                       uint32_t &outTriangleId)
    {
        outVertexId = 0xFF;
        outTriangleId = 0xFF;

        const auto &mesh = getMesh(id);
        const std::string meshName = getMeshName(id);

        if (!mesh.vertices || mesh.vertexCount == 0 || !mesh.triangles || mesh.triangleCount == 0)
        {
            std::printf("[Model] Baked mesh %s has no geometry data\n", meshName.c_str());
            return false;
        }

        if (mesh.vertexCount > std::numeric_limits<uint16_t>::max() ||
            mesh.triangleCount > std::numeric_limits<uint16_t>::max())
        {
            std::printf("[Model] Baked mesh %s exceeds rasterizer limits (%zu verts, %zu tris)\n",
                        meshName.c_str(),
                        mesh.vertexCount,
                        mesh.triangleCount);
            return false;
        }

        const auto vertResp = gfx.createVertex(mesh.vertices, static_cast<uint16_t>(mesh.vertexCount));
        if (!vertResp.isSuccess())
        {
            std::printf("[Model] Failed to create baked vertex buffer for %s (status=%u)\n",
                        meshName.c_str(),
                        static_cast<unsigned>(vertResp.getStatus()));
            return false;
        }

        const auto triResp = gfx.createTriangle(mesh.triangles, static_cast<uint16_t>(mesh.triangleCount));
        if (!triResp.isSuccess())
        {
            std::printf("[Model] Failed to create baked triangle buffer for %s (status=%u)\n",
                        meshName.c_str(),
                        static_cast<unsigned>(triResp.getStatus()));
            return false;
        }

        outVertexId = vertResp.getVertexId();
        outTriangleId = triResp.getTriangleId();
        return true;
    }

} // namespace mcu_game::assets::baked
