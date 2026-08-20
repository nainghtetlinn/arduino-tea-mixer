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

            JsonObjectConst payload = doc["payload"].as<JsonObjectConst>();

            if (type == BleCommand::MAKE_TEA)
            {
                TeaRecipe recipe;
                recipe.fromJson(payload);
                teaManager.startMakingTea(recipe);
            }
            else if (type == BleCommand::GET_BUTTONS_INFO)
            {
                bluetoothManager.sendJSON(BleResponse::BUTTONS_INFO, buttonManager.getButtonsInfoJSON());
            }
            else if (type == BleCommand::SET_BUTTON_INFO)
            {
                int id = payload["id"] | -1;
                ButtonInfo info;
                info.fromJson(payload);
                buttonManager.setButtonInfo(id, info);
                bluetoothManager.sendJSON(BleResponse::BUTTONS_INFO, buttonManager.getButtonsInfoJSON());
            }
            else if (type == BleCommand::SET_DISPENSING_MODE)
            {
                bool weightMode = payload["weightMode"] | teaManager.isWeightMode();
                teaManager.setDispensingMode(weightMode);
                JsonDocument resp;
                resp["weightMode"] = teaManager.isWeightMode();
                bluetoothManager.sendJSON(BleResponse::DISPENSING_MODE, resp);
            }
            else if (type == BleCommand::GET_DEVICE_INFO)
            {
                JsonDocument resp;
                resp["temperature"] = sensorManager.getTemperature();
                resp["weight"] = sensorManager.getWeight();
                resp["calibrationFactor"] = sensorManager.getCalibrationFactor();
                resp["cupWeight"] = teaManager.getMinCupWeight();
                resp["weightMode"] = teaManager.isWeightMode();
                bluetoothManager.sendJSON(BleResponse::DEVICE_INFO, resp);
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
                    // Btn 1: toggle cup weight calibration
                    // First press enters "place cup" mode, second press saves the weight
                    if (teaManager.isCalibrating())
                    {
                        teaManager.saveCupWeight();
                    }
                    else
                    {
                        teaManager.startCupWeightCalibration();
                    }
                }
                else if (btnIndex == 1)
                {
                    // Btn 2: toggle dispensing mode (WEIGHT <-> TIME)
                    teaManager.toggleDispensingMode();
                }
                else if (btnIndex == 2)
                {
                    // Btn 3: tare (zero) the weight sensor
                    teaManager.resetCalibrationWeight();
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
