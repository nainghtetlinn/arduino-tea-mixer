#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =========================================================================
// Debug & Logging Configuration
// =========================================================================
// Set to 1 to enable Serial logs, or 0 to strip them completely at compile-time
#define DEBUG_MODE 1

#if DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

// =========================================================================
// Hardware Configuration
// =========================================================================
constexpr uint8_t BUTTON_COUNT = 3;
constexpr uint8_t RELAY_COUNT = 4;

// =========================================================================
// BLE Command Types
// =========================================================================
namespace BleCommand
{
    constexpr const char *MAKE_TEA = "MAKE_TEA";
    constexpr const char *GET_BUTTONS_INFO = "GET_BUTTONS_INFO";
    constexpr const char *SET_BUTTON_INFO = "SET_BUTTON_INFO";
    constexpr const char *GET_DEVICE_INFO = "GET_DEVICE_INFO";
    constexpr const char *SET_DISPENSING_MODE = "SET_DISPENSING_MODE";
} // namespace BleCommand

// =========================================================================
// BLE Response Types
// =========================================================================
namespace BleResponse
{
    constexpr const char *BUTTONS_INFO = "BUTTONS_INFO";
    constexpr const char *DEVICE_INFO = "DEVICE_INFO";
    constexpr const char *DISPENSING_MODE = "DISPENSING_MODE";
    constexpr const char *TEA_START = "TEA_START";
    constexpr const char *TEA_PROGRESS = "TEA_PROGRESS";
    constexpr const char *TEA_FINISH = "TEA_FINISH";
    constexpr const char *CUP_REMOVED = "CUP_REMOVED";
    constexpr const char *CUP_WARNING = "CUP_WARNING";
    constexpr const char *ERROR = "ERROR";
    constexpr const char *CLEANING_START = "CLEANING_START";
    constexpr const char *CLEANING_FINISHED = "CLEANING_FINISHED";
    constexpr const char *CLEANING_PROGRESS = "CLEANING_PROGRESS";
    constexpr const char *CLEANING_CANCELLED = "CLEANING_CANCELLED";
} // namespace BleResponse

#endif