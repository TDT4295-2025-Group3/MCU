#pragma once

#include <string>

/**
 * Interface for a generic 7-segment display.
 * Supports displaying upto 4 digits (as to match our hardware).
 */
class ISevenSeg {
public:
    virtual ~ISevenSeg() = default;

    /**
     * Displays a string on the 7-segment display.
     * @param value The string to display (max 4 characters).
     */
    virtual void setDisplayedValue(std::string value) = 0;

    /**
     * Displays a numeric value on the 7-segment display.
     * Automatically adds padding and clamps to 4 digits.
     * @param value The integer value to display.
     */
    virtual void setDisplayedValue(int value)
    {
        // Convert number to string
        std::string s = std::to_string(value);

        // Clamp to 4 digits (keep last 4 digits)
        if (s.length() > 4)
            s = s.substr(s.length() - 4);

        // Left-pad with spaces so the string is exactly 4 chars
        if (s.length() < 4)
            s.insert(s.begin(), 4 - s.length(), ' ');

        // Forward to the string-based implementation
        setDisplayedValue(s);
    }
};