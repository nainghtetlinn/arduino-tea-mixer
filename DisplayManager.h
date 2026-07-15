#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>

// Initializes the SSD1306 OLED display. Returns false if it is unavailable.
bool initDisplay();

// Draws the animated tea icon and brewing progress (0-100 percent).
void displayTeaProgress(uint8_t progress);

#endif
