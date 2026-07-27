#ifndef TEA_MANAGER_H
#define TEA_MANAGER_H

#include <ArduinoJson.h>
#include "Config.h"
#include "Models.h"
#include "WeightDispensingStrategy.h"
#include "TimeDispensingStrategy.h"
#include "CleaningMode.h"

// Forward declarations to avoid circular includes
class RelayManager;
class SensorManager;
class DisplayManager;
class ButtonManager;
class BluetoothManager;

class TeaManager
{
public:
    TeaManager();
    void begin(RelayManager *relayMgr, SensorManager *sensorMgr, DisplayManager *displayMgr,
               ButtonManager *buttonMgr, BluetoothManager *bleMgr);
    void update();

    void startMakingTea(const TeaRecipe &recipe);

    // Preview flow — shows recipe before brewing
    void previewRecipe(int btnIndex, const ButtonInfo &info);
    void confirmBrew();
    void cancelPreview();
    int getPreviewButtonIndex() const;

    // Emergency stop - delegates to RelayManager::stopAll()
    void stopAll();

    // Cleaning mode
    void startCleaningMode();
    void cancelCleaningMode();

    // Calibration mode
    void startCalibrationMode();
    void exitCalibrationMode();
    void resetCalibrationWeight();
    void toggleDispensingMode();
    bool isWeightMode() const;

    TeaState getState() const;
    uint8_t getProgress() const;

    // BLE connected screen overlay (3-second transient display)
    void showBleConnected();

private:
    RelayManager *relayMgr;
    SensorManager *sensorMgr;
    DisplayManager *displayMgr;
    ButtonManager *buttonMgr;
    BluetoothManager *bleMgr;

    TeaRecipe target;
    TeaRecipe dispensed;
    TeaState currentState;
    uint8_t currentProgress;
    unsigned long lastUpdate;
    float finishedWeight;
    bool weightDispensingMode;
    unsigned long bleConnectedUntil;

    void loadDispensingMode();
    void saveDispensingMode(bool isWeight);

    // --- Preview state ---
    ButtonInfo previewInfo;
    int previewButtonIndex;
    unsigned long previewStartTime;

    // --- Dispensing strategies ---
    WeightDispensingStrategy weightStrategy;
    TimeDispensingStrategy timeStrategy;
    DispensingStrategy *activeStrategy;

    // --- Cleaning mode ---
    CleaningMode cleaningMode;

    void finishDispensing();

    // JSON reporting
    void sendTeaStartJSON();
    void sendTeaProgressJSON();
    void sendTeaFinishedJSON();
    void sendCupRemovedJSON();
    void calculateProgress();
    void updateDisplay();
};

#endif