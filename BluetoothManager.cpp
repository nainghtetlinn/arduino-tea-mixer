#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "BluetoothManager.h"
#include "ButtonManager.h"
#include "TeaManager.h"
#include "Config.h"

#define SERVICE_UUID "3b0947a7-1654-4b40-8f26-8a21169e054b"
#define CHARACTERISTIC_UUID "ede453c3-a6f3-42b4-9077-77dc68fd2f73"

BLECharacteristic *pCharacteristic;
static BLEServer *pServer;

static bool isDeviceConnected = false;

class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pChar)
    {
        String value = pChar->getValue();
        DEBUG_PRINTLN(value);

        JsonDocument doc;
        if (!deserializeJson(doc, value))
        {
            if (doc["type"].is<const char *>())
            {
                String type = doc["type"].as<String>();
                if (type == "MAKE_TEA")
                    handleMakeTeaCommand(doc);

                else if (type == "GET_BUTTONS_INFO")
                    handleGetButtonsInfoCommand();

                else if (type == "SET_BUTTON_INFO")
                    handleSetButtonInfoCommand(doc);

                else
                    DEBUG_PRINTLN("Unknown Type");
            }
            else
                DEBUG_PRINTLN("Unknown data");
        }
    }
};

class ServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServerInst)
    {
        isDeviceConnected = true;
        DEBUG_PRINTLN("Client connected");
    }

    void onDisconnect(BLEServer *pServerInst)
    {
        isDeviceConnected = false;
        DEBUG_PRINTLN("Client disconnected");
        delay(100);
        pServerInst->getAdvertising()->start();
        DEBUG_PRINTLN("Advertising restarted");
    }
};

void initBluetooth()
{
    BLEDevice::init("TeaMixer");
    BLEDevice::setMTU(512);

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY);

    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());

    pService->start();
    pServer->getAdvertising()->start();
}

void sendJSON(String type, String data)
{
    if (!isDeviceConnected)
        return;

    pCharacteristic->setValue(data.c_str());
    pCharacteristic->notify();

    DEBUG_PRINTLN("NOTIFY: " + type);
}

void sendError(String errorCode, String errorMessage)
{
    if (!isDeviceConnected)
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
