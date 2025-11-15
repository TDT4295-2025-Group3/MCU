#pragma once

#include "camera.hpp"
#include "collider.hpp"

namespace mcu_game
{
    struct GameState
    {
        Camera &camera;
        std::vector<BoxCollider> boxColliders;

        GameState(Camera &cam) : camera(cam) {}
    };
} // namespace mcu_game