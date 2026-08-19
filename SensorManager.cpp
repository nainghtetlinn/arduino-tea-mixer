#include "SensorManager.h"
#include "Config.h"
#include <Preferences.h>
#include <cmath>

namespace
{
    // --- Pins ---
    constexpr uint8_t PIN_HX711_DT = 14;
    constexpr uint8_t PIN_HX711_CLK = 13;
    constexpr uint8_t PIN_ONE_WIRE = 4;

    // Parameters
    constexpr unsigned long SENSOR_UPDATE_INTERVAL_MS = 500;

    // Default
    constexpr float DEFAULT_CALIBRATION_FACTOR = 582.5f;
} // anonymous namespace

SensorManager::SensorManager()
    : oneWire(PIN_ONE_WIRE),
      sensors(&oneWire),
      lastSensorUpdate(0),
      newDataAvailable(false),
      currentTemperature(0.0f),
      currentWeight(0.0f),
      activeCalibrationFactor(DEFAULT_CALIBRATION_FACTOR)
{
}

void SensorManager::begin()
{
    loadSavedCalibrationFactor();

    sensors.begin();

    scale.begin(PIN_HX711_DT, PIN_HX711_CLK);
    scale.set_scale(activeCalibrationFactor);
    scale.tare();

    DEBUG_PRINT(F("[SensorManager] Initialized with calibration factor: "));
    DEBUG_PRINTLN(activeCalibrationFactor);
}

void SensorManager::update()
{
    unsigned long currentMillis = millis();

    if (currentMillis - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL_MS)
    {
        lastSensorUpdate = currentMillis;

        sensors.requestTemperatures();
        currentTemperature = sensors.getTempCByIndex(0);

        currentWeight = scale.get_units();

        newDataAvailable = true;
    }
}

void SensorManager::startCalibrationZero()
{
    scale.tare(10);
    DEBUG_PRINTLN(F("[SensorManager] Zero tare completed."));
}

bool SensorManager::finishCalibrationWeight(float knownWeightGrams)
{
    if (knownWeightGrams <= 0.0f)
    {
        DEBUG_PRINTLN(F("[SensorManager] Error: Known weight must be > 0"));
        return false;
    }

    float rawUnits = scale.get_value(10);
    float newFactor = rawUnits / knownWeightGrams;

    scale.set_scale(newFactor);
    saveCalibrationFactor(newFactor);

    DEBUG_PRINTF("[SensorManager] Calibrated successfully. New factor: %.2f\n", newFactor);
    return true;
}

float SensorManager::getCalibrationFactor() const
{
    return activeCalibrationFactor;
}

float SensorManager::getTemperature() const
{
    return currentTemperature;
}

float SensorManager::getWeight() const
{
    return currentWeight;
}

bool SensorManager::isDataNew() const
{
    return newDataAvailable;
}

void SensorManager::clearDataNew()
{
    newDataAvailable = false;
}

void SensorManager::loadSavedCalibrationFactor()
{
    Preferences prefs;
    prefs.begin("tea_settings", true);
    activeCalibrationFactor = prefs.getFloat("cal_factor", DEFAULT_CALIBRATION_FACTOR);
    prefs.end();

    if (std::isnan(activeCalibrationFactor) || activeCalibrationFactor <= 0.0f)
    {
        activeCalibrationFactor = DEFAULT_CALIBRATION_FACTOR;
    }
}

void SensorManager::saveCalibrationFactor(float newFactor)
{
    activeCalibrationFactor = newFactor;

    Preferences prefs;
    prefs.begin("tea_settings", false);
    prefs.putFloat("cal_factor", newFactor);
    prefs.end();
}