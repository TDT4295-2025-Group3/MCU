#pragma once

#include <cstdint>
#include "isevenseg.hpp"


namespace SevenSeg {

// Initializes the seven-segment controller (assumes MAX7219-compatible).
void init();

// Displays an unsigned integer value across up to four digits. Leading digits are blanked.
void displayNumber(uint16_t value);

// Displays the provided digit values (0-9) on the seven-segment controller. Index 0 is LSB/rightmost digit.
void displayDigits(const uint8_t* digits, std::size_t count);

// Displays up to four characters (0-9, E, H, P, L, '-', blank) using MAX7219 decode mode.
void displayChars(const char* text, std::size_t count);

// Convenience overload accepting a null-terminated string (uses up to four characters).
void displayChars(const char* text);

// Backwards-compatible alias for displayChars(const char*).
void displayString(const char* text);

// Clears all digits on the display.
void blank();

} // namespace SevenSeg

/**
 * Implementation of ISevenSeg for MCU platform.
 */
class MCUSevenSeg : public ISevenSeg {
public:
    MCUSevenSeg();
    void setDisplayedValue(std::string value) override;
};