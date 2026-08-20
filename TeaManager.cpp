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
    constexpr unsigned long PREVIEW_TIMEOUT_MS = 7000;
    constexpr unsigned long BLE_CONNECTED_DISPLAY_MS = 3000;
    constexpr float CUP_REMOVED_THRESHOLD = 0.30f;         // weight must drop below 30% of finished weight
    constexpr unsigned long CUP_WARNING_TIMEOUT_MS = 5000; // auto-dismiss warning after 5s
    constexpr float DEFAULT_MIN_CUP_WEIGHT = 50.0f;        // default minimum cup weight in grams

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
      bleConnectedUntil(0),
      minCupWeight(DEFAULT_MIN_CUP_WEIGHT),
      weightBeforeDispensing(0.0f),
      cupWarningIsCupRemoved(false),
      cupWarningStartTime(0),
      previewButtonIndex(-1),
      previewStartTime(0),
      activeStrategy(nullptr)
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
}

// ===== Main Update Loop =====

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

    case TeaState::CUP_WARNING:
    {
        // Auto-dismiss after timeout
        if (millis() - cupWarningStartTime >= CUP_WARNING_TIMEOUT_MS)
        {
            dismissCupWarning();
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

                    cupWarningIsCupRemoved = true;
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

            if (done)
            {
                finishDispensing();
            }
        }
        break;
    }

    default:
        break;
    }

    updateDisplay();
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
            DEBUG_PRINTF("[TeaManager] No cup detected (weight: %.1fg, min: %.1fg)\n",
                         currentWeight, minCupWeight);

            cupWarningIsCupRemoved = false;
            cupWarningStartTime = millis();
            currentState = TeaState::CUP_WARNING;

            if (displayMgr)
            {
                displayMgr->showNoCupWarningScreen();
            }

            if (bleMgr)
            {
                bleMgr->sendError("NO_CUP", "No cup detected on the scale");
            }
            return;
        }
    }

    target = recipe;
    dispensed = {0, 0, 0, 0};
    currentState = TeaState::MAKING;
    currentProgress = 0;
    lastUpdate = millis();

    // Record weight baseline for mid-dispense cup removal detection
    weightBeforeDispensing = sensorMgr ? sensorMgr->getWeight() : 0.0f;

    DEBUG_PRINTF("[TeaManager] Starting dispensing (mode: %s)\n",
                 weightDispensingMode ? "WEIGHT" : "TIME");
    DEBUG_PRINTF("[TeaManager] Target: tea=%d, cm=%d, em=%d, milk=%d\n",
                 target.tea, target.condensedMilk, target.evaporatedMilk, target.milk);

    if (weightDispensingMode)
    {
        activeStrategy = &weightStrategy;
    }
    else
    {
        activeStrategy = &timeStrategy;
    }
    activeStrategy->start(target);

    sendTeaStartJSON();
}

void TeaManager::finishDispensing()
{
    activeStrategy = nullptr;

    // Turn off all relays (safety net)
    if (relayMgr)
    {
        relayMgr->stopAll();
    }

    currentState = TeaState::FINISHED;
    currentProgress = 100;
    finishedWeight = sensorMgr ? sensorMgr->getWeight() : 0.0f;

    DEBUG_PRINTLN(F("[TeaManager] Dispensing complete"));

    sendTeaFinishedJSON();
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

void TeaManager::stopAll()
{
    if (activeStrategy)
    {
        activeStrategy->stop();
        activeStrategy = nullptr;
    }
    else if (relayMgr)
    {
        relayMgr->stopAll();
    }

    currentState = TeaState::IDLE;
    currentProgress = 0;

    DEBUG_PRINTLN(F("[TeaManager] Emergency stop"));
}

// ===== Recipe Preview Flow =====

void TeaManager::previewRecipe(int btnIndex, const ButtonInfo &info)
{
    previewInfo = info;
    previewButtonIndex = btnIndex;
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

    DEBUG_PRINTLN(F("[TeaManager] Confirmed — starting brew"));
    previewButtonIndex = -1;
    startMakingTea(previewInfo.recipe);
}

void TeaManager::cancelPreview()
{
    if (currentState != TeaState::PREVIEW)
        return;

    currentState = TeaState::IDLE;
    previewButtonIndex = -1;
    DEBUG_PRINTLN(F("[TeaManager] Preview cancelled"));
}

int TeaManager::getPreviewButtonIndex() const
{
    return previewButtonIndex;
}

// ===== Cleaning Mode =====

void TeaManager::startCleaningMode()
{
    if (currentState != TeaState::IDLE)
    {
        DEBUG_PRINTLN(F("[TeaManager] Cannot enter Cleaning mode: state is not IDLE"));
        return;
    }

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
    {
        DEBUG_PRINTLN(F("[TeaManager] Cannot enter Calibration mode: state is not IDLE"));
        return;
    }

    currentState = TeaState::CALIBRATION;
    DEBUG_PRINTLN(F("[TeaManager] Calibration mode started"));

    if (sensorMgr && displayMgr)
    {
        displayMgr->showCalibrationScreen(sensorMgr->getTemperature(), sensorMgr->getWeight(), weightDispensingMode, minCupWeight, calibratingCup);
    }
}

void TeaManager::exitCalibrationMode()
{
    if (currentState != TeaState::CALIBRATION)
        return;

    calibratingCup = false;
    currentState = TeaState::IDLE;
    DEBUG_PRINTLN(F("[TeaManager] Exited Calibration mode"));
}

void TeaManager::resetCalibrationWeight()
{
    if (currentState != TeaState::CALIBRATION)
        return;

    if (sensorMgr)
    {
        sensorMgr->startCalibrationZero();
        DEBUG_PRINTLN(F("[TeaManager] Weight cell zeroed in calibration mode"));
    }
}

void TeaManager::toggleDispensingMode()
{
    // Block during active dispensing or cleaning
    if (currentState == TeaState::MAKING || currentState == TeaState::CLEANING)
        return;

    weightDispensingMode = !weightDispensingMode;
    saveDispensingMode(weightDispensingMode);

    DEBUG_PRINTF("[TeaManager] Dispensing mode toggled to: %s\n",
                 weightDispensingMode ? "WEIGHT" : "TIME");

    // Refresh calibration screen if currently in calibration mode
    if (currentState == TeaState::CALIBRATION && sensorMgr && displayMgr)
    {
        displayMgr->showCalibrationScreen(sensorMgr->getTemperature(), sensorMgr->getWeight(), weightDispensingMode, minCupWeight, calibratingCup);
    }
}

bool TeaManager::isWeightMode() const
{
    return weightDispensingMode;
}

// ===== Cup Weight Calibration =====

void TeaManager::startCupWeightCalibration()
{
    if (currentState != TeaState::CALIBRATION)
        return;

    calibratingCup = true;
    DEBUG_PRINTLN(F("[TeaManager] Cup weight calibration started — place cup on scale"));
}

void TeaManager::saveCupWeight()
{
    if (currentState != TeaState::CALIBRATION || !calibratingCup)
        return;

    if (sensorMgr)
    {
        float currentWeight = sensorMgr->getWeight();
        if (currentWeight > 0.0f)
        {
            setMinCupWeight(currentWeight);
            DEBUG_PRINTF("[TeaManager] Cup weight saved: %.1fg\n", currentWeight);
        }
        else
        {
            DEBUG_PRINTLN(F("[TeaManager] Cup weight not saved — weight is zero or negative"));
        }
    }

    calibratingCup = false;
}

bool TeaManager::isCalibrating() const
{
    return calibratingCup;
}

// ===== State Queries =====

TeaState TeaManager::getState() const
{
    return currentState;
}

uint8_t TeaManager::getProgress() const
{
    return currentProgress;
}

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
    DEBUG_PRINTF("[TeaManager] Saved dispensing mode to NVS: %s\n",
                 isWeight ? "WEIGHT" : "TIME");
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

float TeaManager::getMinCupWeight() const
{
    return minCupWeight;
}

void TeaManager::setMinCupWeight(float weight)
{
    if (weight < 0.0f)
        weight = 0.0f;
    minCupWeight = weight;
    saveMinCupWeight(weight);
}

void TeaManager::dismissCupWarning()
{
    if (currentState != TeaState::CUP_WARNING)
        return;

    currentState = TeaState::IDLE;
    currentProgress = 0;
    DEBUG_PRINTLN(F("[TeaManager] Cup warning dismissed"));
}

// ===== JSON Reporting =====

void TeaManager::sendTeaStartJSON()
{
    if (bleMgr)
    {
        JsonDocument doc;
        JsonObject obj = doc.to<JsonObject>();
        target.toJson(obj);
        bleMgr->sendJSON("TEA_START", doc);
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
        bleMgr->sendJSON("TEA_PROGRESS", doc);
    }
}

void TeaManager::sendTeaFinishedJSON()
{
    if (bleMgr)
    {
        JsonDocument emptyPayload;
        bleMgr->sendJSON("TEA_FINISH", emptyPayload);
    }
}

void TeaManager::sendCupRemovedJSON()
{
    if (bleMgr)
    {
        JsonDocument emptyPayload;
        bleMgr->sendJSON("CUP_REMOVED", emptyPayload);
    }
}

void TeaManager::sendCupRemovedWarningJSON()
{
    if (bleMgr)
    {
        JsonDocument doc;
        doc["reason"] = "CUP_REMOVED_DURING_DISPENSING";
        bleMgr->sendJSON("CUP_WARNING", doc);
    }
}

// ===== Display & UI Rendering =====

void TeaManager::showBleConnected()
{
    bleConnectedUntil = millis() + BLE_CONNECTED_DISPLAY_MS;
    if (displayMgr)
    {
        displayMgr->showBleConnectedScreen();
    }
}

void TeaManager::updateDisplay()
{
    if (!displayMgr)
        return;

    // BLE connected overlay takes priority for its duration
    if (bleConnectedUntil > 0)
    {
        if (millis() < bleConnectedUntil)
        {
            displayMgr->showBleConnectedScreen();
            return;
        }
        bleConnectedUntil = 0;
    }

    switch (currentState)
    {
    case TeaState::IDLE:
    {
        String n0 = buttonMgr ? buttonMgr->getButtonName(0) : F("---");
        String n1 = buttonMgr ? buttonMgr->getButtonName(1) : F("---");
        String n2 = buttonMgr ? buttonMgr->getButtonName(2) : F("---");
        bool ble = bleMgr ? bleMgr->isConnected() : false;
        displayMgr->showIdleScreen(n0, n1, n2, ble);
        break;
    }
    case TeaState::PREVIEW:
        displayMgr->showRecipePreviewScreen(previewInfo.name, previewInfo.recipe);
        break;
    case TeaState::MAKING:
        displayMgr->showTeaProgressScreen(currentProgress);
        break;
    case TeaState::FINISHED:
        displayMgr->showTeaReadyScreen();
        break;
    case TeaState::CLEANING:
        // Handled dynamically in CleaningMode::update()
        break;
    case TeaState::CALIBRATION:
        if (sensorMgr)
        {
            displayMgr->showCalibrationScreen(sensorMgr->getTemperature(), sensorMgr->getWeight(), weightDispensingMode, minCupWeight, calibratingCup);
        }
        break;
    case TeaState::CUP_WARNING:
        if (cupWarningIsCupRemoved)
        {
            displayMgr->showCupRemovedWarningScreen();
        }
        else
        {
            displayMgr->showNoCupWarningScreen();
        }
        break;
    default:
        break;
    }
}
