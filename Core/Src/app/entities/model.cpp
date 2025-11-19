#include "entities/model.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

namespace mcu_game
{

    bool Model::init(GameState &gameState)
    {
        if (!gameState.load_model(meshId, vertexId, triangleId))
            return false;

        const auto platformInstanceResp = gameState.gfx.createInstance(static_cast<uint8_t>(vertexId),
                                                                       static_cast<uint8_t>(triangleId),
                                                                       transform);
        if (!platformInstanceResp.isSuccess())
            return false;
        instanceId = platformInstanceResp.getInstanceId();

        return true;
    }

}
