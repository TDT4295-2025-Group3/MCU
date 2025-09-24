#include "host_input_timer.hpp"
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