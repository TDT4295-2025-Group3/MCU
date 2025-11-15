// transform.hpp
#pragma once
#include "math.hpp"
#include "platform/irasterizer.hpp" // for Rasterizer::Transform

namespace mcu_game
{
    struct Transform
    {
        Vec3 position{0.0f, 0.0f, 0.0f};
        float yaw{0.0f};
        float pitch{0.0f};
        float roll{0.0f};
        Vec3 scale{1.0f, 1.0f, 1.0f};

        Vec3 forward() const
        {
            // same convention you use for camera
            // yaw around Y, pitch around X
            const float cy = std::cos(yaw);
            const float sy = std::sin(yaw);
            const float cp = std::cos(pitch);
            const float sp = std::sin(pitch);

            // forward in your world (Z forward, X right, Y up)
            return Vec3{
                sy * cp,
                -sp,
                cy * cp};
        }

        Vec3 right() const
        {
            // cross(up, forward) or derive analytically
            Vec3 f = forward();
            return normalize(cross(Vec3{0.0f, 1.0f, 0.0f}, f));
        }

        Rasterizer::Transform toRasterizer() const
        {
            return Rasterizer::Transform{
                position.x, position.y, position.z,
                // your rasterizer uses rotX, rotY, rotZ:
                pitch, yaw, roll,
                scale.x, scale.y, scale.z};
        }
    };

} // namespace mcu_game
