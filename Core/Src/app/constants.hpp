#ifndef IDONTKNOW_CONSTANTS_H
#define IDONTKNOW_CONSTANTS_H

#include <cstdint>

/**
 * Game physics/logic tickrate
 */
constexpr uint32_t GAME_TICKRATE = 60;

/**
 * Actual graphics framerate
 */
constexpr uint32_t GAME_FRAMERATE = 60;


constexpr uint32_t TICK_MS = 1000 / GAME_TICKRATE;
constexpr uint32_t FRAME_MS = 1000 / GAME_FRAMERATE;

/**
 * How many logic updates can be done in one frame to catch up
 * avoiding spiral of death
 */
constexpr uint32_t MAX_CATCHUP_STEPS = 5;

#endif //IDONTKNOW_CONSTANTS_H