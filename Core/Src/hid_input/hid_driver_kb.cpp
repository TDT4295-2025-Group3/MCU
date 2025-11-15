#include <cstring>
#include "hid_driver.hpp"

KeyboardDriver::KeyboardDriver() : HIDDriver() {
    // Any keyboard-specific initialization can go here
}

bool KeyboardDriver::getModifierState(const HID_KeyboardInputReport &report, KBModifier modifier) {
    switch (modifier) {
        case KBModifier::LeftCtrl:   return report.LeftCtrl;
        case KBModifier::LeftShift:  return report.LeftShift;
        case KBModifier::LeftAlt:    return report.LeftAlt;
        case KBModifier::LeftGui:    return report.LeftGui;
        case KBModifier::RightCtrl:  return report.RightCtrl;
        case KBModifier::RightShift: return report.RightShift;
        case KBModifier::RightAlt:   return report.RightAlt;
        case KBModifier::RightGui:   return report.RightGui;
        default: return false;
    }
}

bool KeyboardDriver::isKeyInReport(const HID_KeyboardInputReport &report, KBKey key) {
    uint8_t keyCode = static_cast<uint8_t>(key);
    for (int i = 0; i < 6; i++) {
        if (report.keys[i] == keyCode) {
            return true;
        }
    }
    return false;
}

bool KeyboardDriver::isModifierDown(KBModifier modifier) const {
    return getModifierState(currentInput, modifier);
}

bool KeyboardDriver::isModifierPressed(KBModifier modifier) const {
    return getModifierState(currentInput, modifier) && 
           !getModifierState(previousInput, modifier);
}

bool KeyboardDriver::isModifierReleased(KBModifier modifier) const {
    return !getModifierState(currentInput, modifier) && 
           getModifierState(previousInput, modifier);
}

bool KeyboardDriver::isKeyDown(KBKey key) const {
    return isKeyInReport(currentInput, key);
}

bool KeyboardDriver::isKeyPressed(KBKey key) const {
    return isKeyInReport(currentInput, key) && 
           !isKeyInReport(previousInput, key);
}

bool KeyboardDriver::isKeyReleased(KBKey key) const {
    return !isKeyInReport(currentInput, key) && 
           isKeyInReport(previousInput, key);
}

bool KeyboardDriver::getLedState(KBLed led) const {
    switch (led) {
        case KBLed::NumLock:    return currentInput.reserved & 0x01;  // Typically from output report
        case KBLed::CapsLock:   return currentInput.reserved & 0x02;
        case KBLed::ScrollLock: return currentInput.reserved & 0x04;
        case KBLed::Compose:    return currentInput.reserved & 0x08;
        case KBLed::Kana:       return currentInput.reserved & 0x10;
        default: return false;
    }
}

int KeyboardDriver::getKeyCount() const {
    int count = 0;
    for (int i = 0; i < 6; i++) {
        if (currentInput.keys[i] != 0x00) {
            count++;
        }
    }
    return count;
}

KBKey KeyboardDriver::getPressedKey(int index) const {
    if (index < 0 || index >= 6) {
        return KBKey::None;
    }
    return static_cast<KBKey>(currentInput.keys[index]);
}

void KeyboardDriver::processInput(const HID_KeyboardInputReport &report) {
    HIDDriver::processInput(report);
    // Add any keyboard-specific input processing here
    // For example, you could detect key combinations, track typing speed, etc.
}