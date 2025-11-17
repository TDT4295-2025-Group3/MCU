#pragma once

#include "math.hpp"
#include "collider.hpp"
#include <vector>

namespace mcu_game
{
    struct GameState
    {
        Vec3 cameraForward{0.0f, 0.0f, 1.0f};
        Vec3 playerPosition{0.0f, 0.0f, 0.0f};
        std::vector<BoxCollider> boxColliders;

        GameState() = default;
    };
} // namespace mcu_game