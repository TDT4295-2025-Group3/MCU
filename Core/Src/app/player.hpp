#pragma once

#include "math.hpp"
#include "input.hpp"

namespace mcu_game {

struct PlayerConfig {
    float moveSpeed = 4.0f;      // units per second for full input
    float airControlFactor = 1.2f; // enhanced control in air (greater than ground control)
    float jumpVelocity = 6.0f;   // initial jump impulse (units/sec)
    float gravity = -9.8f;       // gravity acceleration (units/sec^2)
    float friction = 8.0f;       // ground friction (per second)
    float turnSpeed = 10.0f;     // radians per second player can rotate toward movement
};

class Player {
public:
    Player() = default;

    void reset() {
        position = {0, 1.0f, 0};
        velocity = {0,0,0};
        grounded = false; // placed slightly above ground so will fall and settle
        yaw = 0.0f;
    }

    void update(const InputState &in, const class Camera &cam, float dt);
    void landOn(float surfaceY);
    void applyCollisionResult(const Vec3& newPosition, const Vec3& newVelocity, bool groundedState);

    const Vec3 &getPosition() const { return position; }
    const Vec3 &getVelocity() const { return velocity; }
    bool isGrounded() const { return grounded; }
    float getYaw() const { return yaw; }

private:
    Vec3 position{0,1.0f,0};
    Vec3 velocity{0,0,0};
    bool grounded{false};
    float yaw{0.0f};
    PlayerConfig playerConfig{};
};

} // namespace mcu_game
