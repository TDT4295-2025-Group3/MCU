#include "camera.hpp"
#include "player.hpp"

namespace mcu_game {

void Camera::update(float yawDelta, float pitchDelta, const Player &player, float dt) {
    // Apply look deltas
    yaw += yawDelta * cameraConfig.yawSensitivity;
    pitch += pitchDelta * cameraConfig.pitchSensitivity;
    if (pitch < cameraConfig.minPitch) pitch = cameraConfig.minPitch;
    if (pitch > cameraConfig.maxPitch) pitch = cameraConfig.maxPitch;

    // Desired target (look-at) point: player position + height offset
    Vec3 desiredTarget = player.getPosition();
    desiredTarget.y += cameraConfig.heightOffset;

    // Smooth target
    // Exponential smoothing: newTarget = lerp(oldTarget, desiredTarget, 1 - exp(-smooth * dt))
    target = lerp(target, desiredTarget, 1.0f - std::exp(-cameraConfig.smooth * dt));

    // Desired camera position is behind target along forward vector
    Vec3 fwd = forward_vector_from_yaw_pitch(yaw, pitch);
    Vec3 desiredPos = target - fwd * cameraConfig.distance;
    position = lerp(position, desiredPos, 1.0f - std::exp(-cameraConfig.smooth * dt));
}

} // namespace mcu_game
