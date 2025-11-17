#pragma once
#include <memory>

#include "iinput.hpp"
#include "irasterizer.hpp"
#include "itimer.hpp"
#include "entities/player.hpp"
#include "entities/camera.hpp"
#include "isevenseg.hpp"

static constexpr float RUMBLE_THRESHOLD = -5.0f; // velocity Y fall threshold to trigger rumble

class Game
{
public:
    Game(Rasterizer::IRasterizer &gfx, IInput &in, ITimer &time, ISevenSeg& sevenseg, bool showHitboxDebug = false)
        : gfx(gfx), input(in), timer(time), showHitboxDebug(showHitboxDebug), sevenseg(sevenseg) {}

    void init();
    void tick_once();

private:
    void tick_logic();
    void tick_graphics();
    void createEntity(mcu_game::Entity *entity);

    bool initialized = false;
    std::vector<mcu_game::Entity *> entities;
    mcu_game::GameState gameState{};

    bool showHitboxDebug = false;

private:
    void initializeHitboxDebug();

    Rasterizer::IRasterizer &gfx;
    IInput &input;
    ITimer &timer;
    ISevenSeg& sevenseg;
    uint32_t next_tick_ms;
    uint32_t next_frame_ms;

    float lastRumbleIntensity = 0.0f;

    // Pointer to the player entity for easy access
    std::unique_ptr<mcu_game::Player> _player;
};
