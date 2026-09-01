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
                buttonManager.sendButtonsInfoJSON();
            }
            else if (type == BleCommand::SET_BUTTON_INFO)
            {
                int id = payload["id"] | -1;
                ButtonInfo info;
                info.fromJson(payload);
                buttonManager.setButtonInfo(id, info);
                buttonManager.sendButtonsInfoJSON();
            }
            else if (type == BleCommand::GET_DEVICE_INFO)
            {
                teaManager.sendDeviceInfoJSON();
            }
            else if (type == BleCommand::GET_TEMPERATURE)
            {
                teaManager.sendTemperatureJSON();
            }
            else if (type == BleCommand::GET_WEIGHT)
            {
                teaManager.sendWeightJSON();
            }
            else if (type == BleCommand::SET_DISPENSING_MODE)
            {
                bool weightMode = payload["weightMode"] | teaManager.isWeightMode();
                teaManager.setDispensingMode(weightMode);
            }
            else if (type == BleCommand::SET_TARGET_TOTAL_ML)
            {
                int ml = payload["targetTotalMl"] | teaManager.getTargetTotalMl();
                teaManager.setTargetTotalMl(ml);
            }
            else if (type == BleCommand::START_CLEANING)
            {
                teaManager.startCleaningMode();
            }
            else if (type == BleCommand::CANCEL_CLEANING)
            {
                teaManager.cancelCleaningMode();
            }

            else
                DEBUG_PRINTLN(F("Unknown Type"));
        });

    bluetoothManager.attachOnConnectCallback(
        []()
        {
            teaManager.showBleConnected();
        });

    buttonManager.begin(&bluetoothManager);
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
                    if (teaManager.isCalibratingCupWeight())
                    {
                        teaManager.finishCupWeightCalibration();
                    }
                    else
                    {
                        teaManager.startCupWeightCalibration();
                    }
                }
                else if (btnIndex == 1)
                {
                    if (teaManager.isCalibratingCupWeight())
                    {
                        teaManager.cancelCupWeightCalibration();
                    }
                    else
                    {
                        // Btn 2: toggle dispensing mode (WEIGHT <-> TIME)
                        teaManager.toggleDispensingMode();
                    }
                }
                else if (btnIndex == 2)
                {
                    if (teaManager.isCalibratingCupWeight())
                    {
                        teaManager.cancelCupWeightCalibration();
                    }
                    else
                    {
                        // Btn 3: tare (zero) the weight sensor
                        teaManager.resetCalibrationWeight();
                    }
                }
            }
            else if (state == TeaState::CLEANING)
            {
                // Any button click while cleaning cancels cleaning mode immediately
                teaManager.cancelCleaningMode();
            }
            else if (state == TeaState::TARGET_ML)
            {
                if (btnIndex == 0)
                {
                    // B1: +10 ml
                    teaManager.incrementTargetMl();
                }
                else if (btnIndex == 1)
                {
                    // B2: -10 ml
                    teaManager.decrementTargetMl();
                }
                else if (btnIndex == 2)
                {
                    // B3: save to NVS + broadcast BLE + return to IDLE
                    teaManager.exitTargetMlMode();
                }
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
            else if (state == TeaState::WAITING_FOR_CUP)
            {
                // Any button press cancel preview
                teaManager.cancelPreview();
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

    buttonManager.attachTargetMlComboCallback(
        []()
        {
            if (teaManager.getState() == TeaState::IDLE)
            {
                teaManager.startTargetMlMode();
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
