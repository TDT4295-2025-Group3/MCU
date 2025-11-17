#pragma once
#include "game_state.hpp"
#include "platform/irasterizer.hpp"
#include "platform/iinput.hpp"
namespace mcu_game
{
    class Entity
    {
    public:
        virtual ~Entity() = default;
        virtual bool init(GameState &gameState) { return true; }
        virtual void update(float deltaTime, GameState &gameState) {}
        virtual void render(GameState &gameState) {}
    };
} // namespace mcu_game