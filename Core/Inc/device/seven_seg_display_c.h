#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void SevenSeg_Init(void);
void SevenSeg_DisplayNumber(uint16_t value);
void SevenSeg_DisplayChars(const char* text, size_t count);
void SevenSeg_DisplayString(const char* text);

#ifdef __cplusplus
} // extern "C"
#endif
