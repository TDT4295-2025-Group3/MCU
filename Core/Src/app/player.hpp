#pragma once

#include "math.hpp"
#include "input.hpp"

namespace mcu_game {

struct PlayerConfig {
    float moveSpeed = 4.0f;      // units per second for full input
    float airControlFactor = 0.3f; // reduced control in air
    float jumpVelocity = 6.0f;   // initial jump impulse (units/sec)
    float gravity = -9.8f;       // gravity acceleration (units/sec^2)
    float friction = 8.0f;       // ground friction (per second)
};

class Player {
public:
    Player() = default;

    void reset() {
        position = {0, 1.0f, 0};
        velocity = {0,0,0};
        grounded = false; // placed slightly above ground so will fall and settle
    }

    void update(const InputState &in, const class Camera &cam, float dt);

    const Vec3 &getPosition() const { return position; }
    const Vec3 &getVelocity() const { return velocity; }
    bool isGrounded() const { return grounded; }

private:
    Vec3 position{0,1.0f,0};
    Vec3 velocity{0,0,0};
    bool grounded{false};
    PlayerConfig playerConfig{};
};

} // namespace mcu_game
