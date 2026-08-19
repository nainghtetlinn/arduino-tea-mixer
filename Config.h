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

#endif