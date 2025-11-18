#pragma once

#include "math.hpp"

#include "entities/entity.hpp"
#include "anim/anim.hpp"
#include "rigidbody.hpp"
#include "game_state.hpp"

namespace mcu_game
{
    class Burger : public Entity
    {
    public:
        Burger(Vec3 center, float rotation) : transform{center.x, center.y, center.z, 0.0f, rotation, 0.0f, 1.0f, 1.0f, 1.0f}
        {
        }
        bool init(GameState &gameState) override;
        void update(float deltaTime, GameState &gameState) override;
        void render(GameState &gameState) override;

    private:
        void landCallback();
        float landTimer{0.0f};
        Rasterizer::Transform transform;
        Animator animator{};
        uint32_t vertexId = 0xFF;
        uint32_t triangleId = 0xFF;
        uint32_t instanceId = 0xFF;
    };

} // namespace mcu_game
