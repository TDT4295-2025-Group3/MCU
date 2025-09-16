#pragma once
#include <platform.hpp>
#include <chrono>

#include <SDL3/SDL.h>


class HostTimer : public ITimer {
public:
    uint32_t get_ticks_ms() override;
};

class HostInput : public IInput {
public:
    KeyState poll() override;
};

class HostRasterizer : public IRasterizer {
public:
    explicit HostRasterizer(int width, int height);

    ~HostRasterizer() override;

    void clear(uint32_t argb) override;

    void rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t argb) override;

    void end_frame() override;

private:
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    void set_draw_color(uint32_t argb);
};
