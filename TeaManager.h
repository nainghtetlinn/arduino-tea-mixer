#ifndef TEA_MANAGER_H
#define TEA_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "Config.h"
#include "Models.h"
#include "BluetoothManager.h"
#include "ButtonManager.h"
#include "DisplayManager.h"
#include "RelayManager.h"
#include "SensorManager.h"
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
    void dismissCupWarning();

    void previewRecipe(int btnIndex, const ButtonInfo &info);
    void confirmBrew();
    void cancelPreview();

    // Cleaning mode
    void startCleaningMode();
    void cancelCleaningMode();

    // Calibration mode
    void startCalibrationMode();
    void exitCalibrationMode();
    void resetCalibrationWeight();
    void toggleDispensingMode();
    void setDispensingMode(bool weightMode);

    // Cup weight calibration (within calibration mode)
    void startCupWeightCalibration();
    void finishCupWeightCalibration();
    void cancelCupWeightCalibration();

    // Target total ml (NVS-configurable)
    void setTargetTotalMl(int ml);

    // Target ML adjustment mode (B2+B3 combo)
    void startTargetMlMode();
    void exitTargetMlMode();
    void incrementTargetMl();
    void decrementTargetMl();

    // getters
    int getPreviewButtonIndex() const;
    bool isWeightMode() const;
    bool isCalibratingCupWeight() const;
    float getMinCupWeight() const;
    int getTargetTotalMl() const;
    TeaState getState() const;

    // BLE connected screen overlay (3-second transient display)
    void showBleConnected();

    void sendDeviceInfoJSON();
    void sendTemperatureJSON();
    void sendWeightJSON();

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
    bool calibratingCup;
    unsigned long bleConnectedStartTime;

    // Cup presence detection
    float minCupWeight;
    unsigned long cupWarningStartTime;
    int targetTotalMl; // target total ml; recipe is scaled to this before dispensing

    // WAITING_FOR_CUP state
    TeaRecipe pendingRecipe;       // recipe to start once cup is placed
    unsigned long cupDetectedTime; // timestamp when cup was first detected (for 1 s delay)

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

    // Helpers
    void finishDispensing();
    void calculateProgress();
    // Emergency stop - delegates to RelayManager::stopAll()
    // void stopAll();
    // uint8_t getProgress() const;

    // NVS persistence
    void loadDispensingMode();
    void saveDispensingMode(bool isWeight);
    void loadMinCupWeight();
    void saveMinCupWeight(float weight);
    void loadTargetTotalMl();
    void saveTargetTotalMl(int ml);

    // JSON reporting

    void sendTeaStartJSON();
    void sendTeaProgressJSON();
    void sendTeaFinishedJSON();
    void sendCupRemovedJSON();
    void sendCupRemovedWarningJSON();
};

#endif