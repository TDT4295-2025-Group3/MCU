#include "player.hpp"
#include "camera.hpp"

#include <algorithm>
#include <cmath>
#include "game_model_loader.hpp"

namespace mcu_game
{

    bool Player::init(Rasterizer::IRasterizer &gfx, GameState &gameState)
    {
        transform = Rasterizer::Transform();
        transform.position.y = 1.0f;
        velocity = {0, 0, 0};
        grounded = false; // placed slightly above ground so will fall and settle

        if (!createBuffersWithFallback(gfx, "player.obj",
                                       assets::baked::MeshId::Player,
                                       vertexId,
                                       triangleId,
                                       true))
        {
            std::printf("Player geometry unavailable, aborting init\n");
            return false;
        }

        const auto playerInstanceResp = gfx.createInstance(static_cast<uint8_t>(vertexId),
                                                           static_cast<uint8_t>(triangleId),
                                                           transform);
        if (!playerInstanceResp.isSuccess())
            return false;
        instanceId = playerInstanceResp.getInstanceId();
        return true;
    }

    void Player::update(const InputState &in, float deltaTime, GameState &gameState)
    {
        constexpr float PI = 3.14159265359f;

        // Players forward direction is aligned with camera forward/right
        // This ensures movement input is relative to the camera's orientation

        // Forward vector in XZ-plane
        Vec3 camForward = gameState.camera.getForward();
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
        Vec3 inputDir = forward * in.moveZ + right * in.moveX;
        const bool hasInput = length_sq(inputDir) > 1e-6f;
        Vec3 desiredMoveDir = hasInput ? normalize(inputDir) : Vec3{0, 0, 0}; // normalized desired move direction

        // Use input direction for orientation
        // Fall back to current horizontal velocity when sliding
        Vec3 orientDir = desiredMoveDir;
        if (!hasInput)
        {
            Vec3 horizVel{velocity.x, 0.0f, velocity.z};
            if (length_sq(horizVel) > 1e-6f)
            {
                orientDir = normalize(horizVel);
            }
        }

        // Update player yaw to face orientDir
        if (length_sq(orientDir) > 1e-6f)
        {
            float desiredYaw = std::atan2(-orientDir.x, orientDir.z);
            float yawDelta = desiredYaw - transform.rotation.y;
            if (yawDelta > PI)
                yawDelta -= 2.0f * PI;
            if (yawDelta < -PI)
                yawDelta += 2.0f * PI;
            const float maxStep = playerConfig.turnSpeed * deltaTime;
            yawDelta = std::clamp(yawDelta, -maxStep, maxStep);
            transform.rotation.y += yawDelta;
            if (transform.rotation.y > PI)
                transform.rotation.y -= 2.0f * PI;
            if (transform.rotation.y < -PI)
                transform.rotation.y += 2.0f * PI;
        }

        float control = grounded ? 1.0f : playerConfig.airControlFactor;      // movement control in air is different from ground
        Vec3 targetVel = desiredMoveDir * (playerConfig.moveSpeed * control); // desired target velocity

        // Accelerate towards target velocity (simple critically damped style)
        // Use friction on ground when no input
        if (grounded)
        {
            // Blend velocity horizontally
            Vec3 horizVel{velocity.x, 0, velocity.z};
            Vec3 newHoriz = lerp(horizVel, targetVel, 1.0f - std::exp(-playerConfig.friction * deltaTime));
            velocity.x = newHoriz.x;
            velocity.z = newHoriz.z;
        }
        else
        {
            // Air control limited: simply approach target
            velocity.x = lerp(velocity.x, targetVel.x, control * deltaTime * 2.0f);
            velocity.z = lerp(velocity.z, targetVel.z, control * deltaTime * 2.0f);
        }

        // Jump
        // TODO: coyote time, variable jump height
        if (in.jump && grounded)
        {
            velocity.y = playerConfig.jumpVelocity;
            grounded = false;
        }

        // Gravity
        velocity.y += playerConfig.gravity * deltaTime;

        // Integrate
        transform.position += velocity * deltaTime;

        // Assume no longer grounded
        // Collision system will set it back if still on ground
        grounded = false;
    }

    void Player::render(Rasterizer::IRasterizer &gfx)
    {
        if (vertexId == 0xFF || triangleId == 0xFF)
            return;

        gfx.updateInstance(vertexId, triangleId, instanceId, transform);
    }

    void Player::landOn(float surfaceY)
    {
        transform.position.y = surfaceY;
        if (velocity.y < 0.0f)
        {
            velocity.y = 0.0f;
        }
        grounded = true;
    }

    void Player::applyCollisionResult(const Vec3 &newPosition, const Vec3 &newVelocity, bool groundedState)
    {
        transform.position = newPosition;
        velocity = newVelocity;
        grounded = groundedState;
    }

} // namespace mcu_game
