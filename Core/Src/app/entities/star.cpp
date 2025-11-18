#include "entities/star.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

#include <cmath> // make sure this is included at the top

namespace mcu_game
{

    bool Star::init(GameState &gameState)
    {
        if (!gameState.load_model(assets::baked::MeshId::Star, vertexId, triangleId))
            return false;
        if (!gameState.load_model(assets::baked::MeshId::Empty, emptyVertexId, emptyTriangleId))
            return false;

        const auto platformInstanceResp = gameState.gfx.createInstance(static_cast<uint8_t>(vertexId),
                                                                       static_cast<uint8_t>(triangleId),
                                                                       transform);
        if (!platformInstanceResp.isSuccess())
            return false;
        instanceId = platformInstanceResp.getInstanceId();

        animator.addAnimation(Animation{
            "Squash",
            {
                {true, vertexId, triangleId,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.6f},

                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.4f, 0.7f, 1.4f,
                 0.6f},

                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.0f},
            },
            true});

        return true;
    }

    void Star::update(float deltaTime, GameState &gameState)
    {
        if (!animationStarted)
        {
            startDelay -= deltaTime;
            if (startDelay <= 0.0f)
            {
                animator.playAnimation("Squash");
                animationStarted = true;
            }
        }

        if (animationStarted)
        {
            animator.update(deltaTime);
        }
    }

    void Star::render(GameState &gameState)
    {
        bool isVisible = gameState.playerPosition.y > transform.position.y + 35.0f;

        if (!isVisible && !lastIsVisible)
            return;

        if (!isVisible && lastIsVisible)
        {
            gameState.gfx.updateInstance(emptyVertexId, emptyTriangleId, instanceId, transform);
            lastIsVisible = isVisible;
            return;
        }

        const auto animState = animator.getCurrentAnimState(vertexId, triangleId, transform);
        gameState.gfx.updateInstance(animState.vertexId, animState.triangleId, instanceId, animState.transform);
        lastIsVisible = isVisible;
    }

}
