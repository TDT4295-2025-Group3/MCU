#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
uint8_t wipe_all(void);
uint16_t create_vertex(uint32_t *vertexBuffer, uint32_t vertCount);
uint16_t create_triangle(uint32_t *triangleBuffer, uint32_t triCount);
uint16_t create_instance(uint32_t *instanceBuffer, uint32_t instCount);
uint8_t update_instance(uint32_t *instanceBuffer, uint32_t instCount);
#ifdef __cplusplus
}
#endif