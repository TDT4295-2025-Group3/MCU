// fixed_point.hpp
#pragma once
#include <cstdint>

// Define the raw type for Q16.16
using Q16_16 = int32_t;

// floating point to Q16.16 conversion

inline Q16_16 floatToQ16_16(float value) {
    return static_cast<int32_t>(value * (1 << 16));
}

// Q16.16 to floating point conversion
inline float q16_16ToFloat(Q16_16 value) {
    return static_cast<float>(value) / (1 << 16);
}