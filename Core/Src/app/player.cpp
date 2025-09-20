#include "player.hpp"
#include "camera.hpp"

namespace mcu_game {

void Player::update(const InputState &in, const Camera &cam, float dt) {
    // Horizontal movement direction derived from camera yaw (ignore pitch)
    // Project camera forward onto XZ plane
    auto camForward = cam.getForward();
    Vec3 forward = { camForward.x, 0.0f, camForward.z };
    if (length_sq(forward) < 1e-6f) forward = {0,0,1};
    forward = normalize(forward);
    Vec3 right = normalize(cross({0,1,0}, forward)); // right-handed

    Vec3 desiredDir = forward * in.moveZ + right * in.moveX; // camera-relative
    if (length_sq(desiredDir) > 1e-6f) desiredDir = normalize(desiredDir);

    float control = grounded ? 1.0f : playerConfig.airControlFactor;
    Vec3 targetVel = desiredDir * (playerConfig.moveSpeed * control);

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
    if (in.jump && grounded) {
        velocity.y = playerConfig.jumpVelocity;
        grounded = false;
    }

    // Gravity
    velocity.y += playerConfig.gravity * dt;

    // Integrate
    position += velocity * dt;

    // Very primitive ground collision at y=0
    if (position.y < 0.0f) {
        position.y = 0.0f;
        if (velocity.y < 0) velocity.y = 0;
        grounded = true;
    } else {
        grounded = false;
    }
}

} // namespace mcu_game
