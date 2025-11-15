#pragma once

namespace mcu_game
{
    class Entity
    {
    public:
        virtual ~Entity() = default;
        virtual void init(Rasterizer::IRasterizer &gfx, GameState &gameState) = 0;
        virtual void update(const InputState &in, float deltaTime) = 0;
        virtual void render(Rasterizer::IRasterizer &gfx, GameState &gameState) = 0;
    };
} // namespace mcu_game