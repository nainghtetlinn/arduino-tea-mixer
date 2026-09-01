#include "TeaManager.h"
#include "Config.h"
#include "BluetoothManager.h"
#include "ButtonManager.h"
#include "DisplayManager.h"
#include "RelayManager.h"
#include "SensorManager.h"
#include <Preferences.h>

namespace
{
    constexpr unsigned long DISPENSING_CHECK_INTERVAL_MS = 100;
    constexpr unsigned long PREVIEW_TIMEOUT_MS = 5000;
    constexpr unsigned long BLE_CONNECTED_DISPLAY_MS = 3000;
    constexpr float CUP_REMOVED_THRESHOLD = 0.30f;         // weight must drop below 30% of finished weight
    constexpr unsigned long CUP_WARNING_TIMEOUT_MS = 5000; // auto-dismiss warning after 5s
    constexpr float DEFAULT_MIN_CUP_WEIGHT = 50.0f;        // default minimum cup weight in grams
    constexpr int DEFAULT_TARGET_TOTAL_ML = 300;
    constexpr unsigned long CUP_SETTLE_DELAY_MS = 1000; // 1 s settling delay after cup placed
} // anonymous namespace

// ===== Lifecycle =====

TeaManager::TeaManager()
    : relayMgr(nullptr),
      sensorMgr(nullptr),
      displayMgr(nullptr),
      buttonMgr(nullptr),
      bleMgr(nullptr),
      target({0, 0, 0, 0}),
      dispensed({0, 0, 0, 0}),
      currentState(TeaState::IDLE),
      currentProgress(0),
      lastUpdate(0),
      finishedWeight(0.0f),
      weightDispensingMode(true),
      calibratingCup(false),
      bleConnectedStartTime(0),
      minCupWeight(DEFAULT_MIN_CUP_WEIGHT),
      cupWarningStartTime(0),
      targetTotalMl(DEFAULT_TARGET_TOTAL_ML),
      previewButtonIndex(-1),
      previewStartTime(0),
      activeStrategy(nullptr),
      pendingRecipe({0, 0, 0, 0}),
      cupDetectedTime(0)
{
}

void TeaManager::begin(RelayManager *relay, SensorManager *sensor, DisplayManager *display,
                       ButtonManager *button, BluetoothManager *ble)
{
    relayMgr = relay;
    sensorMgr = sensor;
    displayMgr = display;
    buttonMgr = button;
    bleMgr = ble;

    cleaningMode.begin(relayMgr, displayMgr, bleMgr);
    weightStrategy.begin(sensorMgr, relayMgr);
    timeStrategy.begin(sensorMgr, relayMgr);

    loadDispensingMode();
    loadMinCupWeight();
    loadTargetTotalMl();

    DEBUG_PRINTLN(F("[TeaManager] Initialized successfully."));
}

// ===== Main Update Loop =====

void TeaManager::update()
{
    if (bleConnectedStartTime > 0)
    {
        if (millis() - bleConnectedStartTime >= BLE_CONNECTED_DISPLAY_MS)
        {
            bleConnectedStartTime = 0;
        }
        return;
    }

    switch (currentState)
    {
    case TeaState::IDLE:
    {
        if (buttonMgr && sensorMgr && bleMgr && displayMgr)
        {
            String n0 = buttonMgr ? buttonMgr->getButtonName(0) : F("---");
            String n1 = buttonMgr ? buttonMgr->getButtonName(1) : F("---");
            String n2 = buttonMgr ? buttonMgr->getButtonName(2) : F("---");
            float temp = sensorMgr ? sensorMgr->getTemperature() : 0.0f;
            bool ble = bleMgr ? bleMgr->isConnected() : false;
            displayMgr->showIdleScreen(n0, n1, n2, ble, temp);
        }

        return;
    }

    case TeaState::PREVIEW:
    {
        if (millis() - previewStartTime >= PREVIEW_TIMEOUT_MS)
        {
            cancelPreview();
        }
        return;
    }

    case TeaState::CLEANING:
    {
        if (cleaningMode.update())
        {
            currentState = TeaState::IDLE;
            currentProgress = 0;
        }
        return;
    }

    case TeaState::CALIBRATION:
    {
        if (sensorMgr && displayMgr)
        {
            displayMgr->showCalibrationScreen(sensorMgr->getTemperature(), sensorMgr->getWeight(), weightDispensingMode, minCupWeight, calibratingCup);
        }
        return;
    }

    case TeaState::WAITING_FOR_CUP:
    {
        if (!sensorMgr)
            return;

        float currentWeight = sensorMgr->getWeight();
        unsigned long now = millis();

        if (currentWeight >= minCupWeight)
        {
            // Cup just detected — start / continue settle timer
            if (cupDetectedTime == 0)
            {
                cupDetectedTime = now;
                DEBUG_PRINTLN(F("[TeaManager] Cup detected: waiting 1s before brewing"));
            }
            else if (now - cupDetectedTime >= CUP_SETTLE_DELAY_MS)
            {
                // Delay expired — kick off the actual brew
                DEBUG_PRINTLN(F("[TeaManager] Cup settle: starting dispensing"));
                startMakingTea(pendingRecipe);
            }
        }
        else
        {
            // Cup removed again — reset the settle timer
            if (cupDetectedTime != 0)
            {
                DEBUG_PRINTLN(F("[TeaManager] Cup removed again: resetting settle timer"));
                cupDetectedTime = 0;
            }
        }
        return;
    }

    case TeaState::MAKING:
    {
        unsigned long now = millis();
        if (now - lastUpdate < DISPENSING_CHECK_INTERVAL_MS)
            return;

        lastUpdate = now;

        if (activeStrategy)
        {
            // --- Mid-dispense cup removal check ---
            if (sensorMgr)
            {
                float currentWeight = sensorMgr->getWeight();
                if (currentWeight < minCupWeight)
                {
                    // Cup was removed mid-dispense — emergency stop
                    DEBUG_PRINTLN(F("[TeaManager] Cup removed during dispensing — stopping!"));
                    activeStrategy->stop();
                    activeStrategy = nullptr;

                    cupWarningStartTime = millis();
                    currentState = TeaState::CUP_WARNING;
                    currentProgress = 0;

                    if (displayMgr)
                    {
                        displayMgr->showCupRemovedWarningScreen();
                    }

                    sendCupRemovedWarningJSON();
                    return;
                }
            }

            bool done = activeStrategy->update(dispensed);
            calculateProgress();
            sendTeaProgressJSON();

            if (displayMgr)
            {
                displayMgr->showTeaProgressScreen(currentProgress);
            }

            if (done)
            {
                finishDispensing();
            }
        }
        return;
    }

    case TeaState::CUP_WARNING:
    {
        // Auto-dismiss after timeout (only for cup-removed-mid-dispense warning)
        if (millis() - cupWarningStartTime >= CUP_WARNING_TIMEOUT_MS)
        {
            dismissCupWarning();
        }
        return;
    }

    case TeaState::FINISHED:
    {
        if (sensorMgr)
        {
            float currentWeight = sensorMgr->getWeight();
            if (currentWeight < finishedWeight * CUP_REMOVED_THRESHOLD)
            {
                currentState = TeaState::IDLE;
                currentProgress = 0;
                DEBUG_PRINTLN(F("[TeaManager] Cup removed — returning to idle"));
                sendCupRemovedJSON();
            }
        }
        return;
    }

    default:
        break;
    }
}

// ===== Tea Dispensing & Brewing =====

void TeaManager::startMakingTea(const TeaRecipe &recipe)
{
    if (currentState == TeaState::MAKING)
    {
        if (bleMgr)
        {
            bleMgr->sendError("MAKE_TEA_ERROR", "Tea is already being made");
        }
        return;
    }

    // --- Pre-dispense cup check ---
    if (sensorMgr)
    {
        float currentWeight = sensorMgr->getWeight();
        if (currentWeight < minCupWeight)
        {
            DEBUG_PRINTF("[TeaManager] No cup detected (weight: %.1fg, min: %.1fg).\n",
                         currentWeight, minCupWeight);

            pendingRecipe = recipe;
            cupDetectedTime = 0;
            currentState = TeaState::WAITING_FOR_CUP;

            if (displayMgr)
            {
                displayMgr->showPlaceCupScreen();
            }

            if (bleMgr)
            {
                bleMgr->sendError("NO_CUP", "No cup detected — waiting for cup placement");
            }
            return;
        }
    }

    // Scale recipe ingredients proportionally to the target total ml
    target = scaleRecipeToTarget(recipe, targetTotalMl);
    dispensed = {0, 0, 0, 0};
    currentState = TeaState::MAKING;
    currentProgress = 0;
    lastUpdate = millis();

    if (weightDispensingMode)
    {
        activeStrategy = &weightStrategy;
    }
    else
    {
        activeStrategy = &timeStrategy;
    }
    activeStrategy->start(target);

    DEBUG_PRINTF("[TeaManager] Starting dispensing (mode: %s)\n",
                 weightDispensingMode ? "WEIGHT" : "TIME");
    DEBUG_PRINTF("[TeaManager] Raw recipe: tea=%d, cm=%d, em=%d, milk=%d\n",
                 recipe.tea, recipe.condensedMilk, recipe.evaporatedMilk, recipe.milk);
    DEBUG_PRINTF("[TeaManager] Scaled to %dml: tea=%d, cm=%d, em=%d, milk=%d\n",
                 targetTotalMl, target.tea, target.condensedMilk, target.evaporatedMilk, target.milk);

    sendTeaStartJSON();
}

// dismiss cup warning after cup is removed during dispensing
void TeaManager::dismissCupWarning()
{
    if (currentState != TeaState::CUP_WARNING)
        return;

    currentState = TeaState::IDLE;
    currentProgress = 0;
    DEBUG_PRINTLN(F("[TeaManager] Cup warning dismissed"));
}

// private helper
void TeaManager::finishDispensing()
{
    activeStrategy->stop();
    activeStrategy = nullptr;

    if (displayMgr)
    {
        displayMgr->showTeaReadyScreen();
    }

    currentState = TeaState::FINISHED;
    currentProgress = 100;
    finishedWeight = sensorMgr ? sensorMgr->getWeight() : 0.0f;

    DEBUG_PRINTLN(F("[TeaManager] Dispensing complete"));

    sendTeaFinishedJSON();
}

// private helper
void TeaManager::calculateProgress()
{
    const int totalTarget = target.tea + target.condensedMilk + target.evaporatedMilk + target.milk;
    const int totalCurrent = dispensed.tea + dispensed.condensedMilk + dispensed.evaporatedMilk + dispensed.milk;

    if (totalTarget <= 0)
        currentProgress = 0;
    else
        currentProgress = constrain((totalCurrent * 100) / totalTarget, 0, 100);
}

// void TeaManager::stopAll()
// {
//     if (activeStrategy)
//     {
//         activeStrategy->stop();
//         activeStrategy = nullptr;
//     }
//     else if (relayMgr)
//     {
//         relayMgr->stopAll();
//     }

//     currentState = TeaState::IDLE;
//     currentProgress = 0;

//     DEBUG_PRINTLN(F("[TeaManager] Emergency stop"));
// }

// ===== Recipe Preview Flow =====

void TeaManager::previewRecipe(int btnIndex, const ButtonInfo &info)
{
    previewButtonIndex = btnIndex;
    previewInfo = info;
    previewStartTime = millis();
    currentState = TeaState::PREVIEW;

    if (displayMgr)
    {
        displayMgr->showRecipePreviewScreen(previewInfo.name, previewInfo.recipe);
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

    DEBUG_PRINTF("[TeaManager] Confirmed: %s\n", previewInfo.name.c_str());
    previewButtonIndex = -1;
    startMakingTea(previewInfo.recipe);
}

void TeaManager::cancelPreview()
{
    if (currentState != TeaState::PREVIEW && currentState != TeaState::WAITING_FOR_CUP)
        return;

    currentState = TeaState::IDLE;
    previewButtonIndex = -1;
    DEBUG_PRINTLN(F("[TeaManager] Preview cancelled"));
}

// ===== Cleaning Mode =====

void TeaManager::startCleaningMode()
{
    if (currentState != TeaState::IDLE)
        return;

    currentState = TeaState::CLEANING;
    cleaningMode.start();
}

void TeaManager::cancelCleaningMode()
{
    if (currentState != TeaState::CLEANING)
        return;

    cleaningMode.cancel();
    currentState = TeaState::IDLE;
}

// ===== Calibration Mode =====

void TeaManager::startCalibrationMode()
{
    if (currentState != TeaState::IDLE)
        return;

    currentState = TeaState::CALIBRATION;
}

void TeaManager::exitCalibrationMode()
{
    if (currentState != TeaState::CALIBRATION)
        return;

    calibratingCup = false;
    currentState = TeaState::IDLE;
}

void TeaManager::resetCalibrationWeight()
{
    if (currentState != TeaState::CALIBRATION)
        return;

    if (sensorMgr)
    {
        sensorMgr->zeroTare();
    }
}

void TeaManager::toggleDispensingMode()
{
    if (currentState != TeaState::CALIBRATION)
        return;

    weightDispensingMode = !weightDispensingMode;
    saveDispensingMode(weightDispensingMode);

    DEBUG_PRINTF("[TeaManager] Toggle dispensing mode: %s\n",
                 weightDispensingMode ? "WEIGHT" : "TIME");
}

// will be called via ble
void TeaManager::setDispensingMode(bool isWeight)
{
    // Block during active dispensing or cleaning
    if (currentState == TeaState::MAKING || currentState == TeaState::CLEANING)
        return;

    weightDispensingMode = isWeight;
    saveDispensingMode(weightDispensingMode);

    DEBUG_PRINTF("[TeaManager] Set dispensing mode: %s\n",
                 isWeight ? "WEIGHT" : "TIME");
}

void TeaManager::startCupWeightCalibration()
{
    if (currentState != TeaState::CALIBRATION)
        return;

    calibratingCup = true;
}

void TeaManager::finishCupWeightCalibration()
{
    if (currentState != TeaState::CALIBRATION || !calibratingCup)
        return;

    if (sensorMgr)
    {
        float currentWeight = sensorMgr->getWeight() - 1.0f;
        if (currentWeight > 0.0f)
        {
            minCupWeight = currentWeight;
            saveMinCupWeight(currentWeight);
        }
        else
        {
            DEBUG_PRINTLN(F("[TeaManager] Cup weight not saved — weight is zero or negative"));
        }
    }

    calibratingCup = false;
}

void TeaManager::cancelCupWeightCalibration()
{
    calibratingCup = false;
}

void TeaManager::setTargetTotalMl(int ml)
{
    constexpr int MIN_ML = 200;
    constexpr int MAX_ML = 1000;
    if (ml < MIN_ML)
        ml = MIN_ML;
    if (ml > MAX_ML)
        ml = MAX_ML;
    targetTotalMl = ml;
    saveTargetTotalMl(ml);
}

void TeaManager::showBleConnected()
{
    bleConnectedStartTime = millis();
    if (displayMgr)
    {
        displayMgr->showBleConnectedScreen();
    }
}

// ===== Getters =====

int TeaManager::getPreviewButtonIndex() const
{
    return previewButtonIndex;
}

bool TeaManager::isWeightMode() const
{
    return weightDispensingMode;
}

bool TeaManager::isCalibratingCupWeight() const
{
    return calibratingCup;
}

float TeaManager::getMinCupWeight() const
{
    return minCupWeight;
}

int TeaManager::getTargetTotalMl() const
{
    return targetTotalMl;
}

TeaState TeaManager::getState() const
{
    return currentState;
}

// uint8_t TeaManager::getProgress() const
// {
//     return currentProgress;
// }

// ===== NVS Persistence =====

void TeaManager::loadDispensingMode()
{
    Preferences prefs;
    prefs.begin("tea_settings", true);
    weightDispensingMode = prefs.getBool("disp_mode", true);
    prefs.end();
    DEBUG_PRINTF("[TeaManager] Loaded dispensing mode from NVS: %s\n",
                 weightDispensingMode ? "WEIGHT" : "TIME");
}

void TeaManager::saveDispensingMode(bool isWeight)
{
    Preferences prefs;
    prefs.begin("tea_settings", false);
    prefs.putBool("disp_mode", isWeight);
    prefs.end();
}

void TeaManager::loadMinCupWeight()
{
    Preferences prefs;
    prefs.begin("tea_settings", true);
    minCupWeight = prefs.getFloat("cup_weight", DEFAULT_MIN_CUP_WEIGHT);
    prefs.end();
    DEBUG_PRINTF("[TeaManager] Loaded min cup weight from NVS: %.1fg\n", minCupWeight);
}

void TeaManager::saveMinCupWeight(float weight)
{
    Preferences prefs;
    prefs.begin("tea_settings", false);
    prefs.putFloat("cup_weight", weight);
    prefs.end();
    DEBUG_PRINTF("[TeaManager] Saved min cup weight to NVS: %.1fg\n", weight);
}

void TeaManager::loadTargetTotalMl()
{
    Preferences prefs;
    prefs.begin("tea_settings", true);
    targetTotalMl = prefs.getInt("target_ml", DEFAULT_TARGET_TOTAL_ML);
    prefs.end();
    DEBUG_PRINTF("[TeaManager] Loaded target total ml from NVS: %dml\n", targetTotalMl);
}

void TeaManager::saveTargetTotalMl(int ml)
{
    Preferences prefs;
    prefs.begin("tea_settings", false);
    prefs.putInt("target_ml", ml);
    prefs.end();
    DEBUG_PRINTF("[TeaManager] Saved target total ml to NVS: %dml\n", ml);
}

// ===== JSON Reporting =====

void TeaManager::sendTeaStartJSON()
{
    if (bleMgr)
    {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        target.toJson(obj);
        bleMgr->sendJSON(BleResponse::TEA_START, doc);
    }
}

void TeaManager::sendTeaProgressJSON()
{
    if (bleMgr)
    {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        dispensed.toJson(obj);
        obj["percentage"] = currentProgress;
        bleMgr->sendJSON(BleResponse::TEA_PROGRESS, doc);
    }
}

void TeaManager::sendTeaFinishedJSON()
{
    if (bleMgr)
    {
        JsonDocument emptyPayload;
        bleMgr->sendJSON(BleResponse::TEA_FINISH, emptyPayload);
    }
}

// after finished dispensing
void TeaManager::sendCupRemovedJSON()
{
    if (bleMgr)
    {
        JsonDocument emptyPayload;
        bleMgr->sendJSON(BleResponse::CUP_REMOVED, emptyPayload);
    }
}

// during dispensing
void TeaManager::sendCupRemovedWarningJSON()
{
    if (bleMgr)
    {
        JsonDocument emptyPayload;
        bleMgr->sendJSON(BleResponse::CUP_WARNING, emptyPayload);
    }
}
