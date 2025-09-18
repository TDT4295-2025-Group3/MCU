#pragma once

#include "math.hpp"
#include <algorithm>

namespace mcu_game {

struct CameraConfig {
    float minPitch = -1.2f; // radians (~-69 deg)
    float maxPitch =  1.2f; // radians
    float distance = 5.0f;  // follow distance
    float heightOffset = 1.5f; // look at point above player origin
    float yawSensitivity = 1.0f;   // multiplier for input yaw delta
    float pitchSensitivity = 1.0f; // multiplier for input pitch delta
    float smooth = 12.0f; // higher = snappier
};

class Camera {
public:
    void reset() {
        yaw = 0.0f; 
        pitch = 0.3f; // slight downward angle
        position = {0,0, -cameraConfig.distance};
        target = {0, cameraConfig.heightOffset, 0};
    }

    void update(float yawDelta, float pitchDelta, const class Player &player, float dt);

    const Vec3 &getPosition() const { return position; }
    Vec3 getForward() const { return forward_vector_from_yaw_pitch(yaw, pitch); }
    Vec3 getRight() const { return normalize(cross({0,1,0}, getForward())); }
    const Vec3 &getTarget() const { return target; }
    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }

private:
    CameraConfig cameraConfig{};
    float yaw{0};
    float pitch{0};
    Vec3 position{0,0,0};
    Vec3 target{0,0,0};
};

} // namespace mcu_game

