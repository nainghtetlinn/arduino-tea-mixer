#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <ArduinoJson.h>
#include <OneButton.h>
#include "Models.h"

typedef void (*ButtonPressCallback)(const TeaRecipe &recipe);
typedef void (*ButtonSendJsonCallback)(const String &type, const JsonDocument &payload);

class ButtonManager
{
public:
    ButtonManager();
    void begin(ButtonPressCallback pressCb, ButtonSendJsonCallback sendCb);
    void update();

    void handleGetButtonsInfoCommand();
    void handleSetButtonInfoCommand(const JsonDocument &doc);

private:
    OneButton buttons[3];

    ButtonPressCallback btnPressCb;
    ButtonSendJsonCallback btnSendJsonCb;

    void makeNvsKey(char *buffer, size_t bufferSize, int index, const char *field);
    ButtonInfo getButtonInfo(int index);
    void setButtonInfo(int index, ButtonInfo info);

    void handleButtonClick(int btnIndex);
    void sendButtonsInfoJSON();
};

#endif
