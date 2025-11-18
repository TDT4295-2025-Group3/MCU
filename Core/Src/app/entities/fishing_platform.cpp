#include "entities/fishing_platform.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

namespace mcu_game
{

    bool FishingPlatform::init(GameState &gameState)
    {
        if (!gameState.load_model(assets::baked::MeshId::FishingPlatform, vertexId, triangleId))
            return false;

        const auto platformInstanceResp = gameState.gfx.createInstance(static_cast<uint8_t>(vertexId),
                                                                       static_cast<uint8_t>(triangleId),
                                                                       transform);
        if (!platformInstanceResp.isSuccess())
            return false;
        instanceId = platformInstanceResp.getInstanceId();

        hasLanded = false;
        gameState.isEndingFishSequenceActive = false;

        BoxCollider collider{{transform.position.x, transform.position.y - 0.5f, transform.position.z}, {2.3f, 0.6f, 2.3f}};
        collider.onLand = [this, &gameState]()
        {
            this->onLand();
        };
        gameState.boxColliders.push_back(collider);
        return true;
    }

    void FishingPlatform::onLand()
    {
        hasLanded = true;
    }

    void FishingPlatform::update(float deltaTime, GameState &gameState)
    {
        if (hasLanded)
        {
            gameState.endFishPosition = Vec3{transform.position.x, transform.position.y, transform.position.z - 2.2f};
            gameState.isEndingFishSequenceActive = true;
        }
    }

} // namespace mcu_game
