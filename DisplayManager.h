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
    void showIdle(const String &btn1, const String &btn2, const String &btn3,
                  float temperature = 0.0f, float weight = 0.0f);
    void showRecipePreview(const String &name, const TeaRecipe &recipe);
    void showReady();
    void showProgress(uint8_t progress);

private:
    Adafruit_SSD1306 display;
    bool displayReady;
};

#endif
