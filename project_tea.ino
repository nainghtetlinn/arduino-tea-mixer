#include "BluetoothManager.h"
#include "TeaManager.h"
#include "ButtonManager.h"
#include "Config.h"
#include "DisplayManager.h"

void setup()
{
    Serial.begin(115200);
    initDisplay();
    initButtons();
    initBluetooth();
    DEBUG_PRINTLN("Tea Mixer Ready");
}

void loop()
{
    updateButtons();
    updateTeaProgress();
    updateDisplay();

    // Keep delay small so buttons respond quickly
    delay(20);
}
