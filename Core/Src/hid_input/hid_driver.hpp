#pragma once
#include "hid_reports.h"
#include <cstring>



/**
 * A simple ring buffer implementation which overwrites old data when full.
 * @tparam T Type of elements stored in the buffer.
 * @tparam Capacity Maximum number of elements the buffer can hold.
 */
template <typename T, int Capacity>
class UnsafeRingBuffer {
public:
    UnsafeRingBuffer() : head(0), tail(0), count(0) {}

    void push(const T& item) {
        buffer[head] = item;
        head = (head + 1) % Capacity;

        if (count == Capacity) {
            // Buffer is full, advance tail to overwrite oldest
            tail = (tail + 1) % Capacity;
        } else {
            count++;
        }
    }

    bool pop(T& item) {
        if (count == 0) return false; // empty
        item = buffer[tail];
        tail = (tail + 1) % Capacity;
        count--;
        return true;
    }

    bool isEmpty() const { return count == 0; }
    bool isFull() const { return count == Capacity; }

private:
    T buffer[Capacity];
    int head;
    int tail;
    int count;
};

/**
 * Base class for all HID device drivers
 * Provides common functionality for input processing and output management
 *
 * General usage:
 *  1) Call processInput() when a new input report is received
 *  2) Modify outputDraft as needed to prepare output reports, and call flushOutput() when ready
 *  3) Use getReadyOutputReport() to retrieve output reports to send to the device
 */
template<typename InputReportType, typename OutputReportType, size_t BufferSize = 8>
class HIDDriver {
public:
    HIDDriver() {
        memset(&currentInput, 0, sizeof(InputReportType));
        memset(&previousInput, 0, sizeof(InputReportType));
        memset(&outputDraft, 0, sizeof(OutputReportType));
        dirty = false;
    }

    virtual ~HIDDriver() = default;

    /**
     * Process a new input report from the device
     * @param report The input report to process
     */
    virtual void processInput(const InputReportType &report) {
        memcpy(&previousInput, &currentInput, sizeof(InputReportType));
        memcpy(&currentInput, &report, sizeof(InputReportType));
    }

    /**
     * Get the next ready output report, if available
     * @param report Reference to store the output report
     * @return true if a report was available, false otherwise
     */
    bool getReadyOutputReport(OutputReportType &report) {
        if (!outputBuffer.isEmpty()) {
            return outputBuffer.pop(report);
        }
        return false;
    }

    /**
     * Send the current output report to the output queue
     */
    void flushOutput() {
        if (dirty) {
            outputBuffer.push(outputDraft);
            dirty = false;
        }
    }

protected:
    /** Current and previous input reports for state tracking */
    InputReportType currentInput, previousInput;

    /** Ring buffer to hold pending output reports */
    UnsafeRingBuffer<OutputReportType, BufferSize> outputBuffer;

    /** Current draft of the output report to be sent */
    OutputReportType outputDraft;

    /** Flag indicating if the output report has been modified */
    bool dirty;

    /**
     * Mark the output as dirty (needs to be sent)
     */
    void markDirty() {
        dirty = true;
    }
};

/**
* Enum for all available DS4 buttons
*/
enum class DS4Button {
    Square,
    Cross,
    Circle,
    Triangle,
    L1,
    R1,
    L2,
    R2,
    Share,
    Options,
    L3,
    R3,
    Home,
    Pad,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
};

/**
* Enum for the two analog sticks on the DS4
*/
enum class DS4Stick {
    Left,
    Right
};

/**
* 2D vector structure for stick positions.
*/
typedef struct Vec2 {
    float x;
    float y;
} Vec2;

/**
* Driver class for a DualShock 4 controller.
* Extends the base HIDDriver functionality with DS4-specific features.
*/
class DS4Driver : public HIDDriver<DS4_InputUSBReport, DS4_OutputUSBReport, 8> {
public:
    DS4Driver();

    void queueInitReport();

    void requeueOutputReport(const DS4_OutputUSBReport& report) {
        outputBuffer.push(report);
    }

    /**
     * Check if a button is currently being held down.
     * @param button The button to check.
     * @return true if the button is currently pressed, false otherwise.
     */
    bool isKeyDown(DS4Button button) const;

    /**
     * Check if a button was just pressed this frame (transition from up to down).
     * @param button The button to check.
     * @return true if the button was just pressed, false otherwise.
     */
    bool isKeyPressed(DS4Button button) const;

    /**
     * Check if a button was just released this frame (transition from down to up).
     * @param button The button to check.
     * @return true if the button was just released, false otherwise.
     */
    bool isKeyReleased(DS4Button button) const;

    /**
     * Set the weak rumble motor intensity.
     * @param value Intensity value (0-255).
     */
    void setRumbleWeak(uint8_t value);

    /**
     * Set the strong rumble motor intensity.
     * @param value Intensity value (0-255).
     */
    void setRumbleStrong(uint8_t value);

    /**
     * Set the color of the controller's LED.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     */
    void setLedColor(uint8_t r, uint8_t g, uint8_t b);

    /**
     * Set the color of the controller's LED using a single 32-bit RGB value.
     * @param rgb 32-bit RGB value (0xRRGGBB).
     */
    void setLedColor(uint32_t rgb);

    /**
     * Get the current position of the specified stick, normalized to the range [-1.0, 1.0].
     * @param stick The stick to get the position of (Left or Right).
     * @return Normalized stick position as a Vec2.
     */
    Vec2 getStickPosition(DS4Stick stick) const;

    /**
     * Override processInput to add any DS4-specific processing if needed
     */
    void processInput(const DS4_InputUSBReport &report) override {
        HIDDriver::processInput(report);
        // Add any DS4-specific input processing here
    }

private:
    /**
     * Helper function to get button state from a report.
     * @param report The input report to check.
     * @param button The button to check.
     * @return true if the button is pressed in the report, false otherwise.
     */
    static bool getButtonState(const DS4_InputUSBReport &report, DS4Button button);
};

/**
 * Enum for keyboard modifier keys
 */
enum class KBModifier {
    LeftCtrl,
    LeftShift,
    LeftAlt,
    LeftGui,
    RightCtrl,
    RightShift,
    RightAlt,
    RightGui
};

/**
 * Enum for keyboard LED indicators
 */
enum class KBLed {
    NumLock,
    CapsLock,
    ScrollLock,
    Compose,
    Kana
};

/**
 * USB HID Keyboard Scan Codes (common keys)
 * Full list: https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
 */
enum class KBKey : uint8_t {
    None = 0x00,
    ErrorRollOver = 0x01,

    // Letters
    A = 0x04, B = 0x05, C = 0x06, D = 0x07, E = 0x08, F = 0x09,
    G = 0x0A, H = 0x0B, I = 0x0C, J = 0x0D, K = 0x0E, L = 0x0F,
    M = 0x10, N = 0x11, O = 0x12, P = 0x13, Q = 0x14, R = 0x15,
    S = 0x16, T = 0x17, U = 0x18, V = 0x19, W = 0x1A, X = 0x1B,
    Y = 0x1C, Z = 0x1D,

    // Numbers
    Num1 = 0x1E, Num2 = 0x1F, Num3 = 0x20, Num4 = 0x21, Num5 = 0x22,
    Num6 = 0x23, Num7 = 0x24, Num8 = 0x25, Num9 = 0x26, Num0 = 0x27,

    // Special keys
    Enter = 0x28,
    Escape = 0x29,
    Backspace = 0x2A,
    Tab = 0x2B,
    Space = 0x2C,
    Minus = 0x2D,
    Equals = 0x2E,
    LeftBracket = 0x2F,
    RightBracket = 0x30,
    Backslash = 0x31,
    Semicolon = 0x33,
    Apostrophe = 0x34,
    Grave = 0x35,
    Comma = 0x36,
    Period = 0x37,
    Slash = 0x38,
    CapsLock = 0x39,

    // Function keys
    F1 = 0x3A, F2 = 0x3B, F3 = 0x3C, F4 = 0x3D, F5 = 0x3E, F6 = 0x3F,
    F7 = 0x40, F8 = 0x41, F9 = 0x42, F10 = 0x43, F11 = 0x44, F12 = 0x45,

    // Navigation
    PrintScreen = 0x46,
    ScrollLock = 0x47,
    Pause = 0x48,
    Insert = 0x49,
    Home = 0x4A,
    PageUp = 0x4B,
    Delete = 0x4C,
    End = 0x4D,
    PageDown = 0x4E,
    Right = 0x4F,
    Left = 0x50,
    Down = 0x51,
    Up = 0x52,

    // Keypad
    NumLock = 0x53,
    KPDivide = 0x54,
    KPMultiply = 0x55,
    KPMinus = 0x56,
    KPPlus = 0x57,
    KPEnter = 0x58,
    KP1 = 0x59, KP2 = 0x5A, KP3 = 0x5B, KP4 = 0x5C, KP5 = 0x5D,
    KP6 = 0x5E, KP7 = 0x5F, KP8 = 0x60, KP9 = 0x61, KP0 = 0x62,
    KPPeriod = 0x63
};

/**
 * Driver class for a USB HID Keyboard.
 * Extends the base HIDDriver functionality with keyboard-specific features.
 */
class KeyboardDriver : public HIDDriver<HID_KeyboardInputReport, HID_KeyboardOutputReport, 4> {
public:
    KeyboardDriver();

    /**
     * Check if a modifier key is currently being held down.
     * @param modifier The modifier to check.
     * @return true if the modifier is currently pressed, false otherwise.
     */
    bool isModifierDown(KBModifier modifier) const;

    /**
     * Check if a modifier was just pressed this frame.
     * @param modifier The modifier to check.
     * @return true if the modifier was just pressed, false otherwise.
     */
    bool isModifierPressed(KBModifier modifier) const;

    /**
     * Check if a modifier was just released this frame.
     * @param modifier The modifier to check.
     * @return true if the modifier was just released, false otherwise.
     */
    bool isModifierReleased(KBModifier modifier) const;

    /**
     * Check if a specific key is currently being held down.
     * @param key The key to check.
     * @return true if the key is currently pressed, false otherwise.
     */
    bool isKeyDown(KBKey key) const;

    /**
     * Check if a key was just pressed this frame.
     * @param key The key to check.
     * @return true if the key was just pressed, false otherwise.
     */
    bool isKeyPressed(KBKey key) const;

    /**
     * Check if a key was just released this frame.
     * @param key The key to check.
     * @return true if the key was just released, false otherwise.
     */
    bool isKeyReleased(KBKey key) const;

    /**
     * Get the state of an LED indicator.
     * @param led The LED to check.
     * @return true if the LED is on, false otherwise.
     */
    bool getLedState(KBLed led) const;

    /**
     * Get the number of keys currently pressed (excluding modifiers).
     * @return Number of keys pressed (0-6).
     */
    int getKeyCount() const;

    /**
     * Get a specific pressed key by index.
     * @param index Index (0-5) of the key slot to check.
     * @return The key code, or KBKey::None if no key in that slot.
     */
    KBKey getPressedKey(int index) const;

    /**
     * Override processInput to add any keyboard-specific processing if needed
     */
    void processInput(const HID_KeyboardInputReport &report) override;

private:
    /**
     * Helper function to get modifier state from a report.
     */
    static bool getModifierState(const HID_KeyboardInputReport &report, KBModifier modifier);

    /**
     * Helper function to check if a key is in the keys array.
     */
    static bool isKeyInReport(const HID_KeyboardInputReport &report, KBKey key);
};
