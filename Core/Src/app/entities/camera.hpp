#pragma once

#include "math.hpp"
#include <algorithm>
#include <cmath>
#include "entities/entity.hpp"

namespace mcu_game
{

    struct CameraConfig
    {
        float minPitch = -1.22f;       // radians (~-70 deg)
        float maxPitch = 1.22f;        // radians (~+70 deg)
        float distance = 7.0f;         // follow distance
        float heightOffset = 1.5f;     // look at point above player origin
        float yawSensitivity = 1.0f;   // multiplier for input yaw delta
        float pitchSensitivity = 1.0f; // multiplier for input pitch delta
        float smooth = 12.0f;          // higher = snappier
        float lookStep = 0.03f;
    };

    class Camera : public Entity
    {
    public:
        Camera(Vec3 position, Vec3 rotation)
        {
            transform.position = position;
            transform.rotation = rotation;
        }

        bool init(GameState &gameState) override;
        void update(float deltaTime, GameState &gameState) override;
        void render(GameState &gameState) override;

        Vec3 getForward() const { return forward_vector_from_yaw_pitch(transform.rotation.y, transform.rotation.x); }
        Vec3 getRight() const { return normalize(cross({0, 1, 0}, getForward())); }
        const Vec3 &getTarget() const { return target; }

    private:
        void updateSkyColor(float playerHeight);

        CameraConfig cameraConfig{};
        Rasterizer::Transform transform{};
        Vec3 target{0, 0, 0};
        uint8_t r, g, b;
    };

} // namespace mcu_game
