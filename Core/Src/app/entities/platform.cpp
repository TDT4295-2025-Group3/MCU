#include "entities/platform.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

namespace mcu_game
{

    bool Platform::init(GameState &gameState)
    {
        if (!gameState.load_model(assets::baked::MeshId::Platform, vertexId, triangleId))
            return false;

        const auto platformInstanceResp = gameState.gfx.createInstance(static_cast<uint8_t>(vertexId),
                                                                       static_cast<uint8_t>(triangleId),
                                                                       transform);
        if (!platformInstanceResp.isSuccess())
            return false;
        instanceId = platformInstanceResp.getInstanceId();

        gameState.boxColliders.push_back(collider);
        return true;
    }

} // namespace mcu_game
