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
    constexpr float CUP_REMOVED_THRESHOLD = 0.30f; // weight must drop below 30% of finished weight

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
      bleConnectedUntil(0),
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

    loadDispensingMode();
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
        if (cleaningMode.update(relayMgr, displayMgr, bleMgr))
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
            displayMgr->showCalibrationScreen(sensorMgr->getTemperature(), sensorMgr->getWeight(), weightDispensingMode);
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

    case TeaState::MAKING:
    {
        unsigned long now = millis();
        if (now - lastUpdate < DISPENSING_CHECK_INTERVAL_MS)
            return;

        lastUpdate = now;

        if (activeStrategy)
        {
            bool done = activeStrategy->update(dispensed, sensorMgr, relayMgr);
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

    target = recipe;
    dispensed = {0, 0, 0, 0};
    currentState = TeaState::MAKING;
    currentProgress = 0;
    lastUpdate = millis();

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
    activeStrategy->start(target, sensorMgr, relayMgr);

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
        activeStrategy->stop(relayMgr);
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
    cleaningMode.start(relayMgr, displayMgr, bleMgr);
}

void TeaManager::cancelCleaningMode()
{
    if (currentState != TeaState::CLEANING)
        return;

    cleaningMode.cancel(relayMgr, bleMgr);
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
        displayMgr->showCalibrationScreen(sensorMgr->getTemperature(), sensorMgr->getWeight(), weightDispensingMode);
    }
}

void TeaManager::exitCalibrationMode()
{
    if (currentState != TeaState::CALIBRATION)
        return;

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
    if (currentState != TeaState::CALIBRATION)
        return;

    weightDispensingMode = !weightDispensingMode;
    saveDispensingMode(weightDispensingMode);

    DEBUG_PRINTF("[TeaManager] Dispensing mode toggled to: %s\n",
                 weightDispensingMode ? "WEIGHT" : "TIME");

    if (sensorMgr && displayMgr)
    {
        displayMgr->showCalibrationScreen(sensorMgr->getTemperature(), sensorMgr->getWeight(), weightDispensingMode);
    }
}

bool TeaManager::isWeightMode() const
{
    return weightDispensingMode;
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

// ===== Display & UI Rendering =====

void TeaManager::showBleConnected()
{
    bleConnectedUntil = millis() + BLE_CONNECTED_DISPLAY_MS;
    if (displayMgr)
    {
        displayMgr->showBleConnectedScreen();
    }
    DEBUG_PRINTLN(F("[TeaManager] Showing BLE connected screen"));
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
            displayMgr->showCalibrationScreen(sensorMgr->getTemperature(), sensorMgr->getWeight(), weightDispensingMode);
        }
        break;
    default:
        break;
    }
}
