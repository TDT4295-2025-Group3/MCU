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
    };

    class Camera : public Entity
    {
    public:
        Camera(uint8_t r_, uint8_t g_, uint8_t b_)
            : r(r_), g(g_), b(b_)
        {
        }

        bool init(Rasterizer::IRasterizer &gfx, GameState &gameState) override;
        void update(const InputState &in, float deltaTime, GameState &gameState) override;
        void render(Rasterizer::IRasterizer &gfx) override;

        Vec3 getForward() const { return forward_vector_from_yaw_pitch(transform.rotation.y, transform.rotation.x); }
        Vec3 getRight() const { return normalize(cross({0, 1, 0}, getForward())); }
        const Vec3 &getTarget() const { return target; }

    private:
        CameraConfig cameraConfig{};
        Rasterizer::Transform transform{};
        Vec3 target{0, 0, 0};
        uint8_t r, g, b;
    };

} // namespace mcu_game
