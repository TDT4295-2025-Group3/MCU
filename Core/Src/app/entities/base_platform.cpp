#include "entities/base_platform.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

namespace mcu_game
{

    bool BasePlatform::init(Rasterizer::IRasterizer &gfx, GameState &gameState)
    {
        if (!createBuffersWithFallback(gfx, assets::baked::MeshId::BasePlatform,
                                       vertexId,
                                       triangleId))
            return false;

        const auto platformInstanceResp = gfx.createInstance(static_cast<uint8_t>(vertexId),
                                                             static_cast<uint8_t>(triangleId),
                                                             transform);
        if (!platformInstanceResp.isSuccess())
            return false;
        instanceId = platformInstanceResp.getInstanceId();

        gameState.boxColliders.push_back({{transform.position.x, transform.position.y - 0.5f, transform.position.z}, {8.4f, 0.5f, 8.4f}});
        gameState.boxColliders.push_back({{transform.position.x + 2.7f, transform.position.y + 0.5f, transform.position.z - 5.8f}, {1.8f, 1.5f, 1.4f}});

        if (!fire.init(gfx, gameState))
            return false;

        return true;
    }

    void BasePlatform::update(const InputState &in, float deltaTime, GameState &gameState)
    {
        fire.update(in, deltaTime, gameState);
    }

    void BasePlatform::render(Rasterizer::IRasterizer &gfx)
    {
        fire.render(gfx);
    }

} // namespace mcu_game
