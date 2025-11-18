#pragma once
#include <cstdint>

#include "itimer.hpp"

class HalTimer : public ITimer
{
public:
    uint32_t get_ticks_ms() override;
};
