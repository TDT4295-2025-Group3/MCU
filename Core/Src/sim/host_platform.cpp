#include "host_platform.hpp"
#include <chrono>
#include <stdexcept>

uint32_t HostTimer::get_ticks_ms() {
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return static_cast<uint32_t>(ms);
}

KeyState HostInput::poll() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        const auto k = SDL_GetKeyboardState(nullptr);
        return KeyState{
            .up = k[SDL_SCANCODE_UP],
            .down = k[SDL_SCANCODE_DOWN],
            .left = k[SDL_SCANCODE_LEFT],
            .right = k[SDL_SCANCODE_RIGHT],
            .a = k[SDL_SCANCODE_Z],
            .b = k[SDL_SCANCODE_X]
        };
    }
    return KeyState{
        .up = false,
        .down = false,
        .left = false,
        .right = false,
        .a = false,
        .b = false
    };
}

HostRasterizer::HostRasterizer(int width, int height) {
    window = SDL_CreateWindow("Super Cool Game", width, height, 0);
    if (!window) throw std::runtime_error(SDL_GetError());
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) throw std::runtime_error(SDL_GetError());

    SDL_ShowWindow(window);
}

HostRasterizer::~HostRasterizer() {
    if (window) {
        SDL_DestroyWindow(window);
    }
}

void HostRasterizer::clear(uint32_t argb) {
    set_draw_color(argb);
    SDL_RenderClear(renderer);
}

void HostRasterizer::rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t argb) {
    set_draw_color(argb);
    SDL_FRect r{
        static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(w), static_cast<float>(h)
    };
    SDL_RenderFillRect(renderer, &r);
}

void HostRasterizer::end_frame() {
    SDL_RenderPresent(renderer);
}

void HostRasterizer::set_draw_color(uint32_t argb) {
    Uint8 a = (argb >> 24) & 0xFF;
    Uint8 r = (argb >> 16) & 0xFF;
    Uint8 g = (argb >> 8) & 0xFF;
    Uint8 b = (argb >> 0) & 0xFF;
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}
