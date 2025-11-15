#include "host_input_timer.hpp"
#include <chrono>
#include <stdexcept>
#include <SDL3/SDL_events.h>

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
    if (k[SDL_SCANCODE_RIGHT]) ks.x += 1.0f;
    if (k[SDL_SCANCODE_LEFT])  ks.x -= 1.0f;

    if (k[SDL_SCANCODE_UP])   ks.y += 1.0f;   // forward
    if (k[SDL_SCANCODE_DOWN]) ks.y -= 1.0f;   // backward

    // camera look axes (W/S = pitch, A/D = yaw)
    if (k[SDL_SCANCODE_D]) ks.cam_x += 1.0f;   // yaw right
    if (k[SDL_SCANCODE_A]) ks.cam_x -= 1.0f;   // yaw left
    // jump
    ks.space = k[SDL_SCANCODE_SPACE];
    return ks;
}

KeyState DS4Input::poll() {

    KeyState ks{};
    ks.cam_x = _hid_ds4.getStickPosition(DS4Stick::Right).x;;
    ks.cam_y = _hid_ds4.getStickPosition(DS4Stick::Right).y;
    ks.x = _hid_ds4.getStickPosition(DS4Stick::Left).x;
    ks.y = _hid_ds4.getStickPosition(DS4Stick::Left).y;
    ks.space = _hid_ds4.isKeyDown(DS4Button::Cross);
    return ks;
}
