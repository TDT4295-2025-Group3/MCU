
#include "hid_driver.hpp"
#include <cmath>

DS4Driver::DS4Driver() : HIDDriver() {
    outputDraft.EnableRumbleUpdate = 1;
    outputDraft.EnableLedUpdate = 1;

    // We start with LED off
    outputDraft.LedRed = 0x00;
    outputDraft.LedGreen = 0x00;
    outputDraft.LedBlue = 0x00;

    outputBuffer.push(outputDraft);
}

bool DS4Driver::getButtonState(const DS4_InputUSBReport &report, DS4Button button) {
    switch (button) {
        case DS4Button::Square:
            return report.ButtonSquare;
        case DS4Button::Cross:
            return report.ButtonCross;
        case DS4Button::Circle:
            return report.ButtonCircle;
        case DS4Button::Triangle:
            return report.ButtonTriangle;
        case DS4Button::L1:
            return report.ButtonL1;
        case DS4Button::R1:
            return report.ButtonR1;
        case DS4Button::L2:
            return report.ButtonL2Btn;
        case DS4Button::R2:
            return report.ButtonR2Btn;
        case DS4Button::Share:
            return report.ButtonShare;
        case DS4Button::Options:
            return report.ButtonOptions;
        case DS4Button::L3:
            return report.ButtonL3;
        case DS4Button::R3:
            return report.ButtonR3;
        case DS4Button::Home:
            return report.ButtonHome;
        case DS4Button::Pad:
            return report.ButtonPad;
        case DS4Button::DPadUp:
            return report.DPad == DS4_DpadDirection::North ||
                   report.DPad == DS4_DpadDirection::NorthEast ||
                   report.DPad == DS4_DpadDirection::NorthWest;
        case DS4Button::DPadDown:
            return report.DPad == DS4_DpadDirection::South ||
                   report.DPad == DS4_DpadDirection::SouthEast ||
                   report.DPad == DS4_DpadDirection::SouthWest;
        case DS4Button::DPadLeft:
            return report.DPad == DS4_DpadDirection::West ||
                   report.DPad == DS4_DpadDirection::NorthWest ||
                   report.DPad == DS4_DpadDirection::SouthWest;
        case DS4Button::DPadRight:
            return report.DPad == DS4_DpadDirection::East ||
                   report.DPad == DS4_DpadDirection::NorthEast ||
                   report.DPad == DS4_DpadDirection::SouthEast;
        default:
            return false;
    }
}


bool DS4Driver::isKeyDown(DS4Button button) const {
    return getButtonState(currentInput, button);
}

bool DS4Driver::isKeyPressed(DS4Button button) const {
    if (previousInput.ReportID == 0x00) return false; // No previous input to compare to

    return getButtonState(currentInput, button) && !getButtonState(previousInput, button);
}

bool DS4Driver::isKeyReleased(DS4Button button) const {
    if (previousInput.ReportID == 0x00) return false; // No previous input to compare to

    return !getButtonState(currentInput, button) && getButtonState(previousInput, button);
}

void DS4Driver::setRumbleWeak(uint8_t value) {
    this->outputDraft.RumbleRight = value;
    dirty = true;
}

void DS4Driver::setRumbleStrong(uint8_t value) {
    this->outputDraft.RumbleLeft = value;
    dirty = true;
}

void DS4Driver::setLedColor(uint8_t r, uint8_t g, uint8_t b) {
    this->outputDraft.LedRed = r;
    this->outputDraft.LedGreen = g;
    this->outputDraft.LedBlue = b;
    dirty = true;
}

void DS4Driver::setLedColor(uint32_t rgb) {
    uint8_t r = (rgb >> 16) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = rgb & 0xFF;
    setLedColor(r, g, b);
}

/**
 * Normalize a stick value from [0, 255] to [-1.0, 1.0].
 * @param value The raw stick value (0-255).
 * @param deadzone The deadzone threshold (default is 0.15).
 * @return Normalized stick value (-1.0 to 1.0).
 */
inline float normalizeStickValue(uint8_t value, float deadzone = 0.15f) {

    const float val = (static_cast<float>(value) - 128) / 128.0f;
    return (fabsf(val) < deadzone) ? 0.0f : val;
}

Vec2 DS4Driver::getStickPosition(DS4Stick stick) const {
    switch (stick) {
        case DS4Stick::Left:
            return Vec2{
                    normalizeStickValue(currentInput.LeftStickX), normalizeStickValue(currentInput.LeftStickY)
            };
        case DS4Stick::Right:
            return Vec2{
                    normalizeStickValue(currentInput.RightStickX), normalizeStickValue(currentInput.RightStickY)
            };
        default:
            return Vec2{0, 0};
    }
}
