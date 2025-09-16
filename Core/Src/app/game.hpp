#pragma once
#include "platform.hpp"
#include "math/Vec3.hpp"

class Game {
public:
    Game(IRasterizer& gfx, IInput& in, ITimer& time)
        : gfx(gfx), input(in), timer(time) {}

    void init();
    void tick_once();
private:
    void tick_logic();
    void tick_graphics();
    bool initialized = false;
    Vec3 pos{};

private:
    IRasterizer& gfx;
    IInput&      input;
    ITimer&      timer;
    uint32_t next_tick_ms;
    uint32_t next_frame_ms;
};
