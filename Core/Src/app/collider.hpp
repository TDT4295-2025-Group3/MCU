#pragma once
#include "math.hpp"

namespace mcu_game
{
    struct BoxCollider
    {
        Vec3 center;
        Vec3 halfExtents;

        BoxCollider() = default;
        BoxCollider(const Vec3 &center_, const Vec3 &halfExtents_)
            : center(center_), halfExtents(halfExtents_) {}
    };
} // namespace mcu_game
