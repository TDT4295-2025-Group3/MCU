#include "seven_seg_display.hpp"

#include <algorithm>
#include <cstring>

#include "main.h"

extern SPI_HandleTypeDef hspi2;

namespace {
constexpr uint8_t kCmdDecodeMode = 0x09;
constexpr uint8_t kCmdIntensity = 0x0A;
constexpr uint8_t kCmdScanLimit = 0x0B;
constexpr uint8_t kCmdShutdown = 0x0C;
constexpr uint8_t kCmdDisplayTest = 0x0F;
constexpr uint8_t kDigitRegisterBase = 0x01;
constexpr uint8_t kBlankCode = 0x0F; // In decode mode this blanks a digit.
constexpr uint8_t kMinusCode = 0x0A;
constexpr uint8_t kECode = 0x0B;
constexpr uint8_t kHCode = 0x0C;
constexpr uint8_t kLCode = 0x0D;
constexpr uint8_t kPCode = 0x0E;
constexpr uint32_t kSpiTimeoutMs = 10;
constexpr std::size_t kMaxDigits = 4;

void writeRegister(uint8_t address, uint8_t value) {
  uint8_t frame[2] = {address, value};
  HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port, SPI2_NCS_Pin, GPIO_PIN_RESET);
  const HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi2, frame, static_cast<uint16_t>(sizeof(frame)), kSpiTimeoutMs);
  if (status != HAL_OK) {
    HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port, SPI2_NCS_Pin, GPIO_PIN_SET);
    Error_Handler();
  }
  HAL_GPIO_WritePin(SPI2_NCS_GPIO_Port, SPI2_NCS_Pin, GPIO_PIN_SET);
}

void initializeController() {
  // Leave decode mode enabled for the first four digits so we can send raw BCD values.
  writeRegister(kCmdDisplayTest, 0x00);  // Disable display test mode.
  writeRegister(kCmdDecodeMode, 0x0F);   // Enable BCD decode for digits 0-3.
  writeRegister(kCmdScanLimit, 0x03);    // Scan digits 0..3.
  writeRegister(kCmdIntensity, 0x08);    // Medium intensity.
  writeRegister(kCmdShutdown, 0x01);     // Exit shutdown - normal operation.
}

uint8_t encodeSymbol(char symbol) {
  if ((symbol >= '0') && (symbol <= '9')) {
    return static_cast<uint8_t>(symbol - '0');
  }

  switch (symbol) {
    case '-':
      return kMinusCode;
    case 'E':
    case 'e':
      return kECode;
    case 'H':
    case 'h':
      return kHCode;
    case 'L':
    case 'l':
      return kLCode;
    case 'P':
    case 'p':
      return kPCode;
    case ' ':
      return kBlankCode;
    default:
      return kBlankCode;
  }
}

} // namespace

namespace SevenSeg {

void init() {
  initializeController();
  blank();
}

void displayDigits(const uint8_t* digits, std::size_t count) {
  const std::size_t digitsToWrite = (digits == nullptr) ? 0U : std::min(count, kMaxDigits);
  const std::size_t offset = kMaxDigits - digitsToWrite;

  for (std::size_t idx = 0; idx < kMaxDigits; ++idx) {
    uint8_t value = kBlankCode;
    if ((idx >= offset) && (digitsToWrite > 0U)) {
      value = static_cast<uint8_t>(digits[idx - offset] & 0x0F);
    }
    writeRegister(static_cast<uint8_t>(kDigitRegisterBase + idx), value);
  }
}

void displayNumber(uint16_t value) {
  uint8_t encoded[kMaxDigits];
  std::fill_n(encoded, kMaxDigits, kBlankCode);

  std::size_t digits = 0;
  if (value == 0U) {
    encoded[kMaxDigits - 1U] = 0U;
    digits = 1U;
  } else {
    std::size_t pos = kMaxDigits;
    while ((value > 0U) && (pos > 0U)) {
      encoded[--pos] = static_cast<uint8_t>(value % 10U);
      value /= 10U;
      ++digits;
    }
  }

  displayDigits(encoded + (kMaxDigits - digits), digits);
}

void displayChars(const char* text, std::size_t count) {
  if ((text == nullptr) || (count == 0U)) {
    blank();
    return;
  }

  const std::size_t charsToWrite = std::min(count, kMaxDigits);
  uint8_t encoded[kMaxDigits];
  std::fill_n(encoded, kMaxDigits, kBlankCode);

  const std::size_t offset = kMaxDigits - charsToWrite;
  for (std::size_t idx = 0; idx < charsToWrite; ++idx) {
    encoded[offset + idx] = encodeSymbol(text[idx]);
  }

  displayDigits(encoded, kMaxDigits);
}

void displayChars(const char* text) {
  if (text == nullptr) {
    blank();
    return;
  }

  displayChars(text, std::strlen(text));
}

void displayString(const char* text) {
  displayChars(text);
}

void blank() {
  for (std::size_t idx = 0; idx < kMaxDigits; ++idx) {
    writeRegister(static_cast<uint8_t>(kDigitRegisterBase + idx), kBlankCode);
  }
}

} // namespace SevenSeg
