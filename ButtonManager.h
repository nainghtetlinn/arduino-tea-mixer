#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <ArduinoJson.h>
#include "TeaData.h"

struct ButtonInfo
{
    String name;
    TeaRecipe recipe;
};

void initButtons();
void updateButtons();

// command handlers
void handleGetButtonsInfoCommand();
void handleSetButtonInfoCommand(JsonDocument doc);

#endif
