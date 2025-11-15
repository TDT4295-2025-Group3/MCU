#include "player.hpp"
#include "camera.hpp"

#include <algorithm>
#include <cmath>

namespace mcu_game {

void Player::update(const InputState &in, const Camera &cam, float dt) {
    constexpr float PI = 3.14159265359f;

    // Players forward direction is aligned with camera forward/right
    // This ensures movement input is relative to the camera's orientation

    // Forward vector in XZ-plane
    Vec3 camForward = cam.getForward();
    Vec3 forward{camForward.x, 0.0f, camForward.z};
    if (length_sq(forward) < 1e-6f) {
        forward = {0.0f, 0.0f, 1.0f};
    } else {
        forward = normalize(forward);
    }

    // Right vector in XZ-plane
    Vec3 right = cross({0.0f, 1.0f, 0.0f}, forward);
    if (length_sq(right) < 1e-6f) {
        right = {1.0f, 0.0f, 0.0f};
    } else {
        right = normalize(right);
    }

    // Input vector in camera space
    Vec3 inputDir = forward * in.moveZ + -1*right * in.moveX;
    const bool hasInput = length_sq(inputDir) > 1e-6f;
    Vec3 desiredMoveDir = hasInput ? normalize(inputDir) : Vec3{0, 0, 0}; // normalized desired move direction

    // Use input direction for orientation
    // Fall back to current horizontal velocity when sliding
    Vec3 orientDir = desiredMoveDir;
    if (!hasInput) {
        Vec3 horizVel{velocity.x, 0.0f, velocity.z};
        if (length_sq(horizVel) > 1e-6f) {
            orientDir = normalize(horizVel);
        }
    }

    // Update player yaw to face orientDir
    if (length_sq(orientDir) > 1e-6f) {
        float desiredYaw = std::atan2(orientDir.x, orientDir.z);
        float yawDelta = desiredYaw - yaw;
        if (yawDelta > PI) yawDelta -= 2.0f * PI;
        if (yawDelta < -PI) yawDelta += 2.0f * PI;
        const float maxStep = playerConfig.turnSpeed * dt;
        yawDelta = std::clamp(yawDelta, -maxStep, maxStep);
        yaw += yawDelta;
        if (yaw > PI) yaw -= 2.0f * PI;
        if (yaw < -PI) yaw += 2.0f * PI;
    }

    float control = grounded ? 1.0f : playerConfig.airControlFactor; // movement control in air is different from ground
    Vec3 targetVel = desiredMoveDir * (playerConfig.moveSpeed * control); // desired target velocity

    // Accelerate towards target velocity (simple critically damped style)
    // Use friction on ground when no input
    if (grounded) {
        // Blend velocity horizontally
        Vec3 horizVel{velocity.x, 0, velocity.z};
        Vec3 newHoriz = lerp(horizVel, targetVel, 1.0f - std::exp(-playerConfig.friction * dt));
        velocity.x = newHoriz.x;
        velocity.z = newHoriz.z;
    } else {
        // Air control limited: simply approach target
        velocity.x = lerp(velocity.x, targetVel.x, control * dt * 2.0f);
        velocity.z = lerp(velocity.z, targetVel.z, control * dt * 2.0f);
    }

    // Jump
    // TODO: coyote time, variable jump height
    if (in.jump && grounded) {
        velocity.y = playerConfig.jumpVelocity;
        grounded = false;
    }

    // Gravity
    velocity.y += playerConfig.gravity * dt;

    // Integrate
    position += velocity * dt;

    // Assume no longer grounded
    // Collision system will set it back if still on ground
    grounded = false;
}

void Player::landOn(float surfaceY) {
    position.y = surfaceY;
    if (velocity.y < 0.0f) {
        velocity.y = 0.0f;
    }
    grounded = true;
}

void Player::applyCollisionResult(const Vec3& newPosition, const Vec3& newVelocity, bool groundedState) {
    position = newPosition;
    velocity = newVelocity;
    grounded = groundedState;
}

} // namespace mcu_game
