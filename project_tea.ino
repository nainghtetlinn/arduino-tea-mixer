#include "BluetoothManager.h"
#include "ButtonManager.h"
#include "Config.h"
#include "DisplayManager.h"
#include "SensorManager.h"
#include "RelayManager.h"
#include "TeaManager.h"
#include "Models.h"

// Instantiate managers as global objects
DisplayManager displayManager;
SensorManager sensorManager;
BluetoothManager bluetoothManager;
ButtonManager buttonManager;
RelayManager relayManager;
TeaManager teaManager;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    bluetoothManager.begin(
        [](const JsonDocument &doc)
        {
            String type = doc["type"].as<String>();
            DEBUG_PRINTLN("[BluetoothManager] Handle: " + type);

            if (type == "MAKE_TEA")
            {
                JsonObjectConst payload = doc["payload"];
                TeaRecipe recipe;
                recipe.fromJson(payload);
                teaManager.startMakingTea(recipe);
            }
            else if (type == "GET_BUTTONS_INFO")
            {
                bluetoothManager.sendJSON("BUTTONS_INFO", buttonManager.getButtonsInfoJSON());
            }
            else if (type == "SET_BUTTON_INFO")
            {
                JsonObjectConst payload = doc["payload"];
                int id = payload["id"] | -1;
                ButtonInfo info;
                info.fromJson(payload);
                buttonManager.setButtonInfo(id, info);
                bluetoothManager.sendJSON("BUTTONS_INFO", buttonManager.getButtonsInfoJSON());
            }
            else
                DEBUG_PRINTLN(F("Unknown Type"));
        });

    buttonManager.begin(
        [](const int &btnIndex)
        {
            TeaState state = teaManager.getState();

            if (state == TeaState::IDLE)
            {
                // First press: show recipe preview
                ButtonInfo info = buttonManager.getButtonInfo(btnIndex);
                teaManager.previewRecipe(info);
            }
            else if (state == TeaState::PREVIEW)
            {
                // Second press while previewing: confirm and start brewing
                teaManager.confirmBrew();
            }
        });

    displayManager.begin();

    sensorManager.begin();

    relayManager.begin();

    // Initialize TeaManager with RelayManager, SensorManager, and callbacks
    teaManager.begin(
        &relayManager,
        &sensorManager,
        &displayManager,
        &buttonManager,
        [](const String &type, const JsonDocument &data)
        {
            bluetoothManager.sendJSON(type, data);
        },
        [](const String &errorCode, const String &errorMessage)
        {
            bluetoothManager.sendError(errorCode, errorMessage);
        });

    DEBUG_PRINTLN("Tea Mixer Ready");
}

void loop()
{
    buttonManager.update();
    sensorManager.update();
    teaManager.update();

    // Keep delay small so buttons respond quickly
    delay(20);
}
