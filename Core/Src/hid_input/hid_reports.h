#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DualShock 4 reports (USB).
 * Reports based on from: https://controllers.fandom.com/wiki/Sony_DualShock_4
 */

#include <stdint.h>

#pragma pack(push, 1)

typedef struct DS4_OutputUSBReport {
    uint8_t ReportID;
    uint8_t EnableRumbleUpdate: 1;
    uint8_t EnableLedUpdate: 1;
    uint8_t EnableLedBlink: 1;
    uint8_t EnableExtWrite: 1;
    uint8_t EnableVolumeLeftUpdate: 1;
    uint8_t EnableVolumeRightUpdate: 1;
    uint8_t EnableVolumeMicUpdate: 1;
    uint8_t EnableVolumeSpeakerUpdate: 1;
    uint8_t UNK_RESET1: 1;
    uint8_t UNK_RESET2: 1;
    uint8_t UNK1: 1;
    uint8_t UNK2: 1;
    uint8_t UNK3: 1;
    uint8_t UNKPad: 3;
    uint8_t Empty1;
    uint8_t RumbleRight; // weak
    uint8_t RumbleLeft; // strong
    uint8_t LedRed;
    uint8_t LedGreen;
    uint8_t LedBlue;
    uint8_t LedFlashOnPeriod;
    uint8_t LedFlashOffPeriod;
    uint8_t ExtDataSend[8];
    uint8_t VolumeLeft;
    uint8_t VolumeRight;
    uint8_t VolumeMic;
    uint8_t VolumeSpeaker;
    uint8_t UNK_AUDIO1: 7;
    uint8_t UNK_AUDIO2: 1;
    uint8_t Pad[8];
} DS4_OutputUSBReport;

typedef enum DS4_DpadDirection : uint8_t {
    North = 0,
    NorthEast,
    East,
    SouthEast,
    South,
    SouthWest,
    West,
    NorthWest,
    None = 8
} DS4_DpadDirection;

typedef struct DS4_TouchFingerData {
    uint8_t Index: 7;
    uint8_t NotTouching: 1;
    uint16_t FingerX: 12;
    uint16_t FingerY: 12;
} DS4_TouchFingerData;

typedef struct DS4_TouchData {
    uint8_t Timestamp;
    DS4_TouchFingerData Finger[2];
} DS4_TouchData;

typedef struct DS4_InputUSBReport {
    uint8_t ReportID;
    uint8_t LeftStickX;
    uint8_t LeftStickY;
    uint8_t RightStickX;
    uint8_t RightStickY;
    DS4_DpadDirection DPad: 4;
    uint8_t ButtonSquare: 1;
    uint8_t ButtonCross: 1;
    uint8_t ButtonCircle: 1;
    uint8_t ButtonTriangle: 1;
    uint8_t ButtonL1: 1;
    uint8_t ButtonR1: 1;
    uint8_t ButtonL2Btn: 1;
    uint8_t ButtonR2Btn: 1;
    uint8_t ButtonShare: 1;
    uint8_t ButtonOptions: 1;
    uint8_t ButtonL3: 1;
    uint8_t ButtonR3: 1;
    uint8_t ButtonHome: 1;
    uint8_t ButtonPad: 1;
    uint8_t Counter: 6;
    uint8_t TriggerLeft;
    uint8_t TriggerRight;
    uint16_t Timestamp;
    uint8_t Temperature;
    int16_t AngularVelocityX;
    int16_t AngularVelocityZ;
    int16_t AngularVelocityY;
    int16_t AccelerometerX;
    int16_t AccelerometerY;
    int16_t AccelerometerZ;
    uint8_t ExtData[5];
    uint8_t PowerPercent: 4;
    uint8_t PluggedPowerCable: 1;
    uint8_t PluggedHeadphones: 1;
    uint8_t PluggedMic: 1;
    uint8_t PluggedExt: 1;
    uint8_t UnkExt1: 1;
    uint8_t UnkExt2: 1;
    uint8_t NotConnected: 1;
    uint8_t Unk1: 5;
    uint8_t Unk2;
    uint8_t TouchCount;
    DS4_TouchData Touch[3];
    uint8_t Pad[3];
} DS4_InputUSBReport;

#pragma pack(pop)

static const int INPUT_REPORT_LENGTH = sizeof(DS4_InputUSBReport);
static const int OUTPUT_REPORT_LENGTH = sizeof(DS4_OutputUSBReport);

/**
 * Keyboard reports
 * More info can be found at:
 * - https://www.kenkoonwong.com/blog/usb-hid-key-press-report/
 * - https://wiki.osdev.org/USB_Human_Interface_Devices
 **/

// Standard HID Keyboard Input Report (from keyboard to host)
typedef struct HID_KeyboardInputReport {
    // Modifier keys bitmask
    uint8_t LeftCtrl: 1;
    uint8_t LeftShift: 1;
    uint8_t LeftAlt: 1;
    uint8_t LeftGui: 1; // Windows/Command key
    uint8_t RightCtrl: 1;
    uint8_t RightShift: 1;
    uint8_t RightAlt: 1;
    uint8_t RightGui: 1;

    uint8_t reserved; // Reserved byte (always 0)
    uint8_t keys[6]; // Currently pressed regular keys (6-key rollover)
} HID_KeyboardInputReport;

// Standard HID Keyboard Output Report (from host to keyboard - typically just LEDs)
typedef struct HID_KeyboardOutputReport {
    uint8_t NumLock: 1;
    uint8_t CapsLock: 1;
    uint8_t ScrollLock: 1;
    uint8_t Compose: 1;
    uint8_t Kana: 1;
    uint8_t Pad: 3; // Padding bits
} HID_KeyboardOutputReport;

#ifdef __cplusplus
}
#endif
