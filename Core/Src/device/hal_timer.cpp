#include "hal_timer.hpp"

#include "stm32u5xx_hal.h"

uint32_t HalTimer::get_ticks_ms()
{
    return HAL_GetTick();
}
