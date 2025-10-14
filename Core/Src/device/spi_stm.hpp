#include <stdint.h>
#include "irasterizer.hpp"

#ifdef __cplusplus
extern "C" {
#endif

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

uint8_t wipe_all(void);
uint16_t create_vertex(Rasterizer::Vertex *vertexBuffer, uint16_t vertCount);
uint16_t create_triangle(Rasterizer::Triangle *triangleBuffer, uint16_t triCount);
uint16_t create_instance(Rasterizer::Transform *instanceData, uint8_t vertbufferID, uint8_t tribufferID);
uint8_t update_instance(Rasterizer::Transform *instanceData, uint8_t instID);
#ifdef __cplusplus
}
#endif