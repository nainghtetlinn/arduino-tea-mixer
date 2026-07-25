#ifndef TEA_MANAGER_H
#define TEA_MANAGER_H

#include <ArduinoJson.h>
#include "Config.h"
#include "Models.h"

// Forward declarations to avoid circular includes
class RelayManager;
class SensorManager;
class DisplayManager;
class ButtonManager;

typedef void (*TeaSendJsonCallback)(const String &type, const JsonDocument &data);
typedef void (*TeaSendErrorCallback)(const String &errorCode, const String &errorMessage);

class TeaManager
{
public:
    TeaManager();
    void begin(RelayManager *relayMgr, SensorManager *sensorMgr, DisplayManager *displayMgr,
               ButtonManager *buttonMgr,
               TeaSendJsonCallback sendJsonCb, TeaSendErrorCallback sendErrorCb);
    void update();

    void startMakingTea(const TeaRecipe &recipe);

    // Preview flow — shows recipe before brewing
    void previewRecipe(const ButtonInfo &info);
    void confirmBrew();
    void cancelPreview();

    // Emergency stop - delegates to RelayManager::stopAll()
    void stopAll();

    TeaState getState() const;
    uint8_t getProgress() const;

private:
    RelayManager *relayMgr;
    SensorManager *sensorMgr;
    DisplayManager *displayMgr;
    ButtonManager *buttonMgr;

    TeaRecipe target;
    TeaRecipe dispensed;
    TeaState currentState;
    uint8_t currentProgress;
    unsigned long lastUpdate;
    unsigned long lastFinished;

    TeaSendJsonCallback teaSendJsonCb;
    TeaSendErrorCallback teaSendErrorCb;

    // --- Preview state ---
    ButtonInfo previewInfo;
    unsigned long previewStartTime;
    static constexpr unsigned long PREVIEW_TIMEOUT_MS = 10000; // 10 seconds

    // --- Weight-based mode state ---
    DispensingPhase currentPhase;
    float weightAtPhaseStart;
    unsigned long phaseStartTime; // for safety timeout

    // --- Time-based mode state ---
    unsigned long relayStartTime[RELAY_COUNT];
    unsigned long relayDuration[RELAY_COUNT]; // required duration in ms
    bool relayRunning[RELAY_COUNT];

    // Dispensing orchestration
    void startWeightMode();
    void startTimeMode();
    void updateWeightMode();
    void updateTimeMode();

    // Weight mode helpers
    void startPhase(DispensingPhase phase);
    void advancePhase();
    int getTargetMlForPhase(DispensingPhase phase) const;
    float getGramsPerMlForPhase(DispensingPhase phase) const;
    uint8_t getRelayIndexForPhase(DispensingPhase phase) const;
    float getFlowRateForIndex(uint8_t index) const;

    void finishDispensing();

    // JSON reporting
    void sendTeaStartJSON();
    void sendTeaProgressJSON();
    void sendTeaFinishedJSON();
    void calculateProgress();
    void updateDisplay();
};

#endif