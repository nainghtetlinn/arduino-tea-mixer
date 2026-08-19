#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "Models.h"

class DisplayManager
{
public:
    DisplayManager();
    bool begin();

    void showIdleScreen(const String &btn1, const String &btn2, const String &btn3, bool bleConnected);
    void showRecipePreviewScreen(const String &name, const TeaRecipe &recipe);

    void showTeaProgressScreen(uint8_t progress);
    void showTeaReadyScreen();

    void showCleaningProgressScreen(uint8_t remainingSeconds, uint8_t progress);
    void showCleaningCompleteScreen();

    void showCalibrationScreen(float temperature, float weight, bool weightMode);

    void showBleConnectedScreen();

    void showNoCupWarningScreen();
    void showCupRemovedWarningScreen();

private:
    Adafruit_SSD1306 display;
    bool ready;
};

#endif