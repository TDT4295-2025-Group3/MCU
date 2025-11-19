#pragma once
#include "math.hpp"
#include <functional>

namespace mcu_game
{
    struct BoxCollider
    {
        Vec3 center;
        Vec3 halfExtents;

        float bounciness{0.0f};
        float friction{0.0f};

        std::function<void()> onLand;

        BoxCollider() = default;
        BoxCollider(const Vec3 &center_, const Vec3 &halfExtents_)
            : center(center_), halfExtents(halfExtents_) {}

        void notifyLand()
        {
            if (onLand)
                onLand();
        }
    };
} // namespace mcu_game
