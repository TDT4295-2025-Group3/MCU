#include "entities/platform.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

namespace mcu_game
{

    bool Platform::init(Rasterizer::IRasterizer &gfx, GameState &gameState)
    {
        if (!createBuffersWithFallback(gfx, "platform.obj",
                                       assets::baked::MeshId::Platform,
                                       vertexId,
                                       triangleId))
            return false;

        const auto platformInstanceResp = gfx.createInstance(static_cast<uint8_t>(vertexId),
                                                             static_cast<uint8_t>(triangleId),
                                                             transform);
        if (!platformInstanceResp.isSuccess())
            return false;
        instanceId = platformInstanceResp.getInstanceId();

        gameState.boxColliders.push_back(collider);
        return true;
    }

    void Platform::update(const InputState &in, float deltaTime, GameState &gameState)
    {
    }

    void Platform::render(Rasterizer::IRasterizer &gfx)
    {
    }

} // namespace mcu_game
