#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

void initSensors();
void updateSensors();
float getTemperature();
float getWeight();
bool isSensorDataNew();
void clearSensorDataNewFlag();

#endif
