#include "BluetoothManager.h"
#include "Config.h"

#define SERVICE_UUID "3b0947a7-1654-4b40-8f26-8a21169e054b"
#define CHARACTERISTIC_UUID "ede453c3-a6f3-42b4-9077-77dc68fd2f73"

class ServerCallbacks : public BLEServerCallbacks
{
    BluetoothManager &manager;

public:
    ServerCallbacks(BluetoothManager &mgr) : manager(mgr) {}
    void onConnect(BLEServer *pServerInst) override
    {
        manager.isDeviceConnected = true;
        DEBUG_PRINTLN("Client connected");
    }

    void onDisconnect(BLEServer *pServerInst) override
    {
        manager.isDeviceConnected = false;
        DEBUG_PRINTLN("Client disconnected");
        delay(100);
        pServerInst->getAdvertising()->start();
        DEBUG_PRINTLN("Advertising restarted");
    }
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks
{
    BluetoothManager &manager;

public:
    CharacteristicCallbacks(BluetoothManager &mgr) : manager(mgr) {}
    void onWrite(BLECharacteristic *pChar) override
    {
        String value = pChar->getValue();
        DEBUG_PRINTLN(value);

        JsonDocument doc;
        if (!deserializeJson(doc, value))
        {
            if (doc["type"].is<const char *>())
            {
                manager.bleOnWriteCb(doc);
            }
            else
                DEBUG_PRINTLN("Unknown data");
        }
    }
};

BluetoothManager::BluetoothManager()
    : bleOnWriteCb(nullptr),
      pCharacteristic(nullptr),
      pServer(nullptr),
      isDeviceConnected(false)
{
}

void BluetoothManager::begin(OnWriteCallback onWriteCb)
{
    bleOnWriteCb = onWriteCb;

    BLEDevice::init("TeaMixer");
    BLEDevice::setMTU(512);

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(*this));

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY);

    pCharacteristic->setCallbacks(new CharacteristicCallbacks(*this));
    pCharacteristic->addDescriptor(new BLE2902());

    pService->start();
    pServer->getAdvertising()->start();
}

void BluetoothManager::sendJSON(const String &type, const JsonDocument &payload)
{
    if (!isDeviceConnected || !pCharacteristic)
        return;

    JsonDocument doc;
    doc["type"] = type;

    if (!payload.isNull())
    {
        doc["payload"] = payload;
    }

    String output;
    serializeJson(doc, output);

    pCharacteristic->setValue(output.c_str());
    pCharacteristic->notify();

    DEBUG_PRINTLN("NOTIFY: " + type);
}

void BluetoothManager::sendError(const String &errorCode, const String &errorMessage)
{
    if (!isDeviceConnected || !pCharacteristic)
        return;

    JsonDocument doc;
    doc["type"] = "ERROR";
    JsonObject payload = doc["payload"].to<JsonObject>();
    payload["code"] = errorCode;
    payload["message"] = errorMessage;

    String output;
    serializeJson(doc, output);

    pCharacteristic->setValue(output.c_str());
    pCharacteristic->notify();

    DEBUG_PRINTLN("NOTIFY: " + errorCode);
}

bool BluetoothManager::isConnected() const
{
    return isDeviceConnected;
}
