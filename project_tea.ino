#include "BluetoothManager.h"
#include "ButtonManager.h"
#include "Config.h"
#include "DisplayManager.h"
#include "SensorManager.h"
#include "TeaManager.h"
#include "Models.h"

// Instantiate managers as global objects
DisplayManager displayManager;
SensorManager sensorManager;
BluetoothManager bluetoothManager;
ButtonManager buttonManager;
TeaManager teaManager;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // Wire managers together (Mediator Pattern)
    bluetoothManager.begin(
        [](const JsonDocument &doc)
        {
            String type = doc["type"].as<String>();
            if (type == "MAKE_TEA")
            {
                teaManager.handleMakeTeaCommand(doc);
            }
            else if (type == "GET_BUTTONS_INFO")
            {
                buttonManager.handleGetButtonsInfoCommand();
            }
            else if (type == "SET_BUTTON_INFO")
            {
                buttonManager.handleSetButtonInfoCommand(doc);
            }
            else
                DEBUG_PRINTLN("Unknown Type");
        });

    teaManager.begin(
        [](const String &type, const JsonDocument &data)
        {
            bluetoothManager.sendJSON(type, data);
        },
        [](const String &errorCode, const String &errorMessage)
        {
            bluetoothManager.sendError(errorCode, errorMessage);
        });

    buttonManager.begin(
        [](const TeaRecipe &recipe)
        {
            teaManager.startMakingTea(recipe);
        },
        [](const String &type, const JsonDocument &data)
        {
            bluetoothManager.sendJSON(type, data);
        });

    displayManager.begin();
    sensorManager.begin();

    DEBUG_PRINTLN("Tea Mixer Ready");
}

void loop()
{
    buttonManager.update();
    sensorManager.update();
    teaManager.update();

    TeaState state = teaManager.getState();

    if (state == TeaState::IDLE)
    {
        displayManager.showIdle(sensorManager.getTemperature(), sensorManager.getWeight());
    }
    else if (state == TeaState::FINISHED)
    {
        displayManager.showReady();
    }
    else if (state == TeaState::MAKING)
    {
        displayManager.showProgress(teaManager.getProgress());
    }

    // Keep delay small so buttons respond quickly
    delay(20);
}
