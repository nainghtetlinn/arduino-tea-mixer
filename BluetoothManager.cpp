#include "BluetoothManager.h"
#include "Config.h"

namespace
{
    constexpr char DEVICE_NAME[] = "TeaMixer";
    constexpr char SERVICE_UUID[] = "3b0947a7-1654-4b40-8f26-8a21169e054b";
    constexpr char CHARACTERISTIC_UUID[] = "ede453c3-a6f3-42b4-9077-77dc68fd2f73";
} // anonymous namespace

class ServerCallbacks : public BLEServerCallbacks
{
    BluetoothManager &manager;

public:
    ServerCallbacks(BluetoothManager &mgr) : manager(mgr) {}
    void onConnect(BLEServer *pServerInst) override
    {
        manager.isDeviceConnected = true;
        DEBUG_PRINTLN("[Bluetooth] Client connected");
        if (manager.bleOnConnectCb)
        {
            manager.bleOnConnectCb();
        }
    }

    void onDisconnect(BLEServer *pServerInst) override
    {
        manager.isDeviceConnected = false;
        DEBUG_PRINTLN("[Bluetooth] Client disconnected");
        delay(500);
        pServerInst->getAdvertising()->start();
        DEBUG_PRINTLN("[Bluetooth] Advertising restarted");
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
        DEBUG_PRINTLN("[Bluetooth] RECEIVED: " + value);

        JsonDocument doc;
        if (!deserializeJson(doc, value))
        {
            if (doc["type"].is<const char *>())
            {
                if (manager.bleOnWriteCb)
                {
                    manager.bleOnWriteCb(doc);
                }
            }
            else
                DEBUG_PRINTLN("[Bluetooth] Unknown data received");
        }
    }
};

BluetoothManager::BluetoothManager()
    : bleOnWriteCb(nullptr),
      bleOnConnectCb(nullptr),
      pCharacteristic(nullptr),
      pServer(nullptr),
      isDeviceConnected(false)
{
}

void BluetoothManager::begin()
{
    BLEDevice::init(DEVICE_NAME);
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

void BluetoothManager::attachOnWriteCallback(OnWriteCallback onWriteCb)
{
    bleOnWriteCb = onWriteCb;
}

void BluetoothManager::attachOnConnectCallback(OnConnectCallback onConnectCb)
{
    bleOnConnectCb = onConnectCb;
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

    DEBUG_PRINTLN("[Bluetooth] NOTIFY: " + output);
}

void BluetoothManager::sendError(const String &errorCode, const String &errorMessage)
{
    if (!isDeviceConnected || !pCharacteristic)
        return;

    JsonDocument doc;
    doc["type"] = BleResponse::ERROR;
    JsonObject payload = doc["payload"].to<JsonObject>();
    payload["code"] = errorCode;
    payload["message"] = errorMessage;

    String output;
    serializeJson(doc, output);

    pCharacteristic->setValue(output.c_str());
    pCharacteristic->notify();

    DEBUG_PRINTLN("[Bluetooth] NOTIFY: " + errorCode + "___" + errorMessage);
}

bool BluetoothManager::isConnected() const
{
    return isDeviceConnected;
}
