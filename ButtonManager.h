#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <ArduinoJson.h>
#include <OneButton.h>
#include "Models.h"

typedef void (*ButtonPressCallback)(const int &btnIndex);

class ButtonManager
{
public:
    ButtonManager();
    void begin(ButtonPressCallback pressCb);
    void update();
    void setButtonInfo(int index, ButtonInfo info);
    ButtonInfo getButtonInfo(int index);
    JsonDocument getButtonsInfoJSON();
    String getButtonName(int index);

private:
    OneButton buttons[3];
    ButtonPressCallback btnPressCb;

    void makeNvsKey(char *buffer, size_t bufferSize, int index, const char *field);
    void handleButtonClick(int btnIndex);
};

#endif
