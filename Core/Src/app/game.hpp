#pragma once
#include "iinput.hpp"
#include "irasterizer.hpp"
#include "itimer.hpp"
#include "player.hpp"
#include "camera.hpp"

class Game {
public:
    Game(Rasterizer::IRasterizer& gfx, IInput& in, ITimer& time)
        : gfx(gfx), input(in), timer(time) {}

    void init();
    void tick_once();
private:
    void tick_logic();
    void tick_graphics();
    bool initialized = false;
    mcu_game::Player player{};
    mcu_game::Camera camera{};

    uint32_t instanceCubeId = 0xFF;
    uint32_t instancePyrId  = 0xFF;
    uint32_t instancePlaneId = 0xFF;
private:
    Rasterizer::IRasterizer& gfx;
    IInput&      input;
    ITimer&      timer;
    uint32_t next_tick_ms;
    uint32_t next_frame_ms;
};
