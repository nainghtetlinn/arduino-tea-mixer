#include "SensorManager.h"
#include <Arduino.h>

static float currentTemperature = 25.0f;
static float currentWeight = 0.0f;
static bool newDataAvailable = false;
static unsigned long lastSensorUpdate = 0;

#define SENSOR_UPDATE_INTERVAL 2000

void initSensors()
{
    // Initialize actual I2C, OneWire, HX711 etc here
    // e.g. sensors.begin();
    // scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
}

void updateSensors()
{
    if (millis() - lastSensorUpdate > SENSOR_UPDATE_INTERVAL)
    {
        lastSensorUpdate = millis();

        // TODO: Read actual values here
        // currentTemperature = sensors.getTempCByIndex(0);
        // currentWeight = scale.get_units();
        
        // Mock data for variation
        currentTemperature += random(-10, 11) / 10.0f;
        currentWeight += random(-5, 6) / 10.0f;

        newDataAvailable = true;
    }
}

float getTemperature()
{
    return currentTemperature;
}

float getWeight()
{
    return currentWeight;
}

bool isSensorDataNew()
{
    return newDataAvailable;
}

void clearSensorDataNewFlag()
{
    newDataAvailable = false;
}
