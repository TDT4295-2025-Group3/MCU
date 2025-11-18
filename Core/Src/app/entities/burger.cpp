#include "entities/burger.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

namespace mcu_game
{

    bool Burger::init(GameState &gameState)
    {
        if (!gameState.load_model(assets::baked::MeshId::Burger, vertexId, triangleId))
            return false;

        const auto platformInstanceResp = gameState.gfx.createInstance(static_cast<uint8_t>(vertexId),
                                                                       static_cast<uint8_t>(triangleId),
                                                                       transform);
        if (!platformInstanceResp.isSuccess())
            return false;
        instanceId = platformInstanceResp.getInstanceId();

        animator.addAnimation(Animation{
            "Squish",
            {
                {true, vertexId, triangleId,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.15f},

                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.1f, 0.86f, 1.1f,
                 0.02f},
            },
            false});

        animator.addAnimation(Animation{
            "NoSquish",
            {
                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.1f, 0.86f, 1.1f,
                 0.2f},

                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.0f},
            },
            false});

        animator.playAnimation("NoSquish");

        BoxCollider collider{{transform.position.x, transform.position.y - 0.76f, transform.position.z}, {2.1f, 0.5f, 2.1f}};
        collider.friction = 0.22f;
        collider.onLand = [this]()
        { this->landCallback(); };
        gameState.boxColliders.push_back(collider);

        return true;
    }

    void Burger::landCallback()
    {
        animator.playAnimation("Squish");
        landTimer = 0.05f;
    }

    void Burger::update(float deltaTime, GameState &gameState)
    {
        animator.update(deltaTime);
        landTimer -= deltaTime;
        if (landTimer <= 0.0f)
        {
            animator.playAnimation("NoSquish");
        }
    }

    void Burger::render(GameState &gameState)
    {
        const auto animState = animator.getCurrentAnimState(vertexId, triangleId, transform);
        gameState.gfx.updateInstance(animState.vertexId, animState.triangleId, instanceId, animState.transform);
    }

}
