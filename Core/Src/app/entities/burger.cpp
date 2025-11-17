#include "entities/mushroom.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

namespace mcu_game
{

    bool Mushroom::init(GameState &gameState)
    {
        if (!gameState.load_model(assets::baked::MeshId::Mushroom, vertexId, triangleId))
            return false;

        const auto platformInstanceResp = gameState.gfx.createInstance(static_cast<uint8_t>(vertexId),
                                                                       static_cast<uint8_t>(triangleId),
                                                                       transform);
        if (!platformInstanceResp.isSuccess())
            return false;
        instanceId = platformInstanceResp.getInstanceId();

        animator.addAnimation(Animation{
            "Bounce",
            {
                {true, vertexId, triangleId,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.15f},

                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.2f, 0.94f, 1.2f,
                 0.15f},

                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.0f},
            },
            false});

        animator.playAnimation("Bounce");

        BoxCollider collider{{transform.position.x, transform.position.y - 0.76f, transform.position.z}, {1.76f, 0.5f, 1.76f}};
        collider.friction = 0.8f;
        collider.onLand = [this]()
        { this->landCallback(); };
        gameState.boxColliders.push_back(collider);

        return true;
    }

    void Burger::landCallback()
    {
        animator.playAnimation("Bounce", true);
    }

    void Burger::update(float deltaTime, GameState &gameState)
    {
        animator.update(deltaTime);
    }

    void Burger::render(GameState &gameState)
    {
        const auto animState = animator.getCurrentAnimState(vertexId, triangleId, transform);
        gameState.gfx.updateInstance(animState.vertexId, animState.triangleId, instanceId, animState.transform);
    }

}
