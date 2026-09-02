#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "Config.h"

typedef void (*OnWriteCallback)(const JsonDocument &doc);
typedef void (*OnConnectCallback)();

class BluetoothManager
{
public:
    BluetoothManager();
    void begin();
    void attachOnWriteCallback(OnWriteCallback onWriteCb);
    void attachOnConnectCallback(OnConnectCallback onConnectCb);
    void sendJSON(const String &type, const JsonDocument &payload);
    void sendError(const String &errorCode, const String &errorMessage);
    bool isConnected() const;

private:
    OnWriteCallback bleOnWriteCb;
    OnConnectCallback bleOnConnectCb;

    BLECharacteristic *pCharacteristic;
    BLEServer *pServer;

    bool isDeviceConnected;

    friend class ServerCallbacks;
    friend class CharacteristicCallbacks;
};

#endif