#include "entities/player.hpp"
#include "camera.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"
#include <cstddef>
#include <cstdio>

namespace mcu_game
{

    bool Player::init(GameState &gameState)
    {
        body.getTransform().position.y = 1.0f;
        body.setVelocity({0.0f, 0.0f, 0.0f});
        gameState.playerPosition = body.getTransform().position;

        if (!gameState.load_model(assets::baked::MeshId::PlayerIdle, vertexIdleId, triangleIdleId))
            return false;

        if (!gameState.load_model(assets::baked::MeshId::PlayerRun1, vertexRun1Id, triangleRun1Id))
            return false;

        if (!gameState.load_model(assets::baked::MeshId::PlayerRun2, vertexRun2Id, triangleRun2Id))
            return false;

        if (!gameState.load_model(assets::baked::MeshId::PlayerJumpUp, vertexJumpUpId, triangleJumpUpId))
            return false;

        if (!gameState.load_model(assets::baked::MeshId::PlayerJumpDown, vertexJumpDownId,
                                  triangleJumpDownId))
            return false;

        const auto playerInstanceResp = gameState.gfx.createInstance(static_cast<uint8_t>(vertexIdleId),
                                                                     static_cast<uint8_t>(triangleIdleId),
                                                                     body.getTransform());
        if (!playerInstanceResp.isSuccess())
            return false;
        instanceId = playerInstanceResp.getInstanceId();

        animator.addAnimation(Animation{
            "Idle",
            {
                {true, vertexIdleId, triangleIdleId,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.4f},

                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.1f, 0.96f, 1.1f,
                 0.4f},

                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.0f},
            },
            true});
        animator.addAnimation(Animation{
            "Run",
            {
                {true, vertexRun1Id, triangleRun1Id,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 0.97f, 1.0f,
                 0.08f},
                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.08f},
                {true, vertexIdleId, triangleIdleId,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 1.0f, 1.0f, 1.0f,
                 0.08f},
                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 0.94f, 1.0f,
                 0.08f},
                {true, vertexRun2Id, triangleRun2Id,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 1.0f, 1.0f, 1.0f,
                 0.08f},
                {false, 0, 0,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.08f},
                {true, vertexIdleId, triangleIdleId,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 0.97f, 1.0f,
                 0.08f},
            },
            true});
        animator.addAnimation(Animation{
            "JumpUp",
            {
                {true, vertexJumpUpId, triangleJumpUpId,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.2f},
            },
            true});
        animator.addAnimation(Animation{
            "JumpDown",
            {
                {true, vertexJumpDownId, triangleJumpDownId,
                 false, 0.0f, 0.0f, 0.0f,
                 false, 0.0f, 0.0f, 0.0f,
                 true, 1.0f, 1.0f, 1.0f,
                 0.2f},
            },
            true});

        animator.playAnimation("Idle");

        return true;
    }

    void Player::update(float deltaTime, GameState &gameState)
    {
        constexpr float PI = 3.14159265359f;

        // Forward vector in XZ-plane
        Vec3 camForward = gameState.cameraForward;
        Vec3 forward{camForward.x, 0.0f, camForward.z};
        if (length_sq(forward) < 1e-6f)
        {
            forward = {0.0f, 0.0f, 1.0f};
        }
        else
        {
            forward = normalize(forward);
        }

        // Right vector in XZ-plane
        Vec3 right = cross({0.0f, 1.0f, 0.0f}, forward);
        if (length_sq(right) < 1e-6f)
        {
            right = {1.0f, 0.0f, 0.0f};
        }
        else
        {
            right = normalize(right);
        }

        // Input vector in camera space
        Vec2 runInput = gameState.input.getRunInput();
        Vec3 inputDir = forward * runInput.y + right * runInput.x;
        const bool hasInput = length_sq(inputDir) > 1e-6f;
        Vec3 desiredMoveDir = hasInput ? normalize(inputDir) : Vec3{0, 0, 0}; // normalized desired move direction

        if (hasInput && body.isGrounded())
        {
            animator.playAnimation("Run");
        }
        else if (!hasInput && body.isGrounded())
        {
            animator.playAnimation("Idle");
        }
        else if (!body.isGrounded())
        {
            if (body.getVelocity().y > 0.0f)
            {
                animator.playAnimation("JumpUp");
            }
            else
            {
                animator.playAnimation("JumpDown");
            }
        }

        // Use input direction for orientation
        // Fall back to current horizontal velocity when sliding
        Vec3 orientDir = desiredMoveDir;
        if (!hasInput)
        {
            Vec3 horizVel{body.getVelocity().x, 0.0f, body.getVelocity().z};
            if (length_sq(horizVel) > 1e-6f)
            {
                orientDir = normalize(horizVel);
            }
        }

        // Update player yaw to face orientDir
        if (length_sq(orientDir) > 1e-6f)
        {
            float desiredYaw = std::atan2(-orientDir.x, orientDir.z);
            float yawDelta = desiredYaw - body.getTransform().rotation.y;
            if (yawDelta > PI)
                yawDelta -= 2.0f * PI;
            if (yawDelta < -PI)
                yawDelta += 2.0f * PI;
            const float maxStep = playerConfig.turnSpeed * deltaTime;
            yawDelta = std::clamp(yawDelta, -maxStep, maxStep);

            if (body.isGrounded())
            {
                body.getTransform().rotation.y += yawDelta;
                if (body.getTransform().rotation.y > PI)
                    body.getTransform().rotation.y -= 2.0f * PI;
                if (body.getTransform().rotation.y < -PI)
                    body.getTransform().rotation.y += 2.0f * PI;
            }
        }

        float control = body.isGrounded() ? 1.0f : playerConfig.airControlFactor; // movement control in air is different from ground
        Vec3 targetVel = desiredMoveDir * (playerConfig.moveSpeed * control);     // desired target velocity

        // Accelerate towards target velocity (simple critically damped style)
        // Use friction on ground when no input
        if (body.isGrounded())
        {
            // Blend velocity horizontally
            Vec3 horizVel{body.getVelocity().x, 0, body.getVelocity().z};
            Vec3 newHoriz = lerp(horizVel, targetVel, 1.0f - std::exp(-playerConfig.friction * deltaTime));
            body.setVelocity({newHoriz.x, body.getVelocity().y, newHoriz.z});
        }
        else
        {
            // Air control limited: simply approach target
            Vec3 vel = body.getVelocity();
            vel.x = lerp(vel.x, targetVel.x, control * deltaTime * 2.0f);
            vel.z = lerp(vel.z, targetVel.z, control * deltaTime * 2.0f);
            body.setVelocity(vel);
        }

        // Jump
        bool jumpPressed = gameState.input.getJump();
        if (jumpPressed && body.isGrounded())
        {
            Vec3 vel = body.getVelocity();
            vel.y = playerConfig.jumpVelocity;
            body.setVelocity(vel);
        }

        // Gravity
        Vec3 vel = body.getVelocity();

        // Base gravity
        float g = playerConfig.gravity;

        if (vel.y < 0.0f)
            g *= playerConfig.fallGravityMultiplier;
        else if (!jumpPressed && vel.y > 0.0f)
            g *= playerConfig.lowJumpGravityMultiplier;

        vel.y += g * deltaTime;
        body.setVelocity(vel);

        animator.update(deltaTime);
        body.update(deltaTime, gameState.boxColliders);

        if (getPosition().y < -10.0f)
        {
            body.setBottomPosition(startPosition);
            body.setVelocity({0.0f, 0.0f, 0.0f});
        }

        gameState.playerPosition = getPosition();

        // rumble for player velocity
        float speedY = getVelocity().y;
        if (speedY < playerConfig.fall_rumble_threshold)
            gameState.input.setRumble(std::min(1.0f, std::abs(speedY - playerConfig.fall_rumble_threshold) / 20.0f));
        else
            gameState.input.clearRumble();
    }

    void Player::render(GameState &gameState)
    {
        const Keyframe &keyframe = animator.getCurrentKeyframe();

        Rasterizer::Transform animTransform = body.getTransform();
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

        gameState.gfx.updateInstance(keyframe.vertexId, keyframe.triangleId, instanceId, animTransform);
    }

} // namespace mcu_game
