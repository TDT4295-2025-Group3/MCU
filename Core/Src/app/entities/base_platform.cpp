#include "entities/base_platform.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

namespace mcu_game
{

    bool BasePlatform::init(GameState &gameState)
    {
        if (!gameState.load_model(assets::baked::MeshId::BasePlatform, vertexId, triangleId))
            return false;

        const auto platformInstanceResp = gameState.gfx.createInstance(static_cast<uint8_t>(vertexId),
                                                                       static_cast<uint8_t>(triangleId),
                                                                       transform);
        if (!platformInstanceResp.isSuccess())
            return false;
        instanceId = platformInstanceResp.getInstanceId();

        gameState.boxColliders.push_back({{transform.position.x, transform.position.y - 0.5f, transform.position.z}, {8.4f, 0.5f, 8.4f}});
        gameState.boxColliders.push_back({{transform.position.x + 2.7f, transform.position.y + 0.5f, transform.position.z - 5.8f}, {1.8f, 1.5f, 1.4f}});
        gameState.boxColliders.push_back({{transform.position.x + 7.0f, transform.position.y + 0.5f, transform.position.z - 1.0f}, {0.2f, 1.0f, 2.4f}});
        gameState.boxColliders.push_back({{transform.position.x - 4.0f, transform.position.y + 0.5f, transform.position.z - 7.0f}, {2.4f, 1.0f, 0.2f}});

        if (!fire.init(gameState))
            return false;

        if (!fence.init(gameState))
            return false;
        if (!fence2.init(gameState))
            return false;

        return true;
    }

    void BasePlatform::update(float deltaTime, GameState &gameState)
    {
        fire.update(deltaTime, gameState);
    }

    void BasePlatform::render(GameState &gameState)
    {
        fire.render(gameState);
    }

} // namespace mcu_game
