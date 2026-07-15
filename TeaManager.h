#ifndef TEA_MANAGER_H
#define TEA_MANAGER_H

#include <ArduinoJson.h>
#include "TeaData.h"

void startMakingTea(TeaRecipe recipe);
void updateTeaProgress();

// command handlers
void handleMakeTeaCommand(JsonDocument doc);

#endif