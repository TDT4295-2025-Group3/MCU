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
    // Pump events to keep window responsive and update keyboard state
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            // No quit handling here; the simulator main loop runs forever
        }
    }
    const bool *k = SDL_GetKeyboardState(nullptr);
    KeyState ks{};
    ks.up = k[SDL_SCANCODE_UP];
    ks.down = k[SDL_SCANCODE_DOWN];
    ks.left = k[SDL_SCANCODE_LEFT];
    ks.right = k[SDL_SCANCODE_RIGHT];
    // camera look
    ks.w = k[SDL_SCANCODE_W];
    ks.s = k[SDL_SCANCODE_S];
    ks.a = k[SDL_SCANCODE_A];
    ks.d = k[SDL_SCANCODE_D];
    // jump
    ks.space = k[SDL_SCANCODE_SPACE];
    return ks;
}