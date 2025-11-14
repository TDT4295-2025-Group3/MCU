#include "tusb_input.hpp"
#include "usbh.h"
#include "hid_host.h"
#include "tusb.h"
#include "stm32u5xx_hal.h"
#include "seven_seg_display.hpp"

namespace
{
constexpr uint16_t kSonyVendorId = 0x054C;

bool isDualShock4(uint16_t vid, uint16_t pid)
{
    if (vid != kSonyVendorId)
    {
        return false;
    }

    switch (pid)
    {
    case 0x05C4: // DualShock 4 (CUH-ZCT1)
    case 0x09CC: // DualShock 4 (CUH-ZCT2)
    case 0x0BA0: // DualShock 4 Wireless Controller
    case 0x0CE6: // DualShock 4 USB Wireless Adapter
    case 0x0DAE: // Additional Sony DS4 variation
        return true;
    default:
        return false;
    }
}
}
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
            ks.cam_x = _hid_ds4.getStickPosition(DS4Stick::Right).x;;
            ks.cam_y = _hid_ds4.getStickPosition(DS4Stick::Right).y;
            ks.x = _hid_ds4.getStickPosition(DS4Stick::Left).x;
            ks.y = _hid_ds4.getStickPosition(DS4Stick::Left).y;
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

void TinyUSBInput::mountController(HIDInputType type, uint8_t dev_addr, uint8_t instance, bool hasOutEndpoint)
{
    _controller_type = type;
    _usb_dev_addr = dev_addr;
    _usb_instance = instance;
    _ds4SupportsOutput = false;
    _ds4DisplayedWait = false;
    _ds4DisplayedReady = false;

    if (type == DS4)
    {
        _ds4FrameCount = 0;
        _ds4LastOutputTick = 0;
        _ds4SupportsOutput = hasOutEndpoint;
        _hid_ds4.queueInitReport();

        // Ensure the device operates in Report protocol for full feature set
        if (!tuh_hid_set_protocol(dev_addr, instance, HID_PROTOCOL_REPORT))
        {
            SevenSeg::displayChars("HELP");
        }

        SevenSeg::displayNumber(0);

        if (!_ds4SupportsOutput)
        {
            SevenSeg::displayChars("1111");
        }
    }
}

void TinyUSBInput::driverTask() {
    // send output reports periodically
    if (_controller_type == UNDEFINED) {
        return;
    }

    if (_controller_type == DS4) {
        constexpr uint32_t kDs4KeepAlivePeriodMs = 5U;
        const uint32_t now = HAL_GetTick();

        if (!_ds4SupportsOutput) {
            return;
        }

        if ((now - _ds4LastOutputTick) >= kDs4KeepAlivePeriodMs) {
            if (!tuh_hid_send_ready(_usb_dev_addr, _usb_instance)) {
                if (!_ds4DisplayedWait)
                {
                    SevenSeg::displayChars("2222");
                    _ds4DisplayedWait = true;
                }
                return;
            }

            if (!_ds4DisplayedReady)
            {
                SevenSeg::displayChars("4444 ");
                _ds4DisplayedReady = true;
            }

            DS4_OutputUSBReport report;
            if (_hid_ds4.getReadyOutputReport(report)) {
                const uint8_t reportId = report.ReportID;
                const uint8_t* payload = reinterpret_cast<const uint8_t*>(&report) + 1U;
                const uint16_t payloadLength = static_cast<uint16_t>(sizeof(report) - 1U);

                if (tuh_hid_send_report(_usb_dev_addr, _usb_instance, reportId, payload, payloadLength))
                {
                    _ds4LastOutputTick = now;
                    _hid_ds4.requeueOutputReport(report);
                }
                else
                {
                    // Failed unexpectedly; requeue so we can retry later
                    SevenSeg::displayChars("HHHH");
                    _hid_ds4.requeueOutputReport(report);
                }
            }
        }
    }
    else if (_controller_type == KEYBOARD) {
        HID_KeyboardOutputReport report;
        if (_hid_keyboard.getReadyOutputReport(report)) {
            // TODO: double check report ID
            tuh_hid_set_report(_usb_dev_addr, _usb_instance, 0, HID_REPORT_TYPE_OUTPUT, &report, sizeof(report));
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

                _ds4FrameCount = static_cast<uint16_t>((_ds4FrameCount + 1) % 10000);
                SevenSeg::displayNumber(_ds4FrameCount);
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

extern "C" void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report,
                                 uint16_t desc_len) {
    (void)desc_report;
    (void)desc_len;
    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    TinyUSBInput& input = TinyUSBInput::getInstance();

    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    bool hasOutEndpoint = true;
    tuh_itf_info_t itfInfo{};
    if (tuh_hid_itf_get_info(dev_addr, instance, &itfInfo))
    {
        hasOutEndpoint = (itfInfo.desc.bNumEndpoints > 1U);
        if (!hasOutEndpoint)
        {
            SevenSeg::displayChars("NOO ");
        }
    }
    else
    {
        SevenSeg::displayChars("INFO");
    }

    // Device identifies itself as keyboard
    if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
        input.mountController(KEYBOARD, dev_addr, instance);
    }
    // else, check if is DS4
    else {
        // Determine device type based on VID/PID or report descriptor
        if (isDualShock4(vid, pid)) {
            input.mountController(DS4, dev_addr, instance, hasOutEndpoint);
        }
    }

    // Always queue the next report so the controller keeps sending data.
    (void)tuh_hid_receive_report(dev_addr, instance);
}

extern "C" void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    (void)dev_addr;
    (void)instance;

    TinyUSBInput& input = TinyUSBInput::getInstance();
    input.clearController();
}

extern "C" void tuh_mount_cb(uint8_t dev_addr) {
    (void)dev_addr;
    SevenSeg::displayChars("EEEE");
}

extern "C" void tuh_umount_cb(uint8_t dev_addr) {
    (void)dev_addr;
    TinyUSBInput::getInstance().clearController();
    SevenSeg::displayChars("LLLL");
}

extern "C" void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {

    TinyUSBInput& input = TinyUSBInput::getInstance();

    SevenSeg::displayString("PPPP");

    input.processReport(report, len);

    // Request next report
    if (!tuh_hid_receive_report(dev_addr, instance)) {
        SevenSeg::displayChars("H  H");
    }
}

extern "C" uint32_t tusb_time_millis_api(void) {
    return HAL_GetTick();
}