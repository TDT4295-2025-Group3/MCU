#include "gameloop.hpp"

#include "constants.hpp"
#include "stm32u5xx_hal.h"
#include "stm32u5xx_nucleo.h"

// Game specific includes
#include "player.hpp"
#include "camera.hpp"
#include "input.hpp"

using namespace mcu_game;

static constexpr float FIXED_DT = 1.0f / static_cast<float>(GAME_TICKRATE);

// Persistent game objects
static Player g_player;
static Camera g_camera;

// Simple placeholder input provider. Replace with real hardware input later.
static InputState sample_input() {
    InputState in{};
    // Example: move forward constantly for now
    // You can tie this to a button: if(HAL_GPIO_ReadPin(...)) in.jump = true;
    in.moveZ = 0.0f; // set to 1.0f to auto-walk
    in.moveX = 0.0f;
    in.jump = false;
    in.lookYawDelta = 0.0f;   // could spin: e.g., 0.02f
    in.lookPitchDelta = 0.0f;
    return in;
}

static bool g_initialized = false;

static inline bool time_reached(uint32_t now, uint32_t target) {
    // signed diff handles wraparound
    return static_cast<int32_t>(now - target) >= 0;
}

void update() {
    if (!g_initialized) {
        g_player.reset();
        g_camera.reset();
        g_initialized = true;
    }

    InputState input = sample_input();

    // Update systems
    g_player.update(input, g_camera, FIXED_DT);
    g_camera.update(input.lookYawDelta, input.lookPitchDelta, g_player, FIXED_DT);
}

void render() {
    HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);
    // Placeholder: In future, package and send player & camera state over SPI to FPGA.
    // Example packet idea (not implemented here): [header][playerPos xyz][cameraPos xyz][cameraForward xyz]
}


void gameloop() {
    uint32_t now = HAL_GetTick();
    uint32_t next_tick = now + TICK_MS;
    uint32_t next_frame = now + FRAME_MS;

    for (;;) {
        now = HAL_GetTick();

        uint32_t steps = 0;
        while (time_reached(now, next_tick) && steps < MAX_CATCHUP_STEPS) {
            update();
            next_tick += TICK_MS;
            steps++;
        }

        if (steps == MAX_CATCHUP_STEPS && time_reached(now, next_tick)) {
            next_tick = now + TICK_MS;
        }

        if (time_reached(now, next_frame)) {
            render();
            next_frame += FRAME_MS;
        }
    }
}
