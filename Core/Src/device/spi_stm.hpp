#include <stdint.h>

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

typedef struct {
    int32_t x, y, z;   // Q16.16
    uint8_t r, g, b;   // 4-bit each
} Vertex108;


uint8_t wipe_all(void);
uint16_t create_vertex(Vertex108 *vertexBuffer, uint32_t vertCount);
uint16_t create_triangle(uint32_t *triangleBuffer, uint32_t triCount);
uint16_t create_instance(uint32_t *instanceBuffer, uint32_t instCount);
uint8_t update_instance(uint32_t *instanceBuffer, uint32_t instCount);
#ifdef __cplusplus
}
#endif