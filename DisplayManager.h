#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

// Initializes the SSD1306 OLED display. Returns false if it is unavailable.
bool initDisplay();

// Draws the screen shown when no tea is being made.
void displayIdleScreen();

// Shows the completed-tea message for three seconds.
void displayTeaReady();

// Updates time-based display states. Call this regularly from loop().
void updateDisplay();

// Draws the animated tea icon and brewing progress (0-100 percent).
void displayTeaProgress(uint8_t progress);

#endif
