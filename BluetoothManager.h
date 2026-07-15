#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <ArduinoJson.h>

void initBluetooth();

void sendJSON(String type, String data);
void sendError(String errorCode, String errorMessage);

#endif