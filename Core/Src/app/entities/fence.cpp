#include "entities/fence.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

namespace mcu_game
{

    bool Fence::init(Rasterizer::IRasterizer &gfx, GameState &gameState)
    {
        if (!createBuffersWithFallback(gfx, assets::baked::MeshId::Fence,
                                       vertexId,
                                       triangleId))
            return false;

        const auto platformInstanceResp = gfx.createInstance(static_cast<uint8_t>(vertexId),
                                                             static_cast<uint8_t>(triangleId),
                                                             transform);
        if (!platformInstanceResp.isSuccess())
            return false;
        instanceId = platformInstanceResp.getInstanceId();

        return true;
    }

    void Fence::update(IInput &input, float deltaTime, GameState &gameState)
    {
    }

    void Fence::render(Rasterizer::IRasterizer &gfx)
    {
    }

} // namespace mcu_game
