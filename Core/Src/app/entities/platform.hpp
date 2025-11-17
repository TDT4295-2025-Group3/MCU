#pragma once

#include "math.hpp"

#include "entities/entity.hpp"
#include "anim/anim.hpp"
#include "rigidbody.hpp"
#include "game_state.hpp"

namespace mcu_game
{
    class Platform : public Entity
    {
    public:
        Platform(Vec3 center, float rotation) : collider{{center.x, center.y - 0.5f, center.z}, {1.8f, 0.5f, 1.8f}},
                                                transform{center.x, center.y, center.z, 0.0f, rotation, 0.0f, 1.0f, 1.0f, 1.0f}
        {
        }

        bool init(GameState &gameState) override;

    private:
        BoxCollider collider;
        Rasterizer::Transform transform;
        uint32_t vertexId = 0xFF;
        uint32_t triangleId = 0xFF;
        uint32_t instanceId = 0xFF;
    };

} // namespace mcu_game
