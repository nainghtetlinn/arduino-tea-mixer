#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <ArduinoJson.h>
#include <OneButton.h>
#include "Config.h"
#include "Models.h"
#include "BluetoothManager.h"

typedef void (*ButtonPressCallback)(const int &btnIndex);
typedef void (*ButtonComboCallback)();

// Forward declarations to avoid circular includes
class BluetoothManager;

class ButtonManager
{
public:
    ButtonManager();
    void begin(BluetoothManager *bleMgr);
    void attachPressCallback(ButtonPressCallback pressCb);
    void attachCleaningComboCallback(ButtonComboCallback comboCallback);
    void attachCalibrationComboCallback(ButtonComboCallback comboCallback);
    void attachTargetMlComboCallback(ButtonComboCallback comboCallback);
    void update();

    void setButtonInfo(int index, ButtonInfo info);
    ButtonInfo getButtonInfo(int index);
    String getButtonName(int index);
    void sendButtonsInfoJSON();

private:
    BluetoothManager *bleMgr;
    OneButton buttons[3];
    ButtonPressCallback btnPressCb;
    ButtonComboCallback combo13Cb;
    ButtonComboCallback combo12Cb;
    ButtonComboCallback combo23Cb;

    unsigned long combo13HoldStartTime;
    bool combo13Triggered;

    unsigned long combo12HoldStartTime;
    bool combo12Triggered;

    unsigned long combo23HoldStartTime;
    bool combo23Triggered;

    bool suppressClick[3];

    void makeNvsKey(char *buffer, size_t bufferSize, int index, const char *field);
    void handleButtonClick(int btnIndex);
    void updateCombo(bool aPinHeld, bool bPinHeld, bool thirdPinHeld,
                     int suppressA, int suppressB,
                     unsigned long &holdStart, bool &triggered,
                     ButtonComboCallback cb,
                     bool canClearA, bool canClearB);
};

#endif
