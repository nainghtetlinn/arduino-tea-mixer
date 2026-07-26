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
    void showIdle(const String &btn1, const String &btn2, const String &btn3);
    void showRecipePreview(const String &name, const TeaRecipe &recipe);
    void showReady();
    void showProgress(uint8_t progress);
    void showCleaningProgress(uint8_t remainingSeconds, uint8_t progress);
    void showCleaningComplete();
    void showCalibration(float temperature, float weight, bool weightMode);

private:
    Adafruit_SSD1306 display;
    bool displayReady;
};

#endif
