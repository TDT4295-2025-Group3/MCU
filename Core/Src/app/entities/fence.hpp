#pragma once

#include "math.hpp"
#include "input.hpp"
#include "entities/entity.hpp"
#include "anim/anim.hpp"
#include "game_state.hpp"

namespace mcu_game
{
    class Fence : public Entity
    {
    public:
        Fence(Vec3 center, float rotation) : transform{center.x, center.y, center.z, 0.0f, rotation, 0.0f, 1.0f, 1.0f, 1.0f}
        {
        }
        bool init(Rasterizer::IRasterizer &gfx, GameState &gameState) override;
        void update(const InputState &in, float deltaTime, GameState &gameState) override;
        void render(Rasterizer::IRasterizer &gfx) override;

    private:
        Rasterizer::Transform transform;
        uint32_t vertexId = 0xFF;
        uint32_t triangleId = 0xFF;
        uint32_t instanceId = 0xFF;
    };

} // namespace mcu_game
