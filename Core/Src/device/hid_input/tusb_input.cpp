#include "tusb_input.hpp"

#include <algorithm>
#include <cstdio>

#include "usbh.h"
#include "hid_host.h"
#include "main.h"
#include "stm32u5xx_hal.h"
#include "tusb.h"
/**
 * Class to actually handle TinyUSB-based callbacks and implement TinyUSBInput.
 *
 * Good resources:
 *  - for DS4: https://github.com/hathach/tinyusb/blob/master/examples/host/hid_controller/src/hid_app.c
 *  - for keyboard: https://github.com/hathach/tinyusb/blob/master/examples/host/cdc_msc_hid/src/hid_app.c
 */

KeyState TinyUSBInput::poll() {
    KeyState ks{};
    switch (_controller_type) {
        case DS4:
            ks.cam_x = -_hid_ds4.getStickPosition(DS4Stick::Right).x;;
            ks.cam_y = _hid_ds4.getStickPosition(DS4Stick::Right).y;
            ks.x = _hid_ds4.getStickPosition(DS4Stick::Left).x;
            ks.y = -_hid_ds4.getStickPosition(DS4Stick::Left).y;
            ks.space = _hid_ds4.isKeyDown(DS4Button::Cross);
            break;
        case KEYBOARD:
            ks.cam_x = (_hid_keyboard.isKeyDown(KBKey::Right) ? 1.0f : 0.0f) - (_hid_keyboard.isKeyDown(KBKey::Left)
                           ? 1.0f
                           : 0.0f);
            ks.cam_y = (_hid_keyboard.isKeyDown(KBKey::Up) ? 1.0f : 0.0f) - (_hid_keyboard.isKeyDown(KBKey::Down)
                                                                                 ? 1.0f
                                                                                 : 0.0f);
            ks.x = (_hid_keyboard.isKeyDown(KBKey::D) ? 1.0f : 0.0f) - (
                       _hid_keyboard.isKeyDown(KBKey::A) ? 1.0f : 0.0f);
            ks.y = (_hid_keyboard.isKeyDown(KBKey::W) ? 1.0f : 0.0f) - (
                       _hid_keyboard.isKeyDown(KBKey::S) ? 1.0f : 0.0f);
            ks.space = _hid_keyboard.isKeyDown(KBKey::Space);
            break;
        default:
            break;
    }
    return ks;
}

void TinyUSBInput::driverTask() {
    tuh_task();

    // send output reports periodically
    if (_controller_type == UNDEFINED) {
        return;
    }

    if (_controller_type == DS4) {
        DS4_OutputUSBReport report;
        _hid_ds4.flushOutput();
        while (_hid_ds4.getReadyOutputReport(report)) {
            tuh_hid_send_report(_usb_dev_addr, _usb_instance, 0x5, &report, sizeof(report));
            // TODO - doublecheck 0x5 report ID is correct
        }
    }
    else if (_controller_type == KEYBOARD) {
        HID_KeyboardOutputReport report;
        while (_hid_keyboard.getReadyOutputReport(report)) {
            // TODO: double check report ID
            tuh_hid_send_report(_usb_dev_addr, _usb_instance, 0, &report, sizeof(report));
        }
    }
}

void TinyUSBInput::processReport(uint8_t const *report, uint16_t len) {
    switch (_controller_type) {
        case DS4:
            if (len == sizeof(DS4_InputUSBReport)) {
                if (report[0] != 0x01) {
                    // Not a DS4 input report
                    break;
                }
                DS4_InputUSBReport ds4_report;
                memcpy(&ds4_report, report, sizeof(DS4_InputUSBReport));
                _hid_ds4.processInput(ds4_report);
            }
            break;
        case KEYBOARD:
            if (len == sizeof(HID_KeyboardInputReport)) {
                HID_KeyboardInputReport kb_report;
                memcpy(&kb_report, report, sizeof(HID_KeyboardInputReport));
                _hid_keyboard.processInput(kb_report);
            }
            break;
        default:
            break;
    }
}

void TinyUSBInput::setRumble(float x) {
    if (x == lastRumble) return;

    if (_controller_type == DS4) {
        x = std::clamp(x, 0.0f, 1.0f);

        const float a = 1.0f;   // strong contribution
        const float b = 0.2f;   // weak contribution

        // First fill the strong motor
        float strongF = std::min(x / a, 1.0f);

        // Remaining energy goes to weak
        float remaining = x - strongF * a;
        float weakF = std::clamp(remaining / b, 0.0f, 1.0f);

        auto rumbleStrong = static_cast<uint8_t>(strongF * 255.0f);
        auto rumbleWeak = static_cast<uint8_t>(weakF * 255.0f);

        _hid_ds4.setRumbleStrong(rumbleStrong);
        _hid_ds4.setRumbleWeak(rumbleWeak);
        lastRumble = x;
    }
}

TinyUSBInput::TinyUSBInput() {
    // Initalize USB host
    // USB power active low -> disable power, init tinyusb, then enable power
    HAL_GPIO_WritePin(USB_Enable_GPIO_Port, USB_Enable_Pin, GPIO_PIN_SET);
    if (!tuh_init(BOARD_TUH_RHPORT))
    {
        Error_Handler();
    }
    HAL_Delay(500);
    HAL_GPIO_WritePin(USB_Enable_GPIO_Port, USB_Enable_Pin, GPIO_PIN_RESET);
    _usb_dev_addr = 0;
    _usb_instance = 0;
}


extern "C" void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report,
                                 uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    TinyUSBInput& input = TinyUSBInput::getInstance();

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    // Device identifies itself as keyboard
    if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
        input.mountController(KEYBOARD, dev_addr, instance);
    }
    // else, check if is DS4
    else {
        // Determine device type based on VID/PID or report descriptor
        if ((vid == 0x054C && pid == 0x09CC) /* Sony DS4 VIDs/PIDs */) {
            input.mountController(DS4, dev_addr, instance);
        }
    }

    if (!tuh_hid_receive_report(dev_addr, instance)) {
        printf("Error: cannot request to receive report\r\n");
    }
}

extern "C" void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr;
    (void)instance;

    TinyUSBInput& input = TinyUSBInput::getInstance();
    input.clearController();
}

extern "C" void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {

    TinyUSBInput& input = TinyUSBInput::getInstance();

    input.processReport(report, len);

    // Request next report
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        printf("Cannot request to receive more reports\r\n");
    }
}

extern "C" uint32_t tusb_time_millis_api(void) {
    return HAL_GetTick();
}