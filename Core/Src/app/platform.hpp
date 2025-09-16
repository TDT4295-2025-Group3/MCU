#pragma once
#include <cstdint>
/**
 * Definition of platform interface.
 * It can then be implemented for different platforms, for example
 * one for embedded, one for desktop simulation.
 **/

struct KeyState {
    bool up{}, down{}, left{}, right{}, a{}, b{};
};

class IInput {
    // Interface for input
public:
    virtual ~IInput() = default;
    virtual KeyState poll() = 0;
};

class ITimer {
    // Interface for timer
public:
    virtual ~ITimer() = default;
    virtual uint32_t get_ticks_ms() = 0;
};


class IRasterizer {
public:
    // Inteface for rasterizer
    virtual ~IRasterizer() = default;
    virtual void clear(uint32_t argb) = 0;
    virtual void rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t argb) = 0;
    virtual void end_frame() = 0;
};

class ILed {
public:
    // Interface for LED's
    virtual ~ILed() = default;
    virtual void toggle() = 0;
};