#include "TeaManager.h"
#include "Config.h"

TeaManager::TeaManager()
    : target({0, 0, 0, 0}),
      current({0, 0, 0, 0}),
      currentState(TeaState::IDLE),
      currentProgress(0),
      lastUpdate(0),
      lastFinished(0),

      teaSendJsonCb(nullptr),
      teaSendErrorCb(nullptr)
{
}

void TeaManager::begin(TeaSendJsonCallback sendJsonCb, TeaSendErrorCallback sendErrorCb)
{
    teaSendJsonCb = sendJsonCb;
    teaSendErrorCb = sendErrorCb;
}

void TeaManager::update()
{
    switch (currentState)
    {
    case TeaState::FINISHED:
    {
        unsigned long currentMillis = millis();
        if (currentMillis - lastFinished >= TEA_FINISHED_INTERVAL_MS)
        {
            currentState = TeaState::IDLE;
            currentProgress = 0;
            lastFinished = currentMillis;
        }
        return;
    }

    case TeaState::MAKING:
    {
        unsigned long currentMillis = millis();

        if (currentMillis - lastUpdate >= TEA_UPDATE_INTERVAL_MS)
        {
            lastUpdate = currentMillis;

            if (current.tea < target.tea)
            {
                current.tea += min(10, target.tea - current.tea);
            }
            else if (current.condensedMilk < target.condensedMilk)
            {
                current.condensedMilk += min(5, target.condensedMilk - current.condensedMilk);
            }
            else if (current.evaporatedMilk < target.evaporatedMilk)
            {
                current.evaporatedMilk += min(5, target.evaporatedMilk - current.evaporatedMilk);
            }
            else if (current.milk < target.milk)
            {
                current.milk += min(5, target.milk - current.milk);
            }
            else
            {
                currentState = TeaState::FINISHED;
                currentProgress = 100;
                lastFinished = currentMillis;

                sendTeaFinishedJSON();
                return;
            }

            calculateProgress();
            sendTeaProgressJSON();
        }
    }
    }
}

void TeaManager::handleMakeTeaCommand(const JsonDocument &doc)
{
    DEBUG_PRINTLN("HANDLE: MAKE_TEA");

    if (currentState == TeaState::MAKING)
    {
        if (teaSendErrorCb)
        {
            teaSendErrorCb("MAKE_TEA_ERROR", "Tea is already being made");
        }
        return;
    }

    JsonObjectConst payload = doc["payload"];
    if (payload.isNull())
    {
        DEBUG_PRINTLN(F("[TeaManager] Error: Invalid JSON payload"));
        if (teaSendErrorCb)
        {
            teaSendErrorCb("INVALID_PAYLOAD", "Missing or invalid payload object");
        }
        return;
    }

    TeaRecipe recipe;
    recipe.tea = payload["tea"] | 0;
    recipe.condensedMilk = payload["condensedMilk"] | 0;
    recipe.evaporatedMilk = payload["evaporatedMilk"] | 0;
    recipe.milk = payload["milk"] | 0;

    startMakingTea(recipe);
}

void TeaManager::startMakingTea(const TeaRecipe &recipe)
{
    if (currentState == TeaState::MAKING)
        return;

    target = recipe;
    current = {0, 0, 0, 0};
    currentState = TeaState::MAKING;
    currentProgress = 0;
    lastUpdate = millis();

    sendTeaStartJSON();
}

TeaState TeaManager::getState() const
{
    return currentState;
}

uint8_t TeaManager::getProgress() const
{
    return currentProgress;
}

void TeaManager::sendTeaStartJSON()
{
    JsonObject doc;
    target.toJson(doc);

    if (teaSendJsonCb)
    {
        teaSendJsonCb("TEA_START", doc);
    }
}

void TeaManager::sendTeaProgressJSON()
{
    JsonObject doc;
    current.toJson(doc);
    doc["percentage"] = currentProgress;

    if (teaSendJsonCb)
    {
        teaSendJsonCb("TEA_PROGRESS", doc);
    }
}

void TeaManager::sendTeaFinishedJSON()
{
    if (teaSendJsonCb)
    {
        JsonDocument emptyPayload;
        teaSendJsonCb("TEA_FINISH", emptyPayload);
    }
}

void TeaManager::calculateProgress()
{
    const int totalTarget = target.tea + target.condensedMilk + target.evaporatedMilk + target.milk;
    const int totalCurrent = current.tea + current.condensedMilk + current.evaporatedMilk + current.milk;

    if (totalTarget <= 0)
        currentProgress = 0;
    else
        currentProgress = constrain((totalCurrent * 100) / totalTarget, 0, 100);
}
