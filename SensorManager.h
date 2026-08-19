#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <HX711.h>
#include <OneWire.h>
#include <DallasTemperature.h>

class SensorManager
{
public:
    SensorManager();
    void begin();
    void update();

    float getTemperature() const;
    float getWeight() const;

    bool isDataNew() const;
    void clearDataNew();

    // --- Calibration API ---
    void startCalibrationZero();
    bool finishCalibrationWeight(float knownWeightGrams);
    float getCalibrationFactor() const;

private:
    OneWire oneWire;
    DallasTemperature sensors;
    HX711 scale;

    unsigned long lastSensorUpdate;
    bool newDataAvailable;
    float currentTemperature;
    float currentWeight;
    float activeCalibrationFactor;

    void loadSavedCalibrationFactor();
    void saveCalibrationFactor(float newFactor);
};

#endif
