#include "camera.hpp"
#include "entities/player.hpp"

namespace mcu_game
{

    void Camera::update(float yawDelta, float pitchDelta, const Player &player, float dt)
    {
        // Apply look deltas
        yaw += yawDelta * cameraConfig.yawSensitivity;
        pitch += pitchDelta * cameraConfig.pitchSensitivity;
        if (pitch < cameraConfig.minPitch)
            pitch = cameraConfig.minPitch;
        if (pitch > cameraConfig.maxPitch)
            pitch = cameraConfig.maxPitch;

        // Desired target (look-at) point: player position + height offset
        Vec3 desiredTarget = player.getPosition();
        desiredTarget.y += cameraConfig.heightOffset;

        // Smooth target toward player to reduce jitter
        float smoothFactor = (dt > 0.0f) ? 1.0f - std::exp(-cameraConfig.smooth * dt) : 1.0f;
        target = lerp(target, desiredTarget, smoothFactor);

        // Place camera on orbit sphere defined by yaw/pitch around the target
        const Vec3 fwd = forward_vector_from_yaw_pitch(yaw, pitch);
        position = target - fwd * cameraConfig.distance;
    }

} // namespace mcu_game
