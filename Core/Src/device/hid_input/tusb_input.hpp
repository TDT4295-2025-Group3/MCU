#pragma once

#include "hid_driver.hpp"
#include "iinput.hpp"

typedef enum {
    UNDEFINED = 0,
    DS4 = 1,
    KEYBOARD = 2,
} HIDInputType;

/**
 * TinyUSB-based input handler supporting DS4 controllers and keyboards.
 * Implements a singleton pattern to ensure a single instance.
 * You also need to periodically call tusb_driver_task() in your main loop to handle OUT reports etc.
 *
 * Use as a normal IInput implementation via getInstance().
 *
 * Complete installation:
 *  1) Setup TinyUSB
 *      - call tuh_init(0) in your main initialization code
 *      - periodically call tuh_task() in your main loop to ensure USB callbacks are handled
*   2) Periodically call TinyUSBInput::driverTask() in your main loop to handle report reading, etc.
*   3) Use TinyUSBInput::getInstance() to get the input handler and use it in the game.
 */
class TinyUSBInput : public IInput {

public:
    KeyState poll() override;

    static TinyUSBInput& getInstance() {
        static TinyUSBInput instance;
        return instance;
    }

    // Delete copy and move constructors to enforce singleton behavior
    TinyUSBInput(const TinyUSBInput&) = delete;
    TinyUSBInput& operator=(const TinyUSBInput&) = delete;
    TinyUSBInput(TinyUSBInput&&) = delete;
    TinyUSBInput& operator=(TinyUSBInput&&) = delete;

    void setRumble(float x) override;

    void clearController() {
        _controller_type = UNDEFINED;
    }

    void mountController(HIDInputType type, uint8_t dev_addr, uint8_t instance) {
        _controller_type = type;
        _usb_dev_addr = dev_addr;
        _usb_instance = instance;

        // force flush output report to restore state (e.g. LEDs) incase controller was reconnected
        if (type == DS4) {
            _hid_ds4.flushOutput(true);
        } else if (type == KEYBOARD) {
            _hid_keyboard.flushOutput(true);
        }
    }

    void driverTask();


    void processReport(uint8_t const *report, uint16_t len);

private:
    TinyUSBInput() = default;
    ~TinyUSBInput() = default;

    DS4Driver _hid_ds4;
    KeyboardDriver _hid_keyboard;

    uint8_t _usb_dev_addr, _usb_instance;

    HIDInputType _controller_type = UNDEFINED;
};