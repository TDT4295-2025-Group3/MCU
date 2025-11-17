#include "entities/fire.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

namespace mcu_game
{

    bool Fire::init(Rasterizer::IRasterizer &gfx, GameState &gameState)
    {
        if (!createBuffersWithFallback(gfx, assets::baked::MeshId::Fire,
                                       vertexId,
                                       triangleId))
            return false;

        const auto platformInstanceResp = gfx.createInstance(static_cast<uint8_t>(vertexId),
                                                             static_cast<uint8_t>(triangleId),
                                                             transform);
        if (!platformInstanceResp.isSuccess())
            return false;
        instanceId = platformInstanceResp.getInstanceId();

        animator.addAnimation(Animation{
            "Fire",
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
            true});

        animator.playAnimation("Fire");

        return true;
    }

    void Fire::update(const InputState &in, float deltaTime, GameState &gameState)
    {
        animator.update(deltaTime);
    }

    void Fire::render(Rasterizer::IRasterizer &gfx)
    {
        const Keyframe &keyframe = animator.getCurrentKeyframe();

        Rasterizer::Transform animTransform = transform;
        if (keyframe.useTranslation)
        {
            animTransform.position.x += keyframe.translationX;
            animTransform.position.y += keyframe.translationY;
            animTransform.position.z += keyframe.translationZ;
        }

        if (keyframe.useRotation)
        {
            animTransform.rotation.x += keyframe.rotationX;
            animTransform.rotation.y += keyframe.rotationY;
            animTransform.rotation.z += keyframe.rotationZ;
        }
        if (keyframe.useScale)
        {
            animTransform.scale.x *= keyframe.scaleX;
            animTransform.scale.y *= keyframe.scaleY;
            animTransform.scale.z *= keyframe.scaleZ;
        }

        gfx.updateInstance(keyframe.vertexId, keyframe.triangleId, instanceId, animTransform);
    }

} // namespace mcu_game
