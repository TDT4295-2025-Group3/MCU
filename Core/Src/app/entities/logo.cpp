#include "entities/logo.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

namespace mcu_game
{

    bool Logo::init(GameState &gameState)
    {
        if (!gameState.load_model(assets::baked::MeshId::Logo, vertexLogoId, triangleLogoId))
            return false;

        if (!gameState.load_model(assets::baked::MeshId::PressXToStart, vertexPressXId, trianglePressXId))
            return false;

        if (!gameState.load_model(assets::baked::MeshId::Empty, emptyVertexId, emptyTriangleId))
            return false;

        const auto logoInstanceResp = gameState.gfx.createInstance(static_cast<uint8_t>(vertexLogoId),
                                                                   static_cast<uint8_t>(triangleLogoId),
                                                                   transform);
        if (!logoInstanceResp.isSuccess())
            return false;
        instanceLogoId = logoInstanceResp.getInstanceId();

        const auto pressXInstanceResp = gameState.gfx.createInstance(static_cast<uint8_t>(vertexPressXId),
                                                                     static_cast<uint8_t>(trianglePressXId),
                                                                     transform);
        if (!pressXInstanceResp.isSuccess())
            return false;
        instancePressXId = pressXInstanceResp.getInstanceId();

        animator.addAnimation(Animation{
            "Float",
            {
                {false, 0, 0,
                 true, 0.0f, -0.14f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 1.0f, 1.0f, 1.0f,
                 1.5f},

                {false, 0, 0,
                 true, 0.0f, 0.14f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 1.0f, 1.0f, 1.0f,
                 1.5f},

                {false, 0, 0,
                 true, 0.0f, -0.14f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 1.0f, 1.0f, 1.0f,
                 0.0f},
            },
            true});

        animator.addAnimation(Animation{
            "Empty",
            {
                {true, emptyVertexId, emptyTriangleId,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 1.0f, 1.0f, 1.0f,
                 0.1f},
            },
        });

        animator.playAnimation("Float");
        return true;
    }

    void Logo::update(float deltaTime, GameState &gameState)
    {
        if (gameState.isMenuActive)
            animator.playAnimation("Float");
        else
            animator.playAnimation("Empty");
        animator.update(deltaTime);
    }

    void Logo::render(GameState &gameState)
    {
        const auto animStateLogo = animator.getCurrentAnimState(vertexLogoId, triangleLogoId, transform);
        const auto animStatePressX = animator.getCurrentAnimState(vertexPressXId, trianglePressXId,
                                                                  transform);
        gameState.gfx.updateInstance(animStateLogo.vertexId, animStateLogo.triangleId, instanceLogoId, animStateLogo.transform);
        gameState.gfx.updateInstance(animStatePressX.vertexId, animStatePressX.triangleId, instancePressXId, animStatePressX.transform);
    }
}
