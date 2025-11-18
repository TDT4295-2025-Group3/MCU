#include "entities/cloud.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

#include <cmath> // make sure this is included at the top

namespace mcu_game
{

    bool Cloud::init(GameState &gameState)
    {
        if (!gameState.load_model(assets::baked::MeshId::Cloud, vertexId, triangleId))
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
                 0.8f},

                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.1f, 0.96f, 1.1f,
                 0.8f},

                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.0f},
            },
            true});

        animator.playAnimation("Squash");
        return true;
    }

    void Cloud::update(float deltaTime, GameState &gameState)
    {
        animator.update(deltaTime);

        // --- advance along the square path ---
        phase += speed * deltaTime; // speed = loops per second
        if (phase >= 1.0f)
            phase -= std::floor(phase); // wrap to [0,1)

        float t = phase * 4.0f;
        int edge = static_cast<int>(t); // 0,1,2,3
        float u = t - edge;             // 0..1 along this edge

        auto lerp = [](float a, float b, float t)
        {
            return a + (b - a) * t;
        };

        Vec3 a, b;
        float targetYaw = 0.0f; // radians

        constexpr float PI = 3.14159265358979323846f;

        switch (edge)
        {
        case 0: // bottom edge: left -> right  ( +X )
            a = {center.x - halfSize, center.y, center.z - halfSize};
            b = {center.x + halfSize, center.y, center.z - halfSize};
            targetYaw = 0.0f; // facing +X
            break;
        case 1: // right edge: bottom -> top  ( +Z )
            a = {center.x + halfSize, center.y, center.z - halfSize};
            b = {center.x + halfSize, center.y, center.z + halfSize};
            targetYaw = 0.5f * PI; // +90° -> +Z
            break;
        case 2: // top edge: right -> left    ( -X )
            a = {center.x + halfSize, center.y, center.z + halfSize};
            b = {center.x - halfSize, center.y, center.z + halfSize};
            targetYaw = PI; // 180° -> -X
            break;
        default: // 3: left edge: top -> bottom ( -Z )
            a = {center.x - halfSize, center.y, center.z + halfSize};
            b = {center.x - halfSize, center.y, center.z - halfSize};
            targetYaw = 1.5f * PI; // 270° -> -Z
            break;
        }

        // Interpolate position along current edge
        transform.position.x = lerp(a.x, b.x, u);
        transform.position.y = center.y;
        transform.position.z = lerp(a.z, b.z, u);

        // --- smooth rotation in radians ---

        // shortest-angle difference in [-PI, PI]
        auto shortestAngle = [PI](float from, float to)
        {
            float diff = to - from;
            while (diff > PI)
                diff -= 2.0f * PI;
            while (diff < -PI)
                diff += 2.0f * PI;
            return diff;
        };

        float currentYaw = transform.rotation.y; // radians
        float diff = shortestAngle(currentYaw, targetYaw);

        // how fast it turns into the new direction
        const float rotationSmooth = 1.0f; // tweak this
        float factor = std::clamp(deltaTime * rotationSmooth, 0.0f, 1.0f);

        currentYaw += diff * factor;

        // keep angle somewhat normalized (optional but nice)
        if (currentYaw > PI)
            currentYaw -= 2.0f * PI;
        if (currentYaw < -PI)
            currentYaw += 2.0f * PI;

        transform.rotation.y = currentYaw;
    }

    void Cloud::render(GameState &gameState)
    {
        bool isVisible = gameState.playerPosition.y < transform.position.y + 25.0f;

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
