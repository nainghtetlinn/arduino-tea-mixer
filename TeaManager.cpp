#include "TeaManager.h"
#include "Config.h"
#include "ButtonManager.h"
#include "DisplayManager.h"
#include "RelayManager.h"
#include "SensorManager.h"

namespace
{
    constexpr unsigned long TEA_FINISHED_INTERVAL_MS = 2000;
    constexpr unsigned long DISPENSING_CHECK_INTERVAL_MS = 100;

    // Dispensing mode: true = weight-based, false = time-based
    constexpr bool DISPENSING_MODE_WEIGHT = true;

    // Weight tolerance - stop relay when within this margin (grams)
    constexpr float WEIGHT_TOLERANCE_G = 2.0f;
    // Safety timeout - max dispensing time per liquid (ms), 0 = disabled
    constexpr unsigned long DISPENSING_SAFETY_TIMEOUT_MS = 300000; // 5 minutes

    // Time-based mode: flow rate in ml/s for each pump
    constexpr float FLOW_RATE_TEA_ML_PER_SEC = 5.0f; // ~300ml/min
    constexpr float FLOW_RATE_CONDENSED_MILK_ML_PER_SEC = 5.0f;
    constexpr float FLOW_RATE_EVAPORATED_MILK_ML_PER_SEC = 5.0f;
    constexpr float FLOW_RATE_MILK_ML_PER_SEC = 5.0f;

    // Weight-based mode: grams per ml conversion for each liquid
    constexpr float GRAMS_PER_ML_TEA = 1.0f;
    constexpr float GRAMS_PER_ML_CONDENSED_MILK = 1.0f;
    constexpr float GRAMS_PER_ML_EVAPORATED_MILK = 1.0f;
    constexpr float GRAMS_PER_ML_MILK = 1.0f;

} // anonymous namespace

TeaManager::TeaManager()
    : relayMgr(nullptr),
      sensorMgr(nullptr),
      displayMgr(nullptr),
      buttonMgr(nullptr),
      target({0, 0, 0, 0}),
      dispensed({0, 0, 0, 0}),
      currentState(TeaState::IDLE),
      currentProgress(0),
      lastUpdate(0),
      lastFinished(0),
      teaSendJsonCb(nullptr),
      teaSendErrorCb(nullptr),
      previewStartTime(0),
      currentPhase(DispensingPhase::DONE),
      weightAtPhaseStart(0.0f),
      phaseStartTime(0)
{
    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        relayStartTime[i] = 0;
        relayDuration[i] = 0;
        relayRunning[i] = false;
    }
}

void TeaManager::begin(RelayManager *relay, SensorManager *sensor, DisplayManager *display,
                       ButtonManager *button,
                       TeaSendJsonCallback sendJsonCb, TeaSendErrorCallback sendErrorCb)
{
    relayMgr = relay;
    sensorMgr = sensor;
    displayMgr = display;
    buttonMgr = button;
    teaSendJsonCb = sendJsonCb;
    teaSendErrorCb = sendErrorCb;
}

void TeaManager::update()
{
    switch (currentState)
    {
    case TeaState::PREVIEW:
    {
        // Auto-cancel preview after timeout
        if (millis() - previewStartTime >= PREVIEW_TIMEOUT_MS)
        {
            cancelPreview();
        }
        return;
    }

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
        unsigned long now = millis();
        if (now - lastUpdate < DISPENSING_CHECK_INTERVAL_MS)
            return;

        lastUpdate = now;

        if (DISPENSING_MODE_WEIGHT)
        {
            updateWeightMode();
        }
        else
        {
            updateTimeMode();
        }
        break;
    }

    default:
        break;
    }

    updateDisplay();
}

// =========================================================================
// Command Handling
// =========================================================================

void TeaManager::startMakingTea(const TeaRecipe &recipe)
{
    if (currentState == TeaState::MAKING)
    {
        if (teaSendErrorCb)
        {
            teaSendErrorCb("MAKE_TEA_ERROR", "Tea is already being made");
        }
        return;
    }

    target = recipe;
    dispensed = {0, 0, 0, 0};
    currentState = TeaState::MAKING;
    currentProgress = 0;
    lastUpdate = millis();

    DEBUG_PRINTF("[TeaManager] Starting dispensing (mode: %s)\n",
                 DISPENSING_MODE_WEIGHT ? "WEIGHT" : "TIME");
    DEBUG_PRINTF("[TeaManager] Target: tea=%d, cm=%d, em=%d, milk=%d\n",
                 target.tea, target.condensedMilk, target.evaporatedMilk, target.milk);

    if (DISPENSING_MODE_WEIGHT)
    {
        startWeightMode();
    }
    else
    {
        startTimeMode();
    }

    sendTeaStartJSON();
}

void TeaManager::previewRecipe(const ButtonInfo &info)
{
    previewInfo = info;
    previewStartTime = millis();
    currentState = TeaState::PREVIEW;

    if (displayMgr)
    {
        displayMgr->showRecipePreview(previewInfo.name, previewInfo.recipe);
    }

    DEBUG_PRINTF("[TeaManager] Preview: %s (tea=%d, cm=%d, em=%d, milk=%d)\n",
                 previewInfo.name.c_str(),
                 previewInfo.recipe.tea,
                 previewInfo.recipe.condensedMilk,
                 previewInfo.recipe.evaporatedMilk,
                 previewInfo.recipe.milk);
}

void TeaManager::confirmBrew()
{
    if (currentState != TeaState::PREVIEW)
        return;

    DEBUG_PRINTLN(F("[TeaManager] Confirmed — starting brew"));
    startMakingTea(previewInfo.recipe);
}

void TeaManager::cancelPreview()
{
    if (currentState != TeaState::PREVIEW)
        return;

    currentState = TeaState::IDLE;
    DEBUG_PRINTLN(F("[TeaManager] Preview cancelled"));
}

void TeaManager::stopAll()
{
    if (relayMgr)
    {
        relayMgr->stopAll();
    }

    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        relayRunning[i] = false;
    }

    currentState = TeaState::IDLE;
    currentPhase = DispensingPhase::DONE;
    currentProgress = 0;

    DEBUG_PRINTLN(F("[TeaManager] Emergency stop"));
}

TeaState TeaManager::getState() const
{
    return currentState;
}

uint8_t TeaManager::getProgress() const
{
    return currentProgress;
}

// =========================================================================
// Weight-Based Mode
// =========================================================================

void TeaManager::startWeightMode()
{
    // Record the current weight as baseline
    if (sensorMgr)
    {
        weightAtPhaseStart = sensorMgr->getWeight();
    }
    else
    {
        weightAtPhaseStart = 0.0f;
    }

    // Start with the first liquid that has a non-zero target
    currentPhase = DispensingPhase::TEA;
    startPhase(currentPhase);
}

void TeaManager::updateWeightMode()
{
    if (currentPhase == DispensingPhase::DONE)
    {
        finishDispensing();
        return;
    }

    // Safety timeout check
    if (DISPENSING_SAFETY_TIMEOUT_MS > 0)
    {
        unsigned long elapsed = millis() - phaseStartTime;
        if (elapsed > DISPENSING_SAFETY_TIMEOUT_MS)
        {
            DEBUG_PRINTLN(F("[TeaManager] SAFETY TIMEOUT - stopping"));
            stopAll();
            return;
        }
    }

    if (!sensorMgr)
        return;

    float currentWeight = sensorMgr->getWeight();
    float dispensedGrams = currentWeight - weightAtPhaseStart;
    if (dispensedGrams < 0)
        dispensedGrams = 0;

    int targetMl = getTargetMlForPhase(currentPhase);
    float gramsPerMl = getGramsPerMlForPhase(currentPhase);
    float targetGrams = targetMl * gramsPerMl;

    // Update dispensed amount in ml for progress tracking
    float dispensedMl = dispensedGrams / gramsPerMl;
    int dispensedMlInt = constrain((int)dispensedMl, 0, targetMl);

    switch (currentPhase)
    {
    case DispensingPhase::TEA:
        dispensed.tea = dispensedMlInt;
        break;
    case DispensingPhase::CONDENSED_MILK:
        dispensed.condensedMilk = dispensedMlInt;
        break;
    case DispensingPhase::EVAPORATED_MILK:
        dispensed.evaporatedMilk = dispensedMlInt;
        break;
    case DispensingPhase::MILK:
        dispensed.milk = dispensedMlInt;
        break;
    default:
        break;
    }

    // Report progress
    calculateProgress();
    sendTeaProgressJSON();

    // Check if this phase is complete
    if (dispensedGrams >= (targetGrams - WEIGHT_TOLERANCE_G))
    {
        // Phase complete - turn off current relay
        uint8_t relayIdx = getRelayIndexForPhase(currentPhase);
        if (relayMgr)
        {
            relayMgr->setRelay(relayIdx, false);
        }

        // Finalize dispensed amount for this phase
        switch (currentPhase)
        {
        case DispensingPhase::TEA:
            dispensed.tea = targetMl;
            break;
        case DispensingPhase::CONDENSED_MILK:
            dispensed.condensedMilk = targetMl;
            break;
        case DispensingPhase::EVAPORATED_MILK:
            dispensed.evaporatedMilk = targetMl;
            break;
        case DispensingPhase::MILK:
            dispensed.milk = targetMl;
            break;
        default:
            break;
        }

        DEBUG_PRINTF("[TeaManager] Phase complete. Dispensed: %.1fg\n", dispensedGrams);

        // Update baseline weight for next phase
        weightAtPhaseStart = currentWeight;

        // Move to next phase
        advancePhase();

        if (currentPhase != DispensingPhase::DONE)
        {
            startPhase(currentPhase);
        }
        else
        {
            finishDispensing();
        }
    }
}

void TeaManager::startPhase(DispensingPhase phase)
{
    int targetMl = getTargetMlForPhase(phase);

    // Skip phases with zero target
    if (targetMl <= 0)
    {
        advancePhase();
        if (currentPhase != DispensingPhase::DONE)
        {
            startPhase(currentPhase);
        }
        else
        {
            finishDispensing();
        }
        return;
    }

    uint8_t relayIdx = getRelayIndexForPhase(phase);
    if (relayMgr)
    {
        relayMgr->setRelay(relayIdx, true);
    }
    phaseStartTime = millis();

    DEBUG_PRINTF("[TeaManager] Phase started: relay %d, target %d ml\n", relayIdx, targetMl);
}

void TeaManager::advancePhase()
{
    switch (currentPhase)
    {
    case DispensingPhase::TEA:
        currentPhase = DispensingPhase::CONDENSED_MILK;
        break;
    case DispensingPhase::CONDENSED_MILK:
        currentPhase = DispensingPhase::EVAPORATED_MILK;
        break;
    case DispensingPhase::EVAPORATED_MILK:
        currentPhase = DispensingPhase::MILK;
        break;
    case DispensingPhase::MILK:
        currentPhase = DispensingPhase::DONE;
        break;
    case DispensingPhase::DONE:
        break;
    }
}

int TeaManager::getTargetMlForPhase(DispensingPhase phase) const
{
    switch (phase)
    {
    case DispensingPhase::TEA:
        return target.tea;
    case DispensingPhase::CONDENSED_MILK:
        return target.condensedMilk;
    case DispensingPhase::EVAPORATED_MILK:
        return target.evaporatedMilk;
    case DispensingPhase::MILK:
        return target.milk;
    default:
        return 0;
    }
}

float TeaManager::getGramsPerMlForPhase(DispensingPhase phase) const
{
    switch (phase)
    {
    case DispensingPhase::TEA:
        return GRAMS_PER_ML_TEA;
    case DispensingPhase::CONDENSED_MILK:
        return GRAMS_PER_ML_CONDENSED_MILK;
    case DispensingPhase::EVAPORATED_MILK:
        return GRAMS_PER_ML_EVAPORATED_MILK;
    case DispensingPhase::MILK:
        return GRAMS_PER_ML_MILK;
    default:
        return 1.0f;
    }
}

uint8_t TeaManager::getRelayIndexForPhase(DispensingPhase phase) const
{
    switch (phase)
    {
    case DispensingPhase::TEA:
        return 0;
    case DispensingPhase::CONDENSED_MILK:
        return 1;
    case DispensingPhase::EVAPORATED_MILK:
        return 2;
    case DispensingPhase::MILK:
        return 3;
    default:
        return 0;
    }
}

// =========================================================================
// Time-Based Mode
// =========================================================================

void TeaManager::startTimeMode()
{
    // Calculate duration for each relay based on flow rate
    const int targets[] = {target.tea, target.condensedMilk, target.evaporatedMilk, target.milk};
    unsigned long now = millis();

    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        if (targets[i] > 0)
        {
            float flowRate = getFlowRateForIndex(i);
            relayDuration[i] = (unsigned long)((targets[i] / flowRate) * 1000.0f);
            relayStartTime[i] = now;
            relayRunning[i] = true;
            if (relayMgr)
            {
                relayMgr->setRelay(i, true);
            }

            DEBUG_PRINTF("[TeaManager] Relay %d ON for %lu ms (%d ml)\n",
                         i, relayDuration[i], targets[i]);
        }
        else
        {
            relayDuration[i] = 0;
            relayStartTime[i] = 0;
            relayRunning[i] = false;
        }
    }
}

void TeaManager::updateTimeMode()
{
    unsigned long now = millis();
    bool anyActive = false;

    const int targets[] = {target.tea, target.condensedMilk, target.evaporatedMilk, target.milk};

    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        if (!relayRunning[i])
            continue;

        unsigned long elapsed = now - relayStartTime[i];

        if (elapsed >= relayDuration[i])
        {
            // This relay is done
            if (relayMgr)
            {
                relayMgr->setRelay(i, false);
            }
            relayRunning[i] = false;

            // Set dispensed to exact target
            switch (i)
            {
            case 0:
                dispensed.tea = targets[0];
                break;
            case 1:
                dispensed.condensedMilk = targets[1];
                break;
            case 2:
                dispensed.evaporatedMilk = targets[2];
                break;
            case 3:
                dispensed.milk = targets[3];
                break;
            }

            DEBUG_PRINTF("[TeaManager] Relay %d OFF (completed)\n", i);
        }
        else
        {
            anyActive = true;

            // Calculate proportional dispensed amount for progress
            float fraction = (float)elapsed / (float)relayDuration[i];
            int dispensedMl = (int)(targets[i] * fraction);

            switch (i)
            {
            case 0:
                dispensed.tea = dispensedMl;
                break;
            case 1:
                dispensed.condensedMilk = dispensedMl;
                break;
            case 2:
                dispensed.evaporatedMilk = dispensedMl;
                break;
            case 3:
                dispensed.milk = dispensedMl;
                break;
            }
        }
    }

    // Report progress
    calculateProgress();
    sendTeaProgressJSON();

    if (!anyActive)
    {
        finishDispensing();
    }
}

float TeaManager::getFlowRateForIndex(uint8_t index) const
{
    switch (index)
    {
    case 0:
        return FLOW_RATE_TEA_ML_PER_SEC;
    case 1:
        return FLOW_RATE_CONDENSED_MILK_ML_PER_SEC;
    case 2:
        return FLOW_RATE_EVAPORATED_MILK_ML_PER_SEC;
    case 3:
        return FLOW_RATE_MILK_ML_PER_SEC;
    default:
        return 1.0f;
    }
}

// =========================================================================
// Common
// =========================================================================

void TeaManager::finishDispensing()
{
    // Turn off all relays
    if (relayMgr)
    {
        relayMgr->stopAll();
    }

    for (uint8_t i = 0; i < RELAY_COUNT; i++)
    {
        relayRunning[i] = false;
    }

    currentPhase = DispensingPhase::DONE;
    currentState = TeaState::FINISHED;
    currentProgress = 100;
    lastFinished = millis();

    DEBUG_PRINTLN(F("[TeaManager] Dispensing complete"));

    sendTeaFinishedJSON();
}

// =========================================================================
// JSON Reporting
// =========================================================================

void TeaManager::sendTeaStartJSON()
{
    if (teaSendJsonCb)
    {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        target.toJson(obj);
        teaSendJsonCb("TEA_START", doc);
    }
}

void TeaManager::sendTeaProgressJSON()
{
    if (teaSendJsonCb)
    {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        dispensed.toJson(obj);
        obj["percentage"] = currentProgress;
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
    const int totalCurrent = dispensed.tea + dispensed.condensedMilk + dispensed.evaporatedMilk + dispensed.milk;

    if (totalTarget <= 0)
        currentProgress = 0;
    else
        currentProgress = constrain((totalCurrent * 100) / totalTarget, 0, 100);
}

void TeaManager::updateDisplay()
{
    if (!displayMgr)
        return;

    switch (currentState)
    {
    case TeaState::IDLE:
    {
        String n0 = buttonMgr ? buttonMgr->getButtonName(0) : F("---");
        String n1 = buttonMgr ? buttonMgr->getButtonName(1) : F("---");
        String n2 = buttonMgr ? buttonMgr->getButtonName(2) : F("---");
        displayMgr->showIdle(n0, n1, n2);
        break;
    }
    case TeaState::PREVIEW:
        displayMgr->showRecipePreview(previewInfo.name, previewInfo.recipe);
        break;
    case TeaState::MAKING:
        displayMgr->showProgress(currentProgress);
        break;
    case TeaState::FINISHED:
        displayMgr->showReady();
        break;
    default:
        break;
    }
}
