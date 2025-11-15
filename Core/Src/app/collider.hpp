#pragma once
#include "math.hpp"

namespace mcu_game
{
    struct BoxCollider
    {
        Vec3 center;
        Vec3 halfExtents;
    };
} // namespace mcu_game
