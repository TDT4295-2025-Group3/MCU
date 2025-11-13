#include "seven_seg_display.hpp"

extern "C" {

void SevenSeg_Init(void) {
  SevenSeg::init();
}

void SevenSeg_DisplayNumber(uint16_t value) {
  SevenSeg::displayNumber(value);
}

void SevenSeg_DisplayChars(const char* text, size_t count) {
  SevenSeg::displayChars(text, count);
}

void SevenSeg_DisplayString(const char* text) {
  SevenSeg::displayChars(text);
}

} // extern "C"
