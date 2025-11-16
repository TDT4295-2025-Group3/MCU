#pragma once
#include "game_state.hpp"
#include "platform/irasterizer.hpp"
#include "input.hpp"

namespace mcu_game
{
    class Entity
    {
    public:
        virtual ~Entity() = default;
        virtual bool init(Rasterizer::IRasterizer &gfx, GameState &gameState) = 0;
        virtual void update(const InputState &in, float deltaTime, GameState &gameState) = 0;
        virtual void render(Rasterizer::IRasterizer &gfx) = 0;
    };
} // namespace mcu_game