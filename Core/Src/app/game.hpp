#pragma once
#include "iinput.hpp"
#include "irasterizer.hpp"
#include "itimer.hpp"
#include "math/Vec3.hpp"

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
    Vec3 pos{};

    uint32_t instanceId;
private:
    Rasterizer::IRasterizer& gfx;
    IInput&      input;
    ITimer&      timer;
    uint32_t next_tick_ms;
    uint32_t next_frame_ms;

    float cameraZ = -2;
};
