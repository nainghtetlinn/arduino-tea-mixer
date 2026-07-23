#ifndef TEA_MANAGER_H
#define TEA_MANAGER_H

#include <ArduinoJson.h>
#include "Models.h"

typedef void (*TeaSendJsonCallback)(const String &type, const JsonDocument &data);
typedef void (*TeaSendErrorCallback)(const String &errorCode, const String &errorMessage);

class TeaManager
{
public:
    TeaManager();
    void begin(TeaSendJsonCallback sendJsonCb, TeaSendErrorCallback sendErrorCb);
    void update();

    void handleMakeTeaCommand(const JsonDocument &doc);
    void startMakingTea(const TeaRecipe &recipe);

    TeaState getState() const;
    uint8_t getProgress() const;

private:
    TeaRecipe target;
    TeaRecipe current;
    TeaState currentState;
    uint8_t currentProgress;
    unsigned long lastUpdate;
    unsigned long lastFinished;

    TeaSendJsonCallback teaSendJsonCb;
    TeaSendErrorCallback teaSendErrorCb;

    void sendTeaStartJSON();
    void sendTeaProgressJSON();
    void sendTeaFinishedJSON();
    void calculateProgress();
};

#endif