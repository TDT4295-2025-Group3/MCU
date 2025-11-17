#include "entities/camera.hpp"
#include "entities/player.hpp"

// Helper functions for camera sky color calculation
namespace
{
    using mcu_game::lerp;
    using mcu_game::Vec3;

    // Returns the height value scaled to [0, 1] for the provided range
    float normalizedHeightValue(float heightValue, float minVal, float maxVal)
    {
        if (maxVal <= minVal)
            return 0.0f;
        const float t = (heightValue - minVal) / (maxVal - minVal);
        return std::clamp(t, 0.0f, 1.0f);
    }

    // Convert a color channel in [0,255] to a 4-bit nibble [0,15]
    uint8_t toNibble(float channel)
    {
        channel = std::clamp(channel, 0.0f, 255.0f);
        const float scaled = channel * (15.0f / 255.0f);
        return static_cast<uint8_t>(std::lround(scaled));
    }
} // namespace

namespace mcu_game
{

    bool Camera::init(GameState &gameState)
    {
        gameState.cameraForward = getForward();

        target = {0, cameraConfig.heightOffset, 0};
        transform.position = target - getForward() * cameraConfig.distance;
        transform.rotation = {0.3f, 0.0f, 0.0f};

        updateSkyColor(gameState.playerPosition.y);
        gameState.gfx.updateCamera(r, g, b, transform);

        return true;
    }

    void Camera::update(float deltaTime, GameState &gameState)
    {
        // Apply look deltas
        Vec2 lookInput = gameState.input.getLookInput();
        transform.rotation.y += lookInput.x * cameraConfig.lookStep * cameraConfig.yawSensitivity;
        transform.rotation.x += lookInput.y * cameraConfig.lookStep * cameraConfig.pitchSensitivity;
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
        updateSkyColor(gameState.playerPosition.y);
    }

    void Camera::render(GameState &gameState)
    {
        gameState.gfx.updateCamera(r, g, b, transform);
    }

    void Camera::updateSkyColor(float playerHeight)
    {
        constexpr float startLevel = 0.0f;      // ground level
        constexpr float darkeningLevel = 50.0f; // begins to get darker
        constexpr float spaceLevel = 120.0f;    // goes to black in space

        const Vec3 lightSky{135.0f, 206.0f, 235.0f};
        const Vec3 darkSky{25.0f, 70.0f, 130.0f};
        const Vec3 space{0.0f, 0.0f, 0.0f};

        const float clampedHeight = std::max(playerHeight, startLevel);

        Vec3 result = lightSky;

        if (clampedHeight <= darkeningLevel) // below darkening level
        {
            const float heightFraction = normalizedHeightValue(clampedHeight, startLevel, darkeningLevel);
            const float eased = std::pow(heightFraction, 3.0f); // Ease-in so colour shifts slowly close to the surface.
            result = lerp(lightSky, darkSky, eased);
        }
        else if (clampedHeight < spaceLevel) // between darkening and space
        {
            const float heightFraction = normalizedHeightValue(clampedHeight, darkeningLevel, spaceLevel);
            const float eased = 1.0f - std::pow(1.0f - heightFraction, 3.0f); // Ease-out to linger in dark blue before fading fully.
            result = lerp(darkSky, space, eased);
        }
        else // in space
        {
            result = space;
        }

        r = toNibble(result.x);
        g = toNibble(result.y);
        b = toNibble(result.z);
    }

} // namespace mcu_game
