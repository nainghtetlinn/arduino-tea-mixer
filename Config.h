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
// Pin Mapping
// =========================================================================

// --- SensorManager Pins ---
constexpr uint8_t PIN_HX711_DT = 14;
constexpr uint8_t PIN_HX711_CLK = 13;
constexpr uint8_t PIN_ONE_WIRE = 4;

// --- Display / I2C Pins ---
constexpr uint8_t PIN_I2C_SDA = 21;
constexpr uint8_t PIN_I2C_SCL = 22;

// --- Button Pins ---
constexpr uint8_t PIN_BTN_1 = 27;
constexpr uint8_t PIN_BTN_2 = 26;
constexpr uint8_t PIN_BTN_3 = 25;

// =========================================================================
// Default System Parameters
// =========================================================================

// --- Sensor Settings ---
constexpr unsigned long SENSOR_UPDATE_INTERVAL_MS = 500;
constexpr float DEFAULT_CALIBRATION_FACTOR = 886.47f;

// --- Display Settings ---
constexpr unsigned long ANIMATION_SPEED_MS = 300;

// --- Tea Settings ---
constexpr unsigned long TEA_UPDATE_INTERVAL_MS = 200;
constexpr unsigned long TEA_FINISHED_INTERVAL_MS = 2000;

// --- Storage Keys ---
constexpr char PREFS_NAMESPACE[] = "sensors";
constexpr char PREFS_KEY_CAL_FACTOR[] = "cal_factor";

constexpr char PREFS_NAMESPACE_TEA[] = "tea_settings";

#endif