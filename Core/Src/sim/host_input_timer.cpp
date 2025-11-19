#include "host_input_timer.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <SDL3/SDL.h>
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
    if (k[SDL_SCANCODE_W]) ks.cam_y -= 1.0f;   // pitch up
    if (k[SDL_SCANCODE_S]) ks.cam_y += 1.0f;   // pitch down
    if (k[SDL_SCANCODE_D]) ks.cam_x += 1.0f;   // yaw right
    if (k[SDL_SCANCODE_A]) ks.cam_x -= 1.0f;   // yaw left
    // jump
    ks.space = k[SDL_SCANCODE_SPACE];
    return ks;
}

void HostInput::setRumble(float x) {
    std::cout << "[HostInput] Rumble set to " << x << " (no effect in simulation)" << std::endl;
}

void HostInput::setInputColor(int r, int g, int b) {
    std::cout << "[HostInput] Set input color to (" << r << ", " << g << ", " << b << ") (no effect in simulation)" << std::endl;
}

void HostInput::setBlinking(int interval_on_ms, int interval_off_ms) {
    std::cout << "[HostInput] Set blinking to on=" << interval_on_ms << "ms, off=" << interval_off_ms << "ms (no effect in simulation)" << std::endl;
}


KeyState DS4Input::poll() {
    KeyState ks{};
    ks.cam_x = controller.getStickPosition(DS4Stick::Right).x;
    ks.cam_y = controller.getStickPosition(DS4Stick::Right).y;
    ks.x = controller.getStickPosition(DS4Stick::Left).x;
    ks.y = -controller.getStickPosition(DS4Stick::Left).y;
    ks.space = controller.isKeyDown(DS4Button::Cross);
    return ks;
}


void DS4Input::setRumble(float x) {
    x = std::clamp(x, 0.0f, 1.0f);

    const float a = 1.0f;   // strong contribution
    const float b = 0.2f;   // weak contribution

    // First fill the strong motor
    float strongF = std::min(x / a, 1.0f);

    // Remaining energy goes to weak
    float remaining = x - strongF * a;
    float weakF = std::clamp(remaining / b, 0.0f, 1.0f);

    auto rumbleStrong = static_cast<uint8_t>(strongF * 255.0f);
    auto rumbleWeak = static_cast<uint8_t>(weakF * 255.0f);

    std::cout << "[DS4Input] Rumble set to " << x
              << " (strong=" << static_cast<int>(rumbleStrong)
              << ", weak=" << static_cast<int>(rumbleWeak) << ")" << std::endl;

    controller.setRumbleStrong(rumbleStrong);
    controller.setRumbleWeak(rumbleWeak);

    controller.flushOutput();
}


void DS4Input::setInputColor(int r, int g, int b) {
    controller.setLedColor(r, g, b);
}

void DS4Input::setBlinking(int interval_on_ms, int interval_off_ms) {
    controller.setBlinking(interval_on_ms, interval_off_ms);
}