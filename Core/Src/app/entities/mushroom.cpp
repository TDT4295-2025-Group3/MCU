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
                 true, 1.3f, 0.9f, 1.3f,
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
        collider.bounciness = 30.0f;
        collider.onLand = [this]()
        { this->bounceCallback(); };
        gameState.boxColliders.push_back(collider);

        return true;
    }

    void Mushroom::bounceCallback()
    {
        animator.playAnimation("Bounce", true);
    }

    void Mushroom::update(float deltaTime, GameState &gameState)
    {
        animator.update(deltaTime);
    }

    void Mushroom::render(GameState &gameState)
    {
        const auto animState = animator.getCurrentAnimState(vertexId, triangleId, transform);
        if (animState.transform.position.x == lastAnimTransform.position.x &&
            animState.transform.position.y == lastAnimTransform.position.y &&
            animState.transform.position.z == lastAnimTransform.position.z &&
            animState.transform.rotation.x == lastAnimTransform.rotation.x &&
            animState.transform.rotation.y == lastAnimTransform.rotation.y &&
            animState.transform.rotation.z == lastAnimTransform.rotation.z &&
            animState.transform.scale.x == lastAnimTransform.scale.x &&
            animState.transform.scale.y == lastAnimTransform.scale.y &&
            animState.transform.scale.z == lastAnimTransform.scale.z)
        {
            return; // no change
        }

        gameState.gfx.updateInstance(animState.vertexId, animState.triangleId, instanceId, animState.transform);
        lastAnimTransform = animState.transform;
    }

}
