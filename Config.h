#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Set to 1 to enable Serial logs, or 0 to completely strip them out of Flash
#define DEBUG_MODE 1

#if DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
// When DEBUG_MODE is 0, these turn into empty spaces during compilation
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

#endif