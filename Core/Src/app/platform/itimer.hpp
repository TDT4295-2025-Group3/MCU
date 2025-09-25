#pragma once
#include <cstdint>

class ITimer {
    // Interface for timer
public:
    virtual ~ITimer() = default;

    virtual uint32_t get_ticks_ms() = 0;
};
