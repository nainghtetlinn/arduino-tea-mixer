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

    bluetoothManager.begin();
    bluetoothManager.attachOnWriteCallback(
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
            else if (type == "SET_CUP_WEIGHT")
            {
                JsonObjectConst payload = doc["payload"];
                float weight = payload["weight"] | -1.0f;
                if (weight >= 0.0f)
                {
                    teaManager.setMinCupWeight(weight);
                    JsonDocument resp;
                    resp["cupWeight"] = teaManager.getMinCupWeight();
                    bluetoothManager.sendJSON("CUP_WEIGHT", resp);
                }
                else
                {
                    bluetoothManager.sendError("SET_CUP_WEIGHT_ERROR", "Invalid weight value");
                }
            }
            else if (type == "GET_CUP_WEIGHT")
            {
                JsonDocument resp;
                resp["cupWeight"] = teaManager.getMinCupWeight();
                bluetoothManager.sendJSON("CUP_WEIGHT", resp);
            }
            else
                DEBUG_PRINTLN(F("Unknown Type"));
        });

    bluetoothManager.attachOnConnectCallback(
        []()
        {
            teaManager.showBleConnected();
        });

    buttonManager.begin();
    buttonManager.attachPressCallback(
        [](const int &btnIndex)
        {
            TeaState state = teaManager.getState();

            if (state == TeaState::CALIBRATION)
            {
                if (btnIndex == 0)
                {
                    // Btn 1 clicked while calibration mode is active: reset weight reading to 0
                    teaManager.resetCalibrationWeight();
                }
                else if (btnIndex == 1)
                {
                    // Btn 2 clicked while calibration mode is active: toggle dispensing mode (WEIGHT <-> TIME)
                    teaManager.toggleDispensingMode();
                }
            }
            else if (state == TeaState::CLEANING)
            {
                // Any button click while cleaning cancels cleaning mode immediately
                teaManager.cancelCleaningMode();
            }
            else if (state == TeaState::IDLE)
            {
                // First press: show recipe preview
                ButtonInfo info = buttonManager.getButtonInfo(btnIndex);
                teaManager.previewRecipe(btnIndex, info);
            }
            else if (state == TeaState::PREVIEW)
            {
                if (btnIndex == teaManager.getPreviewButtonIndex())
                {
                    // Same button pressed again: confirm and start brewing
                    teaManager.confirmBrew();
                }
                else
                {
                    // Different button: switch to its recipe preview
                    ButtonInfo info = buttonManager.getButtonInfo(btnIndex);
                    teaManager.previewRecipe(btnIndex, info);
                }
            }
            else if (state == TeaState::CUP_WARNING)
            {
                // Any button press dismisses the cup warning
                teaManager.dismissCupWarning();
            }
        });

    buttonManager.attachCleaningComboCallback(
        []()
        {
            if (teaManager.getState() == TeaState::IDLE)
            {
                teaManager.startCleaningMode();
            }
        });

    buttonManager.attachCalibrationComboCallback(
        []()
        {
            if (teaManager.getState() == TeaState::CALIBRATION)
            {
                teaManager.exitCalibrationMode();
            }
            else if (teaManager.getState() == TeaState::IDLE)
            {
                teaManager.startCalibrationMode();
            }
        });

    displayManager.begin();

    sensorManager.begin();

    relayManager.begin();

    // Initialize TeaManager with RelayManager, SensorManager, DisplayManager, ButtonManager, and BluetoothManager
    teaManager.begin(
        &relayManager,
        &sensorManager,
        &displayManager,
        &buttonManager,
        &bluetoothManager);

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
