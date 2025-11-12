#pragma once

#include <cstddef>
#include <cstdint>

namespace SevenSeg {

// Initializes the seven-segment controller (assumes MAX7219-compatible).
void init();

// Displays an unsigned integer value across up to four digits. Leading digits are blanked.
void displayNumber(uint16_t value);

// Displays the provided digit values (0-9) on the seven-segment controller. Index 0 is LSB/rightmost digit.
void displayDigits(const uint8_t* digits, std::size_t count);

// Clears all digits on the display.
void blank();

} // namespace SevenSeg
