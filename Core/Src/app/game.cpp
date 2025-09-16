#include "game.hpp"

#include <__algorithm/clamp.h>

#include "constants.hpp"
#include "platform.hpp"

static inline bool time_reached(uint32_t now, uint32_t target) {
    // signed diff handles wraparound
    return static_cast<int32_t>(now - target) >= 0;
}

void Game::init() {
    auto tick = timer.get_ticks_ms();
    next_tick_ms = tick + TICK_MS;
    next_frame_ms = tick + FRAME_MS;
    initialized = true;

    pos = Vec3{10.0f, 10.0f, 0.0f};
}

void Game::tick_once() {
    // Called from sim/device in a infinite loop
    if (!initialized) init();

    auto now = timer.get_ticks_ms();

    // Do catchup logic ticks
    uint32_t steps = 0;
    while (time_reached(now, next_tick_ms) && steps < MAX_CATCHUP_STEPS) {
        tick_logic();
        next_tick_ms += TICK_MS;
        steps++;
    }

    // Resync if we are too far behind
    if (steps == MAX_CATCHUP_STEPS && time_reached(now, next_tick_ms)) {
        next_tick_ms = now + TICK_MS;
    }

    if (time_reached(now, next_frame_ms)) {
        tick_graphics();
        next_frame_ms += FRAME_MS;
    }
}

void Game::tick_graphics() {
    gfx.clear(0xFF000000);

    // red hue based on z
    uint8_t red = static_cast<uint8_t>(std::clamp(pos.z, 125.0f, 255.0f));
    uint32_t color = 0xFF000000 | (red << 16);

    gfx.rect(static_cast<uint16_t>(pos.x),
             static_cast<uint16_t>(pos.y),
             10, 10, color);

    gfx.end_frame();
}

void Game::tick_logic() {
    auto ks = input.poll();
    if (ks.up) pos.y -= 5.0f;
    if (ks.down) pos.y += 5.0f;
    if (ks.left) pos.x -= 5.0f;
    if (ks.right) pos.x += 5.0f;
    if (ks.a) pos.z += 5.0f;
    if (ks.b) pos.z -= 5.0f;
}