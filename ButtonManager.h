#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <ArduinoJson.h>
#include <OneButton.h>
#include "Models.h"

typedef void (*ButtonPressCallback)(const int &btnIndex);
typedef void (*ButtonComboCallback)();

class ButtonManager
{
public:
    ButtonManager();
    void begin();
    void attachPressCallback(ButtonPressCallback pressCb);
    void attachCleaningComboCallback(ButtonComboCallback comboCallback);
    void attachCalibrationComboCallback(ButtonComboCallback comboCallback);
    void update();
    void setButtonInfo(int index, ButtonInfo info);
    ButtonInfo getButtonInfo(int index);
    JsonDocument getButtonsInfoJSON();
    String getButtonName(int index);

private:
    OneButton buttons[3];
    ButtonPressCallback btnPressCb;
    ButtonComboCallback combo13Cb;
    ButtonComboCallback combo12Cb;

    unsigned long combo13HoldStartTime;
    bool combo13Triggered;

    unsigned long combo12HoldStartTime;
    bool combo12Triggered;

    bool suppressClick[3];

    void makeNvsKey(char *buffer, size_t bufferSize, int index, const char *field);
    void handleButtonClick(int btnIndex);
};

#endif
