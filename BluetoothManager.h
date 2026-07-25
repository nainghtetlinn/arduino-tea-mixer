#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

typedef void (*OnWriteCallback)(const JsonDocument &doc);

class BluetoothManager
{
public:
    BluetoothManager();
    void begin(OnWriteCallback onWriteCb);
    void sendJSON(const String &type, const JsonDocument &payload);
    void sendError(const String &errorCode, const String &errorMessage);
    bool isConnected() const;

private:
    OnWriteCallback bleOnWriteCb;

    BLECharacteristic *pCharacteristic;
    BLEServer *pServer;

    bool isDeviceConnected;

    friend class ServerCallbacks;
    friend class CharacteristicCallbacks;
};

#endif