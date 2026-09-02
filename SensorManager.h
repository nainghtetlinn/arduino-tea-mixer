#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <HX711.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include <cmath>
#include "Config.h"

class SensorManager
{
public:
    SensorManager();
    void begin();
    void update();

    float getTemperature() const;
    float getWeight() const;

    // --- Calibration API ---
    void zeroTare();
    bool finishCalibrationWeight(float knownWeightGrams);
    float getCalibrationFactor() const;

private:
    OneWire oneWire;
    DallasTemperature sensors;
    HX711 scale;

    unsigned long lastSensorUpdate;
    float currentTemperature;
    float currentWeight;
    float activeCalibrationFactor;

    void loadSavedCalibrationFactor();
    void saveCalibrationFactor(float newFactor);
};

#endif
