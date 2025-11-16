#include "entities/camera.hpp"
#include "entities/player.hpp"

namespace mcu_game
{

    bool Camera::init(Rasterizer::IRasterizer &gfx, GameState &gameState)
    {
        gameState.cameraForward = getForward();

        target = {0, cameraConfig.heightOffset, 0};
        transform.position = target - getForward() * cameraConfig.distance;
        transform.rotation = {0.3f, 0.0f, 0.0f};

        gfx.updateCamera(r, g, b, transform);

        return true;
    }

    void Camera::update(const InputState &in, float deltaTime, GameState &gameState)
    {
        // Apply look deltas
        transform.rotation.y += in.lookYawDelta * cameraConfig.yawSensitivity;
        transform.rotation.x += in.lookPitchDelta * cameraConfig.pitchSensitivity;
        if (transform.rotation.x < cameraConfig.minPitch)
            transform.rotation.x = cameraConfig.minPitch;
        if (transform.rotation.x > cameraConfig.maxPitch)
            transform.rotation.x = cameraConfig.maxPitch;

        // Desired target (look-at) point: player position + height offset
        Vec3 desiredTarget = gameState.playerPosition;
        desiredTarget.y += cameraConfig.heightOffset;

        // Smooth target toward player to reduce jitter
        float smoothFactor = (deltaTime > 0.0f) ? 1.0f - std::exp(-cameraConfig.smooth * deltaTime) : 1.0f;
        target = lerp(target, desiredTarget, smoothFactor);

        // Place camera on orbit sphere defined by yaw/pitch around the target
        const Vec3 fwd = forward_vector_from_yaw_pitch(transform.rotation.y, transform.rotation.x);
        transform.position = target - fwd * cameraConfig.distance;

        gameState.cameraForward = getForward();
    }

    void Camera::render(Rasterizer::IRasterizer &gfx)
    {
        gfx.updateCamera(r, g, b, transform);
    }

} // namespace mcu_game
