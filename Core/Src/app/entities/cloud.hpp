#pragma once

#include "math.hpp"

#include "entities/entity.hpp"
#include "anim/anim.hpp"
#include "rigidbody.hpp"
#include "game_state.hpp"
#include <cstdlib>
#include <cmath>
#include <cstdlib> // std::rand, RAND_MAX
#include <cmath>   // std::fabs, std::clamp

namespace mcu_game
{
    class Cloud : public Entity
    {
    public:
        // startPos = initial position (also determines square size)
        Cloud(Vec3 startPos, float rotation)
            : transform{startPos.x, startPos.y, startPos.z,
                        0.0f, rotation, 0.0f,
                        1.0f, 1.0f, 1.0f},
              center{0.0f, startPos.y, 0.0f}, // square around world center (x,z) = (0,0)
              halfSize(1.0f),
              phase(0.0f),
              speed(0.0f)
        {
            constexpr float PI = 3.14159265358979323846f;

            // ---- 1) Determine square size ----
            Vec3 offset{
                transform.position.x - center.x,
                0.0f,
                transform.position.z - center.z};

            float ax = std::fabs(offset.x);
            float az = std::fabs(offset.z);

            halfSize = std::max(ax, az);
            if (halfSize < 0.01f)
                halfSize = 1.0f; // fallback if too close to center

            float edgeLen = 2.0f * halfSize;
            float perim = 4.0f * edgeLen; // = 8 * halfSize

            // ---- 2) Figure out which edge and how far along it ----
            float dAlong = 0.0f; // distance along perimeter
            int edgeIndex = 0;   // 0..3, must match update()

            if (az >= ax)
            {
                // closer to top/bottom edges
                if (offset.z < 0.0f)
                {
                    // Edge 0: bottom edge, from left(-x) to right(+x)
                    float s = (offset.x + halfSize) / (2.0f * halfSize); // 0..1
                    s = std::clamp(s, 0.0f, 1.0f);
                    edgeIndex = 0;
                    dAlong = 0.0f * edgeLen + s * edgeLen;
                }
                else
                {
                    // Edge 2: top edge, from right(+x) to left(-x)
                    float s = (halfSize - offset.x) / (2.0f * halfSize); // 0..1
                    s = std::clamp(s, 0.0f, 1.0f);
                    edgeIndex = 2;
                    dAlong = 2.0f * edgeLen + s * edgeLen;
                }
            }
            else
            {
                // closer to left/right edges
                if (offset.x > 0.0f)
                {
                    // Edge 1: right edge, from bottom(-z) to top(+z)
                    float s = (offset.z + halfSize) / (2.0f * halfSize); // 0..1
                    s = std::clamp(s, 0.0f, 1.0f);
                    edgeIndex = 1;
                    dAlong = 1.0f * edgeLen + s * edgeLen;
                }
                else
                {
                    // Edge 3: left edge, from top(+z) to bottom(-z)
                    float s = (halfSize - offset.z) / (2.0f * halfSize); // 0..1
                    s = std::clamp(s, 0.0f, 1.0f);
                    edgeIndex = 3;
                    dAlong = 3.0f * edgeLen + s * edgeLen;
                }
            }

            // Convert distance along perimeter to phase in [0,1)
            phase = (perim > 0.0f) ? (dAlong / perim) : 0.0f;

            // ---- 3) Snap rotation to correct direction at startup ----
            float targetYaw = 0.0f;
            switch (edgeIndex)
            {
            case 0: // moving +X
                targetYaw = 0.0f;
                break;
            case 1: // moving +Z
                targetYaw = 0.5f * PI;
                break;
            case 2: // moving -X
                targetYaw = PI;
                break;
            case 3: // moving -Z
                targetYaw = 1.5f * PI;
                break;
            }
            // override whatever rotation was passed in; start aligned with path
            transform.rotation.y = targetYaw;

            // ---- 4) Randomize speed slightly per cloud ----
            float baseSpeed = 0.01f;                                                  // your original loops-per-second
            float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); // [0,1]
            float factor = 0.9f + 0.2f * r;                                           // [0.9, 1.1]
            speed = baseSpeed * factor;
        }

        bool init(GameState &gameState) override;
        void update(float deltaTime, GameState &gameState) override;
        void render(GameState &gameState) override;

    private:
        Rasterizer::Transform transform;
        Animator animator{};
        uint32_t vertexId = 0xFF;
        uint32_t triangleId = 0xFF;
        uint32_t instanceId = 0xFF;

        Vec3 center;
        float halfSize;
        float phase;
        float speed;
    };
}
