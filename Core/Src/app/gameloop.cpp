#include "gameloop.hpp"

#include "constants.hpp"
#include "stm32u5xx_hal.h"
#include "stm32u5xx_nucleo.h"

static inline bool time_reached(uint32_t now, uint32_t target) {
    // signed diff handles wraparound
    return static_cast<int32_t>(now - target) >= 0;
}

void update() {
}

void render() {
    HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);
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
