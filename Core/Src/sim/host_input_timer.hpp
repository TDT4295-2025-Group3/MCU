#pragma once
#include <chrono>

#include "iinput.hpp"
#include "itimer.hpp"
#include <SDL3/SDL.h>


class HostTimer : public ITimer {
public:
    uint32_t get_ticks_ms() override;
};

class HostInput : public IInput {
public:
    KeyState poll() override;
};
